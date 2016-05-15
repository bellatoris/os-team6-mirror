#include "sched.h"

void init_wrr_rq(struct wrr_rq *wrr_rq)
{
	wrr_rq->wrr_nr_running = 0;
	INIT_LIST_HEAD(&wrr_rq->head);
	
#ifdef CONFIG_SMP
	wrr_rq->wrr_nr_running = 0;
	//plist_head_init
#endif
}

/*
__init void init_sched_wrr_class(void)
{
#ifdef CONFIG_SMP
        open_softirq(SCHED_SOFTIRQ, run_rebalance_domains);

#ifdef CONFIG_NO_HZ_COMMON
        nohz.next_balance = jiffies;
        zalloc_cpumask_var(&nohz.idle_cpus_mask, GFP_NOWAIT);
        cpu_notifier(sched_ilb_notifier, 0);
#endif

#ifdef CONFIG_SCHED_HMP
        hmp_cpu_mask_setup();
#endif
#endif 

}
*/
static inline void inc_wrr_running(struct wrr_rq *wrr_rq)
{
	wrr_rq->wrr_nr_running++;
}
static inline void dec_wrr_running(struct wrr_rq *wrr_rq)
{
	wrr_rq->wrr_nr_running--;
}
 

static inline struct wrr_rq *wrr_rq_of_se(struct sched_wrr_entity *wrr_se)
{
        return wrr_se->wrr_rq;
}


static void __enqueue_wrr_entity(struct sched_wrr_entity *wrr_se)
{
	struct wrr_rq *wrr_rq = wrr_rq_of_se(wrr_se);
	list_add_tail(&wrr_se->run_list, &wrr_rq->head); 
}

static void enqueue_wrr_entity(struct sched_wrr_entity *wrr_se)
{
	__enqueue_wrr_entity(wrr_se);
}

#ifdef CONFIG_SMP

static void __enqueue_plist_task(struct wrr_rq *wrr_rq,
				struct task_struct *p)
{
	plist_del(&p->pushable_tasks, &wrr_rq->movable_tasks);
	plist_node_init(&p->pushable_tasks, p->wrr_weight);
	plist_add(&p->pushable_tasks, &wrr_rq->movable_tasks);
}

static void __dequeue_plist_task(struct wrr_rq *wrr_rq,
				struct task_struct *p)
{
	plist_del(&p->pushable_tasks, &wrr_rq->movable_tasks);
}

#else

static void __enqueue_plist_task(struct wrr_rq *wrr_rq, struct task_struct *p)
{
}

static void __dequeue_plist_task(struct wrr_rq *wrr_rq, struct task_struct *p)
{
}

#endif

static void enqueue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_wrr_entity *wrr_se = &p->wrr_se;
	struct wrr_rq *wrr_rq = wrr_rq_of_se(wrr_se);
	enqueue_wrr_entity(wrr_se);
	inc_wrr_running(wrr_rq);
	if(!task_current(rq, p))
		__enqueue_plist_task(wrr_rq, p);
}

static inline int on_wrr_rq(struct sched_wrr_entity *wrr_se)
{
        return !list_empty(&wrr_se->run_list);
}

static void __dequeue_wrr_entity(struct sched_wrr_entity *wrr_se)
{
	list_del_init(&wrr_se->run_list);
}

static void dequeue_wrr_entity(struct sched_wrr_entity *wrr_se)
{
	if(on_wrr_rq(wrr_se))
		__dequeue_wrr_entity(wrr_se);
}
static void dequeue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_wrr_entity *wrr_se = &p->wrr_se;
	struct wrr_rq *wrr_rq = wrr_rq_of_se(wrr_se);
	update_curr_wrr(rq);	
	
	if(on_wrr_rq(wrr_se)){
		dequeue_wrr_entity(wrr_se);
		dec_wrr_running(wrr_rq);
		__dequeue_plist_task(wrr_rq, p);
	}
}

static void requeue_wrr_entity(struct wrr_rq *wrr_rq, struct sched_wrr_entity *wrr_se)
{
	list_move_tail(&wrr_se->run_list, &wrr_rq->head); 
	
}

static void requeue_task_wrr(struct rq *rq, struct task_struct *p)
{
	struct sched_wrr_entity *wrr_se = &p->wrr_se;
	struct wrr_sq *wrr_rq = wrr_rq_of_se(wrr_se);
	requeue_wrr_entity(wrr_rq, wrr_se);
	__enqueue_plist_task(&rq->wrr , p);
}

static void yield_task_wrr(struct rq *rq)
{
	requeue_task_wrr(rq, rq->curr);
}
static void check_preemt_curr_wrr(struct rq *rq, struct task_struct *p, int flag)
{}

#define wrr_entity_is_task(wrr_se) (1)

static inline struct task_struct *wrr_task_of(struct sched_rt_entity *wrr_se)
{
#ifdef CONFIG_SCHED_DEBUG
        WARN_ON_ONCE(!wrr_entity_is_task(wrr_se));
#endif
        return container_of(wrr_se, struct task_struct, wrr_se);
}


static struct sched_wrr_entity *pick_next_wrr_entity(struct wrr_rq *wrr_rq)
{
	struct sched_wrr_entity *next = NULL;
	struct list_head queue;
	queue = wrr_rq->head;
	next = list_entry(queue.next, struct sched_wrr_entity, run_list);
}

static struct task_struct *__pick_next_task_wrr(struct rq *rq)
{
	struct sched_wrr_entity *wrr_se;
	struct task_struct *p;
	struct wrr_rq *wrr_rq;
	
	wrr_rq = &rq->wrr;
	
	if(!wrr_rq->wrr_nr_running)
		return NULL;
	wrr_se = pick_next_wrr_entity(wrr_rq);
	//BUG_ON(!wrr_se);
	p = wrr_task_of(wrr_se);
	return p;
}

static struct task_struct *pick_next_task_wrr(struct rq *rq)
{
	struct task_struct *p = __pick_next_task_wrr(rq);
	if(p)
		__dequeue_plist(&(rq->wrr), p);
	return p;
	
}
static void put_prev_task_wrr(struct rq *rq, struct task_struct *p)
{
	update_curr_wrr(rq);

	if(on_wrr_rq(&p->wrr_se) && p->nr_cpus_allowed >1)
		enqueue_pushable_task(rq,p);
}
static void enqueue_pushable_task(struct rq *rq, struct task_struct *p)
{
}

static void set_curr_task_wrr(struct rq *rq)
{
	struct task_struct *p = rq->curr;
	//p->se.exec_start = rq->clock_task;
	__dequeue_plist_task(&(rq->wrr), p);
}

static unsigned int get_wrr_timeslice(int weight)
{
	return BASE_TIMESLICE * weight;
}

static unsigned int get_rr_interval_wrr(struct rq *rq, struct task_struct *task)
{
	return task->wrr_se.time_slice;
}


static void task_tick_wrr(struct rq *rq, struct task_struct *p, int queued)
{
	update_curr_wrr(rq);
	
	if (--p->wrr_se.time_slice > 0)
		return;
	p->wrr_se.time_slice = get_wrr_timeslice(p->wrr_weight);

	if (p->wrr_se.run_list.prev != p->wrr_se.run_list.next) {
		requeue_task_wrr(rq, p);
		set_tsk_need_resched(p);
	}
}
static void task_fork_wrr(struct task_struct *p)
{
	struct sched_wrr_entity *wrr_se;
	struct rq *rq;
	
	rq = this_rq();

	wrr_se = &p->wrr_se;
	wrr_se->time_slice = get_wrr_timeslice(p->wrr_weight);

}


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


	/* Update the entity's runtime */
	curr->se.sum_exec_runtime += delta_exec;

	account_group_exec_runtime(curr, delta_exec);

	/* Reset the start time */
	curr->se.exec_start = rq->clock_task;

	cpuacct_charge(curr, delta_exec);

}

const struct sched_class wrr_sched_class = {
        .next                   = &cfs_sched_class,
        .enqueue_task           = enqueue_task_wrr,
        .dequeue_task           = dequeue_task_wrr,
        .yield_task             = yield_task_wrr,

        .check_preempt_curr     = check_preempt_curr_wrr,

        .pick_next_task         = pick_next_task_wrr
        .put_prev_task          = put_prev_task_wrr,

        .set_curr_task          = set_curr_task_wrr,
        .task_tick              = task_tick_wrr,

        .task_fork              = task_fork_wrr,
/*
        .prio_changed           = prio_changed_wrr,
        .switched_from          = switched_from_wrr,
        .switched_to            = switched_to_wrr,
*/
        .get_rr_interval        = get_rr_interval_wrr,

};

