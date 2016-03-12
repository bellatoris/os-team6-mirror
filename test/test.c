#include <unistd.h>
#include <sys/syscall.h>
#include <linux/unistd.h>

#define __NR_helloworld (__NR_SYSCALL_BASE+382)

int main()
{
	syscall(__NR_helloworld);
	return 0;
}
