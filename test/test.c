#include <stdio.h>
#include <linux/unistd.h>
//#include <include/linux/rotation.h>
#define __NR_get_rotation 385

struct dev_rotation{
	int degree;
};


int main(int argc, char* argv[]){
	struct dev_rotation rot;
	while(1){
		printf("work?\n");
		syscall(__NR_get_rotation, &rot);
		printf("%d", rot.degree); 
		sleep(2);
	}
}
