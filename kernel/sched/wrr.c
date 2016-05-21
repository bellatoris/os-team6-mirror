
/*
 * Weighted Round Robin Scheduling (WRR) Class (SCHED_WRR)
 *
 * Copyright 2016 Doogie Min <bellatoris@snu.ac.kr>
 *
 */


#include <linux/sched.h>
#include <linux/cpumask.h>


#include "sched.h"

/*
 * The bawrr time slice (quantum) is 10ms.
 * Weights of tasks can range between 1 and 20
 */

void init_wrr_rq(struct wrr_rq *wrr_rq)
{
	wrr_rq->wrr_nr_running = 0;
	INIT_LIST_HEAD(&wrr_rq->wrr_queue);
	wrr_rq->wrr_load = 0;
}

/*
 * wrr을 가진 task_struct를 return한다.
 */
static inline struct task_struct *task_of(struct sched_wrr_entity *wrr)
{
	return container_of(wrr, struct task_struct, wrr);
}

/*
 * wrr_rq를 가진 rq를 return한다.
 */
static inline struct rq *rq_of(struct wrr_rq *wrr_rq)

{
	return container_of(wrr_rq, struct rq, wrr);
}

/*
 * p의 rq에 접근해서 rq의 wrr_rq를 retrun 한다.
 */
static inline struct wrr_rq *task_wrr_rq(struct task_struct *p)
{
	return &task_rq(p)->wrr;
}

/*
 * wrr을 가진 task_struct p를 구하고, p의 rq에 접근해 rq의
 * wrr_rq를 return한다.
 */
static inline struct wrr_rq *wrr_rq_of(struct sched_wrr_entity *wrr)
{
	struct task_struct *p = task_of(wrr);
	struct rq *rq = task_rq(p);

	return &rq->wrr;
}

/*
 * Update the current task's runtime statistics. Skip current tasks that
 * are not in our scheduling class.
 */
static void update_curr_wrr(struct rq *rq)
{
	struct task_struct *curr = rq->curr;
	u64 delta_exec;

	if (curr->sched_class != &wrr_sched_class)
		return;

	delta_exec = rq->clock_task - curr->se.exec_start;
	if (unlikely((s64)delta_exec < 0))
		delta_exec = 0;

	schedstat_set(curr->se.statistics.exec_max,
		max(curr->se.statistics.exec_max, delta_exec));

	curr->se.sum_exec_runtime += delta_exec;
	account_group_exec_runtime(curr, delta_exec);

	curr->se.exec_start = rq->clock_task;
	cpuacct_charge(curr, delta_exec);
}

/*
 * wrr_se를 wrr_rq에 추가하고, wrr_rq의 load를 wrr_se의 weight만큼
 * 증가시킨다. 이 함수는 process가 sleeping state에서 runnable state로
 * 바뀔때 불린다.
 */
static void __enqueue_wrr_entity(struct wrr_rq *wrr_rq,
				struct sched_wrr_entity *wrr_se)
{
	list_add_tail(&wrr_se->run_list, &wrr_rq->wrr_queue);
	wrr_rq->wrr_load += wrr_se->weight;
}

static void enqueue_wrr_entity(struct wrr_rq *wrr_rq,
			struct sched_wrr_entity *wrr_se, int flags)
{
	update_curr_wrr(rq_of(wrr_rq));
	__enqueue_wrr_entity(wrr_rq, wrr_se);
}

/*
 * wrr_se를 wrr_rq에서 제거하고, wrr_rq의 load를 wrr_se의 weight만큼
 * 감소시킨다. 이 함수는 process가 runnable state에서 sleeping state
 * 로 바뀔때 불리거나, kernel이 다른 이유로 run_queue에서 이 process를
 * 빼고자 할때 불린다.
 */
static void __dequeue_wrr_entity(struct wrr_rq *wrr_rq,
				struct sched_wrr_entity *wrr_se)
{
	list_del_init(&wrr_se->run_list);
	wrr_rq->wrr_load -= wrr_se->weight;
}

static void dequeue_wrr_entity(struct wrr_rq *wrr_rq,
			struct sched_wrr_entity *wrr_se, int flags)
{
	update_curr_wrr(rq_of(wrr_rq));
	__dequeue_wrr_entity(wrr_rq, wrr_se);
}

/*
 * The enqueue_task_wrr method is called before nr_running is
 * increased. Here we update the wrr scheduing stats and then
 * put the task int to the queue
 */
static void enqueue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct wrr_rq *wrr_rq;
	struct sched_wrr_entity *wrr_se = &p->wrr;

	/* wrr_rq is task_struct's wrr_rq */
	wrr_rq = wrr_rq_of(wrr_se);
	/* enqueue wrr_entity to wrr_rq */
	enqueue_wrr_entity(wrr_rq, wrr_se, flags);
	wrr_rq->wrr_nr_running++;

	inc_nr_running(rq);
}

static void dequeue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct wrr_rq *wrr_rq;
	struct sched_wrr_entity *wrr_se = &p->wrr;

	wrr_rq = wrr_rq_of(wrr_se);
	dequeue_wrr_entity(wrr_rq, wrr_se, flags);
	wrr_rq->wrr_nr_running--;

	dec_nr_running(rq);
}

/*
 * Put task to the end of the run list without the overhead of
 * dequeue followed by enqueue
 */
static void
requeue_wrr_entity(struct wrr_rq *wrr_rq, struct sched_wrr_entity *wrr_se)
{
	list_move_tail(&wrr_se->run_list, &wrr_rq->wrr_queue);
}

static void requeue_task_wrr(struct rq *rq, struct task_struct *p)
{
	struct sched_wrr_entity *wrr_se = &p->wrr;
	struct wrr_rq *wrr_rq;

	wrr_rq = wrr_rq_of(wrr_se);
	requeue_wrr_entity(wrr_rq, wrr_se);
}

static void yield_task_wrr(struct rq *rq)
{
	requeue_task_wrr(rq, rq->curr);
}

static void check_preempt_curr_wrr(struct rq *rq,
			struct task_struct *p, int wakeflags)
{
}

static struct sched_wrr_entity *pick_next_wrr_entity(struct rq *rq,
						struct wrr_rq *wrr_rq)
{
	struct sched_wrr_entity *next = NULL;

	next = list_first_entry(&wrr_rq->wrr_queue,
			    struct sched_wrr_entity, run_list);

	return next;
}

static struct task_struct *_pick_next_task_wrr(struct rq *rq)
{
	struct sched_wrr_entity *wrr_se;
	struct task_struct *p;
	struct wrr_rq *wrr_rq;

	wrr_rq = &rq->wrr;

	if (!wrr_rq->wrr_nr_running)
		return NULL;

	wrr_se = pick_next_wrr_entity(rq, wrr_rq);

	p = task_of(wrr_se);
	p->se.exec_start = rq->clock_task;

	return p;
}


static struct task_struct *pick_next_task_wrr(struct rq *rq)
{
	struct task_struct *p = _pick_next_task_wrr(rq);
	return p;
}

static void put_prev_task_wrr(struct rq *rq, struct task_struct *p)
{
	update_curr_wrr(rq);
	p->se.exec_start = 0;
}

#ifdef CONFIG_SMP
static int find_lowest_cpu(struct task_struct *p)
{
	struct rq *rq;
	unsigned int min_cpu, load, cpu;

	load = 0xffffffff;
	for_each_cpu(cpu, cpu_active_mask) {
		rq = cpu_rq(cpu);

		if (rq->wrr.wrr_load < load) {
			load = rq->wrr.wrr_load;
			min_cpu = cpu;
		}
	}

	return min_cpu;
}

static int
select_task_rq_wrr(struct task_struct *p, int sd_flag, int flags)
{
	int cpu = task_cpu(p);

	if (p->nr_cpus_allowed == 1)
		return cpu;

	rcu_read_lock();
	cpu = find_lowest_cpu(p);
	rcu_read_unlock();

	return cpu;
}
#endif /* CONFIG_SMP */

static void set_curr_task_wrr(struct rq *rq)
{
	struct task_struct *p = rq->curr;
	p->se.exec_start = rq->clock_task;
}

/*
 * task_tick_wrris called by the periodic scheduler each time it is activated.
 * Each task should execute in intervals equal to the quantum value.
 * To accomplish this, this function should decrement the current
 * task’s time slice to indicate that it has run for 1 time unit.
 * When a task has run for its entire allotted time slice
 * (i.e. the task’s time slice reaches 0), this function should
 * reset its time slice value, move it to the end of the run queue,
 * and set its TIF_NEED_RESCHED flag (using the set tsk need
 * resched(task) function) to indicate to the generic scheduler
 * that it should be preempted.
 */
static void task_tick_wrr(struct rq *rq, struct task_struct *p, int queued)
{
	struct sched_wrr_entity *wrr_se = &p->wrr;

	update_curr_wrr(rq);

	if (--wrr_se->time_slice)
		return;

	wrr_se->time_slice = WRR_TIMESLICE * wrr_se->weight;

	requeue_task_wrr(rq, p);
	set_tsk_need_resched(p);
	return;
}

static void switched_to_wrr(struct rq *rq, struct task_struct *p)
{
	/*
	 * We were most likely switched from sched_rt, so
	 * kick off the schedule if running, otherwise just see
	 * if we can still preempt the current task
	 */
	if (rq->curr == p)
		resched_task(rq->curr);
	else
		check_preempt_curr(rq, p, 0);
}

const struct sched_class wrr_sched_class = {
	.next			= &idle_sched_class,
	.enqueue_task		= enqueue_task_wrr,
	.dequeue_task		= dequeue_task_wrr,
	.yield_task		= yield_task_wrr,

	.check_preempt_curr	= check_preempt_curr_wrr,
	.pick_next_task		= pick_next_task_wrr,
	.put_prev_task		= put_prev_task_wrr,

#ifdef CONFIG_SMP
	.select_task_rq		= select_task_rq_wrr,
#endif


	.set_curr_task		= set_curr_task_wrr,
	.task_tick		= task_tick_wrr,



	.switched_to		= switched_to_wrr,
};

/*
__init void init_sched_wrr_class(void)
{
#ifdef CONFIG_SMP
	open_softirq(SCHED_SOFRIRQ, load_balance);
#endif
}
*/
#ifdef CONFIG_SMP
static int
can_migrate_task(struct task_struct *p, struct rq *src, struct rq *dest)
{
	/*
	 * We do not migrate tasks that are:
	 * 1) cannot be migrated to this CPU due to cpus_allowed, or
	 * 2) running (obviously).
	 * 3) 옮기고 나서 dest의 load가 src의 load보다 커질 경우
	 */
	if (!cpumask_test_cpu(dest->cpu, tsk_cpus_allowed(p)))
		return 0;
	if (!task_running(src, p))
		return 0;
	if (src->wrr.wrr_load - p->wrr.weight <=
			    dest->wrr.wrr_load + p->wrr.weight)
		return 0;
	return 1;
}

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
		deactivate_task(src, max_task, 0);
		set_task_cpu(max_task, dest->cpu);
		activate_task(dest, max_task, 0);
	}
	double_rq_unlock(src, dest);
	local_irq_restore(flags);
}

void wrr_load_balance(void)
{
//	if (time_after_eq(jiffies, rq->wrr->next_balance))
//		raise_softirq(SCHED_SOFTIRQ);
	int cpu, max_cpu, min_cpu;
	int i = 0;
	unsigned long max_load = 0;
	unsigned long min_load = 0xffffffff;

	struct rq *rq;

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
#endif

#ifdef CONFIG_SCHED_DEBUG
extern void print_wrr_rq(struct seq_file *m, int cpu, struct wrr_rq *wrr_rq);


void print_wrr_stats(struct seq_file *m, int cpu)
{
	struct wrr_rq *wrr_rq;
	wrr_rq = &cpu_rq(cpu)->wrr;
	rcu_read_lock();
	print_wrr_rq(m, cpu, wrr_rq);
	rcu_read_unlock();
}
#endif /* CONFIG_SCHED_DEBUG */
