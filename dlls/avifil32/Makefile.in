MODULE    = avifil32.dll
IMPORTLIB = avifil32
IMPORTS   = uuid msacm32 msvfw32 winmm ole32 user32 advapi32 rpcrt4

SOURCES = \
	acmstream.c \
	api.c \
	avifil32.idl \
	avifil32.rc \
	avifile.c \
	editstream.c \
	extrachunk.c \
	factory.c \
	getframe.c \
	icmstream.c \
	tmpfile.c \
	wavfile.c

dlldata_EXTRADEFS = -DENTRY_PREFIX=avifil32_
