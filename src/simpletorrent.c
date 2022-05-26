#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bitfield.h"
#include "parser.h"
#include "util.h"
#include "raw.h"
#include "url.h"
#include "tracker.h"
#include "peers_handler.h"
#include "pieces_handler.h"
#include "peer_message.h"
#include "test.h"

void initialize_tables(char *file_name, char *prog) {
    FILE *fp;
    int size;
    int i = 0;
    if ( (fp = fopen(file_name, "rb")) == NULL ) {
        fprintf(stderr, "%s: can't open %s", prog, file_name);
        exit(1);
    }
    size = file_size(file_name);
    char buffer[size];

    fread(buffer, 1, sizeof buffer, fp);
    build_raw_table(buffer, 0, raw_table, RAW_TABLE_SIZE);
    parse(raw_table, RAW_TABLE_SIZE, decode_table, TORRENT_TABLE_SIZE);
    get_info_hash();
    send_tracker_request();
    init_peers();
    generate_pieces();
}

void display_progress() {
    static char last_log_line[1024];
    static float percentage_completed = -1.0;
    int number_of_pieces, number_of_peers;
    float progress = 0.0, percentage = 0.0;
    char current_log_line[1024];
    number_of_pieces = get_piece_size();
    for ( int i = 0; i < number_of_pieces; i++ ) {
        for ( int j = 0; j < pieces[i]->number_of_blocks; j++ ) {
            if ( pieces[i]->block_list[j]->state == FULL ) {
                progress += pieces[i]->block_list[j]->block_size;
            }
        }
    }

    if ( progress == percentage_completed ) return;

    number_of_peers = unchoked_peers_count();
    percentage = (progress/get_torrent_file_size()) * 100;
    sprintf(current_log_line, "Number of connected peers: %d\tPercentage completed: %.2f\t%d/%d pieces\n",
            number_of_peers, percentage, complete_pieces, number_of_pieces);
    if ( strcmp(current_log_line, last_log_line) != 0 ) printf("%s", current_log_line);
    strcpy(last_log_line, current_log_line);
    percentage_completed = progress;
}

void start_download(char *file_name, char *prog) {
    int number_of_pieces, i;
    struct peer **start_peers, *peer;
    struct piece *single_piece;
    struct block *single_block;
    char *message;
    pthread_t th;
    initialize_tables(file_name, prog);
    start_peers = get_peers();
    add_peers(start_peers, get_number_original_peers());
    free(start_peers);
    number_of_pieces = get_piece_size();
    printf("Number of pieces: %d\n", number_of_pieces);
    printf("Number of peers: %d\n", peer_count);
    pthread_create(&th, NULL, connect_to_peers, NULL);
    while ( !all_pieces_completed() ) {
        if ( !has_unchoked_peers() ) {
            printf("No piece\n");
            sleep(1);
            continue;
        }
        for ( i = 0; i < number_of_pieces; i++ ) {
            single_piece = pieces[i];

            if ( single_piece->is_full ) continue;

            peer = get_random_peer_having_piece(i);

            if ( peer == NULL ) continue;

            update_block_status(single_piece);

            single_block = get_empty_block(single_piece);
            if ( single_block == NULL ) continue;

            message = malloc(17);
            generate_request_message(message, i, single_block->block_offset, single_block->block_size);
            send_to_peer(peer, message, 17);
            free(message);
        }

        display_progress();
        sleep(1);
    }
    printf("File downloaded!");
    display_progress();
}

int main(int argc, char *argv[]) {
    char *file_name = "./three.busy.debras.s02e01.1080p.web.h264-whosnext[eztv.re].mkv.torrent";
    char *prog = argv[0];
    start_download(file_name, prog);
    //test(file_name);
}