#include <linux/linkage.h>
#include <linux/unistd.h>
#include <linux/prinfo.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <asm-generic/uaccess.h>
#include <asm-generic/errno-base.h>
/*
 * DFS
 * Lock
 * buf, nr
 *
 */

asmlinkage long sys_dfs(struct prinfo *buf, int *nr, 
				struct task_struct *root);
asmlinkage void sys_visit(struct prinfo *buf, int *nr,
			struct task_struct *task, int *i);

asmlinkage long sys_ptree(struct prinfo __user *buf, int __user *nr)
{
	struct task_struct *root;
	struct prinfo *kbuf;
	int knr;
	int i;

	root = &init_task;
	i = 0;
	
	if (buf == NULL || nr == NULL) {
    		return -EINVAL;
	}
	
	if (get_user(knr, nr) < 0) {
		return -EFAULT;
	}
	if (knr < 1) {
		return -EINVAL;
	}

	kbuf = kmalloc_array(knr, sizeof(struct prinfo), GFP_KERNEL);
	if (kbuf == NULL)
		return -ENOMEM;
	}

	read_lock(&tasklist_lock);
	i = sys_dfs(kbuf, &knr, root);
	read_unlock(&tasklist_lock);

	if (copy_to_user(buf, kbuf, knr * sizeof(struct prinfo)) < 0) {
		kfree(kbuf);
		return -EFAULT;
	}

	kfree(kbuf);
	return i;
}

asmlinkage long sys_dfs(struct prinfo *buf, int *nr,
				struct task_struct *root)
{
	struct task_struct *task;
	int i;

	task = root;
	i = 0;

	while (true) {
		sys_visit(buf, nr, task, &i);

		if (!list_empty(&task->children)) {
			task = list_first_entry(&task->children,
					struct task_struct, sibling);
			continue;
		} else {
			while (list_is_last(&task->sibling, 
					    &task->real_parent->children) &&
							    task != root) {
				task = task->real_parent;
			}
			if (task == root)
				break;
			task = list_next_entry(task, sibling);
		}
	}
	return i;
}

asmlinkage void sys_visit(struct prinfo *buf, int *nr,
			struct task_struct *task, int *i)
{
    if (*i < *nr) {
	    buf[*i].state = task->state;
	    buf[*i].pid = task->pid;
	    buf[*i].parent_pid = task->real_parent->pid;

	    if (!list_empty(&task->children))
		    buf[*i].first_child_pid = list_first_entry(&task->children,
					    struct task_struct, sibling)->pid;
	    else
		    buf[*i].first_child_pid = 0;
		
	    if (!list_is_last(&task->sibling, &task->real_parent->children))
		    buf[*i].next_sibling_pid = list_next_entry(task,
	                                                    sibling)->pid;
	    else
		    buf[*i].next_sibling_pid = 0;

	    buf[*i].uid = task->real_cred->uid;
	    get_task_comm(buf[*i].comm, task);
    }
    (*i)++;
}

/*
asmlinkage long sys_dfs(struct prinfo *buf, int *nr, 
		struct task_struct *task, int *i)
{
    struct list_head *list;
    struct task_struct *child;

	if (*i < *nr) {		
		buf[*i].state = task->state;
		buf[*i].pid = task->pid;
		buf[*i].parent_pid = task->real_parent->pid;
		
		if (!list_empty(&task->children)) 
			buf[*i].first_child_pid = list_first_entry(&task->children,
					    struct task_struct, sibling)->pid;
		else
			buf[*i].first_child_pid = 0;

		buf[*i].next_sibling_pid = list_entry(task->sibling.next,
					    struct task_struct, sibling)->pid;

		if(buf[*i].next_sibling_pid == 1)
			buf[*i].next_sibling_pid = 0;
		    
		
		buf[*i].uid = task->real_cred->uid;
	    
		strcpy(buf[*i].comm, task->comm);

		printk("task_state = %ld\n task_pid = %ld\n task_parent_pid = %ld\n task_first_child_pid = %ld\n task_next_sibling_pid = %ld\n task_uid = %ld\n task_name_of_program_executed = %s\n", buf[*i].state, buf[*i].pid, buf[*i].parent_pid, buf[*i].first_child_pid, buf[*i].next_sibling_pid, buf[*i].uid, buf[*i].comm);
		
		(*i)++;

		list_for_each(list, &task->children) {
			child = list_entry(list, struct task_struct, sibling);
			if(child)
				sys_dfs(buf, nr, child, i);
		}
	}
	return 1;
}
*/
