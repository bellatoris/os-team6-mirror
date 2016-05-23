#ifndef _SCHED_WRR_H
#define _SCHED_WRR_H

/*
 * Default timeslice is 10 msecs (used only for SCHED_WRR tasks).
 * Timeslices get refilled after they expire.
 */
#define WRR_TIMESLICE		(10 * HZ / 1000)

/*
 * Weights of tasks can range between 1 and 20.
 * And the default weight is 10
 */
#define DEFAULT_WRR_WEIGHT	10
#define MAX_WRR_WEIGHT		20
#define MIN_WRR_WEIGHT		1

#endif /* _SCHED_WRR_H */
