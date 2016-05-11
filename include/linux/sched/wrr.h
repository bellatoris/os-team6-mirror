#ifndef _SCHED_WRR_H
#define _SCHED_WRR_H

#define BASE_TIMESLICE (10 * HZ / 1000)
#define DEFAULT_WEIGHT 10

static inline int wrr_task(struct task_struct *p)
{
        return (p->policy == SCHED_WRR);
}

#endif
