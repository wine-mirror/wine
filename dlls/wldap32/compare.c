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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

ULONG ldap_compareA( WLDAP32_LDAP *ld, PCHAR dn, PCHAR attr, PCHAR value )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW = NULL, *attrW = NULL, *valueW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %s, %s)\n", ld, debugstr_a(dn), debugstr_a(attr),
           debugstr_a(value) );

    if (!ld) return ~0UL;

    if (dn) {
        dnW = strAtoW( dn );
        if (!dnW) goto exit;
    }
    if (attr) {
        attrW = strAtoW( attr );
        if (!attrW) goto exit;
    }
    if (value) {
        valueW = strAtoW( value );
        if (!valueW) goto exit;
    }

    ret = ldap_compareW( ld, dnW, attrW, valueW );

exit:
    strfreeW( dnW );
    strfreeW( attrW );
    strfreeW( valueW );

#endif
    return ret;
}

ULONG ldap_compareW( WLDAP32_LDAP *ld, PWCHAR dn, PWCHAR attr, PWCHAR value )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU = NULL, *attrU = NULL, *valueU = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %s, %s)\n", ld, debugstr_w(dn), debugstr_w(attr),
           debugstr_w(value) );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    if (!attr) return ~0UL;

    if (dn) {
        dnU = strWtoU( dn );
        if (!dnU) goto exit;
    }

    attrU = strWtoU( attr );
    if (!attrU) goto exit;

    if (value) {
        valueU = strWtoU( value );
        if (!valueU) goto exit;
    }

    ret = ldap_compare( ld, dn ? dnU : "", attrU, value ? valueU : "" );

exit:
    strfreeU( dnU );
    strfreeU( attrU );
    strfreeU( valueU );

#endif
    return ret;
}

ULONG ldap_compare_extA( WLDAP32_LDAP *ld, PCHAR dn, PCHAR attr, PCHAR value,
    struct WLDAP32_berval *data, PLDAPControlA *serverctrls, PLDAPControlA *clientctrls,
    ULONG *message )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW = NULL, *attrW = NULL, *valueW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %s, %s, %p, %p, %p, %p)\n", ld, debugstr_a(dn),
           debugstr_a(attr), debugstr_a(value), data, serverctrls,
           clientctrls, message );

    if (!ld || !message) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnW = strAtoW( dn );
        if (!dnW) goto exit;
    }
    if (attr) {
        attrW = strAtoW( attr );
        if (!attrW) goto exit;
    }
    if (value) {
        valueW = strAtoW( value );
        if (!valueW) goto exit;
    }
    if (serverctrls) {
        serverctrlsW = controlarrayAtoW( serverctrls );
        if (!serverctrlsW) goto exit;
    }
    if (clientctrls) {
        clientctrlsW = controlarrayAtoW( clientctrls );
        if (!clientctrlsW) goto exit;
    }

    ret = ldap_compare_extW( ld, dnW, attrW, valueW, data,
                             serverctrlsW, clientctrlsW, message );

exit:
    strfreeW( dnW );
    strfreeW( attrW );
    strfreeW( valueW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );

#endif
    return ret;
}

ULONG ldap_compare_extW( WLDAP32_LDAP *ld, PWCHAR dn, PWCHAR attr, PWCHAR value,
    struct WLDAP32_berval *data, PLDAPControlW *serverctrls, PLDAPControlW *clientctrls,
    ULONG *message )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU = NULL, *attrU = NULL, *valueU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;
    struct berval berval;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %s, %s, %p, %p, %p, %p)\n", ld, debugstr_w(dn),
           debugstr_w(attr), debugstr_w(value), data, serverctrls,
           clientctrls, message );

    if (!ld || !message) return WLDAP32_LDAP_PARAM_ERROR;
    if (!attr) return WLDAP32_LDAP_NO_MEMORY;

    if (dn) {
        dnU = strWtoU( dn );
        if (!dnU) goto exit;
    }

    attrU = strWtoU( attr );
    if (!attrU) goto exit;

    if (!data) {
        if (value) {
            valueU = strWtoU( value );
            if (!valueU) goto exit;
        }
        berval.bv_len = valueU ? strlen( valueU ) : 0;
        berval.bv_val = valueU;
    }
    if (serverctrls) {
        serverctrlsU = controlarrayWtoU( serverctrls );
        if (!serverctrlsU) goto exit;
    }
    if (clientctrls) {
        clientctrlsU = controlarrayWtoU( clientctrls );
        if (!clientctrlsU) goto exit;
    }

    ret = ldap_compare_ext( ld, dn ? dnU : "", attrU, data ? (struct berval *)data : &berval,
                            serverctrlsU, clientctrlsU, (int *)message );

exit:
    strfreeU( dnU );
    strfreeU( attrU );
    strfreeU( valueU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );

#endif
    return ret;
}

ULONG ldap_compare_ext_sA( WLDAP32_LDAP *ld, PCHAR dn, PCHAR attr, PCHAR value,
    struct WLDAP32_berval *data, PLDAPControlA *serverctrls, PLDAPControlA *clientctrls )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW = NULL, *attrW = NULL, *valueW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %s, %s, %p, %p, %p)\n", ld, debugstr_a(dn),
           debugstr_a(attr), debugstr_a(value), data, serverctrls,
           clientctrls );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnW = strAtoW( dn );
        if (!dnW) goto exit;
    }
    if (attr) {
        attrW = strAtoW( attr );
        if (!attrW) goto exit;
    }
    if (value) {
        valueW = strAtoW( value );
        if (!valueW) goto exit;
    }
    if (serverctrls) {
        serverctrlsW = controlarrayAtoW( serverctrls );
        if (!serverctrlsW) goto exit;
    }
    if (clientctrls) {
        clientctrlsW = controlarrayAtoW( clientctrls );
        if (!clientctrlsW) goto exit;
    }

    ret = ldap_compare_ext_sW( ld, dnW, attrW, valueW, data, serverctrlsW,
                               clientctrlsW );

exit:
    strfreeW( dnW );
    strfreeW( attrW );
    strfreeW( valueW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );

#endif
    return ret;
}

ULONG ldap_compare_ext_sW( WLDAP32_LDAP *ld, PWCHAR dn, PWCHAR attr, PWCHAR value,
    struct WLDAP32_berval *data, PLDAPControlW *serverctrls, PLDAPControlW *clientctrls )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    struct berval berval;
    char *dnU = NULL, *attrU = NULL, *valueU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %s, %s, %p, %p, %p)\n", ld, debugstr_w(dn),
           debugstr_w(attr), debugstr_w(value), data, serverctrls,
           clientctrls );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnU = strWtoU( dn );
        if (!dnU) goto exit;
    }
    if (attr) {
        attrU = strWtoU( attr );
        if (!attrU) goto exit;
    }
    if (!data) {
        if (value) {
            valueU = strWtoU( value );
            if (!valueU) goto exit;
        }
        berval.bv_len = valueU ? strlen( valueU ) : 0;
        berval.bv_val = valueU;
    }
    if (serverctrls) {
        serverctrlsU = controlarrayWtoU( serverctrls );
        if (!serverctrlsU) goto exit;
    }
    if (clientctrls) {
        clientctrlsU = controlarrayWtoU( clientctrls );
        if (!clientctrlsU) goto exit;
    }

    ret = ldap_compare_ext_s( ld, dn ? dnU : "", attr ? attrU : "", data ? (struct berval *)data : &berval,
                              serverctrlsU, clientctrlsU );

exit:
    strfreeU( dnU );
    strfreeU( attrU );
    strfreeU( valueU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );

#endif
    return ret;
}

ULONG ldap_compare_sA( WLDAP32_LDAP *ld, PCHAR dn, PCHAR attr, PCHAR value )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW = NULL, *attrW = NULL, *valueW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %s, %s)\n", ld, debugstr_a(dn), debugstr_a(attr),
           debugstr_a(value) );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnW = strAtoW( dn );
        if (!dnW) goto exit;
    }
    if (attr) {
        attrW = strAtoW( attr );
        if (!attrW) goto exit;
    }
    if (value) {
        valueW = strAtoW( value );
        if (!valueW) goto exit;
    }

    ret = ldap_compare_sW( ld, dnW, attrW, valueW );

exit:
    strfreeW( dnW );
    strfreeW( attrW );
    strfreeW( valueW );

#endif
    return ret;
}

ULONG ldap_compare_sW( WLDAP32_LDAP *ld, PWCHAR dn, PWCHAR attr, PWCHAR value )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU = NULL, *attrU = NULL, *valueU = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %s, %s)\n", ld, debugstr_w(dn), debugstr_w(attr),
           debugstr_w(value) );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnU = strWtoU( dn );
        if (!dnU) goto exit;
    }
    if (attr) {
        attrU = strWtoU( attr );
        if (!attrU) goto exit;
    }
    if (value) {
        valueU = strWtoU( value );
        if (!valueU) goto exit;
    }

    ret = ldap_compare_s( ld, dn ? dnU : "", attr ? attrU : "", value ? valueU : "" );

exit:
    strfreeU( dnU );
    strfreeU( attrU );
    strfreeU( valueU );

#endif
    return ret;
}
