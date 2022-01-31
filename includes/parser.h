#define TORRENT_TABLE_SIZE 20

/**
 *  Encrypted string can contain NUL characters. This makes the string 
 * handling functions in C return wrong value e.g string length. This
 * struct handles string values including the length.
 */
struct str {
    char *data;
    int length;
};

struct decode {
    struct str *value;
    struct decode *next;
};

struct parse_item {
    char *key;
    struct decode *head;
    int count;
};

struct parse_item *decode_table[TORRENT_TABLE_SIZE];

struct str *parse_string(char buffer[], int *index_ptr);
char *parse_integer(char buffer[], int *index_ptr);
void parse_list(char buffer[], char *key, int *index_ptr);
void parse_dict(char buffer[], int *index_ptr);
int parser_table_find_slot(char *key);
void parser_table_set(char *key, struct str *value);
struct parse_item *parser_table_lookup(char *key);
void add_value_to_node(int index, struct str *value);
void parse();
void concatenate_string(struct str *first, char *second, int second_len);