#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
int main(int c , char ** v){
	int limit = 100;
	while(limit){
		printf("%d\n",limit--);
		sleep(1);
	}
	return 0;
}
