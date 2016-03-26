#os-team6

Project1

1.How to build kernel

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

3.Investigation of the process tree		 
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
