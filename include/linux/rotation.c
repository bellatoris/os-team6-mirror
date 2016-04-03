#include <linux/export.h>

struct dev_rotation rotation;
EXPORT_SYMBOL(rotation);

asmlinkage int set_rotation(struct dev_rotation __user *rot) 
{
	get_user(rotation.degree, &rot->degree);
	return 0;
}

asmlinkage int get_rotation(struct dev_rotation __uesr *rot)
{
	put_user(rotation.degree, &rot->degree);
}

