
DEPDIR := .deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

SRC = zmdm.c crctab.c unixterm.c unixfile.c zmrx.c zmtx.c
OBJ = zmdm.o crctab.o unixterm.o unixfile.o


CFLAGS := -Wall -Werror -std=c99 -D_DEFAULT_SOURCE -O2

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) -c

ALL = zmtx zmrx

all: $(DEPDIR) $(ALL)

zmrx: $(OBJ) zmrx.o
	$(CC) $(CFLAGS) $? -o zmrx

zmtx: $(OBJ) zmtx.o
	$(CC) $(CFLAGS) $? -o zmtx

%.o : %.c $(DEPDIR)/%d | $(DEPDIR)
	$(COMPILE.c) $<

$(DEPDIR):
	@mkdir -p $@

DEPFILES := $(SRC:%.c=$(DEPDIR)/%.d)
$(DEPFILES):

include $(wildcard $(DEPFILES))

clean:
	rm -f *.o $(ALL)
