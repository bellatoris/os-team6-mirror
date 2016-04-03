#include <linux/export.h>

struct dev_rotation rotation;
EXPORT_SYMBOL(rotation);

int set_rotation(struct dev_rotation *rot) 
{
	rotation.degree = rot->degree;
	return 0;
}
