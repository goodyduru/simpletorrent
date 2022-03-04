#ifndef PIECES_HEADER
#define PIECES_HEADER

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
void init_block(struct piece *single_piece);
void generate_files();
void load_files(char **file_names, int *file_sizes, int num_of_files);
void add_piece_file_list(struct piece *single_piece, struct file *file_item);
void update_bitfield(int piece_index);
void update_block_status(int piece_index);
void set_block(int piece_index, int piece_offset, char *data);
void get_block(int piece_index, int block_offset, int block_size, char *result);
struct block *get_empty_block(int piece_index);
int are_all_block_full(int piece_index);
int set_to_full(struct piece *piece_node);
void merge_blocks(struct piece *piece_node, char *data);
int valid_blocks(struct piece *piece_node, char *data);
void write_piece_to_disk(struct piece *piece_node);
#endif