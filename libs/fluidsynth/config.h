#ifndef CONFIG_H
#define CONFIG_H

/* Define to enable ALSA driver */
/* #undef ALSA_SUPPORT TRUE */

/* Define to activate sound output to files */
/* #undef AUFILE_SUPPORT 1 */

/* whether or not we are supporting CoreAudio */
/* #undef COREAUDIO_SUPPORT */

/* whether or not we are supporting CoreMIDI */
/* #undef COREMIDI_SUPPORT */

/* whether or not we are supporting DART */
/* #undef DART_SUPPORT */

/* Define if building for Mac OS X Darwin */
/* #undef DARWIN */

/* Define if D-Bus support is enabled */
/* #undef DBUS_SUPPORT  1 */

/* Soundfont to load automatically in some use cases */
/* #undef DEFAULT_SOUNDFONT "/usr/local/share/soundfonts/default.sf2" */

/* Define to enable FPE checks */
/* #undef FPE_CHECK */

/* Define to 1 if you have the <arpa/inet.h> header file. */
/* #undef HAVE_ARPA_INET_H */

/* Define to 1 if you have the <errno.h> header file. */
#define HAVE_ERRNO_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
/* #undef HAVE_FCNTL_H */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <io.h> header file. */
#define HAVE_IO_H 1

/* whether or not we are supporting lash */
/* #undef HAVE_LASH */

/* Define if systemd support is enabled */
/* #undef SYSTEMD_SUPPORT */

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the <linux/soundcard.h> header file. */
/* #undef HAVE_LINUX_SOUNDCARD_H */

/* Define to 1 if you have the <machine/soundcard.h> header file. */
/* #undef HAVE_MACHINE_SOUNDCARD_H */

/* Define to 1 if you have the <math.h> header file. */
#define HAVE_MATH_H 1

/* Define to 1 if you have the <netinet/in.h> header file. */
/* #undef HAVE_NETINET_IN_H */

/* Define to 1 if you have the <netinet/tcp.h> header file. */
/* #undef HAVE_NETINET_TCP_H */

/* Define if compiling the mixer with multi-thread support */
#define ENABLE_MIXER_THREADS 1

/* Define if compiling with openMP to enable parallel audio rendering */
/* #undef HAVE_OPENMP */

/* Define to 1 if you have the <pthread.h> header file. */
/* #undef HAVE_PTHREAD_H */

/* Define to 1 if you have the <signal.h> header file. */
#define HAVE_SIGNAL_H 1

/* Define to 1 if you have the <stdarg.h> header file. */
#define HAVE_STDARG_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
/* #undef HAVE_STRINGS_H */

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/mman.h> header file. */
/* #undef HAVE_SYS_MMAN_H */

/* Define to 1 if you have the <sys/socket.h> header file. */
/* #undef HAVE_SYS_SOCKET_H */

/* Define to 1 if you have the <sys/soundcard.h> header file. */
/* #undef HAVE_SYS_SOUNDCARD_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
/* #undef HAVE_SYS_TIME_H */

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
/* #undef HAVE_UNISTD_H */

/* Define to 1 if you have the <windows.h> header file. */
/* #undef HAVE_WINDOWS_H */

/* Define to 1 if you have the <getopt.h> header file. */
/* #undef HAVE_GETOPT_H */

/* Define to 1 if you have the inet_ntop() function. */
/* #undef HAVE_INETNTOP */

/* Define to enable JACK driver */
/* #undef JACK_SUPPORT */

/* Define to enable PipeWire driver */
/* #undef PIPEWIRE_SUPPORT */

/* Include the LADSPA Fx unit */
/* #undef LADSPA */

/* Define to enable IPV6 support */
/* #undef IPV6_SUPPORT */

/* Define to enable network support */
/* #undef NETWORK_SUPPORT */

/* Defined when fluidsynth is build in an automated environment, where no MSVC++ Runtime Debug Assertion dialogs should pop up */
#define NO_GUI 1

/* libinstpatch for DLS and GIG */
/* #undef LIBINSTPATCH_SUPPORT */

/* libsndfile has ogg vorbis support */
/* #undef LIBSNDFILE_HASVORBIS */

/* Define to enable libsndfile support */
/* #undef LIBSNDFILE_SUPPORT */

/* Define to enable MidiShare driver */
/* #undef MIDISHARE_SUPPORT */

/* Define if using the MinGW32 environment */
#define MINGW32 1

/* Define to enable OSS driver */
/* #undef OSS_SUPPORT */

/* Define to enable OPENSLES driver */
/* #undef OPENSLES_SUPPORT */

/* Define to enable Oboe driver */
/* #undef OBOE_SUPPORT */

/* Name of package */
#define PACKAGE "fluidsynth"

/* Define to the address where bug reports for this package should be sent. */
/* #undef PACKAGE_BUGREPORT */

/* Define to the full name of this package. */
/* #undef PACKAGE_NAME */

/* Define to the full name and version of this package. */
/* #undef PACKAGE_STRING */

/* Define to the one symbol short name of this package. */
/* #undef PACKAGE_TARNAME */

/* Define to the version of this package. */
/* #undef PACKAGE_VERSION */

/* Define to enable PortAudio driver */
/* #undef PORTAUDIO_SUPPORT */

/* Define to enable PulseAudio driver */
/* #undef PULSE_SUPPORT */

/* Define to enable DirectSound driver */
/* #undef DSOUND_SUPPORT 1 */

/* Define to enable Windows WASAPI driver */
/* #undef WASAPI_SUPPORT 1 */

/* Define to enable Windows WaveOut driver */
/* #undef WAVEOUT_SUPPORT 1 */

/* Define to enable Windows MIDI driver */
/* #undef WINMIDI_SUPPORT */

/* Define to enable SDL2 audio driver */
/* #undef SDL2_SUPPORT */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Soundfont to load for unit testing */
/* #undef TEST_SOUNDFONT */

/* Soundfont to load for UTF-8 unit testing */
/* #undef TEST_SOUNDFONT_UTF8_1 */
/* #undef TEST_SOUNDFONT_UTF8_2 */
/* #undef TEST_MIDI_UTF8 */

/* SF3 Soundfont to load for unit testing */
/* #undef TEST_SOUNDFONT_SF3 */

/* Define to enable SIGFPE assertions */
/* #undef TRAP_ON_FPE */

/* Define to do all DSP in single floating point precision */
/* #undef WITH_FLOAT */

/* Define to profile the DSP code */
/* #undef WITH_PROFILING */

/* Define to use the readline library for line editing */
/* #undef READLINE_SUPPORT */

/* Define if the compiler supports VLA */
#define SUPPORTS_VLA 1

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
/* #undef WORDS_BIGENDIAN */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to 1 if you have the sinf() function. */
#define HAVE_SINF 1

/* Define to 1 if you have the cosf() function. */
#define HAVE_COSF 1

/* Define to 1 if you have the fabsf() function. */
#define HAVE_FABSF 1

/* Define to 1 if you have the powf() function. */
#define HAVE_POWF 1

/* Define to 1 if you have the sqrtf() function. */
#define HAVE_SQRTF 1

/* Define to 1 if you have the logf() function. */
#define HAVE_LOGF 1

/* Define to 1 if you have the socklen_t type. */
/* #undef HAVE_SOCKLEN_T */

#endif /* CONFIG_H */
