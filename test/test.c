#include <stdio.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include <sched.h>

int main()
{
    struct sched_param param;
    param.sched_priority = 0;
    int i;

    printf("hello\n");
}
    
