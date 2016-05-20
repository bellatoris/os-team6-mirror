#include <stdio.h>
#include <linux/unistd.h>
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
    
