/* src/config.h.  Generated from config.h.in by configure.  */
/* src/config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if your architecture wants/needs/can use attribute_align_arg and
   alignment checks. It is for 32bit x86... */
/* #undef ABI_ALIGN_FUN */

/* Define to use proper rounding. */
#define ACCURATE_ROUNDING 1

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define if .balign is present. */
#define ASMALIGN_BALIGN 1

/* Define if .align just takes byte count. */
/* #undef ASMALIGN_BYTE */

/* Define if .align takes 3 for alignment of 2^3=8 bytes instead of 8. */
/* #undef ASMALIGN_EXP */

/* Define if __attribute__((aligned(16))) shall be used */
#define CCALIGN 1

/* Define if debugging is enabled. */
/* #undef DEBUG */

/* The default audio output module(s) to use */
#define DEFAULT_OUTPUT_MODULE "pulse,win32,win32_wasapi,sdl"

/* Define if building with dynamcally linked libmpg123 */
/* #undef DYNAMIC_BUILD */

/* Use EFBIG as substitude for EOVERFLOW, mingw.org may lack the latter */
/* #undef EOVERFLOW */

/* Define if FIFO support is enabled. */
#define FIFO 1

/* Define if frame index should be used. */
#define FRAME_INDEX 1

/* Define if gapless is enabled. */
#define GAPLESS 1

/* Define to 1 if you have the <alc.h> header file. */
/* #undef HAVE_ALC_H */

/* Define to 1 if you have the <Alib.h> header file. */
/* #undef HAVE_ALIB_H */

/* Define to 1 if you have the <AL/alc.h> header file. */
/* #undef HAVE_AL_ALC_H */

/* Define to 1 if you have the <AL/al.h> header file. */
/* #undef HAVE_AL_AL_H */

/* Define to 1 if you have the <al.h> header file. */
/* #undef HAVE_AL_H */

/* Define to 1 if you have the <arpa/inet.h> header file. */
/* #undef HAVE_ARPA_INET_H */

/* Define to 1 if you have the <asm/audioio.h> header file. */
/* #undef HAVE_ASM_AUDIOIO_H */

/* Define to 1 if you have the `atoll' function. */
#define HAVE_ATOLL 1

/* Define to 1 if you have the <audios.h> header file. */
/* #undef HAVE_AUDIOS_H */

/* Define to 1 if you have the <AudioToolbox/AudioToolbox.h> header file. */
/* #undef HAVE_AUDIOTOOLBOX_AUDIOTOOLBOX_H */

/* Define to 1 if you have the <AudioUnit/AudioUnit.h> header file. */
/* #undef HAVE_AUDIOUNIT_AUDIOUNIT_H */

/* Define to 1 if you have the <byteswap.h> header file. */
/* #undef HAVE_BYTESWAP_H */

/* Define to 1 if you have the `clock_gettime' function. */
/* #undef HAVE_CLOCK_GETTIME */

/* Define to 1 if you have the <CoreServices/CoreServices.h> header file. */
/* #undef HAVE_CORESERVICES_CORESERVICES_H */

/* Define to 1 if you have the `ctermid' function. */
/* #undef HAVE_CTERMID */

/* Define to 1 if you have the <CUlib.h> header file. */
/* #undef HAVE_CULIB_H */

/* Define to 1 if you have the <dirent.h> header file. */
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the `dlclose' function. */
/* #undef HAVE_DLCLOSE */

/* Define to 1 if you have the <dlfcn.h> header file. */
/* #undef HAVE_DLFCN_H */

/* Define to 1 if you have the `dlopen' function. */
/* #undef HAVE_DLOPEN */

/* Define to 1 if you have the `dlsym' function. */
/* #undef HAVE_DLSYM */

/* Define to 1 if you have the `execvp' function. */
/* #undef HAVE_EXECVP */

/* Define to 1 if you have the `fork' function. */
/* #undef HAVE_FORK */

/* Define to 1 if you have the `getaddrinfo' function. */
/* #undef HAVE_GETADDRINFO */

/* Define to 1 if you have the `getuid' function. */
/* #undef HAVE_GETUID */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `iswprint' function. */
#define HAVE_ISWPRINT 1

/* Define to 1 if you have the <langinfo.h> header file. */
/* #undef HAVE_LANGINFO_H */

/* Define to 1 if you have the `m' library (-lm). */
#define HAVE_LIBM 1

/* Define to 1 if you have the `mx' library (-lmx). */
/* #undef HAVE_LIBMX */

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the <linux/soundcard.h> header file. */
/* #undef HAVE_LINUX_SOUNDCARD_H */

/* Define to 1 if you have the <locale.h> header file. */
#define HAVE_LOCALE_H 1

/* Define to 1 if you have the `lseek64' function. */
/* #undef HAVE_LSEEK64 */

/* Define to 1 if you have the <machine/soundcard.h> header file. */
/* #undef HAVE_MACHINE_SOUNDCARD_H */

/* Define to 1 if you have the `mbstowcs' function. */
#define HAVE_MBSTOWCS 1

/* Define to 1 if you have the `mkfifo' function. */
/* #undef HAVE_MKFIFO */

/* Define to 1 if you have the `mmap' function. */
/* #undef HAVE_MMAP */

/* Define to 1 if you have the <netdb.h> header file. */
/* #undef HAVE_NETDB_H */

/* Define to 1 if you have the <netinet/in.h> header file. */
/* #undef HAVE_NETINET_IN_H */

/* Define to 1 if you have the <netinet/tcp.h> header file. */
/* #undef HAVE_NETINET_TCP_H */

/* Define to 1 if you have the `nl_langinfo' function. */
/* #undef HAVE_NL_LANGINFO */

/* Define to 1 if you have the <OpenAL/alc.h> header file. */
/* #undef HAVE_OPENAL_ALC_H */

/* Define to 1 if you have the <OpenAL/al.h> header file. */
/* #undef HAVE_OPENAL_AL_H */

/* Define to 1 if you have the <os2me.h> header file. */
/* #undef HAVE_OS2ME_H */

/* Define to 1 if you have the <os2.h> header file. */
/* #undef HAVE_OS2_H */

/* Define if O_LARGEFILE flag for open(2) exists. */
/* #undef HAVE_O_LARGEFILE */

/* Define to 1 if you have the `random' function. */
/* #undef HAVE_RANDOM */

/* Define to 1 if you have the <sched.h> header file. */
#define HAVE_SCHED_H 1

/* Define to 1 if you have the `sched_setscheduler' function. */
/* #undef HAVE_SCHED_SETSCHEDULER */

/* Define to 1 if you have the `setenv' function. */
/* #undef HAVE_SETENV */

/* Define to 1 if you have the `setlocale' function. */
#define HAVE_SETLOCALE 1

/* for Win/DOS system with setmode() */
#define HAVE_SETMODE 1

/* Define to 1 if you have the `setpriority' function. */
/* #undef HAVE_SETPRIORITY */

/* Define to 1 if you have the `setuid' function. */
/* #undef HAVE_SETUID */

/* Define to 1 if you have the `shmat' function. */
/* #undef HAVE_SHMAT */

/* Define to 1 if you have the `shmctl' function. */
/* #undef HAVE_SHMCTL */

/* Define to 1 if you have the `shmdt' function. */
/* #undef HAVE_SHMDT */

/* Define to 1 if you have the `shmget' function. */
/* #undef HAVE_SHMGET */

/* Define to 1 if you have the <signal.h> header file. */
#define HAVE_SIGNAL_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the `strerror_l' function. */
/* #undef HAVE_STRERROR_L */

/* Define to 1 if you have the <strings.h> header file. */
/* #undef HAVE_STRINGS_H 1 */

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sun/audioio.h> header file. */
/* #undef HAVE_SUN_AUDIOIO_H */

/* Define to 1 if you have the <sys/audioio.h> header file. */
/* #undef HAVE_SYS_AUDIOIO_H */

/* Define to 1 if you have the <sys/audio.h> header file. */
/* #undef HAVE_SYS_AUDIO_H */

/* Define to 1 if you have the <sys/ioctl.h> header file. */
/* #undef HAVE_SYS_IOCTL_H */

/* Define to 1 if you have the <sys/ipc.h> header file. */
/* #undef HAVE_SYS_IPC_H */

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/resource.h> header file. */
/* #undef HAVE_SYS_RESOURCE_H */

/* Define to 1 if you have the <sys/select.h> header file. */
/* #undef HAVE_SYS_SELECT_H */

/* Define to 1 if you have the <sys/shm.h> header file. */
/* #undef HAVE_SYS_SHM_H */

/* Define to 1 if you have the <sys/signal.h> header file. */
/* #undef HAVE_SYS_SIGNAL_H */

/* Define to 1 if you have the <sys/socket.h> header file. */
/* #undef HAVE_SYS_SOCKET_H */

/* Define to 1 if you have the <sys/soundcard.h> header file. */
/* #undef HAVE_SYS_SOUNDCARD_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
/* #undef HAVE_SYS_TIME_H 1 */

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/wait.h> header file. */
/* #undef HAVE_SYS_WAIT_H */

/* Define this if you have the POSIX termios library */
/* #undef HAVE_TERMIOS */

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `unsetenv' function. */
/* #undef HAVE_UNSETENV */

/* Define to 1 if you have the `uselocale' function. */
/* #undef HAVE_USELOCALE */

/* Define to 1 if you have the <wchar.h> header file. */
#define HAVE_WCHAR_H 1

/* Define to 1 if you have the `wcstombs' function. */
#define HAVE_WCSTOMBS 1

/* Define to 1 if you have the `wcswidth' function. */
/* #undef HAVE_WCSWIDTH */

/* Define to 1 if you have the <wctype.h> header file. */
#define HAVE_WCTYPE_H 1

/* Define to 1 if you have the <wincon.h> header file. */
#define HAVE_WINCON_H 1

/* Define to 1 if you have the <windows.h> header file. */
#define HAVE_WINDOWS_H 1

/* Define to 1 if you have the <ws2tcpip.h> header file. */
#define HAVE_WS2TCPIP_H 1

/* for Win/DOS system with _setmode() */
#define HAVE__SETMODE 1

/* Define to indicate that float storage follows IEEE754. */
#define IEEE_FLOAT 1

/* size of the frame index seek table */
#define INDEX_SIZE 1000

/* Define if IPV6 support is enabled. */
#define IPV6 1

/* Define if we use _LARGEFILE64_SOURCE with off64_t and lseek64. */
/* #undef LFS_LARGEFILE_64 */

/* System redefines off_t when defining _FILE_OFFSET_BITS to 64. */
/* #undef LFS_SENSITIVE */

/* Define to the extension used for runtime loadable modules, say, ".so". */
#define LT_MODULE_EXT ".dll"

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Define to the shared library suffix, say, ".dylib". */
/* #undef LT_SHARED_EXT */

/* Define to the shared archive member specification, say "(shr.o)". */
/* #undef LT_SHARED_LIB_MEMBER */

/* Define to for new net123 network stack. */
/* #undef NET123 */

/* Define for executable-based networking (for HTTPS). */
/* #undef NET123_EXEC */

/* Define for winhttp networking (for HTTPS). */
#define NET123_WINHTTP 1

/* Define for wininet networking (for HTTPS). */
/* #undef NET123_WININET */

/* Define if network support is enabled. */
/* #undef NETWORK */

/* Define to disable 16 bit integer output. */
/* #undef NO_16BIT */

/* Define to disable 32 bit and 24 bit integer output. */
/* #undef NO_32BIT */

/* Define to disable 8 bit integer output. */
/* #undef NO_8BIT */

/* Define to disable downsampled decoding. */
/* #undef NO_DOWNSAMPLE */

/* Define to disable equalizer. */
/* #undef NO_EQUALIZER */

/* Define to disable error messages in combination with a return value (the
   return is left intact). */
/* #undef NO_ERETURN */

/* Define to disable error messages. */
/* #undef NO_ERRORMSG */

/* Define to disable feeder and buffered readers. */
/* #undef NO_FEEDER */

/* Define to disable ICY handling. */
/* #undef NO_ICY */

/* Define to disable ID3v2 parsing. */
/* #undef NO_ID3V2 */

/* Define to disable layer I. */
/* #undef NO_LAYER1 */

/* Define to disable layer II. */
/* #undef NO_LAYER2 */

/* Define to disable layer III. */
/* #undef NO_LAYER3 */

/* Define to disable analyzer info. */
/* #undef NO_MOREINFO */

/* Define to disable ntom resampling. */
/* #undef NO_NTOM */

/* Define to disable real output. */
/* #undef NO_REAL */

/* Define to disable string functions. */
/* #undef NO_STRING */

/* Define for post-processed 32 bit formats. */
/* #undef NO_SYNTH32 */

/* Define to disable warning messages. */
/* #undef NO_WARNING */

/* Name of package */
#define PACKAGE "mpg123"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "maintainer@mpg123.org"

/* Define to the full name of this package. */
#define PACKAGE_NAME "mpg123"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "mpg123 1.32.2"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "mpg123"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.32.2"

/* Define to only include portable library API (no off_t, no internal I/O). */
/* #undef PORTABLE_API */

/* Define if portaudio v18 API is wanted. */
/* #undef PORTAUDIO18 */

/* Define for calculating tables at runtime. */
/* #undef RUNTIME_TABLES */

/* The size of `int32_t', as computed by sizeof. */
#define SIZEOF_INT32_T 4

/* The size of `long', as computed by sizeof. */
#define SIZEOF_LONG 4

/* The size of `off_t', as computed by sizeof. */
#define SIZEOF_OFF_T 4

/* The size of `size_t', as computed by sizeof. */
#define SIZEOF_SIZE_T 4

/* The size of `ssize_t', as computed by sizeof. */
#define SIZEOF_SSIZE_T 4

/* Define to 1 if all of the C90 standard headers exist (not just the ones
   required in a freestanding environment). This macro is provided for
   backward compatibility; new code need not use it. */
#define STDC_HEADERS 1

/* Define to not duplicate some code for likely cases in libsyn123. */
/* #undef SYN123_NO_CASES */

/* Define if modules are enabled */
#define USE_MODULES 1

/* Define for new Huffman decoding scheme. */
#define USE_NEW_HUFFTABLE 1

/* Define to use yasm for assemble AVX sources. */
/* #undef USE_YASM_FOR_AVX */

/* Version number of package */
#define VERSION "1.32.2"

/* Define to use Win32 named pipes */
#define WANT_WIN32_FIFO 1

/* Define to use Win32 sockets */
#define WANT_WIN32_SOCKETS 1

/* Define to use Unicode for Windows */
#define WANT_WIN32_UNICODE 1

/* Windows UWP build */
/* #undef WINDOWS_UWP */

/* Windows Vista and later APIs */
/* #undef WINVER */

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Define for extreme debugging. */
/* #undef XDEBUG */

/* Windows Vista and later APIs */
/* #undef _WIN32_WINNT */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to `short' if <sys/types.h> does not define. */
/* #undef int16_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef int32_t */

/* Define to `long long' if <sys/types.h> does not define. */
/* #undef int64_t */

/* Define to `long' if <sys/types.h> does not define. */
/* #undef ptrdiff_t */

/* Define to `unsigned long' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define to `long' if <sys/types.h> does not define. */
/* #undef ssize_t */

/* Define to `unsigned short' if <sys/types.h> does not define. */
/* #undef uint16_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef uint32_t */

/* Define to `unsigned long' if <sys/types.h> does not define. */
/* #undef uintptr_t */
