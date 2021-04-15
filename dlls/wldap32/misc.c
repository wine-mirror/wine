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
#include "windef.h"
#include "winbase.h"
#include "winnls.h"

#include "wine/debug.h"
#include "wine/heap.h"
#include "winldap_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

/***********************************************************************
 *      ldap_abandon     (WLDAP32.@)
 *
 * Cancel an asynchronous operation.
 *
 * PARAMS
 *  ld    [I] Pointer to an LDAP context.
 *  msgid [I] ID of the operation to cancel.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 */
ULONG CDECL WLDAP32_ldap_abandon( WLDAP32_LDAP *ld, ULONG msgid )
{
    TRACE( "(%p, 0x%08x)\n", ld, msgid );

    if (!ld) return ~0u;
    return map_error( ldap_funcs->ldap_abandon_ext( ld->ld, msgid, NULL, NULL ) );
}

/***********************************************************************
 *      ldap_check_filterA     (WLDAP32.@)
 *
 * See ldap_check_filterW.
 */
ULONG CDECL ldap_check_filterA( WLDAP32_LDAP *ld, char *filter )
{
    ULONG ret;
    WCHAR *filterW = NULL;

    TRACE( "(%p, %s)\n", ld, debugstr_a(filter) );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    if (filter && !(filterW = strAtoW( filter ))) return WLDAP32_LDAP_NO_MEMORY;

    ret = ldap_check_filterW( ld, filterW );

    strfreeW( filterW );
    return ret;
}

/***********************************************************************
 *      ldap_check_filterW     (WLDAP32.@)
 *
 * Check filter syntax.
 *
 * PARAMS
 *  ld     [I] Pointer to an LDAP context.
 *  filter [I] Filter string.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 */
ULONG CDECL ldap_check_filterW( WLDAP32_LDAP *ld, WCHAR *filter )
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
 *
 * Get the LDAP context for a given message.
 *
 * PARAMS
 *  ld  [I] Pointer to an LDAP context.
 *  res [I] LDAP message.
 *
 * RETURNS
 *  Success: Pointer to an LDAP context.
 *  Failure: NULL
 */
WLDAP32_LDAP * CDECL ldap_conn_from_msg( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *res )
{
    TRACE( "(%p, %p)\n", ld, res );

    if (!ld || !res) return NULL;
    return ld; /* FIXME: not always correct */
}

/***********************************************************************
 *      ldap_count_entries     (WLDAP32.@)
 *
 * Count the number of entries returned from a search.
 *
 * PARAMS
 *  ld  [I] Pointer to an LDAP context.
 *  res [I] LDAP message.
 *
 * RETURNS
 *  Success: The number of entries.
 *  Failure: ~0u
 */
ULONG CDECL WLDAP32_ldap_count_entries( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *res )
{
    TRACE( "(%p, %p)\n", ld, res );

    if (!ld) return ~0u;
    return ldap_funcs->ldap_count_entries( ld->ld, res->Request );
}

/***********************************************************************
 *      ldap_count_references     (WLDAP32.@)
 *
 * Count the number of references returned from a search.
 *
 * PARAMS
 *  ld  [I] Pointer to an LDAP context.
 *  res [I] LDAP message.
 *
 * RETURNS
 *  Success: The number of references.
 *  Failure: ~0u
 */
ULONG CDECL WLDAP32_ldap_count_references( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *res )
{
    TRACE( "(%p, %p)\n", ld, res );

    if (!ld) return 0;
    return ldap_funcs->ldap_count_references( ld->ld, res->Request );
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
 *
 * See ldap_escape_filter_elementW.
 */
ULONG CDECL ldap_escape_filter_elementA( char *src, ULONG srclen, char *dst, ULONG dstlen )
{
    ULONG len = get_escape_size( src, srclen );

    TRACE( "(%p, 0x%08x, %p, 0x%08x)\n", src, srclen, dst, dstlen );

    if (!dst) return len;
    if (!src || dstlen < len) return WLDAP32_LDAP_PARAM_ERROR;

    escape_filter_element( src, srclen, dst );
    return WLDAP32_LDAP_SUCCESS;
}

/***********************************************************************
 *      ldap_escape_filter_elementW     (WLDAP32.@)
 *
 * Escape binary data for safe passing in filters.
 *
 * PARAMS
 *  src    [I] Filter element to be escaped.
 *  srclen [I] Length in bytes of the filter element.
 *  dst    [O] Destination buffer for the escaped filter element.
 *  dstlen [I] Length in bytes of the destination buffer.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 */
ULONG CDECL ldap_escape_filter_elementW( char *src, ULONG srclen, WCHAR *dst, ULONG dstlen )
{
    ULONG len = get_escape_size( src, srclen );

    TRACE( "(%p, 0x%08x, %p, 0x%08x)\n", src, srclen, dst, dstlen );

    if (!dst) return len;

    /* no matter what you throw at it, this is what native returns */
    return WLDAP32_LDAP_PARAM_ERROR;
}

/***********************************************************************
 *      ldap_first_attributeA     (WLDAP32.@)
 *
 * See ldap_first_attributeW.
 */
char * CDECL ldap_first_attributeA( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *entry, WLDAP32_BerElement **ber )
{
    char *ret = NULL;
    WCHAR *retW;

    TRACE( "(%p, %p, %p)\n", ld, entry, ber );

    if (!ld || !entry) return NULL;

    retW = ldap_first_attributeW( ld, entry->Request, ber );
    if (retW)
    {
        ret = strWtoA( retW );
        ldap_memfreeW( retW );
    }

    return ret;
}

/***********************************************************************
 *      ldap_first_attributeW     (WLDAP32.@)
 *
 * Get the first attribute for a given entry.
 *
 * PARAMS
 *  ld    [I] Pointer to an LDAP context.
 *  entry [I] Entry to retrieve attribute for.
 *  ptr   [O] Position pointer.
 *
 * RETURNS
 *  Success: Name of the first attribute.
 *  Failure: NULL
 *
 * NOTES
 *  Use ldap_memfree to free the returned string.
 */
WCHAR * CDECL ldap_first_attributeW( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *entry, WLDAP32_BerElement **ptr )
{
    WCHAR *ret = NULL;
    WLDAP32_BerElement *ber;
    char *retU;
    void *berU;

    TRACE( "(%p, %p, %p)\n", ld, entry, ptr );

    if (!ld || !entry) return NULL;

    retU = ldap_funcs->ldap_first_attribute( ld->ld, entry->Request, &berU );
    if (retU && (ber = heap_alloc( sizeof(*ber) )))
    {
        ber->opaque = (char *)berU;
        *ptr = ber;
        ret = strUtoW( retU );
    }

    ldap_funcs->ldap_memfree( retU );
    return ret;
}

/***********************************************************************
 *      ldap_first_entry     (WLDAP32.@)
 *
 * Get the first entry from a result message.
 *
 * PARAMS
 *  ld  [I] Pointer to an LDAP context.
 *  res [I] Search result message.
 *
 * RETURNS
 *  Success: The first entry.
 *  Failure: NULL
 *
 * NOTES
 *  The returned entry will be freed when the message is freed.
 */
WLDAP32_LDAPMessage * CDECL WLDAP32_ldap_first_entry( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *res )
{
    void *msgU;

    TRACE( "(%p, %p)\n", ld, res );

    if (!ld || !res) return NULL;

    msgU = ldap_funcs->ldap_first_entry( ld->ld, res->Request );
    if (msgU)
    {
        assert( msgU == res->Request );
        return res;
    }

    return NULL;
}

/***********************************************************************
 *      ldap_first_reference     (WLDAP32.@)
 *
 * Get the first reference from a result message.
 *
 * PARAMS
 *  ld  [I] Pointer to an LDAP context.
 *  res [I] Search result message.
 *
 * RETURNS
 *  Success: The first reference.
 *  Failure: NULL
 */
WLDAP32_LDAPMessage * CDECL WLDAP32_ldap_first_reference( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *res )
{
    void *msgU;

    TRACE( "(%p, %p)\n", ld, res );

    if (!ld) return NULL;

    msgU = ldap_funcs->ldap_first_reference( ld->ld, res->Request );
    if (msgU)
    {
        assert( msgU == res->Request );
        return res;
    }

    return NULL;
}

/***********************************************************************
 *      ldap_memfreeA     (WLDAP32.@)
 *
 * See ldap_memfreeW.
 */
void CDECL ldap_memfreeA( char *block )
{
    TRACE( "(%p)\n", block );
    strfreeA( block );
}

/***********************************************************************
 *      ldap_memfreeW     (WLDAP32.@)
 *
 * Free a block of memory.
 *
 * PARAMS
 *  block [I] Pointer to memory block to be freed.
 */
void CDECL ldap_memfreeW( WCHAR *block )
{
    TRACE( "(%p)\n", block );
    strfreeW( block );
}

/***********************************************************************
 *      ldap_msgfree     (WLDAP32.@)
 *
 * Free a message.
 *
 * PARAMS
 *  res [I] Message to be freed.
 */
ULONG CDECL WLDAP32_ldap_msgfree( WLDAP32_LDAPMessage *res )
{
    WLDAP32_LDAPMessage *entry, *list = res;

    TRACE( "(%p)\n", res );

    if (!res) return WLDAP32_LDAP_SUCCESS;

    ldap_funcs->ldap_msgfree( res->Request );
    while (list)
    {
        entry = list;
        list = entry->lm_next;
        heap_free( entry );
    }

    return WLDAP32_LDAP_SUCCESS;
}

/***********************************************************************
 *      ldap_next_attributeA     (WLDAP32.@)
 *
 * See ldap_next_attributeW.
 */
char * CDECL ldap_next_attributeA( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *entry, WLDAP32_BerElement *ptr )
{
    char *ret = NULL;
    WCHAR *retW;

    TRACE( "(%p, %p, %p)\n", ld, entry, ptr );

    if (!ld || !entry || !ptr) return NULL;

    retW = ldap_next_attributeW( ld, entry->Request, ptr );
    if (retW)
    {
        ret = strWtoA( retW );
        ldap_memfreeW( retW );
    }

    return ret;
}

/***********************************************************************
 *      ldap_next_attributeW     (WLDAP32.@)
 *
 * Get the next attribute for a given entry.
 *
 * PARAMS
 *  ld    [I]   Pointer to an LDAP context.
 *  entry [I]   Entry to retrieve attribute for.
 *  ptr   [I/O] Position pointer.
 *
 * RETURNS
 *  Success: The name of the next attribute.
 *  Failure: NULL
 *
 * NOTES
 *  Free the returned string after each iteration with ldap_memfree.
 *  When done iterating and when ptr != NULL, call ber_free( ptr, 0 ).
 */
WCHAR * CDECL ldap_next_attributeW( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *entry, WLDAP32_BerElement *ptr )
{
    WCHAR *ret = NULL;
    char *retU;

    TRACE( "(%p, %p, %p)\n", ld, entry, ptr );

    if (!ld || !entry || !ptr) return NULL;

    retU = ldap_funcs->ldap_next_attribute( ld->ld, entry->Request, ptr->opaque );
    if (retU)
    {
        ret = strUtoW( retU );
        ldap_funcs->ldap_memfree( retU );
    }

    return ret;
}

/***********************************************************************
 *      ldap_next_entry     (WLDAP32.@)
 *
 * Get the next entry from a result message.
 *
 * PARAMS
 *  ld    [I] Pointer to an LDAP context.
 *  entry [I] Entry returned by a previous call.
 *
 * RETURNS
 *  Success: The next entry.
 *  Failure: NULL
 *
 * NOTES
 *  The returned entry will be freed when the message is freed.
 */
WLDAP32_LDAPMessage * CDECL WLDAP32_ldap_next_entry( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *entry )
{
    WLDAP32_LDAPMessage *msg = NULL;
    void *msgU;

    TRACE( "(%p, %p)\n", ld, entry );

    if (!ld || !entry) return NULL;

    if (entry->lm_next) return entry->lm_next;

    msgU = ldap_funcs->ldap_next_entry( ld->ld, entry->Request );
    if (msgU && (msg = heap_alloc_zero( sizeof(*msg) )))
    {
        msg->Request = msgU;
        entry->lm_next = msg;
    }

    return msg;
}

/***********************************************************************
 *      ldap_next_reference     (WLDAP32.@)
 *
 * Get the next reference from a result message.
 *
 * PARAMS
 *  ld    [I] Pointer to an LDAP context.
 *  entry [I] Entry returned by a previous call.
 *
 * RETURNS
 *  Success: The next reference.
 *  Failure: NULL
 *
 * NOTES
 *  The returned entry will be freed when the message is freed.
 */
WLDAP32_LDAPMessage * CDECL WLDAP32_ldap_next_reference( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *entry )
{
    WLDAP32_LDAPMessage *msg = NULL;
    void *msgU;

    TRACE( "(%p, %p)\n", ld, entry );

    if (!ld || !entry) return NULL;

    if (entry->lm_next) return entry->lm_next;

    msgU = ldap_funcs->ldap_next_reference( ld->ld, entry->Request );
    if (msgU && (msg = heap_alloc_zero( sizeof(*msg) )))
    {
        msg->Request = msgU;
        entry->lm_next = msg;
    }

    return msg;
}

/***********************************************************************
 *      ldap_result     (WLDAP32.@)
 *
 * Get the result of an asynchronous operation.
 *
 * PARAMS
 *  ld      [I] Pointer to an LDAP context.
 *  msgid   [I] Message ID of the operation.
 *  all     [I] How many results should be returned?
 *  timeout [I] How long to wait for the results?
 *  res     [O] Result message for the operation.
 *
 * RETURNS
 *  Success: One of the following values:
 *
 *   LDAP_RES_ADD
 *   LDAP_RES_BIND
 *   LDAP_RES_COMPARE
 *   LDAP_RES_DELETE
 *   LDAP_RES_EXTENDED
 *   LDAP_RES_MODIFY
 *   LDAP_RES_MODRDN
 *   LDAP_RES_REFERRAL
 *   LDAP_RES_SEARCH_ENTRY
 *   LDAP_RES_SEARCH_RESULT
 *
 *  Failure: ~0u
 *
 *  This function returns 0 when the timeout has expired.
 *
 * NOTES
 *  A NULL timeout pointer causes the function to block waiting
 *  for results to arrive. A timeout value of 0 causes the function
 *  to immediately return any available results. Free returned results
 *  with ldap_msgfree.
 */
ULONG CDECL WLDAP32_ldap_result( WLDAP32_LDAP *ld, ULONG msgid, ULONG all, struct l_timeval *timeout,
    WLDAP32_LDAPMessage **res )
{
    WLDAP32_LDAPMessage *msg;
    struct timevalU timeval;
    void *msgU = NULL;
    ULONG ret;

    TRACE( "(%p, 0x%08x, 0x%08x, %p, %p)\n", ld, msgid, all, timeout, res );

    if (!ld || !res || msgid == ~0u) return ~0u;

    if (timeout)
    {
        timeval.tv_sec = timeout->tv_sec;
        timeval.tv_usec = timeout->tv_usec;
    }

    ret = ldap_funcs->ldap_result( ld->ld, msgid, all, timeout ? &timeval : NULL, &msgU );
    if (msgU && (msg = heap_alloc_zero( sizeof(*msg) )))
    {
        msg->Request = msgU;
        *res = msg;
    }

    return ret;
}

/***********************************************************************
 *      LdapUnicodeToUTF8     (WLDAP32.@)
 *
 * Convert a wide character string to a UTF8 string.
 *
 * PARAMS
 *  src    [I] Wide character string to convert.
 *  srclen [I] Size of string to convert, in characters.
 *  dst    [O] Pointer to a buffer that receives the converted string.
 *  dstlen [I] Size of the destination buffer in characters.
 *
 * RETURNS
 *  The number of characters written into the destination buffer.
 *
 * NOTES
 *  Set dstlen to zero to ask for the required buffer size.
 */
int CDECL LdapUnicodeToUTF8( const WCHAR *src, int srclen, char *dst, int dstlen )
{
    return WideCharToMultiByte( CP_UTF8, 0, src, srclen, dst, dstlen, NULL, NULL );
}

/***********************************************************************
 *      LdapUTF8ToUnicode     (WLDAP32.@)
 *
 * Convert a UTF8 string to a wide character string.
 *
 * PARAMS
 *  src    [I] UTF8 string to convert.
 *  srclen [I] Size of string to convert, in characters.
 *  dst    [O] Pointer to a buffer that receives the converted string.
 *  dstlen [I] Size of the destination buffer in characters.
 *
 * RETURNS
 *  The number of characters written into the destination buffer.
 *
 * NOTES
 *  Set dstlen to zero to ask for the required buffer size.
 */
int CDECL LdapUTF8ToUnicode( const char *src, int srclen, WCHAR *dst, int dstlen )
{
    return MultiByteToWideChar( CP_UTF8, 0, src, srclen, dst, dstlen );
}
