CFLAGS=$(COPTS) $(DEBUGOPTS) -I$(INCLUDE_DIR)

OBJS=bitmap.o brush.o font.o gdiobj.o palette.o pen.o dib.o region.o \
	text.o dcvalues.o clipping.o bitblt.o linedda.o

default: objects.o

objects.o: $(OBJS)
	$(LD) -r -o objects.o $(OBJS)

clean:
	rm -f *.o *~ *.s dll_* *.a

depend:
	$(CC) $(CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
