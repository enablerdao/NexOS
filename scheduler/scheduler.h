/**
 * NexOS Scheduler Subsystem
 * 
 * This file defines the scheduler interface for NexOS.
 * The scheduler is responsible for process and thread scheduling,
 * context switching, and CPU time management.
 */

#ifndef NEXOS_SCHEDULER_H
#define NEXOS_SCHEDULER_H

#include "../kernel/kernel.h"
#include <stdint.h>
#include <stdbool.h>

/* Scheduling policies */
typedef enum {
    SCHED_POLICY_FIFO,         /* First-in, first-out */
    SCHED_POLICY_ROUND_ROBIN,  /* Round-robin */
    SCHED_POLICY_PRIORITY,     /* Priority-based */
    SCHED_POLICY_ADAPTIVE,     /* AI-adaptive scheduling */
    SCHED_POLICY_REALTIME      /* Real-time scheduling */
} scheduling_policy_t;

/* CPU context */
typedef struct {
    uint64_t registers[16];    /* General purpose registers */
    uint64_t program_counter;  /* Program counter */
    uint64_t stack_pointer;    /* Stack pointer */
    uint64_t status_register;  /* Status register */
    /* Architecture-specific context data */
} cpu_context_t;

/* Run queue */
typedef struct {
    uint32_t thread_count;     /* Number of threads in queue */
    thread_t** threads;        /* Array of thread pointers */
} run_queue_t;

/* Scheduler state */
typedef struct {
    scheduling_policy_t policy;        /* Current scheduling policy */
    uint32_t time_slice;               /* Time slice in milliseconds */
    uint32_t context_switches;         /* Number of context switches */
    uint64_t last_context_switch;      /* Time of last context switch */
    run_queue_t run_queues[MAX_PRIORITY_LEVELS]; /* Run queues for each priority level */
    thread_t* idle_thread;             /* Idle thread */
    bool preemption_enabled;           /* Whether preemption is enabled */
} scheduler_state_t;

/* Performance metrics for scheduler */
typedef struct {
    uint64_t total_cpu_time;           /* Total CPU time in milliseconds */
    uint64_t idle_cpu_time;            /* Idle CPU time in milliseconds */
    uint32_t context_switch_count;     /* Number of context switches */
    uint32_t preemption_count;         /* Number of preemptions */
    float cpu_utilization;             /* CPU utilization (0-1) */
    float average_wait_time;           /* Average wait time in milliseconds */
    float average_turnaround_time;     /* Average turnaround time in milliseconds */
    float average_response_time;       /* Average response time in milliseconds */
    uint32_t priority_inversions;      /* Number of priority inversions */
} scheduler_metrics_t;

/* Optimization history for scheduler */
typedef struct {
    uint32_t policy_changes;           /* Number of policy changes */
    uint32_t time_slice_adjustments;   /* Number of time slice adjustments */
    uint32_t priority_adjustments;     /* Number of priority adjustments */
    uint32_t load_balancing_events;    /* Number of load balancing events */
} scheduler_optimization_t;

/* Initialize scheduler */
error_code_t scheduler_init(void);

/* Start scheduler */
error_code_t scheduler_start(void);

/* Add process to scheduler */
error_code_t scheduler_add_process(process_t* process);

/* Remove process from scheduler */
error_code_t scheduler_remove_process(process_t* process);

/* Find process by ID */
process_t* scheduler_find_process(uint32_t pid);

/* Add thread to scheduler */
error_code_t scheduler_add_thread(thread_t* thread);

/* Remove thread from scheduler */
error_code_t scheduler_remove_thread(thread_t* thread);

/* Find thread by ID */
thread_t* scheduler_find_thread(uint32_t tid);

/* Initialize thread context */
error_code_t scheduler_init_thread_context(thread_t* thread);

/* Perform context switch */
error_code_t scheduler_context_switch(void);

/* Yield CPU */
error_code_t scheduler_yield(void);

/* Block current thread */
error_code_t scheduler_block_current(void);

/* Unblock thread */
error_code_t scheduler_unblock(thread_t* thread);

/* Sleep current thread */
error_code_t scheduler_sleep(uint64_t milliseconds);

/* Set scheduling policy */
error_code_t scheduler_set_policy(scheduling_policy_t policy);

/* Set time slice */
error_code_t scheduler_set_time_slice(uint32_t milliseconds);

/* Enable/disable preemption */
error_code_t scheduler_set_preemption(bool enable);

/* Get scheduler metrics */
error_code_t scheduler_get_metrics(scheduler_metrics_t* metrics);

/* Self-optimization interface */
error_code_t scheduler_optimize(void);
error_code_t scheduler_analyze_workload(void);
error_code_t scheduler_adjust_priorities(void);
error_code_t scheduler_apply_optimization(void* optimization_data);

#endif /* NEXOS_SCHEDULER_H */