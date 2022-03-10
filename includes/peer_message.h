#ifndef PEER_MESSAGE_HEADER
#define PEER_MESSAGE_HEADER

#define PSTR_V1 "BitTorrent protocol"
#define PSTR_V1_LENGTH 19

struct handshake_message {
    char *peer_id;
    unsigned char *info_hash;
};

void generate_handshake_message(char *message);
struct handshake_message *read_handshake_message(char *message);
void generate_keepalive_message(char *message);
int is_keepalive_message(char *message);
void generate_choke_message(char *message);
int is_choke_message(char *message);
void generate_unchoke_message(char *message);
int is_unchoke_message(char *message);
void generate_interested_message(char *message);
int is_interested_message(char *message);
void generate_uninterested_message(char *message);
int is_uninterested_message(char *message);
void generate_have_message(char *message, int piece_index);
int read_have_message(char *message);
void generate_bitfield_message(char *message);
int read_bitfield_message(char *message, char field[]);
void generate_request_message(char *message, int piece_index, int piece_offset, int block_size);
void read_request_message(char *message, int result[]);
void generate_piece_message(char *message, int piece_index, int piece_offset, int block_length, char *data);
int read_piece_message(char *message, char *block_data);
void generate_cancel_message(char *message, int piece_index, int piece_offset, int block_size);
void read_cancel_message(char *message, int result[]);
void generate_port_message(char *message, int port);
int read_port_message(char *message);
#endif