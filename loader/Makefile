CFLAGS=$(COPTS) $(DEBUGOPTS) -I$(INCLUDE_DIR)

OBJS=dump.o ldt.o ldtlib.o resource.o selector.o signal.o wine.o

default: loader.o

loader.o: $(OBJS)
	$(LD) -r -o loader.o $(OBJS)

clean:
	rm -f *.o *~ *.s dll_* *.a

depend:
	$(CC) $(CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
