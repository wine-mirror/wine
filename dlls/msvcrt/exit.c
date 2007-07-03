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

static const char szMsgBoxTitle[] = "Wine C++ Runtime Library";

extern int MSVCRT_app_type;
extern char *MSVCRT__pgmptr;

void (*_aexit_rtn)(int) = MSVCRT__exit;

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

  tmp = MSVCRT_realloc(*start, len * sizeof(tmp));
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
static void DoMessageBox(LPCSTR lead, LPCSTR message)
{
  MSGBOXPARAMSA msgbox;
  char text[2048];
  INT ret;

  snprintf(text,sizeof(text),"%s\n\nProgram: %s\n%s\n\n"
               "Press OK to exit the program, or Cancel to start the Wine debugger.\n ",
               lead, MSVCRT__pgmptr, message);

  msgbox.cbSize = sizeof(msgbox);
  msgbox.hwndOwner = GetActiveWindow();
  msgbox.hInstance = 0;
  msgbox.lpszText = text;
  msgbox.lpszCaption = szMsgBoxTitle;
  msgbox.dwStyle = MB_OKCANCEL|MB_ICONERROR;
  msgbox.lpszIcon = NULL;
  msgbox.dwContextHelpId = 0;
  msgbox.lpfnMsgBoxCallback = NULL;
  msgbox.dwLanguageId = LANG_NEUTRAL;

  ret = MessageBoxIndirectA(&msgbox);
  if (ret == IDCANCEL)
    DebugBreak();
}

/*********************************************************************
 *		_amsg_exit (MSVCRT.@)
 */
void CDECL _amsg_exit(int errnum)
{
  TRACE("(%d)\n", errnum);
  /* FIXME: text for the error number. */
  if (MSVCRT_app_type == 2)
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
  if (MSVCRT_app_type == 2)
  {
    DoMessageBox("Runtime error!", "abnormal program termination");
  }
  else
    _cputs("\nabnormal program termination\n");
  MSVCRT_raise(MSVCRT_SIGABRT);
  /* in case raise() returns */
  MSVCRT__exit(3);
}

/*********************************************************************
 *		_assert (MSVCRT.@)
 */
void CDECL MSVCRT__assert(const char* str, const char* file, unsigned int line)
{
  TRACE("(%s,%s,%d)\n",str,file,line);
  if (MSVCRT_app_type == 2)
  {
    char text[2048];
    snprintf(text, sizeof(text), "File: %s\nLine: %d\n\nExpression: \"%s\"", file, line, str);
    DoMessageBox("Assertion failed!", text);
  }
  else
    _cprintf("Assertion failed: %s, file %s, line %d\n\n",str, file, line);
  MSVCRT_raise(MSVCRT_SIGABRT);
  /* in case raise() returns */
  MSVCRT__exit(3);
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
  TRACE("(%d)\n",exitcode);
  MSVCRT__cexit();
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
 *		_purecall (MSVCRT.@)
 */
void CDECL _purecall(void)
{
  TRACE("(void)\n");
  _amsg_exit( 25 );
}
