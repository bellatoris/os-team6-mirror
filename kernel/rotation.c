#include <linux/rotation.h>

extern struct dev_rotation rotation;
extern struct lock_queue waiting_writer;
extern struct lock_queue acquire_writer;
extern struct lock_queue waiting_reader;
extern struct lock_queue acquire_reader;
extern spinlock_t my_lock;
extern spinlock_t glob_lock;


/*
 * signal의 경우 waiting_list가 비어있지 않다면,
 * wake_up시킨다. wake_up함수 자체에 reschedule함수가 존재한다.
 */
static void thread_cond_signal()
{	
	struct rotation_lock *curr, *next;
	spin_lock(&glob_lock)
	list_for_each_entry_safe(curr, next, &waiting_writer.lock_list, 
								lock_list) {
		if (rotation.degree <= curr->max && 
				    rotation.degree >= curr->min) {
			wake_up_process(pid_task(&curr->pid, PIDTYPE_PID));
			break;
		}
	}
	spin_unlock(&glob_lock);
}

static void thread_cond_broadcast()
{
	struct rotation_lock *curr, *next;
	spin_lock(&glob_lock)
	list_for_each_entry_safe(curr, next, &waiting_reader.lock_list, 
								lock_list) {
		if (rotation.degree <= curr->max &&
				    rotation.degree >= curr->min) {
			wake_up_process(pid_task(&curr->pid, PIDTYPE_PID));
		}
	}
	spin_unlock(&glob_lock);
}

static unsigned long __sched thread_cond_wait(unsigned long flag)
{
	//wait을 할 때 lock을 풀고 wait을 한다.
	spin_unlock_irqrestore(&my_lock, flag);

	set_current_state(TASK_INTERRUPTIBLE);
	schedule();

	//wake_up해서 돌아오면 lock을 다시 잡는다.
	spin_lock_irqsave(&my_lock, flag);
	return flag;
}

static int read_should_wait(struct rotation_lock *rot_lock)
{
	//현재 각도에 write wait or acquirer가 있으면
	//내각도가 아니야
	struct rotation_lock *curr, *next;
	int cur = ((rot_lock->min <= rotation.degree) && (rotation.degree < 360)) ?
				    rotation.degree : rotation.degree + 360;
	printk("current = %d min = %d max = %d\n", cur, 
				    rot_lock->min, rot_lock->max);
	if (cur < rot_lock->min || cur > rot_lock->max)
		return 1;

	spin_lock(&glob_lock);
	list_for_each_entry_safe(curr, next, &acquire_writer.lock_list, 
								lock_list) {
		if (rot_lock->max >= curr->min && rot_lock->min <= curr->max) {
			spin_unlock(&glob_lock);
			return 1;
		}
	}
	spin_unlock(&glob_lock);

	spin_lock(&glob_lock);
	list_for_each_entry_safe(curr, next, &waiting_writer.lock_list, 
								lock_list) {
		if (rot_lock->max >= curr->min && rot_lock->min <= curr->max) {
			spin_unlock(&glob_lock);
			return 1;
		}
	}
	spin_unlock(&glob_lock);
	
	return 0;
}

static int write_should_wait(struct rotation_lock *rot_lock)
{
	//현재 각도에 write acquirer가 있으면
	//현재 각도에 read acquirer가 있으면
	//내각도가 아니야
	struct rotation_lock *curr, *next;
	int cur = ((rot_lock->min <= rotation.degree) && (rotation.degree < 360)) ?
				    rotation.degree : rotation.degree + 360;
	printk("current = %d min = %d max = %d\n", cur, 
				    rot_lock->min, rot_lock->max);
	if (cur < rot_lock->min || cur > rot_lock->max)
		return 1;

	spin_lock(&glob_lock);
	list_for_each_entry_safe(curr, next, &acquire_writer.lock_list, 
								lock_list) {
		if (rot_lock->max >= curr->min && rot_lock->min <= curr->max) {
			spin_unlock(&glob_lock);
			return 1;
		}
	}
	spin_unlock(&glob_lock);

	spin_lock(&glob_lock);
	list_for_each_entry_safe(curr, next, &acquire_reader.lock_list, 
								lock_list) {
		if (rot_lock->max >= curr->min && rot_lock->min <= curr->max) {
			spin_unlock(&glob_lock);
			return 1;
		}
	}
	spin_unlock(&glob_lock);
	
	return 0;
}


static inline void remove_read_waiter(struct rotation_lock *rot_lock)
{
	__set_current_state(TASK_RUNNIG);
	if (!list_empty_careful(&waiting_reader.lock_list)) {
		spin_lock(&glob_lock);
		list_del_init(&rot_lock->lock_list);
		spin_unlock(&glob_lock);
	}
}

static inline void add_read_waiter(struct rotation_lock *rot_lock)
{
	spin_lock(&glob_lock);
	list_add_tail(&rot_lock->lock_list, &waiting_reader.lock_list);
	spin_unlock(&glob_lock);
}

static inline void remove_write_waiter(struct rotation_lock *rot_lock)
{	
	__set_current_state(TASK_RUNNIG);
	if (!list_empty_careful(&waiting_writer.lock_list)) {
		spin_lock(&glob_lock);
		list_del_init(&rot_lock->lock_list);
		spin_unlock(&glob_lock);
	}
}

static inline void add_write_waiter(struct rotation_lock *rot_lock)
{	
	spin_lock(&glob_lock);
	list_add_tail(&rot_lock->lock_list, &waiting_writer.lock_list);
	spin_unlock(&glob_lock);
}

static struct rotation_lock *remove_read_acquirer(struct rotation_range *rot)
{
	int max = rot->rot.degree + rot->degree_range + 360;
	int min = rot->rot.degree - (int)rot->degree_ragne + 360;
	struct rotation_lock *curr, *next;
	spin_lock(&glob_lock);
	list_for_each_entry_safe(curr, next, &acquire_reader.lock_list, 
								lock_list) {
		if (current->pid == curr->pid && max == curr->max && 
						    min == curr->min) {
			list_del_init(curr);
			break;
		}
	}
	spin_unlock(&glob_lock);
	return curr;
}

static inline void add_read_acquirer(struct rotation_lock *rot_lock)
{	
	spin_lock(&glob_lock);
	list_add_tail(&rot_lock->lock_list, &acquire_reader.lock_list);
	spin_unlock(&glob_lock);
}

static  struct rotation_lock *remove_write_acquirer(struct rotation_range *rot)
{	
	int max = rot->rot.degree + rot->degree_range + 360;
	int min = rot->rot.degree - (int)rot->degree_ragne + 360;
	struct rotation_lock *curr, *next;
	spin_lock(&glob_lock);
	list_for_each_entry_safe(curr, next, &acquire_writer.lock_list, 
								lock_list) {
		if (current->pid == curr->pid && max == curr->max && 
						    min == curr->min) {
			list_del_init(curr);
			break;
		}
	}
	spin_unlock(&glob_lock);
	return curr;
}

static inline void add_write_acquirer(struct rotation_lock *rot_lock)
{	
	spin_lock(&glob_lock);
	list_add_tail(&rot_lock->lock_list, &acquire_writer.lock_list);
	spin_unlock(&glob_lock);
}

void exit_rotlock()
{
	struct rotation_lock *curr, *next;
	spin_lock(&glob_lock);
	list_for_each_entry_safe(curr, next, &acquire_writer.lock_list, 
								lock_list) {
		if (current->pid == curr->pid) {
			list_del_init(curr);
			kfree(curr);
		}
	}
	spin_unlock(&glob_lock);

	spin_lock(&glob_lock);
	list_for_each_entry_safe(curr, next, &waiting_writer.lock_list, 
								lock_list) {
		if (current->pid == curr->pid) {
			list_del_init(curr);
			kfree(curr);
		}
	}
	spin_unlock(&glob_lock);
	
	spin_lock(&glob_lock);
	list_for_each_entry_safe(curr, next, &acquire_reader.lock_list, 
								lock_list) {
		if (current->pid == curr->pid) {
			list_del_init(curr);
			kfree(curr);
		}
	}
	spin_unlock(&glob_lock);

	spin_lock(&glob_lock);`
	list_for_each_entry_safe(curr, next, &waiting_reader.lock_list, 
								lock_list) {
		if (current->pid == curr->pid) {
			list_del_init(curr);
			kfree(curr);
		}
	}
	spin_unlock(&glob_lock);
}

asmlinkage int sys_set_rotation(struct dev_rotation __user *rot) 
{
	unsigned long flags;

	get_user(rotation.degree, &rot->degree);
	printk("%d\n", rotation.degree);
	thread_cond_signal();
	thread_cond_broadcast();
	return 0;
}

asmlinkage int sys_rotlock_read(struct rotation_range __user *rot)
{
	struct rotation_range krot;
	unsigned long flags;
	DEFINE_WAIT(wait);
	wait.flags &= ~WQ_FLAG_EXCLUSIVE;

	get_user(krot, rot);

	spin_lock_irqsave(&my_lock, flags);
	add_read_waiter(&krot);
	printk("i add read waiter\n");
	while (read_should_wait(&krot)) {
		flags = thread_cond_wait(&rotation_read, flags, &wait);
	}
	remove_read_waiter(&krot);
	add_read_acquirer(&krot);
	//finish_wait(&rotation_read.wait, &wait);
	__remove_wait_queue(&rotation_read.wait, &wait);
	spin_unlock_irqrestore(&my_lock, flags);
	return 0;
}

asmlinkage int sys_rotlock_write(struct rotation_range __user *rot)
{
	struct rotation_range krot;
	unsigned long flags;
	DEFINE_WAIT(wait);
	wait.flags &= ~WQ_FLAG_EXCLUSIVE;

	get_user(krot, rot);

	spin_lock_irqsave(&my_lock, flags);
	add_write_waiter(&krot);
	while (write_should_wait(&krot)) {
		flags = thread_cond_wait(&rotation_write, flags, &wait);
	}
	remove_write_waiter(&krot);
	add_write_acquirer(&krot);
	//finish_wait(&rotation_write.wait, &wait);
	__remove_wait_queue(&rotation_write.wait, &wait);
	printk("yeah i wake up and num of waiting writer = %d\n",
		    rot_area.waiting_writers[rotation.degree / 30]);
	spin_unlock_irqrestore(&my_lock, flags);
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

	spin_lock_irqsave(&my_lock, flags);
	remove_read_acquirer(&krot);
	if (cur <= max && cur >= min) {
		if (rot_area.active_readers[rotation.degree / 30] == 0 &&
			rot_area.waiting_writers[rotation.degree / 30] > 0) {
			thread_cond_broadcast(&rotation_write);
		}
	}
	spin_unlock_irqrestore(&my_lock, flags);
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

	spin_lock_irqsave(&my_lock, flags);
	remove_write_acquirer(&krot);
	if (cur <= max && cur >= min) {
		if (rot_area.waiting_writers[rotation.degree / 30] > 0) {
			thread_cond_broadcast(&rotation_write);
			printk("hey wake up!\n");
		} else {
			thread_cond_broadcast(&rotation_read);
		}
	}
	spin_unlock_irqrestore(&my_lock, flags);
	return 0;
}
