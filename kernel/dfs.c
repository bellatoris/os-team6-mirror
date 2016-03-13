asmlinkage long sys_dfs(struct prinfo *buf, int *nr, struct task_struct *root)
{
	int flag; // 0 : down, 1: right, 2: up
	int *i; // 방문한 node 횟수
	struct task_struct *node = root; // 작업할 node
	struct task_struct *parent;
	i = 0;
	flag = 0;

	while(true)
	{
		if(flag != 2)   // down, right 일 때만 visit 한다.
			visit(buf, nr, node, i);
		
		if (!list_empty(&node->children)){ //자식이 있다면...
			node = list_first_entry(&node->children, struct task_struct, children) //walk down
			flag = 0;
		}

		else {// not have a child
			while(list_empty(&node->sibling)){ // 형제가 없으면.. -- 수정 필요
				if (node == root)
					return //break 다 돌았음
				node = node->real_parent; //walk up
				flag = 2;
			}
			node = list_entry(&p->sibling->next,struct task_struct, );   //walk right -- 수정 필요
			flag = 1;
		}
	} //다시 root로 올라오면 멈춤
}



// visit node function
asmlinkage void sys_visit(struct pinfo *buf, int *nr,
		struct task_struct *node, int i)
{
	if (*i < *nr) {
		(buf + *i)->state = node->state;
		(buf + *i)->pid = node->pid;
		(buf + *i)->parent_pid = node->real_parent->pid;
		

		if (!list_empty(&node->children))
			(buf + *i)->first_child_pid = list_first_entry(&node->children,
						struct task_struct, sibling)->pid;
		else
			(buf + *i)->first_child_pid =0;
		
		(buf + *i)->next_sibling_pid = list_entry(node->sibling.next,
						struct task_struct, sibling)->pid;
		
		if((buf + *i)->next_sibling_pid ==1)
			(buf + *i)->next_sibling_pid = 0;

		(buf + *i)->uid = node->real_cred->uid;

		strcpy((buf + *i)->comm, node->comm);		
	}
	/*
	printk("task_state = %ld\n task_pid = %ld\n task_parent_pid = %ld\n task_first_child_pid = %ld\n task_next_sibling_pid = %ld\n task_uid = %ld\n task_name_of_program_executed = %s\n", buf[*i].state, buf[*i].pid, buf[*i].parent_pid, buf[*i].first_child_pid, buf[*i].next_sibling_pid, buf[*i].uid, buf[*i].comm);*/
}
