CFLAGS=$(COPTS) $(DEBUGOPTS) -I$(INCLUDE_DIR)

OBJS=global.o heap.o

default: memory.o

memory.o: $(OBJS)
	$(LD) -r -o memory.o $(OBJS)

clean:
	rm -f *.o *~ *.s dll_* *.a *#

depend:
	$(CC) $(CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
