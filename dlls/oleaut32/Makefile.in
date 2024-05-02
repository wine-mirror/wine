EXTRADEFS = -D_OLEAUT32_
MODULE    = oleaut32.dll
IMPORTLIB = oleaut32
IMPORTS   = uuid ole32 rpcrt4 user32 gdi32 advapi32
DELAYIMPORTS = comctl32 urlmon windowscodecs

SOURCES = \
	connpt.c \
	dispatch.c \
	hash.c \
	oleaut.c \
	oleaut32.rc \
	oleaut32_oaidl.idl \
	oleaut32_ocidl.idl \
	olefont.c \
	olepicture.c \
	olepropframe.c \
	recinfo.c \
	safearray.c \
	thunks.c \
	typelib.c \
	usrmarshal.c \
	varformat.c \
	variant.c \
	vartype.c

dlldata_EXTRADEFS = -DENTRY_PREFIX=OLEAUTPS_ -DPROXY_DELEGATION -DWINE_REGISTER_DLL -DPROXY_CLSID=CLSID_PSFactoryBuffer
