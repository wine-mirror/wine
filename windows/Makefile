CFLAGS=$(COPTS) $(DEBUGOPTS) -I$(INCLUDE_DIR)

OBJS=class.o dc.o dce.o event.o message.o win.o timer.o graphics.o \
    	clipping.o mapping.o painting.o keyboard.o utility.o

default: windows.o

windows.o: $(OBJS)
	$(LD) -r -o windows.o $(OBJS)

clean:
	rm -f *.o *~ *.s dll_* *.a

depend:
	$(CC) $(CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
