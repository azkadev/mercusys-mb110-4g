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

#ifndef _QMI_PROTO_H_
#define _QMI_PROTO_H_

#include "cdc_enc.h"

/* the different QMI subsystems */
#define	QMI_CTL	0x00
#define	QMI_WDS 0x01

/* Error codes */
#define QMI_ERR_MALFORMED_MSG 0x0001

/* QMI protocol structures */
struct qmux_header {
	u8 tf;		/* always 1 */
	__le16 len;	/* excluding tf */
	u8 ctrl;	/* b7: sendertype 1 => service, 0 => control point */
	u8 service;	/* 0 => QMI_CTL, 1 => QMI_WDS, .. */
	u8 qmicid;	/* client id or 0xff for broadcast */
	u8 flags;	/* always 0 for req */
} __packed;

struct qmi_msg {
	__le16 msgid;
	__le16 len;
	u8 tlv[];	/* zero or more tlvs */
} __packed;

struct qmi_ctl {
	struct qmux_header h;
	u8 tid;	/* system QMI_CTL uses one byte transaction ids! */
	struct qmi_msg m;
} __packed;

struct qmi_wds {
	struct qmux_header h;
	__le16 tid;
	struct qmi_msg m;
} __packed;

struct qmi_tlv {
	u8 type;
	__le16 len;
	u8 bytes[];
} __packed;

struct qmi_tlv_response_data {
	__le16 error;
	__le16 code;
} __packed;

/* for QMI_WDS state tracking */
struct qmi_state {
	u8 wds_handle[4];	/* connection handle */
	u8 ctl_tid;		/* for matching up QMI_CTL replies */
	u8 wds_cid;		/* allocated QMI_WDS client ID */
	u8 wds_status;		/* result of the last QMI_WDS_GET_PKT_SRVC_STATUS message */
	u8 flags;
} __packed;

#define QMI_STATE_FLAG_START	0x01	/* START in progress */
#define QMI_STATE_FLAG_CIDERR	0x02	/* client ID allocation failure */
#define QMI_STATE_FLAG_NOTFIRST	0x04	/* have been connected before */

/* parsing the buffered QMUX message */
extern int qmi_parse_qmux(struct cdc_enc_client *client);

/* reset state variables */
extern void qmi_state_reset(struct cdc_enc_client *client);

/* QMI_CTL commands */
extern int qmi_ctl_request_wds_cid(struct cdc_enc_client *client);
extern int qmi_ctl_release_wds_cid(struct cdc_enc_client *client);

/* QMI_WDS commands */
extern int qmi_wds_start(struct cdc_enc_client *client);
extern int qmi_wds_stop(struct cdc_enc_client *client);
extern int qmi_wds_status(struct cdc_enc_client *client);

#endif /* _QMI_PROTO_H_ */