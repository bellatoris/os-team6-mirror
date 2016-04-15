# os-team6

제출할 코드는 proj12branch에 모두 merge해 두었습니다.
System call의 추가와 관련해서
/arch/arm/include/uapi/asm/unistd.h
/arch/arm/kernel/calls.S
/kernel/rotation.c
/kernel/exit.c
/kernel/Makefile
/include/linux/rotation.h
/include/linux/rotexit.h
을 변경/생성 하였고 trial, selector 함수는 test폴더에 Makefile과 함께 들어있습니다.

**1.high-level design (policy)**  
rotation 맞춰서 동작하는 read/write lock을 구현하기 위한 4가지 시스템 콜과, 
device의 rotation을 임의로 생성하는 daemon을 위한 시스템콜 하나를 구현했다.

1. policy  
acquire된 write lock과 range가 겹치는 lock은 그 lock이 unlock될 때 까지 wait한다.  
read lock은 acquire된 read lock이 있더라도 lock을 acquire 할 수 있다.   
write의 starvation을 막기 위해서 wait writer가 존재하는 range에는 새로운 read lock은 절대로 lock을 acquire 하지 못하도록 했고
read가 먼저 lock을 요구 했더라도 write가 먼저 lock을 acquire 한다  
예를 들어 (30,60)의 write가 acquired/wait되어 있다면 (10,30)과 (60,100) read는 write와 각각 30 , 60에서 겹치기 때문에 
(30, 60) write가 사라질 때 까지 lock을 acquire 할 수 없다.  
wait하는 lock이 여럿인 경우에는 lock을 요구한 순서 대로 lock을 acquire 한다.

2. rotation_range, dev_rotation 
kernel에 rotation, range를 전달하기 위해 rotation_range와 dev_rotation 이라는 구조체를 사용했다.
```c
struct rotation_range {
    struct dev_rotation rot;  /* device rotation */
    unsigned int degree_range;      /* lock range = rot.degree ± degree_range */
    /* rot.degree - degree_range <= LOCK RANGE <= rot.degree + degree_range */
};

struct dev_rotation {
    int degree;     /* 0 <= degree < 360 */
};

```

3. sys_set_rotation  
user level에서 rotation을 받으면 kernel의 dev_rotation에 rotation을 copy한다.
또한 해당 rotation에 wait하고 있고 일어 날 수 있는 read/write l의ock이 존재 한다면 깨우고, 깨운 
process의 수를 return 한다. 에러가 발생하면 -1을 리턴한다.

4. sys_rotlock_read / sys_rotlock_write  
user level에서 ratation_range를 받으면 해당 각도에 lock을 잡는다. 
range가 겹치고 acquired 된 Lock이 이미 존재하거나/ 해당 각도가 아닌 경우 schdule되고 signal을 기다린다.
시스템 콜 내에서 에러가 나면 -1/ 정상적으로 종료된 경우 0을 리턴한다.

5. sys_rotunlock_read / sys_rotunlock_write  
user level에서 ratation_range를 받으면 해당 각도의 lock을 unlock한다. unlock은 어느 시점에서도 가능하다. unlock 후 깨어날 수 있는 process가 존재하면 확인 하고 signal을 보낸다.
시스템 콜 내에서 에러가 나면 -1/ 정상적으로 종료 경우 0을 리턴한다.

**2.implementation**  
 

extern을 이용해서 커널 내부에 전역변수로 rotation을 선언한다.
queue의 경우 waiting_writer,acquire_writer, waitgin_reader, acquire_reader로 4개를 전역 변수로 선언했고
각 quque에 add/remove하는 함수를 따로 만들어서 사용했다.
```c
extern struct dev_rotation rotation;
extern struct lock_queue waiting_writer;
extern struct lock_queue acquire_writer;
extern struct lock_queue waiting_reader;
extern struct lock_queue acquire_reader;
```
단 queue의 첫번째 entry가 현재 rotation_lock을 range에 포함하는지 탐색하는 함수는 하나로 만들었다.
```c
static int traverse_list_safe(struct rotation_lock *rot_lock,
                                                struct lock_queue *queue)
{
        struct rotation_lock *curr, *next;
        list_for_each_entry_safe(curr, next, &queue->lock_list, lock_list) {
                if (rot_lock->max >= curr->min && rot_lock->min <= curr->max) {
                        printk("range overlap!!\n");
                        return 1;
                }
        }
        return 0;
}
```

1. range 계산을 위한 struct, macro, fuction들   
range의 계산을 편하게 하기 위해서  rotation_lock structure를 정의했다.
```c
struct rotation_lock {
        int min;
        int max;
        pid_t pid;
        struct list_head lock_list;
};
```
rotation_lock를 초기화 할 때 다음과 같은 함수를 사용했다.
degree + range 가 360보다 작으면 360을 더하고 max,min에 할당한다.
```c
inline void init_rotation_lock(struct rotation_lock *lock,
                        struct task_struct *p, struct rotation_range *rot)
{     
        lock->max = rot->rot.degree + rot->degree_range > 360 ?
                            rot->rot.degree + rot->degree_range : 
                            rot->rot.degree + rot->degree_range + 360;
    
        lock->min = rot->rot.degree + rot->degree_range > 360 ?
                            rot->rot.degree - rot->degree_range :
                            rot->rot.degree - rot->degree_range + 360;
    
    
        lock->pid = p->pid;
        lock->lock_list.prev = &lock->lock_list;
        lock->lock_list.next = &lock->lock_list;
}

```
현재 rotation도 비교하기 전에 초기화 하기 위해 아래의 매크로를 사용했다.
rotation의 degree가 rotation_lock의 min 보다 작으면 360 +degree, 크면 degree를 name에게 초기화하는 매크로이다.
```c
#define SET_CUR(name, rot, degree) \
        (name = (rot->min <= degree) ? degree : \
        degree + 360)
```
초기화된 rotation_lock 안에 name이 존재 하는지 확인하면 range에 포함되는지를 알 수 있다.
```c
	rot_lock->max >= name->min && rot_lock->min <= name->max
```


2. sys_set_rotation  
copy_from_user를 이용해서 커널 내부의 rotation에 값을 넣고
잘못된 rotation값에 대해서 error를 출력한다. 
```c
asmlinkage int sys_set_rotation(struct dev_rotation __user *rot)
{

 if (copy_from_user(&rotation.degree, &rot->degree, sizeof(struct dev_rotation))!=0)
                return -EFAULT;

 if (rotation.degree < 0 ||rotation.degree > 359)
                return -EINVAL;


}
```

Process를 깨우는 것은 thread_cond_signal, thread_cond_broadcast를 이용해서 구현했다.
signal은 writer들의 waiting queue를 돌며 현재 조건에서 일어 날수 있는 첫번째 process를 
while문을 돌면서 깨운다.이 때 일어 날 수 없는 조건은 현재 rotation이 range에 포함 되지 않거나
현재 rotation에 acquire_writer,acquire_reader가 존재하는 경우이다
thread_cond_braodcast는 일어 날 수 있는 모든 reader들을 while문을 돌면서 깨운다.
```c
static int thread_cond_broadcast(void)
{ 
	...
	if (cur <= curr->max && cur >= curr->min) {
		if (!traverse_list_safe(curr, &acquire_writer) && !traverse_list_safe(curr, &acquire_reader)){
			while (WAKE_UP(curr) != 1) {}// queue에는 존재하지만 아직 schedule되지 않았을 경우를 위해서 반복문을 돈다
  		}
	}
}
```
3. sys_rotlock_read / sys_rotlock_write  

잘못된 rotation으로 lock을 잡으려고 하거나, kernel에 메모리가 부족한 경우 에러를 리턴한다.
```c
asmlinkage int sys_rotlock_read(struct rotation_range __user *rot){
	//error_check.1  kmalloc is fine?
        if (klock == NULL)
                return -ENOMEM;

        printk("sys_rotlock_write %p\n", klock);

        //error_check.2  copy_from_user returns 0 when it succeeds.
        if (copy_from_user(&krot, rot, sizeof(struct rotation_range)) != 0)
                return -EFAULT;
        //error_check.3  range_degree should be positive + 0;
        if (krot.degree_range <0)
                return -EINVAL;
        //error_check.4  0 =< rot.degree <360
        if (krot.rot.degree < 0 ||krot.rot.degree > 359)
                return -EINVAL;
   }
```
my_lock을 spin_lock으로 잡은 후에 queue에 접근하도록 해서 wating, acquired queue에 동시에 접근하는
일을 막았다. 현재 rotlock을 잡을 수 있는지 판단 후 잡을 수 있다면 spin_lock을 풀고 잡을 수 없다면
spin_lock을 풀고 schedule되어 signal을 기다린다. 

```c
static void __sched thread_write_wait(){

        spin_unlock(&my_lock);
        set_current_state(TASK_INTERRUPTIBLE);
        schedule();


}

```

4. sys_rotunlock_read / sys_rotunlock_write  
잘못된 rotation으로 unlock하려고 할때 에러를 리턴한다.

```c

asmlinkage int sys_rotunlock_read(struct rotation_range __user *rot){
 				...
	//error_check.2  copy_from_user returns 0 when it succeeds.
        if (copy_from_user(&krot, rot, sizeof(struct rotation_range)) != 0)
                return -EFAULT;
        //error_check.3  range_degree should be positive + 0;
        if (krot.degree_range <0)
                return -EINVAL;
        //error_check.4  0 =< rot.degree <360
        if (krot.rot.degree < 0 ||krot.rot.degree > 359)
                return -EINVAL;
                ...
}
```
lock과 마찬가지로 my_lock으로 spin_lock을 잡고 queue에 접근한다.
reader에 block되는 것은 writer 뿐이므로 unlock_read는 signal함수만을 호출하고
unlock_write는 signal/broadcast 둘 다 호출한다.
```c
asmlinkage int sys_rotunlock_read(struct rotation_range __user *rot){
	...

	if (!flag)
    	thread_cond_signal();
    ...
}

```
5. exit_loclock
process가 중간에 종료될 경우
lock을 잡고 모든 queue에서 해당 process의 pid를 가진 entry를 제거한다

**3.lesson learned**  

멀티쓰레딩 상태에서 Heisenbug의 위험성을 알게 되었다.  
멀티쓰레딩에서 lock의 중요성을 알게 되었다.  
일반  lock에 비해 fine grain lock의 장점을 알게 되었다.  
Preemption과 스케줄을 조절할 수 있다는 것을 알았다.
