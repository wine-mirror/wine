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

dnl Default values
LIBEXT=so       # library type .so or .a

AC_ARG_ENABLE(debug, AC_HELP_STRING([--disable-debug],[compile out all debugging messages]))
AC_ARG_ENABLE(trace, AC_HELP_STRING([--disable-trace],[compile out TRACE messages]))
AC_ARG_ENABLE(opengl,AC_HELP_STRING([--enable-opengl],[force usage of OpenGL even if the latter is thread-safe via pthread]))
AC_ARG_WITH(curses,     AC_HELP_STRING([--without-curses],[do not use curses]))

AC_SUBST(OPTIONS)
if test "x$enable_debug" = "xno"
then
    AC_DEFINE(NO_DEBUG_MSGS,1,[Define to disable all debug messages.])
fi
if test "x$enable_trace" = "xno" -o "x$enable_debug" = "xno"
then
    AC_DEFINE(NO_TRACE_MSGS,1,[Define to disable trace messages.])
fi

dnl **** Check for some programs ****

AC_CANONICAL_TARGET
AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_CPP
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
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_LN_S
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

case $host_os in
  *cygwin*) LDCOMBINE="ld -r --enable-stdcall-fixup" ;;
         *) LDCOMBINE="ld -r" ;;
esac
AC_SUBST(LDCOMBINE)

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
            AC_MSG_WARN([[Xkb extension not found, Wine will be built without it]])
        )

        dnl *** Check for X Shm extension
        AC_CHECK_HEADERS(X11/extensions/XShm.h,
            [ dnl *** If X11/extensions/XShm.h exists...
              AC_CHECK_LIB(Xext, XShmQueryExtension,
              AC_DEFINE(HAVE_LIBXXSHM, 1, [Define if you have the X Shm extension]),,
              $X_LIBS -lXext -lX11 $X_EXTRA_LIBS)
            ],
            AC_MSG_WARN([[XShm extension not found, Wine will be built without it]]),
            [#include <X11/Xlib.h>])

        dnl *** Check for X shape extension
        AC_CHECK_HEADERS(X11/extensions/shape.h,
            [ dnl *** If X11/extensions/shape.h exists...
              AC_CHECK_LIB(Xext,XShapeQueryExtension,
	      AC_DEFINE(HAVE_LIBXSHAPE, 1, [Define if you have the X Shape extension]),,
	      $X_LIBS -lXext -lX11 $X_EXTRA_LIBS)
	    ],
	    AC_MSG_WARN([[XShape extension not found, Wine will be built without it]]),
            [#include <X11/Xlib.h>])
        
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
    if test "x$enable_opengl" != "xno"
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

	    dnl Check for the thread-safety of the OpenGL library
	    AC_CACHE_CHECK([for thread-safe OpenGL version], 
			   wine_cv_opengl_version_threadsafe,
              [saved_libs=$LIBS
	       LIBS="$X_LIBS -lGL"
	       AC_TRY_LINK([],[pthread_getspecific();],
			      [wine_cv_opengl_version_threadsafe="yes"],
			      [wine_cv_opengl_version_threadsafe="no"])
	       LIBS=$saved_libs]
	    )

	    if test "$wine_cv_opengl_version_OK" = "yes" -a \( "$wine_cv_opengl_version_threadsafe" = "no" -o "x$enable_opengl" = "xyes" \)
            then
		dnl Check for the presence of the library
		AC_CHECK_LIB(GL,glXCreateContext,
			     X_PRE_LIBS="$X_PRE_LIBS -lGL"
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
			     [X_PRE_LIBS="$X_PRE_LIBS -lGLU"
			     GLU32FILES='$(GLU32FILES)']
			     ,,
			     $X_LIBS $X_PRE_LIBS -lXext -lX11 -lm $X_EXTRA_LIBS
		)
	     fi
	 fi
    fi

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

CUPSLIBS=""
dnl **** Check for CUPS ****
wine_cv_warn_cups_h=no
AC_CHECK_LIB(cups,cupsGetPPD,
	[AC_CHECK_HEADER(cups/cups.h,
	    [AC_DEFINE(HAVE_CUPS, 1, [Define if we have CUPS])
	    CUPSLIBS="-lcups"],
            wine_cv_warn_cups_h=yes)]
)
AC_SUBST(CUPSLIBS)

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

dnl **** Check for IPX (currently Linux only) ****
AC_CACHE_CHECK([for GNU style IPX support], ac_cv_c_ipx_gnu,
 AC_TRY_COMPILE(
   [#include <sys/socket.h>
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
    [#include <sys/socket.h>
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
  AC_CACHE_CHECK([for gcc -mpreferred-stack-boundary=2 support],
                 ac_cv_c_gcc_stack_boundary,
  [saved_cflags=$CFLAGS
  CFLAGS="$CFLAGS -mpreferred-stack-boundary=2"
  AC_TRY_COMPILE(,[return 0],ac_cv_c_gcc_stack_boundary="yes",ac_cv_c_gcc_stack_boundary="no")
  CFLAGS=$saved_cflags
  ])
  if test "$ac_cv_c_gcc_stack_boundary" = "yes"
  then
    CFLAGS="$CFLAGS -mpreferred-stack-boundary=2"
  fi
fi

dnl **** Check if we need to place .type inside a .def directive ****

AC_CACHE_CHECK([whether .type must sit inside a .def directive],
	       ac_cv_c_type_in_def,
[saved_libs=$LIBS
LIBS="conftest_asm.s $LIBS"
cat > conftest_asm.s <<EOF
	.globl _ac_test
	.def _ac_test; .scl 2; .type 32; .endef
_ac_test:
	.long 0
EOF
AC_TRY_LINK(,,ac_cv_c_type_in_def="yes",ac_cv_c_type_in_def="no")
LIBS=$saved_libs])
if test "$ac_cv_c_type_in_def" = "yes"
then
  AC_DEFINE(NEED_TYPE_IN_DEF, 1, [Define if .type asm directive must be inside a .def directive])
fi
 
dnl **** Check for underscore on external symbols ****

AC_CACHE_CHECK([whether external symbols need an underscore prefix],
               ac_cv_c_extern_prefix,
[saved_libs=$LIBS
LIBS="conftest_asm.s $LIBS"
cat > conftest_asm.s <<EOF
	.globl _ac_test
_ac_test:
	.long 0
EOF
AC_TRY_LINK([extern int ac_test;],[if (ac_test) return 1],
            ac_cv_c_extern_prefix="yes",ac_cv_c_extern_prefix="no")
LIBS=$saved_libs])
if test "$ac_cv_c_extern_prefix" = "yes"
then
  AC_DEFINE(NEED_UNDERSCORE_PREFIX, 1,
            [Define if symbols declared in assembly code need an underscore prefix])
fi

dnl **** Check whether stdcall symbols need to be decorated ****

AC_CACHE_CHECK([whether stdcall symbols need to be decorated],
               ac_cv_c_stdcall_decoration,
[saved_libs=$LIBS
LIBS="conftest_asm.s $LIBS"
if test "$ac_cv_c_extern_prefix" = "yes"
then
cat > conftest_asm.s <<EOF
	.globl _ac_test@0
_ac_test@0:
EOF
else
cat > conftest_asm.s <<EOF
	.globl ac_test@0
ac_test@0:
EOF
fi
AC_TRY_LINK([extern void __attribute__((__stdcall__)) ac_test(void);],
            [ac_test(); return 0],
            ac_cv_c_stdcall_decoration="yes",ac_cv_c_stdcall_decoration="no")
LIBS=$saved_libs])
if test "$ac_cv_c_stdcall_decoration" = "yes"
then
  AC_DEFINE(NEED_STDCALL_DECORATION, 1,
            [Define if stdcall symbols need to be decorated])
fi

dnl **** Check for .string in assembler ****

AC_CACHE_CHECK([whether assembler accepts .string],
               ac_cv_c_asm_string,
[saved_libs=$LIBS
LIBS="conftest_asm.s $LIBS"
cat > conftest_asm.s <<EOF
	.string "test"
EOF
AC_TRY_LINK(,,ac_cv_c_asm_string="yes",ac_cv_c_asm_string="no")
LIBS=$saved_libs])
if test "$ac_cv_c_asm_string" = "yes"
then
  AC_DEFINE(HAVE_ASM_STRING, 1, [Define to use .string instead of .ascii])
fi

dnl **** Check for working dll ****

LDSHARED=""
LDDLLFLAGS=""
DLLIBS=""

case $target_os in
  cygwin|mingw32)
    AC_CHECK_PROG(DLLWRAP,dllwrap,dllwrap,false)
    if test "$DLLWRAP" = "dllwrap"; then
      dnl FIXME - check whether dllwrap works correctly...
      LIBEXT="dll"
    else
      LIBEXT="a"
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
      AC_CACHE_CHECK([whether we can build a GNU style ELF dll],ac_cv_c_dll_gnuelf,
      [saved_cflags=$CFLAGS
      CFLAGS="$CFLAGS -fPIC -shared -Wl,-soname,conftest.so.1.0,-Bsymbolic"
      AC_TRY_LINK(,[return 1],ac_cv_c_dll_gnuelf="yes",ac_cv_c_dll_gnuelf="no")
      CFLAGS=$saved_cflags
      ])
      if test "$ac_cv_c_dll_gnuelf" = "yes"
      then
        LDSHARED="\$(CC) -shared \$(SONAME:%=-Wl,-soname,%)"
        LDDLLFLAGS="-Wl,-Bsymbolic"
      else
        AC_CACHE_CHECK(whether we can build a UnixWare (Solaris) dll,
                     ac_cv_c_dll_unixware,
        [saved_cflags=$CFLAGS
        CFLAGS="$CFLAGS -fPIC -Wl,-G,-h,conftest.so.1.0,-B,symbolic"
        AC_TRY_LINK(,[return 1],ac_cv_c_dll_unixware="yes",ac_cv_c_dll_unixware="no")
        CFLAGS=$saved_cflags
        ])
        if test "$ac_cv_c_dll_unixware" = "yes"
        then
          LDSHARED="\$(CC) -Wl,-G \$(SONAME:%=-Wl,-h,%)"
          LDDLLFLAGS="-Wl,-B,symbolic"
        fi
      fi
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

DLLFLAGS=""
LDPATH=""

if test "$LIBEXT" = "so"; then
    DLLFLAGS="-fPIC"
    DLLEXT=".so"
    LDPATH="LD_LIBRARY_PATH=\"\$(TOPOBJDIR)/library:\$(TOPOBJDIR)/unicode:\$\$LD_LIBRARY_PATH\""
elif test "$LIBEXT" = "dll"; then
    #DLLFLAGS="-fPIC" # -fPIC doesn't work(at least in cygwin-b20) - FIXME
    DLLEXT=""
    LDPATH="PATH=\"\$(TOPOBJDIR)/library:\$(TOPOBJDIR)/unicode:\$\$PATH\""
fi

AC_SUBST(DLLIBS)
AC_SUBST(DLLFLAGS)
AC_SUBST(DLLEXT)
AC_SUBST(LDSHARED)
AC_SUBST(LDDLLFLAGS)
AC_SUBST(LIBEXT)
AC_SUBST(LDPATH)

dnl **** Check for reentrant libc ****
dnl
dnl For cross-compiling we blindly assume that libc is reentrant. This is
dnl ok since non-reentrant libc is quite rare (mostly old libc5 versions).

AC_DEFUN([WINE_CHECK_ERRNO],
[
  AC_CACHE_CHECK(for reentrant libc: $1, wine_cv_libc_r_$1,
  [AC_TRY_RUN([int myerrno = 0;
char buf[256];
int *$1(){return &myerrno;}
main(){connect(0,buf,255); exit(!myerrno);}],
  wine_cv_libc_r_$1=yes, wine_cv_libc_r_$1=no,
  wine_cv_libc_r_$1=yes )
])
if test "$wine_cv_libc_r_$1" = "yes"
then
    wine_cv_libc_reentrant=$1 
fi
])

wine_cv_libc_reentrant=no 
dnl Linux style errno location
WINE_CHECK_ERRNO(__errno_location)
dnl FreeBSD style errno location
WINE_CHECK_ERRNO(__error)
dnl Solaris style errno location
WINE_CHECK_ERRNO(___errno)
dnl UnixWare style errno location
WINE_CHECK_ERRNO(__thr_errno)
dnl NetBSD style errno location
WINE_CHECK_ERRNO(__errno)

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
	__libc_fork \
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
	getservbyport \
	getsockopt \
	inet_network \
	lseek64 \
	lstat \
	memmove \
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
	net/if.h \
	netdb.h \
	netinet/in.h \
	netinet/in_systm.h \
	netinet/ip.h \
	netinet/tcp.h \
	pty.h \
	resolv.h \
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
	sys/ipc.h \
	sys/link.h \
	sys/lwp.h \
	sys/mman.h \
	sys/modem.h \
	sys/mount.h \
	sys/msg.h \
	sys/param.h \
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
	sys/user.h \
	sys/v86.h \
	sys/v86intr.h \
	sys/vfs.h \
	sys/vm86.h \
	sys/wait.h \
	syscall.h \
	ucontext.h \
	unistd.h \
)
AC_HEADER_STAT()

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

dnl *** Check for some structure members

dnl Macro to check if a structure contains a specified member
dnl Usage: WINE_CHECK_STRUCT_MEMBER(struct,member,[includes,[action-if-found,[action-if-not-found]]])

AC_DEFUN([WINE_CHECK_STRUCT_MEMBER],
[AC_CACHE_CHECK([for $2 in struct $1], ac_cv_c_$1_$2,
 AC_TRY_COMPILE([$3],[struct $1 s; s.$2 = 0],ac_cv_c_$1_$2="yes",ac_cv_c_$1_$2="no"))
AS_IF([ test "x$ac_cv_c_$1_$2" = "xyes"],[$4],[$5])
])

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
#include <sys/socket.h>],
    [AC_DEFINE(HAVE_MSGHDR_ACCRIGHTS, 1, [Define if struct msghdr contains msg_accrights])])

dnl Check for the sa_len member in struct sockaddr
WINE_CHECK_STRUCT_MEMBER(sockaddr,sa_len,
[#include <sys/types.h>
#include <sys/socket.h>],
    [AC_DEFINE(HAVE_SOCKADDR_SA_LEN, 1, [Define if struct sockaddr contains sa_len])])

dnl Check for the sun_len member in struct sockaddr_un
WINE_CHECK_STRUCT_MEMBER(sockaddr_un,sun_len,
[#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>],
    [AC_DEFINE(HAVE_SOCKADDR_SUN_LEN, 1, [Define if struct sockaddr_un contains sun_len])])

dnl *** check for the need to define __i386__

case $target_cpu in
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

dnl **** Generate output files ****

dnl Macro to create non-existent directories from config.status
dnl Usage: WINE_CONFIG_EXTRA_DIR(dirname)
AC_DEFUN([WINE_CONFIG_EXTRA_DIR],
[AC_CONFIG_COMMANDS([$1],[test -d "$1" || (AC_MSG_NOTICE([creating $1]) && mkdir "$1")])])

AH_TOP([#define __WINE_CONFIG_H])

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
WINE_CONFIG_EXTRA_DIR(dlls/kernel/tests)
WINE_CONFIG_EXTRA_DIR(dlls/oleaut32/tests)
WINE_CONFIG_EXTRA_DIR(dlls/shlwapi/tests)
WINE_CONFIG_EXTRA_DIR(dlls/user/dde)
WINE_CONFIG_EXTRA_DIR(dlls/user/resources)
WINE_CONFIG_EXTRA_DIR(dlls/user/tests)
WINE_CONFIG_EXTRA_DIR(dlls/wineps/data)
WINE_CONFIG_EXTRA_DIR(dlls/winsock/tests)
WINE_CONFIG_EXTRA_DIR(include/wine)
WINE_CONFIG_EXTRA_DIR(programs/regapi/tests)
WINE_CONFIG_EXTRA_DIR(programs/winetest/tests)

MAKE_RULES=Make.rules
AC_SUBST_FILE(MAKE_RULES)

MAKE_DLL_RULES=dlls/Makedll.rules
AC_SUBST_FILE(MAKE_DLL_RULES)

MAKE_PROG_RULES=programs/Makeprog.rules
AC_SUBST_FILE(MAKE_PROG_RULES)

AC_CONFIG_FILES([
Make.rules
dlls/Makedll.rules
programs/Makeprog.rules
Makefile
controls/Makefile
debugger/Makefile
dlls/Makefile
dlls/advapi32/Makefile
dlls/avicap32/Makefile
dlls/avifil32/Makefile
dlls/comctl32/Makefile
dlls/commdlg/Makefile
dlls/crtdll/Makefile
dlls/crypt32/Makefile
dlls/dciman32/Makefile
dlls/ddraw/Makefile
dlls/devenum/Makefile
dlls/dinput/Makefile
dlls/dplay/Makefile
dlls/dplayx/Makefile
dlls/dsound/Makefile
dlls/gdi/Makefile
dlls/glu32/Makefile
dlls/icmp/Makefile
dlls/imagehlp/Makefile
dlls/imm32/Makefile
dlls/kernel/Makefile
dlls/lzexpand/Makefile
dlls/mapi32/Makefile
dlls/mpr/Makefile
dlls/msacm/Makefile
dlls/msacm/imaadp32/Makefile
dlls/msacm/msg711/Makefile
dlls/msdmo/Makefile
dlls/msimg32/Makefile
dlls/msisys/Makefile
dlls/msnet32/Makefile
dlls/msrle32/Makefile
dlls/msvcrt/Makefile
dlls/msvcrt20/Makefile
dlls/msvideo/Makefile
dlls/netapi32/Makefile
dlls/ntdll/Makefile
dlls/odbc32/Makefile
dlls/ole32/Makefile
dlls/oleaut32/Makefile
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
dlls/serialui/Makefile
dlls/setupapi/Makefile
dlls/shdocvw/Makefile
dlls/shell32/Makefile
dlls/shfolder/Makefile
dlls/shlwapi/Makefile
dlls/snmpapi/Makefile
dlls/sti/Makefile
dlls/tapi32/Makefile
dlls/ttydrv/Makefile
dlls/twain/Makefile
dlls/url/Makefile
dlls/urlmon/Makefile
dlls/user/Makefile
dlls/version/Makefile
dlls/win32s/Makefile
dlls/winaspi/Makefile
dlls/winedos/Makefile
dlls/wineps/Makefile
dlls/wininet/Makefile
dlls/winmm/Makefile
dlls/winmm/joystick/Makefile
dlls/winmm/mcianim/Makefile
dlls/winmm/mciavi/Makefile
dlls/winmm/mcicda/Makefile
dlls/winmm/mciseq/Makefile
dlls/winmm/mciwave/Makefile
dlls/winmm/midimap/Makefile
dlls/winmm/wavemap/Makefile
dlls/winmm/winearts/Makefile
dlls/winmm/wineoss/Makefile
dlls/winnls/Makefile
dlls/winsock/Makefile
dlls/winspool/Makefile
dlls/wintrust/Makefile
dlls/wow32/Makefile
dlls/wsock32/Makefile
dlls/x11drv/Makefile
documentation/Makefile
files/Makefile
graphics/Makefile
graphics/x11drv/Makefile
if1632/Makefile
include/Makefile
library/Makefile
libtest/Makefile
loader/Makefile
loader/ne/Makefile
memory/Makefile
misc/Makefile
miscemu/Makefile
msdos/Makefile
objects/Makefile
ole/Makefile
programs/Makefile
programs/avitools/Makefile
programs/clock/Makefile
programs/cmdlgtst/Makefile
programs/control/Makefile
programs/notepad/Makefile
programs/osversioncheck/Makefile
programs/progman/Makefile
programs/regapi/Makefile
programs/regedit/Makefile
programs/regsvr32/Makefile
programs/regtest/Makefile
programs/uninstaller/Makefile
programs/view/Makefile
programs/wcmd/Makefile
programs/wineconsole/Makefile
programs/winemine/Makefile
programs/winepath/Makefile
programs/winetest/Makefile
programs/winhelp/Makefile
programs/winver/Makefile
relay32/Makefile
scheduler/Makefile
server/Makefile
tools/Makefile
tools/winapi/Makefile
tools/winebuild/Makefile
tools/winedump/Makefile
tools/wmc/Makefile
tools/wrc/Makefile
tsx11/Makefile
unicode/Makefile
win32/Makefile
windows/Makefile])

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

if test "$wine_cv_opengl_version_threadsafe" = "yes" -a "x$enable_opengl" = "x"
then
  echo
  echo "*** Warning: the OpenGL version you have installed relies on libpthread for"
  echo "*** thread-safety. To prevent crashes, OpenGL support has been removed."
  echo "*** A fix for glibc 2.1.3 that seems to work is included in this version of Wine,"
  echo "*** start configure with '--enable-opengl' to force OpenGL support."
fi

if test "$wine_cv_opengl_version_threadsafe" = "yes" -a "x$enable_opengl" = "xyes"
then
  echo
  echo "*** Warning: you explicitly linked in a thread-safe OpenGL version. If you"
  echo "*** experience unusual crashes on DirectDraw games, try first to disable OpenGL"
  echo "*** support before reporting bugs."
fi

if test "$wine_cv_warn_cups_h" = "yes"
then
  echo
  echo "*** Note: You have cups runtime libraries, but no development"
  echo "*** libraries. Install the cups-devel package or whichever package"
  echo "*** contains cups.h to enable CUPS support in Wine."
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
