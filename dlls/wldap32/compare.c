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
 *      ldap_compareA     (WLDAP32.@)
 *
 * See ldap_compareW.
 */
ULONG CDECL ldap_compareA( LDAP *ld, char *dn, char *attr, char *value )
{
    ULONG ret = ~0u;
    WCHAR *dnW = NULL, *attrW = NULL, *valueW = NULL;

    TRACE( "(%p, %s, %s, %s)\n", ld, debugstr_a(dn), debugstr_a(attr), debugstr_a(value) );

    if (!ld || !attr) return ~0u;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (!(attrW = strAtoW( attr ))) goto exit;
    if (value && !(valueW = strAtoW( value ))) goto exit;

    ret = ldap_compareW( ld, dnW, attrW, valueW );

exit:
    free( dnW );
    free( attrW );
    free( valueW );
    return ret;
}

/***********************************************************************
 *      ldap_compareW     (WLDAP32.@)
 *
 * Check if an attribute has a certain value (asynchronous operation).
 *
 * PARAMS
 *  ld      [I] Pointer to an LDAP context.
 *  dn      [I] DN of entry to compare value for.
 *  attr    [I] Attribute to compare value for.
 *  value   [I] Value to compare.
 *
 * RETURNS
 *  Success: Message ID of the compare operation.
 *  Failure: An LDAP error code.
 */
ULONG CDECL ldap_compareW( LDAP *ld, WCHAR *dn, WCHAR *attr, WCHAR *value )
{
    ULONG msg, ret;

    TRACE( "(%p, %s, %s, %s)\n", ld, debugstr_w(dn), debugstr_w(attr), debugstr_w(value) );

    ret = ldap_compare_extW( ld, dn, attr, value, NULL, NULL, NULL, &msg );
    if (ret == LDAP_SUCCESS) return msg;
    return ~0u;
}

/***********************************************************************
 *      ldap_compare_extA     (WLDAP32.@)
 *
 * See ldap_compare_extW.
 */
ULONG CDECL ldap_compare_extA( LDAP *ld, char *dn, char *attr, char *value,
    struct berval *data, LDAPControlA **serverctrls, LDAPControlA **clientctrls,
    ULONG *message )
{
    ULONG ret = LDAP_NO_MEMORY;
    WCHAR *dnW = NULL, *attrW = NULL, *valueW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    TRACE( "(%p, %s, %s, %s, %p, %p, %p, %p)\n", ld, debugstr_a(dn), debugstr_a(attr), debugstr_a(value),
           data, serverctrls, clientctrls, message );

    if (!ld || !message) return LDAP_PARAM_ERROR;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (attr && !(attrW = strAtoW( attr ))) goto exit;
    if (value && !(valueW = strAtoW( value ))) goto exit;
    if (serverctrls && !(serverctrlsW = controlarrayAtoW( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsW = controlarrayAtoW( clientctrls ))) goto exit;

    ret = ldap_compare_extW( ld, dnW, attrW, valueW, data, serverctrlsW, clientctrlsW, message );

exit:
    free( dnW );
    free( attrW );
    free( valueW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );
    return ret;
}

/***********************************************************************
 *      ldap_compare_extW     (WLDAP32.@)
 *
 * Check if an attribute has a certain value (asynchronous operation).
 *
 * PARAMS
 *  ld          [I] Pointer to an LDAP context.
 *  dn          [I] DN of entry to compare value for.
 *  attr        [I] Attribute to compare value for.
 *  value       [I] string encoded value to compare.
 *  data        [I] berval encoded value to compare.
 *  serverctrls [I] Array of LDAP server controls.
 *  clientctrls [I] Array of LDAP client controls.
 *  message     [O] Message ID of the compare operation.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  Set value to compare strings or data to compare binary values. If
 *  both are non-NULL, data will be used. The serverctrls and clientctrls
 *  parameters are optional and should be set to NULL if not used.
 */
ULONG CDECL ldap_compare_extW( LDAP *ld, WCHAR *dn, WCHAR *attr, WCHAR *value, struct berval *data,
    LDAPControlW **serverctrls, LDAPControlW **clientctrls, ULONG *message )
{
    ULONG ret = LDAP_NO_MEMORY;
    char *dnU = NULL, *attrU = NULL, *valueU = NULL;
    LDAPControlU **serverctrlsU = NULL, **clientctrlsU = NULL;
    struct bervalU *dataU = NULL, val = { 0, NULL };

    TRACE( "(%p, %s, %s, %s, %p, %p, %p, %p)\n", ld, debugstr_w(dn), debugstr_w(attr), debugstr_w(value),
           data, serverctrls, clientctrls, message );

    if (!ld || !message) return LDAP_PARAM_ERROR;
    if (!attr) return LDAP_NO_MEMORY;

    if (dn && !(dnU = strWtoU( dn ))) goto exit;
    if (!(attrU = strWtoU( attr ))) goto exit;
    if (!data)
    {
        if (value)
        {
            if (!(valueU = strWtoU( value ))) goto exit;
            val.bv_len = strlen( valueU );
            val.bv_val = valueU;
        }
    }
    else if (!(dataU = bervalWtoU( data ))) goto exit;

    if (serverctrls && !(serverctrlsU = controlarrayWtoU( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsU = controlarrayWtoU( clientctrls ))) goto exit;
    else
    {
        struct ldap_compare_ext_params params = { CTX(ld), dnU, attrU, dataU ? dataU : &val, serverctrlsU, clientctrlsU, message };
        ret = map_error( LDAP_CALL( ldap_compare_ext, &params ));
    }

exit:
    free( dnU );
    free( attrU );
    free( valueU );
    bvfreeU( dataU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );
    return ret;
}

/***********************************************************************
 *      ldap_compare_ext_sA     (WLDAP32.@)
 *
 * See ldap_compare_ext_sW.
 */
ULONG CDECL ldap_compare_ext_sA( LDAP *ld, char *dn, char *attr, char *value, struct berval *data,
    LDAPControlA **serverctrls, LDAPControlA **clientctrls )
{
    ULONG ret = LDAP_NO_MEMORY;
    WCHAR *dnW = NULL, *attrW = NULL, *valueW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    TRACE( "(%p, %s, %s, %s, %p, %p, %p)\n", ld, debugstr_a(dn), debugstr_a(attr), debugstr_a(value),
           data, serverctrls, clientctrls );

    if (!ld) return LDAP_PARAM_ERROR;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (attr && !(attrW = strAtoW( attr ))) goto exit;
    if (value && !(valueW = strAtoW( value ))) goto exit;
    if (serverctrls && !(serverctrlsW = controlarrayAtoW( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsW = controlarrayAtoW( clientctrls ))) goto exit;

    ret = ldap_compare_ext_sW( ld, dnW, attrW, valueW, data, serverctrlsW, clientctrlsW );

exit:
    free( dnW );
    free( attrW );
    free( valueW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );
    return ret;
}

/***********************************************************************
 *      ldap_compare_ext_sW     (WLDAP32.@)
 *
 * Check if an attribute has a certain value (synchronous operation).
 *
 * PARAMS
 *  ld          [I] Pointer to an LDAP context.
 *  dn          [I] DN of entry to compare value for.
 *  attr        [I] Attribute to compare value for.
 *  value       [I] string encoded value to compare.
 *  data        [I] berval encoded value to compare.
 *  serverctrls [I] Array of LDAP server controls.
 *  clientctrls [I] Array of LDAP client controls.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 *
 * NOTES
 *  Set value to compare strings or data to compare binary values. If
 *  both are non-NULL, data will be used. The serverctrls and clientctrls
 *  parameters are optional and should be set to NULL if not used.
 */
ULONG CDECL ldap_compare_ext_sW( LDAP *ld, WCHAR *dn, WCHAR *attr, WCHAR *value, struct berval *data,
    LDAPControlW **serverctrls, LDAPControlW **clientctrls )
{
    ULONG ret = LDAP_NO_MEMORY;
    char *dnU = NULL, *attrU = NULL, *valueU = NULL;
    LDAPControlU **serverctrlsU = NULL, **clientctrlsU = NULL;
    struct bervalU *dataU = NULL, val = { 0, NULL };

    TRACE( "(%p, %s, %s, %s, %p, %p, %p)\n", ld, debugstr_w(dn), debugstr_w(attr), debugstr_w(value), data,
           serverctrls, clientctrls );

    if (!ld) return LDAP_PARAM_ERROR;

    if (dn && !(dnU = strWtoU( dn ))) goto exit;
    if (attr && !(attrU = strWtoU( attr ))) goto exit;
    if (!data)
    {
        if (value)
        {
            if (!(valueU = strWtoU( value ))) goto exit;
            val.bv_len = strlen( valueU );
            val.bv_val = valueU;
        }
    }
    else if (!(dataU = bervalWtoU( data ))) goto exit;

    if (serverctrls && !(serverctrlsU = controlarrayWtoU( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsU = controlarrayWtoU( clientctrls ))) goto exit;
    else
    {
        struct ldap_compare_ext_s_params params = { CTX(ld), dnU, attrU, dataU ? dataU : &val, serverctrlsU, clientctrlsU };
        ret = map_error( LDAP_CALL( ldap_compare_ext_s, &params ));
    }
exit:
    free( dnU );
    free( attrU );
    free( valueU );
    bvfreeU( dataU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );
    return ret;
}

/***********************************************************************
 *      ldap_compare_sA     (WLDAP32.@)
 *
 * See ldap_compare_sW.
 */
ULONG CDECL ldap_compare_sA( LDAP *ld, PCHAR dn, PCHAR attr, PCHAR value )
{
    ULONG ret = LDAP_NO_MEMORY;
    WCHAR *dnW = NULL, *attrW = NULL, *valueW = NULL;

    TRACE( "(%p, %s, %s, %s)\n", ld, debugstr_a(dn), debugstr_a(attr), debugstr_a(value) );

    if (!ld) return LDAP_PARAM_ERROR;

    if (dn && !(dnW = strAtoW( dn ))) goto exit;
    if (attr && !(attrW = strAtoW( attr ))) goto exit;
    if (value && !(valueW = strAtoW( value ))) goto exit;

    ret = ldap_compare_sW( ld, dnW, attrW, valueW );

exit:
    free( dnW );
    free( attrW );
    free( valueW );
    return ret;
}

/***********************************************************************
 *      ldap_compare_sW     (WLDAP32.@)
 *
 * Check if an attribute has a certain value (synchronous operation).
 *
 * PARAMS
 *  ld      [I] Pointer to an LDAP context.
 *  dn      [I] DN of entry to compare value for.
 *  attr    [I] Attribute to compare value for.
 *  value   [I] Value to compare.
 *
 * RETURNS
 *  Success: LDAP_SUCCESS
 *  Failure: An LDAP error code.
 */
ULONG CDECL ldap_compare_sW( LDAP *ld, WCHAR *dn, WCHAR *attr, WCHAR *value )
{
    TRACE( "(%p, %s, %s, %s)\n", ld, debugstr_w(dn), debugstr_w(attr), debugstr_w(value) );
    return ldap_compare_ext_sW( ld, dn, attr, value, NULL, NULL, NULL );
}
