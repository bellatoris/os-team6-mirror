#include <linux/linkage.h>
#include <linux/types.h>
asmlinkage int sys_sched_setweight(pid_t pid, int weight);
asmlinkage int sys_sched_getweight(pid_t pid, int weight);
asmlinkage int sys_dummy1(void);
asmlinkage int sys_dummy2(void);


asmlinkage int sys_sched_setweight(pid_t pid, int weight){
	return 0;
}

asmlinkage int sys_sched_getweight(pid_t pid, int weight){
	return 0;
}

asmlinkage int sys_dummy1(void){
	return 0;
}

asmlinkage int sys_dummy2(void){
	return 0;
}
