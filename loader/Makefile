CFLAGS=$(COPTS) $(DEBUGOPTS) -I../include

OBJS=dump.o files.o ldt.o ldtlib.o resource.o selector.o signal.o int1a.o \
     int21.o wine.o library.o

default: loader.o

loader.o: $(OBJS)
	$(LD) -r -o loader.o $(OBJS)

clean:
	rm -f *.o *~ *.s dll_* *.a *#

depend:
	$(CC) $(CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
