#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "bitfield.h"

char *create_bitarray(int length) {
    int bit_array_length = (int)ceil(length/8.0);
    char *bitarray = (char *) malloc(bit_array_length);
    for ( int f = 0; f < bit_array_length; f++ ) {
        bitarray[f] = 0;
    }
    return bitarray;
}

void bitarray_to_string(char array[], int array_length, char *result) {
    unsigned char bit;
    for ( int i = 0; i < array_length; i++ ) {
        bit = reverse_bit((unsigned char) array[i]);
        memcpy(result+i, &bit, 1);
    }
}

void string_to_bitarray(char *bitstring, int bitstring_length, char result[]) {
    unsigned char rev;
    for ( int i = 0; i < bitstring_length; i++ ) {
        memcpy(&rev, bitstring+i, 1);
        result[i] = reverse_bit(rev);
    }
}

//https://stackoverflow.com/a/2602885
unsigned char reverse_bit(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}
