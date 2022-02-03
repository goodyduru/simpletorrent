#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "url.h"

struct url *parse_url(char *announce, int uri_length) {
    int i, j, k, seen_slash, seen_colon;
    char prev, current;
    char host_name[HOST_NAME_LENGTH];
    char path[PATH_LENGTH];
    char port[PORT_LENGTH];
    seen_slash = 0, seen_colon = 0, j = 0; k = 0;
    struct url *result = (struct url *)malloc(sizeof(struct url));
    for ( i = 0; i < uri_length; i++ ) {
        current = announce[i];
        // We want to strip protocol info from the url
        if ( seen_slash == 0 && current == '/' && prev == '/' ) {
            j = 0;
        }
        // This should signify the switch from host address to  path
        else if ( seen_slash == 0 && current == '/' && prev != ':') {
            host_name[j+1] = '\0';
            j = 0;
            path[j++] = current;
            seen_slash = 1;
        }
        // Check if colon isn't followed by slash, initialise seen_slash
        else if ( seen_slash == 0 && current == ':' &&  seen_colon == 0 && announce[i+1] != '/' ) {
            seen_colon = 1;
        }
        // Check if this is port number
        else if ( seen_slash == 0 && seen_colon == 1 && isdigit(current) ) {
            port[k++] = current;
        }
        // Add to hostname
        else if ( seen_slash == 0 ) {
            host_name[j++] = current;
        }
        // Add to path
        else {
            path[j++] = current;
        }
        prev = current;
    }
    path[j] = '\0';
    port[k++] = '\0';
    result->host_name = strdup(host_name);
    result->path = strdup(path);
    result->port = (k == 1) ? "6969" : strdup(port);
    return result;
}

void urlencode_table_init() {
    int i;
    for ( i = 0; i < 256; i++ ) {
        html5[i] = isalnum(i) || i == '*' || i == '-' || i == '.' || i == '_' ? i : ( i == ' ') ? '+' : 0;
    }
}

char* urlencode(char* url_string, int text_len) {
    // allocate memory for the worst possible case (all characters need to be encoded)
    char *encoded_url = (char *)malloc(sizeof(char)*text_len*3+1);
    char *begin = encoded_url;
    if ( !html5['a' - 0] ) {
        urlencode_table_init();
    }
    for ( int i = 0; i < text_len; i++ ) {
        if ( html5[url_string[i]] ) *begin = html5[url_string[i]];
        else sprintf(begin, "%%%02X", url_string[i]);
        while (*++begin);
    }
    return encoded_url;
}
