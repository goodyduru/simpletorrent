#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>

#include <openssl/sha.h>

#include "parser.h"
#include "pieces.h"
#include "util.h"

void generate_pieces() {
    int start, end, piece_size, number_of_blocks;
    long total_file_size;
    struct parse_item *pieces_item = parser_table_lookup("pieces", decode_table, TORRENT_TABLE_SIZE);
    struct parse_item *piece_length_item = parser_table_lookup("piece length", decode_table, TORRENT_TABLE_SIZE);
    char *pieces_char = pieces_item->head->value->data;
    int number_of_pieces  = pieces_item->head->value->length / HASHED_PIECE_LENGTH;
    int last_piece = number_of_pieces - 1;
    int i = 0;
    total_file_size = get_torrent_file_size();
    pieces = (struct piece **) malloc(number_of_pieces * sizeof(struct piece *));
    bitfields = (int *) malloc(number_of_pieces * sizeof(int));
    piece_size = atoi(piece_length_item->head->value->data);
    complete_pieces = 0;
    while ( i < number_of_pieces ) {
        start = i * HASHED_PIECE_LENGTH;
        if ( i == last_piece ) {
            piece_size = total_file_size - last_piece*piece_size;
        }
        number_of_blocks = (int) ceil((float)piece_size/BLOCK_SIZE);
        pieces[i] = (struct piece *) malloc(sizeof(struct piece));
        pieces[i]->piece_index = i;
        pieces[i]->piece_size = piece_size;
        pieces[i]->is_full = 0;
        pieces[i]->number_of_blocks = number_of_blocks;
        pieces[i]->piece_hash = (char *) malloc(sizeof(char) * HASHED_PIECE_LENGTH);
        memcpy(pieces[i]->piece_hash, pieces_char+start, HASHED_PIECE_LENGTH);
        pieces[i]->raw_data = NULL;
        init_block(pieces[i]);
        pieces[i]->file_list = NULL;
        i++;
    }
    generate_files();
}

void init_block(struct piece *single_piece) {
    int i, block_size;
    struct block *piece_block;
    single_piece->block_list = (struct block **) malloc(sizeof(struct block *) * single_piece->number_of_blocks);
    if ( single_piece->number_of_blocks == 1 ) {
        piece_block = (struct block *) malloc(sizeof(struct block));
        piece_block->block_size = single_piece->piece_size;
        piece_block->last_seen = 0;
        piece_block->state = FREE;
        piece_block->block_index = 0;
        piece_block->data = NULL;
        single_piece->block_list[0] = piece_block;
        return;
    }
    block_size = BLOCK_SIZE;
    for ( i = 0; i < single_piece->number_of_blocks; i++ ) {
        if ( i == (single_piece->number_of_blocks - 1) && single_piece->piece_size % BLOCK_SIZE > 0 ) {
            block_size = single_piece->piece_size % BLOCK_SIZE;
        }
        piece_block = (struct block *) malloc(sizeof(struct block));
        piece_block->block_size = block_size;
        piece_block->last_seen = 0;
        piece_block->state = FREE;
        piece_block->block_index = i;
        piece_block->data = NULL;
        single_piece->block_list[i] = piece_block;
    }
}

void generate_files() {
    int title_length, num_of_files, i, *file_sizes;
    char *title_name, *file_name, **file_names;
    struct decode *path_node, *size_node;
    struct parse_item *name_item = parser_table_lookup("name", decode_table, TORRENT_TABLE_SIZE);
    struct parse_item *path_item = parser_table_lookup("path", decode_table, TORRENT_TABLE_SIZE);
    struct parse_item *size_item = parser_table_lookup("length", decode_table, TORRENT_TABLE_SIZE);
    title_name = name_item->head->value->data;
    title_length = name_item->head->value->length;
    if ( path_item == NULL ) {
        num_of_files = 1;
        file_names = (char **) malloc(num_of_files*sizeof(char *));
        file_sizes = (int *) malloc(num_of_files*sizeof(int));
        file_names[0] = title_name;
        file_sizes[0] = atoi(size_item->head->value->data);
    }
    else {
        mkdir(title_name, 0744);
        path_node = path_item->head;
        size_node = size_item->head;
        num_of_files = path_item->count;
        file_names = (char **) malloc(num_of_files*sizeof(char *));
        file_sizes = (int *) malloc(num_of_files*sizeof(int));
        i = 0;
        while ( path_node != NULL ) {
            file_names[i] = (char *) malloc((title_length+path_node->value->length+2)*sizeof(char));
            sprintf(file_names[i], "%s/%s", title_name, path_node->value->data);
            create_directory(file_names[i]);
            file_sizes[i] = atoi(size_node->value->data);
            i++;
            path_node = path_node->next;
            size_node = size_node->next;
        }
    }
    load_files(file_names, file_sizes, num_of_files);
    free(file_names);
    free(file_sizes);
}

void load_files(char **file_names, int *file_sizes, int num_of_files) {
    int file_offset, current_file_size, piece_index, piece_length, piece_size;
    struct file *file_item;
    int piece_offset = 0, piece_size_used = 0, index = 0;
    struct parse_item *piece_length_item = parser_table_lookup("piece length", decode_table, TORRENT_TABLE_SIZE);
    piece_length = atoi(piece_length_item->head->value->data);
    while ( index < num_of_files ) {
        file_offset = 0;
        current_file_size = file_sizes[index];
        while ( current_file_size > 0 ) {
            piece_index = piece_offset / piece_length;
            piece_size = pieces[piece_index]->piece_size - piece_size_used;
            file_item = ( struct file *) malloc(sizeof(struct file));
            if ( current_file_size - piece_size  < 0 ) {
                file_item->length = current_file_size;
                file_item->file_offset = file_offset;
                file_item->piece_offset = piece_size_used;
                file_item->path = file_names[index];
                piece_offset += current_file_size;
                piece_size_used += current_file_size;
                file_offset += current_file_size;
                current_file_size = 0;
            }
            else {
                file_item->length = piece_size;
                file_item->file_offset = file_offset;
                file_item->piece_offset = piece_size_used;
                file_item->path = file_names[index];
                current_file_size -= piece_size;
                piece_offset += piece_size;
                file_offset += piece_size;
                piece_size_used = 0;
            }
            file_item->next = NULL;
            add_piece_file_list(pieces[piece_index], file_item);
        }
        index++;
    }
}

void add_piece_file_list(struct piece *single_piece, struct file *file_item) {
    if ( single_piece->file_list == NULL ) {
        single_piece->file_list = file_item;
        return;
    }
    struct file *node = single_piece->file_list;
    while ( node->next != NULL ) {
        node = node->next;
    }
    node->next = file_item;
}

void update_bitfield(int piece_index) {
    bitfields[piece_index] = 1;
}

void update_block_status(int piece_index) {
    struct block *single_block;
    time_t result;
    int num_of_blocks = pieces[piece_index]->number_of_blocks;
    for ( int i = 0; i < num_of_blocks; i++ ) {
        single_block = pieces[piece_index]->block_list[i];
        result = time(NULL);
        if ( single_block->state == PENDING && ((uintmax_t)result - single_block->last_seen) > 5 ) {
            single_block->state = FREE;
            single_block->last_seen = 0;
            if ( single_block->data != NULL ) {
                free(single_block->data);
                single_block->data = NULL;
            }
        }
    }
}

void set_block(int piece_index, int piece_offset, char *data) {
    int block_index = piece_offset/BLOCK_SIZE;
    struct block *single_block = pieces[piece_index]->block_list[block_index];
    if ( !pieces[piece_index]->is_full && single_block->state != FULL ) {
        single_block->data = data;
        single_block->state = FULL;
    }
}

void get_block(int piece_index, int block_offset, int block_size, char *result) {
    memcpy(result, pieces[piece_index]->raw_data+block_offset, block_size);
}

struct block *get_empty_block(int piece_index) {
    if ( pieces[piece_index]->is_full ) {
        return NULL;
    }
    struct block *single_block;
    int num_of_blocks = pieces[piece_index]->number_of_blocks;
    for ( int i = 0; i < num_of_blocks; i++ ) {
        single_block = pieces[piece_index]->block_list[i];
        if ( single_block->state == FREE ) {
            continue;
        }
        single_block->state = PENDING;
        single_block->last_seen = (uintmax_t)time(NULL);
        return single_block;
    }
    return NULL;
}

int are_all_block_full(int piece_index) {
    struct block *single_block;
    int num_of_blocks = pieces[piece_index]->number_of_blocks;
    for ( int i = 0; i < num_of_blocks; i++ ) {
        single_block = pieces[piece_index]->block_list[i];
        if ( single_block->state != FULL ) {
            return 0;
        }
    }
    return 1;
}

int set_to_full(struct piece *piece_node) {
    char *data = (char *) malloc(sizeof(char)*piece_node->piece_size);
    merge_blocks(piece_node, data);
    if ( !valid_blocks(piece_node, data) ) {
        init_block(piece_node);
        free(data);
        return 0;
    }

    piece_node->is_full = 1;
    piece_node->raw_data = data;
    write_piece_to_disk(piece_node);
    return 1;
}

void merge_blocks(struct piece *piece_node, char *data) {
    struct block *single_block;
    int piece_offset = 0;
    int num_of_blocks = piece_node->number_of_blocks;
    for ( int i = 0; i < num_of_blocks; i++ ) {
        single_block = piece_node->block_list[i];
        memcpy(data+piece_offset, single_block->data, single_block->block_size);
        free(single_block->data);
        free(single_block);
        piece_offset += single_block->block_size;
    }
    free(piece_node->block_list);
}

int valid_blocks(struct piece *single_piece, char *data) {
    unsigned char *hash = (unsigned char *) malloc(SHA_DIGEST_LENGTH*sizeof(unsigned char));
    SHA1((unsigned char *)data, single_piece->piece_size, hash);

    if ( memcmp(hash, single_piece->piece_hash, SHA_DIGEST_LENGTH) == 0 ) {
        return 1;
    }
    printf("Invalid Hash\n ");
    return 0;
}   

void write_piece_to_disk(struct piece *single_piece) {
    char *path;
    int file_offset, piece_offset, length;
    FILE *bfile;
    struct file *file_node = single_piece->file_list;
    while ( file_node != NULL ) {
        file_offset = file_node->file_offset;
        length = file_node->length;
        piece_offset = file_node->piece_offset;
        path = file_node->path;
        bfile = fopen(path, "r+b");
        if ( bfile == NULL ) {
            bfile = fopen(path, "wb");
        }
        if ( bfile == NULL ) {
            printf("Problems creating file %s\n", path);
            return;
        }
        fseek(bfile, file_offset, SEEK_SET);
        fwrite(single_piece->raw_data+piece_offset, sizeof(char), length, bfile);
        fclose(bfile);
        file_node = file_node->next;
    }
}