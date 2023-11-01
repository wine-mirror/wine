MODULE    = oleacc.dll
IMPORTLIB = oleacc
IMPORTS   = uuid oleaut32 ole32 user32 rpcrt4

SOURCES = \
	client.c \
	main.c \
	oleacc.rc \
	oleacc_classes.idl \
	propservice.c \
	window.c

dlldata_EXTRADEFS = -DENTRY_PREFIX=OLEACC_ -DPROXY_DELEGATION -DWINE_REGISTER_DLL
