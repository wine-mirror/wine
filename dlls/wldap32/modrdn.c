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
 *      ldap_modrdnA     (WLDAP32.@)
 */
ULONG CDECL ldap_modrdnA( LDAP *ld, char *dn, char *newdn )
{
    ULONG ret = ~0u;
    WCHAR *dnW = NULL, *newdnW = NULL;

    TRACE( "(%p, %s, %s)\n", ld, debugstr_a(dn), debugstr_a(newdn) );

    if (!ld || !newdn) return ~0u;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (!(newdnW = strAtoW( newdn ))) goto exit;

    ret = ldap_modrdnW( ld, dnW, newdnW );

exit:
    free( dnW );
    free( newdnW );
    return ret;
}

/***********************************************************************
 *      ldap_modrdnW     (WLDAP32.@)
 */
ULONG CDECL ldap_modrdnW( LDAP *ld, WCHAR *dn, WCHAR *newdn )
{
    TRACE( "(%p, %s, %s)\n", ld, debugstr_w(dn), debugstr_w(newdn) );
    return ldap_modrdn2W( ld, dn, newdn, 1 );
}

/***********************************************************************
 *      ldap_modrdn2A     (WLDAP32.@)
 */
ULONG CDECL ldap_modrdn2A( LDAP *ld, char *dn, char *newdn, int delete )
{
    ULONG ret = ~0u;
    WCHAR *dnW = NULL, *newdnW = NULL;

    TRACE( "(%p, %s, %p, 0x%02x)\n", ld, debugstr_a(dn), newdn, delete );

    if (!ld || !newdn) return ~0u;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (!(newdnW = strAtoW( newdn ))) goto exit;

    ret = ldap_modrdn2W( ld, dnW, newdnW, delete );

exit:
    free( dnW );
    free( newdnW );
    return ret;
}

/***********************************************************************
 *      ldap_modrdn2W     (WLDAP32.@)
 */
ULONG CDECL ldap_modrdn2W( LDAP *ld, WCHAR *dn, WCHAR *newdn, int delete )
{
    ULONG ret = ~0u;
    char *dnU = NULL, *newdnU = NULL;
    int msg;

    TRACE( "(%p, %s, %p, 0x%02x)\n", ld, debugstr_w(dn), newdn, delete );

    if (!ld || !newdn) return ~0u;
    if (WLDAP32_ldap_connect( ld, NULL ) != WLDAP32_LDAP_SUCCESS) return ~0u;

    if (dn && !(dnU = strWtoU( dn ))) return ~0u;

    if ((newdnU = strWtoU( newdn )))
    {
        if (ldap_rename( CTX(ld), dnU, newdnU, NULL, delete, NULL, NULL, &msg ) == LDAP_SUCCESS)
            ret = msg;
        free( newdnU );
    }
    free( dnU );
    return ret;
}

/***********************************************************************
 *      ldap_modrdn2_sA     (WLDAP32.@)
 */
ULONG CDECL ldap_modrdn2_sA( LDAP *ld, char *dn, char *newdn, int delete )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *dnW = NULL, *newdnW = NULL;

    TRACE( "(%p, %s, %p, 0x%02x)\n", ld, debugstr_a(dn), newdn, delete );

    if (!ld || !newdn) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (!(newdnW = strAtoW( newdn ))) goto exit;

    ret = ldap_modrdn2_sW( ld, dnW, newdnW, delete );

exit:
    free( dnW );
    free( newdnW );
    return ret;
}

/***********************************************************************
 *      ldap_modrdn2_sW     (WLDAP32.@)
 */
ULONG CDECL ldap_modrdn2_sW( LDAP *ld, WCHAR *dn, WCHAR *newdn, int delete )
{
    ULONG ret;
    char *dnU = NULL, *newdnU = NULL;

    TRACE( "(%p, %s, %p, 0x%02x)\n", ld, debugstr_w(dn), newdn, delete );

    if (!ld || !newdn) return WLDAP32_LDAP_PARAM_ERROR;
    if ((ret = WLDAP32_ldap_connect( ld, NULL ))) return ret;

    if (dn && !(dnU = strWtoU( dn ))) return WLDAP32_LDAP_NO_MEMORY;

    ret = WLDAP32_LDAP_NO_MEMORY;
    if ((newdnU = strWtoU( newdn )))
    {
        ret = map_error( ldap_rename_s( CTX(ld), dnU, newdnU, NULL, delete, NULL, NULL ) );
        free( newdnU );
    }
    free( dnU );
    return ret;
}

/***********************************************************************
 *      ldap_modrdn_sA     (WLDAP32.@)
 */
ULONG CDECL ldap_modrdn_sA( LDAP *ld, char *dn, char *newdn )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *dnW = NULL, *newdnW = NULL;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_a(dn), newdn );

    if (!ld || !newdn) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (!(newdnW = strAtoW( newdn ))) goto exit;

    ret = ldap_modrdn_sW( ld, dnW, newdnW );

exit:
    free( dnW );
    free( newdnW );
    return ret;
}

/***********************************************************************
 *      ldap_modrdn_sW     (WLDAP32.@)
 */
ULONG CDECL ldap_modrdn_sW( LDAP *ld, WCHAR *dn, WCHAR *newdn )
{
    TRACE( "(%p, %s, %p)\n", ld, debugstr_w(dn), newdn );
    return ldap_modrdn2_sW( ld, dn, newdn, 1 );
}
