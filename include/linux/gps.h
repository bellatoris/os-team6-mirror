#ifndef GPS_H
#define GPS_H

#include <linux/export.h>
#include <linux/linkage.h>
#include <linux/uaccess.h>

struct gps_location {
	double latitude;
	double longitude;
	float accuracy; /* in meters */
};
/*
asmlinkage int set_gps_location(struct gps_location __user *loc);
asmlinkage int get_gps_location(const char *pathname, struct gps_location __user *loc);
*/
#endif
