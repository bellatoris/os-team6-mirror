/*
 * This program is a program that  searches processes
 * by Depth First Search algorithm(DFS),
 * obtains the infomation,
 * saves it to temporary kernel buf,
 * passs it to user buf,
 * and returns the number of proesses..
 */


#include <linux/linkage.h>
#include <linux/unistd.h>
#include <linux/prinfo.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <asm-generic/uaccess.h>
#include <asm-generic/errno-base.h>

static int dfs(struct prinfo *buf, int *nr,
				struct task_struct *root);
static void visit(struct prinfo *buf, int *nr,
			struct task_struct *task, int *i);
/*
 * sys_ptree obtain all processes's  "prinfo",
 * passes it to user buf,
 * and returns the total number of processes.
 */

asmlinkage int sys_ptree(struct prinfo __user *buf, int __user *nr)
{
	struct task_struct *root;
	struct prinfo *kbuf;
	int knr;
	int i;

	root = &init_task;

	if (buf == NULL || nr == NULL)
		return -EINVAL;

	if (get_user(knr, nr) < 0)
		return -EFAULT;

	if (knr < 1)
		return -EINVAL;

	kbuf = kmalloc_array(knr, sizeof(struct prinfo), GFP_KERNEL);

	if (kbuf == NULL)
		return -ENOMEM;

	read_lock(&tasklist_lock);
	i = dfs(kbuf, &knr, root);
	read_unlock(&tasklist_lock);

	if (copy_to_user(buf, kbuf, knr * sizeof(struct prinfo)) < 0) {
		kfree(kbuf);
		return -EFAULT;
	}

	kfree(kbuf);
	return i;
}

/*
 * dfs searches all processes in dfs way,
 * and returns the number of that.
 *
 */
static int dfs(struct prinfo *buf, int *nr, struct task_struct *root)
{
	struct task_struct *task;
	int i;

	task = root;
	i = 0;

	while (true) {
		visit(buf, nr, task, &i);
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

/*
 * visit saves a prinfo of each process to the kernel buffer.
 *
 *
 */
static void visit(struct prinfo *buf, int *knr,
			struct task_struct *task, int *i)
{
	if (*i < *knr) {
		buf[*i].state = task->state;
		buf[*i].pid = task->pid;
		buf[*i].parent_pid = task->real_parent->pid;

		if (!list_empty(&task->children))
			buf[*i].first_child_pid = list_first_entry(
				&task->children, struct task_struct,
							sibling)->pid;
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
