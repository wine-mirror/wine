MODULE    = opengl32.dll
UNIXLIB   = opengl32.so
EXTRADEFS = -D_OPENGL32_
IMPORTLIB = opengl32
IMPORTS   = user32 gdi32 advapi32 win32u
DELAYIMPORTS = glu32
UNIX_LIBS    = -lwin32u

EXTRADLLFLAGS = -Wl,--image-base,0x7a800000

SOURCES = \
	thunks.c \
	unix_thunks.c \
	unix_wgl.c \
	version.rc \
	wgl.c
