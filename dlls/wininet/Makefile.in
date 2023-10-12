EXTRADEFS = -D_WININET_INTERNAL_
MODULE    = wininet.dll
IMPORTLIB = wininet
IMPORTS   = $(ZLIB_PE_LIBS) mpr shlwapi shell32 user32 ws2_32 advapi32
EXTRAINCL = $(ZLIB_PE_CFLAGS)
DELAYIMPORTS = secur32 crypt32 cryptui dhcpcsvc iphlpapi

SOURCES = \
	cookie.c \
	dialogs.c \
	ftp.c \
	gopher.c \
	http.c \
	internet.c \
	netconnection.c \
	urlcache.c \
	utility.c \
	wininet.rc \
	winineterror.mc
