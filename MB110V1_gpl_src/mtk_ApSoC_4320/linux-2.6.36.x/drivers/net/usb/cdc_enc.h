/*
 * Copyright (c) 2012  Bj?rn Mork <bjorn@...k.no>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#ifndef _CDC_ENC_H_
#define _CDC_ENC_H_

#include <linux/cdev.h>
#include <linux/list.h>
#include <linux/usb.h>

#define CDC_ENC_BUFLEN 512
#define CDC_ENC_MAX_MINOR 16
#define PARM_STR_LEN 30
enum cdc_enc_client_flag {
	CDC_ENC_CLIENT_BUSY = 0,	/* RX buffer is in use */
	CDC_ENC_CLIENT_TX,		/* TX buffer is in use */
	CDC_ENC_CLIENT_RX,		/* new data available */
	CDC_ENC_CLIENT_SHUTDOWN,	/* shutdown requested */
};

/* per client data */
struct cdc_enc_client {
	unsigned char buf[CDC_ENC_BUFLEN];	/* rx buffer */
	unsigned char tx_buf[CDC_ENC_BUFLEN];	/* tx buffer */
	size_t len;			/* length of data in rx buffer */
	unsigned long flags;
	struct cdc_enc_state *cdc_enc;	/* CDC_ENC instance owning the client */
	struct completion ready;
	struct list_head list;		/* linux/list.h pointers */
	struct work_struct work;	/* task to call when new data is recvd */

	unsigned char apnName[PARM_STR_LEN];	/* apn name buffer */
	unsigned char devName[PARM_STR_LEN];	/* dev name buffer */
	unsigned char  authType;
	
	unsigned long priv;		/* client specific data */
};

/* per CDC_ENC interface state */
struct cdc_enc_state {
	struct usb_interface *intf;
	struct urb *urb;		/* receive urb */
	struct urb *interrupt;		/* interrupt urb */
	unsigned char rcvbuf[CDC_ENC_BUFLEN];     /* receive buffer */
	struct usb_ctrlrequest setup;	/* the receive setup - 8 bytes */
	struct list_head clients;	/* list of clients - first entry is wwan client */
	struct mutex clients_lock;
	struct cdev cdev;		/* registered character device */
	const char *protocol;
	wait_queue_head_t waitq;	/* so we can wait for all clients on exit */
};

/* submit the read URB immediately - can be called in interrupt context */
extern int cdc_enc_submit_readurb(struct cdc_enc_state *cdc_enc, int kmalloc_flags);

/* kill the read URB */
extern void cdc_enc_kill_readurb(struct cdc_enc_state *cdc_enc);

/* call this to enable cdc_enc to submit the interrupt URB when it needs to */
extern int cdc_enc_set_interrupt(struct cdc_enc_state *cdc_enc, struct urb *urb);

/* initialize */
extern struct cdc_enc_state *cdc_enc_init_one(struct usb_interface *intf, const char *protocol);

/* clean up */
extern int cdc_enc_free_one(struct cdc_enc_state *cdc_enc);

/* send a synchronous message from client */
extern int cdc_enc_send_sync(struct cdc_enc_client *client, unsigned char *msg, size_t len);

/* get a new client */
extern struct cdc_enc_client *cdc_enc_add_client(struct cdc_enc_state *cdc_enc, work_func_t recv_callback);

/* destroy client */
extern void cdc_enc_destroy_client(struct cdc_enc_client *client);

#endif /* _CDC_ENC_H_ */