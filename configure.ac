dnl Process this file with autoconf to produce a configure script.
dnl Original author: Michael Patra
dnl See ChangeLog file for detailed change history.

m4_define(WINE_VERSION,regexp(m4_include(VERSION),[version \([-.0-9A-Za-z]+\)],[\1]))

AC_PREREQ(2.53b)
AC_INIT([Wine],WINE_VERSION,[wine-devel@winehq.org])
AC_CONFIG_SRCDIR(server/atom.c)
AC_CONFIG_HEADERS(include/config.h)
AC_CONFIG_AUX_DIR(tools)

dnl **** Command-line arguments ****

AC_ARG_ENABLE(win16, AC_HELP_STRING([--disable-win16],[do not include Win16 support]))
AC_ARG_ENABLE(debug, AC_HELP_STRING([--disable-debug],[compile out all debugging messages]))
AC_ARG_ENABLE(trace, AC_HELP_STRING([--disable-trace],[compile out TRACE messages]))
AC_ARG_ENABLE(win64, AC_HELP_STRING([--enable-win64], [build a Win64 emulator on AMD64 (won't run Win32 binaries)]))

AC_ARG_WITH(opengl,    AC_HELP_STRING([--without-opengl],[do not use OpenGL]))
AC_ARG_WITH(curses,    AC_HELP_STRING([--without-curses],[do not use curses]))
AC_ARG_WITH(wine-tools,AC_HELP_STRING([--with-wine-tools=<dir>],[use Wine tools from directory <dir>]))

AC_SUBST(DLLDEFS,"")
if test "x$enable_debug" = "xno"
then
  DLLDEFS="$DLLDEFS -DWINE_NO_DEBUG_MSGS"
fi
if test "x$enable_trace" = "xno" -o "x$enable_debug" = "xno"
then
  DLLDEFS="$DLLDEFS -DWINE_NO_TRACE_MSGS"
fi

AC_CANONICAL_HOST
case $host in
  x86_64*linux*)
    if test "x$enable_win64" != "xyes"
    then
      test -n "$CC" || CC="gcc -m32"
      test -n "$LD" || LD="ld -m elf_i386"
      test -n "$AS" || AS="as --32"
      host_cpu="i386"
    fi
    ;;
esac

dnl enable_win16 defaults to yes on x86, to no on other CPUs
case $host_cpu in
  *i[[3456789]]86*)
    if test "x$enable_win16" != "xno" 
    then
      enable_win16="yes"
    fi
    ;;
esac
   
AC_SUBST(WIN16_FILES,"\$(WIN16_FILES)")
AC_SUBST(WIN16_INSTALL,"\$(WIN16_INSTALL)")
if test "x$enable_win16" != "xyes"
then
  WIN16_FILES=""
  WIN16_INSTALL=""
fi

dnl **** Check for some programs ****

AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_CXX
dnl We can't use AC_PROG_CPP for winegcc, it uses by default $(CC) -E
AC_CHECK_TOOL(CPPBIN,cpp,cpp)

AC_CACHE_CHECK([for the directory containing the Wine tools], wine_cv_toolsdir,
  [if test -z "$with_wine_tools"; then
     if test "$cross_compiling" = "yes"; then
       AC_MSG_ERROR([you must use the --with-wine-tools option when cross-compiling.])
     else
       wine_cv_toolsdir="\$(TOPOBJDIR)"
     fi
   elif test -d "$with_wine_tools/tools/winebuild"; then
     case $with_wine_tools in
       /*) wine_cv_toolsdir="$with_wine_tools" ;;
       *)  wine_cv_toolsdir="\$(TOPOBJDIR)/$with_wine_tools" ;;
     esac
   else
     AC_MSG_ERROR([could not find Wine tools in $with_wine_tools.])
   fi])
AC_SUBST(TOOLSDIR,$wine_cv_toolsdir)

AC_PATH_XTRA
AC_PROG_LEX

dnl **** Just additional warning checks, since AC_PROG just sets 'lex' even
dnl **** without one present.
AC_CHECK_PROGS(XLEX,$LEX flex lex,none)
if test "$XLEX" = "none"
then
  AC_MSG_ERROR([no suitable lex found. Please install the 'flex' package.])
fi

dnl Check for bison
AC_CHECK_PROGS(BISON,bison,none)
if test "$BISON" = "none"
then
  AC_MSG_ERROR([no suitable bison found. Please install the 'bison' package.])
fi

AC_CHECK_TOOLS(AS,[gas as],as)
AC_CHECK_TOOL(LD,ld,ld)
AC_CHECK_TOOL(AR,ar,ar)
AC_PROG_RANLIB
AC_CHECK_TOOL(STRIP,strip,strip)
AC_CHECK_TOOL(WINDRES,windres,false)
AC_PROG_LN_S
WINE_PROG_LN
AC_PROG_EGREP
AC_PATH_PROG(LDCONFIG, ldconfig, true, [/sbin /usr/sbin $PATH])
AC_PROG_INSTALL
dnl Prepend src dir to install path dir if it's a relative path
case "$INSTALL" in
  [[\\/$]]* | ?:[[\\/]]* ) ;;
  *)  INSTALL="\\\$(TOPSRCDIR)/$INSTALL" ;;
esac

dnl Check for lint
AC_CHECK_PROGS(LINT, lclint lint)
if test "$LINT" = "lint"
then
  LINTFLAGS="$LINTFLAGS -errchk=%all,no%longptr64 -errhdr=%user -Ncheck=macro -Nlevel=4"
  dnl LINTFLAGS='-D_SIZE_T "-Dsize_t=unsigned long" -errchk=longptr64'
fi
AC_SUBST(LINT)
AC_SUBST(LINTFLAGS)

dnl Check for various programs
AC_CHECK_PROGS(FONTFORGE, fontforge, false)
AC_CHECK_PROGS(PKG_CONFIG, pkg-config, false)

case $host_cpu in
  *i[[3456789]]86*)
    AC_CHECK_PROGS(PRELINK, prelink, false, [/sbin /usr/sbin $PATH])
    ;;
esac

dnl **** Check for some libraries ****

dnl Check for -li386 for NetBSD and OpenBSD
AC_CHECK_LIB(i386,i386_set_ldt)
dnl Check for -lossaudio for NetBSD
AC_CHECK_LIB(ossaudio,_oss_ioctl)
dnl Check for -lnsl for Solaris
AC_CHECK_FUNCS(gethostbyname,,AC_CHECK_LIB(nsl,gethostbyname))
dnl Check for -lsocket for Solaris
AC_CHECK_FUNCS(connect,,AC_CHECK_LIB(socket,connect))
dnl Check for -lresolv for Solaris
AC_CHECK_FUNCS(inet_aton,,AC_CHECK_LIB(resolv,inet_aton))
dnl Check for -lxpg4 for FreeBSD
AC_CHECK_LIB(xpg4,_xpg4_setrunelocale)
dnl Check for -lpoll for Mac OS X/Darwin
AC_CHECK_LIB(poll,poll)
dnl Check for -lresolv for Mac OS X/Darwin
AC_CHECK_LIB(resolv,res_9_init)
dnl Check for -lpthread
AC_CHECK_LIB(pthread,pthread_create,AC_SUBST(LIBPTHREAD,"-lpthread"))

AC_SUBST(XLIB,"")
AC_SUBST(XFILES,"")
AC_SUBST(OPENGLFILES,"")
AC_SUBST(GLU32FILES,"")
AC_SUBST(OPENGL_LIBS,"")

dnl **** Check for header files ****

AC_CHECK_HEADERS(\
	IOKit/IOKitLib.h \
	alsa/asoundlib.h \
	arpa/inet.h \
	arpa/nameser.h \
	asm/types.h \
	capi20.h \
	cups/cups.h \
	curses.h \
	direct.h \
	dlfcn.h \
	elf.h \
	float.h \
	fontconfig/fontconfig.h \
	getopt.h \
	gif_lib.h \
	ieeefp.h \
	io.h \
	jack/jack.h \
	jpeglib.h \
	lber.h \
	lcms.h \
	lcms/lcms.h \
	ldap.h \
	libaudioio.h \
	link.h \
	linux/capi.h \
	linux/cdrom.h \
	linux/compiler.h \
	linux/hdreg.h \
	linux/input.h \
	linux/ioctl.h \
	linux/joystick.h \
	linux/major.h \
	linux/param.h \
	linux/serial.h \
	linux/ucdrom.h \
	mach/machine.h \
	machine/cpu.h \
	machine/limits.h \
	machine/soundcard.h \
	mntent.h \
	ncurses.h \
	netdb.h \
	netinet/in.h \
	netinet/in_systm.h \
	netinet/tcp.h \
	netinet/tcp_fsm.h \
	openssl/err.h \
	openssl/ssl.h \
	poll.h \
	process.h \
	pthread.h \
	pwd.h \
	regex.h \
	sched.h \
	scsi/scsi.h \
	scsi/scsi_ioctl.h \
	scsi/sg.h \
	soundcard.h \
	stdint.h \
	strings.h \
	sys/asoundlib.h \
	sys/cdio.h \
	sys/elf32.h \
	sys/epoll.h \
	sys/errno.h \
	sys/exec_elf.h \
	sys/filio.h \
	sys/ioctl.h \
	sys/ipc.h \
	sys/limits.h \
	sys/link.h \
	sys/lwp.h \
	sys/mman.h \
	sys/modem.h \
	sys/msg.h \
	sys/mtio.h \
	sys/param.h \
	sys/poll.h \
	sys/ptrace.h \
	sys/reg.h \
	sys/scsiio.h \
	sys/shm.h \
	sys/signal.h \
	sys/socket.h \
	sys/sockio.h \
	sys/soundcard.h \
	sys/statvfs.h \
	sys/strtio.h \
	sys/syscall.h \
	sys/sysctl.h \
	sys/time.h \
	sys/times.h \
	sys/uio.h \
	sys/un.h \
	sys/vm86.h \
	sys/wait.h \
	syscall.h \
	termios.h \
	unicode/ubidi.h \
	unistd.h \
	utime.h \
	valgrind/memcheck.h
)
AC_HEADER_STAT()

dnl **** Check for X11 ****

if test "$have_x" = "yes"
then
    XLIB="-lXext -lX11"
    ac_save_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $X_CFLAGS"

    dnl *** All of the following tests require X11/Xlib.h
    AC_CHECK_HEADERS([X11/Xlib.h \
                      X11/XKBlib.h \
                      X11/Xutil.h \
                      X11/extensions/shape.h \
                      X11/extensions/XInput.h \
                      X11/extensions/XShm.h \
                      X11/extensions/Xrandr.h \
                      X11/extensions/Xrender.h \
                      X11/extensions/xf86dga.h \
                      X11/extensions/xf86vmode.h],,,
[#ifdef HAVE_X11_XLIB_H
# include <X11/Xlib.h>
#endif
#ifdef HAVE_X11_XUTIL_H
# include <X11/Xutil.h>
#endif])

        dnl *** Check for X keyboard extension
        if test "$ac_cv_header_X11_XKBlib_h" = "yes"
        then
              AC_CHECK_LIB(X11, XkbQueryExtension,
              AC_DEFINE(HAVE_XKB, 1, [Define if you have the XKB extension]),,
              $X_LIBS -lXext -lX11 $X_EXTRA_LIBS)
        fi

        dnl *** Check for X Shm extension
        if test "$ac_cv_header_X11_extensions_XShm_h" = "yes"
        then
              AC_CHECK_LIB(Xext, XShmQueryExtension,
              AC_DEFINE(HAVE_LIBXXSHM, 1, [Define if you have the X Shm extension]),,
              $X_LIBS -lXext -lX11 $X_EXTRA_LIBS)
        fi

        dnl *** Check for X shape extension
        if test "$ac_cv_header_X11_extensions_shape_h" = "yes"
        then
              AC_CHECK_LIB(Xext,XShapeQueryExtension,
	      AC_DEFINE(HAVE_LIBXSHAPE, 1, [Define if you have the X Shape extension]),,
	      $X_LIBS -lXext -lX11 $X_EXTRA_LIBS)
        fi

        dnl *** Check for XFree86 DGA / DGA 2.0 extension
        if test "$ac_cv_header_X11_extensions_xf86dga_h" = "yes"
        then
              AC_CHECK_LIB(Xxf86dga, XDGAQueryExtension,
                [ AC_DEFINE(HAVE_LIBXXF86DGA2, 1,
                            [Define if you have the Xxf86dga library version 2])
                  X_PRE_LIBS="$X_PRE_LIBS -lXxf86dga"
                ],,
                $X_LIBS -lXext -lX11 $X_EXTRA_LIBS)
        fi

        dnl *** Check for XFree86 VMODE extension
        if test "$ac_cv_header_X11_extensions_xf86vmode_h" = "yes"
        then
                AC_CHECK_LIB(Xxf86vm, XF86VidModeQueryExtension,
                  [ AC_DEFINE(HAVE_LIBXXF86VM, 1, [Define if you have the Xxf86vm library])
                     X_PRE_LIBS="$X_PRE_LIBS -lXxf86vm"
                  ],,
                  $X_LIBS -lXext -lX11 $X_EXTRA_LIBS)
        fi

        dnl *** Check for X RandR extension
        if test "$ac_cv_header_X11_extensions_Xrandr_h" = "yes"
        then
                AC_TRY_COMPILE([#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>],[static typeof(XRRSetScreenConfigAndRate) * func;],
                  [AC_DEFINE(HAVE_LIBXRANDR, 1, [Define if you have the Xrandr library])])
        fi

        dnl *** Check for Transform functions in Xrender
        if test "$ac_cv_header_X11_extensions_Xrender_h" = "yes"
        then
              AC_CHECK_LIB(Xrender, XRenderSetPictureTransform,
                [AC_DEFINE(HAVE_XRENDERSETPICTURETRANSFORM, 1,
                 [Define if Xrender has the XRenderSetPictureTransform function])],,
                $X_LIBS -lXext -lX11 $X_EXTRA_LIBS)
        fi
    dnl *** End of X11/Xlib.h check

    dnl Check for the presence of OpenGL
    if test "x$with_opengl" != "xno"
    then
	AC_CHECK_HEADERS(GL/gl.h GL/glx.h GL/glext.h,,,
[#ifdef HAVE_GL_GLX_H
# include <GL/glx.h>
#endif])
	if test "$ac_cv_header_GL_gl_h" = "yes" -a "$ac_cv_header_GL_glx_h" = "yes"
	then
	    dnl Check for some problems due to old Mesa versions
	    AC_CACHE_CHECK([for up-to-date OpenGL version], wine_cv_opengl_version_OK,
	      AC_TRY_COMPILE(
		[#include <GL/gl.h>],
		[GLenum test = GL_UNSIGNED_SHORT_5_6_5;],
		[wine_cv_opengl_version_OK="yes"],
		[wine_cv_opengl_version_OK="no"]
	      )
            )

	    if test "$wine_cv_opengl_version_OK" = "yes"
            then
		dnl Check for the presence of the library
		AC_CHECK_LIB(GL,glXCreateContext,
			     OPENGL_LIBS="-lGL"
			     ,,
			     $X_LIBS -lXext -lX11 -lm $X_EXTRA_LIBS)

		if test "$ac_cv_lib_GL_glXCreateContext" = "yes"
		then
			OPENGLFILES='$(OPENGLFILES)'
			AC_DEFINE(HAVE_OPENGL, 1, [Define if OpenGL is present on the system])
		else
                    if test -f /usr/X11R6/lib/libGL.a
                    then 
                       AC_MSG_ERROR([/usr/X11R6/lib/libGL.a is present on your system.
This prevents linking to OpenGL. Delete the file and restart configure.])
                    fi
		fi

	        dnl Check for GLU32 library.
		AC_CHECK_LIB(GLU,gluLookAt,
			     [OPENGL_LIBS="$OPENGL_LIBS -lGLU"
			     GLU32FILES='$(GLU32FILES)']
			     ,,
			     $OPENGL_LIBS $X_LIBS $X_PRE_LIBS -lXext -lX11 -lm $X_EXTRA_LIBS
		)
	     fi

             dnl Check for glut32 library.
             AC_CHECK_LIB(glut,glutMainLoop,
                       [AC_SUBST(GLUT_LIBS,"-lglut -lXmu -lXi")
                        AC_SUBST(GLUT32FILES,'$(GLUT32FILES)')],,
                        $OPENGL_LIBS $X_LIBS $X_PRE_LIBS -lXmu -lXi -lX11 $X_EXTRA_LIBS)
	 fi
    fi

    dnl **** Check for NAS ****
    AC_SUBST(NASLIBS,"")
    AC_CHECK_HEADERS(audio/audiolib.h,
         [AC_CHECK_HEADERS(audio/soundlib.h,,,[#include <audio/audiolib.h>])
          AC_CHECK_LIB(audio,AuCreateFlow,
                       [AC_DEFINE(HAVE_NAS,1,[Define if you have NAS including devel headers])
                        NASLIBS="-laudio -lXt $X_LIBS -lXext -lX11 $X_EXTRA_LIBS"],,
                       [-lXt $X_LIBS -lXext -lX11 $X_EXTRA_LIBS])])

    CPPFLAGS="$ac_save_CPPFLAGS"
    XFILES='$(XFILES)'
else
    XLIB=""
    X_CFLAGS=""
    X_LIBS=""
fi

dnl **** Check for libxml2 ****

AC_SUBST(XML2LIBS,"")
AC_SUBST(XML2INCL,"")
AC_SUBST(XSLTLIBS,"")
AC_SUBST(XSLTINCL,"")
if test "$PKG_CONFIG" != "false"
then
    ac_save_CPPFLAGS="$CPPFLAGS"
    ac_xml_libs="`$PKG_CONFIG --libs libxml-2.0`"
    ac_xml_cflags="`$PKG_CONFIG --cflags libxml-2.0`"
    CPPFLAGS="$CPPFLAGS $ac_xml_cflags"
    AC_CHECK_HEADERS(libxml/parser.h,
        [AC_CHECK_LIB(xml2, xmlParseMemory,
            [AC_DEFINE(HAVE_LIBXML2, 1, [Define if you have the libxml2 library])
             XML2LIBS="$ac_xml_libs"
             XML2INCL="$ac_xml_cflags"],,$ac_xml_libs)
         AC_CHECK_LIB(xml2, xmlReadMemory,
            [AC_DEFINE(HAVE_XMLREADMEMORY,1,[Define if libxml2 has the xmlReadMemory function])],,$ac_xml_libs)
        ])
    CPPFLAGS="$ac_save_CPPFLAGS"
    ac_xslt_libs="`$PKG_CONFIG --libs libxslt`"
    ac_xslt_cflags="`$PKG_CONFIG --cflags libxslt`"
    CPPFLAGS="$CPPFLAGS $ac_xslt_cflags"
    AC_CHECK_HEADERS([libxslt/pattern.h libxslt/transform.h],
        [AC_CHECK_LIB(xslt, xsltCompilePattern,
            [AC_DEFINE(HAVE_LIBXSLT, 1, [Define if you have the libxslt library])
             XSLTLIBS="$ac_xslt_libs"
             XSLTINCL="$ac_xslt_cflags"],,$ac_xslt_libs)
        ],,
[#ifdef HAVE_LIBXSLT_PATTERN_H
# include <libxslt/pattern.h>
#endif])
    CPPFLAGS="$ac_save_CPPFLAGS"
fi

dnl **** Check which curses lib to use ***
CURSESLIBS=""
if test "x$with_curses" != "xno"
then
    if test "$ac_cv_header_ncurses_h" = "yes"
    then
        AC_CHECK_LIB(ncurses,waddch,
            [AC_DEFINE(HAVE_LIBNCURSES, 1, [Define if you have the ncurses library (-lncurses)])
             CURSESLIBS="-lncurses"])
    elif test "$ac_cv_header_curses_h" = "yes"
    then
        AC_CHECK_LIB(curses,waddch,
            [AC_DEFINE(HAVE_LIBCURSES, 1, [Define if you have the curses library (-lcurses)])
             CURSESLIBS="-lcurses"])
    fi
fi
AC_SUBST(CURSESLIBS)

dnl **** Check for SANE ****
AC_CHECK_PROG(sane_devel,sane-config,sane-config,no)
AC_SUBST(SANELIBS,"")
AC_SUBST(SANEINCL,"")
if test "$sane_devel" != "no"
then
    SANELIBS="`$sane_devel --libs`"
    SANEINCL="`$sane_devel --cflags`"
    ac_save_CPPFLAGS="$CPPFLAGS"
    ac_save_LIBS="$LIBS"
    CPPFLAGS="$CPPFLAGS $SANEINCL"
    LIBS="$LIBS $SANELIBS"
    AC_CHECK_HEADER(sane/sane.h,
                    [AC_CHECK_LIB(sane,sane_open,
                                  [AC_DEFINE(HAVE_SANE, 1, [Define if we have SANE development environment])],
                                  [SANELIBS=""
                                  SANEINCL=""])],
                    [SANELIBS=""
                    SANEINCL=""])
    LIBS="$ac_save_LIBS"
    CPPFLAGS="$ac_save_CPPFLAGS"
fi

dnl **** Check for the ICU library ****
if test "$ac_cv_header_unicode_ubidi_h" = "yes"
then
    saved_libs="$LIBS"
    ICU_LIB_DIR="${ICU_LIB_DIR-/usr/lib}"
    ICUUC_LIB="${ICUUC_LIB-$ICU_LIB_DIR/libsicuuc.a}"
    ICUDATA_LIB="${ICUDATA_LIB-$ICU_LIB_DIR/libsicudata.a}"
    AC_MSG_CHECKING(whether can link with ICU libraries $ICUUC_LIB and $ICUDATA_LIB)
    LIBS="$LIBS $ICUUC_LIB $ICUDATA_LIB -lstdc++ -lgcc_s"
    AC_TRY_LINK([#include <unicode/ubidi.h>],[ubidi_open()],
                [AC_DEFINE(HAVE_ICU,1,[Define to 1 if the ICU libraries are installed])
                 AC_SUBST(ICULIBS,"$ICUUC_LIB $ICUDATA_LIB -lstdc++ -lgcc_s")
                 AC_MSG_RESULT(yes)],
                [AC_MSG_RESULT(no)])
    LIBS="$saved_libs"
fi

dnl **** Check for LittleCMS ***
AC_SUBST(LCMSLIBS,"")
if test "$ac_cv_header_lcms_h" = "yes" -o "$ac_cv_header_lcms_lcms_h" = "yes"
then
    AC_CHECK_LIB(lcms, cmsOpenProfileFromFile,
        [AC_DEFINE(HAVE_LCMS, 1, [Define if you have the LittleCMS development environment])
         LCMSLIBS="-llcms"])
fi

dnl **** Check for OpenLDAP ***
AC_SUBST(LDAPLIBS,"")
if test "$ac_cv_header_ldap_h" = "yes" -a "$ac_cv_header_lber_h" = "yes"
then
    AC_CHECK_LIB(ldap_r, ldap_initialize,
        [AC_CHECK_LIB(lber, ber_init,
            [AC_DEFINE(HAVE_LDAP, 1, [Define if you have the OpenLDAP development environment])
             LDAPLIBS="-lldap_r -llber"],,
             [$LIBPTHREAD])],,
             [$LIBPTHREAD])
    WINE_CHECK_LIB_FUNCS(\
	ldap_count_references \
	ldap_first_reference \
	ldap_next_reference \
	ldap_parse_reference,
        [$LDAPLIBS $LIBPTHREAD])
fi

dnl **** Check for FreeType 2 ****
AC_SUBST(FREETYPELIBS,"")
AC_SUBST(FREETYPEINCL,"")
AC_CHECK_LIB(freetype,FT_Init_FreeType,ft_lib=yes,ft_lib=no,$X_LIBS)
if test "$ft_lib" = "no"
then
    wine_cv_msg_freetype=no
else
    AC_CHECK_PROG(ft_devel,freetype-config,freetype-config,no)
    if test "$ft_devel" = "no"
    then
        AC_CHECK_PROG(ft_devel2,freetype2-config,freetype2-config,no)
        if test "$ft_devel2" = "freetype2-config"
	then
		ft_devel=$ft_devel2
	fi
    fi
    if test "$ft_devel" = "no"
    then
	wine_cv_msg_freetype=yes
    else
	FREETYPELIBS=`$ft_devel --libs`
	FREETYPEINCL=`$ft_devel --cflags`
	ac_save_CPPFLAGS="$CPPFLAGS"
	CPPFLAGS="$FREETYPEINCL $CPPFLAGS"
	AC_CHECK_HEADERS(ft2build.h \
                         freetype/freetype.h \
                         freetype/ftglyph.h \
                         freetype/tttables.h \
                         freetype/ftnames.h \
                         freetype/ftsnames.h \
                         freetype/ttnameid.h \
                         freetype/ftoutln.h \
                         freetype/ftwinfnt.h \
                         freetype/ftmodapi.h \
                         freetype/internal/sfnt.h,,,
                         [#if HAVE_FT2BUILD_H
                          #include <ft2build.h>
                          #endif])
	AC_TRY_CPP([#include <ft2build.h>
                    #include <freetype/fttrigon.h>],
                    [AC_DEFINE(HAVE_FREETYPE_FTTRIGON_H, 1,
          [Define if you have the <freetype/fttrigon.h> header file.])
                    wine_cv_fttrigon=yes],
                    wine_cv_fttrigon=no)
	CPPFLAGS="$ac_save_CPPFLAGS"
	dnl Check that we have at least freetype/freetype.h
	if test "$ac_cv_header_freetype_freetype_h" = "yes" -a "$wine_cv_fttrigon" = "yes"
	then
	    AC_DEFINE(HAVE_FREETYPE, 1, [Define if FreeType 2 is installed])
	    wine_cv_msg_freetype=no
	else
	    FREETYPELIBS=""
	    FREETYPEINCL=""
	    wine_cv_msg_freetype=yes
	fi
    fi
fi

dnl Only build the fonts dir if we have both freetype and fontforge
if test "$FONTFORGE" != "false" -a -n "$FREETYPELIBS"
then
  AC_SUBST(FONTSSUBDIRS,"fonts")
fi

dnl **** Check for parport (currently Linux only) ****
AC_CACHE_CHECK([for parport header/ppdev.h], ac_cv_c_ppdev,
 AC_TRY_COMPILE(
   [#include <linux/ppdev.h>],
   [ioctl (1,PPCLAIM,0)],
   [ac_cv_c_ppdev="yes"],
   [ac_cv_c_ppdev="no"])
 )
if test "$ac_cv_c_ppdev" = "yes"
then
    AC_DEFINE(HAVE_PPDEV, 1, [Define if we can use ppdev.h for parallel port access])
fi

dnl **** Check for va_copy ****
AC_CACHE_CHECK([for va_copy], ac_cv_c_va_copy,
 AC_TRY_LINK(
   [#include <stdarg.h>],
   [va_list ap1, ap2;
    va_copy(ap1,ap2);
   ],
   [ac_cv_c_va_copy="yes"],
   [ac_cv_c_va_copy="no"])
 )
if test "$ac_cv_c_va_copy" = "yes"
then
    AC_DEFINE(HAVE_VA_COPY, 1, [Define if we have va_copy])
fi
AC_CACHE_CHECK([for __va_copy], ac_cv_c___va_copy,
 AC_TRY_LINK(
   [#include <stdarg.h>],
   [va_list ap1, ap2;
    __va_copy(ap1,ap2);
   ],
   [ac_cv_c___va_copy="yes"],
   [ac_cv_c___va_copy="no"])
 )
if test "$ac_cv_c___va_copy" = "yes"
then
    AC_DEFINE(HAVE___VA_COPY, 1, [Define if we have __va_copy])
fi

dnl **** Check for sigsetjmp ****
AC_CACHE_CHECK([for sigsetjmp], ac_cv_c_sigsetjmp,
 AC_TRY_LINK(
   [#include <setjmp.h>],
   [sigjmp_buf buf;
    sigsetjmp( buf, 1 );
    siglongjmp( buf, 1 );],
   [ac_cv_c_sigsetjmp="yes"],
   [ac_cv_c_sigsetjmp="no"])
 )
if test "$ac_cv_c_sigsetjmp" = "yes"
then
    AC_DEFINE(HAVE_SIGSETJMP, 1, [Define to 1 if you have the sigsetjmp (and siglongjmp) function])
fi

dnl **** Check for pthread_rwlock_t ****
AC_CHECK_TYPES([pthread_rwlock_t, pthread_rwlockattr_t],,,[#define _GNU_SOURCE
#include <pthread.h>])

dnl **** Check for pthread functions ****
WINE_CHECK_LIB_FUNCS(\
        pthread_getattr_np \
        pthread_get_stackaddr_np \
        pthread_get_stacksize_np,
        [$LIBPTHREAD])

dnl **** Check for aRts Sound Server ****
AC_PATH_PROG(ARTSCCONFIG, artsc-config)
if test x$ARTSCCONFIG != x -a x$ARTSCCONFIG != x'"$ARTSCCONFIG"';
then
    ARTSC_CFLAGS=""
    for i in `$ARTSCCONFIG --cflags`
    do
      case "$i" in
        -I*) ARTSC_CFLAGS="$ARTSC_CFLAGS $i";;
      esac
    done
    ARTSC_LIBS=`$ARTSCCONFIG --libs`
    save_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS $ARTSC_CFLAGS"
    AC_CHECK_LIB(artsc,arts_init,
        [AC_TRY_COMPILE([#include <artsc.h>],[arts_stream_t stream;],
            [AC_SUBST(ARTSLIBS, $ARTSC_LIBS)
            AC_SUBST(ARTSINCL, $ARTSC_CFLAGS)
            AC_DEFINE(HAVE_ARTS, 1, [Define if you have ARTS sound server])])],,
            $ARTSC_LIBS)
    CFLAGS="$save_CFLAGS"
fi

dnl **** Check for EsounD ****
AC_PATH_PROG(ESDCONFIG, esd-config)
if test x$ESDCONFIG != x -a x$ESDCONFIG != x'"$ESDCONFIG"';
then
    ESD_CFLAGS=""
    for i in `$ESDCONFIG --cflags`
    do
      case "$i" in
        -I*) ESD_CFLAGS="$ESD_CFLAGS $i";;
      esac
    done
    ESD_LIBS=`$ESDCONFIG --libs`
    save_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS $ESD_CFLAGS"
    AC_CHECK_LIB(esd,esd_open_sound,
        [AC_SUBST(ESDLIBS, $ESD_LIBS)
         AC_SUBST(ESDINCL, $ESD_CFLAGS)
         AC_DEFINE(HAVE_ESD, 1, [Define if you have EsounD sound server])])
    CFLAGS="$save_CFLAGS"
fi

dnl **** Check for ALSA 1.x ****
AC_SUBST(ALSALIBS,"")
if test "$ac_cv_header_sys_asoundlib_h" = "yes" -o "$ac_cv_header_alsa_asoundlib_h" = "yes"
then
    AC_CHECK_LIB(asound,snd_pcm_hw_params_get_access,
        [AC_TRY_COMPILE([#ifdef HAVE_ALSA_ASOUNDLIB_H
#include <alsa/asoundlib.h>
#elif defined(HAVE_SYS_ASOUNDLIB_H)
#include <sys/asoundlib.h>
#endif],
                        [int ret = snd_pcm_hw_params_get_access(NULL, NULL)],
                        [AC_DEFINE(HAVE_ALSA,1,[Define if you have ALSA 1.x including devel headers])
                         ALSALIBS="-lasound"])])
fi

dnl **** Check for libaudioio (which can be used to get solaris audio support) ****

AC_SUBST(AUDIOIOLIBS,"")
if test "$ac_cv_header_libaudioio_h" = "yes"
then
    AC_CHECK_LIB(audioio,AudioIOGetVersion,
                  [AUDIOIOLIBS="-laudioio"
                   AC_DEFINE(HAVE_LIBAUDIOIO, 1, [Define if you have libaudioIO])])
fi

dnl **** Check for capi4linux ****

if test "$ac_cv_header_capi20_h" = "yes" -a "$ac_cv_header_linux_capi_h" = "yes"
then
    AC_CHECK_LIB(capi20,capi20_register,[AC_DEFINE(HAVE_CAPI4LINUX,1,[Define if you have capi4linux libs and headers])])
fi

dnl **** Check for gcc specific options ****

AC_SUBST(EXTRACFLAGS,"")
if test "x${GCC}" = "xyes"
then
  EXTRACFLAGS="-Wall -pipe"

  dnl Check for strength-reduce bug
  AC_CACHE_CHECK( [for gcc strength-reduce bug], ac_cv_c_gcc_strength_bug,
                  AC_TRY_RUN([
int	L[[4]] = {0,1,2,3};
int main(void) {
  static int Array[[3]];
  unsigned int B = 3;
  int i;
  for(i=0; i<B; i++) Array[[i]] = i - 3;
  for(i=0; i<4 - 1; i++) L[[i]] = L[[i + 1]];
  L[[i]] = 4;

  exit( Array[[1]] != -2 || L[[2]] != 3);
}],
    ac_cv_c_gcc_strength_bug="no",
    ac_cv_c_gcc_strength_bug="yes",
    ac_cv_c_gcc_strength_bug="yes") )
  if test "$ac_cv_c_gcc_strength_bug" = "yes"
  then
    EXTRACFLAGS="$EXTRACFLAGS -fno-strength-reduce"
  fi

  dnl Check for -fshort-wchar
  AC_CACHE_CHECK([for gcc -fshort-wchar support], ac_cv_c_gcc_fshort_wchar,
      [WINE_TRY_CFLAGS([-fshort-wchar],
                      ac_cv_c_gcc_fshort_wchar="yes",ac_cv_c_gcc_fshort_wchar="no")])
  if test "$ac_cv_c_gcc_fshort_wchar" = "yes"
  then
      AC_DEFINE(CC_FLAG_SHORT_WCHAR, "-fshort-wchar", [Specifies the compiler flag that forces a short wchar_t])
  fi

  dnl Check for -fno-strict-aliasing
  AC_CACHE_CHECK([for gcc -fno-strict-aliasing support], ac_cv_c_gcc_no_strict_aliasing,
      [WINE_TRY_CFLAGS([-fno-strict-aliasing],
                      ac_cv_c_gcc_no_strict_aliasing="yes",ac_cv_c_gcc_no_strict_aliasing="no")])
  if test "$ac_cv_c_gcc_no_strict_aliasing" = "yes"
  then
    EXTRACFLAGS="$EXTRACFLAGS -fno-strict-aliasing"
  fi

  dnl Check for -gstabs+ option
  AC_CACHE_CHECK([for gcc -gstabs+ support], ac_cv_c_gcc_gstabs,
      [WINE_TRY_CFLAGS([-gstabs+],ac_cv_c_gcc_gstabs="yes", ac_cv_c_gcc_gstabs="no")])
  if test "$ac_cv_c_gcc_gstabs" = "yes"
  then
    EXTRACFLAGS="$EXTRACFLAGS -gstabs+"
  fi

  dnl Check for -Wdeclaration-after-statement option
  AC_CACHE_CHECK([for gcc -Wdeclaration-after-statement support], ac_cv_c_gcc_decl_after_statement,
      [WINE_TRY_CFLAGS([-Wdeclaration-after-statement],ac_cv_c_gcc_decl_after_statement="yes", ac_cv_c_gcc_decl_after_statement="no")])
  if test "$ac_cv_c_gcc_decl_after_statement" = "yes"
  then
    EXTRACFLAGS="$EXTRACFLAGS -Wdeclaration-after-statement"
  fi

  dnl Check for noisy string.h
  saved_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS -Wpointer-arith -Werror"
  AC_CACHE_CHECK([for broken string.h that generates warnings], ac_cv_c_string_h_warnings,
      AC_TRY_COMPILE([#include <string.h>],[],
                     [ac_cv_c_string_h_warnings=no],[ac_cv_c_string_h_warnings=yes]))
  CFLAGS="$saved_CFLAGS"
  if test "$ac_cv_c_string_h_warnings" = "no"
  then
    EXTRACFLAGS="$EXTRACFLAGS -Wpointer-arith"
  fi

  AC_SUBST(BUILTINFLAG,"")
  saved_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS -Werror"
  AC_CACHE_CHECK([for builtin wchar inlines], ac_cv_c_builtin_wchar_ctype,
      AC_TRY_COMPILE([],
                     [int iswlower(unsigned short);],
                     [ac_cv_c_builtin_wchar_ctype=no],[ac_cv_c_builtin_wchar_ctype=yes]))
  CFLAGS="$saved_CFLAGS"
  if test "$ac_cv_c_builtin_wchar_ctype" = "yes"
  then
    BUILTINFLAG=""
    for builtin in \
      iswalnum iswalpha iswcntrl iswdigit iswgraph iswlower \
      iswprint iswpunct iswspace iswupper iswxdigit towlower towupper
    do
      BUILTINFLAG="$BUILTINFLAG -fno-builtin-$builtin"
    done
  fi
fi

dnl **** Check how to define a function in assembly code ****

AC_CACHE_CHECK([how to define a function in assembly code], ac_cv_asm_func_def,
  WINE_TRY_ASM_LINK(
      ["\t.globl _ac_test\n\t.def _ac_test; .scl 2; .type 32; .endef\n_ac_test:\t.long 0"],,,
      ac_cv_asm_func_def=".def",
    [WINE_TRY_ASM_LINK(["\t.globl _ac_test\n\t.type _ac_test,@function\n_ac_test:\t.long 0"],,,
      ac_cv_asm_func_def=".type @function",
    [WINE_TRY_ASM_LINK(["\t.globl _ac_test\n\t.type _ac_test,2\n_ac_test:\t.long 0"],,,
      ac_cv_asm_func_def=".type 2",
      ac_cv_asm_func_def="unknown")])]))

AH_TEMPLATE(__ASM_FUNC,[Define to a macro to generate an assembly function directive])
case "$ac_cv_asm_func_def" in
  ".def")
     AC_DEFINE([__ASM_FUNC(name)], [".def " __ASM_NAME(name) "; .scl 2; .type 32; .endef"]) ;;
  ".type @function")
     AC_DEFINE([__ASM_FUNC(name)], [".type " __ASM_NAME(name) ",@function"]) ;;
  ".type 2")
     AC_DEFINE([__ASM_FUNC(name)], [".type " __ASM_NAME(name) ",2"]) ;;
  *)
     AC_DEFINE([__ASM_FUNC(name)], [""]) ;;
esac

dnl **** Check for underscore on external symbols ****

AC_CACHE_CHECK([whether external symbols need an underscore prefix], ac_cv_c_extern_prefix,
    WINE_TRY_ASM_LINK([".globl _ac_test\n_ac_test:\t.long 0"],
                      [extern int ac_test;],
                      [if (ac_test) return 1],
                      ac_cv_c_extern_prefix="yes",ac_cv_c_extern_prefix="no"))

AH_TEMPLATE(__ASM_NAME,[Define to a macro to generate an assembly name from a C symbol])
if test "$ac_cv_c_extern_prefix" = "yes"
then
  AC_DEFINE([__ASM_NAME(name)], ["_" name])
else
  AC_DEFINE([__ASM_NAME(name)], [name])
fi

dnl **** Check for working dll ****

AC_SUBST(DLLEXT,"")
AC_SUBST(DLLFLAGS,"-D_REENTRANT")
AC_SUBST(DLLIBS,"")
AC_SUBST(LDSHARED,"")
AC_SUBST(LDDLLFLAGS,"")
AC_SUBST(LIBEXT,"so")
AC_SUBST(IMPLIBEXT,"def")

case $host_os in
  cygwin*|mingw32*)
    AC_CHECK_TOOL(DLLTOOL,dlltool,false)
    AC_CHECK_TOOL(DLLWRAP,dllwrap,false)
    if test "$DLLWRAP" = "false"; then
      LIBEXT="a"
    else
      dnl FIXME - check whether dllwrap works correctly...
      LIBEXT="dll"
    fi
    IMPLIBEXT="a"
    dnl We can't build 16-bit NE dlls
    WIN16_FILES=""
    WIN16_INSTALL=""
    ;;
  *)
    if test "$ac_cv_header_dlfcn_h" = "yes"
    then
        AC_CHECK_FUNCS(dlopen,,
            [AC_CHECK_LIB(dl,dlopen,
                         [AC_DEFINE(HAVE_DLOPEN,1,[Define if you have dlopen])
                          DLLIBS="-ldl"],
                         [LIBEXT="a"])])
        WINE_CHECK_LIB_FUNCS(dladdr,[$DLLIBS])
    else
        LIBEXT="a"
    fi

    if test "$LIBEXT" = "so"
    then
      DLLFLAGS="$DLLFLAGS -fPIC"
      DLLEXT=".so"
      AC_CACHE_CHECK([whether we can build a GNU style ELF dll], ac_cv_c_dll_gnuelf,
          [WINE_TRY_SHLIB_FLAGS([-fPIC -shared -Wl,-soname,conftest.so.1.0,-Bsymbolic],
                                ac_cv_c_dll_gnuelf="yes",ac_cv_c_dll_gnuelf="no")])
      if test "$ac_cv_c_dll_gnuelf" = "yes"
      then
        LDSHARED="\$(CC) -shared \$(SONAME:%=-Wl,-soname,%)"
        LDDLLFLAGS="-shared -Wl,-Bsymbolic"
        AC_CACHE_CHECK([whether the linker accepts -z defs], ac_cv_c_dll_zdefs,
          [WINE_TRY_CFLAGS([-fPIC -shared -Wl,-Bsymbolic,-z,defs],
                           ac_cv_c_dll_zdefs="yes",ac_cv_c_dll_zdefs="no")])
        if test "$ac_cv_c_dll_zdefs" = "yes"
        then
          LDDLLFLAGS="$LDDLLFLAGS,-z,defs"
        fi

        AC_CACHE_CHECK([whether the linker accepts -init and -fini], ac_cv_c_dll_init_fini,
          [WINE_TRY_CFLAGS([-fPIC -shared -Wl,-Bsymbolic,-init,__wine_spec_init,-fini,__wine_spec_fini],
                           ac_cv_c_dll_init_fini="yes",ac_cv_c_dll_init_fini="no")])
        if test "$ac_cv_c_dll_init_fini" = "yes"
        then
          LDDLLFLAGS="$LDDLLFLAGS,-init,__wine_spec_init,-fini,__wine_spec_fini"
        fi

        AC_CACHE_CHECK([whether the linker accepts version scripts], ac_cv_c_ld_version_scripts,
          [echo '{ global: *; };' >conftest.map
           WINE_TRY_CFLAGS([-fPIC -shared -Wl,--version-script=conftest.map],
                           ac_cv_c_ld_version_scripts="yes",ac_cv_c_ld_version_scripts="no")
           rm -f conftest.map])
        if test "$ac_cv_c_ld_version_scripts" = "yes"
        then
          LDSHARED="$LDSHARED \$(VERSCRIPT:%=-Wl,--version-script=%)"
        fi

        AC_CACHE_CHECK([whether the linker accepts --export-dynamic], ac_cv_c_export_dynamic,
          [WINE_TRY_CFLAGS([-fPIC -Wl,--export-dynamic],
                           ac_cv_c_export_dynamic="yes",ac_cv_c_export_dynamic="no")])
        if test "$ac_cv_c_export_dynamic" = "yes"
        then
          AC_SUBST(LDEXECFLAGS,["-Wl,--export-dynamic"])
        fi

        case $host_cpu in
          *i[[3456789]]86*)
            AC_CACHE_CHECK([whether we can relocate the executable to 0x7bf00000], ac_cv_ld_reloc_exec,
              [WINE_TRY_CFLAGS([-Wl,--section-start,.interp=0x7bf00400],
                               ac_cv_ld_reloc_exec="yes", ac_cv_ld_reloc_exec="no")])
            if test "$ac_cv_ld_reloc_exec" = "yes"
            then
              LDEXECFLAGS="$LDEXECFLAGS -Wl,--section-start,.interp=0x7bf00400"
            fi
            ;;
        esac

      else
        AC_CACHE_CHECK(whether we can build a UnixWare (Solaris) dll, ac_cv_c_dll_unixware,
            [WINE_TRY_SHLIB_FLAGS([-fPIC -Wl,-G,-h,conftest.so.1.0,-B,symbolic],
                                  ac_cv_c_dll_unixware="yes",ac_cv_c_dll_unixware="no")])
        if test "$ac_cv_c_dll_unixware" = "yes"
        then
          LDSHARED="\$(CC) -Wl,-G \$(SONAME:%=-Wl,-h,%)"
          LDDLLFLAGS="-Wl,-G,-B,symbolic"

        else
          AC_CACHE_CHECK(whether we can build a Mach-O (Mac OS X/Darwin) dll, ac_cv_c_dll_macho,
            [WINE_TRY_SHLIB_FLAGS([-bundle], ac_cv_c_dll_macho="yes", ac_cv_c_dll_macho="no")])
          if test "$ac_cv_c_dll_macho" = "yes"
          then
            LIBEXT="dylib"
            LDDLLFLAGS="-bundle"
            LDSHARED="\$(CC) -dynamiclib"
            STRIP="$STRIP -u -r"
            dnl declare needed frameworks
            AC_SUBST(COREFOUNDATIONLIB,"-framework CoreFoundation")
            AC_SUBST(IOKITLIB,"-framework IOKit -framework CoreFoundation")
            case $host_cpu in
              *i[[3456789]]86*)
                AC_SUBST(LDEXECFLAGS,"-image_base 0x7bf00000")
                ;;
              *powerpc*)
                CFLAGS="$CFLAGS -ffixed-r13"
                LDDLLFLAGS="$LDDLLFLAGS -read_only_relocs warning"
                dnl Relocate wine executable (FIXME: is this really needed?)
                AC_SUBST(LDEXECFLAGS,"-seg1addr 0x120000")
                AC_SUBST(LDLIBWINEFLAGS,"-seg1addr 0x140000")
                ;;
            esac

          else
            AC_CACHE_CHECK(whether we can build an HP-UX dll, ac_cv_c_dll_hpux,
              [WINE_TRY_SHLIB_FLAGS([-shared], ac_cv_c_dll_hpux="yes", ac_cv_c_dll_hpux="no")])
            if test "$ac_cv_c_dll_hpux" = "yes"
            then
              LIBEXT="sl"
              DLLEXT=".sl"
              LDDLLFLAGS="-shared -fPIC"
              LDSHARED="\$(CC) -shared"
            fi
          fi
        fi
      fi
    fi

    dnl Check for cross compiler to build test programs
    AC_SUBST(CROSSTEST,"")
    if test "$cross_compiling" = "no"
    then
      AC_CHECK_PROGS(CROSSCC,i586-mingw32msvc-gcc i386-mingw32msvc-gcc i386-mingw32-gcc mingw-gcc,false)
      AC_CHECK_PROGS(DLLTOOL,i586-mingw32msvc-dlltool i386-mingw32msvc-dlltool i386-mingw32-dlltool mingw-dlltool,false)
      AC_CHECK_PROGS(CROSSWINDRES,i586-mingw32msvc-windres i386-mingw32msvc-windres i386-mingw32-windres mingw-windres,false)
      if test "$CROSSCC" != "false"; then CROSSTEST="\$(CROSSTEST)"; fi
    fi
    ;;
esac

if test "$LIBEXT" = "a"; then
  AC_MSG_ERROR(
[could not find a way to build shared libraries.
It is currently not possible to build Wine without shared library
(.so) support to allow transparent switch between .so and .dll files.
If you are using Linux, you will need a newer binutils.]
)
fi

case $build_os in
  cygwin*|mingw32*)
    AC_SUBST(LDPATH,"PATH=\"\$(TOOLSDIR)/libs/unicode:\$\$PATH\"") ;;
  darwin*|macosx*)
    AC_SUBST(LDPATH,"DYLD_LIBRARY_PATH=\"\$(TOOLSDIR)/libs/unicode:\$\$DYLD_LIBRARY_PATH\"") ;;
  *)
    AC_SUBST(LDPATH,"LD_LIBRARY_PATH=\"\$(TOOLSDIR)/libs/unicode:\$\$LD_LIBRARY_PATH\"") ;;
esac

dnl Mingw needs explicit msvcrt for linking libwine and winsock for wininet
case $host_os in
  mingw32*)
    AC_SUBST(CRTLIBS,"-lmsvcrt")
    AC_SUBST(SOCKETLIBS,"-lws2_32")
    ;;
esac

case $host_os in
  linux* | k*bsd*-gnu)
    case $host_cpu in
      *i[[3456789]]86*) AC_SUBST(WINE_BINARIES,"wine-glibc wine-kthread wine-pthread wine-preloader") ;;
      *) AC_SUBST(WINE_BINARIES,"wine-glibc wine-kthread wine-pthread") ;;
    esac
    AC_SUBST(MAIN_BINARY,"wine-glibc")
    ;;
  darwin*)
    AC_SUBST(WINE_BINARIES,"wine-pthread")
    AC_SUBST(MAIN_BINARY,"wine-pthread")
    ;;
  *)
    AC_SUBST(WINE_BINARIES,"wine-kthread")
    AC_SUBST(MAIN_BINARY,"wine-kthread")
    ;;
esac

dnl **** Get the soname for libraries that we load dynamically ****

if test "$LIBEXT" = "so" -o "$LIBEXT" = "dylib"
then
  WINE_GET_SONAME(X11,XCreateWindow,[$X_LIBS $X_EXTRA_LIBS])
  WINE_GET_SONAME(Xext,XextCreateExtension,[$X_LIBS -lX11 $X_EXTRA_LIBS])
  WINE_GET_SONAME(Xi,XOpenDevice,[$X_LIBS -lXext -lX11 $X_EXTRA_LIBS])
  WINE_GET_SONAME(Xrender,XRenderQueryExtension,[$X_LIBS -lXext -lX11 $X_EXTRA_LIBS])
  WINE_GET_SONAME(Xrandr,XRRQueryExtension,[$X_LIBS -lXext -lX11 $X_EXTRA_LIBS])
  WINE_GET_SONAME(freetype,FT_Init_FreeType,[$X_LIBS])
  WINE_GET_SONAME(GL,glXQueryExtension,[$X_LIBS $X_EXTRA_LIBS])
  WINE_GET_SONAME(txc_dxtn,fetch_2d_texel_rgba_dxt1)
  WINE_GET_SONAME(cups,cupsGetDefault)
  WINE_GET_SONAME(jack,jack_client_new)
  WINE_GET_SONAME(fontconfig,FcInit)
  WINE_GET_SONAME(ssl,SSL_library_init)
  WINE_GET_SONAME(crypto,BIO_new_socket)
  WINE_GET_SONAME(ncurses,waddch)
  WINE_GET_SONAME(curses,waddch)
  WINE_GET_SONAME(jpeg,jpeg_start_decompress)
  WINE_GET_SONAME(ungif,DGifOpen)
  WINE_GET_SONAME(gif,DGifOpen)
  WINE_GET_SONAME(capi20,capi20_isinstalled)
fi


dnl **** Check for functions ****

AC_CHECK_FUNCS(\
	_lwp_create \
	_lwp_self \
	_pclose \
	_popen \
	_snprintf \
	_spawnvp \
	_stricmp \
	_strnicmp \
	_vsnprintf \
	asctime_r \
	chsize \
	clone \
	epoll_create \
	ffs \
	finite \
	fork \
	fpclass \
	fstatfs \
	fstatvfs \
	ftruncate \
	futimes \
	futimesat \
	getaddrinfo \
	getnetbyname \
	getopt_long \
	getpagesize \
	getprotobyname \
	getprotobynumber \
	getpwuid \
	getservbyport \
	gettid \
	gettimeofday \
	getuid \
	inet_network \
	lstat \
	memmove \
	mmap \
	pclose \
	popen \
	pread \
	pwrite \
	readlink \
	rfork \
	sched_yield \
	select \
	sendmsg \
	settimeofday \
	sigaltstack \
	sigprocmask \
	snprintf \
	spawnvp \
	statfs \
	statvfs \
	strcasecmp \
	strerror \
	strncasecmp \
	tcgetattr \
	timegm \
	usleep \
	vsnprintf \
	wait4 \
	waitpid \
)

dnl **** Checks for headers that depend on other ones ****

AC_CHECK_HEADERS([sys/mount.h sys/statfs.h sys/user.h sys/vfs.h],,,
    [#include <sys/types.h>
     #if HAVE_SYS_PARAM_H
     # include <sys/param.h>
     #endif])

AC_CHECK_HEADERS([net/if.h net/if_arp.h net/if_dl.h net/if_types.h net/route.h netipx/ipx.h],,,
    [#include <sys/types.h>
     #if HAVE_SYS_SOCKET_H
     # include <sys/socket.h>
     #endif])

AC_CHECK_HEADERS([resolv.h],,,
    [#include <sys/types.h>
     #if HAVE_SYS_SOCKET_H
     # include <sys/socket.h>
     #endif
     #if HAVE_NETINET_IN_H
     # include <netinet/in.h>
     #endif
     #if HAVE_ARPA_NAMESER_H
     # include <arpa/nameser.h>
     #endif])

AC_CHECK_HEADERS(ucontext.h,,,[#include <signal.h>])

AC_CHECK_HEADERS([linux/ipx.h linux/videodev.h],,,
[#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <sys/types.h>
#ifdef HAVE_ASM_TYPES_H
#include <asm/types.h>
#endif])

dnl **** Check for types ****

AC_C_CONST
AC_C_INLINE
AC_CHECK_TYPES([mode_t, off_t, pid_t, size_t, ssize_t, long long, fsblkcnt_t, fsfilcnt_t])
AC_CHECK_TYPES([sigset_t],,,[#include <signal.h>])
AC_CHECK_TYPES([request_sense],,,[#include <linux/cdrom.h>])

AC_CHECK_MEMBERS([struct ff_effect.direction],,,
[#ifdef HAVE_LINUX_INPUT_H
#include <linux/input.h>
#endif])

AC_CACHE_CHECK([for sigaddset],wine_cv_have_sigaddset,
               AC_TRY_LINK([#include <signal.h>],[sigset_t set; sigaddset(&set,SIGTERM);],
                           wine_cv_have_sigaddset=yes,wine_cv_have_sigaddset=no))
if test "$wine_cv_have_sigaddset" = "yes"
then
  AC_DEFINE(HAVE_SIGADDSET, 1, [Define if sigaddset is supported])
fi


AC_CACHE_CHECK([whether we can use re-entrant gethostbyname_r Linux style],
   	wine_cv_linux_gethostbyname_r_6,
	AC_TRY_LINK([
#include <netdb.h>
	], [
    char *name=NULL;
    struct hostent he;
    struct hostent *result;
    char *buf=NULL;
    int bufsize=0;
    int res,errnr;
    char *addr=NULL;
    int addrlen=0;
    int addrtype=0;
    res=gethostbyname_r(name,&he,buf,bufsize,&result,&errnr);
    res=gethostbyaddr_r(addr, addrlen, addrtype,&he,buf,bufsize,&result,&errnr);
    ],
	wine_cv_linux_gethostbyname_r_6=yes,
	wine_cv_linux_gethostbyname_r_6=no
	)
   )
   if test "$wine_cv_linux_gethostbyname_r_6" = "yes"
   then
      AC_DEFINE(HAVE_LINUX_GETHOSTBYNAME_R_6, 1,
                [Define if Linux-style gethostbyname_r and gethostbyaddr_r are available])
   fi

if test "$ac_cv_header_linux_joystick_h" = "yes"
then
   AC_CACHE_CHECK([whether linux/joystick.h uses the Linux 2.2+ API],
   	wine_cv_linux_joystick_22_api,
	AC_TRY_COMPILE([
	#include <sys/ioctl.h>
	#include <linux/joystick.h>

	struct js_event blub;
	#if !defined(JS_EVENT_AXIS) || !defined(JS_EVENT_BUTTON)
	#error "no 2.2 header"
	#endif
	],/*empty*/,
	wine_cv_linux_joystick_22_api=yes,
	wine_cv_linux_joystick_22_api=no,
	wine_cv_linux_joystick_22_api=no
	)
   )
   if test "$wine_cv_linux_joystick_22_api" = "yes"
   then
      AC_DEFINE(HAVE_LINUX_22_JOYSTICK_API, 1,
                [Define if <linux/joystick.h> defines the Linux 2.2 joystick API])
   fi
fi

dnl **** FIXME: what about mixed cases, where we need two of them? ***

dnl Check for statfs members
AC_CHECK_MEMBERS([struct statfs.f_bfree, struct statfs.f_bavail, struct statfs.f_frsize, struct statfs.f_ffree, struct statfs.f_favail, struct statfs.f_namelen],,,
[#include <sys/types.h>
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
# include <sys/mount.h>
#endif
#ifdef HAVE_SYS_VFS_H
# include <sys/vfs.h>
#endif
#ifdef HAVE_SYS_STATFS_H
# include <sys/statfs.h>
#endif])

AC_CHECK_MEMBERS([struct statvfs.f_blocks],,,
[#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif])

dnl Check for socket structure members
AC_CHECK_MEMBERS([struct msghdr.msg_accrights, struct sockaddr.sa_len, struct sockaddr_un.sun_len],,,
[#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_UN_H
# include <sys/un.h>
#endif])

dnl Check for scsireq_t and sg_io_hdr_t members
AC_CHECK_MEMBERS([scsireq_t.cmd, sg_io_hdr_t.interface_id],,,
[#include <sys/types.h>
#ifdef HAVE_SCSI_SG_H
#include <scsi/sg.h>
#endif])

dnl Check for siginfo_t members
AC_CHECK_MEMBERS([siginfo_t.si_fd],,,[#include <signal.h>])

dnl Check for struct mtget members
AC_CHECK_MEMBERS([struct mtget.mt_blksiz, struct mtget.mt_gstat, struct mtget.mt_blkno],,,
[#include <sys/types.h>
#ifdef HAVE_SYS_MTIO_H
#include <sys/mtio.h>
#endif])

dnl Check for struct option
AC_CHECK_MEMBERS([struct option.name],,,
[#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif])

dnl Check for stat.st_blocks
AC_CHECK_MEMBERS([struct stat.st_blocks])

dnl Check for the external timezone variables timezone and daylight
AC_CACHE_CHECK([for timezone variable], ac_cv_have_timezone,
               AC_TRY_LINK([#include <time.h>],[timezone;],
			   ac_cv_have_timezone="yes", ac_cv_have_timezone="no"))
if test "$ac_cv_have_timezone" = "yes"
then
    AC_DEFINE(HAVE_TIMEZONE, 1, [Define if you have the timezone variable])
fi
AC_CACHE_CHECK([for daylight variable], ac_cv_have_daylight,
               AC_TRY_LINK([#include <time.h>],[daylight;],
			   ac_cv_have_daylight="yes", ac_cv_have_daylight="no"))
if test "$ac_cv_have_daylight" = "yes"
then
    AC_DEFINE(HAVE_DAYLIGHT, 1, [Define if you have the daylight variable])
fi

dnl *** check for the need to define platform-specific symbols

case $host_cpu in
  *i[[3456789]]86*) WINE_CHECK_DEFINE([__i386__]) ;;
  *x86_64*)         WINE_CHECK_DEFINE([__x86_64__]) ;;
  *alpha*)          WINE_CHECK_DEFINE([__ALPHA__]) ;;
  *sparc*)          WINE_CHECK_DEFINE([__sparc__]) ;;
  *powerpc*)        WINE_CHECK_DEFINE([__powerpc__]) ;;
esac

case $host_vendor in
  *sun*) WINE_CHECK_DEFINE([__sun__]) ;;
esac

dnl **** Generate output files ****

AH_TOP([#define __WINE_CONFIG_H])

WINE_CONFIG_EXTRA_DIR(dlls/gdi/enhmfdrv)
WINE_CONFIG_EXTRA_DIR(dlls/gdi/mfdrv)
WINE_CONFIG_EXTRA_DIR(dlls/kernel/messages)
WINE_CONFIG_EXTRA_DIR(dlls/user/resources)
WINE_CONFIG_EXTRA_DIR(dlls/wineps/data)
WINE_CONFIG_EXTRA_DIR(include/wine)

MAKE_RULES=Make.rules
AC_SUBST_FILE(MAKE_RULES)

MAKE_DLL_RULES=dlls/Makedll.rules
AC_SUBST_FILE(MAKE_DLL_RULES)

MAKE_IMPLIB_RULES=dlls/Makeimplib.rules
AC_SUBST_FILE(MAKE_IMPLIB_RULES)

MAKE_TEST_RULES=dlls/Maketest.rules
AC_SUBST_FILE(MAKE_TEST_RULES)

MAKE_LIB_RULES=libs/Makelib.rules
AC_SUBST_FILE(MAKE_LIB_RULES)

MAKE_PROG_RULES=programs/Makeprog.rules
AC_SUBST_FILE(MAKE_PROG_RULES)

AC_CONFIG_FILES([
Make.rules
dlls/Makedll.rules
dlls/Makeimplib.rules
dlls/Maketest.rules
libs/Makelib.rules
programs/Makeprog.rules
Makefile
dlls/Makefile
dlls/activeds/Makefile
dlls/advapi32/Makefile
dlls/advapi32/tests/Makefile
dlls/advpack/Makefile
dlls/advpack/tests/Makefile
dlls/amstream/Makefile
dlls/atl/Makefile
dlls/avicap32/Makefile
dlls/avifil32/Makefile
dlls/cabinet/Makefile
dlls/cabinet/tests/Makefile
dlls/capi2032/Makefile
dlls/cards/Makefile
dlls/cfgmgr32/Makefile
dlls/comcat/Makefile
dlls/comctl32/Makefile
dlls/comctl32/tests/Makefile
dlls/commdlg/Makefile
dlls/crtdll/Makefile
dlls/crypt32/Makefile
dlls/crypt32/tests/Makefile
dlls/cryptdll/Makefile
dlls/ctl3d/Makefile
dlls/d3d8/Makefile
dlls/d3d9/Makefile
dlls/d3d9/tests/Makefile
dlls/d3dim/Makefile
dlls/d3drm/Makefile
dlls/d3dx8/Makefile
dlls/d3dxof/Makefile
dlls/dbghelp/Makefile
dlls/dciman32/Makefile
dlls/ddraw/Makefile
dlls/ddraw/tests/Makefile
dlls/devenum/Makefile
dlls/dinput/Makefile
dlls/dinput/tests/Makefile
dlls/dinput8/Makefile
dlls/dmband/Makefile
dlls/dmcompos/Makefile
dlls/dmime/Makefile
dlls/dmloader/Makefile
dlls/dmscript/Makefile
dlls/dmstyle/Makefile
dlls/dmsynth/Makefile
dlls/dmusic/Makefile
dlls/dmusic32/Makefile
dlls/dplay/Makefile
dlls/dplayx/Makefile
dlls/dpnet/Makefile
dlls/dpnhpast/Makefile
dlls/dsound/Makefile
dlls/dsound/tests/Makefile
dlls/dswave/Makefile
dlls/dxdiagn/Makefile
dlls/dxerr8/Makefile
dlls/dxerr9/Makefile
dlls/dxguid/Makefile
dlls/gdi/Makefile
dlls/gdi/tests/Makefile
dlls/glu32/Makefile
dlls/glut32/Makefile
dlls/hhctrl.ocx/Makefile
dlls/iccvid/Makefile
dlls/icmp/Makefile
dlls/ifsmgr.vxd/Makefile
dlls/imagehlp/Makefile
dlls/imm32/Makefile
dlls/iphlpapi/Makefile
dlls/iphlpapi/tests/Makefile
dlls/itss/Makefile
dlls/kernel/Makefile
dlls/kernel/tests/Makefile
dlls/lzexpand/Makefile
dlls/lzexpand/tests/Makefile
dlls/mapi32/Makefile
dlls/mapi32/tests/Makefile
dlls/mciavi32/Makefile
dlls/mcicda/Makefile
dlls/mciseq/Makefile
dlls/midimap/Makefile
dlls/mlang/Makefile
dlls/mlang/tests/Makefile
dlls/mmdevldr.vxd/Makefile
dlls/monodebg.vxd/Makefile
dlls/mpr/Makefile
dlls/msacm/Makefile
dlls/msacm/imaadp32/Makefile
dlls/msacm/msadp32/Makefile
dlls/msacm/msg711/Makefile
dlls/msacm/tests/Makefile
dlls/msacm/winemp3/Makefile
dlls/mscms/Makefile
dlls/mscms/tests/Makefile
dlls/msdmo/Makefile
dlls/mshtml/Makefile
dlls/mshtml/tests/Makefile
dlls/msi/Makefile
dlls/msi/tests/Makefile
dlls/msimg32/Makefile
dlls/msisys/Makefile
dlls/msnet32/Makefile
dlls/msrle32/Makefile
dlls/msvcrt/Makefile
dlls/msvcrt/tests/Makefile
dlls/msvcrt20/Makefile
dlls/msvcrt40/Makefile
dlls/msvcrtd/Makefile
dlls/msvcrtd/tests/Makefile
dlls/msvidc32/Makefile
dlls/msvideo/Makefile
dlls/mswsock/Makefile
dlls/msxml3/Makefile
dlls/msxml3/tests/Makefile
dlls/netapi32/Makefile
dlls/netapi32/tests/Makefile
dlls/newdev/Makefile
dlls/ntdll/Makefile
dlls/ntdll/tests/Makefile
dlls/objsel/Makefile
dlls/odbc32/Makefile
dlls/odbccp32/Makefile
dlls/ole32/Makefile
dlls/ole32/tests/Makefile
dlls/oleacc/Makefile
dlls/oleaut32/Makefile
dlls/oleaut32/tests/Makefile
dlls/olecli/Makefile
dlls/oledlg/Makefile
dlls/olepro32/Makefile
dlls/olesvr/Makefile
dlls/opengl32/Makefile
dlls/powrprof/Makefile
dlls/psapi/Makefile
dlls/psapi/tests/Makefile
dlls/qcap/Makefile
dlls/quartz/Makefile
dlls/quartz/tests/Makefile
dlls/rasapi32/Makefile
dlls/riched20/Makefile
dlls/riched20/tests/Makefile
dlls/richedit/Makefile
dlls/rpcrt4/Makefile
dlls/rpcrt4/tests/Makefile
dlls/rsabase/Makefile
dlls/rsabase/tests/Makefile
dlls/rsaenh/Makefile
dlls/rsaenh/tests/Makefile
dlls/secur32/Makefile
dlls/secur32/tests/Makefile
dlls/security/Makefile
dlls/sensapi/Makefile
dlls/serialui/Makefile
dlls/setupapi/Makefile
dlls/setupapi/tests/Makefile
dlls/shdocvw/Makefile
dlls/shdocvw/tests/Makefile
dlls/shell32/Makefile
dlls/shell32/tests/Makefile
dlls/shfolder/Makefile
dlls/shlwapi/Makefile
dlls/shlwapi/tests/Makefile
dlls/snmpapi/Makefile
dlls/stdole2.tlb/Makefile
dlls/stdole32.tlb/Makefile
dlls/sti/Makefile
dlls/strmiids/Makefile
dlls/tapi32/Makefile
dlls/twain/Makefile
dlls/unicows/Makefile
dlls/url/Makefile
dlls/urlmon/Makefile
dlls/urlmon/tests/Makefile
dlls/user/Makefile
dlls/user/tests/Makefile
dlls/usp10/Makefile
dlls/uuid/Makefile
dlls/uxtheme/Makefile
dlls/vdhcp.vxd/Makefile
dlls/vdmdbg/Makefile
dlls/version/Makefile
dlls/version/tests/Makefile
dlls/vmm.vxd/Makefile
dlls/vnbt.vxd/Makefile
dlls/vnetbios.vxd/Makefile
dlls/vtdapi.vxd/Makefile
dlls/vwin32.vxd/Makefile
dlls/win32s/Makefile
dlls/winaspi/Makefile
dlls/winecrt0/Makefile
dlls/wined3d/Makefile
dlls/winedos/Makefile
dlls/wineps/Makefile
dlls/wininet/Makefile
dlls/wininet/tests/Makefile
dlls/winmm/Makefile
dlls/winmm/joystick/Makefile
dlls/winmm/mciwave/Makefile
dlls/winmm/tests/Makefile
dlls/winmm/wavemap/Makefile
dlls/winmm/winealsa/Makefile
dlls/winmm/winearts/Makefile
dlls/winmm/wineaudioio/Makefile
dlls/winmm/wineesd/Makefile
dlls/winmm/winejack/Makefile
dlls/winmm/winenas/Makefile
dlls/winmm/wineoss/Makefile
dlls/winnls/Makefile
dlls/winsock/Makefile
dlls/winsock/tests/Makefile
dlls/winspool/Makefile
dlls/winspool/tests/Makefile
dlls/wintab32/Makefile
dlls/wintrust/Makefile
dlls/wldap32/Makefile
dlls/wow32/Makefile
dlls/wsock32/Makefile
dlls/wtsapi32/Makefile
dlls/x11drv/Makefile
documentation/Makefile
fonts/Makefile
include/Makefile
libs/Makefile
libs/port/Makefile
libs/unicode/Makefile
libs/wine/Makefile
libs/wpp/Makefile
loader/Makefile
programs/Makefile
programs/avitools/Makefile
programs/clock/Makefile
programs/cmdlgtst/Makefile
programs/control/Makefile
programs/eject/Makefile
programs/expand/Makefile
programs/explorer/Makefile
programs/hh/Makefile
programs/msiexec/Makefile
programs/notepad/Makefile
programs/progman/Makefile
programs/regedit/Makefile
programs/regsvr32/Makefile
programs/rpcss/Makefile
programs/rundll32/Makefile
programs/start/Makefile
programs/taskmgr/Makefile
programs/uninstaller/Makefile
programs/view/Makefile
programs/wcmd/Makefile
programs/wineboot/Makefile
programs/winebrowser/Makefile
programs/winecfg/Makefile
programs/wineconsole/Makefile
programs/winedbg/Makefile
programs/winefile/Makefile
programs/winemenubuilder/Makefile
programs/winemine/Makefile
programs/winepath/Makefile
programs/winetest/Makefile
programs/winevdm/Makefile
programs/winhelp/Makefile
programs/winver/Makefile
server/Makefile
tools/Makefile
tools/widl/Makefile
tools/winapi/Makefile
tools/winebuild/Makefile
tools/winedump/Makefile
tools/winegcc/Makefile
tools/wmc/Makefile
tools/wrc/Makefile])

AC_OUTPUT

if test "$have_x" = "no"
then
  echo
  echo "*** Warning: X development files not found. Wine will be built without"
  echo "*** X support, which currently does not work, and would probably not be"
  echo "*** what you want anyway. You will need to install devel packages of"
  echo "*** Xlib/Xfree86 at the very least."
fi

if test "$wine_cv_opengl_version_OK" = "no"
then
  echo
  echo "*** Warning: old Mesa headers detected. Wine will be built without Direct3D"
  echo "*** support. Consider upgrading your Mesa libraries (http://www.mesa3d.org/)."
fi

if test "$wine_cv_msg_freetype" = "yes"
then
  echo
  echo "*** Note: Your system appears to have the FreeType 2 runtime libraries"
  echo "*** installed, but 'freetype-config' is not in your PATH. Install the"
  echo "*** freetype-devel package (or its equivalent on your distribution) to"
  echo "*** enable Wine to use TrueType fonts."
fi

if test -z "$ALSALIBS" -a \
        -z "$ARTSC_LIBS" -a \
        -z "$AUDIOIOLIBS" -a \
        -z "$NASLIBS" -a \
        -z "$ESD_LIBS" -a \
        -z "$ac_cv_lib_soname_jack" -a \
        "$ac_cv_header_sys_soundcard_h" != "yes" -a \
        "$ac_cv_header_machine_soundcard_h" != "yes" -a \
        "$ac_cv_header_soundcard_h" != "yes"
then
    echo "*** No sound system was found. Windows applications will be silent."
    echo "*** The currently supported sound systems are:"
    echo "*** ALSA, ARTS, EsounD, AudioIO, Jack, NAS and OSS"
fi

echo
echo "Configure finished.  Do '${ac_make} depend && ${ac_make}' to compile Wine."
echo

dnl Local Variables:
dnl comment-start: "dnl "
dnl comment-end: ""
dnl comment-start-skip: "\\bdnl\\b\\s *"
dnl compile-command: "autoconf"
dnl End:
