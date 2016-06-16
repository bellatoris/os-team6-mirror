#include <linux/gps.h>

struct gps_location kernel_location;
EXPORT_SYMBOL(kernel_location);

SYSCALL_DEFINE1(set_gps_location, struct gps_location __user *, loc)
{
	copy_from_user(&kernel_location, loc,sizeof(struct gps_location));
	return 0;
}

SYSCALL_DEFINE2(get_gps_location, const char *, pathname, struct gps_location __user *, loc)
{
	return 0;
}

