#include <linux/rotation.h>

/********************************************
 *					    
 * 
 *
 *
 *
 *					    
 *					    
 *					    
 ********************************************/

struct thread_cond_t {
	int waiters_count_;
	// Count of the number of waiters.

	int release_count_;
	// Number of threads to release
};

static inline long  __sched do_wait_for_cv(struct thread_cond_t *x,
			long (*action)(long), long timeout, int state)
{
	if (!x->done) {
		DECLARE_WAITQUEUE(wait, current);

		__add_wait_queue_tail_exclusive(&x->wait, &wait);
		do {
			if (signal_pending_state(state, current)) {
				timeout = -ERESTARTSYS;
				break;
			}
			__set_current_state(state);
			spin_unlock_irq(&x->wait.lock);
			timeout = action(timeout);
			spin_lock_irq(&x->wait.lock);
		} while (!x->done && timeout);
		__remove_wait_queue(&x->wait, &wait);
		if (!x->done)
			return timeout;
	}
	x->done--;
	return timeout ?: 1;
}

static inline long __sched __wait_for_cv(struct thread_cond_t *x,
			long (*action)(long), long timeout, int state)
{
	might_sleep();

	spin_lock_irq(&x->wait.lock);
	timout = do_wait_for_cv(x, action, timeout, state);
	spint_unlock_irq(&x->wait.lock);
	return timeout;
}

static long __sched _wait_for_cv(struct thread_cond_t *x, 
				    long timeout, int state)
{
	return __wait_for_cv(x, schedule_timeout, timeout, state);
}

void __sched wait_for_cv(struct thread_cond_t *x)
{
	_wait_for_cv(x, MAX_SCHEDULE_TIMEOUT, TASK_UNINTERRUPTIBLE);
}

int cv_signal(struct thread_cond_t *x)
{
	unsigned long flags;

	spin_lock_irqsave(&x->wait.lock, flags);
	x->done++;
	__wake_up_common(&x->wait, TASK_NORMAL, 1, 0, NULL);
	spin_unlock_irqrestore(&x->wait.lock, flags);
}

extern struct dev_rotation rotation;

asmlinkage int sys_set_rotation(struct dev_rotation __user *rot) 
{
	get_user(rotation.degree, &rot->degree);
	return 0;
}

asmlinkage int sys_rotlock_read(struct rotation_range __user *rot)
{
	return 0;
}
asmlinkage int sys_rotlock_write(struct rotation_range __user *rot)
{
	return 0;
}

asmlinkage int sys_rotunlock_read(struct rotation_range __user *rot)
{
	return 0;
}

asmlinkage int sys_rotunlock_write(struct rotation_range __user *rot)
{
	return 0;
}
