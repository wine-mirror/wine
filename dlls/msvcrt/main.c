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
#include "winternl.h"

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

static inline void msvcrt_free_tls_mem(void)
{
  thread_data_t *tls = TlsGetValue(msvcrt_tls_index);

  if (tls)
  {
    CloseHandle(tls->handle);
    HeapFree(GetProcessHeap(),0,tls->efcvt_buffer);
    HeapFree(GetProcessHeap(),0,tls->asctime_buffer);
    HeapFree(GetProcessHeap(),0,tls->wasctime_buffer);
    HeapFree(GetProcessHeap(),0,tls->strerror_buffer);
    HeapFree(GetProcessHeap(),0,tls->wcserror_buffer);
    HeapFree(GetProcessHeap(),0,tls->time_buffer);
    HeapFree(GetProcessHeap(),0,tls->tmpnam_buffer);
    HeapFree(GetProcessHeap(),0,tls->wtmpnam_buffer);
    if(tls->have_locale) {
        free_locinfo(tls->locinfo);
        free_mbcinfo(tls->mbcinfo);
    }
  }
  HeapFree(GetProcessHeap(), 0, tls);
}

/*********************************************************************
 *                  Init
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  TRACE("(%p, %s, %p) pid(%x), tid(%x), tls(%u)\n",
        hinstDLL, msvcrt_get_reason(fdwReason), lpvReserved,
        GetCurrentProcessId(), GetCurrentThreadId(),
        msvcrt_tls_index);

  switch (fdwReason)
  {
  case DLL_PROCESS_ATTACH:
    msvcrt_init_exception(hinstDLL);
    if(!msvcrt_init_heap())
        return FALSE;
    if(!msvcrt_init_tls()) {
      msvcrt_destroy_heap();
      return FALSE;
    }
    msvcrt_init_mt_locks();
    if(!msvcrt_init_locale()) {
        msvcrt_free_mt_locks();
        msvcrt_free_tls_mem();
        msvcrt_destroy_heap();
        return FALSE;
    }
    msvcrt_init_math();
    msvcrt_init_io();
    msvcrt_init_console();
    msvcrt_init_args();
    msvcrt_init_signals();
#if _MSVCR_VER == 0
    /* don't allow unloading msvcrt, we can't setup file handles twice */
    LdrAddRefDll( LDR_ADDREF_DLL_PIN, hinstDLL );
#elif _MSVCR_VER >= 80
    MSVCRT__set_printf_count_output(0);
#endif
    TRACE("finished process init\n");
    break;
  case DLL_THREAD_ATTACH:
    break;
  case DLL_PROCESS_DETACH:
    msvcrt_free_io();
    if (lpvReserved) break;
    msvcrt_free_popen_data();
    msvcrt_free_mt_locks();
    msvcrt_free_console();
    msvcrt_free_args();
    msvcrt_free_signals();
    msvcrt_free_tls_mem();
    if (!msvcrt_free_tls())
      return FALSE;
    MSVCRT__free_locale(MSVCRT_locale);
    msvcrt_destroy_heap();
    TRACE("finished process free\n");
    break;
  case DLL_THREAD_DETACH:
    msvcrt_free_tls_mem();
    TRACE("finished thread free\n");
    break;
  }
  return TRUE;
}
