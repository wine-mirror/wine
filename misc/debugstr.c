#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "debugstr.h"
#include "debugtools.h"
#include "wtypes.h"
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
  dst = res = gimme1 (n * 4 + 6);
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
  *dst++ = '"';
  if (*src)
    {
      *dst++ = '.';
      *dst++ = '.';
      *dst++ = '.';
    }
  *dst = '\0';
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
  dst = res = gimme1 (n * 5 + 7);
  *dst++ = 'L';
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
              sprintf(dst,"%04x",c);
              dst+=4;
	    }
	}
    }
  *dst++ = '"';
  if (*src)
    {
      *dst++ = '.';
      *dst++ = '.';
      *dst++ = '.';
    }
  *dst = '\0';
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

LPSTR debugstr_guid( const GUID *id )
{
    LPSTR str;

    if (!id) return "(null)";
    if (!HIWORD(id))
    {
        str = gimme1(12);
        sprintf( str, "<guid-0x%04x>", LOWORD(id) );
    }
    else
    {
        str = gimme1(40);
        sprintf( str, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                 id->Data1, id->Data2, id->Data3,
                 id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
                 id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7] );
    }
    return str;
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



/*--< Function >---------------------------------------------------------
**  
**              debugstr_hex_dump
**              
**  Description:
**      This function creates a hex dump, with a readable ascii
**  section, for displaying memory.
**  
**  Parameters:
**      1.  ptr             Pointer to memory
**      2.  len             How much to dump.
**
**  Returns:
**    Temporarily allocated buffer, with the hex dump in it.
**  Don't rely on this pointer being around for very long, just
**  long enough to use it in a TRACE statement; e.g.:
**  TRACE("struct dump is \n%s", debugstr_hex_dump(&x, sizeof(x)));
**          
**-------------------------------------------------------------------------*/
LPSTR 
debugstr_hex_dump (const void *ptr, int len)
{
    /* Locals */
    char          dumpbuf[59];
    char          charbuf[20];
    char          tempbuf[8];
    const char    *p;
    int           i;
    unsigned int  nosign;
    LPSTR         dst;
    LPSTR         outptr;

/* Begin function dbg_hex_dump */

    /*-----------------------------------------------------------------------
    **  Allocate an output buffer
    **      A reasonable value is one line overhand (80 chars), and
    **      then one line (80) for every 16 bytes.
    **---------------------------------------------------------------------*/
    outptr = dst = gimme1 ((len * (80 / 16)) + 80);

    /*-----------------------------------------------------------------------
    **  Loop throught the input buffer, one character at a time
    **---------------------------------------------------------------------*/
    for (i = 0, p = ptr; (i < len); i++, p++)
    {

        /*-------------------------------------------------------------------
        **  If we're just starting a line, 
        **      we need to possibly flush the old line, and then
        **      intialize the line buffer.
        **-----------------------------------------------------------------*/
        if ((i % 16) == 0)
        {
            if (i)
            {
                sprintf(outptr, "  %-43.43s   %-16.16s\n", dumpbuf, charbuf);
                outptr += strlen(outptr);
            }
            sprintf (dumpbuf, "%04x: ", i);
            strcpy (charbuf, "");
        }

        /*-------------------------------------------------------------------
        **  Add the current data byte to the dump section.
        **-----------------------------------------------------------------*/
        nosign = (unsigned char) *p;
        sprintf (tempbuf, "%02X", nosign);

        /*-------------------------------------------------------------------
        **  If we're two DWORDS through, add a hyphen for readability,
        **      if it's a DWORD boundary, add a space for more
        **      readability.
        **-----------------------------------------------------------------*/
        if ((i % 16) == 7)
            strcat(tempbuf, " - ");
        else if ( (i % 4) == 3)
            strcat(tempbuf, " ");
        strcat (dumpbuf, tempbuf);

        /*-------------------------------------------------------------------
        **  Add the current byte to the character display part of the
        **      hex dump
        **-----------------------------------------------------------------*/
        sprintf (tempbuf, "%c", isprint(*p) ? *p : '.');
        strcat (charbuf, tempbuf);
    }

    /*-----------------------------------------------------------------------
    **  Flush the last line, if any
    **---------------------------------------------------------------------*/
    if (i > 0)
    {
        sprintf(outptr, "  %-43.43s   %-16.16s\n", dumpbuf, charbuf);
        outptr += strlen(outptr);
    }

    return(dst);
} /* End function dbg_hex_dump */



