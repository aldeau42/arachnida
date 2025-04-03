#include "spider.h"


////////////////////////////////////////////////////////////////
// CONNECTION / HTTP REQUESTS & RESPONSES //
////////////////////////////////////////////////////////////////

int create_socket(const char *host, int port, SSL_CTX **ctx, SSL **ssl) {
    struct sockaddr_in server_addr;
    struct hostent *server = gethostbyname(host);
    if (!server) {
        fprintf(stderr, "Error: No such host\n");
        return -1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket error\n");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    printf("\nHost: %s\nIP Address: %s:%d\n", host, inet_ntoa(server_addr.sin_addr), server_addr.sin_port);
    
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed\n");
        return -1;
    }

    // If HTTPS, set up SSL connection
    if (port == 443) {
        *ctx = SSL_CTX_new(TLS_client_method());
        if (!*ctx) {
            fprintf(stderr, "SSL_CTX_new failed\n");
            return -1;
        }

        *ssl = SSL_new(*ctx);
        SSL_set_fd(*ssl, sock);

        if (SSL_connect(*ssl) != 1) {
            fprintf(stderr, "SSL connection failed\n");
            return -1;
        }
        printf("SSL connection success\n");
    }
    return sock;
}


void send_http_request(const char *host, const char *path, int sock, int is_https, SSL *ssl) {
    char request[MAX_URL_LEN + 100];
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: Mozilla/5.0\r\n"
             "Connection: close\r\n\r\n", 
             path, host);

    if (is_https) {
        SSL_write(ssl, request, strlen(request));
    } else {
        send(sock, request, strlen(request), 0);
    }
}


char *receive_http_response(int sock, int is_https, SSL *ssl, int *response_size) {
    int buffer_size = MAX_BUF;
    char *response = malloc(buffer_size);
    if (!response) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    int total_size = 0, bytes;
    while (1) {
        bytes = is_https ? SSL_read(ssl, response + total_size, MAX_BUF) 
                         : recv(sock, response + total_size, MAX_BUF, 0);
        if (bytes <= 0)
            break;

        total_size += bytes;

        // Ensure we have enough space
        if (total_size + MAX_BUF > buffer_size) {
            buffer_size *= 2;
            char *temp = realloc(response, buffer_size);
            if (!temp) {
                fprintf(stderr, "Memory reallocation failed\n");
                free(response);
                return NULL;
            }
            response = temp;
        }
    }

    close(sock);
    *response_size = total_size;
    return response;
}


////////////////////////////////////////////////////////////////
// EXTRACT URLs //
////////////////////////////////////////////////////////////////

int extract_images(const char *html, char img_urls[MAX_IMAGES][MAX_URL_LEN]) {
    if (!html || html[0] == '\0') {
        fprintf(stderr, "Invalid HTML input\n");
        return -1;
    }

    // Parse HTML
    htmlDocPtr doc = htmlReadMemory(html, strlen(html), NULL, NULL, HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if (!doc) {
        xmlFreeDoc(doc);
        fprintf(stderr, "Failed to parse HTML\n");
        return -1;
    }

    // Create XPath context
    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    if (!xpathCtx) {
        fprintf(stderr, "Failed to create XPath context\n");
        xmlFreeDoc(doc);
        return -1;
    }

    const char *xpaths[] = {
        "//img/@src",
        "//source/@srcset",
        "//link[@rel='preload' or @rel='stylesheet']/@imagesrcset",
        "//meta[@property='og:image']/@content"
    };

    printf("#### Image URLs ####\n");
    int count = 0;

    for (int i = 0; i < 4 && count < MAX_IMAGES; i++) {
        xmlXPathObjectPtr result = xmlXPathEvalExpression((xmlChar *)xpaths[i], xpathCtx);
        if (!result) 
            continue;

        xmlNodeSetPtr nodes = result->nodesetval;
        if (nodes) {
            for (int j = 0; j < nodes->nodeNr && count < MAX_IMAGES; j++) {
                xmlNodePtr node = nodes->nodeTab[j];
                if (node->children && node->children->content) {
                    char *raw_url = (char *)node->children->content;

                    // Ensure the URL is copied before modification
                    char url[MAX_URL_LEN];
                    strncpy(url, raw_url, MAX_URL_LEN - 1);
                    url[MAX_URL_LEN - 1] = '\0';

                    // Remove leading spaces
                    char *clean_url = url;
                    while (*clean_url == ' ') clean_url++;

                    // Store the final cleaned URL
                    strncpy(img_urls[count], clean_url, MAX_URL_LEN - 1);
                    img_urls[count][MAX_URL_LEN - 1] = '\0';

                    printf("%s\n", img_urls[count]);
                    count++;
                }
            }
        }

        xmlXPathFreeObject(result); // Free XPath result to avoid memory leaks
    }

    xmlXPathFreeContext(xpathCtx); // Free XPath context
    xmlFreeDoc(doc); // Free parsed HTML document

    printf("\nTotal images found: %d\n", count);
    return count;
}

int extract_links(const char *html, char links[MAX_IMAGES][MAX_URL_LEN]) {

    htmlDocPtr doc = htmlReadMemory(html, strlen(html), NULL, NULL, HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if (!doc) 
        return 0;

    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    if (!xpathCtx) {
        xmlFreeDoc(doc);
        return 0;
    }

    xmlXPathObjectPtr result = xmlXPathEvalExpression((xmlChar *)"//a/@href", xpathCtx);
    if (!result || !result->nodesetval) {
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return 0;
    }

    printf("#### href URLs ####\n");
    int count = 0;
    for (int i = 0; i < result->nodesetval->nodeNr && count < MAX_IMAGES; i++) {
        xmlNodePtr node = result->nodesetval->nodeTab[i];
        if (node->children && node->children->content) {
            char *url = (char *)node->children->content;
            strncpy(links[count], url, MAX_URL_LEN - 1);
            links[count][MAX_URL_LEN - 1] = '\0';
            printf("%s\n", links[count]);
            count++;
        }
    }

    xmlXPathFreeObject(result);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);
    
    return count;
}


////////////////////////////////////////////////////////////////
// IMAGES //
////////////////////////////////////////////////////////////////

char *find_image_data(char *response, int response_size, int *img_size) {
    char *header_end = strstr(response, "\r\n\r\n");
    if (!header_end) {
        fprintf(stderr, "Invalid HTTP response (no header found)\n");
        return NULL;
    }
    header_end += 4;  // Move past "\r\n\r\n"

    *img_size = response_size - (header_end - response);
    return (*img_size > 0) ? header_end : NULL;
}


void dl_images(const char img_urls[MAX_IMAGES][MAX_URL_LEN], int img_count, Options *opts, Image *img) {
    
    struct stat st;
    if (stat(opts->path, &st) == -1)
        mkdir(opts->path, 0777);
    
    for (int i = 0; i < img_count; i++) {
        char host[100], path[MAX_URL_LEN];
        get_host(img_urls[i], host, path);

        int is_https = (strncmp(img_urls[i], "https://", 8) == 0);
        int port = is_https ? 443 : 80;
        img->ext = get_file_extension(img_urls[i]);
        
        SSL_CTX *ctx = NULL;
        SSL *ssl = NULL;
        int sock = create_socket(host, port, &ctx, &ssl);
        if (sock == -1) 
            continue;

        send_http_request(host, path, sock, is_https, ssl);

        int response_size;
        char *response = receive_http_response(sock, is_https, ssl, &response_size);
        if (!response) {
            free(response);
            continue;
        }

        printf("%s\n", img_urls[i]);
        int img_size;
        char *img_data = find_image_data(response, response_size, &img_size);
        if (!img_data) {
            free(response);
            continue;
        }

        char filename[512];
        snprintf(filename, sizeof(filename), "%s/image_%d.%s", opts->path, img->nb++, img->ext);

        FILE *file = fopen(filename, "wb");
        if (!file) {
            perror("Error opening file");
            free(response);
            continue;
        }

        fwrite(img_data, 1, img_size, file);
        fclose(file);

        printf("Image saved as: %s (%d bytes)\n", filename, img_size);
        free(response);
    }
}

void dl_images_recursive(const char *url, Options *opts, int current_depth, Image *img) {
    
    if (current_depth > opts->depth)
        return;  

    printf("\n\n**************************************************************************************\n");
    printf("%s --- Current depth: %d/%d\n", url, current_depth, opts->depth);
    printf("**************************************************************************************\n");

    char host[100], path[MAX_URL_LEN];
    get_host(url, host, path);

    int is_https = (strncmp(url, "https://", 8) == 0);
    int port = is_https ? 443 : 80;

    SSL_CTX *ctx = NULL;
    SSL *ssl = NULL;
    int sock = create_socket(host, port, &ctx, &ssl);
    if (sock == -1) 
        return;

    send_http_request(host, path, sock, is_https, ssl);

    int response_size;
    char *html = receive_http_response(sock, is_https, ssl, &response_size);
    if (!html) {
        free(html);
        fprintf(stderr, "Failed to receive HTTP response\n");
        return;
    }

    char img_urls[MAX_IMAGES][MAX_URL_LEN];
    int img_count = extract_images(html, img_urls);

    dl_images(img_urls, img_count, opts, img);

    if (opts->recursive) {
        char links[MAX_IMAGES][MAX_URL_LEN];
        int link_count = extract_links(html, links);

        for (int i = 0; i < link_count; i++) {
            dl_images_recursive(links[i], opts, current_depth + 1, img);
        }
    }

    free(html);
}
