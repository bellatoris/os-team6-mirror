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
struct gps_location location;
EXPORT_SYMBOL(location);

asmlinkage int set_gps_location(struct gps_location __user *loc);
asmlinkage int get_gps_location(struct gps_location __user *loc);
