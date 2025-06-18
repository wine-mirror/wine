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

#include "wine/debug.h"
#include "winldap_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

/***********************************************************************
 *      ldap_count_values_len     (WLDAP32.@)
 */
ULONG CDECL WLDAP32_ldap_count_values_len( struct WLDAP32_berval **values )
{
    ULONG ret = 0;
    struct WLDAP32_berval **ptr = values;

    TRACE( "(%p)\n", values );

    if (!values) return 0;
    while (*ptr++) ret++;
    return ret;
}

/***********************************************************************
 *      ldap_count_valuesA     (WLDAP32.@)
 */
ULONG CDECL ldap_count_valuesA( char **values )
{
    ULONG ret = 0;
    char **ptr = values;

    TRACE( "(%p)\n", values );

    if (!values) return 0;
    while (*ptr++) ret++;
    return ret;
}

/***********************************************************************
 *      ldap_count_valuesW     (WLDAP32.@)
 */
ULONG CDECL ldap_count_valuesW( WCHAR **values )
{
    ULONG ret = 0;
    WCHAR **p = values;

    TRACE( "(%p)\n", values );

    if (!values) return 0;
    while (*p++) ret++;
    return ret;
}

/***********************************************************************
 *      ldap_get_valuesA     (WLDAP32.@)
 */
char ** CDECL ldap_get_valuesA( LDAP *ld, WLDAP32_LDAPMessage *entry, char *attr )
{
    char **ret;
    WCHAR *attrW = NULL, **retW;

    TRACE( "(%p, %p, %s)\n", ld, entry, debugstr_a(attr) );

    if (!ld || !entry || !attr || !(attrW = strAtoW( attr ))) return NULL;

    retW = ldap_get_valuesW( ld, entry, attrW );
    ret = strarrayWtoA( retW );

    ldap_value_freeW( retW );
    free( attrW );
    return ret;
}

static char *bv2str( struct berval *bv )
{
    char *str = NULL;
    unsigned int len = bv->bv_len;

    if ((str = malloc( len + 1 )))
    {
        memcpy( str, bv->bv_val, len );
        str[len] = '\0';
    }
    return str;
}

static char **bv2str_array( struct berval **bv )
{
    unsigned int len = 0, i = 0;
    struct berval **p = bv;
    char **str;

    while (*p)
    {
        len++;
        p++;
    }
    if (!(str = malloc( (len + 1) * sizeof(char *) ))) return NULL;

    p = bv;
    while (*p)
    {
        str[i] = bv2str( *p );
        if (!str[i])
        {
            while (i > 0) free( str[--i] );
            free( str );
            return NULL;
        }
        i++;
        p++;
    }
    str[i] = NULL;
    return str;
}

/***********************************************************************
 *      ldap_get_valuesW     (WLDAP32.@)
 */
WCHAR ** CDECL ldap_get_valuesW( LDAP *ld, WLDAP32_LDAPMessage *entry, WCHAR *attr )
{
    WCHAR **ret = NULL;
    char *attrU, **retU;
    struct berval **bv;

    TRACE( "(%p, %p, %s)\n", ld, entry, debugstr_w(attr) );

    if (ld && entry && attr && (attrU = strWtoU( attr )))
    {
        if ((bv = ldap_get_values_len( CTX(ld), MSG(entry), attrU )))
        {
            retU = bv2str_array( bv );
            ret = strarrayUtoW( retU );

            ldap_value_free_len( bv );
            strarrayfreeU( retU );
        }
        free( attrU );
    }
    return ret;
}

/***********************************************************************
 *      ldap_get_values_lenA     (WLDAP32.@)
 */
struct WLDAP32_berval ** CDECL ldap_get_values_lenA( LDAP *ld, LDAPMessage *message, char *attr )
{
    WCHAR *attrW;
    struct WLDAP32_berval **ret;

    TRACE( "(%p, %p, %s)\n", ld, message, debugstr_a(attr) );

    if (!ld || !message || !attr || !(attrW = strAtoW( attr ))) return NULL;

    ret = ldap_get_values_lenW( ld, message, attrW );

    free( attrW );
    return ret;
}

/***********************************************************************
 *      ldap_get_values_lenW     (WLDAP32.@)
 */
struct WLDAP32_berval ** CDECL ldap_get_values_lenW( LDAP *ld, WLDAP32_LDAPMessage *message, WCHAR *attr )
{
    char *attrU = NULL;
    struct berval **retU;
    struct WLDAP32_berval **ret = NULL;

    TRACE( "(%p, %p, %s)\n", ld, message, debugstr_w(attr) );

    if (ld && message && attr && (attrU = strWtoU( attr )))
    {
        if ((retU = ldap_get_values_len( CTX(ld), MSG(message), attrU )))
        {
            ret = bvarrayUtoW( retU );
            bvarrayfreeU( retU );
        }
        free( attrU );
    }
    return ret;
}

/***********************************************************************
 *      ldap_value_free_len     (WLDAP32.@)
 */
ULONG CDECL WLDAP32_ldap_value_free_len( struct WLDAP32_berval **values )
{
    TRACE( "(%p)\n", values );

    bvarrayfreeW( values );
    return WLDAP32_LDAP_SUCCESS;
}

/***********************************************************************
 *      ldap_value_freeA     (WLDAP32.@)
 */
ULONG CDECL ldap_value_freeA( char **values )
{
    TRACE( "(%p)\n", values );

    strarrayfreeA( values );
    return WLDAP32_LDAP_SUCCESS;
}

/***********************************************************************
 *      ldap_value_freeW     (WLDAP32.@)
 */
ULONG CDECL ldap_value_freeW( WCHAR **values )
{
    TRACE( "(%p)\n", values );

    strarrayfreeW( values );
    return WLDAP32_LDAP_SUCCESS;
}
