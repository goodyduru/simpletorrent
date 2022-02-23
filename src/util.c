#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>

#include "parser.h"
#include "util.h"

int file_size(char *filename) {
    struct stat sbuf;
    stat(filename, &sbuf);
    return (int) sbuf.st_size;
}

void out(struct parse_item *item) {
    int i = 0;
    struct decode *current;
    current = item->head;
    while ( i < item->count ) {
        printf("%s: \t", item->key);
        echo(current->value->data, current->value->length);
        current = current->next;
        i++;
    }
}

void echo(char *string, int str_length) {
    int i = 0;
    while ( i < str_length ) {
        printf("%c", string[i++]);
    }
    printf("\n");
}

unsigned int hash(char *data, int data_len) {
    if ( data == NULL || data_len == 0 ) {
        return 0;
    }
    unsigned char *dp;
    unsigned int h =  0x811C9DC5;
    for ( dp = (unsigned char *) data; *dp && data_len > 0; dp++, data_len-- ) {
        h *= 0x01000193;
        h ^= *dp;
    }
    return h;
}

char *generate_string(int string_length) {
    char *result, *begin;
    char choices[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    result = (char *) malloc(string_length*sizeof(char));
    begin = result;
    srand(time(0));
    for ( int i = 0; i < string_length; i++ ) {
        *result++ = choices[rand() % 36];
    }
    return begin;
}