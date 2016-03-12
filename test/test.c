#include <stdio.h>
#include <linux/unistd.h>

#define __NR_helloworld 382

int main()
{
	syscall(__NR_helloworld);
	return 0;
}
