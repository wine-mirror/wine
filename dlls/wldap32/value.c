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
#include "winldap.h"

#include "wine/debug.h"
#include "winldap_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

/***********************************************************************
 *      ldap_count_values_len     (WLDAP32.@)
 *
 * Count the number of values in an array of berval structures.
 *
 * PARAMS
 *  values  [I] Pointer to an array of berval structures.
 *
 * RETURNS
 *  Success: The number of values counted.
 *  Failure: 0
 *
 * NOTES
 *  Call ldap_count_values_len with the result of a call to
 *  ldap_get_values_len.
 */
ULONG CDECL ldap_count_values_len( struct berval **values )
{
    ULONG ret = 0;
    struct berval **ptr = values;

    TRACE( "(%p)\n", values );

    if (!values) return 0;
    while (*ptr++) ret++;
    return ret;
}

/***********************************************************************
 *      ldap_count_valuesA     (WLDAP32.@)
 *
 * See ldap_count_valuesW.
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
 *
 * Count the number of values in a string array.
 *
 * PARAMS
 *  values  [I] Pointer to an array of strings.
 *
 * RETURNS
 *  Success: The number of values counted.
 *  Failure: 0
 *
 * NOTES
 *  Call ldap_count_valuesW with the result of a call to
 *  ldap_get_valuesW.
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
 *
 * See ldap_get_valuesW.
 */
char ** CDECL ldap_get_valuesA( LDAP *ld, LDAPMessage *entry, char *attr )
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

static char *bv2str( struct bervalU *bv )
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

static char **bv2str_array( struct bervalU **bv )
{
    unsigned int len = 0, i = 0;
    struct bervalU **p = bv;
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
 *
 * Retrieve string values for a given attribute.
 *
 * PARAMS
 *  ld     [I] Pointer to an LDAP context.
 *  entry  [I] Entry to retrieve values from.
 *  attr   [I] Attribute to retrieve values for.
 *
 * RETURNS
 *  Success: Pointer to a character array holding the values.
 *  Failure: NULL
 *
 * NOTES
 *  Call ldap_get_valuesW with the result of a call to
 *  ldap_first_entry or ldap_next_entry. Free the returned
 *  array with a call to ldap_value_freeW.
 */
WCHAR ** CDECL ldap_get_valuesW( LDAP *ld, LDAPMessage *entry, WCHAR *attr )
{
    WCHAR **ret = NULL;
    char *attrU, **retU;
    struct bervalU **bv;

    TRACE( "(%p, %p, %s)\n", ld, entry, debugstr_w(attr) );

    if (ld && entry && attr && (attrU = strWtoU( attr )))
    {
        struct ldap_get_values_len_params params = { CTX(ld), MSG(entry), attrU, &bv };

        if (!LDAP_CALL( ldap_get_values_len, &params ))
        {
            retU = bv2str_array( bv );
            ret = strarrayUtoW( retU );

            LDAP_CALL( ldap_value_free_len, bv );
            strarrayfreeU( retU );
        }
        free( attrU );
    }
    return ret;
}

/***********************************************************************
 *      ldap_get_values_lenA     (WLDAP32.@)
 *
 * See ldap_get_values_lenW.
 */
struct berval ** CDECL ldap_get_values_lenA( LDAP *ld, LDAPMessage *message, char *attr )
{
    WCHAR *attrW;
    struct berval **ret;

    TRACE( "(%p, %p, %s)\n", ld, message, debugstr_a(attr) );

    if (!ld || !message || !attr || !(attrW = strAtoW( attr ))) return NULL;

    ret = ldap_get_values_lenW( ld, message, attrW );

    free( attrW );
    return ret;
}

/***********************************************************************
 *      ldap_get_values_lenW     (WLDAP32.@)
 *
 * Retrieve binary values for a given attribute.
 *
 * PARAMS
 *  ld      [I] Pointer to an LDAP context.
 *  message [I] Entry to retrieve values from.
 *  attr    [I] Attribute to retrieve values for.
 *
 * RETURNS
 *  Success: Pointer to a berval array holding the values.
 *  Failure: NULL
 *
 * NOTES
 *  Call ldap_get_values_lenW with the result of a call to
 *  ldap_first_entry or ldap_next_entry. Free the returned
 *  array with a call to ldap_value_free_len.
 */
struct berval ** CDECL ldap_get_values_lenW( LDAP *ld, LDAPMessage *message, WCHAR *attr )
{
    char *attrU = NULL;
    struct bervalU **retU;
    struct berval **ret = NULL;

    TRACE( "(%p, %p, %s)\n", ld, message, debugstr_w(attr) );

    if (ld && message && attr && (attrU = strWtoU( attr )))
    {
        struct ldap_get_values_len_params params = { CTX(ld), MSG(message), attrU, &retU };

        if (!LDAP_CALL( ldap_get_values_len, &params ))
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
 *
 * Free an array of berval structures.
 *
 * PARAMS
 *  values  [I] Array of berval structures.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 */
ULONG CDECL ldap_value_free_len( struct berval **values )
{
    TRACE( "(%p)\n", values );

    bvarrayfreeW( values );
    return LDAP_SUCCESS;
}

/***********************************************************************
 *      ldap_value_freeA     (WLDAP32.@)
 *
 * See ldap_value_freeW.
 */
ULONG CDECL ldap_value_freeA( char **values )
{
    TRACE( "(%p)\n", values );

    strarrayfreeA( values );
    return LDAP_SUCCESS;
}

/***********************************************************************
 *      ldap_value_freeW     (WLDAP32.@)
 *
 * Free an array of string values.
 *
 * PARAMS
 *  values  [I] Array of string values.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 */
ULONG CDECL ldap_value_freeW( WCHAR **values )
{
    TRACE( "(%p)\n", values );

    strarrayfreeW( values );
    return LDAP_SUCCESS;
}
