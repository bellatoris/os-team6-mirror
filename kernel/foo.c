#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/sched.h>
#include <linux/unistd.h>


asmlinkage long sys_foo(void)
{
	struct task_struct *task;
	task = current;
	printk("parent pid = %d\n", task->parent->pid);
	printk("task pid = %d\n", task->pid);

	printk("==========star=============\n");

	return 1;
}
