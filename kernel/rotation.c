#include <linux/rotation.h>

struct dev_rotation rotation;
EXPORT_SYMBOL(rotation);

asmlinkage int sys_set_rotation(struct dev_rotation __user *rot) 
{
	get_user(rotation.degree, &rot->degree);
	return 0;
}

asmlinkage int sys_get_rotation(struct dev_rotation __user *rot)
{
	put_user(rotation.degree, &rot->degree);
	return 0;
};

asmlinkage int sys_rot_lock_read()
{
	return 0;
}
asmlinkage int sys_rot_lock_write()
{
	return 0;
}
