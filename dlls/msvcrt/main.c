/*
 * msvcrt.dll initialisation functions
 *
 * Copyright 2000 Jon Griffiths
 */
#include "msvcrt.h"

DEFAULT_DEBUG_CHANNEL(msvcrt);

/* Index to TLS */
DWORD MSVCRT_tls_index;

/* MT locks */
CRITICAL_SECTION MSVCRT_heap_cs;
CRITICAL_SECTION MSVCRT_file_cs;
CRITICAL_SECTION MSVCRT_exit_cs;
CRITICAL_SECTION MSVCRT_console_cs;
CRITICAL_SECTION MSVCRT_locale_cs;

static inline BOOL MSVCRT_init_tls(void);
static inline BOOL MSVCRT_free_tls(void);
static inline void MSVCRT_init_critical_sections(void);
static inline void MSVCRT_free_critical_sections(void);
#ifdef __GNUC__
const char *MSVCRT_get_reason(DWORD reason) __attribute__((unused));
#else
const char *MSVCRT_get_reason(DWORD reason);
#endif

void MSVCRT_init_io(void);
void MSVCRT_init_console(void);
void MSVCRT_free_console(void);
void MSVCRT_init_args(void);
void MSVCRT_free_args(void);
void MSVCRT_init_vtables(void);
char *__cdecl MSVCRT_setlocale(int category, const char *locale);


/*********************************************************************
 *                  Init
 */
BOOL WINAPI MSVCRT_Init(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  MSVCRT_thread_data *tls;

  TRACE("(0x%08x, %s, %p) pid(%ld), tid(%ld), tls(%ld)\n",
        hinstDLL, MSVCRT_get_reason(fdwReason), lpvReserved,
        (long)GetCurrentProcessId(), (long)GetCurrentThreadId(),
        (long)MSVCRT_tls_index);

  switch (fdwReason)
  {
  case DLL_PROCESS_ATTACH:
    if (!MSVCRT_init_tls())
      return FALSE;
    MSVCRT_init_vtables();
    MSVCRT_init_critical_sections();
    MSVCRT_init_io();
    MSVCRT_init_console();
    MSVCRT_init_args();
    MSVCRT_setlocale(0, "C");
    TRACE("finished process init\n");
    /* FALL THROUGH for Initial TLS allocation!! */
  case DLL_THREAD_ATTACH:
    TRACE("starting thread init\n");
    /* Create TLS */
    tls = (MSVCRT_thread_data*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                          sizeof(MSVCRT_thread_data));
    if (!tls || !TlsSetValue(MSVCRT_tls_index, tls))
    {
      ERR("TLS init failed! error = %ld\n", GetLastError());
      return FALSE;
    }
    TRACE("finished thread init\n");
    break;
  case DLL_PROCESS_DETACH:
    MSVCRT_free_critical_sections();
    MSVCRT__fcloseall();
    MSVCRT_free_console();
    MSVCRT_free_args();
    if (!MSVCRT_free_tls())
      return FALSE;
    TRACE("finished process free\n");
    break;
  case DLL_THREAD_DETACH:
    /* Free TLS */
    tls = TlsGetValue(MSVCRT_tls_index);

    if (!tls)
    {
      ERR("TLS free failed! error = %ld\n", GetLastError());
      return FALSE;
    }
    HeapFree(GetProcessHeap(), 0, tls);
    TRACE("finished thread free\n");
    break;
  }
  return TRUE;
}

static inline BOOL MSVCRT_init_tls(void)
{
  MSVCRT_tls_index = TlsAlloc();

  if (MSVCRT_tls_index == TLS_OUT_OF_INDEXES)
  {
    ERR("TlsAlloc() failed!\n");
    return FALSE;
  }
  return TRUE;
}

static inline BOOL MSVCRT_free_tls(void)
{
  if (!TlsFree(MSVCRT_tls_index))
  {
    ERR("TlsFree() failed!\n");
    return FALSE;
  }
  return TRUE;
}

static inline void MSVCRT_init_critical_sections(void)
{
  InitializeCriticalSectionAndSpinCount(&MSVCRT_heap_cs, 4000);
  InitializeCriticalSection(&MSVCRT_file_cs);
  InitializeCriticalSection(&MSVCRT_exit_cs);
  InitializeCriticalSection(&MSVCRT_console_cs);
  InitializeCriticalSection(&MSVCRT_locale_cs);
}

static inline void MSVCRT_free_critical_sections(void)
{
  DeleteCriticalSection(&MSVCRT_locale_cs);
  DeleteCriticalSection(&MSVCRT_console_cs);
  DeleteCriticalSection(&MSVCRT_exit_cs);
  DeleteCriticalSection(&MSVCRT_file_cs);
  DeleteCriticalSection(&MSVCRT_heap_cs);
}

const char *MSVCRT_get_reason(DWORD reason)
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


/*********************************************************************
 *                  Fixup functions
 *
 * Anything not really understood but needed to make the DLL work
 */
void MSVCRT_I10_OUTPUT(void)
{
  /* FIXME: This is probably data, not a function */
}

void MSVCRT___unDName(void)
{
  /* Called by all VC compiled progs on startup. No idea what it does */
}

void MSVCRT___unDNameEx(void)
{
  /* As above */
}

