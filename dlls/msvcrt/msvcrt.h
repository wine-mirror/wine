/*
 * Copyright 2001 Jon Griffiths
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_MSVCRT_H
#define __WINE_MSVCRT_H

#include <stdarg.h>
#include <ctype.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"

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

extern int MSVCRT_current_lc_all_cp;

void _purecall(void);
void   MSVCRT__set_errno(int);
char*  msvcrt_strndup(const char*,unsigned int);
LPWSTR msvcrt_wstrndup(LPCWSTR, unsigned int);

/* FIXME: This should be declared in new.h but it's not an extern "C" so
 * it would not be much use anyway. Even for Winelib applications.
 */
int    MSVCRT__set_new_mode(int mode);

/* Setup and teardown multi threaded locks */
extern void msvcrt_init_mt_locks(void);
extern void msvcrt_free_mt_locks(void);

extern void msvcrt_init_io(void);
extern void msvcrt_free_io(void);
extern void msvcrt_init_console(void);
extern void msvcrt_free_console(void);
extern void msvcrt_init_args(void);
extern void msvcrt_free_args(void);
extern void msvcrt_init_vtables(void);

#endif /* __WINE_MSVCRT_H */
