#include <stdio.h>
#include <stdlib.h>
#include <openssl/sha.h>

#include "parser.h"
#include "util.h"
#include "raw.h"
#include "url.h"

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
    struct str *result = get_raw_content(buffer, 0, "announce");
    for ( int i = 0; i < result->length; i++ ) {
        printf("%c", result->data[i]);
    }
    printf("\n");

    result = get_raw_content(buffer, 0, "info");
    for ( int i = 0; i < result->length; i++ ) {
        printf("%c", result->data[i]);
    }
    printf("\n");
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*) result->data, result->length, hash);
    for ( int i = 0; i < SHA_DIGEST_LENGTH; i++ ) {
        printf("%c", hash[i]);
    }
    printf("\n");

    struct url *res = parse_url("http://bt1.archive.org:6969/announce", 36);
    printf("%s\t%d\t%s\n", res->host_name, res->port, res->path);

    char *url_encoded = urlencode("http://bt1.archive.org:6969/announce", 36);
    printf("%s\n", url_encoded);
    return 0;
}