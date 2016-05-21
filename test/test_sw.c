#include <stdio.h>
#include <linux/types.h>
#include <sched.h>

int main(int argc, char *argv[])
{
    struct sched_param param;
    param.sched_priority = 0;
    int i = atoi(argv[1]);
    sched_setscheduler(i, 6, &param);
    perror("sched_setscheduler");
}
