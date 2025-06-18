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
 *      ldap_rename_extA     (WLDAP32.@)
 */
ULONG CDECL ldap_rename_extA( LDAP *ld, char *dn, char *newrdn, char *newparent, int delete,
                              LDAPControlA **serverctrls, LDAPControlA **clientctrls, ULONG *message )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *dnW = NULL, *newrdnW = NULL, *newparentW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    TRACE( "(%p, %s, %s, %s, 0x%02x, %p, %p, %p)\n", ld, debugstr_a(dn), debugstr_a(newrdn), debugstr_a(newparent),
           delete, serverctrls, clientctrls, message );

    if (!ld || !message) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (newrdn && !(newrdnW = strAtoW( newrdn ))) goto exit;
    if (newparent && !(newparentW = strAtoW( newparent ))) goto exit;
    if (serverctrls && !(serverctrlsW = controlarrayAtoW( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsW = controlarrayAtoW( clientctrls ))) goto exit;

    ret = ldap_rename_extW( ld, dnW, newrdnW, newparentW, delete, serverctrlsW, clientctrlsW, message );

exit:
    free( dnW );
    free( newrdnW );
    free( newparentW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );
    return ret;
}

/***********************************************************************
 *      ldap_rename_extW     (WLDAP32.@)
 */
ULONG CDECL ldap_rename_extW( LDAP *ld, WCHAR *dn, WCHAR *newrdn, WCHAR *newparent, int delete,
                              LDAPControlW **serverctrls, LDAPControlW **clientctrls, ULONG *message )
{
    ULONG ret;
    char *dnU = NULL, *newrdnU = NULL, *newparentU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;

    TRACE( "(%p, %s, %s, %s, 0x%02x, %p, %p, %p)\n", ld, debugstr_w(dn), debugstr_w(newrdn), debugstr_w(newparent),
           delete, serverctrls, clientctrls, message );

    if (!ld || !message) return WLDAP32_LDAP_PARAM_ERROR;
    if ((ret = WLDAP32_ldap_connect( ld, NULL ))) return ret;

    ret = WLDAP32_LDAP_NO_MEMORY;
    if (dn && !(dnU = strWtoU( dn ))) goto exit;
    if (newrdn && !(newrdnU = strWtoU( newrdn ))) goto exit;
    if (newparent && !(newparentU = strWtoU( newparent ))) goto exit;
    if (serverctrls && !(serverctrlsU = controlarrayWtoU( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsU = controlarrayWtoU( clientctrls ))) goto exit;
    else
    {
        ret = map_error( ldap_rename( CTX(ld), dnU, newrdnU, newparentU, delete, serverctrlsU, clientctrlsU,
                                      (int *)message ) );
    }
exit:
    free( dnU );
    free( newrdnU );
    free( newparentU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );
    return ret;
}

/***********************************************************************
 *      ldap_rename_ext_sA     (WLDAP32.@)
 */
ULONG CDECL ldap_rename_ext_sA( LDAP *ld, char *dn, char *newrdn, char *newparent, int delete,
                                LDAPControlA **serverctrls, LDAPControlA **clientctrls )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *dnW = NULL, *newrdnW = NULL, *newparentW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    TRACE( "(%p, %s, %s, %s, 0x%02x, %p, %p)\n", ld, debugstr_a(dn), debugstr_a(newrdn), debugstr_a(newparent),
           delete, serverctrls, clientctrls );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (newrdn && !(newrdnW = strAtoW( newrdn ))) goto exit;
    if (newparent && !(newparentW = strAtoW( newparent ))) goto exit;
    if (serverctrls && !(serverctrlsW = controlarrayAtoW( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsW = controlarrayAtoW( clientctrls ))) goto exit;

    ret = ldap_rename_ext_sW( ld, dnW, newrdnW, newparentW, delete, serverctrlsW, clientctrlsW );

exit:
    free( dnW );
    free( newrdnW );
    free( newparentW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );
    return ret;
}

/***********************************************************************
 *      ldap_rename_ext_sW     (WLDAP32.@)
 */
ULONG CDECL ldap_rename_ext_sW( LDAP *ld, WCHAR *dn, WCHAR *newrdn, WCHAR *newparent, int delete,
                                LDAPControlW **serverctrls, LDAPControlW **clientctrls )
{
    ULONG ret;
    char *dnU = NULL, *newrdnU = NULL, *newparentU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;

    TRACE( "(%p, %s, %s, %s, 0x%02x, %p, %p)\n", ld, debugstr_w(dn), debugstr_w(newrdn), debugstr_w(newparent),
           delete, serverctrls, clientctrls );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    if ((ret = WLDAP32_ldap_connect( ld, NULL ))) return ret;

    ret = WLDAP32_LDAP_NO_MEMORY;
    if (dn && !(dnU = strWtoU( dn ))) goto exit;
    if (newrdn && !(newrdnU = strWtoU( newrdn ))) goto exit;
    if (newparent && !(newparentU = strWtoU( newparent ))) goto exit;
    if (serverctrls && !(serverctrlsU = controlarrayWtoU( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsU = controlarrayWtoU( clientctrls ))) goto exit;
    else
    {
        ret = map_error( ldap_rename_s( CTX(ld), dnU, newrdnU, newparentU, delete, serverctrlsU, clientctrlsU ) );
    }
exit:
    free( dnU );
    free( newrdnU );
    free( newparentU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );
    return ret;
}
