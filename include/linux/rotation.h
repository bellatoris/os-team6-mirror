/*
 * sets current device rotation in the kernel.
 * syscall number 384 (you may want to check this number!)
 */
#include <linux/export.h>
#include <asm-generic/uaccess.h>

struct dev_rotation {
        int degree;     /* 0 <= degree < 360 */
}; 

asmlinkage int set_rotation(struct dev_rotation __user *rot);

asmlinkage int get_rotation(struct dev_rotation __user *rot);

