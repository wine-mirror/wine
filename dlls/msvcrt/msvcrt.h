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

/* Files */
#define MSVCRT_EOF   -1
#define MSVCRT_WEOF (WCHAR)(0xFFFF)

/* TLS data */
extern DWORD MSVCRT_tls_index;

typedef struct __MSVCRT_thread_data
{
  int errno;
  int doserrno;
} MSVCRT_thread_data;

#define GET_THREAD_DATA(x) \
  x = TlsGetValue(MSVCRT_tls_index)
#define GET_THREAD_VAR(x) \
  ((MSVCRT_thread_data*)TlsGetValue(MSVCRT_tls_index))->x
#define GET_THREAD_VAR_PTR(x) \
  (&((MSVCRT_thread_data*)TlsGetValue(MSVCRT_tls_index))->x)
#define SET_THREAD_VAR(x,y) \
  ((MSVCRT_thread_data*)TlsGetValue(MSVCRT_tls_index))->x = y

void  MSVCRT__set_errno(int);
int   __cdecl MSVCRT__set_new_mode(int mode);
int   __cdecl MSVCRT__fcloseall(void);
void *__cdecl MSVCRT_malloc(unsigned int);
void *__cdecl MSVCRT_calloc(unsigned int, unsigned int);
void  __cdecl MSVCRT_free(void *);
int   __cdecl MSVCRT__cputs(const char *);
int   __cdecl MSVCRT__cprintf( const char *, ... );
char *__cdecl MSVCRT__strdup(const char *);

#endif /* __WINE_MSVCRT_H */
