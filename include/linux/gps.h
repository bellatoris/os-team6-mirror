#include <linux/export.h>

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

int set_gps_location(struct pgs_location __user *loc);
