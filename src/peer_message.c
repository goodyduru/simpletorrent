#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <openssl/sha.h>
#include <arpa/inet.h>

#include "bitfield.h"
#include "peer_message.h"
#include "pieces_handler.h"
#include "util.h"

/**
 * format = <pstrlen><pstr><reserved><info_hash><peer_id>
 * pstrlen: Length of protocol identifier
 * pstr: Protocol Identifier i.e BitTorrent protocol
 * reserved: '0' 8 times
 * info_hash: Hash of info string gotten from tracker
 * peer_id: Peer unique identifier
 * 
 * Total length = 1 + 19 + 8 + 20 + 20 = 68
 *
 * */
void generate_handshake_message(char *message) {
    memset(message, (char) PSTR_V1_LENGTH, 1);
    memcpy(message+1, PSTR_V1, PSTR_V1_LENGTH);
    memset(message+20, 0, 8);
    unsigned char *hash = get_info_hash();
    memcpy(message+28, hash, SHA_DIGEST_LENGTH);
    free(hash);
    memcpy(message+48, peer_id, SHA_DIGEST_LENGTH);
}

//Read message as struct
struct handshake_message *read_handshake_message(char *message) {
    int pstrlen = message[0] - 0;
    char *pstr = (char *)malloc(sizeof(char)*(pstrlen+1));
    memcpy(pstr, message+1, pstrlen);
    pstr[pstrlen] = '\0';
    if ( strcmp(pstr, PSTR_V1) != 0 ) {
        printf("Invalid protocol string identifier\n");
        return NULL;
    }
    unsigned char *hash = (unsigned char *)malloc(sizeof(char)*SHA_DIGEST_LENGTH);
    char *peer_id = (char *)malloc(sizeof(char)*(SHA_DIGEST_LENGTH+1));
    struct handshake_message *peer_message = (struct handshake_message *) malloc(sizeof(struct handshake_message));
    memcpy(hash, message+1+pstrlen+8, SHA_DIGEST_LENGTH);
    memcpy(peer_id, message+1+pstrlen+8+SHA_DIGEST_LENGTH, PEER_ID_LENGTH);
    peer_id[SHA_DIGEST_LENGTH] = '\0';
    peer_message->info_hash = hash;
    peer_message->peer_id = peer_id;
    return peer_message;
}

void generate_keepalive_message(char *message) {
    int message_length = htonl(0x0);
    memcpy(message, &message_length, 4);
}

int is_keepalive_message(char *message) {
    int message_length = 0;
    memcpy(&message_length, message, 4);
    return ntohl(message_length) == 0;
}

void generate_choke_message(char *message) {
    int message_length = htonl(0x1);
    int message_id = 0x0;
    memcpy(message, &message_length, 4);
    memcpy(message+4, &message_id, 1);
}

int is_choke_message(char *message) {
    int message_length = 0;
    int message_id = 0;
    memcpy(&message_length, message, 4);
    memcpy(&message_id, message+4, 1);
    message_length = ntohl(message_length);
    return (message_length == 1 && message_id == 0);
}

void generate_unchoke_message(char *message) {
    int message_length = htonl(0x1);
    int message_id = 0x1;
    memcpy(message, &message_length, 4);
    memcpy(message+4, &message_id, 1);
}

int is_unchoke_message(char *message) {
    int message_length = 0;
    int message_id = 0;
    memcpy(&message_length, message, 4);
    memcpy(&message_id, message+4, 1);
    message_length = ntohl(message_length);
    return (message_length == 1 && message_id == 1);
}

void generate_interested_message(char *message) {
    int message_length = htonl(0x1);
    int message_id = 0x2;
    memcpy(message, &message_length, 4);
    memcpy(message+4, &message_id, 1);
}

int is_interested_message(char *message) {
    int message_length = 0;
    int message_id = 0;
    memcpy(&message_length, message, 4);
    memcpy(&message_id, message+4, 1);
    message_length = ntohl(message_length);
    return (message_length == 1 && message_id == 2);
}

void generate_uninterested_message(char *message) {
    int message_length = htonl(0x1);
    int message_id = 0x3;
    memcpy(message, &message_length, 4);
    memcpy(message+4, &message_id, 1);
}

int is_uninterested_message(char *message) {
    int message_length = 0;
    int message_id = 0;
    memcpy(&message_length, message, 4);
    memcpy(&message_id, message+4, 1);
    message_length = ntohl(message_length);
    return (message_length == 1 && message_id == 3);
}

void generate_have_message(char *message, int piece_index) {
    int message_length = htonl(0x5);
    int message_id = 0x4;
    piece_index = htonl(piece_index);
    memcpy(message, &message_length, 4);
    memcpy(message+4, &message_id, 1);
    memcpy(message+5, &piece_index, 4);
}

int read_have_message(char *message) {
    int message_length = 0;
    int message_id = 0;
    int piece_index = 0;
    memcpy(&message_length, message, 4);
    memcpy(&message_id, message+4, 1);
    memcpy(&piece_index, message+5, 4);
    message_length = ntohl(message_length);
    if ( message_length != 5 || message_id != 4 ) {
        return -1;
    }
    piece_index = ntohl(piece_index);
    return piece_index;
}

void generate_bitfield_message(char *message) {
    int nob;
    int message_id = 0x5;
    int message_length = 5;
    int bitfield_length = (int)ceil(get_piece_size()/8.0);
    int total_length = 0;
    message_length += bitfield_length;
    total_length = message_length;
    message_length = htonl(message_length);
    memcpy(message, &message_length, 4);
    memcpy(message+4, &message_id, 1);
    bitarray_to_string(bitfields, bitfield_length, message+5);
}

int read_bitfield_message(char *message, char field[]) {
    int normal_number = 0;
    int message_length = 0;
    int message_id = 0;
    int bitfield_length = (int)ceil(get_piece_size()/8.0);
    memcpy(&message_length, message, 4);
    memcpy(&message_id, message+4, 1);
    message_length = ntohl(message_length);
    if ( message_length != (bitfield_length+5) || message_id != 5 ) {
        return 0;
    }
    string_to_bitarray(message+5, bitfield_length, field);
    return 1;
}

void generate_request_message(char *message, int piece_index, int piece_offset, int block_size) {
    int message_id = 0x6;
    int message_length = 13;
    message_length = htonl(message_length);
    piece_index = htonl(piece_index);
    piece_offset = htonl(piece_offset);
    block_size = htonl(block_size);
    memcpy(message, &message_length, 4);
    memcpy(message+4, &message_id, 1);
    memcpy(message+5, &piece_index, 4);
    memcpy(message+9, &piece_offset, 4);
    memcpy(message+13, &block_size, 4);
}

void read_request_message(char *message, int result[]) {
    int piece_index = 0;
    int piece_offset = 0;
    int block_size = 0;
    int message_length = 0;
    int message_id = 0;
    memcpy(&message_length, message, 4);
    memcpy(&message_id, message+4, 1);
    if ( ntohl(message_length) != 13 || message_id != 6 ) {
        return;
    }
    memcpy(&piece_index, message+5, 4);
    memcpy(&piece_offset, message+9, 4);
    memcpy(&block_size, message+13, 4);
    result[0] = ntohl(piece_index);
    result[1] = ntohl(piece_offset);
    result[2] = ntohl(block_size);
}

void generate_piece_message(char *message, int piece_index, int piece_offset, int block_length, char *data) {
    int message_id = 0x7;
    int message_length = 9;
    message_length += block_length;
    message_length = htonl(message_length);
    piece_index = htonl(piece_index);
    piece_offset = htonl(piece_offset);
    memcpy(message, &message_length, 4);
    memcpy(message+4, &message_id, 1);
    memcpy(message+5, &piece_index, 4);
    memcpy(message+9, &piece_offset, 4);
    memcpy(message+13, data, block_length);
}

struct piece_message *get_piece_message(char *message) {
    struct piece_message *result;
    int message_length = 0;
    int message_id = 0;
    int piece_index = 0;
    int piece_offset = 0;
    int block_length = 0;
    char *block_data;
    memcpy(&message_length, message, 4);
    memcpy(&message_id, message+4, 1);
    memcpy(message+5, &piece_index, 4);
    memcpy(message+9, &piece_offset, 4);
    message_length = ntohl(message_length);
    block_length = message_length - 9;
    if ( message_length != (block_length+9) || message_id != 7 ) {
        return NULL;
    }
    block_data = malloc(block_length);
    memcpy(block_data, message+13, block_length);
    result = (struct piece_message *)malloc(sizeof(struct piece_message));
    result->piece_index = ntohl(piece_index);
    result->piece_offset = ntohl(piece_offset);
    result->data = block_data;
    return result;
}

void generate_cancel_message(char *message, int piece_index, int piece_offset, int block_size) {
    int message_id = 0x8;
    int message_length = 13;
    message_length = htonl(message_length);
    piece_index = htonl(piece_index);
    piece_offset = htonl(piece_offset);
    block_size = htonl(block_size);
    memcpy(message, &message_length, 4);
    memcpy(message+4, &message_id, 1);
    memcpy(message+5, &piece_index, 4);
    memcpy(message+9, &piece_offset, 4);
    memcpy(message+13, &block_size, 4);
}

void read_cancel_message(char *message, int result[]) {
    int piece_index = 0;
    int piece_offset = 0;
    int block_size = 0;
    int message_length = 0;
    int message_id = 0;
    memcpy(&message_length, message, 4);
    memcpy(&message_id, message+4, 1);
    if ( ntohl(message_length) != 13 || message_id != 8 ) {
        return;
    }
    memcpy(&piece_index, message+5, 4);
    memcpy(&piece_offset, message+9, 4);
    memcpy(&block_size, message+13, 4);
    result[0] = ntohl(piece_index);
    result[1] = ntohl(piece_offset);
    result[2] = ntohl(block_size);
}

void generate_port_message(char *message, int port) {
    int message_length = htonl(0x5);
    int message_id = 0x9;
    port = htonl(port);
    memcpy(message, &message_length, 4);
    memcpy(message+4, &message_id, 1);
    memcpy(message+5, &port, 4);
}

int read_port_message(char *message) {
    int message_length = 0;
    int message_id = 0;
    int port = 0;
    memcpy(&message_length, message, 4);
    memcpy(&message_id, message+4, 1);
    memcpy(&port, message+5, 4);
    message_length = ntohl(message_length);
    if ( message_length != 5 || message_id != 9 ) {
        return 0;
    }
    port = ntohl(port);
    return port;
}

int get_message_length(char *message, int offset) {
    int message_length = 0;
    memcpy(&message_length, message+offset, 4);
    message_length = ntohl(message_length);
    return message_length + 4;
}

int get_message_id(char *message) {
    int message_id = 0;
    memcpy(&message_id, message+4, 1);
    return message_id;
}