/*
 * Debugging functions
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "debugtools.h"
#include "thread.h"
#include "winbase.h"
#include "winnt.h"
#include "wtypes.h"

DECLARE_DEBUG_CHANNEL(tid);

/* ---------------------------------------------------------------------- */

struct debug_info
{
    char *str_pos;       /* current position in strings buffer */
    char *out_pos;       /* current position in output buffer */
    char  strings[1024]; /* buffer for temporary strings */
    char  output[1024];  /* current output line */
};

static struct debug_info tmp;

/* get the debug info pointer for the current thread */
static inline struct debug_info *get_info(void)
{
    struct debug_info *info = NtCurrentTeb()->debug_info;
    if (!info)
    {
        if (!tmp.str_pos)
        {
            tmp.str_pos = tmp.strings;
            tmp.out_pos = tmp.output;
        }
        if (!GetProcessHeap()) return &tmp;
        /* setup the temp structure in case HeapAlloc wants to print something */
        NtCurrentTeb()->debug_info = &tmp;
        info = HeapAlloc( GetProcessHeap(), 0, sizeof(*info) );
        info->str_pos = info->strings;
        info->out_pos = info->output;
        NtCurrentTeb()->debug_info = info;
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

/***********************************************************************
 *		wine_dbgstr_an
 */
const char *wine_dbgstr_an( const char *src, int n )
{
    char *dst, *res;

    if (!HIWORD(src))
    {
        if (!src) return "(null)";
        res = gimme1(6);
        sprintf(res, "#%04x", LOWORD(src) );
        return res;
    }
    if (n < 0) n = 0;
    dst = res = gimme1 (n * 4 + 6);
    *dst++ = '"';
    while (n-- > 0 && *src)
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

/***********************************************************************
 *		wine_dbgstr_wn
 */
const char *wine_dbgstr_wn( const WCHAR *src, int n )
{
    char *dst, *res;

    if (!HIWORD(src))
    {
        if (!src) return "(null)";
        res = gimme1(6);
        sprintf(res, "#%04x", LOWORD(src) );
        return res;
    }
    if (n < 0) n = 0;
    dst = res = gimme1 (n * 5 + 7);
    *dst++ = 'L';
    *dst++ = '"';
    while (n-- > 0 && *src)
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
 *		wine_dbgstr_guid
 */
const char *wine_dbgstr_guid( const GUID *id )
{
    char *str;

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

/***********************************************************************
 *		wine_dbg_vprintf
 */
int wine_dbg_vprintf( const char *format, va_list args )
{
    struct debug_info *info = get_info();
    char *p;

    int ret = vsnprintf( info->out_pos, sizeof(info->output) - (info->out_pos - info->output),
                         format, args );

    /* make sure we didn't exceed the buffer length
     * the two asserts are due to glibc changes in vsnprintfs return value */
    assert( ret != -1 );
    assert( ret < sizeof(info->output) - (info->out_pos - info->output) );

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
 *		wine_dbg_printf
 */
int wine_dbg_printf(const char *format, ...)
{
    int ret;
    va_list valist;

    va_start(valist, format);
    ret = wine_dbg_vprintf( format, valist );
    va_end(valist);
    return ret;
}

/***********************************************************************
 *		wine_dbg_log
 */
int wine_dbg_log(enum __DEBUG_CLASS cls, const char *channel,
                 const char *function, const char *format, ... )
{
    static const char *classes[__DBCL_COUNT] = { "fixme", "err", "warn", "trace" };
    va_list valist;
    int ret = 0;

    va_start(valist, format);
    if (TRACE_ON(tid))
        ret = wine_dbg_printf( "%08lx:", (DWORD)NtCurrentTeb()->tid );
    if (cls < __DBCL_COUNT)
        ret += wine_dbg_printf( "%s:%s:%s ", classes[cls], channel + 1, function );
    if (format)
        ret += wine_dbg_vprintf( format, valist );
    va_end(valist);
    return ret;
}
