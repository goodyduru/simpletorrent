#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
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

void send_tracker_request(){
    struct url *tracker_url;
    struct url **tracker_urls = get_urls();
    int tracker_size = number_of_trackers();
    peer_id = generate_string(PEER_ID_LENGTH);
    for ( int i = 0; i < tracker_size; i++ ) {
        tracker_url = tracker_urls[i];
        if ( strcmp("http", tracker_url->scheme) == 0 ) {
            char *param = generate_param(tracker_url->path, peer_id);
            char *body = get(tracker_url, param);
            free(param);
            if ( body == NULL ) {
                continue;
            }
            struct str *complete = get_raw_content(body, 0, "complete", raw_tracker_response_table, TRACKER_RAW_RESPONSE_SIZE);
            free(complete->data);
            free(complete);
            parse(raw_tracker_response_table, TRACKER_RAW_RESPONSE_SIZE, tracker_response_table, TRACKER_RESPONSE_SIZE);
        }
        else if ( strcmp("udp", tracker_url->scheme) == 0 ) {
            add_udp_peers(tracker_url, peer_id);
        }
        free_url(tracker_url);
    }
    free(tracker_urls);
}

void free_url(struct url *uri) {
    free(uri->host_name);
    free(uri->scheme);
    free(uri->port);
    free(uri->path);
    free(uri);
}

struct url **get_urls() {
    struct url **urls;
    char *url_str;
    int url_length;
    struct parse_item *announce_list = parser_table_lookup("announce-list", decode_table, TORRENT_TABLE_SIZE);
    if ( announce_list == NULL || announce_list->count == 0 ) {
        urls = (struct url **) malloc(sizeof(struct url *));
        struct parse_item *announce = parser_table_lookup("announce", decode_table, TORRENT_TABLE_SIZE);
        url_str = announce->head->value->data;
        url_length = announce->head->value->length;
        urls[0] = parse_url(url_str, url_length);
        return urls;
    }
    int count = announce_list->count;
    urls = (struct url **) malloc(sizeof(struct url *)*count);
    struct decode *current_url = announce_list->head;
    for ( int i = 0; i < count; i++ ) {
        url_str = current_url->value->data;
        url_length = current_url->value->length;
        urls[i] = parse_url(url_str, url_length);
        current_url = current_url->next;
    }
    return urls;
}

int number_of_trackers() {
    struct parse_item *announce_list = parser_table_lookup("announce-list", decode_table, TORRENT_TABLE_SIZE);
    if ( announce_list == NULL || announce_list->count == 0 ) {
        return 1;
    }
    else {
        return announce_list->count;
    }
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
    long left = get_torrent_file_size();
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
    free(info_hash);
    free(hash_encode);
    free(peer_encode);
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
        fprintf(stderr, "client failed to connect\n");
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

void add_udp_peers(struct url *uri, char *peer_id) {
    int sockfd = get_socket(uri);
    printf("%s\n", uri->host_name);
    if ( sockfd == -1 ) {
        return;
    }
    struct udp_response *response = NULL;
    int transaction_id = get_rand();

    //Connect message
    int i = 0;
    char *connect_message = generate_connect_message(transaction_id);
    while ( i < 4 && response == NULL ) {
        sleep(i);
        response = send_udp_message(sockfd, connect_message, 16);
        i++;
    }
    free(connect_message);
    if ( response == NULL  ) {
        printf("Cannot connect to tracker\n");
        close(sockfd);
        return;
    }
    long connection_id = parse_connect_message(response->response);
    printf("Connection ID: %ld\n", connection_id);
    free(response->response);
    free(response);
    response = NULL;

    //Announce message
    i = 0;
    char *announce_message = generate_announce_message(transaction_id, connection_id);
    while ( i < 4 && response == NULL ) {
        sleep(i);
        response = send_udp_message(sockfd, announce_message, 98);
        i++;
    }
    free(announce_message);
    if ( response == NULL ) {
        printf("Cannot announce to tracker\n");
        close(sockfd);
        return;
    }
    parse_announce_message(response->response);
    free(response->response);
    free(response);
    close(sockfd);
}

int get_socket(struct url *uri) {
    int sockfd, rv, enable;
    struct addrinfo hints, *servinfo, *p;
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    char s[INET_ADDRSTRLEN];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(uri->host_name, uri->port, &hints, &servinfo)) != 0 ){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    for ( p = servinfo; p != NULL; p = p->ai_next ) {
        if ( (sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1 ) {
            perror("tracker: socket");
            continue;
        } 

        if ( connect(sockfd, p->ai_addr, p->ai_addrlen) == -1 ) {
            close(sockfd);
            perror("tracker: connect");
            continue;
        }

        break;
    }

    if ( p == NULL ) {
        fprintf(stderr, "client failed to connect to tracker");
        return -1;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("tracker: connecting to %s\n", s);

    freeaddrinfo(servinfo);
    enable = 1;
    if ( setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout)) < 0 ){
        perror("Set Timeout Error");
    }
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    return sockfd;
}

char *generate_connect_message(int transaction_id) {
    char *message = malloc(16);
    long protocol = htonll(0x41727101980);
    int action = htonl(0);
    transaction_id = htonl(transaction_id);
    memcpy(message, &protocol, 8);
    memcpy(message+8, &action, 4);
    memcpy(message+12, &transaction_id, 4);
    return message;
}

long parse_connect_message(char *response) {
    long connection_id  = 0;
    memcpy(&connection_id, response+8, 8);
    return ntohll(connection_id);
}

char *generate_announce_message(int transaction_id, long connection_id) {
    char *message = malloc(98);
    connection_id = htonll(connection_id);
    int action = htonl(1);
    transaction_id = htonl(transaction_id);
    unsigned char *info_hash = get_info_hash();
    long downloaded = htonll(0);
    long left = htonll(get_torrent_file_size());
    long uploaded = htonll(0);
    int event = htonl(0); //started
    int ip = htonl(0);
    int key = htonl(0);
    int num_want = 30;
    short int port = htons(atoi(MY_PORT));

    memcpy(message, &connection_id, 8);
    memcpy(message+8, &action, 4);
    memcpy(message+12, &transaction_id, 4);
    memcpy(message+16, info_hash, SHA_DIGEST_LENGTH);
    memcpy(message+36, peer_id, PEER_ID_LENGTH);
    memcpy(message+56, &downloaded, 8);
    memcpy(message+64, &left, 8);
    memcpy(message+72, &uploaded, 8);
    memcpy(message+80, &event, 4);
    memcpy(message+84, &ip, 4);
    memcpy(message+88, &key, 4);
    memcpy(message+92, &num_want, 4);
    memcpy(message+96, &port, 2);
    return message;
}

void parse_announce_message(char *response) {
    struct str *peer;
    char *item;
    int leechers = 0;
    int seeders = 0;
    int peers = 0;
    char echo[6];
    memcpy(&leechers, response+12, 4);
    memcpy(&seeders, response+16, 4);
    peers = ntohl(leechers) + ntohl(seeders);
    printf("Total: %d\n", peers);
    for ( int i = 0; i < peers; i++ ) {
        item = malloc(7);
        peer = (struct str *)malloc(sizeof(sizeof(struct str)));
        memcpy(item, response+20+(i*6), 6);
        item[6] = '\0';
        peer->data = item;
        peer->length = 6;
        char* p[2];
        gen_ip_and_port(item, p);
        parser_table_set("peers", peer, tracker_response_table, TRACKER_RESPONSE_SIZE);
    }
}

struct udp_response *send_udp_message(int sockfd, char *message, int message_length) {
    int action = 0, transaction_id = 0, bytes, sent;
    struct udp_response *response;
    memcpy(&action, message+8, 4);
    memcpy(&transaction_id, message+12, 4);
    action = ntohl(action);
    transaction_id = ntohl(transaction_id);
    sent = 0;
    do {
        bytes = write(sockfd, message+sent, message_length-sent);
        if ( bytes < 0 ) {
            perror("Failed to connect to tracker");
            return NULL;
        }
        if ( bytes == 0 ) {
            break;
        }
        sent += bytes;
    } while ( sent < message_length );
    response = receive_udp_message(sockfd, action, transaction_id, message_length);
    return response;
}

struct udp_response *receive_udp_message(int sockfd, int action, int transaction_id, int message_length) {
    int bytes, trans_id, act;
    int size = 200;
    char *response = malloc(size);
    while (1) {
        bytes = recv(sockfd, response, size, 0);
        printf("Receive Bytes: %d\n", bytes);
        if ( bytes == 0 ) {
            break;
        }
        else if ( bytes < 0 ) {
            perror("Failed to connect to tracker");
            free(response);
            return NULL;
        }
        else {
            break;
        }
    }
    if ( bytes < 20 && message_length > 20 ) {
        perror("Invalid response");
        free(response);
        return NULL;
    }
    memcpy(&act, response, 4);
    memcpy(&trans_id, response+4, 4);
    if ( action != ntohl(act) || transaction_id != ntohl(trans_id) ) {
        perror("Invalid response");
        free(response);
        return NULL;
    }
    struct udp_response *resp = ( struct udp_response *) malloc(sizeof(struct udp_response));
    resp->response = response;
    resp->len = bytes;
    return resp;
}