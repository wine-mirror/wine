MODULE    = winebus.sys
UNIXLIB   = winebus.so
IMPORTS   = ntoskrnl hidparse
UNIX_LIBS    = $(IOKIT_LIBS) $(UDEV_LIBS) $(PTHREAD_LIBS) $(INOTIFY_LIBS)
UNIX_CFLAGS  = $(UDEV_CFLAGS) $(SDL2_CFLAGS)

EXTRADLLFLAGS = -Wl,--subsystem,native

SOURCES = \
	bus_iohid.c \
	bus_sdl.c \
	bus_udev.c \
	hid.c \
	main.c \
	unixlib.c \
	winebus.rc
