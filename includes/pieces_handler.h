#ifndef PIECES_HANDLER_HEADER
#define PIECES_HANDLER_HEADER
#include <stdint.h>

#define BLOCK_SIZE 16384
#define HASHED_PIECE_LENGTH 20
typedef enum {FREE=0, PENDING, FULL} State;

struct block {
    int block_size;
    uintmax_t last_seen;
    State state;
    char *data;
    int block_index;
};

struct file {
    int length;
    int file_offset;
    int piece_offset;
    char *path;
    struct file *next;
};

struct piece {
    int piece_index;
    int piece_size;
    int is_full;
    int number_of_blocks;
    char *piece_hash;
    char *raw_data;
    struct file *file_list;
    struct block **block_list;
};

struct piece **pieces;
int *bitfields;
int complete_pieces;

void generate_pieces();
void generate_files();
void load_files(char **file_names, int *file_sizes, int num_of_files);
void update_bitfield(int piece_index);
#endif