/**
 * NexOS I/O Subsystem
 * 
 * This file defines the I/O interface for NexOS.
 * The I/O subsystem is responsible for device access, file operations,
 * and I/O scheduling.
 */

#ifndef NEXOS_IO_H
#define NEXOS_IO_H

#include "../kernel/kernel.h"
#include <stdint.h>
#include <stdbool.h>

/* I/O operation types */
typedef enum {
    IO_OP_READ,                /* Read operation */
    IO_OP_WRITE,               /* Write operation */
    IO_OP_SEEK,                /* Seek operation */
    IO_OP_IOCTL,               /* I/O control operation */
    IO_OP_OPEN,                /* Open operation */
    IO_OP_CLOSE,               /* Close operation */
    IO_OP_FLUSH                /* Flush operation */
} io_operation_t;

/* I/O scheduling policies */
typedef enum {
    IO_SCHED_FIFO,             /* First-in, first-out */
    IO_SCHED_DEADLINE,         /* Deadline-based */
    IO_SCHED_PRIORITY,         /* Priority-based */
    IO_SCHED_ADAPTIVE          /* AI-adaptive scheduling */
} io_scheduling_policy_t;

/* Device types */
typedef enum {
    DEVICE_TYPE_BLOCK,         /* Block device */
    DEVICE_TYPE_CHAR,          /* Character device */
    DEVICE_TYPE_NETWORK,       /* Network device */
    DEVICE_TYPE_VIRTUAL        /* Virtual device */
} device_type_t;

/* Device descriptor */
typedef struct {
    uint32_t id;               /* Device ID */
    char name[32];             /* Device name */
    device_type_t type;        /* Device type */
    uint32_t major;            /* Major device number */
    uint32_t minor;            /* Minor device number */
    bool present;              /* Whether device is present */
    bool initialized;          /* Whether device is initialized */
    void* driver;              /* Device driver */
    void* private_data;        /* Device-specific data */
} device_t;

/* File descriptor */
typedef struct {
    uint32_t id;               /* File descriptor ID */
    uint32_t pid;              /* Owner process ID */
    void* device;              /* Associated device */
    uint64_t position;         /* Current file position */
    uint32_t flags;            /* File flags */
    uint32_t mode;             /* File mode */
    void* private_data;        /* File-specific data */
} file_t;

/* I/O request */
typedef struct {
    uint32_t id;               /* Request ID */
    io_operation_t operation;  /* Operation type */
    void* device;              /* Target device */
    void* buffer;              /* Data buffer */
    uint32_t size;             /* Buffer size */
    uint64_t offset;           /* Device offset */
    uint32_t flags;            /* Request flags */
    uint8_t priority;          /* Request priority */
    uint64_t deadline;         /* Request deadline */
    uint32_t pid;              /* Requesting process ID */
    void* completion_callback; /* Completion callback */
    void* private_data;        /* Request-specific data */
} io_request_t;

/* I/O statistics */
typedef struct {
    uint64_t read_count;       /* Number of read operations */
    uint64_t write_count;      /* Number of write operations */
    uint64_t read_bytes;       /* Number of bytes read */
    uint64_t write_bytes;      /* Number of bytes written */
    uint64_t io_wait_time;     /* Total I/O wait time */
    float read_throughput;     /* Read throughput in bytes/second */
    float write_throughput;    /* Write throughput in bytes/second */
    float average_latency;     /* Average I/O latency in milliseconds */
    uint32_t queue_depth;      /* Average I/O queue depth */
} io_statistics_t;

/* Performance metrics for I/O subsystem */
typedef struct {
    io_statistics_t global;    /* Global I/O statistics */
    io_statistics_t* devices;  /* Per-device I/O statistics */
    uint32_t device_count;     /* Number of devices */
    uint32_t pending_requests; /* Number of pending requests */
    uint32_t completed_requests; /* Number of completed requests */
    float io_utilization;      /* I/O subsystem utilization (0-1) */
} io_metrics_t;

/* Optimization history for I/O subsystem */
typedef struct {
    uint32_t policy_changes;   /* Number of policy changes */
    uint32_t buffer_adjustments; /* Number of buffer size adjustments */
    uint32_t queue_adjustments; /* Number of queue size adjustments */
    uint32_t prefetch_adjustments; /* Number of prefetch adjustments */
} io_optimization_t;

/* Initialize I/O subsystem */
error_code_t io_init(void);

/* Register device */
error_code_t io_register_device(device_t* device);

/* Unregister device */
error_code_t io_unregister_device(uint32_t device_id);

/* Find device by ID */
device_t* io_find_device(uint32_t device_id);

/* Find device by name */
device_t* io_find_device_by_name(const char* name);

/* Open file */
error_code_t io_open(const char* path, uint32_t flags, uint32_t mode, file_t** file);

/* Close file */
error_code_t io_close(file_t* file);

/* Read from file */
error_code_t io_read(file_t* file, void* buffer, uint32_t size, uint32_t* bytes_read);

/* Write to file */
error_code_t io_write(file_t* file, const void* buffer, uint32_t size, uint32_t* bytes_written);

/* Seek in file */
error_code_t io_seek(file_t* file, int64_t offset, uint32_t whence);

/* I/O control */
error_code_t io_ioctl(file_t* file, uint32_t request, void* arg);

/* Submit I/O request */
error_code_t io_submit_request(io_request_t* request);

/* Cancel I/O request */
error_code_t io_cancel_request(uint32_t request_id);

/* Wait for I/O completion */
error_code_t io_wait_completion(uint32_t request_id, uint64_t timeout);

/* Set I/O scheduling policy */
error_code_t io_set_scheduling_policy(io_scheduling_policy_t policy);

/* Get I/O metrics */
error_code_t io_get_metrics(io_metrics_t* metrics);

/* Self-optimization interface */
error_code_t io_optimize(void);
error_code_t io_analyze_patterns(void);
error_code_t io_adjust_buffers(void);
error_code_t io_apply_optimization(void* optimization_data);

/* Network I/O functions for web server */
error_code_t io_create_server_socket(uint16_t port, int* server_fd);
error_code_t io_accept_connection(int server_fd, int* client_fd);
error_code_t io_read_socket(int fd, void* buffer, uint32_t size, uint32_t* bytes_read);
error_code_t io_write_socket(int fd, const void* buffer, uint32_t size, uint32_t* bytes_written);
error_code_t io_close_socket(int fd);

#endif /* NEXOS_IO_H */