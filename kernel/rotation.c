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

spinlock_t lock = SPIN_LOCK_UNLOCKED;

struct thread_cond_t {
	struct completion cv;
};

thread_cond_t read ;
thread_cond_t write;

init_completion(&read.cv);
init_completion(&write.cv);

int waiting_writers;
int waiting_readers;
int active_readers;
int active_writers;
/*
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
*/


/*
 * signal의 경우 waiting_list가 비어있지 않다면,
 * wake_up시킨다. wake_up함수 자체에 reschedule함수가 존재한다.
 */
void thread_cond_signal(struct thread_cond_t *x)
{
	unsigned long flags;
	
	spin_lock_irqsave(&x->cv->wait.lock, flags);
	x->cv->done++;
	__wake_up_common(&x->cv->wait, TASK_NORMAL, 1, 0, NULL);
	spin_unlock_irqrestore(&x->cv->wait.lock, flags);
}

void thread_cond_broadcast(struct thread_cond_t *x)
{
	unsigned long flags;

	spin_lock_irqsave(&x->cv->wait.lock, flags);
	x->cv->done += UINT_MAX/2;
	__wake_up_common(&x->cv->wait, TASK_NORMAL, 0, 0, NULL);
	spin_unlock_irqrestore(&x->cv->wait.lock, flags);
}

void thread_cond_wait(struct thead_cond_t *x)
{
	unsigned long flags;	
	//check_if_lock_is_held

	//wait을 할 때 lock을 풀고 wait을 한다.
	spin_unlock_irqrestre(lock, flags);
	//wait으로 들어간다.
	wait_for_completion(&x->cv);
	//wake_up해서 돌아오면 lock을 다시 잡는다.
	spin_locK_irqsave(lock, flags);
}

extern struct dev_rotation rotation;

asmlinkage int sys_set_rotation(struct dev_rotation __user *rot) 
{
	get_user(rotation.degree, &rot->degree);
	spin_lock(&lock);
	if (waiting_writers > 0) {
		thread_cond_signal(&write);
	} else { 
		thread_cond_broadcast(&read);
	}
	spin_unlock(&lokc);
	return 0;
}

asmlinkage int sys_rotlock_read(struct rotation_range __user *rot)
{
	spin_lock(&lock);
	waiting_readers++;
	while (read_should_wait()) {
		thread_cond_wait(&read);
	}
	waiting_readers--;
	active_readers++;
	spin_unlock(&lock);
	return 0;
}
asmlinkage int sys_rotlock_write(struct rotation_range __user *rot)
{
	spin_lock(&lock);
	waiting_writers++;
	while (write_should_wait()) {
		thread_cond_wait(&write);
	}
	waiting_writers--;
	active_writers++;
	spin_unlock(&lock);
	return 0;
}

asmlinkage int sys_rotunlock_read(struct rotation_range __user *rot)
{
	spin_lock(&lock);
	actvie_readers--;
	if (actvie_readers == 0 && waiting_writers > 0) {
		thread_cond_signal(&write);
	}
	spin_unlock(&lock);
	return 0;
}

asmlinkage int sys_rotunlock_write(struct rotation_range __user *rot)
{
	spin_lock(&lock);
	active_writers--;
	if (waiting_writers > 0) {
		thread_cond_signal(&write);
	} else {
		thread_cond_broadcast(&read);
	}
	return 0;
}
