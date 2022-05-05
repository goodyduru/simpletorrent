#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raw.h"
#include "parser.h"
#include "util.h"

int parser_table_find_slot(char *key, struct parse_item *table[], int table_size) {
    int index = hash(key, strlen(key)) % table_size;
    while ( table[index] != NULL && strcmp(table[index]->key, key) != 0 ) {
        index = (index + 1) % table_size;
    }
    return index;
}

int node_value_exists(int index, struct str *value, struct parse_item *table[]) {
    struct parse_item *item;
    struct decode *current;

    item = table[index];
    current = item->head;
    while ( current != NULL ) {
        if ( memcmp(current->value->data, value->data, value->length) == 0 ) {
            return 1;
        }
        current = current->next;
    }
    return 0;
}

void parser_table_set(char *key, struct str *value, struct parse_item *table[], int table_size) {
    int index = parser_table_find_slot(key, table, table_size);
    if ( table[index] != NULL ) {
        if ( strcmp(key, "peers") == 0 && !node_value_exists(index, value, table) ) {
            add_value_to_node(index, value, table);
        }
        else if ( strcmp(key, "peers") != 0 ) {
            add_value_to_node(index, value, table);
        }
        return;
    }
    struct decode *head = (struct decode *)malloc(sizeof(struct decode));
    head->value = value;
    head->next = NULL;
    struct parse_item *item = (struct parse_item *)malloc(sizeof(struct parse_item));
    item->key = strdup(key);
    item->count = 1;
    item->head = head;
    table[index] = item;
}

void add_value_to_node(int index, struct str *value, struct parse_item *table[]) {
    struct parse_item *item;
    struct decode *current, *next;

    item = table[index];
    current = item->head;
    while ( current->next != NULL ) {
        current = current->next;
    }
    next = (struct decode *)malloc(sizeof(struct decode));
    next->value = value;
    next->next = NULL;
    current->next = next;
    item->count++;
}

struct parse_item *parser_table_lookup(char *key, struct parse_item *table[], int table_size) {
    int index = parser_table_find_slot(key, table, table_size);
    if ( table[index] != NULL ) {
        return table[index];
    }
    return NULL;
}


void parse_table_item(struct raw_item *item, struct parse_item *table[], int table_size) {
    char *parsed_value;
    struct str *result;
    char *string = item->value->data;
    char type = *string;
    int begin = 0;
    if ( type == 'i' ) { // This is an integer
        parsed_value = parse_integer(string, &begin);
        result = (struct str *) malloc(sizeof(struct str));
        result->data = parsed_value;
        result->length = strlen(parsed_value);
        parser_table_set(item->key, result, table, table_size);
    }
    else if ( isdigit(type) ) {
        result = parse_string(string, &begin);
        parser_table_set(item->key, result, table, table_size);
    }
    else if ( type == 'l' ) {
        parse_list(string, item->key, &begin, table, table_size);
    }
    else {
        parse_dict(string, &begin, table, table_size);
    }

}

void parse(struct raw_item *table_raw[], int raw_table_size, struct parse_item *parse_table[], int table_size){
    struct raw_item *item;
    for ( int i = 0; i < raw_table_size; i++ ) {
        if ( table_raw[i] != NULL ) {
            item = table_raw[i];
            parse_table_item(item, parse_table, table_size);
        }
    }
}

char *parse_integer(char buffer[], int *index_ptr) {
    char *num, *begin;
    int index = *index_ptr;
    index++; //move ahead for i
    num = (char *) malloc(255);
    begin = num;
    if ( buffer[index] == '-' ) {
        *num++ = buffer[index];
        index++;
    }
    while ( isdigit(buffer[index]) ) {
        *num++ = buffer[index];
        index++;
    }
    *num = '\0';
    //For e
    index++;
    *index_ptr = index;
    return begin;
}

struct str *parse_string(char buffer[], int *index_ptr) {
    int index = *index_ptr;
    int len = 0;
    int passed_column = 0;
    struct str *result = (struct str *) malloc(sizeof(struct str));
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
    result->length = len;
    result->data = item;
    return result;
}

void parse_list(char buffer[], char *key, int *index_ptr, struct parse_item *table[], int table_size) {
    char *value;
    int index = *index_ptr;
    index++; //move ahead of l
    struct str *result, *origin;
    //Initialize origin if key is path
    if ( strcmp(key, "path") == 0 ) {
        origin = (struct str *) malloc(sizeof(struct str));
    }
    while ( buffer[index] != 'e' ) {
        // Do for integer
        if ( buffer[index] == 'i' ) {
            value = parse_integer(buffer, &index);
            if ( strcmp(key, "path") == 0 ) {
                concatenate_string(origin, value, strlen(value));
            }
            else {
                result = (struct str *) malloc(sizeof(struct str));
                result->data = value;
                result->length = strlen(value);
                parser_table_set(key, result, table, table_size);
            }
        }
        // Do for string
        else if ( isdigit(buffer[index]) ) {
            result = parse_string(buffer, &index);
            if ( strcmp(key, "path") == 0 ) {
                concatenate_string(origin, result->data, result->length);
                free(result);
            }
            else {
                parser_table_set(key, result, table, table_size);
            }
        }
        else if ( buffer[index] == 'l' ) {
            parse_list(buffer, key, &index, table, table_size);
        }
        else if ( buffer[index] == 'd' ) {
            parse_dict(buffer, &index, table, table_size);
        }
    }

    if ( strcmp(key, "path") == 0 ) {
        parser_table_set(key, origin, table, table_size);
    }
    index++; //account for e
    *index_ptr = index;
}

void concatenate_string(struct str *first, char *second, int second_len) {
    char *result;
    int i;
    if ( first->data == NULL ) {
        first->data = second;
        first->length = second_len;
        return;
    }
    result = realloc(first->data, (first->length+second_len+2) * sizeof(char));
    result[first->length] = '/';
    for ( i = 0; i < second_len; i++ ) {
        result[first->length+i+1] = second[i];
    }
    result[first->length+second_len+1] = '\0';
    first->data = result;
    first->length += second_len;
    first->length += 2;
}

void parse_dict(char buffer[], int *index_ptr, struct parse_item *table[], int table_size) {
    int index = *index_ptr;
    char *value;
    char *key = NULL;
    index++; //move ahead of d
    struct str *result;
    while ( buffer[index] != 'e' ) {
        // Do for integer
        if ( buffer[index] == 'i' ) {
            value = parse_integer(buffer, &index);
            result = (struct str *) malloc(sizeof(struct str));
            result->data = value;
            result->length = strlen(value);
            parser_table_set(key, result, table, table_size);
            free(key);
            key = NULL;
        }
        // Do for string
        else if ( isdigit(buffer[index]) ) {
            if ( key == NULL ) {
                result = parse_string(buffer, &index);
                key = result->data;
            }
            else {
                result = parse_string(buffer, &index);
                parser_table_set(key, result, table, table_size);
                free(key);
                key = NULL;
            }
        }
        else if ( buffer[index] == 'l' ) {
            parse_list(buffer, key, &index, table, table_size);
            free(key);
            key = NULL;
        }
        else if ( buffer[index] == 'd' ) {
            parse_dict(buffer, &index, table, table_size);
            free(key);
            key = NULL;
        }
    }
    index++; //account for e
    *index_ptr = index;
}

struct str *to_str(char *item, int length) {
    struct str *result = ( struct str * ) malloc(sizeof(struct str));
    result->data = item;
    result->length = length;
    return result;
}