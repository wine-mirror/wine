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

/* helper function for _cscanf.  Returns the value of character c in the
 * given base, or -1 if the given character is not a digit of the base.
 */
static int char2digit(char c, int base) {
    if ((c>='0') && (c<='9') && (c<='0'+base-1)) return (c-'0');
    if (base<=10) return -1;
    if ((c>='A') && (c<='Z') && (c<='A'+base-11)) return (c-'A'+10);
    if ((c>='a') && (c<='z') && (c<='a'+base-11)) return (c-'a'+10);
    return -1;
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
	/* a whitespace character in the format string causes scanf to read,
	 * but not store, all consecutive white-space characters in the input
	 * up to the next non-white-space character.  One white space character
	 * in the input matches any number (including zero) and combination of
	 * white-space characters in the input. */
	if (isspace(*format)) {
            /* skip whitespace */
            while ((nch!=MSVCRT_EOF) && isspace(nch))
                nch = _getch();
        }
	/* a format specification causes scanf to read and convert characters
	 * in the input into values of a specified type.  The value is assigned
	 * to an argument in the argument list.  Format specifications have
	 * the form %[*][width][{h | l | I64 | L}]type */
	/* FIXME: unimplemented: h/l/I64/L modifiers and some type specs. */
        else if (*format == '%') {
            int st = 0; int suppress = 0; int width = 0;
	    int base, number_signed;
            format++;
	    /* look for leading asterisk, which means 'suppress assignment of
	     * this field'. */
	    if (*format=='*') {
		format++;
		suppress=1;
	    }
	    /* look for width specification */
	    while (isdigit(*format)) {
		width*=10;
		width+=*format++ - '0';
	    }
	    if (width==0) width=-1; /* no width spec seen */
            switch(*format) {
	    case '%': /* read a percent symbol */
		if (nch!='%') break;
		nch = _getch();
		break;
	    case 'x':
	    case 'X': /* hexadecimal integer. */
		base = 16; number_signed = 0;
		goto number;
	    case 'o': /* octal integer */
		base = 8; number_signed = 0;
		goto number;
	    case 'u': /* unsigned decimal integer */
		base = 10; number_signed = 0;
		goto number;
	    case 'd': /* signed decimal integer */
		base = 10; number_signed = 1;
		goto number;
	    case 'i': /* generic integer */
		base = 0; number_signed = 1;
	    number: {
		    /* read an integer */
                    int*val = suppress ? NULL : va_arg(ap, int*);
                    int cur = 0; int negative = 0; int seendigit=0;
                    /* skip initial whitespace */
                    while ((nch!=MSVCRT_EOF) && isspace(nch))
                        nch = _getch();
                    /* get sign */
                    if (number_signed && (nch == '-' || nch == '+')) {
			negative = (nch=='-');
                        nch = _getch();
			if (width>0) width--;
                    }
		    /* look for leading indication of base */
		    if (width!=0 && nch == '0') {
                        nch = _getch();
			if (width>0) width--;
			seendigit=1;
			if (width!=0 && (nch=='x' || nch=='X')) {
			    if (base==0)
				base=16;
			    if (base==16) {
				nch = _getch();
				if (width>0) width--;
				seendigit=0;
			    }
			} else if (base==0)
			    base = 8;
		    }
		    if (base==0)
			base=10;
		    /* throw away leading zeros */
		    while (width!=0 && nch=='0') {
                        nch = _getch();
			if (width>0) width--;
			seendigit=1;
		    }
		    /* get first digit.  Keep working copy negative, as the
		     * range of negative numbers in two's complement notation
		     * is one larger than the range of positive numbers. */
		    if (width!=0 && char2digit(nch, base)!=-1) {
			cur = -char2digit(nch, base);
			nch = _getch();
			if (width>0) width--;
			seendigit=1;
		    }
                    /* read until no more digits */
                    while (width!=0 && (nch!=MSVCRT_EOF) && isdigit(nch)) {
                        cur = cur*base + char2digit(nch, base);
                        nch = _getch();
			if (width>0) width--;
			seendigit=1;
                    }
		    /* negate parsed number if non-negative */
		    if (!negative) cur=-cur;
		    /* okay, done! */
		    if (!seendigit) break; /* not a valid number */
                    st = 1;
                    if (!suppress) *val = cur;
                }
                break;
	    case 'e':
	    case 'E':
	    case 'f':
	    case 'g':
            case 'G': { /* read a float */
                    float*val = suppress ? NULL : va_arg(ap, float*);
                    float cur = 0;
		    int negative = 0;
                    /* skip initial whitespace */
                    while ((nch!=MSVCRT_EOF) && isspace(nch))
                        nch = _getch();
		    /* get sign. */
                    if (nch == '-' || nch == '+') {
			negative = (nch=='-');
			if (width>0) width--;
			if (width==0) break;
                        nch = _getch();
                    }
		    /* get first digit. */
		    if (!isdigit(nch)) break;
		    cur = (nch - '0') * (negative ? -1 : 1);
                    nch = _getch();
		    if (width>0) width--;
                    /* read until no more digits */
                    while (width!=0 && (nch!=MSVCRT_EOF) && isdigit(nch)) {
                        cur = cur*10 + (nch - '0');
                        nch = _getch();
			if (width>0) width--;
                    }
		    /* handle decimals */
                    if (width!=0 && nch == '.') {
                        float dec = 1;
                        nch = _getch();
			if (width>0) width--;
                        while (width!=0 && (nch!=MSVCRT_EOF) && isdigit(nch)) {
                            dec /= 10;
                            cur += dec * (nch - '0');
                            nch = _getch();
			    if (width>0) width--;
                        }
                    }
		    /* handle exponent */
		    if (width!=0 && (nch == 'e' || nch == 'E')) {
			int exponent = 0, negexp = 0;
			float expcnt;
                        nch = _getch();
			if (width>0) width--;
			/* possible sign on the exponent */
			if (width!=0 && (nch=='+' || nch=='-')) {
			    negexp = (nch=='-');
                            nch = _getch();
			    if (width>0) width--;
			}
			/* exponent digits */
			while (width!=0 && (nch!=MSVCRT_EOF) && isdigit(nch)) {
			    exponent *= 10;
			    exponent += (nch - '0');
                            nch = _getch();
			    if (width>0) width--;
                        }
			/* update 'cur' with this exponent. */
			expcnt =  negexp ? .1 : 10;
			while (exponent!=0) {
			    if (exponent&1)
				cur*=expcnt;
			    exponent/=2;
			    expcnt=expcnt*expcnt;
			}
		    }
                    st = 1;
                    if (!suppress) *val = cur;
                }
                break;
            case 's': { /* read a word */
                    char*str = suppress ? NULL : va_arg(ap, char*);
                    char*sptr = str;
                    /* skip initial whitespace */
                    while ((nch!=MSVCRT_EOF) && isspace(nch))
                        nch = _getch();
                    /* read until whitespace */
                    while (width!=0 && (nch!=MSVCRT_EOF) && !isspace(nch)) {
                        if (!suppress) *sptr++ = nch;
			st++;
                        nch = _getch();
			if (width>0) width--;
                    }
                    /* terminate */
                    if (!suppress) *sptr = 0;
                    TRACE("read word: %s\n", str);
                }
                break;
            default: FIXME("unhandled: %%%c\n", *format);
		/* From spec: "if a percent sign is followed by a character
		 * that has no meaning as a format-control character, that
		 * character and the following characters are treated as
		 * an ordinary sequence of characters, that is, a sequence
		 * of characters that must match the input.  For example,
		 * to specify that a percent-sign character is to be input,
		 * use %%."
		 * LEAVING AS-IS because we catch bugs better that way. */
            }
            if (st && !suppress) rd++;
            else break;
        }
	/* a non-white-space character causes scanf to read, but not store,
	 * a matching non-white-space character. */
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
