#include <stdio.h>
#include "utlist.h"
#include "utils.h"

#include "memory_controller.h"

#define NUM_THREADS 2 // Assuming threads correspond to cores

int thread_priority[NUM_THREADS]; // Priority of each thread
int thread_cluster[NUM_THREADS];  // Cluster: 0 = latency-sensitive, 1 = bandwidth-sensitive
long long int thread_request_count[NUM_THREADS]; // Number of requests per thread
long long int thread_service_rate[NUM_THREADS]; // Service rate per thread

#define LATENCY_SENSITIVE 0
#define BANDWIDTH_SENSITIVE 1

extern long long int CYCLE_VAL;

void init_scheduler_vars()
{
    int i;
    for (i = 0; i < NUM_THREADS; i++) {
        thread_priority[i] = 0; // Default priority
        thread_cluster[i] = LATENCY_SENSITIVE; // Default cluster
        thread_request_count[i] = 0;
        thread_service_rate[i] = 0;
    }
}

// write queue high water mark; begin draining writes if write queue exceeds this value
#define HI_WM 40

// end write queue drain once write queue has this many writes in it
#define LO_WM 20

// 1 means we are in write-drain mode for that channel
int drain_writes[MAX_NUM_CHANNELS];

/* Each cycle it is possible to issue a valid command from the read or write queues
   OR
   a valid precharge command to any bank (issue_precharge_command())
   OR 
   a valid precharge_all bank command to a rank (issue_all_bank_precharge_command())
   OR
   a power_down command (issue_powerdown_command()), programmed either for fast or slow exit mode
   OR
   a refresh command (issue_refresh_command())
   OR
   a power_up command (issue_powerup_command())
   OR
   an activate to a specific row (issue_activate_command()).

   If a COL-RD or COL-WR is picked for issue, the scheduler also has the
   option to issue an auto-precharge in this cycle (issue_autoprecharge()).

   Before issuing a command it is important to check if it is issuable. For the RD/WR queue resident commands, checking the "command_issuable" flag is necessary. To check if the other commands (mentioned above) can be issued, it is important to check one of the following functions: is_precharge_allowed, is_all_bank_precharge_allowed, is_powerdown_fast_allowed, is_powerdown_slow_allowed, is_powerup_allowed, is_refresh_allowed, is_autoprecharge_allowed, is_activate_allowed.
   */

void cluster_threads()
{
    int i;
    for (i = 0; i < NUM_THREADS; i++) {
        if (thread_service_rate[i] < 4/NUM_THREADS) {
            thread_cluster[i] = LATENCY_SENSITIVE;
        } else {
            thread_cluster[i] = BANDWIDTH_SENSITIVE;
        }
    }
}


void schedule(int channel)
{
    request_t * rd_ptr = NULL;
    request_t * wr_ptr = NULL;

    // Cluster threads periodically (e.g., every 1000 cycles)
    if (CYCLE_VAL % 1000 == 0) {
        cluster_threads();
    }
    //periodically reset thread service rates
    if (CYCLE_VAL % 1000 == 0) {
        int i;
        for (i = 0; i < NUM_THREADS; i++) {
            thread_service_rate[i] = 0;
        }
    }


    // Prioritize latency-sensitive threads
    LL_FOREACH(read_queue_head[channel], rd_ptr)
    {
        if (rd_ptr->command_issuable && thread_cluster[rd_ptr->thread_id] == LATENCY_SENSITIVE) {
            issue_request_command(rd_ptr);
            thread_service_rate[rd_ptr->thread_id]++;
            return;
        }
    }

    // Process bandwidth-sensitive threads if no latency-sensitive requests are ready
    LL_FOREACH(read_queue_head[channel], rd_ptr)
    {
        if (rd_ptr->command_issuable && thread_cluster[rd_ptr->thread_id] == BANDWIDTH_SENSITIVE) {
            issue_request_command(rd_ptr);
            thread_service_rate[rd_ptr->thread_id]++;
            return;
        }
    }

    // Handle write requests (similar logic can be applied for writes)
    LL_FOREACH(write_queue_head[channel], wr_ptr)
    {
        if (wr_ptr->command_issuable) {
            issue_request_command(wr_ptr);
            thread_service_rate[wr_ptr->thread_id]++;
            return;
        }
    }
}

void scheduler_stats()
{
    int i;
    printf("Thread Statistics:\n");
    for (i = 0; i < NUM_THREADS; i++) {
        printf("Thread %d: Cluster = %s, Priority = %d, Requests = %lld, Service Rate = %lld\n",
               i,
               thread_cluster[i] == LATENCY_SENSITIVE ? "Latency-Sensitive" : "Bandwidth-Sensitive",
               thread_priority[i],
               thread_request_count[i],
               thread_service_rate[i]);
    }
}
