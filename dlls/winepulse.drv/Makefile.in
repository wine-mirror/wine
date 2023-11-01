MODULE    = winepulse.drv
UNIXLIB   = winepulse.so
IMPORTS   = dxguid uuid winmm user32 advapi32 ole32
UNIX_LIBS    = $(PULSE_LIBS) $(PTHREAD_LIBS) -lm
UNIX_CFLAGS  = $(PULSE_CFLAGS)

SOURCES = \
	mmdevdrv.c \
	pulse.c
