MODULE    = wineandroid.drv
UNIXLIB   = wineandroid.so
IMPORTS   = ntoskrnl
UNIX_LIBS = -lwin32u $(PTHREAD_LIBS)

SOURCES = \
	build.gradle.in \
	device.c \
	dllmain.c \
	init.c \
	keyboard.c \
	opengl.c \
	window.c \
	wine.svg

EXTRA_TARGETS = wine-debug.apk
