/*
 * Copyright (c) 2012  Bj?rn Mork <bjorn@...k.no>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/completion.h>
#include <linux/usb.h>
#include <linux/usb/cdc.h>
#include "cdc_enc.h"

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

/* copy data to the client local buffer */
static void cdc_enc_copy_to_client(struct cdc_enc_client *client, void *data, size_t len)
{
	FUNTIONCALL();
	/* someone is using the RX buffer - ignore data */
	if (test_and_set_bit(CDC_ENC_CLIENT_BUSY, &client->flags))
		return;
FUNTIONCALL();
	/* refusing to overwrite unread data */
	if (test_and_set_bit(CDC_ENC_CLIENT_RX, &client->flags))
		goto err_oflow;
FUNTIONCALL();
	memcpy(client->buf, data, len);
	client->len = len;
FUNTIONCALL();
err_oflow:
	clear_bit(CDC_ENC_CLIENT_BUSY, &client->flags);
	schedule_work(&client->work);
}

/* copy data to all clients */
static void cdc_enc_copy_to_all(struct cdc_enc_state *cdc_enc, void *data, size_t len)
{
	FUNTIONCALL();
	struct cdc_enc_client *tmp;

	list_for_each_entry(tmp, &cdc_enc->clients, list)
		cdc_enc_copy_to_client(tmp, data, len);
}

/* URB callback, copying common recv buffer to all clients */
static void cdc_enc_read_callback(struct urb *urb)
{
	FUNTIONCALL();
	/* silently ignoring any errors */
	printk("eeeeeeeeeeeeeeee rb->status %d urb->actual_length %d \n", urb->status, urb->actual_length);
	printBuf(urb->transfer_buffer,0x0f, "urb->transfer_buffer");
	if (urb->status == 0 && urb->actual_length > 0)
		cdc_enc_copy_to_all((struct cdc_enc_state *)urb->context, urb->transfer_buffer, urb->actual_length);
}

/* make sure the interrupt URB is queued by forcibly resubmitting it */
static void cdc_enc_status_urb(struct cdc_enc_state *cdc_enc, int kmalloc_flags)
{
	FUNTIONCALL();
	if (cdc_enc->interrupt)
		usb_submit_urb(cdc_enc->interrupt, kmalloc_flags);
}

/* submit a command */
int cdc_enc_send_sync(struct cdc_enc_client *client, unsigned char *msg, size_t len)
{
	FUNTIONCALL();
	int status;
	struct cdc_enc_state *cdc_enc = client->cdc_enc;
	struct usb_device *udev = interface_to_usbdev(cdc_enc->intf);

	/* enable status even if networking is down */
	cdc_enc_status_urb(cdc_enc, GFP_KERNEL);

	status = usb_control_msg(udev,
				usb_sndctrlpipe(udev, 0),
				USB_CDC_SEND_ENCAPSULATED_COMMAND, USB_DIR_OUT|USB_TYPE_CLASS|USB_RECIP_INTERFACE,
				0, cdc_enc->intf->cur_altsetting->desc.bInterfaceNumber,
				msg, len, 1000);
	FUNTIONCALL();
	/* copy successfully transmitted data to all clients, including ourselves */
	if (status == len)
	{
		cdc_enc_copy_to_all(cdc_enc, msg, len);FUNTIONCALL();FUNTIONCALL();FUNTIONCALL();
	}else
	{
		FUNTIONCALL();FUNTIONCALL();FUNTIONCALL();
	}
	return status;
}
EXPORT_SYMBOL_GPL(cdc_enc_send_sync);

struct cdc_enc_client *cdc_enc_add_client(struct cdc_enc_state *cdc_enc, work_func_t recv_callback)
{
	FUNTIONCALL();
	struct cdc_enc_client *client;

	client = kzalloc(sizeof(struct cdc_enc_client), GFP_KERNEL);
	if (!client)
		goto done;

	client->cdc_enc = cdc_enc;
	init_completion(&client->ready);

	/* setup callback */
	INIT_WORK(&client->work, recv_callback);

	mutex_lock(&cdc_enc->clients_lock);
	list_add(&client->list, &cdc_enc->clients);
	mutex_unlock(&cdc_enc->clients_lock);

done:
	return client;
}
EXPORT_SYMBOL_GPL(cdc_enc_add_client);

void cdc_enc_destroy_client(struct cdc_enc_client *client)
{
	FUNTIONCALL();
	struct cdc_enc_state *cdc_enc = client->cdc_enc;

	mutex_lock(&cdc_enc->clients_lock);
	cancel_work_sync(&client->work);
	list_del(&client->list);
	kfree(client);
	mutex_unlock(&cdc_enc->clients_lock);

	wake_up(&cdc_enc->waitq);
}
EXPORT_SYMBOL_GPL(cdc_enc_destroy_client);

static ssize_t cdc_enc_fops_read(struct file *file, char __user *buf, size_t size, loff_t *pos)
{
	FUNTIONCALL();
	struct cdc_enc_client *client = file->private_data;
	int ret = -EFAULT;

	if (!client || !client->cdc_enc)
		return -ENODEV;

	while (!test_bit(CDC_ENC_CLIENT_RX, &client->flags)) {/* no data */
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

		if (wait_for_completion_interruptible(&client->ready) < 0)
			return -EINTR;

		/* shutdown requested? */
		if (test_bit(CDC_ENC_CLIENT_SHUTDOWN, &client->flags))
			return -ENXIO;
	}

	/* someone else is using our buffer */
	if (test_and_set_bit(CDC_ENC_CLIENT_BUSY, &client->flags))
		return -ERESTARTSYS;

	/* must read a complete packet */
	if (client->len > size || copy_to_user(buf, client->buf, client->len)) {
		ret = -EFAULT;
		goto err;
	}
	ret = client->len;

err:
	/* order is important! */
	clear_bit(CDC_ENC_CLIENT_RX, &client->flags);
	clear_bit(CDC_ENC_CLIENT_BUSY, &client->flags);

	return ret;
}

static ssize_t cdc_enc_fops_write(struct file *file, const char __user *buf, size_t size, loff_t *pos)
{
	FUNTIONCALL();
	struct cdc_enc_client *client = file->private_data;
	int ret = -EFAULT;

	if (!client || !client->cdc_enc)
		return -ENODEV;

	/* shutdown requested? */
	if (test_bit(CDC_ENC_CLIENT_SHUTDOWN, &client->flags))
		return -ENXIO;

	/* no meaning in attempting to send an incomplete packet */
	if (size > sizeof(client->tx_buf))
		return -EFAULT;

	/* are someone else using our buffer? */
	if (test_and_set_bit(CDC_ENC_CLIENT_TX, &client->flags))
		return -ERESTARTSYS;

	if (size > sizeof(client->tx_buf) || copy_from_user(client->tx_buf, buf, size))
		goto err;

	/* send to the device */
	ret = cdc_enc_send_sync(client, client->tx_buf, size);
	if (ret < 0)
		return -EFAULT;

err:
	clear_bit(CDC_ENC_CLIENT_TX, &client->flags);
	return ret;
}

/* receive callback for character device */
static void cdc_enc_newdata_rcvd(struct work_struct *work)
{
	FUNTIONCALL();
	struct cdc_enc_client *client = container_of(work, struct cdc_enc_client, work);

	/* signal new data available to any waiting reader */
	complete(&client->ready);
}

static int cdc_enc_fops_open(struct inode *inode, struct file *file)
{
	FUNTIONCALL();
	struct cdc_enc_state *cdc_enc;
	struct cdc_enc_client *client;

	/* associate the file with our backing CDC_ENC device */
	cdc_enc = container_of(inode->i_cdev, struct cdc_enc_state, cdev);
	if (!cdc_enc)
		return -ENODEV;

	/* don't allow interface to sleep while we are using it */
	usb_autopm_get_interface(cdc_enc->intf);

	/* enable status URB even if networking is down */
	cdc_enc_status_urb(cdc_enc, GFP_KERNEL);

	/* set up a ring buffer to receive our readable data? */
	client = cdc_enc_add_client(cdc_enc, cdc_enc_newdata_rcvd);

	if (!client)
		return -ENOMEM;

	file->private_data = client;
	return 0;
}

static int cdc_enc_fops_release(struct inode *inode, struct file *file)
{
	FUNTIONCALL();
	struct cdc_enc_client *client = file->private_data;
	struct cdc_enc_state *cdc_enc = client->cdc_enc;

	if (!client || !cdc_enc)
		return -ENODEV;

	/* allow interface to sleep again */
	usb_autopm_put_interface(cdc_enc->intf);

	cdc_enc_destroy_client(client);
	file->private_data = NULL;
	return 0;
}

static const struct file_operations cdc_enc_fops = {
	.owner   = THIS_MODULE,
	.read    = cdc_enc_fops_read,
	.write   = cdc_enc_fops_write,
	.open    = cdc_enc_fops_open,
	.release = cdc_enc_fops_release,
	//.llseek  = noop_llseek,
};

/* submit read URB */
int cdc_enc_submit_readurb(struct cdc_enc_state *cdc_enc, int kmalloc_flags)
{
	FUNTIONCALL();
	printBuf(&cdc_enc->setup,8, "cdc_enc_submit_readurb cdc_enc->setup value");
	return usb_submit_urb(cdc_enc->urb, kmalloc_flags);

}
EXPORT_SYMBOL_GPL(cdc_enc_submit_readurb);

/* kill the read URB */
void cdc_enc_kill_readurb(struct cdc_enc_state *cdc_enc)
{
	FUNTIONCALL();
	usb_kill_urb(cdc_enc->urb);
}
EXPORT_SYMBOL_GPL(cdc_enc_kill_readurb);

/* call this to enable interrupt processing when netif is down */
int cdc_enc_set_interrupt(struct cdc_enc_state *cdc_enc, struct urb *urb)
{
	FUNTIONCALL();
	/* save URB once */
	if (urb && !cdc_enc->interrupt) {
		cdc_enc->interrupt = urb;
		/* and submit immediately in case we have clients waiting */
		cdc_enc_status_urb(cdc_enc, GFP_KERNEL);
	}
	return 0;
}
EXPORT_SYMBOL_GPL(cdc_enc_set_interrupt);

/* global state relating to the character device */
static struct class *cdc_enc_class;	/* registered device class */
static dev_t cdc_enc_dev0;		/* our allocated major/minor range */
static unsigned long cdc_enc_minor;	/* bitmap of minors in use */

static ssize_t protocol_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	FUNTIONCALL();
	struct cdc_enc_state *cdc_enc = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", cdc_enc->protocol ? cdc_enc->protocol : "none");
}
static struct device_attribute cdc_enc_dev_attrs[] = {
	__ATTR_RO(protocol),
	__ATTR_NULL
};

/* allocate and initiate a CDC_ENC state device */
struct cdc_enc_state *cdc_enc_init_one(struct usb_interface *intf, const char *protocol)
{
	FUNTIONCALL();
	struct cdc_enc_state *cdc_enc;
	dev_t devno;
	int i, ret;
	struct usb_device *udev = interface_to_usbdev(intf);

	/* find an unused minor */
	for (i = 0; i < CDC_ENC_MAX_MINOR; i++)
		if (!test_and_set_bit(i, &cdc_enc_minor))
			break;

	/* no free devices */
	if (i == CDC_ENC_MAX_MINOR)
		goto err_nodev;

	cdc_enc = kzalloc(sizeof(struct cdc_enc_state), GFP_KERNEL);
	if (!cdc_enc)
		goto err_nodev;

	/* save protocol name */
	cdc_enc->protocol = protocol;

	/* make device number */
	devno = cdc_enc_dev0 + i;

	/*
	 * keep track of the device receiving the control messages and the
	 * number of the CDC (like) control interface which is our target.
	 * Note that the interface might be disguised as vendor specific,
	 * and be a combined CDC control/data interface
	 */
	cdc_enc->intf = intf;
	/* FIXME: it would be useful to verify that this interface actually talks CDC */

	/* create async receive URB */
	cdc_enc->urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!cdc_enc->urb)
		goto err_urb;

	/* usb control setup */
	cdc_enc->setup.bRequestType = USB_DIR_IN|USB_TYPE_CLASS|USB_RECIP_INTERFACE;
	cdc_enc->setup.bRequest = USB_CDC_GET_ENCAPSULATED_RESPONSE;
	cdc_enc->setup.wValue = 0; /* zero */
	cdc_enc->setup.wIndex = intf->cur_altsetting->desc.bInterfaceNumber;
	cdc_enc->setup.wLength = CDC_ENC_BUFLEN;
	__le16 value = 0;
	cdc_enc->setup.wValue = cpu_to_le16((value = cdc_enc->setup.wValue));
	cdc_enc->setup.wIndex = cpu_to_le16((value = cdc_enc->setup.wIndex));
	cdc_enc->setup.wLength = cpu_to_le16((value = cdc_enc->setup.wLength));

	printBuf(&cdc_enc->setup,8, "cdc_enc->setup value");
	/* prepare the async receive URB */
	usb_fill_control_urb(cdc_enc->urb, udev,
			usb_rcvctrlpipe(udev, 0),
			(char *)&cdc_enc->setup,
			cdc_enc->rcvbuf,
			CDC_ENC_BUFLEN,
			cdc_enc_read_callback, cdc_enc);

	init_waitqueue_head(&cdc_enc->waitq);

	/* initialize client list */
	INIT_LIST_HEAD(&cdc_enc->clients);
	mutex_init(&cdc_enc->clients_lock);

	/* finally, create the character device, using the interface as a parent dev */
	cdev_init(&cdc_enc->cdev, &cdc_enc_fops);
	ret = cdev_add(&cdc_enc->cdev, devno, 1);
	if (ret < 0)
		goto err_cdev;

	device_create(cdc_enc_class, &intf->dev, devno, cdc_enc, "cdc-enc%d", i);
	dev_info(&intf->dev, "attached to cdc-enc%d using protocol '%s'\n", i, protocol);

	/* this must be set later by calling cdc_enc_set_interrupt() */
	cdc_enc->interrupt = NULL;


	return cdc_enc;

err_cdev:
	usb_free_urb(cdc_enc->urb);
err_urb:
	kfree(cdc_enc);
err_nodev:
	return NULL;
}
EXPORT_SYMBOL_GPL(cdc_enc_init_one);

/* disable and free a CDC_ENC state device */
int cdc_enc_free_one(struct cdc_enc_state *cdc_enc)
{
	FUNTIONCALL();
	struct cdc_enc_client *tmp;

	/* kill any pending recv urb */
	cdc_enc_kill_readurb(cdc_enc);

	/* wait for all clients to exit first ... */
	list_for_each_entry(tmp, &cdc_enc->clients, list) {
		dev_dbg(&cdc_enc->intf->dev, "waiting for client %p to die\n", tmp);
		set_bit(CDC_ENC_CLIENT_SHUTDOWN, &tmp->flags);
		complete(&tmp->ready);
	}
	wait_event_interruptible(cdc_enc->waitq, list_empty(&cdc_enc->clients));

	/* delete character device */
	device_destroy(cdc_enc_class, cdc_enc->cdev.dev);
	cdev_del(&cdc_enc->cdev);

	/* free URB */
	usb_free_urb(cdc_enc->urb);

	/* mark minor available again */
	clear_bit(MINOR(cdc_enc->cdev.dev) - MINOR(cdc_enc_dev0), &cdc_enc_minor);

	/* release this slot */
	kfree(cdc_enc);

	return 0;
}
EXPORT_SYMBOL_GPL(cdc_enc_free_one);

static int __init cdc_enc_init(void)
{
	FUNTIONCALL();
	int ret;

	ret = alloc_chrdev_region(&cdc_enc_dev0, 0, CDC_ENC_MAX_MINOR, "cdc_enc");
	if (ret < 0)
		goto err_region;

	/* create a chardev class */
	cdc_enc_class = class_create(THIS_MODULE, "cdc_enc");
	if (IS_ERR(cdc_enc_class)) {
		ret = PTR_ERR(cdc_enc_class);
		goto err_class;
	}
	cdc_enc_class->dev_attrs = cdc_enc_dev_attrs;

	return 0;

err_class:
	unregister_chrdev_region(cdc_enc_dev0, CDC_ENC_MAX_MINOR);
err_region:
	return ret;
}
module_init(cdc_enc_init);

static void __exit cdc_enc_exit(void)
{
	FUNTIONCALL();
	class_destroy(cdc_enc_class);
	unregister_chrdev_region(cdc_enc_dev0, CDC_ENC_MAX_MINOR);
}
module_exit(cdc_enc_exit);

MODULE_AUTHOR("Bj?rn Mork <bjorn@...k.no>");
MODULE_DESCRIPTION("CDC Encapsulated Command Driver");
MODULE_LICENSE("GPL");
