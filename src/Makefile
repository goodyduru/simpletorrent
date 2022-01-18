IDIR=../includes
CC=gcc
CFLAGS=-I$(IDIR)

ODIR=obj
LDIR=../lib

_DEPS = parser.h util.h raw.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = simpletorrent.o parser.o util.o raw.o
OBJ = $(patsubst %, $(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

simpletorrent: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~