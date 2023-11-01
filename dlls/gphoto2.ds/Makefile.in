MODULE    = gphoto2.ds
UNIXLIB   = gphoto2.so
IMPORTS   = $(JPEG_PE_LIBS) comctl32 user32 gdi32 advapi32
EXTRAINCL = $(JPEG_PE_CFLAGS)
UNIX_LIBS    = $(GPHOTO2_LIBS) $(GPHOTO2_PORT_LIBS)
UNIX_CFLAGS  = $(GPHOTO2_CFLAGS) $(GPHOTO2_PORT_CFLAGS)

SOURCES = \
	capability.c \
	ds_image.c \
	gphoto2.rc \
	gphoto2_main.c \
	ui.c \
	unixlib.c
