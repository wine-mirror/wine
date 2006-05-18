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

#include "config.h"

#include "wine/port.h"
#include "wine/debug.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"

#ifdef HAVE_LDAP_H
#include <ldap.h>
#else
#define LDAP_NOT_SUPPORTED  0x5c
#endif

#include "winldap_private.h"
#include "wldap32.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

ULONG ldap_rename_extA( WLDAP32_LDAP *ld, PCHAR dn, PCHAR newrdn,
    PCHAR newparent, INT delete, PLDAPControlA *serverctrls,
    PLDAPControlA *clientctrls, ULONG *message )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW = NULL, *newrdnW = NULL, *newparentW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %s, %s, 0x%02x, %p, %p, %p)\n", ld, debugstr_a(dn),
           debugstr_a(newrdn), debugstr_a(newparent), delete,
           serverctrls, clientctrls, message );

    if (!ld || !message) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnW = strAtoW( dn );
        if (!dnW) goto exit;
    }
    if (newrdn) {
        newrdnW = strAtoW( newrdn );
        if (!newrdnW) goto exit;
    }
    if (newparent) {
        newparentW = strAtoW( newparent );
        if (!newparentW) goto exit;
    }
    if (serverctrls) {
        serverctrlsW = controlarrayAtoW( serverctrls );
        if (!serverctrlsW) goto exit;
    }
    if (clientctrls) {
        clientctrlsW = controlarrayAtoW( clientctrls );
        if (!clientctrlsW) goto exit;
    }

    ret = ldap_rename_extW( ld, dnW, newrdnW, newparentW, delete,
                            serverctrlsW, clientctrlsW, message );

exit:
    strfreeW( dnW );
    strfreeW( newrdnW );
    strfreeW( newparentW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );

#endif
    return ret;
}

ULONG ldap_rename_extW( WLDAP32_LDAP *ld, PWCHAR dn, PWCHAR newrdn,
    PWCHAR newparent, INT delete, PLDAPControlW *serverctrls,
    PLDAPControlW *clientctrls, ULONG *message )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU = NULL, *newrdnU = NULL, *newparentU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %s, %s, 0x%02x, %p, %p, %p)\n", ld, debugstr_w(dn),
           debugstr_w(newrdn), debugstr_w(newparent), delete,
           serverctrls, clientctrls, message );

    if (!ld || !message) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnU = strWtoU( dn );
        if (!dnU) goto exit;
    }
    if (newrdn) {
        newrdnU = strWtoU( newrdn );
        if (!newrdnU) goto exit;
    }
    if (newparent) {
        newparentU = strWtoU( newparent );
        if (!newparentU) goto exit;
    }
    if (serverctrls) {
        serverctrlsU = controlarrayWtoU( serverctrls );
        if (!serverctrlsU) goto exit;
    }
    if (clientctrls) {
        clientctrlsU = controlarrayWtoU( clientctrls );
        if (!clientctrlsU) goto exit;
    }

    ret = ldap_rename( ld, dn ? dnU : "", newrdn ? newrdnU : "", newparentU,
                       delete, serverctrlsU, clientctrlsU, (int *)message );

exit:
    strfreeU( dnU );
    strfreeU( newrdnU );
    strfreeU( newparentU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );

#endif
    return ret;
}

ULONG ldap_rename_ext_sA( WLDAP32_LDAP *ld, PCHAR dn, PCHAR newrdn,
    PCHAR newparent, INT delete, PLDAPControlA *serverctrls,
    PLDAPControlA *clientctrls )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW = NULL, *newrdnW = NULL, *newparentW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %s, %s, 0x%02x, %p, %p)\n", ld, debugstr_a(dn),
           debugstr_a(newrdn), debugstr_a(newparent), delete,
           serverctrls, clientctrls );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnW = strAtoW( dn );
        if (!dnW) goto exit;
    }
    if (newrdn) {
        newrdnW = strAtoW( newrdn );
        if (!newrdnW) goto exit;
    }
    if (newparent) {
        newparentW = strAtoW( newparent );
        if (!newparentW) goto exit;
    }
    if (serverctrls) {
        serverctrlsW = controlarrayAtoW( serverctrls );
        if (!serverctrlsW) goto exit;
    }
    if (clientctrls) {
        clientctrlsW = controlarrayAtoW( clientctrls );
        if (!clientctrlsW) goto exit;
    }

    ret = ldap_rename_ext_sW( ld, dnW, newrdnW, newparentW, delete,
                              serverctrlsW, clientctrlsW );

exit:
    strfreeW( dnW );
    strfreeW( newrdnW );
    strfreeW( newparentW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );

#endif
    return ret;
}

ULONG ldap_rename_ext_sW( WLDAP32_LDAP *ld, PWCHAR dn, PWCHAR newrdn,
    PWCHAR newparent, INT delete, PLDAPControlW *serverctrls,
    PLDAPControlW *clientctrls )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU = NULL, *newrdnU = NULL, *newparentU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %s, %s, 0x%02x, %p, %p)\n", ld, debugstr_w(dn),
           debugstr_w(newrdn), debugstr_w(newparent), delete,
           serverctrls, clientctrls );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnU = strWtoU( dn );
        if (!dnU) goto exit;
    }
    if (newrdn) {
        newrdnU = strWtoU( newrdn );
        if (!newrdnU) goto exit;
    }
    if (newparent) {
        newparentU = strWtoU( newparent );
        if (!newparentU) goto exit;
    }
    if (serverctrls) {
        serverctrlsU = controlarrayWtoU( serverctrls );
        if (!serverctrlsU) goto exit;
    }
    if (clientctrls) {
        clientctrlsU = controlarrayWtoU( clientctrls );
        if (!clientctrlsU) goto exit;
    }

    ret = ldap_rename_s( ld, dn ? dnU : "", newrdn ? newrdnU : "", newparentU,
                         delete, serverctrlsU, clientctrlsU );

exit:
    strfreeU( dnU );
    strfreeU( newrdnU );
    strfreeU( newparentU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );

#endif
    return ret;
}
