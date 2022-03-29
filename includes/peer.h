#ifndef PEER_BUFFER_SIZE
#define PEER_BUFFER_SIZE 50000

struct peer_state {
    int am_choking;
    int am_interested;
    int peer_choking;
    int peer_interested;
};

struct peer {
    float last_call;
    int has_handshaked;
    int healthy;
    char *buffer;
    int buffer_size;
    int buffer_start;
    int socket;
    char *ip;
    char *port;
    int number_of_pieces;
    char *bitfield;
    struct peer_state *state;
};

struct peer **peers;
int peer_count;
int peer_size;
int is_active;

struct peer *init_peer(char *ip, char *port, int number_of_pieces);
int peer_connect(struct peer *pr);
void send_to_peer(struct peer *p, char *message, int message_length);
int receive_from_peer(struct peer *p);
int is_eligible(struct peer *p);
int has_piece(struct peer *p, int index);
int am_choking(struct peer *p);
int am_unchoked(struct peer *p);
int is_choking(struct peer *p);
int is_unchoked(struct peer *p);
int am_interested(struct peer *p);
int is_interested(struct peer *p);
void handle_choke(struct peer *p);
void handle_unchoke(struct peer *p);
void handle_interested(struct peer *p);
void handle_uninterested(struct peer *p);
void handle_have(struct peer *p, char *mess);
void handle_bitfield(struct peer *p, char *mess);
void handle_request(struct peer *p, char *mess);
void handle_piece(struct peer *p, char *message);
void handle_cancel(struct peer *p);
void handle_port(struct peer *p);
void handshake(struct peer *p);
int handle_handshake(struct peer *p);
int handle_keepalive(struct peer *p);
void handle_messages(struct peer *p);
void slice_buffer(struct peer *p, int offset);
char *get_slice(struct peer *p, int offset_length);
void handle_single_message(struct peer *p, char *message);
#endif