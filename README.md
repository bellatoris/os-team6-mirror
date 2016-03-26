#os-team6

Project1

**1.How to build kernel**

a.시스템 콜 번호 추가

/arch/arm/include/uapi/asm/unistd.h에서 우리가 추가할 시스템 콜인 ptree에 해당하는 번호를 추가했다

```
#define __NR_ptree                      (__NR_SYSCALL_BASE+382) 
```
b.시스템 콜 테이블에 함수 등록

arch/arm/kernel/calls.S에 우리가 추가하는 시스템 콜을 처리할 함수를 등록했다.
이 때 더미 시스콜인 nisyscall의 번호를 ptree로 바꾸는 식으로 구현해서 시스콜의 갯수가 4의 배수가 아니면
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

 프로젝트의 목표가 프로세스트 트리를 dfs로 탐색하여 user가 원하는 만큼 prinfo를 복사하여
 주는것 이었기 때문에 user에게 제공되는 system call ptree함수와 프로세스 트리를 dfs로 탐색하는
 dfs함수 프로세스에게서 원하는 데이터를 얻어오는 visit함수를 구현하였다.
 
 -ptree함수
 ```
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
  (위의 함수는 구현을 설명하기 위해서 실제 구현한 함수에 비해 매우 생략되고 간략화 되어있음을
  염두해고 두고 보시기 바랍니다.)
  user에게서 prinfo potinter와 int pointer를 받아온다. 다만 kernel에서 함부로 user의 메모리 스페이스를 
  변경하거나 참조하는 것은 위험하기 때문에 buf와 nr을 그대로 쓰지 않고 리눅스에서 주어진 매크로를 
  사용하여 user에게서 값을 얻어오고 user로 값을 복사한다. 그러한 과정에서 에러가 났다면 적절한 에러값을
  리턴하도록 하였다. 또한 dfs를 하는 도중 task_struct list가 변경되는 것을 막기 위해 lock을 걸어주었다.
  
  -dfs함수 
  ```
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
  ```
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
   전체 프로세스개수를 리턴해야 하므로 *i는 visit할 때마다 증가하지만, *knr과 같아지는
   순간 정보의 복사는 일어나지 않고 *i의 개수만 커지게 된다.

**3.Investigation of the process tree**	 
 a
 
 b
 
 타이젠에서 앱을 실행시키면 launchpad의 child로 실행시킨 앱의 프로세스 하나가 추가된다 		
 앱을 종료하면 해당하는 프로세스가 종료된다.		
 				
 c_1
 
 타이젠 상에서 앱을 실행시키면 launchpad-process의 child인 launchpad-loader중 에서		
 하나가 그 앱의 이름으로 바뀐다.이때 이름을 제외한 pid, uid 등의 field값들이		
 변하지 않는 것으로 보아 새로운 앱이 생성되는 것이 아니라 loader중 하나가 바뀌는 것을 알 수 있다.		
 그와 동시에 새로운 launchpad-loader가 생성되어 총 3개의 Loader가 유지 된다.		
 				
 c_2
 
launchpad는 앱을 실행 시키는 명령을 받으면 권한이 있는지 판단해서 앱을 띄우는 역할을 한다.		
또한 Loader,앱의 fork,exit등을 관리한다. 		
Loader는 앱을 구성하는데필수적인 요소들을 미리 만들어서 앱의 실행 속도를 빠르게 한다.
