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
int thread_cond_signal(void)
{	
	struct rotation_lock *curr, *next;
	int i = 1;
	int cur;
	printk("thread_cond_signal\n");
	spin_lock(&glob_lock);
	list_for_each_entry_safe(curr, next, &waiting_writer.lock_list, 
								lock_list) {
		cur = ((curr->min <= rotation.degree) && 
			(rotation.degree < 360)) ? rotation.degree :	
						    rotation.degree + 360;
		printk("cur = %d, min = %d, max = %d\n", cur, curr->min, curr->max);
		printk("traverse waiting_writer %p\n", curr);
		if (cur <= curr->max && cur >= curr->min) {
			wake_up_process(pid_task(find_get_pid(curr->pid),
								PIDTYPE_PID));
			printk("wake up the waiting writer pid: %d\n", curr->pid);
			i = 0;
			break;
		}
	}
	spin_unlock(&glob_lock);
	return i;
}

void thread_cond_broadcast(void)
{
	struct rotation_lock *curr, *next;
	int cur;
	spin_lock(&glob_lock);
	list_for_each_entry_safe(curr, next, &waiting_reader.lock_list, 
								lock_list) {
		cur = ((curr->min <= rotation.degree) && 
			(rotation.degree < 360)) ? rotation.degree : 
						    rotation.degree + 360;	
		if (cur<= curr->max && cur >= curr->min) {
			wake_up_process(pid_task(find_get_pid(curr->pid),
								PIDTYPE_PID));
		}
	}
	spin_unlock(&glob_lock);
}

static unsigned long __sched thread_cond_wait(unsigned long flag)
{
	//wait을 할 때 lock을 풀고 wait을 한다.

	spin_unlock_irqrestore(&my_lock, flag);

	set_current_state(TASK_UNINTERRUPTIBLE);
	printk("process go to sleep\n");
	schedule();
	printk("process wake up\n");

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
	//현재 각도에 wrte acquirer가 있으면
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
	printk("remove_read_waiter\n");
	__set_current_state(TASK_RUNNING);
	if (!list_empty_careful(&waiting_reader.lock_list)) {
		spin_lock(&glob_lock);
		list_del_init(&rot_lock->lock_list);
		spin_unlock(&glob_lock);
	}
}

static inline void add_read_waiter(struct rotation_lock *rot_lock)
{
	printk("add_read_waiter\n");
	spin_lock(&glob_lock);
	list_add_tail(&rot_lock->lock_list, &waiting_reader.lock_list);
	spin_unlock(&glob_lock);
}

static inline void remove_write_waiter(struct rotation_lock *rot_lock)
{
	printk("remove_write_waiter\n");
	__set_current_state(TASK_RUNNING);
	if (!list_empty_careful(&waiting_writer.lock_list)) {
		spin_lock(&glob_lock);
		list_del_init(&rot_lock->lock_list);
		spin_unlock(&glob_lock);
	}
}

static inline void add_write_waiter(struct rotation_lock *rot_lock)
{
	printk("add_write_waiter\n");
	spin_lock(&glob_lock);
	list_add_tail(&rot_lock->lock_list, &waiting_writer.lock_list);
	spin_unlock(&glob_lock);
}

static struct rotation_lock *remove_read_acquirer(struct rotation_range *rot)
{
	int max = rot->rot.degree + rot->degree_range + 360;
	int min = rot->rot.degree - (int)rot->degree_range + 360;
	struct rotation_lock *curr, *next;
	printk("remove_read_acquirer\n");
	spin_lock(&glob_lock);
	list_for_each_entry_safe(curr, next, &acquire_reader.lock_list, 
								lock_list) {
		if (current->pid == curr->pid && max == curr->max && 
						    min == curr->min) {
			list_del_init(&curr->lock_list);
			break;
		}
	}
	spin_unlock(&glob_lock);
	return curr;
}

static inline void add_read_acquirer(struct rotation_lock *rot_lock)
{	
	printk("add_read_acquirer\n");
	spin_lock(&glob_lock);
	list_add_tail(&rot_lock->lock_list, &acquire_reader.lock_list);
	spin_unlock(&glob_lock);
}

static  struct rotation_lock *remove_write_acquirer(struct rotation_range *rot)
{	

	int max = rot->rot.degree + rot->degree_range + 360;
	int min = rot->rot.degree - (int)rot->degree_range + 360;
	struct rotation_lock *curr, *next;
	printk("remove_write_acquirer\n");
	spin_lock(&glob_lock);
	list_for_each_entry_safe(curr, next, &acquire_writer.lock_list, 
								lock_list) {
		if (current->pid == curr->pid && max == curr->max && 
						    min == curr->min) {
			list_del_init(&curr->lock_list);
			break;
		}
	}
	spin_unlock(&glob_lock);
	return curr;
}

static inline void add_write_acquirer(struct rotation_lock *rot_lock)
{	
	printk("add_write_acquirer\n");
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
			list_del_init(&curr->lock_list);
			kfree(curr);
		}
	}
	spin_unlock(&glob_lock);

	spin_lock(&glob_lock);
	list_for_each_entry_safe(curr, next, &waiting_writer.lock_list, 
								lock_list) {
		if (current->pid == curr->pid) {
			list_del_init(&curr->lock_list);
			kfree(curr);
		}
	}
	spin_unlock(&glob_lock);
	
	spin_lock(&glob_lock);
	list_for_each_entry_safe(curr, next, &acquire_reader.lock_list, 
								lock_list) {
		if (current->pid == curr->pid) {
			list_del_init(&curr->lock_list);
			kfree(curr);
		}
	}
	spin_unlock(&glob_lock);

	spin_lock(&glob_lock);
	list_for_each_entry_safe(curr, next, &waiting_reader.lock_list, 
								lock_list) {
		if (current->pid == curr->pid) {
			list_del_init(&curr->lock_list);
			kfree(curr);
		}
	}
	spin_unlock(&glob_lock);
}

asmlinkage int sys_set_rotation(struct dev_rotation __user *rot)
{
	get_user(rotation.degree, &rot->degree);
	printk("%d\n", rotation.degree);
	if (thread_cond_signal())
		thread_cond_broadcast();
	return 0;
}

asmlinkage int sys_rotlock_read(struct rotation_range __user *rot)
{
	/*
	struct rotation_range krot;
	unsigned long flags;

	struct rotation_lock *klock = kmalloc(sizeof(struct rotation_lock),
								GFP_KERNEL);
	get_user(krot, rot);
	init_rotation_lock(klock, current, &krot);

	spin_lock_irqsave(&my_lock, flags);
	add_read_waiter(klock);
	while (read_should_wait(klock)) {
		flags = thread_cond_wait(flags);
	}
	remove_read_waiter(klock);
	add_read_acquirer(klock);
	spin_unlock_irqrestore(&my_lock, flags);

	return 0;
	*/

	struct rotation_range krot;
	unsigned long flags;
	struct rotation_lock *klock = kmalloc(sizeof(struct rotation_lock),
								GFP_KERNEL);
	printk("sys_rotlock_write %p\n", klock);
	get_user(krot, rot);
	init_rotation_lock(klock, current, &krot);

	spin_lock_irqsave(&my_lock, flags);
	add_write_waiter(klock);
	while (write_should_wait(klock)) {
		flags = thread_cond_wait(flags);
	}
	remove_write_waiter(klock);
	add_write_acquirer(klock);
	
	spin_unlock_irqrestore(&my_lock, flags);
	return 0;

}

asmlinkage int sys_rotlock_write(struct rotation_range __user *rot){
        struct rotation_range krot;
        unsigned long flags;
        get_user(krot,rot);

        struct rotation_lock *lock = kmalloc(sizeof(rotation_lock), GFP_KERNEL);
        init_rotation_lock(lock, current, krot);

        spin_lock_irqsave(&my_lock, flags);

        add_list(waiting_writer, lock);
        while(write_should_wait(lock)){
                wait(lock);    //signal을 여기서 받아야됨
        }
        remove_list(waiting_writer, lock);
        add_list(acquire_writer, lock);
        spin_unlock_irqstore(&my_lock, flags);
}



asmlinkage int sys_rotunlock_read(struct rotation_range __user *rot)
{
	struct rotation_range krot;
	struct rotation_lock *klock;
	unsigned long flags;

	get_user(krot, rot);

	spin_lock_irqsave(&my_lock, flags);
	klock = remove_read_acquirer(&krot);
	kfree(klock);
	thread_cond_signal();
	spin_unlock_irqrestore(&my_lock, flags);
	return 0;


}


asmlinkage int sys_rotunlock_write(struct rotation_range __user *rot){
	struct rotation_range krot;

	struct rotation_lock *klock;
	unsigned long flags;
	printk("sys_unlock_write\n");
	get_user(krot, rot);

	spin_lock_irqsave(&my_lock, flags);
	klock = remove_write_acquirer(&krot);
	kfree(klock);
	if (thread_cond_signal())
		thread_cond_broadcast();
	spin_unlock_irqrestore(&my_lock, flags);
	return 0;
}

