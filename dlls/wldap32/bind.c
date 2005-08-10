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
#define LDAP_SUCCESS        0x00
#define LDAP_NOT_SUPPORTED  0x5c
#endif

#include "winldap_private.h"
#include "wldap32.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

ULONG ldap_bindA( WLDAP32_LDAP *ld, PCHAR dn, PCHAR cred, ULONG method )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW = NULL, *credW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p, 0x%08lx)\n", ld, debugstr_a(dn), cred, method );

    if (!ld) return ~0UL;

    if (dn) {
        dnW = strAtoW( dn );
        if (!dnW) goto exit;
    }
    if (cred) {
        credW = strAtoW( cred );
        if (!credW) goto exit;
    }

    ret = ldap_bindW( ld, dnW, credW, method );

exit:
    strfreeW( dnW );
    strfreeW( credW );

#endif
    return ret;
}

ULONG ldap_bindW( WLDAP32_LDAP *ld, PWCHAR dn, PWCHAR cred, ULONG method )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU = NULL, *credU = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p, 0x%08lx)\n", ld, debugstr_w(dn), cred, method );

    if (!ld) return ~0UL;

    if (dn) {
        dnU = strWtoU( dn );
        if (!dnU) goto exit;
    }
    if (cred) {
        credU = strWtoU( cred );
        if (!credU) goto exit;
    }

    ret = ldap_bind( ld, dnU, credU, method );

exit:
    strfreeU( dnU );
    strfreeU( credU );

#endif
    return ret;
}

ULONG ldap_bind_sA( WLDAP32_LDAP *ld, PCHAR dn, PCHAR cred, ULONG method )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW = NULL, *credW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p, 0x%08lx)\n", ld, debugstr_a(dn), cred, method );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnW = strAtoW( dn );
        if (!dnW) goto exit;
    }
    if (cred) {
        credW = strAtoW( cred );
        if (!credW) goto exit;
    }

    ret = ldap_bind_sW( ld, dnW, credW, method );

exit:
    strfreeW( dnW );
    strfreeW( credW );

#endif
    return ret;
}

ULONG ldap_bind_sW( WLDAP32_LDAP *ld, PWCHAR dn, PWCHAR cred, ULONG method )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU = NULL, *credU = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p, 0x%08lx)\n", ld, debugstr_w(dn), cred, method );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnU = strWtoU( dn );
        if (!dnU) goto exit;
    }
    if (cred) {
        credU = strWtoU( cred );
        if (!credU) goto exit;
    }

    ret = ldap_bind_s( ld, dnU, credU, method );

exit:
    strfreeU( dnU );
    strfreeU( credU );

#endif
    return ret;
}

ULONG ldap_sasl_bindA( WLDAP32_LDAP *ld, const PCHAR dn,
    const PCHAR mechanism, const BERVAL *cred, PLDAPControlA *serverctrls,
    PLDAPControlA *clientctrls, int *message )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW, *mechanismW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %s, %p, %p, %p, %p)\n", ld, debugstr_a(dn),
           debugstr_a(mechanism), cred, serverctrls, clientctrls, message );

    if (!ld || !dn || !mechanism || !cred || !message)
        return WLDAP32_LDAP_PARAM_ERROR;

    dnW = strAtoW( dn );
    if (!dnW) goto exit;

    mechanismW = strAtoW( mechanism );
    if (!mechanismW) goto exit;

    if (serverctrls) {
        serverctrlsW = controlarrayAtoW( serverctrls );
        if (!serverctrlsW) goto exit;
    }
    if (clientctrls) {
        clientctrlsW = controlarrayAtoW( clientctrls );
        if (!clientctrlsW) goto exit;
    }

    ret = ldap_sasl_bindW( ld, dnW, mechanismW, cred, serverctrlsW, clientctrlsW, message );

exit:
    strfreeW( dnW );
    strfreeW( mechanismW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );

#endif
    return ret;
}

ULONG ldap_sasl_bindW( WLDAP32_LDAP *ld, const PWCHAR dn,
    const PWCHAR mechanism, const BERVAL *cred, PLDAPControlW *serverctrls,
    PLDAPControlW *clientctrls, int *message )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU, *mechanismU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %s, %p, %p, %p, %p)\n", ld, debugstr_w(dn),
           debugstr_w(mechanism), cred, serverctrls, clientctrls, message );

    if (!ld || !dn || !mechanism || !cred || !message)
        return WLDAP32_LDAP_PARAM_ERROR;

    dnU = strWtoU( dn );
    if (!dnU) goto exit;

    mechanismU = strWtoU( mechanism );
    if (!mechanismU) goto exit;

    if (serverctrls) {
        serverctrlsU = controlarrayWtoU( serverctrls );
        if (!serverctrlsU) goto exit;
    }
    if (clientctrls) {
        clientctrlsU = controlarrayWtoU( clientctrls );
        if (!clientctrlsU) goto exit;
    }

    ret = ldap_sasl_bind( ld, dnU, mechanismU, (struct berval *)cred,
                          serverctrlsU, clientctrlsU, message );

exit:
    strfreeU( dnU );
    strfreeU( mechanismU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );

#endif
    return ret;
}

ULONG ldap_sasl_bind_sA( WLDAP32_LDAP *ld, const PCHAR dn,
    const PCHAR mechanism, const BERVAL *cred, PLDAPControlA *serverctrls,
    PLDAPControlA *clientctrls, PBERVAL *serverdata )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW, *mechanismW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %s, %p, %p, %p, %p)\n", ld, debugstr_a(dn),
           debugstr_a(mechanism), cred, serverctrls, clientctrls, serverdata );

    if (!ld || !dn || !mechanism || !cred || !serverdata)
        return WLDAP32_LDAP_PARAM_ERROR;

    dnW = strAtoW( dn );
    if (!dnW) goto exit;

    mechanismW = strAtoW( mechanism );
    if (!mechanismW) goto exit;

    if (serverctrls) {
        serverctrlsW = controlarrayAtoW( serverctrls );
        if (!serverctrlsW) goto exit;
    }
    if (clientctrls) {
        clientctrlsW = controlarrayAtoW( clientctrls );
        if (!clientctrlsW) goto exit;
    }

    ret = ldap_sasl_bind_sW( ld, dnW, mechanismW, cred, serverctrlsW, clientctrlsW, serverdata );

exit:
    strfreeW( dnW );
    strfreeW( mechanismW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );

#endif
    return ret;
}

ULONG ldap_sasl_bind_sW( WLDAP32_LDAP *ld, const PWCHAR dn,
    const PWCHAR mechanism, const BERVAL *cred, PLDAPControlW *serverctrls,
    PLDAPControlW *clientctrls, PBERVAL *serverdata )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU, *mechanismU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %s, %p, %p, %p, %p)\n", ld, debugstr_w(dn),
           debugstr_w(mechanism), cred, serverctrls, clientctrls, serverdata );

    if (!ld || !dn || !mechanism || !cred || !serverdata)
        return WLDAP32_LDAP_PARAM_ERROR;

    dnU = strWtoU( dn );
    if (!dnU) goto exit;

    mechanismU = strWtoU( mechanism );
    if (!mechanismU) goto exit;

    if (serverctrls) {
        serverctrlsU = controlarrayWtoU( serverctrls );
        if (!serverctrlsU) goto exit;
    }
    if (clientctrls) {
        clientctrlsU = controlarrayWtoU( clientctrls );
        if (!clientctrlsU) goto exit;
    }

    ret = ldap_sasl_bind_s( ld, dnU, mechanismU, (struct berval *)cred,
                            serverctrlsU, clientctrlsU, (struct berval **)serverdata );

exit:
    strfreeU( dnU );
    strfreeU( mechanismU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );

#endif
    return ret;
}

ULONG ldap_simple_bindA( WLDAP32_LDAP *ld, PCHAR dn, PCHAR passwd )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW = NULL, *passwdW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_a(dn), passwd );

    if (!ld) return ~0UL;

    if (dn) {
        dnW = strAtoW( dn );
        if (!dnW) goto exit;
    }
    if (passwd) {
        passwdW = strAtoW( passwd );
        if (!passwdW) goto exit;
    }

    ret = ldap_simple_bindW( ld, dnW, passwdW );

exit:
    strfreeW( dnW );
    strfreeW( passwdW );

#endif
    return ret;
}

ULONG ldap_simple_bindW( WLDAP32_LDAP *ld, PWCHAR dn, PWCHAR passwd )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU = NULL, *passwdU = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_w(dn), passwd );

    if (!ld) return ~0UL;

    if (dn) {
        dnU = strWtoU( dn );
        if (!dnU) goto exit;
    }
    if (passwd) {
        passwdU = strWtoU( passwd );
        if (!passwdU) goto exit;
    }

    ret = ldap_simple_bind( ld, dnU, passwdU );

exit:
    strfreeU( dnU );
    strfreeU( passwdU );

#endif
    return ret;
}

ULONG ldap_simple_bind_sA( WLDAP32_LDAP *ld, PCHAR dn, PCHAR passwd )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW = NULL, *passwdW = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_a(dn), passwd );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnW = strAtoW( dn );
        if (!dnW) goto exit;
    }
    if (passwd) {
        passwdW = strAtoW( passwd );
        if (!passwdW) goto exit;
    }

    ret = ldap_simple_bind_sW( ld, dnW, passwdW );

exit:
    strfreeW( dnW );
    strfreeW( passwdW );

#endif
    return ret;
}

ULONG ldap_simple_bind_sW( WLDAP32_LDAP *ld, PWCHAR dn, PWCHAR passwd )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU = NULL, *passwdU = NULL;

    ret = WLDAP32_LDAP_NO_MEMORY;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_w(dn), passwd );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (dn) {
        dnU = strWtoU( dn );
        if (!dnU) goto exit;
    }
    if (passwd) {
        passwdU = strWtoU( passwd );
        if (!passwdU) goto exit;
    }

    ret = ldap_simple_bind_s( ld, dnU, passwdU );

exit:
    strfreeU( dnU );
    strfreeU( passwdU );

#endif
    return ret;
}

ULONG WLDAP32_ldap_unbind( WLDAP32_LDAP *ld )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP

    TRACE( "(%p)\n", ld );

    if (ld)
        ret = ldap_unbind( ld );
    else
        ret = WLDAP32_LDAP_PARAM_ERROR;

#endif
    return ret;
}

ULONG WLDAP32_ldap_unbind_s( WLDAP32_LDAP *ld )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP

    TRACE( "(%p)\n", ld );

    if (ld)
        ret = ldap_unbind_s( ld );
    else
        ret = WLDAP32_LDAP_PARAM_ERROR;

#endif
    return ret;
}
