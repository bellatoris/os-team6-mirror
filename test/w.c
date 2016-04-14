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
	char * name = argv[1];
        range.rot.degree = atoi(argv[2]);
        range.degree_range = atoi(argv[3]);
	int val = atoi(argv[4]);
	printf("%s val : %d\n",name,val);
	printf("%s want lock\n",name); 
	syscall(__NR_rotlock_write,&range);
	printf("%s get lock\n",name);
	FILE* fp= fopen("for_test","w");
	fprintf(fp,"%d", val);
	fclose(fp);  
	printf("%s go to sleep(10)\n",name);
	sleep(10);
	printf("%s unlock\n",name);
	syscall(__NR_rotunlock_write,&range);
}
