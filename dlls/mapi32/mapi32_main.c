/*
 *             MAPI basics
 *
 * Copyright 2001 Codeweavers Inc.
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "mapi.h"
#include "mapicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mapi);

HRESULT WINAPI MAPIInitialize ( LPVOID lpMapiInit )
{
    ERR("Stub\n");
    return MAPI_E_NOT_INITIALIZED;
}

HRESULT WINAPI MAPIAllocateBuffer ( ULONG cvSize, LPVOID *lppBuffer )
{
    ERR("Stub\n");
    *lppBuffer = NULL;
    return MAPI_E_NOT_INITIALIZED;
}

ULONG WINAPI MAPILogon(ULONG ulUIParam, LPSTR lpszProfileName, LPSTR
lpszPassword, FLAGS flFlags, ULONG ulReserver, LPLHANDLE lplhSession)
{
    ERR("Stub\n");
    return MAPI_E_FAILURE;
}

HRESULT WINAPI MAPILogonEx( ULONG ulUIParam, LPSTR lpszProfileName, LPSTR
lpszPassword, FLAGS flFlags, VOID* lppSession)
{
    ERR("Stub\n");
    return MAPI_E_LOGON_FAILURE;
}

VOID WINAPI MAPIUninitialize(void)
{
    ERR("Stub\n");
}

VOID WINAPI DeinitMapiUtil(void)
{
    ERR("Stub\n");
}
