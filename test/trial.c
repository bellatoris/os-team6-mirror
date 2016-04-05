#include <stdio.h>
#include <linux/unistd.h>
#include <math.h>
#define __NR_set_rotation 384
#define __NR_rotlock_read 385
#define __NR_rotlock_write 386
#define __NR_rotunlock_read 387
#define __NR_rotunlock_write 388

struct dev_rotation{
	int degree;
};


struct rotation_range{
	struct dev_rotation rot;	/* device rotation */
	unsigned int degree_range;	/* lock range = rot.degree ± degree_range */
    /* rot.degree - degree_range <= LOCK RANGE <= rot.degree + degree_range */
};


int main(int argc, char* argv[]){
	struct rotation_range range;
	int n;
	range.rot.degree = 90;
	range.degree_range = 90;
	syscall(__NR_rotlock_read, &range);
	FILE* ff = fopen("integer", "r");
	fscanf(ff, "%d", &n);
	fclose(ff);
	printf("trial : ");
	print_prime(n);
	syscall(__NR_rotlock_read, &range);
}

void print_prime(int n){
	int i = 0;
	if(n <2){
		printf("%d\n", n);
		return;
	}
	for(i =2; i < n ; i++){
		if( (n % i) ==0){
			printf("%d *",i);
			print_prime( n /i);
			return; 
		}
	}
	printf("%d\n",n);    
}
