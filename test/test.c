#include <unistd.h>
#include <sys/syscall.h>
#include <linux/unistd.h>

#define __NR_set_foo (__NR_SYSCALL_BASE+384)

int main()
{
    syscall(__NR_set_foo);
    return 0;
}
