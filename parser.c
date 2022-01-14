#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

#define MAX_LINE = 2000

int file_size(FILE *fp) {
    int size;
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    return size;
}

struct attr *add_item(struct attr *p, char *key, struct str *value) {
    p->key = key;
    p->value = value;
    p->next = (struct attr *) malloc(sizeof(struct attr));
    return p->next;
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
    *index_ptr = index;
    result->length = len;
    result->data = item;
    return result;
}

char *parse_integer(char buffer[], int *index_ptr) {
    char *num, *begin;
    int index = *index_ptr;
    index++; //move ahead for i
    num = (char *) malloc(255);
    begin = num;
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

struct attr *parse_list(struct attr *p, char buffer[], int *index_ptr, char *key) {
    int index = *index_ptr;
    char *value;
    index++; //move ahead of d
    struct str *result;
    while ( buffer[index] != 'e' ) {
        // Do for integer
        if ( buffer[index] == 'i' ) {
            value = parse_integer(buffer, &index);
            result = (struct str *) malloc(sizeof(struct str));
            result->data = value;
            result->length = strlen(value);
            p = add_item(p, key, result);
        }
        // Do for string
        else if ( isdigit(buffer[index]) ) {
            result = parse_string(buffer, &index);
            p = add_item(p, key, result);
        }
        else if ( buffer[index] == 'l' ) {
            p = parse_list(p, buffer, &index, key);
        }
        else if ( buffer[index] == 'd' ) {
            p = parse_dict(p, buffer, &index);
        }
    }
    index++; //account for e
    *index_ptr = index;
    return p;
}

struct attr *parse_dict(struct attr *p, char buffer[], int *index_ptr) {
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
            p = add_item(p, key, result);
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
                p = add_item(p, key, result);
                key = NULL;
            }
        }
        else if ( buffer[index] == 'l' ) {
            p = parse_list(p, buffer, &index, key);
            key = NULL;
        }
        else if ( buffer[index] == 'd' ) {
            p = parse_dict(p, buffer, &index);
            key = NULL;
        }
    }
    index++; //account for e
    *index_ptr = index;
    return p;
}


int main(int argc, char *argv[]) {
    FILE *fp;
    char *file_name = "./won.torrent";
    char *prog = argv[0];
    int size;
    int i = 0;
    struct attr *head = (struct attr *) malloc(sizeof(struct attr));
    struct attr *p = (struct attr *) malloc(sizeof(struct attr));
    if ( (fp = fopen(file_name, "rb")) == NULL ) {
        fprintf(stderr, "%s: can't open %s", prog, file_name);
        exit(1);
    }
    size = file_size(fp);
    char buffer[size];

    fread(buffer, sizeof(buffer), 1, fp);
    for ( int i = 0; i < size; i++ ) {
        printf("%c", buffer[i]);
    }
    i = 0;
    head->next = p;
    p = parse_dict(head->next, buffer, &i);
    while ( head->next != NULL ) {
        if ( strcmp(head->next->key, "pieces") == 0 ) {
            printf("%lu\n", strlen(head->next->value->data));
        }
        printf("%s: %s\n", head->next->key, head->next->value->data);
        head = head->next;
    }
    return 0;
}