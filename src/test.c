#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "bitfield.h"
#include "parser.h"
#include "util.h"
#include "raw.h"
#include "url.h"
#include "tracker.h"
#include "peers_handler.h"
#include "pieces_handler.h"
#include "peer_message.h"

int test(char *file_name) {
    FILE *fp;
    int size;
    int i = 0;
    if ( (fp = fopen(file_name, "rb")) == NULL ) {
        fprintf(stderr, "can't open %s", file_name);
        exit(1);
    }
    size = file_size(file_name);
    char buffer[size];

    fread(buffer, 1, sizeof buffer, fp);
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
    item = parser_table_lookup("piece length", decode_table, TORRENT_TABLE_SIZE);
    out(item);

    struct url *res = parse_url("https://bt1.archive.org:6779/announce", 37);
    printf("%s\t%s\t%s\t%s\n", res->host_name, res->port, res->path, res->scheme);

    send_tracker_request();
    struct peer **tmp = get_peers();
    generate_pieces();
    struct parse_item *pieces_item = parser_table_lookup("pieces", decode_table, TORRENT_TABLE_SIZE);
    int number_of_pieces  = pieces_item->head->value->length / HASHED_PIECE_LENGTH;
    /**for ( int i = 0; i < number_of_pieces; i++ ) {
        printf("Index: %d, Size: %d, Is Full: %d, Number of Blocks: %d, Hash: %s, File Length: %d, File Offset: %d, Piece Offset: %d, File Name: %s\n", pieces[i]->piece_index, pieces[i]->piece_size, pieces[i]->is_full, pieces[i]->number_of_blocks, pieces[i]->piece_hash, pieces[i]->file_list->length,  pieces[i]->file_list->file_offset,  pieces[i]->file_list->piece_offset,  pieces[i]->file_list->path);
    }*/

    struct parse_item *announce_list = parser_table_lookup("announce-list", decode_table, TORRENT_TABLE_SIZE);
    out(announce_list);
    struct parse_item *announce = parser_table_lookup("announce", decode_table, TORRENT_TABLE_SIZE);
    out(announce);
    char *me = malloc(68);
    generate_handshake_message(me);
    write(1, me, 68);
    printf("\n");
    struct handshake_message *hm = read_handshake_message(me);
    printf("%s\n", hm->peer_id);
    generate_keepalive_message(me);
    printf("%s\n", me);
    printf("%d\n", is_keepalive_message(me));
    generate_choke_message(me);
    printf("%s\n", me);
    printf("%d\n", is_choke_message(me));
    generate_unchoke_message(me);
    printf("%s\n", me);
    printf("%d\n", is_unchoke_message(me));
    generate_interested_message(me);
    printf("%s\n", me);
    printf("%d\n", is_interested_message(me));
    generate_uninterested_message(me);
    printf("%s\n", me);
    printf("%d\n", is_uninterested_message(me));
    generate_have_message(me, 145);
    printf("%s\n", me);
    printf("%d\n", read_have_message(me));
    free(me);
    me = malloc(get_piece_size()+5+1);
    SetBit(bitfields, 62);
    for ( i = 0; i < (int)ceil(get_piece_size()/8.0); i++ ) {
        printf("%d\t", bitfields[i]);
    }
    printf("\n");
    generate_bitfield_message(me);
    int len = (int)ceil(get_piece_size()/8.0);
    for ( i = 5; i < len+5; i++ ) {
        printf("\\x%02x ", me[i]);
    }
    printf("\n");
    char fields[(int)ceil(get_piece_size()/8.0)];
    read_bitfield_message(me, fields);
    for ( i = 0; i < (int)ceil(get_piece_size()/8.0); i++ ) {
        printf("%d\t", fields[i]);
    }
    printf("\n");
    generate_request_message(me, 10, 30004, 32875);
    int request[3];
    read_request_message(me, request);
    printf("%d\t%d\t%d\n", request[0], request[1], request[2]);
    generate_piece_message(me, 10, 300004, 5, "Hello");
    struct piece_message *t = get_piece_message(me);
    t->data[5] = '\0';
    printf("%s\n", t->data);
    generate_cancel_message(me, 10, 30089, 38275);
    read_cancel_message(me, request);
    printf("%d\t%d\t%d\n", request[0], request[1], request[2]);
    generate_port_message(me, 4145);
    printf("%d\n", read_port_message(me));
    return 0;
}