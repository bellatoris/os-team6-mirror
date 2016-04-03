#include <linux/rotation.h>

extern struct dev_rotation rotation;

asmlinkage int sys_set_rotation(struct dev_rotation __user *rot) 
{
	get_user(rotation.degree, &rot->degree);
	printk("%d\n", rotation.degree);
	return 0;
}

asmlinkage int sys_rotlock_read(struct rotation_range __user *rot)
{
	return 0;
}
asmlinkage int sys_rotlock_write(struct rotation_range __user *rot)
{
	return 0;
}

asmlinkage int sys_rotunlock_read(struct rotation_range __user *rot)
{
	return 0;
}

asmlinkage int sys_rotunlock_write(struct rotation_range __user *rot)
{
	return 0;
}
