#os-team6

Project1

**1.How to build kernel**

a.시스템 콜 번호 추가

/arch/arm/include/uapi/asm/unistd.h에서 우리가 추가할 시스템 콜인 ptree에 해당하는 번호를 추가했다.    

```
#define __NR_ptree                      (__NR_SYSCALL_BASE+382) 
```
b.시스템 콜 테이블에 함수 등록

arch/arm/kernel/calls.S에 우리가 추가하는 시스템 콜을 처리할 함수를 등록했다.
이 때 더미 시스콜인 ni syscall의 번호를 ptree로 바꾸는 식으로 구현해서 시스콜의 갯수가 4의 배수가 아니면 	
OS가 동작하지 않는 문제를 해결했다.  
```
CALL(sys_ni_syscall)		
CALL(sys_ni_syscall)		
CALL(sys_ptree)
CALL(sys_seccomp)
```

c.시스템 콜 함수 구현

kernel에 ptree.c라는 시스템 콜 함수를 구현하고 kernel 폴더의 Makefile에 ptree.o를 추가해서 커널
빌드에 추가 되도록 했다.

```
	obj-y =  fork.o exec_domain.o panic.o printk.o \
		...
		 smpboot.o ptree.o
```
d.커널 빌드

./build.sh tizen_tm1 USR 커맨드로 커널을 빌드하고, sdb를 이용해 생성된 tar파일을 기기에 Push한 후
reboot하면 새로운 커널을 사용할 수 있다.(프로젝트 리드미에 다 있는데 써야될까)가

**2.High-level design and impelementation**

 프로젝트의 목표가 process 트리를 dfs로 탐색하여 user가 원하는 만큼 prinfo를 복사하여
 주는것 이었기 때문에 user에게 제공되는 system call ptree함수와 process 트리를 dfs로 탐색하는
 dfs함수 process에게서 원하는 데이터를 얻어오는 visit함수를 구현하였다.
 
 -ptree함수
 ```c
	 asmlinkage int sys_ptree(struct prinfo __user *buf, int __user *nr)
	 {
	 	if (get_user(knr, nr) < 0)
	 		return -EFAULT;
	 	kbuf = kmalloc_arrary(knt, sizeof(struct prinfo), GFP_KERNEL);
	 	read_lock(&tasklist_lock);
		i = dfs(kbuf, &knr, &init_task);
		read_unlock(&tasklist_lock);
		copy_to_user(buf, kbuf, knr * sizeof(struct prinfo));
		return i;
	 }
 ```
  (위와 아래에 나온 모든 함수는 구현을 설명하기 위해서 실제 구현한 함수에 비해 매우 생략되고
  간략화 되어있음을 염두해고 두고 보시기 바랍니다.)
  user에게서 prinfo potinter와 int pointer를 받아온다. 다만 kernel에서 함부로 user의 메모리 스페이스를
  변경하거나 참조하는 것은 위험하기 때문에 buf와 nr을 그대로 쓰지 않고 리눅스에서 주어진 매크로를
  사용하여 user에게서 값을 얻어오고 user로 값을 복사한다. 그러한 과정에서 에러가 났다면 적절한 에러값을
  리턴하도록 하였다. 또한 dfs를 하는 도중 task_struct list가 변경되는 것을 막기 위해 lock을 걸어주었다.
  
  에러 핸들링  
  시스템 콜에서 에러 핸들링은 발생하는 에러를  errno-base.h에서 에러에 해당하는 값을 찾아
 -를 붙여서 리턴하는 방식으로 이루어 진다.
```c
if (buf == NULL || nr == NULL)
	return -EINVAL;
```
prinfo를 담을 but 나 최대 process수인 nr이 NULL이면 invalid argument인 -EINVAL을 리턴한다
```c
if (knr < 1)
	return -EINVAL;
```
최대 process수가 1보다 작으면 -EINVAL을 리턴한다
```c
if (kbuf == NULL)
		return -ENOMEM;
```
kmalloc에 실패했을 경우 memory가 부족하다는 의미인 -ENOMEM을 리턴한다

```c
if (get_user(knr, nr) < 0)
		return -EFAULT;

if (copy_to_user(buf, kbuf, knr * sizeof(struct prinfo)) < 0) {
		kfree(kbuf);
		return -EFAULT;
	}
```

user영역에서 nr 을 불러오거나, kbuf를 유저 영역으로 보내는데 실패하면 -EFAULT를 리턴한다
  
  -dfs함수 
  ```c
  	static int dfs(struct prinfo *kbuf, int *knr, struct task_struct *root)
  	{
		while (true) {
			visit(kbuf, knr, task, &i);
			if (!list_empty(&task->children)) {
				task = list_first_entry(&task->children,
						struct task_struct, sibling);
				continue;
			} else {
				while (list_is_last(&task->sibling,
					    &task->real_parent->children) &&
								    task != root) {
					task = task->real_parent;
				}
				if (task == root)
					break;
				task = list_next_entry(task, sibling);
			}
		}
		return i;
  	}
  ```
  이 함수는 ptree에서만 불리는 것이므로 static을 사용하여 외부에서 접근하지 못하게 하였고,
  kernel stack의 메모리가 overflow가 나는것을 조금이라도 방지하기 위해 recursive가 아닌
  while문을 사용하여 dfs를 구현하였다. task_struct의 접근은 list_head의 매크로를 사용하였다.
  
  -visit함수
  ```c
  	static void visit(struct prinfo *kbuf, int *knr,
  					struct task_struct *task, int *i)
  	{
  		if (*i < *knr) {
  			kbuf[*i].state = task->state;
  		}
  		(*i)++;
  	}
  ```
   dfs함수에서 visit이 불리면 kbuf로 task_struct의 정보를 복사하도록 구현하였다.
   전체 process 개수를 리턴해야 하므로 *i는 visit할 때마다 증가하지만, *knr과 같아지는
   순간 정보의 복사는 일어나지 않는다.

**3.Investigation of the process tree**	 

 a.

 반복적으로 test를 실행 할 때마다 test process의 pid가 1씩 증가했는데 일정 시간( 약 7초~10초)마다
 pid가 2씩 증가하는경우가 있었다. 그 이유는 전체 process를 살펴 봄으로써 알 수 있었는데
 pid가 2씩 증가 할 때마다 systemd-udevd의 자식 process가 생성되고 있었다.그 자식 process가
 pid를 먼저 차지해서 test의 pid가 2 증가하게 된것이다. systemd-udevd는 커널로 부터 uevent를
 받아서 device에 알맞은 instruction을 수행하는 daemon process이다.왜 주기적으로
 systemd-udevd의 자식이 생성되는지 알아보기 위해 udevadm monitor 커맨드를 사용해서
 다음과 같은 결과를 얻었다.  

```
monitor will print the received events for:
UDEV - the event which udev sends out after rule processing
KERNEL - the kernel uevent

KERNEL[14085.073604] change   /devices/sec-battery.32/power_supply/battery (power_supply)
UDEV  [14085.076809] change   /devices/sec-battery.32/power_supply/battery (power_supply)
....
....
```
 일정시간마다 udev monitor에 메시지가 나오는 것으로 보아 커널이 배터리를 확인하기 위한 event를 udev에게 보내면 그 때마다 udev의 child가 생성되고 있었음을 알 수 있다.  
 
 b. c_1.

 ```
 launchpad-proce, 1004, 1, 1, 1012, 1005, 0
	calender-widget, 1012, 1, 1004, 0, 1013, 5000
	launchpad-loade, 1013, 1, 1004, 0, 1743, 5000
	launchpad-loade, 1743, 1, 1004, 0, 5484, 5000
	launchpad-loade, 5484, 1, 1004, 0, 0, 5000
 ```
 Application 실행전
 ```
 launchpad-proce, 1004, 1, 1, 1012, 1005, 0
	calender-widget, 1012, 1, 1004, 0, 1013, 5000
	launchpad-loade, 1013, 1, 1004, 0, 1743, 5000
	launchpad-loade, 1743, 1, 1004, 0, 5484, 5000
	contacts, 5484, 1, 1004, 1, 0, 5612, 5000
	launchpad-loade, 5612, 1, 1004, 0, 0, 5000 
 ```
 contacts Applicaition 실행후 
 ```
 launchpad-proce, 1004, 1,1 1012, 1005, 0
	calender-widget, 1012, 1, 1004, 0, 1013, 5000
	launchpad-loade, 1013, 1, 1004, 0, 1743, 5000
	launchpad-loade, 1743, 1, 1004, 0, 5484, 5000
	launchpad-loade, 5642, 1, 1004, 0, 0, 5000
 ```
 contacts Applicatoin 종료후  
   
 타이젠에서 Application을 실행시키면 launchpad-process의 child로 실행시킨 Application의 process
 하나가 추가된다. 이 때 오히려 launchpad-loader의 pid가 바뀌는 것을 확인 할 수 있었다.  
 그러나 Application의 pid가 기존의 launchpad-loader중 하나의 pid와 같고 마지막 형제로 추가된게 
 아니고 중간 형제로 존재 하는 것으로 보아 launchpad-loader중 하나가 execev해서 Application이 되고,
 그 후 launchpad-process가 fork 후 execv해서 새로운 launchpad-loader를 만드는 것으로 보인다.
 Application을 종료하면 마지막 launchpad-loader의 pid가 바뀐것을 확인 할 수 있다.
 이는 우리가 Application을 종료하기 위하여 Home button을 꾹 누르고 있으면 Application을
 종료할 수 있게 도와주는 창이 뜨게 되는데 이 창이 task-mgr이라는 process라 이 process를 실행
 하기 위하여 launchpad-loader가 execv과정을 거치고 다시한번 launchpad-process가 fork/exec 과정을
 수행 했기 때문이다. 그래서 종료후에 보이는 마지막 launchpad-loader는
 새로 생겨난 launchpad-loader라 pid가 변경된 것이다.
 				
 c_2  
   
 launchpad-process란 AUL daemon이다. 여기서 AUL이란 Application Uility Library로 한 Application에서
 다른 Application을 런칭, 리쥼 혹은 종료시킬 떄 사용하는 API를 제공한다. 즉 우리가 Appliciation을 
 실행하려고 터치를 하면 homescreen이(caller) 우리가 실행
 시키고자 하는 Appliciation(callee)을 위의 API를 사용하여 런칭 하고자 시도 한다.
 `(실제 과정은 AUL api를 부르는 app-control api를 사용한다.)`
 그럼 AUL daemon이 런칭 요청을 받게
 되는데 앞서 본 launchpad-process가 바로 AUL daemon이다. launchpad-process는 사전에 fork/exec으로 
 만들어 놓은 process(launchpad-loader)를 활용하여 callee Application을 띄우게 된다.  
 **Application launching 이러한 과정을 거치는 이유는 launching을 빠르게 위함이다.**  
 fork/exec자체가 상당한 시간을 소요하기 때문에 launchpad-loader를 사전에 만들어 놓고, 윈도우 elm_win_add(),
 백그라운드 elm_bg_edd() 컴포먼트 elm_conformant_add()등 Application을 구성하는 필수적인 요소들을 사전에 
 만들어 놓는다. 이렇게 사전에 만들어 둔 process가 application의 executable file을
 load 하고 callee Application의 main 함수를 dlsym함수를 통해 load 하여 Application을 빠르게 launching한다.
 정리 하자면 다음과 같다.
 ```
 • App-control API
	– API for launching application
	– homescreen uses app-control API to launch an application
• launchpad
	– Parent process of all applications
	– Handles launch request
	– Manages launchpad-loaders
• launchpad-loader
	– Pre-initialized process for applications
	– launchpad-loader is changed to real application
 ```
