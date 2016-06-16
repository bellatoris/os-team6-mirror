#include <linux/export.h>
#include <linux/linkage.h>
#include <linux/uaccess.h>

struct gps_location {
	double latitude;
	double longitude;
	float accuracy; /* in meters */
};

/*
 * location global variable
 */
struct gps_location kernel_location;
EXPORT_SYMBOL(kernel_location);

asmlinkage int set_gps_location(struct gps_location __user *loc);
asmlinkage int get_gps_location(const char *pathname, struct gps_location __user *loc);
