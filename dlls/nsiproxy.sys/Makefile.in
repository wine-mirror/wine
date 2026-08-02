MODULE    = nsiproxy.sys
UNIXLIB   = nsiproxy.so
IMPORTS   = ntoskrnl
UNIX_LIBS = $(PROCSTAT_LIBS)

EXTRADLLFLAGS = -Wl,--subsystem,native

SOURCES = \
	device.c \
	icmp_echo.c \
	ip.c \
	ndis.c \
	nsi.c \
	tcp.c \
	udp.c
