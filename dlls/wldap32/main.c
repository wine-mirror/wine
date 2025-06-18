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
#include "winternl.h"
#include "winbase.h"

#include "wine/debug.h"
#include "winldap_private.h"

HINSTANCE hwldap32;

WINE_DEFAULT_DEBUG_CHANNEL(wldap32);

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    TRACE( "(%p, %#lx, %p)\n", hinst, reason, reserved );

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        hwldap32 = hinst;
        DisableThreadLibraryCalls( hinst );
        if (TRACE_ON( wldap32 ))
        {
            int ld_debug = -1;
            ldap_set_option( NULL, LDAP_OPT_DEBUG_LEVEL, &ld_debug );
        }
        break;
    }
    return TRUE;
}
