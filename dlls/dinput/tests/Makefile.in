TESTDLL   = dinput.dll
IMPORTS   = dinput dinput8 ole32 version user32 advapi32 hid uuid crypt32 newdev setupapi wintrust winmm

driver_bus_IMPORTS = winecrt0 ntoskrnl hal
driver_bus_EXTRADLLFLAGS = -nodefaultlibs -nostartfiles -Wl,--subsystem,native
driver_hid_IMPORTS = winecrt0 ntoskrnl hal hidclass
driver_hid_EXTRADLLFLAGS = -nodefaultlibs -nostartfiles -Wl,--subsystem,native
driver_hid_poll_IMPORTS = winecrt0 ntoskrnl hal hidclass
driver_hid_poll_EXTRADLLFLAGS = -nodefaultlibs -nostartfiles -Wl,--subsystem,native

SOURCES = \
	device8.c \
	dinput.c \
	driver_bus.c \
	driver_hid.c \
	driver_hid_poll.c \
	force_feedback.c \
	hid.c \
	hotplug.c \
	joystick8.c
