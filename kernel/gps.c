#include <linux/gps.h>
extern gps_location location;

int set_gps_location(struct gps_location __user *loc){
	copy_from_user(&location, loc,sizeof(struct gps_location));
	return 0;
}

int get_gps_location(const char *pathname, struct pgs_location __user *loc){
	return 0;
}

