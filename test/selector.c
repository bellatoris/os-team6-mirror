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
	int val = atoi(argv[1]);  //init value
	struct rotation_range range;
	range.rot.degree = 90;
	range.degree_range = 90;
	FILE* fp;

	do {
		syscall(__NR_rotlock_write,&range);
		fp = fopen("integer", "w");
		fprintf(fp,"%d", val);
		fclose(fp);
		printf("selector : %d\n",val);
		val++;
		usleep(1000 * 100);
		syscall(__NR_rotunlock_write,&range);

	} while(1);

	fp = fopen("interger", "w");
	fprintf(fp, "%d", 0);
	fclose(fp);
}
