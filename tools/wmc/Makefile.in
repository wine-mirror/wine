PROGRAMS = wmc
UNIX_LIBS = $(GETTEXTPO_LIBS)

SOURCES = \
	lang.c \
	mcl.c \
	mcy.y \
	po.c \
	utils.c \
	wmc.c \
	wmc.man.in \
	write.c

wmc_EXTRADEFS = -DBINDIR="\"${bindir}\"" -DDATADIR="\"${datadir}\""

INSTALL_DEV = $(PROGRAMS)
