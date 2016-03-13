[33mcommit da3131adb0f781ab8a479d958ff89f914ed1e19e[m
Merge: 53dd959 17c2fc4
Author: bellatoris <bellatoris@snu.ac.kr>
Date:   Sun Mar 13 14:12:27 2016 +0900

    Merge branch 'proj1' into Doogie
    
    3/14 Doogie Proj1로 부터 Merge한다. gitignore파일을 update한다.

[33mcommit 53dd95976a178afcaf87b711f21529966e1d481e[m
Merge: c5619cc c6f092c
Author: bellatoris <bellatoris@snu.ac.kr>
Date:   Sun Mar 13 13:34:11 2016 +0900

    Merge branch 'Doogie' of https://github.com/swsnu/os-team6 into Doogie

[33mcommit c5619cc3bbcb4798dd209d9df49365739fbedd91[m
Author: bellatoris <bellatoris@snu.ac.kr>
Date:   Sun Mar 13 13:33:20 2016 +0900

    3/13 Doogie Merge에 의해 생겼던 Conlicts를 해결

[1mdiff --git a/arch/arm/boot/dzImage b/arch/arm/boot/dzImage[m
[1mindex 49b3e0e..d687050 100644[m
Binary files a/arch/arm/boot/dzImage and b/arch/arm/boot/dzImage differ
[1mdiff --git a/arch/arm/include/uapi/asm/unistd.h b/arch/arm/include/uapi/asm/unistd.h[m
[1mindex 0b4050c..6fb7294 100644[m
[1m--- a/arch/arm/include/uapi/asm/unistd.h[m
[1m+++ b/arch/arm/include/uapi/asm/unistd.h[m
[36m@@ -411,16 +411,12 @@[m
 #define __NR_sched_getattr		(__NR_SYSCALL_BASE+381)[m
 #define __NR_renameat2			(__NR_SYSCALL_BASE+382)[m
 */[m
[31m-<<<<<<< HEAD[m
 [m
[31m-#define __NR_seccomp			(__NR_SYSCALL_BASE+383)[m
[31m-=======[m
 #define __NR_dfs			(__NR_SYSCALL_BASE+381)[m
 #define __NR_ptree                      (__NR_SYSCALL_BASE+382)    [m
 #define __NR_seccomp			(__NR_SYSCALL_BASE+383)[m
 [m
 [m
[31m->>>>>>> Doogie[m
 /*[m
  * This may need to be greater than __NR_last_syscall+1 in order to[m
  * account for the padding in the syscall table[m
[1mdiff --git a/arch/arm/kernel/calls.S b/arch/arm/kernel/calls.S[m
[1mindex 528dc5e..19f4116 100644[m
[1m--- a/arch/arm/kernel/calls.S[m
[1m+++ b/arch/arm/kernel/calls.S[m
[36m@@ -390,12 +390,8 @@[m
 		CALL(sys_kcmp)[m
 		CALL(sys_finit_module)[m
 /* 380 */	CALL(sys_ni_syscall)		/* reserved sys_sched_setattr */[m
[31m-		CALL(sys_dfs)		/* reserved sys_sched_getattr */[m
[31m-<<<<<<< HEAD[m
[31m-		CALL(sys_ptree)		/* new funtion!     */[m
[31m-=======[m
[32m+[m		[32mCALL(sys_dfs)			/* reserved sys_sched_getattr */[m
 		CALL(sys_ptree)			/* reserved sys_renameat2     */[m
[31m->>>>>>> Doogie[m
 		CALL(sys_seccomp)[m
 [m
 #ifndef syscalls_counted[m
[1mdiff --git a/hihi.c b/hihi.c[m
[1mnew file mode 100644[m
[1mindex 0000000..a541834[m
[1m--- /dev/null[m
[1m+++ b/hihi.c[m
[36m@@ -0,0 +1,6 @@[m
[32m+[m[32m#include <stdio.h>[m
[32m+[m
[32m+[m[32mint main()[m
[32m+[m[32m{[m
[32m+[m[32m    printf("Hello World!\n");[m
[32m+[m[32m}[m
[1mdiff --git a/system_kernel.tar b/system_kernel.tar[m
[1mindex ad92341..620dada 100644[m
Binary files a/system_kernel.tar and b/system_kernel.tar differ

[33mcommit 0f5ea7717c75293d5520582aa91e2b0b3453f717[m
Author: bellatoris <bellatoris@snu.ac.kr>
Date:   Sat Mar 12 20:01:15 2016 +0900

    oops

[1mdiff --git a/README.md b/README.md[m
[1mindex bd0dfec..b5576b0 100644[m
[1m--- a/README.md[m
[1m+++ b/README.md[m
[36m@@ -1 +1,6 @@[m
 # os-team6[m
[32m+[m[32m3/12 수행한것: 프로세스 순회 및 트리 작성,[m[41m [m
[32m+[m		[32m테스트 함수 작성.[m
[32m+[m[32m해야할 것: 시스템 콜에서 에러관리, init노드 이름 관리, 형식 맞나 확인하기, 질문사항 정리하기[m
[32m+[m		[32m테스트 함수 형식에 맞게 바꾸기, 하나 더나오는것 없애기, 코드 깔끔하게 좀 쓰기[m
[32m+[m		[32m우리 모두 일요일 낮에 나와서 사진찍기, 모두다 깃공부 해오기[m
