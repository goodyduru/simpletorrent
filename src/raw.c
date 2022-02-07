#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "raw.h"
#include "parser.h"
#include "url.h"
#include "util.h"

int raw_table_find_slot(char *key, struct raw_item *table[], int table_size) {
    int index = hash(key, strlen(key)) % table_size;
    while ( table[index] != NULL && strcmp(table[index]->key, key) != 0 ) {
        index = (index + 1) % table_size;
    }
    return index;
}

void raw_table_set(char *key, struct str *value, struct raw_item *table[], int table_size) {
    int index = raw_table_find_slot(key, table, table_size);
    if ( table[index] != NULL ) {
        table[index]->value = value;
        return;
    }
    struct raw_item *item = (struct raw_item *)malloc(sizeof(struct raw_item));
    item->key = strdup(key);
    item->value = value;
    table[index] = item;
}

struct str *raw_table_lookup(char *key, struct raw_item *table[], int table_size) {
    int index = raw_table_find_slot(key, table, table_size);
    if ( table[index] != NULL ) {
        return table[index]->value;
    }
    return NULL;
}


struct str *get_raw_content(char buffer[], int index, char *key, struct raw_item *table[], int table_size) {
    int i, string_len;
    int *boundaries;
    struct str *content;
    char *data;
    content = raw_table_lookup(key, table, table_size);
    if ( content != NULL ) {
        return content;
    }

    //fill dictionary
    build_raw_table(buffer, index, table, table_size);
    content = raw_table_lookup(key, table, table_size);
    return content;
}

void build_raw_table(char buffer[], int index, struct raw_item *table[], int table_size) {
    get_dict(buffer, &index, 0, table, table_size);
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

void get_list(char buffer[], int *index_ptr, int level, struct raw_item *table[], int table_size) {
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
            get_list(buffer, &index, level+1, table, table_size);
        }
        else if ( buffer[index] == 'd' ) {
            get_dict(buffer, &index, level+1, table, table_size);
        }
    }
    index++; //account for e
    *index_ptr = index;
}

//Store first level dictionary in a hashtable.
void get_dict(char buffer[], int *index_ptr, int level, struct raw_item *table[], int table_size){
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
                raw_table_set(res, content, table, table_size);
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
                    raw_table_set(res, content, table, table_size);
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
            get_list(buffer, &index, level+1, table, table_size);
            if ( level == 0 && res != NULL ) {
                end = index;
                content = generate_str(buffer, begin, end);
                raw_table_set(res, content, table, table_size);
                free(res);
                res = NULL;
            }
        }
        else if ( buffer[index] == 'd' ) {
            get_dict(buffer, &index, level+1, table, table_size);
            if ( level == 0 && res != NULL ) {
                end = index;
                content = generate_str(buffer, begin, end);
                raw_table_set(res, content, table, table_size);
                free(res);
                res = NULL;
            }
        }
    }
    index++; //account for e
    *index_ptr = index;
}