MODULE    = secur32.dll
IMPORTLIB = secur32
UNIXLIB   = secur32.so
IMPORTS   = advapi32
DELAYIMPORTS = crypt32
UNIX_CFLAGS  = $(GNUTLS_CFLAGS)

SOURCES = \
	lsa.c \
	negotiate.c \
	schannel.c \
	schannel_gnutls.c \
	secur32.c \
	thunks.c \
	wrapper.c
