
DEPDIR := .deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

LIBSRC = zmdm.c crctab.c
LIBOBJ = zmdm.o crctab.o
SRC = unixterm.c unixfile.c zmrx.c zmtx.c
OBJ = unixterm.o unixfile.o


CFLAGS := -Wall -Werror -std=c99 -D_DEFAULT_SOURCE -O2

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) -c

ALL = libzmdm.a zmtx zmrx

all: $(DEPDIR) $(ALL)

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
