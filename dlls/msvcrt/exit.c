/*
 * msvcrt.dll exit functions
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
#include <stdio.h>
#include "msvcrt.h"
#include "mtdll.h"
#include "winuser.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* MT */
#define LOCK_EXIT   _mlock(_EXIT_LOCK1)
#define UNLOCK_EXIT _munlock(_EXIT_LOCK1)

static MSVCRT_purecall_handler purecall_handler = NULL;

static MSVCRT__onexit_table_t MSVCRT_atexit_table;

typedef void (__stdcall *_tls_callback_type)(void*,ULONG,void*);
static _tls_callback_type tls_atexit_callback;

static CRITICAL_SECTION MSVCRT_onexit_cs;
static CRITICAL_SECTION_DEBUG MSVCRT_onexit_cs_debug =
{
    0, 0, &MSVCRT_onexit_cs,
    { &MSVCRT_onexit_cs_debug.ProcessLocksList, &MSVCRT_onexit_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": MSVCRT_onexit_cs") }
};
static CRITICAL_SECTION MSVCRT_onexit_cs = { &MSVCRT_onexit_cs_debug, -1, 0, 0, 0, 0 };

extern int MSVCRT_app_type;
extern MSVCRT_wchar_t *MSVCRT__wpgmptr;

static unsigned int MSVCRT_abort_behavior =  MSVCRT__WRITE_ABORT_MSG | MSVCRT__CALL_REPORTFAULT;
static int MSVCRT_error_mode = MSVCRT__OUT_TO_DEFAULT;

void (*CDECL _aexit_rtn)(int) = MSVCRT__exit;

static int initialize_onexit_table(MSVCRT__onexit_table_t *table)
{
    if (!table)
        return -1;

    if (table->_first == table->_end)
        table->_last = table->_end = table->_first = NULL;
    return 0;
}

static int register_onexit_function(MSVCRT__onexit_table_t *table, MSVCRT__onexit_t func)
{
    if (!table)
        return -1;

    EnterCriticalSection(&MSVCRT_onexit_cs);
    if (!table->_first)
    {
        table->_first = MSVCRT_calloc(32, sizeof(void *));
        if (!table->_first)
        {
            WARN("failed to allocate initial table.\n");
            LeaveCriticalSection(&MSVCRT_onexit_cs);
            return -1;
        }
        table->_last = table->_first;
        table->_end = table->_first + 32;
    }

    /* grow if full */
    if (table->_last == table->_end)
    {
        int len = table->_end - table->_first;
        MSVCRT__onexit_t *tmp = MSVCRT_realloc(table->_first, 2 * len * sizeof(void *));
        if (!tmp)
        {
            WARN("failed to grow table.\n");
            LeaveCriticalSection(&MSVCRT_onexit_cs);
            return -1;
        }
        table->_first = tmp;
        table->_end = table->_first + 2 * len;
        table->_last = table->_first + len;
    }

    *table->_last = func;
    table->_last++;
    LeaveCriticalSection(&MSVCRT_onexit_cs);
    return 0;
}

static int execute_onexit_table(MSVCRT__onexit_table_t *table)
{
    MSVCRT__onexit_t *func;
    MSVCRT__onexit_table_t copy;

    if (!table)
        return -1;

    EnterCriticalSection(&MSVCRT_onexit_cs);
    if (!table->_first || table->_first >= table->_last)
    {
        LeaveCriticalSection(&MSVCRT_onexit_cs);
        return 0;
    }
    copy._first = table->_first;
    copy._last  = table->_last;
    copy._end   = table->_end;
    memset(table, 0, sizeof(*table));
    initialize_onexit_table(table);
    LeaveCriticalSection(&MSVCRT_onexit_cs);

    for (func = copy._last - 1; func >= copy._first; func--)
    {
        if (*func)
           (*func)();
    }

    MSVCRT_free(copy._first);
    return 0;
}

static void call_atexit(void)
{
    /* Note: should only be called with the exit lock held */
    if (tls_atexit_callback) tls_atexit_callback(NULL, DLL_PROCESS_DETACH, NULL);
    execute_onexit_table(&MSVCRT_atexit_table);
}

/*********************************************************************
 *		__dllonexit (MSVCRT.@)
 */
MSVCRT__onexit_t CDECL __dllonexit(MSVCRT__onexit_t func, MSVCRT__onexit_t **start, MSVCRT__onexit_t **end)
{
  MSVCRT__onexit_t *tmp;
  int len;

  TRACE("(%p,%p,%p)\n", func, start, end);

  if (!start || !*start || !end || !*end)
  {
   FIXME("bad table\n");
   return NULL;
  }

  len = (*end - *start);

  TRACE("table start %p-%p, %d entries\n", *start, *end, len);

  if (++len <= 0)
    return NULL;

  tmp = MSVCRT_realloc(*start, len * sizeof(*tmp));
  if (!tmp)
    return NULL;
  *start = tmp;
  *end = tmp + len;
  tmp[len - 1] = func;
  TRACE("new table start %p-%p, %d entries\n", *start, *end, len);
  return func;
}

/*********************************************************************
 *		_exit (MSVCRT.@)
 */
void CDECL MSVCRT__exit(int exitcode)
{
  TRACE("(%d)\n", exitcode);
  ExitProcess(exitcode);
}

/* Print out an error message with an option to debug */
static void DoMessageBoxW(const MSVCRT_wchar_t *lead, const MSVCRT_wchar_t *message)
{
  static const MSVCRT_wchar_t message_format[] = {'%','l','s','\n','\n','P','r','o','g','r','a','m',':',' ','%','l','s','\n',
    '%','l','s','\n','\n','P','r','e','s','s',' ','O','K',' ','t','o',' ','e','x','i','t',' ','t','h','e',' ',
    'p','r','o','g','r','a','m',',',' ','o','r',' ','C','a','n','c','e','l',' ','t','o',' ','s','t','a','r','t',' ',
    't','h','e',' ','W','i','n','e',' ','d','e','b','u','g','g','e','r','.','\n',0};
  static const WCHAR title[] =
    {'W','i','n','e',' ','C','+','+',' ','R','u','n','t','i','m','e',' ','L','i','b','r','a','r','y',0};

  MSGBOXPARAMSW msgbox;
  MSVCRT_wchar_t text[2048];
  INT ret;

  MSVCRT__snwprintf(text, ARRAY_SIZE(text), message_format, lead, MSVCRT__wpgmptr, message);

  msgbox.cbSize = sizeof(msgbox);
  msgbox.hwndOwner = GetActiveWindow();
  msgbox.hInstance = 0;
  msgbox.lpszText = text;
  msgbox.lpszCaption = title;
  msgbox.dwStyle = MB_OKCANCEL|MB_ICONERROR;
  msgbox.lpszIcon = NULL;
  msgbox.dwContextHelpId = 0;
  msgbox.lpfnMsgBoxCallback = NULL;
  msgbox.dwLanguageId = LANG_NEUTRAL;

  ret = MessageBoxIndirectW(&msgbox);
  if (ret == IDCANCEL)
    DebugBreak();
}

static void DoMessageBox(const char *lead, const char *message)
{
  MSVCRT_wchar_t leadW[1024], messageW[1024];

  MSVCRT_mbstowcs(leadW, lead, 1024);
  MSVCRT_mbstowcs(messageW, message, 1024);

  DoMessageBoxW(leadW, messageW);
}

/*********************************************************************
 *		_amsg_exit (MSVCRT.@)
 */
void CDECL _amsg_exit(int errnum)
{
  TRACE("(%d)\n", errnum);

  if ((MSVCRT_error_mode == MSVCRT__OUT_TO_MSGBOX) ||
     ((MSVCRT_error_mode == MSVCRT__OUT_TO_DEFAULT) && (MSVCRT_app_type == 2)))
  {
    char text[32];
    MSVCRT_sprintf(text, "Error: R60%d",errnum);
    DoMessageBox("Runtime error!", text);
  }
  else
    _cprintf("\nruntime error R60%d\n",errnum);
  _aexit_rtn(255);
}

/*********************************************************************
 *		abort (MSVCRT.@)
 */
void CDECL MSVCRT_abort(void)
{
  TRACE("()\n");

  if (MSVCRT_abort_behavior & MSVCRT__WRITE_ABORT_MSG)
  {
    if ((MSVCRT_error_mode == MSVCRT__OUT_TO_MSGBOX) ||
       ((MSVCRT_error_mode == MSVCRT__OUT_TO_DEFAULT) && (MSVCRT_app_type == 2)))
    {
      DoMessageBox("Runtime error!", "abnormal program termination");
    }
    else
      _cputs("\nabnormal program termination\n");
  }
  MSVCRT_raise(MSVCRT_SIGABRT);
  /* in case raise() returns */
  MSVCRT__exit(3);
}

#if _MSVCR_VER>=80
/*********************************************************************
 *		_set_abort_behavior (MSVCR80.@)
 */
unsigned int CDECL MSVCRT__set_abort_behavior(unsigned int flags, unsigned int mask)
{
  unsigned int old = MSVCRT_abort_behavior;

  TRACE("%x, %x\n", flags, mask);
  if (mask & MSVCRT__CALL_REPORTFAULT)
    FIXME("_WRITE_CALL_REPORTFAULT unhandled\n");

  MSVCRT_abort_behavior = (MSVCRT_abort_behavior & ~mask) | (flags & mask);
  return old;
}
#endif

/*********************************************************************
 *              _wassert (MSVCRT.@)
 */
void CDECL MSVCRT__wassert(const MSVCRT_wchar_t* str, const MSVCRT_wchar_t* file, unsigned int line)
{
  static const MSVCRT_wchar_t assertion_failed[] = {'A','s','s','e','r','t','i','o','n',' ','f','a','i','l','e','d','!',0};
  static const MSVCRT_wchar_t format_msgbox[] = {'F','i','l','e',':',' ','%','l','s','\n','L','i','n','e',':',' ','%','d',
      '\n','\n','E','x','p','r','e','s','s','i','o','n',':',' ','\"','%','l','s','\"',0};
  static const MSVCRT_wchar_t format_console[] = {'A','s','s','e','r','t','i','o','n',' ','f','a','i','l','e','d',':',' ',
      '%','l','s',',',' ','f','i','l','e',' ','%','l','s',',',' ','l','i','n','e',' ','%','d','\n','\n',0};

  TRACE("(%s,%s,%d)\n", debugstr_w(str), debugstr_w(file), line);

  if ((MSVCRT_error_mode == MSVCRT__OUT_TO_MSGBOX) ||
     ((MSVCRT_error_mode == MSVCRT__OUT_TO_DEFAULT) && (MSVCRT_app_type == 2)))
  {
    MSVCRT_wchar_t text[2048];
    MSVCRT__snwprintf(text, sizeof(text), format_msgbox, file, line, str);
    DoMessageBoxW(assertion_failed, text);
  }
  else
    MSVCRT_fwprintf(MSVCRT_stderr, format_console, str, file, line);

  MSVCRT_raise(MSVCRT_SIGABRT);
  MSVCRT__exit(3);
}

/*********************************************************************
 *		_assert (MSVCRT.@)
 */
void CDECL MSVCRT__assert(const char* str, const char* file, unsigned int line)
{
    MSVCRT_wchar_t strW[1024], fileW[1024];

    MSVCRT_mbstowcs(strW, str, 1024);
    MSVCRT_mbstowcs(fileW, file, 1024);

    MSVCRT__wassert(strW, fileW, line);
}

/*********************************************************************
 *		_c_exit (MSVCRT.@)
 */
void CDECL MSVCRT__c_exit(void)
{
  TRACE("(void)\n");
  /* All cleanup is done on DLL detach; Return to caller */
}

/*********************************************************************
 *		_cexit (MSVCRT.@)
 */
void CDECL MSVCRT__cexit(void)
{
  TRACE("(void)\n");
  LOCK_EXIT;
  call_atexit();
  UNLOCK_EXIT;
}

/*********************************************************************
 *		_onexit (MSVCRT.@)
 */
MSVCRT__onexit_t CDECL MSVCRT__onexit(MSVCRT__onexit_t func)
{
  TRACE("(%p)\n",func);

  if (!func)
    return NULL;

  LOCK_EXIT;
  register_onexit_function(&MSVCRT_atexit_table, func);
  UNLOCK_EXIT;

  return func;
}

/*********************************************************************
 *		exit (MSVCRT.@)
 */
void CDECL MSVCRT_exit(int exitcode)
{
  HMODULE hmscoree;
  static const WCHAR mscoreeW[] = {'m','s','c','o','r','e','e',0};
  void (WINAPI *pCorExitProcess)(int);

  TRACE("(%d)\n",exitcode);
  MSVCRT__cexit();

  hmscoree = GetModuleHandleW(mscoreeW);

  if (hmscoree)
  {
    pCorExitProcess = (void*)GetProcAddress(hmscoree, "CorExitProcess");

    if (pCorExitProcess)
      pCorExitProcess(exitcode);
  }

  ExitProcess(exitcode);
}

/*********************************************************************
 *		atexit (MSVCRT.@)
 */
int CDECL MSVCRT_atexit(void (__cdecl *func)(void))
{
  TRACE("(%p)\n", func);
  return MSVCRT__onexit((MSVCRT__onexit_t)func) == (MSVCRT__onexit_t)func ? 0 : -1;
}

#if _MSVCR_VER >= 140
static MSVCRT__onexit_table_t MSVCRT_quick_exit_table;

/*********************************************************************
 *             _crt_at_quick_exit (UCRTBASE.@)
 */
int CDECL MSVCRT__crt_at_quick_exit(void (__cdecl *func)(void))
{
  TRACE("(%p)\n", func);
  return register_onexit_function(&MSVCRT_quick_exit_table, (MSVCRT__onexit_t)func);
}

/*********************************************************************
 *             quick_exit (UCRTBASE.@)
 */
void CDECL MSVCRT_quick_exit(int exitcode)
{
  TRACE("(%d)\n", exitcode);

  execute_onexit_table(&MSVCRT_quick_exit_table);
  MSVCRT__exit(exitcode);
}

/*********************************************************************
 *		_crt_atexit (UCRTBASE.@)
 */
int CDECL MSVCRT__crt_atexit(void (__cdecl *func)(void))
{
  TRACE("(%p)\n", func);
  return MSVCRT__onexit((MSVCRT__onexit_t)func) == (MSVCRT__onexit_t)func ? 0 : -1;
}

/*********************************************************************
 *		_initialize_onexit_table (UCRTBASE.@)
 */
int CDECL MSVCRT__initialize_onexit_table(MSVCRT__onexit_table_t *table)
{
    TRACE("(%p)\n", table);

    return initialize_onexit_table(table);
}

/*********************************************************************
 *		_register_onexit_function (UCRTBASE.@)
 */
int CDECL MSVCRT__register_onexit_function(MSVCRT__onexit_table_t *table, MSVCRT__onexit_t func)
{
    TRACE("(%p %p)\n", table, func);

    return register_onexit_function(table, func);
}

/*********************************************************************
 *		_execute_onexit_table (UCRTBASE.@)
 */
int CDECL MSVCRT__execute_onexit_table(MSVCRT__onexit_table_t *table)
{
    TRACE("(%p)\n", table);

    return execute_onexit_table(table);
}
#endif

/*********************************************************************
 *		_register_thread_local_exe_atexit_callback (UCRTBASE.@)
 */
void CDECL _register_thread_local_exe_atexit_callback(_tls_callback_type callback)
{
    TRACE("(%p)\n", callback);
    tls_atexit_callback = callback;
}

#if _MSVCR_VER>=71
/*********************************************************************
 *		_set_purecall_handler (MSVCR71.@)
 */
MSVCRT_purecall_handler CDECL _set_purecall_handler(MSVCRT_purecall_handler function)
{
    MSVCRT_purecall_handler ret = purecall_handler;

    TRACE("(%p)\n", function);
    purecall_handler = function;
    return ret;
}
#endif

#if _MSVCR_VER>=80
/*********************************************************************
 *		_get_purecall_handler (MSVCR80.@)
 */
MSVCRT_purecall_handler CDECL _get_purecall_handler(void)
{
    TRACE("\n");
    return purecall_handler;
}
#endif

/*********************************************************************
 *		_purecall (MSVCRT.@)
 */
void CDECL _purecall(void)
{
  TRACE("(void)\n");

  if(purecall_handler)
      purecall_handler();
  _amsg_exit( 25 );
}

/******************************************************************************
 *		_set_error_mode (MSVCRT.@)
 *
 * Set the error mode, which describes where the C run-time writes error messages.
 *
 * PARAMS
 *   mode - the new error mode
 *
 * RETURNS
 *   The old error mode.
 *
 */
int CDECL _set_error_mode(int mode)
{

  const int old = MSVCRT_error_mode;
  if ( MSVCRT__REPORT_ERRMODE != mode ) {
    MSVCRT_error_mode = mode;
  }
  return old;
}
