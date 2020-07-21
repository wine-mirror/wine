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
 * (de)attachment time, which is synchronised for us
 */

#include "msvcrt.h"
#include "winnls.h"
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
static MSVCRT_wchar_t __MSVCRT_console_buffer_w = MSVCRT_WEOF;

/* INTERNAL: Initialise console handles */
void msvcrt_init_console(void)
{
  TRACE(":Opening console handles\n");

  MSVCRT_console_in = CreateFileA("CONIN$", GENERIC_WRITE|GENERIC_READ,
                                  FILE_SHARE_WRITE|FILE_SHARE_READ,
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
  int len, retval = -1;

  if (!MSVCRT_CHECK_PMT(str != NULL)) return -1;
  len = strlen(str);

  LOCK_CONSOLE;
  if (WriteConsoleA(MSVCRT_console_out, str, len, &count, NULL)
      && count == len)
    retval = 0;
  UNLOCK_CONSOLE;
  return retval;
}

/*********************************************************************
 *		_cputws (MSVCRT.@)
 */
int CDECL _cputws(const MSVCRT_wchar_t* str)
{
  DWORD count;
  int len, retval = -1;

  if (!MSVCRT_CHECK_PMT(str != NULL)) return -1;
  len = MSVCRT_wcslen(str);

  LOCK_CONSOLE;
  if (WriteConsoleW(MSVCRT_console_out, str, len, &count, NULL)
      && count == len)
    retval = 0;
  UNLOCK_CONSOLE;
  return retval;
}

#define NORMAL_CHAR     0
#define ALT_CHAR        1
#define CTRL_CHAR       2
#define SHIFT_CHAR      3

static const struct {unsigned short vk; unsigned char ch[4][2];} enh_map[] = {
    {0x47, {{0xE0, 0x47}, {0x00, 0x97}, {0xE0, 0x77}, {0xE0, 0x47}}},
    {0x48, {{0xE0, 0x48}, {0x00, 0x98}, {0xE0, 0x8D}, {0xE0, 0x48}}},
    {0x49, {{0xE0, 0x49}, {0x00, 0x99}, {0xE0, 0x86}, {0xE0, 0x49}}},
    {0x4B, {{0xE0, 0x4B}, {0x00, 0x9B}, {0xE0, 0x73}, {0xE0, 0x4B}}},
    {0x4D, {{0xE0, 0x4D}, {0x00, 0x9D}, {0xE0, 0x74}, {0xE0, 0x4D}}},
    {0x4F, {{0xE0, 0x4F}, {0x00, 0x9F}, {0xE0, 0x75}, {0xE0, 0x4F}}},
    {0x50, {{0xE0, 0x50}, {0x00, 0xA0}, {0xE0, 0x91}, {0xE0, 0x50}}},
    {0x51, {{0xE0, 0x51}, {0x00, 0xA1}, {0xE0, 0x76}, {0xE0, 0x51}}},
    {0x52, {{0xE0, 0x52}, {0x00, 0xA2}, {0xE0, 0x92}, {0xE0, 0x52}}},
    {0x53, {{0xE0, 0x53}, {0x00, 0xA3}, {0xE0, 0x93}, {0xE0, 0x53}}},
};

static BOOL handle_enhanced_keys(INPUT_RECORD *ir, unsigned char *ch1, unsigned char *ch2)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(enh_map); i++)
    {
        if (ir->Event.KeyEvent.wVirtualScanCode == enh_map[i].vk)
        {
            unsigned idx;

            if (ir->Event.KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
                idx = ALT_CHAR;
            else if (ir->Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED) )
                idx = CTRL_CHAR;
            else if (ir->Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED)
                idx = SHIFT_CHAR;
            else
                idx = NORMAL_CHAR;

            *ch1 = enh_map[i].ch[idx][0];
            *ch2 = enh_map[i].ch[idx][1];
            return TRUE;
        }
    }

    WARN("Unmapped char keyState=%x vk=%x\n",
            ir->Event.KeyEvent.dwControlKeyState, ir->Event.KeyEvent.wVirtualScanCode);
    return FALSE;
}

/*********************************************************************
 *		_getch_nolock (MSVCR80.@)
 */
int CDECL _getch_nolock(void)
{
  int retval = MSVCRT_EOF;

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
            ir.Event.KeyEvent.bKeyDown)
        {
            unsigned char ch1, ch2;

            if (ir.Event.KeyEvent.uChar.AsciiChar)
            {
                retval = ir.Event.KeyEvent.uChar.AsciiChar;
                break;
            }

            if (handle_enhanced_keys(&ir, &ch1, &ch2))
            {
                retval = ch1;
                __MSVCRT_console_buffer = ch2;
                break;
            }
        }
      }
      else
        break;
    } while(1);
    if (mode)
      SetConsoleMode(MSVCRT_console_in, mode);
  }
  return retval;
}

/*********************************************************************
 *		_getch (MSVCRT.@)
 */
int CDECL _getch(void)
{
    int ret;

    LOCK_CONSOLE;
    ret = _getch_nolock();
    UNLOCK_CONSOLE;
    return ret;
}

/*********************************************************************
 *		_getwch_nolock (MSVCR80.@)
 */
MSVCRT_wchar_t CDECL _getwch_nolock(void)
{
    MSVCRT_wchar_t retval = MSVCRT_WEOF;

    if (__MSVCRT_console_buffer_w != MSVCRT_WEOF)
    {
        retval = __MSVCRT_console_buffer_w;
        __MSVCRT_console_buffer_w = MSVCRT_WEOF;
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
            if (ReadConsoleInputW(MSVCRT_console_in, &ir, 1, &count))
            {
                /* Only interested in ASCII chars */
                if (ir.EventType == KEY_EVENT &&
                        ir.Event.KeyEvent.bKeyDown)
                {
                    unsigned char ch1, ch2;

                    if (ir.Event.KeyEvent.uChar.UnicodeChar)
                    {
                        retval = ir.Event.KeyEvent.uChar.UnicodeChar;
                        break;
                    }

                    if (handle_enhanced_keys(&ir, &ch1, &ch2))
                    {
                        retval = ch1;
                        __MSVCRT_console_buffer_w = ch2;
                        break;
                    }
                }
            }
            else
                break;
        } while(1);
        if (mode)
            SetConsoleMode(MSVCRT_console_in, mode);
    }
    return retval;
}

/*********************************************************************
 *              _getwch (MSVCRT.@)
 */
MSVCRT_wchar_t CDECL _getwch(void)
{
    MSVCRT_wchar_t ret;

    LOCK_CONSOLE;
    ret = _getwch_nolock();
    UNLOCK_CONSOLE;
    return ret;
}

/*********************************************************************
 *		_putch_nolock (MSVCR80.@)
 */
int CDECL _putch_nolock(int c)
{
  DWORD count;
  if (WriteConsoleA(MSVCRT_console_out, &c, 1, &count, NULL) && count == 1)
    return c;
  return MSVCRT_EOF;
}

/*********************************************************************
 *		_putch (MSVCRT.@)
 */
int CDECL _putch(int c)
{
    LOCK_CONSOLE;
    c = _putch_nolock(c);
    UNLOCK_CONSOLE;
    return c;
}

/*********************************************************************
 *		_putwch_nolock (MSVCR80.@)
 */
MSVCRT_wchar_t CDECL _putwch_nolock(MSVCRT_wchar_t c)
{
    DWORD count;
    if (WriteConsoleW(MSVCRT_console_out, &c, 1, &count, NULL) && count==1)
        return c;
    return MSVCRT_WEOF;
}

/*********************************************************************
 *		_putwch (MSVCRT.@)
 */
MSVCRT_wchar_t CDECL _putwch(MSVCRT_wchar_t c)
{
    LOCK_CONSOLE;
    c = _putwch_nolock(c);
    UNLOCK_CONSOLE;
    return c;
}

/*********************************************************************
 *		_getche_nolock (MSVCR80.@)
 */
int CDECL _getche_nolock(void)
{
  int retval;
  retval = _getch_nolock();
  if (retval != MSVCRT_EOF)
    retval = _putch_nolock(retval);
  return retval;
}

/*********************************************************************
 *		_getche (MSVCRT.@)
 */
int CDECL _getche(void)
{
    int ret;

    LOCK_CONSOLE;
    ret = _getche_nolock();
    UNLOCK_CONSOLE;
    return ret;
}

/*********************************************************************
 *              _getwche_nolock (MSVCR80.@)
 */
MSVCRT_wchar_t CDECL _getwche_nolock(void)
{
    MSVCRT_wchar_t wch;
    wch = _getch_nolock();
    if (wch == MSVCRT_WEOF)
        return wch;
    return _putwch_nolock(wch);
}

/*********************************************************************
 *              _getwche (MSVCRT.@)
 */
MSVCRT_wchar_t CDECL _getwche(void)
{
    MSVCRT_wchar_t ret;

    LOCK_CONSOLE;
    ret = _getwche_nolock();
    UNLOCK_CONSOLE;
    return ret;
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
 *		_ungetch_nolock (MSVCRT.@)
 */
int CDECL _ungetch_nolock(int c)
{
  int retval = MSVCRT_EOF;
  if (c != MSVCRT_EOF && __MSVCRT_console_buffer == MSVCRT_EOF)
    retval = __MSVCRT_console_buffer = c;
  return retval;
}

/*********************************************************************
 *		_ungetch (MSVCRT.@)
 */
int CDECL _ungetch(int c)
{
    LOCK_CONSOLE;
    c = _ungetch_nolock(c);
    UNLOCK_CONSOLE;
    return c;
}

/*********************************************************************
 *              _ungetwch_nolock (MSVCR80.@)
 */
MSVCRT_wchar_t CDECL _ungetwch_nolock(MSVCRT_wchar_t c)
{
    MSVCRT_wchar_t retval = MSVCRT_WEOF;
    if (c != MSVCRT_WEOF && __MSVCRT_console_buffer_w == MSVCRT_WEOF)
        retval = __MSVCRT_console_buffer_w = c;
    return retval;
}

/*********************************************************************
 *              _ungetwch (MSVCRT.@)
 */
MSVCRT_wchar_t CDECL _ungetwch(MSVCRT_wchar_t c)
{
    LOCK_CONSOLE;
    c = _ungetwch_nolock(c);
    UNLOCK_CONSOLE;
    return c;
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

static int puts_clbk_console_a(void *ctx, int len, const char *str)
{
    LOCK_CONSOLE;
    if(!WriteConsoleA(MSVCRT_console_out, str, len, NULL, NULL))
        len = -1;
    UNLOCK_CONSOLE;
    return len;
}

static int puts_clbk_console_w(void *ctx, int len, const MSVCRT_wchar_t *str)
{
    LOCK_CONSOLE;
    if(!WriteConsoleW(MSVCRT_console_out, str, len, NULL, NULL))
        len = -1;
    UNLOCK_CONSOLE;
    return len;
}

/*********************************************************************
 *		_vcprintf (MSVCRT.@)
 */
int CDECL _vcprintf(const char* format, __ms_va_list valist)
{
    return pf_printf_a(puts_clbk_console_a, NULL, format, NULL, 0, arg_clbk_valist, NULL, &valist);
}

/*********************************************************************
 *		_cprintf (MSVCRT.@)
 */
int WINAPIV _cprintf(const char* format, ...)
{
  int retval;
  __ms_va_list valist;

  __ms_va_start( valist, format );
  retval = _vcprintf(format, valist);
  __ms_va_end(valist);

  return retval;
}


/*********************************************************************
 *		_vcwprintf (MSVCRT.@)
 */
int CDECL _vcwprintf(const MSVCRT_wchar_t* format, __ms_va_list valist)
{
    return pf_printf_w(puts_clbk_console_w, NULL, format, NULL, 0, arg_clbk_valist, NULL, &valist);
}

/*********************************************************************
 *		_cwprintf (MSVCRT.@)
 */
int WINAPIV _cwprintf(const MSVCRT_wchar_t* format, ...)
{
  int retval;
  __ms_va_list valist;

  __ms_va_start( valist, format );
  retval = _vcwprintf(format, valist);
  __ms_va_end(valist);

  return retval;
}

#if _MSVCR_VER>=140

/*********************************************************************
 *		__conio_common_vcprintf (UCRTBASE.@)
 */
int CDECL MSVCRT__conio_common_vcprintf(unsigned __int64 options, const char* format,
                                        MSVCRT__locale_t locale, __ms_va_list valist)
{
    if (options & ~UCRTBASE_PRINTF_MASK)
        FIXME("options %s not handled\n", wine_dbgstr_longlong(options));
    return pf_printf_a(puts_clbk_console_a, NULL, format, locale,
             options & UCRTBASE_PRINTF_MASK, arg_clbk_valist, NULL, &valist);
}

/*********************************************************************
 *		__conio_common_vcwprintf (UCRTBASE.@)
 */
int CDECL MSVCRT__conio_common_vcwprintf(unsigned __int64 options, const MSVCRT_wchar_t* format,
                                         MSVCRT__locale_t locale, __ms_va_list valist)
{
    if (options & ~UCRTBASE_PRINTF_MASK)
        FIXME("options %s not handled\n", wine_dbgstr_longlong(options));
    return pf_printf_w(puts_clbk_console_w, NULL, format, locale,
             options & UCRTBASE_PRINTF_MASK, arg_clbk_valist, NULL, &valist);
}

#endif /* _MSVCR_VER>=140 */
