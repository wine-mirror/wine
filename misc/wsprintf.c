/*
 * wsprintf functions
 *
 * Copyright 1996 Alexandre Julliard
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "windows.h"
#include "ldt.h"
#include "stackframe.h"

#define WPRINTF_LEFTALIGN   0x0001  /* Align output on the left ('-' prefix) */
#define WPRINTF_PREFIX_HEX  0x0002  /* Prefix hex with 0x ('#' prefix) */
#define WPRINTF_ZEROPAD     0x0004  /* Pad with zeros ('0' prefix) */
#define WPRINTF_LONG        0x0008  /* Long arg ('l' prefix) */
#define WPRINTF_SHORT       0x0010  /* Short arg ('h' prefix) */
#define WPRINTF_UPPER_HEX   0x0020  /* Upper-case hex ('X' specifier) */

typedef enum
{
    WPR_CHAR,
    WPR_WCHAR,
    WPR_STRING,
    WPR_WSTRING,
    WPR_SIGNED,
    WPR_UNSIGNED,
    WPR_HEXA
} WPRINTF_TYPE;

typedef struct
{
    UINT32         flags;
    UINT32         width;
    UINT32         precision;
    WPRINTF_TYPE   type;
} WPRINTF_FORMAT;


/***********************************************************************
 *           WPRINTF_ParseFormatA
 *
 * Parse a format specification. A format specification has the form:
 *
 * [-][#][0][width][.precision]type
 *
 * Return value is the length of the format specification in characters.
 */
static INT32 WPRINTF_ParseFormatA( LPCSTR format, WPRINTF_FORMAT *res )
{
    LPCSTR p = format;

    res->flags = 0;
    res->width = 0;
    res->precision = 0;
    if (*p == '-') { res->flags |= WPRINTF_LEFTALIGN; p++; }
    if (*p == '#') { res->flags |= WPRINTF_PREFIX_HEX; p++; }
    if (*p == '0') { res->flags |= WPRINTF_ZEROPAD; p++; }
    while ((*p >= '0') && (*p <= '9'))  /* width field */
    {
        res->width = res->width * 10 + *p - '0';
        p++;
    }
    if (*p == '.')  /* precision field */
    {
        p++;
        while ((*p >= '0') && (*p <= '9'))
        {
            res->precision = res->precision * 10 + *p - '0';
            p++;
        }
    }
    if (*p == 'l') { res->flags |= WPRINTF_LONG; p++; }
    else if (*p == 'h') { res->flags |= WPRINTF_SHORT; p++; }
    switch(*p)
    {
    case 'c':
        res->type = (res->flags & WPRINTF_LONG) ? WPR_WCHAR : WPR_CHAR;
        break;
    case 'C':
        res->type = (res->flags & WPRINTF_SHORT) ? WPR_CHAR : WPR_WCHAR;
        break;
    case 'd':
    case 'i':
        res->type = WPR_SIGNED;
        break;
    case 's':
        res->type = (res->flags & WPRINTF_LONG) ? WPR_WSTRING : WPR_STRING;
        break;
    case 'S':
        res->type = (res->flags & WPRINTF_SHORT) ? WPR_STRING : WPR_WSTRING;
        break;
    case 'u':
        res->type = WPR_UNSIGNED;
        break;
    case 'X':
        res->flags |= WPRINTF_UPPER_HEX;
        /* fall through */
    case 'x':
        res->type = WPR_HEXA;
        break;
    default:
        fprintf( stderr, "wvsprintf32A: unknown format '%c'\n", *p );
        break;
    }
    return (INT32)(p - format) + 1;
}


/***********************************************************************
 *           WPRINTF_ParseFormatW
 *
 * Parse a format specification. A format specification has the form:
 *
 * [-][#][0][width][.precision]type
 *
 * Return value is the length of the format specification in characters.
 */
static INT32 WPRINTF_ParseFormatW( LPCWSTR format, WPRINTF_FORMAT *res )
{
    LPCWSTR p = format;

    res->flags = 0;
    res->width = 0;
    res->precision = 0;
    if (*p == '-') { res->flags |= WPRINTF_LEFTALIGN; p++; }
    if (*p == '#') { res->flags |= WPRINTF_PREFIX_HEX; p++; }
    if (*p == '0') { res->flags |= WPRINTF_ZEROPAD; p++; }
    while ((*p >= '0') && (*p <= '9'))  /* width field */
    {
        res->width = res->width * 10 + *p - '0';
        p++;
    }
    if (*p == '.')  /* precision field */
    {
        p++;
        while ((*p >= '0') && (*p <= '9'))
        {
            res->precision = res->precision * 10 + *p - '0';
            p++;
        }
    }
    if (*p == 'l') { res->flags |= WPRINTF_LONG; p++; }
    else if (*p == 'h') { res->flags |= WPRINTF_SHORT; p++; }
    switch((CHAR)*p)
    {
    case 'c':
        res->type = (res->flags & WPRINTF_SHORT) ? WPR_CHAR : WPR_WCHAR;
        break;
    case 'C':
        res->type = (res->flags & WPRINTF_LONG) ? WPR_WCHAR : WPR_CHAR;
        break;
    case 'd':
    case 'i':
        res->type = WPR_SIGNED;
        break;
    case 's':
        res->type = (res->flags & WPRINTF_SHORT) ? WPR_STRING : WPR_WSTRING;
        break;
    case 'S':
        res->type = (res->flags & WPRINTF_LONG) ? WPR_WSTRING : WPR_STRING;
        break;
    case 'u':
        res->type = WPR_UNSIGNED;
        break;
    case 'X':
        res->flags |= WPRINTF_UPPER_HEX;
        /* fall through */
    case 'x':
        res->type = WPR_HEXA;
        break;
    default:
        fprintf( stderr, "wvsprintf32W: unknown format '%c'\n", (CHAR)*p );
        break;
    }
    return (INT32)(p - format) + 1;
}


/***********************************************************************
 *           WPRINTF_GetLen
 */
static UINT32 WPRINTF_GetLen( WPRINTF_FORMAT *format, LPCVOID arg,
                              LPSTR number, UINT32 maxlen )
{
    UINT32 len;

    if (format->flags & WPRINTF_LEFTALIGN) format->flags &= ~WPRINTF_ZEROPAD;
    if (format->width > maxlen) format->width = maxlen;
    switch(format->type)
    {
    case WPR_CHAR:
    case WPR_WCHAR:
        return (format->precision = 1);
    case WPR_STRING:
        for (len = 0; !format->precision || (len < format->precision); len++)
            if (!*(*(LPCSTR *)arg + len)) break;
        if (len > maxlen) len = maxlen;
        return (format->precision = len);
    case WPR_WSTRING:
        for (len = 0; !format->precision || (len < format->precision); len++)
            if (!*(*(LPCWSTR *)arg + len)) break;
        if (len > maxlen) len = maxlen;
        return (format->precision = len);
    case WPR_SIGNED:
        len = sprintf( number, "%d", *(INT32 *)arg );
        break;
    case WPR_UNSIGNED:
        len = sprintf( number, "%u", *(UINT32 *)arg );
        break;
    case WPR_HEXA:
        len = sprintf( number,
                        (format->flags & WPRINTF_UPPER_HEX) ? "%X" : "%x",
                        *(UINT32 *)arg );
        if (format->flags & WPRINTF_PREFIX_HEX) len += 2;
        break;
    default:
        return 0;
    }
    if (len > maxlen) len = maxlen;
    if (format->precision < len) format->precision = len;
    if (format->precision > maxlen) format->precision = maxlen;
    if ((format->flags & WPRINTF_ZEROPAD) && (format->width > format->precision))
        format->precision = format->width;
    return len;
}


/***********************************************************************
 *           wvsnprintf16   (Not a Windows API)
 */
INT16 wvsnprintf16( LPSTR buffer, UINT16 maxlen, LPCSTR spec, LPCVOID args )
{
    WPRINTF_FORMAT format;
    LPSTR p = buffer;
    UINT32 i, len;
    CHAR number[20];
    DWORD cur_arg;

    while (*spec && (maxlen > 1))
    {
        if (*spec != '%') { *p++ = *spec++; maxlen--; continue; }
        spec++;
        if (*spec == '%') { *p++ = *spec++; maxlen--; continue; }
        spec += WPRINTF_ParseFormatA( spec, &format );
        switch(format.type)
        {
        case WPR_WCHAR:  /* No Unicode in Win16 */
        case WPR_CHAR:
            cur_arg = (DWORD)*(CHAR *)args;
            args = (WORD *)args + 1;
            break;
        case WPR_WSTRING:  /* No Unicode in Win16 */
        case WPR_STRING:
            if (IsBadReadPtr16( *(SEGPTR *)args, 1 )) cur_arg = (DWORD)"";
            else cur_arg = (DWORD)PTR_SEG_TO_LIN( *(SEGPTR *)args );
            args = (SEGPTR *)args + 1;
            break;
        case WPR_HEXA:
        case WPR_SIGNED:
        case WPR_UNSIGNED:
            if (format.flags & WPRINTF_LONG)
            {
                cur_arg = (DWORD)*(UINT32 *)args;
                args = (UINT32 *)args + 1;
            }
            else
            {
                cur_arg = (DWORD)*(UINT16 *)args;
                args = (UINT16 *)args + 1;
            }
            break;
        }
        len = WPRINTF_GetLen( &format, &cur_arg, number, maxlen - 1 );
        if (!(format.flags & WPRINTF_LEFTALIGN))
            for (i = format.precision; i < format.width; i++, maxlen--)
                *p++ = ' ';
        switch(format.type)
        {
        case WPR_CHAR:
            if ((*p = (CHAR)cur_arg)) p++;
            else if (format.width > 1) *p++ = ' ';
            else len = 0;
            break;
        case WPR_STRING:
            if (len) memcpy( p, (LPCSTR)cur_arg, len );
            p += len;
            break;
        case WPR_HEXA:
            if ((format.flags & WPRINTF_PREFIX_HEX) && (maxlen > 3))
            {
                *p++ = '0';
                *p++ = (format.flags & WPRINTF_UPPER_HEX) ? 'X' : 'x';
                maxlen -= 2;
                len -= 2;
                format.precision -= 2;
                format.width -= 2;
            }
            /* fall through */
        case WPR_SIGNED:
        case WPR_UNSIGNED:
            for (i = len; i < format.precision; i++, maxlen--) *p++ = '0';
            if (len) memcpy( p, number, len );
            p += len;
            break;
        case WPR_WCHAR:
        case WPR_WSTRING:
            fprintf( stderr, "Unicode not supported in wsprintf16\n" );
            break;
        }
        if (format.flags & WPRINTF_LEFTALIGN)
            for (i = format.precision; i < format.width; i++, maxlen--)
                *p++ = ' ';
        maxlen -= len;
    }
    *p = 0;
    return (maxlen > 1) ? (INT32)(p - buffer) : -1;
}


/***********************************************************************
 *           wvsnprintf32A   (Not a Windows API)
 */
INT32 wvsnprintf32A( LPSTR buffer, UINT32 maxlen, LPCSTR spec, LPCVOID args )
{
    WPRINTF_FORMAT format;
    LPSTR p = buffer;
    UINT32 i, len;
    CHAR number[20];

    while (*spec && (maxlen > 1))
    {
        if (*spec != '%') { *p++ = *spec++; maxlen--; continue; }
        spec++;
        if (*spec == '%') { *p++ = *spec++; maxlen--; continue; }
        spec += WPRINTF_ParseFormatA( spec, &format );
        len = WPRINTF_GetLen( &format, args, number, maxlen - 1 );
        if (!(format.flags & WPRINTF_LEFTALIGN))
            for (i = format.precision; i < format.width; i++, maxlen--)
                *p++ = ' ';
        switch(format.type)
        {
        case WPR_WCHAR:
            if ((*p = (CHAR)*(WCHAR *)args)) p++;
            else if (format.width > 1) *p++ = ' ';
            else len = 0;
            break;
        case WPR_CHAR:
            if ((*p = *(CHAR *)args)) p++;
            else if (format.width > 1) *p++ = ' ';
            else len = 0;
            break;
        case WPR_STRING:
            if (len) memcpy( p, *(LPCSTR *)args, len );
            p += len;
            break;
        case WPR_WSTRING:
            for (i = 0; i < len; i++) *p++ = (CHAR)*(*(LPCWSTR *)args + i);
            break;
        case WPR_HEXA:
            if ((format.flags & WPRINTF_PREFIX_HEX) && (maxlen > 3))
            {
                *p++ = '0';
                *p++ = (format.flags & WPRINTF_UPPER_HEX) ? 'X' : 'x';
                maxlen -= 2;
                len -= 2;
                format.precision -= 2;
                format.width -= 2;
            }
            /* fall through */
        case WPR_SIGNED:
        case WPR_UNSIGNED:
            for (i = len; i < format.precision; i++, maxlen--) *p++ = '0';
            if (len) memcpy( p, number, len );
            p += len;
            break;
        }
        if (format.flags & WPRINTF_LEFTALIGN)
            for (i = format.precision; i < format.width; i++, maxlen--)
                *p++ = ' ';
        args = (INT32 *)args + 1;
        maxlen -= len;
    }
    *p = 0;
    return (maxlen > 1) ? (INT32)(p - buffer) : -1;
}


/***********************************************************************
 *           wvsnprintf32W   (Not a Windows API)
 */
INT32 wvsnprintf32W( LPWSTR buffer, UINT32 maxlen, LPCWSTR spec, LPCVOID args )
{
    WPRINTF_FORMAT format;
    LPWSTR p = buffer;
    UINT32 i, len;
    CHAR number[20];

    while (*spec && (maxlen > 1))
    {
        if (*spec != '%') { *p++ = *spec++; maxlen--; continue; }
        spec++;
        if (*spec == '%') { *p++ = *spec++; maxlen--; continue; }
        spec += WPRINTF_ParseFormatW( spec, &format );
        len = WPRINTF_GetLen( &format, args, number, maxlen - 1 );
        if (!(format.flags & WPRINTF_LEFTALIGN))
            for (i = format.precision; i < format.width; i++, maxlen--)
                *p++ = ' ';
        switch(format.type)
        {
        case WPR_WCHAR:
            if ((*p = *(WCHAR *)args)) p++;
            else if (format.width > 1) *p++ = ' ';
            else len = 0;
            break;
        case WPR_CHAR:
            if ((*p = (WCHAR)*(CHAR *)args)) p++;
            else if (format.width > 1) *p++ = ' ';
            else len = 0;
            break;
        case WPR_STRING:
            for (i = 0; i < len; i++) *p++ = (WCHAR)*(*(LPCSTR *)args + i);
            break;
        case WPR_WSTRING:
            if (len) memcpy( p, *(LPCWSTR *)args, len * sizeof(WCHAR) );
            p += len;
            break;
        case WPR_HEXA:
            if ((format.flags & WPRINTF_PREFIX_HEX) && (maxlen > 3))
            {
                *p++ = '0';
                *p++ = (format.flags & WPRINTF_UPPER_HEX) ? 'X' : 'x';
                maxlen -= 2;
                len -= 2;
                format.precision -= 2;
                format.width -= 2;
            }
            /* fall through */
        case WPR_SIGNED:
        case WPR_UNSIGNED:
            for (i = len; i < format.precision; i++, maxlen--) *p++ = '0';
            for (i = 0; i < len; i++) *p++ = (WCHAR)number[i];
            break;
        }
        if (format.flags & WPRINTF_LEFTALIGN)
            for (i = format.precision; i < format.width; i++, maxlen--)
                *p++ = ' ';
        args = (INT32 *)args + 1;
        maxlen -= len;
    }
    *p = 0;
    return (maxlen > 1) ? (INT32)(p - buffer) : -1;
}


/***********************************************************************
 *           wvsprintf16   (USER.421)
 */
INT16 wvsprintf16( LPSTR buffer, LPCSTR spec, LPCVOID args )
{
    return wvsnprintf16( buffer, 0xffff, spec, args );
}


/***********************************************************************
 *           wvsprintf32A   (USER32.586)
 */
INT32 wvsprintf32A( LPSTR buffer, LPCSTR spec, LPCVOID args )
{
    return wvsnprintf32A( buffer, 0xffffffff, spec, args );
}


/***********************************************************************
 *           wvsprintf32W   (USER32.587)
 */
INT32 wvsprintf32W( LPWSTR buffer, LPCWSTR spec, LPCVOID args )
{
    return wvsnprintf32W( buffer, 0xffffffff, spec, args );
}


/***********************************************************************
 *           wsprintf16   (USER.420)
 */
/* Winelib version */
INT16 wsprintf16( LPSTR buffer, LPCSTR spec, ... )
{
    va_list valist;
    INT16 res;

    va_start( valist, spec );
    /* Note: we call the 32-bit version, because the args are 32-bit */
    res = (INT16)wvsprintf32A( buffer, spec, (LPCVOID)valist );
    va_end( valist );
    return res;
}

/* Emulator version */
INT16 WIN16_wsprintf16(void)
{
    SEGPTR *win_stack = (SEGPTR *)CURRENT_STACK16->args;
    LPSTR buffer = (LPSTR)PTR_SEG_TO_LIN(win_stack[0]);
    LPCSTR spec  = (LPCSTR)PTR_SEG_TO_LIN(win_stack[1]);
    return wvsprintf16( buffer, spec, &win_stack[2] );

}


/***********************************************************************
 *           wsprintf32A   (USER32.584)
 */
/* Winelib version */
INT32 wsprintf32A( LPSTR buffer, LPCSTR spec, ... )
{
    va_list valist;
    INT32 res;

    va_start( valist, spec );
    res = wvsprintf32A( buffer, spec, (LPCVOID)valist );
    va_end( valist );
    return res;
}

/* Emulator version */
INT32 WIN32_wsprintf32A( int *args )
{
    return wvsprintf32A( (LPSTR)args[0], (LPCSTR)args[1], (LPCVOID)&args[2] );
}


/***********************************************************************
 *           wsprintf32W   (USER32.585)
 */
/* Winelib version */
INT32 wsprintf32W( LPWSTR buffer, LPCWSTR spec, ... )
{
    va_list valist;
    INT32 res;

    va_start( valist, spec );
    res = wvsprintf32W( buffer, spec, (LPCVOID)valist );
    va_end( valist );
    return res;
}

/* Emulator version */
INT32 WIN32_wsprintf32W( int *args )
{
    return wvsprintf32W( (LPWSTR)args[0], (LPCWSTR)args[1], (LPCVOID)&args[2]);
}
