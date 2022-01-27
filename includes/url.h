struct url {
    char *host_name;
    char *path;
    int port;
};

struct url *parse_url(char *uri, int uri_length);
char* urlencode(char* url_string, int text_len);