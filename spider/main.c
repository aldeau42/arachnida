#include "spider.h"

////////////////////////////////////////////////////////////////
// OPTIONS //
////////////////////////////////////////////////////////////////

// -recursive option [default limit is 5]
void r_option(Options *opts) {
    opts->recursive = 1;
}

// -limit option
int l_option(int ac, char **av, Options *opts, int i) {
    if (i + 1 < ac && isdigit(av[i + 1][0])) {
        int depth = atoi(av[i + 1]);
        if (depth > 5) {
            fprintf(stderr, "Error: Max depth (-l N) is 5.\n");
            exit(EXIT_FAILURE);
        }
        opts->depth = depth;
        i++;
    } else {
        opts->depth = DEFAULT_DEPTH;
    }
    return i;
}

// -path option
int p_option(int ac, char **av, Options *opts, int i) {
    if (i + 1 < ac) {
        strncpy(opts->path, av[i + 1], sizeof(opts->path) - 1);
        opts->path[sizeof(opts->path) - 1] = '\0';
        i++;
    } else {
        strcpy(opts->path, DEFAULT_PATH);
    }
    return (i);
}

void    get_opt(int ac, char **av, Options *opts) {
    opts->recursive = 0;
    opts->depth = DEFAULT_DEPTH;
    strcpy(opts->path, DEFAULT_PATH);

    for (int i = 1; i < ac; i++) {
        if (strcmp(av[i], "-r") == 0)
            r_option(opts);
        else if (strcmp(av[i], "-l") == 0)
            i = l_option(ac, av, opts, i);
        else if (strcmp(av[i], "-p") == 0)
            i = p_option(ac, av, opts, i);
    }
    //printf("r = %d\nl = %d\np = %s\n", opts->recursive, opts->depth, opts->path);
}


////////////////////////////////////////////////////////////////
// MAIN //
////////////////////////////////////////////////////////////////

int main(int ac, char **av) {
    if (ac < 2 || ac > 7) {
        fprintf(stderr, "Usage: ./spider [-r] [-l N] [-p PATH] <URL>\n"); 
        return 1;
    }
    
    Options opts;
    get_opt(ac, av, &opts);    
    
    char    url[MAX_URL_LEN];
    char    host[100];
    char    path[MAX_URL_LEN];
    strcpy(url, av[ac - 1]);
    get_host(url, host, path);
    init_openssl();
    Image img;

    if (opts.recursive) {
        dl_images_recursive(url, &opts, 0, &img);
    } else {
        int is_https = (strncmp(url, "https://", 8) == 0) ? 1 : 0;
        int port = is_https ? 443 : 80;
        SSL_CTX *ctx = NULL;
        SSL *ssl = NULL;
        int sock = create_socket(host, port, &ctx, &ssl);
        if (sock == -1) return 1;

        send_http_request(host, path, sock, is_https, ssl);
        int response_size;
        char *html = receive_http_response(sock, is_https, ssl, &response_size);
        
        //printf("\n%s\n", html);

        if (!html) {
            fprintf(stderr, "Failed to receive HTTP response\n");
            return 1;
        }

        char img_urls[MAX_IMAGES][MAX_URL_LEN];
        int img_count = extract_images(html, img_urls);
        dl_images(img_urls, img_count, &opts, &img);

        free(html);
    }
    return 0;
}
