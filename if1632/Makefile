CFLAGS=$(COPTS) $(DEBUGOPTS) -I$(INCLUDE_DIR)

BUILDOBJS=dll_kernel.o dll_user.o dll_gdi.o dll_unixlib.o \
	  dll_win87em.o dll_shell.o dll_sound.o dll_keyboard.o\
	  dll_kernel_tab.o dll_user_tab.o dll_gdi_tab.o dll_unixlib_tab.o \
	  dll_win87em_tab.o dll_shell_tab.o \
	  dll_sound_tab.o dll_keyboard_tab.o

MUST_BE_LINKED_FIRST=call.o $(BUILDOBJS)

OBJS=$(MUST_BE_LINKED_FIRST) callback.o relay.o

default: if1632.o

if1632.o: $(OBJS)
	$(LD) -r -o if1632.o $(OBJS)

clean:
	rm -f *.o *~ *.s dll_* *.a *#

dll_kernel.S dll_kernel_tab.c: ../tools/build kernel.spec
	../tools/build kernel.spec

dll_user.S dll_user_tab.c: ../tools/build user.spec
	../tools/build user.spec

dll_gdi.S dll_gdi_tab.c: ../tools/build gdi.spec
	../tools/build gdi.spec

dll_unixlib.S dll_unixlib_tab.c: ../tools/build unixlib.spec
	../tools/build unixlib.spec

dll_win87em.S dll_win87em_tab.c: ../tools/build win87em.spec
	../tools/build win87em.spec

dll_shell.S dll_shell_tab.c: ../tools/build shell.spec
	../tools/build shell.spec

dll_sound.S dll_sound_tab.c: ../tools/build sound.spec
	../tools/build sound.spec

dll_keyboard.S dll_keyboard_tab.c: ../tools/build keyboard.spec
	../tools/build keyboard.spec

depend:
	$(CC) $(CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif




