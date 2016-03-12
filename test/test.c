#include <stdio.h>
#include <linux/unistd.h>
#include <linux/prinfo.h>
#define __NR_ptree 382
#define no_child 0
#define have_child 1

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
	int nr = 50;			//randomly choosed
	struct prinfo buf[nr]; 
	syscall(__NR_ptree, &buf, &nr);
	dfs(buf , &nr);
	return 0;
}

void dfs(struct prinfo *buf, int *nr)
{
        int index = 0;
        int indent_index = 0;
        int parent_arr[*nr];

        while(1){
                struct prinfo node = buf[index];
                indent_print(indent_index);
                info_print(&node);
                if(node.first_child_pid){
                        if(buf[index+1].next_sibling_pid)
                                parent_arr[indent_index]=have_child;
                        else
                                parent_arr[indent_index]=no_child;
                        indent_index++;
                }else{
                        if(!node.next_sibling_pid){
                                if(node.pid==0)
                                        return
                                indent_index = indent_cal(parent_arr,indent_index);
                        }
                }
                index++;
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

int indent_cal(int *arr , int indent_index)
{
        while(!arr[indent_index])
                if(indent_index ==0)
                        return 0;
                indent_index--;
        return indent_index;
}


