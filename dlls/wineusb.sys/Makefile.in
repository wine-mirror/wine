MODULE    = wineusb.sys
UNIXLIB   = wineusb.so
IMPORTS   = ntoskrnl
UNIX_LIBS    = $(USB_LIBS) $(PTHREAD_LIBS)
UNIX_CFLAGS  = $(USB_CFLAGS)

EXTRADLLFLAGS = -Wl,--subsystem,native

SOURCES = \
	unixlib.c \
	wineusb.c \
	wineusb.rc
