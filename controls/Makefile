CFLAGS=$(COPTS) $(DEBUGOPTS) -I$(INCLUDE_DIR)

OBJS = menu.o caption.o widgets.o button.o \
	scroll.o listbox.o combo.o static.o

default: controls.o

controls.o: $(OBJS)
	$(LD) -r -o controls.o $(OBJS)

clean:
	rm -f *.o *~ *.s dll_* *.a *#

depend:
	$(CC) $(CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
