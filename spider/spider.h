#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <regex.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h> // For mode_t

#define MAX_BUF         8192
#define MAX_IMAGES      100
#define MAX_URL_LEN     1024
#define DEFAULT_DEPTH   5
#define DEFAULT_PATH    "./data"


typedef struct {
    int recursive;      // -r option
    int depth;          // -l [N] option (default 5)
    char path[256];     // -p [PATH] option (default ./data)
} Options;

typedef struct {
    int nb;
    const char    *ext;
} Image;


// TOOLS
void 	init_openssl();
void 	get_host(const char *url, char *host, char *path);
const char	*get_file_extension(const char *url);

// OPTIONS
void    get_opt(int ac, char **av, Options *opts);
void 	r_option(Options *opts);
int 	l_option(int ac, char **av, Options *opts, int i);
int 	p_option(int ac, char **av, Options *opts, int i);
void 	v_option(Options *opts);

// CONNECTION / HTTP REQUESTS & RESPONSES
int 	create_socket(const char *host, int port, SSL_CTX **ctx, SSL **ssl);
void 	send_http_request(const char *host, const char *path, int sock, int is_https, SSL *ssl);
char 	*receive_http_response(int sock, int is_https, SSL *ssl, int *response_size);

// EXTRACTION
int 	extract_images(const char *html, char img_urls[MAX_IMAGES][MAX_URL_LEN]);
int 	extract_links(const char *html, char links[MAX_IMAGES][MAX_URL_LEN]);
char 	*find_image_data(char *response, int response_size, int *img_size);

// DOWNLOAD
void 	dl_images(const char img_urls[MAX_IMAGES][MAX_URL_LEN], int img_count, Options *opts, Image *img);
void 	dl_images_recursive(const char *url, Options *opts, int current_depth, Image *img);
