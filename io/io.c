#include "io.h"
#include "../memory/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

/* I/O subsystem state */
static struct {
    bool initialized;
    io_metrics_t metrics;
    io_scheduling_policy_t policy;
    uint32_t next_request_id;
    uint32_t next_device_id;
    uint32_t next_file_id;
} io_state = {0};

/**
 * Initialize I/O subsystem
 */
error_code_t io_init(void) {
    printf("Initializing I/O subsystem...\n");
    
    if (io_state.initialized) {
        return ERROR_NONE;
    }
    
    /* Initialize I/O state */
    io_state.initialized = true;
    io_state.policy = IO_SCHED_FIFO;
    io_state.next_request_id = 1;
    io_state.next_device_id = 1;
    io_state.next_file_id = 1;
    
    /* Initialize metrics */
    memset(&io_state.metrics, 0, sizeof(io_metrics_t));
    
    printf("I/O subsystem initialized successfully\n");
    return ERROR_NONE;
}

/**
 * Get I/O metrics
 */
error_code_t io_get_metrics(io_metrics_t* metrics) {
    if (!io_state.initialized) {
        return ERROR_NOT_IMPLEMENTED;
    }
    
    if (!metrics) {
        return ERROR_INVALID_PARAMETER;
    }
    
    /* Copy metrics */
    memcpy(metrics, &io_state.metrics, sizeof(io_metrics_t));
    
    return ERROR_NONE;
}

/**
 * Create a TCP server socket
 */
error_code_t io_create_server_socket(uint16_t port, int* server_fd) {
    if (!io_state.initialized) {
        return ERROR_NOT_IMPLEMENTED;
    }
    
    if (!server_fd) {
        return ERROR_INVALID_PARAMETER;
    }
    
    /* Create socket */
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return ERROR_RESOURCE_BUSY;
    }
    
    /* Set socket options */
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(fd);
        return ERROR_RESOURCE_BUSY;
    }
    
    /* Set non-blocking mode */
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    
    /* Bind socket */
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    if (bind(fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        close(fd);
        return ERROR_RESOURCE_BUSY;
    }
    
    /* Listen for connections */
    if (listen(fd, SOMAXCONN) < 0) {
        close(fd);
        return ERROR_RESOURCE_BUSY;
    }
    
    *server_fd = fd;
    return ERROR_NONE;
}

/**
 * Accept a connection on a server socket
 */
error_code_t io_accept_connection(int server_fd, int* client_fd) {
    if (!io_state.initialized) {
        return ERROR_NOT_IMPLEMENTED;
    }
    
    if (!client_fd) {
        return ERROR_INVALID_PARAMETER;
    }
    
    /* Accept connection */
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    
    if (fd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return ERROR_TIMEOUT;
        }
        return ERROR_RESOURCE_BUSY;
    }
    
    /* Set non-blocking mode */
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    
    /* Update metrics */
    io_state.metrics.global.io_wait_time = 0;
    io_state.metrics.pending_requests++;
    
    *client_fd = fd;
    return ERROR_NONE;
}

/**
 * Read data from a file descriptor
 */
error_code_t io_read(int fd, void* buffer, uint32_t size, uint32_t* bytes_read) {
    if (!io_state.initialized) {
        return ERROR_NOT_IMPLEMENTED;
    }
    
    if (!buffer || !bytes_read) {
        return ERROR_INVALID_PARAMETER;
    }
    
    /* Read data */
    ssize_t result = read(fd, buffer, size);
    
    if (result < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            *bytes_read = 0;
            return ERROR_TIMEOUT;
        }
        return ERROR_RESOURCE_BUSY;
    }
    
    /* Update metrics */
    io_state.metrics.global.read_count++;
    io_state.metrics.global.read_bytes += result;
    
    *bytes_read = (uint32_t)result;
    return ERROR_NONE;
}

/**
 * Write data to a file descriptor
 */
error_code_t io_write(int fd, const void* buffer, uint32_t size, uint32_t* bytes_written) {
    if (!io_state.initialized) {
        return ERROR_NOT_IMPLEMENTED;
    }
    
    if (!buffer || !bytes_written) {
        return ERROR_INVALID_PARAMETER;
    }
    
    /* Write data */
    ssize_t result = write(fd, buffer, size);
    
    if (result < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            *bytes_written = 0;
            return ERROR_TIMEOUT;
        }
        return ERROR_RESOURCE_BUSY;
    }
    
    /* Update metrics */
    io_state.metrics.global.write_count++;
    io_state.metrics.global.write_bytes += result;
    
    *bytes_written = (uint32_t)result;
    return ERROR_NONE;
}

/**
 * Close a file descriptor
 */
error_code_t io_close(int fd) {
    if (!io_state.initialized) {
        return ERROR_NOT_IMPLEMENTED;
    }
    
    /* Close file descriptor */
    if (close(fd) < 0) {
        return ERROR_RESOURCE_BUSY;
    }
    
    /* Update metrics */
    io_state.metrics.pending_requests--;
    io_state.metrics.completed_requests++;
    
    return ERROR_NONE;
}

/**
 * Set I/O scheduling policy
 */
error_code_t io_set_scheduling_policy(io_scheduling_policy_t policy) {
    if (!io_state.initialized) {
        return ERROR_NOT_IMPLEMENTED;
    }
    
    io_state.policy = policy;
    return ERROR_NONE;
}

/**
 * Optimize I/O subsystem
 */
error_code_t io_optimize(void) {
    if (!io_state.initialized) {
        return ERROR_NOT_IMPLEMENTED;
    }
    
    /* In a real implementation, this would analyze I/O patterns and optimize accordingly */
    /* For now, we'll just return success */
    
    return ERROR_NONE;
}