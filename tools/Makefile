CFLAGS=$(COPTS) $(DEBUGOPTS) -I$(INCLUDE_DIR)

build: build.c
	cc $(CFLAGS) -o build build.c

clean:
	rm -f *.o *~ build *#

depend:
	$(CC) $(CFLAGS) -M *.c > .depend

dummy:

#
# Dependency lists
#
ifeq (.depend,$(wildcard .depend))
include .depend
endif
