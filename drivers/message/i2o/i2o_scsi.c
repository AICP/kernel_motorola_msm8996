/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * For the avoidance of doubt the "preferred form" of this code is one which
 * is in an open non patent encumbered format. Where cryptographic key signing
 * forms part of the process of creating an executable the information
 * including keys needed to generate an equivalently functional executable
 * are deemed to be part of the source code.
 *
 *  Complications for I2O scsi
 *
 *	o	Each (bus,lun) is a logical device in I2O. We keep a map
 *		table. We spoof failed selection for unmapped units
 *	o	Request sense buffers can come back for free.
 *	o	Scatter gather is a bit dynamic. We have to investigate at
 *		setup time.
 *	o	Some of our resources are dynamically shared. The i2o core
 *		needs a message reservation protocol to avoid swap v net
 *		deadlocking. We need to back off queue requests.
 *
 *	In general the firmware wants to help. Where its help isn't performance
 *	useful we just ignore the aid. Its not worth the code in truth.
 *
 * Fixes/additions:
 *	Steve Ralston:
 *		Scatter gather now works
 *	Markus Lidel <Markus.Lidel@shadowconnect.com>:
 *		Minor fixes for 2.6.
 *
 * To Do:
 *	64bit cleanups
 *	Fix the resource management problems.
 */

#define DEBUG 1
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/jiffies.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/prefetch.h>
#include <linux/pci.h>
#include <linux/blkdev.h>
#include <linux/i2o.h>
#include <linux/scatterlist.h>

#include <asm/dma.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/atomic.h>

#include <scsi/scsi.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_request.h>
#include <scsi/sg.h>
#include <scsi/sg_request.h>

#define OSM_NAME	"scsi-osm"
#define OSM_VERSION	"1.282"
#define OSM_DESCRIPTION	"I2O SCSI Peripheral OSM"

static struct i2o_driver i2o_scsi_driver;

static unsigned int i2o_scsi_max_id = 16;
static unsigned int i2o_scsi_max_lun = 255;

struct i2o_scsi_host {
	struct Scsi_Host *scsi_host;	/* pointer to the SCSI host */
	struct i2o_controller *iop;	/* pointer to the I2O controller */
	unsigned int lun;	/* lun's used for block devices */
	struct i2o_device *channel[0];	/* channel->i2o_dev mapping table */
};

static struct scsi_host_template i2o_scsi_host_template;

#define I2O_SCSI_CAN_QUEUE	4

/* SCSI OSM class handling definition */
static struct i2o_class_id i2o_scsi_class_id[] = {
	{I2O_CLASS_SCSI_PERIPHERAL},
	{I2O_CLASS_END}
};

static struct i2o_scsi_host *i2o_scsi_host_alloc(struct i2o_controller *c)
{
	struct i2o_scsi_host *i2o_shost;
	struct i2o_device *i2o_dev;
	struct Scsi_Host *scsi_host;
	int max_channel = 0;
	u8 type;
	int i;
	size_t size;
	u16 body_size = 6;

#ifdef CONFIG_I2O_EXT_ADAPTEC
	if (c->adaptec)
		body_size = 8;
#endif

	list_for_each_entry(i2o_dev, &c->devices, list)
	    if (i2o_dev->lct_data.class_id == I2O_CLASS_BUS_ADAPTER) {
		if (i2o_parm_field_get(i2o_dev, 0x0000, 0, &type, 1)
		    && (type == 0x01))	/* SCSI bus */
			max_channel++;
	}

	if (!max_channel) {
		osm_warn("no channels found on %s\n", c->name);
		return ERR_PTR(-EFAULT);
	}

	size = max_channel * sizeof(struct i2o_device *)
	    + sizeof(struct i2o_scsi_host);

	scsi_host = scsi_host_alloc(&i2o_scsi_host_template, size);
	if (!scsi_host) {
		osm_warn("Could not allocate SCSI host\n");
		return ERR_PTR(-ENOMEM);
	}

	scsi_host->max_channel = max_channel - 1;
	scsi_host->max_id = i2o_scsi_max_id;
	scsi_host->max_lun = i2o_scsi_max_lun;
	scsi_host->this_id = c->unit;
	scsi_host->sg_tablesize = i2o_sg_tablesize(c, body_size);

	i2o_shost = (struct i2o_scsi_host *)scsi_host->hostdata;
	i2o_shost->scsi_host = scsi_host;
	i2o_shost->iop = c;
	i2o_shost->lun = 1;

	i = 0;
	list_for_each_entry(i2o_dev, &c->devices, list)
	    if (i2o_dev->lct_data.class_id == I2O_CLASS_BUS_ADAPTER) {
		if (i2o_parm_field_get(i2o_dev, 0x0000, 0, &type, 1)
		    && (type == 0x01))	/* only SCSI bus */
			i2o_shost->channel[i++] = i2o_dev;

		if (i >= max_channel)
			break;
	}

	return i2o_shost;
};

/**
 *	i2o_scsi_get_host - Get an I2O SCSI host
 *	@c: I2O controller to for which to get the SCSI host
 *
 *	If the I2O controller already exists as SCSI host, the SCSI host
 *	is returned, otherwise the I2O controller is added to the SCSI
 *	core.
 *
 *	Returns pointer to the I2O SCSI host on success or NULL on failure.
 */
static struct i2o_scsi_host *i2o_scsi_get_host(struct i2o_controller *c)
{
	return c->driver_data[i2o_scsi_driver.context];
};

/**
 *	i2o_scsi_remove - Remove I2O device from SCSI core
 *	@dev: device which should be removed
 *
 *	Removes the I2O device from the SCSI core again.
 *
 *	Returns 0 on success.
 */
static int i2o_scsi_remove(struct device *dev)
{
	struct i2o_device *i2o_dev = to_i2o_device(dev);
	struct i2o_controller *c = i2o_dev->iop;
	struct i2o_scsi_host *i2o_shost;
	struct scsi_device *scsi_dev;

	osm_info("device removed (TID: %03x)\n", i2o_dev->lct_data.tid);

	i2o_shost = i2o_scsi_get_host(c);

	shost_for_each_device(scsi_dev, i2o_shost->scsi_host)
	    if (scsi_dev->hostdata == i2o_dev) {
		sysfs_remove_link(&i2o_dev->device.kobj, "scsi");
		scsi_remove_device(scsi_dev);
		scsi_device_put(scsi_dev);
		break;
	}

	return 0;
};

/**
 *	i2o_scsi_probe - verify if dev is a I2O SCSI device and install it
 *	@dev: device to verify if it is a I2O SCSI device
 *
 *	Retrieve channel, id and lun for I2O device. If everthing goes well
 *	register the I2O device as SCSI device on the I2O SCSI controller.
 *
 *	Returns 0 on success or negative error code on failure.
 */
static int i2o_scsi_probe(struct device *dev)
{
	struct i2o_device *i2o_dev = to_i2o_device(dev);
	struct i2o_controller *c = i2o_dev->iop;
	struct i2o_scsi_host *i2o_shost;
	struct Scsi_Host *scsi_host;
	struct i2o_device *parent;
	struct scsi_device *scsi_dev;
	u32 id = -1;
	u64 lun = -1;
	int channel = -1;
	int i;

	i2o_shost = i2o_scsi_get_host(c);
	if (!i2o_shost)
		return -EFAULT;

	scsi_host = i2o_shost->scsi_host;

	switch (i2o_dev->lct_data.class_id) {
	case I2O_CLASS_RANDOM_BLOCK_STORAGE:
	case I2O_CLASS_EXECUTIVE:
#ifdef CONFIG_I2O_EXT_ADAPTEC
		if (c->adaptec) {
			u8 type;
			struct i2o_device *d = i2o_shost->channel[0];

			if (i2o_parm_field_get(d, 0x0000, 0, &type, 1)
			    && (type == 0x01))	/* SCSI bus */
				if (i2o_parm_field_get(d, 0x0200, 4, &id, 4)) {
					channel = 0;
					if (i2o_dev->lct_data.class_id ==
					    I2O_CLASS_RANDOM_BLOCK_STORAGE)
						lun = i2o_shost->lun++;
					else
						lun = 0;
				}
		}
#endif
		break;

	case I2O_CLASS_SCSI_PERIPHERAL:
		if (i2o_parm_field_get(i2o_dev, 0x0000, 3, &id, 4) < 0)
			return -EFAULT;

		if (i2o_parm_field_get(i2o_dev, 0x0000, 4, &lun, 8) < 0)
			return -EFAULT;

		parent = i2o_iop_find_device(c, i2o_dev->lct_data.parent_tid);
		if (!parent) {
			osm_warn("can not find parent of device %03x\n",
				 i2o_dev->lct_data.tid);
			return -EFAULT;
		}

		for (i = 0; i <= i2o_shost->scsi_host->max_channel; i++)
			if (i2o_shost->channel[i] == parent)
				channel = i;
		break;

	default:
		return -EFAULT;
	}

	if (channel == -1) {
		osm_warn("can not find channel of device %03x\n",
			 i2o_dev->lct_data.tid);
		return -EFAULT;
	}

	if (id >= scsi_host->max_id) {
		osm_warn("SCSI device id (%d) >= max_id of I2O host (%d)", id,
			 scsi_host->max_id);
		return -EFAULT;
	}

	if (lun >= scsi_host->max_lun) {
		osm_warn("SCSI device id (%d) >= max_lun of I2O host (%d)",
			 (unsigned int)lun, scsi_host->max_lun);
		return -EFAULT;
	}

	scsi_dev =
	    __scsi_add_device(i2o_shost->scsi_host, channel, id, lun, i2o_dev);

	if (IS_ERR(scsi_dev)) {
		osm_warn("can not add SCSI device %03x\n",
			 i2o_dev->lct_data.tid);
		return PTR_ERR(scsi_dev);
	}

	sysfs_create_link(&i2o_dev->device.kobj, &scsi_dev->sdev_gendev.kobj,
			  "scsi");

	osm_info("device added (TID: %03x) channel: %d, id: %d, lun: %d\n",
		 i2o_dev->lct_data.tid, channel, id, (unsigned int)lun);

	return 0;
};

static const char *i2o_scsi_info(struct Scsi_Host *SChost)
{
	struct i2o_scsi_host *hostdata;
	hostdata = (struct i2o_scsi_host *)SChost->hostdata;
	return hostdata->iop->name;
}

/**
 *	i2o_scsi_reply - SCSI OSM message reply handler
 *	@c: controller issuing the reply
 *	@m: message id for flushing
 *	@msg: the message from the controller
 *
 *	Process reply messages (interrupts in normal scsi controller think).
 *	We can get a variety of messages to process. The normal path is
 *	scsi command completions. We must also deal with IOP failures,
 *	the reply to a bus reset and the reply to a LUN query.
 *
 *	Returns 0 on success and if the reply should not be flushed or > 0
 *	on success and if the reply should be flushed. Returns negative error
 *	code on failure and if the reply should be flushed.
 */
static int i2o_scsi_reply(struct i2o_controller *c, u32 m,
			  struct i2o_message *msg)
{
	struct scsi_cmnd *cmd;
	struct device *dev;
	u8 as, ds, st;

	cmd = i2o_cntxt_list_get(c, le32_to_cpu(msg->u.s.tcntxt));

	if (msg->u.head[0] & (1 << 13)) {
		struct i2o_message __iomem *pmsg;	/* preserved message */
		u32 pm;
		int err = DID_ERROR;

		pm = le32_to_cpu(msg->body[3]);

		pmsg = i2o_msg_in_to_virt(c, pm);

		osm_err("IOP fail.\n");
		osm_err("From %d To %d Cmd %d.\n",
			(msg->u.head[1] >> 12) & 0xFFF,
			msg->u.head[1] & 0xFFF, msg->u.head[1] >> 24);
		osm_err("Failure Code %d.\n", msg->body[0] >> 24);
		if (msg->body[0] & (1 << 16))
			osm_err("Format error.\n");
		if (msg->body[0] & (1 << 17))
			osm_err("Path error.\n");
		if (msg->body[0] & (1 << 18))
			osm_err("Path State.\n");
		if (msg->body[0] & (1 << 18))
		{
			osm_err("Congestion.\n");
			err = DID_BUS_BUSY;
		}

		osm_debug("Failing message is %p.\n", pmsg);

		cmd = i2o_cntxt_list_get(c, readl(&pmsg->u.s.tcntxt));
		if (!cmd)
			return 1;

		cmd->result = err << 16;
		cmd->scsi_done(cmd);

		/* Now flush the message by making it a NOP */
		i2o_msg_nop(c, pm);

		return 1;
	}

	/*
	 *      Low byte is device status, next is adapter status,
	 *      (then one byte reserved), then request status.
	 */
	ds = (u8) le32_to_cpu(msg->body[0]);
	as = (u8) (le32_to_cpu(msg->body[0]) >> 8);
	st = (u8) (le32_to_cpu(msg->body[0]) >> 24);

	/*
	 *      Is this a control request coming back - eg an abort ?
	 */

	if (!cmd) {
		if (st)
			osm_warn("SCSI abort: %08X", le32_to_cpu(msg->body[0]));
		osm_info("SCSI abort completed.\n");
		return -EFAULT;
	}

	osm_debug("Completed %ld\n", cmd->serial_number);

	if (st) {
		u32 count, error;
		/* An error has occurred */

		switch (st) {
		case 0x06:
			count = le32_to_cpu(msg->body[1]);
			if (count < cmd->underflow) {
				int i;

				osm_err("SCSI underflow 0x%08X 0x%08X\n", count,
					cmd->underflow);
				osm_debug("Cmd: ");
				for (i = 0; i < 15; i++)
					pr_debug("%02X ", cmd->cmnd[i]);
				pr_debug(".\n");
				cmd->result = (DID_ERROR << 16);
			}
			break;

		default:
			error = le32_to_cpu(msg->body[0]);

			osm_err("SCSI error %08x\n", error);

			if ((error & 0xff) == 0x02 /*CHECK_CONDITION */ ) {
				int i;
				u32 len = sizeof(cmd->sense_buffer);
				len = (len > 40) ? 40 : len;
				// Copy over the sense data
				memcpy(cmd->sense_buffer, (void *)&msg->body[3],
				       len);
				for (i = 0; i <= len; i++)
					osm_info("%02x\n",
						 cmd->sense_buffer[i]);
				if (cmd->sense_buffer[0] == 0x70
				    && cmd->sense_buffer[2] == DATA_PROTECT) {
					/* This is to handle an array failed */
					cmd->result = (DID_TIME_OUT << 16);
					printk(KERN_WARNING "%s: SCSI Data "
					       "Protect-Device (%d,%d,%d) "
					       "hba_status=0x%x, dev_status="
					       "0x%x, cmd=0x%x\n", c->name,
					       (u32) cmd->device->channel,
					       (u32) cmd->device->id,
					       (u32) cmd->device->lun,
					       (error >> 8) & 0xff,
					       error & 0xff, cmd->cmnd[0]);
				} else
					cmd->result = (DID_ERROR << 16);

				break;
			}

			switch (as) {
			case 0x0E:
				/* SCSI Reset */
				cmd->result = DID_RESET << 16;
				break;

			case 0x0F:
				cmd->result = DID_PARITY << 16;
				break;

			default:
				cmd->result = DID_ERROR << 16;
				break;
			}

			break;
		}

		cmd->scsi_done(cmd);
		return 1;
	}

	cmd->result = DID_OK << 16 | ds;

	dev = &c->pdev->dev;
	if (cmd->use_sg)
		dma_unmap_sg(dev, (struct scatterlist *)cmd->buffer,
			     cmd->use_sg, cmd->sc_data_direction);
	else if (cmd->request_bufflen)
		dma_unmap_single(dev, (dma_addr_t) ((long)cmd->SCp.ptr),
				 cmd->request_bufflen, cmd->sc_data_direction);

	cmd->scsi_done(cmd);

	return 1;
};

/**
 *	i2o_scsi_notify_controller_add - Retrieve notifications of added
 *					 controllers
 *	@c: the controller which was added
 *
 *	If a I2O controller is added, we catch the notification to add a
 *	corresponding Scsi_Host.
 */
static void i2o_scsi_notify_controller_add(struct i2o_controller *c)
{
	struct i2o_scsi_host *i2o_shost;
	int rc;

	i2o_shost = i2o_scsi_host_alloc(c);
	if (IS_ERR(i2o_shost)) {
		osm_err("Could not initialize SCSI host\n");
		return;
	}

	rc = scsi_add_host(i2o_shost->scsi_host, &c->device);
	if (rc) {
		osm_err("Could not add SCSI host\n");
		scsi_host_put(i2o_shost->scsi_host);
		return;
	}

	c->driver_data[i2o_scsi_driver.context] = i2o_shost;

	osm_debug("new I2O SCSI host added\n");
};

/**
 *	i2o_scsi_notify_controller_remove - Retrieve notifications of removed
 *					    controllers
 *	@c: the controller which was removed
 *
 *	If a I2O controller is removed, we catch the notification to remove the
 *	corresponding Scsi_Host.
 */
static void i2o_scsi_notify_controller_remove(struct i2o_controller *c)
{
	struct i2o_scsi_host *i2o_shost;
	i2o_shost = i2o_scsi_get_host(c);
	if (!i2o_shost)
		return;

	c->driver_data[i2o_scsi_driver.context] = NULL;

	scsi_remove_host(i2o_shost->scsi_host);
	scsi_host_put(i2o_shost->scsi_host);
	osm_debug("I2O SCSI host removed\n");
};

/* SCSI OSM driver struct */
static struct i2o_driver i2o_scsi_driver = {
	.name = OSM_NAME,
	.reply = i2o_scsi_reply,
	.classes = i2o_scsi_class_id,
	.notify_controller_add = i2o_scsi_notify_controller_add,
	.notify_controller_remove = i2o_scsi_notify_controller_remove,
	.driver = {
		   .probe = i2o_scsi_probe,
		   .remove = i2o_scsi_remove,
		   },
};

/**
 *	i2o_scsi_queuecommand - queue a SCSI command
 *	@SCpnt: scsi command pointer
 *	@done: callback for completion
 *
 *	Issue a scsi command asynchronously. Return 0 on success or 1 if
 *	we hit an error (normally message queue congestion). The only
 *	minor complication here is that I2O deals with the device addressing
 *	so we have to map the bus/dev/lun back to an I2O handle as well
 *	as faking absent devices ourself.
 *
 *	Locks: takes the controller lock on error path only
 */

static int i2o_scsi_queuecommand(struct scsi_cmnd *SCpnt,
				 void (*done) (struct scsi_cmnd *))
{
	struct i2o_controller *c;
	struct i2o_device *i2o_dev;
	int tid;
	struct i2o_message __iomem *msg;
	u32 m;
	/*
	 * ENABLE_DISCONNECT
	 * SIMPLE_TAG
	 * RETURN_SENSE_DATA_IN_REPLY_MESSAGE_FRAME
	 */
	u32 scsi_flags = 0x20a00000;
	u32 sgl_offset;
	u32 __iomem *mptr;
	u32 cmd = I2O_CMD_SCSI_EXEC << 24;
	int rc = 0;

	/*
	 *      Do the incoming paperwork
	 */
	i2o_dev = SCpnt->device->hostdata;
	c = i2o_dev->iop;

	SCpnt->scsi_done = done;

	if (unlikely(!i2o_dev)) {
		osm_warn("no I2O device in request\n");
		SCpnt->result = DID_NO_CONNECT << 16;
		done(SCpnt);
		goto exit;
	}

	tid = i2o_dev->lct_data.tid;

	osm_debug("qcmd: Tid = %03x\n", tid);
	osm_debug("Real scsi messages.\n");

	/*
	 *      Put together a scsi execscb message
	 */
	switch (SCpnt->sc_data_direction) {
	case PCI_DMA_NONE:
		/* DATA NO XFER */
		sgl_offset = SGL_OFFSET_0;
		break;

	case PCI_DMA_TODEVICE:
		/* DATA OUT (iop-->dev) */
		scsi_flags |= 0x80000000;
		sgl_offset = SGL_OFFSET_10;
		break;

	case PCI_DMA_FROMDEVICE:
		/* DATA IN  (iop<--dev) */
		scsi_flags |= 0x40000000;
		sgl_offset = SGL_OFFSET_10;
		break;

	default:
		/* Unknown - kill the command */
		SCpnt->result = DID_NO_CONNECT << 16;
		done(SCpnt);
		goto exit;
	}

	/*
	 *      Obtain an I2O message. If there are none free then
	 *      throw it back to the scsi layer
	 */

	m = i2o_msg_get_wait(c, &msg, I2O_TIMEOUT_MESSAGE_GET);
	if (m == I2O_QUEUE_EMPTY) {
		rc = SCSI_MLQUEUE_HOST_BUSY;
		goto exit;
	}

	mptr = &msg->body[0];

#ifdef CONFIG_I2O_EXT_ADAPTEC
	if (c->adaptec) {
		u32 adpt_flags = 0;

		if (SCpnt->sc_request && SCpnt->sc_request->upper_private_data) {
			i2o_sg_io_hdr_t __user *usr_ptr =
			    ((Sg_request *) (SCpnt->sc_request->
					     upper_private_data))->header.
			    usr_ptr;

			if (usr_ptr)
				get_user(adpt_flags, &usr_ptr->flags);
		}

		switch (i2o_dev->lct_data.class_id) {
		case I2O_CLASS_EXECUTIVE:
		case I2O_CLASS_RANDOM_BLOCK_STORAGE:
			/* interpret flag has to be set for executive */
			adpt_flags ^= I2O_DPT_SG_FLAG_INTERPRET;
			break;

		default:
			break;
		}

		/*
		 * for Adaptec controllers we use the PRIVATE command, because
		 * the normal SCSI EXEC doesn't support all SCSI commands on
		 * all controllers (for example READ CAPACITY).
		 */
		if (sgl_offset == SGL_OFFSET_10)
			sgl_offset = SGL_OFFSET_12;
		cmd = I2O_CMD_PRIVATE << 24;
		writel(I2O_VENDOR_DPT << 16 | I2O_CMD_SCSI_EXEC, mptr++);
		writel(adpt_flags | tid, mptr++);
	}
#endif

	writel(cmd | HOST_TID << 12 | tid, &msg->u.head[1]);
	writel(i2o_scsi_driver.context, &msg->u.s.icntxt);

	/* We want the SCSI control block back */
	writel(i2o_cntxt_list_add(c, SCpnt), &msg->u.s.tcntxt);

	/* LSI_920_PCI_QUIRK
	 *
	 *      Intermittant observations of msg frame word data corruption
	 *      observed on msg[4] after:
	 *        WRITE, READ-MODIFY-WRITE
	 *      operations.  19990606 -sralston
	 *
	 *      (Hence we build this word via tag. Its good practice anyway
	 *       we don't want fetches over PCI needlessly)
	 */

	/* Attach tags to the devices */
	/*
	   if(SCpnt->device->tagged_supported) {
	   if(SCpnt->tag == HEAD_OF_QUEUE_TAG)
	   scsi_flags |= 0x01000000;
	   else if(SCpnt->tag == ORDERED_QUEUE_TAG)
	   scsi_flags |= 0x01800000;
	   }
	 */

	writel(scsi_flags | SCpnt->cmd_len, mptr++);

	/* Write SCSI command into the message - always 16 byte block */
	memcpy_toio(mptr, SCpnt->cmnd, 16);
	mptr += 4;

	if (sgl_offset != SGL_OFFSET_0) {
		/* write size of data addressed by SGL */
		writel(SCpnt->request_bufflen, mptr++);

		/* Now fill in the SGList and command */
		if (SCpnt->use_sg) {
			if (!i2o_dma_map_sg(c, SCpnt->request_buffer,
					    SCpnt->use_sg,
					    SCpnt->sc_data_direction, &mptr))
				goto nomem;
		} else {
			SCpnt->SCp.dma_handle =
			    i2o_dma_map_single(c, SCpnt->request_buffer,
					       SCpnt->request_bufflen,
					       SCpnt->sc_data_direction, &mptr);
			if (dma_mapping_error(SCpnt->SCp.dma_handle))
				goto nomem;
		}
	}

	/* Stick the headers on */
	writel(I2O_MESSAGE_SIZE(mptr - &msg->u.head[0]) | sgl_offset,
	       &msg->u.head[0]);

	/* Queue the message */
	i2o_msg_post(c, m);

	osm_debug("Issued %ld\n", SCpnt->serial_number);

	return 0;

      nomem:
	rc = -ENOMEM;
	i2o_msg_nop(c, m);

      exit:
	return rc;
};

/**
 *	i2o_scsi_abort - abort a running command
 *	@SCpnt: command to abort
 *
 *	Ask the I2O controller to abort a command. This is an asynchrnous
 *	process and our callback handler will see the command complete with an
 *	aborted message if it succeeds.
 *
 *	Returns 0 if the command is successfully aborted or negative error code
 *	on failure.
 */
static int i2o_scsi_abort(struct scsi_cmnd *SCpnt)
{
	struct i2o_device *i2o_dev;
	struct i2o_controller *c;
	struct i2o_message __iomem *msg;
	u32 m;
	int tid;
	int status = FAILED;

	osm_warn("Aborting command block.\n");

	i2o_dev = SCpnt->device->hostdata;
	c = i2o_dev->iop;
	tid = i2o_dev->lct_data.tid;

	m = i2o_msg_get_wait(c, &msg, I2O_TIMEOUT_MESSAGE_GET);
	if (m == I2O_QUEUE_EMPTY)
		return SCSI_MLQUEUE_HOST_BUSY;

	writel(FIVE_WORD_MSG_SIZE | SGL_OFFSET_0, &msg->u.head[0]);
	writel(I2O_CMD_SCSI_ABORT << 24 | HOST_TID << 12 | tid,
	       &msg->u.head[1]);
	writel(i2o_cntxt_list_get_ptr(c, SCpnt), &msg->body[0]);

	if (i2o_msg_post_wait(c, m, I2O_TIMEOUT_SCSI_SCB_ABORT))
		status = SUCCESS;

	return status;
}

/**
 *	i2o_scsi_bios_param	-	Invent disk geometry
 *	@sdev: scsi device
 *	@dev: block layer device
 *	@capacity: size in sectors
 *	@ip: geometry array
 *
 *	This is anyones guess quite frankly. We use the same rules everyone
 *	else appears to and hope. It seems to work.
 */

static int i2o_scsi_bios_param(struct scsi_device *sdev,
			       struct block_device *dev, sector_t capacity,
			       int *ip)
{
	int size;

	size = capacity;
	ip[0] = 64;		/* heads                        */
	ip[1] = 32;		/* sectors                      */
	if ((ip[2] = size >> 11) > 1024) {	/* cylinders, test for big disk */
		ip[0] = 255;	/* heads                        */
		ip[1] = 63;	/* sectors                      */
		ip[2] = size / (255 * 63);	/* cylinders                    */
	}
	return 0;
}

static struct scsi_host_template i2o_scsi_host_template = {
	.proc_name = OSM_NAME,
	.name = OSM_DESCRIPTION,
	.info = i2o_scsi_info,
	.queuecommand = i2o_scsi_queuecommand,
	.eh_abort_handler = i2o_scsi_abort,
	.bios_param = i2o_scsi_bios_param,
	.can_queue = I2O_SCSI_CAN_QUEUE,
	.sg_tablesize = 8,
	.cmd_per_lun = 6,
	.use_clustering = ENABLE_CLUSTERING,
};

/**
 *	i2o_scsi_init - SCSI OSM initialization function
 *
 *	Register SCSI OSM into I2O core.
 *
 *	Returns 0 on success or negative error code on failure.
 */
static int __init i2o_scsi_init(void)
{
	int rc;

	printk(KERN_INFO OSM_DESCRIPTION " v" OSM_VERSION "\n");

	/* Register SCSI OSM into I2O core */
	rc = i2o_driver_register(&i2o_scsi_driver);
	if (rc) {
		osm_err("Could not register SCSI driver\n");
		return rc;
	}

	return 0;
};

/**
 *	i2o_scsi_exit - SCSI OSM exit function
 *
 *	Unregisters SCSI OSM from I2O core.
 */
static void __exit i2o_scsi_exit(void)
{
	/* Unregister I2O SCSI OSM from I2O core */
	i2o_driver_unregister(&i2o_scsi_driver);
};

MODULE_AUTHOR("Red Hat Software");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(OSM_DESCRIPTION);
MODULE_VERSION(OSM_VERSION);

module_init(i2o_scsi_init);
module_exit(i2o_scsi_exit);
