#include <stdio.h>
#include <linux/unistd.h>
<<<<<<< HEAD
#include <math.h>
#include <unistd.h>

#define __NR_sched_setweight 384
#define __NR_sched_getweight 385
#define SCHED_WRR 6

const struct sched_param {
	int sched_priority;
};

int main(int argc, char *argv[]){
	int policy;
	pid_t pid;
	int weight;
	struct sched_param param;
	param.sched_priority = 0;

	pid = atoi(argv[1]);
	printf("pid : %d\n",pid);
	policy = syscall(__NR_sched_getscheduler, pid);
	perror("before");
	printf("before change policy : %d\n", policy);

	syscall(__NR_sched_setscheduler,pid, SCHED_WRR, &param);
	perror("set WRR");

	policy = syscall(__NR_sched_getscheduler, pid);
	perror("after");
	printf("after change policy : %d\n",policy);

	weight = syscall(__NR_sched_getweight,pid);
	printf("weight : %d\n",weight);
	perror("getweight");

	return 0;
}
=======
#include <linux/types.h>
#include <sched.h>

int main()
{
    struct sched_param param;
    param.sched_priority = 0;
    int i;

    for (i = 0; i < 1000; i++) {
	if (sched_setscheduler(i, 6, &param) == -1) {
	    perror("sched_setscheduler");
	} 
    }

    /*
    sleep(1);
    for (i = 0; i < 1000; i++) {
	printf("%d\n", sched_getscheduler(i));
    }
    sched_setscheduler(1, 6, &param);
    sleep(1);
    printf("%d\n", sched_getscheduler(1));*/
}
    
>>>>>>> Doogie3
