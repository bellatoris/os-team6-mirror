#include <unistd.h>
#include <sys/syscall.h>
#include <linux/unistd.h>

#define __NR_foo 382

int main()
{
    syscall(__NR_foo);
    return 0;
}
