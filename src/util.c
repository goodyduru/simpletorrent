#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "parser.h"

void out(struct attr *data) {
    int i = 0;
    if ( data->value->length == strlen(data->value->data) ) {
        printf("%s: %s\n", data->key, data->value->data);
    }
    else {
        printf("%s: ", data->key);
        while ( i < data->value->length ) {
            printf("%c", data->value->data[i++]);
        }
        printf("\n");
    }
}


int hash(char *data, int data_len) {
    if ( data == NULL || data_len == 0 ) {
        return 0;
    }

    unsigned char *dp;
    int h =  0x811C9DC5;
    for ( dp = data; *dp && data_len > 0; dp++, data_len-- ) {
        h *= 0x01000193;
        h ^= *dp;
    }
    return h;
}