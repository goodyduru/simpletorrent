#include <stdio.h>

void out(struct parse_item *item);
void echo(char *string, int str_length);
unsigned int hash(char *data, int data_len);
int file_size(FILE *fp);
char *generate_string(int string_length);