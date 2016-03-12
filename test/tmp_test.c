#include <stdio.h>
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
        int nr = 50;
        struct prinfo buf[50]; 
        dfs(buf , &nr);
        return 0;
}

void dfs(struct prinfo *buf, int *nr)
{
        int index = 0;
        int indent_index = 0;
        int parent_arr[*nr];

        while(1){
                indent_print(indent_index);
                info_print(buf);
                index++;
                if(buf[index].first_child_pid){
                        if(buf[index].next_sibling_pid)
                                parent_arr[indent_index]=have_child;
                        else
                                parent_arr[indent_index]=no_child;
                        indent_index++;
                }else{
                        if(!buf[index].next_sibling_pid){
                                if(buf[index].pid==0)
                                        return;
                                indent_index = indent_cal(parent_arr,indent_index);
                        }
                }
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
                indent_index--;
        return indent_index;
}

