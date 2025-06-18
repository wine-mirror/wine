MODULE    = wmphoto.dll
IMPORTS   = $(JXR_PE_LIBS) windowscodecs uuid ole32 oleaut32 propsys rpcrt4 shlwapi
EXTRAINCL = $(JXR_PE_CFLAGS)
PARENTSRC = ../windowscodecs

EXTRADLLFLAGS = -Wb,--prefer-native

SOURCES = \
	decoder.c \
	jxrlib.c \
	main.c \
	wincodecs_common.c \
	wmphoto.idl
