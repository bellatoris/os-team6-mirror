#include <stdio.h>
#include <linux/unistd.h>
#include <sched.h>
#include <math.h>
#include <unistd.h>
#include <time.h>

#define __NR_sched_setweight 384
#define __NR_sched_getweight 385
#define N 20000

int main(int argc, char* argv[]) {
	int i = atoi(argv[1]);
	int weight = atoi(argv[2]);
	syscall(__NR_sched_setweight,i,weight);
	perror("sched_setweight");
}

