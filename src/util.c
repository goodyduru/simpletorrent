#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "parser.h"
#include "util.h"

int file_size(FILE *fp) {
    int size;
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    return size;
}

void out(struct parse_item *item) {
    int i = 0;
    struct decode *current;
    current = item->head;
    while ( i < item->count ) {
        printf("%s: \t", item->key);
        echo(current->value);
        current = current->next;
        i++;
    }
}

void echo(struct str *string) {
    int i = 0;
    while ( i < string->length ) {
        printf("%c", string->data[i++]);
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