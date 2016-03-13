#include <stdio.h>
#include <linux/unistd.h>
#define __NR_ptree 382

typedef struct prinfo {
        int state;
        int pid;
        int parent_pid;
        int first_child_pid;
        int next_sibling_pid;
        int uid;
        char comm[64];
};

int main()
{
	int nr = 80;			//randomly choosed
	struct prinfo buf[nr]; 
	syscall(__NR_ptree, &buf, &nr);
	dfs(buf , &nr);
	return 0;
}
void dfs(struct prinfo *buf, int *nr)
{
        int index = 0;
        int indent_index = 0;
        tree_print(buf, nr, index, indent_index);
}
void tree_print(struct prinfo *buf, int *nr, int index, int indent){
	if(buf[index+1].pid == 0 && index != 0)
		return;
        struct prinfo node = buf[index];
	printf("line %d: ", index);
        indent_print(indent);
        info_print(&node);
        index++;
        if(node.first_child_pid){
                indent++;
                tree_print(buf,nr,index,indent);
        }else{
                if(!node.next_sibling_pid)
			tree_print(buf,nr,index,indent-1);
                else
                	tree_print(buf,nr,index,indent);
		
        }
}

void indent_print(int index)
{
        int i = 0;
        for(; i < index; i++)
                printf("\t");
		
}
void info_print(struct prinfo *node)
{
        printf("%s, %d, %d, %d, %d, %d, %d\n", node->comm, node->state,
                        node->pid, node->parent_pid, node->first_child_pid,
                        node->next_sibling_pid, node->uid);
}
