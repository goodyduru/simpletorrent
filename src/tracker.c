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
    char *result = get(tracker_url, param);
}

struct url *get_url() {
    struct parse_item *announce = parser_table_lookup("announce");
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
    struct parse_item *pieces = parser_table_lookup("pieces");
    struct parse_item *piece_size = parser_table_lookup("piece length");
    int total_piece_length = pieces->head->value->length;
    int num_pieces = total_piece_length / HASHED_PIECE_LENGTH;
    return num_pieces * atoi(piece_size->head->value->data);
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
    char *para, *item, *param_encode;
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
    //copy path
    while ( (ch = *path++) != '\0' ) {
        param[param_length++] = ch;
    }
    param[param_length++] = '?';
    for ( int i = 0; i < 9; i++ ) {
        para = param_array[i];
        while ( (ch = *para++) != '\0' ) {
            param[param_length++] = ch;
        }
        //Copy sha1 hash
        if ( i == 0 ) {
            param_encode = (char *)malloc(sizeof(char)*SHA_DIGEST_LENGTH*3+1);
            item = urlencode(info_hash, SHA_DIGEST_LENGTH, param_encode);
            copy_param(param, &param_length, item, strlen(item));
            free(param_encode);
            param_encode = NULL;
        }
        else if ( i == 1 ) {
            param_encode = (char *)malloc(sizeof(char)*strlen(peer_id)*3+1);
            item = urlencode((unsigned char *)peer_id, strlen(peer_id), param_encode);
            copy_param(param, &param_length, item, strlen(item));
            free(param_encode);
            param_encode = NULL;
        }
        else if ( i == 2 ) {
            copy_param(param, &param_length, port, strlen(port));
        }
        else if ( i == 3 ) {
            item = itoa(uploaded);
            copy_param(param, &param_length, item, strlen(item));
            free(item);
            item = NULL;
        }
        else if ( i == 4 ) {
            item = itoa(downloaded);
            copy_param(param, &param_length, item, strlen(item));
            free(item);
            item = NULL;
        }
        else if ( i == 5 ) {
            item = itoa(left);
            copy_param(param, &param_length, item, strlen(item));
            free(item);
            item = NULL;
        }
        else if ( i == 6 ) {
            item = itoa(compact);
            copy_param(param, &param_length, item, strlen(item));
            free(item);
            item = NULL;
        }
        else if ( i == 7 ) {
            copy_param(param, &param_length, event, strlen(event));
        }
        else if ( i == 8 ) {
            copy_param(param, &param_length, ip, strlen(ip));
        }
    }
    return param;
}

void copy_param(char *param, int *param_length, char *src, int src_length) {
    int j = 0;
    int len = *param_length;
    while ( j < src_length ) {
        param[len++] = src[j++];
    }
    param[len] = '\0';
    *param_length = len;
}

char *itoa(int number) {
    char *figures;
    if ( number == 0 ) {
        figures = malloc(2*sizeof(char));
        figures[0] = '0';
        figures[1] = '\0';
        return figures;
    }
    char temp;
    figures = malloc(255 * sizeof(char));
    int i = 0, j = 0, k = 0;
    while ( (number / 10) > 0 ) {
        figures[i++] = (number % 10) + '0';
        number /= 10;
    }
    if ( number > 0 ) {
        figures[i++] = (number % 10) + '0';
    }
    figures[i] = '\0';
    //Reverse string
    k = strlen(figures) - 1;
    while ( j < k ) {
        temp = figures[j];
        figures[j] = figures[k];
        figures[k] = figures[j];
        j++;
        k--;
    }
    figures[i] = '\0';
    return figures;
}

char *get(struct url *uri, char *param) {
    struct addrinfo hints, *servinfo, *p;
    int sockfd;
    int rv, total, sent, received, bytes;
    char s[INET_ADDRSTRLEN];
    char *message_fmt = "GET %s HTTP/1.1\r\n\r\n";
    char message[1024], *response;
    response = (char *) malloc(4096 * sizeof(char));

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

    sprintf(message, message_fmt, param);
    total = strlen(message);
    sent = 0;
    do {
        bytes = write(sockfd, message+sent, total-sent);
        if ( bytes < 0 ) {
            perror("Error writing to socket\n");
            return NULL;
        }
        if ( bytes == 0 ) {
            break;
        }
        sent += bytes;
    } while ( sent < total );

    memset(response, 0, 4096);
    total = 4095;
    received = 0;

    do {
        bytes = read(sockfd, response+received, total-received);
        if ( bytes < 0 ) {
            perror("Error reading from socket");
            return NULL;
        }
        if ( bytes == 0 ) {
            break;
        }
        received += bytes;
    } while ( received < total );

    if ( received == total ) {
        perror("Error storing complete response from socket");
        return NULL;
    }
    close(sockfd);
    printf("Response:\n%s\n", response);
    return response;
}

void *get_in_addr(struct sockaddr *sa) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
}