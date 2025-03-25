#include "kernel/kernel.h"
#include "memory/memory.h"
#include "scheduler/scheduler.h"
#include "io/io.h"
#include "ai_engine/ai_engine.h"
#include "security/security.h"
#include "webserver/webserver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

/* Global variables */
volatile sig_atomic_t running = 1;

/* Function prototypes */
void handle_signal(int sig);
void setup_signal_handlers();

/**
 * Signal handler
 */
void handle_signal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\nShutting down NexOS...\n");
        running = 0;
    }
}

/**
 * Setup signal handlers
 */
void setup_signal_handlers() {
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);  /* Ignore SIGPIPE to handle client disconnects gracefully */
}

/**
 * Main function
 */
int main(int argc, char* argv[]) {
    /* Default configuration */
    uint16_t port = 8080;
    char* webroot = "./webroot";
    
    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
            if (i + 1 < argc) {
                port = atoi(argv[i + 1]);
                i++;
            }
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--root") == 0) {
            if (i + 1 < argc) {
                webroot = argv[i + 1];
                i++;
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  -p, --port PORT    Port to listen on (default: 8080)\n");
            printf("  -r, --root DIR     Web root directory (default: ./webroot)\n");
            printf("  -h, --help         Show this help message\n");
            return 0;
        }
    }
    
    /* Setup signal handlers */
    setup_signal_handlers();
    
    printf("\nNexOS Web Server\n");
    printf("===============\n\n");
    
    /* Initialize kernel */
    printf("Initializing kernel...\n");
    error_code_t err = kernel_init();
    if (err != ERROR_NONE) {
        printf("Failed to initialize kernel: %d\n", err);
        return 1;
    }
    
    /* Initialize security subsystem */
    printf("\nInitializing security subsystem...\n");
    err = security_init();
    if (err != ERROR_NONE) {
        printf("Failed to initialize security subsystem: %d\n", err);
        return 1;
    }
    
    /* Initialize AI engine */
    printf("\nInitializing AI engine...\n");
    err = ai_engine_init();
    if (err != ERROR_NONE) {
        printf("Failed to initialize AI engine: %d\n", err);
        return 1;
    }
    
    /* Initialize self-evolution system */
    printf("\nInitializing self-evolution system...\n");
    err = self_evolution_init();
    if (err != ERROR_NONE) {
        printf("Failed to initialize self-evolution system: %d\n", err);
        return 1;
    }
    
    /* Enable self-evolution */
    printf("\nEnabling self-evolution...\n");
    err = self_evolution_enable(true);
    if (err != ERROR_NONE) {
        printf("Failed to enable self-evolution: %d\n", err);
        return 1;
    }
    
    /* Initialize web server */
    printf("\nInitializing web server...\n");
    webserver_config_t config = {
        .port = port,
        .webroot = webroot,
        .max_connections = 1000,
        .timeout = 30000
    };
    err = webserver_init(&config);
    if (err != ERROR_NONE) {
        printf("Failed to initialize web server: %d\n", err);
        return 1;
    }
    
    /* Start web server */
    printf("\nStarting web server on port %d...\n", port);
    printf("Web root: %s\n", webroot);
    printf("Press Ctrl+C to stop\n\n");
    
    /* Create a thread for the web server */
    process_t* web_process;
    err = process_create(&web_process, (void (*)(void*))webserver_start, NULL, 10);
    if (err != ERROR_NONE) {
        printf("Failed to create web server process: %d\n", err);
        return 1;
    }
    
    /* Main loop */
    while (running) {
        /* Perform periodic self-evolution analysis */
        self_evolution_analyze();
        
        /* Sleep for a while */
        sleep(60);
    }
    
    /* Stop web server */
    printf("\nStopping web server...\n");
    webserver_stop();
    
    /* Terminate web process */
    process_terminate(web_process->pid);
    
    printf("\nNexOS shut down successfully\n");
    return 0;
}