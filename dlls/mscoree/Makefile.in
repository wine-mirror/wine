MODULE    = mscoree.dll
IMPORTS   = dbghelp uuid shell32 advapi32 ole32 oleaut32 shlwapi

EXTRADLLFLAGS = -Wb,--prefer-native

SOURCES = \
	assembly.c \
	config.c \
	cordebug.c \
	corruntimehost.c \
	metadata.c \
	metahost.c \
	mscoree_classes.idl \
	mscoree_main.c

metahost_EXTRADEFS = -DINSTALL_DATADIR="\"${datadir}\""
