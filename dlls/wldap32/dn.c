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
#endif

#include "winldap_private.h"
#include "wldap32.h"

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

PCHAR ldap_dn2ufnA( PCHAR dn )
{
    PCHAR ret = NULL;
#ifdef HAVE_LDAP
    WCHAR *dnW, *retW;

    TRACE( "(%s)\n", debugstr_a(dn) );

    dnW = strAtoW( dn );
    if (!dnW) return NULL;

    retW = ldap_dn2ufnW( dnW );
    ret = strWtoA( retW );

    strfreeW( dnW );
    ldap_memfreeW( retW );

#endif
    return ret;
}

PWCHAR ldap_dn2ufnW( PWCHAR dn )
{
    PWCHAR ret = NULL;
#ifdef HAVE_LDAP
    char *dnU, *retU;

    TRACE( "(%s)\n", debugstr_w(dn) );

    dnU = strWtoU( dn );
    if (!dnU) return NULL;

    retU = ldap_dn2ufn( dnU );
    ret = strUtoW( retU );

    strfreeU( dnU );
    ldap_memfree( retU );

#endif
    return ret;
}

PCHAR *ldap_explode_dnA( PCHAR dn, ULONG notypes )
{
    PCHAR *ret = NULL;
#ifdef HAVE_LDAP
    WCHAR *dnW, **retW;

    TRACE( "(%s, 0x%08lx)\n", debugstr_a(dn), notypes );

    dnW = strAtoW( dn );
    if (!dnW) return NULL;

    retW = ldap_explode_dnW( dnW, notypes );
    ret = strarrayWtoA( retW );

    strfreeW( dnW );
    ldap_value_freeW( retW );

#endif
    return ret;
}

PWCHAR *ldap_explode_dnW( PWCHAR dn, ULONG notypes )
{
    PWCHAR *ret = NULL;
#ifdef HAVE_LDAP
    char *dnU, **retU;

    TRACE( "(%s, 0x%08lx)\n", debugstr_w(dn), notypes );

    dnU = strWtoU( dn );
    if (!dnU) return NULL;

    retU = ldap_explode_dn( dnU, notypes );
    ret = strarrayUtoW( retU );

    strfreeU( dnU );
    ldap_value_free( retU );

#endif
    return ret;
}

PCHAR ldap_get_dnA( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *entry )
{
    PCHAR ret = NULL;
#ifdef HAVE_LDAP
    PWCHAR retW;

    TRACE( "(%p, %p)\n", ld, entry );

    if (!ld || !entry) return NULL;

    retW = ldap_get_dnW( ld, entry );

    ret = strWtoA( retW );
    ldap_memfreeW( retW );

#endif
    return ret;
}

PWCHAR ldap_get_dnW( WLDAP32_LDAP *ld, WLDAP32_LDAPMessage *entry )
{
    PWCHAR ret = NULL;
#ifdef HAVE_LDAP
    char *retU;

    TRACE( "(%p, %p)\n", ld, entry );

    if (!ld || !entry) return NULL;

    retU = ldap_get_dn( ld, entry );

    ret = strUtoW( retU );
    ldap_memfree( retU );

#endif
    return ret;
}

ULONG ldap_ufn2dnA( PCHAR ufn, PCHAR *dn )
{
    ULONG ret = LDAP_SUCCESS;
#ifdef HAVE_LDAP
    PWCHAR ufnW = NULL, dnW = NULL;

    TRACE( "(%s, %p)\n", debugstr_a(ufn), dn );

    if (!dn) return WLDAP32_LDAP_PARAM_ERROR;

    *dn = NULL;

    if (ufn) {
        ufnW = strAtoW( ufn );
        if (!ufnW) return WLDAP32_LDAP_NO_MEMORY;
    }

    ret = ldap_ufn2dnW( ufnW, &dnW );

    if (dnW) {
        *dn = strWtoA( dnW );
        if (!*dn) ret = WLDAP32_LDAP_NO_MEMORY;
    }

    strfreeW( ufnW );
    ldap_memfreeW( dnW );

#endif
    return ret;
}

ULONG ldap_ufn2dnW( PWCHAR ufn, PWCHAR *dn )
{
    ULONG ret = LDAP_SUCCESS;
#ifdef HAVE_LDAP
    char *ufnU = NULL;

    TRACE( "(%s, %p)\n", debugstr_w(ufn), dn );

    if (!dn) return WLDAP32_LDAP_PARAM_ERROR;

    *dn = NULL;

    if (ufn) {
        ufnU = strWtoU( ufn );
        if (!ufnU) return WLDAP32_LDAP_NO_MEMORY;

        /* FIXME: do more than just a copy */
        *dn = strUtoW( ufnU );
        if (!*dn) ret = WLDAP32_LDAP_NO_MEMORY;
    }

    strfreeU( ufnU );

#endif
    return ret;
}
