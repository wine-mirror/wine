MODULE  = winebth.sys
IMPORTS = ntoskrnl uuid
UNIXLIB = winebth.so
UNIX_CFLAGS = $(DBUS_CFLAGS)

EXTRADLLFLAGS = -Wl,--subsystem,native

SOURCES = \
	dbus.c \
	unixlib.c \
	winebluetooth.c \
	winebth.c \
	winebth.rc
