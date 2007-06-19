/*
 * msvcrt.dll initialisation functions
 *
 * Copyright 2000 Jon Griffiths
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include "msvcrt.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* Index to TLS */
DWORD msvcrt_tls_index;

static const char* msvcrt_get_reason(DWORD reason)
{
  switch (reason)
  {
  case DLL_PROCESS_ATTACH: return "DLL_PROCESS_ATTACH";
  case DLL_PROCESS_DETACH: return "DLL_PROCESS_DETACH";
  case DLL_THREAD_ATTACH:  return "DLL_THREAD_ATTACH";
  case DLL_THREAD_DETACH:  return "DLL_THREAD_DETACH";
  }
  return "UNKNOWN";
}

static inline BOOL msvcrt_init_tls(void)
{
  msvcrt_tls_index = TlsAlloc();

  if (msvcrt_tls_index == TLS_OUT_OF_INDEXES)
  {
    ERR("TlsAlloc() failed!\n");
    return FALSE;
  }
  return TRUE;
}

static inline BOOL msvcrt_free_tls(void)
{
  if (!TlsFree(msvcrt_tls_index))
  {
    ERR("TlsFree() failed!\n");
    return FALSE;
  }
  return TRUE;
}

/*********************************************************************
 *                  Init
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  thread_data_t *tls;

  TRACE("(%p, %s, %p) pid(%x), tid(%x), tls(%ld)\n",
        hinstDLL, msvcrt_get_reason(fdwReason), lpvReserved,
        GetCurrentProcessId(), GetCurrentThreadId(),
        (long)msvcrt_tls_index);

  switch (fdwReason)
  {
  case DLL_PROCESS_ATTACH:
    if (!msvcrt_init_tls())
      return FALSE;
    msvcrt_init_mt_locks();
    msvcrt_init_io();
    msvcrt_init_console();
    msvcrt_init_args();
    msvcrt_init_signals();
    MSVCRT_setlocale(0, "C");
    TRACE("finished process init\n");
    break;
  case DLL_THREAD_ATTACH:
    break;
  case DLL_PROCESS_DETACH:
    msvcrt_free_mt_locks();
    msvcrt_free_io();
    msvcrt_free_console();
    msvcrt_free_args();
    msvcrt_free_signals();
    if (!msvcrt_free_tls())
      return FALSE;
    TRACE("finished process free\n");
    break;
  case DLL_THREAD_DETACH:
    /* Free TLS */
    tls = TlsGetValue(msvcrt_tls_index);
    if (tls)
    {
	HeapFree(GetProcessHeap(),0,tls->efcvt_buffer);
	HeapFree(GetProcessHeap(),0,tls->asctime_buffer);
	HeapFree(GetProcessHeap(),0,tls->wasctime_buffer);
	HeapFree(GetProcessHeap(),0,tls->strerror_buffer);
    }
    HeapFree(GetProcessHeap(), 0, tls);
    TRACE("finished thread free\n");
    break;
  }
  return TRUE;
}

/*********************************************************************
 *		$I10_OUTPUT (MSVCRT.@)
 * Function not really understood but needed to make the DLL work
 */
void CDECL MSVCRT_I10_OUTPUT(void)
{
  /* FIXME: This is probably data, not a function */
  /* no it is a function. I10 is an Int of 10 bytes */
  /* also known as 80 bit flotaing point (long double */
  /* for some compilers, not MSVC) */
}

/*********************************************************************
 *		_adjust_fdiv (MSVCRT.@)
 * Used by the MSVC compiler to work around the Pentium FDIV bug.
 */
int MSVCRT__adjust_fdiv = 0;
