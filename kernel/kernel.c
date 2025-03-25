/**
 * NexOS Kernel - Core Implementation
 * 
 * This file implements the core functionality of the NexOS microkernel.
 */

#include "kernel.h"
#include "../memory/memory.h"
#include "../scheduler/scheduler.h"
#include "../ai_engine/ai_engine.h"
#include "../security/security.h"
#include <stddef.h>

/* Global kernel state */
static struct {
    bool initialized;
    process_t* current_process;
    thread_t* current_thread;
    uint32_t next_pid;
    uint32_t next_tid;
    uint64_t uptime;
    self_evolution_t evolution;
} kernel_state = {0};

/**
 * Initialize the kernel and all subsystems
 */
error_code_t kernel_init(void) {
    if (kernel_state.initialized) {
        return ERROR_NONE;
    }
    
    /* Initialize memory subsystem first */
    error_code_t err = memory_init();
    if (err != ERROR_NONE) {
        return err;
    }
    
    /* Initialize scheduler */
    err = scheduler_init();
    if (err != ERROR_NONE) {
        return err;
    }
    
    /* Initialize AI engine */
    err = ai_engine_init();
    if (err != ERROR_NONE) {
        return err;
    }
    
    /* Initialize security module */
    err = security_init();
    if (err != ERROR_NONE) {
        return err;
    }
    
    /* Initialize self-evolution system */
    err = self_evolution_init();
    if (err != ERROR_NONE) {
        return err;
    }
    
    /* Set initial kernel state */
    kernel_state.initialized = true;
    kernel_state.next_pid = 1;  /* PID 0 reserved for kernel */
    kernel_state.next_tid = 1;  /* TID 0 reserved for kernel main thread */
    kernel_state.uptime = 0;
    
    return ERROR_NONE;
}

/**
 * Create a new process
 */
error_code_t process_create(process_t** process, void (*entry)(void*), void* arg, uint8_t priority) {
    if (!kernel_state.initialized) {
        return ERROR_NOT_IMPLEMENTED;
    }
    
    if (!process || !entry || priority >= MAX_PRIORITY_LEVELS) {
        return ERROR_INVALID_PARAMETER;
    }
    
    /* Allocate process descriptor */
    process_t* new_process = (process_t*)memory_allocate(sizeof(process_t));
    if (!new_process) {
        return ERROR_MEMORY_ALLOCATION;
    }
    
    /* Initialize process descriptor */
    new_process->pid = kernel_state.next_pid++;
    new_process->parent_pid = process_get_current() ? process_get_current()->pid : 0;
    new_process->state = PROCESS_CREATED;
    new_process->priority = priority;
    new_process->thread_count = 0;
    new_process->cpu_time = 0;
    new_process->creation_time = kernel_state.uptime;
    new_process->last_scheduled = 0;
    
    /* Allocate memory space for the process */
    error_code_t err = memory_allocate_process_space(new_process);
    if (err != ERROR_NONE) {
        memory_free(new_process);
        return err;
    }
    
    /* Create initial thread for the process */
    thread_t* initial_thread;
    err = thread_create(&initial_thread, new_process, entry, arg, priority);
    if (err != ERROR_NONE) {
        memory_free_process_space(new_process);
        memory_free(new_process);
        return err;
    }
    
    /* Initialize AI profile for process optimization */
    err = ai_engine_create_process_profile(new_process);
    if (err != ERROR_NONE) {
        /* Non-critical error, continue anyway */
    }
    
    /* Add process to scheduler */
    err = scheduler_add_process(new_process);
    if (err != ERROR_NONE) {
        thread_terminate(initial_thread->tid);
        memory_free_process_space(new_process);
        memory_free(new_process);
        return err;
    }
    
    *process = new_process;
    return ERROR_NONE;
}

/**
 * Terminate a process
 */
error_code_t process_terminate(uint32_t pid) {
    if (!kernel_state.initialized) {
        return ERROR_NOT_IMPLEMENTED;
    }
    
    /* Find process */
    process_t* process = scheduler_find_process(pid);
    if (!process) {
        return ERROR_INVALID_PARAMETER;
    }
    
    /* Check permissions */
    if (process_get_current()->pid != 0 && /* Kernel can terminate any process */
        process_get_current()->pid != process->pid && 
        process_get_current()->pid != process->parent_pid) {
        return ERROR_PERMISSION_DENIED;
    }
    
    /* Terminate all threads */
    for (uint32_t i = 0; i < process->thread_count; i++) {
        thread_terminate(process->threads[i].tid);
    }
    
    /* Free resources */
    memory_free_process_space(process);
    
    /* Remove from scheduler */
    scheduler_remove_process(process);
    
    /* Free process descriptor */
    memory_free(process);
    
    return ERROR_NONE;
}

/**
 * Get current process
 */
process_t* process_get_current(void) {
    return kernel_state.current_process;
}

/**
 * Create a new thread
 */
error_code_t thread_create(thread_t** thread, process_t* process, void (*entry)(void*), void* arg, uint8_t priority) {
    if (!kernel_state.initialized) {
        return ERROR_NOT_IMPLEMENTED;
    }
    
    if (!thread || !process || !entry || priority >= MAX_PRIORITY_LEVELS) {
        return ERROR_INVALID_PARAMETER;
    }
    
    /* Check if process has reached thread limit */
    if (process->thread_count >= MAX_THREADS_PER_PROCESS) {
        return ERROR_RESOURCE_BUSY;
    }
    
    /* Allocate thread descriptor */
    thread_t* new_thread = (thread_t*)memory_allocate(sizeof(thread_t));
    if (!new_thread) {
        return ERROR_MEMORY_ALLOCATION;
    }
    
    /* Initialize thread descriptor */
    new_thread->tid = kernel_state.next_tid++;
    new_thread->pid = process->pid;
    new_thread->state = THREAD_CREATED;
    new_thread->priority = priority;
    new_thread->cpu_time = 0;
    new_thread->last_scheduled = 0;
    new_thread->entry_point = entry;
    new_thread->arg = arg;
    
    /* Allocate stack for the thread */
    error_code_t err = memory_allocate_thread_stack(new_thread, KERNEL_STACK_SIZE);
    if (err != ERROR_NONE) {
        memory_free(new_thread);
        return err;
    }
    
    /* Initialize thread context */
    err = scheduler_init_thread_context(new_thread);
    if (err != ERROR_NONE) {
        memory_free_thread_stack(new_thread);
        memory_free(new_thread);
        return err;
    }
    
    /* Add thread to process */
    process->threads[process->thread_count++] = *new_thread;
    
    /* Add thread to scheduler */
    err = scheduler_add_thread(new_thread);
    if (err != ERROR_NONE) {
        process->thread_count--;
        memory_free_thread_stack(new_thread);
        memory_free(new_thread);
        return err;
    }
    
    *thread = new_thread;
    return ERROR_NONE;
}

/**
 * Get current thread
 */
thread_t* thread_get_current(void) {
    return kernel_state.current_thread;
}

/**
 * Initialize self-evolution system
 */
error_code_t self_evolution_init(void) {
    if (!kernel_state.initialized) {
        return ERROR_NOT_IMPLEMENTED;
    }
    
    /* Initialize self-evolution state */
    kernel_state.evolution.last_analysis_time = 0;
    kernel_state.evolution.optimization_count = 0;
    kernel_state.evolution.patch_count = 0;
    kernel_state.evolution.evolution_level = 0;
    kernel_state.evolution.evolution_enabled = false;
    
    /* Allocate performance metrics storage */
    kernel_state.evolution.performance_metrics = memory_allocate(sizeof(performance_metrics_t));
    if (!kernel_state.evolution.performance_metrics) {
        return ERROR_MEMORY_ALLOCATION;
    }
    
    /* Allocate optimization history storage */
    kernel_state.evolution.optimization_history = memory_allocate(sizeof(optimization_history_t));
    if (!kernel_state.evolution.optimization_history) {
        memory_free(kernel_state.evolution.performance_metrics);
        return ERROR_MEMORY_ALLOCATION;
    }
    
    return ERROR_NONE;
}

/**
 * Enable or disable self-evolution
 */
error_code_t self_evolution_enable(bool enable) {
    if (!kernel_state.initialized) {
        return ERROR_NOT_IMPLEMENTED;
    }
    
    /* Check security policy */
    error_code_t err = security_check_evolution_permission();
    if (err != ERROR_NONE) {
        return err;
    }
    
    kernel_state.evolution.evolution_enabled = enable;
    
    /* If enabling, trigger initial analysis */
    if (enable) {
        return self_evolution_analyze();
    }
    
    return ERROR_NONE;
}

/**
 * Analyze system for potential optimizations
 */
error_code_t self_evolution_analyze(void) {
    if (!kernel_state.initialized || !kernel_state.evolution.evolution_enabled) {
        return ERROR_NOT_IMPLEMENTED;
    }
    
    /* Update analysis timestamp */
    kernel_state.evolution.last_analysis_time = kernel_state.uptime;
    
    /* Collect performance metrics */
    error_code_t err = ai_engine_collect_metrics(kernel_state.evolution.performance_metrics);
    if (err != ERROR_NONE) {
        return err;
    }
    
    /* Analyze collected metrics */
    void* optimization_suggestions = NULL;
    err = ai_engine_analyze_performance(kernel_state.evolution.performance_metrics, &optimization_suggestions);
    if (err != ERROR_NONE) {
        return err;
    }
    
    /* Generate patches for suggested optimizations */
    void* patches = NULL;
    uint32_t patch_count = 0;
    err = ai_engine_generate_patches(optimization_suggestions, &patches, &patch_count);
    if (err != ERROR_NONE) {
        ai_engine_free_suggestions(optimization_suggestions);
        return err;
    }
    
    /* Apply generated patches */
    for (uint32_t i = 0; i < patch_count; i++) {
        void* patch = ((void**)patches)[i];
        uint32_t patch_size = ((uint32_t*)patches)[patch_count + i];
        
        /* Verify patch safety */
        err = security_verify_patch(patch, patch_size);
        if (err != ERROR_NONE) {
            continue;  /* Skip unsafe patches */
        }
        
        /* Apply the patch */
        err = self_evolution_apply_patch(patch, patch_size);
        if (err == ERROR_NONE) {
            kernel_state.evolution.patch_count++;
        }
    }
    
    /* Update optimization history */
    ai_engine_update_optimization_history(kernel_state.evolution.optimization_history, 
                                         optimization_suggestions, patches, patch_count);
    
    /* Clean up */
    ai_engine_free_suggestions(optimization_suggestions);
    ai_engine_free_patches(patches, patch_count);
    
    kernel_state.evolution.optimization_count++;
    return ERROR_NONE;
}

/**
 * Get self-evolution status
 */
self_evolution_t* self_evolution_get_status(void) {
    if (!kernel_state.initialized) {
        return NULL;
    }
    
    return &kernel_state.evolution;
}