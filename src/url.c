#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "url.h"

#define PATH_LENGTH 1831
#define HOST_NAME_LENGTH 254
#define BT_PORT 6969

struct url *parse_url(char *announce, int uri_length) {
    int i, j, seen_slash, seen_colon, port_length;
    char prev, current;
    char host_name[HOST_NAME_LENGTH];
    char path[PATH_LENGTH];
    int port = 0;
    port_length = 0, seen_slash = 0, seen_colon = 0;
    j = 0;
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
            port_length++;
            port = port*10 + (current - '0');
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
    port = (port == 0) ? BT_PORT : port;
    result->host_name = strdup(host_name);
    result->path = strdup(path);
    result->port = port;
    return result;
}

char* urlencode(char* url_string, int text_len) {
    // allocate memory for the worst possible case (all characters need to be encoded)
    char *encoded_url = (char *)malloc(sizeof(char)*text_len*3+1);
    
    const char *hex = "0123456789abcdef";
    
    int pos = 0;
    char t;
    for (int i = 0; i < text_len; i++) {
        t = url_string[i];
        if (isalnum(t) || t == '*'|| t == '-'||t == '.'||t == '_' ) {
            encoded_url[pos++] = t;
        }
        else if ( t == ' ') {
            encoded_url[pos++] = '+';
        } 
        else {
            encoded_url[pos++] = '%';
            encoded_url[pos++] = hex[t >> 4];
            encoded_url[pos++] = hex[t & 15];
        }
    }
    encoded_url[pos] = '\0';
    return encoded_url;
}
