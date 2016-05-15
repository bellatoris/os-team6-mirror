#include <linux/linkage.h>
#include <linux/types.h>
asmlinkage int sys_sched_setweight(pid_t pid, int weight);
asmlinkage int sys_sched_getweight(pid_t pid, int weight);
asmlinkage int sys_dummy1(void);
asmlinkage int sys_dummy2(void);

/* Set the SCHED_WRR weight of process, as identified by 'pid'.
 * If 'pid' is 0, set the weight for the calling process.
 * System call number 384.
 */
asmlinkage int sys_sched_setweight(pid_t pid, int weight){
	return 0;
}

/* Obtain the SCHED_WRR weight of a process as identified by 'pid'.
 * If 'pid' is 0, return the weight of the calling process.
 * System call number 385.
 */
asmlinkage int sys_sched_getweight(pid_t pid, int weight){
	return 0;
}

asmlinkage int sys_dummy1(void){
	return 0;
}

asmlinkage int sys_dummy2(void){
	return 0;
}
