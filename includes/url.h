#ifndef SIMPLETORRENT_URL
#define SIMPLETORRENT_URL

#define PATH_LENGTH 1831
#define HOST_NAME_LENGTH 254
#define PORT_LENGTH 6

char html5[256];

struct url {
    char *host_name;
    char *path;
    char *port;
};

struct url *parse_url(char *uri, int uri_length);
void urlencode_table_init();
char* urlencode(unsigned char* url_string, int text_len);
#endif