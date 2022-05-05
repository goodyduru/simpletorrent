#include <math.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <netinet/in.h>

#include "peers_handler.h"
#include "util.h"

struct peer **get_peers() {
    struct parse_item *peer_list = parser_table_lookup("peers", tracker_response_table, TRACKER_RESPONSE_SIZE);
    if ( peer_list == NULL ) {
        return NULL;
    }
    printf("Number of Peers: %d\n", peer_list->count);
    int array_length = peer_list->count * 2;
    char *peers_array[array_length][2];
    fill_peer(peers_array, peer_list, &array_length);
    int i = 0;
    int number_of_pieces = get_piece_size();
    struct peer **raw_peers = (struct peer **) malloc(sizeof(struct peer *)*array_length);
    struct peer *p;
    while ( i < array_length ) {
        printf("IP: %s\tPort: %s\n", peers_array[i][0], peers_array[i][1]);
        p = init_peer(peers_array[i][0], peers_array[i][1], number_of_pieces);
        raw_peers[i] = p;
        i++;
    }
    return raw_peers;
}

int get_number_original_peers() {
    struct parse_item *peer_item = parser_table_lookup("peers", tracker_response_table, TRACKER_RESPONSE_SIZE);
    struct str *peers = peer_item->head->value;
    return peers->length/NETWORK_LENGTH;
}

void fill_peer(char *peers[][2], struct parse_item *peer_list, int *peer_length) {
    int length = *peer_length, index = 0, j = 0;
    struct str *peer_str;
    char *item = (char *) malloc(sizeof(char)*NETWORK_LENGTH);
    struct decode *current = peer_list->head;
    int pos = parser_table_find_slot("peers", tracker_response_table, TRACKER_RESPONSE_SIZE);
    while ( current && j < length ) {
        if ( current->value->length <= 6 ) {
            memcpy(item, current->value->data, NETWORK_LENGTH);
            char *net[2];
            gen_ip_and_port(item, peers[j++]);
        }
        else {
            while ( index < current->value->length && j < length ) {
                memcpy(item, current->value->data+index, NETWORK_LENGTH);
                peer_str = to_str(item, NETWORK_LENGTH);
                if ( !node_value_exists(pos, peer_str, tracker_response_table) ) {
                    char *net[2];
                    gen_ip_and_port(item, peers[j++]);
                }
                index += NETWORK_LENGTH;
            }
        }
        current = current->next;
    }
    free(item);
    *peer_length = j;
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
    if ( count == 0 ) {
        return NULL;
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
    int connected;
    for ( int i = 0; i < length; i++ ) {
        connected = peer_connect(new_peers[i]);
        printf("Connected: %d\n", connected);
        if ( connected ) {
            handshake(new_peers[i]);
            add_peer(new_peers[i]);
        }
        else {
            free(new_peers[i]);
        }
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
        printf("Socket: %d\n", peers[i]->socket);
        (*pfds)[i].fd = peers[i]->socket;
        (*pfds)[i].events = POLLIN;
    }
}

void *connect_to_peers() {
    struct pollfd *pfds;
    struct peer *p;
    int count, status, poll_count;
    while ( is_active ) {
        if ( !peer_count ) {
            sleep(2);
            continue;
        }
        pfds = malloc(sizeof(*pfds) * peer_count);
        extract_sockets(&pfds);
        poll_count = poll(pfds, peer_count, 1000);
        if ( poll_count == -1 ) {
            perror("poll error");
            exit(1);
        }
        for ( int i = 0; i < peer_count; i++ ) {
            p = get_peer_by_socket(pfds[i].fd);
            if ( !p->healthy ) {
                remove_peer(p, pfds);
                continue;
            }
            if ( pfds[i].revents & POLLIN ) {
                printf("Health: %d\n", p->healthy);
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
    return NULL;
}