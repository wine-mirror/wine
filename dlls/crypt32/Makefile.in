EXTRADEFS = -D_CRYPT32_
MODULE    = crypt32.dll
UNIXLIB   = crypt32.so
IMPORTLIB = crypt32
IMPORTS   = user32 advapi32 bcrypt
DELAYIMPORTS = cryptnet
UNIX_LIBS    = $(SECURITY_LIBS)
UNIX_CFLAGS  = $(GNUTLS_CFLAGS)

SOURCES = \
	base64.c \
	cert.c \
	chain.c \
	collectionstore.c \
	context.c \
	crl.c \
	crypt32.rc \
	ctl.c \
	decode.c \
	encode.c \
	filestore.c \
	main.c \
	message.c \
	msg.c \
	object.c \
	oid.c \
	pfx.c \
	proplist.c \
	protectdata.c \
	provstore.c \
	regstore.c \
	rootstore.c \
	serialize.c \
	sip.c \
	store.c \
	str.c \
	unixlib.c
