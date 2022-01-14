/**
 *  Encrypted string can contain NUL characters. This makes the string 
 * handling functions in C return wrong value e.g string length. This
 * struct handles string values including the length.
 */
struct str {
    char *data;
    int length;
};

struct attr {
    char *key;
    struct str *value;
    struct attr *next;
};
int file_size(FILE *fp);
struct attr *add_item(struct attr *p, char *key, struct str *value);
struct str *parse_string(char buffer[], int *index_ptr);
char *parse_integer(char buffer[], int *index_ptr);
struct attr *parse_list(struct attr *p, char buffer[], int *index_ptr, char *key);
struct attr *parse_dict(struct attr *p, char buffer[], int *index_ptr);
void out(struct attr *data);