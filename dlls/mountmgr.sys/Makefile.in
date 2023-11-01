MODULE    = mountmgr.sys
UNIXLIB   = mountmgr.so
IMPORTS   = uuid advapi32 ntoskrnl
DELAYIMPORTS = user32
UNIX_CFLAGS  = $(DBUS_CFLAGS) $(HAL_CFLAGS)
UNIX_LIBS    = $(DISKARBITRATION_LIBS) $(SYSTEMCONFIGURATION_LIBS) $(CORESERVICES_LIBS) $(SECURITY_LIBS)

EXTRADLLFLAGS = -Wl,--subsystem,native

SOURCES = \
	cred.c \
	dbus.c \
	device.c \
	diskarb.c \
	mountmgr.c \
	unixlib.c
