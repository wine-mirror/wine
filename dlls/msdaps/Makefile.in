MODULE    = msdaps.dll
IMPORTS   = uuid oleaut32 ole32 rpcrt4 advapi32

SOURCES = \
	main.c \
	msdaps.idl \
	row_server.c \
	row_server.idl \
	usrmarshal.c

dlldata_EXTRADEFS  = -DWINE_REGISTER_DLL -DPROXY_CLSID=CLSID_PSFactoryBuffer -DENTRY_PREFIX=msdaps_
i386_EXTRAIDLFLAGS = --packing=2
arm_EXTRAIDLFLAGS  = --packing=2
