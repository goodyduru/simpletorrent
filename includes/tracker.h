#include <netdb.h>
#define MY_PORT "6889"

void send_request();
struct url *get_url();
char *get_ip();
char *generate_param(char *path, char *peer_id);
char *get(struct url *uri, char *param);
void *get_in_addr(struct sockaddr *sa);