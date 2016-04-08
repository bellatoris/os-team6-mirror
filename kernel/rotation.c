#include <linux/rotation.h>
#include <linux/rotexit.h>

extern struct dev_rotation rotation;
extern struct lock_queue waiting_writer;
extern struct lock_queue acquire_writer;
extern struct lock_queue waiting_reader;
extern struct lock_queue acquire_reader;
extern spinlock_t my_lock;
extern spinlock_t glob_lock;

#define SET_CUR(name, rot) \
	(name = ((rot->min <= rotation.degree) && \
	(rotation.degree < 360)) ? rotation.degree : \
	rotation.degree + 360)

#define WAKE_UP(name) \
	wake_up_process(pid_task(\
	find_get_pid(name->pid), PIDTYPE_PID))

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
		SET_CUR(cur, curr);
		printk("cur = %d, min = %d, max = %d\n", cur,
						curr->min, curr->max);
		printk("traverse waiting_writer %p\n", curr);
		if (cur <= curr->max && cur >= curr->min) {
			WAKE_UP(curr);
			printk("wake up the waiting writer pid: %d\n",
								curr->pid);
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
		SET_CUR(cur, curr);
		if (cur <= curr->max && cur >= curr->min) {
			WAKE_UP(curr);
		}
	}
	spin_unlock(&glob_lock);
}

static void __sched thread_cond_wait(void)
{
	spin_unlock(&my_lock);
	set_current_state(TASK_INTERRUPTIBLE);
	printk("process go to sleep\n");
	schedule();
	printk("process wake up\n");
	spin_lock(&my_lock);
}

static int read_should_wait(struct rotation_lock *rot_lock)
{
	struct rotation_lock *curr, *next;
	int cur;
	SET_CUR(cur, rot_lock);
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
	struct rotation_lock *curr, *next;
	int cur;
	SET_CUR(cur, rot_lock);
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

static int remove_read_acquirer(struct rotation_range *rot)
{
	int flag = -1;
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
			flag = 0;
                        break;
                }
        }
        spin_unlock(&glob_lock);
	if(!flag)
		kfree(curr);
        return flag;
}

static inline void add_read_acquirer(struct rotation_lock *rot_lock)
{
	printk("add_read_acquirer\n");
	spin_lock(&glob_lock);
	list_add_tail(&rot_lock->lock_list, &acquire_reader.lock_list);
	spin_unlock(&glob_lock);
}

static int remove_write_acquirer(struct rotation_range *rot)
{
	int flag = -1;
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
			flag = 0;
                        break;
                }
        }
        spin_unlock(&glob_lock);
	if(!flag)
		kfree(curr);
        return flag;
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
	copy_from_user(&rotation.degree, &rot->degree, 
				    sizeof(struct dev_rotation));
	printk("%d\n", rotation.degree);
	if (thread_cond_signal())
		thread_cond_broadcast();
	return 0;
}

asmlinkage int sys_rotlock_read(struct rotation_range __user *rot)
{
	struct rotation_range krot;
	struct rotation_lock *klock = kmalloc(sizeof(struct rotation_lock),
								GFP_KERNEL);
	printk("sys_rotlock_write %p\n", klock);
	copy_from_user(&krot, rot, sizeof(struct rotation_range));
	init_rotation_lock(klock, current, &krot);

	spin_lock(&my_lock);
	add_read_waiter(klock);
	while (read_should_wait(klock)) {
		thread_cond_wait();
	}
	remove_read_waiter(klock);
	add_read_acquirer(klock);
	spin_unlock(&my_lock);

	return 0;
}

asmlinkage int sys_rotlock_write(struct rotation_range __user *rot)
{
	struct rotation_range krot;
	struct rotation_lock *klock = kmalloc(sizeof(struct rotation_lock),
								GFP_KERNEL);
	copy_from_user(&krot, rot, sizeof(struct rotation_range));
	init_rotation_lock(klock, current, &krot);

	spin_lock(&my_lock);
	add_write_waiter(klock);
	while (write_should_wait(klock)) {
		thread_cond_wait();
	}
	remove_write_waiter(klock);
	add_write_acquirer(klock);

	spin_unlock(&my_lock);
	return 0;
}

asmlinkage int sys_rotunlock_read(struct rotation_range __user *rot)
{
        struct rotation_range krot;
        unsigned long flags;
        int flag = -1;

        copy_from_user(&krot, rot, sizeof(struct rotation_range));

        spin_lock(&my_lock);
        flag = remove_read_acquirer(&krot);
        if(!flag)
                thread_cond_signal();
        spin_unlock(&my_lock);
        return flag;
}

asmlinkage int sys_rotunlock_write(struct rotation_range __user *rot)
{
        struct rotation_range krot;
        int flag = -1;
        printk("sys_unlock_write\n");
	copy_from_user(&krot, rot, sizeof(struct rotation_range));
	spin_lock(&my_lock);
        flag = remove_write_acquirer(&krot);
        if(!flag){
                if (thread_cond_signal())
                        thread_cond_broadcast();
        }
        spin_unlock(&my_lock);
        return flag;
}


