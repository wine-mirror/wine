CFLAGS=$(COPTS) $(DEBUGOPTS) -I$(INCLUDE_DIR)

OBJS=dos.o kernel.o user.o xt.o rect.o

default: misc.o

misc.o: $(OBJS)
	$(LD) -r -o misc.o $(OBJS)

clean:
	rm -f *.o *~ *.s dll_* *.a

depend:
	$(CC) $(CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
