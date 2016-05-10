#include "sched.h"

const struct sched_class wrr_sched_class = {
	.next 			= &cfs_sched_class,
	.enqueue_task 		= enqueue_task_wrr,
	.dequeue_task 		= dequeue_task_wrr,
	.yield_task		= yield_task_wrr,

	.check_preempt_curr 	= check_preempt_curr_wrr,

	.pick_next_task 	= pick_next_task_wrr
	.put_prev_task		= put_prev_task_wrr,

	.set_curr_task		= set_curr_task_wrr,
	.task_tick		= task_tick_wrr,
	.task_fork		= task_fork_wrr,

	.prio_changed		= prio_changed_wrr,
	.switched_from          = switched_from_fair,
	.switched_to            = switched_to_fair,

	.get_rr_interval        = get_rr_interval_fair,
};

static inline struct wrr_rq *wrr_rq_of_se(struct sched_wrr_entity *wrr_se)
{
        return wrr_se->wrr_rq;
}


static void __enqueue_wrr_entity(struct sched_wrr_entity *wrr_se, bool head)
{
	struct wrr_rq *wrr_rq = wrr_rq_of_se(wrr_se);
	if(head)
		list_add(&wrr_se->run_list,&wrr_rq->head);
	else
		list_add_tail(&wrr_se->run_list, &wrr_rq->head); 
}

static void enqueue_wrr_entity(struct sched_wrr_entity *wrr_se, bool head)
{
	__enqueue_wrr_entity(wrr_se, head);
}
static void enqueue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_wrr_entity *wrr_se = &p->wrr_se;
	enqueue_wrr_entity(wrr_se, flags & ENQUEUE_HEAD);
	inc_nr_running(rq);	
	
}

static inline int on_wrr_rq(struct sched_wrr_entity *wrr_se)
{
        return !list_empty(&wrr_se->run_list);
}

static void __dequeue_wrr_entity(struct sched_wrr_entity *wrr_se)
{
	struct wrr_rq *wrr_rq = wrr_rq_of_se(wrr_se);
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
	dequeue_wrr_entity(wrr_se);
	dec_nr_running(rq);
}

requeue_wrr_entity(struct wrr_rq *wrr_rq, struct sched_wrr_entity *wrr_se, int head)
{
	if(on_wrr_rq(wrr_se)){
		struct 
	}
}

static void requeue_task_wrr(struct rq *rq, truct task_struct *p, int head)
{
	struct sched_wrr_entity *wrr_se = &p->wrr_se;
	struct wrr_sq *wrr_rq;
	wrr_rq = wrr_rq_of_se(wrr_se);
	requeue_wrr_entity(wrr_rq, wrr_se, head);
}

static void yield_task_wrr(struct rq *rq)
{
	requeue_task_wrr(rq, rq->curr, 0);
}
static void check_preemt_curr_wrr(struct rq *rq, struct task_struct *p, int flag)
{}
static struct task_struct *pick_next_task_wrr(struct rq *rq)
{}
static void put_prev_task_wrr(struct rq *rq, struct task_struct *p)
{}
static void set_curr_task_wrr(struct rq *rq)
{}



static void enqueue_entity(struct wrr_rq *wrr_rq, struct sched_wrr_entity *wrr, int flags)
{
	
}

static inline struct wrr_rq *wrr_rq_of(struct sched_wrr_entity *wrr)
{
	return wrr->wrr_rq;
}
