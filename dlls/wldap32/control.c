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
 *      ldap_control_freeA     (WLDAP32.@)
 */
ULONG CDECL ldap_control_freeA( LDAPControlA *control )
{
    TRACE( "(%p)\n", control );
    controlfreeA( control );
    return WLDAP32_LDAP_SUCCESS;
}

/***********************************************************************
 *      ldap_control_freeW     (WLDAP32.@)
 */
ULONG CDECL ldap_control_freeW( LDAPControlW *control )
{
    TRACE( "(%p)\n", control );
    controlfreeW( control );
    return WLDAP32_LDAP_SUCCESS;
}

/***********************************************************************
 *      ldap_controls_freeA     (WLDAP32.@)
 */
ULONG CDECL ldap_controls_freeA( LDAPControlA **controls )
{
    TRACE( "(%p)\n", controls );
    controlarrayfreeA( controls );
    return WLDAP32_LDAP_SUCCESS;
}

/***********************************************************************
 *      ldap_controls_freeW     (WLDAP32.@)
 */
ULONG CDECL ldap_controls_freeW( LDAPControlW **controls )
{
    TRACE( "(%p)\n", controls );
    controlarrayfreeW( controls );
    return WLDAP32_LDAP_SUCCESS;
}

/***********************************************************************
 *      ldap_create_sort_controlA     (WLDAP32.@)
 */
ULONG CDECL ldap_create_sort_controlA( LDAP *ld, LDAPSortKeyA **sortkey, UCHAR critical, LDAPControlA **control )
{
    ULONG ret;
    LDAPSortKeyW **sortkeyW;
    LDAPControlW *controlW;

    TRACE( "(%p, %p, 0x%02x, %p)\n", ld, sortkey, critical, control );

    if (!ld || !sortkey || !control) return WLDAP32_LDAP_PARAM_ERROR;

    if (!(sortkeyW = sortkeyarrayAtoW( sortkey ))) return WLDAP32_LDAP_NO_MEMORY;

    ret = ldap_create_sort_controlW( ld, sortkeyW, critical, &controlW );
    if (ret == WLDAP32_LDAP_SUCCESS)
    {
        LDAPControlA *controlA = controlWtoA( controlW );
        if (controlA) *control = controlA;
        else ret = WLDAP32_LDAP_NO_MEMORY;
        ldap_control_freeW( controlW );
    }

    sortkeyarrayfreeW( sortkeyW );
    return ret;
}

/***********************************************************************
 *      ldap_create_sort_controlW     (WLDAP32.@)
 */
ULONG CDECL ldap_create_sort_controlW( LDAP *ld, LDAPSortKeyW **sortkey, UCHAR critical, LDAPControlW **control )
{
    ULONG ret;
    LDAPSortKey **sortkeyU;
    LDAPControl *controlU;

    TRACE( "(%p, %p, 0x%02x, %p)\n", ld, sortkey, critical, control );

    if (!ld || !sortkey || !control) return WLDAP32_LDAP_PARAM_ERROR;
    if ((ret = WLDAP32_ldap_connect( ld, NULL ))) return ret;

    if (!(sortkeyU = sortkeyarrayWtoU( sortkey ))) return WLDAP32_LDAP_NO_MEMORY;

    ret = map_error( ldap_create_sort_control( CTX(ld), sortkeyU, critical, &controlU ) );
    if (ret == WLDAP32_LDAP_SUCCESS)
    {
        LDAPControlW *controlW = controlUtoW( controlU );
        if (controlW) *control = controlW;
        else ret = WLDAP32_LDAP_NO_MEMORY;
        ldap_control_free( controlU );
    }

    sortkeyarrayfreeU( sortkeyU );
    return ret;
}

/***********************************************************************
 *      ldap_create_vlv_controlA     (WLDAP32.@)
 */
INT CDECL ldap_create_vlv_controlA( LDAP *ld, WLDAP32_LDAPVLVInfo *info, UCHAR critical, LDAPControlA **control )
{
    INT ret;
    LDAPControlW *controlW;

    TRACE( "(%p, %p, 0x%02x, %p)\n", ld, info, critical, control );

    if (!ld || !control) return ~0u;

    ret = ldap_create_vlv_controlW( ld, info, critical, &controlW );
    if (ret == WLDAP32_LDAP_SUCCESS)
    {
        LDAPControlA *controlA = controlWtoA( controlW );
        if (controlA) *control = controlA;
        else ret = WLDAP32_LDAP_NO_MEMORY;
        ldap_control_freeW( controlW );
    }

    return ret;
}

/***********************************************************************
 *      ldap_create_vlv_controlW     (WLDAP32.@)
 */
INT CDECL ldap_create_vlv_controlW( LDAP *ld, WLDAP32_LDAPVLVInfo *info, UCHAR critical, LDAPControlW **control )
{
    ULONG ret;
    LDAPVLVInfo *infoU = NULL;
    LDAPControl *controlU;

    TRACE( "(%p, %p, 0x%02x, %p)\n", ld, info, critical, control );

    if (!ld || !control) return ~0u;
    if ((ret = WLDAP32_ldap_connect( ld, NULL ))) return ret;

    if (info && !(infoU = vlvinfoWtoU( info ))) return WLDAP32_LDAP_NO_MEMORY;

    ret = map_error( ldap_create_vlv_control( CTX(ld), infoU, &controlU ) );
    if (ret == WLDAP32_LDAP_SUCCESS)
    {
        LDAPControlW *controlW = controlUtoW( controlU );
        if (controlW) *control = controlW;
        else ret = WLDAP32_LDAP_NO_MEMORY;
        ldap_control_free( controlU );
    }

    vlvinfofreeU( infoU );
    return ret;
}

static inline void bv_val_dup( const struct WLDAP32_berval *src, struct WLDAP32_berval *dst )
{
    if ((dst->bv_val = RtlAllocateHeap( GetProcessHeap(), 0 , src->bv_len )))
    {
        memcpy( dst->bv_val, src->bv_val, src->bv_len );
        dst->bv_len = src->bv_len;
    }
    else
        dst->bv_len = 0;
}

/***********************************************************************
 *      ldap_encode_sort_controlA     (WLDAP32.@)
 */
ULONG CDECL ldap_encode_sort_controlA( LDAP *ld, LDAPSortKeyA **sortkeys, LDAPControlA *ret, BOOLEAN critical )
{
    LDAPControlA *control;
    ULONG result;

    if ((result = ldap_create_sort_controlA( ld, sortkeys, critical, &control )) == WLDAP32_LDAP_SUCCESS)
    {
        ret->ldctl_oid = strdup( control->ldctl_oid );
        bv_val_dup( &control->ldctl_value, &ret->ldctl_value );
        ret->ldctl_iscritical = control->ldctl_iscritical;
        ldap_control_freeA( control );
    }
    return result;
}

/***********************************************************************
 *      ldap_encode_sort_controlW     (WLDAP32.@)
 */
ULONG CDECL ldap_encode_sort_controlW( LDAP *ld, LDAPSortKeyW **sortkeys, LDAPControlW *ret, BOOLEAN critical )
{
    LDAPControlW *control;
    ULONG result;

    if ((result = ldap_create_sort_controlW( ld, sortkeys, critical, &control )) == WLDAP32_LDAP_SUCCESS)
    {
        ret->ldctl_oid = wcsdup( control->ldctl_oid );
        bv_val_dup( &control->ldctl_value, &ret->ldctl_value );
        ret->ldctl_iscritical = control->ldctl_iscritical;
        ldap_control_freeW( control );
    }
    return result;
}

/***********************************************************************
 *      ldap_free_controlsA     (WLDAP32.@)
 */
ULONG CDECL ldap_free_controlsA( LDAPControlA **controls )
{
    return ldap_controls_freeA( controls );
}

/***********************************************************************
 *      ldap_free_controlsW     (WLDAP32.@)
 */
ULONG CDECL ldap_free_controlsW( LDAPControlW **controls )
{
    return ldap_controls_freeW( controls );
}
