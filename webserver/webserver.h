#ifndef NEXOS_WEBSERVER_H
#define NEXOS_WEBSERVER_H

#include "../kernel/kernel.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <signal.h>

/* HTTP request methods */
typedef enum {
    HTTP_METHOD_GET,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_OPTIONS,
    HTTP_METHOD_UNKNOWN
} http_method_t;

/* HTTP status codes */
typedef enum {
    HTTP_STATUS_OK = 200,
    HTTP_STATUS_CREATED = 201,
    HTTP_STATUS_ACCEPTED = 202,
    HTTP_STATUS_NO_CONTENT = 204,
    HTTP_STATUS_MOVED_PERMANENTLY = 301,
    HTTP_STATUS_FOUND = 302,
    HTTP_STATUS_NOT_MODIFIED = 304,
    HTTP_STATUS_BAD_REQUEST = 400,
    HTTP_STATUS_UNAUTHORIZED = 401,
    HTTP_STATUS_FORBIDDEN = 403,
    HTTP_STATUS_NOT_FOUND = 404,
    HTTP_STATUS_METHOD_NOT_ALLOWED = 405,
    HTTP_STATUS_INTERNAL_SERVER_ERROR = 500,
    HTTP_STATUS_NOT_IMPLEMENTED = 501,
    HTTP_STATUS_BAD_GATEWAY = 502,
    HTTP_STATUS_SERVICE_UNAVAILABLE = 503
} http_status_t;

/* HTTP request */
typedef struct {
    http_method_t method;
    char* path;
    char* version;
    char* headers[100][2];
    int header_count;
    char* body;
    size_t body_length;
    int keep_alive;
} http_request_t;

/* HTTP response */
typedef struct {
    http_status_t status;
    char* headers[100][2];
    int header_count;
    char* body;
    size_t body_length;
} http_response_t;

/* Web server configuration */
typedef struct {
    uint16_t port;
    char* webroot;
    uint32_t max_connections;
    uint32_t timeout;
} webserver_config_t;

/* Initialize web server */
error_code_t webserver_init(webserver_config_t* config);

/* Start web server */
error_code_t webserver_start(void);

/* Stop web server */
error_code_t webserver_stop(void);

/* Get web server statistics */
error_code_t webserver_get_stats(void* stats);

/* Self-optimization interface */
error_code_t webserver_optimize(void);

#endif /* NEXOS_WEBSERVER_H */