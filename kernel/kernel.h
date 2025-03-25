/**
 * NexOS Kernel - Core Definitions
 * 
 * This file contains the fundamental structures and interfaces for the
 * NexOS microkernel. The design prioritizes simplicity, performance,
 * and the ability to self-evolve.
 */

#ifndef NEXOS_KERNEL_H
#define NEXOS_KERNEL_H

#include <stdint.h>
#include <stdbool.h>

/* Version information */
#define NEXOS_VERSION_MAJOR 0
#define NEXOS_VERSION_MINOR 1
#define NEXOS_VERSION_PATCH 0

/* System constants */
#define MAX_PROCESSES 1024
#define MAX_THREADS_PER_PROCESS 64
#define MAX_PRIORITY_LEVELS 32
#define KERNEL_STACK_SIZE 16384
#define PAGE_SIZE 4096

/* Error codes */
typedef enum {
    ERROR_NONE = 0,
    ERROR_MEMORY_ALLOCATION,
    ERROR_INVALID_PARAMETER,
    ERROR_RESOURCE_BUSY,
    ERROR_NOT_IMPLEMENTED,
    ERROR_PERMISSION_DENIED,
    ERROR_TIMEOUT,
    ERROR_UNKNOWN
} error_code_t;

/* Process states */
typedef enum {
    PROCESS_CREATED,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} process_state_t;

/* Thread states */
typedef enum {
    THREAD_CREATED,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_TERMINATED
} thread_state_t;

/* Process descriptor */
typedef struct process {
    uint32_t pid;                      /* Process ID */
    uint32_t parent_pid;               /* Parent process ID */
    process_state_t state;             /* Current process state */
    uint8_t priority;                  /* Process priority (0-31, 0 highest) */
    void* memory_space;                /* Process memory space */
    struct thread* threads;            /* Threads belonging to this process */
    uint32_t thread_count;             /* Number of threads */
    void* resources;                   /* Resources owned by the process */
    uint64_t cpu_time;                 /* CPU time used */
    uint64_t creation_time;            /* Time when process was created */
    uint64_t last_scheduled;           /* Last time process was scheduled */
    void* ai_profile;                  /* AI optimization profile */
} process_t;

/* Thread descriptor */
typedef struct thread {
    uint32_t tid;                      /* Thread ID */
    uint32_t pid;                      /* Owner process ID */
    thread_state_t state;              /* Current thread state */
    uint8_t priority;                  /* Thread priority (0-31, 0 highest) */
    void* stack;                       /* Thread stack */
    uint32_t stack_size;               /* Stack size */
    void* context;                     /* CPU context for this thread */
    void (*entry_point)(void*);        /* Thread entry point */
    void* arg;                         /* Argument to entry point */
    uint64_t cpu_time;                 /* CPU time used */
    uint64_t last_scheduled;           /* Last time thread was scheduled */
    void* ai_profile;                  /* AI optimization profile */
} thread_t;

/* Self-evolution metadata */
typedef struct {
    uint64_t last_analysis_time;       /* Last time code was analyzed */
    uint32_t optimization_count;       /* Number of self-optimizations */
    uint32_t patch_count;              /* Number of self-patches applied */
    uint8_t evolution_level;           /* Current evolution level */
    bool evolution_enabled;            /* Whether evolution is enabled */
    void* performance_metrics;         /* Performance data collection */
    void* optimization_history;        /* History of applied optimizations */
} self_evolution_t;

/* Kernel initialization */
error_code_t kernel_init(void);

/* Process management */
error_code_t process_create(process_t** process, void (*entry)(void*), void* arg, uint8_t priority);
error_code_t process_terminate(uint32_t pid);
error_code_t process_set_priority(uint32_t pid, uint8_t priority);
process_t* process_get_current(void);

/* Thread management */
error_code_t thread_create(thread_t** thread, process_t* process, void (*entry)(void*), void* arg, uint8_t priority);
error_code_t thread_terminate(uint32_t tid);
error_code_t thread_set_priority(uint32_t tid, uint8_t priority);
thread_t* thread_get_current(void);

/* Scheduler interface */
error_code_t scheduler_init(void);
error_code_t scheduler_start(void);
error_code_t scheduler_yield(void);
error_code_t scheduler_sleep(uint64_t milliseconds);

/* Self-evolution interface */
error_code_t self_evolution_init(void);
error_code_t self_evolution_enable(bool enable);
error_code_t self_evolution_analyze(void);
error_code_t self_evolution_apply_patch(void* patch, uint32_t size);
self_evolution_t* self_evolution_get_status(void);

#endif /* NEXOS_KERNEL_H */