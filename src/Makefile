IDIR=../includes
CC=gcc
CFLAGS=-I$(IDIR)

OPENSSL_IDIR = /usr/local/opt/openssl@3/include
OPENSSL_IFLAG = -I$(OPENSSL_IDIR)

ODIR=obj
LDIR=../lib

LIBS=-L/usr/local/Cellar/openssl@3/3.0.1/lib -lssl -lcrypto

_DEPS = parser.h util.h raw.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = simpletorrent.o parser.o util.o raw.o
OBJ = $(patsubst %, $(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

simpletorrent: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(OPENSSL_IFLAG) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~