#ifndef PIECES_HEADER
#define PIECES_HEADER

#define BLOCK_SIZE 16384
typedef enum {FREE=0, PENDING, FULL} State;

struct block {
    int block_size;
    int block_offset;
    uintmax_t last_seen;
    State state;
    char *data;
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
char *bitfields;
int complete_pieces;

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