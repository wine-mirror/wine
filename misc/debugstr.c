#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "debugstr.h"
#include "debugtools.h"
#include "xmalloc.h"

/* ---------------------------------------------------------------------- */

#define SAVE_STRING_COUNT 50
static void *strings[SAVE_STRING_COUNT];
static int nextstring;

/* ---------------------------------------------------------------------- */

static void *
gimme1 (int n)
{
  void *res;
  if (strings[nextstring]) free (strings[nextstring]);
  res = strings[nextstring] = xmalloc (n);
  if (++nextstring == SAVE_STRING_COUNT) nextstring = 0;
  return res;
}

/* ---------------------------------------------------------------------- */

LPSTR
debugstr_an (LPCSTR src, int n)
{
  LPSTR dst, res;

  if (!src) return "(null)";
  if (n < 0) n = 0;
  dst = res = gimme1 (n * 4 + 10);
  *dst++ = '"';
  while (n-- > 0 && *src)
    {
      BYTE c = *src++;
      switch (c)
	{
	case '\n': *dst++ = '\\'; *dst++ = 'n'; break;
	case '\r': *dst++ = '\\'; *dst++ = 'r'; break;
	case '\t': *dst++ = '\\'; *dst++ = 't'; break;
	case '"': *dst++ = '\\'; *dst++ = '"'; break;
	case '\\': *dst++ = '\\'; *dst++ = '\\'; break;
	default:
	  if (c >= ' ' && c <= 126)
	    *dst++ = c;
	  else
	    {
	      *dst++ = '\\';
	      *dst++ = '0' + ((c >> 6) & 7);
	      *dst++ = '0' + ((c >> 3) & 7);
	      *dst++ = '0' + ((c >> 0) & 7);
	    }
	}
    }
  if (*src)
    {
      *dst++ = '.';
      *dst++ = '.';
      *dst++ = '.';
    }
  *dst++ = '"';
  *dst = 0;
  return res;
}

/* ---------------------------------------------------------------------- */

LPSTR
debugstr_a (LPCSTR s)
{
  return debugstr_an (s, 80);
}

/* ---------------------------------------------------------------------- */

LPSTR
debugstr_wn (LPCWSTR src, int n)
{
  LPSTR dst, res;

  if (!src) return "(null)";
  if (n < 0) n = 0;
  dst = res = gimme1 (n * 4 + 10);
  *dst++ = '"';
  while (n-- > 0 && *src)
    {
      WORD c = *src++;
      switch (c)
	{
	case '\n': *dst++ = '\\'; *dst++ = 'n'; break;
	case '\r': *dst++ = '\\'; *dst++ = 'r'; break;
	case '\t': *dst++ = '\\'; *dst++ = 't'; break;
	case '"': *dst++ = '\\'; *dst++ = '"'; break;
	case '\\': *dst++ = '\\'; *dst++ = '\\'; break;
	default:
	  if (c >= ' ' && c <= 126)
	    *dst++ = c;
	  else
	    {
	      *dst++ = '\\';
	      *dst++ = '0' + ((c >> 6) & 7);
	      *dst++ = '0' + ((c >> 3) & 7);
	      *dst++ = '0' + ((c >> 0) & 7);
	    }
	}
    }
  if (*src)
    {
      *dst++ = '.';
      *dst++ = '.';
      *dst++ = '.';
    }
  *dst++ = '"';
  *dst = 0;
  return res;
}

/* ---------------------------------------------------------------------- */

LPSTR
debugstr_w (LPCWSTR s)
{
  return debugstr_wn (s, 80);
}

/* ---------------------------------------------------------------------- */
/* This routine returns a nicely formated name of the resource res
   If the resource name is a string, it will return '<res-name>'
   If it is a number, it will return #<4-digit-hex-number> */
LPSTR debugres_a( LPCSTR res )
{
    char resname[10];
    if (HIWORD(res)) return debugstr_a(res);
    sprintf(resname, "#%04x", LOWORD(res));
    return debugstr_a (resname);
}

LPSTR debugres_w( LPCWSTR res )
{
    char resname[10];
    if (HIWORD(res)) return debugstr_w(res);
    sprintf(resname, "#%04x", LOWORD(res));
    return debugstr_a (resname);
}

/* ---------------------------------------------------------------------- */

void debug_dumpstr (LPCSTR s)
{
  fputc ('"', stderr);
  while (*s)
    {
      switch (*s)
	{
	case '\\':
	case '"':
	  fputc ('\\', stderr);
	  fputc (*s, stderr);
	  break;
	case '\n':
	  fputc ('\\', stderr);
	  fputc ('n', stderr);
	  break;
	case '\r':
	  fputc ('\\', stderr);
	  fputc ('r', stderr);
	  break;
	case '\t':
	  fputc ('\\', stderr);
	  fputc ('t', stderr);
	  break;
	default:
	  if (*s<' ')
	    fprintf (stderr, "\\0x%02x", *s);
	  else
	    fputc (*s, stderr);
	}
      s++;
    }
  fputc ('"', stderr);
}

/* ---------------------------------------------------------------------- */

int dbg_printf(const char *format, ...)
{
    int ret;
    va_list valist;

    va_start(valist, format);
    ret = vfprintf(stderr, format, valist);
    va_end(valist);
    return ret;
}

