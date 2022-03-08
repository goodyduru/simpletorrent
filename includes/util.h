#ifndef UTIL_HEADER
#define UTIL_HEADER

#include "parser.h"

#define PEER_ID_LENGTH 20
#define HASHED_PIECE_LENGTH 20

char *peer_id;
void out(struct parse_item *item);
void echo(char *string, int str_length);
unsigned int hash(char *data, int data_len);
int file_size(char *filename);
char *generate_string(int string_length);
long get_torrent_file_size();
void create_directory(char *dir);
unsigned char *get_info_hash();
int get_piece_size();
#endif