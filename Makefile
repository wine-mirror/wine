CFLAGS=-g -DDEBUG_RESOURCE -I./

######################################################################
# FILES:
#
#	Be very careful if you change the order of the files listed
# here.  if1632.o must linked first to guarrantee that it sits at a low
# enough address.  I intend to change this requirement someday, but
# for now live with it.
#
DLL_LENGTH=256

BUILDOBJS=dll_kernel.o dll_user.o dll_gdi.o dll_unixlib.o \
	  dll_win87em.o dll_shell.o \
	  dll_kernel_tab.o dll_user_tab.o dll_gdi_tab.o dll_unixlib_tab.o \
	  dll_win87em_tab.o dll_shell_tab.o

MUST_BE_LINKED_FIRST=if1632.o $(BUILDOBJS)

OBJS=$(MUST_BE_LINKED_FIRST) \
	callback.o dump.o global.o heap.o ldt.o kernel.o relay.o resource.o \
	selector.o message.o user.o wine.o class.o win.o widgets.o event.o xt.o

TARGET=wine
LIBS=-L. -L/usr/X386/lib -lldt -lXt -lX11

all: $(TARGET)

clean:
	rm -f *.o *~ *.s dll_* *.a

ci:
	ci Makefile README *.c *.h *.S build-spec.txt *.spec

$(TARGET): $(OBJS) libldt.a
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

build: build.c
	cc -g -o build build.c

libldt.a: ldtlib.c
	$(CC) -O6 -c ldtlib.c
	ar rcs libldt.a ldtlib.o

dll_kernel.S dll_kernel_tab.c: build kernel.spec
	build kernel.spec

dll_user.S dll_user_tab.c: build user.spec
	build user.spec

dll_gdi.S dll_gdi_tab.c: build gdi.spec
	build gdi.spec

dll_unixlib.S dll_unixlib_tab.c: build unixlib.spec
	build unixlib.spec

dll_win87em.S dll_win87em_tab.c: build win87em.spec
	build win87em.spec

dll_shell.S dll_shell_tab.c: build shell.spec
	build shell.spec

wintcl.o: wintcl.c windows.h
	cc -c -I. -g wintcl.c
