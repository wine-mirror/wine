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
#include <stdio.h>

DEFAULT_DEBUG_CHANNEL(crtdll);

static HANDLE __CRTDLL_console_in = INVALID_HANDLE_VALUE;
static HANDLE __CRTDLL_console_out = INVALID_HANDLE_VALUE;
static int __CRTDLL_console_buffer = CRTDLL_EOF;


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
    if (str[1] >= str[0] || (str[1]++, c = CRTDLL__getche()) == CRTDLL_EOF || c == '\n')
    {
      *buf = '\0';
      return str + 2;
    }
    *buf++ = c & 0xff;
  } while(1);
}


/*********************************************************************
 *                  _cprintf     (CRTDLL.064)
 *
 * Write a formatted string to CONOUT$.
 */
INT __cdecl CRTDLL__cprintf( LPCSTR format, ... )
{
  va_list valist;
  char buffer[2048];

  va_start( valist, format );
  if (snprintf( buffer, sizeof(buffer), format, valist ) == -1)
    ERR("Format too large for internal buffer!\n");
  va_end(valist);
  return CRTDLL__cputs( buffer );
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
  return CRTDLL_EOF;
}


/*********************************************************************
 *                  _cscanf      (CRTDLL.067)
 *
 * Read formatted input from CONIN$.
 */
INT __cdecl CRTDLL__cscanf( LPCSTR format, ... )
{
    /* NOTE: If you extend this function, extend CRTDLL_fscanf in file.c too */
    INT rd = 0;
    int nch;
    va_list ap;
    if (!*format) return 0;
    WARN("\"%s\": semi-stub\n", format);
    nch = CRTDLL__getch();
    va_start(ap, format);
    while (*format) {
        if (*format == ' ') {
            /* skip whitespace */
            while ((nch!=CRTDLL_EOF) && isspace(nch))
                nch = CRTDLL__getch();
        }
        else if (*format == '%') {
            int st = 0;
            format++;
            switch(*format) {
            case 'd': { /* read an integer */
                    int*val = va_arg(ap, int*);
                    int cur = 0;
                    /* skip initial whitespace */
                    while ((nch!=CRTDLL_EOF) && isspace(nch))
                        nch = CRTDLL__getch();
                    /* get sign and first digit */
                    if (nch == '-') {
                        nch = CRTDLL__getch();
                        if (isdigit(nch))
                            cur = -(nch - '0');
                        else break;
                    } else {
                        if (isdigit(nch))
                            cur = nch - '0';
                        else break;
                    }
                    nch = CRTDLL__getch();
                    /* read until no more digits */
                    while ((nch!=CRTDLL_EOF) && isdigit(nch)) {
                        cur = cur*10 + (nch - '0');
                        nch = CRTDLL__getch();
                    }
                    st = 1;
                    *val = cur;
                }
                break;
            case 'f': { /* read a float */
                    float*val = va_arg(ap, float*);
                    float cur = 0;
                    /* skip initial whitespace */
                    while ((nch!=CRTDLL_EOF) && isspace(nch))
                        nch = CRTDLL__getch();
                    /* get sign and first digit */
                    if (nch == '-') {
                        nch = CRTDLL__getch();
                        if (isdigit(nch))
                            cur = -(nch - '0');
                        else break;
                    } else {
                        if (isdigit(nch))
                            cur = nch - '0';
                        else break;
                    }
                    /* read until no more digits */
                    while ((nch!=CRTDLL_EOF) && isdigit(nch)) {
                        cur = cur*10 + (nch - '0');
                        nch = CRTDLL__getch();
                    }
                    if (nch == '.') {
                        /* handle decimals */
                        float dec = 1;
                        nch = CRTDLL__getch();
                        while ((nch!=CRTDLL_EOF) && isdigit(nch)) {
                            dec /= 10;
                            cur += dec * (nch - '0');
                            nch = CRTDLL__getch();
                        }
                    }
                    st = 1;
                    *val = cur;
                }
                break;
            case 's': { /* read a word */
                    char*str = va_arg(ap, char*);
                    char*sptr = str;
                    /* skip initial whitespace */
                    while ((nch!=CRTDLL_EOF) && isspace(nch))
                        nch = CRTDLL__getch();
                    /* read until whitespace */
                    while ((nch!=CRTDLL_EOF) && !isspace(nch)) {
                        *sptr++ = nch; st++;
                        nch = CRTDLL__getch();
                    }
                    /* terminate */
                    *sptr = 0;
                    TRACE("read word: %s\n", str);
                }
                break;
            default: FIXME("unhandled: %%%c\n", *format);
            }
            if (st) rd++;
            else break;
        }
        else {
            /* check for character match */
            if (nch == *format)
               nch = CRTDLL__getch();
            else break;
        }
        format++;
    }
    va_end(ap);
    if (nch != CRTDLL_EOF)
      CRTDLL__ungetch(nch);

    TRACE("returning %d\n", rd);
    return rd;
}


/*********************************************************************
 *                  _getch      (CRTDLL.118)
 *
 * Get a character from CONIN$.
 */
INT __cdecl CRTDLL__getch(VOID)
{
  if (__CRTDLL_console_buffer != CRTDLL_EOF)
  {
    INT retVal = __CRTDLL_console_buffer;
    __CRTDLL_console_buffer = CRTDLL_EOF;
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
  return CRTDLL_EOF;
}


/*********************************************************************
 *                  _getche      (CRTDLL.119)
 *
 * Get a character from CONIN$ and echo it to CONOUT$.
 */
INT __cdecl CRTDLL__getche(VOID)
{
  INT res = CRTDLL__getch();
  if (res != CRTDLL_EOF && CRTDLL__putch(res) != CRTDLL_EOF)
    return res;
  return CRTDLL_EOF;
}


/*********************************************************************
 *                  _kbhit      (CRTDLL.169)
 *
 * Check if a character is waiting in CONIN$.
 */
INT __cdecl CRTDLL__kbhit(VOID)
{
  if (__CRTDLL_console_buffer != CRTDLL_EOF)
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
  return CRTDLL_EOF;
}


/*********************************************************************
 *                  _ungetch      (CRTDLL.311)
 *
 * Un-get a character from CONIN$.
 */
INT __cdecl CRTDLL__ungetch(INT c)
{
  if (c == CRTDLL_EOF || __CRTDLL_console_buffer != CRTDLL_EOF)
    return CRTDLL_EOF;

  return __CRTDLL_console_buffer = c;
}

