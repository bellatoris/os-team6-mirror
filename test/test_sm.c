#include <stdio.h>
#include <linux/unistd.h>
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

	syscall(__NR_sched_setweight,pid,15);
	weight = syscall(__NR_sched_getweight,pid);
	printf("weight 15 : %d\n",weight);
	perror("getweight");

	return 0;
}
