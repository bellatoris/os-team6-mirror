#include <stdio.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include <sched.h>

int main()
{
    struct sched_param param;
    param.sched_priority = 99;
    if (sched_setscheduler(1, 6, &param) == -1) {
	perror("sched_setscheduler");
    }
}
    
