#include <stdlib.h>
#include <unistd.h>

#define __NR_set_gps_location 384

struct gps_location {
	double latitude;
	double longitude;
	float accuracy;		/* in meters */
};

/*
 * gpsupdate.c
 * 3개의 인풋 인자를 받아 커널 gps를 업데이트 한다.
 * argv[1] = latitude;
 * argv[2] = longitude;
 * argv[3] = accuracy;
 */

int main(int argc, char* argv[]){

	struct gps_location usr_gps;
	usr_gps.latitude = atof(argv[1]);
	usr_gps.longitude = atof(argv[2]);
	usr_gps.accuracy = atof(argv[3]);



	unsigned long long *lat =(unsigned long long*) (&usr_gps.latitude);
	unsigned long long *lng = (unsigned long long *)&usr_gps.longitude;
	unsigned int *acc = (unsigned int *)&usr_gps.accuracy;

	printf("lat : %llu long : %llu accu : %x",usr_gps.latitude,*lng,*acc);

	syscall(__NR_set_gps_location, &usr_gps);
	return 0;
}
