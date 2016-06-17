#ifndef GPS_H
#define GPS_H

#include <linux/export.h>
#include <linux/linkage.h>
#include <linux/uaccess.h>

#define EXT2_FS_GPS "ext2"

struct gps_location {
	double latitude;
	double longitude;
	float accuracy; /* in meters */
};

/*
asmlinkage long set_gps_location(struct gps_location __user *loc);
asmlinkage long get_gps_location(const char *pathname, struct gps_location __user *loc);
*/
#endif
