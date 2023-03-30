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
 *      ldap_deleteA     (WLDAP32.@)
 */
ULONG CDECL ldap_deleteA( LDAP *ld, char *dn )
{
    ULONG ret;
    WCHAR *dnW = NULL;

    TRACE( "(%p, %s)\n", ld, debugstr_a(dn) );

    if (!ld) return ~0u;
    if (dn && !(dnW = strAtoW( dn ))) return WLDAP32_LDAP_NO_MEMORY;

    ret = ldap_deleteW( ld, dnW );
    free( dnW );
    return ret;
}

/***********************************************************************
 *      ldap_deleteW     (WLDAP32.@)
 */
ULONG CDECL ldap_deleteW( LDAP *ld, WCHAR *dn )
{
    ULONG ret, msg;

    TRACE( "(%p, %s)\n", ld, debugstr_w(dn) );

    ret = ldap_delete_extW( ld, dn, NULL, NULL, &msg );
    if (ret == WLDAP32_LDAP_SUCCESS) return msg;
    return ~0u;
}

/***********************************************************************
 *      ldap_delete_extA     (WLDAP32.@)
 */
ULONG CDECL ldap_delete_extA( LDAP *ld, char *dn, LDAPControlA **serverctrls, LDAPControlA **clientctrls,
                              ULONG *message )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *dnW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    TRACE( "(%p, %s, %p, %p, %p)\n", ld, debugstr_a(dn), serverctrls, clientctrls, message );

    if (!ld || !message) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (serverctrls && !(serverctrlsW = controlarrayAtoW( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsW = controlarrayAtoW( clientctrls ))) goto exit;

    ret = ldap_delete_extW( ld, dnW, serverctrlsW, clientctrlsW, message );

exit:
    free( dnW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );
    return ret;
}

/***********************************************************************
 *      ldap_delete_extW     (WLDAP32.@)
 */
ULONG CDECL ldap_delete_extW( LDAP *ld, WCHAR *dn, LDAPControlW **serverctrls, LDAPControlW **clientctrls,
                              ULONG *message )
{
    ULONG ret;
    char *dnU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;

    TRACE( "(%p, %s, %p, %p, %p)\n", ld, debugstr_w(dn), serverctrls, clientctrls, message );

    if (!ld || !message) return WLDAP32_LDAP_PARAM_ERROR;
    if ((ret = WLDAP32_ldap_connect( ld, NULL ))) return ret;

    ret = WLDAP32_LDAP_NO_MEMORY;
    if (!(dnU = dn ? strWtoU( dn ) : strdup( "" ))) goto exit;
    if (serverctrls && !(serverctrlsU = controlarrayWtoU( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsU = controlarrayWtoU( clientctrls ))) goto exit;
    else
    {
        ret = map_error( ldap_delete_ext( CTX(ld), dnU, serverctrlsU, clientctrlsU, (int *)message ) );
    }

exit:
    free( dnU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );
    return ret;
}

/***********************************************************************
 *      ldap_delete_ext_sA     (WLDAP32.@)
 */
ULONG CDECL ldap_delete_ext_sA( LDAP *ld, char *dn, LDAPControlA **serverctrls, LDAPControlA **clientctrls )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *dnW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    TRACE( "(%p, %s, %p, %p)\n", ld, debugstr_a(dn), serverctrls, clientctrls );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (serverctrls && !(serverctrlsW = controlarrayAtoW( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsW = controlarrayAtoW( clientctrls ))) goto exit;

    ret = ldap_delete_ext_sW( ld, dnW, serverctrlsW, clientctrlsW );

exit:
    free( dnW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );
    return ret;
}

/***********************************************************************
 *      ldap_delete_ext_sW     (WLDAP32.@)
 */
ULONG CDECL ldap_delete_ext_sW( LDAP *ld, WCHAR *dn, LDAPControlW **serverctrls, LDAPControlW **clientctrls )
{
    ULONG ret;
    char *dnU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;

    TRACE( "(%p, %s, %p, %p)\n", ld, debugstr_w(dn), serverctrls, clientctrls );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    if ((ret = WLDAP32_ldap_connect( ld, NULL ))) return ret;

    ret = WLDAP32_LDAP_NO_MEMORY;
    if (!(dnU = dn ? strWtoU( dn ) : strdup( "" ))) goto exit;
    if (serverctrls && !(serverctrlsU = controlarrayWtoU( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsU = controlarrayWtoU( clientctrls ))) goto exit;
    else
    {
        ret = map_error( ldap_delete_ext_s( CTX(ld), dnU, serverctrlsU, clientctrlsU ) );
    }

exit:
    free( dnU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );
    return ret;
}

/***********************************************************************
 *      ldap_delete_sA     (WLDAP32.@)
 */
ULONG CDECL ldap_delete_sA( LDAP *ld, char *dn )
{
    ULONG ret;
    WCHAR *dnW = NULL;

    TRACE( "(%p, %s)\n", ld, debugstr_a(dn) );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    if (dn && !(dnW = strAtoW( dn ))) return WLDAP32_LDAP_NO_MEMORY;

    ret = ldap_delete_sW( ld, dnW );
    free( dnW );
    return ret;
}

/***********************************************************************
 *      ldap_delete_sW     (WLDAP32.@)
 */
ULONG CDECL ldap_delete_sW( LDAP *ld, WCHAR *dn )
{
    TRACE( "(%p, %s)\n", ld, debugstr_w(dn) );
    return ldap_delete_ext_sW( ld, dn, NULL, NULL );
}
