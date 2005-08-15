/*
 * WLDAP32 - LDAP support for Wine
 *
 * Copyright 2005 Hans Leidekker
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
#include "wine/debug.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"

#ifdef HAVE_LDAP_H
#include <ldap.h>
#else
#define LDAP_SUCCESS        0x00
#endif

#include "winldap_private.h"
#include "wldap32.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

void ldap_memfreeA( PCHAR block )
{
    TRACE( "(%p)\n", block );
    strfreeA( block );
}

void ldap_memfreeW( PWCHAR block )
{
    TRACE( "(%p)\n", block );
    strfreeW( block );
}

ULONG WLDAP32_ldap_result( WLDAP32_LDAP *ld, ULONG msgid, ULONG all,
    struct l_timeval *timeout, WLDAP32_LDAPMessage **res )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP

    TRACE( "(%p, 0x%08lx, 0x%08lx, %p, %p)\n", ld, msgid, all, timeout, res );

    if (!ld || !res) return ~0UL;
    ret = ldap_result( ld, msgid, all, (struct timeval *)timeout, res );

#endif
    return ret;
}

ULONG ldap_value_freeA( PCHAR *vals )
{
    TRACE( "(%p)\n", vals );

    strarrayfreeA( vals );
    return LDAP_SUCCESS;
}

ULONG ldap_value_freeW( PWCHAR *vals )
{
    TRACE( "(%p)\n", vals );

    strarrayfreeW( vals );
    return LDAP_SUCCESS;
}

int LdapUnicodeToUTF8( LPCWSTR src, int srclen, LPSTR dst, int dstlen )
{
    return WideCharToMultiByte( CP_UTF8, 0, src, srclen, dst, dstlen, NULL, NULL );
}

int LdapUTF8ToUnicode( LPCSTR src, int srclen, LPWSTR dst, int dstlen )
{
    return MultiByteToWideChar( CP_UTF8, 0, src, srclen, dst, dstlen );
}
