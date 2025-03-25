/**
 * NexOS AI Engine Subsystem
 * 
 * This file defines the AI engine interface for NexOS.
 * The AI engine is responsible for monitoring system performance,
 * analyzing usage patterns, and generating optimizations.
 */

#ifndef NEXOS_AI_ENGINE_H
#define NEXOS_AI_ENGINE_H

#include "../kernel/kernel.h"
#include "../memory/memory.h"
#include "../scheduler/scheduler.h"
#include <stdint.h>
#include <stdbool.h>

/* AI model types */
typedef enum {
    AI_MODEL_PERFORMANCE,      /* Performance optimization model */
    AI_MODEL_MEMORY,           /* Memory usage optimization model */
    AI_MODEL_SCHEDULER,        /* Scheduler optimization model */
    AI_MODEL_POWER,            /* Power usage optimization model */
    AI_MODEL_SECURITY,         /* Security optimization model */
    AI_MODEL_CODE              /* Code optimization model */
} ai_model_type_t;

/* AI model descriptor */
typedef struct {
    ai_model_type_t type;      /* Model type */
    void* model_data;          /* Model data */
    uint32_t model_size;       /* Model size in bytes */
    uint32_t version;          /* Model version */
    uint64_t last_updated;     /* Last update timestamp */
    uint64_t inference_count;  /* Number of inferences performed */
    float accuracy;            /* Model accuracy (0-1) */
} ai_model_t;

/* Performance metrics */
typedef struct {
    memory_metrics_t memory;           /* Memory metrics */
    scheduler_metrics_t scheduler;     /* Scheduler metrics */
    uint64_t io_operations;            /* Number of I/O operations */
    uint64_t network_bytes;            /* Network bytes transferred */
    float power_usage;                 /* Power usage in watts */
    uint32_t error_count;              /* Number of errors */
    uint64_t uptime;                   /* System uptime in milliseconds */
} performance_metrics_t;

/* Optimization suggestion */
typedef struct {
    uint32_t id;                       /* Suggestion ID */
    char description[256];             /* Description of the suggestion */
    float expected_improvement;        /* Expected improvement (0-1) */
    uint32_t confidence;               /* Confidence level (0-100) */
    void* optimization_data;           /* Optimization-specific data */
    uint32_t data_size;                /* Size of optimization data */
} optimization_suggestion_t;

/* Optimization history */
typedef struct {
    uint32_t entry_count;              /* Number of history entries */
    struct {
        uint64_t timestamp;            /* Timestamp of optimization */
        uint32_t suggestion_id;        /* ID of applied suggestion */
        float actual_improvement;      /* Actual improvement (0-1) */
        bool reverted;                 /* Whether optimization was reverted */
    } entries[100];                    /* Circular buffer of history entries */
} optimization_history_t;

/* Process AI profile */
typedef struct {
    uint32_t pid;                      /* Process ID */
    uint64_t creation_time;            /* Creation timestamp */
    uint64_t cpu_time;                 /* Total CPU time used */
    uint64_t memory_usage;             /* Average memory usage */
    uint32_t priority_changes;         /* Number of priority changes */
    uint32_t io_operations;            /* Number of I/O operations */
    float cpu_intensity;               /* CPU intensity (0-1) */
    float memory_intensity;            /* Memory intensity (0-1) */
    float io_intensity;                /* I/O intensity (0-1) */
    uint8_t optimal_priority;          /* AI-determined optimal priority */
    uint32_t optimal_time_slice;       /* AI-determined optimal time slice */
} process_ai_profile_t;

/* Initialize AI engine */
error_code_t ai_engine_init(void);

/* Load AI model */
error_code_t ai_engine_load_model(ai_model_type_t type, void* model_data, uint32_t model_size);

/* Update AI model */
error_code_t ai_engine_update_model(ai_model_type_t type, void* model_data, uint32_t model_size);

/* Create process AI profile */
error_code_t ai_engine_create_process_profile(process_t* process);

/* Update process AI profile */
error_code_t ai_engine_update_process_profile(process_t* process);

/* Collect performance metrics */
error_code_t ai_engine_collect_metrics(performance_metrics_t* metrics);

/* Analyze performance metrics */
error_code_t ai_engine_analyze_performance(performance_metrics_t* metrics, void** suggestions);

/* Generate optimization patches */
error_code_t ai_engine_generate_patches(void* suggestions, void** patches, uint32_t* patch_count);

/* Free optimization suggestions */
error_code_t ai_engine_free_suggestions(void* suggestions);

/* Free optimization patches */
error_code_t ai_engine_free_patches(void* patches, uint32_t patch_count);

/* Update optimization history */
error_code_t ai_engine_update_optimization_history(optimization_history_t* history, 
                                                 void* suggestions, void* patches, uint32_t patch_count);

/* Get optimization history */
error_code_t ai_engine_get_optimization_history(optimization_history_t* history);

/* Self-learning interface */
error_code_t ai_engine_learn_from_history(void);
error_code_t ai_engine_adjust_models(void);
error_code_t ai_engine_generate_new_model(ai_model_type_t type);

#endif /* NEXOS_AI_ENGINE_H */