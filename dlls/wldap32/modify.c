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
 *      ldap_modifyA     (WLDAP32.@)
 */
ULONG CDECL ldap_modifyA( LDAP *ld, char *dn, LDAPModA **mods )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *dnW = NULL;
    LDAPModW **modsW = NULL;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_a(dn), mods );

    if (!ld) return ~0u;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (mods && !(modsW = modarrayAtoW( mods ))) goto exit;

    ret = ldap_modifyW( ld, dnW, modsW );

exit:
    free( dnW );
    modarrayfreeW( modsW );
    return ret;
}

/***********************************************************************
 *      ldap_modifyW     (WLDAP32.@)
 */
ULONG CDECL ldap_modifyW( LDAP *ld, WCHAR *dn, LDAPModW **mods )
{
    ULONG ret, msg;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_w(dn), mods );

    ret = ldap_modify_extW( ld, dn, mods, NULL, NULL, &msg );
    if (ret == WLDAP32_LDAP_SUCCESS) return msg;
    return ~0u;
}

/***********************************************************************
 *      ldap_modify_extA     (WLDAP32.@)
 */
ULONG CDECL ldap_modify_extA( LDAP *ld, char *dn, LDAPModA **mods, LDAPControlA **serverctrls,
                              LDAPControlA **clientctrls, ULONG *message )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *dnW = NULL;
    LDAPModW **modsW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    TRACE( "(%p, %s, %p, %p, %p, %p)\n", ld, debugstr_a(dn), mods, serverctrls, clientctrls, message );

    if (!ld || !message) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (mods && !(modsW = modarrayAtoW( mods ))) goto exit;
    if (serverctrls && !(serverctrlsW = controlarrayAtoW( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsW = controlarrayAtoW( clientctrls ))) goto exit;

    ret = ldap_modify_extW( ld, dnW, modsW, serverctrlsW, clientctrlsW, message );

exit:
    free( dnW );
    modarrayfreeW( modsW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );
    return ret;
}

/***********************************************************************
 *      ldap_modify_extW     (WLDAP32.@)
 */
ULONG CDECL ldap_modify_extW( LDAP *ld, WCHAR *dn, LDAPModW **mods, LDAPControlW **serverctrls,
                              LDAPControlW **clientctrls, ULONG *message )
{
    ULONG ret;
    char *dnU = NULL;
    LDAPMod **modsU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;

    TRACE( "(%p, %s, %p, %p, %p, %p)\n", ld, debugstr_w(dn), mods, serverctrls, clientctrls, message );

    if (!ld || !message) return WLDAP32_LDAP_PARAM_ERROR;
    if ((ret = WLDAP32_ldap_connect( ld, NULL ))) return ret;

    ret = WLDAP32_LDAP_NO_MEMORY;
    if (!(dnU = dn ? strWtoU( dn ) : strdup( "" ))) goto exit;
    if (mods && !(modsU = modarrayWtoU( mods ))) goto exit;
    if (serverctrls && !(serverctrlsU = controlarrayWtoU( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsU = controlarrayWtoU( clientctrls ))) goto exit;
    else
    {
        ret = map_error( ldap_modify_ext( CTX(ld), dnU, modsU, serverctrlsU, clientctrlsU, (int *)message ) );
    }

exit:
    free( dnU );
    modarrayfreeU( modsU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );
    return ret;
}

/***********************************************************************
 *      ldap_modify_ext_sA     (WLDAP32.@)
 */
ULONG CDECL ldap_modify_ext_sA( LDAP *ld, char *dn, LDAPModA **mods, LDAPControlA **serverctrls,
                                LDAPControlA **clientctrls )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *dnW = NULL;
    LDAPModW **modsW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    TRACE( "(%p, %s, %p, %p, %p)\n", ld, debugstr_a(dn), mods, serverctrls, clientctrls );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (mods && !(modsW = modarrayAtoW( mods ))) goto exit;
    if (serverctrls && !(serverctrlsW = controlarrayAtoW( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsW = controlarrayAtoW( clientctrls ))) goto exit;

    ret = ldap_modify_ext_sW( ld, dnW, modsW, serverctrlsW, clientctrlsW );

exit:
    free( dnW );
    modarrayfreeW( modsW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );
    return ret;
}

/***********************************************************************
 *      ldap_modify_ext_sW     (WLDAP32.@)
 */
ULONG CDECL ldap_modify_ext_sW( LDAP *ld, WCHAR *dn, LDAPModW **mods, LDAPControlW **serverctrls,
                                LDAPControlW **clientctrls )
{
    ULONG ret;
    char *dnU = NULL;
    LDAPMod **modsU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;

    TRACE( "(%p, %s, %p, %p, %p)\n", ld, debugstr_w(dn), mods, serverctrls, clientctrls );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    if ((ret = WLDAP32_ldap_connect( ld, NULL ))) return ret;

    ret = WLDAP32_LDAP_NO_MEMORY;
    if (!(dnU = dn ? strWtoU( dn ) : strdup( "" ))) goto exit;
    if (mods && !(modsU = modarrayWtoU( mods ))) goto exit;
    if (serverctrls && !(serverctrlsU = controlarrayWtoU( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsU = controlarrayWtoU( clientctrls ))) goto exit;
    else
    {
        ret = map_error( ldap_modify_ext_s( CTX(ld), dnU, modsU, serverctrlsU, clientctrlsU ) );
    }

exit:
    free( dnU );
    modarrayfreeU( modsU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );
    return ret;
}

/***********************************************************************
 *      ldap_modify_sA     (WLDAP32.@)
 */
ULONG CDECL ldap_modify_sA( LDAP *ld, char *dn, LDAPModA **mods )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *dnW = NULL;
    LDAPModW **modsW = NULL;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_a(dn), mods );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (mods && !(modsW = modarrayAtoW( mods ))) goto exit;

    ret = ldap_modify_sW( ld, dnW, modsW );

exit:
    free( dnW );
    modarrayfreeW( modsW );
    return ret;
}

/***********************************************************************
 *      ldap_modify_sW     (WLDAP32.@)
 */
ULONG CDECL ldap_modify_sW( LDAP *ld, WCHAR *dn, LDAPModW **mods )
{
    TRACE( "(%p, %s, %p)\n", ld, debugstr_w(dn), mods );
    return ldap_modify_ext_sW( ld, dn, mods, NULL, NULL );
}
