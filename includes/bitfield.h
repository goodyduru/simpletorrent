//Bitarray definition. Thanks to http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html
#define SetBit(A,k)     ( A[(k/8)] |= (1 << (k%8)) )
#define ClearBit(A,k)   ( A[(k/8)] &= ~(1 << (k%8)) )
#define TestBit(A,k)    ( A[(k/8)] & (1 << (k%8)) )

char *create_bitarray(int length);
void bitarray_to_string(char array[], int array_length, char *result);
void string_to_bitarray(char *bitstring, int bitstring_length, char result[]);
unsigned char reverse_bit(unsigned char b);