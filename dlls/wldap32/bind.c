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

#ifdef HAVE_LDAP
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
    WCHAR *dnW, *credW;

    TRACE( "(%p, %s, %p, 0x%08lx)\n", ld, debugstr_a(dn), cred, method );

    dnW = strAtoW( dn );
    if (!dnW) return LDAP_NO_MEMORY;

    credW = strAtoW( cred );
    if (!credW) return LDAP_NO_MEMORY;

    ret = ldap_bindW( ld, dnW, credW, method );

    strfreeW( dnW );
    strfreeW( credW );

#endif
    return ret;
}

ULONG ldap_bindW( WLDAP32_LDAP *ld, PWCHAR dn, PWCHAR cred, ULONG method )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU, *credU;

    TRACE( "(%p, %s, %p, 0x%08lx)\n", ld, debugstr_w(dn), cred, method );

    dnU = strWtoU( dn );
    if (!dnU) return LDAP_NO_MEMORY;

    credU = strWtoU( cred );
    if (!credU) return LDAP_NO_MEMORY;

    ret = ldap_bind( ld, dnU, credU, method );

    strfreeU( dnU );
    strfreeU( credU );

#endif
    return ret;
}

ULONG ldap_bind_sA( WLDAP32_LDAP *ld, PCHAR dn, PCHAR cred, ULONG method )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW, *credW;

    TRACE( "(%p, %s, %p, 0x%08lx)\n", ld, debugstr_a(dn), cred, method );

    dnW = strAtoW( dn );
    if (!dnW) return LDAP_NO_MEMORY;

    credW = strAtoW( cred );
    if (!credW) return LDAP_NO_MEMORY;

    ret = ldap_bind_sW( ld, dnW, credW, method );

    strfreeW( dnW );
    strfreeW( credW );

#endif
    return ret;
}

ULONG ldap_bind_sW( WLDAP32_LDAP *ld, PWCHAR dn, PWCHAR cred, ULONG method )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU, *credU;

    TRACE( "(%p, %s, %p, 0x%08lx)\n", ld, debugstr_w(dn), cred, method );

    dnU = strWtoU( dn );
    if (!dnU) return LDAP_NO_MEMORY;

    credU = strWtoU( cred );
    if (!credU) return LDAP_NO_MEMORY;

    ret = ldap_bind_s( ld, dnU, credU, method );

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
    WCHAR *dnW, *mechanismW;
    LDAPControlW **serverctrlsW, **clientctrlsW;

    TRACE( "(%p, %s, %s, %p, %p, %p, %p)\n", ld, debugstr_a(dn),
           debugstr_a(mechanism), cred, serverctrls, clientctrls, message );

    dnW = strAtoW( dn );
    if (!dnW) return LDAP_NO_MEMORY;

    mechanismW = strAtoW( mechanism );
    if (!mechanismW) return LDAP_NO_MEMORY;

    serverctrlsW = controlarrayAtoW( serverctrls );
    if (!serverctrlsW) return LDAP_NO_MEMORY;

    clientctrlsW = controlarrayAtoW( clientctrls );
    if (!clientctrlsW) return LDAP_NO_MEMORY;

    ret = ldap_sasl_bindW( ld, dnW, mechanismW, cred, serverctrlsW, clientctrlsW, message );

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
    char *dnU, *mechanismU;
    LDAPControl **serverctrlsU, **clientctrlsU;

    TRACE( "(%p, %s, %s, %p, %p, %p, %p)\n", ld, debugstr_w(dn),
           debugstr_w(mechanism), cred, serverctrls, clientctrls, message );

    dnU = strWtoU( dn );
    if (!dnU) return LDAP_NO_MEMORY;

    mechanismU = strWtoU( mechanism );
    if (!mechanismU) return LDAP_NO_MEMORY;

    serverctrlsU = controlarrayWtoU( serverctrls );
    if (!serverctrlsU) return LDAP_NO_MEMORY;

    clientctrlsU = controlarrayWtoU( clientctrls );
    if (!clientctrlsU) return LDAP_NO_MEMORY;

    ret = ldap_sasl_bind( ld, dnU, mechanismU, (struct berval *)cred,
                          serverctrlsU, clientctrlsU, message );

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
    WCHAR *dnW, *mechanismW;
    LDAPControlW **serverctrlsW, **clientctrlsW;

    TRACE( "(%p, %s, %s, %p, %p, %p, %p)\n", ld, debugstr_a(dn),
           debugstr_a(mechanism), cred, serverctrls, clientctrls, serverdata );

    dnW = strAtoW( dn );
    if (!dnW) return LDAP_NO_MEMORY;

    mechanismW = strAtoW( mechanism );
    if (!mechanismW) return LDAP_NO_MEMORY;

    serverctrlsW = controlarrayAtoW( serverctrls );
    if (!serverctrlsW) return LDAP_NO_MEMORY;

    clientctrlsW = controlarrayAtoW( clientctrls );
    if (!clientctrlsW) return LDAP_NO_MEMORY;

    ret = ldap_sasl_bind_sW( ld, dnW, mechanismW, cred, serverctrlsW, clientctrlsW, serverdata );

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
    char *dnU, *mechanismU;
    LDAPControl **serverctrlsU, **clientctrlsU;

    TRACE( "(%p, %s, %s, %p, %p, %p, %p)\n", ld, debugstr_w(dn),
           debugstr_w(mechanism), cred, serverctrls, clientctrls, serverdata );

    dnU = strWtoU( dn );
    if (!dnU) return LDAP_NO_MEMORY;

    mechanismU = strWtoU( mechanism );
    if (!mechanismU) return LDAP_NO_MEMORY;

    serverctrlsU = controlarrayWtoU( serverctrls );
    if (!serverctrlsU) return LDAP_NO_MEMORY;

    clientctrlsU = controlarrayWtoU( clientctrls );
    if (!clientctrlsU) return LDAP_NO_MEMORY;

    ret = ldap_sasl_bind_s( ld, dnU, mechanismU, (struct berval *)cred,
                            serverctrlsU, clientctrlsU, (struct berval **)serverdata );

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
    WCHAR *dnW, *passwdW;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_a(dn), passwd );

    dnW = strAtoW( dn );
    if (!dnW) return LDAP_NO_MEMORY;

    passwdW = strAtoW( passwd );
    if (!passwdW) return LDAP_NO_MEMORY;

    ret = ldap_simple_bindW( ld, dnW, passwdW );

    strfreeW( dnW );
    strfreeW( passwdW );

#endif
    return ret;
}

ULONG ldap_simple_bindW( WLDAP32_LDAP *ld, PWCHAR dn, PWCHAR passwd )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU, *passwdU;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_w(dn), passwd );

    dnU = strWtoU( dn );
    if (!dnU) return LDAP_NO_MEMORY;

    passwdU = strWtoU( passwd );
    if (!passwdU) return LDAP_NO_MEMORY;

    ret = ldap_simple_bind( ld, dnU, passwdU );

    strfreeU( dnU );
    strfreeU( passwdU );

#endif
    return ret;
}

ULONG ldap_simple_bind_sA( WLDAP32_LDAP *ld, PCHAR dn, PCHAR passwd )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *dnW, *passwdW;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_a(dn), passwd );

    dnW = strAtoW( dn );
    if (!dnW) return LDAP_NO_MEMORY;

    passwdW = strAtoW( passwd );
    if (!passwdW) return LDAP_NO_MEMORY;

    ret = ldap_simple_bind_sW( ld, dnW, passwdW );

    strfreeW( dnW );
    strfreeW( passwdW );

#endif
    return ret;
}

ULONG ldap_simple_bind_sW( WLDAP32_LDAP *ld, PWCHAR dn, PWCHAR passwd )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *dnU, *passwdU;

    TRACE( "(%p, %s, %p)\n", ld, debugstr_w(dn), passwd );

    dnU = strWtoU( dn );
    if (!dnU) return LDAP_NO_MEMORY;

    passwdU = strWtoU( passwd );
    if (!passwdU) return LDAP_NO_MEMORY;

    ret = ldap_simple_bind_s( ld, dnU, passwdU );

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
    ret = ldap_unbind( ld );

#endif
    return ret;
}

ULONG WLDAP32_ldap_unbind_s( WLDAP32_LDAP *ld )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP

    TRACE( "(%p)\n", ld );
    ret = ldap_unbind_s( ld );

#endif
    return ret;
}
