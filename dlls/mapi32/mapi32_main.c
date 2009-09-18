/*
 *             MAPI basics
 *
 * Copyright 2001, 2009 CodeWeavers Inc.
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
#include "winbase.h"
#include "winerror.h"
#include "objbase.h"
#include "initguid.h"
#include "mapix.h"
#include "mapiform.h"
#include "mapi.h"
#include "wine/debug.h"
#include "util.h"

WINE_DEFAULT_DEBUG_CHANNEL(mapi);

LONG MAPI_ObjectCount = 0;

/***********************************************************************
 *              DllMain (MAPI32.init)
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("(%p,%d,%p)\n", hinstDLL, fdwReason, fImpLoad);

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        load_mapi_providers();
        break;
    case DLL_PROCESS_DETACH:
	TRACE("DLL_PROCESS_DETACH: %d objects remaining\n", MAPI_ObjectCount);
        unload_mapi_providers();
	break;
    }
    return TRUE;
}

/***********************************************************************
 *		DllGetClassObject (MAPI32.27)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    *ppv = NULL;
    FIXME("\n\tCLSID:\t%s,\n\tIID:\t%s\n", debugstr_guid(rclsid), debugstr_guid(iid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 * DllCanUnloadNow (MAPI32.28)
 *
 * Determine if this dll can be unloaded from the callers address space.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  S_OK, if the dll can be unloaded,
 *  S_FALSE, otherwise.
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return MAPI_ObjectCount == 0 ? S_OK : S_FALSE;
}

/***********************************************************************
 * MAPIInitialize
 *
 * Initialises the MAPI library. In our case, we pass through to the
 * loaded Extended MAPI provider.
 */
HRESULT WINAPI MAPIInitialize(LPVOID init)
{
    TRACE("(%p)\n", init);

    if (mapiFunctions.MAPIInitialize)
        return mapiFunctions.MAPIInitialize(init);

    return MAPI_E_NOT_INITIALIZED;
}

/***********************************************************************
 * MAPILogon
 *
 * Logs on to a MAPI provider. If available, we pass this through to a
 * Simple MAPI provider. Otherwise, we maintain basic functionality
 * ourselves.
 */
ULONG WINAPI MAPILogon(ULONG_PTR uiparam, LPSTR profile, LPSTR password,
    FLAGS flags, ULONG reserved, LPLHANDLE session)
{
    TRACE("(0x%08lx %s %p 0x%08x 0x%08x %p)\n", uiparam,
          debugstr_a(profile), password, flags, reserved, session);

    if (mapiFunctions.MAPILogon)
        return mapiFunctions.MAPILogon(uiparam, profile, password, flags, reserved, session);

    if (session) *session = 1;
    return SUCCESS_SUCCESS;
}

/***********************************************************************
 * MAPILogoff
 *
 * Logs off from a MAPI provider. If available, we pass this through to a
 * Simple MAPI provider. Otherwise, we maintain basic functionality
 * ourselves.
 */
ULONG WINAPI MAPILogoff(LHANDLE session, ULONG_PTR uiparam, FLAGS flags,
    ULONG reserved )
{
    TRACE("(0x%08lx 0x%08lx 0x%08x 0x%08x)\n", session,
          uiparam, flags, reserved);

    if (mapiFunctions.MAPILogoff)
        return mapiFunctions.MAPILogoff(session, uiparam, flags, reserved);

    return SUCCESS_SUCCESS;
}

/***********************************************************************
 * MAPILogonEx
 *
 * Logs on to a MAPI provider. If available, we pass this through to an
 * Extended MAPI provider. Otherwise, we return an error.
 */
HRESULT WINAPI MAPILogonEx(ULONG_PTR uiparam, LPWSTR profile,
    LPWSTR password, ULONG flags, LPMAPISESSION *session)
{
    TRACE("(0x%08lx %s %p 0x%08x %p)\n", uiparam,
          debugstr_w(profile), password, flags, session);

    if (mapiFunctions.MAPILogonEx)
        return mapiFunctions.MAPILogonEx(uiparam, profile, password, flags, session);

    return E_FAIL;
}

HRESULT WINAPI MAPIOpenLocalFormContainer(LPVOID *ppfcnt)
{
    FIXME("(%p) Stub\n", ppfcnt);
    return E_FAIL;
}

/***********************************************************************
 * MAPIUninitialize
 *
 * Uninitialises the MAPI library. In our case, we pass through to the
 * loaded Extended MAPI provider.
 *
 */
VOID WINAPI MAPIUninitialize(void)
{
    TRACE("()\n");

    /* Try to uninitialise the Extended MAPI library */
    if (mapiFunctions.MAPIUninitialize)
        mapiFunctions.MAPIUninitialize();
}

HRESULT WINAPI MAPIAdminProfiles(ULONG ulFlags,  LPPROFADMIN *lppProfAdmin)
{
    FIXME("(%u, %p): stub\n", ulFlags, lppProfAdmin);
    *lppProfAdmin = NULL;
    return E_FAIL;
}
