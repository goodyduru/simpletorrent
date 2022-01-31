#define RAW_TABLE_SIZE 10

struct raw_item {
    char *key;
    struct str *value;
};

struct raw_item *raw_table[RAW_TABLE_SIZE];
struct str *get_raw_content(char buffer[], int index, char *key);
void get_integer(char buffer[], int *index_ptr);
char *get_string(char buffer[], int *index_ptr);
void get_list(char buffer[], int *index_ptr, int level);
void get_dict(char buffer[], int *index_ptr, int level);
int raw_table_find_slot(char *key);
void raw_table_set(char *key, struct str *value);
struct str *raw_table_lookup(char *key);
void build_raw_table(char buffer[], int index);
struct str *generate_str(char buffer[], int begin, int end);