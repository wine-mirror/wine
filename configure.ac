dnl Process this file with autoconf to produce a configure script.
dnl Original author: Michael Patra
dnl See ChangeLog file for detailed change history.

m4_define(WINE_VERSION,regexp(m4_include(VERSION),[version \([-.0-9A-Za-z]+\)],[\1]))

AC_PREREQ(2.53)
AC_INIT([Wine],WINE_VERSION)
AC_CONFIG_SRCDIR(server/atom.c)
AC_CONFIG_HEADERS(include/config.h)
AC_CONFIG_AUX_DIR(tools)

dnl **** Command-line arguments ****

dnl Library type .so or .a
AC_SUBST(LIBEXT,"so")

AC_ARG_ENABLE(win16, AC_HELP_STRING([--disable-win16],[do not include Win16 support]))
AC_ARG_ENABLE(debug, AC_HELP_STRING([--disable-debug],[compile out all debugging messages]))
AC_ARG_ENABLE(trace, AC_HELP_STRING([--disable-trace],[compile out TRACE messages]))

AC_ARG_WITH(opengl,    AC_HELP_STRING([--without-opengl],[do not use OpenGL]))
AC_ARG_WITH(curses,    AC_HELP_STRING([--without-curses],[do not use curses]))
AC_ARG_WITH(wine-tools,AC_HELP_STRING([--with-wine-tools=<dir>],[use Wine tools from directory <dir>]))

AC_SUBST(OPTIONS)
AC_SUBST(WIN16_FILES,"\$(WIN16_FILES)")
AC_SUBST(WIN16_INSTALL,"\$(WIN16_INSTALL)")
if test "x$enable_win16" = "xno"
then
  WIN16_FILES=""
  WIN16_INSTALL=""
fi
if test "x$enable_debug" = "xno"
then
    AC_DEFINE(NO_DEBUG_MSGS,1,[Define to disable all debug messages.])
fi
if test "x$enable_trace" = "xno" -o "x$enable_debug" = "xno"
then
    AC_DEFINE(NO_TRACE_MSGS,1,[Define to disable trace messages.])
fi

dnl **** Check for some programs ****

AC_CANONICAL_HOST
AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_CPP

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
AC_PROG_YACC
AC_PROG_LEX

dnl **** Just additional warning checks, since AC_PROG just sets 'yacc' even
dnl **** without one present.
AC_CHECK_PROGS(XYACC,$YACC bison yacc,none)
if test "$XYACC" = "none"
then
  AC_MSG_ERROR([no suitable bison/yacc found. Please install the 'bison' package.])
fi
AC_CHECK_PROGS(XLEX,$LEX flex lex,none)
if test "$XLEX" = "none"
then
  AC_MSG_ERROR([no suitable lex found. Please install the 'flex' package.])
fi

AC_CHECK_TOOL(LD,ld,ld)
AC_CHECK_TOOL(AR,ar,ar)
AC_PROG_RANLIB
AC_CHECK_TOOL(STRIP,strip,strip)
AC_CHECK_TOOL(WINDRES,windres,false)
AC_PROG_INSTALL
AC_PROG_LN_S
WINE_PROG_LN
AC_CHECK_PROG(C2MAN,c2man,c2man,\$(TOPSRCDIR)/tools/c2man.pl)
AC_PATH_PROG(LDCONFIG, ldconfig, true, [/sbin /usr/sbin $PATH])

dnl Check for lint
AC_CHECK_PROGS(LINT, lclint lint)
if test "$LINT" = "lint"
then
  LINTFLAGS="$LINTFLAGS -errchk=%all,no%longptr64 -errhdr=%user -Ncheck=macro -Nlevel=4"
  dnl LINTFLAGS='-D_SIZE_T "-Dsize_t=unsigned long" -errchk=longptr64'
fi
AC_SUBST(LINT)
AC_SUBST(LINTFLAGS)

dnl **** Check for some libraries ****

dnl Check for -lm
AC_CHECK_LIB(m,sqrt)
dnl Check for -li386 for NetBSD and OpenBSD
AC_CHECK_LIB(i386,i386_set_ldt)
dnl Check for -lossaudio for NetBSD
AC_CHECK_LIB(ossaudio,_oss_ioctl)
dnl Check for -lw for Solaris
AC_CHECK_FUNCS(iswalnum,,AC_CHECK_LIB(w,iswalnum))
dnl Check for -lnsl for Solaris
AC_CHECK_FUNCS(gethostbyname,,AC_CHECK_LIB(nsl,gethostbyname))
dnl Check for -lsocket for Solaris
AC_CHECK_FUNCS(connect,,AC_CHECK_LIB(socket,connect))
dnl Check for -lresolv for Solaris
AC_CHECK_FUNCS(inet_aton,,AC_CHECK_LIB(resolv,inet_aton))
dnl Check for -lxpg4 for FreeBSD
AC_CHECK_LIB(xpg4,_xpg4_setrunelocale)
dnl Check for -lmmap for OS/2
AC_CHECK_LIB(mmap,mmap)

JPEGLIB=""
AC_SUBST(JPEGLIB)
AC_CHECK_HEADERS(jpeglib.h,
    AC_CHECK_LIB(jpeg,jpeg_start_decompress,
    	AC_DEFINE(HAVE_LIBJPEG,1,[Define if you have libjpeg including devel headers])
	JPEGLIB="-ljpeg"
    )
)


AC_SUBST(XLIB)
AC_SUBST(XFILES)
XFILES=""
AC_SUBST(OPENGLFILES)
OPENGLFILES=""
AC_SUBST(GLU32FILES)
GLU32FILES=""
AC_SUBST(OPENGL_LIBS)
OPENGL_LIBS=""
if test "$have_x" = "yes"
then
    XLIB="-lXext -lX11"
    ac_save_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $X_CFLAGS"

    dnl *** All of the following tests require X11/Xlib.h
    AC_CHECK_HEADERS(X11/Xlib.h,
      [
        dnl *** Check for X keyboard extension
        AC_CHECK_HEADERS(X11/XKBlib.h,
            [ dnl *** If X11/XKBlib.h exists...
              AC_CHECK_LIB(X11, XkbQueryExtension,
              AC_DEFINE(HAVE_XKB, 1, [Define if you have the XKB extension]),,
              $X_LIBS -lXext -lX11 $X_EXTRA_LIBS)
            ],
            AC_MSG_WARN([[Xkb extension not found, Wine will be built without it]]),
            [#include <X11/Xlib.h>])

        dnl *** Check for X Shm extension
        AC_CHECK_HEADERS(X11/extensions/XShm.h,
            [ dnl *** If X11/extensions/XShm.h exists...
              AC_CHECK_LIB(Xext, XShmQueryExtension,
              AC_DEFINE(HAVE_LIBXXSHM, 1, [Define if you have the X Shm extension]),,
              $X_LIBS -lXext -lX11 $X_EXTRA_LIBS)
            ],
            AC_MSG_WARN([[XShm extension not found, Wine will be built without it]]),
            [#include <X11/Xlib.h>])

        dnl *** Check for Xutil
        AC_CHECK_HEADERS(X11/Xutil.h,,,
            [#include <X11/Xlib.h>])

        dnl *** Check for X shape extension
        AC_CHECK_HEADERS(X11/extensions/shape.h,
            [ dnl *** If X11/extensions/shape.h exists...
              AC_CHECK_LIB(Xext,XShapeQueryExtension,
	      AC_DEFINE(HAVE_LIBXSHAPE, 1, [Define if you have the X Shape extension]),,
	      $X_LIBS -lXext -lX11 $X_EXTRA_LIBS)
	    ],
	    AC_MSG_WARN([[XShape extension not found, Wine will be built without it]]),
            [#include <X11/Xlib.h>
             #ifdef HAVE_X11_XUTIL_H
             # include <X11/Xutil.h>
             #endif])

        dnl *** Check for XFree86 DGA / DGA 2.0 extension
        AC_CHECK_HEADERS(X11/extensions/xf86dga.h,
            [ dnl *** If X11/extensions/xf86dga.h exists, check
              dnl *** for XDGAQueryExtension()...
              AC_CHECK_LIB(Xxf86dga, XDGAQueryExtension,
                [ dnl *** If found...
                  AC_DEFINE(HAVE_LIBXXF86DGA2, 1,
                            [Define if you have the Xxf86dga library version 2])
                  X_PRE_LIBS="$X_PRE_LIBS -lXxf86dga"
               ],
                [ dnl *** If not found, look for XF86DGAQueryExtension()
                  dnl *** instead (DGA 2.0 not found)...
                  AC_CHECK_LIB(Xxf86dga, XF86DGAQueryExtension,
                    [ AC_DEFINE(HAVE_LIBXXF86DGA, 1,
                                [Define if you have the Xxf86dga library version 1])
                      X_PRE_LIBS="$X_PRE_LIBS -lXxf86dga"
                    ],,
                    $X_LIBS -lXext -lX11 $X_EXTRA_LIBS
                  )
                ],
                $X_LIBS -lXext -lX11 $X_EXTRA_LIBS
              )
            ],
            AC_MSG_WARN([[DGA extension not found, Wine will be built without it]]),
            [#include <X11/Xlib.h>])

        dnl *** Check for XFree86 VMODE extension
        AC_CHECK_HEADERS(X11/extensions/xf86vmode.h,
            [ dnl *** If X11/extensions/xf86vmode.h exists...
                AC_CHECK_LIB(Xxf86vm, XF86VidModeQueryExtension,
                  [ AC_DEFINE(HAVE_LIBXXF86VM, 1, [Define if you have the Xxf86vm library])
                     X_PRE_LIBS="$X_PRE_LIBS -lXxf86vm"
                  ],,
                  $X_LIBS -lXext -lX11 $X_EXTRA_LIBS
                )
            ],
            AC_MSG_WARN([[XFree86 VMODE extension not found, Wine will be built without it]]),
            [#include <X11/Xlib.h>])

        dnl *** Check for XVideo extension supporting XvImages
        AC_CHECK_HEADERS(X11/extensions/Xvlib.h,
            [ dnl *** If X11/extensions/Xvlib.h exists...
                AC_CHECK_LIB(Xv, XvShmCreateImage,
                  [ AC_DEFINE(HAVE_XVIDEO, 1, [Define if the X libraries support XVideo])
                     X_PRE_LIBS="$X_PRE_LIBS -lXv"
                  ],,
                  $X_LIBS -lXext -lX11 $X_EXTRA_LIBS
                )
            ],
            AC_MSG_WARN([[XVideo extension not found, Wine will be built without it]]),
            [#include <X11/Xlib.h>])

        dnl *** Check for XRender include file
        AC_CHECK_HEADERS(X11/extensions/Xrender.h,,,[#include <X11/Xlib.h>])
      ]
    ) dnl *** End of X11/Xlib.h check

    dnl Check for the presence of OpenGL
    if test "x$with_opengl" != "xno"
    then
        if test -f /usr/X11R6/lib/libGL.a -a ! -f /usr/X11R6/lib/libGL.so
        then
	    AC_MSG_ERROR([/usr/X11R6/lib/libGL.a is present on your system.
This prevents linking to OpenGL. Delete the file and restart configure.])
        fi

	AC_CHECK_HEADERS(GL/gl.h GL/glx.h)
	if test "$ac_cv_header_GL_gl_h" = "yes" -a "$ac_cv_header_GL_glx_h" = "yes"
	then
            AC_CHECK_HEADERS(GL/glext.h,,,[#include <GL/glx.h>])
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

			AC_CHECK_LIB(GL,glXGetProcAddressARB,
                                     AC_DEFINE(HAVE_GLX_GETPROCADDRESS, 1,
                                               [Define if the OpenGL library supports the glXGetProcAddressARB call]),,
                                     $X_LIBS -lXext -lX11 -lm $X_EXTRA_LIBS)

			if test "$ac_cv_lib_GL_glXGetProcAddressARB" = "yes"
			then
			       AC_CACHE_CHECK([for OpenGL extension functions prototypes], wine_cv_extension_prototypes,
			          [AC_TRY_COMPILE([#include <GL/gl.h>
				  		  #ifdef HAVE_GL_GLEXT_H
						  # include <GL/glext.h>
						  #endif
						  ],
						 [PFNGLCOLORTABLEEXTPROC test_proc;],
						 [wine_cv_extension_prototypes="yes"],
						 [wine_cv_extension_prototypes="no"]
				  )]
				)
				if test "$wine_cv_extension_prototypes" = "yes"
				then
				    AC_DEFINE(HAVE_GLEXT_PROTOTYPES, 1,
                                              [Define if the OpenGL headers define extension typedefs])
				fi
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

dnl **** Check which curses lib to use ***
CURSESLIBS=""
if test "x$with_curses" != "xno"
then
    AC_CHECK_HEADERS(ncurses.h,
        [AC_CHECK_LIB(ncurses,waddch,
            [AC_DEFINE(HAVE_LIBNCURSES, 1, [Define if you have the ncurses library (-lncurses)])
             CURSESLIBS="-lncurses"],
             [AC_CHECK_HEADERS(curses.h,
                 [AC_CHECK_LIB(curses,waddch,
                     [AC_DEFINE(HAVE_LIBCURSES, 1, [Define if you have the curses library (-lcurses)])
                      CURSESLIBS="-lcurses"])])])])
    saved_libs="$LIBS"
    LIBS="$CURSESLIBS $LIBS"
    AC_CHECK_FUNCS(getbkgd resizeterm)
    LIBS="$saved_libs"
fi
AC_SUBST(CURSESLIBS)

dnl **** Check for SANE ****
AC_CHECK_PROG(sane_devel,sane-config,sane-config,no)
if test "$sane_devel" = "no"
then
    SANELIBS=""
    SANEINCL=""
else
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
AC_SUBST(SANELIBS)
AC_SUBST(SANEINCL)

dnl **** Check for FreeType 2 ****
AC_CHECK_LIB(freetype,FT_Init_FreeType,ft_lib=yes,ft_lib=no,$X_LIBS)
if test "$ft_lib" = "no"
then
    FREETYPEINCL=""
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
	FREETYPEINCL=""
	wine_cv_msg_freetype=yes
    else
	FREETYPEINCL=`$ft_devel --cflags`
	ac_save_CPPFLAGS="$CPPFLAGS"
	CPPFLAGS="$FREETYPEINCL $CPPFLAGS"
	AC_CHECK_HEADERS(freetype/freetype.h \
                         freetype/ftglyph.h \
                         freetype/tttables.h \
                         freetype/ftnames.h \
                         freetype/ftsnames.h \
                         freetype/ttnameid.h \
                         freetype/ftoutln.h \
                         freetype/internal/sfnt.h)
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
	    FREETYPEINCL=""
	    wine_cv_msg_freetype=yes
	fi
    fi
fi
AC_SUBST(FREETYPEINCL)

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

dnl **** Check for IPX (currently Linux only) ****
AC_CACHE_CHECK([for GNU style IPX support], ac_cv_c_ipx_gnu,
 AC_TRY_COMPILE(
   [#include <sys/types.h>
    #ifdef HAVE_SYS_SOCKET_H
    # include <sys/socket.h>
    #endif
    #include <netipx/ipx.h>],
   [((struct sockaddr_ipx *)0)->sipx_family == AF_IPX],
   [ac_cv_c_ipx_gnu="yes"],
   [ac_cv_c_ipx_gnu="no"])
 )
if test "$ac_cv_c_ipx_gnu" = "yes"
then
    AC_DEFINE(HAVE_IPX_GNU, 1, [Define if IPX should use netipx/ipx.h from libc])
fi

if test "$ac_cv_c_ipx_gnu" = "no"
then
 AC_CACHE_CHECK([for linux style IPX support], ac_cv_c_ipx_linux,
  AC_TRY_COMPILE(
    [#include <sys/types.h>
     #ifdef HAVE_SYS_SOCKET_H
     # include <sys/socket.h>
     #endif
     #include <asm/types.h>
     #include <linux/ipx.h>],
    [((struct sockaddr_ipx *)0)->sipx_family == AF_IPX],
    [ac_cv_c_ipx_linux="yes"],
    [ac_cv_c_ipx_linux="no"])
  )
  if test "$ac_cv_c_ipx_linux" = "yes"
  then
      AC_DEFINE(HAVE_IPX_LINUX, 1, [Define if IPX includes are taken from Linux kernel])
  fi
fi

dnl **** Check for Open Sound System ****
AC_CHECK_HEADERS(sys/soundcard.h machine/soundcard.h soundcard.h, break)

AC_CACHE_CHECK([for Open Sound System],
	ac_cv_c_opensoundsystem,
	AC_TRY_COMPILE([
	#if defined(HAVE_SYS_SOUNDCARD_H)
		#include <sys/soundcard.h>
	#elif defined(HAVE_MACHINE_SOUNDCARD_H)
		#include <machine/soundcard.h>
	#elif defined(HAVE_SOUNDCARD_H)
		#include <soundcard.h>
	#endif
	],[

/* check for one of the Open Sound System specific SNDCTL_ defines */
#if !defined(SNDCTL_DSP_STEREO)
#error No open sound system
#endif
],ac_cv_c_opensoundsystem="yes",ac_cv_c_opensoundsystem="no"))

if test "$ac_cv_c_opensoundsystem" = "yes"
then
    AC_DEFINE(HAVE_OSS, 1, [Define if you have the Open Sound system])
fi

AC_CACHE_CHECK([for Open Sound System/MIDI interface],
	ac_cv_c_opensoundsystem_midi,
	AC_TRY_COMPILE([
	#if defined(HAVE_SYS_SOUNDCARD_H)
		#include <sys/soundcard.h>
	#elif defined(HAVE_MACHINE_SOUNDCARD_H)
		#include <machine/soundcard.h>
	#elif defined(HAVE_SOUNDCARD_H)
		#include <soundcard.h>
	#endif
	],[

/* check for one of the Open Sound System specific SNDCTL_SEQ defines */
#if !defined(SNDCTL_SEQ_SYNC)
#error No open sound system MIDI interface
#endif
],ac_cv_c_opensoundsystem_midi="yes",ac_cv_c_opensoundsystem_midi="no"))

if test "$ac_cv_c_opensoundsystem_midi" = "yes"
then
    AC_DEFINE(HAVE_OSS_MIDI, 1, [Define if you have the Open Sound system (MIDI interface)])
fi

dnl **** Check for aRts Sound Server ****
AC_PATH_PROG(ARTSCCONFIG, artsc-config)
AC_CACHE_CHECK([for aRts Sound server],
        ac_cv_c_artsserver,
        if test x$ARTSCCONFIG = x -o x$ARTSCCONFIG = x'"$ARTSCCONFIG"';
        then
            ac_cv_c_artsserver=no
        else
            ARTSC_CFLAGS=`$ARTSCCONFIG --cflags`
            ARTSC_LIBS=`$ARTSCCONFIG --libs`
            ac_cv_c_artsserver=no
            save_CFLAGS="$CFLAGS"
            CFLAGS="$CFLAGS $ARTSC_CFLAGS"
            AC_TRY_COMPILE([
             #include <artsc.h>
            ],[
             arts_stream_t stream;
            ],[
            ac_cv_c_artsserver=yes
            ])
            CFLAGS="$save_CFLAGS"
        fi)

if test "$ac_cv_c_artsserver" = "yes"
then
    AC_SUBST(ARTSLIBS, $ARTSC_LIBS)
    AC_SUBST(ARTSINCL, $ARTSC_CFLAGS)

    AC_DEFINE(HAVE_ARTS, 1, [Define if you have ARTS sound server])
fi

dnl **** Check for ALSA ****
AC_SUBST(ALSALIBS,"")
AC_CHECK_HEADERS(alsa/asoundlib.h sys/asoundlib.h, break)
if test "$ac_cv_header_sys_asoundlib_h" = "yes" -o "$ac_cv_header_alsa_asoundlib_h" = "yes"
then
    AC_CHECK_LIB(asound,snd_pcm_open,
    	AC_DEFINE(HAVE_ALSA,1,[Define if you have ALSA including devel headers])
	ALSALIBS="-lasound")
fi

dnl **** Check for libaudioio (which can be used to get solaris audio support) ****

AC_SUBST(AUDIOIOLIBS,"")
AC_CHECK_HEADERS(libaudioio.h,
    [AC_CHECK_LIB(audioio,AudioIOGetVersion,
                  [AUDIOIOLIBS="-laudioio"
                   AC_DEFINE(HAVE_LIBAUDIOIO, 1, [Define if you have libaudioIO])])])

dnl **** Check for broken glibc mmap64 ****

AC_CACHE_CHECK( [whether mmap64 works defined as mmap], ac_cv_mmap64_works,
	AC_TRY_RUN([
		#define _FILE_OFFSET_BITS 64
		#include <stdio.h>
		#include <unistd.h>
		#include <fcntl.h>
		#include <sys/mman.h>
		#include <errno.h>

		int main(int argc,char **argv) {
			int fd = open("conftest.map",O_CREAT|O_RDWR,0600);
			if (fd == -1) exit(1);

			unlink("conftest.map");

			write(fd,"test",4);

			if ((-1 == mmap(0,4,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0)) &&
			    (errno == EINVAL)
			) {
				exit(1);
			}
			close(fd);
			fprintf(stderr,"success!\n");
			exit(0);
		}

	],
    ac_cv_mmap64_works="yes",
    ac_cv_mmap64_works="no",
    ac_cv_mmap64_works="no") )

if test "$ac_cv_mmap64_works" = "yes"
then
    AC_DEFINE(_FILE_OFFSET_BITS, 64, [Set this to 64 to enable 64-bit file support on Linux])
fi

dnl **** Check for gcc strength-reduce bug ****

if test "x${GCC}" = "xyes"
then
  CFLAGS="$CFLAGS -Wall"
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
    CFLAGS="$CFLAGS -fno-strength-reduce"
  fi

  dnl Check for -mpreferred-stack-boundary
  AC_CACHE_CHECK([for gcc -mpreferred-stack-boundary=2 support], ac_cv_c_gcc_stack_boundary,
      [WINE_TRY_CFLAGS([-mpreferred-stack-boundary=2],
                      ac_cv_c_gcc_stack_boundary="yes",ac_cv_c_gcc_stack_boundary="no")])
  if test "$ac_cv_c_gcc_stack_boundary" = "yes"
  then
    CFLAGS="$CFLAGS -mpreferred-stack-boundary=2"
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

dnl **** Check whether stdcall symbols need to be decorated ****

AC_CACHE_CHECK([whether stdcall symbols need to be decorated], ac_cv_c_stdcall_decoration,
    WINE_TRY_ASM_LINK(["\t.globl _ac_test@0\n_ac_test@0:\n\t.globl ac_test@0\nac_test@0:"],
                      [extern void __attribute__((__stdcall__)) ac_test(void);],
                      [ac_test()],
                      ac_cv_c_stdcall_decoration="yes",ac_cv_c_stdcall_decoration="no"))
if test "$ac_cv_c_stdcall_decoration" = "yes"
then
  AC_DEFINE(NEED_STDCALL_DECORATION, 1, [Define if stdcall symbols need to be decorated])
fi

dnl **** Check for .string in assembler ****

AC_CACHE_CHECK([whether assembler accepts .string], ac_cv_c_asm_string,
    WINE_TRY_ASM_LINK([".data\n\t.string \"test\"\n\t.text"],,,
                      ac_cv_c_asm_string="yes",ac_cv_c_asm_string="no"))
if test "$ac_cv_c_asm_string" = "yes"
then
  AC_DEFINE(HAVE_ASM_STRING, 1, [Define to use .string instead of .ascii])
fi

dnl **** Check for working dll ****

AC_SUBST(DLLEXT,"")
AC_SUBST(DLLFLAGS,"")
AC_SUBST(DLLIBS,"")
AC_SUBST(LDDLLFLAGS,"")
AC_SUBST(LDSHARED,"")

case $host_os in
  cygwin*|mingw32*)
    AC_CHECK_TOOL(DLLWRAP,dllwrap,false)
    if test "$DLLWRAP" = "false"; then
      LIBEXT="a"
    else
      dnl FIXME - check whether dllwrap works correctly...
      LIBEXT="dll"
    fi
    ;;
  *)
    AC_CHECK_HEADERS(dlfcn.h,
        [AC_CHECK_FUNCS(dlopen,,
            [AC_CHECK_LIB(dl,dlopen,
                         [AC_DEFINE(HAVE_DLOPEN,1,[Define if you have dlopen])
                          DLLIBS="-ldl"],
                         [LIBEXT="a"])])],
        [LIBEXT="a"])

    if test "$LIBEXT" = "so"
    then
      DLLFLAGS="-fPIC"
      DLLEXT=".so"
      AC_CACHE_CHECK([whether we can build a GNU style ELF dll], ac_cv_c_dll_gnuelf,
          [WINE_TRY_CFLAGS([-fPIC -shared -Wl,-soname,conftest.so.1.0,-Bsymbolic],
                           ac_cv_c_dll_gnuelf="yes",ac_cv_c_dll_gnuelf="no")])
      if test "$ac_cv_c_dll_gnuelf" = "yes"
      then
        LDSHARED="\$(CC) -shared \$(SONAME:%=-Wl,-soname,%)"
        LDDLLFLAGS="-Wl,-Bsymbolic"
        AC_CACHE_CHECK([whether the linker accepts -z defs], ac_cv_c_dll_zdefs,
          [WINE_TRY_CFLAGS([-fPIC -shared -Wl,-Bsymbolic,-z,defs],
                           ac_cv_c_dll_zdefs="yes",ac_cv_c_dll_zdefs="no")])
        if test "$ac_cv_c_dll_zdefs" = "yes"
        then
          LDDLLFLAGS="$LDDLLFLAGS,-z,defs"
        fi
      else
        AC_CACHE_CHECK(whether we can build a UnixWare (Solaris) dll, ac_cv_c_dll_unixware,
            [WINE_TRY_CFLAGS([-fPIC -Wl,-G,-h,conftest.so.1.0,-B,symbolic],
                             ac_cv_c_dll_unixware="yes",ac_cv_c_dll_unixware="no")])
        if test "$ac_cv_c_dll_unixware" = "yes"
        then
          LDSHARED="\$(CC) -Wl,-G \$(SONAME:%=-Wl,-h,%)"
          LDDLLFLAGS="-Wl,-B,symbolic"
        fi
      fi
    fi

    dnl Check for cross compiler to build test programs
    AC_SUBST(CROSSTEST,"")
    if test "$cross_compiling" = "no"
    then
      AC_CHECK_PROGS(CROSSCC,i586-mingw32msvc-gcc,false)
      AC_CHECK_PROGS(DLLTOOL,i586-mingw32msvc-dlltool,false)
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
    AC_SUBST(LDPATH,"PATH=\"\$(TOOLSDIR)/library:\$(TOOLSDIR)/unicode:\$\$PATH\"") ;;
  *)
    AC_SUBST(LDPATH,"LD_LIBRARY_PATH=\"\$(TOOLSDIR)/library:\$(TOOLSDIR)/unicode:\$\$LD_LIBRARY_PATH\"") ;;
esac

dnl Mingw needs explicit msvcrt for linking libwine
AC_SUBST(CRTLIBS,"")
case $host_os in
  mingw32*)
    CRTLIBS="-lmsvcrt" ;;
esac

dnl **** Get the soname for libraries that we load dynamically ****

if test "$LIBEXT" = "so"
then
  WINE_GET_SONAME(X11,XCreateWindow,[$X_LIBS $X_EXTRA_LIBS])
  WINE_GET_SONAME(Xext,XextCreateExtension,[$X_LIBS -lX11 $X_EXTRA_LIBS])
  WINE_GET_SONAME(Xrender,XRenderQueryExtension,[$X_LIBS -lXext -lX11 $X_EXTRA_LIBS])
  WINE_GET_SONAME(freetype,FT_Init_FreeType,[$X_LIBS])
  WINE_GET_SONAME(GL,glXQueryExtension,[$X_LIBS $X_EXTRA_LIBS])
  WINE_GET_SONAME(cups,cupsGetDefault)
fi


dnl **** Check for reentrant libc ****

wine_cv_libc_reentrant=no
dnl Linux style errno location
WINE_CHECK_ERRNO([__errno_location], [wine_cv_libc_reentrant=__errno_location],
  dnl FreeBSD style errno location
  WINE_CHECK_ERRNO([__error], [wine_cv_libc_reentrant=__error],
    dnl Solaris style errno location
    WINE_CHECK_ERRNO([___errno], [wine_cv_libc_reentrant=___errno],
      dnl UnixWare style errno location
      WINE_CHECK_ERRNO([__thr_errno], [wine_cv_libc_reentrant=__thr_errno],
        dnl NetBSD style errno location
        WINE_CHECK_ERRNO([__errno], [wine_cv_libc_reentrant=__errno])
))))

if test "$wine_cv_libc_reentrant" != "no"
then
  AC_DEFINE_UNQUOTED(ERRNO_LOCATION,$wine_cv_libc_reentrant,
	             [Define to the name of the function returning errno for reentrant libc])
fi

dnl **** Check for reentrant X libraries ****
dnl
dnl This may fail to determine whether X libraries are reentrant if
dnl AC_PATH_XTRA does not set x_libraries.

if test "$have_x" = "yes"
then
AC_CACHE_CHECK( [for reentrant X libraries], wine_cv_x_reentrant,
    [libX11_check=none
    for dir in "$x_libraries" /usr/lib /usr/local/lib /lib; do
        if test -r $dir/libX11.so; then
            libX11_check="-D $dir/libX11.so"
            break
        fi
        if test -r $dir/libX11.a; then
            libX11_check="$dir/libX11.a"
            break
        fi
    done
    if test "$libX11_check" != "none"; then
        if nm $libX11_check | grep $wine_cv_libc_reentrant >/dev/null 2>&1
        then
            wine_cv_x_reentrant=yes
        else
            wine_cv_x_reentrant=no
        fi
    else
        wine_cv_x_reentrant=unknown
    fi])
fi

dnl **** Check for functions ****

AC_FUNC_ALLOCA()
AC_CHECK_FUNCS(\
	_lwp_create \
	_pclose \
	_popen \
	_snprintf \
	_stricmp \
	_strnicmp \
	chsize \
	clone \
	ecvt \
	finite \
	fpclass \
	ftruncate \
	ftruncate64 \
	getnetbyaddr \
	getnetbyname \
	getpagesize \
	getprotobyname \
	getprotobynumber \
	getpwuid \
	getservbyport \
	getsockopt \
	inet_network \
	lseek64 \
	lstat \
	memmove \
	mkstemp \
	mmap \
	pclose \
	popen \
	pread \
	pwrite \
	rfork \
	select \
	sendmsg \
	settimeofday \
	sigaltstack \
	snprintf \
	statfs \
	strcasecmp \
	strerror \
	strncasecmp \
	tcgetattr \
	timegm \
	usleep \
	vfscanf \
	wait4 \
	waitpid \
)

dnl **** Check for header files ****

AC_CHECK_HEADERS(\
	arpa/inet.h \
	arpa/nameser.h \
	cups/cups.h \
	direct.h \
	elf.h \
	float.h \
	ieeefp.h \
	io.h \
	libio.h \
	libutil.h \
	link.h \
	linux/cdrom.h \
	linux/hdreg.h \
	linux/input.h \
	linux/joystick.h \
	linux/major.h \
	linux/param.h \
	linux/serial.h \
	linux/ucdrom.h \
	netdb.h \
	netinet/in.h \
	netinet/in_systm.h \
	netinet/tcp.h \
	pty.h \
	pwd.h \
	sched.h \
	scsi/sg.h \
	socket.h \
	stdint.h \
	strings.h \
	sys/cdio.h \
	sys/errno.h \
	sys/file.h \
	sys/filio.h \
	sys/inttypes.h \
	sys/ioctl.h \
	sys/ipc.h \
	sys/link.h \
	sys/lwp.h \
	sys/mman.h \
	sys/modem.h \
	sys/msg.h \
	sys/param.h \
	sys/poll.h \
	sys/ptrace.h \
	sys/reg.h \
	sys/shm.h \
	sys/signal.h \
	sys/socket.h \
	sys/sockio.h \
	sys/statfs.h \
	sys/strtio.h \
	sys/syscall.h \
	sys/sysctl.h \
	sys/time.h \
	sys/times.h \
	sys/uio.h \
	sys/un.h \
	sys/v86.h \
	sys/v86intr.h \
	sys/vfs.h \
	sys/vm86.h \
	sys/wait.h \
	syscall.h \
	termios.h \
	unistd.h \
	utime.h \
)
AC_HEADER_STAT()

dnl **** Checks for headers that depend on other ones ****

AC_CHECK_HEADERS(sys/mount.h sys/user.h,,,
    [#include <sys/types.h>
     #if HAVE_SYS_PARAM_H
     # include <sys/param.h>
     #endif])

AC_CHECK_HEADERS(net/if.h,,,
    [#include <sys/types.h>
     #if HAVE_SYS_SOCKET_H
     # include <sys/socket.h>
     #endif])

AC_CHECK_HEADERS(netinet/ip.h,,,
    [#include <sys/types.h>
     #if HAVE_SYS_SOCKET_H
     # include <sys/socket.h>
     #endif
     #if HAVE_NETINET_IN_SYSTM_H
     # include <netinet/in_systm.h>
     #endif])

AC_CHECK_HEADERS(resolv.h,,,
    [#include <sys/types.h>
     #if HAVE_SYS_SOCKET_H
     # include <sys/socket.h>
     #endif])

AC_CHECK_HEADERS(ucontext.h,,,[#include <signal.h>])

dnl **** Check for types ****

AC_C_CONST
AC_C_INLINE
AC_CHECK_TYPES([mode_t, off_t, pid_t, size_t, ssize_t])
AC_CHECK_SIZEOF(long long,0)

AC_CACHE_CHECK([whether linux/input.h is for real],
	wine_cv_linux_input_h,
	AC_TRY_COMPILE([
	    #include <linux/input.h>
	] , [
	    int foo = EVIOCGBIT(EV_ABS,42);
	    int bar = BTN_PINKIE;
	    int fortytwo = 42;
	],
	wine_cv_linux_input_h=yes,
	wine_cv_linux_input_h=no,
	no
	)
    )
    if test "$wine_cv_linux_input_h" = "yes"
    then
        AC_DEFINE(HAVE_CORRECT_LINUXINPUT_H, 1,
                  [Define if we have linux/input.h AND it contains the INPUT event API])
    fi


AC_CACHE_CHECK([whether we can use re-entrant gethostbyname_r Linux style],
   	wine_cv_linux_gethostbyname_r_6,
	AC_TRY_COMPILE([
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

dnl **** statfs checks ****

if test "$ac_cv_header_sys_vfs_h" = "yes"
then
    AC_CACHE_CHECK( [whether sys/vfs.h defines statfs],
		    wine_cv_sys_vfs_has_statfs,
	AC_TRY_COMPILE([
	#include <sys/types.h>
	#ifdef HAVE_SYS_PARAM_H
	# include <sys/param.h>
	#endif
	#include <sys/vfs.h>
	],[
		struct statfs stfs;

		memset(&stfs,0,sizeof(stfs));
	],wine_cv_sys_vfs_has_statfs=yes,wine_cv_sys_vfs_has_statfs=no
	)
    )
    if test "$wine_cv_sys_vfs_has_statfs" = "yes"
    then
      AC_DEFINE(STATFS_DEFINED_BY_SYS_VFS, 1,
                [Define if the struct statfs is defined by <sys/vfs.h>])
    fi
fi

if test "$ac_cv_header_sys_statfs_h" = "yes"
then
    AC_CACHE_CHECK( [whether sys/statfs.h defines statfs],
		    wine_cv_sys_statfs_has_statfs,
	AC_TRY_COMPILE([
	#include <sys/types.h>
	#ifdef HAVE_SYS_PARAM_H
	# include <sys/param.h>
	#endif
	#include <sys/statfs.h>
	],[
		struct statfs stfs;
	],wine_cv_sys_statfs_has_statfs=yes,wine_cv_sys_statfs_has_statfs=no
	)
    )
    if test "$wine_cv_sys_statfs_has_statfs" = "yes"
    then
      AC_DEFINE(STATFS_DEFINED_BY_SYS_STATFS, 1,
                [Define if the struct statfs is defined by <sys/statfs.h>])
    fi
fi

if test "$ac_cv_header_sys_mount_h" = "yes"
then
    AC_CACHE_CHECK( [whether sys/mount.h defines statfs],
		    wine_cv_sys_mount_has_statfs,
	AC_TRY_COMPILE([
	#include <sys/types.h>
	#ifdef HAVE_SYS_PARAM_H
	# include <sys/param.h>
	#endif
	#include <sys/mount.h>
	],[
		struct statfs stfs;
	],wine_cv_sys_mount_has_statfs=yes,wine_cv_sys_mount_has_statfs=no
	)
    )
    if test "$wine_cv_sys_mount_has_statfs" = "yes"
    then
      AC_DEFINE(STATFS_DEFINED_BY_SYS_MOUNT, 1,
                [Define if the struct statfs is defined by <sys/mount.h>])
    fi
fi

dnl **** FIXME: what about mixed cases, where we need two of them? ***

WINE_CHECK_STRUCT_MEMBER(statfs,f_bfree,
[#include <sys/types.h>
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef STATFS_DEFINED_BY_SYS_MOUNT
# include <sys/mount.h>
#else
# ifdef STATFS_DEFINED_BY_SYS_VFS
#  include <sys/vfs.h>
# else
#  ifdef STATFS_DEFINED_BY_SYS_STATFS
#   include <sys/statfs.h>
#  endif
# endif
#endif],
    [AC_DEFINE(STATFS_HAS_BFREE, 1, [Define if the struct statfs has the member bfree])])

WINE_CHECK_STRUCT_MEMBER(statfs,f_bavail,
[#include <sys/types.h>
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef STATFS_DEFINED_BY_SYS_MOUNT
# include <sys/mount.h>
#else
# ifdef STATFS_DEFINED_BY_SYS_VFS
#  include <sys/vfs.h>
# else
#  ifdef STATFS_DEFINED_BY_SYS_STATFS
#   include <sys/statfs.h>
#  endif
# endif
#endif],
    [AC_DEFINE(STATFS_HAS_BAVAIL, 1, [Define if the struct statfs has the member bavail])])

dnl Check for file descriptor passing with msg_accrights
WINE_CHECK_STRUCT_MEMBER(msghdr,msg_accrights,
[#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif],
    [AC_DEFINE(HAVE_MSGHDR_ACCRIGHTS, 1, [Define if struct msghdr contains msg_accrights])])

dnl Check for the sa_len member in struct sockaddr
WINE_CHECK_STRUCT_MEMBER(sockaddr,sa_len,
[#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif],
    [AC_DEFINE(HAVE_SOCKADDR_SA_LEN, 1, [Define if struct sockaddr contains sa_len])])

dnl Check for the sun_len member in struct sockaddr_un
WINE_CHECK_STRUCT_MEMBER(sockaddr_un,sun_len,
[#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_UN_H
# include <sys/un.h>
#endif],
    [AC_DEFINE(HAVE_SOCKADDR_SUN_LEN, 1, [Define if struct sockaddr_un contains sun_len])])

dnl *** check for the need to define __i386__

case $host_cpu in
  *i[3456789]86* )
    AC_CACHE_CHECK([whether we need to define __i386__],ac_cv_cpp_def_i386,
      AC_EGREP_CPP(yes,[#ifndef __i386__
yes
#endif],
 ac_cv_cpp_def_i386="yes", ac_cv_cpp_def_i386="no"))
    ;;
esac
if test "$ac_cv_cpp_def_i386" = "yes"
then
    CFLAGS="$CFLAGS -D__i386__"
    LINTFLAGS="$LINTFLAGS -D__i386__"
fi

dnl *** check for the need to define __sparc__

case $host_cpu in
  *sparc* )
    AC_CACHE_CHECK([whether we need to define __sparc__],ac_cv_cpp_def_sparc,
      AC_EGREP_CPP(yes,[#ifndef __sparc__
yes
#endif],
 ac_cv_cpp_def_sparc="yes", ac_cv_cpp_def_sparc="no"))
    ;;
esac
if test "$ac_cv_cpp_def_sparc" = "yes"
then
    CFLAGS="$CFLAGS -D__sparc__"
    LINTFLAGS="$LINTFLAGS -D__sparc__"
fi

dnl *** check for the need to define __sun__

case $host_vendor in
  *sun* )
    AC_CACHE_CHECK([whether we need to define __sun__],ac_cv_cpp_def_sun,
      AC_EGREP_CPP(yes,[#ifndef __sun__
yes
#endif],
 ac_cv_cpp_def_sun="yes", ac_cv_cpp_def_sun="no"))
    ;;
esac
if test "$ac_cv_cpp_def_sun" = "yes"
then
    CFLAGS="$CFLAGS -D__sun__"
    LINTFLAGS="$LINTFLAGS -D__sun__"
fi

dnl **** Generate output files ****

AH_TOP([#define __WINE_CONFIG_H])

WINE_CONFIG_EXTRA_DIR(controls)
WINE_CONFIG_EXTRA_DIR(dlls/ddraw/d3ddevice)
WINE_CONFIG_EXTRA_DIR(dlls/ddraw/dclipper)
WINE_CONFIG_EXTRA_DIR(dlls/ddraw/ddraw)
WINE_CONFIG_EXTRA_DIR(dlls/ddraw/direct3d)
WINE_CONFIG_EXTRA_DIR(dlls/ddraw/dpalette)
WINE_CONFIG_EXTRA_DIR(dlls/ddraw/dsurface)
WINE_CONFIG_EXTRA_DIR(dlls/dinput/joystick)
WINE_CONFIG_EXTRA_DIR(dlls/dinput/keyboard)
WINE_CONFIG_EXTRA_DIR(dlls/dinput/mouse)
WINE_CONFIG_EXTRA_DIR(dlls/gdi/enhmfdrv)
WINE_CONFIG_EXTRA_DIR(dlls/gdi/mfdrv)
WINE_CONFIG_EXTRA_DIR(dlls/gdi/win16drv)
WINE_CONFIG_EXTRA_DIR(dlls/kernel/messages)
WINE_CONFIG_EXTRA_DIR(dlls/user/dde)
WINE_CONFIG_EXTRA_DIR(dlls/user/resources)
WINE_CONFIG_EXTRA_DIR(dlls/wineps/data)
WINE_CONFIG_EXTRA_DIR(files)
WINE_CONFIG_EXTRA_DIR(graphics)
WINE_CONFIG_EXTRA_DIR(graphics/x11drv)
WINE_CONFIG_EXTRA_DIR(if1632)
WINE_CONFIG_EXTRA_DIR(include/wine)
WINE_CONFIG_EXTRA_DIR(loader)
WINE_CONFIG_EXTRA_DIR(loader/ne)
WINE_CONFIG_EXTRA_DIR(memory)
WINE_CONFIG_EXTRA_DIR(misc)
WINE_CONFIG_EXTRA_DIR(msdos)
WINE_CONFIG_EXTRA_DIR(objects)
WINE_CONFIG_EXTRA_DIR(programs/regapi/tests)
WINE_CONFIG_EXTRA_DIR(programs/regedit/tests)
WINE_CONFIG_EXTRA_DIR(relay32)
WINE_CONFIG_EXTRA_DIR(scheduler)
WINE_CONFIG_EXTRA_DIR(win32)
WINE_CONFIG_EXTRA_DIR(windows)

MAKE_RULES=Make.rules
AC_SUBST_FILE(MAKE_RULES)

MAKE_DLL_RULES=dlls/Makedll.rules
AC_SUBST_FILE(MAKE_DLL_RULES)

MAKE_TEST_RULES=dlls/Maketest.rules
AC_SUBST_FILE(MAKE_TEST_RULES)

MAKE_PROG_RULES=programs/Makeprog.rules
AC_SUBST_FILE(MAKE_PROG_RULES)

AC_CONFIG_FILES([
Make.rules
dlls/Makedll.rules
dlls/Maketest.rules
programs/Makeprog.rules
Makefile
dlls/Makefile
dlls/advapi32/Makefile
dlls/advapi32/tests/Makefile
dlls/avicap32/Makefile
dlls/avifil32/Makefile
dlls/cabinet/Makefile
dlls/comcat/Makefile
dlls/comctl32/Makefile
dlls/commdlg/Makefile
dlls/crtdll/Makefile
dlls/crypt32/Makefile
dlls/d3d8/Makefile
dlls/dciman32/Makefile
dlls/ddraw/Makefile
dlls/devenum/Makefile
dlls/dinput/Makefile
dlls/dinput8/Makefile
dlls/dplay/Makefile
dlls/dplayx/Makefile
dlls/dsound/Makefile
dlls/gdi/Makefile
dlls/gdi/tests/Makefile
dlls/glu32/Makefile
dlls/icmp/Makefile
dlls/imagehlp/Makefile
dlls/imm32/Makefile
dlls/kernel/Makefile
dlls/kernel/tests/Makefile
dlls/lzexpand/Makefile
dlls/mapi32/Makefile
dlls/mpr/Makefile
dlls/msacm/Makefile
dlls/msacm/imaadp32/Makefile
dlls/msacm/msadp32/Makefile
dlls/msacm/msg711/Makefile
dlls/msacm/winemp3/Makefile
dlls/msdmo/Makefile
dlls/msimg32/Makefile
dlls/msisys/Makefile
dlls/msnet32/Makefile
dlls/msvcrt/Makefile
dlls/msvcrt/tests/Makefile
dlls/msvcrt20/Makefile
dlls/msvideo/Makefile
dlls/msvideo/msrle32/Makefile
dlls/netapi32/Makefile
dlls/netapi32/tests/Makefile
dlls/ntdll/Makefile
dlls/ntdll/tests/Makefile
dlls/odbc32/Makefile
dlls/ole32/Makefile
dlls/oleaut32/Makefile
dlls/oleaut32/tests/Makefile
dlls/olecli/Makefile
dlls/oledlg/Makefile
dlls/olepro32/Makefile
dlls/olesvr/Makefile
dlls/opengl32/Makefile
dlls/psapi/Makefile
dlls/qcap/Makefile
dlls/quartz/Makefile
dlls/rasapi32/Makefile
dlls/richedit/Makefile
dlls/rpcrt4/Makefile
dlls/rpcrt4/tests/Makefile
dlls/serialui/Makefile
dlls/setupapi/Makefile
dlls/shdocvw/Makefile
dlls/shell32/Makefile
dlls/shell32/tests/Makefile
dlls/shfolder/Makefile
dlls/shlwapi/Makefile
dlls/shlwapi/tests/Makefile
dlls/snmpapi/Makefile
dlls/sti/Makefile
dlls/tapi32/Makefile
dlls/ttydrv/Makefile
dlls/twain/Makefile
dlls/url/Makefile
dlls/urlmon/Makefile
dlls/urlmon/tests/Makefile
dlls/user/Makefile
dlls/user/tests/Makefile
dlls/version/Makefile
dlls/win32s/Makefile
dlls/winaspi/Makefile
dlls/winedos/Makefile
dlls/wineps/Makefile
dlls/wininet/Makefile
dlls/wininet/tests/Makefile
dlls/winmm/Makefile
dlls/winmm/joystick/Makefile
dlls/winmm/mcianim/Makefile
dlls/winmm/mciavi/Makefile
dlls/winmm/mcicda/Makefile
dlls/winmm/mciseq/Makefile
dlls/winmm/mciwave/Makefile
dlls/winmm/midimap/Makefile
dlls/winmm/tests/Makefile
dlls/winmm/wavemap/Makefile
dlls/winmm/winealsa/Makefile
dlls/winmm/winearts/Makefile
dlls/winmm/wineaudioio/Makefile
dlls/winmm/winenas/Makefile
dlls/winmm/wineoss/Makefile
dlls/winnls/Makefile
dlls/winsock/Makefile
dlls/winsock/tests/Makefile
dlls/winspool/Makefile
dlls/wintrust/Makefile
dlls/wow32/Makefile
dlls/wsock32/Makefile
dlls/x11drv/Makefile
documentation/Makefile
include/Makefile
library/Makefile
miscemu/Makefile
ole/Makefile
programs/Makefile
programs/avitools/Makefile
programs/clock/Makefile
programs/cmdlgtst/Makefile
programs/control/Makefile
programs/expand/Makefile
programs/notepad/Makefile
programs/osversioncheck/Makefile
programs/progman/Makefile
programs/regapi/Makefile
programs/regedit/Makefile
programs/regsvr32/Makefile
programs/regtest/Makefile
programs/rundll32/Makefile
programs/uninstaller/Makefile
programs/view/Makefile
programs/wcmd/Makefile
programs/wineconsole/Makefile
programs/winedbg/Makefile
programs/winefile/Makefile
programs/winemine/Makefile
programs/winepath/Makefile
programs/winhelp/Makefile
programs/winver/Makefile
server/Makefile
tools/Makefile
tools/widl/Makefile
tools/winapi/Makefile
tools/winebuild/Makefile
tools/winedump/Makefile
tools/wmc/Makefile
tools/wpp/Makefile
tools/wrc/Makefile
unicode/Makefile])

AC_OUTPUT

if test "$have_x" = "no"
then
  echo
  echo "*** Warning: X development files not found. Wine will be built without"
  echo "*** X support, which currently does not work, and would probably not be"
  echo "*** what you want anyway. You will need to install devel packages of"
  echo "*** Xlib/Xfree86 at the very least."
fi

if test "$ac_cv_lib_ncurses_resizeterm" = "no" -a "$ac_cv_lib_ncurses_waddch" = "yes"
then
  echo
  echo "*** Warning: resizeterm not found in ncurses. Wine will be built without"
  echo "*** terminal resize support. Consider upgrading ncurses."
fi

if test "$wine_cv_libc_reentrant" = "no"
then
  echo
  echo "*** Warning: non-reentrant libc detected. Wine will be built without"
  echo "*** threading support. Consider upgrading libc to a more recent"
  echo "*** reentrant version of libc."
fi

if test "$have_x" = "yes" -a "$wine_cv_x_reentrant" != "yes"
then
  echo
  echo "*** Warning: non-reentrant X11 library detected. Multi-threaded"
  echo "*** applications won't work properly. You should upgrade your X11 library."
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

echo
echo "Configure finished.  Do 'make depend && make' to compile Wine."
echo

dnl Local Variables:
dnl comment-start: "dnl "
dnl comment-end: ""
dnl comment-start-skip: "\\bdnl\\b\\s *"
dnl compile-command: "autoconf"
dnl End:
