static void
enqueue_task_wrr(struct rq *rq, struct task_struct *p, int flags){

}

static void
dequeue_task_wrr(struct rq *rq, struct task_struct *p, int flags){

}

static void yield_task_wrr(struct rq *rq){

}

static void
check_preempt_curr_wrr(struct rq *rq, struct task_struct *p){

}

static struct task_struct*
pick_next_task_wrr(struct rq *rq){

}

static void
put_prev_task_wrr(struct rq *rq, struct task_struct *p){

}

static unsigned long
load_balance_wrr(struct rq *this_rq, int this_cpu,
	struct rq *busiest,
	unsigned long max_nr_move, unsigned long max_load_move,
	struct sched_domain *sd, enum cpu_idle_type idle,
	int *all_pinned, int *this_best_prio){

}

const struct sched_class wrr_sched_class = {
	.next			= &fair_sched_class, //?? is this mean a priority??
	.enqueue_task		= enqueue_task_wrr,
	.dequeue_task		= dequeue_task_wrr,
	.yield_task		= yield_task_wrr,

	.check_preempt_curr	= check_preempt_curr_wrr,

	.pick_next_task		= pick_next_task_wrr,
	.put_prev_task		= put_prev_task_wrr,

#ifdef CONFIG_SMP
	.select_task_rq		= select_task_rq_wrr,

	.set_cpus_allowed       = set_cpus_allowed_wrr,
	.rq_online              = rq_online_wrr,
	.rq_offline             = rq_offline_wrr,
	.pre_schedule		= pre_schedule_wrr,
	.post_schedule		= post_schedule_wrr,
	.task_woken		= task_woken_wrr,
	.switched_from		= switched_from_wrr,
#endif

	.set_curr_task          = set_curr_task_wrr,
	.task_tick		= task_tick_wrr,

	.get_rr_interval	= get_rr_interval_wrr,

	.prio_changed		= prio_changed_wrr,
	.switched_to		= switched_to_wrr,
};
