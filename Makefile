######################################################################
# These variables are inherited by the sub-makefiles
DEBUGOPTS=
COPTS=-O2 -m486
INCLUDE_DIR=include
LDFLAGS=

######################################################################
# These definitions are for the top level
TARGET=wine
LIBS=-L. -L/usr/X386/lib -lXext -lXaw -lXt -lXmu -lX11 -lm
OBJS=if1632/if1632.o controls/controls.o loader/loader.o \
	memory/memory.o misc/misc.o objects/objects.o windows/windows.o debugger/debugger.o
SUBDIRS=if1632 controls loader memory misc objects windows debugger
TAGFILES=if1632/{*.c,*.S} controls/{*.c,*.S} loader/{*.c,*.S} \
	memory/{*.c,*.S} misc/{*.c,*.S} objects/{*.c,*.S} \
	windows/{*.c,*.S} debugger/{*.c,*.S}

all: $(TARGET)

dummy:

tags:
	etags $(TAGFILES)

clean:
	rm -f *~ *.o *#
	@for i in tools $(SUBDIRS); do (cd $$i && $(MAKE) clean) || exit; done

$(TARGET): dummy
	@for i in tools $(SUBDIRS); \
	do (cd $$i && echo $$i && $(MAKE) INCLUDE_DIR=../$(INCLUDE_DIR) \
	COPTS="$(COPTS)" DEBUGOPTS="$(DEBUGOPTS)") || exit; done
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

depend:
	@for i in tools $(SUBDIRS); \
	     do (cd $$i && echo $$i && \
	     $(MAKE) INCLUDE_DIR=../$(INCLUDE_DIR) depend) \
	     || exit; done
