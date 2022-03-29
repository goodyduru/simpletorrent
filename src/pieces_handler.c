#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#include "bitfield.h"
#include "parser.h"
#include "pieces_handler.h"
#include "piece.h"
#include "util.h"

void generate_pieces() {
    int start, end, piece_size, number_of_blocks;
    long total_file_size;
    struct parse_item *pieces_item = parser_table_lookup("pieces", decode_table, TORRENT_TABLE_SIZE);
    struct parse_item *piece_length_item = parser_table_lookup("piece length", decode_table, TORRENT_TABLE_SIZE);
    char *pieces_char = pieces_item->head->value->data;
    int number_of_pieces  = get_piece_size();
    int last_piece = number_of_pieces - 1;
    int i = 0;
    total_file_size = get_torrent_file_size();
    pieces = (struct piece **) malloc(number_of_pieces * sizeof(struct piece *));
    bitfields = create_bitarray(number_of_pieces);
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

void update_bitfield(int piece_index) {
    SetBit(bitfields, piece_index);
}

void receive_block_piece(int piece_index, int piece_offset, char *data) {
    struct piece *piece_node = pieces[piece_index];
    if ( piece_node->is_full ) return;
    int success;
    set_block(piece_node, piece_offset, data);
    if ( are_all_block_full(piece_node) && (success = set_to_full(piece_node)) == 1  ) {
        complete_pieces += 1; 
    }
}

void fill_block(int piece_index, int block_offset, int block_size, char *result) {
    struct piece *single_piece;
    int number_of_pieces  = get_piece_size();
    for ( int i = 0; i < number_of_pieces; i++ ) {
        if ( i != pieces[i]->piece_index ) {
            continue;
        }
        single_piece = pieces[i];
        if ( single_piece->is_full ) {
            get_block(single_piece, block_offset, block_size, result);
        }
        else {
            break;
        }
    }
}

int all_pieces_completed() {
    int number_of_pieces = get_piece_size();
    for ( int i = 0; i < number_of_pieces; i++ ) {
        if ( !pieces[i]->is_full ) {
            return 0;
        }
    }
    return 1;
}

char *get_piece_block(int piece_index, int piece_offset, int block_length) {
    char *block = malloc(block_length);
    get_block(pieces[piece_index], piece_offset, block_length, block);
    return block;
}