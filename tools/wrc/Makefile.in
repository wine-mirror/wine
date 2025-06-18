PROGRAMS = wrc
UNIX_LIBS = $(GETTEXTPO_LIBS)

SOURCES = \
	genres.c \
	newstruc.c \
	parser.l \
	parser.y \
	po.c \
	ppl.l \
	ppy.y \
	utils.c \
	wpp.c \
	wrc.c \
	wrc.man.in

wrc_EXTRADEFS = -DBINDIR="\"${bindir}\"" -DDATADIR="\"${datadir}\"" -DINCLUDEDIR="\"${includedir}\""

INSTALL_DEV = $(PROGRAMS)
