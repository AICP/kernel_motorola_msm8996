#ifndef	__WLAN_THREAD_H_
#define	__WLAN_THREAD_H_

#include	<linux/kthread.h>

struct wlan_thread {
	struct task_struct *task;
	wait_queue_head_t waitq;
	pid_t pid;
	void *priv;
};

static inline void wlan_activate_thread(struct wlan_thread * thr)
{
	/** Record the thread pid */
	thr->pid = current->pid;

	/** Initialize the wait queue */
	init_waitqueue_head(&thr->waitq);
}

static inline void wlan_deactivate_thread(struct wlan_thread * thr)
{
	ENTER();

	thr->pid = 0;

	LEAVE();
}

static inline void wlan_create_thread(int (*wlanfunc) (void *),
				      struct wlan_thread * thr, char *name)
{
	thr->task = kthread_run(wlanfunc, thr, "%s", name);
}

static inline int wlan_terminate_thread(struct wlan_thread * thr)
{
	ENTER();

	/* Check if the thread is active or not */
	if (!thr->pid) {
		printk(KERN_ERR "Thread does not exist\n");
		return -1;
	}
	kthread_stop(thr->task);

	LEAVE();
	return 0;
}

#endif
