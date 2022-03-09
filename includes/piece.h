#ifndef PIECES_HEADER
#define PIECES_HEADER

#include "pieces_handler.h"

void init_block(struct piece *single_piece);
void update_block_status(struct piece *piece_node);
void set_block(struct piece *piece_node, int piece_offset, char *data);
void get_block(struct piece *piece_node, int piece_offset, int block_size, char *result);
struct block *get_empty_block(struct piece *piece_node);
int are_all_block_full(struct piece *piece_node);
int set_to_full(struct piece *piece_node);
void merge_blocks(struct piece *piece_node, char *data);
int valid_blocks(struct piece *piece_node, char *data);
void write_piece_to_disk(struct piece *piece_node);
void add_piece_file_list(struct piece *single_piece, struct file *file_item);
#endif