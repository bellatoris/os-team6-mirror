#include <stdio.h>
#include <stdlib.h>
#include <linux/unistd.h>
#define __NR_rotlock_write 386
#define __NR_rotunlock_write 388

struct dev_rotation{
	int degree;
};

struct rotation_range{
        struct dev_rotation rot;        /* device rotation */
        unsigned int degree_range;      /* lock range = rot.degree ± degree_range */
    /* rot.degree - degree_range <= LOCK RANGE <= rot.degree + degree_range */
};


void main(int argc, char* argv[])
{
	int val = 4;  //init value
	struct rotation_range range;
	range.rot.degree = 30;
	range.degree_range = 10;
	FILE* fp;
	
	while(1)
	{
		syscall(__NR_rotlock_write,&range);
		fp = fopen("integer","w");
		fprintf(fp,"%d", val);
		fclose(fp);
		printf("selector : %d\n",val);
		sleep(1);
		syscall(__NR_rotunlock_write,&range);
		val+=1;
	}
}
