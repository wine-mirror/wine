/*
 * CRTDLL console functions
 * 
 * Copyright 2000 Jon Griffiths
 *
 * NOTES
 * Only a one byte ungetch buffer is implemented, as per MS docs.
 * Output is not redirectable using these functions, as per MS docs.
 *
 * FIXME:
 * There are several problems with the console input mechanism as
 * currently implemented in Wine. When these are ironed out the
 * getch() function will work correctly (gets() is currently fine).
 * The problem is that opening CONIN$ does not work, and
 * reading from STD_INPUT_HANDLE is line buffered.
 */
#include "crtdll.h"
#include "wincon.h"

DEFAULT_DEBUG_CHANNEL(crtdll);

static HANDLE __CRTDLL_console_in = INVALID_HANDLE_VALUE;
static HANDLE __CRTDLL_console_out = INVALID_HANDLE_VALUE;
static int __CRTDLL_console_buffer = EOF;


/* INTERNAL: Initialise console handles */
VOID __CRTDLL_init_console(VOID)
{
  TRACE(":Opening console handles\n");

  __CRTDLL_console_in = GetStdHandle(STD_INPUT_HANDLE);

/* FIXME: Should be initialised with:
   * CreateFileA("CONIN$", GENERIC_READ, FILE_SHARE_READ,
   * NULL, OPEN_EXISTING, 0, (HANDLE)NULL);
   */

  __CRTDLL_console_out = CreateFileA("CONOUT$", GENERIC_WRITE, FILE_SHARE_WRITE,
				    NULL, OPEN_EXISTING, 0, (HANDLE)NULL);

  if ((__CRTDLL_console_in == INVALID_HANDLE_VALUE) ||
      (__CRTDLL_console_out == INVALID_HANDLE_VALUE))
    WARN(":Console handle Initialisation FAILED!\n");
}

/* INTERNAL: Free console handles */
void __CRTDLL_free_console(void)
{
  TRACE(":Closing console handles\n");
  CloseHandle(__CRTDLL_console_in);
  CloseHandle(__CRTDLL_console_out);
}


/*********************************************************************
 *                  _cgets       (CRTDLL.050)
 *
 * Get a string from CONIN$.
 */
LPSTR __cdecl CRTDLL__cgets(LPSTR str)
{
  char *buf = str + 2; 
  int c;
  str[1] = 0; /* Length */
  /* FIXME: No editing of string supported */
  do
  {
    if (str[1] >= str[0] || (str[1]++, c = CRTDLL__getche()) == EOF || c == '\n')
    {
      *buf = '\0';
      return str + 2;
    }
    *buf++ = c & 0xff;
  } while(1);
}


/*********************************************************************
 *                  _cputs       (CRTDLL.065)
 *
 * Write a string to CONOUT$.
 */
INT __cdecl CRTDLL__cputs(LPCSTR str)
{
  DWORD count;
  if (WriteConsoleA(__CRTDLL_console_out, str, strlen(str), &count, NULL)
      && count == 1)
    return 0;
  return EOF;
}


/*********************************************************************
 *                  _getch      (CRTDLL.118)
 *
 * Get a character from CONIN$.
 */
INT __cdecl CRTDLL__getch(VOID)
{
  if (__CRTDLL_console_buffer != EOF)
  {
    INT retVal = __CRTDLL_console_buffer;
    __CRTDLL_console_buffer = EOF;
    return retVal;
  }
  else
  {
    INPUT_RECORD ir;
    DWORD count;
    DWORD mode = 0;

    GetConsoleMode(__CRTDLL_console_in, &mode);
    if(mode) SetConsoleMode(__CRTDLL_console_in, 0);

    do {
      if (ReadConsoleInputA(__CRTDLL_console_in, &ir, 1, &count))
      {
        /* Only interested in ASCII chars */
        if (ir.EventType == KEY_EVENT &&
            ir.Event.KeyEvent.bKeyDown &&
            ir.Event.KeyEvent.uChar.AsciiChar)
        {
	  if(mode) SetConsoleMode(__CRTDLL_console_in, mode);
         return ir.Event.KeyEvent.uChar.AsciiChar;
        }
      }
      else
        break;
    } while(1);
    if (mode) SetConsoleMode(__CRTDLL_console_in, mode);
  }
  return EOF;
}


/*********************************************************************
 *                  _getche      (CRTDLL.119)
 *
 * Get a character from CONIN$ and echo it to CONOUT$.
 */
INT __cdecl CRTDLL__getche(VOID)
{
  INT res = CRTDLL__getch();
  if (res != EOF && CRTDLL__putch(res) != EOF)
    return res;
  return EOF;
}


/*********************************************************************
 *                  _kbhit      (CRTDLL.169)
 *
 * Check if a character is waiting in CONIN$.
 */
INT __cdecl CRTDLL__kbhit(VOID)
{
  if (__CRTDLL_console_buffer != EOF)
    return 1;
  else
  {
    /* FIXME: There has to be a faster way than this in Win32.. */
    INPUT_RECORD *ir;
    DWORD count = 0;
    int retVal = 0, i;

    GetNumberOfConsoleInputEvents(__CRTDLL_console_in, &count);
    if (!count)
      return 0;

    if (!(ir = CRTDLL_malloc(count * sizeof(INPUT_RECORD))))
      return 0;

    if (!PeekConsoleInputA(__CRTDLL_console_in, ir, count, &count))
      return 0;

    for(i = 0; i < count - 1; i++)
    {
      if (ir[i].EventType == KEY_EVENT &&
          ir[i].Event.KeyEvent.bKeyDown &&
          ir[i].Event.KeyEvent.uChar.AsciiChar)
      {
        retVal = 1;
        break;
      }
    }
    CRTDLL_free(ir);
    return retVal;
  }
}


/*********************************************************************
 *                  _putch      (CRTDLL.250)
 *
 * Write a character to CONOUT$.
 */
INT __cdecl CRTDLL__putch(INT c)
{
  DWORD count;
  if (WriteConsoleA(__CRTDLL_console_out, &c, 1, &count, NULL) &&
      count == 1)
    return c;
  return EOF;
}


/*********************************************************************
 *                  _ungetch      (CRTDLL.311)
 *
 * Un-get a character from CONIN$.
 */
INT __cdecl CRTDLL__ungetch(INT c)
{
  if (c == EOF || __CRTDLL_console_buffer != EOF)
    return EOF;

  return __CRTDLL_console_buffer = c;
}

