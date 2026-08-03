EXTRADEFS = -D_OLE32_
MODULE    = ole32.dll
IMPORTLIB = ole32
IMPORTS   = uuid advapi32 user32 gdi32 combase rpcrt4 kernelbase coml2
DELAYIMPORTS = oleaut32

SOURCES = \
	antimoniker.c \
	bindctx.c \
	classmoniker.c \
	clipboard.c \
	comcat.c \
	compobj.c \
	compositemoniker.c \
	datacache.c \
	dcom.idl \
	defaulthandler.c \
	dictionary.c \
	drag_copy.svg \
	drag_link.svg \
	drag_move.svg \
	filelockbytes.c \
	filemoniker.c \
	ftmarshal.c \
	git.c \
	irot.idl \
	itemmoniker.c \
	marshal.c \
	moniker.c \
	nodrop.svg \
	ole2.c \
	ole2stubs.c \
	ole32_main.c \
	ole32_objidl.idl \
	ole32_oleidl.idl \
	ole32_unknwn.idl \
	ole32res.rc \
	oleobj.c \
	oleproxy.c \
	pointermoniker.c \
	stg_prop.c \
	stg_stream.c \
	storage32.c \
	usrmarshal.c

dlldata_EXTRADEFS = -DENTRY_PREFIX=OLE32_ -DPROXY_CLSID=CLSID_PSFactoryBuffer -DWINE_REGISTER_DLL
