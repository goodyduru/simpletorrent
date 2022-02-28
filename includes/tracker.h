#include <netdb.h>
#define HASHED_PIECE_LENGTH 20
#define MY_PORT "6889"

void send_request();
struct url *get_url();
unsigned char *get_info_hash();
long get_left();
char *get_ip();
char *generate_param(char *path, char *peer_id);
char *get(struct url *uri, char *param);
void *get_in_addr(struct sockaddr *sa);