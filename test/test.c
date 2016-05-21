#include <stdio.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include <sched.h>

int main(int argc, char *argv[])
{
    struct sched_param param;
    param.sched_priority = 0;
    int i = atoi(argv[1]);
    int j = atoi(argv[2]);
    sched_setscheduler(i, j, &param);
    perror("sched_setscheduler");
}
    
