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
