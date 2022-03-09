#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <openssl/sha.h>

#include "piece.h"


void init_block(struct piece *single_piece) {
    int i, block_size;
    struct block *piece_block;
    single_piece->block_list = (struct block **) malloc(sizeof(struct block *) * single_piece->number_of_blocks);
    if ( single_piece->number_of_blocks == 1 ) {
        piece_block = (struct block *) malloc(sizeof(struct block));
        piece_block->block_size = single_piece->piece_size;
        piece_block->last_seen = 0;
        piece_block->state = FREE;
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
        piece_block->data = NULL;
        single_piece->block_list[i] = piece_block;
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

void update_block_status(struct piece *piece_node) {
    struct block *single_block;
    time_t result;
    int num_of_blocks = piece_node->number_of_blocks;
    for ( int i = 0; i < num_of_blocks; i++ ) {
        single_block = piece_node->block_list[i];
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

void set_block(struct piece *piece_node, int piece_offset, char *data) {
    int block_index = piece_offset/BLOCK_SIZE;
    struct block *single_block = piece_node->block_list[block_index];
    if ( !piece_node->is_full && single_block->state != FULL ) {
        single_block->data = data;
        single_block->state = FULL;
    }
}

void get_block(struct piece *piece_node, int piece_offset, int block_size, char *result) {
    memcpy(result, piece_node->raw_data+piece_offset, block_size);
}

struct block *get_empty_block(struct piece *piece_node) {
    if ( piece_node->is_full ) {
        return NULL;
    }
    struct block *single_block;
    int num_of_blocks = piece_node->number_of_blocks;
    for ( int i = 0; i < num_of_blocks; i++ ) {
        single_block = piece_node->block_list[i];
        if ( single_block->state == FREE ) {
            continue;
        }
        single_block->state = PENDING;
        single_block->last_seen = (uintmax_t)time(NULL);
        return single_block;
    }
    return NULL;
}

int are_all_block_full(struct piece *piece_node) {
    struct block *single_block;
    int num_of_blocks = piece_node->number_of_blocks;
    for ( int i = 0; i < num_of_blocks; i++ ) {
        single_block = piece_node->block_list[i];
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