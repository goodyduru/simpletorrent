struct attr {
    char *key;
    char *value;
    struct attr *next;
};

struct attr *add_item(struct attr *p, char *key, char *value);
char *parse_string(char buffer[], int *index_ptr);
char *parse_integer(char buffer[], int *index_ptr);
struct attr *parse_list(struct attr *p, char buffer[], int *index_ptr, char *key);
struct attr *parse_dict(struct attr *p, char buffer[], int *index_ptr);