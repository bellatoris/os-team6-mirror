/*
 * sets current device rotation in the kernel.
 * syscall number 384 (you may want to check this number!)
 */
#include <linux/linkage.h>
#include <asm-generic/uaccess.h>>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/sched.h>

struct dev_rotation {
        int degree;     /* 0 <= degree < 360 */
};
/*
 * Defines rotation range. The ranges give the "width" of
 * the range. If one of the range values is 0, it is not considered
 * as defining the range (ignored).
 */
struct rotation_range {
        struct dev_rotation rot;  /* device rotation */
	unsigned int degree_range;      /* lock range = rot.degree ± degree_range */
};

struct dev_rotation rotation;
EXPORT_SYMBOL(rotation);

struct rotation_lock {
	int min;
	int max;
	pid_t pid;
	struct list_head lock_list;
};

#define ROTATION_LOCK_INITIALIZER(name) \
	{ 0, 0, 0, LIST_HEAD_INIT((name).lock_list) }
#define ROTATION_LOCK(name) \
	struct rot_lock name = ROTATION_LOCK_INITIALIZER(name)

inline void init_rotation_lock(struct rotation_lock *lock, 
			struct task_struct *p, struct rotation_range *rot)
{
    lock->max = rot->rot.degree + rot->degree_range + 360;
    lock->min = rot->rot.degree - (int)rot->degree_range + 360;
    lock->pid = p->pid;
    lock->lock_list.prev = &lock->lock_list;
    lock->lock_list.next = &lock->lock_list;
}

struct lock_queue {
	struct list_head lock_list;
//	spinlock_t = queue_lock;
};

#define LOCK_QUEUE_INITIALIZER(name) \
	{ LIST_HEAD_INIT((name).lock_list) }
#define LOCK_QUEUE(name) \
	struct lock_queue name = LOCK_QUEUE_INITIALIZER(name)

LOCK_QUEUE(waiting_writer);
LOCK_QUEUE(acquire_writer);
LOCK_QUEUE(waiting_reader);
LOCK_QUEUE(acquire_reader);

EXPORT_SYMBOL(waiting_writer);
EXPORT_SYMBOL(acquire_writer);
EXPORT_SYMBOL(waiting_reader);
EXPORT_SYMBOL(acquire_reader);

/*
 * Define my_lock for read/wirte lock
 * Define glob_lock for queue lock
 */
DEFINE_SPINLOCK(my_lock);
EXPORT_SYMBOL(my_lock);

DEFINE_SPINLOCK(glob_lock);
EXPORT_SYMBOL(glob_lock);

void exit_rotlock(void);

asmlinkage int sys_set_rotation(struct dev_rotation __user *rot);

/* Take a read/or write lock using the given rotation range
 * returning 0 on success, -1 on failure.
 * system call numbers 385 and 386
 */
asmlinkage int sys_rotlock_read(struct rotation_range __user *rot);
asmlinkage int sys_rotlock_write(struct rotation_range __user *rot);

/* Release a read/or write lock using the given rotation range
 * returning 0 on success, -1 on failure.
 * system call numbers 387 and 388 
 */
asmlinkage int sys_rotunlock_read(struct rotation_range __user *rot);
asmlinkage int sys_rotunlock_write(struct rotation_range __user *rot);
