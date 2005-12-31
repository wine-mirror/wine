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
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"

#ifdef HAVE_LDAP_H
#include <ldap.h>
#else
#define LDAP_SUCCESS        0x00
#define LDAP_NOT_SUPPORTED  0x5c
#endif

#include "winldap_private.h"
#include "wldap32.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

ULONG WLDAP32_ldap_abandon( WLDAP32_LDAP *ld, ULONG msgid )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP

    TRACE( "(%p, 0x%08lx)\n", ld, msgid );

    if (!ld) return ~0UL;
    ret = ldap_abandon_ext( ld, msgid, NULL, NULL );

#endif
    return ret;
}

ULONG ldap_check_filterA( WLDAP32_LDAP *ld, PCHAR filter )
{
    ULONG ret;
    WCHAR *filterW = NULL;

    TRACE( "(%p, %s)\n", ld, debugstr_a(filter) );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (filter) {
        filterW = strAtoW( filter );
        if (!filterW) return WLDAP32_LDAP_NO_MEMORY;
    }

    ret = ldap_check_filterW( ld, filterW );

    strfreeW( filterW );
    return ret;
}

ULONG ldap_check_filterW( WLDAP32_LDAP *ld, PWCHAR filter )
{
    TRACE( "(%p, %s)\n", ld, debugstr_w(filter) );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    return LDAP_SUCCESS; /* FIXME: do some checks */
}

ULONG ldap_cleanup( HANDLE instance )
{
    TRACE( "(%p)\n", instance );
    return LDAP_SUCCESS;
}

WLDAP32_LDAP *ldap_conn_from_msg( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *res )
{
    TRACE( "(%p, %p)\n", ld, res );

    if (!ld || !res) return NULL;
    return ld; /* FIXME: not always correct */
}

ULONG WLDAP32_ldap_count_entries( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *res )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP

    TRACE( "(%p, %p)\n", ld, res );

    if (!ld) return ~0UL;
    ret = ldap_count_entries( ld, res );

#endif
    return ret;
}

ULONG WLDAP32_ldap_count_references( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *res )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP_COUNT_REFERENCES

    TRACE( "(%p, %p)\n", ld, res );

    if (!ld) return 0;
    ret = ldap_count_references( ld, res );

#endif
    return ret;
}

static ULONG get_escape_size( PCHAR src, ULONG srclen )
{
    ULONG i, size = 0;

    if (src)
    {
        for (i = 0; i < srclen; i++)
        {
            if ((src[i] >= '0' && src[i] <= '9') ||
                (src[i] >= 'A' && src[i] <= 'Z') ||
                (src[i] >= 'a' && src[i] <= 'z'))
                size++;
            else
                size += 3;
        }
    }
    return size + 1;
}

static void escape_filter_element( PCHAR src, ULONG srclen, PCHAR dst )
{
    ULONG i;
    char fmt[] = "\\%02X";
    char *d = dst;

    for (i = 0; i < srclen; i++)
    {
        if ((src[i] >= '0' && src[i] <= '9') ||
            (src[i] >= 'A' && src[i] <= 'Z') ||
            (src[i] >= 'a' && src[i] <= 'z'))
            *d++ = src[i];
        else
        {
            sprintf( d, fmt, (unsigned char)src[i] );
            d += 3;
        }
    }
    *++d = 0;
}

ULONG ldap_escape_filter_elementA( PCHAR src, ULONG srclen, PCHAR dst, ULONG dstlen )
{
    ULONG len;

    TRACE( "(%p, 0x%08lx, %p, 0x%08lx)\n", src, srclen, dst, dstlen );

    len = get_escape_size( src, srclen );
    if (!dst) return len;

    if (!src || dstlen < len)
        return WLDAP32_LDAP_PARAM_ERROR;
    else
    {
        escape_filter_element( src, srclen, dst );
        return LDAP_SUCCESS;
    }
}

ULONG ldap_escape_filter_elementW( PCHAR src, ULONG srclen, PWCHAR dst, ULONG dstlen )
{
    ULONG len;

    TRACE( "(%p, 0x%08lx, %p, 0x%08lx)\n", src, srclen, dst, dstlen );

    len = get_escape_size( src, srclen );
    if (!dst) return len;

    /* no matter what you throw at it, this is what native returns */
    return WLDAP32_LDAP_PARAM_ERROR;
}

PCHAR ldap_first_attributeA( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *entry,
    WLDAP32_BerElement** ptr )
{
    PCHAR ret = NULL;
#ifdef HAVE_LDAP
    WCHAR *retW;

    TRACE( "(%p, %p, %p)\n", ld, entry, ptr );

    if (!ld || !entry) return NULL;
    retW = ldap_first_attributeW( ld, entry, ptr );

    ret = strWtoA( retW );
    ldap_memfreeW( retW );

#endif
    return ret;
}

PWCHAR ldap_first_attributeW( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *entry,
    WLDAP32_BerElement** ptr )
{
    PWCHAR ret = NULL;
#ifdef HAVE_LDAP
    char *retU;

    TRACE( "(%p, %p, %p)\n", ld, entry, ptr );

    if (!ld || !entry) return NULL;
    retU = ldap_first_attribute( ld, entry, ptr );

    ret = strUtoW( retU );
    ldap_memfree( retU );

#endif
    return ret;
}

WLDAP32_LDAPMessage *WLDAP32_ldap_first_entry( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *res )
{
#ifdef HAVE_LDAP

    TRACE( "(%p, %p)\n", ld, res );

    if (!ld || !res) return NULL;
    return ldap_first_entry( ld, res );

#endif
    return NULL;
}

WLDAP32_LDAPMessage *WLDAP32_ldap_first_reference( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *res )
{
#ifdef HAVE_LDAP_FIRST_REFERENCE

    TRACE( "(%p, %p)\n", ld, res );

    if (!ld) return NULL;
    return ldap_first_reference( ld, res );

#endif
    return NULL;
}

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

ULONG WLDAP32_ldap_msgfree( WLDAP32_LDAPMessage *res )
{
    ULONG ret = LDAP_SUCCESS;
#ifdef HAVE_LDAP

    TRACE( "(%p)\n", res );
    ldap_msgfree( res );

#endif
    return ret;
}

PCHAR ldap_next_attributeA( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *entry,
    WLDAP32_BerElement *ptr )
{
    PCHAR ret = NULL;
#ifdef HAVE_LDAP
    WCHAR *retW;

    TRACE( "(%p, %p, %p)\n", ld, entry, ptr );

    if (!ld || !entry || !ptr) return NULL;
    retW = ldap_next_attributeW( ld, entry, ptr );

    ret = strWtoA( retW );
    ldap_memfreeW( retW );

#endif
    return ret;
}

PWCHAR ldap_next_attributeW( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *entry,
    WLDAP32_BerElement *ptr )
{
    PWCHAR ret = NULL;
#ifdef HAVE_LDAP
    char *retU;

    TRACE( "(%p, %p, %p)\n", ld, entry, ptr );

    if (!ld || !entry || !ptr) return NULL;
    retU = ldap_next_attribute( ld, entry, ptr );

    ret = strUtoW( retU );
    ldap_memfree( retU );

#endif
    return ret;
}

WLDAP32_LDAPMessage *WLDAP32_ldap_next_entry( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *entry )
{
#ifdef HAVE_LDAP

    TRACE( "(%p, %p)\n", ld, entry );

    if (!ld || !entry) return NULL;
    return ldap_next_entry( ld, entry );

#endif
    return NULL;
}

WLDAP32_LDAPMessage *WLDAP32_ldap_next_reference( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *entry )
{
#ifdef HAVE_LDAP_NEXT_REFERENCE

    TRACE( "(%p, %p)\n", ld, entry );

    if (!ld || !entry) return NULL;
    return ldap_next_reference( ld, entry );

#endif
    return NULL;
}

ULONG WLDAP32_ldap_result( WLDAP32_LDAP *ld, ULONG msgid, ULONG all,
    struct l_timeval *timeout, WLDAP32_LDAPMessage **res )
{
    ULONG ret = WLDAP32_LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP

    TRACE( "(%p, 0x%08lx, 0x%08lx, %p, %p)\n", ld, msgid, all, timeout, res );

    if (!ld || !res || msgid == ~0UL) return ~0UL;
    ret = ldap_result( ld, msgid, all, (struct timeval *)timeout, res );

#endif
    return ret;
}

int LdapUnicodeToUTF8( LPCWSTR src, int srclen, LPSTR dst, int dstlen )
{
    return WideCharToMultiByte( CP_UTF8, 0, src, srclen, dst, dstlen, NULL, NULL );
}

int LdapUTF8ToUnicode( LPCSTR src, int srclen, LPWSTR dst, int dstlen )
{
    return MultiByteToWideChar( CP_UTF8, 0, src, srclen, dst, dstlen );
}
