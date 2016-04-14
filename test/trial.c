#include <stdio.h>
#include <linux/unistd.h>
#include <math.h>
#include <unistd.h>
#define __NR_set_rotation 384
#define __NR_rotlock_read 385
#define __NR_rotlock_write 386
#define __NR_rotunlock_read 387
#define __NR_rotunlock_write 388
int prime[10000];


struct dev_rotation{
	int degree;
};


struct rotation_range{
	struct dev_rotation rot;	/* device rotation */
	unsigned int degree_range;	/* lock range = rot.degree ± degree_range */
    /* rot.degree - degree_range <= LOCK RANGE <= rot.degree + degree_range */
};

void print_prime(int n);

int main(int argc, char* argv[]){
	struct rotation_range range;
	int n;
	int i,j;
	int obj;
	prime[0]=2;
	for(i=0;i< 10000;i++){
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
	range.rot.degree = 90;
	range.degree_range = 90;
	do {
	syscall(__NR_rotlock_read, &range);
	FILE* ff = fopen("integer", "r");
	fscanf(ff, "%d", &n);
	fclose(ff);
	printf("trial : ");
	print_prime(n);
	usleep(1000* 100);
	syscall(__NR_rotunlock_read, &range);
	} while(1);
}

void print_prime(int n){
	int i;
	if(n <2){
		return;
	}
	for(i =0; i < 10000 ; i++){
		if(n == prime[i]){
			printf("%d \n" ,prime[i]);	
			break;
		}
		if( (n % prime[i]) ==0){
			printf("%d * ",prime[i]);
			n = (n / prime[i]);
			i--; 
		}
	} 
}
