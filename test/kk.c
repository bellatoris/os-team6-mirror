#include <stdio.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include <sched.h>

int main()
{
    int i;
    
    for (i = 0; i < 2000; i++) {
	if (sched_getscheduler(i) != -1) 
	    printf("%d\n", sched_getscheduler(i));
    }
}
    
