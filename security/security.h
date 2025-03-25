/**
 * NexOS Security Subsystem
 * 
 * This file defines the security interface for NexOS.
 * The security subsystem is responsible for verifying self-evolution patches,
 * enforcing security policies, and protecting system integrity.
 */

#ifndef NEXOS_SECURITY_H
#define NEXOS_SECURITY_H

#include "../kernel/kernel.h"
#include <stdint.h>
#include <stdbool.h>

/* Security policy levels */
typedef enum {
    SECURITY_POLICY_PERMISSIVE,    /* Allow most operations */
    SECURITY_POLICY_STANDARD,      /* Standard security checks */
    SECURITY_POLICY_STRICT,        /* Strict security checks */
    SECURITY_POLICY_PARANOID       /* Maximum security */
} security_policy_t;

/* Patch verification result */
typedef enum {
    PATCH_VERIFY_SUCCESS,          /* Patch is safe */
    PATCH_VERIFY_INVALID_FORMAT,   /* Invalid patch format */
    PATCH_VERIFY_UNSAFE_OPERATION, /* Patch contains unsafe operations */
    PATCH_VERIFY_INTEGRITY_FAIL,   /* Patch integrity check failed */
    PATCH_VERIFY_POLICY_VIOLATION, /* Patch violates security policy */
    PATCH_VERIFY_UNKNOWN_ERROR     /* Unknown verification error */
} patch_verify_result_t;

/* Security policy descriptor */
typedef struct {
    security_policy_t level;               /* Policy level */
    bool allow_self_evolution;             /* Allow self-evolution */
    bool allow_kernel_modifications;       /* Allow kernel modifications */
    bool allow_driver_modifications;       /* Allow driver modifications */
    bool allow_memory_layout_changes;      /* Allow memory layout changes */
    bool allow_scheduler_modifications;    /* Allow scheduler modifications */
    bool require_verification;             /* Require patch verification */
    bool require_rollback_capability;      /* Require rollback capability */
    uint32_t max_patch_size;               /* Maximum patch size in bytes */
    uint32_t max_patches_per_cycle;        /* Maximum patches per cycle */
} security_policy_descriptor_t;

/* Patch descriptor */
typedef struct {
    uint32_t id;                           /* Patch ID */
    uint32_t size;                         /* Patch size in bytes */
    uint64_t timestamp;                    /* Creation timestamp */
    uint32_t target_module;                /* Target module ID */
    uint32_t target_offset;                /* Target offset within module */
    uint32_t original_size;                /* Size of original code */
    void* original_code;                   /* Original code backup */
    void* patch_code;                      /* Patch code */
    bool applied;                          /* Whether patch is applied */
    bool verified;                         /* Whether patch is verified */
} patch_descriptor_t;

/* Rollback entry */
typedef struct {
    uint32_t patch_id;                     /* Patch ID */
    uint64_t apply_timestamp;              /* Application timestamp */
    void* original_code;                   /* Original code backup */
    uint32_t original_size;                /* Size of original code */
    uint32_t target_module;                /* Target module ID */
    uint32_t target_offset;                /* Target offset within module */
} rollback_entry_t;

/* Rollback log */
typedef struct {
    uint32_t entry_count;                  /* Number of entries */
    rollback_entry_t* entries;             /* Array of entries */
} rollback_log_t;

/* Initialize security subsystem */
error_code_t security_init(void);

/* Set security policy */
error_code_t security_set_policy(security_policy_t policy);

/* Get security policy */
error_code_t security_get_policy(security_policy_descriptor_t* policy);

/* Verify patch safety */
error_code_t security_verify_patch(void* patch, uint32_t size);

/* Check evolution permission */
error_code_t security_check_evolution_permission(void);

/* Create rollback entry */
error_code_t security_create_rollback_entry(patch_descriptor_t* patch);

/* Rollback patch */
error_code_t security_rollback_patch(uint32_t patch_id);

/* Rollback all patches */
error_code_t security_rollback_all(void);

/* Get rollback log */
error_code_t security_get_rollback_log(rollback_log_t* log);

/* Verify system integrity */
error_code_t security_verify_integrity(void);

/* Self-protection interface */
error_code_t security_protect_critical_regions(void);
error_code_t security_monitor_modifications(void);
error_code_t security_detect_anomalies(void);

#endif /* NEXOS_SECURITY_H */