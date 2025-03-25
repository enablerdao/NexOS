#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <ctype.h>

#define BUFFER_SIZE 4096
#define SERVER_NAME "NexOS WebServer/1.0"
#define DEFAULT_PORT 8080
#define DEFAULT_WEBROOT "./webroot"

/* Global variables */
volatile sig_atomic_t running = 1;
char* webroot = DEFAULT_WEBROOT;

/* Function prototypes */
void handle_signal(int sig);
void setup_signal_handlers();
int create_server_socket(int port);
void handle_client(int client_fd);
void serve_file(int client_fd, const char* path);
void serve_directory(int client_fd, const char* path);
const char* get_mime_type(const char* path);
char* url_decode(const char* str);

/**
 * Signal handler
 */
void handle_signal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\nShutting down server...\n");
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
 * Create and setup server socket
 */
int create_server_socket(int port) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    /* Create socket */
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    /* Set socket options */
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    /* Setup address structure */
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    /* Bind socket to address */
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    /* Listen for connections */
    if (listen(server_fd, SOMAXCONN) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    return server_fd;
}

/**
 * Handle client connection
 */
void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    /* Read request */
    bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read <= 0) {
        close(client_fd);
        return;
    }
    
    buffer[bytes_read] = '\0';
    
    /* Parse request line */
    char* method = strtok(buffer, " ");
    if (!method) {
        close(client_fd);
        return;
    }
    
    char* path = strtok(NULL, " ");
    if (!path) {
        close(client_fd);
        return;
    }
    
    /* Decode URL */
    char* decoded_path = url_decode(path);
    if (!decoded_path) {
        close(client_fd);
        return;
    }
    
    /* Construct full path */
    char full_path[BUFFER_SIZE];
    snprintf(full_path, BUFFER_SIZE, "%s%s", webroot, decoded_path);
    
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
            if (decoded_path[strlen(decoded_path) - 1] != '/') {
                char response[BUFFER_SIZE];
                char location[BUFFER_SIZE];
                snprintf(location, BUFFER_SIZE, "%s/", decoded_path);
                
                snprintf(response, BUFFER_SIZE,
                    "HTTP/1.1 301 Moved Permanently\r\n"
                    "Location: %s\r\n"
                    "Content-Length: 0\r\n"
                    "Connection: close\r\n"
                    "\r\n",
                    location);
                
                write(client_fd, response, strlen(response));
            } else {
                /* Serve directory listing */
                serve_directory(client_fd, full_path);
            }
        } else {
            /* Serve file */
            serve_file(client_fd, full_path);
        }
    } else {
        /* File not found */
        char response[BUFFER_SIZE];
        snprintf(response, BUFFER_SIZE,
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 95\r\n"
            "Connection: close\r\n"
            "\r\n"
            "<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1></body></html>");
        
        write(client_fd, response, strlen(response));
    }
    
    free(decoded_path);
    close(client_fd);
}

/**
 * Serve a file
 */
void serve_file(int client_fd, const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        char response[BUFFER_SIZE];
        snprintf(response, BUFFER_SIZE,
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 95\r\n"
            "Connection: close\r\n"
            "\r\n"
            "<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1></body></html>");
        
        write(client_fd, response, strlen(response));
        return;
    }
    
    /* Get file size */
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    /* Send headers */
    char headers[BUFFER_SIZE];
    snprintf(headers, BUFFER_SIZE,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "Server: %s\r\n"
        "\r\n",
        get_mime_type(path), file_size, SERVER_NAME);
    
    write(client_fd, headers, strlen(headers));
    
    /* Send file content */
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        write(client_fd, buffer, bytes_read);
    }
    
    fclose(file);
}

/**
 * Serve a directory listing
 */
void serve_directory(int client_fd, const char* path) {
    DIR* dir = opendir(path);
    if (!dir) {
        char response[BUFFER_SIZE];
        snprintf(response, BUFFER_SIZE,
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 115\r\n"
            "Connection: close\r\n"
            "\r\n"
            "<html><head><title>500 Internal Server Error</title></head><body><h1>500 Internal Server Error</h1></body></html>");
        
        write(client_fd, response, strlen(response));
        return;
    }
    
    /* Build HTML directory listing */
    char* html = malloc(BUFFER_SIZE * 16);
    if (!html) {
        closedir(dir);
        char response[BUFFER_SIZE];
        snprintf(response, BUFFER_SIZE,
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 115\r\n"
            "Connection: close\r\n"
            "\r\n"
            "<html><head><title>500 Internal Server Error</title></head><body><h1>500 Internal Server Error</h1></body></html>");
        
        write(client_fd, response, strlen(response));
        return;
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
    
    /* Send headers */
    char headers[BUFFER_SIZE];
    snprintf(headers, BUFFER_SIZE,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "Server: %s\r\n"
        "\r\n",
        len, SERVER_NAME);
    
    write(client_fd, headers, strlen(headers));
    
    /* Send HTML content */
    write(client_fd, html, len);
    
    free(html);
}

/**
 * Get MIME type based on file extension
 */
const char* get_mime_type(const char* path) {
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
char* url_decode(const char* str) {
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

/**
 * Main function
 */
int main(int argc, char* argv[]) {
    int port = DEFAULT_PORT;
    
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
            printf("  -p, --port PORT    Port to listen on (default: %d)\n", DEFAULT_PORT);
            printf("  -r, --root DIR     Web root directory (default: %s)\n", DEFAULT_WEBROOT);
            printf("  -h, --help         Show this help message\n");
            return 0;
        }
    }
    
    /* Setup signal handlers */
    setup_signal_handlers();
    
    /* Create webroot directory if it doesn't exist */
    struct stat st = {0};
    if (stat(webroot, &st) == -1) {
        mkdir(webroot, 0755);
    }
    
    /* Create server socket */
    int server_fd = create_server_socket(port);
    
    printf("\nNexOS Web Server\n");
    printf("===============\n\n");
    printf("Server started on port %d\n", port);
    printf("Web root: %s\n", webroot);
    printf("Press Ctrl+C to stop\n\n");
    
    /* Main server loop */
    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd;
        
        /* Accept connection */
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (running) {
                perror("Accept failed");
            }
            continue;
        }
        
        /* Handle client */
        handle_client(client_fd);
    }
    
    /* Close server socket */
    close(server_fd);
    
    printf("\nServer stopped\n");
    return 0;
}