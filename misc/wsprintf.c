/*
 * wsprintf functions
 *
 * Copyright 1996 Alexandre Julliard
 */

#include <stdarg.h>
#include <string.h>
#include "wine/winbase16.h"
#include "winuser.h"
#include "ldt.h"
#include "stackframe.h"
#include "debug.h"


#define WPRINTF_LEFTALIGN   0x0001  /* Align output on the left ('-' prefix) */
#define WPRINTF_PREFIX_HEX  0x0002  /* Prefix hex with 0x ('#' prefix) */
#define WPRINTF_ZEROPAD     0x0004  /* Pad with zeros ('0' prefix) */
#define WPRINTF_LONG        0x0008  /* Long arg ('l' prefix) */
#define WPRINTF_SHORT       0x0010  /* Short arg ('h' prefix) */
#define WPRINTF_UPPER_HEX   0x0020  /* Upper-case hex ('X' specifier) */
#define WPRINTF_WIDE        0x0040  /* Wide arg ('w' prefix) */

typedef enum
{
    WPR_UNKNOWN,
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
    UINT         flags;
    UINT         width;
    UINT         precision;
    WPRINTF_TYPE   type;
} WPRINTF_FORMAT;

typedef union {
    WCHAR   wchar_view;
    CHAR    char_view;
    LPCSTR  lpcstr_view;
    LPCWSTR lpcwstr_view;
    INT     int_view;
} WPRINTF_DATA;

static const CHAR null_stringA[] = "(null)";
static const WCHAR null_stringW[] = { '(', 'n', 'u', 'l', 'l', ')', 0 };

/***********************************************************************
 *           WPRINTF_ParseFormatA
 *
 * Parse a format specification. A format specification has the form:
 *
 * [-][#][0][width][.precision]type
 *
 * Return value is the length of the format specification in characters.
 */
static INT WPRINTF_ParseFormatA( LPCSTR format, WPRINTF_FORMAT *res )
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
    else if (*p == 'w') { res->flags |= WPRINTF_WIDE; p++; }
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
        res->type = (res->flags & (WPRINTF_LONG |WPRINTF_WIDE)) 
	            ? WPR_WSTRING : WPR_STRING;
        break;
    case 'S':
        res->type = (res->flags & (WPRINTF_SHORT|WPRINTF_WIDE))
	            ? WPR_STRING : WPR_WSTRING;
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
    default: /* unknown format char */
        res->type = WPR_UNKNOWN;
        p--;  /* print format as normal char */
        break;
    }
    return (INT)(p - format) + 1;
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
static INT WPRINTF_ParseFormatW( LPCWSTR format, WPRINTF_FORMAT *res )
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
    else if (*p == 'w') { res->flags |= WPRINTF_WIDE; p++; }
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
        res->type = ((res->flags & WPRINTF_SHORT) && !(res->flags & WPRINTF_WIDE)) ? WPR_STRING : WPR_WSTRING;
        break;
    case 'S':
        res->type = (res->flags & (WPRINTF_LONG|WPRINTF_WIDE)) ? WPR_WSTRING : WPR_STRING;
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
        res->type = WPR_UNKNOWN;
        p--;  /* print format as normal char */
        break;
    }
    return (INT)(p - format) + 1;
}


/***********************************************************************
 *           WPRINTF_GetLen
 */
static UINT WPRINTF_GetLen( WPRINTF_FORMAT *format, WPRINTF_DATA *arg,
                              LPSTR number, UINT maxlen )
{
    UINT len;

    if (format->flags & WPRINTF_LEFTALIGN) format->flags &= ~WPRINTF_ZEROPAD;
    if (format->width > maxlen) format->width = maxlen;
    switch(format->type)
    {
    case WPR_CHAR:
    case WPR_WCHAR:
        return (format->precision = 1);
    case WPR_STRING:
        if (!arg->lpcstr_view) arg->lpcstr_view = null_stringA;
        for (len = 0; !format->precision || (len < format->precision); len++)
            if (!arg->lpcstr_view + len) break;
        if (len > maxlen) len = maxlen;
        return (format->precision = len);
    case WPR_WSTRING:
        if (!arg->lpcwstr_view) arg->lpcwstr_view = null_stringW;
        for (len = 0; !format->precision || (len < format->precision); len++)
            if (!*(arg->lpcwstr_view + len)) break;
        if (len > maxlen) len = maxlen;
        return (format->precision = len);
    case WPR_SIGNED:
        len = sprintf( number, "%d", arg->int_view );
        break;
    case WPR_UNSIGNED:
        len = sprintf( number, "%u", (UINT)arg->int_view );
        break;
    case WPR_HEXA:
        len = sprintf( number,
                        (format->flags & WPRINTF_UPPER_HEX) ? "%X" : "%x",
                        (UINT)arg->int_view);
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
 *           WPRINTF_ExtractVAPtr (Not a Windows API)
 */
static WPRINTF_DATA WPRINTF_ExtractVAPtr( WPRINTF_FORMAT *format, va_list* args )
{
    WPRINTF_DATA result;
    switch(format->type)
    {
        case WPR_WCHAR:
            result.wchar_view = va_arg( *args, WCHAR );     break;
        case WPR_CHAR:
            result.char_view = va_arg( *args, CHAR );       break;
        case WPR_STRING:
            result.lpcstr_view = va_arg( *args, LPCSTR);    break;
        case WPR_WSTRING:
            result.lpcwstr_view = va_arg( *args, LPCWSTR);  break;
        case WPR_HEXA:
        case WPR_SIGNED:
        case WPR_UNSIGNED:
            result.int_view = va_arg( *args, INT );         break;
        default:
            result.wchar_view = 0;                          break;
    }
    return result;
}

/***********************************************************************
 *           wvsnprintf16   (Not a Windows API)
 */
INT16 WINAPI wvsnprintf16( LPSTR buffer, UINT16 maxlen, LPCSTR spec,
                           LPCVOID args )
{
    WPRINTF_FORMAT format;
    LPSTR p = buffer;
    UINT i, len;
    CHAR number[20];
    WPRINTF_DATA cur_arg;
    SEGPTR seg_str;

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
            cur_arg.char_view = VA_ARG16( args, CHAR );
            break;
        case WPR_WSTRING:  /* No Unicode in Win16 */
        case WPR_STRING:
            seg_str = VA_ARG16( args, SEGPTR );
            if (IsBadReadPtr16(seg_str, 1 )) cur_arg.lpcstr_view = "";
            else cur_arg.lpcstr_view = PTR_SEG_TO_LIN( seg_str );
            break;
        case WPR_SIGNED:
            if (!(format.flags & WPRINTF_LONG))
            {
                cur_arg.int_view = VA_ARG16( args, INT16 );
                break;
            }
            /* fall through */
        case WPR_HEXA:
        case WPR_UNSIGNED:
            if (format.flags & WPRINTF_LONG)
                cur_arg.int_view = VA_ARG16( args, UINT );
            else
                cur_arg.int_view = VA_ARG16( args, UINT16 );
            break;
        case WPR_UNKNOWN:
            continue;
        }
        len = WPRINTF_GetLen( &format, &cur_arg, number, maxlen - 1 );
        if (!(format.flags & WPRINTF_LEFTALIGN))
            for (i = format.precision; i < format.width; i++, maxlen--)
                *p++ = ' ';
        switch(format.type)
        {
        case WPR_WCHAR:  /* No Unicode in Win16 */
        case WPR_CHAR:
	    *p= cur_arg.char_view;
            if (*p != '\0') p++;
            else if (format.width > 1) *p++ = ' ';
            else len = 0;
            break;
        case WPR_WSTRING:  /* No Unicode in Win16 */
        case WPR_STRING:
            if (len) memcpy( p, cur_arg.lpcstr_view, len );
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
        case WPR_UNKNOWN:
            continue;
        }
        if (format.flags & WPRINTF_LEFTALIGN)
            for (i = format.precision; i < format.width; i++, maxlen--)
                *p++ = ' ';
        maxlen -= len;
    }
    *p = 0;
    return (maxlen > 1) ? (INT)(p - buffer) : -1;
}


/***********************************************************************
 *           wvsnprintfA   (Not a Windows API)
 */
INT WINAPI wvsnprintfA( LPSTR buffer, UINT maxlen, LPCSTR spec,
                            va_list args )
{
    WPRINTF_FORMAT format;
    LPSTR p = buffer;
    UINT i, len;
    CHAR number[20];
    WPRINTF_DATA argData = (WPRINTF_DATA)0;

    while (*spec && (maxlen > 1))
    {
        if (*spec != '%') { *p++ = *spec++; maxlen--; continue; }
        spec++;
        if (*spec == '%') { *p++ = *spec++; maxlen--; continue; }
        spec += WPRINTF_ParseFormatA( spec, &format );
        argData = WPRINTF_ExtractVAPtr( &format, &args );
        len = WPRINTF_GetLen( &format, &argData, number, maxlen - 1 );
        if (!(format.flags & WPRINTF_LEFTALIGN))
            for (i = format.precision; i < format.width; i++, maxlen--)
                *p++ = ' ';
        switch(format.type)
        {
        case WPR_WCHAR:
	    *p = argData.wchar_view;
            if (*p != '\0') p++;
            else if (format.width > 1) *p++ = ' ';
            else len = 0;
            break;
        case WPR_CHAR:
	    *p = argData.char_view;
            if (*p != '\0') p++;
            else if (format.width > 1) *p++ = ' ';
            else len = 0;
            break;
        case WPR_STRING:
            memcpy( p, argData.lpcstr_view, len );
            p += len;
            break;
        case WPR_WSTRING:
            {
                LPCWSTR ptr = argData.lpcwstr_view;
                for (i = 0; i < len; i++) *p++ = (CHAR)*ptr++;
            }
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
            memcpy( p, number, len );
            p += len;
            /* Go to the next arg */
            break;
        case WPR_UNKNOWN:
            continue;
        }
        if (format.flags & WPRINTF_LEFTALIGN)
            for (i = format.precision; i < format.width; i++, maxlen--)
                *p++ = ' ';
        maxlen -= len;
    }
    *p = 0;
    TRACE(string,"%s\n",buffer);
    return (maxlen > 1) ? (INT)(p - buffer) : -1;
}


/***********************************************************************
 *           wvsnprintfW   (Not a Windows API)
 */
INT WINAPI wvsnprintfW( LPWSTR buffer, UINT maxlen, LPCWSTR spec,
                            va_list args )
{
    WPRINTF_FORMAT format;
    LPWSTR p = buffer;
    UINT i, len;
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
	    *p = va_arg( args, WCHAR );
            if (*p != '\0') p++;
            else if (format.width > 1) *p++ = ' ';
            else len = 0;
            break;
        case WPR_CHAR:
	    *p = (WCHAR)va_arg( args, CHAR );
            if (*p != '\0') p++;
            else if (format.width > 1) *p++ = ' ';
            else len = 0;
            break;
        case WPR_STRING:
            {
                LPCSTR ptr = va_arg( args, LPCSTR );
                for (i = 0; i < len; i++) *p++ = (WCHAR)*ptr++;
            }
            break;
        case WPR_WSTRING:
            if (len) memcpy( p, va_arg( args, LPCWSTR ), len * sizeof(WCHAR) );
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
            (void)va_arg( args, INT ); /* Go to the next arg */
            break;
        case WPR_UNKNOWN:
            continue;
        }
        if (format.flags & WPRINTF_LEFTALIGN)
            for (i = format.precision; i < format.width; i++, maxlen--)
                *p++ = ' ';
        maxlen -= len;
    }
    *p = 0;
    return (maxlen > 1) ? (INT)(p - buffer) : -1;
}


/***********************************************************************
 *           wvsprintf16   (USER.421)
 */
INT16 WINAPI wvsprintf16( LPSTR buffer, LPCSTR spec, LPCVOID args )
{
    TRACE(string,"for %p got:\n",buffer);
    return wvsnprintf16( buffer, 0xffff, spec, args );
}


/***********************************************************************
 *           wvsprintfA   (USER32.587)
 */
INT WINAPI wvsprintfA( LPSTR buffer, LPCSTR spec, va_list args )
{
    TRACE(string,"for %p got:\n",buffer);
    return wvsnprintfA( buffer, 0xffffffff, spec, args );
}


/***********************************************************************
 *           wvsprintfW   (USER32.588)
 */
INT WINAPI wvsprintfW( LPWSTR buffer, LPCWSTR spec, va_list args )
{
    TRACE(string,"for %p got:\n",buffer);
    return wvsnprintfW( buffer, 0xffffffff, spec, args );
}


/***********************************************************************
 *           wsprintf16   (USER.420)
 */
/* Winelib version */
INT16 WINAPIV wsprintf16( LPSTR buffer, LPCSTR spec, ... )
{
    va_list valist;
    INT16 res;

    TRACE(string,"for %p got:\n",buffer);
    va_start( valist, spec );
    /* Note: we call the 32-bit version, because the args are 32-bit */
    res = (INT16)wvsnprintfA( buffer, 0xffffffff, spec, valist );
    va_end( valist );
    return res;
}

/* Emulator version */
INT16 WINAPIV WIN16_wsprintf16(void)
{
    VA_LIST16 valist;
    INT16 res;
    SEGPTR buffer, spec;

    VA_START16( valist );
    buffer = VA_ARG16( valist, SEGPTR );
    spec   = VA_ARG16( valist, SEGPTR );
    TRACE(string,"got:\n");
    res = wvsnprintf16( (LPSTR)PTR_SEG_TO_LIN(buffer), 0xffff,
                        (LPCSTR)PTR_SEG_TO_LIN(spec), valist );
    VA_END16( valist );
    return res;
}


/***********************************************************************
 *           wsprintfA   (USER32.585)
 */
INT WINAPIV wsprintfA( LPSTR buffer, LPCSTR spec, ... )
{
    va_list valist;
    INT res;

    TRACE(string,"for %p got:\n",buffer);
    va_start( valist, spec );
    res = wvsnprintfA( buffer, 0xffffffff, spec, valist );
    va_end( valist );
    return res;
}


/***********************************************************************
 *           wsprintfW   (USER32.586)
 */
INT WINAPIV wsprintfW( LPWSTR buffer, LPCWSTR spec, ... )
{
    va_list valist;
    INT res;

    TRACE(string,"wsprintfW for %p\n",buffer);
    va_start( valist, spec );
    res = wvsnprintfW( buffer, 0xffffffff, spec, valist );
    va_end( valist );
    return res;
}


/***********************************************************************
 *           wsnprintf16   (Not a Windows API)
 */
INT16 WINAPIV wsnprintf16( LPSTR buffer, UINT16 maxlen, LPCSTR spec, ... )
{
    va_list valist;
    INT16 res;

    va_start( valist, spec );
    res = wvsnprintf16( buffer, maxlen, spec, valist );
    va_end( valist );
    return res;
}


/***********************************************************************
 *           wsnprintfA   (Not a Windows API)
 */
INT WINAPIV wsnprintfA( LPSTR buffer, UINT maxlen, LPCSTR spec, ... )
{
    va_list valist;
    INT res;

    va_start( valist, spec );
    res = wvsnprintfA( buffer, maxlen, spec, valist );
    va_end( valist );
    return res;
}


/***********************************************************************
 *           wsnprintfW   (Not a Windows API)
 */
INT WINAPIV wsnprintfW( LPWSTR buffer, UINT maxlen, LPCWSTR spec, ... )
{
    va_list valist;
    INT res;

    va_start( valist, spec );
    res = wvsnprintfW( buffer, maxlen, spec, valist );
    va_end( valist );
    return res;
}
