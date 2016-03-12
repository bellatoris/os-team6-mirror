#include <linux/unistd.h>
#include <linux/kernel.h>

asmlinkage long sys_helloworld(void)
{
	printk("[sys_helloworld] HELLO WORLD\n");
	return 0;
}
