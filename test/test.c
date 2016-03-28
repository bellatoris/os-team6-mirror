#include <stdio.h>
#include <linux/unistd.h>
#include <linux/types.h>
#define __NR_ptree 382


typedef struct prinfo {
        long state;
        pid_t pid;
        pid_t parent_pid;
        pid_t first_child_pid;
        pid_t next_sibling_pid;
        long uid;
        char comm[64];
};

int main()
{
	int nr = 2000;			//randomly choosed
	int np = 0;
	struct prinfo buf[nr];
	np = syscall(__NR_ptree,buf, &nr);
	
	perror("ptree");
	if(np < 0 ) {
		return 0;
	}
	dfs(buf , &nr);
	printf("%d\n",np);
	return 0;
}
void dfs(struct prinfo *buf, int *nr)
{
        int index = 0;
        int indent_index = 0;
        tree_print(buf, nr, index, indent_index);
}
int tree_print(struct prinfo *buf, int *nr, int index, int indent){
	if(*nr <= index){
		return *nr;	
	}
        struct prinfo node = buf[index];
        indent_print(indent);
        info_print(&node);
        index++;
        if(node.first_child_pid)
                index = tree_print(buf,nr,index,indent + 1);
        if(node.next_sibling_pid)
                index = tree_print(buf,nr,index,indent);
        return index;
}


void indent_print(int index)
{
        int i = 0;
        for(; i < index; i++)
                printf("\t");
		
}
void info_print(struct prinfo *node)
{
        printf("%s, %d, %ld, %d, %d, %d, %d\n", node->comm, node->pid,
                        node->state, node->parent_pid, node->first_child_pid,
                        node->next_sibling_pid, node->uid);
}
