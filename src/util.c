#include <stdio.h>
#include <string.h>

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