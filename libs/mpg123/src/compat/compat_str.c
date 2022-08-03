/*
	compat: Some compatibility functions (basic memory and string stuff)

	The mpg123 code is determined to keep it's legacy. A legacy of old, old UNIX.
	So anything possibly somewhat advanced should be considered to be put here, with proper #ifdef;-)

	copyright 2007-2016 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis, Windows Unicode stuff by JonY.
*/

#include "compat.h"

/* Win32 is only supported with unicode now. These headers also cover
   module stuff. The WANT_WIN32_UNICODE macro is synonymous with
   "want windows-specific API, and only the unicode variants of which". */
#if defined (_WIN32) || defined (__CYGWIN__)
#include <wchar.h>
#include <windows.h>
#include <winnls.h>
#endif

#include "debug.h"

/* A safe realloc also for very old systems where realloc(NULL, size) returns NULL. */
void *safe_realloc(void *ptr, size_t size)
{
	if(ptr == NULL) return malloc(size);
	else return realloc(ptr, size);
}

// A more sensible variant of realloc: It deallocates the original memory if
// realloc fails or if size zero was requested.
void *safer_realloc(void *ptr, size_t size)
{
	void *nptr = size ? safe_realloc(ptr, size) : NULL;
	if(!nptr && ptr)
		free(ptr);
	return nptr;
}

#ifndef HAVE_STRERROR
const char *strerror(int errnum)
{
	extern int sys_nerr;
	extern char *sys_errlist[];

	return (errnum < sys_nerr) ?  sys_errlist[errnum]  :  "";
}
#endif

char* compat_strdup(const char *src)
{
	char *dest = NULL;
	if(src)
	{
		size_t len;
		len = strlen(src)+1;
		if((dest = malloc(len)))
			memcpy(dest, src, len);
	}
	return dest;
}

/* Windows Unicode stuff */
/* Provided unconditionally, since WASAPI needs it to configure the audio device */
#if defined (_WIN32) || defined (__CYGWIN__)
static
int win32_wide_common(const wchar_t * const wptr, char **mbptr, size_t * buflen, UINT cp)
{
  size_t len;
  char *buf;
  int ret = 0;

  len = WideCharToMultiByte(cp, 0, wptr, -1, NULL, 0, NULL, NULL); /* Get utf-8 string length */
  buf = calloc(len + 1, sizeof (char)); /* Can we assume sizeof char always = 1? */

  if(!buf) len = 0;
  else {
    if (len != 0) ret = WideCharToMultiByte(cp, 0, wptr, -1, buf, len, NULL, NULL); /*Do actual conversion*/
    buf[len] = '0'; /* Must terminate */
  }
  *mbptr = buf; /* Set string pointer to allocated buffer */
  if(buflen != NULL) *buflen = (len) * sizeof (char); /* Give length of allocated memory if needed. */
  return ret;
}

int win32_wide_utf8(const wchar_t * const wptr, char **mbptr, size_t * buflen)
{
  return win32_wide_common(wptr, mbptr, buflen, CP_UTF8);
}

int win32_wide_utf7(const wchar_t * const wptr, char **mbptr, size_t * buflen)
{
  return win32_wide_common(wptr, mbptr, buflen, CP_UTF7);
}

int win32_utf8_wide(const char *const mbptr, wchar_t **wptr, size_t *buflen)
{
  size_t len;
  wchar_t *buf;
  int ret = 0;

  len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, mbptr, -1, NULL, 0); /* Get converted size */
  buf = calloc(len + 1, sizeof (wchar_t)); /* Allocate memory accordingly */

  if(!buf) len = 0;
  else {
    if (len != 0) ret = MultiByteToWideChar (CP_UTF8, MB_ERR_INVALID_CHARS, mbptr, -1, buf, len); /* Do conversion */
    buf[len] = L'0'; /* Must terminate */
  }
  *wptr = buf; /* Set string pointer to allocated buffer */
  if (buflen != NULL) *buflen = len * sizeof (wchar_t); /* Give length of allocated memory if needed. */
  return ret; /* Number of characters written */
}
#endif
