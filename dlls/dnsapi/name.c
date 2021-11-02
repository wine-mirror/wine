/*
 * DNS support
 *
 * Copyright (C) 2006 Matthew Kehrer
 * Copyright (C) 2006 Hans Leidekker
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#include "windns.h"

#include "wine/debug.h"
#include "dnsapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(dnsapi);

/******************************************************************************
 * DnsNameCompare_A               [DNSAPI.@]
 *
 */
BOOL WINAPI DnsNameCompare_A( PCSTR name1, PCSTR name2 )
{
    BOOL ret;
    PWSTR name1W, name2W;

    TRACE( "(%s,%s)\n", debugstr_a(name1), debugstr_a(name2) );

    name1W = strdup_aw( name1 );
    name2W = strdup_aw( name2 );

    ret = DnsNameCompare_W( name1W, name2W );

    free( name1W );
    free( name2W );

    return ret;
}

/******************************************************************************
 * DnsNameCompare_W               [DNSAPI.@]
 *
 */
BOOL WINAPI DnsNameCompare_W( PCWSTR name1, PCWSTR name2 )
{
    PCWSTR p, q;

    TRACE( "(%s,%s)\n", debugstr_w(name1), debugstr_w(name2) );

    if (!name1 && !name2) return TRUE;
    if (!name1 || !name2) return FALSE;

    p = name1 + lstrlenW( name1 );
    q = name2 + lstrlenW( name2 );

    while (p > name1 && p[-1] == '.') p--;
    while (q > name2 && q[-1] == '.') q--;

    return CompareStringOrdinal( name1, p - name1, name2, q - name2, TRUE ) == CSTR_EQUAL;
}

/******************************************************************************
 * DnsValidateName_A              [DNSAPI.@]
 *
 */
DNS_STATUS WINAPI DnsValidateName_A( PCSTR name, DNS_NAME_FORMAT format )
{
    PWSTR nameW;
    DNS_STATUS ret;

    TRACE( "(%s, %d)\n", debugstr_a(name), format );

    nameW = strdup_aw( name );
    ret = DnsValidateName_W( nameW, format );

    free( nameW );
    return ret;
}

/******************************************************************************
 * DnsValidateName_UTF8           [DNSAPI.@]
 *
 */
DNS_STATUS WINAPI DnsValidateName_UTF8( PCSTR name, DNS_NAME_FORMAT format )
{
    PWSTR nameW;
    DNS_STATUS ret;

    TRACE( "(%s, %d)\n", debugstr_a(name), format );

    nameW = strdup_uw( name );
    ret = DnsValidateName_W( nameW, format );

    free( nameW );
    return ret;
}

#define HAS_EXTENDED        0x0001
#define HAS_NUMERIC         0x0002
#define HAS_NON_NUMERIC     0x0004
#define HAS_DOT             0x0008
#define HAS_DOT_DOT         0x0010
#define HAS_SPACE           0x0020
#define HAS_INVALID         0x0040
#define HAS_ASTERISK        0x0080
#define HAS_UNDERSCORE      0x0100
#define HAS_LONG_LABEL      0x0200

/******************************************************************************
 * DnsValidateName_W              [DNSAPI.@]
 *
 */
DNS_STATUS WINAPI DnsValidateName_W( PCWSTR name, DNS_NAME_FORMAT format )
{
    PCWSTR p;
    unsigned int i, j, state = 0;
    static const WCHAR invalid[] = L"{|}~[\\]^':;<=>?@!\"#$%&`()+/,";

    TRACE( "(%s, %d)\n", debugstr_w(name), format );

    if (!name) return ERROR_INVALID_NAME;

    for (p = name, i = 0, j = 0; *p; p++, i++, j++)
    {
        if (*p == '.')
        {
            j = 0;
            state |= HAS_DOT;
            if (p[1] == '.') state |= HAS_DOT_DOT;
        }
        else if (*p < '0' || *p > '9') state |= HAS_NON_NUMERIC;
        else state |= HAS_NUMERIC;

        if (j > 62) state |= HAS_LONG_LABEL;

        if (wcschr( invalid, *p )) state |= HAS_INVALID;
        else if ((unsigned)*p > 127) state |= HAS_EXTENDED;
        else if (*p == ' ') state |= HAS_SPACE;
        else if (*p == '_') state |= HAS_UNDERSCORE;
        else if (*p == '*') state |= HAS_ASTERISK;
    }

    if (i == 0 || i > 255 ||
        (state & HAS_LONG_LABEL) ||
        (state & HAS_DOT_DOT) ||
        (name[0] == '.' && name[1])) return ERROR_INVALID_NAME;

    switch (format)
    {
    case DnsNameDomain:
    {
        if ((state & HAS_EXTENDED) || (state & HAS_UNDERSCORE))
            return DNS_ERROR_NON_RFC_NAME;
        if ((state & HAS_SPACE) ||
            (state & HAS_INVALID) ||
            (state & HAS_ASTERISK)) return DNS_ERROR_INVALID_NAME_CHAR;
        break;
    }
    case DnsNameDomainLabel:
    {
        if (state & HAS_DOT) return ERROR_INVALID_NAME;
        if ((state & HAS_EXTENDED) || (state & HAS_UNDERSCORE))
            return DNS_ERROR_NON_RFC_NAME;
        if ((state & HAS_SPACE) ||
            (state & HAS_INVALID) ||
            (state & HAS_ASTERISK)) return DNS_ERROR_INVALID_NAME_CHAR;
        break;
    }
    case DnsNameHostnameFull:
    {
        if ((state & HAS_EXTENDED) || (state & HAS_UNDERSCORE))
            return DNS_ERROR_NON_RFC_NAME;
        if ((state & HAS_SPACE) ||
            (state & HAS_INVALID) ||
            (state & HAS_ASTERISK)) return DNS_ERROR_INVALID_NAME_CHAR;
        break;
    }
    case DnsNameHostnameLabel:
    {
        if (state & HAS_DOT) return ERROR_INVALID_NAME;
        if ((state & HAS_EXTENDED) || (state & HAS_UNDERSCORE))
            return DNS_ERROR_NON_RFC_NAME;
        if ((state & HAS_SPACE) ||
            (state & HAS_INVALID) ||
            (state & HAS_ASTERISK)) return DNS_ERROR_INVALID_NAME_CHAR;
        break;
    }
    case DnsNameWildcard:
    {
        if (!(state & HAS_NON_NUMERIC) && (state & HAS_NUMERIC))
            return ERROR_INVALID_NAME;
        if (name[0] != '*') return ERROR_INVALID_NAME;
        if (name[1] && name[1] != '.')
            return DNS_ERROR_INVALID_NAME_CHAR;
        if ((state & HAS_EXTENDED) ||
            (state & HAS_SPACE) ||
            (state & HAS_INVALID)) return ERROR_INVALID_NAME;
        break;
    }
    case DnsNameSrvRecord:
    {
        if (!(state & HAS_NON_NUMERIC) && (state & HAS_NUMERIC))
            return ERROR_INVALID_NAME;
        if (name[0] != '_') return ERROR_INVALID_NAME;
        if ((state & HAS_UNDERSCORE) && !name[1])
            return DNS_ERROR_NON_RFC_NAME;
        if ((state & HAS_EXTENDED) ||
            (state & HAS_SPACE) ||
            (state & HAS_INVALID)) return ERROR_INVALID_NAME;
        break;
    }
    default:
        WARN( "unknown format: %d\n", format );
        break;
    }
    return ERROR_SUCCESS;
}
