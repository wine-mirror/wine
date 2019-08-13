dnl Process this file with autoconf to produce a configure script.
dnl Original author: Michael Patra
dnl For detailed change history, see the git commit logs.

m4_define(WINE_VERSION,regexp(m4_include(VERSION),[version \([-.0-9A-Za-z]+\)],[\1]))

dnl autoconf versions before 2.62 don't handle source dir symlinks correctly
AC_PREREQ(2.62)
AC_INIT([Wine],[WINE_VERSION],[wine-devel@winehq.org],[wine],[https://www.winehq.org])
AC_CONFIG_SRCDIR(server/atom.c)
AC_CONFIG_HEADERS(include/config.h)
AC_CONFIG_AUX_DIR(tools)

libwine_version="1.0"

dnl autoconf versions before 2.63b don't have AS_VAR_APPEND or AS_VAR_IF
m4_ifdef([AS_VAR_APPEND],,[as_fn_append () { eval $[1]=\$$[1]\$[2]; }
AC_DEFUN([AS_VAR_APPEND],[as_fn_append $1 $2])])dnl
m4_ifdef([AS_VAR_IF],,[AC_DEFUN([AS_VAR_IF],
[AS_LITERAL_IF([$1],
  [AS_IF([test "x$$1" = x""$2], [$3], [$4])],
  [eval as_val=\$$1
   AS_IF([test "x$as_val" = x""$2], [$3], [$4])])])])dnl
dnl autoconf versions before 2.64 don't have AC_PACKAGE_URL
m4_ifdef([AC_PACKAGE_URL],,
  [AC_DEFINE([PACKAGE_URL], ["https://www.winehq.org"], [Define to the home page for this package.])
   AC_SUBST([PACKAGE_URL], ["https://www.winehq.org"])])dnl

dnl **** Command-line arguments ****

AC_ARG_ENABLE(win16, AS_HELP_STRING([--disable-win16],[do not include Win16 support]))
AC_ARG_ENABLE(win64, AS_HELP_STRING([--enable-win64],[build a Win64 emulator on AMD64 (won't run Win32 binaries)]))
AC_ARG_ENABLE(tests, AS_HELP_STRING([--disable-tests],[do not build the regression tests]))
AC_ARG_ENABLE(maintainer-mode, AS_HELP_STRING([--enable-maintainer-mode],[enable maintainer-specific build rules]))

AC_ARG_WITH(alsa,      AS_HELP_STRING([--without-alsa],[do not use the Alsa sound support]))
AC_ARG_WITH(capi,      AS_HELP_STRING([--without-capi],[do not use CAPI (ISDN support)]))
AC_ARG_WITH(cms,       AS_HELP_STRING([--without-cms],[do not use CMS (color management support)]))
AC_ARG_WITH(coreaudio, AS_HELP_STRING([--without-coreaudio],[do not use the CoreAudio sound support]),
            [if test "x$withval" = "xno"; then ac_cv_header_CoreAudio_CoreAudio_h=no; fi])
AC_ARG_WITH(cups,      AS_HELP_STRING([--without-cups],[do not use CUPS]))
AC_ARG_WITH(curses,    AS_HELP_STRING([--without-curses],[do not use (n)curses]),
            [if test "x$withval" = "xno"; then ac_cv_header_ncurses_h=no; ac_cv_header_curses_h=no; fi])
AC_ARG_WITH(dbus,      AS_HELP_STRING([--without-dbus],[do not use DBus (dynamic device support)]))
AC_ARG_WITH(faudio,    AS_HELP_STRING([--without-faudio],[do not use FAudio (XAudio2 support)]))
AC_ARG_WITH(float-abi, AS_HELP_STRING([--with-float-abi=abi],[specify the ABI (soft|softfp|hard) for ARM platforms]))
AC_ARG_WITH(fontconfig,AS_HELP_STRING([--without-fontconfig],[do not use fontconfig]))
AC_ARG_WITH(freetype,  AS_HELP_STRING([--without-freetype],[do not use the FreeType library]))
AC_ARG_WITH(gettext,   AS_HELP_STRING([--without-gettext],[do not use gettext]))
AC_ARG_WITH(gettextpo, AS_HELP_STRING([--with-gettextpo],[use the GetTextPO library to rebuild po files]),
            [if test "x$withval" = "xno"; then ac_cv_header_gettext_po_h=no; fi])
AC_ARG_WITH(gphoto,    AS_HELP_STRING([--without-gphoto],[do not use gphoto (Digital Camera support)]))
AC_ARG_WITH(glu,       AS_HELP_STRING([--without-glu],[do not use the GLU library]))
AC_ARG_WITH(gnutls,    AS_HELP_STRING([--without-gnutls],[do not use GnuTLS (schannel support)]))
AC_ARG_WITH(gsm,       AS_HELP_STRING([--without-gsm],[do not use libgsm (GSM 06.10 codec support)]),
            [if test "x$withval" = "xno"; then ac_cv_header_gsm_h=no; ac_cv_header_gsm_gsm_h=no; fi])
AC_ARG_WITH(gssapi,    AS_HELP_STRING([--without-gssapi],[do not use GSSAPI (Kerberos SSP support)]))
AC_ARG_WITH(gstreamer, AS_HELP_STRING([--without-gstreamer],[do not use GStreamer (codecs support)]))
AC_ARG_WITH(hal,       AS_HELP_STRING([--without-hal],[do not use HAL (dynamic device support)]))
AC_ARG_WITH(inotify,   AS_HELP_STRING([--without-inotify],[do not use inotify (filesystem change notifications)]))
AC_ARG_WITH(jpeg,      AS_HELP_STRING([--without-jpeg],[do not use JPEG]))
AC_ARG_WITH(krb5,      AS_HELP_STRING([--without-krb5],[do not use krb5 (Kerberos)]))
AC_ARG_WITH(ldap,      AS_HELP_STRING([--without-ldap],[do not use LDAP]),
            [if test "x$withval" = "xno"; then ac_cv_header_ldap_h=no; ac_cv_header_lber_h=no; fi])
AC_ARG_WITH(mingw,     AS_HELP_STRING([--without-mingw],[do not use the MinGW cross-compiler]))
AC_ARG_WITH(mpg123,    AS_HELP_STRING([--without-mpg123],[do not use the mpg123 library]))
AC_ARG_WITH(netapi,    AS_HELP_STRING([--without-netapi],[do not use the Samba NetAPI library]))
AC_ARG_WITH(openal,    AS_HELP_STRING([--without-openal],[do not use OpenAL]),
            [if test "x$withval" = "xno"; then ac_cv_header_AL_al_h=no; ac_cv_header_OpenAL_al_h=no; fi])
AC_ARG_WITH(opencl,    AS_HELP_STRING([--without-opencl],[do not use OpenCL]),
            [if test "x$withval" = "xno"; then ac_cv_header_CL_cl_h=no; ac_cv_header_OpenCL_opencl_h=no; fi])
AC_ARG_WITH(opengl,    AS_HELP_STRING([--without-opengl],[do not use OpenGL]))
AC_ARG_WITH(osmesa,     AS_HELP_STRING([--without-osmesa],[do not use the OSMesa library]))
AC_ARG_WITH(oss,       AS_HELP_STRING([--without-oss],[do not use the OSS sound support]))
AC_ARG_WITH(pcap,      AS_HELP_STRING([--without-pcap],[do not use the Packet Capture library]),
            [if test "x$withval" = "xno"; then ac_cv_header_pcap_pcap_h=no; fi])
AC_ARG_WITH(png,       AS_HELP_STRING([--without-png],[do not use PNG]))
AC_ARG_WITH(pthread,   AS_HELP_STRING([--without-pthread],[do not use the pthread library]),
            [if test "x$withval" = "xno"; then ac_cv_header_pthread_h=no; fi])
AC_ARG_WITH(pulse,     AS_HELP_STRING([--without-pulse],[do not use PulseAudio sound support]))
AC_ARG_WITH(sane,      AS_HELP_STRING([--without-sane],[do not use SANE (scanner support)]))
AC_ARG_WITH(sdl,       AS_HELP_STRING([--without-sdl],[do not use SDL]))
AC_ARG_WITH(tiff,      AS_HELP_STRING([--without-tiff],[do not use TIFF]))
AC_ARG_WITH(udev,      AS_HELP_STRING([--without-udev],[do not use udev (plug and play support)]))
AC_ARG_WITH(v4l2,      AS_HELP_STRING([--without-v4l2],[do not use v4l2 (video capture)]))
AC_ARG_WITH(vkd3d,     AS_HELP_STRING([--without-vkd3d],[do not use vkd3d (Direct3D 12 support)]))
AC_ARG_WITH(vulkan,    AS_HELP_STRING([--without-vulkan],[do not use Vulkan]))
AC_ARG_WITH(xcomposite,AS_HELP_STRING([--without-xcomposite],[do not use the Xcomposite extension]),
            [if test "x$withval" = "xno"; then ac_cv_header_X11_extensions_Xcomposite_h=no; fi])
AC_ARG_WITH(xcursor,   AS_HELP_STRING([--without-xcursor],[do not use the Xcursor extension]),
            [if test "x$withval" = "xno"; then ac_cv_header_X11_Xcursor_Xcursor_h=no; fi])
AC_ARG_WITH(xfixes,  AS_HELP_STRING([--without-xfixes],[do not use Xfixes for clipboard change notifications]),
            [if test "x$withval" = "xno"; then ac_cv_header_X11_extensions_Xfixes_h=no; fi])
AC_ARG_WITH(xinerama,  AS_HELP_STRING([--without-xinerama],[do not use Xinerama (multi-monitor support)]),
            [if test "x$withval" = "xno"; then ac_cv_header_X11_extensions_Xinerama_h=no; fi])
AC_ARG_WITH(xinput,    AS_HELP_STRING([--without-xinput],[do not use the Xinput extension]),
            [if test "x$withval" = "xno"; then ac_cv_header_X11_extensions_XInput_h=no; fi])
AC_ARG_WITH(xinput2,   AS_HELP_STRING([--without-xinput2],[do not use the Xinput 2 extension]),
            [if test "x$withval" = "xno"; then ac_cv_header_X11_extensions_XInput2_h=no; fi])
AC_ARG_WITH(xml,       AS_HELP_STRING([--without-xml],[do not use XML]))
AC_ARG_WITH(xrandr,    AS_HELP_STRING([--without-xrandr],[do not use Xrandr (resolution changes)]),
            [if test "x$withval" = "xno"; then ac_cv_header_X11_extensions_Xrandr_h=no; fi])
AC_ARG_WITH(xrender,   AS_HELP_STRING([--without-xrender],[do not use the Xrender extension]),
            [if test "x$withval" = "xno"; then ac_cv_header_X11_extensions_Xrender_h=no; fi])
AC_ARG_WITH(xshape,    AS_HELP_STRING([--without-xshape],[do not use the Xshape extension]),
            [if test "x$withval" = "xno"; then ac_cv_header_X11_extensions_shape_h=no; fi])
AC_ARG_WITH(xshm,      AS_HELP_STRING([--without-xshm],[do not use XShm (shared memory extension)]),
            [if test "x$withval" = "xno"; then ac_cv_header_X11_extensions_XShm_h=no; fi])
AC_ARG_WITH(xslt,      AS_HELP_STRING([--without-xslt],[do not use XSLT]))
AC_ARG_WITH(xxf86vm,   AS_HELP_STRING([--without-xxf86vm],[do not use XFree video mode extension]),
            [if test "x$withval" = "xno"; then ac_cv_header_X11_extensions_xf86vmode_h=no; ac_cv_header_X11_extensions_xf86vmproto_h=no; fi])
AC_ARG_WITH(zlib,      AS_HELP_STRING([--without-zlib],[do not use Zlib (data compression)]))

AC_ARG_WITH(wine-tools,AS_HELP_STRING([--with-wine-tools=DIR],[use Wine tools from directory DIR]))
AC_ARG_WITH(wine64,    AS_HELP_STRING([--with-wine64=DIR],[use the 64-bit Wine in DIR for a Wow64 build]))

AC_CANONICAL_HOST

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
AC_DEFINE_UNQUOTED(EXEEXT,["$ac_exeext"],[Define to the file extension for executables.])
AC_CHECK_TOOL(LD,ld)

case $host in
  *-darwin*)
    with_fontconfig=${with_fontconfig:-no}
    ;;
  *-mingw32*|*-cygwin*)
    enable_win16=${enable_win16:-no}
    with_mingw=${with_mingw:-no}
    CFLAGS="$CFLAGS -D_WIN32"
    ;;
esac

case $host in
  x86_64*|amd64*)
    if test "x$enable_win64" != "xyes" -a "$cross_compiling" != "yes"
    then
      CC="$CC -m32"
      CXX="$CXX -m32"
      AC_CACHE_CHECK([whether $CC works], wine_cv_cc_m32,
          [AC_LINK_IFELSE([AC_LANG_PROGRAM()],[wine_cv_cc_m32=yes],[wine_cv_cc_m32=no])])
      test $wine_cv_cc_m32 != no || AC_MSG_ERROR([Cannot build a 32-bit program, you need to install 32-bit development libraries.])
      host_cpu="i386"
      notice_platform="32-bit "
      AC_SUBST(TARGETFLAGS,"-m32")
      PKG_CONFIG_PATH=${PKG_CONFIG_PATH:-/usr/lib/i386-linux-gnu/pkgconfig:/usr/lib/pkgconfig}
      export PKG_CONFIG_PATH
      enable_win16=${enable_win16:-yes}
    else
      if test "x${GCC}" = "xyes"
      then
        AC_CACHE_CHECK([whether $CC supports __builtin_ms_va_list],wine_cv_builtin_ms_va_list,
            [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdarg.h>]], [[void func(__builtin_ms_va_list *args);]])],
                          [wine_cv_builtin_ms_va_list=yes],[wine_cv_builtin_ms_va_list=no])])
        test $wine_cv_builtin_ms_va_list != no || AC_MSG_ERROR([You need gcc >= 4.4 or clang >= 3.8 to build Wine as 64-bit.])
      fi
      CC="$CC -m64"
      CXX="$CXX -m64"
      host_cpu="x86_64"
      notice_platform="64-bit "
      AC_SUBST(TARGETFLAGS,"-m64")
    fi
    ;;
  arm*)
    AC_CACHE_CHECK([whether $CC supports Thumb],wine_cv_thumb,
        [WINE_TRY_ASM_LINK([".thumb\nblx ac_test\n.arm"],[int ac_test(int i) { return i; }], [if (ac_test(1)) return 1],
                           [wine_cv_thumb=yes],[wine_cv_thumb=no])])
    test $wine_cv_thumb != no || AC_MSG_ERROR([You need a target with Thumb support to build Wine for ARM.])
    case $with_float_abi in
      soft|softfp|hard)
        float_abi=$with_float_abi ;;
      *)
        case $host_os in
          *eabihf)
            float_abi=hard ;;
          *)
            float_abi=softfp
            saved_CFLAGS=$CFLAGS
            CFLAGS="$CFLAGS -marm -mfloat-abi=$float_abi"
            AC_CACHE_CHECK([whether $CC supports -mfloat-abi=$float_abi],wine_cv_float_abi,
                [WINE_TRY_ASM_LINK(["vmrs r2,fpscr"],,,[wine_cv_float_abi=yes],[wine_cv_float_abi=no])])
            if test $wine_cv_float_abi = no
            then
                float_abi=soft
                WINE_WARNING([Floating point is not supported for this target. The resulting build won't be compatible with Windows ARM binaries.])
            fi
            CFLAGS=$saved_CFLAGS
        esac
        ;;
    esac
    CFLAGS="$CFLAGS -marm -mfloat-abi=$float_abi"
    AC_SUBST(TARGETFLAGS,"-marm -mfloat-abi=$float_abi")
    ;;
  aarch64*)
    AC_CACHE_CHECK([whether $CC supports __builtin_ms_va_list],wine_cv_builtin_ms_va_list,
        [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdarg.h>]], [[void func(__builtin_ms_va_list *args);]])],
                      [wine_cv_builtin_ms_va_list=yes],[wine_cv_builtin_ms_va_list=no])])
    test $wine_cv_builtin_ms_va_list != no || AC_MSG_ERROR([You need clang >= 5.0 to build Wine for arm64.])
    ;;
  i[[3456789]]86*)
    enable_win16=${enable_win16:-yes}
    ;;
esac

dnl enable_win16 defaults to yes on x86, to no on other CPUs
enable_win16=${enable_win16:-no}
enable_win64=${enable_win64:-no}

dnl Disable winetest too if tests are disabled
enable_winetest=${enable_winetest:-$enable_tests}

dnl Some special cases for the 64-bit build
if test "x$enable_win64" = "xyes"
then
    test -z "$with_wine64" || AC_MSG_ERROR([--enable-win64 and --with-wine64 are mutually exclusive.
--enable-win64 should be used in the 64-bit build tree, --with-wine64 in the 32-bit Wow64 build tree.])
    test "x$libdir" != "x\${exec_prefix}/lib" || libdir="\${exec_prefix}/lib64"
fi

AC_CACHE_CHECK([for the directory containing the Wine tools], wine_cv_toolsdir,
  [wine_cv_toolsdir="$with_wine_tools"
   if test -z "$with_wine_tools"; then
     if test "$cross_compiling" = "yes"; then
       AC_MSG_ERROR([you must use the --with-wine-tools option when cross-compiling.])
     elif test -n "$with_wine64"; then
       wine_cv_toolsdir="$with_wine64"
     fi
   fi])
if test -z "$wine_cv_toolsdir"; then
    wine_makedep=tools/makedep
    AC_SUBST(TOOLSDIR,"\$(top_builddir)")
elif test -d "$wine_cv_toolsdir/tools/winebuild"; then
    wine_makedep=$wine_cv_toolsdir/tools/makedep
    case "$wine_cv_toolsdir" in
       /*) AC_SUBST(TOOLSDIR,$wine_cv_toolsdir) ;;
       *)  AC_SUBST(TOOLSDIR,"\$(top_builddir)/$wine_cv_toolsdir") ;;
    esac
    enable_tools=${enable_tools:-no}
    test -f "$wine_makedep" || AC_MSG_ERROR([the Wine tools have not yet been built in $wine_cv_toolsdir])
else
    AC_MSG_ERROR([could not find Wine tools in $wine_cv_toolsdir])
fi
if test -n "$host_alias" -a "$host_alias" != "$build_alias"
then
    AC_SUBST(TARGETFLAGS,"-b $host_alias $TARGETFLAGS")
fi

dnl Check for flex
AC_CHECK_PROGS(FLEX,flex,none)
if test "$FLEX" = "none"
then
  AC_MSG_ERROR([no suitable flex found. Please install the 'flex' package.])
fi
AC_CACHE_CHECK([whether flex is recent enough],wine_cv_recent_flex,
    [cat >conftest.l <<EOF
%top{
#include "prediluvian.h"
}
%%
EOF
    if $FLEX -t conftest.l >/dev/null 2>&AS_MESSAGE_LOG_FD
    then
      wine_cv_recent_flex=yes
    else
      wine_cv_recent_flex=no
    fi])
test $wine_cv_recent_flex != no || AC_MSG_ERROR([Your flex version is too old. Please install flex version 2.5.33 or newer.])

dnl Check for bison
AC_CHECK_PROGS(BISON,bison,none)
if test "$BISON" = "none"
then
  AC_MSG_ERROR([no suitable bison found. Please install the 'bison' package.])
fi
AC_CACHE_CHECK([whether bison is recent enough],wine_cv_recent_bison,
    [cat >conftest.y <<EOF
%define parse.error verbose
%%
empty: ;
EOF
    if $BISON conftest.y >/dev/null 2>&AS_MESSAGE_LOG_FD
    then
      wine_cv_recent_bison=yes
    else
      wine_cv_recent_bison=no
    fi])
test $wine_cv_recent_bison != no || AC_MSG_ERROR([Your bison version is too old. Please install bison version 3.0 or newer.])

AC_CHECK_TOOLS(AR,[ar gar],ar)
AC_CHECK_TOOL(STRIP,strip,strip)
AC_PROG_RANLIB
AC_PROG_LN_S
AC_PROG_EGREP
AC_PATH_PROG(LDCONFIG, ldconfig, true, [/sbin /usr/sbin $PATH])
AC_CHECK_PROGS(MSGFMT, msgfmt, false)
WINE_PATH_PKG_CONFIG

if test "x$enable_maintainer_mode" != "xyes"
then
  FONTFORGE=""
  RSVG=""
  CONVERT=""
  ICOTOOL=""
else
  test "$srcdir" = . || AC_MSG_ERROR([Maintainer mode cannot work out of tree.])
  AC_CHECK_PROGS(FONTFORGE, fontforge, false)
  AC_CHECK_PROGS(RSVG, rsvg-convert rsvg, false)
  AC_CHECK_PROGS(CONVERT, convert, false)
  AC_CHECK_PROGS(ICOTOOL, icotool, false)
  test "$FONTFORGE" != "false" || AC_MSG_ERROR([You need fontforge to rebuild fonts in maintainer mode.])
  test "$RSVG" != "false" || AC_MSG_ERROR([You need rsvg to rebuild icons in maintainer mode.])

  dnl Check the imagemagick version
  if test "$CONVERT" = false
  then
    AC_MSG_ERROR([You need imagemagick to rebuild icons in maintainer mode.])
  else
    AC_MSG_CHECKING([for recent enough imagemagick])
    convert_version=`convert --version | head -n1`
    if test "x$convert_version" != "x"
    then
      convert_version_major=`expr "$convert_version" : '.* \([[0-9]]*\)\.[[0-9]]*'`
      convert_version_minor=`expr "$convert_version" : '.* [[0-9]]*\.\([[0-9]]*\)'`
      if test "$convert_version_major" -eq 6 -a "$convert_version_minor" -lt 6
      then
        CONVERT=false
      fi
    fi
    if test "$CONVERT" = false
    then
      AC_MSG_RESULT([no ($convert_version_major.$convert_version_minor)])
      AC_MSG_ERROR([You need imagemagick version 6.6 or newer to rebuild icons in maintainer mode.])
    else
      AC_MSG_RESULT([yes ($convert_version_major.$convert_version_minor)])
    fi
  fi

  dnl Check the icotool version
  if test "$ICOTOOL" = false
  then
    AC_MSG_ERROR([You need icotool to rebuild icons in maintainer mode.])
  else
    AC_MSG_CHECKING([for recent enough icotool])
    icotool_version=`icotool --version | head -n1`
    if test "x$icotool_version" != "x"
    then
      icotool_version_major=`expr "$icotool_version" : '.* \([[0-9]]*\)\.[[0-9]]*'`
      icotool_version_minor=`expr "$icotool_version" : '.* [[0-9]]*\.\([[0-9]]*\)'`
      if test "$icotool_version_major" -eq 0 -a "$icotool_version_minor" -lt 29
      then
        ICOTOOL=false
        WINE_WARNING([icotool version 0.29.0 or newer is needed to rebuild icons.])
      fi
    fi
    if test "$ICOTOOL" = false
    then
      AC_MSG_RESULT([no ($icotool_version_major.$icotool_version_minor)])
      AC_MSG_ERROR([You need icotool version 0.29.0 or newer to rebuild icons in maintainer mode.])
    else
      AC_MSG_RESULT([yes ($icotool_version_major.$icotool_version_minor)])
    fi
  fi

  dnl Maintainer mode requires gettext
  with_gettext=yes
  with_gettextpo=yes
fi

test "x$with_gettext" != xno || MSGFMT=false
if test "$MSGFMT" != "false"
then
  AC_CACHE_CHECK([whether msgfmt supports contexts],wine_cv_msgfmt_contexts,
  [cat >conftest.po <<EOF
# comment
msgctxt "ctxt"
msgid "id"
msgstr "str"
EOF
  if $MSGFMT -o /dev/null conftest.po 2>&AS_MESSAGE_LOG_FD
  then
    wine_cv_msgfmt_contexts=yes
  else
    wine_cv_msgfmt_contexts=no
  fi])
  test $wine_cv_msgfmt_contexts != no || MSGFMT=false
fi
WINE_WARNING_WITH(gettext,[test "$MSGFMT" = false],
                  [gettext tools not found (or too old), translations won't be built.],
                  [enable_po])

dnl **** Check for some libraries ****

dnl Check for -li386 for NetBSD and OpenBSD
AC_CHECK_LIB(i386,i386_set_ldt)
dnl Check for -lossaudio for NetBSD
AC_CHECK_LIB(ossaudio,_oss_ioctl)

AC_SUBST(OPENGL_LIBS,"")

dnl **** Check for header files ****

AC_SYS_LARGEFILE()

AC_CHECK_HEADERS(\
	AL/al.h \
	ApplicationServices/ApplicationServices.h \
	AudioToolbox/AudioConverter.h \
	AudioUnit/AudioUnit.h \
	AudioUnit/AudioComponent.h \
	CL/cl.h \
	Carbon/Carbon.h \
	CommonCrypto/CommonCryptor.h \
	CoreAudio/CoreAudio.h \
	CoreServices/CoreServices.h \
	DiskArbitration/DiskArbitration.h \
	EGL/egl.h \
	IOKit/IOKitLib.h \
	IOKit/hid/IOHIDLib.h \
	OpenAL/al.h \
	OpenCL/opencl.h \
	QuickTime/ImageCompression.h \
	Security/Security.h \
	alias.h \
	arpa/inet.h \
	arpa/nameser.h \
	asm/types.h \
	asm/user.h \
	curses.h \
	dirent.h \
	dlfcn.h \
	elf.h \
	float.h \
	fnmatch.h \
	getopt.h \
	gettext-po.h \
	grp.h \
	gsm.h \
	gsm/gsm.h \
	ieeefp.h \
	inet/mib2.h \
	kstat.h \
	libproc.h \
	link.h \
	linux/cdrom.h \
	linux/compiler.h \
	linux/filter.h \
	linux/hdreg.h \
	linux/hidraw.h \
	linux/input.h \
	linux/ioctl.h \
	linux/joystick.h \
	linux/major.h \
	linux/param.h \
	linux/serial.h \
	linux/types.h \
	linux/ucdrom.h \
	lwp.h \
	mach-o/nlist.h \
	mach-o/loader.h \
	mach/mach.h \
	mach/machine.h \
	machine/cpu.h \
	machine/limits.h \
	machine/sysarch.h \
	mntent.h \
	ncurses.h \
	netdb.h \
	netinet/in.h \
	netinet/in_systm.h \
	netinet/tcp.h \
	netinet/tcp_fsm.h \
	pcap/pcap.h \
	poll.h \
	port.h \
	pthread.h \
	pwd.h \
	sched.h \
	scsi/scsi.h \
	scsi/scsi_ioctl.h \
	scsi/sg.h \
	stdbool.h \
	stdint.h \
	stropts.h \
	sys/attr.h \
	sys/auxv.h \
	sys/cdio.h \
	sys/elf32.h \
	sys/epoll.h \
	sys/event.h \
	sys/exec_elf.h \
	sys/filio.h \
	sys/ioctl.h \
	sys/ipc.h \
	sys/limits.h \
	sys/link.h \
	sys/mman.h \
	sys/modem.h \
	sys/msg.h \
	sys/mtio.h \
	sys/param.h \
	sys/poll.h \
	sys/prctl.h \
	sys/protosw.h \
	sys/ptrace.h \
	sys/queue.h \
	sys/resource.h \
	sys/scsiio.h \
	sys/shm.h \
	sys/signal.h \
	sys/socket.h \
	sys/socketvar.h \
	sys/sockio.h \
	sys/statvfs.h \
	sys/strtio.h \
	sys/syscall.h \
	sys/sysinfo.h \
	sys/tihdr.h \
	sys/time.h \
	sys/timeout.h \
	sys/times.h \
	sys/uio.h \
	sys/user.h \
	sys/utsname.h \
	sys/vnode.h \
	sys/wait.h \
	syscall.h \
	termios.h \
	unistd.h \
	utime.h \
	valgrind/memcheck.h \
	valgrind/valgrind.h
)
WINE_HEADER_MAJOR()
AC_HEADER_STAT()

dnl **** Checks for headers that depend on other ones ****

AC_CHECK_HEADERS([sys/mount.h sys/statfs.h sys/sysctl.h sys/user.h sys/vfs.h],,,
    [#include <sys/types.h>
     #ifdef HAVE_SYS_PARAM_H
     # include <sys/param.h>
     #endif])

AC_CHECK_HEADERS(\
	netinet/ip.h \
	net/if.h \
	net/if_arp.h \
	net/if_dl.h \
	net/if_types.h \
	net/route.h \
	netinet/if_ether.h \
	netinet/if_inarp.h \
	netinet/in_pcb.h \
	netinet/ip_icmp.h \
	netinet/ip_var.h \
	netinet/udp.h \
	netipx/ipx.h \
	sys/un.h \
,,,[#include <sys/types.h>
     #ifdef HAVE_SYS_SOCKET_H
     # include <sys/socket.h>
     #endif
     #ifdef HAVE_SYS_SOCKETVAR_H
     # include <sys/socketvar.h>
     #endif
     #ifdef HAVE_NET_ROUTE_H
     # include <net/route.h>
     #endif
     #ifdef HAVE_NETINET_IN_H
     # include <netinet/in.h>
     #endif
     #ifdef HAVE_NETINET_IN_SYSTM_H
     # include <netinet/in_systm.h>
     #endif
     #ifdef HAVE_NET_IF_H
     # include <net/if.h>
     #endif
     #ifdef HAVE_NETINET_IP_H
     # include <netinet/ip.h>
     #endif])

AC_CHECK_HEADERS([netinet/tcp_timer.h netinet/udp_var.h netinet/icmp_var.h netinet/tcp_var.h ],,,
    [#include <sys/types.h>
     #ifdef HAVE_ALIAS_H
     # include <alias.h>
     #endif
     #ifdef HAVE_SYS_SOCKET_H
     # include <sys/socket.h>
     #endif
     #ifdef HAVE_SYS_SOCKETVAR_H
     # include <sys/socketvar.h>
     #endif
     #ifdef HAVE_SYS_TIMEOUT_H
     # include <sys/timeout.h>
     #endif
     #ifdef HAVE_NETINET_IN_H
     # include <netinet/in.h>
     #endif
     #ifdef HAVE_NETINET_IN_SYSTM_H
     # include <netinet/in_systm.h>
     #endif
     #ifdef HAVE_NETINET_IP_H
     # include <netinet/ip.h>
     #endif
     #ifdef HAVE_NETINET_IP_VAR_H
     # include <netinet/ip_var.h>
     #endif
     #ifdef HAVE_NETINET_IP_ICMP_H
     # include <netinet/ip_icmp.h>
     #endif
     #ifdef HAVE_NETINET_UDP_H
     # include <netinet/udp.h>
     #endif
     #ifdef HAVE_NETINET_TCP_H
     # include <netinet/tcp.h>
     #endif
     #ifdef HAVE_NETINET_TCP_TIMER_H
     #include <netinet/tcp_timer.h>
     #endif])

AC_CHECK_HEADERS([linux/ipx.h linux/irda.h linux/rtnetlink.h],,,
    [#include <sys/types.h>
     #ifdef HAVE_ASM_TYPES_H
     # include <asm/types.h>
     #endif
     #ifdef HAVE_SYS_SOCKET_H
     # include <sys/socket.h>
     #endif
     #ifdef HAVE_LINUX_TYPES_H
     # include <linux/types.h>
     #endif])

AC_CHECK_HEADERS([mach-o/dyld_images.h],,,
    [#ifdef HAVE_STDBOOL_H
     # include <stdbool.h>
     #endif
     #ifdef HAVE_STDINT_H
     # include <stdint.h>
     #endif])

AC_CHECK_HEADERS([resolv.h],,,
    [#include <sys/types.h>
     #ifdef HAVE_SYS_SOCKET_H
     # include <sys/socket.h>
     #endif
     #ifdef HAVE_NETINET_IN_H
     # include <netinet/in.h>
     #endif
     #ifdef HAVE_ARPA_NAMESER_H
     # include <arpa/nameser.h>
     #endif])

AC_CHECK_HEADERS([ifaddrs.h],,,[#include <sys/types.h>])

AC_CHECK_HEADERS(sys/ucontext.h,,,[#include <signal.h>])

AC_CHECK_HEADERS([sys/thr.h],,,
[#include <sys/types.h>
#ifdef HAVE_SYS_UCONTEXT_H
#include <sys/ucontext.h>
#endif])

AC_CHECK_HEADERS([pthread_np.h],,,
[#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif])

AC_CHECK_HEADERS([linux/videodev2.h],,,
[#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <sys/types.h>
#ifdef HAVE_ASM_TYPES_H
#include <asm/types.h>
#endif])

AC_CHECK_HEADERS([libprocstat.h],,,
[#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif])

dnl **** Check for working dll ****

AC_SUBST(DLLFLAGS,"-D_REENTRANT")
AC_SUBST(LDRPATH_INSTALL,"")
AC_SUBST(LDRPATH_LOCAL,"")
AC_SUBST(LDDLLFLAGS,"")
AC_SUBST(LDEXECFLAGS,"")
AC_SUBST(TOP_INSTALL_LIB,"")
AC_SUBST(TOP_INSTALL_DEV,"")
AC_SUBST(WINELOADER_LDFLAGS,"")
AC_SUBST(WINEPRELOADER_LDFLAGS,"")
LIBEXT="so"
DLLEXT=".so"
WINE_PATH_SONAME_TOOLS
WINE_CONFIG_HELPERS

wine_binary="wine"
test "x$enable_win64" != "xyes" || wine_binary="wine64"
AC_SUBST(WINELOADER_PROGRAMS,"$wine_binary")
libwine_soversion=`expr $libwine_version : '\([[0-9]]*\)\..*'`

case $host_os in
  cygwin*|mingw32*)
    AC_CHECK_TOOL(DLLTOOL,dlltool,false)
    LIBEXT="dll"
    DLLEXT=""
    dnl Disable stdcall fixups to catch prototype mismatches
    case $host_cpu in
      *i[[3456]]86*) LDDLLFLAGS="-Wl,--disable-stdcall-fixup" ;;
    esac
    dnl Disable modules that can't be used on Windows
    enable_iphlpapi=${enable_iphlpapi:-no}
    enable_kernel32=${enable_kernel32:-no}
    enable_ntdll=${enable_ntdll:-no}
    enable_ws2_32=${enable_ws2_32:-no}
    enable_loader=${enable_loader:-no}
    enable_server=${enable_server:-no}
    dnl Disable dependencies that are not useful on Windows
    with_x=${with_x:-no}
    with_pthread=${with_pthread:-no}
    ;;

  darwin*|macosx*)
    AC_CHECK_HEADERS(libunwind.h)
    AC_LANG_PUSH([Objective C])
        AC_CHECK_HEADERS(Metal/Metal.h)
    AC_LANG_POP([Objective C])
    LIBEXT="dylib"
    DLLFLAGS="$DLLFLAGS -fPIC"
    LDDLLFLAGS="-fPIC"
    LDRPATH_INSTALL="-Wl,-rpath,@loader_path/\`\$(MAKEDEP) -R \${bindir} \${libdir}\`"
    LDRPATH_LOCAL="-Wl,-rpath,@loader_path/\$(top_builddir)/libs/wine"
    dnl declare needed frameworks
    AC_SUBST(COREFOUNDATION_LIBS,"-framework CoreFoundation")
    AC_SUBST(IOKIT_LIBS,"-framework IOKit -framework CoreFoundation")
    AC_SUBST(FORCEFEEDBACK_LIBS,"-framework ForceFeedback -framework CoreFoundation")
    AC_SUBST(APPLICATIONSERVICES_LIBS,"-framework ApplicationServices")
    AC_SUBST(CORESERVICES_LIBS,"-framework CoreServices")
    AC_SUBST(APPKIT_LIBS,"-framework AppKit")

    WINELOADER_LDFLAGS="-Wl,-pie,-pagezero_size,0x1000,-sectcreate,__TEXT,__info_plist,wine_info.plist"

    wine_can_build_preloader=yes
    WINEPRELOADER_LDFLAGS="-nostartfiles -nodefaultlibs -e _start -ldylib1.o -Wl,-image_base,0x7c400000,-pagezero_size,0x1000,-sectcreate,__TEXT,__info_plist,wine_info.plist"
    WINE_TRY_CFLAGS([-Wl,-no_new_main -e _main],
                    [WINEPRELOADER_LDFLAGS="-Wl,-no_new_main $WINEPRELOADER_LDFLAGS"
                     WINE_TRY_CFLAGS([-Wl,-no_new_main -e _main -nostartfiles -nodefaultlibs],,
                        [WINE_TRY_CFLAGS([-Wl,-no_new_main -e _main -mmacosx-version-min=10.7 -nostartfiles -nodefaultlibs],
                                         [WINEPRELOADER_LDFLAGS="-mmacosx-version-min=10.7 $WINEPRELOADER_LDFLAGS"],
                                         [wine_can_build_preloader=no])])],
                    [WINE_TRY_CFLAGS([-nostartfiles -nodefaultlibs],,
                        [WINE_TRY_CFLAGS([-mmacosx-version-min=10.7 -nostartfiles -nodefaultlibs],
                                         [WINEPRELOADER_LDFLAGS="-mmacosx-version-min=10.7 $WINEPRELOADER_LDFLAGS"],
                                         [wine_can_build_preloader=no])])])
    if test "$wine_can_build_preloader" = "yes"
    then
        WINE_TRY_CFLAGS([-Wl,-no_pie],
                        [WINEPRELOADER_LDFLAGS="-Wl,-no_pie $WINEPRELOADER_LDFLAGS"])
        test "$wine_binary" = wine || WINE_IGNORE_FILE("loader/wine-preloader")
        WINELOADER_PROGRAMS="$WINELOADER_PROGRAMS $wine_binary-preloader"
    else
        WINE_WARNING([can't build Wine preloader; many programs won't work])
    fi

    if test "$ac_cv_header_DiskArbitration_DiskArbitration_h" = "yes"
    then
        dnl DiskArbitration API is not public on Darwin < 8.0, use it only if header found
        AC_SUBST(DISKARBITRATION_LIBS,"-framework DiskArbitration -framework CoreFoundation")
    fi
    if test "$ac_cv_header_Security_Security_h" = "yes"
    then
        AC_SUBST(SECURITY_LIBS,"-framework Security -framework CoreFoundation")
        dnl Check for the SSLCopyPeerCertificates function
        ac_save_LIBS="$LIBS"
        LIBS="$LIBS $SECURITY_LIBS"
        AC_CHECK_FUNCS(SSLCopyPeerCertificates)
        LIBS="$ac_save_LIBS"
    fi
    if test "$ac_cv_header_CoreAudio_CoreAudio_h" = "yes" -a "$ac_cv_header_AudioUnit_AudioUnit_h" = "yes"
    then
        if test "$ac_cv_header_AudioUnit_AudioComponent_h" = "yes"
        then
            AC_SUBST(COREAUDIO_LIBS,"-framework CoreFoundation -framework CoreAudio -framework AudioUnit -framework AudioToolbox -framework CoreMIDI")
        else
            dnl CoreServices needed by AudioUnit
            AC_SUBST(COREAUDIO_LIBS,"-framework CoreAudio -framework AudioUnit -framework CoreServices -framework AudioToolbox -framework CoreMIDI")
        fi
        enable_winecoreaudio_drv=${enable_winecoreaudio_drv:-yes}
        dnl Check for the AUGraphAddNode function
        ac_save_LIBS="$LIBS"
        LIBS="$LIBS $COREAUDIO_LIBS"
        AC_CHECK_FUNCS(AUGraphAddNode)
        LIBS="$ac_save_LIBS"
    fi
    if test "$ac_cv_header_OpenAL_al_h" = "yes"
    then
        AC_SUBST(OPENAL_LIBS,"-framework OpenAL")
        AC_DEFINE_UNQUOTED(HAVE_OPENAL,1,[Define to 1 if OpenAL is available])
        ac_cv_lib_openal=yes
    fi
    if test "$ac_cv_header_OpenCL_opencl_h" = "yes"
    then
        AC_SUBST(OPENCL_LIBS,"-framework OpenCL")
        ac_cv_lib_OpenCL_clGetPlatformInfo=yes
    fi
    if test "$ac_cv_header_IOKit_hid_IOHIDLib_h" = "yes"
    then
        ac_save_LIBS="$LIBS"
        LIBS="$LIBS $IOKIT_LIBS"
        AC_CHECK_FUNCS(IOHIDManagerCreate)
        LIBS="$ac_save_LIBS"
    fi
    if test "$ac_cv_header_QuickTime_ImageCompression_h" = "yes"
    then
        dnl Make sure we can actually use the QuickTime framework
        ac_save_LIBS="$LIBS"
        LIBS="-framework QuickTime -framework ApplicationServices -framework CoreVideo $LIBS"
        AC_MSG_CHECKING([for the QuickTime framework])
        AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <QuickTime/ImageCompression.h>]],[[CodecManagerVersion(NULL);]])],
                       [AC_MSG_RESULT([yes])
                        AC_SUBST(QUICKTIME_LIBS,"-framework QuickTime -framework ApplicationServices -framework CoreVideo")
                        enable_wineqtdecoder=${enable_wineqtdecoder:-yes}],
                       [AC_MSG_RESULT([no])
                        ac_cv_header_QuickTime_ImageCompression_h=no])
        LIBS="$ac_save_LIBS"
    fi
    if test "$ac_cv_header_QuickTime_ImageCompression_h" != "yes"
    then
        WINE_NOTICE([QuickTime ${notice_platform}development files not found, video decoding won't be supported.])
    fi
    if test "$ac_cv_header_Carbon_Carbon_h" = "yes"
    then
        AC_SUBST(CARBON_LIBS,"-framework Carbon")
    fi
    if test "$ac_cv_header_Metal_Metal_h" = "yes"
    then
        AC_SUBST(METAL_LIBS,"-framework Metal -framework QuartzCore")
    fi

    dnl Enable Mac driver on Mac OS X 10.6 or later
    if test "$ac_cv_header_ApplicationServices_ApplicationServices_h" = "yes"
    then
        ac_save_LIBS="$LIBS"
        LIBS="$LIBS $APPLICATIONSERVICES_LIBS"
        AC_CHECK_FUNC(CGDisplayModeGetWidth,enable_winemac_drv=${enable_winemac_drv:-yes})
        LIBS="$ac_save_LIBS"
    fi

    dnl Check for Xcode 3.x broken 16-bit support
    if test "x$enable_win16" = "xyes"
    then
        AC_MSG_CHECKING([whether 16-bit code can be built correctly])
        AC_RUN_IFELSE([AC_LANG_PROGRAM([[asm(".text\n"
                                            "bad:\tnop;nop\n"
                                            "good:\tnop;nop\n\t"
                                            ".globl _testfunc\n"
                                            "_testfunc:\tcallw good");
                                        extern void testfunc();]],
                                      [[unsigned short *p = (unsigned short *)testfunc;
                                        return p[0] != 0xe866 || p[1] != 0xfffa]])],
                                      AC_MSG_RESULT(yes),
                                      [AC_MSG_RESULT(no)
                                       AC_MSG_ERROR([Xcode 3.x cannot build 16-bit code correctly. Use --disable-win16 if you don't need 16-bit support.])],
                                       AC_MSG_RESULT([[cross-compiling, assuming yes]]))
    fi

    AC_SUBST(LIBWINE_SHAREDLIB,"libwine.$libwine_version.dylib")
    AC_SUBST(LIBWINE_LDFLAGS,["-dynamiclib -install_name @rpath/libwine.$libwine_soversion.dylib -Wl,-rpath,@loader_path/ -compatibility_version $libwine_soversion -current_version $libwine_version"])
    AC_SUBST(WINELOADER_DEPENDS,"wine_info.plist")
    TOP_INSTALL_LIB="$TOP_INSTALL_LIB libs/wine/libwine.$libwine_version.dylib libs/wine/libwine.$libwine_soversion.dylib"
    TOP_INSTALL_DEV="$TOP_INSTALL_DEV libs/wine/libwine.dylib"
    ;;

  linux-android*)
    DLLFLAGS="$DLLFLAGS -fPIC"
    LDDLLFLAGS="-fPIC"
    LDEXECFLAGS="-Wl,-pie"
    enable_wineandroid_drv=${enable_wineandroid_drv:-yes}
    WINE_TRY_CFLAGS([-fPIC -Wl,--export-dynamic],
                    [WINELOADER_LDFLAGS="-Wl,--export-dynamic"])
    WINEPRELOADER_LDFLAGS="-static -nostartfiles -nodefaultlibs -Wl,-Ttext=0x7c400000"
    WINE_TRY_CFLAGS([-fPIC -Wl,--rpath,\$ORIGIN/../lib],
                    [LDRPATH_INSTALL="-Wl,--rpath,\\\$\$ORIGIN/\`\$(MAKEDEP) -R \${bindir} \${libdir}\`"
                     LDRPATH_LOCAL="-Wl,--rpath,\\\$\$ORIGIN/\$(top_builddir)/libs/wine"],
        [WINE_TRY_CFLAGS([-fPIC -Wl,-R,\$ORIGIN/../lib],
                         [LDRPATH_INSTALL="-Wl,-R,\\\$\$ORIGIN/\`\$(MAKEDEP) -R \${bindir} \${libdir}\`"
                          LDRPATH_LOCAL="-Wl,-R,\\\$\$ORIGIN/\$(top_builddir)/libs/wine"])])
    WINE_TRY_CFLAGS([-Wl,--enable-new-dtags],
                    [LDRPATH_INSTALL="$LDRPATH_INSTALL -Wl,--enable-new-dtags"])

    AC_SUBST(LIBWINE_SHAREDLIB,"libwine.so")
    AC_SUBST(LIBWINE_LDFLAGS,["-shared -Wl,-soname,libwine.so"])
    TOP_INSTALL_LIB="$TOP_INSTALL_LIB libs/wine/libwine.so"

    WINE_CHECK_SONAME(EGL,eglGetProcAddress)
    WINE_CHECK_SONAME(GLESv2,glFlush)

    if test "x$exec_prefix" = xNONE
    then
      case $host_cpu in
        *i[[3456]]86*) exec_prefix='${prefix}/x86' ;;
        *x86_64*)      exec_prefix='${prefix}/x86_64' ;;
        *arm*)         exec_prefix='${prefix}/armeabi-v7a' ;;
        *aarch64*)     exec_prefix='${prefix}/arm64-v8a' ;;
      esac
    fi
    ;;

  *)
    case $host_cpu in
      *i[[3456789]]86*)
        DLLFLAGS="$DLLFLAGS -fno-PIC"
        LDDLLFLAGS="-fno-PIC" ;;
      *)
        DLLFLAGS="$DLLFLAGS -fPIC"
        LDDLLFLAGS="-fPIC" ;;
    esac
    AC_CACHE_CHECK([whether we can build a GNU style ELF dll], ac_cv_c_dll_gnuelf,
        [WINE_TRY_SHLIB_FLAGS([-fPIC -shared -Wl,-Bsymbolic],
                              ac_cv_c_dll_gnuelf="yes",ac_cv_c_dll_gnuelf="no")])
    if test "$ac_cv_c_dll_gnuelf" = "yes"
    then
      LIBWINE_LDFLAGS="-shared"
      WINE_TRY_CFLAGS([-shared -Wl,-soname,confest.so.1],
                      [LIBWINE_LDFLAGS="-shared -Wl,-soname,libwine.so.$libwine_soversion"],
                      [WINE_TRY_CFLAGS([-shared -Wl,-h,confest.so.1],
                                       [LIBWINE_LDFLAGS="-shared -Wl,-h,libwine.so.$libwine_soversion"])])

      echo '{ global: *; };' >conftest.map
      WINE_TRY_CFLAGS([-shared -Wl,--version-script=conftest.map],
                      [LIBWINE_LDFLAGS="$LIBWINE_LDFLAGS -Wl,--version-script=\$(srcdir)/wine.map"])
      rm -f conftest.map

      WINE_TRY_CFLAGS([-Wl,--export-dynamic],
                      [WINELOADER_LDFLAGS="-Wl,--export-dynamic"])
      WINEPRELOADER_LDFLAGS="-static -nostartfiles -nodefaultlibs -Wl,-Ttext=0x7c400000"

      WINE_TRY_CFLAGS([-Wl,--rpath,\$ORIGIN/../lib],
                      [LDRPATH_INSTALL="-Wl,--rpath,\\\$\$ORIGIN/\`\$(MAKEDEP) -R \${bindir} \${libdir}\`"
                       LDRPATH_LOCAL="-Wl,--rpath,\\\$\$ORIGIN/\$(top_builddir)/libs/wine"],
          [WINE_TRY_CFLAGS([-Wl,-R,\$ORIGIN/../lib],
                           [LDRPATH_INSTALL="-Wl,-R,\\\$\$ORIGIN/\`\$(MAKEDEP) -R \${bindir} \${libdir}\`"
                            LDRPATH_LOCAL="-Wl,-R,\\\$\$ORIGIN/\$(top_builddir)/libs/wine"])])

      WINE_TRY_CFLAGS([-Wl,--enable-new-dtags],
                      [LDRPATH_INSTALL="$LDRPATH_INSTALL -Wl,--enable-new-dtags"])

      case $host_cpu in
        *i[[3456789]]86* | x86_64 | *aarch64*)
          WINE_TRY_CFLAGS([-Wl,-Ttext-segment=0x7bc00000],
                          [case $host_os in
                           freebsd* | kfreebsd*-gnu) WINELOADER_LDFLAGS="$WINELOADER_LDFLAGS -Wl,-Ttext-segment=0x60000000" ;;
                           *) WINELOADER_LDFLAGS="$WINELOADER_LDFLAGS -Wl,-Ttext-segment=0x7c000000" ;;
                           esac],
                          [WINE_TRY_CFLAGS([-Wl,--section-start,.interp=0x7c000400],
                                           [case $host_os in
                                              freebsd* | kfreebsd*-gnu) WINELOADER_LDFLAGS="$WINELOADER_LDFLAGS -Wl,--section-start,.interp=0x60000400" ;;
                                              *) WINELOADER_LDFLAGS="$WINELOADER_LDFLAGS -Wl,--section-start,.interp=0x7c000400" ;;
                                           esac])
                           AC_PATH_PROG(PRELINK, prelink, false, [/sbin /usr/sbin $PATH])
                           if test "x$PRELINK" = xfalse
                           then
                               WINE_WARNING([prelink not found and linker does not support relocation, base address of core dlls won't be set correctly.])
                           fi])
          WINE_TRY_CFLAGS([-Wl,-z,max-page-size=0x1000],
                          [WINELOADER_LDFLAGS="$WINELOADER_LDFLAGS -Wl,-z,max-page-size=0x1000"])
          ;;
      esac

    else
      AC_CACHE_CHECK(whether we can build a UnixWare (Solaris) dll, ac_cv_c_dll_unixware,
          [WINE_TRY_SHLIB_FLAGS([-fPIC -Wl,-G,-h,conftest.so.1.0,-B,symbolic],
                                ac_cv_c_dll_unixware="yes",ac_cv_c_dll_unixware="no")])
      if test "$ac_cv_c_dll_unixware" = "yes"
      then
        LIBWINE_LDFLAGS="-Wl,-G,-h,libwine.so.$libwine_soversion"
      fi
    fi
    AC_SUBST(LIBWINE_SHAREDLIB,"libwine.so.$libwine_version")
    AC_SUBST(LIBWINE_DEPENDS,"wine.map")
    TOP_INSTALL_LIB="$TOP_INSTALL_LIB libs/wine/libwine.so.$libwine_version libs/wine/libwine.so.$libwine_soversion"
    TOP_INSTALL_DEV="$TOP_INSTALL_DEV libs/wine/libwine.so"
    ;;
esac

enable_winecoreaudio_drv=${enable_winecoreaudio_drv:-no}
enable_wineqtdecoder=${enable_wineqtdecoder:-no}
enable_wineandroid_drv=${enable_wineandroid_drv:-no}
enable_winemac_drv=${enable_winemac_drv:-no}
test "$ac_cv_header_linux_joystick_h" = "yes" -o "$ac_cv_header_IOKit_hid_IOHIDLib_h" = "yes" || enable_winejoystick_drv=${enable_winejoystick_drv:-no}

dnl Check for cross compiler to build test programs
AC_SUBST(CROSSTARGET,"")
if test "x$with_mingw" != xno
then
    case "x$with_mingw" in
      x|xyes) WINE_CHECK_MINGW_PROG(CROSSCC,false) ;;
      *) CROSSCC=$with_mingw ;;
    esac
    if test "$CROSSCC" != "false"
    then
        ac_save_CC="$CC"
        saved_CFLAGS=$CFLAGS
        AC_SUBST(CROSSCFLAGS,${CROSSCFLAGS:-"-g -O2"})
        AC_SUBST(EXTRACROSSCFLAGS,"-DWINE_CROSS_PE -Wall")

        CC="$CROSSCC"
        CFLAGS=$CROSSCFLAGS
        AC_CACHE_CHECK([whether $CROSSCC works], wine_cv_crosscc,
            [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])],[wine_cv_crosscc=yes],[wine_cv_crosscc=no])])
        if test $wine_cv_crosscc = yes
        then
            set x $CROSSCC
            shift
            while test $# -ge 1
            do
                case "$1" in
                    *-gcc) CROSSTARGET=`expr "$1" : '\(.*\)-gcc'` ;;
                    *-clang) CROSSTARGET=`expr "$1" : '\(.*\)-clang'` ;;
                esac
                shift
            done
        fi
        WINE_TRY_CROSSCFLAGS([-fno-strict-aliasing])
        dnl clang needs to be told to fail on unknown options
        WINE_TRY_CROSSCFLAGS([-Werror=unknown-warning-option],[CFLAGS="$CFLAGS -Werror=unknown-warning-option"])
        WINE_TRY_CROSSCFLAGS([-Wdeclaration-after-statement])
        WINE_TRY_CROSSCFLAGS([-Wempty-body])
        WINE_TRY_CROSSCFLAGS([-Wignored-qualifiers])
        WINE_TRY_CROSSCFLAGS([-Wpacked-not-aligned],[EXTRACROSSCFLAGS="$EXTRACROSSCFLAGS -Wno-packed-not-aligned"])
        WINE_TRY_CROSSCFLAGS([-Wpragma-pack],[EXTRACROSSCFLAGS="$EXTRACROSSCFLAGS -Wno-pragma-pack"])
        WINE_TRY_CROSSCFLAGS([-Wshift-overflow=2])
        WINE_TRY_CROSSCFLAGS([-Wstrict-prototypes])
        WINE_TRY_CROSSCFLAGS([-Wtype-limits])
        WINE_TRY_CROSSCFLAGS([-Wunused-but-set-parameter])
        WINE_TRY_CROSSCFLAGS([-Wvla])
        WINE_TRY_CROSSCFLAGS([-Wwrite-strings])
        WINE_TRY_CROSSCFLAGS([-Wpointer-arith])
        WINE_TRY_CROSSCFLAGS([-Wlogical-op])

        case $host_cpu in
          *i[[3456789]]86*) WINE_TRY_CROSSCFLAGS([-fno-omit-frame-pointer]) ;;
          x86_64) WINE_TRY_CROSSCFLAGS([-Wno-format]) ;;
        esac

        dnl Default to dwarf-2 debug info
        for ac_flag in $CROSSCFLAGS; do
          case $ac_flag in
            -g) WINE_TRY_CROSSCFLAGS([-gdwarf-2])
                WINE_TRY_CROSSCFLAGS([-gstrict-dwarf]) ;;
          esac
        done

        if test "x$enable_maintainer_mode" = xyes
        then
            WINE_TRY_CROSSCFLAGS([-Werror])
        fi

        CFLAGS=$saved_CFLAGS
        CC="$ac_save_CC"
    fi
fi
WINE_NOTICE_WITH(mingw,[test "x$CROSSTARGET" = "x"],
                 [MinGW compiler not found, cross-compiling PE files won't be supported.])

dnl **** Check for pthread ****

if test "$ac_cv_header_pthread_h" = "yes"
then
    AC_CHECK_FUNC(pthread_create,,[AC_CHECK_LIB(pthread,pthread_create,[AC_SUBST(PTHREAD_LIBS,"-lpthread")])])
fi
WINE_ERROR_WITH(pthread,[test "x$ac_cv_func_pthread_create" != xyes -a "x$PTHREAD_LIBS" = x],
[pthread ${notice_platform}development files not found.
Wine cannot support threads without libpthread.])

dnl **** Check for X11 ****

AC_PATH_XTRA

if test "$have_x" = "yes"
then
    ac_save_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $X_CFLAGS"

    WINE_CHECK_SONAME(X11,XCreateWindow,,,[$X_LIBS $X_EXTRA_LIBS])
    WINE_CHECK_SONAME(Xext,XextCreateExtension,[X_LIBS="$X_LIBS -lXext"],,[$X_LIBS -lX11 $X_EXTRA_LIBS])
    X_LIBS="$X_LIBS -lX11"

    dnl *** All of the following tests require X11/Xlib.h
    AC_CHECK_HEADERS([X11/Xlib.h \
                      X11/XKBlib.h \
                      X11/Xutil.h \
                      X11/Xcursor/Xcursor.h \
                      X11/extensions/shape.h \
                      X11/extensions/XInput.h \
                      X11/extensions/XInput2.h \
                      X11/extensions/XShm.h \
                      X11/extensions/Xcomposite.h \
                      X11/extensions/Xfixes.h \
                      X11/extensions/Xinerama.h \
                      X11/extensions/Xrandr.h \
                      X11/extensions/Xrender.h \
                      X11/extensions/xf86vmode.h \
                      X11/extensions/xf86vmproto.h],,,
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
              AC_DEFINE(HAVE_XKB, 1, [Define if you have the XKB extension]),,[$X_LIBS $X_EXTRA_LIBS])
        fi

        dnl *** Check for X cursor
        if test "$ac_cv_header_X11_Xcursor_Xcursor_h" = "yes"
        then
            WINE_CHECK_SONAME(Xcursor,XcursorImageLoadCursor,,,[$X_LIBS $X_EXTRA_LIBS])
        fi
        WINE_NOTICE_WITH(xcursor,[test "x$ac_cv_lib_soname_Xcursor" = "x"],
                         [libxcursor ${notice_platform}development files not found, the Xcursor extension won't be supported.])

        dnl *** Check for X input extension
        if test "$ac_cv_header_X11_extensions_XInput_h" = "yes"
        then
            WINE_CHECK_SONAME(Xi,XOpenDevice,,,[$X_LIBS $X_EXTRA_LIBS])
        fi
        WINE_NOTICE_WITH(xinput,[test "x$ac_cv_lib_soname_Xi" = "x"],
                         [libxi ${notice_platform}development files not found, the Xinput extension won't be supported.])

        dnl *** Check for X input 2 extension
        if test "x$ac_cv_lib_soname_Xi" != x
        then
            WINE_NOTICE_WITH(xinput2,[test "$ac_cv_header_X11_extensions_XInput2_h" != "yes"],
                             [XInput2 headers not found, the XInput 2 extension won't be supported.])
        fi

        dnl *** Check for X Shm extension
        if test "$ac_cv_header_X11_extensions_XShm_h" = "yes"
        then
              AC_CHECK_LIB(Xext, XShmQueryExtension,
              AC_DEFINE(HAVE_LIBXXSHM, 1, [Define if you have the X Shm extension]),,[$X_LIBS $X_EXTRA_LIBS])
        fi
        WINE_NOTICE_WITH(xshm,[test "$ac_cv_lib_Xext_XShmQueryExtension" != "yes"],
                         [XShm ${notice_platform}development files not found, X Shared Memory won't be supported.])

        dnl *** Check for X shape extension
        if test "$ac_cv_header_X11_extensions_shape_h" = "yes"
        then
              AC_CHECK_LIB(Xext,XShapeQueryExtension,
	      AC_DEFINE(HAVE_LIBXSHAPE, 1, [Define if you have the X Shape extension]),,[$X_LIBS $X_EXTRA_LIBS])
        fi
        WINE_NOTICE_WITH(xshape,[test "$ac_cv_lib_Xext_XShapeQueryExtension" != "yes"],
                         [XShape ${notice_platform}development files not found, XShape won't be supported.])

        dnl *** Check for XFree86 VMODE extension
        if test "$ac_cv_header_X11_extensions_xf86vmode_h" = "yes" -o "$ac_cv_header_X11_extensions_xf86vmproto_h" = "yes"
        then
            WINE_CHECK_SONAME(Xxf86vm,XF86VidModeQueryExtension,,,[$X_LIBS $X_EXTRA_LIBS])
        fi
        WINE_NOTICE_WITH(xxf86vm,[test "x$ac_cv_lib_soname_Xxf86vm" = "x"],
                         [libXxf86vm ${notice_platform}development files not found, XFree86 Vidmode won't be supported.])

        dnl *** Check for Transform functions in Xrender
        if test "$ac_cv_header_X11_extensions_Xrender_h" = "yes" -a "x$ac_cv_lib_soname_X11" != "x"
        then
            WINE_CHECK_SONAME(Xrender,XRenderQueryExtension,
              [AC_CHECK_LIB(Xrender,XRenderSetPictureTransform,
                [AC_DEFINE(HAVE_XRENDERSETPICTURETRANSFORM, 1,
                 [Define if Xrender has the XRenderSetPictureTransform function])],,[$X_LIBS $X_EXTRA_LIBS])
               AC_CHECK_LIB(Xrender,XRenderCreateLinearGradient,
                [AC_DEFINE(HAVE_XRENDERCREATELINEARGRADIENT, 1,
                 [Define if Xrender has the XRenderCreateLinearGradient function])],,[$X_LIBS $X_EXTRA_LIBS])],,[$X_LIBS $X_EXTRA_LIBS])

        fi
        WINE_WARNING_WITH(xrender,[test "x$ac_cv_lib_soname_Xrender" = "x"],
                         [libxrender ${notice_platform}development files not found, XRender won't be supported.])

        dnl *** Check for X RandR extension
        if test "$ac_cv_header_X11_extensions_Xrandr_h" = "yes" -a "x$ac_cv_lib_soname_Xrender" != "x"
        then
                AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>]], [[static typeof(XRRSetScreenConfigAndRate) * func; if (func) return 0;]])],
                  [WINE_CHECK_SONAME(Xrandr,XRRQueryExtension,
                    [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>]], [[static typeof(XRRGetScreenResources) *f; if (f) return 0;]])],
                      [AC_DEFINE(HAVE_XRRGETSCREENRESOURCES, 1,
                        [Define if Xrandr has the XRRGetScreenResources function])])],,[$X_LIBS $X_EXTRA_LIBS])])
        fi
        WINE_NOTICE_WITH(xrandr,[test "x$ac_cv_lib_soname_Xrandr" = "x"],
                         [libxrandr ${notice_platform}development files not found, XRandr won't be supported.])

        dnl *** Check for Xfixes extension
        if test "$ac_cv_header_X11_extensions_Xfixes_h" = "yes"
        then
                AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>]], [[static typeof(XFixesQueryVersion) * func; if (func) return 0;]])],
                  [WINE_CHECK_SONAME(Xfixes,XFixesQueryVersion,,,[$X_LIBS $X_EXTRA_LIBS])])
        fi
        WINE_NOTICE_WITH(xfixes,[test "x$ac_cv_lib_soname_Xfixes" = "x"],
                         [libxfixes ${notice_platform}development files not found, Xfixes won't be supported.])

        dnl *** Check for Xinerama extension
        if test "$ac_cv_header_X11_extensions_Xinerama_h" = "yes"
        then
                AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>]], [[static typeof(XineramaQueryScreens) * func; if (func) return 0;]])],
                  [WINE_CHECK_SONAME(Xinerama,XineramaQueryScreens,,,[$X_LIBS $X_EXTRA_LIBS])])
        fi
        WINE_NOTICE_WITH(xinerama,[test "x$ac_cv_lib_soname_Xinerama" = "x"],
                         [libxinerama ${notice_platform}development files not found, multi-monitor setups won't be supported.])

        dnl *** Check for X Composite extension
        if test "$ac_cv_header_X11_extensions_Xcomposite_h" = "yes"
        then
            WINE_CHECK_SONAME(Xcomposite,XCompositeRedirectWindow,,,[$X_LIBS $X_EXTRA_LIBS])
        fi
        WINE_NOTICE_WITH(xcomposite,[test "x$ac_cv_lib_soname_Xcomposite" = "x"],
                         [libxcomposite ${notice_platform}development files not found, Xcomposite won't be supported.])

        dnl *** Check for XICCallback struct
        AC_CHECK_MEMBERS([XICCallback.callback, XEvent.xcookie],,,
[#ifdef HAVE_X11_XLIB_H
#include <X11/Xlib.h>
#endif])

    dnl *** End of X11/Xlib.h check

    dnl Check for the presence of OpenGL
    opengl_msg=""
    if test "x$with_opengl" != "xno"
    then
	WINE_CHECK_SONAME(GL,glXCreateContext,
		     [OPENGL_LIBS="-lGL"],
                     [WINE_CHECK_SONAME(GL,glXCreateContext,
			    [OPENGL_LIBS="-Xlinker -dylib_file -Xlinker /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib -lGL"],
			    [if test -f /usr/X11R6/lib/libGL.a
                            then
	                        opengl_msg="/usr/X11R6/lib/libGL.a is present on your system.
This probably prevents linking to OpenGL. Try deleting the file and restarting configure."
                            else
                                opengl_msg="No OpenGL library found on this system."
                            fi],
			    $X_LIBS -lm $X_EXTRA_LIBS -dylib_file /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib)],
		     $X_LIBS -lm $X_EXTRA_LIBS)
        if test "x$with_glu" != "xno"
        then
            WINE_CHECK_SONAME(GLU,gluLookAt,,,[$OPENGL_LIBS $X_LIBS -lm $X_EXTRA_LIBS])
            WINE_NOTICE_WITH(glu,[test "x$ac_cv_lib_soname_GLU" = "x"],
                             [libGLU ${notice_platform}development files not found, GLU won't be supported.])
        fi

        if test "x$with_osmesa" != "xno"
        then
            WINE_CHECK_SONAME(OSMesa,OSMesaGetProcAddress,,,[$X_LIBS -lm $X_EXTRA_LIBS])
            WINE_NOTICE_WITH(osmesa,[test "x$ac_cv_lib_soname_OSMesa" = "x"],
                             [libOSMesa ${notice_platform}development files not found (or too old), OpenGL rendering in bitmaps won't be supported.])
        fi
    fi
    WINE_WARNING_WITH(opengl,[test -n "$opengl_msg"],[$opengl_msg
OpenGL and Direct3D won't be supported.])

    CPPFLAGS="$ac_save_CPPFLAGS"
else
    X_CFLAGS=""
    X_LIBS=""
fi

if test "$enable_wineandroid_drv$enable_winemac_drv" = "nono"
then
    WINE_ERROR_WITH(x,[test "x$X_LIBS" = "x"],
[X ${notice_platform}development files not found. Wine will be built
without X support, which probably isn't what you want. You will need
to install ${notice_platform}development packages of Xlib at the very least.],
                    [enable_winex11_drv])
else
    WINE_NOTICE_WITH(x,[test "x$X_LIBS" = "x"],
                     [X ${notice_platform}development files not found, the X11 driver won't be supported.],
                     [enable_winex11_drv])
fi

dnl **** Check for OpenCL ****
if test "$ac_cv_header_CL_cl_h" = "yes"
then
    AC_CHECK_LIB(OpenCL,clGetPlatformInfo,[AC_SUBST(OPENCL_LIBS,["-lOpenCL"])])
fi
WINE_NOTICE_WITH(opencl,[test "x$ac_cv_lib_OpenCL_clGetPlatformInfo" != xyes],
                 [OpenCL ${notice_platform}development files not found, OpenCL won't be supported.],
                 [enable_opencl])

dnl **** Check for libpcap ****
if test "$ac_cv_header_pcap_pcap_h" = "yes"
then
    AC_CHECK_LIB(pcap,pcap_create,[AC_SUBST(PCAP_LIBS,["-lpcap"])])
fi
WINE_NOTICE_WITH(pcap,[test "x$ac_cv_lib_pcap_pcap_create" != xyes],
                 [pcap ${notice_platform}development files not found, wpcap won't be supported.],
                 [enable_wpcap])

dnl **** Check for libxml2 ****

if test "x$with_xml" != "xno"
then
    WINE_PACKAGE_FLAGS(XML2,[libxml-2.0],[-lxml2],
                       [`${XML2_CONFIG:-xml2-config} --cflags 2>/dev/null`],[`${XML2_CONFIG:-xml2-config} --libs 2>/dev/null`],
        [AC_CHECK_HEADERS([libxml/parser.h libxml/xmlsave.h libxml/SAX2.h])
        if test "$ac_cv_header_libxml_parser_h" = "yes" -a "$ac_cv_header_libxml_xmlsave_h" = "yes" -a "$ac_cv_header_libxml_SAX2_h" = "yes"
        then
            AC_CHECK_LIB(xml2, xmlParseMemory,
                [AC_DEFINE(HAVE_LIBXML2, 1, [Define if you have the libxml2 library])],[XML2_LIBS=""],[$XML2_LIBS])
            AC_CHECK_LIB(xml2, xmlReadMemory,
                [AC_DEFINE(HAVE_XMLREADMEMORY,1,[Define if libxml2 has the xmlReadMemory function])],,[$XML2_LIBS])
            AC_CHECK_LIB(xml2, xmlNewDocPI,
                [AC_DEFINE(HAVE_XMLNEWDOCPI,1,[Define if libxml2 has the xmlNewDocPI function])],,[$XML2_LIBS])
            AC_CHECK_LIB(xml2, xmlSchemaSetParserStructuredErrors,
                [AC_DEFINE(HAVE_XMLSCHEMASSETPARSERSTRUCTUREDERRORS,1,[Define if libxml2 has the xmlSchemaSetParserStructuredErrors function])],,[$XML2_LIBS])
            AC_CHECK_LIB(xml2, xmlSchemaSetValidStructuredErrors,
                [AC_DEFINE(HAVE_XMLSCHEMASSETVALIDSTRUCTUREDERRORS,1,[Define if libxml2 has the xmlSchemaSetValidStructuredErrors function])],,[$XML2_LIBS])
            AC_CHECK_LIB(xml2, xmlFirstElementChild,
                [AC_DEFINE(HAVE_XMLFIRSTELEMENTCHILD,1,[Define if libxml2 has the xmlFirstElementChild function])],,[$XML2_LIBS])
            AC_CHECK_TYPE([xmlDocProperties],
                [AC_DEFINE(HAVE_XMLDOC_PROPERTIES,1,[Define if libxml2 has the xmlDocProperties enum])],,[[#include <libxml/tree.h>]])
        else
            XML2_CFLAGS=""
            XML2_LIBS=""
        fi])
fi
WINE_WARNING_WITH(xml,[test "$ac_cv_lib_xml2_xmlParseMemory" != "yes"],
                 [libxml2 ${notice_platform}development files not found (or too old), XML won't be supported.])

if test "x$with_xslt" != "xno"
then
    WINE_PACKAGE_FLAGS(XSLT,[libxslt],[-lxml2],
                       [`${XSLT_CONFIG:-xslt-config} --cflags 2>/dev/null`],[`${XSLT_CONFIG:-xslt-config} --libs 2>/dev/null`],
        [AC_CHECK_HEADERS([libxslt/pattern.h libxslt/transform.h],,,
        [#ifdef HAVE_LIBXSLT_PATTERN_H
# include <libxslt/pattern.h>
#endif])
        if test "$ac_cv_header_libxslt_transform_h" = "yes"
        then
            WINE_CHECK_SONAME(xslt,xsltCompilePattern,,,[$XSLT_LIBS])
        else
            XSLT_CFLAGS=""
        fi])
fi
WINE_WARNING_WITH(xslt,[test "x$ac_cv_lib_soname_xslt" = "x"],
                 [libxslt ${notice_platform}development files not found, xslt won't be supported.])

dnl **** Check for inotify ****
if test "x$with_inotify" != "xno"
then
    WINE_PACKAGE_FLAGS(INOTIFY,[libinotify],,,,
        [AC_CHECK_HEADERS([sys/inotify.h])
         AC_CHECK_LIB(inotify,inotify_add_watch,[:],[INOTIFY_LIBS=""],[$INOTIFY_LIBS])])
fi
WINE_NOTICE_WITH(inotify,[test "$ac_cv_header_sys_inotify_h" != "yes"],
                 [libinotify ${notice_platform}development files not found (or too old), filesystem change notifications won't be supported.])

dnl **** Check for libdbus ****
if test "x$with_dbus" != "xno"
then
    WINE_PACKAGE_FLAGS(DBUS,[dbus-1],,,,
        [AC_CHECK_HEADER([dbus/dbus.h],
            [WINE_CHECK_SONAME(dbus-1, dbus_connection_close,,[DBUS_CFLAGS=""],[$DBUS_LIBS])],
            [DBUS_CFLAGS=""])])
fi
WINE_NOTICE_WITH(dbus,[test "x$ac_cv_lib_soname_dbus_1" = "x" -a \
                            "x$ac_cv_header_DiskArbitration_DiskArbitration_h" != "xyes"],
                 [libdbus ${notice_platform}development files not found, no dynamic device support.])

dnl **** Check for libhal ****
if test "x$with_hal" != "xno" -a "x$ac_cv_lib_soname_dbus_1" != x
then
    WINE_PACKAGE_FLAGS(HAL,[hal],[-ldbus-1],,,
        [AC_CHECK_HEADER([hal/libhal.h],
            [WINE_CHECK_SONAME(hal, libhal_ctx_new,,[HAL_CFLAGS=""],[$HAL_LIBS])],
            [HAL_CFLAGS=""])])
    WINE_NOTICE_WITH(hal,[test "x$ac_cv_lib_soname_hal" = "x" -a \
                               "x$ac_cv_header_DiskArbitration_DiskArbitration_h" != "xyes"],
                     [libhal ${notice_platform}development files not found, no legacy dynamic device support.])
fi

dnl **** Check for libgnutls ****
if test "x$with_gnutls" != "xno"
then
    WINE_PACKAGE_FLAGS(GNUTLS,[gnutls],,,,
        [AC_CHECK_HEADER(gnutls/gnutls.h,
            [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>]], [[static typeof(gnutls_mac_get_key_size) *func; if (func) return 0;]])],
                [WINE_CHECK_SONAME(gnutls,gnutls_global_init,,[GNUTLS_CFLAGS=""],[$GNUTLS_LIBS],[[libgnutls\\(-deb0\\)\\{0,1\\}]])
                 WINE_CHECK_LIB_FUNCS(gnutls_cipher_init,[$GNUTLS_LIBS],,
                                      [WINE_NOTICE([libgnutls ${notice_platform}development files too old, bcrypt encryption won't be supported.])])])],
            [GNUTLS_CFLAGS=""])])
fi
WINE_WARNING_WITH(gnutls,[test "x$ac_cv_lib_soname_gnutls" = "x"],
                 [libgnutls ${notice_platform}development files not found, no schannel support.])

dnl **** Check which curses lib to use ***
CURSES_LIBS=""
if test "$ac_cv_header_ncurses_h" = "yes"
then
    WINE_CHECK_SONAME(ncurses,waddch,[CURSES_LIBS="-lncurses"],,,[[libncursesw\\{0,1\\}]])
elif test "$ac_cv_header_curses_h" = "yes"
then
    WINE_CHECK_SONAME(curses,waddch,[CURSES_LIBS="-lcurses"])
fi
ac_save_LIBS="$LIBS"
LIBS="$LIBS $CURSES_LIBS"
AC_CHECK_FUNCS(mousemask)
LIBS="$ac_save_LIBS"
WINE_NOTICE_WITH(curses,[test "x$ac_cv_lib_soname_curses$ac_cv_lib_soname_ncurses" = "x"],
                 [lib(n)curses ${notice_platform}development files not found, curses won't be supported.])

dnl **** Check for SANE ****
if test "x$with_sane" != "xno"
then
    WINE_PACKAGE_FLAGS(SANE,[libsane],,[`${SANE_CONFIG:-sane-config} --cflags 2>/dev/null`],[`${SANE_CONFIG:-sane-config} --ldflags 2>/dev/null`],
        [AC_CHECK_HEADER(sane/sane.h,
            [WINE_CHECK_SONAME(sane,sane_init,,[SANE_CFLAGS=""],[$SANE_LIBS])],
            [SANE_CFLAGS=""])])
fi
WINE_NOTICE_WITH(sane,[test "x$ac_cv_lib_soname_sane" = "x"],
                 [libsane ${notice_platform}development files not found, scanners won't be supported.])

dnl **** Check for libv4l2 ****
if test "x$with_v4l2" != "xno"
then
    WINE_CHECK_SONAME(v4l2,v4l2_open)
fi
WINE_NOTICE_WITH(v4l2,[test "x$ac_cv_lib_soname_v4l2" = "x"],
                 [libv4l2 ${notice_platform}development files not found.])

dnl **** Check for libgphoto2 ****
if test "x$with_gphoto" != "xno"
then
    WINE_PACKAGE_FLAGS(GPHOTO2,[libgphoto2],[-lgphoto2],
                       [`${GPHOTO2_CONFIG:-gphoto2-config} --cflags 2>/dev/null`],[`${GPHOTO2_CONFIG:-gphoto2-config} --libs 2>/dev/null`],
        [AC_CHECK_HEADER(gphoto2-camera.h,
                        [AC_CHECK_LIB(gphoto2,gp_camera_new,
                                      [AC_DEFINE(HAVE_GPHOTO2, 1, [Define if we have the libgphoto2 development environment])],
                                      [GPHOTO2_LIBS=""; GPHOTO2_CFLAGS=""],
                                      [$GPHOTO2_LIBS])],
                        [GPHOTO2_LIBS=""; GPHOTO2_CFLAGS=""])])
    WINE_PACKAGE_FLAGS(GPHOTO2_PORT,[libgphoto2_port],[-lgphoto2_port],
                       [`${GPHOTO2_PORT_CONFIG:-gphoto2-port-config} --cflags 2>/dev/null`],
                       [`${GPHOTO2_PORT_CONFIG:-gphoto2-port-config} --libs 2>/dev/null`],
        [AC_CHECK_HEADER(gphoto2-port.h,
                        [AC_CHECK_LIB(gphoto2_port,gp_port_info_list_new,
                                      [AC_DEFINE(HAVE_GPHOTO2_PORT, 1, [Define if we have the libgphoto2_port development environment])],
                                      [GPHOTO2_PORT_LIBS=""; GPHOTO2_PORT_CFLAGS=""],
                                      [$GPHOTO2_PORT_LIBS])],
                        [GPHOTO2_PORT_LIBS=""; GPHOTO2_PORT_CFLAGS=""])])
fi
WINE_NOTICE_WITH(gphoto,[test "$ac_cv_lib_gphoto2_gp_camera_new" != "yes"],
                 [libgphoto2 ${notice_platform}development files not found, digital cameras won't be supported.])
WINE_NOTICE_WITH(gphoto,[test "$ac_cv_lib_gphoto2_port_gp_port_info_list_new" != "yes"],
                 [libgphoto2_port ${notice_platform}development files not found, digital cameras won't be auto-detected.])


dnl **** Check for resolver library ***
if test "$ac_cv_header_resolv_h" = "yes"
then
    AC_CACHE_CHECK([for resolver library], ac_cv_have_resolv,
      [ac_save_LIBS="$LIBS"
       for lib in '' -lresolv
       do
         LIBS="$lib $ac_save_LIBS"
         AC_LINK_IFELSE([AC_LANG_PROGRAM([[#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#include <resolv.h>]],[[if (!(_res.options & RES_INIT)) res_init(); res_query("foo",ns_c_in,0,0,0)]])],
           [ac_cv_have_resolv=${lib:-"none required"}],[ac_cv_have_resolv="not found"])
         test "x$ac_cv_have_resolv" = "xnot found" || break
       done
       LIBS="$ac_save_LIBS"])

    case "$ac_cv_have_resolv" in
      "not found") ;;
      "none required")
        AC_DEFINE(HAVE_RESOLV, 1, [Define if you have the resolver library and header]) ;;
      *)
        AC_DEFINE(HAVE_RESOLV, 1)
        AC_SUBST(RESOLV_LIBS,$ac_cv_have_resolv) ;;
    esac
fi

dnl **** Check for LittleCMS ***
if test "x$with_cms" != "xno"
then
    WINE_PACKAGE_FLAGS(LCMS2,[lcms2],[-llcms2],,,
        [AC_CHECK_HEADERS([lcms2.h])
        if test "$ac_cv_header_lcms2_h" = "yes"
        then
            AC_CHECK_LIB(lcms2, cmsOpenProfileFromFile,
                [AC_DEFINE(HAVE_LCMS2, 1, [Define if you have the LittleCMS development environment])],[LCMS2_LIBS=""],[$LCMS2_LIBS])
        else
            LCMS2_CFLAGS=""
            LCMS2_LIBS=""
        fi])
fi
WINE_NOTICE_WITH(cms,[test "$ac_cv_lib_lcms2_cmsOpenProfileFromFile" != "yes"],
                 [liblcms2 ${notice_platform}development files not found, Color Management won't be supported.])

dnl **** Check for FreeType 2 ****
if test "x$with_freetype" != "xno"
then
    WINE_PACKAGE_FLAGS(FREETYPE,[freetype2],[-lfreetype],
                       [`(${FREETYPE_CONFIG:-freetype-config} --cflags || ${FREETYPE2_CONFIG:-freetype2-config} --cflags) 2>/dev/null`],
                       [`(${FREETYPE_CONFIG:-freetype-config} --libs || ${FREETYPE2_CONFIG:-freetype2-config} --libs) 2>/dev/null`],
        [AC_CHECK_HEADERS([ft2build.h])
        if test "$ac_cv_header_ft2build_h" = "yes"
        then
            WINE_CHECK_SONAME(freetype,FT_Init_FreeType,
                [AC_DEFINE(HAVE_FREETYPE, 1, [Define if FreeType 2 is installed])
                 AC_CHECK_TYPES(FT_TrueTypeEngineType,,,[#include <ft2build.h>
#include FT_MODULE_H])],
                [FREETYPE_LIBS=""],[$FREETYPE_LIBS])
        else
            FREETYPE_CFLAGS=""
            FREETYPE_LIBS=""
        fi])
fi
WINE_ERROR_WITH(freetype,[test "x$ac_cv_lib_soname_freetype" = x],
                [FreeType ${notice_platform}development files not found. Fonts will not be built.],
                [enable_fonts])

dnl **** Check for parport (currently Linux only) ****
AC_CACHE_CHECK([for parport header/ppdev.h], ac_cv_c_ppdev,
 AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#include <linux/ppdev.h>]], [[ioctl (1,PPCLAIM,0)]])],
    [ac_cv_c_ppdev="yes"],[ac_cv_c_ppdev="no"]))
if test "$ac_cv_c_ppdev" = "yes"
then
    AC_DEFINE(HAVE_PPDEV, 1, [Define if we can use ppdev.h for parallel port access])
fi

dnl **** Check for pthread functions ****
WINE_CHECK_LIB_FUNCS(\
        pthread_getthreadid_np,
        [$PTHREAD_LIBS])

dnl **** Check for zlib ****
if test "x$with_zlib" != "xno"
then
    WINE_PACKAGE_FLAGS(Z,[libz],[-lz],,,
        [AC_CHECK_HEADERS(zlib.h,
            [AC_CHECK_LIB(z,inflate,
                          [AC_DEFINE(HAVE_ZLIB,1,[Define to 1 if you have the `z' library (-lz).])],
                          [Z_LIBS=""],[$Z_LIBS])],
            [Z_LIBS=""])])
fi
WINE_NOTICE_WITH(zlib,[test "x$Z_LIBS" = "x"],[libz ${notice_platform}development files not found, data compression won't be supported.])

dnl **** Check for gettextpo ****
if test "x$enable_tools" != xno -a "x$with_gettextpo" = xyes
then
    if test "$ac_cv_header_gettext_po_h" = "yes"
    then
        AC_CHECK_LIB(gettextpo,po_message_msgctxt,
                     [AC_DEFINE(HAVE_LIBGETTEXTPO,1,[Define to 1 if you have the `gettextpo' library (-lgettextpo).])
                      AC_SUBST(GETTEXTPO_LIBS,"-lgettextpo")])
    fi
    WINE_NOTICE_WITH(gettextpo,[test "x$GETTEXTPO_LIBS" = "x"],
      [GetText ${notice_platform}development files not found (or too old), po files can't be rebuilt.])
    WINE_NOTICE_WITH(gettextpo,[test "$srcdir" != .],
      [Rebuilding po files is not supported for out of tree builds.])
fi

dnl **** Check for PulseAudio ****
if test "x$with_pulse" != "xno";
then
    WINE_PACKAGE_FLAGS(PULSE,[libpulse],,,,
        [AC_CHECK_HEADERS(pulse/pulseaudio.h,
            [AC_CHECK_LIB(pulse, pa_stream_is_corked,[:],[PULSE_LIBS=""],[$PULSE_LIBS])],
            [PULSE_LIBS=""])])
fi
WINE_NOTICE_WITH(pulse, [test -z "$PULSE_LIBS"],
        [libpulse ${notice_platform}development files not found or too old, Pulse won't be supported.],
        [enable_winepulse_drv])

dnl **** Check for gstreamer ****
if test "x$with_gstreamer" != "xno"
then
    WINE_PACKAGE_FLAGS(GSTREAMER,[gstreamer-1.0 gstreamer-video-1.0 gstreamer-audio-1.0],,,,
        [ac_gst_incl=""
        for i in $GSTREAMER_CFLAGS
        do
            case "$i" in
                -I*) ac_gst_incl="$ac_gst_incl $i";;
            esac
        done
        GSTREAMER_CFLAGS=$ac_gst_incl
        CPPFLAGS="$ac_save_CPPFLAGS $GSTREAMER_CFLAGS"
        AC_CHECK_HEADER([gst/gst.h],
            [AC_MSG_CHECKING([whether gint64 defined by gst/gst.h is indeed 64-bit])
              AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <gst/gst.h>]],
                                                 [[static int a[sizeof(gint64) > 4 ? 1 : -1]; if (a[0]) return 0;]])],
                [AC_MSG_RESULT([yes])
                 AC_CHECK_LIB(gstreamer-1.0,gst_pad_new,[:],,[$GSTREAMER_LIBS])],
                [AC_MSG_RESULT([no])
                 ac_glib2_broken=yes
                 WINE_NOTICE([glib-2.0 pkgconfig configuration is for the wrong architecture, winegstreamer won't be built.])])])])
fi
WINE_NOTICE_WITH(gstreamer,[test "x$ac_glib2_broken" != xyes -a "x$ac_cv_lib_gstreamer_1_0_gst_pad_new" != xyes -a "x$ac_cv_header_QuickTime_ImageCompression_h" != xyes ],
                 [gstreamer-1.0 base plugins ${notice_platform}development files not found, GStreamer won't be supported.])
test "x$ac_cv_lib_gstreamer_1_0_gst_pad_new" = xyes || enable_winegstreamer=${enable_winegstreamer:-no}

dnl **** Check for ALSA 1.x ****
AC_SUBST(ALSA_LIBS,"")
if test "x$with_alsa" != "xno"
then
    AC_CHECK_LIB(asound,snd_pcm_hw_params_get_access_mask,
        [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <alsa/asoundlib.h>]], [[snd_pcm_hw_params_get_access_mask(NULL, NULL)]])],
                        [ALSA_LIBS="-lasound"])])
fi
test -n "$ALSA_LIBS" || enable_winealsa_drv=${enable_winealsa_drv:-no}

dnl **** Check for OSSv4 ****
if test "x$with_oss" != xno
then
    ac_save_CPPFLAGS="$CPPFLAGS"
    if test -f /etc/oss.conf
    then
        . /etc/oss.conf
    fi
    ac_oss_incl="-I${OSSLIBDIR:-/usr/lib/oss}/include"
    CPPFLAGS="$CPPFLAGS $ac_oss_incl"
    AC_CHECK_HEADER([sys/soundcard.h],
        [AC_CHECK_MEMBERS([oss_sysinfo.numaudioengines],
                          [AC_SUBST(OSS4_CFLAGS,"$ac_oss_incl")],,
                          [#include <sys/soundcard.h>])])
    CPPFLAGS="$ac_save_CPPFLAGS"
fi
WINE_NOTICE_WITH(oss,[test "x$ac_cv_member_oss_sysinfo_numaudioengines" != xyes],
                 [OSS sound system found but too old (OSSv4 needed), OSS won't be supported.],
                 [enable_wineoss_drv])

dnl **** Check for libudev ****
if test "x$with_udev" != "xno"
then
    WINE_PACKAGE_FLAGS(UDEV,[libudev],[-ludev],,,
        [AC_CHECK_HEADERS(libudev.h,
            [AC_CHECK_LIB(udev,udev_new,
                AC_DEFINE(HAVE_UDEV,1,[Define to 1 if you have the `udev' library (-ludev).]),
                [UDEV_LIBS=""],[$UDEV_LIBS])],
            [UDEV_LIBS=""])])
fi
WINE_NOTICE_WITH(udev,[test "x$UDEV_LIBS" = "x"],
                 [libudev ${notice_platform}development files not found, plug and play won't be supported.])

dnl **** Check for libSDL2 ****
if test "x$with_sdl" != "xno"
then
    WINE_PACKAGE_FLAGS(SDL2,[sdl2],[-lSDL2],,,
        [AC_CHECK_HEADERS([SDL2/SDL.h],
            [WINE_CHECK_SONAME(SDL2,SDL_Init,,,[$SDL2_LIBS],[[libSDL2-2.0*]])])])
fi
WINE_NOTICE_WITH(sdl,[test "x$ac_cv_lib_soname_SDL2" = "x"],
                 [libSDL2 ${notice_platform}development files not found, SDL2 won't be supported.])

dnl **** Check for FAudio ****
if test "x$with_faudio" != "xno"
then
    WINE_PACKAGE_FLAGS(FAUDIO,[faudio],[-lFAudio],,,
        [AC_CHECK_HEADERS([FAudio.h],
            [WINE_CHECK_SONAME(FAudio,FAudioCreate,,,[$FAUDIO_LIBS],[[libFAudio*]])])
             WINE_CHECK_LIB_FUNCS([FAudio_CommitOperationSet \
                                   F3DAudioInitialize8], [$FAUDIO_LIBS])
            ])
fi
WINE_NOTICE_WITH(faudio,[test "x$ac_cv_lib_soname_FAudio" = "x"],
                 [libFAudio ${notice_platform}development files not found, XAudio2 won't be supported.])

dnl **** Check for capi4linux ****
if test "x$with_capi" != "xno"
then
    WINE_PACKAGE_FLAGS(CAPI20,[capi20],,,,
     dnl Check for broken kernel header that doesn't define __user
    [AC_CHECK_HEADERS([capi20.h linux/capi.h],,,[#define __user])
    if test "$ac_cv_header_capi20_h" = "yes" -a "$ac_cv_header_linux_capi_h" = "yes"
    then
        WINE_CHECK_SONAME(capi20,capi20_register,,,[$CAPI20_LIBS])
    fi])
fi
WINE_NOTICE_WITH(capi,[test "x$ac_cv_lib_soname_capi20" = "x"],
                 [libcapi20 ${notice_platform}development files not found, ISDN won't be supported.])

dnl **** Check for cups ****
if test "x$with_cups" != "xno"
then
    WINE_PACKAGE_FLAGS(CUPS,[cups],[-lcups],
                       [`${CUPS_CONFIG:-cups-config} --cflags 2>/dev/null`],
                       [`${CUPS_CONFIG:-cups-config} --libs 2>/dev/null`],
                       [AC_CHECK_HEADERS(cups/cups.h cups/ppd.h)
                        if test "$ac_cv_header_cups_cups_h" = "yes"
                        then
                            WINE_CHECK_SONAME(cups,cupsGetDefault,,[CUPS_LIBS=""],[$CUPS_LIBS])
                        else
                            CUPS_CFLAGS=""
                            CUPS_LIBS=""
                        fi])
fi
WINE_NOTICE_WITH(cups,[test "x$ac_cv_lib_soname_cups" = "x"],
                 [libcups ${notice_platform}development files not found, CUPS won't be supported.])

dnl **** Check for fontconfig ****
if test "x$with_fontconfig" != "xno"
then
    WINE_PACKAGE_FLAGS(FONTCONFIG,[fontconfig],,[$X_CFLAGS],[$X_LIBS],
        [AC_CHECK_HEADERS([fontconfig/fontconfig.h])
        if test "$ac_cv_header_fontconfig_fontconfig_h" = "yes"
        then
            WINE_CHECK_SONAME(fontconfig,FcInit,,[FONTCONFIG_CFLAGS=""],[$FONTCONFIG_LIBS])
        else
            FONTCONFIG_CFLAGS=""
        fi])
fi
WINE_NOTICE_WITH(fontconfig,[test "x$ac_cv_lib_soname_fontconfig" = "x"],
                 [fontconfig ${notice_platform}development files not found, fontconfig won't be supported.])

dnl **** Check for gsm codec ****
if test "$ac_cv_header_gsm_h" = "yes" -o "$ac_cv_header_gsm_gsm_h" = "yes"
then
    WINE_CHECK_SONAME(gsm,gsm_create)
fi
WINE_NOTICE_WITH(gsm,[test "x$ac_cv_lib_soname_gsm" = "x"],
                 [libgsm ${notice_platform}development files not found, gsm 06.10 codec won't be supported.])

dnl **** Check for krb5 ****
if test "x$with_krb5" != "xno"
then
    WINE_PACKAGE_FLAGS(KRB5,[krb5],,
                       [`${KRB5_CONFIG:-krb5-config} --cflags 2>/dev/null`],
                       [`${KRB5_CONFIG:-krb5-config} --libs 2>/dev/null`],
        [AC_CHECK_HEADERS([krb5/krb5.h])
         if test "$ac_cv_header_krb5_krb5_h" = "yes"
         then
             WINE_CHECK_SONAME(krb5,krb5_is_config_principal,,[KRB5_CFLAGS=""],[$KRB5_LIBS])
         else
             KRB5_CFLAGS=""
         fi])
else
    with_gssapi=${with_gssapi:-no}
fi
WINE_NOTICE_WITH(krb5,[test "x$ac_cv_lib_soname_krb5" = "x"],
                 [libkrb5 ${notice_platform}development files not found (or too old), Kerberos won't be supported.])

dnl **** Check for gssapi ****
if test "x$with_gssapi" != "xno"
then
    WINE_PACKAGE_FLAGS(GSSAPI,[krb5-gssapi],,
                       [`${KRB5_CONFIG:-krb5-config} --cflags gssapi 2>/dev/null`],
                       [`${KRB5_CONFIG:-krb5-config} --libs gssapi 2>/dev/null`],
        [AC_CHECK_HEADERS([gssapi/gssapi.h gssapi/gssapi_ext.h])
        if test "$ac_cv_header_gssapi_gssapi_h" = "yes" -a "$ac_cv_header_gssapi_gssapi_ext_h" = "yes"
        then
            WINE_CHECK_SONAME(gssapi_krb5, gss_init_sec_context,,[GSSAPI_CFLAGS=""],[$GSSAPI_LIBS])
        else
            GSSAPI_CFLAGS=""
        fi])
fi
WINE_NOTICE_WITH(gssapi,[test "x$ac_cv_lib_soname_gssapi_krb5" = "x"],
                 [libgssapi_krb5 ${notice_platform}development files not found (or too old), no Kerberos SSP support.])

dnl **** Check for libjpeg ****
if test "x$with_jpeg" != "xno"
then
    WINE_PACKAGE_FLAGS(JPEG,[libjpeg],,,,
        [AC_CHECK_HEADERS([jpeglib.h])
        if test "$ac_cv_header_jpeglib_h" = "yes"
        then
            WINE_CHECK_SONAME(jpeg,jpeg_start_decompress,,[JPEG_CFLAGS=""],[$JPEG_LIBS])
        else
            JPEG_CFLAGS=""
        fi])
fi
WINE_WARNING_WITH(jpeg,[test "x$ac_cv_lib_soname_jpeg" = "x"],
                 [libjpeg ${notice_platform}development files not found, JPEG won't be supported.])

dnl **** Check for libpng ****
if test "x$with_png" != "xno"
then
    WINE_PACKAGE_FLAGS(PNG,[libpng],,[$X_CFLAGS],[$X_LIBS],
        [AC_CHECK_HEADERS([png.h])
        if test "$ac_cv_header_png_h" = "yes"
        then
            WINE_CHECK_SONAME(png,png_create_read_struct,,
                [PNG_CFLAGS=""],[$PNG_LIBS -lm -lz],[[libpng[[0-9]]*]])
        else
            PNG_CFLAGS=""
        fi])
fi
WINE_WARNING_WITH(png,[test "x$ac_cv_lib_soname_png" = "x"],
                 [libpng ${notice_platform}development files not found, PNG won't be supported.])

dnl **** Check for libtiff ****
if test "x$with_tiff" != "xno"
then
    WINE_PACKAGE_FLAGS(TIFF,[libtiff-4],,,,
        [AC_CHECK_HEADERS([tiffio.h])
        if test "$ac_cv_header_tiffio_h" = "yes"
        then
            WINE_CHECK_SONAME(tiff,TIFFClientOpen,,,[$TIFF_LIBS])
        fi])
fi
WINE_NOTICE_WITH(tiff,[test "x$ac_cv_lib_soname_tiff" = "x"],
                 [libtiff ${notice_platform}development files not found, TIFF won't be supported.])

dnl **** Check for mpg123 ****
if test "x$with_mpg123" != "xno"
then
    WINE_PACKAGE_FLAGS(MPG123,[libmpg123],[-lmpg123],,,
        [AC_CHECK_HEADERS([mpg123.h])
        if test "$ac_cv_header_mpg123_h" = "yes"
        then
            AC_CHECK_LIB(mpg123,mpg123_feed,[:],[MPG123_LIBS=""],[$MPG123_LIBS])
        else
            MPG123_CFLAGS=""
            MPG123_LIBS=""
        fi])
fi
WINE_NOTICE_WITH(mpg123,[test "x$ac_cv_lib_mpg123_mpg123_feed" != xyes -a x"$ac_cv_header_CoreAudio_CoreAudio_h" != xyes],
                 [libmpg123 ${notice_platform}development files not found (or too old), mp3 codec won't be supported.],
                 [enable_l3codeca_acm])
test "x$ac_cv_lib_mpg123_mpg123_feed" = xyes || enable_mp3dmod=${enable_mp3dmod:-no}

dnl **** Check for OpenAL 1.1 ****
if test "$ac_cv_header_AL_al_h" = "yes"
then
    WINE_CHECK_SONAME(openal,alGetSource3i,[AC_SUBST(OPENAL_LIBS,"-lopenal")
                      ac_cv_lib_openal=yes
                      AC_DEFINE_UNQUOTED(HAVE_OPENAL,1,[Define to 1 if OpenAL is available])],,)
fi
WINE_NOTICE_WITH(openal,[test "x$ac_cv_lib_openal" != xyes],
                 [libopenal ${notice_platform}development files not found (or too old), OpenAL won't be supported.],
                 [enable_openal32])

if test "x$ac_cv_lib_soname_FAudio" = "x"
then
    enable_x3daudio1_0=${enable_x3daudio1_0:-no}
    enable_x3daudio1_1=${enable_x3daudio1_1:-no}
    enable_x3daudio1_2=${enable_x3daudio1_2:-no}
    enable_x3daudio1_3=${enable_x3daudio1_3:-no}
    enable_x3daudio1_4=${enable_x3daudio1_4:-no}
    enable_x3daudio1_5=${enable_x3daudio1_5:-no}
    enable_x3daudio1_6=${enable_x3daudio1_6:-no}
    enable_x3daudio1_7=${enable_x3daudio1_7:-no}
    enable_xapofx1_1=${enable_xapofx1_1:-no}
    enable_xapofx1_2=${enable_xapofx1_2:-no}
    enable_xapofx1_3=${enable_xapofx1_3:-no}
    enable_xapofx1_4=${enable_xapofx1_4:-no}
    enable_xapofx1_5=${enable_xapofx1_5:-no}
    enable_xaudio2_0=${enable_xaudio2_0:-no}
    enable_xaudio2_1=${enable_xaudio2_1:-no}
    enable_xaudio2_2=${enable_xaudio2_2:-no}
    enable_xaudio2_3=${enable_xaudio2_3:-no}
    enable_xaudio2_4=${enable_xaudio2_4:-no}
    enable_xaudio2_5=${enable_xaudio2_5:-no}
    enable_xaudio2_6=${enable_xaudio2_6:-no}
    enable_xaudio2_7=${enable_xaudio2_7:-no}
    enable_xaudio2_8=${enable_xaudio2_8:-no}
    enable_xaudio2_9=${enable_xaudio2_9:-no}
fi

dnl **** Check for libkstat ****
if test "$ac_cv_header_kstat_h" = "yes"
then
    AC_CHECK_LIB(kstat,kstat_open,
                 [AC_DEFINE(HAVE_LIBKSTAT, 1, [Define to 1 if you have the `kstat' library (-lkstat).])
                  AC_SUBST(KSTAT_LIBS,"-lkstat")])
fi

dnl **** Check for libprocstat ****
if test "$ac_cv_header_libprocstat_h" = "yes"
then
    AC_CHECK_LIB(procstat,procstat_open_sysctl,
                 [AC_DEFINE(HAVE_LIBPROCSTAT, 1, [Define to 1 if you have the `procstat' library (-lprocstat).])
                  AC_SUBST(PROCSTAT_LIBS,"-lprocstat")])
fi

dnl **** Check for libodbc ****
WINE_CHECK_SONAME(odbc,SQLConnect,,[AC_DEFINE_UNQUOTED(SONAME_LIBODBC,["libodbc.$LIBEXT"])])

dnl **** Check for libnetapi ****
if test "x$with_netapi" != "xno"
then
    WINE_PACKAGE_FLAGS(NETAPI,[netapi],,,,
      [WINE_CHECK_SONAME(netapi,libnetapi_init,,[AC_DEFINE_UNQUOTED(SONAME_LIBNETAPI,["libnetapi.$LIBEXT"])],[$NETAPI_LIBS])])
fi

dnl **** Check for any sound system ****
if test "x$enable_winealsa_drv$enable_winecoreaudio_drv$enable_winepulse_drv$enable_wineoss_drv$enable_wineandroid_drv" = xnonononono -a \
        "x$with_alsa$with_coreaudio$with_oss$with_pulse" != xnononono
then
    WINE_WARNING([No sound system was found. Windows applications will be silent.])
fi

dnl *** Check for Vulkan ***
if test "x$with_vulkan" != "xno"
then
    WINE_CHECK_SONAME(vulkan, vkGetInstanceProcAddr)
    if test "x$ac_cv_lib_soname_vulkan" = "x"
    then
        WINE_CHECK_SONAME(MoltenVK, vkGetInstanceProcAddr)
    fi
fi
WINE_NOTICE_WITH(vulkan,[test "x$ac_cv_lib_soname_vulkan" = "x" -a "x$ac_cv_lib_soname_MoltenVK" = "x"],
                 [libvulkan and libMoltenVK ${notice_platform}development files not found, Vulkan won't be supported.])

dnl **** Check for vkd3d ****
if test "x$with_vkd3d" != "xno"
then
    WINE_PACKAGE_FLAGS(VKD3D,[libvkd3d],,,,
        [WINE_CHECK_SONAME(vkd3d,vkd3d_get_dxgi_format,,,[$VKD3D_LIBS])])
fi
WINE_NOTICE_WITH(vkd3d,[test "x$ac_cv_lib_soname_vkd3d" = "x"],
                 [vkd3d ${notice_platform}development files not found (or too old), Direct3D 12 won't be supported.])
test "x$ac_cv_lib_soname_vkd3d" != "x" || enable_d3d12=${enable_d3d12:-no}

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
  return (Array[[1]] != -2 || L[[2]] != 3)]])],
    [ac_cv_c_gcc_strength_bug="no"],[ac_cv_c_gcc_strength_bug="yes"],[ac_cv_c_gcc_strength_bug="no"]) )
  if test "$ac_cv_c_gcc_strength_bug" = "yes"
  then
    EXTRACFLAGS="$EXTRACFLAGS -fno-strength-reduce"
  fi

  dnl Check for some compiler flags
  WINE_TRY_CFLAGS([-fno-strict-aliasing])
  WINE_TRY_CFLAGS([-fexcess-precision=standard],[AC_SUBST(EXCESS_PRECISION_CFLAGS,"-fexcess-precision=standard")])
  dnl clang needs to be told to fail on unknown options
  saved_CFLAGS=$CFLAGS
  WINE_TRY_CFLAGS([-Werror=unknown-warning-option],[CFLAGS="$CFLAGS -Werror=unknown-warning-option"])
  WINE_TRY_CFLAGS([-Werror=unused-command-line-argument],[CFLAGS="$CFLAGS -Werror=unused-command-line-argument"])
  WINE_TRY_CFLAGS([-Wdeclaration-after-statement])
  WINE_TRY_CFLAGS([-Wempty-body])
  WINE_TRY_CFLAGS([-Wignored-qualifiers])
  WINE_TRY_CFLAGS([-Wpacked-not-aligned],[EXTRACFLAGS="$EXTRACFLAGS -Wno-packed-not-aligned"])
  WINE_TRY_CFLAGS([-Wpragma-pack],[EXTRACFLAGS="$EXTRACFLAGS -Wno-pragma-pack"])
  WINE_TRY_CFLAGS([-Wshift-overflow=2])
  WINE_TRY_CFLAGS([-Wstrict-prototypes])
  WINE_TRY_CFLAGS([-Wtype-limits])
  WINE_TRY_CFLAGS([-Wunused-but-set-parameter])
  WINE_TRY_CFLAGS([-Wvla])
  WINE_TRY_CFLAGS([-Wwrite-strings])

  dnl Check for noisy string.h
  WINE_TRY_CFLAGS([-Wpointer-arith],
     [saved_string_h_CFLAGS=$CFLAGS
      CFLAGS="$CFLAGS -Wpointer-arith -Werror"
      AC_CACHE_CHECK([for broken string.h that generates warnings with -Wpointer-arith], ac_cv_c_string_h_warnings,
          AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <string.h>]])],[ac_cv_c_string_h_warnings=no],[ac_cv_c_string_h_warnings=yes]))
      test "$ac_cv_c_string_h_warnings" = yes || EXTRACFLAGS="$EXTRACFLAGS -Wpointer-arith"
      CFLAGS=$saved_string_h_CFLAGS])

  dnl Check for noisy string.h on logical ops
  WINE_TRY_CFLAGS([-Wlogical-op],
     [saved_string_h_CFLAGS=$CFLAGS
      CFLAGS="$CFLAGS -Wlogical-op -Werror"
      AC_CACHE_CHECK([for broken string.h that generates warnings with -Wlogical-op], ac_cv_c_logicalop_noisy,
          AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <string.h>
char*f(const char *h,char n) {return strchr(h,n);}]])],[ac_cv_c_logicalop_noisy=no],[ac_cv_c_logicalop_noisy=yes]))
      CFLAGS=$saved_string_h_CFLAGS
      test "$ac_cv_c_logicalop_noisy" = yes || EXTRACFLAGS="$EXTRACFLAGS -Wlogical-op"])

  dnl Default to dwarf-2 debug info
  for ac_flag in $CFLAGS; do
    case $ac_flag in
      -g) WINE_TRY_CFLAGS([-gdwarf-2])
          WINE_TRY_CFLAGS([-gstrict-dwarf]) ;;
    esac
  done

  dnl Disable gcc builtins except for Mingw
  AC_SUBST(MSVCRTFLAGS,"")
  case $host_os in
    cygwin*|mingw32*) ;;
    *) WINE_TRY_CFLAGS([-fno-builtin],[MSVCRTFLAGS="$MSVCRTFLAGS -fno-builtin"])
       WINE_TRY_CFLAGS([-fshort-wchar],[MSVCRTFLAGS="$MSVCRTFLAGS -fshort-wchar"]) ;;
  esac

  case $host_cpu in
    dnl gcc-4.6+ omits frame pointers by default, breaking some copy protections
    *i[[3456789]]86*) WINE_TRY_CFLAGS([-fno-omit-frame-pointer]) ;;
    *x86_64*)
      case $host_os in
        dnl Mingw uses Windows 64-bit types, not Unix ones
        cygwin*|mingw32*) WINE_TRY_CFLAGS([-Wno-format]) ;;
        dnl Default to ms_abi on 64-bit
        *) WINE_TRY_CFLAGS([-mabi=ms],[MSVCRTFLAGS="$MSVCRTFLAGS -mabi=ms"]) ;;
      esac ;;
  esac

  CFLAGS=$saved_CFLAGS

  dnl Enable -Werror for maintainer mode
  if test "x$enable_maintainer_mode" = "xyes"
  then
      WINE_TRY_CFLAGS([-Werror])
  fi
fi

dnl **** Disable Fortify, it has too many false positives

AC_CACHE_CHECK([for the need to disable Fortify], ac_cv_c_fortify_enabled,
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <string.h>]],
[[#if (defined(__USE_FORTIFY_LEVEL) && __USE_FORTIFY_LEVEL > 0) || (defined(_FORTIFY_SOURCE) && _FORTIFY_SOURCE > 0)
#error Fortify enabled
#endif]])],
        [ac_cv_c_fortify_enabled=no],[ac_cv_c_fortify_enabled=yes]))
if test "$ac_cv_c_fortify_enabled" = yes
then
    CFLAGS="$CFLAGS -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0"
fi

dnl **** Check for CFI directives support ****

AC_CACHE_CHECK([whether CFI directives are supported in assembly code], ac_cv_c_cfi_support,
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[asm(".text\nac_test:\t.cfi_startproc\n\t.long 0\n\t.cfi_endproc");]])],
                      ac_cv_c_cfi_support="yes",ac_cv_c_cfi_support="no"))
if test "$ac_cv_c_cfi_support" = "yes"
then
   LDDLLFLAGS="$LDDLLFLAGS -fasynchronous-unwind-tables"
else
   if test "x$enable_win64" = "xyes"
   then
      WINE_WARNING([building 64-bit Wine without support for CFI directives; exception handling will not work properly.])
   fi
fi

dnl **** Platform-specific checks ****

case $build_os in
  cygwin*|mingw32*) AC_SUBST(TOOLSEXT,".exe") ;;
esac

case $host_os in
  linux*)
    case $host_cpu in
      *i[[3456789]]86*|x86_64*|*aarch64*)
        test "$wine_binary" = wine || WINE_IGNORE_FILE("loader/wine-preloader")
        WINELOADER_PROGRAMS="$WINELOADER_PROGRAMS $wine_binary-preloader"
        ;;
    esac
    ;;
esac

dnl **** Check for functions ****

ac_save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $BUILTINFLAG"
AC_CHECK_FUNCS(\
        __res_get_state \
        __res_getservers \
	_spawnvp \
	dlopen \
	epoll_create \
	ffs \
	finitef \
	fnmatch \
	fork \
	fpclass \
	fstatfs \
	fstatvfs \
	futimens \
	futimes \
	futimesat \
	getattrlist \
	getauxval \
	getifaddrs \
	getopt_long_only \
	kqueue \
	lstat \
	pipe2 \
	poll \
	port_create \
	prctl \
	pread \
	proc_pidinfo \
	pwrite \
	readdir \
	readlink \
	sched_yield \
	select \
	setproctitle \
	setprogname \
	settimeofday \
	sigprocmask \
	statfs \
	statvfs \
	strnlen \
	strtold \
	symlink \
	sysinfo \
	tcdrain \
	thr_kill2 \
	timegm \
	usleep
)
CFLAGS="$ac_save_CFLAGS"

dnl Check for -ldl
if test "$ac_cv_func_dlopen" = no
then
    AC_CHECK_LIB(dl,dlopen,[AC_DEFINE(HAVE_DLOPEN,1) AC_SUBST(DL_LIBS,"-ldl")])
fi
WINE_CHECK_LIB_FUNCS(dladdr,[$DL_LIBS])

dnl Check for -lpoll for Mac OS X/Darwin
if test "$ac_cv_func_poll" = no
then
    AC_CHECK_LIB(poll,poll,[AC_DEFINE(HAVE_POLL,1) AC_SUBST(POLL_LIBS,"-lpoll")])
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
	inet_addr \
	inet_network \
	inet_ntop \
	inet_pton \
)

dnl Check for clock_gettime which may be in -lrt
case $host_os in
    darwin*|macosx*) ;;
    *) ac_save_LIBS=$LIBS
        AC_SEARCH_LIBS(clock_gettime, rt,
            [AC_DEFINE(HAVE_CLOCK_GETTIME, 1, [Define to 1 if you have the `clock_gettime' function.])
                test "$ac_res" = "none required" || AC_SUBST(RT_LIBS,"$ac_res")])
    LIBS=$ac_save_LIBS
    ;;
esac

dnl **** Check for OpenLDAP ***
if test "x$with_ldap" != "xno"
then
    dnl Actually there's no pkg-config support for ldap, but this way we can still override the flags
    WINE_PACKAGE_FLAGS(LDAP,[openldap],[-lldap_r -llber],,,
        [AC_CHECK_HEADERS([ldap.h lber.h])
        if test "$ac_cv_header_ldap_h" = "yes" -a "$ac_cv_header_lber_h" = "yes"
        then
            AC_CHECK_TYPE(LDAPSortKey,
                [AC_CHECK_LIB(ldap_r, ldap_initialize,
                    [AC_CHECK_LIB(lber, ber_init,
                        [AC_DEFINE(HAVE_LDAP, 1, [Define if you have the OpenLDAP development environment])],
                        [LDAP_LIBS=""],
                        [$PTHREAD_LIBS $LDAP_LIBS])],
                    [LDAP_LIBS=""],
                    [$PTHREAD_LIBS $LDAP_LIBS])],
                [LDAP_LIBS=""],
                [#include <ldap.h>])
            WINE_CHECK_LIB_FUNCS(\
                ldap_count_references \
                ldap_first_reference \
                ldap_next_reference \
                ldap_parse_reference \
                ldap_parse_sort_control \
                ldap_parse_sortresponse_control \
                ldap_parse_vlv_control \
                ldap_parse_vlvresponse_control,
                [$LDAP_LIBS $PTHREAD_LIBS])
        else
            LDAP_LIBS=""
        fi])
fi
WINE_NOTICE_WITH(ldap,[test "x$LDAP_LIBS" = "x"],
                 [libldap (OpenLDAP) ${notice_platform}development files not found, LDAP won't be supported.])

AC_CACHE_CHECK([for sched_setaffinity],wine_cv_have_sched_setaffinity,
                AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[#define _GNU_SOURCE
#include <sched.h>]], [[sched_setaffinity(0, 0, 0);]])],[wine_cv_have_sched_setaffinity=yes],[wine_cv_have_sched_setaffinity=no]))
if test "$wine_cv_have_sched_setaffinity" = "yes"
then
  AC_DEFINE(HAVE_SCHED_SETAFFINITY, 1, [Define to 1 if you have the `sched_setaffinity' function.])
fi

AC_CACHE_CHECK([for fallocate],wine_cv_have_fallocate,
                AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[#define _GNU_SOURCE
#include <fcntl.h>]], [[fallocate(-1, 0, 0, 0);]])],[wine_cv_have_fallocate=yes],[wine_cv_have_fallocate=no]))
if test "$wine_cv_have_fallocate" = "yes"
then
  AC_DEFINE(HAVE_FALLOCATE, 1, [Define to 1 if you have the `fallocate' function.])
fi

dnl **** Check for types ****

AC_C_INLINE
AC_CHECK_TYPES([fsblkcnt_t, fsfilcnt_t])
AC_CHECK_TYPES([sigset_t],,,[#include <sys/types.h>
#include <signal.h>])
AC_CHECK_TYPES([request_sense],,,[#include <linux/cdrom.h>])

AC_CHECK_TYPES([struct xinpgen],,,
[#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_SOCKETVAR_H
#include <sys/socketvar.h>
#endif
#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#ifdef HAVE_NETINET_IN_PCB_H
#include <netinet/in_pcb.h>
#endif])

AC_CHECK_TYPES([struct r_debug, struct link_map],,,
[#ifdef HAVE_LINK_H
#include <link.h>
#endif
#ifdef HAVE_SYS_LINK_H
#include <sys/link.h>
#endif])

AC_CHECK_MEMBERS([struct ff_effect.direction],,,
[#ifdef HAVE_LINUX_INPUT_H
#include <linux/input.h>
#endif])

AC_CACHE_CHECK([for if_nameindex],wine_cv_have_if_nameindex,
               AC_LINK_IFELSE([AC_LANG_PROGRAM([[#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NET_IF_H
# include <net/if.h>
#endif]],
                              [[struct if_nameindex *p = if_nameindex(); return p->if_index;]])],[wine_cv_have_if_nameindex=yes],[wine_cv_have_if_nameindex=no]))
if test "$wine_cv_have_if_nameindex" = "yes"
then
  AC_DEFINE(HAVE_IF_NAMEINDEX, 1, [Define to 1 if you have the `if_nameindex' function.])
fi

AC_CACHE_CHECK([for sigaddset],wine_cv_have_sigaddset,
               AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <signal.h>]], [[sigset_t set; sigaddset(&set,SIGTERM);]])],[wine_cv_have_sigaddset=yes],[wine_cv_have_sigaddset=no]))
if test "$wine_cv_have_sigaddset" = "yes"
then
  AC_DEFINE(HAVE_SIGADDSET, 1, [Define if sigaddset is supported])
fi


AC_CACHE_CHECK([whether we can use re-entrant gethostbyname_r Linux style],
   	wine_cv_linux_gethostbyname_r_6,
	AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <netdb.h>]],[[
    char *name=0;
    struct hostent he;
    struct hostent *result;
    char *buf=0;
    int bufsize=0;
    int errnr;
    char *addr=0;
    int addrlen=0;
    int addrtype=0;
    gethostbyname_r(name,&he,buf,bufsize,&result,&errnr);
    gethostbyaddr_r(addr, addrlen, addrtype,&he,buf,bufsize,&result,&errnr);
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

dnl Check for dirent.d_reclen
AC_CHECK_MEMBERS([struct dirent.d_reclen],,,
[#ifdef HAVE_DIRENT_H
#include <dirent.h>
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

dnl Check for stat.st_blocks and ns-resolved times
AC_CHECK_MEMBERS([
	struct stat.st_blocks,
	struct stat.st_mtim,
	struct stat.st_mtimespec,
	struct stat.st_ctim,
	struct stat.st_ctimespec,
	struct stat.st_atim,
	struct stat.st_atimespec,
	struct stat.st_birthtime,
	struct stat.st_birthtim,
	struct stat.st_birthtimespec,
	struct stat.__st_birthtime,
	struct stat.__st_birthtim])

dnl Check for sin6_scope_id
AC_CHECK_MEMBERS([struct sockaddr_in6.sin6_scope_id],,,
[#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif])

dnl Check for struct __res_state
AC_CHECK_TYPES([struct __res_state],,,
[#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_RESOLV_H
#include <resolv.h>
#endif])

dnl Check for _u._ext.nscount6
AC_CHECK_MEMBERS([struct __res_state._u._ext.nscount6],,,
[#ifdef HAVE_RESOLV_H
#include <resolv.h>
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

dnl Check for struct icmpstat
AC_CHECK_MEMBERS([struct icmpstat.icps_inhist],,,
[#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#ifdef HAVE_NETINET_IP_ICMP_H
#include <netinet/ip_icmp.h>
#endif
#ifdef HAVE_NETINET_ICMP_VAR_H
#include <netinet/icmp_var.h>
#endif])

dnl Check for struct icmpstat.icps_outhist
AC_CHECK_MEMBERS([struct icmpstat.icps_outhist],,,
[#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_ALIAS_H
#include <alias.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_SOCKETVAR_H
#include <sys/socketvar.h>
#endif
#ifdef HAVE_SYS_TIMEOUT_H
#include <sys/timeout.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#ifdef HAVE_NETINET_IP_VAR_H
#include <netinet/ip_var.h>
#endif
#ifdef HAVE_NETINET_IP_ICMP_H
#include <netinet/ip_icmp.h>
#endif
#ifdef HAVE_NETINET_ICMP_VAR_H
#include <netinet/icmp_var.h>
#endif])

dnl Check for struct ipstat
AC_CHECK_MEMBERS([struct ipstat.ips_total],,,
[#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKETVAR_H
#include <sys/socketvar.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IP_VAR_H
#include <netinet/ip_var.h>
#endif])

dnl Check for struct ip_stats
AC_CHECK_MEMBERS([struct ip_stats.ips_total],,,
[#ifdef HAVE_NETINET_IP_VAR_H
#include <netinet/ip_var.h>
#endif])

dnl Check for struct tcpstat
AC_CHECK_MEMBERS([struct tcpstat.tcps_connattempt],,,
[#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKETVAR_H
#include <sys/socketvar.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#ifdef HAVE_NETINET_TCP_VAR_H
#include <netinet/tcp_var.h>
#endif])

dnl Check for struct tcp_stats
AC_CHECK_MEMBERS([struct tcp_stats.tcps_connattempt],,,
[#ifdef HAVE_NETINET_TCP_VAR_H
#include <netinet/tcp_var.h>
#endif])

dnl Check for struct udpstat
AC_CHECK_MEMBERS([struct udpstat.udps_ipackets],,,
[#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IP_VAR_H
#include <netinet/ip_var.h>
#endif
#ifdef HAVE_NETINET_UDP_H
#include <netinet/udp.h>
#endif
#ifdef HAVE_NETINET_UDP_VAR_H
#include <netinet/udp_var.h>
#endif])

dnl Check for struct ifreq.ifr_hwaddr
AC_CHECK_MEMBERS([struct ifreq.ifr_hwaddr],,,
[#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NET_IF_H
# include <net/if.h>
#endif])

dnl Check for isfinite
ac_save_LIBS="$LIBS"
LIBS="$LIBS -lm"
AC_CACHE_CHECK([for isfinite], ac_cv_have_isfinite,
               AC_LINK_IFELSE([AC_LANG_PROGRAM([[#define _GNU_SOURCE
#include <math.h>]], [[float f = 0.0; return isfinite(f)]])],[ac_cv_have_isfinite="yes"],[ac_cv_have_isfinite="no"]))
if test "$ac_cv_have_isfinite" = "yes"
then
    AC_DEFINE(HAVE_ISFINITE, 1, [Define to 1 if you have the `isfinite' function.])
fi

dnl Check for isinf
AC_CACHE_CHECK([for isinf], ac_cv_have_isinf,
               AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <math.h>]], [[float f = 0.0; return isinf(f)]])],[ac_cv_have_isinf="yes"],[ac_cv_have_isinf="no"]))
if test "$ac_cv_have_isinf" = "yes"
then
    AC_DEFINE(HAVE_ISINF, 1, [Define to 1 if you have the `isinf' function.])
fi

dnl Check for isnan
AC_CACHE_CHECK([for isnan], ac_cv_have_isnan,
               AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <math.h>]], [[float f = 0.0; return isnan(f)]])],[ac_cv_have_isnan="yes"],[ac_cv_have_isnan="no"]))
if test "$ac_cv_have_isnan" = "yes"
then
    AC_DEFINE(HAVE_ISNAN, 1, [Define to 1 if you have the `isnan' function.])
fi

AC_CHECK_FUNCS(\
	acosh \
	acoshf \
	asinh \
	asinhf \
	atanh \
	atanhf \
	cbrt \
	cbrtf \
	erf \
	erfc \
	erfcf \
	erff \
	exp2 \
	exp2f \
	expm1 \
	expm1f \
	fma \
	fmaf \
	ilogb \
	ilogbf \
	j0 \
	j1 \
	jn \
	lgamma \
	lgammaf \
	llrint \
	llrintf \
	llround \
	llroundf \
	log1p \
	log1pf \
	log2 \
	log2f \
	lrint \
	lrintf \
	lround \
	lroundf \
	nearbyint \
	nearbyintf \
	nexttoward \
	nexttowardf \
	powl \
	remainder \
	remainderf \
	remquo \
	remquof \
	rint \
	rintf \
	round \
	roundf \
	tgamma \
	tgammaf \
	trunc \
	truncf \
	y0 \
	y1 \
	yn
)
LIBS="$ac_save_LIBS"

dnl Check for __builtin_clz
AC_CACHE_CHECK([for __builtin_clz], ac_cv_have___builtin_clz,
               AC_LINK_IFELSE([AC_LANG_PROGRAM(,[[return __builtin_clz(1)]])],
               [ac_cv_have___builtin_clz="yes"], [ac_cv_have___builtin_clz="no"]))
if test "$ac_cv_have___builtin_clz" = "yes"
then
    AC_DEFINE(HAVE___BUILTIN_CLZ, 1, [Define to 1 if you have the `__builtin_clz' built-in function.])
fi

dnl Check for __builtin_popcount
AC_CACHE_CHECK([for __builtin_popcount], ac_cv_have___builtin_popcount,
               AC_LINK_IFELSE([AC_LANG_PROGRAM(,[[return __builtin_popcount(1)]])],
               [ac_cv_have___builtin_popcount="yes"], [ac_cv_have___builtin_popcount="no"]))
if test "$ac_cv_have___builtin_popcount" = "yes"
then
    AC_DEFINE(HAVE___BUILTIN_POPCOUNT, 1, [Define to 1 if you have the `__builtin_popcount' built-in function.])
fi

AC_CACHE_CHECK([for __clear_cache], ac_cv_have___clear_cache,
               AC_LINK_IFELSE([AC_LANG_PROGRAM(,[[__clear_cache((void*)0, (void*)0); return 0;]])],
               [ac_cv_have___clear_cache="yes"], [ac_cv_have___clear_cache="no"]))
if test "$ac_cv_have___clear_cache" = "yes"
then
    AC_DEFINE(HAVE___CLEAR_CACHE, 1, [Define to 1 if you have the `__clear_cache' (potentially built-in) function.])
fi

dnl *** check for the need to define platform-specific symbols

case $host_cpu in
  *i[[3456789]]86*) WINE_CHECK_DEFINE([__i386__]) ;;
  *x86_64*)         WINE_CHECK_DEFINE([__x86_64__]) ;;
  *sparc64*)        WINE_CHECK_DEFINE([__sparc64__]) ;;
  *sparc*)          WINE_CHECK_DEFINE([__sparc__]) ;;
  *powerpc64*)      WINE_CHECK_DEFINE([__powerpc64__]) ;;
  *powerpc*)        WINE_CHECK_DEFINE([__powerpc__]) ;;
  *aarch64*)        WINE_CHECK_DEFINE([__aarch64__]) ;;
  *arm*)            WINE_CHECK_DEFINE([__arm__]) ;;
esac

case $host_vendor in
  *sun*) WINE_CHECK_DEFINE([__sun__]) ;;
esac

dnl **** Generate output files ****

AH_TOP([#ifndef __WINE_CONFIG_H
#define __WINE_CONFIG_H
#ifdef WINE_CROSS_PE
#error config.h should not be used when cross-compiling
#endif])
AH_BOTTOM([#endif /* __WINE_CONFIG_H */])

AC_CONFIG_COMMANDS([include/stamp-h], [echo timestamp > include/stamp-h])
AS_ECHO_N("creating Makefile rules...") >&AS_MESSAGE_FD

AC_SUBST([ALL_VARS_RULES],
"m4_set_foreach([_AC_SUBST_VARS],[var],[m4_if(m4_bregexp(m4_defn([var]),[\(_CFLAGS\|_LIBS\)$]),-1,,[var = $var
])])")

dnl Main makefile

WINE_APPEND_RULE([
# Rules automatically generated by configure

.INIT: Makefile
.MAKEFILEDEPS:
all: Makefile
Makefile: $srcdir/Makefile.in config.status \$(MAKEDEP)
	@./config.status Make.tmp Makefile
depend: \$(MAKEDEP) dummy
	\$(MAKEDEP)])

WINE_CONFIG_SYMLINK(fonts,fonts,[marlett.ttf,symbol.ttf,tahoma.ttf,tahomabd.ttf,wingding.ttf],["x$enable_fonts" != xno])
WINE_CONFIG_SYMLINK(loader,loader,[l_intl.nls])
WINE_CONFIG_SYMLINK(,tools,[wine],,winewrapper)
WINE_CONFIG_SYMLINK(,tools,[wine64],["x$enable_win64" != xno -o -n "$with_wine64"],winewrapper)

WINE_CONFIG_MAKEFILE(dlls/acledit)
WINE_CONFIG_MAKEFILE(dlls/aclui)
WINE_CONFIG_MAKEFILE(dlls/activeds)
WINE_CONFIG_MAKEFILE(dlls/actxprxy)
WINE_CONFIG_MAKEFILE(dlls/adsiid)
WINE_CONFIG_MAKEFILE(dlls/adsldp)
WINE_CONFIG_MAKEFILE(dlls/adsldp/tests)
WINE_CONFIG_MAKEFILE(dlls/adsldpc)
WINE_CONFIG_MAKEFILE(dlls/advapi32)
WINE_CONFIG_MAKEFILE(dlls/advapi32/tests)
WINE_CONFIG_MAKEFILE(dlls/advpack)
WINE_CONFIG_MAKEFILE(dlls/advpack/tests)
WINE_CONFIG_MAKEFILE(dlls/amsi)
WINE_CONFIG_MAKEFILE(dlls/amstream)
WINE_CONFIG_MAKEFILE(dlls/amstream/tests)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-appmodel-identity-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-appmodel-runtime-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-appmodel-runtime-l1-1-2)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-apiquery-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-appcompat-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-appinit-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-atoms-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-bem-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-com-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-com-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-com-private-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-comm-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-console-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-console-l2-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-crt-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-crt-l2-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-datetime-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-datetime-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-debug-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-debug-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-delayload-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-delayload-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-errorhandling-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-errorhandling-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-errorhandling-l1-1-2)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-errorhandling-l1-1-3)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-fibers-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-fibers-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-file-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-file-l1-2-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-file-l1-2-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-file-l1-2-2)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-file-l2-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-file-l2-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-file-l2-1-2)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-handle-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-heap-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-heap-l1-2-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-heap-l2-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-heap-obsolete-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-interlocked-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-interlocked-l1-2-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-io-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-io-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-job-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-job-l2-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-kernel32-legacy-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-kernel32-legacy-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-kernel32-private-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-largeinteger-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-libraryloader-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-libraryloader-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-libraryloader-l1-2-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-libraryloader-l1-2-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-libraryloader-l1-2-2)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-localization-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-localization-l1-2-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-localization-l1-2-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-localization-l2-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-localization-obsolete-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-localization-obsolete-l1-2-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-localization-obsolete-l1-3-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-localization-private-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-localregistry-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-memory-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-memory-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-memory-l1-1-2)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-misc-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-namedpipe-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-namedpipe-l1-2-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-namespace-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-normalization-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-path-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-privateprofile-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-processenvironment-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-processenvironment-l1-2-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-processthreads-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-processthreads-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-processthreads-l1-1-2)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-processthreads-l1-1-3)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-processtopology-obsolete-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-profile-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-psapi-ansi-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-psapi-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-psapi-obsolete-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-quirks-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-realtime-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-registry-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-registry-l2-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-registryuserspecific-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-rtlsupport-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-rtlsupport-l1-2-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-shlwapi-legacy-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-shlwapi-obsolete-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-shlwapi-obsolete-l1-2-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-shutdown-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-sidebyside-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-string-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-string-l2-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-string-obsolete-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-stringansi-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-stringloader-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-synch-ansi-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-synch-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-synch-l1-2-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-synch-l1-2-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-sysinfo-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-sysinfo-l1-2-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-sysinfo-l1-2-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-threadpool-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-threadpool-l1-2-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-threadpool-legacy-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-threadpool-private-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-timezone-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-toolhelp-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-url-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-util-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-version-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-version-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-version-private-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-versionansi-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-windowserrorreporting-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-winrt-error-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-winrt-error-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-winrt-errorprivate-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-winrt-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-winrt-registration-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-winrt-roparameterizediid-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-winrt-string-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-winrt-string-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-wow64-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-wow64-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-xstate-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-core-xstate-l2-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-crt-conio-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-crt-convert-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-crt-environment-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-crt-filesystem-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-crt-heap-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-crt-locale-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-crt-math-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-crt-multibyte-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-crt-private-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-crt-process-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-crt-runtime-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-crt-stdio-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-crt-string-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-crt-time-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-crt-utility-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-devices-config-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-devices-config-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-devices-query-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-downlevel-advapi32-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-downlevel-advapi32-l2-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-downlevel-normaliz-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-downlevel-ole32-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-downlevel-shell32-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-downlevel-shlwapi-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-downlevel-shlwapi-l2-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-downlevel-user32-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-downlevel-version-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-dx-d3dkmt-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-eventing-classicprovider-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-eventing-consumer-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-eventing-controller-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-eventing-legacy-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-eventing-provider-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-eventlog-legacy-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-gdi-dpiinfo-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-mm-joystick-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-mm-misc-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-mm-mme-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-mm-time-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-ntuser-dc-access-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-ntuser-rectangle-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-ntuser-sysparams-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-perf-legacy-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-power-base-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-power-setting-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-rtcore-ntuser-draw-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-rtcore-ntuser-private-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-rtcore-ntuser-private-l1-1-4)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-rtcore-ntuser-window-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-rtcore-ntuser-winevent-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-rtcore-ntuser-wmpointer-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-rtcore-ntuser-wmpointer-l1-1-3)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-security-activedirectoryclient-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-security-audit-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-security-base-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-security-base-l1-2-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-security-base-private-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-security-credentials-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-security-cryptoapi-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-security-grouppolicy-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-security-lsalookup-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-security-lsalookup-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-security-lsalookup-l2-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-security-lsalookup-l2-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-security-lsapolicy-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-security-provider-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-security-sddl-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-security-systemfunctions-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-service-core-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-service-core-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-service-management-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-service-management-l2-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-service-private-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-service-winsvc-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-service-winsvc-l1-2-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-shcore-obsolete-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-shcore-scaling-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-shcore-stream-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-shcore-thread-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-shell-shellcom-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/api-ms-win-shell-shellfolders-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/apphelp)
WINE_CONFIG_MAKEFILE(dlls/apphelp/tests)
WINE_CONFIG_MAKEFILE(dlls/appwiz.cpl)
WINE_CONFIG_MAKEFILE(dlls/atl)
WINE_CONFIG_MAKEFILE(dlls/atl/tests)
WINE_CONFIG_MAKEFILE(dlls/atl100)
WINE_CONFIG_MAKEFILE(dlls/atl100/tests)
WINE_CONFIG_MAKEFILE(dlls/atl110)
WINE_CONFIG_MAKEFILE(dlls/atl80)
WINE_CONFIG_MAKEFILE(dlls/atl80/tests)
WINE_CONFIG_MAKEFILE(dlls/atl90)
WINE_CONFIG_MAKEFILE(dlls/atlthunk)
WINE_CONFIG_MAKEFILE(dlls/atlthunk/tests)
WINE_CONFIG_MAKEFILE(dlls/atmlib)
WINE_CONFIG_MAKEFILE(dlls/authz)
WINE_CONFIG_MAKEFILE(dlls/avicap32)
WINE_CONFIG_MAKEFILE(dlls/avifil32)
WINE_CONFIG_MAKEFILE(dlls/avifil32/tests)
WINE_CONFIG_MAKEFILE(dlls/avifile.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/avrt)
WINE_CONFIG_MAKEFILE(dlls/bcrypt)
WINE_CONFIG_MAKEFILE(dlls/bcrypt/tests)
WINE_CONFIG_MAKEFILE(dlls/bluetoothapis)
WINE_CONFIG_MAKEFILE(dlls/browseui)
WINE_CONFIG_MAKEFILE(dlls/browseui/tests)
WINE_CONFIG_MAKEFILE(dlls/bthprops.cpl)
WINE_CONFIG_MAKEFILE(dlls/cabinet)
WINE_CONFIG_MAKEFILE(dlls/cabinet/tests)
WINE_CONFIG_MAKEFILE(dlls/capi2032)
WINE_CONFIG_MAKEFILE(dlls/cards)
WINE_CONFIG_MAKEFILE(dlls/cdosys)
WINE_CONFIG_MAKEFILE(dlls/cfgmgr32)
WINE_CONFIG_MAKEFILE(dlls/clusapi)
WINE_CONFIG_MAKEFILE(dlls/combase)
WINE_CONFIG_MAKEFILE(dlls/combase/tests)
WINE_CONFIG_MAKEFILE(dlls/comcat)
WINE_CONFIG_MAKEFILE(dlls/comcat/tests)
WINE_CONFIG_MAKEFILE(dlls/comctl32)
WINE_CONFIG_MAKEFILE(dlls/comctl32/tests)
WINE_CONFIG_MAKEFILE(dlls/comdlg32)
WINE_CONFIG_MAKEFILE(dlls/comdlg32/tests)
WINE_CONFIG_MAKEFILE(dlls/comm.drv16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/commdlg.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/compobj.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/compstui)
WINE_CONFIG_MAKEFILE(dlls/comsvcs)
WINE_CONFIG_MAKEFILE(dlls/comsvcs/tests)
WINE_CONFIG_MAKEFILE(dlls/concrt140)
WINE_CONFIG_MAKEFILE(dlls/connect)
WINE_CONFIG_MAKEFILE(dlls/credui)
WINE_CONFIG_MAKEFILE(dlls/credui/tests)
WINE_CONFIG_MAKEFILE(dlls/crtdll)
WINE_CONFIG_MAKEFILE(dlls/crypt32)
WINE_CONFIG_MAKEFILE(dlls/crypt32/tests)
WINE_CONFIG_MAKEFILE(dlls/cryptdlg)
WINE_CONFIG_MAKEFILE(dlls/cryptdll)
WINE_CONFIG_MAKEFILE(dlls/cryptext)
WINE_CONFIG_MAKEFILE(dlls/cryptnet)
WINE_CONFIG_MAKEFILE(dlls/cryptnet/tests)
WINE_CONFIG_MAKEFILE(dlls/cryptui)
WINE_CONFIG_MAKEFILE(dlls/cryptui/tests)
WINE_CONFIG_MAKEFILE(dlls/ctapi32)
WINE_CONFIG_MAKEFILE(dlls/ctl3d.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/ctl3d32)
WINE_CONFIG_MAKEFILE(dlls/ctl3dv2.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/d2d1)
WINE_CONFIG_MAKEFILE(dlls/d2d1/tests)
WINE_CONFIG_MAKEFILE(dlls/d3d10)
WINE_CONFIG_MAKEFILE(dlls/d3d10/tests)
WINE_CONFIG_MAKEFILE(dlls/d3d10_1)
WINE_CONFIG_MAKEFILE(dlls/d3d10_1/tests)
WINE_CONFIG_MAKEFILE(dlls/d3d10core)
WINE_CONFIG_MAKEFILE(dlls/d3d10core/tests)
WINE_CONFIG_MAKEFILE(dlls/d3d11)
WINE_CONFIG_MAKEFILE(dlls/d3d11/tests)
WINE_CONFIG_MAKEFILE(dlls/d3d12)
WINE_CONFIG_MAKEFILE(dlls/d3d12/tests)
WINE_CONFIG_MAKEFILE(dlls/d3d8)
WINE_CONFIG_MAKEFILE(dlls/d3d8/tests)
WINE_CONFIG_MAKEFILE(dlls/d3d9)
WINE_CONFIG_MAKEFILE(dlls/d3d9/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dcompiler_33)
WINE_CONFIG_MAKEFILE(dlls/d3dcompiler_34)
WINE_CONFIG_MAKEFILE(dlls/d3dcompiler_35)
WINE_CONFIG_MAKEFILE(dlls/d3dcompiler_36)
WINE_CONFIG_MAKEFILE(dlls/d3dcompiler_37)
WINE_CONFIG_MAKEFILE(dlls/d3dcompiler_38)
WINE_CONFIG_MAKEFILE(dlls/d3dcompiler_39)
WINE_CONFIG_MAKEFILE(dlls/d3dcompiler_40)
WINE_CONFIG_MAKEFILE(dlls/d3dcompiler_41)
WINE_CONFIG_MAKEFILE(dlls/d3dcompiler_42)
WINE_CONFIG_MAKEFILE(dlls/d3dcompiler_43)
WINE_CONFIG_MAKEFILE(dlls/d3dcompiler_43/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dcompiler_46)
WINE_CONFIG_MAKEFILE(dlls/d3dcompiler_47)
WINE_CONFIG_MAKEFILE(dlls/d3dim)
WINE_CONFIG_MAKEFILE(dlls/d3drm)
WINE_CONFIG_MAKEFILE(dlls/d3drm/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_33)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_34)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_35)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_36)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_37)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_38)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_39)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_40)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_41)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_42)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_43)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_43/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dx11_42)
WINE_CONFIG_MAKEFILE(dlls/d3dx11_43)
WINE_CONFIG_MAKEFILE(dlls/d3dx11_43/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_24)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_25)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_26)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_27)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_28)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_29)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_30)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_31)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_32)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_33)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_34)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_35)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_36)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_36/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_37)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_38)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_39)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_40)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_41)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_42)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_43)
WINE_CONFIG_MAKEFILE(dlls/d3dxof)
WINE_CONFIG_MAKEFILE(dlls/d3dxof/tests)
WINE_CONFIG_MAKEFILE(dlls/davclnt)
WINE_CONFIG_MAKEFILE(dlls/dbgeng)
WINE_CONFIG_MAKEFILE(dlls/dbgeng/tests)
WINE_CONFIG_MAKEFILE(dlls/dbghelp)
WINE_CONFIG_MAKEFILE(dlls/dbghelp/tests)
WINE_CONFIG_MAKEFILE(dlls/dciman32)
WINE_CONFIG_MAKEFILE(dlls/ddeml.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/ddraw)
WINE_CONFIG_MAKEFILE(dlls/ddraw/tests)
WINE_CONFIG_MAKEFILE(dlls/ddrawex)
WINE_CONFIG_MAKEFILE(dlls/ddrawex/tests)
WINE_CONFIG_MAKEFILE(dlls/devenum)
WINE_CONFIG_MAKEFILE(dlls/devenum/tests)
WINE_CONFIG_MAKEFILE(dlls/dhcpcsvc)
WINE_CONFIG_MAKEFILE(dlls/dhtmled.ocx)
WINE_CONFIG_MAKEFILE(dlls/difxapi)
WINE_CONFIG_MAKEFILE(dlls/dinput)
WINE_CONFIG_MAKEFILE(dlls/dinput/tests)
WINE_CONFIG_MAKEFILE(dlls/dinput8)
WINE_CONFIG_MAKEFILE(dlls/dinput8/tests)
WINE_CONFIG_MAKEFILE(dlls/dispdib.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/dispex)
WINE_CONFIG_MAKEFILE(dlls/dispex/tests)
WINE_CONFIG_MAKEFILE(dlls/display.drv16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/dmband)
WINE_CONFIG_MAKEFILE(dlls/dmband/tests)
WINE_CONFIG_MAKEFILE(dlls/dmcompos)
WINE_CONFIG_MAKEFILE(dlls/dmcompos/tests)
WINE_CONFIG_MAKEFILE(dlls/dmime)
WINE_CONFIG_MAKEFILE(dlls/dmime/tests)
WINE_CONFIG_MAKEFILE(dlls/dmloader)
WINE_CONFIG_MAKEFILE(dlls/dmloader/tests)
WINE_CONFIG_MAKEFILE(dlls/dmoguids)
WINE_CONFIG_MAKEFILE(dlls/dmscript)
WINE_CONFIG_MAKEFILE(dlls/dmscript/tests)
WINE_CONFIG_MAKEFILE(dlls/dmstyle)
WINE_CONFIG_MAKEFILE(dlls/dmstyle/tests)
WINE_CONFIG_MAKEFILE(dlls/dmsynth)
WINE_CONFIG_MAKEFILE(dlls/dmsynth/tests)
WINE_CONFIG_MAKEFILE(dlls/dmusic)
WINE_CONFIG_MAKEFILE(dlls/dmusic/tests)
WINE_CONFIG_MAKEFILE(dlls/dmusic32)
WINE_CONFIG_MAKEFILE(dlls/dnsapi)
WINE_CONFIG_MAKEFILE(dlls/dnsapi/tests)
WINE_CONFIG_MAKEFILE(dlls/dplay)
WINE_CONFIG_MAKEFILE(dlls/dplayx)
WINE_CONFIG_MAKEFILE(dlls/dplayx/tests)
WINE_CONFIG_MAKEFILE(dlls/dpnaddr)
WINE_CONFIG_MAKEFILE(dlls/dpnet)
WINE_CONFIG_MAKEFILE(dlls/dpnet/tests)
WINE_CONFIG_MAKEFILE(dlls/dpnhpast)
WINE_CONFIG_MAKEFILE(dlls/dpnlobby)
WINE_CONFIG_MAKEFILE(dlls/dpvoice)
WINE_CONFIG_MAKEFILE(dlls/dpvoice/tests)
WINE_CONFIG_MAKEFILE(dlls/dpwsockx)
WINE_CONFIG_MAKEFILE(dlls/drmclien)
WINE_CONFIG_MAKEFILE(dlls/dsound)
WINE_CONFIG_MAKEFILE(dlls/dsound/tests)
WINE_CONFIG_MAKEFILE(dlls/dsquery)
WINE_CONFIG_MAKEFILE(dlls/dssenh)
WINE_CONFIG_MAKEFILE(dlls/dssenh/tests)
WINE_CONFIG_MAKEFILE(dlls/dswave)
WINE_CONFIG_MAKEFILE(dlls/dswave/tests)
WINE_CONFIG_MAKEFILE(dlls/dwmapi)
WINE_CONFIG_MAKEFILE(dlls/dwrite)
WINE_CONFIG_MAKEFILE(dlls/dwrite/tests)
WINE_CONFIG_MAKEFILE(dlls/dx8vb)
WINE_CONFIG_MAKEFILE(dlls/dxdiagn)
WINE_CONFIG_MAKEFILE(dlls/dxdiagn/tests)
WINE_CONFIG_MAKEFILE(dlls/dxerr8)
WINE_CONFIG_MAKEFILE(dlls/dxerr9)
WINE_CONFIG_MAKEFILE(dlls/dxgi)
WINE_CONFIG_MAKEFILE(dlls/dxgi/tests)
WINE_CONFIG_MAKEFILE(dlls/dxguid)
WINE_CONFIG_MAKEFILE(dlls/dxva2)
WINE_CONFIG_MAKEFILE(dlls/esent)
WINE_CONFIG_MAKEFILE(dlls/evr)
WINE_CONFIG_MAKEFILE(dlls/evr/tests)
WINE_CONFIG_MAKEFILE(dlls/explorerframe)
WINE_CONFIG_MAKEFILE(dlls/explorerframe/tests)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-authz-context-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-domainjoin-netjoin-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-dwmapi-ext-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-gdi-dc-create-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-gdi-dc-create-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-gdi-dc-l1-2-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-gdi-devcaps-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-gdi-draw-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-gdi-draw-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-gdi-font-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-gdi-font-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-gdi-render-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-kernel32-package-current-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-kernel32-package-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-ntuser-dialogbox-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-ntuser-draw-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-ntuser-gui-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-ntuser-gui-l1-3-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-ntuser-keyboard-l1-3-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-ntuser-message-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-ntuser-message-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-ntuser-misc-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-ntuser-misc-l1-2-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-ntuser-misc-l1-5-1)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-ntuser-mouse-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-ntuser-private-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-ntuser-private-l1-3-1)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-ntuser-rectangle-ext-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-ntuser-uicontext-ext-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-ntuser-window-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-ntuser-window-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-ntuser-window-l1-1-4)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-ntuser-windowclass-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-ntuser-windowclass-l1-1-1)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-oleacc-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-ras-rasapi32-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-rtcore-gdi-devcaps-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-rtcore-gdi-object-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-rtcore-gdi-rgn-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-rtcore-ntuser-cursor-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-rtcore-ntuser-dc-access-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-rtcore-ntuser-dpi-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-rtcore-ntuser-dpi-l1-2-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-rtcore-ntuser-rawinput-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-rtcore-ntuser-syscolors-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-rtcore-ntuser-sysparams-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-security-credui-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-security-cryptui-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-shell-comctl32-init-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-shell-comdlg32-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-shell-shell32-l1-2-0)
WINE_CONFIG_MAKEFILE(dlls/ext-ms-win-uxtheme-themes-l1-1-0)
WINE_CONFIG_MAKEFILE(dlls/faultrep)
WINE_CONFIG_MAKEFILE(dlls/faultrep/tests)
WINE_CONFIG_MAKEFILE(dlls/feclient)
WINE_CONFIG_MAKEFILE(dlls/fltlib)
WINE_CONFIG_MAKEFILE(dlls/fltmgr.sys)
WINE_CONFIG_MAKEFILE(dlls/fntcache)
WINE_CONFIG_MAKEFILE(dlls/fontsub)
WINE_CONFIG_MAKEFILE(dlls/fusion)
WINE_CONFIG_MAKEFILE(dlls/fusion/tests)
WINE_CONFIG_MAKEFILE(dlls/fwpuclnt)
WINE_CONFIG_MAKEFILE(dlls/gameux)
WINE_CONFIG_MAKEFILE(dlls/gameux/tests)
WINE_CONFIG_MAKEFILE(dlls/gdi.exe16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/gdi32)
WINE_CONFIG_MAKEFILE(dlls/gdi32/tests)
WINE_CONFIG_MAKEFILE(dlls/gdiplus)
WINE_CONFIG_MAKEFILE(dlls/gdiplus/tests)
WINE_CONFIG_MAKEFILE(dlls/glu32)
WINE_CONFIG_MAKEFILE(dlls/gphoto2.ds)
WINE_CONFIG_MAKEFILE(dlls/gpkcsp)
WINE_CONFIG_MAKEFILE(dlls/hal)
WINE_CONFIG_MAKEFILE(dlls/hhctrl.ocx)
WINE_CONFIG_MAKEFILE(dlls/hid)
WINE_CONFIG_MAKEFILE(dlls/hid/tests)
WINE_CONFIG_MAKEFILE(dlls/hidclass.sys)
WINE_CONFIG_MAKEFILE(dlls/hlink)
WINE_CONFIG_MAKEFILE(dlls/hlink/tests)
WINE_CONFIG_MAKEFILE(dlls/hnetcfg)
WINE_CONFIG_MAKEFILE(dlls/hnetcfg/tests)
WINE_CONFIG_MAKEFILE(dlls/httpapi)
WINE_CONFIG_MAKEFILE(dlls/httpapi/tests)
WINE_CONFIG_MAKEFILE(dlls/iccvid)
WINE_CONFIG_MAKEFILE(dlls/icmp)
WINE_CONFIG_MAKEFILE(dlls/ieframe)
WINE_CONFIG_MAKEFILE(dlls/ieframe/tests)
WINE_CONFIG_MAKEFILE(dlls/ieproxy)
WINE_CONFIG_MAKEFILE(dlls/ifsmgr.vxd,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/imaadp32.acm)
WINE_CONFIG_MAKEFILE(dlls/imagehlp)
WINE_CONFIG_MAKEFILE(dlls/imagehlp/tests)
WINE_CONFIG_MAKEFILE(dlls/imm.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/imm32)
WINE_CONFIG_MAKEFILE(dlls/imm32/tests)
WINE_CONFIG_MAKEFILE(dlls/inetcomm)
WINE_CONFIG_MAKEFILE(dlls/inetcomm/tests)
WINE_CONFIG_MAKEFILE(dlls/inetcpl.cpl)
WINE_CONFIG_MAKEFILE(dlls/inetmib1)
WINE_CONFIG_MAKEFILE(dlls/inetmib1/tests)
WINE_CONFIG_MAKEFILE(dlls/infosoft)
WINE_CONFIG_MAKEFILE(dlls/infosoft/tests)
WINE_CONFIG_MAKEFILE(dlls/initpki)
WINE_CONFIG_MAKEFILE(dlls/inkobj)
WINE_CONFIG_MAKEFILE(dlls/inseng)
WINE_CONFIG_MAKEFILE(dlls/iphlpapi)
WINE_CONFIG_MAKEFILE(dlls/iphlpapi/tests)
WINE_CONFIG_MAKEFILE(dlls/iprop)
WINE_CONFIG_MAKEFILE(dlls/irprops.cpl)
WINE_CONFIG_MAKEFILE(dlls/itircl)
WINE_CONFIG_MAKEFILE(dlls/itss)
WINE_CONFIG_MAKEFILE(dlls/itss/tests)
WINE_CONFIG_MAKEFILE(dlls/joy.cpl)
WINE_CONFIG_MAKEFILE(dlls/jscript)
WINE_CONFIG_MAKEFILE(dlls/jscript/tests)
WINE_CONFIG_MAKEFILE(dlls/jsproxy)
WINE_CONFIG_MAKEFILE(dlls/jsproxy/tests)
WINE_CONFIG_MAKEFILE(dlls/kerberos)
WINE_CONFIG_MAKEFILE(dlls/kernel32)
WINE_CONFIG_MAKEFILE(dlls/kernel32/tests)
WINE_CONFIG_MAKEFILE(dlls/kernelbase)
WINE_CONFIG_MAKEFILE(dlls/kernelbase/tests)
WINE_CONFIG_MAKEFILE(dlls/keyboard.drv16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/krnl386.exe16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/ksecdd.sys)
WINE_CONFIG_MAKEFILE(dlls/ksuser)
WINE_CONFIG_MAKEFILE(dlls/ktmw32)
WINE_CONFIG_MAKEFILE(dlls/l3codeca.acm)
WINE_CONFIG_MAKEFILE(dlls/loadperf)
WINE_CONFIG_MAKEFILE(dlls/localspl)
WINE_CONFIG_MAKEFILE(dlls/localspl/tests)
WINE_CONFIG_MAKEFILE(dlls/localui)
WINE_CONFIG_MAKEFILE(dlls/localui/tests)
WINE_CONFIG_MAKEFILE(dlls/lz32)
WINE_CONFIG_MAKEFILE(dlls/lz32/tests)
WINE_CONFIG_MAKEFILE(dlls/lzexpand.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/mapi32)
WINE_CONFIG_MAKEFILE(dlls/mapi32/tests)
WINE_CONFIG_MAKEFILE(dlls/mapistub)
WINE_CONFIG_MAKEFILE(dlls/mciavi32)
WINE_CONFIG_MAKEFILE(dlls/mcicda)
WINE_CONFIG_MAKEFILE(dlls/mciqtz32)
WINE_CONFIG_MAKEFILE(dlls/mciseq)
WINE_CONFIG_MAKEFILE(dlls/mciwave)
WINE_CONFIG_MAKEFILE(dlls/mf)
WINE_CONFIG_MAKEFILE(dlls/mf/tests)
WINE_CONFIG_MAKEFILE(dlls/mf3216)
WINE_CONFIG_MAKEFILE(dlls/mferror)
WINE_CONFIG_MAKEFILE(dlls/mfplat)
WINE_CONFIG_MAKEFILE(dlls/mfplat/tests)
WINE_CONFIG_MAKEFILE(dlls/mfplay)
WINE_CONFIG_MAKEFILE(dlls/mfreadwrite)
WINE_CONFIG_MAKEFILE(dlls/mfreadwrite/tests)
WINE_CONFIG_MAKEFILE(dlls/mfuuid)
WINE_CONFIG_MAKEFILE(dlls/mgmtapi)
WINE_CONFIG_MAKEFILE(dlls/midimap)
WINE_CONFIG_MAKEFILE(dlls/mlang)
WINE_CONFIG_MAKEFILE(dlls/mlang/tests)
WINE_CONFIG_MAKEFILE(dlls/mmcndmgr)
WINE_CONFIG_MAKEFILE(dlls/mmcndmgr/tests)
WINE_CONFIG_MAKEFILE(dlls/mmdevapi)
WINE_CONFIG_MAKEFILE(dlls/mmdevapi/tests)
WINE_CONFIG_MAKEFILE(dlls/mmdevldr.vxd,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/mmsystem.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/monodebg.vxd,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/mountmgr.sys)
WINE_CONFIG_MAKEFILE(dlls/mouse.drv16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/mp3dmod)
WINE_CONFIG_MAKEFILE(dlls/mp3dmod/tests)
WINE_CONFIG_MAKEFILE(dlls/mpr)
WINE_CONFIG_MAKEFILE(dlls/mpr/tests)
WINE_CONFIG_MAKEFILE(dlls/mprapi)
WINE_CONFIG_MAKEFILE(dlls/msacm.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/msacm32.drv)
WINE_CONFIG_MAKEFILE(dlls/msacm32)
WINE_CONFIG_MAKEFILE(dlls/msacm32/tests)
WINE_CONFIG_MAKEFILE(dlls/msadp32.acm)
WINE_CONFIG_MAKEFILE(dlls/msasn1)
WINE_CONFIG_MAKEFILE(dlls/mscat32)
WINE_CONFIG_MAKEFILE(dlls/mscms)
WINE_CONFIG_MAKEFILE(dlls/mscms/tests)
WINE_CONFIG_MAKEFILE(dlls/mscoree)
WINE_CONFIG_MAKEFILE(dlls/mscoree/tests)
WINE_CONFIG_MAKEFILE(dlls/mscorwks)
WINE_CONFIG_MAKEFILE(dlls/msctf)
WINE_CONFIG_MAKEFILE(dlls/msctf/tests)
WINE_CONFIG_MAKEFILE(dlls/msctfp)
WINE_CONFIG_MAKEFILE(dlls/msdaps)
WINE_CONFIG_MAKEFILE(dlls/msdelta)
WINE_CONFIG_MAKEFILE(dlls/msdmo)
WINE_CONFIG_MAKEFILE(dlls/msdmo/tests)
WINE_CONFIG_MAKEFILE(dlls/msdrm)
WINE_CONFIG_MAKEFILE(dlls/msftedit)
WINE_CONFIG_MAKEFILE(dlls/msftedit/tests)
WINE_CONFIG_MAKEFILE(dlls/msg711.acm)
WINE_CONFIG_MAKEFILE(dlls/msgsm32.acm)
WINE_CONFIG_MAKEFILE(dlls/mshtml.tlb)
WINE_CONFIG_MAKEFILE(dlls/mshtml)
WINE_CONFIG_MAKEFILE(dlls/mshtml/tests)
WINE_CONFIG_MAKEFILE(dlls/msi)
WINE_CONFIG_MAKEFILE(dlls/msi/tests)
WINE_CONFIG_MAKEFILE(dlls/msident)
WINE_CONFIG_MAKEFILE(dlls/msimg32)
WINE_CONFIG_MAKEFILE(dlls/msimsg)
WINE_CONFIG_MAKEFILE(dlls/msimtf)
WINE_CONFIG_MAKEFILE(dlls/msisip)
WINE_CONFIG_MAKEFILE(dlls/msisys.ocx)
WINE_CONFIG_MAKEFILE(dlls/msls31)
WINE_CONFIG_MAKEFILE(dlls/msnet32)
WINE_CONFIG_MAKEFILE(dlls/mspatcha)
WINE_CONFIG_MAKEFILE(dlls/mspatcha/tests)
WINE_CONFIG_MAKEFILE(dlls/msports)
WINE_CONFIG_MAKEFILE(dlls/msrle32)
WINE_CONFIG_MAKEFILE(dlls/msrle32/tests)
WINE_CONFIG_MAKEFILE(dlls/msscript.ocx)
WINE_CONFIG_MAKEFILE(dlls/msscript.ocx/tests)
WINE_CONFIG_MAKEFILE(dlls/mssign32)
WINE_CONFIG_MAKEFILE(dlls/mssip32)
WINE_CONFIG_MAKEFILE(dlls/mstask)
WINE_CONFIG_MAKEFILE(dlls/mstask/tests)
WINE_CONFIG_MAKEFILE(dlls/msvcirt)
WINE_CONFIG_MAKEFILE(dlls/msvcirt/tests)
WINE_CONFIG_MAKEFILE(dlls/msvcm80)
WINE_CONFIG_MAKEFILE(dlls/msvcm90)
WINE_CONFIG_MAKEFILE(dlls/msvcp100)
WINE_CONFIG_MAKEFILE(dlls/msvcp100/tests)
WINE_CONFIG_MAKEFILE(dlls/msvcp110)
WINE_CONFIG_MAKEFILE(dlls/msvcp110/tests)
WINE_CONFIG_MAKEFILE(dlls/msvcp120)
WINE_CONFIG_MAKEFILE(dlls/msvcp120/tests)
WINE_CONFIG_MAKEFILE(dlls/msvcp120_app)
WINE_CONFIG_MAKEFILE(dlls/msvcp140)
WINE_CONFIG_MAKEFILE(dlls/msvcp140/tests)
WINE_CONFIG_MAKEFILE(dlls/msvcp60)
WINE_CONFIG_MAKEFILE(dlls/msvcp60/tests)
WINE_CONFIG_MAKEFILE(dlls/msvcp70)
WINE_CONFIG_MAKEFILE(dlls/msvcp71)
WINE_CONFIG_MAKEFILE(dlls/msvcp80)
WINE_CONFIG_MAKEFILE(dlls/msvcp90)
WINE_CONFIG_MAKEFILE(dlls/msvcp90/tests)
WINE_CONFIG_MAKEFILE(dlls/msvcr100)
WINE_CONFIG_MAKEFILE(dlls/msvcr100/tests)
WINE_CONFIG_MAKEFILE(dlls/msvcr110)
WINE_CONFIG_MAKEFILE(dlls/msvcr110/tests)
WINE_CONFIG_MAKEFILE(dlls/msvcr120)
WINE_CONFIG_MAKEFILE(dlls/msvcr120/tests)
WINE_CONFIG_MAKEFILE(dlls/msvcr120_app)
WINE_CONFIG_MAKEFILE(dlls/msvcr70)
WINE_CONFIG_MAKEFILE(dlls/msvcr71)
WINE_CONFIG_MAKEFILE(dlls/msvcr80)
WINE_CONFIG_MAKEFILE(dlls/msvcr90)
WINE_CONFIG_MAKEFILE(dlls/msvcr90/tests)
WINE_CONFIG_MAKEFILE(dlls/msvcrt)
WINE_CONFIG_MAKEFILE(dlls/msvcrt/tests)
WINE_CONFIG_MAKEFILE(dlls/msvcrt20)
WINE_CONFIG_MAKEFILE(dlls/msvcrt40)
WINE_CONFIG_MAKEFILE(dlls/msvcrtd)
WINE_CONFIG_MAKEFILE(dlls/msvcrtd/tests)
WINE_CONFIG_MAKEFILE(dlls/msvfw32)
WINE_CONFIG_MAKEFILE(dlls/msvfw32/tests)
WINE_CONFIG_MAKEFILE(dlls/msvidc32)
WINE_CONFIG_MAKEFILE(dlls/msvideo.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/mswsock)
WINE_CONFIG_MAKEFILE(dlls/msxml)
WINE_CONFIG_MAKEFILE(dlls/msxml2)
WINE_CONFIG_MAKEFILE(dlls/msxml3)
WINE_CONFIG_MAKEFILE(dlls/msxml3/tests)
WINE_CONFIG_MAKEFILE(dlls/msxml4)
WINE_CONFIG_MAKEFILE(dlls/msxml6)
WINE_CONFIG_MAKEFILE(dlls/mtxdm)
WINE_CONFIG_MAKEFILE(dlls/ncrypt)
WINE_CONFIG_MAKEFILE(dlls/nddeapi)
WINE_CONFIG_MAKEFILE(dlls/ndis.sys)
WINE_CONFIG_MAKEFILE(dlls/netapi32)
WINE_CONFIG_MAKEFILE(dlls/netapi32/tests)
WINE_CONFIG_MAKEFILE(dlls/netcfgx)
WINE_CONFIG_MAKEFILE(dlls/netcfgx/tests)
WINE_CONFIG_MAKEFILE(dlls/netprofm)
WINE_CONFIG_MAKEFILE(dlls/netprofm/tests)
WINE_CONFIG_MAKEFILE(dlls/newdev)
WINE_CONFIG_MAKEFILE(dlls/ninput)
WINE_CONFIG_MAKEFILE(dlls/ninput/tests)
WINE_CONFIG_MAKEFILE(dlls/normaliz)
WINE_CONFIG_MAKEFILE(dlls/npmshtml)
WINE_CONFIG_MAKEFILE(dlls/npptools)
WINE_CONFIG_MAKEFILE(dlls/ntdll)
WINE_CONFIG_MAKEFILE(dlls/ntdll/tests)
WINE_CONFIG_MAKEFILE(dlls/ntdsapi)
WINE_CONFIG_MAKEFILE(dlls/ntdsapi/tests)
WINE_CONFIG_MAKEFILE(dlls/ntoskrnl.exe)
WINE_CONFIG_MAKEFILE(dlls/ntoskrnl.exe/tests)
WINE_CONFIG_MAKEFILE(dlls/ntprint)
WINE_CONFIG_MAKEFILE(dlls/ntprint/tests)
WINE_CONFIG_MAKEFILE(dlls/objsel)
WINE_CONFIG_MAKEFILE(dlls/odbc32)
WINE_CONFIG_MAKEFILE(dlls/odbccp32)
WINE_CONFIG_MAKEFILE(dlls/odbccp32/tests)
WINE_CONFIG_MAKEFILE(dlls/odbccu32)
WINE_CONFIG_MAKEFILE(dlls/ole2.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/ole2conv.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/ole2disp.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/ole2nls.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/ole2prox.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/ole2thk.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/ole32)
WINE_CONFIG_MAKEFILE(dlls/ole32/tests)
WINE_CONFIG_MAKEFILE(dlls/oleacc)
WINE_CONFIG_MAKEFILE(dlls/oleacc/tests)
WINE_CONFIG_MAKEFILE(dlls/oleaut32)
WINE_CONFIG_MAKEFILE(dlls/oleaut32/tests)
WINE_CONFIG_MAKEFILE(dlls/olecli.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/olecli32)
WINE_CONFIG_MAKEFILE(dlls/oledb32)
WINE_CONFIG_MAKEFILE(dlls/oledb32/tests)
WINE_CONFIG_MAKEFILE(dlls/oledlg)
WINE_CONFIG_MAKEFILE(dlls/oledlg/tests)
WINE_CONFIG_MAKEFILE(dlls/olepro32)
WINE_CONFIG_MAKEFILE(dlls/olesvr.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/olesvr32)
WINE_CONFIG_MAKEFILE(dlls/olethk32)
WINE_CONFIG_MAKEFILE(dlls/opcservices)
WINE_CONFIG_MAKEFILE(dlls/opcservices/tests)
WINE_CONFIG_MAKEFILE(dlls/openal32)
WINE_CONFIG_MAKEFILE(dlls/opencl)
WINE_CONFIG_MAKEFILE(dlls/opengl32)
WINE_CONFIG_MAKEFILE(dlls/opengl32/tests)
WINE_CONFIG_MAKEFILE(dlls/packager)
WINE_CONFIG_MAKEFILE(dlls/packager/tests)
WINE_CONFIG_MAKEFILE(dlls/pdh)
WINE_CONFIG_MAKEFILE(dlls/pdh/tests)
WINE_CONFIG_MAKEFILE(dlls/photometadatahandler)
WINE_CONFIG_MAKEFILE(dlls/pidgen)
WINE_CONFIG_MAKEFILE(dlls/powrprof)
WINE_CONFIG_MAKEFILE(dlls/printui)
WINE_CONFIG_MAKEFILE(dlls/prntvpt)
WINE_CONFIG_MAKEFILE(dlls/propsys)
WINE_CONFIG_MAKEFILE(dlls/propsys/tests)
WINE_CONFIG_MAKEFILE(dlls/psapi)
WINE_CONFIG_MAKEFILE(dlls/psapi/tests)
WINE_CONFIG_MAKEFILE(dlls/pstorec)
WINE_CONFIG_MAKEFILE(dlls/pstorec/tests)
WINE_CONFIG_MAKEFILE(dlls/qcap)
WINE_CONFIG_MAKEFILE(dlls/qcap/tests)
WINE_CONFIG_MAKEFILE(dlls/qedit)
WINE_CONFIG_MAKEFILE(dlls/qedit/tests)
WINE_CONFIG_MAKEFILE(dlls/qmgr)
WINE_CONFIG_MAKEFILE(dlls/qmgr/tests)
WINE_CONFIG_MAKEFILE(dlls/qmgrprxy)
WINE_CONFIG_MAKEFILE(dlls/quartz)
WINE_CONFIG_MAKEFILE(dlls/quartz/tests)
WINE_CONFIG_MAKEFILE(dlls/query)
WINE_CONFIG_MAKEFILE(dlls/qwave)
WINE_CONFIG_MAKEFILE(dlls/qwave/tests)
WINE_CONFIG_MAKEFILE(dlls/rasapi16.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/rasapi32)
WINE_CONFIG_MAKEFILE(dlls/rasapi32/tests)
WINE_CONFIG_MAKEFILE(dlls/rasdlg)
WINE_CONFIG_MAKEFILE(dlls/regapi)
WINE_CONFIG_MAKEFILE(dlls/resutils)
WINE_CONFIG_MAKEFILE(dlls/riched20)
WINE_CONFIG_MAKEFILE(dlls/riched20/tests)
WINE_CONFIG_MAKEFILE(dlls/riched32)
WINE_CONFIG_MAKEFILE(dlls/riched32/tests)
WINE_CONFIG_MAKEFILE(dlls/rpcrt4)
WINE_CONFIG_MAKEFILE(dlls/rpcrt4/tests)
WINE_CONFIG_MAKEFILE(dlls/rsabase)
WINE_CONFIG_MAKEFILE(dlls/rsaenh)
WINE_CONFIG_MAKEFILE(dlls/rsaenh/tests)
WINE_CONFIG_MAKEFILE(dlls/rstrtmgr)
WINE_CONFIG_MAKEFILE(dlls/rtutils)
WINE_CONFIG_MAKEFILE(dlls/samlib)
WINE_CONFIG_MAKEFILE(dlls/sane.ds)
WINE_CONFIG_MAKEFILE(dlls/sapi)
WINE_CONFIG_MAKEFILE(dlls/sapi/tests)
WINE_CONFIG_MAKEFILE(dlls/sas)
WINE_CONFIG_MAKEFILE(dlls/scarddlg)
WINE_CONFIG_MAKEFILE(dlls/sccbase)
WINE_CONFIG_MAKEFILE(dlls/schannel)
WINE_CONFIG_MAKEFILE(dlls/schannel/tests)
WINE_CONFIG_MAKEFILE(dlls/schedsvc)
WINE_CONFIG_MAKEFILE(dlls/schedsvc/tests)
WINE_CONFIG_MAKEFILE(dlls/scrobj)
WINE_CONFIG_MAKEFILE(dlls/scrrun)
WINE_CONFIG_MAKEFILE(dlls/scrrun/tests)
WINE_CONFIG_MAKEFILE(dlls/scsiport.sys)
WINE_CONFIG_MAKEFILE(dlls/secur32)
WINE_CONFIG_MAKEFILE(dlls/secur32/tests)
WINE_CONFIG_MAKEFILE(dlls/security)
WINE_CONFIG_MAKEFILE(dlls/sensapi)
WINE_CONFIG_MAKEFILE(dlls/serialui)
WINE_CONFIG_MAKEFILE(dlls/serialui/tests)
WINE_CONFIG_MAKEFILE(dlls/setupapi)
WINE_CONFIG_MAKEFILE(dlls/setupapi/tests)
WINE_CONFIG_MAKEFILE(dlls/setupx.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/sfc)
WINE_CONFIG_MAKEFILE(dlls/sfc_os)
WINE_CONFIG_MAKEFILE(dlls/shcore)
WINE_CONFIG_MAKEFILE(dlls/shcore/tests)
WINE_CONFIG_MAKEFILE(dlls/shdoclc)
WINE_CONFIG_MAKEFILE(dlls/shdocvw)
WINE_CONFIG_MAKEFILE(dlls/shdocvw/tests)
WINE_CONFIG_MAKEFILE(dlls/shell.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/shell32)
WINE_CONFIG_MAKEFILE(dlls/shell32/tests)
WINE_CONFIG_MAKEFILE(dlls/shfolder)
WINE_CONFIG_MAKEFILE(dlls/shlwapi)
WINE_CONFIG_MAKEFILE(dlls/shlwapi/tests)
WINE_CONFIG_MAKEFILE(dlls/slbcsp)
WINE_CONFIG_MAKEFILE(dlls/slc)
WINE_CONFIG_MAKEFILE(dlls/slc/tests)
WINE_CONFIG_MAKEFILE(dlls/snmpapi)
WINE_CONFIG_MAKEFILE(dlls/snmpapi/tests)
WINE_CONFIG_MAKEFILE(dlls/softpub)
WINE_CONFIG_MAKEFILE(dlls/sound.drv16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/spoolss)
WINE_CONFIG_MAKEFILE(dlls/spoolss/tests)
WINE_CONFIG_MAKEFILE(dlls/srclient)
WINE_CONFIG_MAKEFILE(dlls/sspicli)
WINE_CONFIG_MAKEFILE(dlls/stdole2.tlb)
WINE_CONFIG_MAKEFILE(dlls/stdole32.tlb)
WINE_CONFIG_MAKEFILE(dlls/sti)
WINE_CONFIG_MAKEFILE(dlls/sti/tests)
WINE_CONFIG_MAKEFILE(dlls/storage.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/stress.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/strmbase)
WINE_CONFIG_MAKEFILE(dlls/strmdll)
WINE_CONFIG_MAKEFILE(dlls/strmiids)
WINE_CONFIG_MAKEFILE(dlls/svrapi)
WINE_CONFIG_MAKEFILE(dlls/sxs)
WINE_CONFIG_MAKEFILE(dlls/sxs/tests)
WINE_CONFIG_MAKEFILE(dlls/system.drv16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/t2embed)
WINE_CONFIG_MAKEFILE(dlls/t2embed/tests)
WINE_CONFIG_MAKEFILE(dlls/tapi32)
WINE_CONFIG_MAKEFILE(dlls/taskschd)
WINE_CONFIG_MAKEFILE(dlls/taskschd/tests)
WINE_CONFIG_MAKEFILE(dlls/tdh)
WINE_CONFIG_MAKEFILE(dlls/tdi.sys)
WINE_CONFIG_MAKEFILE(dlls/toolhelp.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/traffic)
WINE_CONFIG_MAKEFILE(dlls/twain.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/twain_32)
WINE_CONFIG_MAKEFILE(dlls/twain_32/tests)
WINE_CONFIG_MAKEFILE(dlls/typelib.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/tzres)
WINE_CONFIG_MAKEFILE(dlls/ucrtbase)
WINE_CONFIG_MAKEFILE(dlls/ucrtbase/tests)
WINE_CONFIG_MAKEFILE(dlls/uianimation)
WINE_CONFIG_MAKEFILE(dlls/uiautomationcore)
WINE_CONFIG_MAKEFILE(dlls/uiautomationcore/tests)
WINE_CONFIG_MAKEFILE(dlls/uiribbon)
WINE_CONFIG_MAKEFILE(dlls/unicows)
WINE_CONFIG_MAKEFILE(dlls/updspapi)
WINE_CONFIG_MAKEFILE(dlls/url)
WINE_CONFIG_MAKEFILE(dlls/urlmon)
WINE_CONFIG_MAKEFILE(dlls/urlmon/tests)
WINE_CONFIG_MAKEFILE(dlls/usbd.sys)
WINE_CONFIG_MAKEFILE(dlls/user.exe16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/user32)
WINE_CONFIG_MAKEFILE(dlls/user32/tests)
WINE_CONFIG_MAKEFILE(dlls/userenv)
WINE_CONFIG_MAKEFILE(dlls/userenv/tests)
WINE_CONFIG_MAKEFILE(dlls/usp10)
WINE_CONFIG_MAKEFILE(dlls/usp10/tests)
WINE_CONFIG_MAKEFILE(dlls/uuid)
WINE_CONFIG_MAKEFILE(dlls/uxtheme)
WINE_CONFIG_MAKEFILE(dlls/uxtheme/tests)
WINE_CONFIG_MAKEFILE(dlls/vbscript)
WINE_CONFIG_MAKEFILE(dlls/vbscript/tests)
WINE_CONFIG_MAKEFILE(dlls/vcomp)
WINE_CONFIG_MAKEFILE(dlls/vcomp/tests)
WINE_CONFIG_MAKEFILE(dlls/vcomp100)
WINE_CONFIG_MAKEFILE(dlls/vcomp110)
WINE_CONFIG_MAKEFILE(dlls/vcomp120)
WINE_CONFIG_MAKEFILE(dlls/vcomp140)
WINE_CONFIG_MAKEFILE(dlls/vcomp90)
WINE_CONFIG_MAKEFILE(dlls/vcruntime140)
WINE_CONFIG_MAKEFILE(dlls/vdhcp.vxd,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/vdmdbg)
WINE_CONFIG_MAKEFILE(dlls/ver.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/version)
WINE_CONFIG_MAKEFILE(dlls/version/tests)
WINE_CONFIG_MAKEFILE(dlls/virtdisk)
WINE_CONFIG_MAKEFILE(dlls/virtdisk/tests)
WINE_CONFIG_MAKEFILE(dlls/vmm.vxd,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/vnbt.vxd,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/vnetbios.vxd,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/vssapi)
WINE_CONFIG_MAKEFILE(dlls/vtdapi.vxd,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/vulkan-1)
WINE_CONFIG_MAKEFILE(dlls/vulkan-1/tests)
WINE_CONFIG_MAKEFILE(dlls/vwin32.vxd,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/w32skrnl,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/w32sys.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/wbemdisp)
WINE_CONFIG_MAKEFILE(dlls/wbemdisp/tests)
WINE_CONFIG_MAKEFILE(dlls/wbemprox)
WINE_CONFIG_MAKEFILE(dlls/wbemprox/tests)
WINE_CONFIG_MAKEFILE(dlls/wdscore)
WINE_CONFIG_MAKEFILE(dlls/webservices)
WINE_CONFIG_MAKEFILE(dlls/webservices/tests)
WINE_CONFIG_MAKEFILE(dlls/wer)
WINE_CONFIG_MAKEFILE(dlls/wer/tests)
WINE_CONFIG_MAKEFILE(dlls/wevtapi)
WINE_CONFIG_MAKEFILE(dlls/wiaservc)
WINE_CONFIG_MAKEFILE(dlls/wiaservc/tests)
WINE_CONFIG_MAKEFILE(dlls/wimgapi)
WINE_CONFIG_MAKEFILE(dlls/win32s16.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/win87em.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/winaspi.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/windebug.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/windowscodecs)
WINE_CONFIG_MAKEFILE(dlls/windowscodecs/tests)
WINE_CONFIG_MAKEFILE(dlls/windowscodecsext)
WINE_CONFIG_MAKEFILE(dlls/windowscodecsext/tests)
WINE_CONFIG_MAKEFILE(dlls/winealsa.drv)
WINE_CONFIG_MAKEFILE(dlls/wineandroid.drv)
WINE_CONFIG_MAKEFILE(dlls/winebus.sys)
WINE_CONFIG_MAKEFILE(dlls/winecoreaudio.drv)
WINE_CONFIG_MAKEFILE(dlls/winecrt0)
WINE_CONFIG_MAKEFILE(dlls/wined3d)
WINE_CONFIG_MAKEFILE(dlls/winegstreamer)
WINE_CONFIG_MAKEFILE(dlls/winehid.sys)
WINE_CONFIG_MAKEFILE(dlls/winejoystick.drv)
WINE_CONFIG_MAKEFILE(dlls/winemac.drv)
WINE_CONFIG_MAKEFILE(dlls/winemapi)
WINE_CONFIG_MAKEFILE(dlls/wineoss.drv)
WINE_CONFIG_MAKEFILE(dlls/wineps.drv)
WINE_CONFIG_MAKEFILE(dlls/wineps16.drv16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/winepulse.drv)
WINE_CONFIG_MAKEFILE(dlls/wineqtdecoder)
WINE_CONFIG_MAKEFILE(dlls/winevulkan)
WINE_CONFIG_MAKEFILE(dlls/winex11.drv)
WINE_CONFIG_MAKEFILE(dlls/wing.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/wing32)
WINE_CONFIG_MAKEFILE(dlls/winhttp)
WINE_CONFIG_MAKEFILE(dlls/winhttp/tests)
WINE_CONFIG_MAKEFILE(dlls/wininet)
WINE_CONFIG_MAKEFILE(dlls/wininet/tests)
WINE_CONFIG_MAKEFILE(dlls/winmm)
WINE_CONFIG_MAKEFILE(dlls/winmm/tests)
WINE_CONFIG_MAKEFILE(dlls/winnls.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/winnls32)
WINE_CONFIG_MAKEFILE(dlls/winscard)
WINE_CONFIG_MAKEFILE(dlls/winsock.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/winspool.drv)
WINE_CONFIG_MAKEFILE(dlls/winspool.drv/tests)
WINE_CONFIG_MAKEFILE(dlls/winsta)
WINE_CONFIG_MAKEFILE(dlls/wintab.dll16,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/wintab32)
WINE_CONFIG_MAKEFILE(dlls/wintab32/tests)
WINE_CONFIG_MAKEFILE(dlls/wintrust)
WINE_CONFIG_MAKEFILE(dlls/wintrust/tests)
WINE_CONFIG_MAKEFILE(dlls/winusb)
WINE_CONFIG_MAKEFILE(dlls/wlanapi)
WINE_CONFIG_MAKEFILE(dlls/wlanapi/tests)
WINE_CONFIG_MAKEFILE(dlls/wlanui)
WINE_CONFIG_MAKEFILE(dlls/wldap32)
WINE_CONFIG_MAKEFILE(dlls/wldap32/tests)
WINE_CONFIG_MAKEFILE(dlls/wmasf)
WINE_CONFIG_MAKEFILE(dlls/wmcodecdspuuid)
WINE_CONFIG_MAKEFILE(dlls/wmi)
WINE_CONFIG_MAKEFILE(dlls/wmiutils)
WINE_CONFIG_MAKEFILE(dlls/wmiutils/tests)
WINE_CONFIG_MAKEFILE(dlls/wmp)
WINE_CONFIG_MAKEFILE(dlls/wmp/tests)
WINE_CONFIG_MAKEFILE(dlls/wmphoto)
WINE_CONFIG_MAKEFILE(dlls/wmvcore)
WINE_CONFIG_MAKEFILE(dlls/wmvcore/tests)
WINE_CONFIG_MAKEFILE(dlls/wnaspi32)
WINE_CONFIG_MAKEFILE(dlls/wow32,enable_win16)
WINE_CONFIG_MAKEFILE(dlls/wpc)
WINE_CONFIG_MAKEFILE(dlls/wpc/tests)
WINE_CONFIG_MAKEFILE(dlls/wpcap)
WINE_CONFIG_MAKEFILE(dlls/ws2_32)
WINE_CONFIG_MAKEFILE(dlls/ws2_32/tests)
WINE_CONFIG_MAKEFILE(dlls/wsdapi)
WINE_CONFIG_MAKEFILE(dlls/wsdapi/tests)
WINE_CONFIG_MAKEFILE(dlls/wshom.ocx)
WINE_CONFIG_MAKEFILE(dlls/wshom.ocx/tests)
WINE_CONFIG_MAKEFILE(dlls/wsnmp32)
WINE_CONFIG_MAKEFILE(dlls/wsnmp32/tests)
WINE_CONFIG_MAKEFILE(dlls/wsock32)
WINE_CONFIG_MAKEFILE(dlls/wtsapi32)
WINE_CONFIG_MAKEFILE(dlls/wtsapi32/tests)
WINE_CONFIG_MAKEFILE(dlls/wuapi)
WINE_CONFIG_MAKEFILE(dlls/wuaueng)
WINE_CONFIG_MAKEFILE(dlls/x3daudio1_0)
WINE_CONFIG_MAKEFILE(dlls/x3daudio1_1)
WINE_CONFIG_MAKEFILE(dlls/x3daudio1_2)
WINE_CONFIG_MAKEFILE(dlls/x3daudio1_3)
WINE_CONFIG_MAKEFILE(dlls/x3daudio1_4)
WINE_CONFIG_MAKEFILE(dlls/x3daudio1_5)
WINE_CONFIG_MAKEFILE(dlls/x3daudio1_6)
WINE_CONFIG_MAKEFILE(dlls/x3daudio1_7)
WINE_CONFIG_MAKEFILE(dlls/xapofx1_1)
WINE_CONFIG_MAKEFILE(dlls/xapofx1_2)
WINE_CONFIG_MAKEFILE(dlls/xapofx1_3)
WINE_CONFIG_MAKEFILE(dlls/xapofx1_4)
WINE_CONFIG_MAKEFILE(dlls/xapofx1_5)
WINE_CONFIG_MAKEFILE(dlls/xaudio2_0)
WINE_CONFIG_MAKEFILE(dlls/xaudio2_1)
WINE_CONFIG_MAKEFILE(dlls/xaudio2_2)
WINE_CONFIG_MAKEFILE(dlls/xaudio2_3)
WINE_CONFIG_MAKEFILE(dlls/xaudio2_4)
WINE_CONFIG_MAKEFILE(dlls/xaudio2_5)
WINE_CONFIG_MAKEFILE(dlls/xaudio2_6)
WINE_CONFIG_MAKEFILE(dlls/xaudio2_7)
WINE_CONFIG_MAKEFILE(dlls/xaudio2_7/tests)
WINE_CONFIG_MAKEFILE(dlls/xaudio2_8)
WINE_CONFIG_MAKEFILE(dlls/xaudio2_9)
WINE_CONFIG_MAKEFILE(dlls/xinput1_1)
WINE_CONFIG_MAKEFILE(dlls/xinput1_2)
WINE_CONFIG_MAKEFILE(dlls/xinput1_3)
WINE_CONFIG_MAKEFILE(dlls/xinput1_3/tests)
WINE_CONFIG_MAKEFILE(dlls/xinput1_4)
WINE_CONFIG_MAKEFILE(dlls/xinput9_1_0)
WINE_CONFIG_MAKEFILE(dlls/xmllite)
WINE_CONFIG_MAKEFILE(dlls/xmllite/tests)
WINE_CONFIG_MAKEFILE(dlls/xolehlp)
WINE_CONFIG_MAKEFILE(dlls/xpsprint)
WINE_CONFIG_MAKEFILE(dlls/xpssvcs)
WINE_CONFIG_MAKEFILE(fonts)
WINE_CONFIG_MAKEFILE(include)
WINE_CONFIG_MAKEFILE(libs/port)
WINE_CONFIG_MAKEFILE(libs/wine)
WINE_CONFIG_MAKEFILE(libs/wpp)
WINE_CONFIG_MAKEFILE(loader)
WINE_CONFIG_MAKEFILE(po)
WINE_CONFIG_MAKEFILE(programs/arp)
WINE_CONFIG_MAKEFILE(programs/aspnet_regiis)
WINE_CONFIG_MAKEFILE(programs/attrib)
WINE_CONFIG_MAKEFILE(programs/cabarc)
WINE_CONFIG_MAKEFILE(programs/cacls)
WINE_CONFIG_MAKEFILE(programs/clock)
WINE_CONFIG_MAKEFILE(programs/cmd)
WINE_CONFIG_MAKEFILE(programs/cmd/tests)
WINE_CONFIG_MAKEFILE(programs/conhost)
WINE_CONFIG_MAKEFILE(programs/control)
WINE_CONFIG_MAKEFILE(programs/cscript)
WINE_CONFIG_MAKEFILE(programs/dism)
WINE_CONFIG_MAKEFILE(programs/dpnsvr)
WINE_CONFIG_MAKEFILE(programs/dxdiag)
WINE_CONFIG_MAKEFILE(programs/eject)
WINE_CONFIG_MAKEFILE(programs/expand)
WINE_CONFIG_MAKEFILE(programs/explorer)
WINE_CONFIG_MAKEFILE(programs/extrac32)
WINE_CONFIG_MAKEFILE(programs/fc)
WINE_CONFIG_MAKEFILE(programs/find)
WINE_CONFIG_MAKEFILE(programs/findstr)
WINE_CONFIG_MAKEFILE(programs/fsutil)
WINE_CONFIG_MAKEFILE(programs/hh)
WINE_CONFIG_MAKEFILE(programs/hostname)
WINE_CONFIG_MAKEFILE(programs/icacls)
WINE_CONFIG_MAKEFILE(programs/icinfo)
WINE_CONFIG_MAKEFILE(programs/iexplore)
WINE_CONFIG_MAKEFILE(programs/ipconfig)
WINE_CONFIG_MAKEFILE(programs/lodctr)
WINE_CONFIG_MAKEFILE(programs/mofcomp)
WINE_CONFIG_MAKEFILE(programs/mshta)
WINE_CONFIG_MAKEFILE(programs/msidb)
WINE_CONFIG_MAKEFILE(programs/msiexec)
WINE_CONFIG_MAKEFILE(programs/msinfo32)
WINE_CONFIG_MAKEFILE(programs/net)
WINE_CONFIG_MAKEFILE(programs/netsh)
WINE_CONFIG_MAKEFILE(programs/netstat)
WINE_CONFIG_MAKEFILE(programs/ngen)
WINE_CONFIG_MAKEFILE(programs/notepad)
WINE_CONFIG_MAKEFILE(programs/oleview)
WINE_CONFIG_MAKEFILE(programs/ping)
WINE_CONFIG_MAKEFILE(programs/plugplay)
WINE_CONFIG_MAKEFILE(programs/powershell)
WINE_CONFIG_MAKEFILE(programs/presentationfontcache)
WINE_CONFIG_MAKEFILE(programs/progman)
WINE_CONFIG_MAKEFILE(programs/reg)
WINE_CONFIG_MAKEFILE(programs/reg/tests)
WINE_CONFIG_MAKEFILE(programs/regasm)
WINE_CONFIG_MAKEFILE(programs/regedit)
WINE_CONFIG_MAKEFILE(programs/regedit/tests)
WINE_CONFIG_MAKEFILE(programs/regsvcs)
WINE_CONFIG_MAKEFILE(programs/regsvr32)
WINE_CONFIG_MAKEFILE(programs/rpcss)
WINE_CONFIG_MAKEFILE(programs/rundll.exe16,enable_win16)
WINE_CONFIG_MAKEFILE(programs/rundll32)
WINE_CONFIG_MAKEFILE(programs/sc)
WINE_CONFIG_MAKEFILE(programs/schtasks)
WINE_CONFIG_MAKEFILE(programs/schtasks/tests)
WINE_CONFIG_MAKEFILE(programs/sdbinst)
WINE_CONFIG_MAKEFILE(programs/secedit)
WINE_CONFIG_MAKEFILE(programs/servicemodelreg)
WINE_CONFIG_MAKEFILE(programs/services)
WINE_CONFIG_MAKEFILE(programs/services/tests)
WINE_CONFIG_MAKEFILE(programs/shutdown)
WINE_CONFIG_MAKEFILE(programs/spoolsv)
WINE_CONFIG_MAKEFILE(programs/start)
WINE_CONFIG_MAKEFILE(programs/subst)
WINE_CONFIG_MAKEFILE(programs/svchost)
WINE_CONFIG_MAKEFILE(programs/systeminfo)
WINE_CONFIG_MAKEFILE(programs/taskkill)
WINE_CONFIG_MAKEFILE(programs/tasklist)
WINE_CONFIG_MAKEFILE(programs/taskmgr)
WINE_CONFIG_MAKEFILE(programs/termsv)
WINE_CONFIG_MAKEFILE(programs/uninstaller)
WINE_CONFIG_MAKEFILE(programs/unlodctr)
WINE_CONFIG_MAKEFILE(programs/view)
WINE_CONFIG_MAKEFILE(programs/wevtutil)
WINE_CONFIG_MAKEFILE(programs/wineboot)
WINE_CONFIG_MAKEFILE(programs/winebrowser)
WINE_CONFIG_MAKEFILE(programs/winecfg)
WINE_CONFIG_MAKEFILE(programs/wineconsole)
WINE_CONFIG_MAKEFILE(programs/winedbg)
WINE_CONFIG_MAKEFILE(programs/winedevice)
WINE_CONFIG_MAKEFILE(programs/winefile)
WINE_CONFIG_MAKEFILE(programs/winemenubuilder)
WINE_CONFIG_MAKEFILE(programs/winemine)
WINE_CONFIG_MAKEFILE(programs/winemsibuilder)
WINE_CONFIG_MAKEFILE(programs/winepath)
WINE_CONFIG_MAKEFILE(programs/winetest)
WINE_CONFIG_MAKEFILE(programs/winevdm,enable_win16)
WINE_CONFIG_MAKEFILE(programs/winhelp.exe16,enable_win16)
WINE_CONFIG_MAKEFILE(programs/winhlp32)
WINE_CONFIG_MAKEFILE(programs/winmgmt)
WINE_CONFIG_MAKEFILE(programs/winoldap.mod16,enable_win16)
WINE_CONFIG_MAKEFILE(programs/winver)
WINE_CONFIG_MAKEFILE(programs/wmic)
WINE_CONFIG_MAKEFILE(programs/wmplayer)
WINE_CONFIG_MAKEFILE(programs/wordpad)
WINE_CONFIG_MAKEFILE(programs/write)
WINE_CONFIG_MAKEFILE(programs/wscript)
WINE_CONFIG_MAKEFILE(programs/wscript/tests)
WINE_CONFIG_MAKEFILE(programs/wusa)
WINE_CONFIG_MAKEFILE(programs/xcopy)
WINE_CONFIG_MAKEFILE(programs/xcopy/tests)
WINE_CONFIG_MAKEFILE(server)
WINE_CONFIG_MAKEFILE(tools,,[test "x$enable_tools" = xno])
WINE_CONFIG_MAKEFILE(tools/sfnt2fon,,[test "x$enable_tools" = xno])
WINE_CONFIG_MAKEFILE(tools/widl,,[test "x$enable_tools" = xno])
WINE_CONFIG_MAKEFILE(tools/winebuild,,[test "x$enable_tools" = xno])
WINE_CONFIG_MAKEFILE(tools/winedump,,[test "x$enable_tools" = xno])
WINE_CONFIG_MAKEFILE(tools/winegcc,,[test "x$enable_tools" = xno])
WINE_CONFIG_MAKEFILE(tools/winemaker,,[test "x$enable_tools" = xno])
WINE_CONFIG_MAKEFILE(tools/wmc,,[test "x$enable_tools" = xno])
WINE_CONFIG_MAKEFILE(tools/wrc,,[test "x$enable_tools" = xno])
dnl End of auto-generated output commands

dnl Main ignore file

WINE_IGNORE_FILE("TAGS")
WINE_IGNORE_FILE("tags")
WINE_IGNORE_FILE("autom4te.cache")
WINE_IGNORE_FILE("config.log")
WINE_IGNORE_FILE("config.status")
WINE_IGNORE_FILE("include/config.h")
WINE_IGNORE_FILE("include/stamp-h")
test "$wine_binary" = wine || WINE_IGNORE_FILE("loader/wine")

if test "x$enable_tools" != xno
then
    WINE_IGNORE_FILE("tools/makedep$ac_exeext")
    AC_CONFIG_COMMANDS([tools/makedep],[wine_fn_output_makedep || AS_EXIT],
[wine_fn_output_makedep ()
{
    AS_MKDIR_P(tools)
    $CC -Iinclude -I$srcdir/include -D__WINESRC__ $EXTRACFLAGS $CPPFLAGS $CFLAGS -o tools/makedep$ac_exeext $srcdir/tools/makedep.c $LDFLAGS
}])
fi

AC_CONFIG_COMMANDS([Makefile], [wine_fn_output_makefile Makefile],
[wine_fn_output_makefile ()
{
    cat Make.tmp - <<\_WINE_EOF >\$tmp/makefile && mv -f \$tmp/makefile \$[]1 && rm -f Make.tmp && "$wine_makedep" && return
`cat $wine_rules_file`
_WINE_EOF
    AS_ERROR([could not create Makefile])
}])

AC_CONFIG_FILES([Make.tmp:Makefile.in])

dnl Some final makefile rules

if test "x$enable_maintainer_mode" = xyes
then
    WINE_APPEND_RULE(
[configure: configure.ac aclocal.m4
	autoconf --warnings=all
include/config.h.in: include/stamp-h.in
include/stamp-h.in: configure.ac aclocal.m4
	autoheader --warnings=all
	@echo timestamp > \$[@]])
fi

if test "x$enable_tools" != xno
then
    WINE_APPEND_RULE(
[tools/makedep$ac_exeext: $srcdir/tools/makedep.c include/config.h config.status
	@./config.status tools/makedep
Makefile: tools/makedep$ac_exeext
fonts: tools/sfnt2fon
include: tools tools/widl])
else
    WINE_APPEND_RULE(
[\$(MAKEDEP):
	@echo \"You need to run make in $TOOLSDIR first\" && false])
fi

if test -n "$with_wine64"
then
    case "$with_wine64" in
      /*) reldir="" ;;
      *)  reldir="../" ;;
    esac
    dnl Get rid of old symlinks
    rm -f fonts server 2>/dev/null
    WINE_APPEND_RULE(
[all: loader/wine64 loader/wine64-preloader $with_wine64/loader/wine $with_wine64/loader/wine-preloader
loader/wine64 loader/wine64-preloader:
	rm -f \$[@] && \$(LN_S) $reldir$with_wine64/\$[@] \$[@]
$with_wine64/loader/wine:
	rm -f \$[@] && \$(LN_S) $ac_pwd/loader/wine \$[@]
$with_wine64/loader/wine-preloader:
	rm -f \$[@] && \$(LN_S) $ac_pwd/loader/wine-preloader \$[@]
clean::
	rm -f loader/wine64 loader/wine64-preloader $with_wine64/loader/wine $with_wine64/loader/wine-preloader])
else
    TOP_INSTALL_DEV="$TOP_INSTALL_DEV include"
    TOP_INSTALL_LIB="$TOP_INSTALL_LIB \
fonts \
loader/l_intl.nls \
loader/wine.inf \
loader/winehid.inf \
programs/msidb/msidb \
programs/msiexec/msiexec \
programs/notepad/notepad \
programs/regedit/regedit \
programs/regsvr32/regsvr32 \
programs/wineboot/wineboot \
programs/winecfg/winecfg \
programs/wineconsole/wineconsole \
programs/winedbg/winedbg \
programs/winefile/winefile \
programs/winemine/winemine \
programs/winepath/winepath \
server/wineserver"

    case $host_os in
      cygwin*|mingw32*|darwin*|macosx*|linux-android*) ;;
      *) TOP_INSTALL_LIB="$TOP_INSTALL_LIB loader/wine.desktop" ;;
    esac
fi

AS_ECHO(" done") >&AS_MESSAGE_FD
AC_OUTPUT

if test "$no_create" = "yes"
then
  exit 0
fi

WINE_PRINT_MESSAGES

AS_ECHO("
$as_me: Finished.  Do '${ac_make}' to compile Wine.
") >&AS_MESSAGE_FD

dnl Local Variables:
dnl comment-start: "dnl "
dnl comment-end: ""
dnl comment-start-skip: "\\bdnl\\b\\s *"
dnl compile-command: "autoreconf --warnings=all"
dnl End:
