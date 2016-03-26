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

 a. 반복적으로 test를 실행 할 때마다 test 프로세스의 pid가 1씩 증가했는데 일정 시간( 약 7초~10초)마다 pid가 2씩 증가하는 경우가 있었다. 그 이유는 전체 프로세스를 살펴 봄으로써 알 수 있었는데 Pid가 2씩 증가 할 때마다 systemd-udevd의 자식 프로세스가 생성되고 있었다.그 자식 프로세스가 pid를 먼저 차지해서 test의 pid가 2 증가하게 된것이다.
systemd-udevd는 커널로 부터 uevent를 받아서 device에 알맞은 instruction을 수행하는 daemon process이다.왜 주기적으로 systemd-udevd의 자식이 생성되는지 알아보기 위해 udevadm monitor 커맨드를 사용해서 다음과 같은 결과를 얻었다.
```
monitor will print the received events for:
UDEV - the event which udev sends out after rule processing
KERNEL - the kernel uevent

KERNEL[14085.073604] change   /devices/sec-battery.32/power_supply/battery (power_supply)
UDEV  [14085.076809] change   /devices/sec-battery.32/power_supply/battery (power_supply)
....
....
```
앞서 말했던 일정시간마다 udev monitor에 메시지가 나오는 것으로 보아 
커널이 배터리를 확인하기 위한 event를 udev에게 보내면 그 때마다 udev의 child가 생성되고 있었음을
알 수 있다.


 
 b. 타이젠에서 앱을 실행시키면 launchpad의 child로 실행시킨 앱의 프로세스 하나가 추가되고,
 앱을 종료하면 해당하는 프로세스가 종료된다. 이 때 앱의 pid 가 기존의 loader중 하나의 pid와 
 같은 것으로 보아 loader중 하나가 execev해서 loader가 되고, launchpad가 fork 후 execv해서
 새로운 loader를 만드는 것으로 보인다.
 				
