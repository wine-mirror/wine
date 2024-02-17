/*
	compat: Some compatibility functions and header inclusions.
	Basic standard C stuff, that may barely be above/around C89.

	The mpg123 code is determined to keep it's legacy. A legacy of old, old UNIX.
	It is envisioned to include this compat header instead of any of the "standard" headers, to catch compatibility issues.
	So, don't include stdlib.h or string.h ... include compat.h.

	copyright 2007-23 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis
*/

#ifndef MPG123_COMPAT_H
#define MPG123_COMPAT_H

#include "config.h"

// We are using C99 now, including possibly single-precision math.
#define _ISO_C99_SOURCE

#include <errno.h>

/* realloc, size_t */
#include <stdlib.h>

#include <stddef.h>

#include        <stdio.h>
#include        <math.h>

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#else
#ifdef HAVE_SYS_SIGNAL_H
#include <sys/signal.h>
#endif
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* Types, types, types. */
/* Do we actually need these two in addition to sys/types.h? As replacement? */
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <inttypes.h>
#include <stdint.h>
/* We want SIZE_MAX, etc. */
#include <limits.h>
 
#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif
#ifndef SSIZE_MAX
#define SSIZE_MAX ((size_t)-1/2)
#endif
#ifndef PTRDIFF_MAX
#define PTRDIFF_MAX SSIZE_MAX
#endif
#ifndef ULONG_MAX
#define ULONG_MAX ((unsigned long)-1)
#endif

#ifndef INT64_MAX
#define INT64_MAX 9223372036854775807LL
#endif
#ifndef INT64_MIN
#define INT64_MIN (-INT64_MAX - 1)
#endif
#ifndef INT32_MAX
#define INT32_MAX 2147483647L
#endif
#ifndef INT32_MIN
#define INT32_MIN (-INT32_MAX - 1)
#endif

// Add two values (themselves assumed to be < limit), saturating to given limit.
#define SATURATE_ADD(inout, add, limit) inout = (limit-add >= inout) ? inout+add : limit;
#define SATURATE_SUB(inout, sub, limit) inout = (limit+sub >= inout) ? inout-sub : limit;

#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#ifdef __OS2__
#include <float.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
/* For select(), I need select.h according to POSIX 2001, else: sys/time.h sys/types.h unistd.h */
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

/* INT123_compat_open makes little sense without */
#include <fcntl.h>

/* To parse big numbers... */
#ifdef HAVE_ATOLL
#define atobigint atoll
#else
#define atobigint atol
#endif

typedef unsigned char byte;

#if defined(_MSC_VER)
// For _setmode(), at least.
#include <io.h>
#endif

/* A safe realloc also for very old systems where realloc(NULL, size) returns NULL. */
void *INT123_safe_realloc(void *ptr, size_t size);
// Also freeing ptr if result is NULL. You can do
// ptr = INT123_safer_realloc(ptr, size)
// Also, ptr = INT123_safer_realloc(ptr, 0) will do free(ptr); ptr=NULL;.
void *INT123_safer_realloc(void *ptr, size_t size);
const char *INT123_strerror(int errnum);

/* Roll our own strdup() that does not depend on libc feature test macros
   and returns NULL on NULL input instead of crashing. */
char* INT123_compat_strdup(const char *s);

/* Get an environment variable, possibly converted to UTF-8 from wide string.
   The return value is a copy that you shall free. */
char *INT123_compat_getenv(const char* name);

/**
 * Opening a file handle can be different.
 * This function here is defined to take a path in native encoding (ISO8859 / UTF-8 / ...), or, when MS Windows Unicode support is enabled, an UTF-8 string that will be converted back to native UCS-2 (wide character) before calling the system's open function.
 * @param[in] wptr Pointer to wide string.
 * @param[in] mbptr Pointer to multibyte string.
 * @return file descriptor (>=0) or error code.
 */
int INT123_compat_open(const char *filename, int flags);
FILE* INT123_compat_fopen(const char *filename, const char *mode);
/**
 * Also fdopen to avoid having to define POSIX macros in various source files.
 */
FILE* INT123_compat_fdopen(int fd, const char *mode);

/**
 * Closing a file handle can be platform specific.
 * This function takes a file descriptor that is to be closed.
 * @param[in] infd File descriptor to be closed.
 * @return 0 if the file was successfully closed. A return value of -1 indicates an error.
 */
int INT123_compat_close(int infd);
int INT123_compat_fclose(FILE* stream);

/**
 * Setting binary mode on a descriptor, where necessary.
 * We do not bother with errors. This has to work.
 * You can enable or disable binary mode.
 */
void INT123_compat_binmode(int fd, int enable);

/* Those do make sense in a separate file, but I chose to include them in compat.c because that's the one source whose object is shared between mpg123 and libmpg123 -- and both need the functionality internally. */

#if defined (_WIN32) || defined (__CYGWIN__)
/**
 * win32_uni2mbc
 * Converts a null terminated UCS-2 string to a multibyte (UTF-8) equivalent.
 * Caller is supposed to free allocated buffer.
 * @param[in] wptr Pointer to wide string.
 * @param[out] mbptr Pointer to multibyte string.
 * @param[out] buflen Optional parameter for length of allocated buffer.
 * @return status of WideCharToMultiByte conversion.
 *
 * WideCharToMultiByte - http://msdn.microsoft.com/en-us/library/dd374130(VS.85).aspx
 */
int INT123_win32_wide_utf8(const wchar_t * const wptr, char **mbptr, size_t * buflen);

/**
 * win32_uni2mbc
 * Converts a null terminated UCS-2 string to a multibyte (UTF-7) equivalent.
 * Caller is supposed to free allocated buffer.
 * @param[in] wptr Pointer to wide string.
 * @param[out] mbptr Pointer to multibyte string.
 * @param[out] buflen Optional parameter for length of allocated buffer.
 * @return status of WideCharToMultiByte conversion.
 *
 * WideCharToMultiByte - http://msdn.microsoft.com/en-us/library/dd374130(VS.85).aspx
 */
int INT123_win32_wide_utf7(const wchar_t * const wptr, char **mbptr, size_t * buflen);

/**
 * win32_mbc2uni
 * Converts a null terminated UTF-8 string to a UCS-2 equivalent.
 * Caller is supposed to free allocated buffer.
 * @param[in] mbptr Pointer to multibyte string.
 * @param[out] wptr Pointer to wide string.
 * @param[out] buflen Optional parameter for length of allocated buffer.
 * @return status of WideCharToMultiByte conversion.
 *
 * MultiByteToWideChar - http://msdn.microsoft.com/en-us/library/dd319072(VS.85).aspx
 */

int INT123_win32_utf8_wide(const char *const mbptr, wchar_t **wptr, size_t *buflen);
#endif

/*
	A little bit of path abstraction: We always work with plain char strings
	that usually represent POSIX-ish UTF-8 paths (something like c:/some/file
	might appear). For Windows, those are converted to wide strings with \
	instead of / and possible fun is had with prefixes to get around the old
	path length limit. Outside of the compat library, that stuff should not
	matter, although something like //?/UNC/server/some/file could be thrown
	around as UTF-8 string, to be converted to a wide \\?\UNC\server\some\file
	just before handing it to Windows API.

	There is a lot of unnecessary memory allocation and string copying because
	of this, but this filesystem stuff is not really relevant to mpg123
	performance, so the goal is to keep the code outside the compatibility layer
	simple.
*/

/*
	Concatenate a prefix and a path, one of them alowed to be NULL.
	If the path is already absolute, the prefix is ignored. Relative
	parts (like /..) are resolved if this is sensible for the platform
	(meaning: for Windows), else they are preserved (on POSIX, actual
	file system access would be needed because of symlinks).
*/
char* INT123_compat_catpath(const char *prefix, const char* path);

/* Return 1 if the given path indicates an existing directory,
   0 otherwise. */
int INT123_compat_isdir(const char *path);

/*
	Directory traversal. This talks ASCII/UTF-8 paths externally, converts
	to/from wchar_t internally if the platform wants that. Returning NULL
	means failure to open/end of listing.
	There is no promise about sorting entries.
*/
struct compat_dir;
/* Returns NULL if either directory failed to open or listing is empty.
   Listing can still be empty even if non-NULL, so always rely on the
   nextfile/nextdir functions. */
struct compat_dir* INT123_compat_diropen(char *path);
void               INT123_compat_dirclose(struct compat_dir*);
/* Get the next entry that is a file (or symlink to one).
   The returned string is a copy that needs to be freed after use. */
char* INT123_compat_nextfile(struct compat_dir*);
/* Get the next entry that is a directory (or symlink to one).
   The returned string is a copy that needs to be freed after use. */
char* INT123_compat_nextdir (struct compat_dir*);

#ifdef USE_MODULES
/*
	For keeping the path mess local, a system-specific dlopen() variant
	is contained in here, too. This is very thin wrapping, even sparing
	definition of a handle type, just using void pointers.
	Use of absolute paths is a good idea if you want to be sure which
	file is openend, as default search paths vary.
*/
void *INT123_compat_dlopen (const char *path);
void *INT123_compat_dlsym  (void *handle, const char* name);
void  INT123_compat_dlclose(void *handle);
#endif

/* Blocking write/read of data with signal resilience.
   They continue after being interrupted by signals and always return the
   amount of processed data (shortage indicating actual problem or EOF). */
size_t INT123_unintr_write(int fd, void const *buffer, size_t bytes);
size_t INT123_unintr_read (int fd, void *buffer, size_t bytes);
size_t INT123_unintr_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

/* OSX SDK defines an enum with "normal" as value. That clashes with
   optimize.h */
#ifdef __APPLE__
#define normal mpg123_normal
#endif

#include "../common/true.h"

#if (!defined(WIN32) || defined (__CYGWIN__)) && defined(HAVE_SIGNAL_H)
void (*INT123_catchsignal(int signum, void(*handler)(int)))(int);
#endif

// Some ancient toolchains miss the documented errno value.
#if defined(_WIN32) && !defined(EOVERFLOW)
#define EOVERFLOW 132
#endif

#endif
