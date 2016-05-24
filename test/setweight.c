#include <stdio.h>
#include <linux/unistd.h>
#include <sched.h>
#include <unistd.h>

#define __NR_sched_setweight 384
#define __NR_sched_getweight 385
#define N 10000
#define self 0

int main(int argc, char* argv[]){
	int weight;
	int pid;
	weight = atoi(argv[1]);
	pid = atoi(argv[2]);

	syscall(__NR_sched_setweight, pid, weight);
	perror("sched_setweight");

	weight = syscall(__NR_sched_getweight, pid);
	perror("sched_getweight");
	printf("process's weight = %d\n", weight);
}
