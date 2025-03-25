/**
 * NexOS Memory Management Subsystem
 * 
 * This file defines the memory management interface for NexOS.
 * The memory manager is responsible for physical and virtual memory
 * allocation, page mapping, and memory protection.
 */

#ifndef NEXOS_MEMORY_H
#define NEXOS_MEMORY_H

#include "../kernel/kernel.h"
#include <stdint.h>
#include <stdbool.h>

/* Memory region types */
typedef enum {
    MEMORY_REGION_KERNEL,      /* Kernel code and data */
    MEMORY_REGION_USER,        /* User process code and data */
    MEMORY_REGION_DEVICE,      /* Memory-mapped devices */
    MEMORY_REGION_SHARED       /* Shared memory between processes */
} memory_region_type_t;

/* Memory protection flags */
typedef enum {
    MEMORY_PROT_NONE  = 0,
    MEMORY_PROT_READ  = (1 << 0),
    MEMORY_PROT_WRITE = (1 << 1),
    MEMORY_PROT_EXEC  = (1 << 2)
} memory_protection_t;

/* Page descriptor */
typedef struct {
    uint64_t physical_addr;    /* Physical memory address */
    uint64_t virtual_addr;     /* Virtual memory address */
    uint32_t size;             /* Page size in bytes */
    memory_protection_t prot;  /* Protection flags */
    bool present;              /* Whether page is present in physical memory */
    bool dirty;                /* Whether page has been modified */
    bool accessed;             /* Whether page has been accessed */
    bool cached;               /* Whether page should be cached */
} page_t;

/* Memory region descriptor */
typedef struct {
    uint64_t start_addr;       /* Start virtual address */
    uint64_t end_addr;         /* End virtual address */
    memory_region_type_t type; /* Region type */
    memory_protection_t prot;  /* Protection flags */
    uint32_t page_count;       /* Number of pages in region */
    page_t* pages;             /* Array of pages */
    void* owner;               /* Owner process or NULL for kernel */
} memory_region_t;

/* Memory space descriptor */
typedef struct {
    uint32_t region_count;     /* Number of memory regions */
    memory_region_t* regions;  /* Array of memory regions */
    void* page_directory;      /* Architecture-specific page directory */
} memory_space_t;

/* Performance metrics for memory subsystem */
typedef struct {
    uint64_t total_physical_memory;    /* Total physical memory in bytes */
    uint64_t free_physical_memory;     /* Free physical memory in bytes */
    uint64_t total_virtual_memory;     /* Total virtual memory in bytes */
    uint64_t free_virtual_memory;      /* Free virtual memory in bytes */
    uint32_t page_fault_count;         /* Number of page faults */
    uint32_t allocation_count;         /* Number of memory allocations */
    uint32_t free_count;               /* Number of memory frees */
    uint64_t peak_memory_usage;        /* Peak memory usage in bytes */
    float fragmentation_ratio;         /* Memory fragmentation ratio (0-1) */
} memory_metrics_t;

/* Optimization history for memory subsystem */
typedef struct {
    uint32_t optimization_count;       /* Number of optimizations applied */
    uint32_t defragmentation_count;    /* Number of defragmentations */
    uint32_t page_reallocation_count;  /* Number of page reallocations */
    uint32_t cache_optimization_count; /* Number of cache optimizations */
} memory_optimization_t;

/* Initialize memory subsystem */
error_code_t memory_init(void);

/* Allocate memory */
void* memory_allocate(uint32_t size);

/* Free memory */
error_code_t memory_free(void* ptr);

/* Allocate memory space for a process */
error_code_t memory_allocate_process_space(process_t* process);

/* Free memory space for a process */
error_code_t memory_free_process_space(process_t* process);

/* Allocate stack for a thread */
error_code_t memory_allocate_thread_stack(thread_t* thread, uint32_t size);

/* Free stack for a thread */
error_code_t memory_free_thread_stack(thread_t* thread);

/* Map physical memory to virtual address space */
error_code_t memory_map(memory_space_t* space, uint64_t physical_addr, 
                        uint64_t virtual_addr, uint32_t size, memory_protection_t prot);

/* Unmap virtual memory */
error_code_t memory_unmap(memory_space_t* space, uint64_t virtual_addr, uint32_t size);

/* Change memory protection */
error_code_t memory_protect(memory_space_t* space, uint64_t virtual_addr, 
                           uint32_t size, memory_protection_t prot);

/* Get memory metrics */
error_code_t memory_get_metrics(memory_metrics_t* metrics);

/* Self-optimization interface */
error_code_t memory_optimize(void);
error_code_t memory_defragment(void);
error_code_t memory_analyze_usage_patterns(void);
error_code_t memory_apply_optimization(void* optimization_data);

#endif /* NEXOS_MEMORY_H */