#ifndef __WINE_MSVCRT_H
#define __WINE_MSVCRT_H

#include "config.h"
#include "windef.h"
#include "debugtools.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

/* TLS data */
extern DWORD MSVCRT_tls_index;

typedef struct __MSVCRT_thread_data
{
  int errno;
  unsigned long doserrno;
} MSVCRT_thread_data;

#define GET_THREAD_DATA(x) \
  x = TlsGetValue(MSVCRT_tls_index)
#define GET_THREAD_VAR(x) \
  ((MSVCRT_thread_data*)TlsGetValue(MSVCRT_tls_index))->x
#define GET_THREAD_VAR_PTR(x) \
  (&((MSVCRT_thread_data*)TlsGetValue(MSVCRT_tls_index))->x)
#define SET_THREAD_VAR(x,y) \
  ((MSVCRT_thread_data*)TlsGetValue(MSVCRT_tls_index))->x = y

void   MSVCRT__set_errno(int);
char*  msvcrt_strndup(const char*,unsigned int);
LPWSTR msvcrt_wstrndup(LPCWSTR, unsigned int);

/* FIXME: This should be declared in new.h but it's not an extern "C" so 
 * it would not be much use anyway. Even for Winelib applications.
 */
int    MSVCRT__set_new_mode(int mode);

#endif /* __WINE_MSVCRT_H */
