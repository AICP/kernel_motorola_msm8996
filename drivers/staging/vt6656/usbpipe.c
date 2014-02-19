/*
 * Copyright (c) 1996, 2003 VIA Networking Technologies, Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *
 * File: usbpipe.c
 *
 * Purpose: Handle USB control endpoint
 *
 * Author: Warren Hsu
 *
 * Date: Mar. 29, 2005
 *
 * Functions:
 *      CONTROLnsRequestOut - Write variable length bytes to MEM/BB/MAC/EEPROM
 *      CONTROLnsRequestIn - Read variable length bytes from MEM/BB/MAC/EEPROM
 *      ControlvWriteByte - Write one byte to MEM/BB/MAC/EEPROM
 *      ControlvReadByte - Read one byte from MEM/BB/MAC/EEPROM
 *      ControlvMaskByte - Read one byte from MEM/BB/MAC/EEPROM and clear/set some bits in the same address
 *
 * Revision History:
 *      04-05-2004 Jerry Chen:  Initial release
 *      11-24-2004 Warren Hsu: Add ControlvWriteByte,ControlvReadByte,ControlvMaskByte
 *
 */

#include "int.h"
#include "rxtx.h"
#include "dpc.h"
#include "control.h"
#include "desc.h"
#include "device.h"

//endpoint def
//endpoint 0: control
//endpoint 1: interrupt
//endpoint 2: read bulk
//endpoint 3: write bulk

//static int          msglevel                =MSG_LEVEL_DEBUG;
static int          msglevel                =MSG_LEVEL_INFO;

#define USB_CTL_WAIT   500 //ms

#ifndef URB_ASYNC_UNLINK
#define URB_ASYNC_UNLINK    0
#endif

static void s_nsInterruptUsbIoCompleteRead(struct urb *urb);
static void s_nsBulkInUsbIoCompleteRead(struct urb *urb);
static void s_nsBulkOutIoCompleteWrite(struct urb *urb);
static void s_nsControlInUsbIoCompleteRead(struct urb *urb);
static void s_nsControlInUsbIoCompleteWrite(struct urb *urb);

int PIPEnsControlOutAsyn(struct vnt_private *pDevice, u8 byRequest,
	u16 wValue, u16 wIndex, u16 wLength, u8 *pbyBuffer)
{
	int ntStatus;

    if (pDevice->Flags & fMP_DISCONNECTED)
        return STATUS_FAILURE;

    if (pDevice->Flags & fMP_CONTROL_WRITES)
        return STATUS_FAILURE;

    if (in_interrupt()) {
        DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO"in_interrupt return ..byRequest %x\n", byRequest);
        return STATUS_FAILURE;
    }

    ntStatus = usb_control_msg(
                            pDevice->usb,
                            usb_sndctrlpipe(pDevice->usb , 0),
                            byRequest,
                            0x40, // RequestType
                            wValue,
                            wIndex,
			    (void *) pbyBuffer,
                            wLength,
                            HZ
                          );
    if (ntStatus >= 0) {
        DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO"usb_sndctrlpipe ntStatus= %d\n", ntStatus);
        ntStatus = 0;
    } else {
        DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO"usb_sndctrlpipe fail, ntStatus= %d\n", ntStatus);
    }

    return ntStatus;
}

int PIPEnsControlOut(struct vnt_private *pDevice, u8 byRequest, u16 wValue,
		u16 wIndex, u16 wLength, u8 *pbyBuffer)
{
	int ntStatus = 0;
	int ii;

    if (pDevice->Flags & fMP_DISCONNECTED)
        return STATUS_FAILURE;

    if (pDevice->Flags & fMP_CONTROL_WRITES)
        return STATUS_FAILURE;

	if (pDevice->Flags & fMP_CONTROL_READS)
		return STATUS_FAILURE;

	if (pDevice->pControlURB->hcpriv)
		return STATUS_FAILURE;

	MP_SET_FLAG(pDevice, fMP_CONTROL_WRITES);

	pDevice->sUsbCtlRequest.bRequestType = 0x40;
	pDevice->sUsbCtlRequest.bRequest = byRequest;
	pDevice->sUsbCtlRequest.wValue = cpu_to_le16p(&wValue);
	pDevice->sUsbCtlRequest.wIndex = cpu_to_le16p(&wIndex);
	pDevice->sUsbCtlRequest.wLength = cpu_to_le16p(&wLength);
	pDevice->pControlURB->transfer_flags |= URB_ASYNC_UNLINK;
    pDevice->pControlURB->actual_length = 0;
    // Notice, pbyBuffer limited point to variable buffer, can't be constant.
  	usb_fill_control_urb(pDevice->pControlURB, pDevice->usb,
			 usb_sndctrlpipe(pDevice->usb , 0), (char *) &pDevice->sUsbCtlRequest,
			 pbyBuffer, wLength, s_nsControlInUsbIoCompleteWrite, pDevice);

	ntStatus = usb_submit_urb(pDevice->pControlURB, GFP_ATOMIC);
	if (ntStatus != 0) {
		DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO
			"control send request submission failed: %d\n",
				ntStatus);
		MP_CLEAR_FLAG(pDevice, fMP_CONTROL_WRITES);
		return STATUS_FAILURE;
	}

	spin_unlock_irq(&pDevice->lock);
    for (ii = 0; ii <= USB_CTL_WAIT; ii ++) {

	if (pDevice->Flags & fMP_CONTROL_WRITES)
		mdelay(1);
        else
		break;

        if (ii >= USB_CTL_WAIT) {
		DBG_PRT(MSG_LEVEL_DEBUG,
			KERN_INFO "control send request submission timeout\n");
            spin_lock_irq(&pDevice->lock);
            MP_CLEAR_FLAG(pDevice, fMP_CONTROL_WRITES);
            return STATUS_FAILURE;
        }
    }
	spin_lock_irq(&pDevice->lock);

    return STATUS_SUCCESS;
}

int PIPEnsControlIn(struct vnt_private *pDevice, u8 byRequest, u16 wValue,
	u16 wIndex, u16 wLength,  u8 *pbyBuffer)
{
	int ntStatus = 0;
	int ii;

    if (pDevice->Flags & fMP_DISCONNECTED)
        return STATUS_FAILURE;

    if (pDevice->Flags & fMP_CONTROL_READS)
	return STATUS_FAILURE;

	if (pDevice->Flags & fMP_CONTROL_WRITES)
		return STATUS_FAILURE;

	if (pDevice->pControlURB->hcpriv)
		return STATUS_FAILURE;

	MP_SET_FLAG(pDevice, fMP_CONTROL_READS);

	pDevice->sUsbCtlRequest.bRequestType = 0xC0;
	pDevice->sUsbCtlRequest.bRequest = byRequest;
	pDevice->sUsbCtlRequest.wValue = cpu_to_le16p(&wValue);
	pDevice->sUsbCtlRequest.wIndex = cpu_to_le16p(&wIndex);
	pDevice->sUsbCtlRequest.wLength = cpu_to_le16p(&wLength);
	pDevice->pControlURB->transfer_flags |= URB_ASYNC_UNLINK;
    pDevice->pControlURB->actual_length = 0;
	usb_fill_control_urb(pDevice->pControlURB, pDevice->usb,
			 usb_rcvctrlpipe(pDevice->usb , 0), (char *) &pDevice->sUsbCtlRequest,
			 pbyBuffer, wLength, s_nsControlInUsbIoCompleteRead, pDevice);

	ntStatus = usb_submit_urb(pDevice->pControlURB, GFP_ATOMIC);
	if (ntStatus != 0) {
		DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO
			"control request submission failed: %d\n", ntStatus);
		MP_CLEAR_FLAG(pDevice, fMP_CONTROL_READS);
		return STATUS_FAILURE;
	}

	spin_unlock_irq(&pDevice->lock);
    for (ii = 0; ii <= USB_CTL_WAIT; ii ++) {

	if (pDevice->Flags & fMP_CONTROL_READS)
		mdelay(1);
	else
		break;

	if (ii >= USB_CTL_WAIT) {
		DBG_PRT(MSG_LEVEL_DEBUG,
			KERN_INFO "control rcv request submission timeout\n");
            spin_lock_irq(&pDevice->lock);
            MP_CLEAR_FLAG(pDevice, fMP_CONTROL_READS);
            return STATUS_FAILURE;
        }
    }
	spin_lock_irq(&pDevice->lock);

    return ntStatus;
}

static void s_nsControlInUsbIoCompleteWrite(struct urb *urb)
{
	struct vnt_private *pDevice = (struct vnt_private *)urb->context;

	pDevice = urb->context;
	switch (urb->status) {
	case 0:
		break;
	case -EINPROGRESS:
		DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO"ctrl write urb status EINPROGRESS%d\n", urb->status);
		break;
	case -ENOENT:
		DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO"ctrl write urb status ENOENT %d\n", urb->status);
		break;
	default:
		DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO"ctrl write urb status %d\n", urb->status);
	}

    MP_CLEAR_FLAG(pDevice, fMP_CONTROL_WRITES);
}

/*
 * Description:
 *      Complete function of usb Control callback
 *
 * Parameters:
 *  In:
 *      pDevice     - Pointer to the adapter
 *
 *  Out:
 *      none
 *
 * Return Value: STATUS_INSUFFICIENT_RESOURCES or result of IoCallDriver
 *
 */

static void s_nsControlInUsbIoCompleteRead(struct urb *urb)
{
	struct vnt_private *pDevice = (struct vnt_private *)urb->context;

	switch (urb->status) {
	case 0:
		break;
	case -EINPROGRESS:
		DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO"ctrl read urb status EINPROGRESS%d\n", urb->status);
		break;
	case -ENOENT:
		DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO"ctrl read urb status = ENOENT %d\n", urb->status);
		break;
	default:
		DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO"ctrl read urb status %d\n", urb->status);
	}

    MP_CLEAR_FLAG(pDevice, fMP_CONTROL_READS);
}

/*
 * Description:
 *      Allocates an usb interrupt in irp and calls USBD.
 *
 * Parameters:
 *  In:
 *      pDevice     - Pointer to the adapter
 *  Out:
 *      none
 *
 * Return Value: STATUS_INSUFFICIENT_RESOURCES or result of IoCallDriver
 *
 */

int PIPEnsInterruptRead(struct vnt_private *priv)
{
	int status = STATUS_FAILURE;

	DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO
			"---->s_nsStartInterruptUsbRead()\n");

	if (priv->int_buf.in_use == true)
		return STATUS_FAILURE;

	priv->int_buf.in_use = true;
	priv->ulIntInPosted++;

	usb_fill_int_urb(priv->pInterruptURB,
		priv->usb,
		usb_rcvbulkpipe(priv->usb, 1),
		priv->int_buf.data_buf,
		MAX_INTERRUPT_SIZE,
		s_nsInterruptUsbIoCompleteRead,
		priv,
		priv->int_interval);

	status = usb_submit_urb(priv->pInterruptURB, GFP_ATOMIC);
	if (status) {
		DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO
			"Submit int URB failed %d\n", status);
		priv->int_buf.in_use = false;
	}

	DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO
		"<----s_nsStartInterruptUsbRead Return(%x)\n", status);

	return status;
}

/*
 * Description:
 *      Complete function of usb interrupt in irp.
 *
 * Parameters:
 *  In:
 *      pDevice     - Pointer to the adapter
 *
 *  Out:
 *      none
 *
 * Return Value: STATUS_INSUFFICIENT_RESOURCES or result of IoCallDriver
 *
 */

static void s_nsInterruptUsbIoCompleteRead(struct urb *urb)
{
	struct vnt_private *priv = (struct vnt_private *)urb->context;
	int status;

	DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO
			"---->s_nsInterruptUsbIoCompleteRead\n");

	switch (urb->status) {
	case 0:
	case -ETIMEDOUT:
		break;
	case -ECONNRESET:
	case -ENOENT:
	case -ESHUTDOWN:
		priv->int_buf.in_use = false;
		return;
	default:
		break;
	}

	status = urb->status;

	DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO
		"s_nsInterruptUsbIoCompleteRead Status %d\n", status);

	if (status != STATUS_SUCCESS) {
		priv->ulBulkInError++;
		priv->int_buf.in_use = false;

		DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO
			"IntUSBIoCompleteControl STATUS = %d\n", status);
	} else {
		priv->ulIntInBytesRead += (unsigned long)urb->actual_length;
		priv->ulIntInContCRCError = 0;
		priv->bEventAvailable = true;
		INTnsProcessData(priv);
	}

	status = usb_submit_urb(priv->pInterruptURB, GFP_ATOMIC);
	if (status) {
		DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO
			"Submit int URB failed %d\n", status);
	} else {
		priv->int_buf.in_use = true;
	}

	return;
}

/*
 * Description:
 *      Allocates an usb BulkIn  irp and calls USBD.
 *
 * Parameters:
 *  In:
 *      pDevice     - Pointer to the adapter
 *  Out:
 *      none
 *
 * Return Value: STATUS_INSUFFICIENT_RESOURCES or result of IoCallDriver
 *
 */

int PIPEnsBulkInUsbRead(struct vnt_private *pDevice, struct vnt_rcb *pRCB)
{
	int ntStatus = 0;
	struct urb *pUrb;

    DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO"---->s_nsStartBulkInUsbRead\n");

    if (pDevice->Flags & fMP_DISCONNECTED)
        return STATUS_FAILURE;

    pDevice->ulBulkInPosted++;

	pUrb = pRCB->pUrb;
    //
    // Now that we have created the urb, we will send a
    // request to the USB device object.
    //
    if (pRCB->skb == NULL) {
        DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO"pRCB->skb is null \n");
        return ntStatus;
    }

	usb_fill_bulk_urb(pUrb,
		pDevice->usb,
		usb_rcvbulkpipe(pDevice->usb, 2),
		(void *) (pRCB->skb->data),
		MAX_TOTAL_SIZE_WITH_ALL_HEADERS,
		s_nsBulkInUsbIoCompleteRead,
		pRCB);

	ntStatus = usb_submit_urb(pUrb, GFP_ATOMIC);
	if (ntStatus != 0) {
		DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO"Submit Rx URB failed %d\n", ntStatus);
		return STATUS_FAILURE ;
	}
    pRCB->Ref = 1;
    pRCB->bBoolInUse= true;

    return ntStatus;
}

/*
 * Description:
 *      Complete function of usb BulkIn irp.
 *
 * Parameters:
 *  In:
 *      pDevice     - Pointer to the adapter
 *
 *  Out:
 *      none
 *
 * Return Value: STATUS_INSUFFICIENT_RESOURCES or result of IoCallDriver
 *
 */

static void s_nsBulkInUsbIoCompleteRead(struct urb *urb)
{
	struct vnt_rcb *pRCB = (struct vnt_rcb *)urb->context;
	struct vnt_private *pDevice = pRCB->pDevice;
	unsigned long   bytesRead;
	int bIndicateReceive = false;
	int bReAllocSkb = false;
	int status;

    DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO"---->s_nsBulkInUsbIoCompleteRead\n");
    status = urb->status;
    bytesRead = urb->actual_length;

    if (status) {
        pDevice->ulBulkInError++;
        DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO"BULK In failed %d\n", status);
//todo...xxxxxx
//        if (status == USBD_STATUS_CRC) {
//            pDevice->ulBulkInContCRCError++;
//        }
//        if (status == STATUS_DEVICE_NOT_CONNECTED )
//        {
//            MP_SET_FLAG(pDevice, fMP_DISCONNECTED);
//        }
    } else {
	if (bytesRead)
		bIndicateReceive = true;
        pDevice->ulBulkInContCRCError = 0;
        pDevice->ulBulkInBytesRead += bytesRead;
    }

    if (bIndicateReceive) {
        spin_lock(&pDevice->lock);
        if (RXbBulkInProcessData(pDevice, pRCB, bytesRead) == true)
            bReAllocSkb = true;
        spin_unlock(&pDevice->lock);
    }
    pRCB->Ref--;
    if (pRCB->Ref == 0)
    {
        DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO"RxvFreeNormal %d \n",pDevice->NumRecvFreeList);
        spin_lock(&pDevice->lock);
        RXvFreeRCB(pRCB, bReAllocSkb);
        spin_unlock(&pDevice->lock);
    }

    return;
}

/*
 * Description:
 *      Allocates an usb BulkOut  irp and calls USBD.
 *
 * Parameters:
 *  In:
 *      pDevice     - Pointer to the adapter
 *  Out:
 *      none
 *
 * Return Value: STATUS_INSUFFICIENT_RESOURCES or result of IoCallDriver
 *
 */

int PIPEnsSendBulkOut(struct vnt_private *pDevice,
				struct vnt_usb_send_context *pContext)
{
	int status;
	struct urb          *pUrb;

    pDevice->bPWBitOn = false;

/*
    if (pDevice->pPendingBulkOutContext != NULL) {
        pDevice->NumContextsQueued++;
        EnqueueContext(pDevice->FirstTxContextQueue, pDevice->LastTxContextQueue, pContext);
        status = STATUS_PENDING;
        DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO"Send pending!\n");
        return status;
    }
*/

    DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO"s_nsSendBulkOut\n");

    if (MP_IS_READY(pDevice) && (pDevice->Flags & fMP_POST_WRITES)) {

        pUrb = pContext->pUrb;
        pDevice->ulBulkOutPosted++;
//        pDevice->pPendingBulkOutContext = pContext;
        usb_fill_bulk_urb(
        	    pUrb,
        		pDevice->usb,
		    usb_sndbulkpipe(pDevice->usb, 3),
		    (void *) &(pContext->Data[0]),
        		pContext->uBufLen,
        		s_nsBulkOutIoCompleteWrite,
        		pContext);

    	status = usb_submit_urb(pUrb, GFP_ATOMIC);
    	if (status != 0)
    	{
    		DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO"Submit Tx URB failed %d\n", status);
		pContext->bBoolInUse = false;
    		return STATUS_FAILURE;
    	}
        return STATUS_PENDING;
    }
    else {
        pContext->bBoolInUse = false;
        return STATUS_RESOURCES;
    }
}

/*
 * Description: s_nsBulkOutIoCompleteWrite
 *     1a) Indicate to the protocol the status of the write.
 *     1b) Return ownership of the packet to the protocol.
 *
 *     2)  If any more packets are queue for sending, send another packet
 *         to USBD.
 *         If the attempt to send the packet to the driver fails,
 *         return ownership of the packet to the protocol and
 *         try another packet (until one succeeds).
 *
 * Parameters:
 *  In:
 *      pdoUsbDevObj  - pointer to the USB device object which
 *                      completed the irp
 *      pIrp          - the irp which was completed by the
 *                      device object
 *      pContext      - the context given to IoSetCompletionRoutine
 *                      before calling IoCallDriver on the irp
 *                      The pContext is a pointer to the USB device object.
 *  Out:
 *      none
 *
 * Return Value: STATUS_MORE_PROCESSING_REQUIRED - allows the completion routine
 *               (IofCompleteRequest) to stop working on the irp.
 *
 */

static void s_nsBulkOutIoCompleteWrite(struct urb *urb)
{
	struct vnt_usb_send_context *context =
			(struct vnt_usb_send_context *)urb->context;
	struct vnt_private *priv = context->pDevice;
	CONTEXT_TYPE context_type = context->Type;
	unsigned long buf_len = context->uBufLen;
	int status;

	DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO"---->s_nsBulkOutIoCompleteWrite\n");

	switch (urb->status) {
	case 0:
	case -ETIMEDOUT:
		break;
	case -ECONNRESET:
	case -ENOENT:
	case -ESHUTDOWN:
		context->bBoolInUse = false;
		return;
	default:
		break;
	}

	if (!netif_device_present(priv->dev))
		return;


	status = urb->status;

	if (status == STATUS_SUCCESS) {
		DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO
			"Write %d bytes\n", (int)buf_len);
		priv->ulBulkOutBytesWrite += buf_len;
		priv->ulBulkOutContCRCError = 0;
	} else {
		DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO
				"BULK Out failed %d\n", status);
		priv->ulBulkOutError++;
	}


	if (CONTEXT_DATA_PACKET == context_type) {
		if (context->pPacket != NULL) {
			dev_kfree_skb_irq(context->pPacket);
			context->pPacket = NULL;
			DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO
				"tx  %d bytes\n", (int)buf_len);
		}

		priv->dev->trans_start = jiffies;

		if (status == STATUS_SUCCESS) {
			priv->packetsSent++;
		} else {
			DBG_PRT(MSG_LEVEL_DEBUG, KERN_INFO
				"Send USB error! [%08xh]\n", status);
			priv->packetsSentDropped++;
		}

	}

	if (priv->bLinkPass == true) {
		if (netif_queue_stopped(priv->dev))
			netif_wake_queue(priv->dev);
	}

	context->bBoolInUse = false;

	return;
}
