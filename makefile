
LIBSRC = zmdm.c crctab.c
LIBOBJ = zmdm.o crctab.o
SRC = unixterm.c unixfile.c zmrx.c zmtx.c
OBJ = unixterm.o unixfile.o


CFLAGS := -DDEBUG -Wall -Werror -std=c99 -D_DEFAULT_SOURCE -O0 -g

COMPILE.c = $(CC) $(CFLAGS) -c

ALL = libzmdm.a zmtx zmrx

all: $(ALL)


libzmdm.a: $(LIBOBJ)
	ar -crs $@ $?

zmrx: $(LIBOBJ) $(OBJ) zmrx.o
	$(CC) $(CFLAGS) $? -o zmrx

zmtx: $(LIBOBJ) $(OBJ) zmtx.o
	$(CC) $(CFLAGS) $? -o zmtx

%.o : %.c
	$(COMPILE.c) $<

clean:
	rm -f *.o $(ALL)
