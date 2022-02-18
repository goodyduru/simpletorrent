#ifndef PEER_HASH_TABLE
#define PEER_HASH_TABLE
#define NETWORK_LENGTH 6
#define MAX_PORT_LENGTH 5 //ports have a maximum of 65536
#define BINARY_PORT_LENGTH 2
#define BINARY_IP_LENGTH 4

void parse_peer();
void fill_peer(char *peers[][2], struct str *peers_str);
void gen_ip_and_port(char *item, char *peer[]);
#endif