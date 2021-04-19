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
 *      ldap_rename_extA     (WLDAP32.@)
 *
 *  See ldap_rename_extW.
 */
ULONG CDECL ldap_rename_extA( LDAP *ld, char *dn, char *newrdn, char *newparent, int delete,
    LDAPControlA **serverctrls, LDAPControlA **clientctrls, ULONG *message )
{
    ULONG ret = LDAP_NO_MEMORY;
    WCHAR *dnW = NULL, *newrdnW = NULL, *newparentW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    TRACE( "(%p, %s, %s, %s, 0x%02x, %p, %p, %p)\n", ld, debugstr_a(dn), debugstr_a(newrdn), debugstr_a(newparent),
           delete, serverctrls, clientctrls, message );

    if (!ld || !message) return LDAP_PARAM_ERROR;

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
 *
 * Change the DN of a directory entry (asynchronous operation).
 *
 * PARAMS
 *  ld          [I] Pointer to an LDAP context.
 *  dn          [I] DN of the entry to change.
 *  newrdn      [I] New RDN for the entry.
 *  newparent   [I] New parent for the entry.
 *  delete      [I] Delete old RDN?
 *  serverctrls [I] Array of LDAP server controls.
 *  clientctrls [I] Array of LDAP client controls.
 *  message     [O] Message ID of the operation.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  Call ldap_result with the message ID to get the result of
 *  the operation. Cancel the operation by calling ldap_abandon
 *  with the message ID.
 */
ULONG CDECL ldap_rename_extW( LDAP *ld, WCHAR *dn, WCHAR *newrdn, WCHAR *newparent, int delete,
    LDAPControlW **serverctrls, LDAPControlW **clientctrls, ULONG *message )
{
    ULONG ret = LDAP_NO_MEMORY;
    char *dnU = NULL, *newrdnU = NULL, *newparentU = NULL;
    LDAPControlU **serverctrlsU = NULL, **clientctrlsU = NULL;

    TRACE( "(%p, %s, %s, %s, 0x%02x, %p, %p, %p)\n", ld, debugstr_w(dn), debugstr_w(newrdn), debugstr_w(newparent),
           delete, serverctrls, clientctrls, message );

    if (!ld || !message) return LDAP_PARAM_ERROR;

    if (dn && !(dnU = strWtoU( dn ))) goto exit;
    if (newrdn && !(newrdnU = strWtoU( newrdn ))) goto exit;
    if (newparent && !(newparentU = strWtoU( newparent ))) goto exit;
    if (serverctrls && !(serverctrlsU = controlarrayWtoU( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsU = controlarrayWtoU( clientctrls ))) goto exit;

    ret = map_error( ldap_funcs->fn_ldap_rename( CTX(ld), dnU, newrdnU, newparentU, delete, serverctrlsU, clientctrlsU,
                                                 message ) );
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
 *
 *  See ldap_rename_ext_sW.
 */
ULONG CDECL ldap_rename_ext_sA( LDAP *ld, char *dn, char *newrdn, char *newparent, int delete,
    LDAPControlA **serverctrls, LDAPControlA **clientctrls )
{
    ULONG ret = LDAP_NO_MEMORY;
    WCHAR *dnW = NULL, *newrdnW = NULL, *newparentW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    TRACE( "(%p, %s, %s, %s, 0x%02x, %p, %p)\n", ld, debugstr_a(dn), debugstr_a(newrdn), debugstr_a(newparent),
           delete, serverctrls, clientctrls );

    if (!ld) return LDAP_PARAM_ERROR;

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
 *
 * Change the DN of a directory entry (synchronous operation).
 *
 * PARAMS
 *  ld          [I] Pointer to an LDAP context.
 *  dn          [I] DN of the entry to change.
 *  newrdn      [I] New RDN for the entry.
 *  newparent   [I] New parent for the entry.
 *  delete      [I] Delete old RDN?
 *  serverctrls [I] Array of LDAP server controls.
 *  clientctrls [I] Array of LDAP client controls.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 */
ULONG CDECL ldap_rename_ext_sW( LDAP *ld, WCHAR *dn, WCHAR *newrdn, WCHAR *newparent, int delete,
    LDAPControlW **serverctrls, LDAPControlW **clientctrls )
{
    ULONG ret = LDAP_PARAM_ERROR;
    char *dnU = NULL, *newrdnU = NULL, *newparentU = NULL;
    LDAPControlU **serverctrlsU = NULL, **clientctrlsU = NULL;

    TRACE( "(%p, %s, %s, %s, 0x%02x, %p, %p)\n", ld, debugstr_w(dn), debugstr_w(newrdn), debugstr_w(newparent),
           delete, serverctrls, clientctrls );

    if (!ld) return LDAP_PARAM_ERROR;

    if (dn && !(dnU = strWtoU( dn ))) goto exit;
    if (newrdn && !(newrdnU = strWtoU( newrdn ))) goto exit;
    if (newparent && !(newparentU = strWtoU( newparent ))) goto exit;
    if (serverctrls && !(serverctrlsU = controlarrayWtoU( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsU = controlarrayWtoU( clientctrls ))) goto exit;

    ret = map_error( ldap_funcs->fn_ldap_rename_s( CTX(ld), dnU, newrdnU, newparentU, delete, serverctrlsU,
                                                   clientctrlsU ) );
exit:
    free( dnU );
    free( newrdnU );
    free( newparentU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );
    return ret;
}
