/*
 * Intel 5100 Memory Controllers kernel module
 *
 * This file may be distributed under the terms of the
 * GNU General Public License.
 *
 * This module is based on the following document:
 *
 * Intel 5100X Chipset Memory Controller Hub (MCH) - Datasheet
 *      http://download.intel.com/design/chipsets/datashts/318378.pdf
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/pci_ids.h>
#include <linux/slab.h>
#include <linux/edac.h>
#include <linux/delay.h>
#include <linux/mmzone.h>

#include "edac_core.h"

/* register addresses and bit field accessors... */

/* device 16, func 1 */
#define I5100_MS		0x44	/* Memory Status Register */
#define I5100_SPDDATA		0x48	/* Serial Presence Detect Status Reg */
#define		I5100_SPDDATA_RDO(a)	((a) >> 15 & 1)
#define		I5100_SPDDATA_SBE(a)	((a) >> 13 & 1)
#define		I5100_SPDDATA_BUSY(a)	((a) >> 12 & 1)
#define		I5100_SPDDATA_DATA(a)	((a)       & ((1 << 8) - 1))
#define I5100_SPDCMD		0x4c	/* Serial Presence Detect Command Reg */
#define		I5100_SPDCMD_DTI(a)	(((a) & ((1 << 4) - 1)) << 28)
#define		I5100_SPDCMD_CKOVRD(a)	(((a) & 1)              << 27)
#define		I5100_SPDCMD_SA(a)	(((a) & ((1 << 3) - 1)) << 24)
#define		I5100_SPDCMD_BA(a)	(((a) & ((1 << 8) - 1)) << 16)
#define		I5100_SPDCMD_DATA(a)	(((a) & ((1 << 8) - 1)) <<  8)
#define		I5100_SPDCMD_CMD(a)	((a) & 1)
#define I5100_TOLM		0x6c	/* Top of Low Memory */
#define		I5100_TOLM_TOLM(a)	((a) >> 12 & ((1 << 4) - 1))
#define I5100_MIR0		0x80	/* Memory Interleave Range 0 */
#define I5100_MIR1		0x84	/* Memory Interleave Range 1 */
#define I5100_AMIR_0		0x8c	/* Adjusted Memory Interleave Range 0 */
#define I5100_AMIR_1		0x90	/* Adjusted Memory Interleave Range 1 */
#define		I5100_MIR_LIMIT(a)	((a) >> 4 & ((1 << 12) - 1))
#define		I5100_MIR_WAY1(a)	((a) >> 1 & 1)
#define		I5100_MIR_WAY0(a)	((a)      & 1)
#define I5100_FERR_NF_MEM	0xa0	/* MC First Non Fatal Errors */
#define		I5100_FERR_NF_MEM_CHAN_INDX(a)	((a) >> 28 & 1)
#define		I5100_FERR_NF_MEM_SPD_MASK	(1 << 18)
#define		I5100_FERR_NF_MEM_M16ERR_MASK	(1 << 16)
#define		I5100_FERR_NF_MEM_M15ERR_MASK	(1 << 15)
#define		I5100_FERR_NF_MEM_M14ERR_MASK	(1 << 14)
#define		I5100_FERR_NF_MEM_
#define		I5100_FERR_NF_MEM_
#define		I5100_FERR_NF_MEM_ANY_MASK	\
			(I5100_FERR_NF_MEM_M16ERR_MASK | \
			I5100_FERR_NF_MEM_M15ERR_MASK | \
			I5100_FERR_NF_MEM_M14ERR_MASK)
#define		I5100_FERR_NF_MEM_ANY(a)  ((a) & I5100_FERR_NF_MEM_ANY_MASK)
#define	I5100_NERR_NF_MEM	0xa4	/* MC Next Non-Fatal Errors */
#define		I5100_NERR_NF_MEM_ANY(a)  I5100_FERR_NF_MEM_ANY(a)

/* device 21 and 22, func 0 */
#define I5100_MTR_0	0x154	/* Memory Technology Registers 0-3 */
#define I5100_DMIR	0x15c	/* DIMM Interleave Range */
#define		I5100_DMIR_LIMIT(a)	((a) >> 16 & ((1 << 11) - 1))
#define		I5100_DMIR_RANK(a, i)	((a) >> (4 * i) & ((1 <<  2) - 1))
#define I5100_MTR_4	0x1b0	/* Memory Technology Registers 4,5 */
#define		I5100_MTR_PRESENT(a)	((a) >> 10 & 1)
#define		I5100_MTR_ETHROTTLE(a)	((a) >>  9 & 1)
#define		I5100_MTR_WIDTH(a)	((a) >>  8 & 1)
#define		I5100_MTR_NUMBANK(a)	((a) >>  6 & 1)
#define		I5100_MTR_NUMROW(a)	((a) >>  2 & ((1 << 2) - 1))
#define		I5100_MTR_NUMCOL(a)	((a)       & ((1 << 2) - 1))
#define	I5100_VALIDLOG	0x18c	/* Valid Log Markers */
#define		I5100_VALIDLOG_REDMEMVALID(a)	((a) >> 2 & 1)
#define		I5100_VALIDLOG_RECMEMVALID(a)	((a) >> 1 & 1)
#define		I5100_VALIDLOG_NRECMEMVALID(a)	((a)      & 1)
#define	I5100_NRECMEMA	0x190	/* Non-Recoverable Memory Error Log Reg A */
#define		I5100_NRECMEMA_MERR(a)		((a) >> 15 & ((1 << 5) - 1))
#define		I5100_NRECMEMA_BANK(a)		((a) >> 12 & ((1 << 3) - 1))
#define		I5100_NRECMEMA_RANK(a)		((a) >>  8 & ((1 << 3) - 1))
#define		I5100_NRECMEMA_DM_BUF_ID(a)	((a)       & ((1 << 8) - 1))
#define	I5100_NRECMEMB	0x194	/* Non-Recoverable Memory Error Log Reg B */
#define		I5100_NRECMEMB_CAS(a)		((a) >> 16 & ((1 << 13) - 1))
#define		I5100_NRECMEMB_RAS(a)		((a)       & ((1 << 16) - 1))
#define	I5100_REDMEMA	0x198	/* Recoverable Memory Data Error Log Reg A */
#define		I5100_REDMEMA_SYNDROME(a)	(a)
#define	I5100_REDMEMB	0x19c	/* Recoverable Memory Data Error Log Reg B */
#define		I5100_REDMEMB_ECC_LOCATOR(a)	((a) & ((1 << 18) - 1))
#define	I5100_RECMEMA	0x1a0	/* Recoverable Memory Error Log Reg A */
#define		I5100_RECMEMA_MERR(a)		I5100_NRECMEMA_MERR(a)
#define		I5100_RECMEMA_BANK(a)		I5100_NRECMEMA_BANK(a)
#define		I5100_RECMEMA_RANK(a)		I5100_NRECMEMA_RANK(a)
#define		I5100_RECMEMA_DM_BUF_ID(a)	I5100_NRECMEMA_DM_BUF_ID(a)
#define	I5100_RECMEMB	0x1a4	/* Recoverable Memory Error Log Reg B */
#define		I5100_RECMEMB_CAS(a)		I5100_NRECMEMB_CAS(a)
#define		I5100_RECMEMB_RAS(a)		I5100_NRECMEMB_RAS(a)

/* some generic limits */
#define I5100_MAX_RANKS_PER_CTLR	6
#define I5100_MAX_CTLRS			2
#define I5100_MAX_RANKS_PER_DIMM	4
#define I5100_DIMM_ADDR_LINES		(6 - 3)	/* 64 bits / 8 bits per byte */
#define I5100_MAX_DIMM_SLOTS_PER_CTLR	4
#define I5100_MAX_RANK_INTERLEAVE	4
#define I5100_MAX_DMIRS			5

struct i5100_priv {
	/* ranks on each dimm -- 0 maps to not present -- obtained via SPD */
	int dimm_numrank[I5100_MAX_CTLRS][I5100_MAX_DIMM_SLOTS_PER_CTLR];

	/*
	 * mainboard chip select map -- maps i5100 chip selects to
	 * DIMM slot chip selects.  In the case of only 4 ranks per
	 * controller, the mapping is fairly obvious but not unique.
	 * we map -1 -> NC and assume both controllers use the same
	 * map...
	 *
	 */
	int dimm_csmap[I5100_MAX_DIMM_SLOTS_PER_CTLR][I5100_MAX_RANKS_PER_DIMM];

	/* memory interleave range */
	struct {
		u64	 limit;
		unsigned way[2];
	} mir[I5100_MAX_CTLRS];

	/* adjusted memory interleave range register */
	unsigned amir[I5100_MAX_CTLRS];

	/* dimm interleave range */
	struct {
		unsigned rank[I5100_MAX_RANK_INTERLEAVE];
		u64	 limit;
	} dmir[I5100_MAX_CTLRS][I5100_MAX_DMIRS];

	/* memory technology registers... */
	struct {
		unsigned present;	/* 0 or 1 */
		unsigned ethrottle;	/* 0 or 1 */
		unsigned width;		/* 4 or 8 bits  */
		unsigned numbank;	/* 2 or 3 lines */
		unsigned numrow;	/* 13 .. 16 lines */
		unsigned numcol;	/* 11 .. 12 lines */
	} mtr[I5100_MAX_CTLRS][I5100_MAX_RANKS_PER_CTLR];

	u64 tolm;		/* top of low memory in bytes */
	unsigned ranksperctlr;	/* number of ranks per controller */

	struct pci_dev *mc;	/* device 16 func 1 */
	struct pci_dev *ch0mm;	/* device 21 func 0 */
	struct pci_dev *ch1mm;	/* device 22 func 0 */
};

/* map a rank/ctlr to a slot number on the mainboard */
static int i5100_rank_to_slot(const struct mem_ctl_info *mci,
			      int ctlr, int rank)
{
	const struct i5100_priv *priv = mci->pvt_info;
	int i;

	for (i = 0; i < I5100_MAX_DIMM_SLOTS_PER_CTLR; i++) {
		int j;
		const int numrank = priv->dimm_numrank[ctlr][i];

		for (j = 0; j < numrank; j++)
			if (priv->dimm_csmap[i][j] == rank)
				return i * 2 + ctlr;
	}

	return -1;
}

/*
 * The processor bus memory addresses are broken into three
 * pieces, whereas the controller addresses are contiguous.
 *
 * here we map from the controller address space to the
 * processor address space:
 *
 *    Processor Address Space
 * +-----------------------------+
 * |                             |
 * |  "high" memory addresses    |
 * |                             |
 * +-----------------------------+ <- 4GB on the i5100
 * |                             |
 * |  other non-memory addresses |
 * |                             |
 * +-----------------------------+ <- top of low memory
 * |                             |
 * | "low" memory addresses      |
 * |                             |
 * +-----------------------------+
 */
static unsigned long i5100_ctl_page_to_phys(struct mem_ctl_info *mci,
					    unsigned long cntlr_addr)
{
	const struct i5100_priv *priv = mci->pvt_info;

	if (cntlr_addr < priv->tolm)
		return cntlr_addr;

	return (1ULL << 32) + (cntlr_addr - priv->tolm);
}

static const char *i5100_err_msg(unsigned err)
{
	const char *merrs[] = {
		"unknown", /* 0 */
		"uncorrectable data ECC on replay", /* 1 */
		"unknown", /* 2 */
		"unknown", /* 3 */
		"aliased uncorrectable demand data ECC", /* 4 */
		"aliased uncorrectable spare-copy data ECC", /* 5 */
		"aliased uncorrectable patrol data ECC", /* 6 */
		"unknown", /* 7 */
		"unknown", /* 8 */
		"unknown", /* 9 */
		"non-aliased uncorrectable demand data ECC", /* 10 */
		"non-aliased uncorrectable spare-copy data ECC", /* 11 */
		"non-aliased uncorrectable patrol data ECC", /* 12 */
		"unknown", /* 13 */
		"correctable demand data ECC", /* 14 */
		"correctable spare-copy data ECC", /* 15 */
		"correctable patrol data ECC", /* 16 */
		"unknown", /* 17 */
		"SPD protocol error", /* 18 */
		"unknown", /* 19 */
		"spare copy initiated", /* 20 */
		"spare copy completed", /* 21 */
	};
	unsigned i;

	for (i = 0; i < ARRAY_SIZE(merrs); i++)
		if (1 << i & err)
			return merrs[i];

	return "none";
}

/* convert csrow index into a rank (per controller -- 0..5) */
static int i5100_csrow_to_rank(const struct mem_ctl_info *mci, int csrow)
{
	const struct i5100_priv *priv = mci->pvt_info;

	return csrow % priv->ranksperctlr;
}

/* convert csrow index into a controller (0..1) */
static int i5100_csrow_to_cntlr(const struct mem_ctl_info *mci, int csrow)
{
	const struct i5100_priv *priv = mci->pvt_info;

	return csrow / priv->ranksperctlr;
}

static unsigned i5100_rank_to_csrow(const struct mem_ctl_info *mci,
				    int ctlr, int rank)
{
	const struct i5100_priv *priv = mci->pvt_info;

	return ctlr * priv->ranksperctlr + rank;
}

static void i5100_handle_ce(struct mem_ctl_info *mci,
			    int ctlr,
			    unsigned bank,
			    unsigned rank,
			    unsigned long syndrome,
			    unsigned cas,
			    unsigned ras,
			    const char *msg)
{
	const int csrow = i5100_rank_to_csrow(mci, ctlr, rank);

	printk(KERN_ERR
		"CE ctlr %d, bank %u, rank %u, syndrome 0x%lx, "
		"cas %u, ras %u, csrow %u, label \"%s\": %s\n",
		ctlr, bank, rank, syndrome, cas, ras,
		csrow, mci->csrows[csrow].channels[0].label, msg);

	mci->ce_count++;
	mci->csrows[csrow].ce_count++;
	mci->csrows[csrow].channels[0].ce_count++;
}

static void i5100_handle_ue(struct mem_ctl_info *mci,
			    int ctlr,
			    unsigned bank,
			    unsigned rank,
			    unsigned long syndrome,
			    unsigned cas,
			    unsigned ras,
			    const char *msg)
{
	const int csrow = i5100_rank_to_csrow(mci, ctlr, rank);

	printk(KERN_ERR
		"UE ctlr %d, bank %u, rank %u, syndrome 0x%lx, "
		"cas %u, ras %u, csrow %u, label \"%s\": %s\n",
		ctlr, bank, rank, syndrome, cas, ras,
		csrow, mci->csrows[csrow].channels[0].label, msg);

	mci->ue_count++;
	mci->csrows[csrow].ue_count++;
}

static void i5100_read_log(struct mem_ctl_info *mci, int ctlr,
			   u32 ferr, u32 nerr)
{
	struct i5100_priv *priv = mci->pvt_info;
	struct pci_dev *pdev = (ctlr) ? priv->ch1mm : priv->ch0mm;
	u32 dw;
	u32 dw2;
	unsigned syndrome = 0;
	unsigned ecc_loc = 0;
	unsigned merr;
	unsigned bank;
	unsigned rank;
	unsigned cas;
	unsigned ras;

	pci_read_config_dword(pdev, I5100_VALIDLOG, &dw);

	if (I5100_VALIDLOG_REDMEMVALID(dw)) {
		pci_read_config_dword(pdev, I5100_REDMEMA, &dw2);
		syndrome = I5100_REDMEMA_SYNDROME(dw2);
		pci_read_config_dword(pdev, I5100_REDMEMB, &dw2);
		ecc_loc = I5100_REDMEMB_ECC_LOCATOR(dw2);
	}

	if (I5100_VALIDLOG_RECMEMVALID(dw)) {
		const char *msg;

		pci_read_config_dword(pdev, I5100_RECMEMA, &dw2);
		merr = I5100_RECMEMA_MERR(dw2);
		bank = I5100_RECMEMA_BANK(dw2);
		rank = I5100_RECMEMA_RANK(dw2);

		pci_read_config_dword(pdev, I5100_RECMEMB, &dw2);
		cas = I5100_RECMEMB_CAS(dw2);
		ras = I5100_RECMEMB_RAS(dw2);

		/* FIXME:  not really sure if this is what merr is...
		 */
		if (!merr)
			msg = i5100_err_msg(ferr);
		else
			msg = i5100_err_msg(nerr);

		i5100_handle_ce(mci, ctlr, bank, rank, syndrome, cas, ras, msg);
	}

	if (I5100_VALIDLOG_NRECMEMVALID(dw)) {
		const char *msg;

		pci_read_config_dword(pdev, I5100_NRECMEMA, &dw2);
		merr = I5100_NRECMEMA_MERR(dw2);
		bank = I5100_NRECMEMA_BANK(dw2);
		rank = I5100_NRECMEMA_RANK(dw2);

		pci_read_config_dword(pdev, I5100_NRECMEMB, &dw2);
		cas = I5100_NRECMEMB_CAS(dw2);
		ras = I5100_NRECMEMB_RAS(dw2);

		/* FIXME:  not really sure if this is what merr is...
		 */
		if (!merr)
			msg = i5100_err_msg(ferr);
		else
			msg = i5100_err_msg(nerr);

		i5100_handle_ue(mci, ctlr, bank, rank, syndrome, cas, ras, msg);
	}

	pci_write_config_dword(pdev, I5100_VALIDLOG, dw);
}

static void i5100_check_error(struct mem_ctl_info *mci)
{
	struct i5100_priv *priv = mci->pvt_info;
	u32 dw;


	pci_read_config_dword(priv->mc, I5100_FERR_NF_MEM, &dw);
	if (I5100_FERR_NF_MEM_ANY(dw)) {
		u32 dw2;

		pci_read_config_dword(priv->mc, I5100_NERR_NF_MEM, &dw2);
		if (dw2)
			pci_write_config_dword(priv->mc, I5100_NERR_NF_MEM,
					       dw2);
		pci_write_config_dword(priv->mc, I5100_FERR_NF_MEM, dw);

		i5100_read_log(mci, I5100_FERR_NF_MEM_CHAN_INDX(dw),
			       I5100_FERR_NF_MEM_ANY(dw),
			       I5100_NERR_NF_MEM_ANY(dw2));
	}
}

static struct pci_dev *pci_get_device_func(unsigned vendor,
					   unsigned device,
					   unsigned func)
{
	struct pci_dev *ret = NULL;

	while (1) {
		ret = pci_get_device(vendor, device, ret);

		if (!ret)
			break;

		if (PCI_FUNC(ret->devfn) == func)
			break;
	}

	return ret;
}

static unsigned long __devinit i5100_npages(struct mem_ctl_info *mci,
					    int csrow)
{
	struct i5100_priv *priv = mci->pvt_info;
	const unsigned ctlr_rank = i5100_csrow_to_rank(mci, csrow);
	const unsigned ctlr = i5100_csrow_to_cntlr(mci, csrow);
	unsigned addr_lines;

	/* dimm present? */
	if (!priv->mtr[ctlr][ctlr_rank].present)
		return 0ULL;

	addr_lines =
		I5100_DIMM_ADDR_LINES +
		priv->mtr[ctlr][ctlr_rank].numcol +
		priv->mtr[ctlr][ctlr_rank].numrow +
		priv->mtr[ctlr][ctlr_rank].numbank;

	return (unsigned long)
		((unsigned long long) (1ULL << addr_lines) / PAGE_SIZE);
}

static void __devinit i5100_init_mtr(struct mem_ctl_info *mci)
{
	struct i5100_priv *priv = mci->pvt_info;
	struct pci_dev *mms[2] = { priv->ch0mm, priv->ch1mm };
	int i;

	for (i = 0; i < I5100_MAX_CTLRS; i++) {
		int j;
		struct pci_dev *pdev = mms[i];

		for (j = 0; j < I5100_MAX_RANKS_PER_CTLR; j++) {
			const unsigned addr =
				(j < 4) ? I5100_MTR_0 + j * 2 :
					  I5100_MTR_4 + (j - 4) * 2;
			u16 w;

			pci_read_config_word(pdev, addr, &w);

			priv->mtr[i][j].present = I5100_MTR_PRESENT(w);
			priv->mtr[i][j].ethrottle = I5100_MTR_ETHROTTLE(w);
			priv->mtr[i][j].width = 4 + 4 * I5100_MTR_WIDTH(w);
			priv->mtr[i][j].numbank = 2 + I5100_MTR_NUMBANK(w);
			priv->mtr[i][j].numrow = 13 + I5100_MTR_NUMROW(w);
			priv->mtr[i][j].numcol = 10 + I5100_MTR_NUMCOL(w);
		}
	}
}

/*
 * FIXME: make this into a real i2c adapter (so that dimm-decode
 * will work)?
 */
static int i5100_read_spd_byte(const struct mem_ctl_info *mci,
			       u8 ch, u8 slot, u8 addr, u8 *byte)
{
	struct i5100_priv *priv = mci->pvt_info;
	u16 w;
	u32 dw;
	unsigned long et;

	pci_read_config_word(priv->mc, I5100_SPDDATA, &w);
	if (I5100_SPDDATA_BUSY(w))
		return -1;

	dw =	I5100_SPDCMD_DTI(0xa) |
		I5100_SPDCMD_CKOVRD(1) |
		I5100_SPDCMD_SA(ch * 4 + slot) |
		I5100_SPDCMD_BA(addr) |
		I5100_SPDCMD_DATA(0) |
		I5100_SPDCMD_CMD(0);
	pci_write_config_dword(priv->mc, I5100_SPDCMD, dw);

	/* wait up to 100ms */
	et = jiffies + HZ / 10;
	udelay(100);
	while (1) {
		pci_read_config_word(priv->mc, I5100_SPDDATA, &w);
		if (!I5100_SPDDATA_BUSY(w))
			break;
		udelay(100);
	}

	if (!I5100_SPDDATA_RDO(w) || I5100_SPDDATA_SBE(w))
		return -1;

	*byte = I5100_SPDDATA_DATA(w);

	return 0;
}

/*
 * fill dimm chip select map
 *
 * FIXME:
 *   o only valid for 4 ranks per controller
 *   o not the only way to may chip selects to dimm slots
 *   o investigate if there is some way to obtain this map from the bios
 */
static void __devinit i5100_init_dimm_csmap(struct mem_ctl_info *mci)
{
	struct i5100_priv *priv = mci->pvt_info;
	int i;

	WARN_ON(priv->ranksperctlr != 4);

	for (i = 0; i < I5100_MAX_DIMM_SLOTS_PER_CTLR; i++) {
		int j;

		for (j = 0; j < I5100_MAX_RANKS_PER_DIMM; j++)
			priv->dimm_csmap[i][j] = -1; /* default NC */
	}

	/* only 2 chip selects per slot... */
	priv->dimm_csmap[0][0] = 0;
	priv->dimm_csmap[0][1] = 3;
	priv->dimm_csmap[1][0] = 1;
	priv->dimm_csmap[1][1] = 2;
	priv->dimm_csmap[2][0] = 2;
	priv->dimm_csmap[3][0] = 3;
}

static void __devinit i5100_init_dimm_layout(struct pci_dev *pdev,
					     struct mem_ctl_info *mci)
{
	struct i5100_priv *priv = mci->pvt_info;
	int i;

	for (i = 0; i < I5100_MAX_CTLRS; i++) {
		int j;

		for (j = 0; j < I5100_MAX_DIMM_SLOTS_PER_CTLR; j++) {
			u8 rank;

			if (i5100_read_spd_byte(mci, i, j, 5, &rank) < 0)
				priv->dimm_numrank[i][j] = 0;
			else
				priv->dimm_numrank[i][j] = (rank & 3) + 1;
		}
	}

	i5100_init_dimm_csmap(mci);
}

static void __devinit i5100_init_interleaving(struct pci_dev *pdev,
					      struct mem_ctl_info *mci)
{
	u16 w;
	u32 dw;
	struct i5100_priv *priv = mci->pvt_info;
	struct pci_dev *mms[2] = { priv->ch0mm, priv->ch1mm };
	int i;

	pci_read_config_word(pdev, I5100_TOLM, &w);
	priv->tolm = (u64) I5100_TOLM_TOLM(w) * 256 * 1024 * 1024;

	pci_read_config_word(pdev, I5100_MIR0, &w);
	priv->mir[0].limit = (u64) I5100_MIR_LIMIT(w) << 28;
	priv->mir[0].way[1] = I5100_MIR_WAY1(w);
	priv->mir[0].way[0] = I5100_MIR_WAY0(w);

	pci_read_config_word(pdev, I5100_MIR1, &w);
	priv->mir[1].limit = (u64) I5100_MIR_LIMIT(w) << 28;
	priv->mir[1].way[1] = I5100_MIR_WAY1(w);
	priv->mir[1].way[0] = I5100_MIR_WAY0(w);

	pci_read_config_word(pdev, I5100_AMIR_0, &w);
	priv->amir[0] = w;
	pci_read_config_word(pdev, I5100_AMIR_1, &w);
	priv->amir[1] = w;

	for (i = 0; i < I5100_MAX_CTLRS; i++) {
		int j;

		for (j = 0; j < 5; j++) {
			int k;

			pci_read_config_dword(mms[i], I5100_DMIR + j * 4, &dw);

			priv->dmir[i][j].limit =
				(u64) I5100_DMIR_LIMIT(dw) << 28;
			for (k = 0; k < I5100_MAX_RANKS_PER_DIMM; k++)
				priv->dmir[i][j].rank[k] =
					I5100_DMIR_RANK(dw, k);
		}
	}

	i5100_init_mtr(mci);
}

static void __devinit i5100_init_csrows(struct mem_ctl_info *mci)
{
	int i;
	unsigned long total_pages = 0UL;
	struct i5100_priv *priv = mci->pvt_info;

	for (i = 0; i < mci->nr_csrows; i++) {
		const unsigned long npages = i5100_npages(mci, i);
		const unsigned cntlr = i5100_csrow_to_cntlr(mci, i);
		const unsigned rank = i5100_csrow_to_rank(mci, i);

		if (!npages)
			continue;

		/*
		 * FIXME: these two are totally bogus -- I don't see how to
		 * map them correctly to this structure...
		 */
		mci->csrows[i].first_page = total_pages;
		mci->csrows[i].last_page = total_pages + npages - 1;
		mci->csrows[i].page_mask = 0UL;

		mci->csrows[i].nr_pages = npages;
		mci->csrows[i].grain = 32;
		mci->csrows[i].csrow_idx = i;
		mci->csrows[i].dtype =
			(priv->mtr[cntlr][rank].width == 4) ? DEV_X4 : DEV_X8;
		mci->csrows[i].ue_count = 0;
		mci->csrows[i].ce_count = 0;
		mci->csrows[i].mtype = MEM_RDDR2;
		mci->csrows[i].edac_mode = EDAC_SECDED;
		mci->csrows[i].mci = mci;
		mci->csrows[i].nr_channels = 1;
		mci->csrows[i].channels[0].chan_idx = 0;
		mci->csrows[i].channels[0].ce_count = 0;
		mci->csrows[i].channels[0].csrow = mci->csrows + i;
		snprintf(mci->csrows[i].channels[0].label,
			 sizeof(mci->csrows[i].channels[0].label),
			 "DIMM%u", i5100_rank_to_slot(mci, cntlr, rank));

		total_pages += npages;
	}
}

static int __devinit i5100_init_one(struct pci_dev *pdev,
				    const struct pci_device_id *id)
{
	int rc;
	struct mem_ctl_info *mci;
	struct i5100_priv *priv;
	struct pci_dev *ch0mm, *ch1mm;
	int ret = 0;
	u32 dw;
	int ranksperch;

	if (PCI_FUNC(pdev->devfn) != 1)
		return -ENODEV;

	rc = pci_enable_device(pdev);
	if (rc < 0) {
		ret = rc;
		goto bail;
	}

	/* figure out how many ranks, from strapped state of 48GB_Mode input */
	pci_read_config_dword(pdev, I5100_MS, &dw);
	ranksperch = !!(dw & (1 << 8)) * 2 + 4;

	if (ranksperch != 4) {
		/* FIXME: get 6 ranks / controller to work - need hw... */
		printk(KERN_INFO "i5100_edac: unsupported configuration.\n");
		ret = -ENODEV;
		goto bail;
	}

	/* device 21, func 0, Channel 0 Memory Map, Error Flag/Mask, etc... */
	ch0mm = pci_get_device_func(PCI_VENDOR_ID_INTEL,
				    PCI_DEVICE_ID_INTEL_5100_21, 0);
	if (!ch0mm)
		return -ENODEV;

	rc = pci_enable_device(ch0mm);
	if (rc < 0) {
		ret = rc;
		goto bail_ch0;
	}

	/* device 22, func 0, Channel 1 Memory Map, Error Flag/Mask, etc... */
	ch1mm = pci_get_device_func(PCI_VENDOR_ID_INTEL,
				    PCI_DEVICE_ID_INTEL_5100_22, 0);
	if (!ch1mm) {
		ret = -ENODEV;
		goto bail_ch0;
	}

	rc = pci_enable_device(ch1mm);
	if (rc < 0) {
		ret = rc;
		goto bail_ch1;
	}

	mci = edac_mc_alloc(sizeof(*priv), ranksperch * 2, 1, 0);
	if (!mci) {
		ret = -ENOMEM;
		goto bail_ch1;
	}

	mci->dev = &pdev->dev;

	priv = mci->pvt_info;
	priv->ranksperctlr = ranksperch;
	priv->mc = pdev;
	priv->ch0mm = ch0mm;
	priv->ch1mm = ch1mm;

	i5100_init_dimm_layout(pdev, mci);
	i5100_init_interleaving(pdev, mci);

	mci->mtype_cap = MEM_FLAG_FB_DDR2;
	mci->edac_ctl_cap = EDAC_FLAG_SECDED;
	mci->edac_cap = EDAC_FLAG_SECDED;
	mci->mod_name = "i5100_edac.c";
	mci->mod_ver = "not versioned";
	mci->ctl_name = "i5100";
	mci->dev_name = pci_name(pdev);
	mci->ctl_page_to_phys = i5100_ctl_page_to_phys;

	mci->edac_check = i5100_check_error;

	i5100_init_csrows(mci);

	/* this strange construction seems to be in every driver, dunno why */
	switch (edac_op_state) {
	case EDAC_OPSTATE_POLL:
	case EDAC_OPSTATE_NMI:
		break;
	default:
		edac_op_state = EDAC_OPSTATE_POLL;
		break;
	}

	if (edac_mc_add_mc(mci)) {
		ret = -ENODEV;
		goto bail_mc;
	}

	goto bail;

bail_mc:
	edac_mc_free(mci);

bail_ch1:
	pci_dev_put(ch1mm);

bail_ch0:
	pci_dev_put(ch0mm);

bail:
	return ret;
}

static void __devexit i5100_remove_one(struct pci_dev *pdev)
{
	struct mem_ctl_info *mci;
	struct i5100_priv *priv;

	mci = edac_mc_del_mc(&pdev->dev);

	if (!mci)
		return;

	priv = mci->pvt_info;
	pci_dev_put(priv->ch0mm);
	pci_dev_put(priv->ch1mm);

	edac_mc_free(mci);
}

static const struct pci_device_id i5100_pci_tbl[] __devinitdata = {
	/* Device 16, Function 0, Channel 0 Memory Map, Error Flag/Mask, ... */
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_5100_16) },
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, i5100_pci_tbl);

static struct pci_driver i5100_driver = {
	.name = KBUILD_BASENAME,
	.probe = i5100_init_one,
	.remove = __devexit_p(i5100_remove_one),
	.id_table = i5100_pci_tbl,
};

static int __init i5100_init(void)
{
	int pci_rc;

	pci_rc = pci_register_driver(&i5100_driver);

	return (pci_rc < 0) ? pci_rc : 0;
}

static void __exit i5100_exit(void)
{
	pci_unregister_driver(&i5100_driver);
}

module_init(i5100_init);
module_exit(i5100_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR
    ("Arthur Jones <ajones@riverbed.com>");
MODULE_DESCRIPTION("MC Driver for Intel I5100 memory controllers");
