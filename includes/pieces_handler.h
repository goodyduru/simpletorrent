#ifndef PIECES_HANDLER_HEADER
#define PIECES_HANDLER_HEADER
#include <stdint.h>
#include "piece.h"

void generate_pieces();
void generate_files();
void load_files(char **file_names, int *file_sizes, int num_of_files);
void update_bitfield(int piece_index);
void receive_block_piece(int piece_index, int piece_offset, char *data, int block_length);
void fill_block(int piece_index, int block_offset, int block_size, char *result);
int all_pieces_completed();
char *get_piece_block(int piece_index, int piece_offset, int block_length);
#endif