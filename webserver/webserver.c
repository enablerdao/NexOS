#include "webserver.h"
#include "../io/io.h"
#include "../memory/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <ctype.h>

#define BUFFER_SIZE 4096
#define SERVER_NAME "NexOS WebServer/1.0"

/* Web server state */
static struct {
    bool initialized;
    bool running;
    webserver_config_t config;
    int server_fd;
    uint32_t request_count;
    uint32_t error_count;
    uint64_t bytes_sent;
    uint64_t bytes_received;
} webserver_state = {0};

/* Function prototypes */
static error_code_t handle_client(int client_fd);
static error_code_t parse_request(int client_fd, http_request_t* request);
static error_code_t build_response(http_request_t* request, http_response_t* response);
static error_code_t send_response(int client_fd, http_response_t* response);
static void free_request(http_request_t* request);
static void free_response(http_response_t* response);
static error_code_t serve_file(const char* path, http_response_t* response);
static error_code_t serve_directory(const char* path, http_response_t* response);
static const char* get_mime_type(const char* path);
static char* url_decode(const char* str);

/**
 * Initialize web server
 */
error_code_t webserver_init(webserver_config_t* config) {
    printf("Initializing web server...\n");
    
    if (webserver_state.initialized) {
        return ERROR_NONE;
    }
    
    if (!config) {
        return ERROR_INVALID_PARAMETER;
    }
    
    /* Initialize I/O subsystem if not already initialized */
    error_code_t err = io_init();
    if (err != ERROR_NONE) {
        return err;
    }
    
    /* Copy configuration */
    webserver_state.config.port = config->port;
    webserver_state.config.webroot = strdup(config->webroot);
    webserver_state.config.max_connections = config->max_connections;
    webserver_state.config.timeout = config->timeout;
    
    /* Create webroot directory if it doesn't exist */
    struct stat st = {0};
    if (stat(webserver_state.config.webroot, &st) == -1) {
        mkdir(webserver_state.config.webroot, 0755);
    }
    
    webserver_state.initialized = true;
    printf("Web server initialized successfully\n");
    return ERROR_NONE;
}

/**
 * Start web server
 */
error_code_t webserver_start(void) {
    printf("Starting web server on port %d...\n", webserver_state.config.port);
    
    if (!webserver_state.initialized) {
        return ERROR_NOT_IMPLEMENTED;
    }
    
    if (webserver_state.running) {
        return ERROR_NONE;
    }
    
    /* Create server socket */
    error_code_t err = io_create_server_socket(webserver_state.config.port, &webserver_state.server_fd);
    if (err != ERROR_NONE) {
        return err;
    }
    
    webserver_state.running = true;
    printf("Web server started successfully\n");
    
    /* Main server loop */
    while (webserver_state.running) {
        int client_fd;
        err = io_accept_connection(webserver_state.server_fd, &client_fd);
        
        if (err == ERROR_NONE) {
            /* Handle client */
            handle_client(client_fd);
        } else if (err == ERROR_TIMEOUT) {
            /* No new connections, just continue */
            usleep(1000); /* Sleep for 1ms to avoid busy waiting */
        } else {
            /* Error accepting connection */
            webserver_state.error_count++;
        }
    }
    
    return ERROR_NONE;
}

/**
 * Stop web server
 */
error_code_t webserver_stop(void) {
    printf("Stopping web server...\n");
    
    if (!webserver_state.initialized || !webserver_state.running) {
        return ERROR_NONE;
    }
    
    /* Close server socket */
    io_close(webserver_state.server_fd);
    
    webserver_state.running = false;
    printf("Web server stopped successfully\n");
    return ERROR_NONE;
}

/**
 * Get web server statistics
 */
error_code_t webserver_get_stats(void* stats) {
    if (!webserver_state.initialized) {
        return ERROR_NOT_IMPLEMENTED;
    }
    
    if (!stats) {
        return ERROR_INVALID_PARAMETER;
    }
    
    /* Copy statistics */
    struct {
        uint32_t request_count;
        uint32_t error_count;
        uint64_t bytes_sent;
        uint64_t bytes_received;
    } *s = stats;
    
    s->request_count = webserver_state.request_count;
    s->error_count = webserver_state.error_count;
    s->bytes_sent = webserver_state.bytes_sent;
    s->bytes_received = webserver_state.bytes_received;
    
    return ERROR_NONE;
}

/**
 * Optimize web server
 */
error_code_t webserver_optimize(void) {
    if (!webserver_state.initialized) {
        return ERROR_NOT_IMPLEMENTED;
    }
    
    /* In a real implementation, this would analyze web server performance and optimize accordingly */
    /* For now, we'll just optimize the I/O subsystem */
    
    return io_optimize();
}

/**
 * Handle client connection
 */
static error_code_t handle_client(int client_fd) {
    http_request_t request = {0};
    http_response_t response = {0};
    
    /* Parse request */
    error_code_t err = parse_request(client_fd, &request);
    if (err != ERROR_NONE) {
        /* Bad request */
        response.status = HTTP_STATUS_BAD_REQUEST;
        response.body = strdup("<html><body><h1>400 Bad Request</h1></body></html>");
        response.body_length = strlen(response.body);
        send_response(client_fd, &response);
        free_response(&response);
        io_close(client_fd);
        return err;
    }
    
    /* Build response */
    err = build_response(&request, &response);
    if (err != ERROR_NONE) {
        /* Internal server error */
        response.status = HTTP_STATUS_INTERNAL_SERVER_ERROR;
        response.body = strdup("<html><body><h1>500 Internal Server Error</h1></body></html>");
        response.body_length = strlen(response.body);
        send_response(client_fd, &response);
        free_response(&response);
        io_close(client_fd);
        return err;
    }
    
    /* Send response */
    err = send_response(client_fd, &response);
    
    /* Clean up */
    free_request(&request);
    free_response(&response);
    
    /* Close connection if not keep-alive */
    if (!request.keep_alive) {
        io_close(client_fd);
    }
    
    /* Update statistics */
    webserver_state.request_count++;
    
    return err;
}

/**
 * Parse HTTP request
 */
static error_code_t parse_request(int client_fd, http_request_t* request) {
    char buffer[BUFFER_SIZE];
    uint32_t bytes_read;
    
    /* Read request */
    error_code_t err = io_read(client_fd, buffer, BUFFER_SIZE - 1, &bytes_read);
    if (err != ERROR_NONE || bytes_read == 0) {
        return ERROR_INVALID_PARAMETER;
    }
    
    buffer[bytes_read] = '\0';
    
    /* Update statistics */
    webserver_state.bytes_received += bytes_read;
    
    /* Parse request line */
    char* line = strtok(buffer, "\r\n");
    if (!line) return ERROR_INVALID_PARAMETER;
    
    char* method_str = strtok(line, " ");
    if (!method_str) return ERROR_INVALID_PARAMETER;
    
    char* path = strtok(NULL, " ");
    if (!path) return ERROR_INVALID_PARAMETER;
    
    char* version = strtok(NULL, " ");
    if (!version) return ERROR_INVALID_PARAMETER;
    
    /* Parse method */
    if (strcmp(method_str, "GET") == 0) {
        request->method = HTTP_METHOD_GET;
    } else if (strcmp(method_str, "HEAD") == 0) {
        request->method = HTTP_METHOD_HEAD;
    } else if (strcmp(method_str, "POST") == 0) {
        request->method = HTTP_METHOD_POST;
    } else if (strcmp(method_str, "PUT") == 0) {
        request->method = HTTP_METHOD_PUT;
    } else if (strcmp(method_str, "DELETE") == 0) {
        request->method = HTTP_METHOD_DELETE;
    } else if (strcmp(method_str, "OPTIONS") == 0) {
        request->method = HTTP_METHOD_OPTIONS;
    } else {
        request->method = HTTP_METHOD_UNKNOWN;
    }
    
    /* Copy path and version */
    request->path = url_decode(path);
    request->version = strdup(version);
    
    /* Parse headers */
    while ((line = strtok(NULL, "\r\n")) != NULL && line[0] != '\0') {
        char* key = strtok(line, ":");
        char* value = strtok(NULL, "\0");
        
        if (key && value) {
            /* Skip leading whitespace in value */
            while (*value == ' ') value++;
            
            request->headers[request->header_count][0] = strdup(key);
            request->headers[request->header_count][1] = strdup(value);
            request->header_count++;
            
            /* Check for Connection header */
            if (strcasecmp(key, "Connection") == 0) {
                if (strcasecmp(value, "keep-alive") == 0) {
                    request->keep_alive = 1;
                }
            }
        }
        
        if (request->header_count >= 100) break;
    }
    
    /* TODO: Parse request body if needed */
    
    return ERROR_NONE;
}

/**
 * Build HTTP response
 */
static error_code_t build_response(http_request_t* request, http_response_t* response) {
    /* Default to 200 OK */
    response->status = HTTP_STATUS_OK;
    
    /* Add default headers */
    response->headers[response->header_count][0] = strdup("Server");
    response->headers[response->header_count][1] = strdup(SERVER_NAME);
    response->header_count++;
    
    response->headers[response->header_count][0] = strdup("Date");
    time_t now = time(NULL);
    char date[100];
    strftime(date, sizeof(date), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
    response->headers[response->header_count][1] = strdup(date);
    response->header_count++;
    
    /* Handle Connection header */
    response->headers[response->header_count][0] = strdup("Connection");
    if (request->keep_alive) {
        response->headers[response->header_count][1] = strdup("keep-alive");
    } else {
        response->headers[response->header_count][1] = strdup("close");
    }
    response->header_count++;
    
    /* Handle different request methods */
    if (request->method == HTTP_METHOD_GET) {
        /* Construct full path */
        char full_path[BUFFER_SIZE];
        snprintf(full_path, BUFFER_SIZE, "%s%s", webserver_state.config.webroot, request->path);
        
        /* Check if path ends with '/' */
        size_t path_len = strlen(full_path);
        if (path_len > 0 && full_path[path_len - 1] == '/') {
            strcat(full_path, "index.html");
        }
        
        /* Check if file exists */
        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                /* If it's a directory and doesn't end with '/', redirect */
                if (request->path[strlen(request->path) - 1] != '/') {
                    response->status = HTTP_STATUS_MOVED_PERMANENTLY;
                    char location[BUFFER_SIZE];
                    snprintf(location, BUFFER_SIZE, "%s/", request->path);
                    response->headers[response->header_count][0] = strdup("Location");
                    response->headers[response->header_count][1] = strdup(location);
                    response->header_count++;
                    
                    response->body = strdup("<html><body><h1>301 Moved Permanently</h1></body></html>");
                    response->body_length = strlen(response->body);
                } else {
                    /* Serve directory listing */
                    serve_directory(full_path, response);
                }
            } else {
                /* Serve file */
                serve_file(full_path, response);
            }
        } else {
            /* File not found */
            response->status = HTTP_STATUS_NOT_FOUND;
            response->body = strdup("<html><body><h1>404 Not Found</h1></body></html>");
            response->body_length = strlen(response->body);
        }
    } else if (request->method == HTTP_METHOD_HEAD) {
        /* Same as GET but without body */
        char full_path[BUFFER_SIZE];
        snprintf(full_path, BUFFER_SIZE, "%s%s", webserver_state.config.webroot, request->path);
        
        struct stat st;
        if (stat(full_path, &st) == 0) {
            /* File exists, set Content-Type and Content-Length but don't send body */
            response->headers[response->header_count][0] = strdup("Content-Type");
            response->headers[response->header_count][1] = strdup(get_mime_type(full_path));
            response->header_count++;
            
            char content_length[20];
            snprintf(content_length, sizeof(content_length), "%ld", (long)st.st_size);
            response->headers[response->header_count][0] = strdup("Content-Length");
            response->headers[response->header_count][1] = strdup(content_length);
            response->header_count++;
        } else {
            response->status = HTTP_STATUS_NOT_FOUND;
        }
    } else {
        /* Method not supported */
        response->status = HTTP_STATUS_METHOD_NOT_ALLOWED;
        response->headers[response->header_count][0] = strdup("Allow");
        response->headers[response->header_count][1] = strdup("GET, HEAD");
        response->header_count++;
        
        response->body = strdup("<html><body><h1>405 Method Not Allowed</h1></body></html>");
        response->body_length = strlen(response->body);
    }
    
    /* Add Content-Type and Content-Length headers if not already set */
    if (response->body && response->body_length > 0) {
        int has_content_type = 0;
        int has_content_length = 0;
        
        for (int i = 0; i < response->header_count; i++) {
            if (strcasecmp(response->headers[i][0], "Content-Type") == 0) {
                has_content_type = 1;
            }
            if (strcasecmp(response->headers[i][0], "Content-Length") == 0) {
                has_content_length = 1;
            }
        }
        
        if (!has_content_type) {
            response->headers[response->header_count][0] = strdup("Content-Type");
            response->headers[response->header_count][1] = strdup("text/html");
            response->header_count++;
        }
        
        if (!has_content_length) {
            char content_length[20];
            snprintf(content_length, sizeof(content_length), "%zu", response->body_length);
            response->headers[response->header_count][0] = strdup("Content-Length");
            response->headers[response->header_count][1] = strdup(content_length);
            response->header_count++;
        }
    }
    
    return ERROR_NONE;
}

/**
 * Send HTTP response
 */
static error_code_t send_response(int client_fd, http_response_t* response) {
    char buffer[BUFFER_SIZE];
    uint32_t bytes_written;
    
    /* Status line */
    const char* status_text;
    switch (response->status) {
        case HTTP_STATUS_OK: status_text = "OK"; break;
        case HTTP_STATUS_CREATED: status_text = "Created"; break;
        case HTTP_STATUS_ACCEPTED: status_text = "Accepted"; break;
        case HTTP_STATUS_NO_CONTENT: status_text = "No Content"; break;
        case HTTP_STATUS_MOVED_PERMANENTLY: status_text = "Moved Permanently"; break;
        case HTTP_STATUS_FOUND: status_text = "Found"; break;
        case HTTP_STATUS_NOT_MODIFIED: status_text = "Not Modified"; break;
        case HTTP_STATUS_BAD_REQUEST: status_text = "Bad Request"; break;
        case HTTP_STATUS_UNAUTHORIZED: status_text = "Unauthorized"; break;
        case HTTP_STATUS_FORBIDDEN: status_text = "Forbidden"; break;
        case HTTP_STATUS_NOT_FOUND: status_text = "Not Found"; break;
        case HTTP_STATUS_METHOD_NOT_ALLOWED: status_text = "Method Not Allowed"; break;
        case HTTP_STATUS_INTERNAL_SERVER_ERROR: status_text = "Internal Server Error"; break;
        case HTTP_STATUS_NOT_IMPLEMENTED: status_text = "Not Implemented"; break;
        case HTTP_STATUS_BAD_GATEWAY: status_text = "Bad Gateway"; break;
        case HTTP_STATUS_SERVICE_UNAVAILABLE: status_text = "Service Unavailable"; break;
        default: status_text = "Unknown";
    }
    
    int len = snprintf(buffer, BUFFER_SIZE, "HTTP/1.1 %d %s\r\n", response->status, status_text);
    io_write(client_fd, buffer, len, &bytes_written);
    webserver_state.bytes_sent += bytes_written;
    
    /* Headers */
    for (int i = 0; i < response->header_count; i++) {
        len = snprintf(buffer, BUFFER_SIZE, "%s: %s\r\n", response->headers[i][0], response->headers[i][1]);
        io_write(client_fd, buffer, len, &bytes_written);
        webserver_state.bytes_sent += bytes_written;
    }
    
    /* End of headers */
    io_write(client_fd, "\r\n", 2, &bytes_written);
    webserver_state.bytes_sent += bytes_written;
    
    /* Body */
    if (response->body && response->body_length > 0) {
        io_write(client_fd, response->body, response->body_length, &bytes_written);
        webserver_state.bytes_sent += bytes_written;
    }
    
    return ERROR_NONE;
}

/**
 * Free request resources
 */
static void free_request(http_request_t* request) {
    if (request->path) free(request->path);
    if (request->version) free(request->version);
    
    for (int i = 0; i < request->header_count; i++) {
        if (request->headers[i][0]) free(request->headers[i][0]);
        if (request->headers[i][1]) free(request->headers[i][1]);
    }
    
    if (request->body) free(request->body);
    
    memset(request, 0, sizeof(http_request_t));
}

/**
 * Free response resources
 */
static void free_response(http_response_t* response) {
    for (int i = 0; i < response->header_count; i++) {
        if (response->headers[i][0]) free(response->headers[i][0]);
        if (response->headers[i][1]) free(response->headers[i][1]);
    }
    
    if (response->body) free(response->body);
    
    memset(response, 0, sizeof(http_response_t));
}

/**
 * Serve a file
 */
static error_code_t serve_file(const char* path, http_response_t* response) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        response->status = HTTP_STATUS_NOT_FOUND;
        response->body = strdup("<html><body><h1>404 Not Found</h1></body></html>");
        response->body_length = strlen(response->body);
        return ERROR_INVALID_PARAMETER;
    }
    
    /* Get file size */
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    /* Allocate memory for file content */
    response->body = malloc(file_size);
    if (!response->body) {
        fclose(file);
        response->status = HTTP_STATUS_INTERNAL_SERVER_ERROR;
        response->body = strdup("<html><body><h1>500 Internal Server Error</h1></body></html>");
        response->body_length = strlen(response->body);
        return ERROR_MEMORY_ALLOCATION;
    }
    
    /* Read file content */
    response->body_length = fread(response->body, 1, file_size, file);
    fclose(file);
    
    /* Set Content-Type header */
    response->headers[response->header_count][0] = strdup("Content-Type");
    response->headers[response->header_count][1] = strdup(get_mime_type(path));
    response->header_count++;
    
    return ERROR_NONE;
}

/**
 * Serve a directory listing
 */
static error_code_t serve_directory(const char* path, http_response_t* response) {
    DIR* dir = opendir(path);
    if (!dir) {
        response->status = HTTP_STATUS_INTERNAL_SERVER_ERROR;
        response->body = strdup("<html><body><h1>500 Internal Server Error</h1></body></html>");
        response->body_length = strlen(response->body);
        return ERROR_INVALID_PARAMETER;
    }
    
    /* Build HTML directory listing */
    char* html = malloc(BUFFER_SIZE * 16);
    if (!html) {
        closedir(dir);
        response->status = HTTP_STATUS_INTERNAL_SERVER_ERROR;
        response->body = strdup("<html><body><h1>500 Internal Server Error</h1></body></html>");
        response->body_length = strlen(response->body);
        return ERROR_MEMORY_ALLOCATION;
    }
    
    int len = sprintf(html, "<html><head><title>Directory Listing</title></head><body><h1>Directory Listing</h1><ul>");
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;  /* Skip hidden files */
        
        char full_path[BUFFER_SIZE];
        snprintf(full_path, BUFFER_SIZE, "%s/%s", path, entry->d_name);
        
        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                len += sprintf(html + len, "<li><a href=\"%s/\">%s/</a></li>", entry->d_name, entry->d_name);
            } else {
                len += sprintf(html + len, "<li><a href=\"%s\">%s</a> (%ld bytes)</li>", 
                              entry->d_name, entry->d_name, (long)st.st_size);
            }
        }
    }
    
    len += sprintf(html + len, "</ul></body></html>");
    closedir(dir);
    
    response->body = html;
    response->body_length = len;
    
    /* Set Content-Type header */
    response->headers[response->header_count][0] = strdup("Content-Type");
    response->headers[response->header_count][1] = strdup("text/html");
    response->header_count++;
    
    return ERROR_NONE;
}

/**
 * Get MIME type based on file extension
 */
static const char* get_mime_type(const char* path) {
    const char* ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    
    ext++;  /* Skip the dot */
    
    if (strcasecmp(ext, "html") == 0 || strcasecmp(ext, "htm") == 0) return "text/html";
    if (strcasecmp(ext, "txt") == 0) return "text/plain";
    if (strcasecmp(ext, "css") == 0) return "text/css";
    if (strcasecmp(ext, "js") == 0) return "application/javascript";
    if (strcasecmp(ext, "json") == 0) return "application/json";
    if (strcasecmp(ext, "xml") == 0) return "application/xml";
    if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0) return "image/jpeg";
    if (strcasecmp(ext, "png") == 0) return "image/png";
    if (strcasecmp(ext, "gif") == 0) return "image/gif";
    if (strcasecmp(ext, "svg") == 0) return "image/svg+xml";
    if (strcasecmp(ext, "ico") == 0) return "image/x-icon";
    if (strcasecmp(ext, "pdf") == 0) return "application/pdf";
    if (strcasecmp(ext, "zip") == 0) return "application/zip";
    
    return "application/octet-stream";
}

/**
 * URL decode
 */
static char* url_decode(const char* str) {
    if (!str) return NULL;
    
    char* decoded = malloc(strlen(str) + 1);
    if (!decoded) return NULL;
    
    char* ptr = decoded;
    
    while (*str) {
        if (*str == '%' && *(str + 1) && *(str + 2)) {
            char hex[3] = {*(str + 1), *(str + 2), 0};
            *ptr++ = (char)strtol(hex, NULL, 16);
            str += 3;
        } else if (*str == '+') {
            *ptr++ = ' ';
            str++;
        } else {
            *ptr++ = *str++;
        }
    }
    
    *ptr = '\0';
    return decoded;
}