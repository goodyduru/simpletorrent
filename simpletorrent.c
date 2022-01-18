#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "util.h"
#include "raw.h"

int main(int argc, char *argv[]) {
    FILE *fp;
    char *file_name = "./won.torrent";
    char *prog = argv[0];
    int size;
    int i = 0;
    struct attr *head = (struct attr *) malloc(sizeof(struct attr));
    struct attr *p = (struct attr *) malloc(sizeof(struct attr));
    if ( (fp = fopen(file_name, "rb")) == NULL ) {
        fprintf(stderr, "%s: can't open %s", prog, file_name);
        exit(1);
    }
    size = file_size(fp);
    char buffer[size];

    fread(buffer, sizeof(buffer), 1, fp);
    for ( int i = 0; i < size; i++ ) {
        printf("%c", buffer[i]);
    }
    printf("\n");
    i = 0;
    head->next = p;
    p = parse_dict(head, buffer, &i);
    free(p);
    while ( head->next != NULL ) {
        out(head);
        head = head->next;
    }
    struct str *result = get_raw_content(buffer, 0, "info");
    for ( int i = 0; i < result->length; i++ ) {
        printf("%c", result->data[i]);
    }
    return 0;
}