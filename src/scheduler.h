#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

void init_scheduler_vars(); //called from main
void scheduler_stats(); //called from main
void schedule(int); // scheduler function called every cycle
void cluster_threads(); // clusters threads based on service rate

#endif //__SCHEDULER_H__

