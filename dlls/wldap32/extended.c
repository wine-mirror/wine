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
 *      ldap_close_extended_op     (WLDAP32.@)
 *
 * Close an extended operation.
 *
 * PARAMS
 *  ld    [I] Pointer to an LDAP context.
 *  msgid [I] Message ID of the operation to be closed.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  Contrary to native, OpenLDAP does not require us to close
 *  extended operations, so this is a no-op.
 */
ULONG CDECL ldap_close_extended_op( LDAP *ld, ULONG msgid )
{
    TRACE( "(%p, %#lx)\n", ld, msgid );

    if (!ld) return LDAP_PARAM_ERROR;
    return LDAP_SUCCESS;
}

/***********************************************************************
 *      ldap_extended_operationA     (WLDAP32.@)
 *
 * See ldap_extended_operationW.
 */
ULONG CDECL ldap_extended_operationA( LDAP *ld, char *oid, struct berval *data, LDAPControlA **serverctrls,
    LDAPControlA **clientctrls, ULONG *message )
{
    ULONG ret = LDAP_NO_MEMORY;
    WCHAR *oidW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    TRACE( "(%p, %s, %p, %p, %p, %p)\n", ld, debugstr_a(oid), data, serverctrls, clientctrls, message );

    if (!ld || !message) return LDAP_PARAM_ERROR;

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
 *
 * Perform an extended operation (asynchronous mode).
 *
 * PARAMS
 *  ld          [I] Pointer to an LDAP context.
 *  oid         [I] OID of the extended operation.
 *  data        [I] Data needed by the operation.
 *  serverctrls [I] Array of LDAP server controls.
 *  clientctrls [I] Array of LDAP client controls.
 *  message     [O] Message ID of the extended operation.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  The data parameter should be set to NULL if the operation
 *  requires no data. Call ldap_result with the message ID to
 *  get the result of the operation or ldap_abandon to cancel
 *  the operation. The serverctrls and clientctrls parameters
 *  are optional and should be set to NULL if not used. Call
 *  ldap_close_extended_op to close the operation.
 */
ULONG CDECL ldap_extended_operationW( LDAP *ld, WCHAR *oid, struct berval *data, LDAPControlW **serverctrls,
    LDAPControlW **clientctrls, ULONG *message )
{
    ULONG ret = LDAP_NO_MEMORY;
    char *oidU = NULL;
    LDAPControlU **serverctrlsU = NULL, **clientctrlsU = NULL;
    struct bervalU *dataU = NULL;

    TRACE( "(%p, %s, %p, %p, %p, %p)\n", ld, debugstr_w(oid), data, serverctrls, clientctrls, message );

    if (!ld || !message) return LDAP_PARAM_ERROR;

    if (oid && !(oidU = strWtoU( oid ))) goto exit;
    if (data && !(dataU = bervalWtoU( data ))) goto exit;
    if (serverctrls && !(serverctrlsU = controlarrayWtoU( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsU = controlarrayWtoU( clientctrls ))) goto exit;
    else
    {
        struct ldap_extended_operation_params params = { CTX(ld), oidU, dataU, serverctrlsU, clientctrlsU, message };
        ret = map_error( LDAP_CALL( ldap_extended_operation, &params ));
    }

exit:
    free( oidU );
    bvfreeU( dataU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );
    return ret;
}

/***********************************************************************
 *      ldap_extended_operation_sA     (WLDAP32.@)
 *
 * See ldap_extended_operation_sW.
 */
ULONG CDECL ldap_extended_operation_sA( LDAP *ld, char *oid, struct berval *data, LDAPControlA **serverctrls,
    LDAPControlA **clientctrls, char **retoid, struct berval **retdata )
{
    ULONG ret = LDAP_NO_MEMORY;
    WCHAR *oidW = NULL, *retoidW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    TRACE( "(%p, %s, %p, %p, %p, %p, %p)\n", ld, debugstr_a(oid), data, serverctrls, clientctrls, retoid, retdata );

    if (!ld) return LDAP_PARAM_ERROR;

    if (oid && !(oidW = strAtoW( oid ))) goto exit;
    if (serverctrls && !(serverctrlsW = controlarrayAtoW( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsW = controlarrayAtoW( clientctrls ))) goto exit;

    ret = ldap_extended_operation_sW( ld, oidW, data, serverctrlsW, clientctrlsW, &retoidW, retdata );
    if (retoid && retoidW)
    {
        char *str = strWtoA( retoidW );
        if (str) *retoid = str;
        else ret = LDAP_NO_MEMORY;
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
 *
 * Perform an extended operation (synchronous mode).
 *
 * PARAMS
 *  ld          [I] Pointer to an LDAP context.
 *  oid         [I] OID of the extended operation.
 *  data        [I] Data needed by the operation.
 *  serverctrls [I] Array of LDAP server controls.
 *  clientctrls [I] Array of LDAP client controls.
 *  retoid      [O] OID of the server response message.
 *  retdata     [O] Data returned by the server.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  The data parameter should be set to NULL if the operation
 *  requires no data. The serverctrls, clientctrls, retoid and
 *  and retdata parameters are also optional. Set to NULL if not
 *  used. Free retoid and retdata after use with ldap_memfree.
 */
ULONG CDECL ldap_extended_operation_sW( LDAP *ld, WCHAR *oid, struct berval *data, LDAPControlW **serverctrls,
    LDAPControlW **clientctrls, WCHAR **retoid, struct berval **retdata )
{
    ULONG ret = LDAP_NO_MEMORY;
    char *oidU = NULL, *retoidU = NULL;
    LDAPControlU **serverctrlsU = NULL, **clientctrlsU = NULL;
    struct bervalU *retdataU, *dataU = NULL;

    TRACE( "(%p, %s, %p, %p, %p, %p, %p)\n", ld, debugstr_w(oid), data, serverctrls, clientctrls, retoid, retdata );

    if (!ld) return LDAP_PARAM_ERROR;

    if (oid && !(oidU = strWtoU( oid ))) goto exit;
    if (data && !(dataU = bervalWtoU( data ))) goto exit;
    if (serverctrls && !(serverctrlsU = controlarrayWtoU( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsU = controlarrayWtoU( clientctrls ))) goto exit;
    else
    {
        struct ldap_extended_operation_s_params params = { CTX(ld), oidU, dataU, serverctrlsU, clientctrlsU, &retoidU, &retdataU };
        ret = map_error( LDAP_CALL( ldap_extended_operation_s, &params ));
    }

    if (retoid && retoidU)
    {
        WCHAR *str = strUtoW( retoidU );
        if (str) *retoid = str;
        else ret = LDAP_NO_MEMORY;
        LDAP_CALL( ldap_memfree, retoidU );
    }
    if (retdata && retdataU)
    {
        struct berval *bv = bervalUtoW( retdataU );
        if (bv) *retdata = bv;
        else ret = LDAP_NO_MEMORY;
        LDAP_CALL( ber_bvfree, retdataU );
    }

exit:
    free( oidU );
    bvfreeU( dataU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );
    return ret;
}
