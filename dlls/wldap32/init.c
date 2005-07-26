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

/* Should eventually be determined by the algorithm documented on MSDN. */
static const WCHAR defaulthost[] = { 'l','o','c','a','l','h','o','s','t',0 };

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

WLDAP32_LDAP *ldap_initA( PCHAR hostname, ULONG portnumber )
{
#ifdef HAVE_LDAP
    WLDAP32_LDAP *ld = NULL;
    WCHAR *hostnameW = NULL;

    TRACE( "(%s, %ld)\n", debugstr_a(hostname), portnumber );

    if (hostname) {
        hostnameW = strAtoW( hostname );
        if (!hostnameW) goto exit;
    }

    ld = ldap_initW( hostnameW, portnumber );

exit:
    strfreeW( hostnameW );
    return ld;

#endif
    return NULL;
}

WLDAP32_LDAP *ldap_initW( PWCHAR hostname, ULONG portnumber )
{
#ifdef HAVE_LDAP
    LDAP *ld = NULL;
    char *hostnameU = NULL;

    TRACE( "(%s, %ld)\n", debugstr_w(hostname), portnumber );

    if (hostname) {
        hostnameU = strWtoU( hostname );
        if (!hostnameU) goto exit;
    }
    else {
        hostnameU = strWtoU( defaulthost );
        if (!hostnameU) goto exit;
    }

    ld = ldap_init( hostnameU, portnumber );

exit:
    strfreeU( hostnameU );
    return ld;

#endif
    return NULL;
}

WLDAP32_LDAP *ldap_openA( PCHAR hostname, ULONG portnumber )
{
#ifdef HAVE_LDAP
    WLDAP32_LDAP *ld = NULL;
    WCHAR *hostnameW = NULL;

    TRACE( "(%s, %ld)\n", debugstr_a(hostname), portnumber );

    if (hostname) {
        hostnameW = strAtoW( hostname );
        if (!hostnameW) goto exit;
    }

    ld = ldap_openW( hostnameW, portnumber );

exit:
    strfreeW( hostnameW );
    return ld;

#endif
    return NULL;
}

WLDAP32_LDAP *ldap_openW( PWCHAR hostname, ULONG portnumber )
{
#ifdef HAVE_LDAP
    LDAP *ld = NULL;
    char *hostnameU = NULL;

    TRACE( "(%s, %ld)\n", debugstr_w(hostname), portnumber );

    if (hostname) {
        hostnameU = strWtoU( hostname );
        if (!hostnameU) goto exit;
    }
    else {
        hostnameU = strWtoU( defaulthost );
        if (!hostnameU) goto exit;
    }

    ld = ldap_open( hostnameU, portnumber );

exit:
    strfreeU( hostnameU );
    return ld;

#endif
    return NULL;
}

ULONG ldap_start_tls_sA( WLDAP32_LDAP *ld, PULONG retval, WLDAP32_LDAPMessage **result,
    PLDAPControlA *serverctrls, PLDAPControlA *clientctrls )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;
    ret = LDAP_NO_MEMORY;

    TRACE( "(%p, %p, %p, %p, %p)\n", ld, retval, result, serverctrls, clientctrls );

    if (!ld) return ~0UL;

    if (serverctrls) {
        serverctrlsW = controlarrayAtoW( serverctrls );
        if (!serverctrlsW) goto exit;
    }
    if (clientctrls) {
        clientctrlsW = controlarrayAtoW( clientctrls );
        if (!clientctrlsW) goto exit;
    }

    ret = ldap_start_tls_sW( ld, retval, result, serverctrlsW, clientctrlsW );

exit:
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );

#endif
    return ret;
}

ULONG ldap_start_tls_sW( WLDAP32_LDAP *ld, PULONG retval, WLDAP32_LDAPMessage **result,
    PLDAPControlW *serverctrls, PLDAPControlW *clientctrls )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;
    ret = LDAP_NO_MEMORY;

    TRACE( "(%p, %p, %p, %p, %p)\n", ld, retval, result, serverctrls, clientctrls );

    if (!ld) return ~0UL;

    if (serverctrls) {
        serverctrlsU = controlarrayWtoU( serverctrls );
        if (!serverctrlsU) goto exit;
    }
    if (clientctrls) {
        clientctrlsU = controlarrayWtoU( clientctrls );
        if (!clientctrlsU) goto exit;
    }

    ret = ldap_start_tls_s( ld, serverctrlsU, clientctrlsU );

exit:
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );

#endif
    return ret;
}
