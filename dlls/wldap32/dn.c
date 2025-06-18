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
#include "winldap_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

/***********************************************************************
 *      ldap_dn2ufnA     (WLDAP32.@)
 */
char * CDECL ldap_dn2ufnA( char *dn )
{
    char *ret = NULL;
    WCHAR *dnW, *retW;

    TRACE( "(%s)\n", debugstr_a(dn) );

    if (!(dnW = strAtoW( dn ))) return NULL;
    if ((retW = ldap_dn2ufnW( dnW ))) ret = strWtoA( retW );
    free( dnW );
    ldap_memfreeW( retW );
    return ret;
}

/***********************************************************************
 *      ldap_dn2ufnW     (WLDAP32.@)
 */
WCHAR * CDECL ldap_dn2ufnW( WCHAR *dn )
{
    WCHAR *ret = NULL;
    char *dnU, *retU;

    TRACE( "(%s)\n", debugstr_w(dn) );

    if (!(dnU = strWtoU( dn ))) return NULL;
    if ((retU = ldap_dn2ufn( dnU ))) ret = strUtoW( retU );
    free( dnU );
    ldap_memfree( retU );
    return ret;
}

/***********************************************************************
 *      ldap_explode_dnA     (WLDAP32.@)
 */
char ** CDECL ldap_explode_dnA( char *dn, ULONG notypes )
{
    char **ret = NULL;
    WCHAR *dnW, **retW;

    TRACE( "(%s, %#lx)\n", debugstr_a(dn), notypes );

    if (!(dnW = strAtoW( dn ))) return NULL;
    if ((retW = ldap_explode_dnW( dnW, notypes ))) ret = strarrayWtoA( retW );
    free( dnW );
    ldap_value_freeW( retW );
    return ret;
}

/***********************************************************************
 *      ldap_explode_dnW     (WLDAP32.@)
 */
WCHAR ** CDECL ldap_explode_dnW( WCHAR *dn, ULONG notypes )
{
    WCHAR **ret = NULL;
    char *dnU, **retU;

    TRACE( "(%s, %#lx)\n", debugstr_w(dn), notypes );

    if (!(dnU = strWtoU( dn ))) return NULL;
    if ((retU = ldap_explode_dn( dnU, notypes ))) ret = strarrayUtoW( retU );
    free( dnU );
    ldap_memvfree( (void **)retU );
    return ret;
}

/***********************************************************************
 *      ldap_get_dnA     (WLDAP32.@)
 */
char * CDECL ldap_get_dnA( LDAP *ld, LDAPMessage *entry )
{
    char *ret = NULL;
    WCHAR *retW;

    TRACE( "(%p, %p)\n", ld, entry );

    if (!ld || !entry) return NULL;

    if ((retW = ldap_get_dnW( ld, entry ))) ret = strWtoA( retW );
    ldap_memfreeW( retW );
    return ret;
}

/***********************************************************************
 *      ldap_get_dnW     (WLDAP32.@)
 */
WCHAR * CDECL ldap_get_dnW( LDAP *ld, LDAPMessage *entry )
{
    WCHAR *ret = NULL;
    char *retU;

    TRACE( "(%p, %p)\n", ld, entry );

    if (ld && entry)
    {
        if ((retU = ldap_get_dn( CTX(ld), MSG(entry) ))) ret = strUtoW( retU );
        ldap_memfree( retU );
    }
    return ret;
}

/***********************************************************************
 *      ldap_ufn2dnA     (WLDAP32.@)
 */
ULONG CDECL ldap_ufn2dnA( char *ufn, char **dn )
{
    ULONG ret;
    WCHAR *ufnW = NULL, *dnW = NULL;

    TRACE( "(%s, %p)\n", debugstr_a(ufn), dn );

    if (!dn) return WLDAP32_LDAP_PARAM_ERROR;

    *dn = NULL;
    if (ufn && !(ufnW = strAtoW( ufn ))) return WLDAP32_LDAP_NO_MEMORY;

    ret = ldap_ufn2dnW( ufnW, &dnW );
    if (dnW)
    {
        char *str;
        if (!(str = strWtoA( dnW ))) ret = WLDAP32_LDAP_NO_MEMORY;
        else *dn = str;
    }

    free( ufnW );
    ldap_memfreeW( dnW );
    return ret;
}

/***********************************************************************
 *      ldap_ufn2dnW     (WLDAP32.@)
 */
ULONG CDECL ldap_ufn2dnW( WCHAR *ufn, WCHAR **dn )
{
    ULONG ret = WLDAP32_LDAP_SUCCESS;
    char *ufnU = NULL;

    TRACE( "(%s, %p)\n", debugstr_w(ufn), dn );

    if (!dn) return WLDAP32_LDAP_PARAM_ERROR;

    *dn = NULL;
    if (ufn)
    {
        WCHAR *str;
        if (!(ufnU = strWtoU( ufn ))) return WLDAP32_LDAP_NO_MEMORY;

        /* FIXME: do more than just a copy */
        if (!(str = strUtoW( ufnU ))) ret = WLDAP32_LDAP_NO_MEMORY;
        else *dn = str;
    }

    free( ufnU );
    return ret;
}
