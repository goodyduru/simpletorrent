#include <stdio.h>
#include <stdlib.h>

#define MAX_LINE = 2000

int main(int argc, char *argv[]) {
    FILE *fp;
    char *file_name = "./won.torrent";
    char *prog = argv[0];
    unsigned char buffer[10];
    if ( (fp = fopen(file_name, "rb")) == NULL ) {
        fprintf(stderr, "%s: can't open %s", prog, file_name);
        exit(1);
    }

    fread(buffer, sizeof(buffer), 1, fp);

    for ( int i = 0; i < 10; i++ ) {
        printf("%c", buffer[i]);
    }
    printf("\n");
    return 0;
}