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
 *      ldap_addA     (WLDAP32.@)
 */
ULONG CDECL ldap_addA( LDAP *ld, char *dn, LDAPModA **attrs )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *dnW = NULL;
    LDAPModW **attrsW = NULL;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_a(dn), attrs );

    if (!ld) return ~0u;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (attrs && !(attrsW = modarrayAtoW( attrs ))) goto exit;

    ret = ldap_addW( ld, dnW, attrsW );

exit:
    free( dnW );
    modarrayfreeW( attrsW );
    return ret;
}

/***********************************************************************
 *      ldap_addW     (WLDAP32.@)
 */
ULONG CDECL ldap_addW( LDAP *ld, WCHAR *dn, LDAPModW **attrs )
{
    ULONG ret, msg;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_w(dn), attrs );

    ret = ldap_add_extW( ld, dn, attrs, NULL, NULL, &msg );
    if (ret == WLDAP32_LDAP_SUCCESS) return msg;
    return ~0u;
}

/***********************************************************************
 *      ldap_add_extA     (WLDAP32.@)
 */
ULONG CDECL ldap_add_extA( LDAP *ld, char *dn, LDAPModA **attrs, LDAPControlA **serverctrls,
                           LDAPControlA **clientctrls, ULONG *message )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *dnW = NULL;
    LDAPModW **attrsW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    TRACE( "(%p, %s, %p, %p, %p, %p)\n", ld, debugstr_a(dn), attrs, serverctrls, clientctrls, message );

    if (!ld || !message) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (attrs && !(attrsW = modarrayAtoW( attrs ))) goto exit;
    if (serverctrls && !(serverctrlsW = controlarrayAtoW( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsW = controlarrayAtoW( clientctrls ))) goto exit;

    ret = ldap_add_extW( ld, dnW, attrsW, serverctrlsW, clientctrlsW, message );

exit:
    free( dnW );
    modarrayfreeW( attrsW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );
    return ret;
}

/***********************************************************************
 *      ldap_add_extW     (WLDAP32.@)
 */
ULONG CDECL ldap_add_extW( LDAP *ld, WCHAR *dn, LDAPModW **attrs, LDAPControlW **serverctrls,
                           LDAPControlW **clientctrls, ULONG *message )
{
    ULONG ret;
    char *dnU = NULL;
    LDAPMod **attrsU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;

    TRACE( "(%p, %s, %p, %p, %p, %p)\n", ld, debugstr_w(dn), attrs, serverctrls, clientctrls, message );

    if (!ld || !message) return WLDAP32_LDAP_PARAM_ERROR;
    if ((ret = WLDAP32_ldap_connect( ld, NULL ))) return ret;

    ret = WLDAP32_LDAP_NO_MEMORY;
    if (!(dnU = dn ? strWtoU( dn ) : strdup( "" ))) goto exit;
    if (attrs && !(attrsU = modarrayWtoU( attrs ))) goto exit;
    if (serverctrls && !(serverctrlsU = controlarrayWtoU( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsU = controlarrayWtoU( clientctrls ))) goto exit;
    else
    {
        ret = map_error( ldap_add_ext( CTX(ld), dnU, attrsU, serverctrlsU, clientctrlsU, (int *)message ) );
    }

exit:
    free( dnU );
    modarrayfreeU( attrsU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );
    return ret;
}

/***********************************************************************
 *      ldap_add_ext_sA     (WLDAP32.@)
 */
ULONG CDECL ldap_add_ext_sA( LDAP *ld, char *dn, LDAPModA **attrs, LDAPControlA **serverctrls,
                             LDAPControlA **clientctrls )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *dnW = NULL;
    LDAPModW **attrsW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    TRACE( "(%p, %s, %p, %p, %p)\n", ld, debugstr_a(dn), attrs, serverctrls, clientctrls );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (attrs && !(attrsW = modarrayAtoW( attrs ))) goto exit;
    if (serverctrls && !(serverctrlsW = controlarrayAtoW( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsW = controlarrayAtoW( clientctrls ))) goto exit;

    ret = ldap_add_ext_sW( ld, dnW, attrsW, serverctrlsW, clientctrlsW );

exit:
    free( dnW );
    modarrayfreeW( attrsW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );
    return ret;
}

/***********************************************************************
 *      ldap_add_ext_sW     (WLDAP32.@)
 */
ULONG CDECL ldap_add_ext_sW( LDAP *ld, WCHAR *dn, LDAPModW **attrs, LDAPControlW **serverctrls,
                             LDAPControlW **clientctrls )
{
    ULONG ret;
    char *dnU = NULL;
    LDAPMod **attrsU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;

    TRACE( "(%p, %s, %p, %p, %p)\n", ld, debugstr_w(dn), attrs, serverctrls, clientctrls );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    if ((ret = WLDAP32_ldap_connect( ld, NULL ))) return ret;

    ret = WLDAP32_LDAP_NO_MEMORY;
    if (!(dnU = dn ? strWtoU( dn ) : strdup(""))) goto exit;
    if (attrs && !(attrsU = modarrayWtoU( attrs ))) goto exit;
    if (serverctrls && !(serverctrlsU = controlarrayWtoU( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsU = controlarrayWtoU( clientctrls ))) goto exit;
    else
    {
        ret = map_error( ldap_add_ext_s( CTX(ld), dnU, attrsU, serverctrlsU, clientctrlsU ) );
    }

exit:
    free( dnU );
    modarrayfreeU( attrsU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );
    return ret;
}

/***********************************************************************
 *      ldap_add_sA     (WLDAP32.@)
 */
ULONG CDECL ldap_add_sA( LDAP *ld, char *dn, LDAPModA **attrs )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *dnW = NULL;
    LDAPModW **attrsW = NULL;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_a(dn), attrs );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (attrs && !(attrsW = modarrayAtoW( attrs ))) goto exit;

    ret = ldap_add_sW( ld, dnW, attrsW );

exit:
    free( dnW );
    modarrayfreeW( attrsW );
    return ret;
}

/***********************************************************************
 *      ldap_add_sW     (WLDAP32.@)
 */
ULONG CDECL ldap_add_sW( LDAP *ld, WCHAR *dn, LDAPModW **attrs )
{
    TRACE( "(%p, %s, %p)\n", ld, debugstr_w(dn), attrs );
    return ldap_add_ext_sW( ld, dn, attrs, NULL, NULL );
}
