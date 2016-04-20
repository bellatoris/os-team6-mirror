# os-team6

제출할 코드는 proj2 branch에 모두 merge해 두었습니다.  
System call의 추가와 관련해서  
/arch/arm/include/uapi/asm/unistd.h  
/arch/arm/kernel/calls.S  
/kernel/rotation.c  
/kernel/exit.c  
/kernel/Makefile  
/include/linux/rotation.h  
/include/linux/rotexit.h  
을 변경, 생성 하였고 trial, selector 함수는 test폴더에 Makefile과 함께 들어있습니다.  

trial은 integer파일에 들어 있는 정수를 소인수 분해해서 출력합니다.  
trial은 trail-divesion을 사용해서 1 만번째 소수까지 미리 구하고 이를 이용해서 소인수 분해를 합니다.  
trial은 인자를 넣지 않으면 소인수 분해 결과값에 prefix로 "trial: " 이 출력되고 인자를 넣을 경우 printf("trial%d :" ,인자) 처럼 출력됩니다.  
selector는 반드시 인자를 넣어야 하고 그 인자에 +0,1,2,3 ...한 값을 계속해서 integer에 입력합니다.  
그리고 입력한 integer값을 "selector: " prefix와 함께 출력합니다.

**1.high-level design**  
rotation 맞춰서 동작하는 read/write lock을 구현하기 위한 4가지 시스템 콜과, 
device의 rotation을 임의로 생성하는 daemon을 위한 시스템콜 하나를 구현했다.

* Policy   
기본적인 policy는 `reader가 acquire한 lock이 writer가 원하는 lock일 때만 그 writer와 겹치는
reader가 오면 그 reader는 wait해야 한다` 였다. 그러나 이러한 policy는 사용자에게 혼란을 준다고 생각하였고, (같은
범위의 reader lock이어도 온 순서에 따라 혹은 현재 어떤 lock이 잡고 있느냐에 따라 lock을 잡을 수도 있고 못잡을 수도 있으므로)
그래서 우리는 policy를 `reader가 waiting인 writer와 range가 overlap될 경우 wait한다.` 로 결정하였다. 즉 언제나 writer를 최우선으로 두며 writer와 range가 overlap되는 reader는 언제나 wait한다.

* rotation\_range, dev\_rotation   
kernel에 rotation, range를 전달하기 위해 rotation\_range와 dev\_rotation 이라는 구조체를 사용했다.
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

* sys\_set\_rotation  
User level에서 rotation을 받으면 kernel의 dev\_rotation에 rotation을 copy한다.
또한 깨어날 수 있는 waiting writer/reader들을 깨운다. `(여기서 "깨어날 수 있다"는 말의 의미는 현재 rotation이 range안에 들어와 있으며, reader의 경우 자신의 range와 overlap되는  waiting writer, acquired writer가 없을 경우,  writer의 경우 자신의 range와 overlap되는 acquired reader, acquired writer가 없을 경우를 말한다. 앞으로 쓰이는 "깨어날 수 있다"는 동일한 의미로 쓰인다.)`
마지막으로 깨어난 process의 수를 return 한다. 에러가 발생하면 -1을 리턴한다.

* sys\_rotlock\_read / sys\_rotlock\_write   
User level에서 rotation\_range를 받으면 현재 rotation이 range안에 들어 왔을 때 다음 조건을 만족하면 lock을 잡는다. 
Reader의 경우 자신의 range와 overlap되는 waiting writer, acquired writer가 없을 경우 lock을 잡을 수 있으며
Writer의 경우 자신과 range와 overlap되는 acquired reader, acquired writer가 없을 경우 lock을 잡을 수 있다.
그러나 이러한 조건을 만족 하지 못할 경우 자신의 area descriptor를 wait queue에 넣고, signal이나 broadcast가 올 때까지 sleep한다.
시스템 콜 내에서 에러가 나면 -1, 정상적으로 종료된 경우 0을 리턴한다.

* sys\_rotunlock\_read / sys\_rotunlock\_write  
User level에서 rotation\_range를 받으면 해당 rotation의 lock을 unlock한다. Unlock은 어느 시점에서도 가능하다. unlock 후 waiting writer에게 signal을 보내 깨어날 수 있는 waiting writer가 있으면 깨우고, 없다면 waiting reader에게 broadcast하여 깨어날 수 있는 모든 reader들을 다 깨운다 (reader의 경우 signal만 이루어 진다).
시스템 콜 내에서 에러가 나면 -1, 정상적으로 종료될 경우 0을 리턴한다.

**2.implementation**  
* Area descriptor  
Wait queue에 넣을 area descriptor는 다음과 같이 정의 하였다.

```c
struct rotation_lock {
        int min;
        int max;
        pid_t pid;
        struct list_head lock_list;
};
```

Rotation이 0도 보다 작아지거나 360도 보다 커지는 경우에도 쉽게 범위 비교를 하기 위해
rotation_lock를 초기화 할 때 다음과 같은 함수를 사용했다.
degree + range 가 360보다 작으면 360을 더하고 max, min에 할당한다.
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
현재 rotation도 비교하기 전에 area descriptor와 scale을 맞춰 주기 위해 다음과 같은 매크로를 사용하였다.
이러한 방법을 사용해 330 ~ 90 같은 경우도 쉽게 비교를 할 수 있다.
```c
#define SET_CUR(name, rot, degree) \
        (name = (rot->min <= degree) ? degree : \
        degree + 360)
```

* Wait queue, Acquire queue  
Queue의 경우 각각의 경우 마다 정의하여 4가지로 다음과 같이 정의 하였다.
경우마다 traverse해야하는 queue가 다르기 때문이다. 이 queue에 접근 하는 부분은
모두 spinlock (global한) 을 사용하여 critical section으로 만들어 두었다. 
```c
extern struct lock_queue waiting_writer;
extern struct lock_queue acquire_writer;
extern struct lock_queue waiting_reader;
extern struct lock_queue acquire_reader;
```  

* sys\_set\_rotation  
copy\_from\_user를 이용해서 커널 내부의 rotation에 값을 넣고 잘못된 rotation값에 대해서 에러를 출력한다. 

```c
asmlinkage int sys_set_rotation(struct dev_rotation __user *rot)
{
	int i;
	if (copy_from_user(&rotation.degree, &rot->degree,
				    sizeof(struct dev_rotation)) != 0)
		return -EFAULT;

	if (rotation.degree < 0 || rotation.degree > 359)
		return -EINVAL;

	spin_lock(&my_lock);
	i = thread_cond_signal();
	if (!i)
		i = thread_cond_broadcast();
	spin_unlock(&my_lock);

	return i;
}

```

* sys\_rotlock\_read / sys\_rotlock\_write  
잘못된 rotation으로 lock을 잡으려고 하거나, kernel에 메모리가 부족한 경우 에러를 리턴한다.
my\_lock을 spin\_lock으로 잡은 후에 queue에 접근하도록 해서 wating, acquire queue에 동시에 접근하는
일을 막았다. 
```c
asmlinkage int sys_rotlock_read(struct rotation_range __user *rot)
{
	struct rotation_range krot;
	struct rotation_lock *klock = kmalloc(sizeof(struct rotation_lock),
								GFP_KERNEL);
	if (klock == NULL)
		return -ENOMEM;

	if (copy_from_user(&krot, rot, sizeof(struct rotation_range)) != 0)
		return -EFAULT;

	if (krot.degree_range <= 0)
		return -EINVAL;

	if (krot.rot.degree < 0 || krot.rot.degree > 359)
		return -EINVAL;

	init_rotation_lock(klock, current, &krot);

	spin_lock(&my_lock);
	add_read_waiter(klock);
	if (read_should_wait(klock))
		thread_cond_wait();
	spin_unlock(&my_lock);

	return 0;
}
```
* thread\_cond\_wait  
spin_lock을 풀기 전에 set_current_state를 호출해서 state를 TASK_INTERRUPTIBLE로 변경한다. spin_lock을 풀고, schedule을  호출해서 state가 TASK_INTERRUPTIBLE이라면 run_queue에서 빠진다.
state가 TASK_RUNNING이 되면 다시 spin_lock을 잡고 wait을 끝낸다.  

```c
static void __sched thread_cond_wait(void)
{
	set_current_state(TASK_INTERRUPTIBLE);
	spin_unlock(&my_lock);
	schedule();
	spin_lock(&my_lock);
}

```

* Read unlock, Write unlock  
잘못된 rotation으로 unlock하려고 할때 에러를 리턴한다. my\_lock으로 spin\_lock을 잡고 queue에 접근한다.
reader에 block되는 것은 writer 뿐이므로 unlock\_read는 signal함수만을 호출하고
unlock\_write는 signal, broadcast 둘 다 호출한다.

```c
asmlinkage int sys_rotunlock_read(struct rotation_range __user *rot)
{
	if (copy_from_user(&krot, rot, sizeof(struct rotation_range)) != 0)
		return -EFAULT;

	if (krot.degree_range <= 0)
		return -EINVAL;
		
	if (krot.rot.degree < 0 || krot.rot.degree > 359)
		return -EINVAL;

	spin_lock(&my_lock);
	flag = remove_read_acquirer(&krot);
	if (!flag)
		thread_cond_signal();
	spin_unlock(&my_lock);
	return flag;
}
```

* thread\_cond\_signal / thread\_cond\_broadcast  
Process를 깨우는 것은 thread\_cond\_signal, thread\_cond\_broadcast를 이용해서 구현했다.
Signal은 waiting writer queue를 돌며 깨어날 수 있는 첫번째 process를 깨운다.
Braodcast는 waiting reader queue를 돌면서 깨어날 수 있는 모든 reader들을 깨운다.
다만 이때 acquire queue에 추가하여, 깨어나야 하는 process가 혹시라도 scheduling 때문에 lock을 못얻을 가능성을 배제하였다.
```c
static int thread_cond_broadcast(void)
{
	list_for_each_entry_safe(curr, next, &waiting_reader.lock_list,
								lock_list) {
		SET_CUR(cur, curr, degree);
		if (cur <= curr->max && cur >= curr->min) {
			if (!traverse_list_safe(curr, &acquire_writer) &&
				!traverse_list_safe(curr, &waiting_writer)) {
				task = FIND(curr);
				while (wake_up_process(task) != 1)
					continue;
				remove_read_waiter(curr);
				add_read_acquirer(curr);
				i++;
			}
		}
	}
	return i;
}
```

* Exit\_lock  
process가 중간에 종료될 경우, spinlock을 잡고 모든 queue에서 해당 process의 pid를 가진 entry를 제거한다.
```c
void exit_rotlock()
{
	struct rotation_lock *curr, *next;
	spin_lock(&my_lock);
	list_for_each_entry_safe(curr, next, &acquire_writer.lock_list,
								lock_list) {
		if (current->pid == curr->pid) {
			list_del_init(&curr->lock_list);
			kfree(curr);
		}
	}
	spin_unlock(&my_lock);
}
```

**3.lesson learned**  

* 멀티쓰레딩 상태에서 Heisenbug의 위험성을 알게 되었다.  
* 멀티쓰레딩에서 lock의 중요성을 알게 되었다.  
* 일반 lock에 비해 fine grained locking의 장점을 알게 되었다.  
* process의 state를 변경해서 스케줄을 조절할 수 있다는 것을 알았다.
