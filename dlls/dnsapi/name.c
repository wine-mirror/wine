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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "wine/debug.h"
#include "wine/unicode.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#include "windns.h"

#include "dnsapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(dnsapi);

/******************************************************************************
 * DnsNameCompare_A               [DNSAPI.@]
 *
 */
BOOL WINAPI DnsNameCompare_A( LPSTR name1, LPSTR name2 )
{
    BOOL ret;
    LPWSTR name1W, name2W;

    TRACE( "(%s,%s)\n", debugstr_a(name1), debugstr_a(name2) );

    name1W = dns_strdup_aw( name1 );
    name2W = dns_strdup_aw( name2 );

    ret = DnsNameCompare_W( name1W, name2W );

    dns_free( name1W );
    dns_free( name2W );

    return ret;
}

/******************************************************************************
 * DnsNameCompare_W               [DNSAPI.@]
 *
 */
BOOL WINAPI DnsNameCompare_W( LPWSTR name1, LPWSTR name2 )
{
    WCHAR *p, *q;

    TRACE( "(%s,%s)\n", debugstr_w(name1), debugstr_w(name2) );

    if (!name1 && !name2) return TRUE;
    if (!name1 || !name2) return FALSE;
 
    p = name1 + lstrlenW( name1 ) - 1;
    q = name2 + lstrlenW( name2 ) - 1;

    while (*p == '.' && p >= name1) p--;
    while (*q == '.' && q >= name2) q--;

    if (p - name1 != q - name2) return FALSE;

    while (name1 <= p)
    {
        if (toupperW( *name1 ) != toupperW( *name2 ))
            return FALSE;

        name1++;
        name2++;
    }
    return TRUE;
}
