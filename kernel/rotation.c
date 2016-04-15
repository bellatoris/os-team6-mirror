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
	(name = (rot->min <= rotation.degree) ? rotation.degree : \
	rotation.degree + 360)

#define WAKE_UP(name) \
	wake_up_process(pid_task(\
	find_get_pid(name->pid), PIDTYPE_PID))

#define FIND(name) \
	pid_task(find_get_pid(name->pid), PIDTYPE_PID)

/*
 * Traverse lock_queue safely and check the range
 */
static int traverse_list_safe(struct rotation_lock *rot_lock,
						struct lock_queue *queue)
{
	struct rotation_lock *curr, *next;
	list_for_each_entry_safe(curr, next, &queue->lock_list, lock_list) {
		if (rot_lock->max >= curr->min && rot_lock->min <= curr->max)
			return 1;
	}
	return 0;
}

static inline void remove_read_waiter(struct rotation_lock *rot_lock)
{
	spin_lock(&glob_lock);
	if (!list_empty_careful(&waiting_reader.lock_list))
		list_del_init(&rot_lock->lock_list);
	spin_unlock(&glob_lock);
}

static inline void add_read_waiter(struct rotation_lock *rot_lock)
{
	spin_lock(&glob_lock);
	list_add_tail(&rot_lock->lock_list, &waiting_reader.lock_list);
	spin_unlock(&glob_lock);
}

static inline void remove_write_waiter(struct rotation_lock *rot_lock)
{
	spin_lock(&glob_lock);
	if (!list_empty_careful(&waiting_writer.lock_list))
		list_del_init(&rot_lock->lock_list);
	spin_unlock(&glob_lock);
}

static inline void add_write_waiter(struct rotation_lock *rot_lock)
{
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
	if (!flag)
		kfree(curr);
	return flag;
}

static inline void add_read_acquirer(struct rotation_lock *rot_lock)
{
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
	if (!flag)
		kfree(curr);
	return flag;
}


static inline void add_write_acquirer(struct rotation_lock *rot_lock)
{
	spin_lock(&glob_lock);
	list_add_tail(&rot_lock->lock_list, &acquire_writer.lock_list);
	spin_unlock(&glob_lock);
}

static int thread_cond_signal(void)
{
	struct rotation_lock *curr, *next;
	struct task_struct *task;
	int i = 0;
	int cur;
	spin_lock(&glob_lock);
	list_for_each_entry_safe(curr, next, &waiting_writer.lock_list,
								lock_list) {
		SET_CUR(cur, curr);
		if (cur <= curr->max && cur >= curr->min) {
			if (!traverse_list_safe(curr, &acquire_writer) &&
				!traverse_list_safe(curr, &acquire_reader)) {
				task = FIND(curr);
				while (wake_up_process(task) != 1)
					continue;
				remove_write_waiter(curr);
				add_write_acquirer(curr);
				i = 1;
				break;
			}
		}
	}
	spin_unlock(&glob_lock);
	return i;
}

static int thread_cond_broadcast(void)
{
	struct rotation_lock *curr, *next;
	struct task_struct *task;
	int cur;
	int i = 0;
	spin_lock(&glob_lock);
	list_for_each_entry_safe(curr, next, &waiting_reader.lock_list,
								lock_list) {
		SET_CUR(cur, curr);
		if (cur <= curr->max && cur >= curr->min) {
			if (!traverse_list_safe(curr, &acquire_writer) &&
				!traverse_list_safe(curr, &waiting_writer)) {
				task = FIND(curr);
				while (wake_up_process(task) != 1)
					continue;
				remove_read_waiter(curr);
				add_read_acquirer(curr);
				i++;
			}
		}
	}
	spin_unlock(&glob_lock);
	return i;
}

static void __sched thread_cond_wait(struct rotation_lock *rot_lock)
{
//	preempt_disable();
	spin_unlock(&my_lock);
	set_current_state(TASK_INTERRUPTIBLE);
//	preempt_enable();
	schedule();
	spin_lock(&my_lock);
}

static int read_should_wait(struct rotation_lock *rot_lock)
{
	int cur;
	SET_CUR(cur, rot_lock);

	if (cur < rot_lock->min || cur > rot_lock->max)
		return 1;

	spin_lock(&glob_lock);
	if (traverse_list_safe(rot_lock, &acquire_writer)) {
		spin_unlock(&glob_lock);
		return 1;
	}
	spin_unlock(&glob_lock);

	spin_lock(&glob_lock);
	if (traverse_list_safe(rot_lock, &waiting_writer)) {
		spin_unlock(&glob_lock);
		return 1;
	}
	spin_unlock(&glob_lock);

	remove_read_waiter(rot_lock);
	add_read_acquirer(rot_lock);

	return 0;
}

static int write_should_wait(struct rotation_lock *rot_lock)
{
	int cur;
	SET_CUR(cur, rot_lock);

	if (cur < rot_lock->min || cur > rot_lock->max)
		return 1;

	spin_lock(&glob_lock);
	if (traverse_list_safe(rot_lock, &acquire_writer)) {
		spin_unlock(&glob_lock);
		return 1;
	}
	spin_unlock(&glob_lock);

	spin_lock(&glob_lock);
	if (traverse_list_safe(rot_lock, &acquire_reader)) {
		spin_unlock(&glob_lock);
		return 1;
	}
	spin_unlock(&glob_lock);

	remove_write_waiter(rot_lock);
	add_write_acquirer(rot_lock);

	return 0;
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
	int i;
	if (copy_from_user(&rotation.degree, &rot->degree,
				    sizeof(struct dev_rotation)) != 0)
		return -EFAULT;

	if (rotation.degree < 0 || rotation.degree > 359)
		return -EINVAL;

	i = thread_cond_signal();
	if (!i)
		i = thread_cond_broadcast();
	return i;
}

asmlinkage int sys_rotlock_read(struct rotation_range __user *rot)
{
	struct rotation_range krot;
	struct rotation_lock *klock = kmalloc(sizeof(struct rotation_lock),
								GFP_KERNEL);
	if (klock == NULL)
		return -ENOMEM;

	if (copy_from_user(&krot, rot, sizeof(struct rotation_range)) != 0)
		return -EFAULT;

	if (krot.degree_range < 0)
		return -EINVAL;

	if (krot.degree_range >= 180)
		krot.degree_range = 180;

	if (krot.rot.degree < 0 || krot.rot.degree > 359)
		return -EINVAL;

	init_rotation_lock(klock, current, &krot);

	add_read_waiter(klock);
	spin_lock(&my_lock);
	if (read_should_wait(klock))
		thread_cond_wait(klock);
	spin_unlock(&my_lock);

	return 0;
}

asmlinkage int sys_rotlock_write(struct rotation_range __user *rot)
{
	struct rotation_range krot;
	struct rotation_lock *klock = kmalloc(sizeof(struct rotation_lock),
								GFP_KERNEL);

	if (klock == NULL)
		return -ENOMEM;

	if (copy_from_user(&krot, rot, sizeof(struct rotation_range)) != 0)
		return -EFAULT;

	if (krot.degree_range < 0)
		return -EINVAL;

	if (krot.degree_range >= 180)
		krot.degree_range = 180;

	if (krot.rot.degree < 0 || krot.rot.degree > 359)
		return -EINVAL;

	init_rotation_lock(klock, current, &krot);

	add_write_waiter(klock);
	spin_lock(&my_lock);
	if (write_should_wait(klock))
		thread_cond_wait(klock);
	spin_unlock(&my_lock);

	return 0;
}

asmlinkage int sys_rotunlock_read(struct rotation_range __user *rot)
{
	struct rotation_range krot;
	int flag = -1;

	if (copy_from_user(&krot, rot, sizeof(struct rotation_range)) != 0)
		return -EFAULT;

	if (krot.degree_range < 0)
		return -EINVAL;

	if (krot.degree_range >= 180)
		krot.degree_range = 180;

	if (krot.rot.degree < 0 || krot.rot.degree > 359)
		return -EINVAL;

	spin_lock(&my_lock);
	flag = remove_read_acquirer(&krot);
	if (!flag)
		thread_cond_signal();
	spin_unlock(&my_lock);
	return flag;
}

asmlinkage int sys_rotunlock_write(struct rotation_range __user *rot)
{
	struct rotation_range krot;
	int flag = -1;

	if (copy_from_user(&krot, rot, sizeof(struct rotation_range)) != 0)
		return -EFAULT;

	if (krot.degree_range < 0)
		return -EINVAL;

	if (krot.degree_range >= 180)
		krot.degree_range = 180;

	if (krot.rot.degree < 0 || krot.rot.degree > 359)
		return -EINVAL;

	spin_lock(&my_lock);
	flag = remove_write_acquirer(&krot);
	if (!flag) {
		if (!thread_cond_signal())
			thread_cond_broadcast();
	}
	spin_unlock(&my_lock);
	return flag;
}
