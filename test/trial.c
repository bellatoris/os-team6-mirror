#include <stdio.h>
#include <linux/unistd.h>
#include <sched.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <sched.h>

#define __NR_sched_setweight 384
#define __NR_sched_getweight 385
#define N 10000
#define self 0
int prime[N];

void print_prime(int n);

int main(int argc, char* argv[]){
	int n;
	int i,j;
	int obj;
	int weight;
	//int pid;
	time_t start = 0;
	time_t end = 0;

	n = 2;
	prime[0]=2;

	struct sched_param param;
	param.sched_priority = 0;
	sched_setscheduler(0, 6, &param);
	
	while(1){
	start = clock();
	/*
	syscall(__NR_sched_setweight, self, weight);
	perror("sched_setweight");
	*/
	weight = syscall(__NR_sched_getweight, self);
	for(i=0;i< N;i++){
	obj = prime[i]+1;
		for(j=0;j<=i;j++){
			if(obj%prime[j]==0){
				obj++;
				j=-1;
				continue;
			}
			else if(i == j ){
				prime[i+1] = obj;
				break;
			}

		}
	}
	end = clock();
	printf("weight : %d, execution time : %f\n",weight,(double)(end-start)/(CLOCKS_PER_SEC));
	}
}

void print_prime(int n){
	int i;
	if(n <2){
		return;
	}
	for(i =0; i < N; i++){

		 if(n == prime[i]){
                        printf("%d \n" ,prime[i]);
                        return;
                }
		if( (n % prime[i]) ==0){
			printf("%d * ",prime[i]);
			n = (n / prime[i]);
			i--;
		}
	}
	printf("%d \n",n);  //if n > 10000th prime
}
