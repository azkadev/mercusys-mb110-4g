/*
 * Copyright (c) 2012  Bj?rn Mork <bjorn@xxxxxxx>
 * Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

/*
 * Dealing with devices using Qualcomm MSM Interface (QMI) for
 * configuration.  Full documentation of the protocol can be obtained
 * from http://developer.qualcomm.com/
 *
 * Some of this code may be inspired by code from the Qualcomm Gobi
 * 2000 and Gobi 3000 drivers, available at http://www.codeaurora.org/
 */

#include "cdc_enc.h"
#include "qmi_proto.h"

//extern void kernel_debug_callback(char *str, ...); /* added by hejian, 120913 */

#define FUNTIONCALL() printk("%s  %d call\n", __FUNCTION__, __LINE__);
 
void printBuf(char *buf, int len, char *str)
{	
	FUNTIONCALL();
	char *p = buf;
	int i = 0;

	printk("%s\n", str);

	while(i < len)
	{
		printk("%2x ",(int)(unsigned char)*p);
		p++;i++;
	}
	printk("\nend %s\n", str);
}
static void qmi_cpu_to_le16(u8 system, u8 *buf, size_t len)
{
	FUNTIONCALL();
	struct qmi_ctl *ctl = (void *) buf;
	struct qmi_wds *wds = (void *) buf;
	struct qmux_header *h= (void *) buf;
	struct qmi_msg *m = NULL;
	__le16 value = 0;
	__le16 mlen = 0;
	__le16 tlen = 0;
	u8 *p;
	char *mbuf = NULL;
	//printBuf(buf, len, "before fix up send data.");
	//fix up header len value.
	h->len = cpu_to_le16((value = h->len));

	//fix up wds tid value.
	if (QMI_CTL == system)
	{
		m = &ctl->m; 
	}
	else if (QMI_WDS == system)
	{
		wds->tid = cpu_to_le16((value = wds->tid));
		m = &wds->m;
	}
	else
	{
		return;
	}
	//fix up qmi_msg len value.
	m->msgid = cpu_to_le16((value = m->msgid));
	m->len = cpu_to_le16((mlen = m->len));
	mbuf = &m->tlv[0];
    if (m->len <= 0) 
	{
		//printBuf(buf, len, "after fix up send data.");
		
		return;
    }
	//fix up qmi_tlv len value.
	struct qmi_tlv *t;
	for (p = mbuf; p < mbuf + mlen; p += tlen + sizeof(struct qmi_tlv))
	{
		t = (struct qmi_tlv *)p;
		t->len = cpu_to_le16((tlen = t->len));		
	}
}
static void qmi_le16_to_cpu(u8 *buf, size_t len)
{
	FUNTIONCALL();
	struct qmi_ctl *ctl = (void *) buf;
	struct qmi_wds *wds = (void *) buf;
	struct qmux_header *h= (void *) buf;
	struct qmi_msg *m = NULL;
	__le16 value = 0;
	__le16 mlen = 0;
	__le16 tlen = 0;
	u8 *p;
	char *mbuf = NULL;

	
	if (len < sizeof(struct qmux_header) || h->tf != 0x01 )
	{
		return;
	}
		
	//fix up header len value.
	h->len = le16_to_cpu((value = h->len));
	if (h->len != (len - 1))
	{
		return;
	}

	//fix up wds tid value.
	if (QMI_CTL == h->service)
	{
		m = &ctl->m; 
	}
	else if (QMI_WDS == h->service)
	{
		wds->tid = le16_to_cpu((value = wds->tid));
		m = &wds->m;
	}
	else
	{
		return;
	}
	//fix up qmi_msg len value.
	m->msgid = cpu_to_le16((value = m->msgid));
	m->len = le16_to_cpu((mlen = m->len));
	mbuf = &m->tlv[0];
    if (m->len <= 0) 
	{
		printBuf(buf, len, "after fix up send data.");
		return;
    }
	//fix up qmi_tlv len value.
	struct qmi_tlv *t;
	for (p = mbuf; p < mbuf + m->len; p += t->len + sizeof(struct qmi_tlv)) {
		t = (struct qmi_tlv *)p;
		t->len = le16_to_cpu((tlen = t->len));
	}
}

/* find and return a pointer to the requested tlv */
static struct qmi_tlv *qmi_get_tlv(u8 type, u8 *buf, size_t len)
{
	FUNTIONCALL();
	u8 *p;
	struct qmi_tlv *t;

	for (p = buf; p < buf + len; p += t->len + sizeof(struct qmi_tlv)) {
		t = (struct qmi_tlv *)p;
		if (t->type == type)
			return (p + t->len <= buf + len) ? t : NULL;
	}
	return NULL;
}

/* TLV 0x02 is status: return a negative QMI error code or 0 if OK */
static int qmi_verify_status_tlv(u8 *buf, size_t len)
{
	FUNTIONCALL();
	struct qmi_tlv *tlv = qmi_get_tlv(0x02, buf, len);
	struct qmi_tlv_response_data *r = (void *)tlv->bytes;

	/* OK: indications and requests do not have any status TLV */
	if (!tlv)
		return 0;

	if (tlv->len != sizeof(struct qmi_tlv_response_data))
		return -QMI_ERR_MALFORMED_MSG;

	return r->error ? -r->code : 0;
}

/* verify QMUX message header and return pointer to the (first) message if OK */
static struct qmi_msg *qmi_qmux_verify(u8 *data, size_t len)
{
	FUNTIONCALL();
	struct qmux_header *h =  (void *)data;
	struct qmi_msg *m = NULL;

	if (len < sizeof(struct qmux_header) || /* short packet */
		h->tf != 0x01 || h->len != (len - 1)) /* invalid QMUX packet! */
		goto err;

	/* tid has a different size for QMI_CTL for some fucking stupid reason */
	if (h->service == QMI_CTL) {
		struct qmi_ctl *ctl = (void *)data;
		if (len >= sizeof(struct qmi_ctl) +  ctl->m.len)
			m = &ctl->m;
	} else {
		struct qmi_wds *wds = (void *)data;
		if (len >= sizeof(struct qmi_wds) +  wds->m.len)
			m = &wds->m;
	}

err:
	return m;
}

/* parse a QMI_CTL replies */
static int qmi_ctl_parse(struct cdc_enc_client *client, struct qmi_msg *m)
{
	FUNTIONCALL();
	struct qmi_state *state = (void *)&client->priv;
	struct qmi_tlv *tlv;
	int status;

	/* check and save status TLV */
	status = qmi_verify_status_tlv(m->tlv, m->len);

	/* clear the saved transaction id */
	state->ctl_tid = 0;

	switch (m->msgid) {
	case 0x0022: /* CTL_GET_CLIENT_ID_RESP */
		/* there's no way out of this I think... */
		if (status < 0) {
			dev_err(&client->cdc_enc->intf->dev,
				"Failed to get QMI_WDS client id %#06x\n", -status);
			state->flags |= QMI_STATE_FLAG_CIDERR;
			break;
		}

		/* TLV 0x01 is a 2 byte system + client ID */
		tlv = qmi_get_tlv(0x01, m->tlv, m->len);
		if (tlv && tlv->len == 2) {
			if (tlv->bytes[0] == QMI_WDS)
				state->wds_cid = tlv->bytes[1];
				
				//qmi_wds_start(client);
		} else {
			status = -QMI_ERR_MALFORMED_MSG;
		}
		break;
	}
	return status;
}

static int qmi_send_msg(struct cdc_enc_client *client, u8 system, __le16 msgid, struct qmi_tlv *tlv, __le16 tlvNum);

/* parse a QMI_WDS indications looking connection status updates */
static int qmi_wds_parse_ind(struct cdc_enc_client *client, struct qmi_msg *m)
{
	FUNTIONCALL();
	struct qmi_state *state = (void *)&client->priv;
	struct qmi_tlv *tlv;
	int status = 0;

	switch (m->msgid) {
	case 0x0022: /* QMI_WDS_PKT_SRVC_STATUS_IND */

		/* TLV 0x01 is a 2 byte connection status. Note the
		 * difference from replies: Indications use the second
		 * byte for a "reconfiguration" flag. We ignore that..
		 */
		tlv = qmi_get_tlv(0x01, m->tlv, m->len);
		if (tlv && tlv->len == 2) {
			dev_dbg(&client->cdc_enc->intf->dev,
				"connstate %#04x => %#04x (flags %04x)\n",
				state->wds_status, tlv->bytes[0], state->flags);

			state->wds_status = tlv->bytes[0];

			/* this is a bit weird, but it seems that the
			 * firmware *requires* us to request address
			 * configuration before it will forward any
			 * packets. DHCP does this just fine for us on
			 * initial connect, but reconnecting will
			 * cause us to blackhole packets unless we
			 * send this QMI request.
			 *
			 * BUT: If we send this after the initial connection,
			 * before the DHCP client has done it's job, then
			 * we end up with a device sending ethernet header-
			 * less frames.  Go figure.  Can you spell buggy
			 * frimware?
			 *
			 * So we attemt to send this on reconnects only
			 *
			 * QMI_WDS msg 0x002d is "QMI_WDS_GET_RUNTIME_SETTINGS"
			 */
			if (state->wds_status == 2) {
				if (state->flags & QMI_STATE_FLAG_NOTFIRST)
					qmi_send_msg(client, QMI_WDS, 0x002d, NULL, 0);
				else
					state->flags |= QMI_STATE_FLAG_NOTFIRST;
			}
		} else {
			status = -QMI_ERR_MALFORMED_MSG;
		}
		break;
	}
	return status;
}


/* parse a QMI_WDS replies looking connection status updates */
static int qmi_wds_parse(struct cdc_enc_client *client, struct qmi_msg *m)
{
	FUNTIONCALL();
	struct qmi_state *state = (void *)&client->priv;
	struct qmi_tlv *tlv;
	int status;

	/* check and save status TLV */
	status = qmi_verify_status_tlv(m->tlv, m->len);

	/* note that we continue with per message processing on
	 * errors, to be able to clear wait states etc.
	 */
	switch (m->msgid) {
	case 0x0020: /* QMI_WDS_START_NETWORK_INTERFACE_RESP */
		/* got a reply - clear flag */
		state->flags &= ~QMI_STATE_FLAG_START;
		if (status < 0) {
			__le32 reason = -1;

			/* TLV 0x11 is a 4 byte call end reason type */
			tlv = qmi_get_tlv(0x11, m->tlv, m->len);
			if (tlv)
				reason = *(__le32 *)tlv->bytes;

			dev_info(&client->cdc_enc->intf->dev,
				"Connection failed with status %#06x and reason %#010x\n",
				-status, reason);
			break;
		}

		/* TLV 0x01 is a 4 byte connection handle */
		tlv = qmi_get_tlv(0x01, m->tlv, m->len);
		if (tlv && tlv->len == sizeof(state->wds_handle))
			memcpy(state->wds_handle, tlv->bytes, sizeof(state->wds_handle));
		else
			status = -QMI_ERR_MALFORMED_MSG;
		break;
	case 0x0022: /* QMI_WDS_GET_PKT_SRVC_STATUS_RESP */
		if (status < 0)
			break;

		/* TLV 0x01 is a 1 byte connection status */
		tlv = qmi_get_tlv(0x01, m->tlv, m->len);
		if (tlv && tlv->len == 1)
			state->wds_status = tlv->bytes[0];
		else
			status = -QMI_ERR_MALFORMED_MSG;
		break;
	case 0x002d: /* might as well parse this then... */
		if (status < 0)
			break;

		/* TLV 0x14 is APN name */
		tlv = qmi_get_tlv(0x14, m->tlv, m->len);
		if (!tlv)
			break;

		/* we "know" the buffer has space for this */
		tlv->bytes[tlv->len] = 0;
		dev_info(&client->cdc_enc->intf->dev,
			"Connected to APN \"%s\"\n", tlv->bytes);

		break;
	}
	return status;
}

/* parse the buffered message and update the saved QMI state if it's for us */
int qmi_parse_qmux(struct cdc_enc_client *client)
{
	FUNTIONCALL();
	struct qmi_ctl *qmux;
	struct qmi_msg *m;
	int ret = -1;
	struct qmi_state *state = (void *)&client->priv;

	/* any unread data available? */
	if (!test_bit(CDC_ENC_CLIENT_RX, &client->flags))
		return 0;

	/* someone else is using the buffer... */
	if (test_and_set_bit(CDC_ENC_CLIENT_BUSY, &client->flags))
		return -1;

	qmi_le16_to_cpu(client->buf, client->len);
	
	m = qmi_qmux_verify(client->buf, client->len);
	if (!m)
		goto done;

	/* we only use the common header for QMI_WDS so this will work */
	qmux = (struct qmi_ctl *)client->buf;

	/* verify that the message is for our eyes */
	if (qmux->h.ctrl != 0x80) /* must be "service" */
		goto done;

	switch (qmux->h.service) {
	case QMI_CTL:
		/* only for us if the transaction ID matches */
		if (state->ctl_tid == qmux->tid)
			ret = qmi_ctl_parse(client, m);
		break;
	case QMI_WDS:
		/* for us if it's a broadcast or the client ID matches */
		if (qmux->h.qmicid == 0xff || qmux->h.qmicid == state->wds_cid) {
			switch (qmux->h.flags) {
			case 0x02: /* reply */
				ret = qmi_wds_parse(client, m);
				break;
			case 0x04: /* unsolicited indication */
				ret = qmi_wds_parse_ind(client, m);
				break;
			}
		}
		break;
	}

done:
	/* order is important! */
	clear_bit(CDC_ENC_CLIENT_RX, &client->flags);
	clear_bit(CDC_ENC_CLIENT_BUSY, &client->flags);
	return ret;
}

/* Get a new transaction id - yeah, yeah, atomic operations blah, blah, blah
 * Fact is that we don't really need to care much about collisions, so this
 * will do
 */
static __le16 new_tid(void)
{
	FUNTIONCALL();
	static __le16 tid;
	return ++tid;
}

/* assemble a QMI_WDS packet */
static size_t qmi_create_wds_msg(struct cdc_enc_client *client, __le16 msgid, struct qmi_tlv *tlv, __le16 tlvNum)
{
	FUNTIONCALL();
	struct qmi_wds *wds = (void *)client->tx_buf;
	struct qmi_state *state = (void *)&client->priv;

	/* cannot send QMI_WDS requests without a client ID */
	if (!state->wds_cid)
		return 0;

	memset(wds, 0, sizeof(*wds));
	wds->h.tf = 1;     /* always 1 */
	wds->h.service = QMI_WDS;
	wds->h.qmicid = state->wds_cid;
	wds->h.len = sizeof(*wds) - 1;
	wds->tid = new_tid();
	wds->m.msgid = msgid;
	if (tlvNum == 0)
	{
	
	}
	else if (tlvNum == 1)
	{
	if (tlv) {
		ssize_t tlvsize = tlv->len + sizeof(struct qmi_tlv);
		memcpy(wds->m.tlv, tlv, tlvsize);
		wds->m.len = tlvsize;
		wds->h.len += tlvsize;
	}
	}
	else if (tlvNum > 1)
	{
		int i =0;
		char *offset = NULL;
		ssize_t alltlvsize = 0;
		
		for (i = 0; i < tlvNum; i++)
		{
			ssize_t tlvsize = tlv->len + sizeof(struct qmi_tlv);
			offset = (char *)(wds->m.tlv + alltlvsize);
			memcpy(offset, tlv, tlvsize);
			wds->m.len += tlvsize;
			wds->h.len += tlvsize;
			tlv =(struct qmi_tlv *)((char *)tlv + tlvsize);
			alltlvsize += tlvsize;			
		}
	}


	return wds->h.len + 1;
}

/* assemble a QMI_CTL packet */
static size_t qmi_create_ctl_msg(struct cdc_enc_client *client, __le16 msgid, struct qmi_tlv *tlv)
{
	FUNTIONCALL();
	struct qmi_ctl *ctl = (void *)client->tx_buf;
	struct qmi_state *state = (void *)&client->priv;

	/* only allowing one outstanding CTL request */
	if (state->ctl_tid)
		return 0;

	memset(ctl, 0, sizeof(*ctl));
	ctl->h.tf = 1;     /* always 1 */
	ctl->h.len = sizeof(*ctl) - 1;
	while (ctl->tid == 0 || ctl->tid == 0xff) /* illegal values */
		ctl->tid = new_tid() & 0xff;

	/* save allocated transaction ID for reply matching */
	state->ctl_tid = ctl->tid;

	ctl->m.msgid = msgid;
	if (tlv) {
		ssize_t tlvsize = tlv->len + sizeof(struct qmi_tlv);
		memcpy(ctl->m.tlv, tlv, tlvsize);
		ctl->m.len = tlvsize;
		ctl->h.len += tlvsize;
	}
	return ctl->h.len + 1;

}

/* send a QMI message syncronously */
static int qmi_send_msg(struct cdc_enc_client *client, u8 system, __le16 msgid, struct qmi_tlv *tlv, __le16 tlvNum)
{
	int ret = -1;
	size_t len = 0;

	/* lock client buffer */
	if (test_and_set_bit(CDC_ENC_CLIENT_TX, &client->flags))
		goto err_noclear;

	/* create message */
	switch (system) {
	case QMI_CTL:
		len = qmi_create_ctl_msg(client, msgid, tlv);
		
		break;
	case QMI_WDS:
		len = qmi_create_wds_msg(client, msgid, tlv, tlvNum);
		 
		break;
	}
	
	if (!len)
		goto err;
	
	qmi_cpu_to_le16(system, client->tx_buf, len);
	
	/* send it */
	ret = cdc_enc_send_sync(client, client->tx_buf, len) - len;

err:
	clear_bit(CDC_ENC_CLIENT_TX, &client->flags);
err_noclear:
	return ret;
}

/* QMI_CTL msg 0x0022 is "request cid", TLV 0x01 is system */
int qmi_ctl_request_wds_cid(struct cdc_enc_client *client)
{
	printk("%s %d", __FUNCTION__, __LINE__);
	FUNTIONCALL();
	static struct qmi_tlv tlvreq = {
		.type = 0x01,
		.len = 1,
		.bytes = { QMI_WDS },
	};
	struct qmi_state *state = (void *)&client->priv;

	/* return immediately if a CID is already allocated */
	if (state->wds_cid)
		return 0;

	int ret = qmi_send_msg(client, QMI_CTL, 0x0022, &tlvreq, 1);
	int i = 0;
	while(i < 10000 * 100) 
	{//wait for some time.
		if (state->wds_cid)
		{
			break;
		}
		i++;
	}
	qmi_wds_start(client);
	return ret;
}

/* QMI_CTL msg 0x0023 is "release cid", TLV 0x01 is system + cid */
int qmi_ctl_release_wds_cid(struct cdc_enc_client *client)
{
	FUNTIONCALL();
	static struct qmi_tlv tlvreq = {
		.type = 0x01,
		.len = 2,
		.bytes = { QMI_WDS, 0 },
	};
	struct qmi_state *state = (void *)&client->priv;


	/* return immediately if no CID is allocated */
	if (!state->wds_cid)
		return 0;

	tlvreq.bytes[1] = state->wds_cid;
	state->wds_cid = 0;

	return qmi_send_msg(client, QMI_CTL, 0x0023, &tlvreq, 1);
}

/* QMI_WDS msg 0x0020 is "QMI_WDS_START_NETWORK_INTERFACE" */
int qmi_wds_start(struct cdc_enc_client *client)
{
	FUNTIONCALL();
	struct qmi_state *state = (void *)&client->priv;
	if (state->wds_cid == 0) return;
	/* avoid sending multiple start messages on top of each other */
	if (state->flags & QMI_STATE_FLAG_START)
		return 0;

	state->flags |= QMI_STATE_FLAG_START;
	FUNTIONCALL();

	if (strcmp(client->apnName, "default") != 0)
	{
		#define BUF_LEN 100
		char buf[BUF_LEN];
		struct qmi_tlv *tlvreq =&buf[0];

		memset(buf, 0, BUF_LEN);
		tlvreq->type = (u8) 0x14;
		tlvreq->len = (__le16) strlen(client->apnName);
		memcpy(tlvreq->bytes, client->apnName, strlen(client->apnName));

		ssize_t tlvsize = tlvreq->len + sizeof(struct qmi_tlv);
		
		tlvreq =&buf[tlvsize];
		tlvreq->type = (u8) 0x16;
		tlvreq->len = (__le16) 1;
		memcpy(tlvreq->bytes, &client->authType, 1);
		
		return qmi_send_msg(client, QMI_WDS, 0x0020, &buf[0], 2);
	}else
	{
		return qmi_send_msg(client, QMI_WDS, 0x0020, NULL, 0);
	}
	
}

/* QMI_WDS msg 0x0021 is "QMI_WDS_STOP_NETWORK_INTERFACE" */
int qmi_wds_stop(struct cdc_enc_client *client)
{
	FUNTIONCALL();
	static struct qmi_tlv tlvreq = {
		.type = 0x01,
		.len = 4,
		.bytes = { 0, 0, 0, 0 },
	};
	struct qmi_state *state = (void *)&client->priv;

	/* cannot send stop unless we have a handle */
	if (!*(u32 *)state->wds_handle)
		return 0;

	memcpy(&tlvreq.bytes, state->wds_handle, sizeof(state->wds_handle));
	return qmi_send_msg(client, QMI_WDS, 0x0021, &tlvreq, 1);
}

/* QMI_WDS msg 0x0022 is "QMI_WDS_GET_PKT_SRVC_STATUS" */
int qmi_wds_status(struct cdc_enc_client *client)
{
	return 0; 
	FUNTIONCALL();
	return qmi_send_msg(client, QMI_WDS, 0x0022, NULL, 0);
}

/* reset everything but the allocated QMI_WDS client ID */
void qmi_state_reset(struct cdc_enc_client *client)
{
	FUNTIONCALL();
	struct qmi_state *state = (void *)&client->priv;

	memset(state->wds_handle, 0, sizeof(state->wds_handle));
	state->ctl_tid = 0;
	state->wds_status = 0;
	state->flags = 0;
}
