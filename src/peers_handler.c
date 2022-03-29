#include <math.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
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

void init_peers() {
    peer_count = 0;
    peer_size = 8;
    peers = ( struct peer ** ) malloc(sizeof(struct peer *)*peer_size);
    is_active = 1;
}

struct peer *get_random_peer_having_piece(int piece_index) {
    int count = 0;
    struct peer *ready_peers[peer_count], *peer;
    for ( int i = 0; i < peer_count; i++ ) {
        peer = peers[i];
        if ( is_eligible(peer) && is_unchoked(peer) && am_interested(peer) && has_piece(peer, piece_index)) {
            ready_peers[count++] = peer;
        }
    }
    srand(time(NULL));
    return ready_peers[rand() % count];
}

int has_unchoked_peers() {
    for ( int i = 0; i < peer_count; i++ ) {
        if ( is_unchoked(peers[i]) ) {
            return 1;
        }
    }
    return 0;
}

int unchoked_peers_count() {
    int count = 0;
    for ( int i = 0; i < peer_count; i++ ) {
        if ( is_unchoked(peers[i]) ) {
            count++;
        }
    }
    return count;
}

void add_peers(struct peer **new_peers, int length) {
    for ( int i = 0; i < length; i++ ) {
        handshake(new_peers[i]);
        add_peer(new_peers[i]);
    }
}

void add_peer(struct peer *p) {
    if ( peer_count == peer_size ) {
        peer_size *= 2;
        peers = realloc(peers, sizeof(struct peer *) * peer_size);
    }
    peers[peer_count++] = p; 
}

void remove_peer(struct peer *p, struct pollfd *pfd) {
    for ( int i = 0; i < peer_count; i++ ) {
        if ( p->socket == peers[i]->socket ) {
            close(p->socket);
            peers[i] =  peers[peer_count - 1];
            pfd[i] = pfd[peer_count - 1];
            peer_count--;
            break;
        }
    }
}

struct peer *get_peer_by_socket(int socket) {
    for ( int i = 0; i < peer_count; i++ ) {
        if ( socket == peers[i]->socket ) 
            return peers[i];
    }
    return NULL;
}

void extract_sockets(struct pollfd **pfds) {
    for ( int i = 0; i < peer_count; i++ ) {
        (*pfds)[i].fd = peers[i]->socket;
        (*pfds)[i].events = POLLIN;
    }
}

void run() {
    struct pollfd *pfds;
    struct peer *p;
    int count, status, poll_count;
    while (is_active && peer_count) {
        pfds = malloc(sizeof(*pfds) * peer_count);
        extract_sockets(&pfds);
        poll_count = poll(pfds, peer_count, 1000);
        if ( poll_count == -1 ) {
            perror("poll error");
            exit(1);
        }
        count = peer_count;
        for ( int i = 0; i < count; i++ ) {
            p = get_peer_by_socket(pfds[i].fd);
            if ( !p->healthy ) {
                remove_peer(p, pfds);
                continue;
            }
            if ( pfds[i].revents & POLLIN ) {
                status = receive_from_peer(p);
                if ( status == 0 ) {
                    remove_peer(p, pfds);
                    continue;
                }
                handle_messages(p);
            }
        }
        free(pfds);
    }
}