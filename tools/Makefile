# tools/Makefile.  Generated from Makefile.in by configure.      -*-Makefile-*-

prefix          = /usr/local
exec_prefix     = ${prefix}
bindir          = ${exec_prefix}/bin
libdir          = ${exec_prefix}/lib64
datarootdir     = ${prefix}/share
datadir         = ${datarootdir}
infodir         = ${datarootdir}/info
mandir          = ${datarootdir}/man
fontdir         = $(datadir)/wine/fonts
includedir      = ${prefix}/include/wine
dlldir          = ${exec_prefix}/lib64/wine
fakedlldir      = $(dlldir)/fakedlls
top_srcdir      = ..
top_builddir    = ..
srcdir          = .

SHELL           = /bin/sh
RM              = rm -f
MV              = mv
CC              = gcc
CROSSCC         = 
CFLAGS          = -g -O2 -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0
CPPFLAGS        = 
EXTRACFLAGS     = -Wall -pipe -fno-strict-aliasing -Wdeclaration-after-statement -Wempty-body -Wignored-qualifiers -Wstrict-prototypes -Wtype-limits -Wunused-but-set-parameter -Wwrite-strings -Wpointer-arith -Wlogical-op -gdwarf-2 -gstrict-dwarf
TARGETFLAGS     = -m64
LIBS            = 
BISON           = bison
FLEX            = flex
EXEEXT          = 
OBJEXT          = o
DLLEXT          = .so
FAKEEXT         = $(DLLEXT:.so=.fake)
TOOLSEXT        = 
IMPLIBEXT       = def
DLLTOOL         = 
AR              = ar
ARFLAGS         = rc
RANLIB          = ranlib
LN_S            = ln -s
TOOLSDIR        = $(top_builddir)
LDFLAGS         = 
PRELINK         = false
LINT            = 
LINTFLAGS       = 
FONTFORGE       = false
RSVG            = false
CONVERT         = false
ICOTOOL         = false
MSGFMT          = false
CROSSTARGET     = 
LINGUAS         = 
ALL_MO_FILES    = $(LINGUAS:%=../po/%.mo)
PORCFLAGS       = 
CROSSAR         = $(CROSSTARGET)-ar
CROSSRANLIB     = $(CROSSTARGET)-ranlib
MKINSTALLDIRS   = $(top_srcdir)/tools/mkinstalldirs -m 755
WINAPI_CHECK    = $(top_srcdir)/tools/winapi/winapi_check
BUILDIMAGE      = $(top_srcdir)/tools/buildimage
C2MAN           = $(top_srcdir)/tools/c2man.pl
RUNTEST         = $(top_srcdir)/tools/runtest
MAKECTESTS      = $(TOOLSDIR)/tools/make_ctests$(TOOLSEXT)
MAKEXFTMPL      = $(TOOLSDIR)/tools/make_xftmpl$(TOOLSEXT)
MAKEDEP         = $(TOOLSDIR)/tools/makedep$(TOOLSEXT)
RELPATH         = $(TOOLSDIR)/tools/relpath$(TOOLSEXT)
SFNT2FNT        = $(TOOLSDIR)/tools/sfnt2fnt$(TOOLSEXT)
WIDL            = $(TOOLSDIR)/tools/widl/widl$(TOOLSEXT)
WINEBUILD       = $(TOOLSDIR)/tools/winebuild/winebuild$(TOOLSEXT)
WMC             = $(TOOLSDIR)/tools/wmc/wmc$(TOOLSEXT)
WRC             = $(TOOLSDIR)/tools/wrc/wrc$(TOOLSEXT)
LIBPORT         = $(top_builddir)/libs/port/libwine_port.a
LIBWPP          = $(top_builddir)/libs/wpp/libwpp.a
LIBWINE         = -L$(top_builddir)/libs/wine -lwine
WINEGCC         = $(TOOLSDIR)/tools/winegcc/winegcc $(TARGETFLAGS) -B$(TOOLSDIR)/tools/winebuild --sysroot=$(top_builddir) -fasynchronous-unwind-tables
CROSSWINEGCC    = $(TOOLSDIR)/tools/winegcc/winegcc $(CROSSTARGET:%=-b %) -B$(TOOLSDIR)/tools/winebuild --sysroot=$(top_builddir) --lib-suffix=.cross.a
LDPATH          = LD_LIBRARY_PATH="$(TOOLSDIR)/libs/wine:$$LD_LIBRARY_PATH"
LDRPATH_INSTALL = -Wl,--rpath,\$$ORIGIN/`$(RELPATH) $(bindir) $(libdir)` -Wl,--enable-new-dtags
LDRPATH_LOCAL   = -Wl,--rpath,\$$ORIGIN/$(top_builddir)/libs/wine
INSTALL         = /usr/bin/install -c $(INSTALL_FLAGS)
INSTALL_PROGRAM = ${INSTALL} $(INSTALL_PROGRAM_FLAGS)
INSTALL_SCRIPT  = ${INSTALL} $(INSTALL_SCRIPT_FLAGS)
INSTALL_DATA    = ${INSTALL} -m 644 $(INSTALL_DATA_FLAGS)
prog_manext     = 1
api_manext      = 3w
conf_manext     = 5


all:

# End of common header
PROGRAMS = \
	makedep \
	make_xftmpl

SOURCES = \
	make_xftmpl.c \
	makedep.c

makedep_OBJS = makedep.o
make_xftmpl_OBJS = make_xftmpl.o
