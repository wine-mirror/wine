MODULE    = sti.dll
IMPORTLIB = sti
IMPORTS   = uuid ole32 oleaut32 rpcrt4 advapi32

EXTRADLLFLAGS = -Wb,--prefer-native

SOURCES = \
	sti.c \
	sti_main.c \
	sti_wia.idl

dlldata_EXTRADEFS = -DENTRY_PREFIX=STI_ -DPROXY_DELEGATION -DWINE_REGISTER_DLL
