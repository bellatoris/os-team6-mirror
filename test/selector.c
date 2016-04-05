#include <stdio.h>
#include <stdlib.h>
#include <linux/unistd.h>
#define __NR_rotlock_write 387
#define __NR_rotunlock_write 389

struct dev_rotation{
	int degree;
};

void main(int argc, char* argv[])
{
	int val = 4;  //init value
	struct dev_rotation rot;
	rot.degree=30;
	FILE* fp;
	
	while(val<10)
	{
		//syscall(__NR_rotlock_write,&rot);
		fp = fopen("integer","w");
		fprintf(fp,"%d", val);
		fclose(fp);
		printf("selector : %d\n",val);
		//syscall(__NR_rotunlock_write,&rot);
		val+=1;
	}
}
