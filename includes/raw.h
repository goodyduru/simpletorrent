#define RAW_TABLE_SIZE 10
#define TRACKER_RESPONSE_TABLE 8

struct raw_item {
    char *key;
    struct str *value;
};

struct raw_item *raw_table[RAW_TABLE_SIZE];
struct raw_item *raw_tracker_response_table[TRACKER_RESPONSE_TABLE];
struct str *get_raw_content(char buffer[], int index, char *key, struct raw_item *table[], int table_size);
void get_integer(char buffer[], int *index_ptr);
char *get_string(char buffer[], int *index_ptr);
void get_list(char buffer[], int *index_ptr, int level, struct raw_item *table[], int table_size);
void get_dict(char buffer[], int *index_ptr, int level, struct raw_item *table[], int table_size);
int raw_table_find_slot(char *key, struct raw_item *table[], int table_size);
void raw_table_set(char *key, struct str *value, struct raw_item *table[], int table_size);
struct str *raw_table_lookup(char *key, struct raw_item *table[], int table_size);
void build_raw_table(char buffer[], int index, struct raw_item *table[], int table_size);
struct str *generate_str(char buffer[], int begin, int end);