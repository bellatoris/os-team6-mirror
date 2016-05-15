#include <linux/linkage.h>
#include <linux/types.h>
#include <linux/sched.h>
asmlinkage int sys_sched_setweight(pid_t pid, int weight);
asmlinkage int sys_sched_getweight(pid_t pid);
asmlinkage int sys_dummy1(void);
asmlinkage int sys_dummy2(void);

/* Set the SCHED_WRR weight of process, as identified by 'pid'.
 * If 'pid' is 0, set the weight for the calling process.
 * System call number 384.
 */
asmlinkage int sys_sched_setweight(pid_t pid, int weight){
	struct task_struct *tsk;
	struct pid *p;
	p = find_get_pid(pid);
	tsk = pid_task(p, PIDTYPE_PID);
	if (pid == 0){
		current->wrr_weight = weight;
	}
	else{
		tsk->wrr_weight = weight;
	}
	return 0;
}

/* Obtain the SCHED_WRR weight of a process as identified by 'pid'.
 * If 'pid' is 0, return the weight of the calling process.
 * System call number 385.
 */

 // need to check whether policy is WRR or not.
asmlinkage int sys_sched_getweight(pid_t pid){
	struct task_struct *tsk;
	int weight;
	struct pid *p;
	p = find_get_pid(pid);
	tsk = pid_task(p, PIDTYPE_PID);
	if (pid == 0){
		weight = current->wrr_weight;
	}
	else{
		weight = tsk->wrr_weight;
	}
	return weight;
}

asmlinkage int sys_dummy1(void){
	return 0;
}

asmlinkage int sys_dummy2(void){
	return 0;
}
