제출할 코드는 proj3 branch에 모두 merge해 두었습니다.  
System call의 추가와 관련해서  
/arch/arm/include/uapi/asm/unistd.h  
/arch/arm/kernel/calls.S  
/kernel/sched/core.c  
/kernel/sched/sched.h  
/kernel/sched/wrr.c  
/kernel/sched/rt.c  
/kernel/sched/fair.c  
/include/linux/wrr.h  
include/linux/init_task.h  
include/linux/sched.h  
include/linux/syscalls.h  
include/uapi/linux/sched.h  
을 변경, 생성하였고 trial 함수는 test폴더에 Makefile과 함께 들어있습니다.  

또한 debug를 위해서  
arch/arm/configs/tizen_tm1_defconfig  
kernel/sched/debug.c  
를 변경하였습니다.  
**1.high-level design**  
* policy  
wrr이란 task마다 할당된 weight가 있고 weight * base time slice 의 time slice를 갖고서 round robin scheduling을 수행하는 policy다. 이를 위해서는 task가 weight, timle slice에 관한 data를 가져야 한다.base time slice는 10ms이고, wrr task의 default weight는 10, time slice는 100ms 이다.  
* 새로운 sched_class  
systemd의 child process를 모두 WRR(weighted round robin)으로 scheduling 하기 위해서 wrr_sched_class라는 새로운 sched_class를 만들고 scheduling에 필요한 함수들을 구현했다.  
* 새로운 run queue  
새로운 run queue를 cpu마다 갖게 하고 wrr_sched_class가 사용하는 함수들은 이 run queue에 접근하여 cpu별로 wrr scheduling을 수행한다. 
* Load balance  
cpu간에 task가 불균형하게 할당되는 것을 최소화 하기 위해서 2가지 정책을 사용한다.  
1.policy가 wrr인 task들의 weight 합이 최소인 cpu에 새로운 task(wrr)를 enqueue한다  
2.*load balance* : 2000ms 마다 weight합이 최대인 cpu에서 weight합이 최소인 cpu로 옮길 수 있고 가장 weight가 큰 task를 하나 옮긴다. 옮긴 후에 최소인 cpu의 weight합이 최대인 cpu보다 크거나 같으면 더 weight가 작은 task를 옮기고, 그런 task가 존재 하지 않으면 옮기지 않는다. 
* 새로운 시스템 콜  
1.``` int sched_setweight(pid_t pid, int weight) ```  
policy가 wrr인 task의 weight를 설정한다. policy가 wrr이 아니면 에러를 리턴한다. pid가 0이면 시스템 콜을 부른 task 자신을 의미한다. task의 weight를 증가 시키는 것은 root user만 가능하고, 감소 시키는 것은 root user와 task를 own한 user에게 가능하다. 그 외의 user가 시스템 콜을 부르면 에러를 리턴한다.  
2.``` int sched_getweight(pid_t pid)```  
policy가 wrr인 task의 weight를 리턴한다. Policy가 wrr이 아니면 에러를 리턴한다. 

**2.implementation**  
* wrr_rq와 sched_wrr_entity  
wrr_sched_class가 policy 가 wrr인 task들을 스케줄링 하기 위해서 쓰일 run queue와 entity로 wrr_rq와 sched_wrr_entity struct를 사용했다.
```c
struct wrr_rq {
        unsigned int wrr_nr_running;
        struct list_head wrr_queue;
        /*
         *   wrr_load is sum of all weights in this queue
         */
        unsigned long wrr_load;
};
```
wrr_nr_runnging은 queue에 들어있는 wrr task의 수, wrr_queue는 queue 구현을 위한 list_head wrr_load는 task들의 weight 합이다. 

```c
struct sched_wrr_entity {
        struct list_head        run_list;
        unsigned int            time_slice;
        unsigned int            weight;
};
 ```
entity에는 time_slice, weight를 저장한다.
* wrr_sched_class의 함수들  

1.enqueue_task_wrr  
```c
static void enqueue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
        struct wrr_rq *wrr_rq = &rq->wrr;
        struct sched_wrr_entity *wrr_se = &p->wrr;
        ...
        enqueue_wrr_entity(wrr_rq, wrr_se, flags);
        ...
}
static void enqueue_wrr_entity(struct wrr_rq *wrr_rq,
                        struct sched_wrr_entity *wrr_se, int flags)
{
        list_add_tail(&wrr_se->run_list, &wrr_rq->wrr_queue);
        wrr_rq->wrr_load += wrr_se->weight;
        wrr_rq->wrr_nr_running++;
}

```
wrr_rq에 wrr_se를 넣고, wrr_rq의 load, nr_running을 증가시킨다.  enqueue, dequeue 등는 wrr_rq에 접근하는 함수는 lock이 필요하지만 enqueue 등을 호출하는 함수들이 모두 호출 하기 전에 rq lock을 걸기 때문에 sched_class의 level에서는 race contion을 고려할 필요가 없었다.

2.dequeue_task_wrr  
```c
static void dequeue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
        struct wrr_rq *wrr_rq = &rq->wrr;
        struct sched_wrr_entity *wrr_se = &p->wrr;
        ...
        dequeue_wrr_entity(wrr_rq, wrr_se, flags);
        ...
}
static void dequeue_wrr_entity(struct wrr_rq *wrr_rq,
                        struct sched_wrr_entity *wrr_se, int flags)
{

        list_del_init(&wrr_se->run_list);
        wrr_rq->wrr_load -= wrr_se->weight;
        wrr_rq->wrr_nr_running--;
}

```
wrr_rq에서 wrr_se를 빼고, wrr_rq의 load, nr_running을 감소시킨다.  

3.yield_task_wrr  
```c
static void yield_task_wrr(struct rq *rq)
{
        requeue_task_wrr(rq, rq->curr);
}
static void requeue_task_wrr(struct rq *rq, struct task_struct *p)
{
        struct sched_wrr_entity *wrr_se = &p->wrr;
        struct wrr_rq *wrr_rq = &rq->wrr;
        requeue_wrr_entity(wrr_rq, wrr_se);
}
static void
requeue_wrr_entity(struct wrr_rq *wrr_rq, struct sched_wrr_entity *wrr_se)
{
        list_move_tail(&wrr_se->run_list, &wrr_rq->wrr_queue);
}
```
wrr_rq의 맨 뒤로 task를 보낸다.  

4.pick_next_task_wrr  
```c
static struct task_struct *pick_next_task_wrr(struct rq *rq)
{
        struct task_struct *p = _pick_next_task_wrr(rq);
        return p;
}
static struct task_struct *_pick_next_task_wrr(struct rq *rq)
{
        ...
        wrr_rq = &rq->wrr;
        wrr_se = list_first_entry(&wrr_rq->wrr_queue,  struct sched_wrr_entity, run_list);
        p = task_of(wrr_se);
        return p;
}
```
wrr_rq의 맨 앞에 있는 task를 리턴 한다.  

5.task_tick_wrr  
```c
static void task_tick_wrr(struct rq *rq, struct task_struct *p, int queued)
{
        struct sched_wrr_entity *wrr_se = &p->wrr;

        if (--wrr_se->time_slice)
                return;
        wrr_se->time_slice = WRR_TIMESLICE * wrr_se->weight;

        requeue_task_wrr(rq, p);
        set_tsk_need_resched(p);
        return;
}
```
cpu에 tick이 발생 할 때 마다 task의 time_slice를 감소 시킨다. time_slice가 0이 되면 task에 할당된 time_slice를 다 사용한 것이므로 time_slice를 처음 값으로 되돌리고 wrr_rq의 맨뒤로 보낸다.  

6.task_fork_wrr  
```c
static void task_fork_wrr(struct task_struct *p)
{
        wrr_se = &p->wrr;

        INIT_LIST_HEAD(&wrr_se->run_list);
        wrr_se->time_slice = wrr_se->weight * WRR_TIMESLICE;
}
```
task의 모든 정보들은 core.c의 sched_fork에서 child에 전달 되지만 child는 새로운 task이므로 time_slice는 parent에서 받지 않고 weight * base time slice 값으로 초기화 해준다.

* 시스템 콜  
1.sched_setweight
```c
SYSCALL_DEFINE2(sched_setweight, pid_t, pid, int, weight)
{
        ...
        if (current_euid() == 0 || current_uid() == 0) {
                task->wrr.weight = weight;
        } else if (current_uid() == task_uid(task)){
                if (task->wrr.weight > weight)
                        task->wrr.weight = weight;
                else
                        return -EACCES;
        } else{
                /* not root && not user who make process */
                return -EACCES;
        }

        rq = task_rq_lock(task, &flags);
        if (!list_empty(&task->wrr.run_list))
                change_load(task_rq(task), old_weight, weight);
        task_rq_unlock(rq, task, &flags);
        return 0;
}
```
current_euid , current uid 가 0이면 root user 이므로 weight를 변경 할 수 있다. 또 current_uid가 task의 uid 와  같고 변경 하려는 weight가 task의 weight보다 낮은 경우에도 weight를 변경 할 수 있다. 그외의 경우에는 에러를 리턴한다.
weight를 바꿨기 때문에 wrr_rq의 load도 바꿔줘야 한다. race condition을 막기 위해서 rq에 lock을 걸고 change_load 함수로 wrr_rq의 load를 바꾼다.  

2.sched_getweight
```c
SYSCALL_DEFINE1(sched_getweight, pid_t, pid)
{
        if (pid == 0)
                task = current;
        else
                task = pid_task(find_get_pid(pid), PIDTYPE_PID);

        /*check valid pid*/
        if (task == NULL)
                return -ESRCH;
        /*check policy*/
        if (task->policy != SCHED_WRR)
                return -EINVAL;

        rq = task_rq_lock(task, &flags);
        weight = task->wrr.weight;
        task_rq_unlock(rq, task, &flags);

        return weight;
}
```
pid가 0이면 current task, 아니면 pid에 해당하는 task의 weight를 얻는다. 이때 policy가 wrr이 아닌 task가 대상이면 에러를 리턴한다. weight를 얻기 전에 race condition을 막기 위해서 rq lock을 걸고 weight를 얻는다.

* load balance 
```c
static struct hrtimer wrr_load_balance_timer;

static enum hrtimer_restart __wrr_load_balance(struct hrtimer *timer)
{
        wrr_load_balance();
        hrtimer_forward_now(&wrr_load_balance_timer,
                            ns_to_ktime(WRR_LB_INTERVAL));
        return HRTIMER_RESTART;
}
void init_wrr_balancer(void)
{
        hrtimer_init(&wrr_load_balance_timer,
                    CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        wrr_load_balance_timer.function = __wrr_load_balance;
        hrtimer_start(&wrr_load_balance_timer,
                ns_to_ktime(WRR_LB_INTERVAL), HRTIMER_MODE_REL);
}
#endif
```
Load balance는 hrtimer를 이용해서 2000ms당 한번만 실행되도록 했다. __wrr_load_balance를 hrtimer가 WRR_LB_INTERVAL(2000ms)마다 실행하고 __wrr_load_balance가 wrr_load_balance를 실행한다.
```c
void wrr_load_balance(void)
{
        ...
        rcu_read_lock();
        for_each_cpu(cpu, cpu_active_mask) {
                i++;
                rq = cpu_rq(cpu);

                if (max_load < rq->wrr.wrr_load) {
                        max_load = rq->wrr.wrr_load;
                        max_cpu = cpu;
                }
                if (min_load > rq->wrr.wrr_load) {
                        min_load = rq->wrr.wrr_load;
                        min_cpu = cpu;
                }
        }
        rcu_read_unlock();

        if (i < 2 || max_load == min_load)
                return;
        load_balance(max_cpu, min_cpu);
}
```
wrr_load_balance는 load까 가장 큰 max cpu 와 가장 작은 min cpu를 찾고 해당 cpu들에 대해서 load_balance를 부른다. cpu를 찾을 때는 race condition을 막기 위해 rcu_read_lock 을 사용했다.
```c
static void load_balance(int max_cpu, int min_cpu)
{
        struct rq *src = cpu_rq(max_cpu);
        struct rq *dest = cpu_rq(min_cpu);
        struct sched_wrr_entity *curr;
        struct task_struct *p, *max_task = NULL;
        unsigned long flags;
        unsigned long max_weight = 0;

        local_irq_save(flags);
        double_rq_lock(src, dest);
        list_for_each_entry(curr, &src->wrr.wrr_queue, run_list) {
                p = task_of(curr);
                if (p->wrr.weight < max_weight)
                        continue;
                if (!can_migrate_task(p, src, dest))
                        continue;
                max_weight = p->wrr.weight;
                max_task = p;
        }
        if (max_task) {
                raw_spin_lock(&max_task->pi_lock);
                deactivate_task(src, max_task, 0);

                set_task_cpu(max_task, dest->cpu);
                activate_task(dest, max_task, 0);
                raw_spin_unlock(&max_task->pi_lock);
        }
        double_rq_unlock(src, dest);
        local_irq_restore(flags);
}
static int
can_migrate_task(struct task_struct *p, struct rq *src, struct rq *dest)
{
        if (!cpumask_test_cpu(dest->cpu, tsk_cpus_allowed(p)))
                return 0;
        if (task_running(src, p))
                return 0;
        if (src->wrr.wrr_load - p->wrr.weight <=
                            dest->wrr.wrr_load + p->wrr.weight)
                return 0;
        return 1;
}
```
load_balance는 wrr_rq에 접근 하기 때문에 double_rq_lock을 사용해서 max, min rq모두에 rq_lock을 걸었다. max rq에서 옮길 수 있는 task를 찾는데 can_migrate_task를 통해서 1.옮길수 있고 2. cpu에서 runnning중이 아니고 3. max 와 min의 load 가 역전 되지 않는 task를 찾는다. 그중에 가장 weight가 큰 task가 존재한다면 deatctivate_task로 max_cpu에서 dequeue하고, set_task_cpu로 min_cpu로 옮기고, activate_task로 min_cpu에서 enqueue한다.  
**2.investigation**  
weight에 따른 task의 수행 시간 변화를 알아 보기 위해서 trial 프로그램을 작성했다.
trial은 반복적으로  trial division 계산을 위해서 소수 set을 5000번째 까지 계산하는데 걸린시간과 자신의 weight를 출력한다.
만들어진 소수 set을 가지고 trial division 계산을 하는 것은 수행 시간에 거의 영향을 주지 않았지만 스펙에 맞게 계산하도록 했다

다른 프로그램을 돌리지 않고 trial만 돌리게 될 경우 weight에 영향을 받지 않았는데 이는 time_slice에 관계없이 혼자만 실행 되었기 때문이다. 그래서 weight의 영향을 알아볼 수 있도록 아래와 같은 while.c 프로그램 4개를 weight 20으로 실행 시키고 같이 trial을 실행했다.이 방법으로 trial은 반드시 weight가 20인 while.c 하나와 같은 큐에서 실행되었다.
```c
while.c 

int main(){
    while(1){}
}
```
 plot.pdf에 나타나는 것처럼 계산시간은 1/weight 에 비례하는 결과를 얻었다.  
trial 계산 시간 = 작업 시간 + 대기 시간 인데 작업시간은 weight에 관계 없이 동일하고 대기시간은 (계산에 필요한 time_slice의 수) * 20 이다. 계산에 필요한 time_slice의 수가 1/weight에 비례하기 때문에  계산 시간 = c1 + c2/weight의 관계식을 갖는다.  
같은 큐에 도는 task의 수가 증가 하면 대기시간의 계산에서 20 이 20  + weight로 증가하게 되어 c2가 증가한다. trial의 소수 set을 더 크게 잡을 경우 c1이 증가한다.

**3.lesson learned**  
1.jiffies 보다 정확하고 간결한 hrtimer framework에 대해 알게 되었다.  
2.C에서 모듈화 하는 방식을 이해했다.  
3.CFS가 정말 잘 구현된 스케줄 방식이라는 것을 알았다.  
4.커널 깨짐등의 현상이 사소한 문제로도 발생할 수 있다는걸 알았다.
