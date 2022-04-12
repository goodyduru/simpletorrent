#include <netdb.h>
#define MY_PORT "6889"

struct udp_response {
    char *response;
    int len;
};

void send_tracker_request();
struct url **get_urls();
int number_of_trackers();
char *get_ip();
char *generate_param(char *path, char *peer_id);
char *get(struct url *uri, char *param);
void *get_in_addr(struct sockaddr *sa);
void add_udp_peers(struct url *uri, char *peer_id);
int get_socket(struct url *uri);
char *generate_connect_message(int transaction_id);
long parse_connect_message(char *response);
char *generate_announce_message(int transaction_id, long connection_id);
void parse_announce_message(char *response);
struct udp_response *send_udp_message(int sockfd, char *message, int message_length);
struct udp_response *receive_udp_message(int sockfd, int action, int transaction_id, int message_length);