#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "debugtools.h"
#include "wtypes.h"
#include "thread.h"

/* ---------------------------------------------------------------------- */

struct debug_info
{
    char *str_pos;       /* current position in strings buffer */
    char *out_pos;       /* current position in output buffer */
    char  strings[500];  /* buffer for temporary strings */
    char  output[500];   /* current output line */
};

static struct debug_info tmp;

static inline struct debug_info *get_info(void)
{
    struct debug_info *info = NtCurrentTeb()->debug_info;
    if (!info)
    {
        /* setup the temp structure in case HeapAlloc wants to print something */
        NtCurrentTeb()->debug_info = &tmp;
        tmp.str_pos = tmp.strings;
        tmp.out_pos = tmp.output;
        info = HeapAlloc( GetProcessHeap(), 0, sizeof(*info) );
        info->str_pos = info->strings;
        info->out_pos = info->output;
        NtCurrentTeb()->debug_info = info;
    }
    return info;
}

/* ---------------------------------------------------------------------- */

static void *
gimme1 (int n)
{
    struct debug_info *info = get_info();
    char *res = info->str_pos;

    if (res + n >= &info->strings[sizeof(info->strings)]) res = info->strings;
    info->str_pos = res + n;
    return res;
}

/* ---------------------------------------------------------------------- */

LPCSTR debugstr_an (LPCSTR src, int n)
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

LPCSTR debugstr_wn (LPCWSTR src, int n)
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
/* This routine returns a nicely formated name of the resource res
   If the resource name is a string, it will return '<res-name>'
   If it is a number, it will return #<4-digit-hex-number> */
LPCSTR debugres_a( LPCSTR res )
{
    char *resname;
    if (HIWORD(res)) return debugstr_a(res);
    resname = gimme1(6);
    sprintf(resname, "#%04x", LOWORD(res) );
    return resname;
}

LPCSTR debugres_w( LPCWSTR res )
{
    char *resname;
    if (HIWORD(res)) return debugstr_w(res);
    resname = gimme1(6);
    sprintf( resname, "#%04x", LOWORD(res) );
    return resname;
}

/* ---------------------------------------------------------------------- */

LPCSTR debugstr_guid( const GUID *id )
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

int dbg_vprintf( const char *format, va_list args )
{
    struct debug_info *info = get_info();

    int ret = vsprintf( info->out_pos, format, args );
    char *p = strrchr( info->out_pos, '\n' );
    if (!p) info->out_pos += ret;
    else
    {
        char *pos = info->output;
        p++;
        write( 2, pos, p - pos );
        /* move beginning of next line to start of buffer */
        while ((*pos = *p++)) pos++;
        info->out_pos = pos;
    }
    return ret;
}

/* ---------------------------------------------------------------------- */

int dbg_printf(const char *format, ...)
{
    int ret;
    va_list valist;

    va_start(valist, format);
    ret = dbg_vprintf( format, valist);
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
LPCSTR debugstr_hex_dump (const void *ptr, int len)
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



