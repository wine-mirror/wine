CFLAGS=$(COPTS) $(DEBUGOPTS) -I../include

OBJS=kernel.o user.o xt.o rect.o file.o sound.o emulate.o \
     keyboard.o profile.o lstr.o exec.o message.o int1a.o int21.o \
     dos_fs.o comm.o

default: misc.o

misc.o: $(OBJS)
	$(LD) -r -o misc.o $(OBJS)

clean:
	rm -f *.o *~ *.s dll_* *.a *#

depend:
	$(CC) $(CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
