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

ULONG ldap_searchA( WLDAP32_LDAP *ld, PCHAR base, ULONG scope, PCHAR filter,
    PCHAR attrs[], ULONG attrsonly )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *baseW, *filterW, **attrsW;

    TRACE( "(%p, %s, 0x%08lx, %s, %p, 0x%08lx)\n", ld, debugstr_a(base),
           scope, debugstr_a(filter), attrs, attrsonly );

    baseW = strAtoW( base );
    if (!baseW) return LDAP_NO_MEMORY;

    filterW = strAtoW( filter );
    if (!filterW) return LDAP_NO_MEMORY;

    attrsW = strarrayAtoW( attrs );
    if (!attrsW) return LDAP_NO_MEMORY;

    ret = ldap_searchW( ld, baseW, scope, filterW, attrsW, attrsonly );

    strfreeW( baseW );
    strfreeW( filterW );
    strarrayfreeW( attrsW );

#endif
    return ret;
}

ULONG ldap_searchW( WLDAP32_LDAP *ld, PWCHAR base, ULONG scope, PWCHAR filter,
    PWCHAR attrs[], ULONG attrsonly )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *baseU, *filterU, **attrsU;

    TRACE( "(%p, %s, 0x%08lx, %s, %p, 0x%08lx)\n", ld, debugstr_w(base),
           scope, debugstr_w(filter), attrs, attrsonly );

    baseU = strWtoU( base );
    if (!baseU) return LDAP_NO_MEMORY;

    filterU = strWtoU( filter );
    if (!filterU) return LDAP_NO_MEMORY;

    attrsU = strarrayWtoU( attrs );
    if (!attrsU) return LDAP_NO_MEMORY;

    ret = ldap_search( ld, baseU, scope, filterU, attrsU, attrsonly );

    strfreeU( baseU );
    strfreeU( filterU );
    strarrayfreeU( attrsU );

#endif
    return ret;
}

ULONG ldap_search_extA( WLDAP32_LDAP *ld, PCHAR base, ULONG scope,
    PCHAR filter, PCHAR attrs[], ULONG attrsonly, PLDAPControlA *serverctrls,
    PLDAPControlA *clientctrls, ULONG timelimit, ULONG sizelimit, ULONG *message )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *baseW, *filterW, **attrsW;
    LDAPControlW **serverctrlsW, **clientctrlsW;

    TRACE( "(%p, %s, 0x%08lx, %s, %p, 0x%08lx, %p, %p, 0x%08lx, 0x%08lx, %p)\n",
           ld, debugstr_a(base), scope, debugstr_a(filter), attrs, attrsonly,
           serverctrls, clientctrls, timelimit, sizelimit, message );

    if (!ld) return ~0UL;

    baseW = strAtoW( base );
    if (!baseW) return LDAP_NO_MEMORY;

    filterW = strAtoW( filter );
    if (!filterW) return LDAP_NO_MEMORY;

    attrsW = strarrayAtoW( attrs );
    if (!attrsW) return LDAP_NO_MEMORY;

    serverctrlsW = controlarrayAtoW( serverctrls );
    if (!serverctrlsW) return LDAP_NO_MEMORY;

    clientctrlsW = controlarrayAtoW( clientctrls );
    if (!clientctrlsW) return LDAP_NO_MEMORY;

    ret = ldap_search_extW( ld, baseW, scope, filterW, attrsW, attrsonly,
                            serverctrlsW, clientctrlsW, timelimit, sizelimit, message );

    strfreeW( baseW );
    strfreeW( filterW );
    strarrayfreeW( attrsW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );

#endif
    return ret;
}

ULONG ldap_search_extW( WLDAP32_LDAP *ld, PWCHAR base, ULONG scope,
    PWCHAR filter, PWCHAR attrs[], ULONG attrsonly, PLDAPControlW *serverctrls,
    PLDAPControlW *clientctrls, ULONG timelimit, ULONG sizelimit, ULONG *message )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *baseU, *filterU, **attrsU;
    LDAPControl **serverctrlsU, **clientctrlsU;
    struct timeval tv;

    TRACE( "(%p, %s, 0x%08lx, %s, %p, 0x%08lx, %p, %p, 0x%08lx, 0x%08lx, %p)\n",
           ld, debugstr_w(base), scope, debugstr_w(filter), attrs, attrsonly,
           serverctrls, clientctrls, timelimit, sizelimit, message );

    if (!ld) return ~0UL;

    baseU = strWtoU( base );
    if (!baseU) return LDAP_NO_MEMORY;

    filterU = strWtoU( filter );
    if (!filterU) return LDAP_NO_MEMORY;

    attrsU = strarrayWtoU( attrs );
    if (!attrsU) return LDAP_NO_MEMORY;

    serverctrlsU = controlarrayWtoU( serverctrls );
    if (!serverctrlsU) return LDAP_NO_MEMORY;

    clientctrlsU = controlarrayWtoU( clientctrls );
    if (!clientctrlsU) return LDAP_NO_MEMORY;

    tv.tv_sec = timelimit;
    tv.tv_usec = 0;

    ret = ldap_search_ext( ld, baseU, scope, filterU, attrsU, attrsonly,
                           serverctrlsU, clientctrlsU, &tv, sizelimit, (int *)message );

    strfreeU( baseU );
    strfreeU( filterU );
    strarrayfreeU( attrsU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );

#endif
    return ret;
}

ULONG ldap_search_ext_sA( WLDAP32_LDAP *ld, PCHAR base, ULONG scope,
    PCHAR filter, PCHAR attrs[], ULONG attrsonly, PLDAPControlA *serverctrls,
    PLDAPControlA *clientctrls, ULONG timelimit, ULONG sizelimit, WLDAP32_LDAPMessage **res )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *baseW, *filterW, **attrsW;
    LDAPControlW **serverctrlsW, **clientctrlsW;

    TRACE( "(%p, %s, 0x%08lx, %s, %p, 0x%08lx, %p, %p, 0x%08lx, 0x%08lx, %p)\n",
           ld, debugstr_a(base), scope, debugstr_a(filter), attrs, attrsonly,
           serverctrls, clientctrls, timelimit, sizelimit, res );

    if (!ld) return ~0UL;

    baseW = strAtoW( base );
    if (!baseW) return LDAP_NO_MEMORY;

    filterW = strAtoW( filter );
    if (!filterW) return LDAP_NO_MEMORY;

    attrsW = strarrayAtoW( attrs );
    if (!attrsW) return LDAP_NO_MEMORY;

    serverctrlsW = controlarrayAtoW( serverctrls );
    if (!serverctrlsW) return LDAP_NO_MEMORY;

    clientctrlsW = controlarrayAtoW( clientctrls );
    if (!clientctrlsW) return LDAP_NO_MEMORY;

    ret = ldap_search_ext_sW( ld, baseW, scope, filterW, attrsW, attrsonly,
                              serverctrlsW, clientctrlsW, timelimit, sizelimit, res );

    strfreeW( baseW );
    strfreeW( filterW );
    strarrayfreeW( attrsW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );

#endif
    return ret;
}

ULONG ldap_search_ext_sW( WLDAP32_LDAP *ld, PWCHAR base, ULONG scope,
    PWCHAR filter, PWCHAR attrs[], ULONG attrsonly, PLDAPControlW *serverctrls,
    PLDAPControlW *clientctrls, ULONG timelimit, ULONG sizelimit, WLDAP32_LDAPMessage **res )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *baseU, *filterU, **attrsU;
    LDAPControl **serverctrlsU, **clientctrlsU;
    struct timeval tv;

    TRACE( "(%p, %s, 0x%08lx, %s, %p, 0x%08lx, %p, %p, 0x%08lx, 0x%08lx, %p)\n",
           ld, debugstr_w(base), scope, debugstr_w(filter), attrs, attrsonly,
           serverctrls, clientctrls, timelimit, sizelimit, res );

    if (!ld) return ~0UL;

    baseU = strWtoU( base );
    if (!baseU) return LDAP_NO_MEMORY;

    filterU = strWtoU( filter );
    if (!filterU) return LDAP_NO_MEMORY;

    attrsU = strarrayWtoU( attrs );
    if (!attrsU) return LDAP_NO_MEMORY;

    serverctrlsU = controlarrayWtoU( serverctrls );
    if (!serverctrlsU) return LDAP_NO_MEMORY;

    clientctrlsU = controlarrayWtoU( clientctrls );
    if (!clientctrlsU) return LDAP_NO_MEMORY;

    tv.tv_sec = timelimit;
    tv.tv_usec = 0;

    ret = ldap_search_ext_s( ld, baseU, scope, filterU, attrsU, attrsonly,
                             serverctrlsU, clientctrlsU, &tv, sizelimit, res );

    strfreeU( baseU );
    strfreeU( filterU );
    strarrayfreeU( attrsU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );

#endif
    return ret;
}

ULONG ldap_search_sA( WLDAP32_LDAP *ld, PCHAR base, ULONG scope, PCHAR filter,
    PCHAR attrs[], ULONG attrsonly, WLDAP32_LDAPMessage **res )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *baseW, *filterW, **attrsW;

    TRACE( "(%p, %s, 0x%08lx, %s, %p, 0x%08lx, %p)\n", ld, debugstr_a(base),
           scope, debugstr_a(filter), attrs, attrsonly, res );

    baseW = strAtoW( base );
    if (!baseW) return LDAP_NO_MEMORY;

    filterW = strAtoW( filter );
    if (!filterW) return LDAP_NO_MEMORY;

    attrsW = strarrayAtoW( attrs );
    if (!attrsW) return LDAP_NO_MEMORY;

    ret = ldap_search_sW( ld, baseW, scope, filterW, attrsW, attrsonly, res );

    strfreeW( baseW );
    strfreeW( filterW );
    strarrayfreeW( attrsW );

#endif
    return ret;
}

ULONG ldap_search_sW( WLDAP32_LDAP *ld, PWCHAR base, ULONG scope, PWCHAR filter,
    PWCHAR attrs[], ULONG attrsonly, WLDAP32_LDAPMessage **res )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *baseU, *filterU, **attrsU;

    TRACE( "(%p, %s, 0x%08lx, %s, %p, 0x%08lx, %p)\n", ld, debugstr_w(base),
           scope, debugstr_w(filter), attrs, attrsonly, res );

    baseU = strWtoU( base );
    if (!baseU) return LDAP_NO_MEMORY;

    filterU = strWtoU( filter );
    if (!filterU) return LDAP_NO_MEMORY;

    attrsU = strarrayWtoU( attrs );
    if (!attrsU) return LDAP_NO_MEMORY;

    ret = ldap_search_s( ld, baseU, scope, filterU, attrsU, attrsonly, res );

    strfreeU( baseU );
    strfreeU( filterU );
    strarrayfreeU( attrsU );

#endif
    return ret;
}

ULONG ldap_search_stA( WLDAP32_LDAP *ld, const PCHAR base, ULONG scope,
    const PCHAR filter, PCHAR attrs[], ULONG attrsonly,
    struct l_timeval *timeout, WLDAP32_LDAPMessage **res )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    WCHAR *baseW, *filterW, **attrsW;

    TRACE( "(%p, %s, 0x%08lx, %s, %p, 0x%08lx, %p, %p)\n", ld,
           debugstr_a(base), scope, debugstr_a(filter), attrs,
           attrsonly, timeout, res );

    baseW = strAtoW( base );
    if (!baseW) return LDAP_NO_MEMORY;

    filterW = strAtoW( filter );
    if (!filterW) return LDAP_NO_MEMORY;

    attrsW = strarrayAtoW( attrs );
    if (!attrsW) return LDAP_NO_MEMORY;

    ret = ldap_search_stW( ld, baseW, scope, filterW, attrsW, attrsonly,
                           timeout, res );

    strfreeW( baseW );
    strfreeW( filterW );
    strarrayfreeW( attrsW );

#endif
    return ret;
}

ULONG ldap_search_stW( WLDAP32_LDAP *ld, const PWCHAR base, ULONG scope,
    const PWCHAR filter, PWCHAR attrs[], ULONG attrsonly,
    struct l_timeval *timeout, WLDAP32_LDAPMessage **res )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    char *baseU, *filterU, **attrsU;

    TRACE( "(%p, %s, 0x%08lx, %s, %p, 0x%08lx, %p, %p)\n", ld,
           debugstr_w(base), scope, debugstr_w(filter), attrs,
           attrsonly, timeout, res );

    baseU = strWtoU( base );
    if (!baseU) return LDAP_NO_MEMORY;

    filterU = strWtoU( filter );
    if (!filterU) return LDAP_NO_MEMORY;

    attrsU = strarrayWtoU( attrs );
    if (!attrsU) return LDAP_NO_MEMORY;

    ret = ldap_search_st( ld, baseU, scope, filterU, attrsU, attrsonly,
                          (struct timeval *)timeout, res );

    strfreeU( baseU );
    strfreeU( filterU );
    strarrayfreeU( attrsU );

#endif
    return ret;
}
