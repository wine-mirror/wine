MODULE    = localspl.dll
UNIXLIB   = localspl.so
IMPORTS   = spoolss user32 advapi32
UNIX_CFLAGS  = $(CUPS_CFLAGS)
UNIX_LIBS    = $(APPLICATIONSERVICES_LIBS)

EXTRADLLFLAGS = -Wb,--prefer-native

SOURCES = \
	cups.c \
	localmon.c \
	localspl.rc \
	provider.c
