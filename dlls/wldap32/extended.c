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
 *      ldap_close_extended_op     (WLDAP32.@)
 */
ULONG CDECL ldap_close_extended_op( LDAP *ld, ULONG msgid )
{
    TRACE( "(%p, %#lx)\n", ld, msgid );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    return WLDAP32_LDAP_SUCCESS;
}

/***********************************************************************
 *      ldap_extended_operationA     (WLDAP32.@)
 */
ULONG CDECL ldap_extended_operationA( LDAP *ld, char *oid, struct WLDAP32_berval *data, LDAPControlA **serverctrls,
                                      LDAPControlA **clientctrls, ULONG *message )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *oidW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    TRACE( "(%p, %s, %p, %p, %p, %p)\n", ld, debugstr_a(oid), data, serverctrls, clientctrls, message );

    if (!ld || !message) return WLDAP32_LDAP_PARAM_ERROR;

    if (oid && !(oidW = strAtoW( oid ))) goto exit;
    if (serverctrls && !(serverctrlsW = controlarrayAtoW( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsW = controlarrayAtoW( clientctrls ))) goto exit;

    ret = ldap_extended_operationW( ld, oidW, data, serverctrlsW, clientctrlsW, message );

exit:
    free( oidW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );
    return ret;
}

/***********************************************************************
 *      ldap_extended_operationW     (WLDAP32.@)
 */
ULONG CDECL ldap_extended_operationW( LDAP *ld, WCHAR *oid, struct WLDAP32_berval *data, LDAPControlW **serverctrls,
                                      LDAPControlW **clientctrls, ULONG *message )
{
    ULONG ret;
    char *oidU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;
    struct berval *dataU = NULL;

    TRACE( "(%p, %s, %p, %p, %p, %p)\n", ld, debugstr_w(oid), data, serverctrls, clientctrls, message );

    if (!ld || !message) return WLDAP32_LDAP_PARAM_ERROR;
    if ((ret = WLDAP32_ldap_connect( ld, NULL ))) return ret;

    ret = WLDAP32_LDAP_NO_MEMORY;
    if (oid && !(oidU = strWtoU( oid ))) goto exit;
    if (data && !(dataU = bervalWtoU( data ))) goto exit;
    if (serverctrls && !(serverctrlsU = controlarrayWtoU( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsU = controlarrayWtoU( clientctrls ))) goto exit;
    else
    {
        ret = map_error( ldap_extended_operation( CTX(ld), oidU, dataU, serverctrlsU, clientctrlsU, (int *)message ) );
    }

exit:
    free( oidU );
    free( dataU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );
    return ret;
}

/***********************************************************************
 *      ldap_extended_operation_sA     (WLDAP32.@)
 */
ULONG CDECL ldap_extended_operation_sA( LDAP *ld, char *oid, struct WLDAP32_berval *data, LDAPControlA **serverctrls,
                                        LDAPControlA **clientctrls, char **retoid, struct WLDAP32_berval **retdata )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *oidW = NULL, *retoidW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    TRACE( "(%p, %s, %p, %p, %p, %p, %p)\n", ld, debugstr_a(oid), data, serverctrls, clientctrls, retoid, retdata );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (oid && !(oidW = strAtoW( oid ))) goto exit;
    if (serverctrls && !(serverctrlsW = controlarrayAtoW( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsW = controlarrayAtoW( clientctrls ))) goto exit;

    ret = ldap_extended_operation_sW( ld, oidW, data, serverctrlsW, clientctrlsW, &retoidW, retdata );
    if (retoid && retoidW)
    {
        char *str = strWtoA( retoidW );
        if (str) *retoid = str;
        else ret = WLDAP32_LDAP_NO_MEMORY;
        ldap_memfreeW( retoidW );
    }

exit:
    free( oidW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );
    return ret;
}

/***********************************************************************
 *      ldap_extended_operation_sW     (WLDAP32.@)
 */
ULONG CDECL ldap_extended_operation_sW( LDAP *ld, WCHAR *oid, struct WLDAP32_berval *data, LDAPControlW **serverctrls,
                                        LDAPControlW **clientctrls, WCHAR **retoid, struct WLDAP32_berval **retdata )
{
    ULONG ret;
    char *oidU = NULL, *retoidU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;
    struct berval *retdataU, *dataU = NULL;

    TRACE( "(%p, %s, %p, %p, %p, %p, %p)\n", ld, debugstr_w(oid), data, serverctrls, clientctrls, retoid, retdata );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    if ((ret = WLDAP32_ldap_connect( ld, NULL ))) return ret;

    ret = WLDAP32_LDAP_NO_MEMORY;
    if (oid && !(oidU = strWtoU( oid ))) goto exit;
    if (data && !(dataU = bervalWtoU( data ))) goto exit;
    if (serverctrls && !(serverctrlsU = controlarrayWtoU( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsU = controlarrayWtoU( clientctrls ))) goto exit;
    else
    {
        ret = map_error( ldap_extended_operation_s( CTX(ld), oidU, dataU, serverctrlsU, clientctrlsU, &retoidU,
                         &retdataU ) );
    }

    if (retoid && retoidU)
    {
        WCHAR *str = strUtoW( retoidU );
        if (str) *retoid = str;
        else ret = WLDAP32_LDAP_NO_MEMORY;
        ldap_memfree( retoidU );
    }
    if (retdata && retdataU)
    {
        struct WLDAP32_berval *bv = bervalUtoW( retdataU );
        if (bv) *retdata = bv;
        else ret = WLDAP32_LDAP_NO_MEMORY;
        ber_bvfree( retdataU );
    }

exit:
    free( oidU );
    free( dataU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );
    return ret;
}
