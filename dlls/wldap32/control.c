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

ULONG ldap_free_controlsA( LDAPControlA **controls )
{
    return ldap_controls_freeA( controls );
}

ULONG ldap_free_controlsW( LDAPControlW **controls )
{
    return ldap_controls_freeW( controls );
}
