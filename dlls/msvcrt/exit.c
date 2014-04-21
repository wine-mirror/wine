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

static MSVCRT__onexit_t *MSVCRT_atexit_table = NULL;
static int MSVCRT_atexit_table_size = 0;
static int MSVCRT_atexit_registered = 0; /* Points to free slot */
static MSVCRT_purecall_handler purecall_handler = NULL;

static const char szMsgBoxTitle[] = "Wine C++ Runtime Library";

extern int MSVCRT_app_type;
extern MSVCRT_wchar_t *MSVCRT__wpgmptr;

static unsigned int MSVCRT_abort_behavior =  MSVCRT__WRITE_ABORT_MSG | MSVCRT__CALL_REPORTFAULT;
static int MSVCRT_error_mode = MSVCRT__OUT_TO_DEFAULT;

void (*CDECL _aexit_rtn)(int) = MSVCRT__exit;

/* INTERNAL: call atexit functions */
static void __MSVCRT__call_atexit(void)
{
  /* Note: should only be called with the exit lock held */
  TRACE("%d atext functions to call\n", MSVCRT_atexit_registered);
  /* Last registered gets executed first */
  while (MSVCRT_atexit_registered > 0)
  {
    MSVCRT_atexit_registered--;
    TRACE("next is %p\n",MSVCRT_atexit_table[MSVCRT_atexit_registered]);
    if (MSVCRT_atexit_table[MSVCRT_atexit_registered])
      (*MSVCRT_atexit_table[MSVCRT_atexit_registered])();
    TRACE("returned\n");
  }
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
  static const MSVCRT_wchar_t message_format[] = {'%','s','\n','\n','P','r','o','g','r','a','m',':',' ','%','s','\n',
    '%','s','\n','\n','P','r','e','s','s',' ','O','K',' ','t','o',' ','e','x','i','t',' ','t','h','e',' ',
    'p','r','o','g','r','a','m',',',' ','o','r',' ','C','a','n','c','e','l',' ','t','o',' ','s','t','a','r','t',' ',
    't','h','e',' ','W','i','n','e',' ','d','e','b','b','u','g','e','r','.','\n',0};

  MSGBOXPARAMSW msgbox;
  MSVCRT_wchar_t text[2048];
  INT ret;

  MSVCRT__snwprintf(text,sizeof(text),message_format, lead, MSVCRT__wpgmptr, message);

  msgbox.cbSize = sizeof(msgbox);
  msgbox.hwndOwner = GetActiveWindow();
  msgbox.hInstance = 0;
  msgbox.lpszText = (LPCWSTR)text;
  msgbox.lpszCaption = (LPCWSTR)szMsgBoxTitle;
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
    sprintf(text, "Error: R60%d",errnum);
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

/*********************************************************************
 *              _wassert (MSVCRT.@)
 */
void CDECL MSVCRT__wassert(const MSVCRT_wchar_t* str, const MSVCRT_wchar_t* file, unsigned int line)
{
  static const MSVCRT_wchar_t assertion_failed[] = {'A','s','s','e','r','t','i','o','n',' ','f','a','i','l','e','d','!',0};
  static const MSVCRT_wchar_t format_msgbox[] = {'F','i','l','e',':',' ','%','s','\n','L','i','n','e',':',' ','%','d',
      '\n','\n','E','x','p','r','e','s','s','i','o','n',':',' ','\"','%','s','\"',0};
  static const MSVCRT_wchar_t format_console[] = {'A','s','s','e','r','t','i','o','n',' ','f','a','i','l','e','d',':',' ',
      '%','s',',',' ','f','i','l','e',' ','%','s',',',' ','l','i','n','e',' ','%','d','\n','\n',0};

  TRACE("(%s,%s,%d)\n", debugstr_w(str), debugstr_w(file), line);

  if ((MSVCRT_error_mode == MSVCRT__OUT_TO_MSGBOX) ||
     ((MSVCRT_error_mode == MSVCRT__OUT_TO_DEFAULT) && (MSVCRT_app_type == 2)))
  {
    MSVCRT_wchar_t text[2048];
    MSVCRT__snwprintf(text, sizeof(text), format_msgbox, file, line, str);
    DoMessageBoxW(assertion_failed, text);
  }
  else
    _cwprintf(format_console, str, file, line);

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
  __MSVCRT__call_atexit();
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
  if (MSVCRT_atexit_registered > MSVCRT_atexit_table_size - 1)
  {
    MSVCRT__onexit_t *newtable;
    TRACE("expanding table\n");
    newtable = MSVCRT_calloc(sizeof(void *),MSVCRT_atexit_table_size + 32);
    if (!newtable)
    {
      TRACE("failed!\n");
      UNLOCK_EXIT;
      return NULL;
    }
    memcpy (newtable, MSVCRT_atexit_table, MSVCRT_atexit_table_size);
    MSVCRT_atexit_table_size += 32;
    MSVCRT_free (MSVCRT_atexit_table);
    MSVCRT_atexit_table = newtable;
  }
  MSVCRT_atexit_table[MSVCRT_atexit_registered] = func;
  MSVCRT_atexit_registered++;
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
int CDECL MSVCRT_atexit(void (*func)(void))
{
  TRACE("(%p)\n", func);
  return MSVCRT__onexit((MSVCRT__onexit_t)func) == (MSVCRT__onexit_t)func ? 0 : -1;
}

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
