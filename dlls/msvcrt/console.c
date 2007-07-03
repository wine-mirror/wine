/*
 * msvcrt.dll console functions
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
 *
 * Note: init and free don't need MT locking since they are called at DLL
 * (de)attachment time, which is syncronised for us
 */

#include "msvcrt.h"
#include "wincon.h"
#include "mtdll.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);



/* MT */
#define LOCK_CONSOLE   _mlock(_CONIO_LOCK)
#define UNLOCK_CONSOLE _munlock(_CONIO_LOCK)

static HANDLE MSVCRT_console_in = INVALID_HANDLE_VALUE;
static HANDLE MSVCRT_console_out= INVALID_HANDLE_VALUE;
static int __MSVCRT_console_buffer = MSVCRT_EOF;

/* INTERNAL: Initialise console handles */
void msvcrt_init_console(void)
{
  TRACE(":Opening console handles\n");

  MSVCRT_console_in = CreateFileA("CONIN$", GENERIC_READ, FILE_SHARE_READ,
                                  NULL, OPEN_EXISTING, 0, NULL);
  MSVCRT_console_out= CreateFileA("CONOUT$", GENERIC_WRITE, FILE_SHARE_WRITE,
				    NULL, OPEN_EXISTING, 0, NULL);

  if ((MSVCRT_console_in == INVALID_HANDLE_VALUE) ||
      (MSVCRT_console_out== INVALID_HANDLE_VALUE))
    WARN(":Console handle Initialisation FAILED!\n");
}

/* INTERNAL: Free console handles */
void msvcrt_free_console(void)
{
  TRACE(":Closing console handles\n");
  CloseHandle(MSVCRT_console_in);
  CloseHandle(MSVCRT_console_out);
}

/*********************************************************************
 *		_cputs (MSVCRT.@)
 */
int CDECL _cputs(const char* str)
{
  DWORD count;
  int retval = MSVCRT_EOF;

  LOCK_CONSOLE;
  if (WriteConsoleA(MSVCRT_console_out, str, strlen(str), &count, NULL)
      && count == 1)
    retval = 0;
  UNLOCK_CONSOLE;
  return retval;
}

/*********************************************************************
 *		_getch (MSVCRT.@)
 */
int CDECL _getch(void)
{
  int retval = MSVCRT_EOF;

  LOCK_CONSOLE;
  if (__MSVCRT_console_buffer != MSVCRT_EOF)
  {
    retval = __MSVCRT_console_buffer;
    __MSVCRT_console_buffer = MSVCRT_EOF;
  }
  else
  {
    INPUT_RECORD ir;
    DWORD count;
    DWORD mode = 0;

    GetConsoleMode(MSVCRT_console_in, &mode);
    if(mode)
      SetConsoleMode(MSVCRT_console_in, 0);

    do {
      if (ReadConsoleInputA(MSVCRT_console_in, &ir, 1, &count))
      {
        /* Only interested in ASCII chars */
        if (ir.EventType == KEY_EVENT &&
            ir.Event.KeyEvent.bKeyDown &&
            ir.Event.KeyEvent.uChar.AsciiChar)
        {
          retval = ir.Event.KeyEvent.uChar.AsciiChar;
          break;
        }
      }
      else
        break;
    } while(1);
    if (mode)
      SetConsoleMode(MSVCRT_console_in, mode);
  }
  UNLOCK_CONSOLE;
  return retval;
}

/*********************************************************************
 *		_putch (MSVCRT.@)
 */
int CDECL _putch(int c)
{
  int retval = MSVCRT_EOF;
  DWORD count;
  LOCK_CONSOLE;
  if (WriteConsoleA(MSVCRT_console_out, &c, 1, &count, NULL) && count == 1)
    retval = c;
  UNLOCK_CONSOLE;
  return retval;
}

/*********************************************************************
 *		_getche (MSVCRT.@)
 */
int CDECL _getche(void)
{
  int retval;
  LOCK_CONSOLE;
  retval = _getch();
  if (retval != MSVCRT_EOF)
    retval = _putch(retval);
  UNLOCK_CONSOLE;
  return retval;
}

/*********************************************************************
 *		_cgets (MSVCRT.@)
 */
char* CDECL _cgets(char* str)
{
  char *buf = str + 2;
  DWORD got;
  DWORD conmode = 0;

  TRACE("(%p)\n", str);
  str[1] = 0; /* Length */
  LOCK_CONSOLE;
  GetConsoleMode(MSVCRT_console_in, &conmode);
  SetConsoleMode(MSVCRT_console_in, ENABLE_LINE_INPUT|ENABLE_ECHO_INPUT|ENABLE_PROCESSED_INPUT);

  if(ReadConsoleA(MSVCRT_console_in, buf, str[0], &got, NULL)) {
    if(buf[got-2] == '\r') {
      buf[got-2] = 0;
      str[1] = got-2;
    }
    else if(got == 1 && buf[got-1] == '\n') {
      buf[0] = 0;
      str[1] = 0;
    }
    else if(got == str[0] && buf[got-1] == '\r') {
      buf[got-1] = 0;
      str[1] = got-1;
    }
    else
      str[1] = got;
  }
  else
    buf = NULL;
  SetConsoleMode(MSVCRT_console_in, conmode);
  UNLOCK_CONSOLE;
  return buf;
}

/*********************************************************************
 *		_ungetch (MSVCRT.@)
 */
int CDECL _ungetch(int c)
{
  int retval = MSVCRT_EOF;
  LOCK_CONSOLE;
  if (c != MSVCRT_EOF && __MSVCRT_console_buffer == MSVCRT_EOF)
    retval = __MSVCRT_console_buffer = c;
  UNLOCK_CONSOLE;
  return retval;
}

/*********************************************************************
 *		_kbhit (MSVCRT.@)
 */
int CDECL _kbhit(void)
{
  int retval = 0;

  LOCK_CONSOLE;
  if (__MSVCRT_console_buffer != MSVCRT_EOF)
    retval = 1;
  else
  {
    /* FIXME: There has to be a faster way than this in Win32.. */
    INPUT_RECORD *ir = NULL;
    DWORD count = 0, i;

    GetNumberOfConsoleInputEvents(MSVCRT_console_in, &count);

    if (count && (ir = MSVCRT_malloc(count * sizeof(INPUT_RECORD))) &&
        PeekConsoleInputA(MSVCRT_console_in, ir, count, &count))
      for(i = 0; i < count - 1; i++)
      {
        if (ir[i].EventType == KEY_EVENT &&
            ir[i].Event.KeyEvent.bKeyDown &&
            ir[i].Event.KeyEvent.uChar.AsciiChar)
        {
          retval = 1;
          break;
        }
      }
    MSVCRT_free(ir);
  }
  UNLOCK_CONSOLE;
  return retval;
}


/*********************************************************************
 *		_cprintf (MSVCRT.@)
 */
int CDECL _cprintf(const char* format, ...)
{
  char buf[2048], *mem = buf;
  int written, resize = sizeof(buf), retval;
  va_list valist;

  va_start( valist, format );
  /* There are two conventions for snprintf failing:
   * Return -1 if we truncated, or
   * Return the number of bytes that would have been written
   * The code below handles both cases
   */
  while ((written = MSVCRT_vsnprintf( mem, resize, format, valist )) == -1 ||
          written > resize)
  {
    resize = (written == -1 ? resize * 2 : written + 1);
    if (mem != buf)
      MSVCRT_free (mem);
    if (!(mem = MSVCRT_malloc(resize)))
      return MSVCRT_EOF;
    va_start( valist, format );
  }
  va_end(valist);
  LOCK_CONSOLE;
  retval = _cputs( mem );
  UNLOCK_CONSOLE;
  if (mem != buf)
    MSVCRT_free (mem);
  return retval;
}
