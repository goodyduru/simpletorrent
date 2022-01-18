#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "raw.h"
#include "parser.h"

struct str *get_raw_content(char buffer[], int index, char *key) {
    int i, string_len;
    int *boundaries;
    struct str *content;
    char *data;

    boundaries = get_dict(buffer, &index, key);
    content = (struct str *) malloc(sizeof(struct str));
    string_len = boundaries[1] - boundaries[0] + 1;
    data = (char *) malloc(sizeof(char)*string_len);
    memcpy(data, buffer+boundaries[0], string_len);
    content->length = string_len;
    content->data = data;
    return content;
}

char *get_string(char buffer[], int *index_ptr) {
    int index = *index_ptr;
    int len = 0;
    int passed_column = 0;
    char *item;
    while ( 1 ) {
        if ( passed_column ) {
            memcpy(item, buffer+index, len);
            index += len;
            break;
        }
        else {
            if ( buffer[index] == ':' ) {
                passed_column = 1;
                item = (char *) malloc(len+1);
            } 
            else {
                len = len*10 + (buffer[index] - '0');
            }
            index++;
        }
    }
    item[len] = '\0';
    *index_ptr = index;
    return item;
}

void get_integer(char buffer[], int *index_ptr) {
    int index = *index_ptr;
    index++; //move ahead for i
    if ( buffer[index] == '-' ) {
        index++;
    }
    while ( isdigit(buffer[index]) ) {
        index++;
    }
    //For e
    index++;
    *index_ptr = index;
}

void get_list(char buffer[], int *index_ptr) {
    int index = *index_ptr;
    char *value;
    index++; //move ahead of l
    int *result;
    while ( buffer[index] != 'e' ) {
        // Do for integer
        if ( buffer[index] == 'i' ) {
            get_integer(buffer, &index);
        }
        // Do for string
        else if ( isdigit(buffer[index]) ) {
            value = get_string(buffer, &index);
        }
        else if ( buffer[index] == 'l' ) {
            get_list(buffer, &index);
        }
        else if ( buffer[index] == 'd' ) {
            result = get_dict(buffer, &index, "list");
        }
    }
    index++; //account for e
    *index_ptr = index;
}

int *get_dict(char buffer[], int *index_ptr, char* key){
    int index = *index_ptr;
    int *result = (int *) calloc(2, sizeof(int));
    char *res;
    int is_info = 0;
    index++; //move ahead of d
    int *dict_result;

    while ( buffer[index] != 'e' ) {
        // Do for integer
        if ( buffer[index] == 'i' ) {
            get_integer(buffer, &index);
        }
        // Do for string
        else if ( isdigit(buffer[index]) ) {
            res = get_string(buffer, &index);
            if ( strcmp(key, res) == 0 ) {
                is_info = 1;
                result[0] = index;
            }
            else {
                is_info = 0;
            }
            free(res);
        }
        else if ( buffer[index] == 'l' ) {
            get_list(buffer, &index);
        }
        else if ( buffer[index] == 'd' ) {
            dict_result = get_dict(buffer, &index, key);
            if ( is_info ) {
                result[1] = index-1;
            }
            is_info = 0;
            free(dict_result);
        }
    }
    index++; //account for e
    *index_ptr = index;
    return result;
}