#include <linux/rotation.h>

extern struct dev_rotation rotation;
extern struct thread_cond_t rotation_read;
extern struct thread_cond_t rotation_write;
extern struct area_descriptor rot_area;
extern spinlock_t rot_lock;


/*
 * signal의 경우 waiting_list가 비어있지 않다면,
 * wake_up시킨다. wake_up함수 자체에 reschedule함수가 존재한다.
 */
static void thread_cond_signal(struct thread_cond_t *x)
{
	wake_up(&x->wait);
}

static void thread_cond_broadcast(struct thread_cond_t *x)
{
	wake_up_all(&x->wait);
}

static unsigned long __sched thread_cond_wait(struct thread_cond_t *x, 
				    unsigned long flag, wait_queue_t *wait)
{
	unsigned long flags;
	//wait을 할 때 lock을 풀고 wait을 한다.
	spin_unlock_irqrestore(&rot_lock, flag);

	spin_lock_irqsave(&x->wait.lock, flags);
	if (list_empty(&wait->task_list)) 
		__add_wait_queue_tail(&x->wait, wait);
	set_current_state(TASK_UNINTERRUPTIBLE);
	spin_unlock_irqrestore(&x->wait.lock, flags);
	printk("i go to sleep\n");
	schedule();
	printk("haha i'm back\n");
	//wake_up해서 돌아오면 lock을 다시 잡는다.
	spin_lock_irqsave(&rot_lock, flag);
	return flag;
}

static int read_should_wait(struct rotation_range *rot)
{
	//현재 각도에 write wait or runner가 있으면
	//내각도가 아니야
	int max = rot->rot.degree + rot->degree_range + 360;
	int min = rot->rot.degree - (int)rot->degree_range + 360;
	int cur = ((min <= rotation.degree) && (rotation.degree < 360)) ?
				    rotation.degree : rotation.degree + 360;

	return (rot_area.active_writers[rotation.degree / 30] > 0 ||
		    rot_area.waiting_writers[rotation.degree /30] > 0 ||
						cur > max || cur < min);
}

static int write_should_wait(struct rotation_range *rot)
{
	//현재 각도에 write runner가 있으면
	//현재 각도에 read runner가 있으면
	//내각도가 아니야
	int max = rot->rot.degree + rot->degree_range + 360;
	int min = rot->rot.degree - (int)rot->degree_range + 360;
	int cur = ((min <= rotation.degree) && (rotation.degree < 360)) ?
				    rotation.degree : rotation.degree + 360;

	printk("active_writers = %d\n", rot_area.active_writers[rotation.degree / 30]);
	printk("active_readers = %d\n", rot_area.active_readers[rotation.degree / 30]);
	printk("current = %d min = %d max = %d\n", cur, min, max);

	return (rot_area.active_writers[rotation.degree / 30] > 0 ||
		    rot_area.active_readers[rotation.degree / 30] > 0 ||
						 cur > max || cur < min);
}


static inline void remove_read_waiter(struct rotation_range *rot)
{
	int max = (rot->rot.degree + rot->degree_range + 360) / 30;
	int min = (rot->rot.degree - (int)rot->degree_range + 360);
	int i;

	min = min % 30 ? min / 30 + 1 : min / 30;

	for (i = min; i <= max; i++) {
		rot_area.waiting_readers[i % 12] -= 1;
	}
}

static inline void add_read_waiter(struct rotation_range *rot)
{
	int max = (rot->rot.degree + rot->degree_range + 360) / 30;
	int min = (rot->rot.degree - (int)rot->degree_range + 360);
	int i;

	min = min % 30 ? min / 30 + 1 : min / 30;

	//printk("min = %d max = %d\n", min, max);

	for (i = min; i <= max; i++) {
		rot_area.waiting_readers[i % 12] += 1;
		//printk("%d\n", rot_area.waiting_readers[i % 12]);
	}
}

static inline void remove_write_waiter(struct rotation_range *rot)
{	
	int max = (rot->rot.degree + rot->degree_range + 360) / 30;
	int min = (rot->rot.degree - (int)rot->degree_range + 360);
	int i;

	min = min % 30 ? min / 30 + 1 : min / 30;

	for (i = min; i <= max; i++) {
		rot_area.waiting_writers[i % 12] -= 1;
	}
}

static inline void add_write_waiter(struct rotation_range *rot)
{	
	int max = (rot->rot.degree + rot->degree_range + 360) / 30;
	int min = (rot->rot.degree - (int)rot->degree_range + 360);
	int i;

	min = min % 30 ? min / 30 + 1 : min / 30;

	for (i = min; i <= max; i++) {
		rot_area.waiting_writers[i % 12] += 1;
	}
}

static inline void remove_read_runner(struct rotation_range *rot)
{	
	int max = (rot->rot.degree + rot->degree_range + 360) / 30;
	int min = (rot->rot.degree - (int)rot->degree_range + 360);
	int i;

	min = min % 30 ? min / 30 + 1 : min / 30;

	for (i = min; i <= max; i++) {
		rot_area.active_readers[i % 12] -= 1;
		//printk("active_reader = %d\n", rot_area.active_readers[i % 12]);
	}
}

static inline void add_read_runner(struct rotation_range *rot)
{	
	int max = (rot->rot.degree + rot->degree_range + 360) / 30;
	int min = (rot->rot.degree - (int)rot->degree_range + 360);
	int i;

	min = min % 30 ? min / 30 + 1 : min / 30;

	for (i = min; i <= max; i++) {
		rot_area.active_readers[i % 12] += 1;
		//printk("active_reader = %d\n", rot_area.active_readers[i % 12]);
	}
}

static inline void remove_write_runner(struct rotation_range *rot)
{	
	int max = (rot->rot.degree + rot->degree_range + 360) / 30;
	int min = (rot->rot.degree - (int)rot->degree_range + 360);
	int i;

	min = min % 30 ? min / 30 + 1 : min / 30;

	for (i = min; i <= max; i++) {
		rot_area.active_writers[i % 12] -= 1;
	}
}

static inline void add_write_runner(struct rotation_range *rot)
{	
	int max = (rot->rot.degree + rot->degree_range + 360) / 30;
	int min = (rot->rot.degree - (int)rot->degree_range + 360);
	int i;

	min = min % 30 ? min / 30 + 1 : min / 30;

	for (i = min; i <= max; i++) {
		rot_area.active_writers[i % 12] += 1;
	}
}

asmlinkage int sys_set_rotation(struct dev_rotation __user *rot) 
{
	unsigned long flags;

	get_user(rotation.degree, &rot->degree);
	printk("%d\n", rotation.degree);

	spin_lock_irqsave(&rot_lock, flags);
	if (rot_area.waiting_writers[rotation.degree / 30] > 0) {
		printk("guys it's time to wake up\n");
		thread_cond_broadcast(&rotation_write);
	} else if (rot_area.waiting_readers[rotation.degree / 30] > 0) {
		thread_cond_broadcast(&rotation_read);
		printk("reader it's time to wake up\n");
	}
	spin_unlock_irqrestore(&rot_lock, flags);
	return 0;
}

asmlinkage int sys_rotlock_read(struct rotation_range __user *rot)
{
	struct rotation_range krot;
	unsigned long flags;
	DEFINE_WAIT(wait);
	wait.flags &= ~WQ_FLAG_EXCLUSIVE;

	get_user(krot, rot);

	spin_lock_irqsave(&rot_lock, flags);
	add_read_waiter(&krot);
	printk("i add read waiter\n");
	while (read_should_wait(&krot)) {
		flags = thread_cond_wait(&rotation_read, flags, &wait);
	}
	remove_read_waiter(&krot);
	add_read_runner(&krot);
	//finish_wait(&rotation_read.wait, &wait);
	__remove_wait_queue(&rotation_read.wait, &wait);
	spin_unlock_irqrestore(&rot_lock, flags);
	return 0;
}

asmlinkage int sys_rotlock_write(struct rotation_range __user *rot)
{
	struct rotation_range krot;
	unsigned long flags;
	DEFINE_WAIT(wait);
	wait.flags &= ~WQ_FLAG_EXCLUSIVE;

	get_user(krot, rot);

	spin_lock_irqsave(&rot_lock, flags);
	add_write_waiter(&krot);
	while (write_should_wait(&krot)) {
		flags = thread_cond_wait(&rotation_write, flags, &wait);
	}
	remove_write_waiter(&krot);
	add_write_runner(&krot);
	//finish_wait(&rotation_write.wait, &wait);
	__remove_wait_queue(&rotation_write.wait, &wait);
	printk("yeah i wake up and num of waiting writer = %d\n",
		    rot_area.waiting_writers[rotation.degree / 30]);
	spin_unlock_irqrestore(&rot_lock, flags);
	return 0;
}

asmlinkage int sys_rotunlock_read(struct rotation_range __user *rot)
{
	struct rotation_range krot;
	unsigned long flags;
	int max = rot->rot.degree + rot->degree_range + 360;
	int min = rot->rot.degree - (int)rot->degree_range + 360;
	int cur = ((min <= rotation.degree) && (rotation.degree < 360)) ?
				    rotation.degree : rotation.degree + 360;

	get_user(krot, rot);

	spin_lock_irqsave(&rot_lock, flags);
	remove_read_runner(&krot);
	if (cur <= max && cur >= min) {
		if (rot_area.active_readers[rotation.degree / 30] == 0 &&
			rot_area.waiting_writers[rotation.degree / 30] > 0) {
			thread_cond_broadcast(&rotation_write);
		}
	}
	spin_unlock_irqrestore(&rot_lock, flags);
	return 0;
}

asmlinkage int sys_rotunlock_write(struct rotation_range __user *rot)
{
	struct rotation_range krot;
	unsigned long flags;
	int max = rot->rot.degree + rot->degree_range + 360;
	int min = rot->rot.degree - (int)rot->degree_range + 360;
	int cur = ((min <= rotation.degree) && (rotation.degree < 360)) ?
				    rotation.degree : rotation.degree + 360;
	get_user(krot, rot);

	spin_lock_irqsave(&rot_lock, flags);
	remove_write_runner(&krot);
	if (cur <= max && cur >= min) {
		if (rot_area.waiting_writers[rotation.degree / 30] > 0) {
			thread_cond_broadcast(&rotation_write);
			printk("hey wake up!\n");
		} else {
			thread_cond_broadcast(&rotation_read);
		}
	}
	spin_unlock_irqrestore(&rot_lock, flags);
	return 0;
}
