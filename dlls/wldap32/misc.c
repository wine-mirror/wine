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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winsock2.h"

#include "wine/debug.h"
#include "winldap_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

/***********************************************************************
 *      ldap_abandon     (WLDAP32.@)
 */
ULONG CDECL WLDAP32_ldap_abandon( LDAP *ld, ULONG msgid )
{
    TRACE( "(%p, %#lx)\n", ld, msgid );

    if (!ld) return ~0u;
    return map_error( ldap_abandon_ext( CTX(ld), msgid, NULL, NULL ) );
}

/***********************************************************************
 *      ldap_check_filterA     (WLDAP32.@)
 */
ULONG CDECL ldap_check_filterA( LDAP *ld, char *filter )
{
    ULONG ret;
    WCHAR *filterW = NULL;

    TRACE( "(%p, %s)\n", ld, debugstr_a(filter) );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    if (filter && !(filterW = strAtoW( filter ))) return WLDAP32_LDAP_NO_MEMORY;

    ret = ldap_check_filterW( ld, filterW );

    free( filterW );
    return ret;
}

/***********************************************************************
 *      ldap_check_filterW     (WLDAP32.@)
 */
ULONG CDECL ldap_check_filterW( LDAP *ld, WCHAR *filter )
{
    TRACE( "(%p, %s)\n", ld, debugstr_w(filter) );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    return WLDAP32_LDAP_SUCCESS; /* FIXME: do some checks */
}

/***********************************************************************
 *      ldap_cleanup     (WLDAP32.@)
 */
ULONG CDECL ldap_cleanup( HANDLE instance )
{
    TRACE( "(%p)\n", instance );
    return WLDAP32_LDAP_SUCCESS;
}

/***********************************************************************
 *      ldap_conn_from_msg     (WLDAP32.@)
 */
LDAP * CDECL ldap_conn_from_msg( LDAP *ld, WLDAP32_LDAPMessage *res )
{
    TRACE( "(%p, %p)\n", ld, res );

    if (!ld || !res) return NULL;
    return ld; /* FIXME: not always correct */
}

/***********************************************************************
 *      ldap_count_entries     (WLDAP32.@)
 */
ULONG CDECL WLDAP32_ldap_count_entries( LDAP *ld, WLDAP32_LDAPMessage *res )
{
    TRACE( "(%p, %p)\n", ld, res );

    if (!res) return 0;
    if (!ld) return ~0u;
    return ldap_count_entries( CTX(ld), MSG(res) );
}

/***********************************************************************
 *      ldap_count_references     (WLDAP32.@)
 */
ULONG CDECL WLDAP32_ldap_count_references( LDAP *ld, WLDAP32_LDAPMessage *res )
{
    TRACE( "(%p, %p)\n", ld, res );

    if (!ld) return 0;
    return ldap_count_references( CTX(ld), MSG(res) );
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

static void escape_filter_element( char *src, ULONG srclen, char *dst )
{
    ULONG i;
    static const char fmt[] = "\\%02X";
    char *d = dst;

    for (i = 0; i < srclen; i++)
    {
        if ((src[i] >= '0' && src[i] <= '9') ||
            (src[i] >= 'A' && src[i] <= 'Z') ||
            (src[i] >= 'a' && src[i] <= 'z'))
            *d++ = src[i];
        else
            d += sprintf( d, fmt, (unsigned char)src[i] );
    }
    *++d = 0;
}

/***********************************************************************
 *      ldap_escape_filter_elementA     (WLDAP32.@)
 */
ULONG CDECL ldap_escape_filter_elementA( char *src, ULONG srclen, char *dst, ULONG dstlen )
{
    ULONG len = get_escape_size( src, srclen );

    TRACE( "(%p, %#lx, %p, %#lx)\n", src, srclen, dst, dstlen );

    if (!dst) return len;
    if (!src || dstlen < len) return WLDAP32_LDAP_PARAM_ERROR;

    escape_filter_element( src, srclen, dst );
    return WLDAP32_LDAP_SUCCESS;
}

/***********************************************************************
 *      ldap_escape_filter_elementW     (WLDAP32.@)
 */
ULONG CDECL ldap_escape_filter_elementW( char *src, ULONG srclen, WCHAR *dst, ULONG dstlen )
{
    ULONG len = get_escape_size( src, srclen );

    TRACE( "(%p, %#lx, %p, %#lx)\n", src, srclen, dst, dstlen );

    if (!dst) return len;

    /* no matter what you throw at it, this is what native returns */
    return WLDAP32_LDAP_PARAM_ERROR;
}

/***********************************************************************
 *      ldap_first_attributeA     (WLDAP32.@)
 */
char * CDECL ldap_first_attributeA( LDAP *ld, WLDAP32_LDAPMessage *entry, WLDAP32_BerElement **ber )
{
    char *ret = NULL;
    WCHAR *retW;

    TRACE( "(%p, %p, %p)\n", ld, entry, ber );

    if (!ld || !entry) return NULL;

    retW = ldap_first_attributeW( ld, entry, ber );
    if (retW)
    {
        ret = strWtoA( retW );
        ldap_memfreeW( retW );
    }

    return ret;
}

/***********************************************************************
 *      ldap_first_attributeW     (WLDAP32.@)
 */
WCHAR * CDECL ldap_first_attributeW( LDAP *ld, WLDAP32_LDAPMessage *entry, WLDAP32_BerElement **ptr )
{
    WCHAR *ret = NULL;
    WLDAP32_BerElement *ber;
    char *retU;
    BerElement *berU;

    TRACE( "(%p, %p, %p)\n", ld, entry, ptr );

    if (ld && entry) retU = ldap_first_attribute( CTX(ld), MSG(entry), &berU );
    else return NULL;

    if (!retU)
        return NULL;

    if (!(ber = malloc( sizeof(*ber) )))
    {
        ld->ld_errno = WLDAP32_LDAP_NO_MEMORY;
        ldap_memfree( retU );
        return NULL;
    }

    ber->opaque = (char *)berU;
    *ptr = ber;
    ret = strUtoW( retU );
    ldap_memfree( retU );
    return ret;
}

/***********************************************************************
 *      ldap_first_entry     (WLDAP32.@)
*/
WLDAP32_LDAPMessage * CDECL WLDAP32_ldap_first_entry( LDAP *ld, WLDAP32_LDAPMessage *res )
{
    LDAPMessage *msgU;

    TRACE( "(%p, %p)\n", ld, res );

    if (ld && res && (msgU = ldap_first_entry( CTX(ld), MSG(res) )))
    {
        assert( msgU == MSG(res) );
        return res;
    }
    return NULL;
}

/***********************************************************************
 *      ldap_first_reference     (WLDAP32.@)
 */
WLDAP32_LDAPMessage * CDECL WLDAP32_ldap_first_reference( LDAP *ld, WLDAP32_LDAPMessage *res )
{
    LDAPMessage *msgU;

    TRACE( "(%p, %p)\n", ld, res );

    if (ld && (msgU = ldap_first_reference( CTX(ld), MSG(res) )))
    {
        assert( msgU == MSG(res) );
        return res;
    }
    return NULL;
}

/***********************************************************************
 *      ldap_memfreeA     (WLDAP32.@)
 */
void CDECL ldap_memfreeA( char *block )
{
    TRACE( "(%p)\n", block );
    free( block );
}

/***********************************************************************
 *      ldap_memfreeW     (WLDAP32.@)
 */
void CDECL ldap_memfreeW( WCHAR *block )
{
    TRACE( "(%p)\n", block );
    free( block );
}

/***********************************************************************
 *      ldap_msgfree     (WLDAP32.@)
 */
ULONG CDECL WLDAP32_ldap_msgfree( WLDAP32_LDAPMessage *res )
{
    WLDAP32_LDAPMessage *entry, *list = res;

    TRACE( "(%p)\n", res );

    if (!res) return WLDAP32_LDAP_SUCCESS;

    ldap_msgfree( MSG(res) );
    while (list)
    {
        entry = list;
        list = entry->lm_next;
        free( entry );
    }

    return WLDAP32_LDAP_SUCCESS;
}

/***********************************************************************
 *      ldap_next_attributeA     (WLDAP32.@)
 */
char * CDECL ldap_next_attributeA( LDAP *ld, WLDAP32_LDAPMessage *entry, WLDAP32_BerElement *ptr )
{
    char *ret = NULL;
    WCHAR *retW;

    TRACE( "(%p, %p, %p)\n", ld, entry, ptr );

    if (!ld || !entry || !ptr) return NULL;

    retW = ldap_next_attributeW( ld, entry, ptr );
    if (retW)
    {
        ret = strWtoA( retW );
        ldap_memfreeW( retW );
    }

    return ret;
}

/***********************************************************************
 *      ldap_next_attributeW     (WLDAP32.@)
 */
WCHAR * CDECL ldap_next_attributeW( LDAP *ld, WLDAP32_LDAPMessage *entry, WLDAP32_BerElement *ptr )
{
    WCHAR *ret = NULL;
    char *retU;

    TRACE( "(%p, %p, %p)\n", ld, entry, ptr );

    if (ld && entry && ptr && (retU = ldap_next_attribute( CTX(ld), MSG(entry), BER(ptr) )))
    {
        ret = strUtoW( retU );
        ldap_memfree( retU );
    }
    return ret;
}

/***********************************************************************
 *      ldap_next_entry     (WLDAP32.@)
 */
WLDAP32_LDAPMessage * CDECL WLDAP32_ldap_next_entry( LDAP *ld, WLDAP32_LDAPMessage *entry )
{
    WLDAP32_LDAPMessage *msg = NULL;
    LDAPMessage *msgU;

    TRACE( "(%p, %p)\n", ld, entry );

    if (!ld || !entry) return NULL;

    if (entry->lm_next) return entry->lm_next;

    msgU = ldap_next_entry( CTX(ld), MSG(entry) );

    if (!msgU)
        return NULL;

    if (!(msg = calloc( 1, sizeof(*msg) )))
    {
        ld->ld_errno = WLDAP32_LDAP_NO_MEMORY;
        return NULL;
    }

    MSG(msg) = msgU;
    entry->lm_next = msg;
    return msg;
}

/***********************************************************************
 *      ldap_next_reference     (WLDAP32.@)
 */
WLDAP32_LDAPMessage * CDECL WLDAP32_ldap_next_reference( LDAP *ld, WLDAP32_LDAPMessage *entry )
{
    WLDAP32_LDAPMessage *msg = NULL;
    LDAPMessage *msgU;

    TRACE( "(%p, %p)\n", ld, entry );

    if (!ld || !entry) return NULL;

    if (entry->lm_next) return entry->lm_next;

    msgU = ldap_next_reference( CTX(ld), MSG(entry) );

    if (!msgU)
        return NULL;

    if (!(msg = calloc( 1, sizeof(*msg) )))
    {
        ld->ld_errno = WLDAP32_LDAP_NO_MEMORY;
        return NULL;
    }

    MSG(msg) = msgU;
    entry->lm_next = msg;
    return msg;
}

/***********************************************************************
 *      ldap_result     (WLDAP32.@)
 */
ULONG CDECL WLDAP32_ldap_result( LDAP *ld, ULONG msgid, ULONG all, struct l_timeval *timeout, WLDAP32_LDAPMessage **res )
{
    WLDAP32_LDAPMessage *msg;
    struct timeval timeval;
    LDAPMessage *msgU = NULL;
    ULONG ret = ~0u;

    TRACE( "(%p, %#lx, %#lx, %p, %p)\n", ld, msgid, all, timeout, res );

    if (ld && res && msgid != ~0u)
    {
        if (timeout)
        {
            timeval.tv_sec = timeout->tv_sec;
            timeval.tv_usec = timeout->tv_usec;
        }

        ret = ldap_result( CTX(ld), msgid, all, timeout ? &timeval : NULL, &msgU );
    }

    if (!msgU)
        return ret;

    if (!(msg = calloc( 1, sizeof(*msg) )))
    {
        free( msgU );
        return WLDAP32_LDAP_NO_MEMORY;
    }

    MSG(msg) = msgU;
    *res = msg;
    return ret;
}

/***********************************************************************
 *      ldap_set_dbg_flags     (WLDAP32.@)
 */
ULONG CDECL ldap_set_dbg_flags( ULONG flags )
{
    FIXME( "(0x%lx) stub\n", flags );
    return 0;
}

/***********************************************************************
 *      LdapUnicodeToUTF8     (WLDAP32.@)
 */
int CDECL LdapUnicodeToUTF8( const WCHAR *src, int srclen, char *dst, int dstlen )
{
    return WideCharToMultiByte( CP_UTF8, 0, src, srclen, dst, dstlen, NULL, NULL );
}

/***********************************************************************
 *      LdapUTF8ToUnicode     (WLDAP32.@)
 */
int CDECL LdapUTF8ToUnicode( const char *src, int srclen, WCHAR *dst, int dstlen )
{
    return MultiByteToWideChar( CP_UTF8, 0, src, srclen, dst, dstlen );
}
