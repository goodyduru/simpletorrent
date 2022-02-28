#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <openssl/sha.h>

#include "parser.h"
#include "raw.h"
#include "url.h"
#include "util.h"
#include "tracker.h"

void send_request(){
    struct url *tracker_url = get_url();
    char *peer_id = generate_string(20);
    char *param = generate_param(tracker_url->path, peer_id);
    char *body = get(tracker_url, param);
    free(param);
    if ( body == NULL ) {
        return;
    }
    struct str *complete = get_raw_content(body, 0, "complete", raw_tracker_response_table, TRACKER_RAW_RESPONSE_SIZE);
    free(body);
    echo(complete->data, complete->length);
    parse(raw_tracker_response_table, TRACKER_RAW_RESPONSE_SIZE, tracker_response_table, TRACKER_RESPONSE_SIZE);
    struct parse_item *min_interval = parser_table_lookup("min interval", tracker_response_table, TRACKER_RESPONSE_SIZE);
    out(min_interval);
}

struct url *get_url() {
    struct parse_item *announce = parser_table_lookup("announce", decode_table, TORRENT_TABLE_SIZE);
    char *url_str = announce->head->value->data;
    int url_length = announce->head->value->length;
    return parse_url(url_str, url_length);
}

unsigned char* get_info_hash() {
    struct str *info = raw_table_lookup("info", raw_table, RAW_TABLE_SIZE);
    unsigned char *hash = (unsigned char *) malloc(SHA_DIGEST_LENGTH*sizeof(unsigned char));
    SHA1((unsigned char *)info->data, info->length, hash);
    return hash;
}

long get_left() {
    int size = 0;
    struct parse_item *length_list = parser_table_lookup("length", decode_table, TORRENT_TABLE_SIZE);
    struct decode *length_node = length_list->head;
    while ( length_node != NULL ) {
        size += atoi(length_node->value->data);
        length_node = length_node->next;
    }
    return size;
}

char *get_ip() {
    char *ip;
    struct ifaddrs *if_addr_struct = NULL;
    struct ifaddrs *ifa = NULL;
    void *tmp_addr_ptr = NULL;

    getifaddrs(&if_addr_struct);
    for ( ifa = if_addr_struct; ifa != NULL; ifa = ifa->ifa_next ) {
        if ( !ifa->ifa_addr ) {
            continue;
        }
        if ( ifa->ifa_addr->sa_family == AF_INET && strcmp(ifa->ifa_name, "en0") == 0 ) {
            tmp_addr_ptr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            ip = (char *) malloc(INET_ADDRSTRLEN*sizeof(char));
            inet_ntop(AF_INET, tmp_addr_ptr, ip, INET_ADDRSTRLEN);
            break;
        }
    }

    if ( if_addr_struct != NULL ) free(if_addr_struct);
    return ip;
}

char *generate_param(char *path, char *peer_id) {
    char ch;
    int j;
    char *hash_encode, *peer_encode;
    unsigned char *info_hash = get_info_hash();
    char *port = MY_PORT;
    int uploaded = 0;
    int downloaded = 0;
    long left = get_left();
    int compact = 1;
    char *event = "started";
    char *ip = get_ip();
    int param_length = 0;
    char *param_array[9] = {
        "info_hash=",
        "&peer_id=",
        "&port=",
        "&uploaded=",
        "&downloaded=",
        "&left=",
        "&compact=",
        "&event=",
        "&ip=",
    };
    char *param = (char *)malloc(sizeof(char)*PATH_LENGTH);

    hash_encode = (char *)malloc(sizeof(char)*SHA_DIGEST_LENGTH*3+1);
    hash_encode = urlencode(info_hash, SHA_DIGEST_LENGTH, hash_encode);

    peer_encode = (char *)malloc(sizeof(char)*strlen(peer_id)*3+1);
    peer_encode = urlencode((unsigned char *)peer_id, strlen(peer_id), peer_encode);

    sprintf(param, "%s?info_hash=%s&peer_id=%s&port=%s&uploaded=%d&downloaded=%d&left=%lu&compact=%d&event=%s&ip=%s",
            path, hash_encode, peer_encode, port, uploaded, downloaded, left, compact, event, ip
        );
    return param;
}

char *get(struct url *uri, char *param) {
    struct addrinfo hints, *servinfo, *p;
    int sockfd;
    int rv, total, sent, received, bytes, status;
    char s[INET_ADDRSTRLEN];
    FILE *stream;
    char *message_fmt = "GET %s HTTP/1.1\r\n\r\n";
    char message[BUFSIZ], *response, *length;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(uri->host_name, uri->port, &hints, &servinfo)) != 0 ) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    }

    for ( p = servinfo; p != NULL; p = p->ai_next ) {
        if ( (sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1 ) {
            perror("client: socket");
            continue;
        }

        if ( connect(sockfd, p->ai_addr, p->ai_addrlen) == -1 ) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    } 

    if ( p == NULL ) {
        fprintf(stderr, "client failed to connect");
        return NULL;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo);

    //send request
    stream = fdopen(sockfd, "r+");
    if ( (sent = fprintf(stream, message_fmt, param)) < 0 || (sent = fflush(stream)) < 0 ) {
        perror("Error writing to socket");
        return NULL;
    }

    //reception
    fgets(message, BUFSIZ, stream);
    sscanf(message, "%*s %d %*s", &status);
    if ( status != 200 ) {
        perror("No 200 response");
        return NULL;
    }
    while (1) {
        fgets(message, BUFSIZ, stream);
        //if empty, just stop
        if ( strcmp(message, "") == 0 ) {
            perror("No Content Length was included");
            return NULL;
        }
        length = strstr(message, "Content-Length:");
        if ( length ) {
            sscanf(length, "%*s %d\r\n", &received);
            response = (char *)malloc(sizeof(char)*received);
            continue;
        }
        //read to end of headers
        if ( strcmp(message, "\r\n") == 0 ) {
            break;
        }
    }
    fread(response, 1, received, stream);
    fclose(stream);
    printf("Response:\n%s\n", response);
    return response;
}

void *get_in_addr(struct sockaddr *sa) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
}
