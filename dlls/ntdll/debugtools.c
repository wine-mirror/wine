/*
 * Debugging functions
 *
 * Copyright 2000 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <ctype.h>

#include "wine/debug.h"
#include "wine/exception.h"
#include "wine/library.h"
#include "wine/unicode.h"
#include "thread.h"
#include "winbase.h"
#include "winnt.h"
#include "winternl.h"
#include "excpt.h"

WINE_DECLARE_DEBUG_CHANNEL(tid);

/* ---------------------------------------------------------------------- */

struct debug_info
{
    char *str_pos;       /* current position in strings buffer */
    char *out_pos;       /* current position in output buffer */
    char  strings[1024]; /* buffer for temporary strings */
    char  output[1024];  /* current output line */
};

static struct debug_info initial_thread_info;  /* debug info for initial thread */

/* filter for page-fault exceptions */
static WINE_EXCEPTION_FILTER(page_fault)
{
    if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

/* get the debug info pointer for the current thread */
static inline struct debug_info *get_info(void)
{
    struct debug_info *info = NtCurrentTeb()->debug_info;

    if (!info) NtCurrentTeb()->debug_info = info = &initial_thread_info;
    if (!info->str_pos)
    {
        info->str_pos = info->strings;
        info->out_pos = info->output;
    }
    return info;
}

/* allocate some tmp space for a string */
static void *gimme1(int n)
{
    struct debug_info *info = get_info();
    char *res = info->str_pos;

    if (res + n >= &info->strings[sizeof(info->strings)]) res = info->strings;
    info->str_pos = res + n;
    return res;
}

/* release extra space that we requested in gimme1() */
static inline void release( void *ptr )
{
    struct debug_info *info = NtCurrentTeb()->debug_info;
    info->str_pos = ptr;
}

/* put an ASCII string into the debug buffer */
inline static char *put_string_a( const char *src, int n )
{
    char *dst, *res;

    if (n == -1) n = strlen(src);
    if (n < 0) n = 0;
    else if (n > 80) n = 80;
    dst = res = gimme1 (n * 4 + 6);
    *dst++ = '"';
    while (n-- > 0)
    {
        unsigned char c = *src++;
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
    *dst++ = '\0';
    release( dst );
    return res;
}

/* put a Unicode string into the debug buffer */
inline static char *put_string_w( const WCHAR *src, int n )
{
    char *dst, *res;

    if (n == -1) n = strlenW(src);
    if (n < 0) n = 0;
    else if (n > 80) n = 80;
    dst = res = gimme1 (n * 5 + 7);
    *dst++ = 'L';
    *dst++ = '"';
    while (n-- > 0)
    {
        WCHAR c = *src++;
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
    *dst++ = '\0';
    release( dst );
    return res;
}

/***********************************************************************
 *		NTDLL_dbgstr_an
 */
static const char *NTDLL_dbgstr_an( const char *src, int n )
{
    char *res, *old_pos;
    struct debug_info *info = get_info();

    if (!HIWORD(src))
    {
        if (!src) return "(null)";
        res = gimme1(6);
        sprintf(res, "#%04x", LOWORD(src) );
        return res;
    }
    /* save current position to restore it on exception */
    old_pos = info->str_pos;
    __TRY
    {
        res = put_string_a( src, n );
    }
    __EXCEPT(page_fault)
    {
        release( old_pos );
        return "(invalid)";
    }
    __ENDTRY
    return res;
}

/***********************************************************************
 *		NTDLL_dbgstr_wn
 */
static const char *NTDLL_dbgstr_wn( const WCHAR *src, int n )
{
    char *res, *old_pos;
    struct debug_info *info = get_info();

    if (!HIWORD(src))
    {
        if (!src) return "(null)";
        res = gimme1(6);
        sprintf(res, "#%04x", LOWORD(src) );
        return res;
    }

    /* save current position to restore it on exception */
    old_pos = info->str_pos;
    __TRY
    {
        res = put_string_w( src, n );
    }
    __EXCEPT(page_fault)
    {
        release( old_pos );
        return "(invalid)";
    }
    __ENDTRY
     return res;
}

/***********************************************************************
 *		NTDLL_dbg_vsprintf
 */
static const char *NTDLL_dbg_vsprintf( const char *format, va_list args )
{
    static const int max_size = 200;

    char *res = gimme1( max_size );
    int len = vsnprintf( res, max_size, format, args );
    if (len == -1 || len >= max_size) res[max_size-1] = 0;
    else release( res + len + 1 );
    return res;
}

/***********************************************************************
 *		NTDLL_dbg_vprintf
 */
static int NTDLL_dbg_vprintf( const char *format, va_list args )
{
    struct debug_info *info = get_info();
    char *p;

    int ret = vsnprintf( info->out_pos, sizeof(info->output) - (info->out_pos - info->output),
                         format, args );

    /* make sure we didn't exceed the buffer length
     * the two checks are due to glibc changes in vsnprintfs return value
     * the buffer size can be exceeded in case of a missing \n in
     * debug output */
    if ((ret == -1) || (ret >= sizeof(info->output) - (info->out_pos - info->output)))
    {
       fprintf( stderr, "wine_dbg_vprintf: debugstr buffer overflow (contents: '%s')\n",
                info->output);
       info->out_pos = info->output;
       abort();
    }

    p = strrchr( info->out_pos, '\n' );
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

/***********************************************************************
 *		NTDLL_dbg_vlog
 */
static int NTDLL_dbg_vlog( unsigned int cls, const char *channel,
                           const char *function, const char *format, va_list args )
{
    static const char * const classes[] = { "fixme", "err", "warn", "trace" };
    struct debug_info *info = get_info();
    int ret = 0;

    /* only print header if we are at the beginning of the line */
    if (info->out_pos == info->output || info->out_pos[-1] == '\n')
    {
        if (TRACE_ON(tid))
            ret = wine_dbg_printf( "%04lx:", NtCurrentTeb()->tid );
        if (cls < sizeof(classes)/sizeof(classes[0]))
            ret += wine_dbg_printf( "%s:%s:%s ", classes[cls], channel + 1, function );
    }
    if (format)
        ret += NTDLL_dbg_vprintf( format, args );
    return ret;
}

/***********************************************************************
 *		debug_init
 */
DECL_GLOBAL_CONSTRUCTOR(debug_init)
{
    __wine_dbgstr_an    = NTDLL_dbgstr_an;
    __wine_dbgstr_wn    = NTDLL_dbgstr_wn;
    __wine_dbg_vsprintf = NTDLL_dbg_vsprintf;
    __wine_dbg_vprintf  = NTDLL_dbg_vprintf;
    __wine_dbg_vlog     = NTDLL_dbg_vlog;
}
