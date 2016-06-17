#include <stdlib.h>
#include <unistd.h>

#define __NR_get_gps_location 385

struct gps_location {
        double latitude;
        double longitude;
        float accuracy;         /* in meters */
};

int main(int argc, char* argv[]){

        struct gps_location usr_gps;
	char * pathname;
        pathname = argv[1];
        syscall(__NR_get_gps_location, pathname,&usr_gps);
	perror("get_gps_location");
	printf("latitude : %f\nlongitude : %f\naccuracy : %f\n",usr_gps.latitude, usr_gps.longitude, usr_gps.accuracy);
	printf("Google map url : https://www.google.com/maps/@%d,%d",usr_gps.latitude,usr_gps.longitude);
        return 0;
}

