build: build.c
	cc -O2 -o build build.c

clean:
	rm -f *.o *~

depend:
	$(CC) $(CFLAGS) -M *.c > .depend

dummy:

#
# Dependency lists
#
ifeq (.depend,$(wildcard .depend))
include .depend
endif