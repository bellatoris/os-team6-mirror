#include <linux/linkage.h>
#include <linux/unistd.h>
#include <linux/prinfo.h>
#include <linux/string.h>
/*
 * DFS
 * Lock
 * buf, nr
 *
 *
 */

asmlinkage long sys_dfs(struct prinfo *buf, int *nr, 
				struct task_struct *root);
asmlinkage long sys_visit(struct prinfo *buf, int *nr
			struct task_struct *task, int *i);

asmlinkage long sys_ptree(struct prinfo *buf, int *nr)
{
	struct task_struct *root;
	int i;
	
	root = &init_task;

	read_lock(&tasklist_lock);
	i = sys_dfs(buf, nr, root);
	read_unlock(&tasklist_lock);

	return i;
}

asmlinkage long sys_dfc(struct prinfo *but, int *nr,
			    struct task_struct *root)
{
	struct task_struct task;
	int next_sibling_pid;
	int i;

	task = root;
	i = 0;

	while (true) {
		sys_visit(buf, nr, task, &i);

		if (!list_empty(&task->children)) {
			task = list_first_entry(&task->children,
					struct task_struct, sibling);
		} else {
			next_sibling_pid = list_entry(task->sibling.next,
					struct task_struct, sibling)->pid;
			while (next_sibling_pid == 1) {
				task = task->real_parent;
				next_sibling_pid = list_entry(
					task->sibling.next, struct task_struct,
								sibling)->pid;
			}
			if (task == root)
				break;
			task = list_entry(task->sibling.next, 
				    struct task_struct, sibling);
		}
	}
	return i;
}

asmlinkage long sys_visit(struct prinfo *buf, int *nr,
			struct task_struct *task, int *i)
{
    if (*i < *nr) {
	    buf[*i]->state = task->state;
	    buf[*i]->pid = task->pid;
	    buf[*i]->parent_pid = task->real_parent->pid;

	    if(!list_empty(&task->children)
		    buf[*i]->first_child_pid = list_first_entry(&task->children,
					    struct task_struct, sibling)->pid;
	    else
		    buf[*i]->first_child_pid = 0;
	
	    buf[*i]->next_sibling_pid = list_entry(task->sibling.next,
					struct task_struct, sibling)->pid;

	    if(buf[*i]->next_sibling_pid == 1)
		    buf[*i]->next_sibling_pid = 0;

	    buf[*i]->uid = node->real_cred->uid;
	    strncpy(buf[*i]->comm, task->comm, 64);
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
