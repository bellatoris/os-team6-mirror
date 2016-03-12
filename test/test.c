#include <stdio.h>
#include <linux/unistd.h>
#include <linux/prinfo.h>
#define __NR_helloworld 382
#define no_child 0
#define have_child 1
int main()
{
	int nr = 50;
	prinfo buf[nr]; 
	syscall(__NR_helloworld, &buf, &nr);
	dfs(buf , &nr);
	return 0;
}

void dfs(prinfo *buf, int *nr)
{
	prinfo node = buf[0];
	int index = 0;   
	int indent_index = 0;   
	int parent_arr[*nr];    

	while(1)
		indent_print(indent_index);
		info_print(&node);
		index++;
		//indet_index = indent_cal(&node, indent_index);
		if(!node.fisrt_child_pid){ 		
			if(!node.next_sibling_pid)	
				parent_arr[indent_index]=no_child; //이거 영어로 정의할것
			else			
				parent_arr[indent_index]=have_child;
			indent_index++;
		}else{
			while(!node.next_sibling_pid){
				if(node.pid)
					return 
				indent_index = indent_cal(parrent_arr,indent_index);
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
void info_print(prinfo *node)
{
	printf("%s, %d, %d, %d, %d, %d, %d\n", node->comm, node->state, 
                        node->pid, node->parent_pid. node->first_child_pid,
                        node->next_sibling_pid, node->uid);
}

int indent_cal(int *arr , int indent_index)
{
	while(!arr[indent_index])
		indent_index--;
	return indent_index;	
}


