#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "bitfield.h"
#include "peer.h"
#include "peer_message.h"
#include "pieces_handler.h"
#include "util.h"

struct peer *init_peer(char *ip, char *port, int number_of_pieces) {
    struct peer *peer = (struct peer *)malloc(sizeof(struct peer));
    struct peer_state *state = (struct peer_state *)malloc(sizeof(struct peer_state));
    peer->last_call = 0.0;
    peer->has_handshaked = 0;
    peer->healthy = 0;
    peer->ip = ip;
    peer->port = port;
    peer->number_of_pieces = number_of_pieces;
    peer->bitfield = create_bitarray(number_of_pieces);
    peer->buffer = malloc(PEER_BUFFER_SIZE);
    peer->buffer_size = 0;
    peer->buffer_start = 0;
    state->am_choking = 1;
    state->am_interested = 0;
    state->peer_choking = 1;
    state->peer_interested = 0;
    peer->state = state;
    return peer;
}

int peer_connect(struct peer *pr) {
    struct addrinfo hints, *servinfo, *p;
    int sockfd, rv, status;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ( (rv = getaddrinfo(pr->ip, pr->port, &hints, &servinfo)) != 0 ) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 0;
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

        status = fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK );

        if ( status == -1 ) {
            close(sockfd);
            perror("client: fcntl");
            continue;
        }
        break;
    }

    if ( p == NULL ) {
        fprintf(stderr, "client failed to connect");
        return 0;
    }
    freeaddrinfo(servinfo);
    pr->healthy = 1;
    pr->socket = sockfd;
    return 1;
}

void send_to_peer(struct peer *p, char *message, int message_length) {
    int bytes, sent;
    sent = 0;
    do {
        bytes = write(p->socket, message+sent, message_length-sent);
        if ( bytes < 0 ) {
            p->healthy = 0;
            perror("Failed to connect to peer");
            return;
        }
        if ( bytes == 0 ) {
            break;
        }
        sent += bytes;
    }
    while ( sent < message_length );
    p->last_call = (int) time(0);
}

int receive_from_peer(struct peer *p) {
    int bytes;
    char message[BUFSIZ*4];
    while (1) {
        bytes = recv(p->socket, message, BUFSIZ*4, 0);
        if ( bytes == 0 ) {
            break;
        }
        else if ( bytes > 0 ) {
            memcpy(p->buffer+p->buffer_size, message, bytes);
            p->buffer_size += bytes;
        }
        else {
            if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
                break;
            }
            else {
                return 0;
            }
        }
    }
    return 1;
}

int is_eligible(struct peer *p) {
    return ( p->last_call - (int) time(0) ) > 1;
}

int has_piece(struct peer *p, int index) {
    return TestBit(p->bitfield, index) >= 0;
}

int am_choking(struct peer *p) {
    return p->state->am_choking;
}

int am_unchoked(struct peer *p) {
    return !am_choking(p);
}

int is_choking(struct peer *p) {
    return p->state->peer_choking;
}

int is_unchoked(struct peer *p) {
    return !is_choking(p);
}

int am_interested(struct peer *p) {
    return p->state->am_interested;
}

int is_interested(struct peer *p) {
    return p->state->peer_interested;
}

void handshake(struct peer *p) {
    char *message = malloc(68);
    generate_handshake_message(message);
    send_to_peer(p, message, 68);
}

int handle_handshake(struct peer *p) {
    struct handshake_message *peer_message = read_handshake_message(p->buffer);
    int length = 68;
    if ( peer_message == NULL ) {
        printf("First message should be handshake\n");
        p->healthy = 0;
        return 0;
    }
    p->has_handshaked = 1;
    slice_buffer(p, length);
    free(peer_message);
    return 1;
}

int handle_keepalive(struct peer *p) {
    char *buffer;
    int is_keepalive = is_keepalive_message(p->buffer);
    int length = 4;
    if ( !is_keepalive ) {
        printf("Not keepalive message");
        return 0;
    }
    slice_buffer(p, length);
    return 1;
}

void handle_choke(struct peer *p) {
    p->state->peer_choking = 1;
}

void handle_unchoke(struct peer *p) {
    p->state->peer_choking = 0;
}

void handle_interested(struct peer *p) {
    char *message;
    p->state->peer_interested = 1;
    if ( am_choking(p) ) {
        message = malloc(6);
        generate_unchoke_message(message);
        send_to_peer(p, message, 5);
        free(message);
    }
}

void handle_uninterested(struct peer *p) {
    p->state->peer_interested = 0;
}

void handle_have(struct peer *p, char *mess) {
    char *message;
    int piece_index = read_have_message(mess);
    if ( piece_index == -1 ) {
        return;
    }
    SetBit(p->bitfield, piece_index);
    if ( is_choking(p) && !am_interested(p) ) {
        message = malloc(6);
        generate_interested_message(message);
        send_to_peer(p, message, 5);
        free(message);
        p->state->am_interested = 1;
    }
}

void handle_bitfield(struct peer *p, char *mess) {
    char *message, *bitfield;
    int status, bitfield_length;
    bitfield_length = (int)ceil(get_piece_size()/8.0);
    bitfield = malloc(bitfield_length);
    status = read_bitfield_message(mess, bitfield);
    if ( status == 0 ) {
        return;
    }
    p->bitfield = bitfield;
    if ( is_choking(p) && !am_interested(p) ) {
        message = malloc(6);
        generate_interested_message(message);
        send_to_peer(p, message, 5);
        free(message);
        p->state->am_interested = 1;
    }
}

void handle_request(struct peer *p, char *mess) {
    if ( is_interested(p) && is_unchoked(p) ) {
        int result[3], piece_index, piece_offset, block_length;
        char *block, *message;
        read_request_message(mess, result);
        piece_index = result[0];
        piece_offset = result[1];
        block_length = result[2];
        if ( block_length == 0 ) {
            return;
        }
        block = get_piece_block(piece_index, piece_offset, block_length);
        message = malloc(block_length+13);
        generate_piece_message(message, piece_index, piece_offset, block_length, block);
        send_to_peer(p, message, block_length+13);
        free(block);
        free(message);
    }
}

void handle_piece(struct peer *p, char *message) {
    struct piece_message *piece_message = get_piece_message(message);
    receive_block_piece(piece_message->piece_index, 
                        piece_message->piece_offset, 
                        piece_message->data);
    free(piece_message);
}

void handle_cancel(struct peer *p) {
    printf("Cancel message\n");
}

void handle_port(struct peer *p) {
    printf("Port message\n");
}

void handle_messages(struct peer *p) {
    char *message;
    int message_length;
    while ( (p->buffer_size - p->buffer_start) > 4 && p->healthy ) {
        if ( (!p->has_handshaked && handle_handshake(p) ) ||  handle_keepalive(p) ) {
            continue;
        }
        message_length = get_message_length(p->buffer);
        if ( p->buffer_size < message_length ) {
           break;
        }
        message = get_slice(p, message_length);
        handle_single_message(p, message);
        free(message);
    }
}

void slice_buffer(struct peer *p, int offset) {
    p->buffer_start += offset;
    int remaining = p->buffer_size - p->buffer_start;
    if ( remaining <= 0 ) {
        p->buffer_start = 0;
        p->buffer_size = 0;
    }
}

char *get_slice(struct peer *p, int offset_length) {
    int remaining = p->buffer_size - offset_length - p->buffer_start;
    char *slice, *temp;
    slice = malloc(offset_length);
    memcpy(slice, p->buffer+p->buffer_start, offset_length);
    if ( remaining <= 0 ) {
        p->buffer_start = 0;
        p->buffer_size = 0;
    }
    else {
        p->buffer_start += remaining;
    }
    return slice;
}

void handle_single_message(struct peer *p, char *message) {
    int message_id = get_message_id(message);
    switch(message_id) {
        case 0: handle_choke(p);
                break;
        case 1: handle_unchoke(p);
                break;
        case 2: handle_interested(p);
                break;
        case 3: handle_uninterested(p);
                break;
        case 4: handle_have(p, message);
                break;
        case 5: handle_bitfield(p, message);
                break;
        case 6: handle_request(p, message);
                break;
        case 7: handle_piece(p, message);
                break;
        case 8: handle_cancel(p);
                break;
        case 9: handle_port(p);
                break;
        default: printf("Unknown message\n");
    }
}