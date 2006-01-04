/*
 * SPU file system
 *
 * (C) Copyright IBM Deutschland Entwicklung GmbH 2005
 *
 * Author: Arnd Bergmann <arndb@de.ibm.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef SPUFS_H
#define SPUFS_H

#include <linux/kref.h>
#include <linux/rwsem.h>
#include <linux/spinlock.h>
#include <linux/fs.h>

#include <asm/spu.h>
#include <asm/spu_csa.h>

/* The magic number for our file system */
enum {
	SPUFS_MAGIC = 0x23c9b64e,
};

struct spu_context_ops;

#define SPU_CONTEXT_PREEMPT_nr          0UL
#define SPU_CONTEXT_PREEMPT             (1UL << SPU_CONTEXT_PREEMPT_nr)

struct spu_context {
	struct spu *spu;		  /* pointer to a physical SPU */
	struct spu_state csa;		  /* SPU context save area. */
	spinlock_t mmio_lock;		  /* protects mmio access */
	struct address_space *local_store;/* local store backing store */

	enum { SPU_STATE_RUNNABLE, SPU_STATE_SAVED } state;
	struct rw_semaphore state_sema;

	struct mm_struct *owner;

	struct kref kref;
	wait_queue_head_t ibox_wq;
	wait_queue_head_t wbox_wq;
	wait_queue_head_t stop_wq;
	struct fasync_struct *ibox_fasync;
	struct fasync_struct *wbox_fasync;
	struct spu_context_ops *ops;
	struct work_struct reap_work;
	u64 flags;
};

/* SPU context query/set operations. */
struct spu_context_ops {
	int (*mbox_read) (struct spu_context * ctx, u32 * data);
	 u32(*mbox_stat_read) (struct spu_context * ctx);
	unsigned int (*mbox_stat_poll)(struct spu_context *ctx,
					unsigned int events);
	int (*ibox_read) (struct spu_context * ctx, u32 * data);
	int (*wbox_write) (struct spu_context * ctx, u32 data);
	 u32(*signal1_read) (struct spu_context * ctx);
	void (*signal1_write) (struct spu_context * ctx, u32 data);
	 u32(*signal2_read) (struct spu_context * ctx);
	void (*signal2_write) (struct spu_context * ctx, u32 data);
	void (*signal1_type_set) (struct spu_context * ctx, u64 val);
	 u64(*signal1_type_get) (struct spu_context * ctx);
	void (*signal2_type_set) (struct spu_context * ctx, u64 val);
	 u64(*signal2_type_get) (struct spu_context * ctx);
	 u32(*npc_read) (struct spu_context * ctx);
	void (*npc_write) (struct spu_context * ctx, u32 data);
	 u32(*status_read) (struct spu_context * ctx);
	char*(*get_ls) (struct spu_context * ctx);
	void (*runcntl_write) (struct spu_context * ctx, u32 data);
	void (*runcntl_stop) (struct spu_context * ctx);
};

extern struct spu_context_ops spu_hw_ops;
extern struct spu_context_ops spu_backing_ops;

struct spufs_inode_info {
	struct spu_context *i_ctx;
	struct inode vfs_inode;
};
#define SPUFS_I(inode) \
	container_of(inode, struct spufs_inode_info, vfs_inode)

extern struct tree_descr spufs_dir_contents[];

/* system call implementation */
long spufs_run_spu(struct file *file,
		   struct spu_context *ctx, u32 *npc, u32 *status);
long spufs_create_thread(struct nameidata *nd, const char *name,
			 unsigned int flags, mode_t mode);
extern struct file_operations spufs_context_fops;

/* context management */
struct spu_context * alloc_spu_context(struct address_space *local_store);
void destroy_spu_context(struct kref *kref);
struct spu_context * get_spu_context(struct spu_context *ctx);
int put_spu_context(struct spu_context *ctx);
void spu_unmap_mappings(struct spu_context *ctx);

void spu_forget(struct spu_context *ctx);
void spu_acquire(struct spu_context *ctx);
void spu_release(struct spu_context *ctx);
int spu_acquire_runnable(struct spu_context *ctx);
void spu_acquire_saved(struct spu_context *ctx);

int spu_activate(struct spu_context *ctx, u64 flags);
void spu_deactivate(struct spu_context *ctx);
void spu_yield(struct spu_context *ctx);
int __init spu_sched_init(void);
void __exit spu_sched_exit(void);

size_t spu_wbox_write(struct spu_context *ctx, u32 data);
size_t spu_ibox_read(struct spu_context *ctx, u32 *data);

/* irq callback funcs. */
void spufs_ibox_callback(struct spu *spu);
void spufs_wbox_callback(struct spu *spu);
void spufs_stop_callback(struct spu *spu);

#endif
