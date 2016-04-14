#include <stdio.h>
#include <linux/unistd.h>
#include <math.h>
#define __NR_set_rotation 384
#define __NR_rotlock_read 385
#define __NR_rotlock_write 386
#define __NR_rotunlock_read 387
#define __NR_rotunlock_write 388

struct dev_rotation{
	int degree;
};


struct rotation_range{
	struct dev_rotation rot;	/* device rotation */
	unsigned int degree_range;	/* lock range = rot.degree ± degree_range */
    /* rot.degree - degree_range <= LOCK RANGE <= rot.degree + degree_range */
};


int main(int argc, char* argv[]){
	struct rotation_range range;
        int n;
        range.rot.degree = 180;
        range.degree_range = 180;
	int val = atoi(argv[1]);
	printf("W1 get lock\n"); 
	syscall(__NR_rotlock_write,&range);
	FILE* fp= fopen("for_test","w");
	fprintf(fp,"%d", val);
	fclose(fp);  
	printf("W1 go to sleep(10)\n");
	sleep(10);
	printf("W1 unlock\n");
	syscall(__NR_rotunlock_write,&range);
}
