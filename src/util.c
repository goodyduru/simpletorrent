#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <openssl/sha.h>

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
    write(1, string, str_length);
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

long get_torrent_file_size() {
    long int size = 0;
    struct parse_item *length_list = parser_table_lookup("length", decode_table, TORRENT_TABLE_SIZE);
    struct decode *length_node = length_list->head;
    while ( length_node != NULL ) {
        size += atoi(length_node->value->data);
        length_node = length_node->next;
    }
    return size;
}

void create_directory(char *dir) {
    char *last = strrchr(dir, '/');
    if ( last == NULL ) {
        return;
    }
    char tmp[256];
    char *p = NULL;
    int len;
    snprintf(tmp, sizeof(tmp), "%s", dir);
    last = strrchr(tmp, '/');
    //Avoid creating file as a directory
    *last = 0;
    for ( p = tmp; *p; p++ ) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0744);
            *p = '/';
        }
    }
    mkdir(tmp, 0744);
}

unsigned char* get_info_hash() {
    struct str *info = raw_table_lookup("info", raw_table, RAW_TABLE_SIZE);
    unsigned char *hash = (unsigned char *) malloc(SHA_DIGEST_LENGTH*sizeof(unsigned char));
    SHA1((unsigned char *)info->data, info->length, hash);
    return hash;
}

int get_piece_size() {
    struct parse_item *pieces_item = parser_table_lookup("pieces", decode_table, TORRENT_TABLE_SIZE);
    return pieces_item->head->value->length / HASHED_PIECE_LENGTH;
}