#ifndef PEER_HASH_TABLE
#define PEER_HASH_TABLE
#define NETWORK_LENGTH 6
#define BINARY_PORT_LENGTH 2
#define BINARY_IP_LENGTH 4

#include <poll.h>
#include "parser.h"
#include "peer.h"

struct peer **get_peers();
int get_number_original_peers();
void fill_peer(char *peers[][2], struct parse_item *peer_list, int *peer_length);
void init_peers();
struct peer *get_random_peer_having_piece(int piece_index);
int has_unchoked_peers();
int unchoked_peers_count();
void add_peers(struct peer **new_peers, int length);
void add_peer(struct peer *p);
void remove_peer(struct peer *p, struct pollfd *pfd);
struct peer *get_peer_by_socket(int socket);
void extract_sockets(struct pollfd **pfds);
void *connect_to_peers();
#endif