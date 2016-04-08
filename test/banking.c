#include <stdio.h>
#include <stdlib.h>
#include <linux/unistd.h>
#define __NR_rotlock_write 386
#define __NR_rotunlock_write 388

struct dev_rotation{
	int degree;
};

struct rotation_range{
        struct dev_rotation rot;        /* device rotation */
        unsigned int degree_range;      /* lock range = rot.degree ± degree_range */
    /* rot.degree - degree_range <= LOCK RANGE <= rot.degree + degree_range */
};
int balance;

void* deposit()
{
    int i;	
    struct rotation_range range;
    range.rot.degree = 180;
    range.degree_range = 150;

    FILE *fp;

    for (i = 0; i < 1e4; i++) {
	syscall(__NR_rotlock_write, &range);

	fp = fopen("balance", "r");
	fscanf(fp, "%d", &balance);
	fclose(fp);
	++balance;
	fp = fopen("balance", "w");
	fprintf(fp, "%d", balance);
	fclose(fp);
	//printf("%d\n", balance);

	syscall(__NR_rotunlock_write, &range);
    }
}

void* withdraw()
{
    int i;	
    struct rotation_range range;
    range.rot.degree = 180;
    range.degree_range = 150;

    FILE *fp;

    for (i = 0; i < 1e4; i++) {
	syscall(__NR_rotlock_write, &range);

	fp = fopen("balance", "r");
	fscanf(fp, "%d", &balance);
	fclose(fp);
	--balance;
	fp = fopen("balance", "w");
	fprintf(fp, "%d", balance);
	fclose(fp);
	//printf("%d\n", balance);

	syscall(__NR_rotunlock_write, &range);
    }
}

int main()
{   
	struct rotation_range range;
	range.rot.degree = 180;
	range.degree_range = 150;

	FILE* fp;

	if (fork()) {
	    deposit();
	} else {
	    withdraw();
	}
	
	syscall(__NR_rotlock_write, &range);
	fp = fopen("balance", "r");
	fscanf(fp, "%d", &balance);
	fclose(fp);
	printf("balance = %d\n", balance);
	syscall(__NR_rotunlock_write, &range);

        return 0;
}
