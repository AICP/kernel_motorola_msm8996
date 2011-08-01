/*
 * QLogic iSCSI HBA Driver
 * Copyright (c) 2011 QLogic Corporation
 *
 * See LICENSE.qla4xxx for copyright and licensing details.
 */

#include "ql4_def.h"
#include "ql4_glbl.h"
#include "ql4_bsg.h"

static int
qla4xxx_read_flash(struct bsg_job *bsg_job)
{
	struct Scsi_Host *host = iscsi_job_to_shost(bsg_job);
	struct scsi_qla_host *ha = to_qla_host(host);
	struct iscsi_bsg_reply *bsg_reply = bsg_job->reply;
	struct iscsi_bsg_request *bsg_req = bsg_job->request;
	uint32_t offset = 0;
	uint32_t length = 0;
	dma_addr_t flash_dma;
	uint8_t *flash = NULL;
	int rval = -EINVAL;

	bsg_reply->reply_payload_rcv_len = 0;

	if (unlikely(pci_channel_offline(ha->pdev)))
		goto leave;

	if (ql4xxx_reset_active(ha)) {
		ql4_printk(KERN_ERR, ha, "%s: reset active\n", __func__);
		rval = -EBUSY;
		goto leave;
	}

	if (ha->flash_state != QLFLASH_WAITING) {
		ql4_printk(KERN_ERR, ha, "%s: another flash operation "
			   "active\n", __func__);
		rval = -EBUSY;
		goto leave;
	}

	ha->flash_state = QLFLASH_READING;
	offset = bsg_req->rqst_data.h_vendor.vendor_cmd[1];
	length = bsg_job->reply_payload.payload_len;

	flash = dma_alloc_coherent(&ha->pdev->dev, length, &flash_dma,
				   GFP_KERNEL);
	if (!flash) {
		ql4_printk(KERN_ERR, ha, "%s: dma alloc failed for flash "
			   "data\n", __func__);
		rval = -ENOMEM;
		goto leave;
	}

	rval = qla4xxx_get_flash(ha, flash_dma, offset, length);
	if (rval) {
		ql4_printk(KERN_ERR, ha, "%s: get flash failed\n", __func__);
		bsg_reply->result = DID_ERROR << 16;
		rval = -EIO;
	} else {
		bsg_reply->reply_payload_rcv_len =
			sg_copy_from_buffer(bsg_job->reply_payload.sg_list,
					    bsg_job->reply_payload.sg_cnt,
					    flash, length);
		bsg_reply->result = DID_OK << 16;
	}

	bsg_job_done(bsg_job, bsg_reply->result,
		     bsg_reply->reply_payload_rcv_len);
	dma_free_coherent(&ha->pdev->dev, length, flash, flash_dma);
leave:
	ha->flash_state = QLFLASH_WAITING;
	return rval;
}

static int
qla4xxx_update_flash(struct bsg_job *bsg_job)
{
	struct Scsi_Host *host = iscsi_job_to_shost(bsg_job);
	struct scsi_qla_host *ha = to_qla_host(host);
	struct iscsi_bsg_reply *bsg_reply = bsg_job->reply;
	struct iscsi_bsg_request *bsg_req = bsg_job->request;
	uint32_t length = 0;
	uint32_t offset = 0;
	uint32_t options = 0;
	dma_addr_t flash_dma;
	uint8_t *flash = NULL;
	int rval = -EINVAL;

	bsg_reply->reply_payload_rcv_len = 0;

	if (unlikely(pci_channel_offline(ha->pdev)))
		goto leave;

	if (ql4xxx_reset_active(ha)) {
		ql4_printk(KERN_ERR, ha, "%s: reset active\n", __func__);
		rval = -EBUSY;
		goto leave;
	}

	if (ha->flash_state != QLFLASH_WAITING) {
		ql4_printk(KERN_ERR, ha, "%s: another flash operation "
			   "active\n", __func__);
		rval = -EBUSY;
		goto leave;
	}

	ha->flash_state = QLFLASH_WRITING;
	length = bsg_job->request_payload.payload_len;
	offset = bsg_req->rqst_data.h_vendor.vendor_cmd[1];
	options = bsg_req->rqst_data.h_vendor.vendor_cmd[2];

	flash = dma_alloc_coherent(&ha->pdev->dev, length, &flash_dma,
				   GFP_KERNEL);
	if (!flash) {
		ql4_printk(KERN_ERR, ha, "%s: dma alloc failed for flash "
			   "data\n", __func__);
		rval = -ENOMEM;
		goto leave;
	}

	sg_copy_to_buffer(bsg_job->request_payload.sg_list,
			  bsg_job->request_payload.sg_cnt, flash, length);

	rval = qla4xxx_set_flash(ha, flash_dma, offset, length, options);
	if (rval) {
		ql4_printk(KERN_ERR, ha, "%s: set flash failed\n", __func__);
		bsg_reply->result = DID_ERROR << 16;
		rval = -EIO;
	} else
		bsg_reply->result = DID_OK << 16;

	bsg_job_done(bsg_job, bsg_reply->result,
		     bsg_reply->reply_payload_rcv_len);
	dma_free_coherent(&ha->pdev->dev, length, flash, flash_dma);
leave:
	ha->flash_state = QLFLASH_WAITING;
	return rval;
}

/**
 * qla4xxx_process_vendor_specific - handle vendor specific bsg request
 * @job: iscsi_bsg_job to handle
 **/
int qla4xxx_process_vendor_specific(struct bsg_job *bsg_job)
{
	struct iscsi_bsg_reply *bsg_reply = bsg_job->reply;
	struct iscsi_bsg_request *bsg_req = bsg_job->request;
	struct Scsi_Host *host = iscsi_job_to_shost(bsg_job);
	struct scsi_qla_host *ha = to_qla_host(host);

	switch (bsg_req->rqst_data.h_vendor.vendor_cmd[0]) {
	case QLISCSI_VND_READ_FLASH:
		return qla4xxx_read_flash(bsg_job);

	case QLISCSI_VND_UPDATE_FLASH:
		return qla4xxx_update_flash(bsg_job);

	default:
		ql4_printk(KERN_ERR, ha, "%s: invalid BSG vendor command: "
			   "0x%x\n", __func__, bsg_req->msgcode);
		bsg_reply->result = (DID_ERROR << 16);
		bsg_reply->reply_payload_rcv_len = 0;
		bsg_job_done(bsg_job, bsg_reply->result,
			     bsg_reply->reply_payload_rcv_len);
		return -ENOSYS;
	}
}

/**
 * qla4xxx_bsg_request - handle bsg request from ISCSI transport
 * @job: iscsi_bsg_job to handle
 */
int qla4xxx_bsg_request(struct bsg_job *bsg_job)
{
	struct iscsi_bsg_request *bsg_req = bsg_job->request;
	struct Scsi_Host *host = iscsi_job_to_shost(bsg_job);
	struct scsi_qla_host *ha = to_qla_host(host);

	switch (bsg_req->msgcode) {
	case ISCSI_BSG_HST_VENDOR:
		return qla4xxx_process_vendor_specific(bsg_job);

	default:
		ql4_printk(KERN_ERR, ha, "%s: invalid BSG command: 0x%x\n",
			   __func__, bsg_req->msgcode);
	}

	return -ENOSYS;
}
