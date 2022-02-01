#define PATH_LENGTH 1831
#define HOST_NAME_LENGTH 254
#define PORT_LENGTH 6

struct url {
    char *host_name;
    char *path;
    char *port;
};

struct url *parse_url(char *uri, int uri_length);
char* urlencode(char* url_string, int text_len);