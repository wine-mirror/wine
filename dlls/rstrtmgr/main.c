/*
 * Copyright 2010 Louis Lenders
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

#include "config.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

#include "restartmanager.h"

WINE_DEFAULT_DEBUG_CHANNEL(rstrtmgr);

/*****************************************************
 *      DllMain
 */
BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    TRACE("(%p, %d, %p)\n", hinst, reason, reserved);

    switch(reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */

    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinst );
        break;
    }
    return TRUE;
}

/***********************************************************************
 * RmGetList (rstrtmgr.@)
 *
 * Retrieve the list of all applications and services using resources registered with the Restart Manager session
 */
DWORD WINAPI RmGetList(DWORD dwSessionHandle, UINT *pnProcInfoNeeded, UINT *pnProcInfo,
                                      RM_PROCESS_INFO *rgAffectedApps[], LPDWORD lpdwRebootReasons)
{
    FIXME("%d, %p, %p, %p, %p stub!\n", dwSessionHandle, pnProcInfoNeeded, pnProcInfo, rgAffectedApps, lpdwRebootReasons);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 * RmRegisterResources (rstrtmgr.@)
 *
 * Register resources for a Restart Manager session
 */
DWORD WINAPI RmRegisterResources(DWORD dwSessionHandle, UINT nFiles, LPCWSTR rgsFilenames[],
                                                     UINT nApplications, RM_UNIQUE_PROCESS *rgApplications,
                                                     UINT nServices, LPCWSTR rgsServiceNames[])
{
    FIXME("%d, %d, %p, %d, %p, %d, %p stub!\n", dwSessionHandle, nFiles, rgsFilenames,
              nApplications, rgApplications, nServices, rgsServiceNames);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 * RmStartSession (rstrtmgr.@)
 *
 * Start a new Restart Manager session
 */
DWORD WINAPI RmStartSession(DWORD *sessionhandle, DWORD flags, WCHAR sessionkey[])
{
    FIXME("%p, %d, %p stub!\n", sessionhandle, flags, sessionkey);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 * RmRestart (rstrtmgr.@)
 */
DWORD WINAPI RmRestart(DWORD handle, DWORD flags, void *fnstatus)
{
    FIXME("%u, 0x%08x, %p stub!\n", handle, flags, fnstatus);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 * RmEndSession (rstrtmgr.@)
 */
DWORD WINAPI RmEndSession(DWORD handle)
{
    FIXME("%u stub!\n", handle);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 * RmShutdown (rstrtmgr.@)
 */
DWORD WINAPI RmShutdown(DWORD handle, ULONG flags, RM_WRITE_STATUS_CALLBACK status)
{
    FIXME("%u, 0x%08x, %p stub!\n", handle, flags, status);
    return ERROR_CALL_NOT_IMPLEMENTED;
}
