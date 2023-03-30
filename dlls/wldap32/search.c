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
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winsock2.h"

#include "wine/debug.h"
#include "winldap_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

/***********************************************************************
 *      ldap_searchA     (WLDAP32.@)
 */
ULONG CDECL ldap_searchA( LDAP *ld, char *base, ULONG scope, char *filter, char **attrs, ULONG attrsonly )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *baseW = NULL, *filterW = NULL, **attrsW = NULL;

    TRACE( "(%p, %s, %#lx, %s, %p, %#lx)\n", ld, debugstr_a(base), scope, debugstr_a(filter), attrs, attrsonly );

    if (!ld) return ~0u;

    if (base && !(baseW = strAtoW( base ))) goto exit;
    if (filter && !(filterW = strAtoW( filter ))) goto exit;
    if (attrs && !(attrsW = strarrayAtoW( attrs ))) goto exit;

    ret = ldap_searchW( ld, baseW, scope, filterW, attrsW, attrsonly );

exit:
    free( baseW );
    free( filterW );
    strarrayfreeW( attrsW );
    return ret;
}

/***********************************************************************
 *      ldap_searchW     (WLDAP32.@)
 */
ULONG CDECL ldap_searchW( LDAP *ld, WCHAR *base, ULONG scope, WCHAR *filter, WCHAR **attrs, ULONG attrsonly )
{
    ULONG ret, msg;
    TRACE( "(%p, %s, %#lx, %s, %p, %#lx)\n", ld, debugstr_w(base), scope, debugstr_w(filter), attrs, attrsonly );

    ret = ldap_search_extW( ld, base, scope, filter, attrs, attrsonly, NULL, NULL, 0, 0, &msg );
    if (ret == WLDAP32_LDAP_SUCCESS) return msg;
    return ~0u;
}

/***********************************************************************
 *      ldap_search_extA     (WLDAP32.@)
 */
ULONG CDECL ldap_search_extA( LDAP *ld, char *base, ULONG scope, char *filter, char **attrs, ULONG attrsonly,
                              LDAPControlA **serverctrls, LDAPControlA **clientctrls, ULONG timelimit,
                              ULONG sizelimit, ULONG *message )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *baseW = NULL, *filterW = NULL, **attrsW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    TRACE( "(%p, %s, %#lx, %s, %p, %#lx, %p, %p, %#lx, %#lx, %p)\n", ld, debugstr_a(base), scope,
           debugstr_a(filter), attrs, attrsonly, serverctrls, clientctrls, timelimit, sizelimit, message );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;

    if (base && !(baseW = strAtoW( base ))) goto exit;
    if (filter && !(filterW = strAtoW( filter ))) goto exit;
    if (attrs && !(attrsW = strarrayAtoW( attrs ))) goto exit;
    if (serverctrls && !(serverctrlsW = controlarrayAtoW( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsW = controlarrayAtoW( clientctrls ))) goto exit;

    ret = ldap_search_extW( ld, baseW, scope, filterW, attrsW, attrsonly, serverctrlsW, clientctrlsW, timelimit,
                            sizelimit, message );

exit:
    free( baseW );
    free( filterW );
    strarrayfreeW( attrsW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );
    return ret;
}

/***********************************************************************
 *      ldap_search_extW     (WLDAP32.@)
 */
ULONG CDECL ldap_search_extW( LDAP *ld, WCHAR *base, ULONG scope, WCHAR *filter, WCHAR **attrs, ULONG attrsonly,
                              LDAPControlW **serverctrls, LDAPControlW **clientctrls, ULONG timelimit,
                              ULONG sizelimit, ULONG *message )
{
    ULONG ret;
    char *baseU = NULL, *filterU = NULL, **attrsU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;
    struct l_timeval timevalW = { timelimit, 0 };
    struct timeval timevalU = { timelimit, 0 };

    TRACE( "(%p, %s, %#lx, %s, %p, %#lx, %p, %p, %#lx, %#lx, %p)\n", ld, debugstr_w(base), scope,
           debugstr_w(filter), attrs, attrsonly, serverctrls, clientctrls, timelimit, sizelimit, message );

    if (!ld) return WLDAP32_LDAP_PARAM_ERROR;
    if ((ret = WLDAP32_ldap_connect( ld, &timevalW ))) return ret;

    ret = WLDAP32_LDAP_NO_MEMORY;
    if (base && !(baseU = strWtoU( base ))) goto exit;
    if (filter && !(filterU = strWtoU( filter ))) goto exit;
    if (attrs && !(attrsU = strarrayWtoU( attrs ))) goto exit;
    if (serverctrls && !(serverctrlsU = controlarrayWtoU( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsU = controlarrayWtoU( clientctrls ))) goto exit;

    ret = map_error( ldap_search_ext( CTX(ld), baseU, scope, filterU, attrsU, attrsonly, serverctrlsU,
                                      clientctrlsU, timelimit ? &timevalU : NULL, sizelimit, (int *)message ) );
exit:
    free( baseU );
    free( filterU );
    strarrayfreeU( attrsU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );
    return ret;
}

/***********************************************************************
 *      ldap_search_ext_sA     (WLDAP32.@)
 */
ULONG CDECL ldap_search_ext_sA( LDAP *ld, char *base, ULONG scope, char *filter, char **attrs, ULONG attrsonly,
                                LDAPControlA **serverctrls, LDAPControlA **clientctrls, struct l_timeval *timeout,
                                ULONG sizelimit, WLDAP32_LDAPMessage **res )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *baseW = NULL, *filterW = NULL, **attrsW = NULL;
    LDAPControlW **serverctrlsW = NULL, **clientctrlsW = NULL;

    TRACE( "(%p, %s, %#lx, %s, %p, %#lx, %p, %p, %p, %#lx, %p)\n", ld, debugstr_a(base), scope,
           debugstr_a(filter), attrs, attrsonly, serverctrls, clientctrls, timeout, sizelimit, res );

    if (!ld || !res) return WLDAP32_LDAP_PARAM_ERROR;

    if (base && !(baseW = strAtoW( base ))) goto exit;
    if (filter && !(filterW = strAtoW( filter ))) goto exit;
    if (attrs && !(attrsW = strarrayAtoW( attrs ))) goto exit;
    if (serverctrls && !(serverctrlsW = controlarrayAtoW( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsW = controlarrayAtoW( clientctrls ))) goto exit;

    ret = ldap_search_ext_sW( ld, baseW, scope, filterW, attrsW, attrsonly, serverctrlsW, clientctrlsW, timeout,
                              sizelimit, res );

exit:
    free( baseW );
    free( filterW );
    strarrayfreeW( attrsW );
    controlarrayfreeW( serverctrlsW );
    controlarrayfreeW( clientctrlsW );
    return ret;
}

/***********************************************************************
 *      ldap_search_ext_sW     (WLDAP32.@)
 */
ULONG CDECL ldap_search_ext_sW( LDAP *ld, WCHAR *base, ULONG scope, WCHAR *filter, WCHAR **attrs, ULONG attrsonly,
                                LDAPControlW **serverctrls, LDAPControlW **clientctrls, struct l_timeval *timeout,
                                ULONG sizelimit, LDAPMessage **res )
{
    ULONG ret;
    char *baseU = NULL, *filterU = NULL, **attrsU = NULL;
    LDAPControl **serverctrlsU = NULL, **clientctrlsU = NULL;
    struct timeval timevalU;
    LDAPMessage *msgU = NULL;

    TRACE( "(%p, %s, %#lx, %s, %p, %#lx, %p, %p, %p, %#lx, %p)\n", ld, debugstr_w(base), scope,
           debugstr_w(filter), attrs, attrsonly, serverctrls, clientctrls, timeout, sizelimit, res );

    if (!ld || !res) return WLDAP32_LDAP_PARAM_ERROR;
    if ((ret = WLDAP32_ldap_connect( ld, timeout ))) return ret;

    ret = WLDAP32_LDAP_NO_MEMORY;
    if (base && !(baseU = strWtoU( base ))) goto exit;
    if (filter && !(filterU = strWtoU( filter ))) goto exit;
    if (attrs && !(attrsU = strarrayWtoU( attrs ))) goto exit;
    if (serverctrls && !(serverctrlsU = controlarrayWtoU( serverctrls ))) goto exit;
    if (clientctrls && !(clientctrlsU = controlarrayWtoU( clientctrls ))) goto exit;

    if (timeout)
    {
        timevalU.tv_sec = timeout->tv_sec;
        timevalU.tv_usec = timeout->tv_usec;
    }

    ret = map_error( ldap_search_ext_s( CTX(ld), baseU, scope, filterU, attrsU, attrsonly, serverctrlsU,
                                        clientctrlsU, timeout ? &timevalU : NULL, sizelimit, &msgU ) );

    if (msgU)
    {
        LDAPMessage *msg = calloc( 1, sizeof(*msg) );
        if (msg)
        {
            MSG(msg) = msgU;
            *res = msg;
        }
        else
        {
            ldap_msgfree( msgU );
            ret = WLDAP32_LDAP_NO_MEMORY;
        }
    }

exit:
    free( baseU );
    free( filterU );
    strarrayfreeU( attrsU );
    controlarrayfreeU( serverctrlsU );
    controlarrayfreeU( clientctrlsU );
    return ret;
}

/***********************************************************************
 *      ldap_search_sA     (WLDAP32.@)
 */
ULONG CDECL ldap_search_sA( LDAP *ld, char *base, ULONG scope, char *filter, char **attrs, ULONG attrsonly,
                            WLDAP32_LDAPMessage **res )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *baseW = NULL, *filterW = NULL, **attrsW = NULL;

    TRACE( "(%p, %s, %#lx, %s, %p, %#lx, %p)\n", ld, debugstr_a(base), scope, debugstr_a(filter), attrs,
           attrsonly, res );

    if (!ld || !res) return WLDAP32_LDAP_PARAM_ERROR;

    if (base && !(baseW = strAtoW( base ))) goto exit;
    if (filter && !(filterW = strAtoW( filter ))) goto exit;
    if (attrs && !(attrsW = strarrayAtoW( attrs ))) goto exit;

    ret = ldap_search_sW( ld, baseW, scope, filterW, attrsW, attrsonly, res );

exit:
    free( baseW );
    free( filterW );
    strarrayfreeW( attrsW );
    return ret;
}

/***********************************************************************
 *      ldap_search_sW     (WLDAP32.@)
 */
ULONG CDECL ldap_search_sW( LDAP *ld, WCHAR *base, ULONG scope, WCHAR *filter, WCHAR **attrs, ULONG attrsonly,
                            WLDAP32_LDAPMessage **res )
{
    TRACE( "(%p, %s, %#lx, %s, %p, %#lx, %p)\n", ld, debugstr_w(base), scope, debugstr_w(filter), attrs,
           attrsonly, res );
    return ldap_search_ext_sW( ld, base, scope, filter, attrs, attrsonly, NULL, NULL, NULL, 0, res );
}

/***********************************************************************
 *      ldap_search_stA     (WLDAP32.@)
 */
ULONG CDECL ldap_search_stA( LDAP *ld, const PCHAR base, ULONG scope, const PCHAR filter, char **attrs,
                             ULONG attrsonly, struct l_timeval *timeout, WLDAP32_LDAPMessage **res )
{
    ULONG ret = WLDAP32_LDAP_NO_MEMORY;
    WCHAR *baseW = NULL, *filterW = NULL, **attrsW = NULL;

    TRACE( "(%p, %s, %#lx, %s, %p, %#lx, %p, %p)\n", ld, debugstr_a(base), scope, debugstr_a(filter), attrs,
           attrsonly, timeout, res );

    if (!ld || !res) return WLDAP32_LDAP_PARAM_ERROR;

    if (base && !(baseW = strAtoW( base ))) goto exit;
    if (filter && !(filterW = strAtoW( filter ))) goto exit;
    if (attrs && !(attrsW = strarrayAtoW( attrs ))) goto exit;

    ret = ldap_search_stW( ld, baseW, scope, filterW, attrsW, attrsonly, timeout, res );

exit:
    free( baseW );
    free( filterW );
    strarrayfreeW( attrsW );
    return ret;
}

/***********************************************************************
 *      ldap_search_stW     (WLDAP32.@)
 */
ULONG CDECL ldap_search_stW( LDAP *ld, const PWCHAR base, ULONG scope, const PWCHAR filter, WCHAR **attrs,
                             ULONG attrsonly, struct l_timeval *timeout, WLDAP32_LDAPMessage **res )
{
    TRACE( "(%p, %s, %#lx, %s, %p, %#lx, %p, %p)\n", ld, debugstr_w(base), scope, debugstr_w(filter), attrs,
           attrsonly, timeout, res );
    return ldap_search_ext_sW( ld, base, scope, filter, attrs, attrsonly, NULL, NULL, timeout, 0, res );
}
