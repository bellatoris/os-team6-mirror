#include <stdio.h>
#include <linux/unistd.h>
#include <sched.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>


#define __NR_sched_setweight 384
#define __NR_sched_getweight 385
#define N 20000
#define self 0
int prime[N];

void print_prime(int n);
float timedifference_msec(struct timeval t0, struct timeval t1);

int main(int argc, char* argv[]){
	int n;
	int i,j;
	int obj;
	int weight;

	time_t start = 0;
	time_t end = 0;

	struct timeval srt, ed;

	n = 2;
	prime[0]=2;
	
	weight = atoi(argv[1]);
	struct sched_param param;
	param.sched_priority = 0;
	sched_setscheduler(0, 6, &param);
	while(1) {
		gettimeofday(&srt,NULL);

		syscall(__NR_sched_setweight, self, weight);
		//perror("sched_setweight");
		
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
		gettimeofday(&ed,NULL);
		printf("weight : %d, execution time : %f\n",
			    weight, timedifference_msec(srt,ed));
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

float timedifference_msec(struct timeval t0, struct timeval t1)
 {
     return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
 }

