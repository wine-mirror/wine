dnl Process this file with autoconf to produce a configure script.
dnl Original author: Michael Patra
dnl See ChangeLog file for detailed change history.

m4_define(WINE_VERSION,regexp(m4_include(VERSION),[version \([-.0-9A-Za-z]+\)],[\1]))

AC_PREREQ(2.53b)
AC_INIT([Wine],[WINE_VERSION],[wine-devel@winehq.org])
AC_CONFIG_SRCDIR(server/atom.c)
AC_CONFIG_HEADERS(include/config.h)
AC_CONFIG_AUX_DIR(tools)

dnl **** Command-line arguments ****

AC_ARG_ENABLE(win16, AS_HELP_STRING([--disable-win16],[do not include Win16 support]))
AC_ARG_ENABLE(win64, AS_HELP_STRING([--enable-win64],[build a Win64 emulator on AMD64 (won't run Win32 binaries)]))

AC_ARG_WITH(opengl,    AS_HELP_STRING([--without-opengl],[do not use OpenGL]))
AC_ARG_WITH(wine-tools,AS_HELP_STRING([--with-wine-tools=<dir>],[use Wine tools from directory <dir>]))

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

dnl check for out of tree build with unclean source tree
case "$srcdir" in
 .) ;;
 *) if test -f "$srcdir/Makefile" -o -f "$srcdir/include/config.h"; then
      AC_MSG_ERROR([you are building out of the source tree, but the source tree contains object files.
You need to run 'make distclean' in the source tree first.])
    fi ;;
esac

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

dnl Check for flex
AC_CHECK_PROGS(FLEX,flex,none)
if test "$FLEX" = "none"
then
  AC_MSG_ERROR([no suitable flex found. Please install the 'flex' package.])
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
    AC_PATH_PROG(PRELINK, prelink, false, [/sbin /usr/sbin $PATH])
    ;;
esac

dnl **** Check for some libraries ****

dnl Check for -li386 for NetBSD and OpenBSD
AC_CHECK_LIB(i386,i386_set_ldt)
dnl Check for -lossaudio for NetBSD
AC_CHECK_LIB(ossaudio,_oss_ioctl)
dnl Check for -lxpg4 for FreeBSD
AC_CHECK_LIB(xpg4,_xpg4_setrunelocale)
dnl Check for -lpthread
AC_CHECK_LIB(pthread,pthread_create,AC_SUBST(LIBPTHREAD,"-lpthread"))

AC_SUBST(XLIB,"")
AC_SUBST(XFILES,"")
AC_SUBST(OPENGLFILES,"")
AC_SUBST(GLU32FILES,"")
AC_SUBST(OPENGL_LIBS,"")
AC_SUBST(QUARTZFILES,"")

dnl **** Check for header files ****

AC_CHECK_HEADERS(\
	AudioUnit/AudioUnit.h \
	Carbon/Carbon.h \
	CoreAudio/CoreAudio.h \
	DiskArbitration/DiskArbitration.h \
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
	mach/mach.h \
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
	png.h \
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
	sys/event.h \
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
	sys/prctl.h \
	sys/ptrace.h \
	sys/reg.h \
	sys/resource.h \
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

AC_CHECK_HEADERS([linux/ipx.h],,,
    [#include <sys/types.h>
     #ifdef HAVE_ASM_TYPES_H
     # include <asm/types.h>
     #endif
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

AC_CHECK_HEADERS([pthread_np.h],,,
[#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif])

AC_CHECK_HEADERS([linux/videodev.h],,,
[#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <sys/types.h>
#ifdef HAVE_ASM_TYPES_H
#include <asm/types.h>
#endif])

dnl Check for broken kernel header that doesn't define __user
AC_CHECK_HEADERS([linux/capi.h],,,[#define __user])

dnl **** Check for working dll ****

AC_SUBST(DLLEXT,"")
AC_SUBST(DLLFLAGS,"-D_REENTRANT")
AC_SUBST(LDSHARED,"")
AC_SUBST(LDDLLFLAGS,"")
AC_SUBST(LIBEXT,"so")
AC_SUBST(IMPLIBEXT,"def")
AC_SUBST(LDRPATH_INSTALL,"")
AC_SUBST(LDRPATH_LOCAL,"")
WINE_PATH_LDD

case $host_os in
  cygwin*|mingw32*)
    AC_CHECK_TOOL(DLLTOOL,dlltool,false)
    AC_CHECK_TOOL(DLLWRAP,dllwrap,false)
    AC_CHECK_TOOL(MINGWAR,ar,false)
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
  darwin*|macosx*)
    DLLEXT=".so"
    LIBEXT="dylib"
    DLLFLAGS="$DLLFLAGS -fPIC"
    LDDLLFLAGS="-bundle -multiply_defined suppress"
    LIBWINE_LDFLAGS="-multiply_defined suppress"
    LDSHARED="\$(CC) -dynamiclib"
    STRIP="$STRIP -x"
    LDRPATH_LOCAL="&& install_name_tool -change @executable_path/\`\$(RELPATH) \$(bindir) \$(libdir)\`/libwine.1.dylib @executable_path/\$(TOPOBJDIR)/libs/wine/libwine.1.dylib \$@ || \$(RM) \$@"
    dnl declare needed frameworks
    AC_SUBST(COREFOUNDATIONLIB,"-framework CoreFoundation")
    AC_SUBST(IOKITLIB,"-framework IOKit -framework CoreFoundation")
    AC_SUBST(LDEXECFLAGS,["-image_base 0x7bf00000 -Wl,-segaddr,WINE_DOS,0x00000000,-segaddr,WINE_SHARED_HEAP,0x7f000000"])
    if test "$ac_cv_header_DiskArbitration_DiskArbitration_h" = "yes"
    then
        dnl DiskArbitration API is not public on Darwin < 8.0, use it only if header found
        AC_SUBST(DISKARBITRATIONLIB,"-framework DiskArbitration -framework CoreFoundation")
    fi
    if test "$ac_cv_header_CoreAudio_CoreAudio_h" = "yes" -a "$ac_cv_header_AudioUnit_AudioUnit_h" = "yes"
    then
        dnl CoreServices needed by AudioUnit
        AC_SUBST(COREAUDIO,"-framework CoreAudio -framework AudioUnit -framework CoreServices -framework AudioToolbox -framework CoreMIDI")
    fi
    case $host_cpu in
      *powerpc*)
        LDDLLFLAGS="$LDDLLFLAGS -read_only_relocs warning"  dnl FIXME
        ;;
    esac
    dnl Enable quartz driver on Mac OS X
    if test "$ac_cv_header_Carbon_Carbon_h" = "yes"
    then
        AC_SUBST(CARBONLIB,"-framework Carbon")
        QUARTZFILES='$(QUARTZFILES)'
    fi
    ;;
  *)
    DLLFLAGS="$DLLFLAGS -fPIC"
    DLLEXT=".so"
    AC_CACHE_CHECK([whether we can build a GNU style ELF dll], ac_cv_c_dll_gnuelf,
        [WINE_TRY_SHLIB_FLAGS([-fPIC -shared -Wl,-Bsymbolic],
                              ac_cv_c_dll_gnuelf="yes",ac_cv_c_dll_gnuelf="no")])
    if test "$ac_cv_c_dll_gnuelf" = "yes"
    then
      LDSHARED="\$(CC) -shared"
      LDDLLFLAGS="-shared -Wl,-Bsymbolic"
      WINE_TRY_CFLAGS([-fPIC -shared -Wl,-soname,confest.so.1],
                      [LDSHARED="\$(CC) -shared \$(SONAME:%=-Wl,-soname,%)"],
                      [WINE_TRY_CFLAGS([-fPIC -shared -Wl,-h,confest.so.1],
                                       [LDSHARED="\$(CC) -shared \$(SONAME:%=-Wl,-h,%)"])])

      WINE_TRY_CFLAGS([-fPIC -shared -Wl,-Bsymbolic,-z,defs], [LDDLLFLAGS="$LDDLLFLAGS,-z,defs"])

      WINE_TRY_CFLAGS([-fPIC -shared -Wl,-Bsymbolic,-init,__wine_spec_init,-fini,__wine_spec_fini],
                      [LDDLLFLAGS="$LDDLLFLAGS,-init,__wine_spec_init,-fini,__wine_spec_fini"])

      echo '{ global: *; };' >conftest.map
      WINE_TRY_CFLAGS([-fPIC -shared -Wl,--version-script=conftest.map],
                      [LDSHARED="$LDSHARED \$(VERSCRIPT:%=-Wl,--version-script=%)"])
      rm -f conftest.map

      WINE_TRY_CFLAGS([-fPIC -Wl,--export-dynamic],
                      [AC_SUBST(LDEXECFLAGS,["-Wl,--export-dynamic"])])

      WINE_TRY_CFLAGS([-fPIC -Wl,--rpath,\$ORIGIN/../lib],
                      [LDRPATH_INSTALL="-Wl,--rpath,\\\$\$ORIGIN/\`\$(RELPATH) \$(bindir) \$(libdir)\`"
                       LDRPATH_LOCAL="-Wl,--rpath,\\\$\$ORIGIN/\$(TOPOBJDIR)/libs/wine"],
          [WINE_TRY_CFLAGS([-fPIC -Wl,-R,\$ORIGIN/../lib],
                           [LDRPATH_INSTALL="-Wl,-R,\\\$\$ORIGIN/\`\$(RELPATH) \$(bindir) \$(libdir)\`"
                            LDRPATH_LOCAL="-Wl,-R,\\\$\$ORIGIN/\$(TOPOBJDIR)/libs/wine"])])

      case $host_cpu in
        *i[[3456789]]86* | x86_64)
          WINE_TRY_CFLAGS([-Wl,--section-start,.interp=0x7bf00400],
                          [LDEXECFLAGS="$LDEXECFLAGS -Wl,--section-start,.interp=0x7bf00400"])
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

    dnl Check for cross compiler to build test programs
    AC_SUBST(CROSSTEST,"")
    if test "$cross_compiling" = "no"
    then
      WINE_CHECK_MINGW_PROG(CROSSCC,gcc,false)
      WINE_CHECK_MINGW_PROG(DLLTOOL,dlltool,false)
      WINE_CHECK_MINGW_PROG(CROSSWINDRES,windres,false)
      WINE_CHECK_MINGW_PROG(MINGWAR,ar,false)
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

dnl **** Check for X11 ****

if test "$have_x" = "yes"
then
    XLIB="-lXext -lX11"
    ac_save_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $X_CFLAGS"

    WINE_CHECK_SONAME(X11,XCreateWindow,,,[$X_LIBS $X_EXTRA_LIBS])
    WINE_CHECK_SONAME(Xext,XextCreateExtension,,,[$X_LIBS -lX11 $X_EXTRA_LIBS])

    dnl *** All of the following tests require X11/Xlib.h
    AC_CHECK_HEADERS([X11/Xlib.h \
                      X11/XKBlib.h \
                      X11/Xutil.h \
                      X11/Xcursor/Xcursor.h \
                      X11/extensions/shape.h \
                      X11/extensions/XInput.h \
                      X11/extensions/XShm.h \
                      X11/extensions/Xinerama.h \
                      X11/extensions/Xrandr.h \
                      X11/extensions/Xrender.h \
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

        dnl *** Check for X cursor
        if test "$ac_cv_header_X11_Xcursor_Xcursor_h" = "yes"
        then
            WINE_CHECK_SONAME(Xcursor,XcursorImageLoadCursor,,,[$X_LIBS -lXext -lX11 $X_EXTRA_LIBS])
        fi

        dnl *** Check for X input extension
        if test "$ac_cv_header_X11_extensions_XInput_h" = "yes"
        then
            WINE_CHECK_SONAME(Xi,XOpenDevice,,,[$X_LIBS -lXext -lX11 $X_EXTRA_LIBS])
        fi

        dnl *** Check for X Shm extension
        if test "$ac_cv_header_X11_extensions_XShm_h" = "yes"
        then
              AC_CHECK_LIB(Xext, XShmQueryExtension,
              AC_DEFINE(HAVE_LIBXXSHM, 1, [Define if you have the X Shm extension]),,
              $X_LIBS -lXext -lX11 $X_EXTRA_LIBS)
        fi
        WINE_NOTICE_IF([test "$ac_cv_lib_Xext_XShmQueryExtension" != "yes"],[XShm development files not found.
Wine will be built without XShm support. (winex11.drv)])

        dnl *** Check for X shape extension
        if test "$ac_cv_header_X11_extensions_shape_h" = "yes"
        then
              AC_CHECK_LIB(Xext,XShapeQueryExtension,
	      AC_DEFINE(HAVE_LIBXSHAPE, 1, [Define if you have the X Shape extension]),,
	      $X_LIBS -lXext -lX11 $X_EXTRA_LIBS)
        fi
        WINE_NOTICE_IF([test "$ac_cv_lib_Xext_XShapeQueryExtension" != "yes"],[XShape development files not found.
Wine will be built without XShape support. (winex11.drv)])

        dnl *** Check for XFree86 VMODE extension
        if test "$ac_cv_header_X11_extensions_xf86vmode_h" = "yes"
        then
                AC_CHECK_LIB(Xxf86vm, XF86VidModeQueryExtension,
                  [ AC_DEFINE(HAVE_LIBXXF86VM, 1, [Define if you have the Xxf86vm library])
                     X_PRE_LIBS="$X_PRE_LIBS -lXxf86vm"
                  ],,
                  $X_LIBS -lXext -lX11 $X_EXTRA_LIBS)
        fi
        WINE_NOTICE_IF([test "$ac_cv_lib_Xxf86vm_XF86VidModeQueryExtension" != "yes"],[Xxf86vm development files not found.
Wine will be built without XF86 Vidmode support. (winex11.drv)])

        dnl *** Check for Transform functions in Xrender
        if test "$ac_cv_header_X11_extensions_Xrender_h" = "yes" -a "x$ac_cv_lib_soname_X11" != "x" -a "x$ac_cv_lib_soname_Xext" != "x"
        then
            WINE_CHECK_SONAME(Xrender,XRenderQueryExtension,
              [AC_CHECK_LIB(Xrender,XRenderSetPictureTransform,
                [AC_DEFINE(HAVE_XRENDERSETPICTURETRANSFORM, 1,
                 [Define if Xrender has the XRenderSetPictureTransform function])],,
                [$X_LIBS -lXext -lX11 $X_EXTRA_LIBS])],,[$X_LIBS -lXext -lX11 $X_EXTRA_LIBS])

        fi
        WINE_NOTICE_IF([test "x$ac_cv_lib_soname_Xrender" = "x"],[XRender development files not found.
Wine will be built without XRender support. (winex11.drv)])

        dnl *** Check for X RandR extension
        if test "$ac_cv_header_X11_extensions_Xrandr_h" = "yes" -a "x$ac_cv_lib_soname_Xrender" != "x"
        then
                AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>]], [[static typeof(XRRSetScreenConfigAndRate) * func;]])],
                  [WINE_CHECK_SONAME(Xrandr,XRRQueryExtension,,,[$X_LIBS -lXext -lX11 $X_EXTRA_LIBS])])
        fi
        WINE_NOTICE_IF([test "x$ac_cv_lib_soname_Xrandr" = "x"],[XRandr development files not found.
Wine will be built without XRandr support. (winex11.drv)])

        dnl *** Check for Xinerama extension
        if test "$ac_cv_header_X11_extensions_Xinerama_h" = "yes"
        then
                AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>]], [[static typeof(XineramaQueryScreens) * func;]])],
                  [WINE_CHECK_SONAME(Xinerama,XineramaQueryScreens,,,[$X_LIBS -lXext -lX11 $X_EXTRA_LIBS])])
        fi
        WINE_NOTICE_IF([test "x$ac_cv_lib_soname_Xinerama" = "x"],[Xinerama development files not found.
Wine will be built without Xinerama support. (winex11.drv)])

    dnl *** End of X11/Xlib.h check

    dnl Check for the presence of OpenGL
    if test "x$with_opengl" != "xno"
    then
	AC_CHECK_HEADERS(GL/gl.h GL/glx.h GL/glext.h GL/glu.h,,,
[#ifdef HAVE_GL_GLX_H
# include <GL/glx.h>
#endif])
	if test "$ac_cv_header_GL_gl_h" = "yes" -a "$ac_cv_header_GL_glx_h" = "yes"
	then
	    dnl Check for some problems due to old Mesa versions
	    AC_CACHE_CHECK([for up-to-date OpenGL version], wine_cv_opengl_header_version_OK,
	      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <GL/gl.h>]], [[GLenum test = GL_UNSIGNED_SHORT_5_6_5;]])],
		[wine_cv_opengl_header_version_OK="yes"],
		[wine_cv_opengl_header_version_OK="no"]))

	    if test "$wine_cv_opengl_header_version_OK" = "yes"
            then
		dnl Check for the presence of the library
		WINE_CHECK_SONAME(GL,glXCreateContext,
			     [OPENGL_LIBS="-lGL"
                             OPENGLFILES='$(OPENGLFILES)'
                             AC_DEFINE(HAVE_OPENGL, 1, [Define if OpenGL is present on the system])],
                             [if test -f /usr/X11R6/lib/libGL.a
                              then
                                  WINE_WARNING([/usr/X11R6/lib/libGL.a is present on your system.
This probably prevents linking to OpenGL. Try deleting the file and restarting configure.])
                              else
                                  WINE_WARNING([No OpenGL library found on this system.
Wine will be build without OpenGL or Direct3D support.])
                              fi],
			     $X_LIBS -lXext -lX11 -lm $X_EXTRA_LIBS)

	        dnl Check for GLU32 library.
		AC_CHECK_LIB(GLU,gluLookAt,
			     [OPENGL_LIBS="$OPENGL_LIBS -lGLU"
			     GLU32FILES='$(GLU32FILES)']
			     ,,
			     $OPENGL_LIBS $X_LIBS $X_PRE_LIBS -lXext -lX11 -lm $X_EXTRA_LIBS)
             else
                 WINE_WARNING([Old Mesa headers detected. Consider upgrading your Mesa libraries (http://www.mesa3d.org/).
Wine will be build without OpenGL or Direct3D support.])
	     fi
         else
             WINE_WARNING([OpenGL development headers not found.
Wine will be build without OpenGL or Direct3D support.])
	 fi
    fi

    dnl **** Check for NAS ****
    AC_SUBST(NASLIBS,"")
    AC_CHECK_HEADERS(audio/audiolib.h,
         [AC_CHECK_HEADERS(audio/soundlib.h,,,[#include <audio/audiolib.h>])
          AC_CHECK_LIB(audio,AuCreateFlow,
                       [AC_DEFINE(HAVE_NAS,1,[Define if you have NAS including devel headers])
                        AC_CHECK_LIB(Xau,XauGetBestAuthByAddr,
                          [NASLIBS="-lXau -laudio -lXt $X_LIBS -lXext -lX11 $X_EXTRA_LIBS"],
                          [NASLIBS="-laudio -lXt $X_LIBS -lXext -lX11 $X_EXTRA_LIBS"],
                          [$X_LIBS])
                       ],,
                       [-lXt $X_LIBS -lXext -lX11 $X_EXTRA_LIBS])])

    CPPFLAGS="$ac_save_CPPFLAGS"
    XFILES='$(XFILES)'
else
    XLIB=""
    X_CFLAGS=""
    X_LIBS=""
    WINE_WARNING([X development files not found. Wine will be built
without X support, which probably isn't what you want. You will need to install
development packages of Xlib/Xfree86 at the very least.])
fi

dnl **** Check for libxml2 ****

AC_SUBST(XML2LIBS,"")
AC_SUBST(XML2INCL,"")
AC_SUBST(XSLTLIBS,"")
AC_SUBST(XSLTINCL,"")
ac_save_CPPFLAGS="$CPPFLAGS"
if test "$PKG_CONFIG" != "false"
then
    ac_xml_libs="`$PKG_CONFIG --libs libxml-2.0 2>/dev/null`"
    ac_xml_cflags="`$PKG_CONFIG --cflags libxml-2.0 2>/dev/null`"
else
    ac_xml_libs="`xml2-config --libs 2>/dev/null`"
    ac_xml_cflags="`xml2-config --cflags  2>/dev/null`"
fi
CPPFLAGS="$CPPFLAGS $ac_xml_cflags"
AC_CHECK_HEADERS(libxml/parser.h,
        [AC_CHECK_LIB(xml2, xmlParseMemory,
            [AC_DEFINE(HAVE_LIBXML2, 1, [Define if you have the libxml2 library])
             XML2LIBS="$ac_xml_libs"
             XML2INCL="$ac_xml_cflags"],,$ac_xml_libs)
         AC_CHECK_LIB(xml2, xmlReadMemory,
            [AC_DEFINE(HAVE_XMLREADMEMORY,1,[Define if libxml2 has the xmlReadMemory function])],,$ac_xml_libs)
         AC_CHECK_LIB(xml2, xmlNewDocPI,
            [AC_DEFINE(HAVE_XMLNEWDOCPI,1,[Define if libxml2 has the xmlNewDocPI function])],,$ac_xml_libs)
    ])
CPPFLAGS="$ac_save_CPPFLAGS"
WINE_NOTICE_IF([test "$ac_cv_lib_xml2_xmlParseMemory" != "yes"],[libxml2 development files not found.
Wine will be built without XML support. (msxml.dll)])

if test "$PKG_CONFIG" != "false"
then
    ac_xslt_libs="`$PKG_CONFIG --libs libxslt 2>/dev/null`"
    ac_xslt_cflags="`$PKG_CONFIG --cflags libxslt 2>/dev/null`"
else
    ac_xslt_libs="`xslt-config --libs 2>/dev/null`"
    ac_xslt_cflags="`xslt-config --cflags 2>/dev/null`"
fi
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
WINE_NOTICE_IF([test "$ac_cv_lib_xslt_xsltCompilePattern" != "yes"],[libxslt development files not found.
Wine will be built without xslt support. (msxml.dll)])

dnl **** Check for libhal ****
AC_SUBST(HALINCL,"")
if test "$PKG_CONFIG" != "false"
then
    ac_save_CPPFLAGS="$CPPFLAGS"
    ac_hal_libs="`$PKG_CONFIG --libs hal 2>/dev/null`"
    ac_hal_cflags="`$PKG_CONFIG --cflags hal 2>/dev/null`"
    CPPFLAGS="$CPPFLAGS $ac_hal_cflags"
    AC_CHECK_HEADERS([dbus/dbus.h hal/libhal.h])
    if test "$ac_cv_header_dbus_dbus_h" = "yes" -a "$ac_cv_header_hal_libhal_h" = "yes"
    then
        AC_CHECK_LIB(dbus-1, dbus_connection_close,
          [WINE_CHECK_SONAME(hal, libhal_ctx_new,
            [HALINCL="$ac_hal_cflags"],,[$ac_hal_libs])],,[$ac_hal_libs])
    fi
    CPPFLAGS="$ac_save_CPPFLAGS"
fi
WINE_NOTICE_IF([test "x$ac_cv_lib_soname_hal" = "x"],[libhal development files not found.
Wine will be built without dynamic device support. (explorer.exe)])

dnl **** Check which curses lib to use ***
CURSESLIBS=""
if test "$ac_cv_header_ncurses_h" = "yes"
then
    WINE_CHECK_SONAME(ncurses,waddch,[CURSESLIBS="-lncurses"])
elif test "$ac_cv_header_curses_h" = "yes"
then
    WINE_CHECK_SONAME(curses,waddch,[CURSESLIBS="-lcurses"])
fi
ac_save_LIBS="$LIBS"
LIBS="$LIBS $CURSESLIBS"
AC_CHECK_FUNCS(mousemask)
LIBS="$ac_save_LIBS"
WINE_NOTICE_IF([test "x$ac_cv_lib_soname_curses$ac_cv_lib_soname_ncurses" = "x"],[lib(n)curses development files not found.
Wine will be built without (n)curses support. (wineconsole.exe)])

dnl **** Check for SANE ****
AC_CHECK_PROG(sane_devel,sane-config,sane-config,no)
if test "$sane_devel" != "no"
then
    ac_sane_incl="`$sane_devel --cflags`"
    ac_save_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $ac_sane_incl"
    AC_CHECK_HEADER(sane/sane.h,
        [WINE_CHECK_SONAME(sane,sane_init,[AC_SUBST(SANEINCL,"$ac_sane_incl")])])
    CPPFLAGS="$ac_save_CPPFLAGS"
fi
WINE_NOTICE_IF([test "x$ac_cv_lib_soname_sane" = "x"],[libsane development files not found.
Wine will be built without scanner support. (sane.ds/twain_32.dll)])

dnl **** Check for libgphoto2 ****
AC_CHECK_PROG(gphoto2_devel,gphoto2-config,gphoto2-config,no)
AC_CHECK_PROG(gphoto2port_devel,gphoto2-port-config,gphoto2-port-config,no)
if test "$gphoto2_devel" != "no" -a "$gphoto2port_devel" != "no"
then
    ac_gphoto2_incl="`$gphoto2_devel --cflags` `$gphoto2port_devel --cflags`"
    ac_gphoto2_libs=""
    for i in `$gphoto2_devel --libs` `$gphoto2port_devel --libs`
    do
      case "$i" in
        -L/usr/lib|-L/usr/lib64) ;;
        -L*|-l*) ac_gphoto2_libs="$ac_gphoto2_libs $i";;
      esac
    done
    ac_save_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $ac_gphoto2_incl"
    AC_CHECK_HEADER(gphoto2-camera.h,
                    [AC_CHECK_LIB(gphoto2,gp_camera_new,
                                  [AC_DEFINE(HAVE_GPHOTO2, 1, [Define if we have libgphoto2 development environment])
                                   AC_SUBST(GPHOTO2LIBS,"$ac_gphoto2_libs")
                                   AC_SUBST(GPHOTO2INCL,"$ac_gphoto2_incl")],,
                                  [$GPHOTO2LIBS])])
    CPPFLAGS="$ac_save_CPPFLAGS"
fi
WINE_NOTICE_IF([test "$ac_cv_lib_gphoto2_gp_camera_new" != "yes"],[libgphoto2 development files not found.
Wine will be built without Digital Camera support. (gphoto2.ds/twain_32.dll)])


dnl **** Check for the ICU library ****
if test "$ac_cv_header_unicode_ubidi_h" = "yes"
then
    saved_libs="$LIBS"
    icu_libs="-lsicuuc -lsicudata -lstdc++ -lgcc_s"
    if test -n "$ICU_LIB_DIR"
    then
        icu_libs="-L$ICU_LIB_DIR $icu_libs"
    fi
    AC_MSG_CHECKING(whether can link with ICU libraries)
    LIBS="$saved_libs $icu_libs"
    AC_RUN_IFELSE([AC_LANG_PROGRAM([[#include <unicode/ubidi.h>]],[[ubidi_open()]])],
                [AC_DEFINE(HAVE_ICU,1,[Define to 1 if the ICU libraries are installed])
                 AC_SUBST(ICULIBS,"$icu_libs")
                 AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no)],[AC_MSG_RESULT(no)])
    LIBS="$saved_libs"
fi
WINE_NOTICE_IF([test "x$ICULIBS" = "x"],[libicu development files not found.
Wine will be built without bidi (Right to Left) support. (gdi32.dll)])

dnl **** Check for resolver library ***
AC_SUBST(RESOLVLIBS,"")
if test "$ac_cv_header_resolv_h" = "yes"
then
    AC_CHECK_LIB(resolv, res_query,
        [AC_DEFINE(HAVE_RESOLV, 1, [Define if you have the resolver library and header])
         RESOLVLIBS="-lresolv"])
fi

dnl **** Check for LittleCMS ***
AC_SUBST(LCMSLIBS,"")
if test "$ac_cv_header_lcms_h" = "yes" -o "$ac_cv_header_lcms_lcms_h" = "yes"
then
    AC_CHECK_LIB(lcms, cmsOpenProfileFromFile,
        [AC_DEFINE(HAVE_LCMS, 1, [Define if you have the LittleCMS development environment])
         LCMSLIBS="-llcms"])
fi
WINE_NOTICE_IF([test "$ac_cv_lib_lcms_cmsOpenProfileFromFile" = "yes"],[liblcms development files not found.
Wine will be built without Color Management support. (mscms.dll)])

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
WINE_NOTICE_IF([test "x$LDAPLIBS" = "x"],[libldap (OpenLDAP) development files not found.
Wine will be built without LDAP support. (wldap32.dll)])

dnl **** Check for FreeType 2 ****
AC_CHECK_PROGS(ft_devel,[freetype-config freetype2-config],false)
if test "$ft_devel" != "false"
then
    ac_freetype_libs=`$ft_devel --libs`
    ac_freetype_incl=`$ft_devel --cflags`
    WINE_CHECK_SONAME(freetype,FT_Init_FreeType,[ft_lib=yes],[ft_lib=no],[$ac_freetype_libs])
    if test "$ft_lib" = "yes"
    then
	ac_save_CPPFLAGS="$CPPFLAGS"
	CPPFLAGS="$ac_freetype_incl $CPPFLAGS"
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
	AC_PREPROC_IFELSE([AC_LANG_SOURCE([[#include <ft2build.h>
                    #include <freetype/fttrigon.h>]])],[AC_DEFINE(HAVE_FREETYPE_FTTRIGON_H, 1,
          [Define if you have the <freetype/fttrigon.h> header file.])
                    wine_cv_fttrigon=yes],[wine_cv_fttrigon=no])
        AC_CHECK_TYPES(FT_TrueTypeEngineType,,,[#include <freetype/ftmodapi.h>])
        ac_save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $ac_freetype_libs"
        AC_CHECK_FUNCS(FT_Load_Sfnt_Table)
        CFLAGS="$ac_save_CFLAGS"
	CPPFLAGS="$ac_save_CPPFLAGS"
	dnl Check that we have at least freetype/freetype.h
	if test "$ac_cv_header_freetype_freetype_h" = "yes" -a "$wine_cv_fttrigon" = "yes"
	then
	    AC_DEFINE(HAVE_FREETYPE, 1, [Define if FreeType 2 is installed])
            AC_SUBST(FREETYPELIBS,"$ac_freetype_libs")
            AC_SUBST(FREETYPEINCL,"$ac_freetype_incl")
	fi
    fi
fi

dnl Only build the fonts dir if we have both freetype and fontforge
if test "$FONTFORGE" = "false"
then
    WINE_WARNING([FontForge is missing.
Fonts will not be built. Dialog text may be invisible or unaligned.])
elif test "x$FREETYPELIBS" = "x"
then
    WINE_WARNING([FreeType development files not found.
Fonts will not be built. Dialog text may be invisible or unaligned.])
else
  AC_SUBST(FONTSSUBDIRS,"fonts")
fi

dnl **** Check for parport (currently Linux only) ****
AC_CACHE_CHECK([for parport header/ppdev.h], ac_cv_c_ppdev,
 AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <linux/ppdev.h>]], [[ioctl (1,PPCLAIM,0)]])],
    [ac_cv_c_ppdev="yes"],[ac_cv_c_ppdev="no"]))
if test "$ac_cv_c_ppdev" = "yes"
then
    AC_DEFINE(HAVE_PPDEV, 1, [Define if we can use ppdev.h for parallel port access])
fi

dnl **** Check for va_copy ****
AC_CACHE_CHECK([for va_copy], ac_cv_c_va_copy,
 AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdarg.h>]], [[va_list ap1, ap2; va_copy(ap1,ap2);]])],
    [ac_cv_c_va_copy="yes"],[ac_cv_c_va_copy="no"])
 )
if test "$ac_cv_c_va_copy" = "yes"
then
    AC_DEFINE(HAVE_VA_COPY, 1, [Define if we have va_copy])
fi
AC_CACHE_CHECK([for __va_copy], ac_cv_c___va_copy,
 AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdarg.h>]], [[va_list ap1, ap2; __va_copy(ap1,ap2);]])],
    [ac_cv_c___va_copy="yes"],[ac_cv_c___va_copy="no"])
 )
if test "$ac_cv_c___va_copy" = "yes"
then
    AC_DEFINE(HAVE___VA_COPY, 1, [Define if we have __va_copy])
fi

dnl **** Check for sigsetjmp ****
AC_CACHE_CHECK([for sigsetjmp], ac_cv_c_sigsetjmp,
 AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <setjmp.h>]], [[sigjmp_buf buf;
    sigsetjmp( buf, 1 );
    siglongjmp( buf, 1 );]])],[ac_cv_c_sigsetjmp="yes"],[ac_cv_c_sigsetjmp="no"])
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
        pthread_attr_get_np \
        pthread_getattr_np \
        pthread_get_stackaddr_np \
        pthread_get_stacksize_np,
        [$LIBPTHREAD])

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
        [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#ifdef HAVE_ALSA_ASOUNDLIB_H
#include <alsa/asoundlib.h>
#elif defined(HAVE_SYS_ASOUNDLIB_H)
#include <sys/asoundlib.h>
#endif]], [[int ret = snd_pcm_hw_params_get_access(NULL, NULL)]])],
                        [AC_DEFINE(HAVE_ALSA,1,[Define if you have ALSA 1.x including devel headers])
                         ALSALIBS="-lasound"],[])])
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
    WINE_CHECK_SONAME(capi20,capi20_register)
fi
WINE_NOTICE_IF([test "x$ac_cv_lib_soname_capi20" = "x"],[libcapi20 development files not found.
Wine will be built without ISDN support. (capi2032.dll)])

dnl **** Check for cups ****
if test "$ac_cv_header_cups_cups_h" = "yes"
then
    WINE_CHECK_SONAME(cups,cupsGetDefault)
fi
WINE_NOTICE_IF([test "x$ac_cv_lib_soname_cups" = "x"],[libcups development files not found.
Wine will be built without CUPS support.])

dnl **** Check for jack ****
if test "$ac_cv_header_jack_jack_h" = "yes"
then
    WINE_CHECK_SONAME(jack,jack_client_new)
fi

dnl **** Check for fontconfig ****
if test "$ac_cv_header_fontconfig_fontconfig_h" = "yes"
then
    WINE_CHECK_SONAME(fontconfig,FcInit)
fi

dnl **** Check for SSL ****
if test "$ac_cv_header_openssl_err_h" = "yes" -a "$ac_cv_header_openssl_ssl_h" = "yes"
then
    WINE_CHECK_SONAME(ssl,SSL_library_init)
    WINE_CHECK_SONAME(crypto,BIO_new_socket)
fi

dnl **** Check for libjpeg ****
if test "$ac_cv_header_jpeglib_h" = "yes"
then
    WINE_CHECK_SONAME(jpeg,jpeg_start_decompress)
fi

dnl **** Check for libpng ****
if test "$ac_cv_header_png_h" = "yes"
then
    WINE_CHECK_SONAME(png,png_create_read_struct,,,,[[libpng[[0-9]]*]])
fi

dnl **** Check for any sound system ****
WINE_WARNING_IF([test "x$ALSALIBS$AUDIOIOLIBS$COREAUDIO$NASLIBS$ESD_LIBS$ac_cv_lib_soname_jack" = "x" -a \
                 "$ac_cv_header_sys_soundcard_h" != "yes" -a \
                 "$ac_cv_header_machine_soundcard_h" != "yes" -a \
                 "$ac_cv_header_soundcard_h" != "yes"],
[No sound system was found. Windows applications will be silent.])

dnl **** Check for gcc specific options ****

AC_SUBST(EXTRACFLAGS,"")
if test "x${GCC}" = "xyes"
then
  EXTRACFLAGS="-Wall -pipe"

  dnl Check for strength-reduce bug
  AC_CACHE_CHECK( [for gcc strength-reduce bug], ac_cv_c_gcc_strength_bug,
                  AC_RUN_IFELSE([AC_LANG_PROGRAM([[int L[[4]] = {0,1,2,3};]],
[[static int Array[[3]];
  unsigned int B = 3;
  int i;
  for(i=0; i<B; i++) Array[[i]] = i - 3;
  for(i=0; i<4 - 1; i++) L[[i]] = L[[i + 1]];
  L[[i]] = 4;
  exit( Array[[1]] != -2 || L[[2]] != 3)]])],
    [ac_cv_c_gcc_strength_bug="no"],[ac_cv_c_gcc_strength_bug="yes"],[ac_cv_c_gcc_strength_bug="yes"]) )
  if test "$ac_cv_c_gcc_strength_bug" = "yes"
  then
    EXTRACFLAGS="$EXTRACFLAGS -fno-strength-reduce"
  fi

  dnl Check for some compiler flags

  WINE_TRY_CFLAGS([-fshort-wchar],
                  [AC_DEFINE(CC_FLAG_SHORT_WCHAR, "-fshort-wchar",
                             [Specifies the compiler flag that forces a short wchar_t])])
  WINE_TRY_CFLAGS([-fno-strict-aliasing])
  WINE_TRY_CFLAGS([-Wdeclaration-after-statement])
  WINE_TRY_CFLAGS([-Wwrite-strings])

  dnl Check for noisy string.h
  saved_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS -Wpointer-arith -Werror"
  AC_CACHE_CHECK([for broken string.h that generates warnings], ac_cv_c_string_h_warnings,
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <string.h>]], [[]])],[ac_cv_c_string_h_warnings=no],[ac_cv_c_string_h_warnings=yes]))
  CFLAGS="$saved_CFLAGS"
  if test "$ac_cv_c_string_h_warnings" = "no"
  then
    EXTRACFLAGS="$EXTRACFLAGS -Wpointer-arith"
  fi

  AC_SUBST(BUILTINFLAG,"")
  saved_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS -Werror"
  AC_CACHE_CHECK([for builtin wchar inlines], ac_cv_c_builtin_wchar_ctype,
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[int iswlower(unsigned short);]])],[ac_cv_c_builtin_wchar_ctype=no],[ac_cv_c_builtin_wchar_ctype=yes]))
  CFLAGS="$saved_CFLAGS"
  if test "$ac_cv_c_builtin_wchar_ctype" = "yes"
  then
    BUILTINFLAG="-fno-builtin"
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

dnl **** Platform-specific checks ****

AC_SUBST(LDPATH,"")
case $build_os in
  cygwin*|mingw32*)
    LDPATH="PATH=\"\$(TOOLSDIR)/libs/wine:\$\$PATH\""
    ;;
  darwin*|macosx*)
    ;;
  linux*|solaris*)  dnl FIXME: What other platforms support $ORIGIN?
    if test -z "$LDRPATH_LOCAL"
    then
      LDPATH="LD_LIBRARY_PATH=\"\$(TOOLSDIR)/libs/wine:\$\$LD_LIBRARY_PATH\""
    fi
    ;;
  *)
    LDPATH="LD_LIBRARY_PATH=\"\$(TOOLSDIR)/libs/wine:\$\$LD_LIBRARY_PATH\""
    ;;
esac

dnl Mingw needs explicit msvcrt for linking libwine and winsock for wininet
case $host_os in
  mingw32*)
    AC_SUBST(CRTLIBS,"-lmsvcrt")
    AC_SUBST(SOCKETLIBS,"-lws2_32")
    ;;
esac

AC_SUBST(MAIN_BINARY,"wine-pthread")  dnl Default value
case $host_cpu in
  *i[[3456789]]86*)
    case $host_os in
      linux* | k*bsd*-gnu)
        AC_SUBST(MAIN_BINARY,"wine-glibc")
        AC_SUBST(EXTRA_BINARIES,"wine-kthread wine-pthread wine-preloader") ;;
    esac
    ;;
esac

dnl **** Check for functions ****

AC_CHECK_FUNCS(\
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
	dlopen \
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
	getdirentries \
	getopt_long \
	getpagesize \
	getpwuid \
	gettid \
	gettimeofday \
	getuid \
	inet_pton \
	kqueue \
	lstat \
	memmove \
	mmap \
	pclose \
	poll \
	popen \
	prctl \
	pread \
	pwrite \
	readlink \
	rfork \
	sched_yield \
	select \
	setrlimit \
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
	strtold \
	tcgetattr \
	timegm \
	usleep \
	vsnprintf \
	wait4 \
	waitpid \
)

dnl Check for -ldl
if test "$ac_cv_func_dlopen" = no
then
    AC_CHECK_LIB(dl,dlopen,[AC_DEFINE(HAVE_DLOPEN,1) AC_SUBST(LIBDL,"-ldl")])
fi
WINE_CHECK_LIB_FUNCS(dladdr,[$LIBDL])

dnl Check for -lpoll for Mac OS X/Darwin
if test "$ac_cv_func_poll" = no
then
    AC_CHECK_LIB(poll,poll,[AC_DEFINE(HAVE_POLL,1) AC_SUBST(LIBPOLL,"-lpoll")])
fi

dnl Check for -lnsl for Solaris
AC_SEARCH_LIBS(gethostbyname, nsl)

dnl Check for -lsocket for Solaris
AC_SEARCH_LIBS(connect, socket)

dnl Check for -lresolv for Solaris
AC_SEARCH_LIBS(inet_aton, resolv)

dnl **** Check for functions which may rely on -lsocket on Solaris.
AC_CHECK_FUNCS(\
	getaddrinfo \
	getnameinfo \
	getnetbyname \
	getprotobyname \
	getprotobynumber \
	getservbyport \
	inet_network \
	sendmsg \
)

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
               AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <signal.h>]], [[sigset_t set; sigaddset(&set,SIGTERM);]])],[wine_cv_have_sigaddset=yes],[wine_cv_have_sigaddset=no]))
if test "$wine_cv_have_sigaddset" = "yes"
then
  AC_DEFINE(HAVE_SIGADDSET, 1, [Define if sigaddset is supported])
fi


AC_CACHE_CHECK([whether we can use re-entrant gethostbyname_r Linux style],
   	wine_cv_linux_gethostbyname_r_6,
	AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <netdb.h>]],[[
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
    ]])],[wine_cv_linux_gethostbyname_r_6=yes],[wine_cv_linux_gethostbyname_r_6=no
	])
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
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
	#include <sys/ioctl.h>
	#include <sys/types.h>
	#include <linux/joystick.h>

	struct js_event blub;
	#if !defined(JS_EVENT_AXIS) || !defined(JS_EVENT_BUTTON)
	#error "no 2.2 header"
	#endif
	]], [[/*empty*/]])],[wine_cv_linux_joystick_22_api=yes],[wine_cv_linux_joystick_22_api=no])
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

dnl Check for sin6_scope_id
AC_CHECK_MEMBERS([struct sockaddr_in6.sin6_scope_id],,,
[#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif])

dnl Check for ns_msg ptr member
AC_CHECK_MEMBERS([ns_msg._msg_ptr],,,
[#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_NAMESER_H
# include <arpa/nameser.h>
#endif])

dnl Check for the external timezone variables timezone and daylight
AC_CACHE_CHECK([for timezone variable], ac_cv_have_timezone,
               AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <time.h>]], [[timezone = 1]])],[ac_cv_have_timezone="yes"],[ac_cv_have_timezone="no"]))
if test "$ac_cv_have_timezone" = "yes"
then
    AC_DEFINE(HAVE_TIMEZONE, 1, [Define if you have the timezone variable])
fi
AC_CACHE_CHECK([for daylight variable], ac_cv_have_daylight,
               AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <time.h>]], [[daylight = 1]])],[ac_cv_have_daylight="yes"],[ac_cv_have_daylight="no"]))
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

dnl autoconf versions before 2.59d need backslashes to escape newlines in subst variables
AC_SUBST(DEPENDENCIES,m4_if(m4_version_compare(m4_defn([m4_PACKAGE_VERSION]), [2.59d]), -1,
["### Dependencies:\\
\\
.INIT: Makefile\\
.BEGIN: Makefile\\
Makefile: dummy\\
	-\$(MAKEDEP) -C\$(SRCDIR) -S\$(TOPSRCDIR) -T\$(TOPOBJDIR) \$(EXTRAINCL) \$(DEPEND_SRCS)\\
\\
\$(OBJS): \$(IDL_GEN_HEADERS)\\
\$(IDL_GEN_C_SRCS:.c=.o): \$(IDL_GEN_C_SRCS)\\
\$(LEX_SRCS:.l=.yy.o): \$(LEX_SRCS:.l=.yy.c)\\
\$(BISON_SRCS:.y=.tab.o): \$(BISON_SRCS:.y=.tab.c)"],
["### Dependencies:

.INIT: Makefile
.BEGIN: Makefile
Makefile: dummy
	-\$(MAKEDEP) -C\$(SRCDIR) -S\$(TOPSRCDIR) -T\$(TOPOBJDIR) \$(EXTRAINCL) \$(DEPEND_SRCS)

\$(OBJS): \$(IDL_GEN_HEADERS)
\$(IDL_GEN_C_SRCS:.c=.o): \$(IDL_GEN_C_SRCS)
\$(LEX_SRCS:.l=.yy.o): \$(LEX_SRCS:.l=.yy.c)
\$(BISON_SRCS:.y=.tab.o): \$(BISON_SRCS:.y=.tab.c)"]))

AH_TOP([#define __WINE_CONFIG_H])

AC_CONFIG_COMMANDS([include/stamp-h], [echo timestamp > include/stamp-h])

WINE_CONFIG_EXTRA_DIR(dlls/gdi32/enhmfdrv)
WINE_CONFIG_EXTRA_DIR(dlls/gdi32/mfdrv)
WINE_CONFIG_EXTRA_DIR(dlls/kernel32/nls)
WINE_CONFIG_EXTRA_DIR(dlls/user32/resources)
WINE_CONFIG_EXTRA_DIR(dlls/wineps.drv/data)
WINE_CONFIG_EXTRA_DIR(include/wine)

MAKE_RULES=Make.rules
AC_SUBST_FILE(MAKE_RULES)

MAKE_DLL_RULES=dlls/Makedll.rules
AC_SUBST_FILE(MAKE_DLL_RULES)

MAKE_IMPLIB_RULES=dlls/Makeimplib.rules
AC_SUBST_FILE(MAKE_IMPLIB_RULES)

MAKE_TEST_RULES=dlls/Maketest.rules
AC_SUBST_FILE(MAKE_TEST_RULES)

MAKE_PROG_RULES=programs/Makeprog.rules
AC_SUBST_FILE(MAKE_PROG_RULES)

AC_CONFIG_FILES([Make.rules])
AC_CONFIG_FILES([dlls/Makedll.rules])
AC_CONFIG_FILES([dlls/Makeimplib.rules])
AC_CONFIG_FILES([dlls/Maketest.rules])
AC_CONFIG_FILES([programs/Makeprog.rules])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([dlls/Makefile])
AC_CONFIG_FILES([dlls/acledit/Makefile])
AC_CONFIG_FILES([dlls/activeds/Makefile])
AC_CONFIG_FILES([dlls/advapi32/Makefile])
AC_CONFIG_FILES([dlls/advapi32/tests/Makefile])
AC_CONFIG_FILES([dlls/advpack/Makefile])
AC_CONFIG_FILES([dlls/advpack/tests/Makefile])
AC_CONFIG_FILES([dlls/amstream/Makefile])
AC_CONFIG_FILES([dlls/atl/Makefile])
AC_CONFIG_FILES([dlls/avicap32/Makefile])
AC_CONFIG_FILES([dlls/avifil32/Makefile])
AC_CONFIG_FILES([dlls/browseui/Makefile])
AC_CONFIG_FILES([dlls/browseui/tests/Makefile])
AC_CONFIG_FILES([dlls/cabinet/Makefile])
AC_CONFIG_FILES([dlls/cabinet/tests/Makefile])
AC_CONFIG_FILES([dlls/capi2032/Makefile])
AC_CONFIG_FILES([dlls/cards/Makefile])
AC_CONFIG_FILES([dlls/cfgmgr32/Makefile])
AC_CONFIG_FILES([dlls/clusapi/Makefile])
AC_CONFIG_FILES([dlls/comcat/Makefile])
AC_CONFIG_FILES([dlls/comcat/tests/Makefile])
AC_CONFIG_FILES([dlls/comctl32/Makefile])
AC_CONFIG_FILES([dlls/comctl32/tests/Makefile])
AC_CONFIG_FILES([dlls/comdlg32/Makefile])
AC_CONFIG_FILES([dlls/comdlg32/tests/Makefile])
AC_CONFIG_FILES([dlls/compstui/Makefile])
AC_CONFIG_FILES([dlls/credui/Makefile])
AC_CONFIG_FILES([dlls/crtdll/Makefile])
AC_CONFIG_FILES([dlls/crypt32/Makefile])
AC_CONFIG_FILES([dlls/crypt32/tests/Makefile])
AC_CONFIG_FILES([dlls/cryptdll/Makefile])
AC_CONFIG_FILES([dlls/cryptnet/Makefile])
AC_CONFIG_FILES([dlls/ctl3d32/Makefile])
AC_CONFIG_FILES([dlls/d3d8/Makefile])
AC_CONFIG_FILES([dlls/d3d8/tests/Makefile])
AC_CONFIG_FILES([dlls/d3d9/Makefile])
AC_CONFIG_FILES([dlls/d3d9/tests/Makefile])
AC_CONFIG_FILES([dlls/d3dim/Makefile])
AC_CONFIG_FILES([dlls/d3drm/Makefile])
AC_CONFIG_FILES([dlls/d3drm/tests/Makefile])
AC_CONFIG_FILES([dlls/d3dx8/Makefile])
AC_CONFIG_FILES([dlls/d3dxof/Makefile])
AC_CONFIG_FILES([dlls/dbghelp/Makefile])
AC_CONFIG_FILES([dlls/dciman32/Makefile])
AC_CONFIG_FILES([dlls/ddraw/Makefile])
AC_CONFIG_FILES([dlls/ddraw/tests/Makefile])
AC_CONFIG_FILES([dlls/ddrawex/Makefile])
AC_CONFIG_FILES([dlls/devenum/Makefile])
AC_CONFIG_FILES([dlls/dinput/Makefile])
AC_CONFIG_FILES([dlls/dinput/tests/Makefile])
AC_CONFIG_FILES([dlls/dinput8/Makefile])
AC_CONFIG_FILES([dlls/dmband/Makefile])
AC_CONFIG_FILES([dlls/dmcompos/Makefile])
AC_CONFIG_FILES([dlls/dmime/Makefile])
AC_CONFIG_FILES([dlls/dmloader/Makefile])
AC_CONFIG_FILES([dlls/dmscript/Makefile])
AC_CONFIG_FILES([dlls/dmstyle/Makefile])
AC_CONFIG_FILES([dlls/dmsynth/Makefile])
AC_CONFIG_FILES([dlls/dmusic/Makefile])
AC_CONFIG_FILES([dlls/dmusic32/Makefile])
AC_CONFIG_FILES([dlls/dnsapi/Makefile])
AC_CONFIG_FILES([dlls/dnsapi/tests/Makefile])
AC_CONFIG_FILES([dlls/dplay/Makefile])
AC_CONFIG_FILES([dlls/dplayx/Makefile])
AC_CONFIG_FILES([dlls/dplayx/tests/Makefile])
AC_CONFIG_FILES([dlls/dpnaddr/Makefile])
AC_CONFIG_FILES([dlls/dpnet/Makefile])
AC_CONFIG_FILES([dlls/dpnhpast/Makefile])
AC_CONFIG_FILES([dlls/dsound/Makefile])
AC_CONFIG_FILES([dlls/dsound/tests/Makefile])
AC_CONFIG_FILES([dlls/dswave/Makefile])
AC_CONFIG_FILES([dlls/dwmapi/Makefile])
AC_CONFIG_FILES([dlls/dxdiagn/Makefile])
AC_CONFIG_FILES([dlls/dxerr8/Makefile])
AC_CONFIG_FILES([dlls/dxerr9/Makefile])
AC_CONFIG_FILES([dlls/dxguid/Makefile])
AC_CONFIG_FILES([dlls/gdi32/Makefile])
AC_CONFIG_FILES([dlls/gdi32/tests/Makefile])
AC_CONFIG_FILES([dlls/gdiplus/Makefile])
AC_CONFIG_FILES([dlls/gdiplus/tests/Makefile])
AC_CONFIG_FILES([dlls/glu32/Makefile])
AC_CONFIG_FILES([dlls/gphoto2.ds/Makefile])
AC_CONFIG_FILES([dlls/hal/Makefile])
AC_CONFIG_FILES([dlls/hhctrl.ocx/Makefile])
AC_CONFIG_FILES([dlls/hid/Makefile])
AC_CONFIG_FILES([dlls/hlink/Makefile])
AC_CONFIG_FILES([dlls/hlink/tests/Makefile])
AC_CONFIG_FILES([dlls/hnetcfg/Makefile])
AC_CONFIG_FILES([dlls/iccvid/Makefile])
AC_CONFIG_FILES([dlls/icmp/Makefile])
AC_CONFIG_FILES([dlls/ifsmgr.vxd/Makefile])
AC_CONFIG_FILES([dlls/imaadp32.acm/Makefile])
AC_CONFIG_FILES([dlls/imagehlp/Makefile])
AC_CONFIG_FILES([dlls/imm32/Makefile])
AC_CONFIG_FILES([dlls/infosoft/Makefile])
AC_CONFIG_FILES([dlls/infosoft/tests/Makefile])
AC_CONFIG_FILES([dlls/inseng/Makefile])
AC_CONFIG_FILES([dlls/iphlpapi/Makefile])
AC_CONFIG_FILES([dlls/iphlpapi/tests/Makefile])
AC_CONFIG_FILES([dlls/itss/Makefile])
AC_CONFIG_FILES([dlls/itss/tests/Makefile])
AC_CONFIG_FILES([dlls/kernel32/Makefile])
AC_CONFIG_FILES([dlls/kernel32/tests/Makefile])
AC_CONFIG_FILES([dlls/localspl/Makefile])
AC_CONFIG_FILES([dlls/localspl/tests/Makefile])
AC_CONFIG_FILES([dlls/localui/Makefile])
AC_CONFIG_FILES([dlls/localui/tests/Makefile])
AC_CONFIG_FILES([dlls/lz32/Makefile])
AC_CONFIG_FILES([dlls/lz32/tests/Makefile])
AC_CONFIG_FILES([dlls/mapi32/Makefile])
AC_CONFIG_FILES([dlls/mapi32/tests/Makefile])
AC_CONFIG_FILES([dlls/mciavi32/Makefile])
AC_CONFIG_FILES([dlls/mcicda/Makefile])
AC_CONFIG_FILES([dlls/mciseq/Makefile])
AC_CONFIG_FILES([dlls/mciwave/Makefile])
AC_CONFIG_FILES([dlls/midimap/Makefile])
AC_CONFIG_FILES([dlls/mlang/Makefile])
AC_CONFIG_FILES([dlls/mlang/tests/Makefile])
AC_CONFIG_FILES([dlls/mmdevldr.vxd/Makefile])
AC_CONFIG_FILES([dlls/monodebg.vxd/Makefile])
AC_CONFIG_FILES([dlls/mpr/Makefile])
AC_CONFIG_FILES([dlls/mprapi/Makefile])
AC_CONFIG_FILES([dlls/msacm32.drv/Makefile])
AC_CONFIG_FILES([dlls/msacm32/Makefile])
AC_CONFIG_FILES([dlls/msacm32/tests/Makefile])
AC_CONFIG_FILES([dlls/msadp32.acm/Makefile])
AC_CONFIG_FILES([dlls/mscms/Makefile])
AC_CONFIG_FILES([dlls/mscms/tests/Makefile])
AC_CONFIG_FILES([dlls/mscoree/Makefile])
AC_CONFIG_FILES([dlls/msdmo/Makefile])
AC_CONFIG_FILES([dlls/msftedit/Makefile])
AC_CONFIG_FILES([dlls/msg711.acm/Makefile])
AC_CONFIG_FILES([dlls/mshtml/Makefile])
AC_CONFIG_FILES([dlls/mshtml/tests/Makefile])
AC_CONFIG_FILES([dlls/msi/Makefile])
AC_CONFIG_FILES([dlls/msi/tests/Makefile])
AC_CONFIG_FILES([dlls/msimg32/Makefile])
AC_CONFIG_FILES([dlls/msisys.ocx/Makefile])
AC_CONFIG_FILES([dlls/msnet32/Makefile])
AC_CONFIG_FILES([dlls/msrle32/Makefile])
AC_CONFIG_FILES([dlls/msvcrt/Makefile])
AC_CONFIG_FILES([dlls/msvcrt/tests/Makefile])
AC_CONFIG_FILES([dlls/msvcrt20/Makefile])
AC_CONFIG_FILES([dlls/msvcrt40/Makefile])
AC_CONFIG_FILES([dlls/msvcrtd/Makefile])
AC_CONFIG_FILES([dlls/msvcrtd/tests/Makefile])
AC_CONFIG_FILES([dlls/msvfw32/Makefile])
AC_CONFIG_FILES([dlls/msvidc32/Makefile])
AC_CONFIG_FILES([dlls/mswsock/Makefile])
AC_CONFIG_FILES([dlls/msxml3/Makefile])
AC_CONFIG_FILES([dlls/msxml3/tests/Makefile])
AC_CONFIG_FILES([dlls/nddeapi/Makefile])
AC_CONFIG_FILES([dlls/netapi32/Makefile])
AC_CONFIG_FILES([dlls/netapi32/tests/Makefile])
AC_CONFIG_FILES([dlls/newdev/Makefile])
AC_CONFIG_FILES([dlls/ntdll/Makefile])
AC_CONFIG_FILES([dlls/ntdll/tests/Makefile])
AC_CONFIG_FILES([dlls/ntdsapi/Makefile])
AC_CONFIG_FILES([dlls/ntoskrnl.exe/Makefile])
AC_CONFIG_FILES([dlls/ntprint/Makefile])
AC_CONFIG_FILES([dlls/ntprint/tests/Makefile])
AC_CONFIG_FILES([dlls/objsel/Makefile])
AC_CONFIG_FILES([dlls/odbc32/Makefile])
AC_CONFIG_FILES([dlls/odbccp32/Makefile])
AC_CONFIG_FILES([dlls/odbccp32/tests/Makefile])
AC_CONFIG_FILES([dlls/ole32/Makefile])
AC_CONFIG_FILES([dlls/ole32/tests/Makefile])
AC_CONFIG_FILES([dlls/oleacc/Makefile])
AC_CONFIG_FILES([dlls/oleaut32/Makefile])
AC_CONFIG_FILES([dlls/oleaut32/tests/Makefile])
AC_CONFIG_FILES([dlls/olecli32/Makefile])
AC_CONFIG_FILES([dlls/oledlg/Makefile])
AC_CONFIG_FILES([dlls/olepro32/Makefile])
AC_CONFIG_FILES([dlls/olesvr32/Makefile])
AC_CONFIG_FILES([dlls/opengl32/Makefile])
AC_CONFIG_FILES([dlls/pdh/Makefile])
AC_CONFIG_FILES([dlls/pdh/tests/Makefile])
AC_CONFIG_FILES([dlls/powrprof/Makefile])
AC_CONFIG_FILES([dlls/printui/Makefile])
AC_CONFIG_FILES([dlls/psapi/Makefile])
AC_CONFIG_FILES([dlls/psapi/tests/Makefile])
AC_CONFIG_FILES([dlls/pstorec/Makefile])
AC_CONFIG_FILES([dlls/qcap/Makefile])
AC_CONFIG_FILES([dlls/quartz/Makefile])
AC_CONFIG_FILES([dlls/quartz/tests/Makefile])
AC_CONFIG_FILES([dlls/query/Makefile])
AC_CONFIG_FILES([dlls/rasapi32/Makefile])
AC_CONFIG_FILES([dlls/resutils/Makefile])
AC_CONFIG_FILES([dlls/riched20/Makefile])
AC_CONFIG_FILES([dlls/riched20/tests/Makefile])
AC_CONFIG_FILES([dlls/riched32/Makefile])
AC_CONFIG_FILES([dlls/rpcrt4/Makefile])
AC_CONFIG_FILES([dlls/rpcrt4/tests/Makefile])
AC_CONFIG_FILES([dlls/rsabase/Makefile])
AC_CONFIG_FILES([dlls/rsabase/tests/Makefile])
AC_CONFIG_FILES([dlls/rsaenh/Makefile])
AC_CONFIG_FILES([dlls/rsaenh/tests/Makefile])
AC_CONFIG_FILES([dlls/sane.ds/Makefile])
AC_CONFIG_FILES([dlls/schannel/Makefile])
AC_CONFIG_FILES([dlls/schannel/tests/Makefile])
AC_CONFIG_FILES([dlls/secur32/Makefile])
AC_CONFIG_FILES([dlls/secur32/tests/Makefile])
AC_CONFIG_FILES([dlls/security/Makefile])
AC_CONFIG_FILES([dlls/sensapi/Makefile])
AC_CONFIG_FILES([dlls/serialui/Makefile])
AC_CONFIG_FILES([dlls/serialui/tests/Makefile])
AC_CONFIG_FILES([dlls/setupapi/Makefile])
AC_CONFIG_FILES([dlls/setupapi/tests/Makefile])
AC_CONFIG_FILES([dlls/sfc/Makefile])
AC_CONFIG_FILES([dlls/sfc_os/Makefile])
AC_CONFIG_FILES([dlls/shdoclc/Makefile])
AC_CONFIG_FILES([dlls/shdocvw/Makefile])
AC_CONFIG_FILES([dlls/shdocvw/tests/Makefile])
AC_CONFIG_FILES([dlls/shell32/Makefile])
AC_CONFIG_FILES([dlls/shell32/tests/Makefile])
AC_CONFIG_FILES([dlls/shfolder/Makefile])
AC_CONFIG_FILES([dlls/shlwapi/Makefile])
AC_CONFIG_FILES([dlls/shlwapi/tests/Makefile])
AC_CONFIG_FILES([dlls/snmpapi/Makefile])
AC_CONFIG_FILES([dlls/snmpapi/tests/Makefile])
AC_CONFIG_FILES([dlls/spoolss/Makefile])
AC_CONFIG_FILES([dlls/stdole2.tlb/Makefile])
AC_CONFIG_FILES([dlls/stdole32.tlb/Makefile])
AC_CONFIG_FILES([dlls/sti/Makefile])
AC_CONFIG_FILES([dlls/strmiids/Makefile])
AC_CONFIG_FILES([dlls/svrapi/Makefile])
AC_CONFIG_FILES([dlls/sxs/Makefile])
AC_CONFIG_FILES([dlls/tapi32/Makefile])
AC_CONFIG_FILES([dlls/twain_32/Makefile])
AC_CONFIG_FILES([dlls/unicows/Makefile])
AC_CONFIG_FILES([dlls/url/Makefile])
AC_CONFIG_FILES([dlls/urlmon/Makefile])
AC_CONFIG_FILES([dlls/urlmon/tests/Makefile])
AC_CONFIG_FILES([dlls/user32/Makefile])
AC_CONFIG_FILES([dlls/user32/tests/Makefile])
AC_CONFIG_FILES([dlls/userenv/Makefile])
AC_CONFIG_FILES([dlls/usp10/Makefile])
AC_CONFIG_FILES([dlls/usp10/tests/Makefile])
AC_CONFIG_FILES([dlls/uuid/Makefile])
AC_CONFIG_FILES([dlls/uxtheme/Makefile])
AC_CONFIG_FILES([dlls/uxtheme/tests/Makefile])
AC_CONFIG_FILES([dlls/vdhcp.vxd/Makefile])
AC_CONFIG_FILES([dlls/vdmdbg/Makefile])
AC_CONFIG_FILES([dlls/version/Makefile])
AC_CONFIG_FILES([dlls/version/tests/Makefile])
AC_CONFIG_FILES([dlls/vmm.vxd/Makefile])
AC_CONFIG_FILES([dlls/vnbt.vxd/Makefile])
AC_CONFIG_FILES([dlls/vnetbios.vxd/Makefile])
AC_CONFIG_FILES([dlls/vtdapi.vxd/Makefile])
AC_CONFIG_FILES([dlls/vwin32.vxd/Makefile])
AC_CONFIG_FILES([dlls/w32skrnl/Makefile])
AC_CONFIG_FILES([dlls/winealsa.drv/Makefile])
AC_CONFIG_FILES([dlls/wineaudioio.drv/Makefile])
AC_CONFIG_FILES([dlls/winecoreaudio.drv/Makefile])
AC_CONFIG_FILES([dlls/winecrt0/Makefile])
AC_CONFIG_FILES([dlls/wined3d/Makefile])
AC_CONFIG_FILES([dlls/winedos/Makefile])
AC_CONFIG_FILES([dlls/wineesd.drv/Makefile])
AC_CONFIG_FILES([dlls/winejack.drv/Makefile])
AC_CONFIG_FILES([dlls/winejoystick.drv/Makefile])
AC_CONFIG_FILES([dlls/winemp3.acm/Makefile])
AC_CONFIG_FILES([dlls/winenas.drv/Makefile])
AC_CONFIG_FILES([dlls/wineoss.drv/Makefile])
AC_CONFIG_FILES([dlls/wineps.drv/Makefile])
AC_CONFIG_FILES([dlls/winequartz.drv/Makefile])
AC_CONFIG_FILES([dlls/winex11.drv/Makefile])
AC_CONFIG_FILES([dlls/wing32/Makefile])
AC_CONFIG_FILES([dlls/wininet/Makefile])
AC_CONFIG_FILES([dlls/wininet/tests/Makefile])
AC_CONFIG_FILES([dlls/winmm/Makefile])
AC_CONFIG_FILES([dlls/winmm/tests/Makefile])
AC_CONFIG_FILES([dlls/winnls32/Makefile])
AC_CONFIG_FILES([dlls/winscard/Makefile])
AC_CONFIG_FILES([dlls/winspool.drv/Makefile])
AC_CONFIG_FILES([dlls/winspool.drv/tests/Makefile])
AC_CONFIG_FILES([dlls/wintab32/Makefile])
AC_CONFIG_FILES([dlls/wintrust/Makefile])
AC_CONFIG_FILES([dlls/wintrust/tests/Makefile])
AC_CONFIG_FILES([dlls/wldap32/Makefile])
AC_CONFIG_FILES([dlls/wnaspi32/Makefile])
AC_CONFIG_FILES([dlls/wow32/Makefile])
AC_CONFIG_FILES([dlls/ws2_32/Makefile])
AC_CONFIG_FILES([dlls/ws2_32/tests/Makefile])
AC_CONFIG_FILES([dlls/wsock32/Makefile])
AC_CONFIG_FILES([dlls/wtsapi32/Makefile])
AC_CONFIG_FILES([documentation/Makefile])
AC_CONFIG_FILES([fonts/Makefile])
AC_CONFIG_FILES([include/Makefile])
AC_CONFIG_FILES([libs/Makefile])
AC_CONFIG_FILES([libs/port/Makefile])
AC_CONFIG_FILES([libs/wine/Makefile])
AC_CONFIG_FILES([libs/wpp/Makefile])
AC_CONFIG_FILES([loader/Makefile])
AC_CONFIG_FILES([programs/Makefile])
AC_CONFIG_FILES([programs/clock/Makefile])
AC_CONFIG_FILES([programs/cmd/Makefile])
AC_CONFIG_FILES([programs/cmdlgtst/Makefile])
AC_CONFIG_FILES([programs/control/Makefile])
AC_CONFIG_FILES([programs/eject/Makefile])
AC_CONFIG_FILES([programs/expand/Makefile])
AC_CONFIG_FILES([programs/explorer/Makefile])
AC_CONFIG_FILES([programs/hh/Makefile])
AC_CONFIG_FILES([programs/icinfo/Makefile])
AC_CONFIG_FILES([programs/iexplore/Makefile])
AC_CONFIG_FILES([programs/msiexec/Makefile])
AC_CONFIG_FILES([programs/net/Makefile])
AC_CONFIG_FILES([programs/notepad/Makefile])
AC_CONFIG_FILES([programs/oleview/Makefile])
AC_CONFIG_FILES([programs/progman/Makefile])
AC_CONFIG_FILES([programs/regedit/Makefile])
AC_CONFIG_FILES([programs/regsvr32/Makefile])
AC_CONFIG_FILES([programs/rpcss/Makefile])
AC_CONFIG_FILES([programs/rundll32/Makefile])
AC_CONFIG_FILES([programs/spoolsv/Makefile])
AC_CONFIG_FILES([programs/start/Makefile])
AC_CONFIG_FILES([programs/taskmgr/Makefile])
AC_CONFIG_FILES([programs/uninstaller/Makefile])
AC_CONFIG_FILES([programs/view/Makefile])
AC_CONFIG_FILES([programs/wineboot/Makefile])
AC_CONFIG_FILES([programs/winebrowser/Makefile])
AC_CONFIG_FILES([programs/winecfg/Makefile])
AC_CONFIG_FILES([programs/wineconsole/Makefile])
AC_CONFIG_FILES([programs/winedbg/Makefile])
AC_CONFIG_FILES([programs/winedevice/Makefile])
AC_CONFIG_FILES([programs/winefile/Makefile])
AC_CONFIG_FILES([programs/winemenubuilder/Makefile])
AC_CONFIG_FILES([programs/winemine/Makefile])
AC_CONFIG_FILES([programs/winepath/Makefile])
AC_CONFIG_FILES([programs/winetest/Makefile])
AC_CONFIG_FILES([programs/winevdm/Makefile])
AC_CONFIG_FILES([programs/winhelp/Makefile])
AC_CONFIG_FILES([programs/winver/Makefile])
AC_CONFIG_FILES([programs/wordpad/Makefile])
AC_CONFIG_FILES([programs/xcopy/Makefile])
AC_CONFIG_FILES([server/Makefile])
AC_CONFIG_FILES([tools/Makefile])
AC_CONFIG_FILES([tools/widl/Makefile])
AC_CONFIG_FILES([tools/winebuild/Makefile])
AC_CONFIG_FILES([tools/winedump/Makefile])
AC_CONFIG_FILES([tools/winegcc/Makefile])
AC_CONFIG_FILES([tools/wmc/Makefile])
AC_CONFIG_FILES([tools/wrc/Makefile])

AC_OUTPUT

if test "$no_create" = "yes"
then
  exit 0
fi

WINE_PRINT_MESSAGES

echo
echo "Configure finished.  Do '${ac_make} depend && ${ac_make}' to compile Wine."
echo

dnl Local Variables:
dnl comment-start: "dnl "
dnl comment-end: ""
dnl comment-start-skip: "\\bdnl\\b\\s *"
dnl compile-command: "autoreconf --warnings=all"
dnl End:
