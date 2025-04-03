#include "spider.h"

////////////////////////////////////////////////////////////////
// TOOLS //
////////////////////////////////////////////////////////////////

// INITIALISE OPENSSL
void init_openssl() {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
}

// Get hostname 
void get_host(const char *url, char *host, char *path) {
    if (strncmp(url, "http://", 7) == 0)
        url += 7;
    else if (strncmp(url, "https://", 8) == 0)
        url += 8;

    const char *slash = strchr(url, '/');
    if (slash) {
        strncpy(host, url, slash - url);
        host[slash - url] = '\0';
        strcpy(path, slash);
    } else {
        strcpy(host, url);
        strcpy(path, "/");
    }
}

const char *get_file_extension(const char *url) {
    const char *dot = strrchr(url, '.');  // Find last '.'
    if (!dot || strchr(dot, '/'))  // Ensure it's a valid extension
        return "jpg";

    return dot + 1;
}
