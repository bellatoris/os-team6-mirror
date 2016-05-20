#include <stdio.h>
#include <linux/unistd.h>
#define __NR_sched_setscheduler 156
#define __NR_sched_getscheduler 157
#define SCHED_WRR 6

const struct sched_param{
	int sched_priority;

};

int main(){
	int sysnum = 0;
	int pid = 0;
	int policy = 6;
	struct sched_param param;
	param.sched_priority = 0;
	while(1){
		policy = SCHED_WRR;
		printf("Input format \n'0 pid' : setscheduler(pid,SCHED_WRR,param), '1 pid': getscheduler(pid)\n");
		scanf("%d %d",&sysnum, &pid);
		if(sysnum == 0){
			syscall(__NR_sched_setscheduler,pid,policy,&param);
			perror("sched_setscheduler");
		}
		else{
			policy = syscall(__NR_sched_getscheduler,pid);
			printf("pid %d's policy = %d\n",pid, policy);
			perror("sched_getscheduler");
		}
	}
}
