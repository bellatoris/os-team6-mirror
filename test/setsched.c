#include <stdio.h>
#include <linux/unistd.h>
#include <sched.h>
#define __NR_sched_setscheduler 156
#define SCHED_WRR 6

int main(int argc, char *argv[]){
	int sysnum = 0;
	int pid = 0;
	int policy = 6;

	struct sched_param param;
	param.sched_priority = 0;

	pid = atoi(argv[1]);
	syscall(__NR_sched_setscheduler,pid,policy,&param);
	perror("sched_setscheduler");
}
