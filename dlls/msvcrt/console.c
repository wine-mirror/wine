/*
 * msvcrt.dll console functions
 *
 * Copyright 2000 Jon Griffiths
 *
 * Note: init and free don't need MT locking since they are called at DLL
 * (de)attachment time, which is syncronised for us
 */
#include "msvcrt.h"
#include "wincon.h"

#include "msvcrt/conio.h"
#include "msvcrt/malloc.h"
#include "msvcrt/stdio.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);



/* MT */
extern CRITICAL_SECTION MSVCRT_console_cs;
#define LOCK_CONSOLE   EnterCriticalSection(&MSVCRT_console_cs)
#define UNLOCK_CONSOLE LeaveCriticalSection(&MSVCRT_console_cs)

static HANDLE MSVCRT_console_in = INVALID_HANDLE_VALUE;
static HANDLE MSVCRT_console_out= INVALID_HANDLE_VALUE;
static int __MSVCRT_console_buffer = MSVCRT_EOF;

/* INTERNAL: Initialise console handles */
void msvcrt_init_console(void)
{
  TRACE(":Opening console handles\n");

  MSVCRT_console_in = GetStdHandle(STD_INPUT_HANDLE);

  /* FIXME: Should be initialised with:
   * CreateFileA("CONIN$", GENERIC_READ, FILE_SHARE_READ,
   * NULL, OPEN_EXISTING, 0, (HANDLE)NULL);
   */

  MSVCRT_console_out= CreateFileA("CONOUT$", GENERIC_WRITE, FILE_SHARE_WRITE,
				    NULL, OPEN_EXISTING, 0, (HANDLE)NULL);

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
int _cputs(const char* str)
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
int _getch(void)
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
int _putch(int c)
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
int _getche(void)
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
char* _cgets(char* str)
{
  char *buf = str + 2;
  int c;
  str[1] = 0; /* Length */
  /* FIXME: No editing of string supported */
  LOCK_CONSOLE;
  do
  {
    if (str[1] >= str[0] || (str[1]++, c = _getche()) == MSVCRT_EOF || c == '\n')
      break;
    *buf++ = c & 0xff;
  } while (1);
  UNLOCK_CONSOLE;
  *buf = '\0';
  return str + 2;
}

/*********************************************************************
 *		_ungetch (MSVCRT.@)
 */
int _ungetch(int c)
{
  int retval = MSVCRT_EOF;
  LOCK_CONSOLE;
  if (c != MSVCRT_EOF && __MSVCRT_console_buffer == MSVCRT_EOF)
    retval = __MSVCRT_console_buffer = c;
  UNLOCK_CONSOLE;
  return retval;
}

/*********************************************************************
 *		_cscanf (MSVCRT.@)
 */
int _cscanf(const char* format, ...)
{
    /* NOTE: If you extend this function, extend MSVCRT_fscanf in file.c too */
    int rd = 0;
    int nch;
    va_list ap;
    if (!*format) return 0;
    WARN("\"%s\": semi-stub\n", format);
    va_start(ap, format);
  LOCK_CONSOLE;
    nch = _getch();
    while (*format) {
        if (*format == ' ') {
            /* skip whitespace */
            while ((nch!=MSVCRT_EOF) && isspace(nch))
                nch = _getch();
        }
        else if (*format == '%') {
            int st = 0;
            format++;
            switch(*format) {
            case 'd': { /* read an integer */
                    int*val = va_arg(ap, int*);
                    int cur = 0;
                    /* skip initial whitespace */
                    while ((nch!=MSVCRT_EOF) && isspace(nch))
                        nch = _getch();
                    /* get sign and first digit */
                    if (nch == '-') {
                        nch = _getch();
                        if (isdigit(nch))
                            cur = -(nch - '0');
                        else break;
                    } else {
                        if (isdigit(nch))
                            cur = nch - '0';
                        else break;
                    }
                    nch = _getch();
                    /* read until no more digits */
                    while ((nch!=MSVCRT_EOF) && isdigit(nch)) {
                        cur = cur*10 + (nch - '0');
                        nch = _getch();
                    }
                    st = 1;
                    *val = cur;
                }
                break;
            case 'f': { /* read a float */
                    float*val = va_arg(ap, float*);
                    float cur = 0;
                    /* skip initial whitespace */
                    while ((nch!=MSVCRT_EOF) && isspace(nch))
                        nch = _getch();
                    /* get sign and first digit */
                    if (nch == '-') {
                        nch = _getch();
                        if (isdigit(nch))
                            cur = -(nch - '0');
                        else break;
                    } else {
                        if (isdigit(nch))
                            cur = nch - '0';
                        else break;
                    }
                    /* read until no more digits */
                    while ((nch!=MSVCRT_EOF) && isdigit(nch)) {
                        cur = cur*10 + (nch - '0');
                        nch = _getch();
                    }
                    if (nch == '.') {
                        /* handle decimals */
                        float dec = 1;
                        nch = _getch();
                        while ((nch!=MSVCRT_EOF) && isdigit(nch)) {
                            dec /= 10;
                            cur += dec * (nch - '0');
                            nch = _getch();
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
                    while ((nch!=MSVCRT_EOF) && isspace(nch))
                        nch = _getch();
                    /* read until whitespace */
                    while ((nch!=MSVCRT_EOF) && !isspace(nch)) {
                        *sptr++ = nch; st++;
                        nch = _getch();
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
               nch = _getch();
            else break;
        }
        format++;
    }
    if (nch != MSVCRT_EOF)
      _ungetch(nch);
    UNLOCK_CONSOLE;
    va_end(ap);
    TRACE("returning %d\n", rd);
    return rd;
}

/*********************************************************************
 *		_kbhit (MSVCRT.@)
 */
int _kbhit(void)
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
    if (ir)
      MSVCRT_free(ir);
  }
  UNLOCK_CONSOLE;
  return retval;
}


/*********************************************************************
 *		_cprintf (MSVCRT.@)
 */
int _cprintf(const char* format, ...)
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
  while ((written = _snprintf( mem, resize, format, valist )) == -1 ||
          written > resize)
  {
    resize = (written == -1 ? resize * 2 : written + 1);
    if (mem != buf)
      MSVCRT_free (mem);
    if (!(mem = (char *)MSVCRT_malloc(resize)))
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
