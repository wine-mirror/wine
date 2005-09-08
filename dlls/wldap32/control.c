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

ULONG ldap_control_freeA( LDAPControlA *control )
{
    ULONG ret = LDAP_SUCCESS;
#ifdef HAVE_LDAP

    TRACE( "(%p)\n", control );
    controlfreeA( control );

#endif
    return ret;
}

ULONG ldap_control_freeW( LDAPControlW *control )
{
    ULONG ret = LDAP_SUCCESS;
#ifdef HAVE_LDAP

    TRACE( "(%p)\n", control );
    controlfreeW( control );

#endif
    return ret;
}

ULONG ldap_controls_freeA( LDAPControlA **controls )
{
    ULONG ret = LDAP_SUCCESS;
#ifdef HAVE_LDAP

    TRACE( "(%p)\n", controls );
    controlarrayfreeA( controls );

#endif
    return ret;
}

ULONG ldap_controls_freeW( LDAPControlW **controls )
{
    ULONG ret = LDAP_SUCCESS;
#ifdef HAVE_LDAP

    TRACE( "(%p)\n", controls );
    controlarrayfreeW( controls );

#endif
    return ret;
}

ULONG ldap_create_sort_controlA( WLDAP32_LDAP *ld, PLDAPSortKeyA *sortkey,
    UCHAR critical, PLDAPControlA *control )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    LDAPSortKeyW **sortkeyW = NULL;
    LDAPControlW *controlW = NULL;

    TRACE( "(%p, %p, 0x%02x, %p)\n", ld, sortkey, critical, control );

    if (!ld || !sortkey || !control)
        return WLDAP32_LDAP_PARAM_ERROR;

    sortkeyW = sortkeyarrayAtoW( sortkey );
    if (!sortkeyW) return WLDAP32_LDAP_NO_MEMORY;

    ret = ldap_create_sort_controlW( ld, sortkeyW, critical, &controlW );

    *control = controlWtoA( controlW );
    if (!*control) ret = WLDAP32_LDAP_NO_MEMORY;

    ldap_control_freeW( controlW );
    sortkeyarrayfreeW( sortkeyW );

#endif
    return ret;
}

ULONG ldap_create_sort_controlW( WLDAP32_LDAP *ld, PLDAPSortKeyW *sortkey,
    UCHAR critical, PLDAPControlW *control )
{
    ULONG ret = LDAP_NOT_SUPPORTED;
#ifdef HAVE_LDAP
    LDAPSortKey **sortkeyU = NULL;
    LDAPControl *controlU = NULL;

    TRACE( "(%p, %p, 0x%02x, %p)\n", ld, sortkey, critical, control );

    if (!ld || !sortkey || !control)
        return WLDAP32_LDAP_PARAM_ERROR;

    sortkeyU = sortkeyarrayWtoU( sortkey );
    if (!sortkeyU) return WLDAP32_LDAP_NO_MEMORY;

    ret = ldap_create_sort_control( ld, sortkeyU, critical, &controlU );

    *control = controlUtoW( controlU );
    if (!*control) ret = WLDAP32_LDAP_NO_MEMORY;

    ldap_control_free( controlU );
    sortkeyarrayfreeU( sortkeyU );

#endif
    return ret;
}

ULONG ldap_free_controlsA( LDAPControlA **controls )
{
    return ldap_controls_freeA( controls );
}

ULONG ldap_free_controlsW( LDAPControlW **controls )
{
    return ldap_controls_freeW( controls );
}
