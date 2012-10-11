/*
 *
 * Intel Management Engine Interface (Intel MEI) Linux driver
 * Copyright (c) 2003-2012, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */


#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/aio.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/uuid.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>


#include "mei_dev.h"
#include "hw.h"
#include <linux/mei.h>
#include "interface.h"

/**
 * mei_io_cb_free - free mei_cb_private related memory
 *
 * @cb: mei callback struct
 */
void mei_io_cb_free(struct mei_cl_cb *cb)
{
	if (cb == NULL)
		return;

	kfree(cb->request_buffer.data);
	kfree(cb->response_buffer.data);
	kfree(cb);
}
/**
 * mei_io_cb_init - allocate and initialize io callback
 *
 * @cl - mei client
 * @file: pointer to file structure
 *
 * returns mei_cl_cb pointer or NULL;
 */
struct mei_cl_cb *mei_io_cb_init(struct mei_cl *cl, struct file *fp)
{
	struct mei_cl_cb *cb;

	cb = kzalloc(sizeof(struct mei_cl_cb), GFP_KERNEL);
	if (!cb)
		return NULL;

	mei_io_list_init(cb);

	cb->file_object = fp;
	cb->file_private = cl;
	cb->buf_idx = 0;
	return cb;
}


/**
 * mei_io_cb_alloc_req_buf - allocate request buffer
 *
 * @cb -  io callback structure
 * @size: size of the buffer
 *
 * returns 0 on success
 *         -EINVAL if cb is NULL
 *         -ENOMEM if allocation failed
 */
int mei_io_cb_alloc_req_buf(struct mei_cl_cb *cb, size_t length)
{
	if (!cb)
		return -EINVAL;

	if (length == 0)
		return 0;

	cb->request_buffer.data = kmalloc(length, GFP_KERNEL);
	if (!cb->request_buffer.data)
		return -ENOMEM;
	cb->request_buffer.size = length;
	return 0;
}
/**
 * mei_io_cb_alloc_req_buf - allocate respose buffer
 *
 * @cb -  io callback structure
 * @size: size of the buffer
 *
 * returns 0 on success
 *         -EINVAL if cb is NULL
 *         -ENOMEM if allocation failed
 */
int mei_io_cb_alloc_resp_buf(struct mei_cl_cb *cb, size_t length)
{
	if (!cb)
		return -EINVAL;

	if (length == 0)
		return 0;

	cb->response_buffer.data = kmalloc(length, GFP_KERNEL);
	if (!cb->response_buffer.data)
		return -ENOMEM;
	cb->response_buffer.size = length;
	return 0;
}


/**
 * mei_me_cl_by_id return index to me_clients for client_id
 *
 * @dev: the device structure
 * @client_id: me client id
 *
 * Locking: called under "dev->device_lock" lock
 *
 * returns index on success, -ENOENT on failure.
 */

int mei_me_cl_by_id(struct mei_device *dev, u8 client_id)
{
	int i;
	for (i = 0; i < dev->me_clients_num; i++)
		if (dev->me_clients[i].client_id == client_id)
			break;
	if (WARN_ON(dev->me_clients[i].client_id != client_id))
		return -ENOENT;

	if (i == dev->me_clients_num)
		return -ENOENT;

	return i;
}

/**
 * mei_ioctl_connect_client - the connect to fw client IOCTL function
 *
 * @dev: the device structure
 * @data: IOCTL connect data, input and output parameters
 * @file: private data of the file object
 *
 * Locking: called under "dev->device_lock" lock
 *
 * returns 0 on success, <0 on failure.
 */
int mei_ioctl_connect_client(struct file *file,
			struct mei_connect_client_data *data)
{
	struct mei_device *dev;
	struct mei_cl_cb *cb;
	struct mei_client *client;
	struct mei_cl *cl;
	struct mei_cl *cl_pos = NULL;
	struct mei_cl *cl_next = NULL;
	long timeout = CONNECT_TIMEOUT;
	int i;
	int err;
	int rets;

	cl = file->private_data;
	if (WARN_ON(!cl || !cl->dev))
		return -ENODEV;

	dev = cl->dev;

	dev_dbg(&dev->pdev->dev, "mei_ioctl_connect_client() Entry\n");

	/* buffered ioctl cb */
	cb = mei_io_cb_init(cl, file);
	if (!cb) {
		rets = -ENOMEM;
		goto end;
	}

	cb->major_file_operations = MEI_IOCTL;

	if (dev->dev_state != MEI_DEV_ENABLED) {
		rets = -ENODEV;
		goto end;
	}
	if (cl->state != MEI_FILE_INITIALIZING &&
	    cl->state != MEI_FILE_DISCONNECTED) {
		rets = -EBUSY;
		goto end;
	}

	/* find ME client we're trying to connect to */
	i = mei_me_cl_by_uuid(dev, &data->in_client_uuid);
	if (i >= 0 && !dev->me_clients[i].props.fixed_address) {
		cl->me_client_id = dev->me_clients[i].client_id;
		cl->state = MEI_FILE_CONNECTING;
	}

	dev_dbg(&dev->pdev->dev, "Connect to FW Client ID = %d\n",
			cl->me_client_id);
	dev_dbg(&dev->pdev->dev, "FW Client - Protocol Version = %d\n",
			dev->me_clients[i].props.protocol_version);
	dev_dbg(&dev->pdev->dev, "FW Client - Max Msg Len = %d\n",
			dev->me_clients[i].props.max_msg_length);

	/* if we're connecting to amthi client then we will use the
	 * existing connection
	 */
	if (uuid_le_cmp(data->in_client_uuid, mei_amthi_guid) == 0) {
		dev_dbg(&dev->pdev->dev, "FW Client is amthi\n");
		if (dev->iamthif_cl.state != MEI_FILE_CONNECTED) {
			rets = -ENODEV;
			goto end;
		}
		clear_bit(cl->host_client_id, dev->host_clients_map);
		list_for_each_entry_safe(cl_pos, cl_next,
					 &dev->file_list, link) {
			if (mei_cl_cmp_id(cl, cl_pos)) {
				dev_dbg(&dev->pdev->dev,
					"remove file private data node host"
				    " client = %d, ME client = %d.\n",
				    cl_pos->host_client_id,
				    cl_pos->me_client_id);
				list_del(&cl_pos->link);
			}

		}
		dev_dbg(&dev->pdev->dev, "free file private data memory.\n");
		kfree(cl);

		cl = NULL;
		file->private_data = &dev->iamthif_cl;

		client = &data->out_client_properties;
		client->max_msg_length =
			dev->me_clients[i].props.max_msg_length;
		client->protocol_version =
			dev->me_clients[i].props.protocol_version;
		rets = dev->iamthif_cl.status;

		goto end;
	}

	if (cl->state != MEI_FILE_CONNECTING) {
		rets = -ENODEV;
		goto end;
	}


	/* prepare the output buffer */
	client = &data->out_client_properties;
	client->max_msg_length = dev->me_clients[i].props.max_msg_length;
	client->protocol_version = dev->me_clients[i].props.protocol_version;
	dev_dbg(&dev->pdev->dev, "Can connect?\n");
	if (dev->mei_host_buffer_is_empty
	    && !mei_other_client_is_connecting(dev, cl)) {
		dev_dbg(&dev->pdev->dev, "Sending Connect Message\n");
		dev->mei_host_buffer_is_empty = false;
		if (mei_connect(dev, cl)) {
			dev_dbg(&dev->pdev->dev, "Sending connect message - failed\n");
			rets = -ENODEV;
			goto end;
		} else {
			dev_dbg(&dev->pdev->dev, "Sending connect message - succeeded\n");
			cl->timer_count = MEI_CONNECT_TIMEOUT;
			list_add_tail(&cb->list, &dev->ctrl_rd_list.list);
		}


	} else {
		dev_dbg(&dev->pdev->dev, "Queuing the connect request due to device busy\n");
		dev_dbg(&dev->pdev->dev, "add connect cb to control write list.\n");
		list_add_tail(&cb->list, &dev->ctrl_wr_list.list);
	}
	mutex_unlock(&dev->device_lock);
	err = wait_event_timeout(dev->wait_recvd_msg,
			(MEI_FILE_CONNECTED == cl->state ||
			 MEI_FILE_DISCONNECTED == cl->state),
			timeout * HZ);

	mutex_lock(&dev->device_lock);
	if (MEI_FILE_CONNECTED == cl->state) {
		dev_dbg(&dev->pdev->dev, "successfully connected to FW client.\n");
		rets = cl->status;
		goto end;
	} else {
		dev_dbg(&dev->pdev->dev, "failed to connect to FW client.cl->state = %d.\n",
		    cl->state);
		if (!err) {
			dev_dbg(&dev->pdev->dev,
				"wait_event_interruptible_timeout failed on client"
				" connect message fw response message.\n");
		}
		rets = -EFAULT;

		mei_io_list_flush(&dev->ctrl_rd_list, cl);
		mei_io_list_flush(&dev->ctrl_wr_list, cl);
		goto end;
	}
	rets = 0;
end:
	dev_dbg(&dev->pdev->dev, "free connect cb memory.");
	mei_io_cb_free(cb);
	return rets;
}

/**
 * find_amthi_read_list_entry - finds a amthilist entry for current file
 *
 * @dev: the device structure
 * @file: pointer to file object
 *
 * returns   returned a list entry on success, NULL on failure.
 */
struct mei_cl_cb *find_amthi_read_list_entry(
		struct mei_device *dev,
		struct file *file)
{
	struct mei_cl *cl_temp;
	struct mei_cl_cb *pos = NULL;
	struct mei_cl_cb *next = NULL;

	list_for_each_entry_safe(pos, next,
	    &dev->amthi_read_complete_list.list, list) {
		cl_temp = (struct mei_cl *)pos->file_private;
		if (cl_temp && cl_temp == &dev->iamthif_cl &&
			pos->file_object == file)
			return pos;
	}
	return NULL;
}

/**
 * amthi_read - read data from AMTHI client
 *
 * @dev: the device structure
 * @if_num:  minor number
 * @file: pointer to file object
 * @*ubuf: pointer to user data in user space
 * @length: data length to read
 * @offset: data read offset
 *
 * Locking: called under "dev->device_lock" lock
 *
 * returns
 *  returned data length on success,
 *  zero if no data to read,
 *  negative on failure.
 */
int amthi_read(struct mei_device *dev, struct file *file,
	       char __user *ubuf, size_t length, loff_t *offset)
{
	int rets;
	int wait_ret;
	struct mei_cl_cb *cb = NULL;
	struct mei_cl *cl = file->private_data;
	unsigned long timeout;
	int i;

	/* Only Posible if we are in timeout */
	if (!cl || cl != &dev->iamthif_cl) {
		dev_dbg(&dev->pdev->dev, "bad file ext.\n");
		return -ETIMEDOUT;
	}

	i = mei_me_cl_by_id(dev, dev->iamthif_cl.me_client_id);

	if (i < 0) {
		dev_dbg(&dev->pdev->dev, "amthi client not found.\n");
		return -ENODEV;
	}
	dev_dbg(&dev->pdev->dev, "checking amthi data\n");
	cb = find_amthi_read_list_entry(dev, file);

	/* Check for if we can block or not*/
	if (cb == NULL && file->f_flags & O_NONBLOCK)
		return -EAGAIN;


	dev_dbg(&dev->pdev->dev, "waiting for amthi data\n");
	while (cb == NULL) {
		/* unlock the Mutex */
		mutex_unlock(&dev->device_lock);

		wait_ret = wait_event_interruptible(dev->iamthif_cl.wait,
			(cb = find_amthi_read_list_entry(dev, file)));

		if (wait_ret)
			return -ERESTARTSYS;

		dev_dbg(&dev->pdev->dev, "woke up from sleep\n");

		/* Locking again the Mutex */
		mutex_lock(&dev->device_lock);
	}


	dev_dbg(&dev->pdev->dev, "Got amthi data\n");
	dev->iamthif_timer = 0;

	if (cb) {
		timeout = cb->read_time + msecs_to_jiffies(IAMTHIF_READ_TIMER);
		dev_dbg(&dev->pdev->dev, "amthi timeout = %lud\n",
				timeout);

		if  (time_after(jiffies, timeout)) {
			dev_dbg(&dev->pdev->dev, "amthi Time out\n");
			/* 15 sec for the message has expired */
			list_del(&cb->list);
			rets = -ETIMEDOUT;
			goto free;
		}
	}
	/* if the whole message will fit remove it from the list */
	if (cb->buf_idx >= *offset && length >= (cb->buf_idx - *offset))
		list_del(&cb->list);
	else if (cb->buf_idx > 0 && cb->buf_idx <= *offset) {
		/* end of the message has been reached */
		list_del(&cb->list);
		rets = 0;
		goto free;
	}
		/* else means that not full buffer will be read and do not
		 * remove message from deletion list
		 */

	dev_dbg(&dev->pdev->dev, "amthi cb->response_buffer size - %d\n",
	    cb->response_buffer.size);
	dev_dbg(&dev->pdev->dev, "amthi cb->buf_idx - %lu\n", cb->buf_idx);

	/* length is being turncated to PAGE_SIZE, however,
	 * the buf_idx may point beyond */
	length = min_t(size_t, length, (cb->buf_idx - *offset));

	if (copy_to_user(ubuf, cb->response_buffer.data + *offset, length))
		rets = -EFAULT;
	else {
		rets = length;
		if ((*offset + length) < cb->buf_idx) {
			*offset += length;
			goto out;
		}
	}
free:
	dev_dbg(&dev->pdev->dev, "free amthi cb memory.\n");
	*offset = 0;
	mei_io_cb_free(cb);
out:
	return rets;
}

/**
 * mei_start_read - the start read client message function.
 *
 * @dev: the device structure
 * @if_num:  minor number
 * @cl: private data of the file object
 *
 * returns 0 on success, <0 on failure.
 */
int mei_start_read(struct mei_device *dev, struct mei_cl *cl)
{
	struct mei_cl_cb *cb;
	int rets;
	int i;

	if (cl->state != MEI_FILE_CONNECTED)
		return -ENODEV;

	if (dev->dev_state != MEI_DEV_ENABLED)
		return -ENODEV;

	if (cl->read_pending || cl->read_cb) {
		dev_dbg(&dev->pdev->dev, "read is pending.\n");
		return -EBUSY;
	}
	i = mei_me_cl_by_id(dev, cl->me_client_id);
	if (i < 0) {
		dev_err(&dev->pdev->dev, "no such me client %d\n",
			cl->me_client_id);
		return  -ENODEV;
	}

	cb = mei_io_cb_init(cl, NULL);
	if (!cb)
		return -ENOMEM;

	rets = mei_io_cb_alloc_resp_buf(cb,
			dev->me_clients[i].props.max_msg_length);
	if (rets)
		goto err;

	cb->major_file_operations = MEI_READ;
	cl->read_cb = cb;
	if (dev->mei_host_buffer_is_empty) {
		dev->mei_host_buffer_is_empty = false;
		if (mei_send_flow_control(dev, cl)) {
			rets = -ENODEV;
			goto err;
		}
		list_add_tail(&cb->list, &dev->read_list.list);
	} else {
		list_add_tail(&cb->list, &dev->ctrl_wr_list.list);
	}
	return rets;
err:
	mei_io_cb_free(cb);
	return rets;
}

/**
 * amthi_write - write iamthif data to amthi client
 *
 * @dev: the device structure
 * @cb: mei call back struct
 *
 * returns 0 on success, <0 on failure.
 */
int amthi_write(struct mei_device *dev, struct mei_cl_cb *cb)
{
	struct mei_msg_hdr mei_hdr;
	int ret;

	if (!dev || !cb)
		return -ENODEV;

	dev_dbg(&dev->pdev->dev, "write data to amthi client.\n");

	dev->iamthif_state = MEI_IAMTHIF_WRITING;
	dev->iamthif_current_cb = cb;
	dev->iamthif_file_object = cb->file_object;
	dev->iamthif_canceled = false;
	dev->iamthif_ioctl = true;
	dev->iamthif_msg_buf_size = cb->request_buffer.size;
	memcpy(dev->iamthif_msg_buf, cb->request_buffer.data,
	       cb->request_buffer.size);

	ret = mei_flow_ctrl_creds(dev, &dev->iamthif_cl);
	if (ret < 0)
		return ret;

	if (ret && dev->mei_host_buffer_is_empty) {
		ret = 0;
		dev->mei_host_buffer_is_empty = false;
		if (cb->request_buffer.size > mei_hbuf_max_data(dev)) {
			mei_hdr.length = mei_hbuf_max_data(dev);
			mei_hdr.msg_complete = 0;
		} else {
			mei_hdr.length = cb->request_buffer.size;
			mei_hdr.msg_complete = 1;
		}

		mei_hdr.host_addr = dev->iamthif_cl.host_client_id;
		mei_hdr.me_addr = dev->iamthif_cl.me_client_id;
		mei_hdr.reserved = 0;
		dev->iamthif_msg_buf_index += mei_hdr.length;
		if (mei_write_message(dev, &mei_hdr,
					(unsigned char *)(dev->iamthif_msg_buf),
					mei_hdr.length))
			return -ENODEV;

		if (mei_hdr.msg_complete) {
			if (mei_flow_ctrl_reduce(dev, &dev->iamthif_cl))
				return -ENODEV;
			dev->iamthif_flow_control_pending = true;
			dev->iamthif_state = MEI_IAMTHIF_FLOW_CONTROL;
			dev_dbg(&dev->pdev->dev, "add amthi cb to write waiting list\n");
			dev->iamthif_current_cb = cb;
			dev->iamthif_file_object = cb->file_object;
			list_add_tail(&cb->list, &dev->write_waiting_list.list);
		} else {
			dev_dbg(&dev->pdev->dev, "message does not complete, "
					"so add amthi cb to write list.\n");
			list_add_tail(&cb->list, &dev->write_list.list);
		}
	} else {
		if (!(dev->mei_host_buffer_is_empty))
			dev_dbg(&dev->pdev->dev, "host buffer is not empty");

		dev_dbg(&dev->pdev->dev, "No flow control credentials, "
				"so add iamthif cb to write list.\n");
		list_add_tail(&cb->list, &dev->write_list.list);
	}
	return 0;
}

/**
 * iamthif_ioctl_send_msg - send cmd data to amthi client
 *
 * @dev: the device structure
 *
 * returns 0 on success, <0 on failure.
 */
void mei_run_next_iamthif_cmd(struct mei_device *dev)
{
	struct mei_cl *cl_tmp;
	struct mei_cl_cb *pos = NULL;
	struct mei_cl_cb *next = NULL;
	int status;

	if (!dev)
		return;

	dev->iamthif_msg_buf_size = 0;
	dev->iamthif_msg_buf_index = 0;
	dev->iamthif_canceled = false;
	dev->iamthif_ioctl = true;
	dev->iamthif_state = MEI_IAMTHIF_IDLE;
	dev->iamthif_timer = 0;
	dev->iamthif_file_object = NULL;

	dev_dbg(&dev->pdev->dev, "complete amthi cmd_list cb.\n");

	list_for_each_entry_safe(pos, next, &dev->amthi_cmd_list.list, list) {
		list_del(&pos->list);
		cl_tmp = (struct mei_cl *)pos->file_private;

		if (cl_tmp && cl_tmp == &dev->iamthif_cl) {
			status = amthi_write(dev, pos);
			if (status) {
				dev_dbg(&dev->pdev->dev,
					"amthi write failed status = %d\n",
						status);
				return;
			}
			break;
		}
	}
}

