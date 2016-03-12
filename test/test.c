#include <stdio.h>
#include <linux/unistd.h>
//#include <linux/pid.h>
//#include <linux/sched.h>
//#include <linux/prinfo.h>
#define __NR_ptree 382

struct prinfo {
    long state;
    long int pid;
    long int parent_pid;
    long int first_child_pid;
    long int next_sibling_pid;
    long uid;
    char comm[64];
};

int main()
{
    int nr;
    struct prinfo buf[40];
    nr = 40;
    syscall(__NR_ptree, buf, &nr);
    return 0;
}
