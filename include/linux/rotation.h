/*
 * sets current device rotation in the kernel.
 * syscall number 384 (you may want to check this number!)
 */
#include <linux/export.h>
#include <linux/linkage.h>
#include <asm-generic/uaccess.h>
#include <linux/kernel.h>
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
