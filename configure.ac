dnl Process this file with autoconf to produce a configure script.
dnl Original author: Michael Patra
dnl For detailed change history, see the git commit logs.

m4_define(WINE_VERSION,regexp(m4_include(VERSION),[version \([-.0-9A-Za-z]+\)],[\1]))

dnl autoconf versions before 2.63b don't have AS_VAR_APPEND or AS_VAR_IF
AC_PREREQ(2.63b)
AC_INIT([Wine],[WINE_VERSION],[wine-devel@winehq.org],[wine],[https://www.winehq.org])
AC_CONFIG_SRCDIR(server/atom.c)
AC_CONFIG_HEADERS(include/config.h)
AC_CONFIG_AUX_DIR(tools)

libwine_version="1.0"

dnl **** Command-line arguments ****

AC_ARG_ENABLE(archs, AS_HELP_STRING([--enable-archs={i386,x86_64,arm,aarch64}],[enable multiple architectures for PE compilation]))
AC_ARG_ENABLE(win16, AS_HELP_STRING([--disable-win16],[do not include Win16 support]))
AC_ARG_ENABLE(win64, AS_HELP_STRING([--enable-win64],[build a Win64 emulator on AMD64 (won't run Win32 binaries)]))
AC_ARG_ENABLE(tests, AS_HELP_STRING([--disable-tests],[do not build the regression tests]))
AC_ARG_ENABLE(build-id, AS_HELP_STRING([--enable-build-id],[include .buildid section in output objects]))
AC_ARG_ENABLE(maintainer-mode, AS_HELP_STRING([--enable-maintainer-mode],[enable maintainer-specific build rules]))
AC_ARG_ENABLE(sast,  AS_HELP_STRING([--enable-sast],[enable static application security testing using Clang]))
AC_ARG_ENABLE(silent-rules, AS_HELP_STRING([--enable-silent-rules],[use silent build rules (override: "make V=1")]))
AC_ARG_ENABLE(werror, AS_HELP_STRING([--enable-werror],[treat compilation warnings as errors]))

AC_ARG_WITH(alsa,      AS_HELP_STRING([--without-alsa],[do not use the Alsa sound support]))
AC_ARG_WITH(capi,      AS_HELP_STRING([--without-capi],[do not use CAPI (ISDN support)]))
AC_ARG_WITH(coreaudio, AS_HELP_STRING([--without-coreaudio],[do not use the CoreAudio sound support]))
AC_ARG_WITH(cups,      AS_HELP_STRING([--without-cups],[do not use CUPS]))
AC_ARG_WITH(dbus,      AS_HELP_STRING([--without-dbus],[do not use DBus (dynamic device support)]))
AC_ARG_WITH(ffmpeg,    AS_HELP_STRING([--without-ffmpeg],[do not use the FFmpeg library]))
AC_ARG_WITH(fontconfig,AS_HELP_STRING([--without-fontconfig],[do not use fontconfig]))
AC_ARG_WITH(freetype,  AS_HELP_STRING([--without-freetype],[do not use the FreeType library]))
AC_ARG_WITH(gettext,   AS_HELP_STRING([--without-gettext],[do not use gettext]))
AC_ARG_WITH(gettextpo, AS_HELP_STRING([--with-gettextpo],[use the GetTextPO library to rebuild po files]),
            [if test "x$withval" = "xno"; then ac_cv_header_gettext_po_h=no; fi])
AC_ARG_WITH(gphoto,    AS_HELP_STRING([--without-gphoto],[do not use gphoto (Digital Camera support)]))
AC_ARG_WITH(gnutls,    AS_HELP_STRING([--without-gnutls],[do not use GnuTLS (schannel support)]))
AC_ARG_WITH(gssapi,    AS_HELP_STRING([--without-gssapi],[do not use GSSAPI (Kerberos SSP support)]))
AC_ARG_WITH(gstreamer, AS_HELP_STRING([--without-gstreamer],[do not use GStreamer (codecs support)]))
AC_ARG_WITH(inotify,   AS_HELP_STRING([--without-inotify],[do not use inotify (filesystem change notifications)]))
AC_ARG_WITH(krb5,      AS_HELP_STRING([--without-krb5],[do not use krb5 (Kerberos)]))
AC_ARG_WITH(mingw,     AS_HELP_STRING([--without-mingw],[do not use the MinGW cross-compiler]))
AC_ARG_WITH(netapi,    AS_HELP_STRING([--without-netapi],[do not use the Samba NetAPI library]))
AC_ARG_WITH(opencl,    AS_HELP_STRING([--without-opencl],[do not use OpenCL]),
            [if test "x$withval" = "xno"; then ac_cv_header_CL_cl_h=no; ac_cv_header_OpenCL_opencl_h=no; fi])
AC_ARG_WITH(opengl,    AS_HELP_STRING([--without-opengl],[do not use OpenGL]))
AC_ARG_WITH(oss,       AS_HELP_STRING([--without-oss],[do not use the OSS sound support]))
AC_ARG_WITH(pcap,      AS_HELP_STRING([--without-pcap],[do not use the Packet Capture library]),
            [if test "x$withval" = "xno"; then ac_cv_header_pcap_pcap_h=no; fi])
AC_ARG_WITH(pcsclite,  AS_HELP_STRING([--without-pcsclite],[do not use PCSC lite]))
AC_ARG_WITH(pthread,   AS_HELP_STRING([--without-pthread],[do not use the pthread library]))
AC_ARG_WITH(pulse,     AS_HELP_STRING([--without-pulse],[do not use PulseAudio sound support]))
AC_ARG_WITH(sane,      AS_HELP_STRING([--without-sane],[do not use SANE (scanner support)]))
AC_ARG_WITH(sdl,       AS_HELP_STRING([--without-sdl],[do not use SDL]))
AC_ARG_WITH(udev,      AS_HELP_STRING([--without-udev],[do not use udev (plug and play support)]))
AC_ARG_WITH(unwind,    AS_HELP_STRING([--without-unwind],[do not use the libunwind library (exception handling)]))
AC_ARG_WITH(usb,       AS_HELP_STRING([--without-usb],[do not use the libusb library]))
AC_ARG_WITH(v4l2,      AS_HELP_STRING([--without-v4l2],[do not use v4l2 (video capture)]))
AC_ARG_WITH(vulkan,    AS_HELP_STRING([--without-vulkan],[do not use Vulkan]))
AC_ARG_WITH(wayland,   AS_HELP_STRING([--without-wayland],[do not build the Wayland driver]))
AC_ARG_WITH(xcomposite,AS_HELP_STRING([--without-xcomposite],[do not use the Xcomposite extension]),
            [if test "x$withval" = "xno"; then ac_cv_header_X11_extensions_Xcomposite_h=no; fi])
AC_ARG_WITH(xcursor,   AS_HELP_STRING([--without-xcursor],[do not use the Xcursor extension]),
            [if test "x$withval" = "xno"; then ac_cv_header_X11_Xcursor_Xcursor_h=no; fi])
AC_ARG_WITH(xfixes,  AS_HELP_STRING([--without-xfixes],[do not use Xfixes for clipboard change notifications]),
            [if test "x$withval" = "xno"; then ac_cv_header_X11_extensions_Xfixes_h=no; fi])
AC_ARG_WITH(xinerama,  AS_HELP_STRING([--without-xinerama],[do not use Xinerama (legacy multi-monitor support)]),
            [if test "x$withval" = "xno"; then ac_cv_header_X11_extensions_Xinerama_h=no; fi])
AC_ARG_WITH(xinput,    AS_HELP_STRING([--without-xinput],[do not use the Xinput extension]),
            [if test "x$withval" = "xno"; then ac_cv_header_X11_extensions_XInput_h=no; fi])
AC_ARG_WITH(xinput2,   AS_HELP_STRING([--without-xinput2],[do not use the Xinput 2 extension]),
            [if test "x$withval" = "xno"; then ac_cv_header_X11_extensions_XInput2_h=no; fi])
AC_ARG_WITH(xrandr,    AS_HELP_STRING([--without-xrandr],[do not use Xrandr (multi-monitor support)]),
            [if test "x$withval" = "xno"; then ac_cv_header_X11_extensions_Xrandr_h=no; fi])
AC_ARG_WITH(xrender,   AS_HELP_STRING([--without-xrender],[do not use the Xrender extension]),
            [if test "x$withval" = "xno"; then ac_cv_header_X11_extensions_Xrender_h=no; fi])
AC_ARG_WITH(xshape,    AS_HELP_STRING([--without-xshape],[do not use the Xshape extension]),
            [if test "x$withval" = "xno"; then ac_cv_header_X11_extensions_shape_h=no; fi])
AC_ARG_WITH(xshm,      AS_HELP_STRING([--without-xshm],[do not use XShm (shared memory extension)]),
            [if test "x$withval" = "xno"; then ac_cv_header_X11_extensions_XShm_h=no; fi])
AC_ARG_WITH(xxf86vm,   AS_HELP_STRING([--without-xxf86vm],[do not use XFree video mode extension]),
            [if test "x$withval" = "xno"; then ac_cv_header_X11_extensions_xf86vmode_h=no; ac_cv_header_X11_extensions_xf86vmproto_h=no; fi])

AC_ARG_WITH(system-dllpath,AS_HELP_STRING([--with-system-dllpath=PATH],[load external PE dependencies from colon-separated path PATH]),
            AC_SUBST(system_dllpath,[$withval]))
AC_ARG_WITH(wine-tools,AS_HELP_STRING([--with-wine-tools=DIR],[use Wine tools from directory DIR]))
AC_ARG_WITH(wine64,    AS_HELP_STRING([--with-wine64=DIR],[use the 64-bit Wine in DIR for a Wow64 build]))

AC_CANONICAL_HOST
AC_SUBST(srcdir)

AS_IF([test "x$enable_sast" = xyes],[CC=${CC:-clang}
with_mingw=${with_mingw:-clang}
AC_CHECK_PROGS(SARIF_CONVERTER,sarif-converter,false)])

dnl **** Check for some programs ****

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
    ;;
esac

AC_SUBST(TARGETFLAGS)
case $host in
  x86_64*|amd64*)
    if test "x$enable_win64" != "xyes" -a "$cross_compiling" != "yes" -a x"$enable_archs" = x
    then
      CC="$CC -m32"
      CXX="$CXX -m32"
      AC_CACHE_CHECK([whether $CC works], wine_cv_cc_m32,
          [AC_LINK_IFELSE([AC_LANG_PROGRAM()],[wine_cv_cc_m32=yes],[wine_cv_cc_m32=no])])
      test $wine_cv_cc_m32 != no || AC_MSG_ERROR([Cannot build a 32-bit program, you need to install 32-bit development libraries.])
      host_cpu="i386"
      notice_platform="32-bit "
      TARGETFLAGS="$TARGETFLAGS -m32"
      PKG_CONFIG_LIBDIR=${PKG_CONFIG_LIBDIR:-/usr/lib/i386-linux-gnu/pkgconfig:/usr/lib32/pkgconfig:/usr/lib/pkgconfig:/usr/share/pkgconfig}
      export PKG_CONFIG_LIBDIR
    else
      CC="$CC -m64"
      CXX="$CXX -m64"
      host_cpu="x86_64"
      notice_platform="64-bit "
      TARGETFLAGS="$TARGETFLAGS -m64"
    fi
    ;;
  arm*)
    AC_CACHE_CHECK([whether $CC supports Thumb-2],wine_cv_thumb2,
        [WINE_TRY_ASM_LINK([".syntax unified\n\t.thumb\n\tldm r0,{r0-r8}"],,,
                           [wine_cv_thumb2=yes],[wine_cv_thumb2=no])])
    if test x"$wine_cv_thumb2" = xyes
    then
        CFLAGS="$CFLAGS -mthumb"
    else
        CFLAGS="$CFLAGS -marm"
    fi
    AC_CACHE_CHECK([whether $CC supports floating point instructions],wine_cv_float_abi,
                   [WINE_TRY_ASM_LINK(["vmrs r2,fpscr"],,,[wine_cv_float_abi=yes],[wine_cv_float_abi=no])])
    test $wine_cv_float_abi != no || AC_MSG_ERROR([The ARM target needs to support floating point instructions.])
    ;;
esac

dnl Some special cases for the 64-bit build
if test "x$enable_win64" = "xyes"
then
    test -z "$with_wine64" || AC_MSG_ERROR([--enable-win64 and --with-wine64 are mutually exclusive.
--enable-win64 should be used in the 64-bit build tree, --with-wine64 in the 32-bit Wow64 build tree.])
elif test -n "$with_wine64"
then
    test -f "$with_wine64/Makefile" || AC_MSG_ERROR([the 64-bit tree in $with_wine64 has not been configured yet.])
    libdir64=$(grep "^libdir *=" "$with_wine64/Makefile" | sed 's/^libdir *= *//')
    if test "$libdir64" != "$libdir"; then
        AS_ECHO(["$as_me:${as_lineno-$LINENO}: libdir64: $libdir64"]) >&AS_MESSAGE_LOG_FD
        AC_MSG_ERROR([when using --with-wine64 you must use the same --libdir in both build trees.])
    fi
fi

case $build_os in
  cygwin*|mingw32*) AC_SUBST(toolsext,".exe") ;;
  *) AC_SUBST(toolsext,"") ;;
esac

HOST_ARCH=unknown
case "$host_cpu" in
  aarch64*)        HOST_ARCH=aarch64 ;;
  arm*)            HOST_ARCH=arm ;;
  i[[3456789]]86*) HOST_ARCH=i386 ;;
  x86_64)          HOST_ARCH=x86_64 ;;
esac
m4_set_add_all([_AC_SUBST_VARS],[HOST_ARCH]m4_foreach([cpu],[aarch64,arm,arm64ec,i386,x86_64],
                                [m4_foreach([var],[CC,CFLAGS,EXTRACFLAGS,LDFLAGS,DEBUG,TARGET,STRIP,DELAYLOADFLAG,DISABLED_SUBDIRS],[,cpu[_]var])]))

AC_SUBST(toolsdir,[$with_wine_tools])
AC_MSG_CHECKING([for the directory containing the Wine tools])
if test -z "$toolsdir"; then
    if test "$cross_compiling" = "yes"; then
        AC_MSG_ERROR([you must use the --with-wine-tools option when cross-compiling.])
    elif test -n "$with_wine64" -a "x$enable_tools" != xyes; then
        toolsdir="$with_wine64"
    fi
fi
if test -z "$toolsdir"; then
    wine_makedep=tools/makedep$toolsext
elif test -d "$toolsdir/tools/winebuild"; then
    wine_makedep=$toolsdir/tools/makedep$toolsext
    enable_tools=${enable_tools:-no}
    test -f "$wine_makedep" || AC_MSG_ERROR([the Wine tools have not yet been built in $toolsdir])
else
    AC_MSG_ERROR([could not find Wine tools in $toolsdir])
fi
AC_MSG_RESULT([${toolsdir:-none needed}])
AC_SUBST(wine64dir,[$with_wine64])
AC_SUBST(RUNTESTFLAGS,["-q -P wine"])
AC_SUBST(SED_CMD,["LC_ALL=C sed -e 's,@bindir@,\${bindir},g' -e 's,@PACKAGE_STRING@,$PACKAGE_STRING,g' -e 's,@PACKAGE_VERSION@,$PACKAGE_VERSION,g'"])

if test -n "$host_alias" -a "$host_alias" != "$build_alias"
then
    TARGETFLAGS="-b $host_alias $TARGETFLAGS"
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

AC_CHECK_TOOL(STRIP,strip,strip)
AC_PROG_LN_S
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

  dnl Maintainer mode wants -Werror
  enable_werror=yes
fi

if test "x$with_gettext" != xno
then
  AC_CHECK_PROGS(MSGFMT, msgfmt, false)
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
else
  MSGFMT=false
fi
WINE_WARNING_WITH(gettext,[test "$MSGFMT" = false],
                  [gettext tools not found (or too old), translations won't be built.],
                  [enable_po])

dnl **** Check for PE cross compiler ****

AC_SUBST(PE_ARCHS,"")
cross_archs=
AS_VAR_SET_IF([enable_archs],
    [test "x$with_system_dllpath" = "x" || AC_MSG_ERROR("The --with-system-dllpath option is not compatible with --enable-archs")
     ac_save_IFS=$IFS
     IFS=' ,'
     set x $enable_archs
     IFS=$ac_save_IFS
     shift
     for arch
     do
         case $arch in
           i386|x86_64|arm|aarch64|arm64ec) cross_archs="$cross_archs $arch" ;;
           no|none) PE_ARCHS=none ;;
           *) AC_MSG_ERROR([Unknown cross-compilation architecture '$arch']) ;;
         esac
     done],
    [if test "x$with_mingw" != xno
     then
         test $HOST_ARCH = unknown || cross_archs=$HOST_ARCH
     fi])

AS_VAR_IF([PE_ARCHS],[none],[cross_archs=])

saved_CC=$CC
saved_CFLAGS=$CFLAGS
saved_CPPFLAGS=$CPPFLAGS
saved_LDFLAGS=$LDFLAGS

AS_UNSET(extra_arch)
for arch in $cross_archs
do
  case $arch in
    arm64ec) test ${extra_arch+y} || extra_arch=x86_64 ;;
    x86_64) extra_arch="" ;;
  esac
done

for wine_arch in $cross_archs $extra_arch
do
    case $wine_arch in
      i386) mingw_cpu=i686 ;;
      arm)  mingw_cpu=armv7 ;;
      *)    mingw_cpu=$wine_arch ;;
    esac
    case "x$with_mingw" in
      xclang|x*/clang) AS_VAR_SET(${wine_arch}_CC,$with_mingw) ;;
      xllvm-mingw)     AS_VAR_SET(${wine_arch}_CC,${mingw_cpu}-w64-mingw32-clang) ;;
    esac
    AS_VAR_IF([${wine_arch}_CC],[],
        [case $wine_arch in
          aarch64)
            AC_CHECK_PROGS(aarch64_CC,[aarch64-w64-mingw32-clang aarch64-w64-mingw32-gcc clang],[false])
            ;;
          arm64ec)
            AC_CHECK_PROGS(arm64ec_CC,[arm64ec-w64-mingw32-clang arm64ec-w64-mingw32-gcc clang],[false])
            ;;
          arm)
            AC_CHECK_PROGS(arm_CC,[armv7-w64-mingw32-clang armv7-w64-mingw32-gcc clang],[false])
            ;;
          i386)
            ac_prefix_list="m4_foreach([cc],[gcc,clang],m4_foreach([cpu],[i686,i586,i486,i386],[cpu-w64-mingw32-cc ]))"
            AC_CHECK_PROGS(i386_CC,[$ac_prefix_list clang],[false])
            ;;
          x86_64)
            ac_prefix_list="m4_foreach([cc],[gcc,clang],m4_foreach([cpu],[x86_64,amd64],[cpu-w64-mingw32-cc ]))"
            AC_CHECK_PROGS(x86_64_CC,[$ac_prefix_list clang],[false])
            ;;
         esac
         wine_try_msvc=yes],
        [wine_try_msvc=no])

    CPPFLAGS=""
    AS_VAR_SET_IF([${wine_arch}_CFLAGS],[],[AS_VAR_SET([${wine_arch}_CFLAGS],[${CROSSCFLAGS:-"-g -O2"}])])
    AS_VAR_SET_IF([${wine_arch}_LDFLAGS],[],[AS_VAR_SET([${wine_arch}_LDFLAGS],[$CROSSLDFLAGS])])
    AS_VAR_COPY([CC],[${wine_arch}_CC])
    AS_VAR_COPY([CFLAGS],[${wine_arch}_CFLAGS])
    AS_VAR_COPY([LDFLAGS],[${wine_arch}_LDFLAGS])
    AS_VAR_SET([${wine_arch}_EXTRACFLAGS],["-D__WINE_PE_BUILD -Wall"])

    target=""
    set x $CC
    shift
    while test $# -ge 1
    do
        case "$1" in
            */*-gcc) target=`expr "$1" : '.*/\(.*\)-gcc'` ;;
            */*-clang) target=`expr "$1" : '.*/\(.*\)-clang'` ;;
            *-gcc) target=`expr "$1" : '\(.*\)-gcc'` ;;
            *-clang) target=`expr "$1" : '\(.*\)-clang'` ;;
        esac
        shift
    done

    dnl Check if cross compiler supports -target argument
    target_strip=${target:-llvm}-strip
    if test "$wine_try_msvc" = yes -o -z "$target"
    then
        llvm_target="$mingw_cpu-windows"
        llvm_extra_cflags="-target $llvm_target -fuse-ld=lld"
        llvm_cflags="-Wl,-subsystem:console -Wl,-WX"
    else
        llvm_target=$target
        llvm_extra_cflags="-fuse-ld=lld"
        llvm_cflags="-target $target"
    fi
    WINE_TRY_PE_CFLAGS([$llvm_extra_cflags $llvm_cflags --no-default-config],
                       [llvm_extra_cflags="$llvm_extra_cflags --no-default-config"
                        llvm_extra_ldflags="--no-default-config"],
                       [WINE_TRY_PE_CFLAGS([$llvm_extra_cflags $llvm_cflags],[:],
                                           [llvm_extra_cflags=""])])
    if test -n "$llvm_extra_cflags"
    then
        target=$llvm_target
        AS_VAR_SET([${wine_arch}_DELAYLOADFLAG],["-Wl,-delayload,"])
        AS_VAR_APPEND([${wine_arch}_EXTRACFLAGS],[" $llvm_extra_cflags"])
        AS_VAR_APPEND([${wine_arch}_LDFLAGS],[" $llvm_extra_ldflags"])
        CFLAGS="$CFLAGS $llvm_extra_cflags $llvm_cflags"
    else
        AS_VAR_PUSHDEF([wine_cv_crosscc], [wine_cv_${wine_arch}_crosscc])
        AC_CACHE_CHECK([whether $CC works], wine_cv_crosscc,
            [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])], [AS_VAR_SET([wine_cv_crosscc],[yes])], [AS_VAR_SET([wine_cv_crosscc],[no])])])
        AS_VAR_IF([wine_cv_crosscc], [no], [AS_VAR_SET([target],[])])
        AS_VAR_POPDEF([wine_cv_crosscc])
    fi

    if test -z "$target"
    then
        AS_VAR_SET_IF([enable_archs],
            [AC_MSG_ERROR([$wine_arch PE cross-compiler not found.
This is an error since --enable-archs=$wine_arch was requested.])])
        continue
    fi

    AS_VAR_SET([${wine_arch}_TARGET],[$target])
    AS_VAR_SET([${wine_arch}_STRIP],[$target_strip])
    AS_VAR_PUSHDEF([wine_cv_crosscc_c99],[ac_cv_${wine_arch}_crosscc_c99])
    AC_MSG_CHECKING([for $CC option to enable C99 features])
    AC_CACHE_VAL([wine_cv_crosscc_c99],
        [AS_VAR_SET([wine_cv_crosscc_c99],[no])
         for arg in '' '-std=gnu99' '-D__STDC__'
         do
             test -z "$arg" || CC="$CC $arg"
             AC_COMPILE_IFELSE([AC_LANG_SOURCE([$ac_c_conftest_c99_program])],
                               [AS_VAR_SET([wine_cv_crosscc_c99],[$arg])],
                               [AS_VAR_SET([wine_cv_crosscc_c99],[no])])
             AS_VAR_COPY([CC],[${wine_arch}_CC])
             AS_VAR_IF([wine_cv_crosscc_c99],[no],[],[break])
         done])
    AS_VAR_COPY([res],[wine_cv_crosscc_c99])
    AS_VAR_POPDEF([wine_cv_crosscc_c99])
    case "x$res" in
      x) AC_MSG_RESULT([none needed]) ;;
      xno) AC_MSG_RESULT([unsupported])
           AS_VAR_SET_IF([enable_archs],
                [AC_MSG_ERROR([$wine_arch PE cross-compiler supporting C99 not found.
This is an error since --enable-archs=$wine_arch was requested.])])
           continue
           ;;
      x*) AC_MSG_RESULT([$res])
          AS_VAR_APPEND([${wine_arch}_CC],[" $res"]) ;;
    esac

    AS_VAR_IF([wine_arch],[i386],[],
              [AS_VAR_PUSHDEF([wine_cv_seh_support],[ac_cv_${wine_arch}_seh_support])
               AC_CACHE_CHECK([whether $CC supports SEH directives], wine_cv_seh_support,
                   AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[asm(".text\nac_test:\t.seh_proc ac_test\n\tnop\n\t.seh_stackalloc 16\n\t.seh_endprologue\n\t.seh_endproc");]])],
                                     [AS_VAR_SET([wine_cv_seh_support],[yes])],[AS_VAR_SET([wine_cv_seh_support],[no])]))
               AS_VAR_COPY([res],[wine_cv_seh_support])
               AS_VAR_POPDEF([wine_cv_seh_support])
               AS_VAR_IF([res],[yes],[],
                         [AS_VAR_SET_IF([enable_archs],
                             [AC_MSG_ERROR([The $wine_arch PE cross-compiler doesn't support SEH directives.
This is an error since --enable-archs=$wine_arch was requested.])])
                          continue])])

    AS_VAR_IF([wine_arch],[$extra_arch],[],[AS_VAR_APPEND([PE_ARCHS],[" $wine_arch"])])

    case "$target" in
      *-windows) wine_compiler_rt_libs="compiler-rt" ;;
    esac

    WINE_TRY_PE_CFLAGS([-fno-strict-aliasing])
    dnl clang needs to be told to fail on unknown options
    WINE_TRY_PE_CFLAGS([-Werror=unknown-warning-option],[CFLAGS="$CFLAGS -Werror=unknown-warning-option"])
    WINE_TRY_PE_CFLAGS([-Werror=unused-command-line-argument],[CFLAGS="$CFLAGS -Werror=unused-command-line-argument"])
    WINE_TRY_PE_CFLAGS([-Werror=ignored-optimization-argument],[CFLAGS="$CFLAGS -Werror=ignored-optimization-argument"])
    WINE_TRY_PE_CFLAGS([-Wdeclaration-after-statement])
    WINE_TRY_PE_CFLAGS([-Wempty-body])
    WINE_TRY_PE_CFLAGS([-Wignored-qualifiers])
    WINE_TRY_PE_CFLAGS([-Winit-self])
    WINE_TRY_PE_CFLAGS([-Wpacked-not-aligned],[AS_VAR_APPEND(${wine_arch}_EXTRACFLAGS,[" -Wno-packed-not-aligned"])])
    WINE_TRY_PE_CFLAGS([-Wmicrosoft-enum-forward-reference],[AS_VAR_APPEND(${wine_arch}_EXTRACFLAGS,[" -Wno-microsoft-enum-forward-reference"])])
    WINE_TRY_PE_CFLAGS([-Wshift-overflow=2])
    WINE_TRY_PE_CFLAGS([-Wstrict-prototypes])
    WINE_TRY_PE_CFLAGS([-Wtype-limits])
    WINE_TRY_PE_CFLAGS([-Wunused-but-set-parameter])
    WINE_TRY_PE_CFLAGS([-Wvla])
    WINE_TRY_PE_CFLAGS([-Wwrite-strings])
    WINE_TRY_PE_CFLAGS([-Wpointer-arith])
    WINE_TRY_PE_CFLAGS([-Wlogical-op])
    WINE_TRY_PE_CFLAGS([-Wabsolute-value])
    WINE_TRY_PE_CFLAGS([-Wenum-enum-conversion],[:],WINE_TRY_PE_CFLAGS([-Wenum-conversion]))
    WINE_TRY_PE_CFLAGS([-ffunction-sections])
    WINE_TRY_PE_CFLAGS([-fasync-exceptions -DMIN_CLANG_VERSION=19], AS_VAR_APPEND([${wine_arch}_EXTRACFLAGS],[" -fasync-exceptions"]))

    dnl clang had broken -fms-hotpatch support before version 18 (https://github.com/llvm/llvm-project/pull/77245)
    WINE_TRY_PE_CFLAGS([-fms-hotpatch -DMIN_CLANG_VERSION=18],
                       [AS_VAR_APPEND([${wine_arch}_EXTRACFLAGS],[" -fms-hotpatch"])
                        AS_VAR_APPEND([${wine_arch}_LDFLAGS],[" -fms-hotpatch"])])

    dnl GCC can't handle large files when -Wmisleading-indentation is enabled (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=89549)
    WINE_TRY_PE_CFLAGS([-flarge-source-files -Wmisleading-indentation],[AS_VAR_APPEND(${wine_arch}_EXTRACFLAGS,[" -Wno-misleading-indentation"])])

    case $wine_arch in
      i386) WINE_TRY_PE_CFLAGS([-fno-omit-frame-pointer])
            WINE_TRY_PE_CFLAGS([-mpreferred-stack-boundary=2])
            WINE_TRY_PE_CFLAGS([-Wl,--disable-stdcall-fixup],
                               [AS_VAR_APPEND([${wine_arch}_LDFLAGS],[" -Wl,--disable-stdcall-fixup"])]) ;;
      x86_64) WINE_TRY_PE_CFLAGS([-Wformat-overflow])
              WINE_TRY_PE_CFLAGS([-Wnonnull])
              WINE_TRY_PE_CFLAGS([-mcx16])
              WINE_TRY_PE_CFLAGS([-mcmodel=small]) ;;
    esac

    dnl Determine debug info format
    wine_crossdebug=$CROSSDEBUG
    if test -z "$wine_crossdebug"
    then
      ac_g_flag_seen=""
      for ac_flag in $CFLAGS; do
        case $ac_flag in
         -gdwarf*)    AS_VAR_APPEND(wine_crossdebug, " dwarf") ;;
         -gcodeview)  AS_VAR_APPEND(wine_crossdebug, " pdb") ;;
         -g)          ac_g_flag_seen=$ac_flag ;;
        esac
      done
      if test -n "$ac_g_flag_seen" -a -z "$wine_crossdebug"
      then
        wine_crossdebug=dwarf
      fi
    fi

    ac_debug_format_seen=""
    for ac_flag in $CFLAGS; do
      case $ac_flag in
       -gdwarf*|-gcodeview)  ac_debug_format_seen=$ac_flag ;;
      esac
    done
    if test "x$ac_debug_format_seen" = x
    then
      for wine_debug_format in $wine_crossdebug
      do
        case $wine_debug_format in
          *dwarf)  dnl clang emits a warning without -Wl,-debug:dwarf, so ensure linker option is present
                   WINE_TRY_PE_CFLAGS([-Wl,-debug:dwarf],[AS_VAR_APPEND([${wine_arch}_LDFLAGS],[" -Wl,-debug:dwarf"])
                                                          CFLAGS="$CFLAGS -Wl,-debug:dwarf"])
                   WINE_TRY_PE_CFLAGS([-gdwarf-4]) ;;
          pdb)     WINE_TRY_PE_CFLAGS([-gcodeview]) ;;
        esac
      done
    fi
    AS_VAR_SET([${wine_arch}_DEBUG],[$wine_crossdebug])

    test "x$enable_werror" != xyes || WINE_TRY_PE_CFLAGS([-Werror])
    dnl clang for PE target wants -build-id (and not --build-id as gcc/mingw or clang in unix target)
    dnl don't bother testing flags, compilation will fail afterwards
    test "x$enable_build_id" != xyes || AS_VAR_APPEND([${wine_arch}_LDFLAGS],[" -Wl,--build-id"])

done

CC=$saved_CC
CFLAGS=$saved_CFLAGS
CPPFLAGS=$saved_CPPFLAGS
LDFLAGS=$saved_LDFLAGS

case $HOST_ARCH in
  arm|aarch64)
    test "x$PE_ARCHS" != x || AC_MSG_ERROR([PE cross-compilation is required for $HOST_ARCH, please install clang/llvm-dlltool/lld, or llvm-mingw.])
    DLLEXT="" ;;
  *)
    AC_SUBST(DLLEXT,".so")
    WINE_WARNING_WITH(mingw,[test "x$PE_ARCHS" = "x"],
                      [Suitable PE cross-compiler not found, you should install mingw-w64 or llvm-mingw.]) ;;
esac

dnl **** Check for some libraries ****

dnl Check for -li386 for NetBSD and OpenBSD
AC_CHECK_LIB(i386,i386_set_ldt,[AC_SUBST(I386_LIBS, "-li386")])

AC_SUBST(OPENGL_LIBS,"")

dnl **** Check for header files ****

AC_SYS_LARGEFILE()
m4_ifdef([AC_SYS_YEAR2038],
         [AS_IF(false,[AC_SYS_YEAR2038()])], dnl bypass misguided autoconf error on wow64 builds
         [test "$ac_cv_sys_file_offset_bits" = 64 && AC_DEFINE(_TIME_BITS,64,[Define to 64 to enable 64-bit time_t])])

AC_CHECK_HEADERS(\
	CL/cl.h \
	OpenCL/opencl.h \
	arpa/inet.h \
	arpa/nameser.h \
	asm/termbits.h \
	asm/types.h \
	asm/user.h \
        bluetooth/bluetooth.h \
	elf.h \
	gettext-po.h \
	link.h \
	linux/cdrom.h \
	linux/filter.h \
	linux/fs.h \
	linux/hdreg.h \
	linux/hidraw.h \
	linux/input.h \
	linux/ioctl.h \
	linux/major.h \
	linux/param.h \
	linux/serial.h \
	linux/types.h \
	linux/ucdrom.h \
	linux/userfaultfd.h \
	linux/wireless.h \
	lwp.h \
	mach-o/loader.h \
	mach/mach.h \
	machine/cpu.h \
	machine/sysarch.h \
	mntent.h \
	netdb.h \
	netinet/in.h \
	netinet/in_systm.h \
	netinet/tcp.h \
	netinet/tcp_fsm.h \
	pcap/pcap.h \
	port.h \
	pwd.h \
	sched.h \
	scsi/scsi.h \
	scsi/scsi_ioctl.h \
	scsi/sg.h \
	sys/attr.h \
	sys/auxv.h \
	sys/cdio.h \
	sys/epoll.h \
	sys/event.h \
	sys/extattr.h \
	sys/filio.h \
	sys/ipc.h \
	sys/link.h \
	sys/modem.h \
	sys/mtio.h \
	sys/param.h \
	sys/prctl.h \
	sys/ptrace.h \
	sys/queue.h \
	sys/random.h \
	sys/resource.h \
	sys/scsiio.h \
	sys/shm.h \
	sys/signal.h \
	sys/socketvar.h \
	sys/sockio.h \
	sys/statvfs.h \
	sys/strtio.h \
	sys/syscall.h \
	sys/sysinfo.h \
	sys/times.h \
	sys/uio.h \
	sys/user.h \
	sys/utsname.h \
	sys/vnode.h \
	sys/xattr.h \
	syscall.h \
	utime.h \
	valgrind/memcheck.h \
	valgrind/valgrind.h
)
AC_HEADER_MAJOR()

dnl **** Checks for headers that depend on other ones ****

AC_CHECK_HEADERS([sys/conf.h sys/mount.h sys/statfs.h sys/user.h sys/vfs.h],,,
    [#include <sys/types.h>
     #ifdef HAVE_SYS_PARAM_H
     # include <sys/param.h>
     #endif])

dnl check for broken Linux sysctl.h that causes deprecation warnings
saved_sysctl_h_CFLAGS=$CFLAGS
test "x${GCC}" != xyes || CFLAGS="$CFLAGS -Werror"
AC_CHECK_HEADERS([sys/sysctl.h],,,
    [#include <sys/types.h>
     #ifdef HAVE_SYS_PARAM_H
     # include <sys/param.h>
     #endif])
CFLAGS=$saved_sysctl_h_CFLAGS

AC_CHECK_HEADERS(\
	netinet/ip.h \
	net/if.h \
	net/if_arp.h \
	net/if_dl.h \
	net/if_types.h \
	net/route.h \
	netinet/if_ether.h \
	netinet/in_pcb.h \
	netinet/ip_icmp.h \
	netinet/ip_var.h \
	netinet/udp.h \
	netinet6/ip6_var.h \
	netipx/ipx.h \
	sys/un.h \
,,,[#include <sys/types.h>
     #include <sys/socket.h>
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

AC_CHECK_HEADERS([netinet/udp_var.h netinet/icmp_var.h netinet/icmp6.h netinet/tcp_var.h ],,,
    [#include <sys/types.h>
     #include <sys/socket.h>
     #ifdef HAVE_SYS_SOCKETVAR_H
     # include <sys/socketvar.h>
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
     #endif])

AC_CHECK_HEADERS([linux/ipx.h linux/irda.h linux/rtnetlink.h],,,
    [#include <sys/types.h>
     #include <sys/socket.h>
     #ifdef HAVE_ASM_TYPES_H
     # include <asm/types.h>
     #endif
     #ifdef HAVE_LINUX_TYPES_H
     # include <linux/types.h>
     #endif])

AC_CHECK_HEADERS([resolv.h],,,
    [#include <sys/types.h>
     #include <sys/socket.h>
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

AC_CHECK_HEADERS([pthread_np.h],,,[#include <pthread.h>])

AC_CHECK_HEADERS([linux/videodev2.h],,,
[#include <sys/time.h>
#include <sys/types.h>
#ifdef HAVE_ASM_TYPES_H
#include <asm/types.h>
#endif])

AC_CHECK_HEADERS([libprocstat.h],,,
[#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <sys/socket.h>
#ifdef HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif])

AC_CHECK_HEADERS([bluetooth/rfcomm.h],,,
[#ifdef HAVE_BLUETOOTH_BLUETOOTH_H
#include <bluetooth/bluetooth.h>
#endif])

if test "x$ac_cv_header_sys_xattr_h" = xyes
then
    AC_CACHE_CHECK([whether getxattr takes additional arguments], wine_cv_xattr_extra_args,
        [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/xattr.h>]], [[getxattr("", "", "", 0, 0, 0);]])],
                           [wine_cv_xattr_extra_args=yes],[wine_cv_xattr_extra_args=no])])
    test $wine_cv_xattr_extra_args != yes || AC_DEFINE(XATTR_ADDITIONAL_OPTIONS, 1, [Define if xattr functions take additional arguments (macOS)])
fi

dnl **** Check for working dll ****

AC_SUBST(DLLFLAGS,"")
AC_SUBST(LDDLLFLAGS,"")
AC_SUBST(LDEXECFLAGS,"")
AC_SUBST(EXTRACFLAGS,"")
AC_SUBST(UNIXDLLFLAGS,"-fPIC")
AC_SUBST(UNIXLDFLAGS,["-shared -Wl,-Bsymbolic -Wl,-soname,\$(UNIXLIB)"])
AC_SUBST(TOP_INSTALL_LIB,"")
AC_SUBST(TOP_INSTALL_DEV,"")
AC_SUBST(WINELOADER_LDFLAGS,"")
AC_SUBST(WINEPRELOADER_LDFLAGS,"")
LIBEXT="so"
WINE_PATH_SONAME_TOOLS
WINE_CONFIG_HELPERS

case $host_os in
  cygwin*|mingw32*)
    LIBEXT="dll"
    DLLEXT=""
    EXTRACFLAGS="-D__WINE_PE_BUILD"
    dnl Disable modules that can't be used on Windows
    enable_loader=${enable_loader:-no}
    enable_server=${enable_server:-no}
    enable_wine=${enable_wine:-no}
    dnl Disable dependencies that are not useful on Windows
    with_x=${with_x:-no}
    with_pthread=${with_pthread:-no}
    ;;

  darwin*)
    LIBEXT="dylib"
    DLLFLAGS="$DLLFLAGS -fPIC"
    LDDLLFLAGS="-fPIC"
    enable_winemac_drv=${enable_winemac_drv:-yes}
    dnl declare needed frameworks
    AC_SUBST(CARBON_LIBS,"-framework Carbon")
    AC_SUBST(COREFOUNDATION_LIBS,"-framework CoreFoundation")
    AC_SUBST(DISKARBITRATION_LIBS,"-framework DiskArbitration -framework CoreFoundation")
    AC_SUBST(IOKIT_LIBS,"-framework IOKit -framework CoreFoundation")
    AC_SUBST(APPLICATIONSERVICES_LIBS,"-framework ApplicationServices")
    AC_SUBST(CORESERVICES_LIBS,"-framework CoreServices")
    AC_SUBST(APPKIT_LIBS,"-framework AppKit")
    AC_SUBST(SECURITY_LIBS,"-framework Security -framework CoreFoundation")
    AC_SUBST(SYSTEMCONFIGURATION_LIBS,"-framework SystemConfiguration")

    WINELOADER_LDFLAGS="-Wl,-segalign,0x1000,-pagezero_size,0x1000,-sectcreate,__TEXT,__info_plist,loader/wine_info.plist"

    case $HOST_ARCH in
      i386) wine_use_preloader=yes ;;
      x86_64)
        dnl If the -no_huge linker option is present (added in Xcode 15.3), use zerofill sections instead of the preloader
        WINE_TRY_CFLAGS([-Wl,-no_huge],
           [wine_use_preloader=no
            WINELOADER_LDFLAGS="$WINELOADER_LDFLAGS -Wl,-no_pie,-image_base,0x200000000,-no_huge,-no_fixup_chains,-segalign,0x1000,-segaddr,WINE_RESERVE,0x1000,-segaddr,WINE_TOP_DOWN,0x7ff000000000"],
           [wine_use_preloader=yes])
        ;;
      *)    wine_use_preloader=no ;;
    esac

    if test "$wine_use_preloader" = "yes"
    then
        WINEPRELOADER_LDFLAGS="-nostartfiles -nodefaultlibs -e _start -ldylib1.o -mmacosx-version-min=10.7 -Wl,-no_new_main,-segalign,0x1000,-pagezero_size,0x1000,-sectcreate,__TEXT,__info_plist,loader/wine_info.plist"
        WINE_TRY_CFLAGS([-Wl,-no_pie],
                        [WINEPRELOADER_LDFLAGS="-Wl,-no_pie $WINEPRELOADER_LDFLAGS"])
        case $HOST_ARCH in
          i386)
            WINEPRELOADER_LDFLAGS="-Wl,-image_base,0x7d400000 $WINEPRELOADER_LDFLAGS"
            ;;
          x86_64)
            WINEPRELOADER_LDFLAGS="-Wl,-image_base,0x200000000,-segalign,0x1000,-segaddr,WINE_RESERVE,0x1000 $WINEPRELOADER_LDFLAGS"
            ;;
        esac
        dnl If preloader is used, the loader needs to be an LC_UNIXTHREAD binary to avoid AppKit/Core Animation problems.
        WINELOADER_LDFLAGS="$WINELOADER_LDFLAGS -mmacosx-version-min=10.7"
    fi

    if test "x$with_coreaudio" != "xno";
    then
        AC_SUBST(COREAUDIO_LIBS,"-framework CoreFoundation -framework CoreAudio -framework AudioUnit -framework AudioToolbox -framework CoreMIDI")
        enable_winecoreaudio_drv=${enable_winecoreaudio_drv:-yes}
    fi
    if test "$ac_cv_header_OpenCL_opencl_h" = "yes"
    then
        AC_SUBST(OPENCL_LIBS,"-framework OpenCL")
        ac_cv_lib_OpenCL_clGetPlatformInfo=yes
    fi

    dnl Check for MTLDevice registryID property
    AC_LANG_PUSH([Objective C])
    AC_CACHE_CHECK([whether MTLDevice protocol supports registryID property], ac_cv_mtldevice_registryid,
                   [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <Metal/Metal.h>]], [[id<MTLDevice> device; device.registryID;]])],
                                      [ac_cv_mtldevice_registryid=yes],
                                      [ac_cv_mtldevice_registryid=no])])
    AC_LANG_POP([Objective C])
    test $ac_cv_mtldevice_registryid != yes || AC_DEFINE(HAVE_MTLDEVICE_REGISTRYID, 1, [Define if MTLDevice protocol has registryID property.])

    UNIXLDFLAGS="-dynamiclib -install_name @rpath/\$(UNIXLIB) -Wl,-rpath,@loader_path\/"
    AC_SUBST(WINELOADER_DEPENDS,"wine_info.plist")
    ;;

  linux-android*)
    AC_DEFINE(_GNU_SOURCE,1,[Define to 1 to enable GNU extensions on Linux])
    DLLFLAGS="$DLLFLAGS -fPIC"
    LDDLLFLAGS="-fPIC"
    LDEXECFLAGS="-Wl,-pie"
    enable_wineandroid_drv=${enable_wineandroid_drv:-yes}
    WINE_TRY_CFLAGS([-Wl,-z,defs],[UNIXLDFLAGS="$UNIXLDFLAGS -Wl,-z,defs"])
    WINE_TRY_CFLAGS([-fPIC -Wl,--export-dynamic],[WINELOADER_LDFLAGS="-Wl,--export-dynamic"])
    WINEPRELOADER_LDFLAGS="-static -nostartfiles -nodefaultlibs -Wl,-Ttext=0x7d400000"

    WINE_CHECK_SONAME(GLESv2,glFlush)

    if test "x$exec_prefix" = xNONE
    then
      case $HOST_ARCH in
        i386)    exec_prefix='${prefix}/x86' ;;
        x86_64)  exec_prefix='${prefix}/x86_64' ;;
        arm)     exec_prefix='${prefix}/armeabi-v7a' ;;
        aarch64) exec_prefix='${prefix}/arm64-v8a' ;;
      esac
    fi
    ;;

  *)
    AC_DEFINE(_GNU_SOURCE,1,[Define to 1 to enable GNU extensions on Linux])
    if test $HOST_ARCH = i386
    then
        DLLFLAGS="$DLLFLAGS -fno-PIC"
        LDDLLFLAGS="-fno-PIC"
        WINE_TRY_CFLAGS([-fno-PIC -Wl,-z,notext],[LDDLLFLAGS="$LDDLLFLAGS -Wl,-z,notext"])
    else
        DLLFLAGS="$DLLFLAGS -fPIC"
        LDDLLFLAGS="-fPIC"
    fi

    AC_CACHE_CHECK([whether -Wl,-z,defs works correctly], ac_cv_wl_z_defs,
                   [ac_save_CFLAGS="$CFLAGS"
                    CFLAGS="$CFLAGS $UNIXDLLFLAGS $UNIXLDFLAGS -Wl,-z,defs"
                    dnl On FreeBSD, shared libraries using environ fail to link with -Wl,-z,defs
                    AC_LINK_IFELSE([AC_LANG_SOURCE([[extern char **environ; char **envp; void myfunc(void) { envp = environ; }]])],
                                                   [ac_cv_wl_z_defs=yes],[ac_cv_wl_z_defs=no])
                    CFLAGS=$ac_save_CFLAGS])
    test $ac_cv_wl_z_defs != yes || AS_VAR_APPEND([UNIXLDFLAGS],[" -Wl,-z,defs"])

    WINE_TRY_CFLAGS([-Wl,--export-dynamic],[WINELOADER_LDFLAGS="-Wl,--export-dynamic"])
    WINEPRELOADER_LDFLAGS="-nostartfiles -nodefaultlibs"

    case $host_os in
      linux*)
        AS_VAR_APPEND([WINELOADER_LDFLAGS],[" -pie"])
        case $HOST_ARCH in
          i386|arm)
            AS_VAR_APPEND([WINEPRELOADER_LDFLAGS],[" -static -Wl,-Ttext=0x7d400000"]) ;;
          *)
            WINE_TRY_CFLAGS([-static-pie],
                            [AS_VAR_APPEND([WINEPRELOADER_LDFLAGS],[" -static-pie"])],
                            [AS_VAR_APPEND([WINEPRELOADER_LDFLAGS],[" -static -Wl,-Ttext=0x7d7d00000000"])])
            ;;
        esac
        ;;
      *)
        case $HOST_ARCH in
          i386|arm)
            WINE_TRY_CFLAGS([-Wl,-Ttext-segment=0x60000000],
                            [AS_VAR_APPEND([WINELOADER_LDFLAGS],[" -Wl,-Ttext-segment=0x60000000"])],
                            [WINE_TRY_CFLAGS([-Wl,--section-start,.interp=0x60000400],
                                [AS_VAR_APPEND([WINELOADER_LDFLAGS],[" -Wl,--section-start,.interp=0x60000400"])])])
            ;;
          *)
            AS_VAR_APPEND([WINELOADER_LDFLAGS],[" -pie"])
            ;;
        esac
        ;;
    esac
    ;;
esac

enable_winecoreaudio_drv=${enable_winecoreaudio_drv:-no}
enable_wineandroid_drv=${enable_wineandroid_drv:-no}
enable_winemac_drv=${enable_winemac_drv:-no}

dnl **** External libraries ****

if test "x$with_system_dllpath" != "x" -a -n "$PE_ARCHS"
then
    WINE_MINGW_PACKAGE_FLAGS(FAUDIO,[FAudio],[-lFAudio],
        [WINE_CHECK_MINGW_HEADER(FAudio.h,
            [WINE_CHECK_MINGW_LIB(FAudio,FAudio_CommitOperationSet,[:],[FAUDIO_PE_CFLAGS=""; FAUDIO_PE_LIBS=""],[$FAUDIO_PE_LIBS])],
            [FAUDIO_PE_CFLAGS=""; FAUDIO_PE_LIBS=""])])
    if test "x$FAUDIO_PE_LIBS" = "x"
    then
        WINE_NOTICE([FAudio ${notice_platform}MinGW development files not found (or too old); using bundled version.])
    fi

    WINE_MINGW_PACKAGE_FLAGS(FLUIDSYNTH,[fluidsynth],[-lfluidsynth],
        [WINE_CHECK_MINGW_HEADER(fluidsynth.h,
            [WINE_CHECK_MINGW_LIB(fluidsynth,new_fluid_synth,[:],[FLUIDSYNTH_PE_CFLAGS=""; FLUIDSYNTH_PE_LIBS=""],[$FLUIDSYNTH_PE_LIBS])],
            [FLUIDSYNTH_PE_CFLAGS=""; FLUIDSYNTH_PE_LIBS=""])])
    if test "x$FLUIDSYNTH_PE_LIBS" = "x"
    then
        WINE_NOTICE([Fluidsynth ${notice_platform}MinGW development files not found (or too old); using bundled version.])
    fi

    WINE_MINGW_PACKAGE_FLAGS(JPEG,[libjpeg],,
        [WINE_CHECK_MINGW_HEADER(jpeglib.h,
            [WINE_CHECK_MINGW_LIB(jpeg,jpeg_start_decompress,[:],[JPEG_PE_CFLAGS=""; JPEG_PE_LIBS=""],[$JPEG_PE_LIBS])],
            [JPEG_PE_CFLAGS=""; JPEG_PE_LIBS=""],
            [#include <stddef.h>
             #include <stdio.h>])])
    if test "x$JPEG_PE_LIBS" = "x"
    then
        WINE_NOTICE([libjpeg ${notice_platform}MinGW development files not found; using bundled version.])
    fi

    WINE_MINGW_PACKAGE_FLAGS(LCMS2,[lcms2],[-llcms2],
        [WINE_CHECK_MINGW_HEADER(lcms2.h,
            [WINE_CHECK_MINGW_LIB(lcms2,cmsOpenProfileFromFile,[:],[LCMS2_PE_CFLAGS=""; LCMS2_PE_LIBS=""],[$LCMS2_PE_LIBS])],
            [LCMS2_PE_CFLAGS=""; LCMS2_PE_LIBS=""])])
    if test "x$LCMS2_PE_LIBS" = "x"
    then
        WINE_NOTICE([liblcms2 ${notice_platform}MinGW development files not found; using bundled version.])
    fi

    WINE_MINGW_PACKAGE_FLAGS(MPG123,[libmpg123],[-lmpg123],
        [WINE_CHECK_MINGW_HEADER(mpg123.h,
            [WINE_CHECK_MINGW_LIB(mpg123,mpg123_feed,[:],[MPG123_PE_CFLAGS=""; MPG123_PE_LIBS=""],[$MPG123_PE_LIBS])],
            [MPG123_PE_CFLAGS=""; MPG123_PE_LIBS=""])])
    if test "x$MPG123_PE_LIBS" = "x"
    then
        WINE_NOTICE([libmpg123 ${notice_platform}MinGW development files not found (or too old); using bundled version.])
    fi

    WINE_MINGW_PACKAGE_FLAGS(PNG,[libpng],,
        [WINE_CHECK_MINGW_HEADER(png.h,
            [WINE_CHECK_MINGW_LIB(png,png_create_read_struct,[:],[PNG_PE_CFLAGS=""; PNG_PE_LIBS=""],[$PNG_PE_LIBS])],
            [PNG_PE_CFLAGS=""; PNG_PE_LIBS=""])])
    if test "x$PNG_PE_LIBS" = "x"
    then
        WINE_NOTICE([libpng ${notice_platform}MinGW development files not found; using bundled version.])
    fi

    WINE_MINGW_PACKAGE_FLAGS(TIFF,[libtiff-4],,
        [WINE_CHECK_MINGW_HEADER(tiffio.h,
            [WINE_CHECK_MINGW_LIB(tiff,TIFFClientOpen,[:],[TIFF_PE_CFLAGS=""; TIFF_PE_LIBS=""],[$TIFF_PE_LIBS])],
            [TIFF_PE_CFLAGS=""; TIFF_PE_LIBS=""])])
    if test "x$TIFF_PE_LIBS" = "x"
    then
        WINE_NOTICE([libtiff ${notice_platform}MinGW development files not found; using bundled version.])
    fi

    WINE_MINGW_PACKAGE_FLAGS(XML2,[libxml-2.0],[-lxml2],
        [WINE_CHECK_MINGW_HEADER(libxml/parser.h)
        WINE_CHECK_MINGW_HEADER(libxml/xmlsave.h)
        WINE_CHECK_MINGW_HEADER(libxml/SAX2.h)
        if test "$ac_cv_mingw_header_libxml_parser_h" = "yes" -a "$ac_cv_mingw_header_libxml_xmlsave_h" = "yes" -a "$ac_cv_mingw_header_libxml_SAX2_h" = "yes"
        then
            WINE_CHECK_MINGW_LIB(xml2,xmlFirstElementChild,[:],[XML2_PE_CFLAGS=""; XML2_PE_LIBS=""],[$XML2_PE_LIBS])
        else
            XML2_PE_CFLAGS=""
            XML2_PE_LIBS=""
        fi])
    if test "x$XML2_PE_LIBS" = "x"
    then
        WINE_NOTICE([libxml2 ${notice_platform}MinGW development files not found (or too old); using bundled version.])
    fi

    WINE_MINGW_PACKAGE_FLAGS(XSLT,[libxslt],[-lxslt],
        [WINE_CHECK_MINGW_HEADER(libxslt/pattern.h)
        WINE_CHECK_MINGW_HEADER(libxslt/transform.h)
        if test "$ac_cv_mingw_header_libxslt_pattern_h" = "yes" -a "$ac_cv_mingw_header_libxslt_transform_h" = "yes"
        then
            WINE_CHECK_MINGW_LIB(xslt,xsltCompilePattern,[:],[XSLT_PE_CFLAGS=""; XSLT_PE_LIBS=""],[$XSLT_PE_LIBS])
        else
            XSLT_PE_CFLAGS=""
            XSLT_PE_LIBS=""
        fi])
    if test "x$XSLT_PE_LIBS" = "x"
    then
        WINE_NOTICE([libxslt ${notice_platform}MinGW development files not found; using bundled version.])
    fi

    WINE_MINGW_PACKAGE_FLAGS(VKD3D,[libvkd3d-utils libvkd3d libvkd3d-shader],[-lvkd3d-utils -lvkd3d -lvkd3d-shader],
        [WINE_CHECK_MINGW_HEADER(vkd3d_utils.h)
        WINE_CHECK_MINGW_HEADER(vkd3d_shader.h)
        if test "$ac_cv_mingw_header_vkd3d_utils_h" = "yes" -a "$ac_cv_mingw_header_vkd3d_shader_h" = "yes"
        then
            WINE_CHECK_MINGW_LIB(vkd3d-utils,vkd3d_utils_set_log_callback,[:],[:],[$VKD3D_PE_LIBS])
            WINE_CHECK_MINGW_LIB(vkd3d-shader,vkd3d_shader_build_varying_map,[:],[:],[$VKD3D_PE_LIBS])
            if test "$ac_cv_mingw_lib_vkd3d_utils" = "no" -o "$ac_cv_mingw_lib_vkd3d_shader" = "no"
            then
                VKD3D_PE_CFLAGS=""
                VKD3D_PE_LIBS=""
            fi
        else
            VKD3D_PE_CFLAGS=""
            VKD3D_PE_LIBS=""
        fi])
    if test "x$VKD3D_PE_LIBS" = "x"
    then
        WINE_NOTICE([libvkd3d ${notice_platform}MinGW development files not found (or too old); using bundled version.])
    fi

    WINE_MINGW_PACKAGE_FLAGS(ZLIB,[zlib],[-lz],
        [WINE_CHECK_MINGW_HEADER(zlib.h,
            [WINE_CHECK_MINGW_LIB(z,inflate,[:],[ZLIB_PE_CFLAGS=""; ZLIB_PE_LIBS=""],[$ZLIB_PE_LIBS])],
            [ZLIB_PE_CFLAGS=""; ZLIB_PE_LIBS=""])])
    if test "x$ZLIB_PE_LIBS" = "x"
    then
        WINE_NOTICE([zlib ${notice_platform}MinGW development files not found; using bundled version.])
    fi
fi

WINE_EXTLIB_FLAGS(CAPSTONE, capstone, capstone, "-I\$(top_srcdir)/libs/capstone/include")
WINE_EXTLIB_FLAGS(COMPILER_RT, compiler-rt, "$wine_compiler_rt_libs")
WINE_EXTLIB_FLAGS(FAUDIO, faudio, "faudio mfplat mfreadwrite mfuuid propsys", "-I\$(top_srcdir)/libs/faudio/include")
WINE_EXTLIB_FLAGS(FLUIDSYNTH, fluidsynth, "fluidsynth", "-I\$(top_srcdir)/libs/fluidsynth/include")
WINE_EXTLIB_FLAGS(GSM, gsm, gsm, "-I\$(top_srcdir)/libs/gsm/inc")
WINE_EXTLIB_FLAGS(JPEG, jpeg, jpeg, "-I\$(top_srcdir)/libs/jpeg")
WINE_EXTLIB_FLAGS(JXR, jxr, jxr, "-I\$(top_srcdir)/libs/jxr/jxrgluelib -I\$(top_srcdir)/libs/jxr/image/sys")
WINE_EXTLIB_FLAGS(LCMS2, lcms2, lcms2, "-I\$(top_srcdir)/libs/lcms2/include")
WINE_EXTLIB_FLAGS(LDAP, ldap, ldap, "-I\$(top_srcdir)/libs/ldap/include")
WINE_EXTLIB_FLAGS(MPG123, mpg123, mpg123, "-I\$(top_srcdir)/libs/mpg123/src/include")
WINE_EXTLIB_FLAGS(MUSL, musl, musl)
WINE_EXTLIB_FLAGS(PNG, png, "png \$(ZLIB_PE_LIBS)", "-I\$(top_srcdir)/libs/png")
WINE_EXTLIB_FLAGS(TIFF, tiff, "tiff \$(ZLIB_PE_LIBS)", "-I\$(top_srcdir)/libs/tiff/libtiff")
WINE_EXTLIB_FLAGS(TOMCRYPT, tomcrypt, tomcrypt, "-I\$(top_srcdir)/libs/tomcrypt/src/headers -DLTC_NO_PROTOTYPES -DLTC_SOURCE")
WINE_EXTLIB_FLAGS(VKD3D, vkd3d, vkd3d, "-I\$(top_srcdir)/libs/vkd3d/include")
WINE_EXTLIB_FLAGS(XML2, xml2, xml2, "-I\$(top_srcdir)/libs/xml2/include -DLIBXML_STATIC")
WINE_EXTLIB_FLAGS(XSLT, xslt, xslt, "-I\$(top_srcdir)/libs/xslt/libxslt -I\$(top_srcdir)/libs/xslt -DLIBXSLT_STATIC")
WINE_EXTLIB_FLAGS(ZLIB, zlib, z, "-I\$(top_srcdir)/libs/zlib -DFAR= -DZ_SOLO")

dnl **** Check for pthread ****

AS_VAR_IF([with_pthread],[no],[],
          [AC_CHECK_FUNC(pthread_create,,[AC_CHECK_LIB(pthread,pthread_create,[AC_SUBST(PTHREAD_LIBS,"-lpthread")])])])
WINE_ERROR_WITH(pthread,[test "x$ac_cv_func_pthread_create" != xyes -a "x$PTHREAD_LIBS" = x],
[pthread ${notice_platform}development files not found.
Wine cannot support threads without libpthread.])

dnl **** Check for EGL ****

if test "x$with_opengl" != "xno"
then
    WINE_PACKAGE_FLAGS(EGL,[egl],[-lEGL],,,
        [WINE_CHECK_SONAME(EGL,eglGetProcAddress,,,[$EGL_LIBS])])
fi
WINE_NOTICE_WITH(opengl, [test -z "$ac_cv_lib_soname_EGL"],
    [EGL ${notice_platform}development files not found])

dnl **** Check for X11 ****

AC_PATH_X
if test "$have_x" = "yes"
then
    test -z "$x_includes" || AC_SUBST([X_CFLAGS],"$X_CFLAGS -I$x_includes")
    test -z "$x_libraries" || AC_SUBST([X_LIBS],"$X_LIBS -L$x_libraries")

    ac_save_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $X_CFLAGS"

    WINE_CHECK_SONAME(X11,XCreateWindow,,,[$X_LIBS])
    WINE_CHECK_SONAME(Xext,XextCreateExtension,[X_LIBS="$X_LIBS -lXext"],,[$X_LIBS -lX11])
    X_LIBS="$X_LIBS -lX11"

    xlib_includes="#include <X11/Xlib.h>
#include <X11/Xutil.h>"

    dnl *** All of the following tests require X11/Xlib.h
    AC_CHECK_HEADERS([X11/extensions/shape.h \
                      X11/extensions/XInput2.h \
                      X11/extensions/XShm.h \
                      X11/extensions/Xfixes.h \
                      X11/extensions/Xinerama.h \
                      X11/extensions/Xrandr.h \
                      X11/extensions/Xrender.h \
                      X11/extensions/xf86vmode.h \
                      X11/extensions/xf86vmproto.h],,,
                      [$xlib_includes])

        dnl *** Check for X cursor
        AC_CHECK_HEADER([X11/Xcursor/Xcursor.h],
                        [WINE_CHECK_SONAME(Xcursor,XcursorImageLoadCursor,,,[$X_LIBS])],,
                        [$xlib_includes])
        WINE_NOTICE_WITH(xcursor,[test "x$ac_cv_lib_soname_Xcursor" = "x"],
                         [libxcursor ${notice_platform}development files not found, the Xcursor extension won't be supported.])

        dnl *** Check for X input extension
        AC_CHECK_HEADER([X11/extensions/XInput.h],
                        [WINE_CHECK_SONAME(Xi,XOpenDevice,,,[$X_LIBS])],,
                        [$xlib_includes])
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
              AC_DEFINE(HAVE_LIBXXSHM, 1, [Define if you have the X Shm extension]),,[$X_LIBS])
        fi
        WINE_NOTICE_WITH(xshm,[test "$ac_cv_lib_Xext_XShmQueryExtension" != "yes"],
                         [XShm ${notice_platform}development files not found, X Shared Memory won't be supported.])

        dnl *** Check for X shape extension
        if test "$ac_cv_header_X11_extensions_shape_h" = "yes"
        then
              AC_CHECK_LIB(Xext,XShapeQueryExtension,
	      AC_DEFINE(HAVE_LIBXSHAPE, 1, [Define if you have the X Shape extension]),,[$X_LIBS])
        fi
        WINE_NOTICE_WITH(xshape,[test "$ac_cv_lib_Xext_XShapeQueryExtension" != "yes"],
                         [XShape ${notice_platform}development files not found, XShape won't be supported.])

        dnl *** Check for XFree86 VMODE extension
        if test "$ac_cv_header_X11_extensions_xf86vmode_h" = "yes" -o "$ac_cv_header_X11_extensions_xf86vmproto_h" = "yes"
        then
            WINE_CHECK_SONAME(Xxf86vm,XF86VidModeQueryExtension,,,[$X_LIBS])
        fi
        WINE_NOTICE_WITH(xxf86vm,[test "x$ac_cv_lib_soname_Xxf86vm" = "x"],
                         [libXxf86vm ${notice_platform}development files not found, XFree86 Vidmode won't be supported.])

        dnl *** Check for Transform functions in Xrender
        if test "$ac_cv_header_X11_extensions_Xrender_h" = "yes" -a "x$ac_cv_lib_soname_X11" != "x"
        then
            WINE_CHECK_SONAME(Xrender,XRenderQueryExtension,
              [AC_CHECK_LIB(Xrender,XRenderSetPictureTransform,
                [AC_DEFINE(HAVE_XRENDERSETPICTURETRANSFORM, 1,
                 [Define if Xrender has the XRenderSetPictureTransform function])],,[$X_LIBS])
               AC_CHECK_LIB(Xrender,XRenderCreateLinearGradient,
                [AC_DEFINE(HAVE_XRENDERCREATELINEARGRADIENT, 1,
                 [Define if Xrender has the XRenderCreateLinearGradient function])],,[$X_LIBS])],,[$X_LIBS])

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
#include <X11/extensions/Xrandr.h>]], [[static typeof(XRRGetProviderResources) *f; if (f) return 0;]])],
                      [AC_DEFINE(HAVE_XRRGETPROVIDERRESOURCES, 1,
                        [Define if Xrandr has the XRRGetProviderResources function])],
                      [WINE_NOTICE([libxrandr ${notice_platform}development files too old, XRandR display device handler won't be supported.])])],,[$X_LIBS])])
        fi
        WINE_NOTICE_WITH(xrandr,[test "x$ac_cv_lib_soname_Xrandr" = "x"],
                         [libxrandr ${notice_platform}development files not found, XRandr won't be supported.])

        dnl *** Check for Xfixes extension
        if test "$ac_cv_header_X11_extensions_Xfixes_h" = "yes"
        then
                AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>]], [[static typeof(XFixesQueryVersion) * func; if (func) return 0;]])],
                  [WINE_CHECK_SONAME(Xfixes,XFixesQueryVersion,,,[$X_LIBS])])
        fi
        WINE_NOTICE_WITH(xfixes,[test "x$ac_cv_lib_soname_Xfixes" = "x"],
                         [libxfixes ${notice_platform}development files not found, Xfixes won't be supported.])

        dnl *** Check for Xinerama extension
        if test "$ac_cv_header_X11_extensions_Xinerama_h" = "yes"
        then
                AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>]], [[static typeof(XineramaQueryScreens) * func; if (func) return 0;]])],
                  [WINE_CHECK_SONAME(Xinerama,XineramaQueryScreens,,,[$X_LIBS])])
        fi
        WINE_NOTICE_WITH(xinerama,[test "x$ac_cv_lib_soname_Xinerama" = "x"],
                         [libxinerama ${notice_platform}development files not found, multi-monitor setups won't be supported.])

        dnl *** Check for X Composite extension
        AC_CHECK_HEADER([X11/extensions/Xcomposite.h],
                        [WINE_CHECK_SONAME(Xcomposite,XCompositeRedirectWindow,,,[$X_LIBS])],,
                        [$xlib_includes])
        WINE_NOTICE_WITH(xcomposite,[test "x$ac_cv_lib_soname_Xcomposite" = "x"],
                         [libxcomposite ${notice_platform}development files not found, Xcomposite won't be supported.])

        dnl *** Check for XICCallback struct
        AC_CHECK_MEMBERS([XICCallback.callback, XEvent.xcookie],,,[$xlib_includes])

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
			    $X_LIBS -lm -dylib_file /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib)],
		     $X_LIBS -lm)
    fi
    WINE_WARNING_WITH(opengl,[test -n "$opengl_msg"],[$opengl_msg
OpenGL and Direct3D won't be supported.])

    CPPFLAGS="$ac_save_CPPFLAGS"
else
    X_CFLAGS=""
    X_LIBS=""
fi

if test "x$with_wayland" != "xno"
then
    WINE_PACKAGE_FLAGS(WAYLAND_CLIENT,[wayland-client],,,,
        [AC_CHECK_HEADER([wayland-client.h],
           [AC_CHECK_LIB(wayland-client,wl_display_connect,
              [AC_PATH_PROG(WAYLAND_SCANNER,wayland-scanner,
                            [`test -n "$PKG_CONFIG" && $PKG_CONFIG --variable=wayland_scanner wayland-scanner 2>/dev/null`])],
              [WAYLAND_CLIENT_LIBS=""],[$WAYLAND_CLIENT_LIBS])])])
    WINE_PACKAGE_FLAGS(XKBCOMMON,[xkbcommon],,,,
        [AC_CHECK_HEADERS([xkbcommon/xkbcommon.h])
         AC_CHECK_LIB(xkbcommon,xkb_context_new,[:],[XKBCOMMON_LIBS=""],[$XKBCOMMON_LIBS])])
    WINE_PACKAGE_FLAGS(XKBREGISTRY,[xkbregistry],,,,
        [AC_CHECK_HEADERS([xkbcommon/xkbregistry.h])
         AC_CHECK_LIB(xkbregistry,rxkb_context_new,[:],[XKBREGISTRY_LIBS=""],[$XKBREGISTRY_LIBS])])
    if test "x$with_opengl" != "xno"
    then
        WINE_PACKAGE_FLAGS(WAYLAND_EGL,[wayland-egl],,,,
            [AC_CHECK_HEADER([wayland-egl.h],
               [AC_CHECK_LIB(wayland-egl,wl_egl_window_create,
                  [AC_DEFINE(HAVE_LIBWAYLAND_EGL, 1, [Define if we have the wayland-egl development environment])],
                  [WAYLAND_EGL_LIBS=""],[$WAYLAND_EGL_LIBS])])])
        if test "x$with_wayland" != "x"
        then
            WINE_NOTICE_WITH(opengl, [test -z "$ac_cv_lib_soname_EGL" -o "$ac_cv_lib_wayland_egl_wl_egl_window_create" != yes],
                [EGL ${notice_platform}development files not found, the Wayland driver won't support OpenGL])
        fi
    fi
fi
WINE_NOTICE_WITH(wayland, [test -z "$WAYLAND_CLIENT_LIBS" -o -z "$WAYLAND_SCANNER" -o -z "$XKBCOMMON_LIBS" -o -z "$XKBREGISTRY_LIBS" -o "$ac_cv_header_linux_input_h" = "no"],
                 [Wayland ${notice_platform}development files not found, the Wayland driver won't be supported.],
                 [enable_winewayland_drv])

if test "$enable_wineandroid_drv$enable_winemac_drv$enable_winewayland_drv" = "nonono"
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
    AC_CHECK_LIB(pcap,pcap_init,[AC_SUBST(PCAP_LIBS,["-lpcap"])])
fi
WINE_NOTICE_WITH(pcap,[test "x$ac_cv_lib_pcap_pcap_init" != xyes],
                 [pcap ${notice_platform}development files not found, wpcap won't be supported.],
                 [enable_wpcap])

dnl **** Check for libpcsclite ****
if test "x$with_pcsclite" != "xno"
then
    AC_CHECK_LIB(pcsclite,SCardEstablishContext,[AC_SUBST(PCSCLITE_LIBS,["-lpcsclite"])],
                 [case $host_os in
                    darwin*) AC_SUBST(PCSCLITE_LIBS,"-framework PCSC") ;;
                  esac])
fi
WINE_NOTICE_WITH(pcsclite,[test "x$PCSCLITE_LIBS" = x],
                 [libpcsclite not found, smart cards won't be supported.],
                 [enable_winscard])

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
case $host_os in
  darwin*) ;;
  *) WINE_NOTICE_WITH(dbus,[test "x$ac_cv_lib_soname_dbus_1" = "x"],
                      [libdbus ${notice_platform}development files not found, no dynamic device support.]) ;;
esac

dnl **** Check for libgnutls ****
if test "x$with_gnutls" != "xno"
then
    WINE_PACKAGE_FLAGS(GNUTLS,[gnutls],[-lgnutls],,,
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

dnl **** Check for SANE ****
if test "x$with_sane" != "xno"
then
    WINE_PACKAGE_FLAGS(SANE,[sane-backends],,[`${SANE_CONFIG:-sane-config} --cflags 2>/dev/null`],[`${SANE_CONFIG:-sane-config} --ldflags 2>/dev/null`],
        [AC_CHECK_HEADER(sane/sane.h,
            [AC_CHECK_LIB(sane,sane_init,[:],,[$SANE_LIBS])])])
fi
WINE_NOTICE_WITH(sane,[test "$ac_cv_lib_sane_sane_init" != "yes"],
                 [libsane ${notice_platform}development files not found, scanners won't be supported.],
                 [enable_sane_ds])

dnl **** Check for libusb ****
if test "x$with_usb" != "xno"
then
    WINE_PACKAGE_FLAGS(USB,[libusb-1.0],[-lusb-1.0],,,
        [AC_CHECK_HEADER([libusb.h],
            [AC_CHECK_LIB(usb-1.0,libusb_interrupt_event_handler,[:],[USB_LIBS=""],[$USB_LIBS])],
            [USB_LIBS=""])])
fi
WINE_NOTICE_WITH(usb,[test "$ac_cv_lib_usb_1_0_libusb_interrupt_event_handler" != "yes"],
                 [libusb-1.0 ${notice_platform}development files not found (or too old), USB devices won't be supported.],
                 [enable_wineusb_sys])

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
                        [AC_CHECK_LIB(gphoto2,gp_camera_new,[:],,[$GPHOTO2_LIBS])])])
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
                 [libgphoto2 ${notice_platform}development files not found, digital cameras won't be supported.],
                 [enable_gphoto2_ds])
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
#include <resolv.h>]],[[if (!(_res.options & RES_INIT)) res_init(); res_query("foo",ns_c_in,0,0,0); ns_initparse(0,0,0)]])],
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

    if test "x$ac_cv_have_resolv" != "xnot found"
    then
        AC_CACHE_CHECK([for res_getservers], ac_cv_have_res_getservers,
                       [ac_save_LIBS="$LIBS"
                        LIBS="$RESOLV_LIBS $LIBS"
                        AC_LINK_IFELSE([AC_LANG_PROGRAM(
        [[#include <resolv.h>]],[[res_getservers(NULL, NULL, 0);]])],[ac_cv_have_res_getservers=yes],[ac_cv_have_res_getservers=no])
	                 LIBS="$ac_save_LIBS"])
        if test "$ac_cv_have_res_getservers" = "yes"
        then
          AC_DEFINE(HAVE_RES_GETSERVERS, 1, [Define to 1 if you have the 'res_getservers' function.])
        fi
    fi
fi

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

dnl **** Check for pthread functions ****
WINE_CHECK_LIB_FUNCS(\
        pthread_getthreadid_np,
        [$PTHREAD_LIBS])

dnl **** Check for gettextpo ****
if test "x$enable_tools" != xno -a "x$with_gettextpo" = xyes
then
    if test "$ac_cv_header_gettext_po_h" = "yes"
    then
        AC_CHECK_LIB(gettextpo,po_message_msgctxt,
                     [AC_DEFINE(HAVE_LIBGETTEXTPO,1,[Define to 1 if you have the 'gettextpo' library (-lgettextpo).])
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
        [AC_CHECK_HEADER([pulse/pulseaudio.h],
            [AC_CHECK_LIB(pulse, pa_stream_is_corked,[:],[PULSE_LIBS=""],[$PULSE_LIBS])],
            [PULSE_LIBS=""])])
fi
WINE_NOTICE_WITH(pulse, [test -z "$PULSE_LIBS"],
        [libpulse ${notice_platform}development files not found or too old, Pulse won't be supported.],
        [enable_winepulse_drv])

dnl **** Check for FFmpeg ****
if test "x$with_ffmpeg" != "xno";
then
    WINE_PACKAGE_FLAGS(FFMPEG,[libavutil libavformat libavcodec],,,,
        [AC_CHECK_HEADERS([libavutil/avutil.h libavformat/avformat.h libavcodec/avcodec.h libavcodec/bsf.h])
         if test "$ac_cv_header_libavutil_avutil_h" = "yes" -a "$ac_cv_header_libavformat_avformat_h" = "yes" -a "$ac_cv_header_libavcodec_avcodec_h" = "yes"
         then
            AC_CHECK_LIB(avutil,av_log_set_callback,
              [AC_CHECK_LIB(avformat,av_find_input_format,
                [AC_CHECK_LIB(avcodec,avcodec_get_name,
                  [AC_DEFINE(HAVE_FFMPEG, 1, [Define to 1 if you have the FFmpeg libraries.])],
                  [FFMPEG_LIBS=""],[$FFMPEG_LIBS])],
                [FFMPEG_LIBS=""],[$FFMPEG_LIBS])],
              [FFMPEG_LIBS=""],[$FFMPEG_LIBS])
         else
            FFMPEG_LIBS=""
         fi])
fi
WINE_NOTICE_WITH(ffmpeg,[test "x$FFMPEG_LIBS" = x],
        [FFmpeg ${notice_platform}development files not found.])

dnl **** Check for gstreamer ****
if test "x$with_gstreamer" != "xno"
then
    WINE_PACKAGE_FLAGS(GSTREAMER,[gstreamer-1.0 gstreamer-video-1.0 gstreamer-audio-1.0 gstreamer-tag-1.0],,,,
        [AC_CHECK_HEADER([gst/gst.h],
            [AC_MSG_CHECKING([whether gint64 defined by gst/gst.h is indeed 64-bit])
              AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <gst/gst.h>]],
                                                 [[static int a[sizeof(gint64) > 4 ? 1 : -1]; if (a[0]) return 0;]])],
                [AC_MSG_RESULT([yes])
                 AC_CHECK_LIB(gstreamer-1.0,gst_pad_new,[:],,[$GSTREAMER_LIBS])],
                [AC_MSG_RESULT([no])
                 ac_glib2_broken=yes
                 enable_winegstreamer=${enable_winegstreamer:-no}
                 WINE_NOTICE([glib-2.0 pkgconfig configuration is for the wrong architecture, winegstreamer won't be built.])])])])
fi
WINE_NOTICE_WITH(gstreamer,[test "x$ac_glib2_broken" != xyes -a "x$ac_cv_lib_gstreamer_1_0_gst_pad_new" != xyes],
                 [gstreamer-1.0 base plugins ${notice_platform}development files not found, GStreamer won't be supported.],
                 [enable_winegstreamer])

dnl **** Check for ALSA 1.x ****
AC_SUBST(ALSA_LIBS,"")
if test "x$with_alsa" != "xno"
then
    WINE_PACKAGE_FLAGS(ALSA,[alsa],[-lasound],,,
        [AC_CHECK_LIB(asound,snd_pcm_hw_params_get_access_mask,[:],[ALSA_LIBS=""])])
fi
WINE_NOTICE_WITH(alsa,[test -z "$ALSA_LIBS"],
                 [libasound ${notice_platform}development files not found, Alsa won't be supported.],
                 [enable_winealsa_drv])

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
            [AC_SUBST(OSS4_CFLAGS,"$ac_oss_incl")
             AC_CHECK_LIB(ossaudio,_oss_ioctl,
                 [AC_SUBST(OSS4_LIBS,"-lossaudio")])],,
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
                AC_DEFINE(HAVE_UDEV,1,[Define to 1 if you have the 'udev' library (-ludev).]),
                [UDEV_LIBS=""],[$UDEV_LIBS])],
            [UDEV_LIBS=""])])
fi
WINE_NOTICE_WITH(udev,[test "x$UDEV_LIBS" = "x"],
                 [libudev ${notice_platform}development files not found, plug and play won't be supported.])

dnl **** Check for libunwind ****
if test $HOST_ARCH = x86_64
then
  if test "x$with_unwind" != xno
  then
    WINE_PACKAGE_FLAGS(UNWIND,[libunwind],[-lunwind],,,
       [AC_CACHE_CHECK([for unw_step],wine_cv_have_unw_step,
           [AC_LINK_IFELSE([AC_LANG_PROGRAM([[#define UNW_LOCAL_ONLY
#include <libunwind.h>]],
                [[unw_cursor_t cursor; unw_step( &cursor );]])],
                [wine_cv_have_unw_step="yes"],[wine_cv_have_unw_step="no"])])
        if test "$wine_cv_have_unw_step" = no -a -n "$UNWIND_LIBS"
        then
            save_libs=$LIBS
            dnl Force static libgcc, libunwind breaks libgcc_s unwinding
            UNWIND_LIBS="-static-libgcc $UNWIND_LIBS"
            LIBS="$UNWIND_LIBS $LIBS"
            AC_CACHE_CHECK([for unw_step in libunwind],wine_cv_have_libunwind_unw_step,
               [AC_LINK_IFELSE([AC_LANG_PROGRAM([[#define UNW_LOCAL_ONLY
#include <libunwind.h>]],
                    [[unw_cursor_t cursor; unw_step( &cursor );]])],
                    [wine_cv_have_libunwind_unw_step="yes"],
                    [wine_cv_have_libunwind_unw_step="no"])])
            LIBS=$save_libs
        fi
        test "$wine_cv_have_libunwind_unw_step" = yes || UNWIND_LIBS=""
        if test "x$wine_cv_have_unw_step$wine_cv_have_libunwind_unw_step" != xnono
        then
            AC_DEFINE(HAVE_LIBUNWIND,1,[Define to 1 if you have the 'unwind' library (-lunwind).])
        fi])
  fi
  case $host in
  *-darwin*)
    WINE_NOTICE_WITH(unwind,[test "x$wine_cv_have_unw_step$wine_cv_have_libunwind_unw_step" = xnono],
                     [libunwind ${notice_platform}development files not found, stack unwinding won't work.]) ;;
  esac
fi

dnl **** Check for libSDL2 ****
if test "x$with_sdl" != "xno"
then
    WINE_PACKAGE_FLAGS(SDL2,[sdl2],[-lSDL2],,,
        [AC_CHECK_HEADERS([SDL.h],
            [WINE_CHECK_SONAME(SDL2,SDL_Init,,,[$SDL2_LIBS],[[libSDL2-2.0*]])])])
fi
WINE_NOTICE_WITH(sdl,[test "x$ac_cv_lib_soname_SDL2" = "x"],
                 [libSDL2 ${notice_platform}development files not found, SDL2 won't be supported.])

dnl **** Check for capi4linux ****
if test "x$with_capi" != "xno"
then
    WINE_PACKAGE_FLAGS(CAPI20,[capi20],,,,
     dnl Check for broken kernel header that doesn't define __user
    [AC_CHECK_HEADERS([capi20.h linux/capi.h],,,[#define __user])
    if test "$ac_cv_header_capi20_h" = "yes" -a "$ac_cv_header_linux_capi_h" = "yes"
    then
        AC_CHECK_LIB(capi20,capi20_register,[:],[CAPI20_LIBS=""],[$CAPI20_LIBS])
    fi])
fi
WINE_NOTICE_WITH(capi,[test "x$ac_cv_lib_capi20_capi20_register" != xyes],
                 [libcapi20 ${notice_platform}development files not found, ISDN won't be supported.],
                 [enable_capi2032])

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
        [AC_CHECK_HEADER([fontconfig/fontconfig.h],
                         [WINE_CHECK_SONAME(fontconfig,FcInit,,[FONTCONFIG_CFLAGS=""],[$FONTCONFIG_LIBS])],
                         [FONTCONFIG_CFLAGS=""])])
fi
WINE_NOTICE_WITH(fontconfig,[test "x$ac_cv_lib_soname_fontconfig" = "x"],
                 [fontconfig ${notice_platform}development files not found, fontconfig won't be supported.])

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
fi
WINE_NOTICE_WITH(krb5,[test "x$ac_cv_lib_soname_krb5" = "x"],
                 [libkrb5 ${notice_platform}development files not found (or too old), Kerberos won't be supported.])
test "x$ac_cv_lib_soname_krb5" != "x" || with_gssapi=${with_gssapi:-no}

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

dnl **** Check for libprocstat ****
if test "$ac_cv_header_libprocstat_h" = "yes"
then
    AC_CHECK_LIB(procstat,procstat_open_sysctl,
                 [AC_DEFINE(HAVE_LIBPROCSTAT, 1, [Define to 1 if you have the 'procstat' library (-lprocstat).])
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
WINE_NOTICE_WITH(netapi,[test "x$ac_cv_lib_soname_netapi" = "x"],
                 [libnetapi not found, Samba NetAPI won't be supported.],
                 [enable_netapi])


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
        WINE_CHECK_SONAME(MoltenVK, vkGetInstanceProcAddr, [AC_DEFINE_UNQUOTED(SONAME_LIBVULKAN,["$ac_cv_lib_soname_MoltenVK"])])
    fi
fi
WINE_NOTICE_WITH(vulkan,[test "x$ac_cv_lib_soname_vulkan" = "x" -a "x$ac_cv_lib_soname_MoltenVK" = "x"],
                 [libvulkan and libMoltenVK ${notice_platform}development files not found, Vulkan won't be supported.])

dnl **** Check for gcc specific options ****

if test "x${GCC}" = "xyes"
then
  EXTRACFLAGS="$EXTRACFLAGS -Wall -pipe"

  dnl Check for some compiler flags
  dnl clang needs to be told to fail on unknown options
  saved_CFLAGS=$CFLAGS
  WINE_TRY_CFLAGS([-Werror=unknown-warning-option],[CFLAGS="$CFLAGS -Werror=unknown-warning-option"])
  WINE_TRY_CFLAGS([-Werror=unused-command-line-argument],[CFLAGS="$CFLAGS -Werror=unused-command-line-argument"])
  WINE_TRY_CFLAGS([-Werror=ignored-optimization-argument],[CFLAGS="$CFLAGS -Werror=ignored-optimization-argument"])
  WINE_TRY_CFLAGS([-fcf-protection=none])
  WINE_TRY_CFLAGS([-fvisibility=hidden])
  WINE_TRY_CFLAGS([-fno-stack-protector])
  WINE_TRY_CFLAGS([-fno-strict-aliasing])
  WINE_TRY_CFLAGS([-Wdeclaration-after-statement])
  WINE_TRY_CFLAGS([-Wempty-body])
  WINE_TRY_CFLAGS([-Wignored-qualifiers])
  WINE_TRY_CFLAGS([-Winit-self])
  WINE_TRY_CFLAGS([-Wlogical-op])
  WINE_TRY_CFLAGS([-Wpacked-not-aligned],[EXTRACFLAGS="$EXTRACFLAGS -Wno-packed-not-aligned"])
  WINE_TRY_CFLAGS([-Wpointer-arith])
  WINE_TRY_CFLAGS([-Wshift-overflow=2])
  WINE_TRY_CFLAGS([-Wstrict-prototypes])
  WINE_TRY_CFLAGS([-Wtype-limits])
  WINE_TRY_CFLAGS([-Wunused-but-set-parameter])
  WINE_TRY_CFLAGS([-Wvla])
  WINE_TRY_CFLAGS([-Wwrite-strings])

  dnl GCC can't handle large files when -Wmisleading-indentation is enabled (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=89549)
  WINE_TRY_CFLAGS([-flarge-source-files -Wmisleading-indentation],[EXTRACFLAGS="$EXTRACFLAGS -Wno-misleading-indentation"])

  dnl Check for 64-bit compare-and-swap
  AC_CACHE_CHECK([for flags needed for 64-bit compare-and-swap support], wine_cv_64bit_compare_swap,
      [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#ifndef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_8
#error no
#endif]])],
          [wine_cv_64bit_compare_swap="none needed"],
          [case $HOST_ARCH in
            i386) wine_cv_64bit_compare_swap="-march=i586" ;;
            arm)  wine_cv_64bit_compare_swap="-march=armv7-a" ;;
            *)    wine_cv_64bit_compare_swap="unknown" ;;
           esac
           if test "x$wine_cv_64bit_compare_swap" != xunknown
           then
               dnl try with the new flags
               CFLAGS="$CFLAGS $wine_cv_64bit_compare_swap"
               AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#ifndef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_8
#error no
#endif]])],,[wine_cv_64bit_compare_swap="unknown"])
               CFLAGS=$saved_CFLAGS
           fi])])
  case "$wine_cv_64bit_compare_swap" in
      unknown) AC_MSG_ERROR([gcc doesn't support 64-bit compare-and-swap on this platform]) ;;
      "none needed") ;;
      *) EXTRACFLAGS="$EXTRACFLAGS $wine_cv_64bit_compare_swap" ;;
  esac

  dnl Determine debug info format
  ac_debug_format_seen=""
  for ac_flag in $CFLAGS; do
    case $ac_flag in
      -gdwarf*)    ac_debug_format_seen=yes ;;
      -g)          ac_debug_format_seen=${ac_debug_format_seen:-default} ;;
    esac
  done
  if test "x$ac_debug_format_seen" = xdefault
  then
    WINE_TRY_CFLAGS([-gdwarf-4])
  fi

  dnl Disable gcc builtins except for Mingw
  AC_SUBST(MSVCRTFLAGS,"")
  case $host_os in
    mingw32*)  dnl Check for delayload linker support
       WINE_TRY_CFLAGS([-Wl,-delayload,autoconftest.dll],[AC_SUBST(DELAYLOADFLAG,["-Wl,-delayload,"])]) ;;
    *) MSVCRTFLAGS="-D_WIN32"
       WINE_TRY_CFLAGS([-fno-builtin],[MSVCRTFLAGS="$MSVCRTFLAGS -fno-builtin"])
       WINE_TRY_CFLAGS([-fshort-wchar],[MSVCRTFLAGS="$MSVCRTFLAGS -fshort-wchar"])
       WINE_TRY_CFLAGS([-Wno-format],[MSVCRTFLAGS="$MSVCRTFLAGS -Wno-format"]) ;;
  esac

  case $HOST_ARCH in
    dnl gcc-4.6+ omits frame pointers by default, breaking some copy protections
    i386) WINE_TRY_CFLAGS([-fno-omit-frame-pointer],[MSVCRTFLAGS="$MSVCRTFLAGS -fno-omit-frame-pointer"]) ;;
    x86_64)
      case $host_os in
        dnl Mingw uses Windows 64-bit types, not Unix ones
        cygwin*|mingw32*) WINE_TRY_CFLAGS([-Wno-format]) ;;
        dnl Default to ms_abi on 64-bit
        *) AC_CACHE_CHECK([for working -mabi=ms], ac_cv_mabi_ms,
               [CFLAGS="$CFLAGS -mabi=ms"
                AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdarg.h>
void a(int b, ...) { __builtin_ms_va_list list; __builtin_ms_va_start(list,b); }]])],
                   [ac_cv_mabi_ms=yes],[ac_cv_mabi_ms=no])
                CFLAGS=$saved_CFLAGS])
           AS_VAR_IF([ac_cv_mabi_ms],[yes],
                     [MSVCRTFLAGS="$MSVCRTFLAGS -mabi=ms"],
                     [test -n "$PE_ARCHS" || AC_MSG_ERROR([The compiler doesn't support -mabi=ms. Use gcc instead of clang, or install mingw-w64.])
                      DLLEXT=""]) ;;
      esac ;;
    arm)
      WINE_TRY_CFLAGS([-Wincompatible-function-pointer-types],[EXTRACFLAGS="$EXTRACFLAGS -Wno-error=incompatible-function-pointer-types"]) ;;
  esac

  CFLAGS=$saved_CFLAGS

  dnl Enable -Werror
  if test "x$enable_werror" = "xyes"
  then
      WINE_TRY_CFLAGS([-Werror])
  fi

  AS_VAR_IF([enable_build_id],[yes],[WINE_TRY_CFLAGS([-Wl,--build-id],[LDFLAGS="$LDFLAGS -Wl,--build-id"])])
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
   DLLFLAGS="$DLLFLAGS -fasynchronous-unwind-tables"
   LDDLLFLAGS="$LDDLLFLAGS -fasynchronous-unwind-tables"
   UNIXDLLFLAGS="$UNIXDLLFLAGS -fasynchronous-unwind-tables"
elif test $HOST_ARCH = x86_64
then
   WINE_WARNING([building 64-bit Wine without support for CFI directives; exception handling will not work properly.])
fi

dnl **** Platform-specific checks ****

AC_SUBST(WINELOADER_PROGRAMS,"wine")

case $host_os in
  linux*)
    if test $HOST_ARCH != unknown
    then
      wine_use_preloader=yes
    fi
    ;;
esac

if test "$wine_use_preloader" = "yes"
then
  WINELOADER_PROGRAMS="$WINELOADER_PROGRAMS wine-preloader"
  AC_DEFINE(HAVE_WINE_PRELOADER, 1, [Define to 1 if the Wine preloader is being used.])
fi

dnl **** Check for functions ****

dnl Check for -ldl
AC_SEARCH_LIBS(dlopen, dl)

ac_save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $BUILTINFLAG"
AC_CHECK_FUNCS(\
        dladdr1 \
	dlinfo \
	epoll_create \
	epoll_pwait2 \
	fstatfs \
	futimens \
	futimes \
	futimesat \
	getaddrinfo \
	getattrlist \
	getauxval \
	getifaddrs \
	getrandom \
	kqueue \
	pipe2 \
	port_create \
	posix_fadvise \
	posix_fallocate \
	prctl \
	sched_getcpu \
	sched_yield \
	setproctitle \
	setprogname \
	sigprocmask \
	sysinfo \
	tcdrain \
	thr_kill2
)
CFLAGS="$ac_save_CFLAGS"

dnl Check for clock_gettime which may be in -lrt
ac_save_LIBS=$LIBS
AC_SEARCH_LIBS(clock_gettime, rt,
               [AC_DEFINE(HAVE_CLOCK_GETTIME, 1, [Define to 1 if you have the 'clock_gettime' function.])
                test "$ac_res" = "none required" || AC_SUBST(RT_LIBS,"$ac_res")])
LIBS=$ac_save_LIBS

AC_CACHE_CHECK([for sched_setaffinity],wine_cv_have_sched_setaffinity,
                AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[#include <sched.h>]], [[sched_setaffinity(0, 0, 0);]])],[wine_cv_have_sched_setaffinity=yes],[wine_cv_have_sched_setaffinity=no]))
if test "$wine_cv_have_sched_setaffinity" = "yes"
then
  AC_DEFINE(HAVE_SCHED_SETAFFINITY, 1, [Define to 1 if you have the 'sched_setaffinity' function.])
fi

dnl **** Check for types ****

AC_CHECK_TYPES([request_sense],,,[#include <linux/cdrom.h>])

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

dnl Check for socket structure members
AC_CHECK_MEMBERS([struct sockaddr_un.sun_len],,,
[#include <sys/types.h>
#include <sys/socket.h>
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

dnl Check for stat.st_blocks and ns-resolved times
AC_CHECK_MEMBERS([
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
[#include <sys/types.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif])

dnl Check for struct __res_state._u._ext.nscount6
AC_CHECK_MEMBERS([struct __res_state._u._ext.nscount6],,,
[#ifdef HAVE_RESOLV_H
#include <resolv.h>
#endif])

dnl Check for struct in6_pktinfo
AC_CHECK_MEMBERS([struct in6_pktinfo.ipi6_addr],,,
[#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif])

dnl Check for struct ipstat
AC_CHECK_MEMBERS([struct ipstat.ips_total],,,
[#include <sys/types.h>
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

dnl Check for struct ip6stat
AC_CHECK_MEMBERS([struct ip6stat.ip6s_total],,,
[#include <sys/types.h>
#ifdef HAVE_SYS_SOCKETVAR_H
#include <sys/socketvar.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET6_IP6_VAR_H
#include <netinet6/ip6_var.h>
#endif])

dnl Check for struct icmpstat
AC_CHECK_MEMBERS([struct icmpstat.icps_error],,,
[#include <sys/types.h>
#ifdef HAVE_SYS_SOCKETVAR_H
#include <sys/socketvar.h>
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

dnl Check for struct icmp6stat
AC_CHECK_MEMBERS([struct icmp6stat.icp6s_error],,,
[#include <sys/types.h>
#ifdef HAVE_SYS_SOCKETVAR_H
#include <sys/socketvar.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_ICMP6_H
#include <netinet/icmp6.h>
#endif])

dnl Check for struct tcpstat
AC_CHECK_MEMBERS([struct tcpstat.tcps_connattempt],,,
[#include <sys/types.h>
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
[#include <sys/types.h>
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
[#include <sys/types.h>
#ifdef HAVE_NET_IF_H
# include <net/if.h>
#endif])

dnl Check for struct sysinfo members totalram and mem_unit
AC_CHECK_MEMBERS([struct sysinfo.totalram, struct sysinfo.mem_unit],,,
[#ifdef HAVE_SYS_SYSINFO_H
# include <sys/sysinfo.h>
#endif])

LIBS="$ac_save_LIBS"

dnl Check for __builtin_popcount
AC_CACHE_CHECK([for __builtin_popcount], ac_cv_have___builtin_popcount,
               AC_LINK_IFELSE([AC_LANG_PROGRAM(,[[return __builtin_popcount(1)]])],
               [ac_cv_have___builtin_popcount="yes"], [ac_cv_have___builtin_popcount="no"]))
if test "$ac_cv_have___builtin_popcount" = "yes"
then
    AC_DEFINE(HAVE___BUILTIN_POPCOUNT, 1, [Define to 1 if you have the '__builtin_popcount' built-in function.])
fi

AC_CACHE_CHECK([for __clear_cache], ac_cv_have___clear_cache,
               AC_LINK_IFELSE([AC_LANG_PROGRAM(,[[__clear_cache((void*)0, (void*)0); return 0;]])],
               [ac_cv_have___clear_cache="yes"], [ac_cv_have___clear_cache="no"]))
if test "$ac_cv_have___clear_cache" = "yes"
then
    AC_DEFINE(HAVE___CLEAR_CACHE, 1, [Define to 1 if you have the '__clear_cache' (potentially built-in) function.])
fi

dnl *** check for the need to define platform-specific symbols

case $host_cpu in
  *i[[3456789]]86*) WINE_CHECK_DEFINE([__i386__]) ;;
  *x86_64*)         WINE_CHECK_DEFINE([__x86_64__]) ;;
  *powerpc64*)      WINE_CHECK_DEFINE([__powerpc64__]) ;;
  *aarch64*)        WINE_CHECK_DEFINE([__aarch64__]) ;;
  *arm*)            WINE_CHECK_DEFINE([__arm__]) ;;
esac

dnl *** Check for modules to disable by default

enable_vcruntime140_1=${enable_vcruntime140_1:-x86_64,arm64ec}

if test -n "$PE_ARCHS"
then
    enable_wow64=${enable_wow64:-aarch64,x86_64}
    enable_wow64win=${enable_wow64win:-aarch64,x86_64}
    enable_wow64cpu=${enable_wow64cpu:-x86_64}
    enable_xtajit64=${enable_xtajit64:-arm64ec}
else
    enable_wow64=${enable_wow64:-no}
    enable_wow64win=${enable_wow64win:-no}
    enable_wow64cpu=${enable_wow64cpu:-no}
    enable_xtajit64=${enable_xtajit64:-no}
    enable_compiler_rt=${enable_compiler_rt:-no}
fi

dnl Disable some programs for other PE archs
enable_conhost=${enable_conhost:-$HOST_ARCH}
enable_plugplay=${enable_plugplay:-$HOST_ARCH}
enable_rpcss=${enable_rpcss:-$HOST_ARCH}
enable_services=${enable_services:-$HOST_ARCH}
enable_spoolsv=${enable_spoolsv:-$HOST_ARCH}
enable_termsv=${enable_termsv:-$HOST_ARCH}
enable_wineboot=${enable_wineboot:-$HOST_ARCH}
enable_winemenubuilder=${enable_winemenubuilder:-$HOST_ARCH}
enable_wuauserv=${enable_wuauserv:-$HOST_ARCH}

dnl Explicitly enable some programs for arm64ec
enable_cmd=${enable_cmd:-yes}
enable_dllhost=${enable_dllhost:-yes}
enable_dpnsvr=${enable_dpnsvr:-i386,x86_64,arm64ec}
enable_dxdiag=${enable_dxdiag:-yes}
enable_msiexec=${enable_msiexec:-yes}
enable_netsh=${enable_netsh:-yes}
enable_regsvr32=${enable_regsvr32:-yes}
enable_rundll32=${enable_rundll32:-yes}

dnl Disable Win16-related modules
enable_win16=${enable_win16:-i386}
enable_w32skrnl=${enable_w32skrnl:-$enable_win16}
enable_wow32=${enable_wow32:-$enable_win16}
enable_winevdm=${enable_winevdm:-$enable_win16}

dnl Disable winetest too if tests are disabled
enable_winetest=${enable_winetest:-$enable_tests}

dnl **** Generate output files ****

AH_TOP([#ifndef __WINE_CONFIG_H
#define __WINE_CONFIG_H])
AH_BOTTOM([#endif /* __WINE_CONFIG_H */])

AC_CONFIG_COMMANDS([include/stamp-h], [echo timestamp > include/stamp-h])
AS_ECHO_N("creating Makefile rules...") >&AS_MESSAGE_FD

makedep_flags=" -C"
test "x$enable_silent_rules" = xyes && makedep_flags="$makedep_flags -S"

wine_srcdir=
test "$srcdir" = . || wine_srcdir="$srcdir/"

WINE_CONFIG_MAKEFILE(dlls/acledit)
WINE_CONFIG_MAKEFILE(dlls/aclui)
WINE_CONFIG_MAKEFILE(dlls/activeds.tlb)
WINE_CONFIG_MAKEFILE(dlls/activeds)
WINE_CONFIG_MAKEFILE(dlls/activeds/tests)
WINE_CONFIG_MAKEFILE(dlls/actxprxy)
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
WINE_CONFIG_MAKEFILE(dlls/apisetschema)
WINE_CONFIG_MAKEFILE(dlls/apphelp)
WINE_CONFIG_MAKEFILE(dlls/apphelp/tests)
WINE_CONFIG_MAKEFILE(dlls/appwiz.cpl)
WINE_CONFIG_MAKEFILE(dlls/appxdeploymentclient)
WINE_CONFIG_MAKEFILE(dlls/atl)
WINE_CONFIG_MAKEFILE(dlls/atl/tests)
WINE_CONFIG_MAKEFILE(dlls/atl100)
WINE_CONFIG_MAKEFILE(dlls/atl100/tests)
WINE_CONFIG_MAKEFILE(dlls/atl110)
WINE_CONFIG_MAKEFILE(dlls/atl110/tests)
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
WINE_CONFIG_MAKEFILE(dlls/avifile.dll16)
WINE_CONFIG_MAKEFILE(dlls/avrt)
WINE_CONFIG_MAKEFILE(dlls/bcp47langs)
WINE_CONFIG_MAKEFILE(dlls/bcrypt)
WINE_CONFIG_MAKEFILE(dlls/bcrypt/tests)
WINE_CONFIG_MAKEFILE(dlls/bcryptprimitives)
WINE_CONFIG_MAKEFILE(dlls/bluetoothapis)
WINE_CONFIG_MAKEFILE(dlls/bluetoothapis/tests)
WINE_CONFIG_MAKEFILE(dlls/browseui)
WINE_CONFIG_MAKEFILE(dlls/browseui/tests)
WINE_CONFIG_MAKEFILE(dlls/bthprops.cpl)
WINE_CONFIG_MAKEFILE(dlls/cabinet)
WINE_CONFIG_MAKEFILE(dlls/cabinet/tests)
WINE_CONFIG_MAKEFILE(dlls/capi2032)
WINE_CONFIG_MAKEFILE(dlls/cards)
WINE_CONFIG_MAKEFILE(dlls/cdosys)
WINE_CONFIG_MAKEFILE(dlls/cfgmgr32)
WINE_CONFIG_MAKEFILE(dlls/cfgmgr32/tests)
WINE_CONFIG_MAKEFILE(dlls/clusapi)
WINE_CONFIG_MAKEFILE(dlls/cng.sys)
WINE_CONFIG_MAKEFILE(dlls/colorcnv)
WINE_CONFIG_MAKEFILE(dlls/combase)
WINE_CONFIG_MAKEFILE(dlls/combase/tests)
WINE_CONFIG_MAKEFILE(dlls/comcat)
WINE_CONFIG_MAKEFILE(dlls/comcat/tests)
WINE_CONFIG_MAKEFILE(dlls/comctl32)
WINE_CONFIG_MAKEFILE(dlls/comctl32/tests)
WINE_CONFIG_MAKEFILE(dlls/comdlg32)
WINE_CONFIG_MAKEFILE(dlls/comdlg32/tests)
WINE_CONFIG_MAKEFILE(dlls/coml2)
WINE_CONFIG_MAKEFILE(dlls/comm.drv16)
WINE_CONFIG_MAKEFILE(dlls/commdlg.dll16)
WINE_CONFIG_MAKEFILE(dlls/compobj.dll16)
WINE_CONFIG_MAKEFILE(dlls/compstui)
WINE_CONFIG_MAKEFILE(dlls/compstui/tests)
WINE_CONFIG_MAKEFILE(dlls/comsvcs)
WINE_CONFIG_MAKEFILE(dlls/comsvcs/tests)
WINE_CONFIG_MAKEFILE(dlls/concrt140)
WINE_CONFIG_MAKEFILE(dlls/concrt140/tests)
WINE_CONFIG_MAKEFILE(dlls/connect)
WINE_CONFIG_MAKEFILE(dlls/coremessaging)
WINE_CONFIG_MAKEFILE(dlls/coremessaging/tests)
WINE_CONFIG_MAKEFILE(dlls/credui)
WINE_CONFIG_MAKEFILE(dlls/credui/tests)
WINE_CONFIG_MAKEFILE(dlls/crtdll)
WINE_CONFIG_MAKEFILE(dlls/crypt32)
WINE_CONFIG_MAKEFILE(dlls/crypt32/tests)
WINE_CONFIG_MAKEFILE(dlls/cryptbase)
WINE_CONFIG_MAKEFILE(dlls/cryptdlg)
WINE_CONFIG_MAKEFILE(dlls/cryptdll)
WINE_CONFIG_MAKEFILE(dlls/cryptext)
WINE_CONFIG_MAKEFILE(dlls/cryptext/tests)
WINE_CONFIG_MAKEFILE(dlls/cryptnet)
WINE_CONFIG_MAKEFILE(dlls/cryptnet/tests)
WINE_CONFIG_MAKEFILE(dlls/cryptowinrt)
WINE_CONFIG_MAKEFILE(dlls/cryptowinrt/tests)
WINE_CONFIG_MAKEFILE(dlls/cryptsp)
WINE_CONFIG_MAKEFILE(dlls/cryptui)
WINE_CONFIG_MAKEFILE(dlls/cryptui/tests)
WINE_CONFIG_MAKEFILE(dlls/ctapi32)
WINE_CONFIG_MAKEFILE(dlls/ctl3d.dll16)
WINE_CONFIG_MAKEFILE(dlls/ctl3d32)
WINE_CONFIG_MAKEFILE(dlls/ctl3dv2.dll16)
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
WINE_CONFIG_MAKEFILE(dlls/d3d12core)
WINE_CONFIG_MAKEFILE(dlls/d3d8)
WINE_CONFIG_MAKEFILE(dlls/d3d8/tests)
WINE_CONFIG_MAKEFILE(dlls/d3d8thk)
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
WINE_CONFIG_MAKEFILE(dlls/d3dcompiler_42/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dcompiler_43)
WINE_CONFIG_MAKEFILE(dlls/d3dcompiler_43/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dcompiler_46)
WINE_CONFIG_MAKEFILE(dlls/d3dcompiler_46/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dcompiler_47)
WINE_CONFIG_MAKEFILE(dlls/d3dcompiler_47/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dim)
WINE_CONFIG_MAKEFILE(dlls/d3dim700)
WINE_CONFIG_MAKEFILE(dlls/d3drm)
WINE_CONFIG_MAKEFILE(dlls/d3drm/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_33)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_34)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_34/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_35)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_35/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_36)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_36/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_37)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_37/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_38)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_38/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_39)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_39/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_40)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_40/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_41)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_41/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_42)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_42/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_43)
WINE_CONFIG_MAKEFILE(dlls/d3dx10_43/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dx11_42)
WINE_CONFIG_MAKEFILE(dlls/d3dx11_42/tests)
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
WINE_CONFIG_MAKEFILE(dlls/d3dx9_35/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_36)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_36/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_37)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_38)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_39)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_40)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_41)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_42)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_42/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_43)
WINE_CONFIG_MAKEFILE(dlls/d3dx9_43/tests)
WINE_CONFIG_MAKEFILE(dlls/d3dxof)
WINE_CONFIG_MAKEFILE(dlls/d3dxof/tests)
WINE_CONFIG_MAKEFILE(dlls/dataexchange)
WINE_CONFIG_MAKEFILE(dlls/dataexchange/tests)
WINE_CONFIG_MAKEFILE(dlls/davclnt)
WINE_CONFIG_MAKEFILE(dlls/dbgeng)
WINE_CONFIG_MAKEFILE(dlls/dbgeng/tests)
WINE_CONFIG_MAKEFILE(dlls/dbghelp)
WINE_CONFIG_MAKEFILE(dlls/dbghelp/tests)
WINE_CONFIG_MAKEFILE(dlls/dciman32)
WINE_CONFIG_MAKEFILE(dlls/dcomp)
WINE_CONFIG_MAKEFILE(dlls/ddeml.dll16)
WINE_CONFIG_MAKEFILE(dlls/ddraw)
WINE_CONFIG_MAKEFILE(dlls/ddraw/tests)
WINE_CONFIG_MAKEFILE(dlls/ddrawex)
WINE_CONFIG_MAKEFILE(dlls/ddrawex/tests)
WINE_CONFIG_MAKEFILE(dlls/desk.cpl)
WINE_CONFIG_MAKEFILE(dlls/devenum)
WINE_CONFIG_MAKEFILE(dlls/devenum/tests)
WINE_CONFIG_MAKEFILE(dlls/dhcpcsvc)
WINE_CONFIG_MAKEFILE(dlls/dhcpcsvc/tests)
WINE_CONFIG_MAKEFILE(dlls/dhcpcsvc6)
WINE_CONFIG_MAKEFILE(dlls/dhtmled.ocx)
WINE_CONFIG_MAKEFILE(dlls/diasymreader)
WINE_CONFIG_MAKEFILE(dlls/difxapi)
WINE_CONFIG_MAKEFILE(dlls/dinput)
WINE_CONFIG_MAKEFILE(dlls/dinput/tests)
WINE_CONFIG_MAKEFILE(dlls/dinput8)
WINE_CONFIG_MAKEFILE(dlls/directmanipulation)
WINE_CONFIG_MAKEFILE(dlls/directmanipulation/tests)
WINE_CONFIG_MAKEFILE(dlls/dispdib.dll16)
WINE_CONFIG_MAKEFILE(dlls/dispex)
WINE_CONFIG_MAKEFILE(dlls/dispex/tests)
WINE_CONFIG_MAKEFILE(dlls/display.drv16)
WINE_CONFIG_MAKEFILE(dlls/dmband)
WINE_CONFIG_MAKEFILE(dlls/dmband/tests)
WINE_CONFIG_MAKEFILE(dlls/dmcompos)
WINE_CONFIG_MAKEFILE(dlls/dmcompos/tests)
WINE_CONFIG_MAKEFILE(dlls/dmime)
WINE_CONFIG_MAKEFILE(dlls/dmime/tests)
WINE_CONFIG_MAKEFILE(dlls/dmloader)
WINE_CONFIG_MAKEFILE(dlls/dmloader/tests)
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
WINE_CONFIG_MAKEFILE(dlls/dpnhupnp)
WINE_CONFIG_MAKEFILE(dlls/dpnlobby)
WINE_CONFIG_MAKEFILE(dlls/dpvoice)
WINE_CONFIG_MAKEFILE(dlls/dpvoice/tests)
WINE_CONFIG_MAKEFILE(dlls/dpwsockx)
WINE_CONFIG_MAKEFILE(dlls/drmclien)
WINE_CONFIG_MAKEFILE(dlls/dsdmo)
WINE_CONFIG_MAKEFILE(dlls/dsdmo/tests)
WINE_CONFIG_MAKEFILE(dlls/dsound)
WINE_CONFIG_MAKEFILE(dlls/dsound/tests)
WINE_CONFIG_MAKEFILE(dlls/dsquery)
WINE_CONFIG_MAKEFILE(dlls/dssenh)
WINE_CONFIG_MAKEFILE(dlls/dssenh/tests)
WINE_CONFIG_MAKEFILE(dlls/dsuiext)
WINE_CONFIG_MAKEFILE(dlls/dswave)
WINE_CONFIG_MAKEFILE(dlls/dswave/tests)
WINE_CONFIG_MAKEFILE(dlls/dwmapi)
WINE_CONFIG_MAKEFILE(dlls/dwmapi/tests)
WINE_CONFIG_MAKEFILE(dlls/dwrite)
WINE_CONFIG_MAKEFILE(dlls/dwrite/tests)
WINE_CONFIG_MAKEFILE(dlls/dx8vb)
WINE_CONFIG_MAKEFILE(dlls/dxcore)
WINE_CONFIG_MAKEFILE(dlls/dxcore/tests)
WINE_CONFIG_MAKEFILE(dlls/dxdiagn)
WINE_CONFIG_MAKEFILE(dlls/dxdiagn/tests)
WINE_CONFIG_MAKEFILE(dlls/dxgi)
WINE_CONFIG_MAKEFILE(dlls/dxgi/tests)
WINE_CONFIG_MAKEFILE(dlls/dxtrans)
WINE_CONFIG_MAKEFILE(dlls/dxva2)
WINE_CONFIG_MAKEFILE(dlls/dxva2/tests)
WINE_CONFIG_MAKEFILE(dlls/esent)
WINE_CONFIG_MAKEFILE(dlls/evr)
WINE_CONFIG_MAKEFILE(dlls/evr/tests)
WINE_CONFIG_MAKEFILE(dlls/explorerframe)
WINE_CONFIG_MAKEFILE(dlls/explorerframe/tests)
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
WINE_CONFIG_MAKEFILE(dlls/gamingtcui)
WINE_CONFIG_MAKEFILE(dlls/gdi.exe16)
WINE_CONFIG_MAKEFILE(dlls/gdi32)
WINE_CONFIG_MAKEFILE(dlls/gdi32/tests)
WINE_CONFIG_MAKEFILE(dlls/gdiplus)
WINE_CONFIG_MAKEFILE(dlls/gdiplus/tests)
WINE_CONFIG_MAKEFILE(dlls/geolocation)
WINE_CONFIG_MAKEFILE(dlls/geolocation/tests)
WINE_CONFIG_MAKEFILE(dlls/glu32)
WINE_CONFIG_MAKEFILE(dlls/glu32/tests)
WINE_CONFIG_MAKEFILE(dlls/gphoto2.ds)
WINE_CONFIG_MAKEFILE(dlls/gpkcsp)
WINE_CONFIG_MAKEFILE(dlls/graphicscapture)
WINE_CONFIG_MAKEFILE(dlls/graphicscapture/tests)
WINE_CONFIG_MAKEFILE(dlls/hal)
WINE_CONFIG_MAKEFILE(dlls/hhctrl.ocx)
WINE_CONFIG_MAKEFILE(dlls/hid)
WINE_CONFIG_MAKEFILE(dlls/hid/tests)
WINE_CONFIG_MAKEFILE(dlls/hidclass.sys)
WINE_CONFIG_MAKEFILE(dlls/hidparse.sys)
WINE_CONFIG_MAKEFILE(dlls/hlink)
WINE_CONFIG_MAKEFILE(dlls/hlink/tests)
WINE_CONFIG_MAKEFILE(dlls/hnetcfg)
WINE_CONFIG_MAKEFILE(dlls/hnetcfg/tests)
WINE_CONFIG_MAKEFILE(dlls/hrtfapo)
WINE_CONFIG_MAKEFILE(dlls/http.sys)
WINE_CONFIG_MAKEFILE(dlls/httpapi)
WINE_CONFIG_MAKEFILE(dlls/httpapi/tests)
WINE_CONFIG_MAKEFILE(dlls/hvsimanagementapi)
WINE_CONFIG_MAKEFILE(dlls/hvsimanagementapi/tests)
WINE_CONFIG_MAKEFILE(dlls/ia2comproxy)
WINE_CONFIG_MAKEFILE(dlls/iccvid)
WINE_CONFIG_MAKEFILE(dlls/icmp)
WINE_CONFIG_MAKEFILE(dlls/icmui)
WINE_CONFIG_MAKEFILE(dlls/icmui/tests)
WINE_CONFIG_MAKEFILE(dlls/ieframe)
WINE_CONFIG_MAKEFILE(dlls/ieframe/tests)
WINE_CONFIG_MAKEFILE(dlls/ieproxy)
WINE_CONFIG_MAKEFILE(dlls/iertutil)
WINE_CONFIG_MAKEFILE(dlls/iertutil/tests)
WINE_CONFIG_MAKEFILE(dlls/ifsmgr.vxd)
WINE_CONFIG_MAKEFILE(dlls/imaadp32.acm)
WINE_CONFIG_MAKEFILE(dlls/imagehlp)
WINE_CONFIG_MAKEFILE(dlls/imagehlp/tests)
WINE_CONFIG_MAKEFILE(dlls/imm.dll16)
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
WINE_CONFIG_MAKEFILE(dlls/ir50_32)
WINE_CONFIG_MAKEFILE(dlls/ir50_32/tests)
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
WINE_CONFIG_MAKEFILE(dlls/keyboard.drv16)
WINE_CONFIG_MAKEFILE(dlls/krnl386.exe16)
WINE_CONFIG_MAKEFILE(dlls/ksecdd.sys)
WINE_CONFIG_MAKEFILE(dlls/ksproxy.ax)
WINE_CONFIG_MAKEFILE(dlls/ksuser)
WINE_CONFIG_MAKEFILE(dlls/ktmw32)
WINE_CONFIG_MAKEFILE(dlls/l3codeca.acm)
WINE_CONFIG_MAKEFILE(dlls/l3codecx.ax)
WINE_CONFIG_MAKEFILE(dlls/light.msstyles)
WINE_CONFIG_MAKEFILE(dlls/loadperf)
WINE_CONFIG_MAKEFILE(dlls/localspl)
WINE_CONFIG_MAKEFILE(dlls/localspl/tests)
WINE_CONFIG_MAKEFILE(dlls/localui)
WINE_CONFIG_MAKEFILE(dlls/localui/tests)
WINE_CONFIG_MAKEFILE(dlls/lz32)
WINE_CONFIG_MAKEFILE(dlls/lz32/tests)
WINE_CONFIG_MAKEFILE(dlls/lzexpand.dll16)
WINE_CONFIG_MAKEFILE(dlls/magnification)
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
WINE_CONFIG_MAKEFILE(dlls/mfasfsrcsnk)
WINE_CONFIG_MAKEFILE(dlls/mferror)
WINE_CONFIG_MAKEFILE(dlls/mfh264enc)
WINE_CONFIG_MAKEFILE(dlls/mfmediaengine)
WINE_CONFIG_MAKEFILE(dlls/mfmediaengine/tests)
WINE_CONFIG_MAKEFILE(dlls/mfmp4srcsnk)
WINE_CONFIG_MAKEFILE(dlls/mfplat)
WINE_CONFIG_MAKEFILE(dlls/mfplat/tests)
WINE_CONFIG_MAKEFILE(dlls/mfplay)
WINE_CONFIG_MAKEFILE(dlls/mfplay/tests)
WINE_CONFIG_MAKEFILE(dlls/mfreadwrite)
WINE_CONFIG_MAKEFILE(dlls/mfreadwrite/tests)
WINE_CONFIG_MAKEFILE(dlls/mfsrcsnk)
WINE_CONFIG_MAKEFILE(dlls/mfsrcsnk/tests)
WINE_CONFIG_MAKEFILE(dlls/mgmtapi)
WINE_CONFIG_MAKEFILE(dlls/midimap)
WINE_CONFIG_MAKEFILE(dlls/mlang)
WINE_CONFIG_MAKEFILE(dlls/mlang/tests)
WINE_CONFIG_MAKEFILE(dlls/mmcndmgr)
WINE_CONFIG_MAKEFILE(dlls/mmcndmgr/tests)
WINE_CONFIG_MAKEFILE(dlls/mmdevapi)
WINE_CONFIG_MAKEFILE(dlls/mmdevapi/tests)
WINE_CONFIG_MAKEFILE(dlls/mmdevldr.vxd)
WINE_CONFIG_MAKEFILE(dlls/mmsystem.dll16)
WINE_CONFIG_MAKEFILE(dlls/monodebg.vxd)
WINE_CONFIG_MAKEFILE(dlls/mouhid.sys)
WINE_CONFIG_MAKEFILE(dlls/mountmgr.sys)
WINE_CONFIG_MAKEFILE(dlls/mouse.drv16)
WINE_CONFIG_MAKEFILE(dlls/mp3dmod)
WINE_CONFIG_MAKEFILE(dlls/mp3dmod/tests)
WINE_CONFIG_MAKEFILE(dlls/mpr)
WINE_CONFIG_MAKEFILE(dlls/mpr/tests)
WINE_CONFIG_MAKEFILE(dlls/mprapi)
WINE_CONFIG_MAKEFILE(dlls/msacm.dll16)
WINE_CONFIG_MAKEFILE(dlls/msacm32.drv)
WINE_CONFIG_MAKEFILE(dlls/msacm32)
WINE_CONFIG_MAKEFILE(dlls/msacm32/tests)
WINE_CONFIG_MAKEFILE(dlls/msado15)
WINE_CONFIG_MAKEFILE(dlls/msado15/tests)
WINE_CONFIG_MAKEFILE(dlls/msadp32.acm)
WINE_CONFIG_MAKEFILE(dlls/msasn1)
WINE_CONFIG_MAKEFILE(dlls/msasn1/tests)
WINE_CONFIG_MAKEFILE(dlls/msauddecmft)
WINE_CONFIG_MAKEFILE(dlls/mscat32)
WINE_CONFIG_MAKEFILE(dlls/mscms)
WINE_CONFIG_MAKEFILE(dlls/mscms/tests)
WINE_CONFIG_MAKEFILE(dlls/mscoree)
WINE_CONFIG_MAKEFILE(dlls/mscoree/tests)
WINE_CONFIG_MAKEFILE(dlls/mscorwks)
WINE_CONFIG_MAKEFILE(dlls/msctf)
WINE_CONFIG_MAKEFILE(dlls/msctf/tests)
WINE_CONFIG_MAKEFILE(dlls/msctfmonitor)
WINE_CONFIG_MAKEFILE(dlls/msctfp)
WINE_CONFIG_MAKEFILE(dlls/msdaps)
WINE_CONFIG_MAKEFILE(dlls/msdasql)
WINE_CONFIG_MAKEFILE(dlls/msdasql/tests)
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
WINE_CONFIG_MAKEFILE(dlls/msmpeg2vdec)
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
WINE_CONFIG_MAKEFILE(dlls/msttsengine)
WINE_CONFIG_MAKEFILE(dlls/msv1_0)
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
WINE_CONFIG_MAKEFILE(dlls/msvcp140_1)
WINE_CONFIG_MAKEFILE(dlls/msvcp140_1/tests)
WINE_CONFIG_MAKEFILE(dlls/msvcp140_2)
WINE_CONFIG_MAKEFILE(dlls/msvcp140_2/tests)
WINE_CONFIG_MAKEFILE(dlls/msvcp140_atomic_wait)
WINE_CONFIG_MAKEFILE(dlls/msvcp140_atomic_wait/tests)
WINE_CONFIG_MAKEFILE(dlls/msvcp140_codecvt_ids)
WINE_CONFIG_MAKEFILE(dlls/msvcp60)
WINE_CONFIG_MAKEFILE(dlls/msvcp60/tests)
WINE_CONFIG_MAKEFILE(dlls/msvcp70)
WINE_CONFIG_MAKEFILE(dlls/msvcp71)
WINE_CONFIG_MAKEFILE(dlls/msvcp80)
WINE_CONFIG_MAKEFILE(dlls/msvcp90)
WINE_CONFIG_MAKEFILE(dlls/msvcp90/tests)
WINE_CONFIG_MAKEFILE(dlls/msvcp_win)
WINE_CONFIG_MAKEFILE(dlls/msvcr100)
WINE_CONFIG_MAKEFILE(dlls/msvcr100/tests)
WINE_CONFIG_MAKEFILE(dlls/msvcr110)
WINE_CONFIG_MAKEFILE(dlls/msvcr110/tests)
WINE_CONFIG_MAKEFILE(dlls/msvcr120)
WINE_CONFIG_MAKEFILE(dlls/msvcr120/tests)
WINE_CONFIG_MAKEFILE(dlls/msvcr120_app)
WINE_CONFIG_MAKEFILE(dlls/msvcr70)
WINE_CONFIG_MAKEFILE(dlls/msvcr70/tests)
WINE_CONFIG_MAKEFILE(dlls/msvcr71)
WINE_CONFIG_MAKEFILE(dlls/msvcr71/tests)
WINE_CONFIG_MAKEFILE(dlls/msvcr80)
WINE_CONFIG_MAKEFILE(dlls/msvcr80/tests)
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
WINE_CONFIG_MAKEFILE(dlls/msvideo.dll16)
WINE_CONFIG_MAKEFILE(dlls/msvproc)
WINE_CONFIG_MAKEFILE(dlls/mswsock)
WINE_CONFIG_MAKEFILE(dlls/msxml)
WINE_CONFIG_MAKEFILE(dlls/msxml2)
WINE_CONFIG_MAKEFILE(dlls/msxml3)
WINE_CONFIG_MAKEFILE(dlls/msxml3/tests)
WINE_CONFIG_MAKEFILE(dlls/msxml4)
WINE_CONFIG_MAKEFILE(dlls/msxml4/tests)
WINE_CONFIG_MAKEFILE(dlls/msxml6)
WINE_CONFIG_MAKEFILE(dlls/msxml6/tests)
WINE_CONFIG_MAKEFILE(dlls/mtxdm)
WINE_CONFIG_MAKEFILE(dlls/ncrypt)
WINE_CONFIG_MAKEFILE(dlls/ncrypt/tests)
WINE_CONFIG_MAKEFILE(dlls/nddeapi)
WINE_CONFIG_MAKEFILE(dlls/ndis.sys)
WINE_CONFIG_MAKEFILE(dlls/ndis.sys/tests)
WINE_CONFIG_MAKEFILE(dlls/netapi32)
WINE_CONFIG_MAKEFILE(dlls/netapi32/tests)
WINE_CONFIG_MAKEFILE(dlls/netcfgx)
WINE_CONFIG_MAKEFILE(dlls/netcfgx/tests)
WINE_CONFIG_MAKEFILE(dlls/netio.sys)
WINE_CONFIG_MAKEFILE(dlls/netprofm)
WINE_CONFIG_MAKEFILE(dlls/netprofm/tests)
WINE_CONFIG_MAKEFILE(dlls/netutils)
WINE_CONFIG_MAKEFILE(dlls/newdev)
WINE_CONFIG_MAKEFILE(dlls/ninput)
WINE_CONFIG_MAKEFILE(dlls/ninput/tests)
WINE_CONFIG_MAKEFILE(dlls/normaliz)
WINE_CONFIG_MAKEFILE(dlls/npmshtml)
WINE_CONFIG_MAKEFILE(dlls/npptools)
WINE_CONFIG_MAKEFILE(dlls/nsi)
WINE_CONFIG_MAKEFILE(dlls/nsi/tests)
WINE_CONFIG_MAKEFILE(dlls/nsiproxy.sys)
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
WINE_CONFIG_MAKEFILE(dlls/odbc32/tests)
WINE_CONFIG_MAKEFILE(dlls/odbcbcp)
WINE_CONFIG_MAKEFILE(dlls/odbccp32)
WINE_CONFIG_MAKEFILE(dlls/odbccp32/tests)
WINE_CONFIG_MAKEFILE(dlls/odbccu32)
WINE_CONFIG_MAKEFILE(dlls/ole2.dll16)
WINE_CONFIG_MAKEFILE(dlls/ole2conv.dll16)
WINE_CONFIG_MAKEFILE(dlls/ole2disp.dll16)
WINE_CONFIG_MAKEFILE(dlls/ole2nls.dll16)
WINE_CONFIG_MAKEFILE(dlls/ole2prox.dll16)
WINE_CONFIG_MAKEFILE(dlls/ole2thk.dll16)
WINE_CONFIG_MAKEFILE(dlls/ole32)
WINE_CONFIG_MAKEFILE(dlls/ole32/tests)
WINE_CONFIG_MAKEFILE(dlls/oleacc)
WINE_CONFIG_MAKEFILE(dlls/oleacc/tests)
WINE_CONFIG_MAKEFILE(dlls/oleaut32)
WINE_CONFIG_MAKEFILE(dlls/oleaut32/tests)
WINE_CONFIG_MAKEFILE(dlls/olecli.dll16)
WINE_CONFIG_MAKEFILE(dlls/olecli32)
WINE_CONFIG_MAKEFILE(dlls/oledb32)
WINE_CONFIG_MAKEFILE(dlls/oledb32/tests)
WINE_CONFIG_MAKEFILE(dlls/oledlg)
WINE_CONFIG_MAKEFILE(dlls/oledlg/tests)
WINE_CONFIG_MAKEFILE(dlls/olepro32)
WINE_CONFIG_MAKEFILE(dlls/olesvr.dll16)
WINE_CONFIG_MAKEFILE(dlls/olesvr32)
WINE_CONFIG_MAKEFILE(dlls/olethk32)
WINE_CONFIG_MAKEFILE(dlls/opcservices)
WINE_CONFIG_MAKEFILE(dlls/opcservices/tests)
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
WINE_CONFIG_MAKEFILE(dlls/prntvpt/tests)
WINE_CONFIG_MAKEFILE(dlls/profapi)
WINE_CONFIG_MAKEFILE(dlls/propsys)
WINE_CONFIG_MAKEFILE(dlls/propsys/tests)
WINE_CONFIG_MAKEFILE(dlls/psapi)
WINE_CONFIG_MAKEFILE(dlls/psapi/tests)
WINE_CONFIG_MAKEFILE(dlls/pstorec)
WINE_CONFIG_MAKEFILE(dlls/pstorec/tests)
WINE_CONFIG_MAKEFILE(dlls/pwrshplugin)
WINE_CONFIG_MAKEFILE(dlls/qasf)
WINE_CONFIG_MAKEFILE(dlls/qasf/tests)
WINE_CONFIG_MAKEFILE(dlls/qcap)
WINE_CONFIG_MAKEFILE(dlls/qcap/tests)
WINE_CONFIG_MAKEFILE(dlls/qdvd)
WINE_CONFIG_MAKEFILE(dlls/qdvd/tests)
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
WINE_CONFIG_MAKEFILE(dlls/rasapi16.dll16)
WINE_CONFIG_MAKEFILE(dlls/rasapi32)
WINE_CONFIG_MAKEFILE(dlls/rasapi32/tests)
WINE_CONFIG_MAKEFILE(dlls/rasdlg)
WINE_CONFIG_MAKEFILE(dlls/regapi)
WINE_CONFIG_MAKEFILE(dlls/resampledmo)
WINE_CONFIG_MAKEFILE(dlls/resutils)
WINE_CONFIG_MAKEFILE(dlls/riched20)
WINE_CONFIG_MAKEFILE(dlls/riched20/tests)
WINE_CONFIG_MAKEFILE(dlls/riched32)
WINE_CONFIG_MAKEFILE(dlls/riched32/tests)
WINE_CONFIG_MAKEFILE(dlls/rometadata)
WINE_CONFIG_MAKEFILE(dlls/rometadata/tests)
WINE_CONFIG_MAKEFILE(dlls/rpcrt4)
WINE_CONFIG_MAKEFILE(dlls/rpcrt4/tests)
WINE_CONFIG_MAKEFILE(dlls/rsabase)
WINE_CONFIG_MAKEFILE(dlls/rsaenh)
WINE_CONFIG_MAKEFILE(dlls/rsaenh/tests)
WINE_CONFIG_MAKEFILE(dlls/rstrtmgr)
WINE_CONFIG_MAKEFILE(dlls/rtutils)
WINE_CONFIG_MAKEFILE(dlls/rtworkq)
WINE_CONFIG_MAKEFILE(dlls/rtworkq/tests)
WINE_CONFIG_MAKEFILE(dlls/samlib)
WINE_CONFIG_MAKEFILE(dlls/sane.ds)
WINE_CONFIG_MAKEFILE(dlls/sapi)
WINE_CONFIG_MAKEFILE(dlls/sapi/tests)
WINE_CONFIG_MAKEFILE(dlls/sas)
WINE_CONFIG_MAKEFILE(dlls/scarddlg)
WINE_CONFIG_MAKEFILE(dlls/scardsvr)
WINE_CONFIG_MAKEFILE(dlls/sccbase)
WINE_CONFIG_MAKEFILE(dlls/schannel)
WINE_CONFIG_MAKEFILE(dlls/schannel/tests)
WINE_CONFIG_MAKEFILE(dlls/schedsvc)
WINE_CONFIG_MAKEFILE(dlls/schedsvc/tests)
WINE_CONFIG_MAKEFILE(dlls/scrobj)
WINE_CONFIG_MAKEFILE(dlls/scrobj/tests)
WINE_CONFIG_MAKEFILE(dlls/scrrun)
WINE_CONFIG_MAKEFILE(dlls/scrrun/tests)
WINE_CONFIG_MAKEFILE(dlls/scsiport.sys)
WINE_CONFIG_MAKEFILE(dlls/sechost)
WINE_CONFIG_MAKEFILE(dlls/secur32)
WINE_CONFIG_MAKEFILE(dlls/secur32/tests)
WINE_CONFIG_MAKEFILE(dlls/security)
WINE_CONFIG_MAKEFILE(dlls/sensapi)
WINE_CONFIG_MAKEFILE(dlls/serialui)
WINE_CONFIG_MAKEFILE(dlls/serialui/tests)
WINE_CONFIG_MAKEFILE(dlls/setupapi)
WINE_CONFIG_MAKEFILE(dlls/setupapi/tests)
WINE_CONFIG_MAKEFILE(dlls/setupx.dll16)
WINE_CONFIG_MAKEFILE(dlls/sfc)
WINE_CONFIG_MAKEFILE(dlls/sfc_os)
WINE_CONFIG_MAKEFILE(dlls/shcore)
WINE_CONFIG_MAKEFILE(dlls/shcore/tests)
WINE_CONFIG_MAKEFILE(dlls/shdoclc)
WINE_CONFIG_MAKEFILE(dlls/shdocvw)
WINE_CONFIG_MAKEFILE(dlls/shdocvw/tests)
WINE_CONFIG_MAKEFILE(dlls/shell.dll16)
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
WINE_CONFIG_MAKEFILE(dlls/sound.drv16)
WINE_CONFIG_MAKEFILE(dlls/spoolss)
WINE_CONFIG_MAKEFILE(dlls/spoolss/tests)
WINE_CONFIG_MAKEFILE(dlls/sppc)
WINE_CONFIG_MAKEFILE(dlls/srclient)
WINE_CONFIG_MAKEFILE(dlls/srvcli)
WINE_CONFIG_MAKEFILE(dlls/srvsvc)
WINE_CONFIG_MAKEFILE(dlls/sspicli)
WINE_CONFIG_MAKEFILE(dlls/stdole2.tlb)
WINE_CONFIG_MAKEFILE(dlls/stdole32.tlb)
WINE_CONFIG_MAKEFILE(dlls/sti)
WINE_CONFIG_MAKEFILE(dlls/sti/tests)
WINE_CONFIG_MAKEFILE(dlls/storage.dll16)
WINE_CONFIG_MAKEFILE(dlls/stress.dll16)
WINE_CONFIG_MAKEFILE(dlls/strmdll)
WINE_CONFIG_MAKEFILE(dlls/svrapi)
WINE_CONFIG_MAKEFILE(dlls/sxs)
WINE_CONFIG_MAKEFILE(dlls/sxs/tests)
WINE_CONFIG_MAKEFILE(dlls/system.drv16)
WINE_CONFIG_MAKEFILE(dlls/t2embed)
WINE_CONFIG_MAKEFILE(dlls/t2embed/tests)
WINE_CONFIG_MAKEFILE(dlls/tapi32)
WINE_CONFIG_MAKEFILE(dlls/tapi32/tests)
WINE_CONFIG_MAKEFILE(dlls/taskschd)
WINE_CONFIG_MAKEFILE(dlls/taskschd/tests)
WINE_CONFIG_MAKEFILE(dlls/tbs)
WINE_CONFIG_MAKEFILE(dlls/tdh)
WINE_CONFIG_MAKEFILE(dlls/tdi.sys)
WINE_CONFIG_MAKEFILE(dlls/threadpoolwinrt)
WINE_CONFIG_MAKEFILE(dlls/threadpoolwinrt/tests)
WINE_CONFIG_MAKEFILE(dlls/toolhelp.dll16)
WINE_CONFIG_MAKEFILE(dlls/traffic)
WINE_CONFIG_MAKEFILE(dlls/twain.dll16)
WINE_CONFIG_MAKEFILE(dlls/twain_32)
WINE_CONFIG_MAKEFILE(dlls/twain_32/tests)
WINE_CONFIG_MAKEFILE(dlls/twinapi.appcore)
WINE_CONFIG_MAKEFILE(dlls/twinapi.appcore/tests)
WINE_CONFIG_MAKEFILE(dlls/typelib.dll16)
WINE_CONFIG_MAKEFILE(dlls/tzres)
WINE_CONFIG_MAKEFILE(dlls/ucrtbase)
WINE_CONFIG_MAKEFILE(dlls/ucrtbase/tests)
WINE_CONFIG_MAKEFILE(dlls/uianimation)
WINE_CONFIG_MAKEFILE(dlls/uianimation/tests)
WINE_CONFIG_MAKEFILE(dlls/uiautomationcore)
WINE_CONFIG_MAKEFILE(dlls/uiautomationcore/tests)
WINE_CONFIG_MAKEFILE(dlls/uiribbon)
WINE_CONFIG_MAKEFILE(dlls/unicows)
WINE_CONFIG_MAKEFILE(dlls/updspapi)
WINE_CONFIG_MAKEFILE(dlls/url)
WINE_CONFIG_MAKEFILE(dlls/urlmon)
WINE_CONFIG_MAKEFILE(dlls/urlmon/tests)
WINE_CONFIG_MAKEFILE(dlls/usbd.sys)
WINE_CONFIG_MAKEFILE(dlls/user.exe16)
WINE_CONFIG_MAKEFILE(dlls/user32)
WINE_CONFIG_MAKEFILE(dlls/user32/tests)
WINE_CONFIG_MAKEFILE(dlls/userenv)
WINE_CONFIG_MAKEFILE(dlls/userenv/tests)
WINE_CONFIG_MAKEFILE(dlls/usp10)
WINE_CONFIG_MAKEFILE(dlls/usp10/tests)
WINE_CONFIG_MAKEFILE(dlls/utildll)
WINE_CONFIG_MAKEFILE(dlls/uxtheme)
WINE_CONFIG_MAKEFILE(dlls/uxtheme/tests)
WINE_CONFIG_MAKEFILE(dlls/vbscript)
WINE_CONFIG_MAKEFILE(dlls/vbscript/tests)
WINE_CONFIG_MAKEFILE(dlls/vcomp)
WINE_CONFIG_MAKEFILE(dlls/vcomp/tests)
WINE_CONFIG_MAKEFILE(dlls/vcomp100)
WINE_CONFIG_MAKEFILE(dlls/vcomp110)
WINE_CONFIG_MAKEFILE(dlls/vcomp110/tests)
WINE_CONFIG_MAKEFILE(dlls/vcomp120)
WINE_CONFIG_MAKEFILE(dlls/vcomp140)
WINE_CONFIG_MAKEFILE(dlls/vcomp90)
WINE_CONFIG_MAKEFILE(dlls/vcruntime140)
WINE_CONFIG_MAKEFILE(dlls/vcruntime140_1)
WINE_CONFIG_MAKEFILE(dlls/vdhcp.vxd)
WINE_CONFIG_MAKEFILE(dlls/vdmdbg)
WINE_CONFIG_MAKEFILE(dlls/ver.dll16)
WINE_CONFIG_MAKEFILE(dlls/version)
WINE_CONFIG_MAKEFILE(dlls/version/tests)
WINE_CONFIG_MAKEFILE(dlls/vga)
WINE_CONFIG_MAKEFILE(dlls/virtdisk)
WINE_CONFIG_MAKEFILE(dlls/virtdisk/tests)
WINE_CONFIG_MAKEFILE(dlls/vmm.vxd)
WINE_CONFIG_MAKEFILE(dlls/vnbt.vxd)
WINE_CONFIG_MAKEFILE(dlls/vnetbios.vxd)
WINE_CONFIG_MAKEFILE(dlls/vssapi)
WINE_CONFIG_MAKEFILE(dlls/vtdapi.vxd)
WINE_CONFIG_MAKEFILE(dlls/vulkan-1)
WINE_CONFIG_MAKEFILE(dlls/vulkan-1/tests)
WINE_CONFIG_MAKEFILE(dlls/vwin32.vxd)
WINE_CONFIG_MAKEFILE(dlls/w32skrnl)
WINE_CONFIG_MAKEFILE(dlls/w32sys.dll16)
WINE_CONFIG_MAKEFILE(dlls/wbemdisp)
WINE_CONFIG_MAKEFILE(dlls/wbemdisp/tests)
WINE_CONFIG_MAKEFILE(dlls/wbemprox)
WINE_CONFIG_MAKEFILE(dlls/wbemprox/tests)
WINE_CONFIG_MAKEFILE(dlls/wdscore)
WINE_CONFIG_MAKEFILE(dlls/webservices)
WINE_CONFIG_MAKEFILE(dlls/webservices/tests)
WINE_CONFIG_MAKEFILE(dlls/websocket)
WINE_CONFIG_MAKEFILE(dlls/wer)
WINE_CONFIG_MAKEFILE(dlls/wer/tests)
WINE_CONFIG_MAKEFILE(dlls/wevtapi)
WINE_CONFIG_MAKEFILE(dlls/wevtapi/tests)
WINE_CONFIG_MAKEFILE(dlls/wevtsvc)
WINE_CONFIG_MAKEFILE(dlls/wiaservc)
WINE_CONFIG_MAKEFILE(dlls/wiaservc/tests)
WINE_CONFIG_MAKEFILE(dlls/wimgapi)
WINE_CONFIG_MAKEFILE(dlls/win32s16.dll16)
WINE_CONFIG_MAKEFILE(dlls/win32u)
WINE_CONFIG_MAKEFILE(dlls/win32u/tests)
WINE_CONFIG_MAKEFILE(dlls/win87em.dll16)
WINE_CONFIG_MAKEFILE(dlls/winaspi.dll16)
WINE_CONFIG_MAKEFILE(dlls/windebug.dll16)
WINE_CONFIG_MAKEFILE(dlls/windows.applicationmodel)
WINE_CONFIG_MAKEFILE(dlls/windows.applicationmodel/tests)
WINE_CONFIG_MAKEFILE(dlls/windows.devices.bluetooth)
WINE_CONFIG_MAKEFILE(dlls/windows.devices.bluetooth/tests)
WINE_CONFIG_MAKEFILE(dlls/windows.devices.enumeration)
WINE_CONFIG_MAKEFILE(dlls/windows.devices.enumeration/tests)
WINE_CONFIG_MAKEFILE(dlls/windows.devices.usb)
WINE_CONFIG_MAKEFILE(dlls/windows.devices.usb/tests)
WINE_CONFIG_MAKEFILE(dlls/windows.gaming.input)
WINE_CONFIG_MAKEFILE(dlls/windows.gaming.input/tests)
WINE_CONFIG_MAKEFILE(dlls/windows.gaming.ui.gamebar)
WINE_CONFIG_MAKEFILE(dlls/windows.gaming.ui.gamebar/tests)
WINE_CONFIG_MAKEFILE(dlls/windows.globalization)
WINE_CONFIG_MAKEFILE(dlls/windows.globalization/tests)
WINE_CONFIG_MAKEFILE(dlls/windows.media.devices)
WINE_CONFIG_MAKEFILE(dlls/windows.media.devices/tests)
WINE_CONFIG_MAKEFILE(dlls/windows.media.mediacontrol)
WINE_CONFIG_MAKEFILE(dlls/windows.media.mediacontrol/tests)
WINE_CONFIG_MAKEFILE(dlls/windows.media.speech)
WINE_CONFIG_MAKEFILE(dlls/windows.media.speech/tests)
WINE_CONFIG_MAKEFILE(dlls/windows.media)
WINE_CONFIG_MAKEFILE(dlls/windows.media/tests)
WINE_CONFIG_MAKEFILE(dlls/windows.networking.connectivity)
WINE_CONFIG_MAKEFILE(dlls/windows.networking.connectivity/tests)
WINE_CONFIG_MAKEFILE(dlls/windows.networking.hostname)
WINE_CONFIG_MAKEFILE(dlls/windows.networking.hostname/tests)
WINE_CONFIG_MAKEFILE(dlls/windows.networking)
WINE_CONFIG_MAKEFILE(dlls/windows.perception.stub)
WINE_CONFIG_MAKEFILE(dlls/windows.perception.stub/tests)
WINE_CONFIG_MAKEFILE(dlls/windows.security.authentication.onlineid)
WINE_CONFIG_MAKEFILE(dlls/windows.security.authentication.onlineid/tests)
WINE_CONFIG_MAKEFILE(dlls/windows.security.credentials.ui.userconsentverifier)
WINE_CONFIG_MAKEFILE(dlls/windows.security.credentials.ui.userconsentverifier/tests)
WINE_CONFIG_MAKEFILE(dlls/windows.storage.applicationdata)
WINE_CONFIG_MAKEFILE(dlls/windows.storage.applicationdata/tests)
WINE_CONFIG_MAKEFILE(dlls/windows.storage)
WINE_CONFIG_MAKEFILE(dlls/windows.storage/tests)
WINE_CONFIG_MAKEFILE(dlls/windows.system.profile.systemid)
WINE_CONFIG_MAKEFILE(dlls/windows.system.profile.systemid/tests)
WINE_CONFIG_MAKEFILE(dlls/windows.system.profile.systemmanufacturers)
WINE_CONFIG_MAKEFILE(dlls/windows.system.profile.systemmanufacturers/tests)
WINE_CONFIG_MAKEFILE(dlls/windows.ui.xaml)
WINE_CONFIG_MAKEFILE(dlls/windows.ui.xaml/tests)
WINE_CONFIG_MAKEFILE(dlls/windows.ui)
WINE_CONFIG_MAKEFILE(dlls/windows.ui/tests)
WINE_CONFIG_MAKEFILE(dlls/windows.web)
WINE_CONFIG_MAKEFILE(dlls/windows.web/tests)
WINE_CONFIG_MAKEFILE(dlls/windowscodecs)
WINE_CONFIG_MAKEFILE(dlls/windowscodecs/tests)
WINE_CONFIG_MAKEFILE(dlls/windowscodecsext)
WINE_CONFIG_MAKEFILE(dlls/windowscodecsext/tests)
WINE_CONFIG_MAKEFILE(dlls/winealsa.drv)
WINE_CONFIG_MAKEFILE(dlls/wineandroid.drv)
WINE_CONFIG_MAKEFILE(dlls/winebth.sys)
WINE_CONFIG_MAKEFILE(dlls/winebus.sys)
WINE_CONFIG_MAKEFILE(dlls/winecoreaudio.drv)
WINE_CONFIG_MAKEFILE(dlls/winecrt0)
WINE_CONFIG_MAKEFILE(dlls/wined3d)
WINE_CONFIG_MAKEFILE(dlls/winedmo)
WINE_CONFIG_MAKEFILE(dlls/winegstreamer)
WINE_CONFIG_MAKEFILE(dlls/winehid.sys)
WINE_CONFIG_MAKEFILE(dlls/winemac.drv)
WINE_CONFIG_MAKEFILE(dlls/winemapi)
WINE_CONFIG_MAKEFILE(dlls/wineoss.drv)
WINE_CONFIG_MAKEFILE(dlls/wineps.drv)
WINE_CONFIG_MAKEFILE(dlls/wineps16.drv16)
WINE_CONFIG_MAKEFILE(dlls/winepulse.drv)
WINE_CONFIG_MAKEFILE(dlls/wineusb.sys)
WINE_CONFIG_MAKEFILE(dlls/winevulkan)
WINE_CONFIG_MAKEFILE(dlls/winewayland.drv)
WINE_CONFIG_MAKEFILE(dlls/winex11.drv)
WINE_CONFIG_MAKEFILE(dlls/winexinput.sys)
WINE_CONFIG_MAKEFILE(dlls/wing.dll16)
WINE_CONFIG_MAKEFILE(dlls/wing32)
WINE_CONFIG_MAKEFILE(dlls/wing32/tests)
WINE_CONFIG_MAKEFILE(dlls/winhttp)
WINE_CONFIG_MAKEFILE(dlls/winhttp/tests)
WINE_CONFIG_MAKEFILE(dlls/wininet)
WINE_CONFIG_MAKEFILE(dlls/wininet/tests)
WINE_CONFIG_MAKEFILE(dlls/winmm)
WINE_CONFIG_MAKEFILE(dlls/winmm/tests)
WINE_CONFIG_MAKEFILE(dlls/winnls.dll16)
WINE_CONFIG_MAKEFILE(dlls/winnls32)
WINE_CONFIG_MAKEFILE(dlls/winprint)
WINE_CONFIG_MAKEFILE(dlls/winscard)
WINE_CONFIG_MAKEFILE(dlls/winscard/tests)
WINE_CONFIG_MAKEFILE(dlls/winsock.dll16)
WINE_CONFIG_MAKEFILE(dlls/winspool.drv)
WINE_CONFIG_MAKEFILE(dlls/winspool.drv/tests)
WINE_CONFIG_MAKEFILE(dlls/winsta)
WINE_CONFIG_MAKEFILE(dlls/wintab.dll16)
WINE_CONFIG_MAKEFILE(dlls/wintab32)
WINE_CONFIG_MAKEFILE(dlls/wintab32/tests)
WINE_CONFIG_MAKEFILE(dlls/wintrust)
WINE_CONFIG_MAKEFILE(dlls/wintrust/tests)
WINE_CONFIG_MAKEFILE(dlls/wintypes)
WINE_CONFIG_MAKEFILE(dlls/wintypes/tests)
WINE_CONFIG_MAKEFILE(dlls/winusb)
WINE_CONFIG_MAKEFILE(dlls/wlanapi)
WINE_CONFIG_MAKEFILE(dlls/wlanapi/tests)
WINE_CONFIG_MAKEFILE(dlls/wlanui)
WINE_CONFIG_MAKEFILE(dlls/wldap32)
WINE_CONFIG_MAKEFILE(dlls/wldap32/tests)
WINE_CONFIG_MAKEFILE(dlls/wldp)
WINE_CONFIG_MAKEFILE(dlls/wldp/tests)
WINE_CONFIG_MAKEFILE(dlls/wmadmod)
WINE_CONFIG_MAKEFILE(dlls/wmasf)
WINE_CONFIG_MAKEFILE(dlls/wmi)
WINE_CONFIG_MAKEFILE(dlls/wmilib.sys)
WINE_CONFIG_MAKEFILE(dlls/wmiutils)
WINE_CONFIG_MAKEFILE(dlls/wmiutils/tests)
WINE_CONFIG_MAKEFILE(dlls/wmp)
WINE_CONFIG_MAKEFILE(dlls/wmp/tests)
WINE_CONFIG_MAKEFILE(dlls/wmphoto)
WINE_CONFIG_MAKEFILE(dlls/wmvcore)
WINE_CONFIG_MAKEFILE(dlls/wmvcore/tests)
WINE_CONFIG_MAKEFILE(dlls/wmvdecod)
WINE_CONFIG_MAKEFILE(dlls/wnaspi32)
WINE_CONFIG_MAKEFILE(dlls/wofutil)
WINE_CONFIG_MAKEFILE(dlls/wow32)
WINE_CONFIG_MAKEFILE(dlls/wow64)
WINE_CONFIG_MAKEFILE(dlls/wow64cpu)
WINE_CONFIG_MAKEFILE(dlls/wow64win)
WINE_CONFIG_MAKEFILE(dlls/wpc)
WINE_CONFIG_MAKEFILE(dlls/wpc/tests)
WINE_CONFIG_MAKEFILE(dlls/wpcap)
WINE_CONFIG_MAKEFILE(dlls/wpcap/tests)
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
WINE_CONFIG_MAKEFILE(dlls/xactengine2_0)
WINE_CONFIG_MAKEFILE(dlls/xactengine2_4)
WINE_CONFIG_MAKEFILE(dlls/xactengine2_7)
WINE_CONFIG_MAKEFILE(dlls/xactengine2_9)
WINE_CONFIG_MAKEFILE(dlls/xactengine3_0)
WINE_CONFIG_MAKEFILE(dlls/xactengine3_1)
WINE_CONFIG_MAKEFILE(dlls/xactengine3_2)
WINE_CONFIG_MAKEFILE(dlls/xactengine3_3)
WINE_CONFIG_MAKEFILE(dlls/xactengine3_4)
WINE_CONFIG_MAKEFILE(dlls/xactengine3_5)
WINE_CONFIG_MAKEFILE(dlls/xactengine3_6)
WINE_CONFIG_MAKEFILE(dlls/xactengine3_7)
WINE_CONFIG_MAKEFILE(dlls/xactengine3_7/tests)
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
WINE_CONFIG_MAKEFILE(dlls/xaudio2_8/tests)
WINE_CONFIG_MAKEFILE(dlls/xaudio2_9)
WINE_CONFIG_MAKEFILE(dlls/xinput1_1)
WINE_CONFIG_MAKEFILE(dlls/xinput1_2)
WINE_CONFIG_MAKEFILE(dlls/xinput1_3)
WINE_CONFIG_MAKEFILE(dlls/xinput1_3/tests)
WINE_CONFIG_MAKEFILE(dlls/xinput1_4)
WINE_CONFIG_MAKEFILE(dlls/xinput9_1_0)
WINE_CONFIG_MAKEFILE(dlls/xinputuap)
WINE_CONFIG_MAKEFILE(dlls/xmllite)
WINE_CONFIG_MAKEFILE(dlls/xmllite/tests)
WINE_CONFIG_MAKEFILE(dlls/xolehlp)
WINE_CONFIG_MAKEFILE(dlls/xpsprint)
WINE_CONFIG_MAKEFILE(dlls/xpssvcs)
WINE_CONFIG_MAKEFILE(dlls/xtajit64)
WINE_CONFIG_MAKEFILE(fonts)
WINE_CONFIG_MAKEFILE(include)
WINE_CONFIG_MAKEFILE(libs/adsiid)
WINE_CONFIG_MAKEFILE(libs/capstone)
WINE_CONFIG_MAKEFILE(libs/compiler-rt)
WINE_CONFIG_MAKEFILE(libs/dmoguids)
WINE_CONFIG_MAKEFILE(libs/dxerr8)
WINE_CONFIG_MAKEFILE(libs/dxerr9)
WINE_CONFIG_MAKEFILE(libs/dxguid)
WINE_CONFIG_MAKEFILE(libs/faudio)
WINE_CONFIG_MAKEFILE(libs/fluidsynth)
WINE_CONFIG_MAKEFILE(libs/gsm)
WINE_CONFIG_MAKEFILE(libs/jpeg)
WINE_CONFIG_MAKEFILE(libs/jxr)
WINE_CONFIG_MAKEFILE(libs/lcms2)
WINE_CONFIG_MAKEFILE(libs/ldap)
WINE_CONFIG_MAKEFILE(libs/mfuuid)
WINE_CONFIG_MAKEFILE(libs/mpg123)
WINE_CONFIG_MAKEFILE(libs/musl)
WINE_CONFIG_MAKEFILE(libs/png)
WINE_CONFIG_MAKEFILE(libs/strmbase)
WINE_CONFIG_MAKEFILE(libs/strmiids)
WINE_CONFIG_MAKEFILE(libs/tiff)
WINE_CONFIG_MAKEFILE(libs/tomcrypt)
WINE_CONFIG_MAKEFILE(libs/uuid)
WINE_CONFIG_MAKEFILE(libs/vkd3d)
WINE_CONFIG_MAKEFILE(libs/wbemuuid)
WINE_CONFIG_MAKEFILE(libs/wmcodecdspuuid)
WINE_CONFIG_MAKEFILE(libs/xml2)
WINE_CONFIG_MAKEFILE(libs/xslt)
WINE_CONFIG_MAKEFILE(libs/zlib)
WINE_CONFIG_MAKEFILE(loader)
WINE_CONFIG_MAKEFILE(nls)
WINE_CONFIG_MAKEFILE(po)
WINE_CONFIG_MAKEFILE(programs/arp)
WINE_CONFIG_MAKEFILE(programs/aspnet_regiis)
WINE_CONFIG_MAKEFILE(programs/attrib)
WINE_CONFIG_MAKEFILE(programs/cabarc)
WINE_CONFIG_MAKEFILE(programs/cacls)
WINE_CONFIG_MAKEFILE(programs/certutil)
WINE_CONFIG_MAKEFILE(programs/chcp.com)
WINE_CONFIG_MAKEFILE(programs/clock)
WINE_CONFIG_MAKEFILE(programs/cmd)
WINE_CONFIG_MAKEFILE(programs/cmd/tests)
WINE_CONFIG_MAKEFILE(programs/conhost)
WINE_CONFIG_MAKEFILE(programs/conhost/tests)
WINE_CONFIG_MAKEFILE(programs/control)
WINE_CONFIG_MAKEFILE(programs/cscript)
WINE_CONFIG_MAKEFILE(programs/dism)
WINE_CONFIG_MAKEFILE(programs/dllhost)
WINE_CONFIG_MAKEFILE(programs/dplaysvr)
WINE_CONFIG_MAKEFILE(programs/dpnsvr)
WINE_CONFIG_MAKEFILE(programs/dpvsetup)
WINE_CONFIG_MAKEFILE(programs/dxdiag)
WINE_CONFIG_MAKEFILE(programs/eject)
WINE_CONFIG_MAKEFILE(programs/expand)
WINE_CONFIG_MAKEFILE(programs/explorer)
WINE_CONFIG_MAKEFILE(programs/explorer/tests)
WINE_CONFIG_MAKEFILE(programs/extrac32)
WINE_CONFIG_MAKEFILE(programs/fc)
WINE_CONFIG_MAKEFILE(programs/fc/tests)
WINE_CONFIG_MAKEFILE(programs/find)
WINE_CONFIG_MAKEFILE(programs/find/tests)
WINE_CONFIG_MAKEFILE(programs/findstr)
WINE_CONFIG_MAKEFILE(programs/findstr/tests)
WINE_CONFIG_MAKEFILE(programs/fsutil)
WINE_CONFIG_MAKEFILE(programs/fsutil/tests)
WINE_CONFIG_MAKEFILE(programs/hh)
WINE_CONFIG_MAKEFILE(programs/hostname)
WINE_CONFIG_MAKEFILE(programs/icacls)
WINE_CONFIG_MAKEFILE(programs/icinfo)
WINE_CONFIG_MAKEFILE(programs/iexplore)
WINE_CONFIG_MAKEFILE(programs/ipconfig)
WINE_CONFIG_MAKEFILE(programs/klist)
WINE_CONFIG_MAKEFILE(programs/lodctr)
WINE_CONFIG_MAKEFILE(programs/makecab)
WINE_CONFIG_MAKEFILE(programs/mofcomp)
WINE_CONFIG_MAKEFILE(programs/mshta)
WINE_CONFIG_MAKEFILE(programs/msidb)
WINE_CONFIG_MAKEFILE(programs/msiexec)
WINE_CONFIG_MAKEFILE(programs/msinfo32)
WINE_CONFIG_MAKEFILE(programs/net)
WINE_CONFIG_MAKEFILE(programs/net/tests)
WINE_CONFIG_MAKEFILE(programs/netsh)
WINE_CONFIG_MAKEFILE(programs/netstat)
WINE_CONFIG_MAKEFILE(programs/ngen)
WINE_CONFIG_MAKEFILE(programs/notepad)
WINE_CONFIG_MAKEFILE(programs/oleview)
WINE_CONFIG_MAKEFILE(programs/ping)
WINE_CONFIG_MAKEFILE(programs/plugplay)
WINE_CONFIG_MAKEFILE(programs/pnputil)
WINE_CONFIG_MAKEFILE(programs/powershell)
WINE_CONFIG_MAKEFILE(programs/presentationfontcache)
WINE_CONFIG_MAKEFILE(programs/progman)
WINE_CONFIG_MAKEFILE(programs/reg)
WINE_CONFIG_MAKEFILE(programs/reg/tests)
WINE_CONFIG_MAKEFILE(programs/regasm)
WINE_CONFIG_MAKEFILE(programs/regedit)
WINE_CONFIG_MAKEFILE(programs/regedit/tests)
WINE_CONFIG_MAKEFILE(programs/regini)
WINE_CONFIG_MAKEFILE(programs/regsvcs)
WINE_CONFIG_MAKEFILE(programs/regsvr32)
WINE_CONFIG_MAKEFILE(programs/robocopy)
WINE_CONFIG_MAKEFILE(programs/rpcss)
WINE_CONFIG_MAKEFILE(programs/rundll.exe16)
WINE_CONFIG_MAKEFILE(programs/rundll32)
WINE_CONFIG_MAKEFILE(programs/sc)
WINE_CONFIG_MAKEFILE(programs/sc/tests)
WINE_CONFIG_MAKEFILE(programs/schtasks)
WINE_CONFIG_MAKEFILE(programs/schtasks/tests)
WINE_CONFIG_MAKEFILE(programs/sdbinst)
WINE_CONFIG_MAKEFILE(programs/secedit)
WINE_CONFIG_MAKEFILE(programs/servicemodelreg)
WINE_CONFIG_MAKEFILE(programs/services)
WINE_CONFIG_MAKEFILE(programs/services/tests)
WINE_CONFIG_MAKEFILE(programs/setx)
WINE_CONFIG_MAKEFILE(programs/shutdown)
WINE_CONFIG_MAKEFILE(programs/sort)
WINE_CONFIG_MAKEFILE(programs/spoolsv)
WINE_CONFIG_MAKEFILE(programs/start)
WINE_CONFIG_MAKEFILE(programs/subst)
WINE_CONFIG_MAKEFILE(programs/svchost)
WINE_CONFIG_MAKEFILE(programs/systeminfo)
WINE_CONFIG_MAKEFILE(programs/taskkill)
WINE_CONFIG_MAKEFILE(programs/tasklist)
WINE_CONFIG_MAKEFILE(programs/tasklist/tests)
WINE_CONFIG_MAKEFILE(programs/taskmgr)
WINE_CONFIG_MAKEFILE(programs/termsv)
WINE_CONFIG_MAKEFILE(programs/timeout)
WINE_CONFIG_MAKEFILE(programs/timeout/tests)
WINE_CONFIG_MAKEFILE(programs/uninstaller)
WINE_CONFIG_MAKEFILE(programs/unlodctr)
WINE_CONFIG_MAKEFILE(programs/view)
WINE_CONFIG_MAKEFILE(programs/wevtutil)
WINE_CONFIG_MAKEFILE(programs/where)
WINE_CONFIG_MAKEFILE(programs/whoami)
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
WINE_CONFIG_MAKEFILE(programs/winevdm)
WINE_CONFIG_MAKEFILE(programs/winhelp.exe16)
WINE_CONFIG_MAKEFILE(programs/winhlp32)
WINE_CONFIG_MAKEFILE(programs/winmgmt)
WINE_CONFIG_MAKEFILE(programs/winoldap.mod16)
WINE_CONFIG_MAKEFILE(programs/winver)
WINE_CONFIG_MAKEFILE(programs/wmic)
WINE_CONFIG_MAKEFILE(programs/wmplayer)
WINE_CONFIG_MAKEFILE(programs/wordpad)
WINE_CONFIG_MAKEFILE(programs/write)
WINE_CONFIG_MAKEFILE(programs/wscript)
WINE_CONFIG_MAKEFILE(programs/wscript/tests)
WINE_CONFIG_MAKEFILE(programs/wuauserv)
WINE_CONFIG_MAKEFILE(programs/wusa)
WINE_CONFIG_MAKEFILE(programs/xcopy)
WINE_CONFIG_MAKEFILE(programs/xcopy/tests)
WINE_CONFIG_MAKEFILE(server)
WINE_CONFIG_MAKEFILE(tools)
WINE_CONFIG_MAKEFILE(tools/sfnt2fon)
WINE_CONFIG_MAKEFILE(tools/widl)
WINE_CONFIG_MAKEFILE(tools/wine)
WINE_CONFIG_MAKEFILE(tools/winebuild)
WINE_CONFIG_MAKEFILE(tools/winedump)
WINE_CONFIG_MAKEFILE(tools/winegcc)
WINE_CONFIG_MAKEFILE(tools/winemaker)
WINE_CONFIG_MAKEFILE(tools/wmc)
WINE_CONFIG_MAKEFILE(tools/wrc)
dnl End of auto-generated output commands

dnl Main ignore file

WINE_IGNORE_FILE(TAGS)
WINE_IGNORE_FILE(tags)
WINE_IGNORE_FILE(config.log)

if test "x$enable_tools" != xno
then
    AC_CONFIG_COMMANDS([tools/makedep],[wine_fn_output_makedep || AS_EXIT],
[wine_fn_output_makedep ()
{
    AS_MKDIR_P(tools)
    $CC -c -I${wine_srcdir}tools -Iinclude -I${wine_srcdir}include -D__WINESRC__ -DWINE_UNIX_LIB $EXTRACFLAGS $CPPFLAGS $CFLAGS -o tools/makedep.o ${wine_srcdir}tools/makedep.c
    $CC -o tools/makedep$ac_exeext tools/makedep.o $LDFLAGS
}])
fi

AC_CONFIG_COMMANDS([Makefile], [wine_fn_output_makefile Makefile],
[wine_fn_output_makefile ()
{
    cat <<\_WINE_EOF >\$tmp/makefile && mv -f \$tmp/makefile \$[]1 && "$wine_makedep"$makedep_flags && return
# This Makefile understands the following targets:
#
# all (default):   build wine
# clean:           remove all intermediate files
# distclean:       also remove all files created by configure
# test:            run tests
# testclean:       clean test results to force running all tests again
# install-lib:     install libraries needed to run applications
# install-dev:     install development environment
# install:         install everything
# uninstall:       uninstall everything
# ctags:           create a tags file for vim and others.
# etags:           create a TAGS file for Emacs.

m4_set_foreach([_AC_SUBST_VARS],[var],[var = $var
])$SET_MAKE
$wine_rules
_WINE_EOF
    AS_ERROR([could not create Makefile])
}])

dnl Main makefile

AC_SUBST(SHELL,[/bin/sh])

dnl Rules for re-running configure

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

dnl Rules for wineloader

if test -z "$with_wine64"
then
    TOP_INSTALL_DEV="$TOP_INSTALL_DEV include"
    TOP_INSTALL_LIB="$TOP_INSTALL_LIB \
fonts \
loader/wine.inf \
nls \
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
      cygwin*|mingw32*|darwin*|linux-android*) ;;
      *) TOP_INSTALL_LIB="$TOP_INSTALL_LIB loader/wine.desktop" ;;
    esac
fi

dnl Rules for generated source files

WINE_APPEND_RULE(
[dlls/ntdll/unix/version.c: dummy
	@version=\`(GIT_DIR=${wine_srcdir}.git git describe HEAD 2>/dev/null || echo \"wine-\$(PACKAGE_VERSION)\") | sed -n -e '\$\$s/\(.*\)/const char wine_build[[]] = \"\\1\";/p'\` && (echo \$\$version | cmp -s - \$[@]) || echo \$\$version >\$[@] || (rm -f \$[@] && exit 1)
programs/winetest/build.rc: dummy
	@build=\"STRINGTABLE { 1 \\\"\`GIT_DIR=${wine_srcdir}.git git rev-parse HEAD 2>/dev/null\`\\\" }\" && (echo \$\$build | cmp -s - \$[@]) || echo \$\$build >\$[@] || (rm -f \$[@] && exit 1)
programs/winetest/build.nfo:
	@-\$(CC) -v 2>\$[@]
dlls/wineandroid.drv/wine-debug.apk: dlls/wineandroid.drv/build.gradle ${wine_srcdir}dlls/wineandroid.drv/AndroidManifest.xml ${wine_srcdir}dlls/wineandroid.drv/WineActivity.java ${wine_srcdir}dlls/wineandroid.drv/wine.svg
	cd dlls/wineandroid.drv && gradle -q -Psrcdir=$srcdir assembleDebug
	mv dlls/wineandroid.drv/build/outputs/apk/wine-debug.apk \$[@]])

dnl Misc rules

AC_SUBST(TAGSFLAGS,["--langmap='c:+.idl.l.rh,make:(Make*.in)'"])
WINE_APPEND_RULE(
[TAGS etags:
	rm -f TAGS
	(test -d .git && git ls-files || find -L $srcdir -name '*.[[ch]]' -print) | xargs etags -a \$(TAGSFLAGS)
tags ctags:
	rm -f tags
	(test -d .git && git ls-files || find -L $srcdir -name '*.[[ch]]' -print) | xargs ctags -a \$(TAGSFLAGS)
dummy:
.PHONY: dummy])

AS_ECHO(" done") >&AS_MESSAGE_FD
AC_OUTPUT

if test "$no_create" = "yes"
then
  exit 0
fi

WINE_PRINT_MESSAGES

AS_ECHO("
$as_me: Finished.  Do '${MAKE-make}' to compile Wine.
") >&AS_MESSAGE_FD

dnl Local Variables:
dnl comment-start: "dnl "
dnl comment-end: ""
dnl comment-start-skip: "\\bdnl\\b\\s *"
dnl compile-command: "autoreconf --warnings=all"
dnl End:
