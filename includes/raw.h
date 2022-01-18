struct str *get_raw_content(char buffer[], int index, char *key);
void get_integer(char buffer[], int *index_ptr);
char *get_string(char buffer[], int *index_ptr);
void get_list(char buffer[], int *index_ptr);
int *get_dict(char buffer[], int *index_ptr, char *key);