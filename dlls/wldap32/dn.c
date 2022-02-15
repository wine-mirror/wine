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
#include "winldap.h"

#include "wine/debug.h"
#include "winldap_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

/***********************************************************************
 *      ldap_dn2ufnA     (WLDAP32.@)
 *
 * See ldap_dn2ufnW.
 */
char * CDECL ldap_dn2ufnA( char *dn )
{
    char *ret;
    WCHAR *dnW, *retW;

    TRACE( "(%s)\n", debugstr_a(dn) );

    if (!(dnW = strAtoW( dn ))) return NULL;

    retW = ldap_dn2ufnW( dnW );
    ret = strWtoA( retW );

    free( dnW );
    ldap_memfreeW( retW );
    return ret;
}

/***********************************************************************
 *      ldap_dn2ufnW     (WLDAP32.@)
 *
 * Convert a DN to a user-friendly name.
 *
 * PARAMS
 *  dn  [I] DN to convert.
 *
 * RETURNS
 *  Success: Pointer to a string containing the user-friendly name.
 *  Failure: NULL
 *
 * NOTES
 *  Free the string with ldap_memfree.
 */
WCHAR * CDECL ldap_dn2ufnW( WCHAR *dn )
{
    WCHAR *ret = NULL;
    char *dnU, *retU;

    TRACE( "(%s)\n", debugstr_w(dn) );

    if ((dnU = strWtoU( dn )))
    {
        struct ldap_dn2ufn_params params = { dnU, &retU };
        LDAP_CALL( ldap_dn2ufn, &params );

        ret = strUtoW( retU );

        free( dnU );
        LDAP_CALL( ldap_memfree, retU );
    }
    return ret;
}

/***********************************************************************
 *      ldap_explode_dnA     (WLDAP32.@)
 *
 * See ldap_explode_dnW.
 */
char ** CDECL ldap_explode_dnA( char *dn, ULONG notypes )
{
    char **ret;
    WCHAR *dnW, **retW;

    TRACE( "(%s, %#lx)\n", debugstr_a(dn), notypes );

    if (!(dnW = strAtoW( dn ))) return NULL;

    retW = ldap_explode_dnW( dnW, notypes );
    ret = strarrayWtoA( retW );

    free( dnW );
    ldap_value_freeW( retW );
    return ret;
}

/***********************************************************************
 *      ldap_explode_dnW     (WLDAP32.@)
 *
 * Break up a DN into its components.
 *
 * PARAMS
 *  dn       [I] DN to break up.
 *  notypes  [I] Remove attribute type information from the components.
 *
 * RETURNS
 *  Success: Pointer to a NULL-terminated array that contains the DN
 *           components.
 *  Failure: NULL
 *
 * NOTES
 *  Free the string array with ldap_value_free.
 */
WCHAR ** CDECL ldap_explode_dnW( WCHAR *dn, ULONG notypes )
{
    WCHAR **ret = NULL;
    char *dnU, **retU;

    TRACE( "(%s, %#lx)\n", debugstr_w(dn), notypes );

    if ((dnU = strWtoU( dn )))
    {
        struct ldap_explode_dn_params params = { dnU, notypes, &retU };
        LDAP_CALL( ldap_explode_dn, &params );
        ret = strarrayUtoW( retU );

        free( dnU );
        LDAP_CALL( ldap_memvfree, retU );
    }
    return ret;
}

/***********************************************************************
 *      ldap_get_dnA     (WLDAP32.@)
 *
 * See ldap_get_dnW.
 */
char * CDECL ldap_get_dnA( LDAP *ld, LDAPMessage *entry )
{
    char *ret;
    WCHAR *retW;

    TRACE( "(%p, %p)\n", ld, entry );

    if (!ld || !entry) return NULL;

    retW = ldap_get_dnW( ld, entry );

    ret = strWtoA( retW );
    ldap_memfreeW( retW );
    return ret;
}

/***********************************************************************
 *      ldap_get_dnW     (WLDAP32.@)
 *
 * Retrieve the DN from a given LDAP message.
 *
 * PARAMS
 *  ld     [I] Pointer to an LDAP context.
 *  entry  [I] LDAPMessage structure to retrieve the DN from.
 *
 * RETURNS
 *  Success: Pointer to a string that contains the DN.
 *  Failure: NULL
 *
 * NOTES
 *  Free the string with ldap_memfree.
 */
WCHAR * CDECL ldap_get_dnW( LDAP *ld, LDAPMessage *entry )
{
    WCHAR *ret = NULL;
    char *retU;

    TRACE( "(%p, %p)\n", ld, entry );

    if (ld && entry)
    {
        struct ldap_get_dn_params params = { CTX(ld), MSG(entry), &retU };
        LDAP_CALL( ldap_get_dn, &params );

        ret = strUtoW( retU );
        LDAP_CALL( ldap_memfree, retU );
    }
    return ret;
}

/***********************************************************************
 *      ldap_ufn2dnA     (WLDAP32.@)
 *
 * See ldap_ufn2dnW.
 */
ULONG CDECL ldap_ufn2dnA( char *ufn, char **dn )
{
    ULONG ret;
    WCHAR *ufnW = NULL, *dnW = NULL;

    TRACE( "(%s, %p)\n", debugstr_a(ufn), dn );

    if (!dn) return LDAP_PARAM_ERROR;

    *dn = NULL;
    if (ufn && !(ufnW = strAtoW( ufn ))) return LDAP_NO_MEMORY;

    ret = ldap_ufn2dnW( ufnW, &dnW );
    if (dnW)
    {
        char *str;
        if (!(str = strWtoA( dnW ))) ret = LDAP_NO_MEMORY;
        else *dn = str;
    }

    free( ufnW );
    ldap_memfreeW( dnW );
    return ret;
}

/***********************************************************************
 *      ldap_ufn2dnW     (WLDAP32.@)
 *
 * Convert a user-friendly name to a DN.
 *
 * PARAMS
 *  ufn  [I] User-friendly name to convert.
 *  dn   [O] Receives a pointer to a string containing the DN.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  Free the string with ldap_memfree.
 */
ULONG CDECL ldap_ufn2dnW( WCHAR *ufn, WCHAR **dn )
{
    ULONG ret = LDAP_SUCCESS;
    char *ufnU = NULL;

    TRACE( "(%s, %p)\n", debugstr_w(ufn), dn );

    if (!dn) return LDAP_PARAM_ERROR;

    *dn = NULL;
    if (ufn)
    {
        WCHAR *str;
        if (!(ufnU = strWtoU( ufn ))) return LDAP_NO_MEMORY;

        /* FIXME: do more than just a copy */
        if (!(str = strUtoW( ufnU ))) ret = LDAP_NO_MEMORY;
        else *dn = str;
    }

    free( ufnU );
    return ret;
}
