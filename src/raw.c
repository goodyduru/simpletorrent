#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "raw.h"
#include "parser.h"
#include "url.h"
#include "util.h"

int raw_table_find_slot(char *key) {
    int index = hash(key, strlen(key)) % RAW_TABLE_SIZE;
    while ( raw_table[index] != NULL && strcmp(raw_table[index]->key, key) != 0 ) {
        index = (index + 1) % RAW_TABLE_SIZE;
    }
    return index;
}

void raw_table_set(char *key, struct str *value) {
    int index = raw_table_find_slot(key);
    if ( raw_table[index] != NULL ) {
        raw_table[index]->value = value;
        return;
    }
    struct raw_item *item = (struct raw_item *)malloc(sizeof(struct raw_item));
    item->key = strdup(key);
    item->value = value;
    raw_table[index] = item;
}

struct str *raw_table_lookup(char *key) {
    int index = raw_table_find_slot(key);
    if ( raw_table[index] != NULL ) {
        return raw_table[index]->value;
    }
    return NULL;
}


struct str *get_raw_content(char buffer[], int index, char *key) {
    int i, string_len;
    int *boundaries;
    struct str *content;
    char *data;
    content = raw_table_lookup(key);
    if ( content != NULL ) {
        return content;
    }

    //fill dictionary
    build_raw_table(buffer, index);
    content = raw_table_lookup(key);
    return content;
}

void build_raw_table(char buffer[], int index) {
    get_dict(buffer, &index, 0);
}

struct str *generate_str(char buffer[], int begin, int end ) {
    struct str *content = (struct str *) malloc(sizeof(struct str));
    int string_len = end - begin;
    char *data = (char *) malloc(sizeof(char)*string_len);
    memcpy(data, buffer+begin, string_len);
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

void get_list(char buffer[], int *index_ptr, int level) {
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
            get_list(buffer, &index, level+1);
        }
        else if ( buffer[index] == 'd' ) {
            get_dict(buffer, &index, level+1);
        }
    }
    index++; //account for e
    *index_ptr = index;
}

//Store first level dictionary in a hashtable.
void get_dict(char buffer[], int *index_ptr, int level){
    int index = *index_ptr;
    index++; //move ahead of d
    char *res = NULL;
    char *value = NULL;
    int begin;
    int end;
    struct str *content;

    while ( buffer[index] != 'e' ) {
        // Do for integer
        if ( buffer[index] == 'i' ) {
            get_integer(buffer, &index);
            if ( level == 0 && res != NULL ) {
                end = index;
                content = generate_str(buffer, begin, end);
                raw_table_set(res, content);
                free(res);
                res = NULL;
            }
        }
        // Do for string
        else if ( isdigit(buffer[index]) ) {
            value = get_string(buffer, &index);
            if ( level == 0 ) {
                if ( res == NULL ) {
                    res = value;
                    begin = index;
                }
                else {
                    end = index;
                    content = generate_str(buffer, begin, end);
                    raw_table_set(res, content);
                    free(res);
                    free(value);
                    res = NULL;
                }
            }
            else {
                free(value);
            }
        }
        else if ( buffer[index] == 'l' ) {
            get_list(buffer, &index, level+1);
            if ( level == 0 && res != NULL ) {
                end = index;
                content = generate_str(buffer, begin, end);
                raw_table_set(res, content);
                free(res);
                res = NULL;
            }
        }
        else if ( buffer[index] == 'd' ) {
            get_dict(buffer, &index, level+1);
            if ( level == 0 && res != NULL ) {
                end = index;
                content = generate_str(buffer, begin, end);
                raw_table_set(res, content);
                free(res);
                res = NULL;
            }
        }
    }
    index++; //account for e
    *index_ptr = index;
}