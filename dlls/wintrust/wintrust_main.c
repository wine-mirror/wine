/*
 * Copyright 2001 Rein Klazes
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "guiddef.h"
#include "wintrust.h"
#include "mscat.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wintrust);

/***********************************************************************
 *		CryptCATAdminAcquireContext (WINTRUST.@)
 */
BOOL WINAPI CryptCATAdminAcquireContext(HCATADMIN* catAdmin,
					const GUID *sysSystem, DWORD dwFlags )
{
    FIXME("%p %s %lx\n", catAdmin, debugstr_guid(sysSystem), dwFlags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *             CryptCATAdminCalcHashFromFileHandle (WINTRUST.@)
 */
BOOL WINAPI CryptCATAdminCalcHashFromFileHandle(HANDLE hFile, DWORD* pcbHash,
                                                BYTE* pbHash, DWORD dwFlags )
{
    FIXME("%p %p %p %lx\n", hFile, pcbHash, pbHash, dwFlags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *		CryptCATAdminReleaseContext (WINTRUST.@)
 */
BOOL WINAPI CryptCATAdminReleaseContext(HCATADMIN hCatAdmin, DWORD dwFlags )
{
    FIXME("%p %lx\n", hCatAdmin, dwFlags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *		WinVerifyTrust (WINTRUST.@)
 */
LONG WINAPI WinVerifyTrust( HWND hwnd, GUID *ActionID,  WINTRUST_DATA* ActionData )
{
    FIXME("%p %p %p\n", hwnd, ActionID,  ActionData);
    return ERROR_SUCCESS;
}

/***********************************************************************
 *		WintrustAddActionID (WINTRUST.@)
 */
BOOL WINAPI WintrustAddActionID( GUID* pgActionID, DWORD fdwFlags,
                                 CRYPT_REGISTER_ACTIONID* psProvInfo)
{
    FIXME("%p %lx %p\n", pgActionID, fdwFlags, psProvInfo);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *		WintrustAddActionID (WINTRUST.@)
 */
void WINAPI WintrustGetRegPolicyFlags( DWORD* pdwPolicyFlags )
{
    FIXME("%p\n", pdwPolicyFlags);
    *pdwPolicyFlags = 0;
}
