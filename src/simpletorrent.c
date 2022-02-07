#include <stdio.h>
#include <stdlib.h>
#include <openssl/sha.h>

#include "parser.h"
#include "util.h"
#include "raw.h"
#include "url.h"
#include "tracker.h"

int main(int argc, char *argv[]) {
    FILE *fp;
    char *file_name = "./won.torrent";
    char *prog = argv[0];
    int size;
    int i = 0;
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
    printf("\n");
    struct str *result = get_raw_content(buffer, 0, "announce", raw_table, RAW_TABLE_SIZE);
    for ( int i = 0; i < result->length; i++ ) {
        printf("%c", result->data[i]);
    }
    printf("\n");

    result = get_raw_content(buffer, 0, "info", raw_table, RAW_TABLE_SIZE);
    for ( int i = 0; i < result->length; i++ ) {
        printf("%c", result->data[i]);
    }
    printf("\n");
    
    parse(raw_table, RAW_TABLE_SIZE, decode_table, TORRENT_TABLE_SIZE);
    struct parse_item *item = parser_table_lookup("path", decode_table, TORRENT_TABLE_SIZE);
    out(item);
    printf("%d\n", item->count);
    item = parser_table_lookup("length", decode_table, TORRENT_TABLE_SIZE);
    printf("%d\n", item->count);
    item = parser_table_lookup("crc32", decode_table, TORRENT_TABLE_SIZE);
    printf("%d\n", item->count);
    item = parser_table_lookup("piece length", decode_table, TORRENT_TABLE_SIZE);
    out(item);
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*) result->data, result->length, hash);
    for ( int i = 0; i < SHA_DIGEST_LENGTH; i++ ) {
        printf("%c", hash[i]);
        printf("%c", '/');
    }
    printf("\n");

    struct url *res = parse_url("bt1.archive.org:6779/announce", 36);
    printf("%s\t%s\t%s\n", res->host_name, res->port, res->path);

    send_request();
    return 0;
}