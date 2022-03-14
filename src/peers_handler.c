#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#include "parser.h"
#include "peers_handler.h"

void parse_peer() {
    struct parse_item *peer_item = parser_table_lookup("peers", tracker_response_table, TRACKER_RESPONSE_SIZE);
    if ( peer_item == NULL ) {
        return;
    }
    struct str *peers = peer_item->head->value;
    int array_length = peers->length/NETWORK_LENGTH;
    char *peers_array[array_length][2];
    fill_peer(peers_array, peers);
    int i = 0;
    while ( i < array_length ) {
        printf("%s: %s\n", peers_array[i][0], peers_array[i][1]);
        i++;
    }
}

void fill_peer(char *peers[][2], struct str *peers_str) {
    int index = 0, j = 0;
    int str_length = peers_str->length;
    char *item = (char *) malloc(sizeof(char)*NETWORK_LENGTH);
    while ( index < str_length ) {
        memcpy(item, peers_str->data+index, NETWORK_LENGTH);
        char *net[2];
        gen_ip_and_port(item, peers[j++]);
        index += NETWORK_LENGTH;
    }
    free(item);
}

void gen_ip_and_port(char *item, char *peer[]) {
    char *port, *ip;
    ip = (char *) malloc(sizeof(char)*INET_ADDRSTRLEN);
    port = (char *) malloc(sizeof(char)* (MAX_PORT_LENGTH+1));
    sprintf(ip, "%d.%d.%d.%d", abs(item[0]), abs(item[1]), abs(item[2]), abs(item[3]));
    sprintf(port, "%d%d", abs(item[4]), abs(item[5]));
    peer[0] = ip;
    peer[1] = port;
}