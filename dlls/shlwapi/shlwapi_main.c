/*
 * SHLWAPI initialisation
 *
 *  Copyright 1998 Marcus Meissner
 *  Copyright 1998 Juergen Schmied (jsch)
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

#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "wine/debug.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

HINSTANCE shlwapi_hInstance = 0;
HMODULE SHLWAPI_hshell32 = 0;
HMODULE SHLWAPI_hwinmm = 0;
HMODULE SHLWAPI_hcomdlg32 = 0;
HMODULE SHLWAPI_hmpr = 0;
HMODULE SHLWAPI_hmlang = 0;
HMODULE SHLWAPI_hversion = 0;

DWORD SHLWAPI_ThreadRef_index = -1;

/*************************************************************************
 * SHLWAPI LibMain
 *
 * NOTES
 *  calling oleinitialize here breaks sone apps.
 */
BOOL WINAPI SHLWAPI_LibMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
	TRACE("0x%x 0x%lx %p\n", hinstDLL, fdwReason, fImpLoad);
	switch (fdwReason)
	{
	  case DLL_PROCESS_ATTACH:
	    shlwapi_hInstance = hinstDLL;
	    SHLWAPI_ThreadRef_index = TlsAlloc();
	    break;
	  case DLL_PROCESS_DETACH:
	    if (SHLWAPI_hshell32)  FreeLibrary(SHLWAPI_hshell32);
	    if (SHLWAPI_hwinmm)    FreeLibrary(SHLWAPI_hwinmm);
	    if (SHLWAPI_hcomdlg32) FreeLibrary(SHLWAPI_hcomdlg32);
	    if (SHLWAPI_hmpr)      FreeLibrary(SHLWAPI_hmpr);
	    if (SHLWAPI_hmlang)    FreeLibrary(SHLWAPI_hmlang);
	    if (SHLWAPI_hversion)  FreeLibrary(SHLWAPI_hversion);
	    if (SHLWAPI_ThreadRef_index >= 0) TlsFree(SHLWAPI_ThreadRef_index);
	    break;
	}
	return TRUE;
}

/***********************************************************************
 * DllGetVersion [SHLWAPI.@]
 *
 * Retrieves version information of the 'SHLWAPI.DLL'
 *
 * PARAMS
 *     pdvi [O] pointer to version information structure.
 *
 * RETURNS
 *     Success: S_OK
 *     Failure: E_INVALIDARG
 *
 * NOTES
 *     Returns version of a SHLWAPI.dll from IE5.01.
 */

HRESULT WINAPI SHLWAPI_DllGetVersion (DLLVERSIONINFO *pdvi)
{
	if (pdvi->cbSize != sizeof(DLLVERSIONINFO))
	{
	  WARN("wrong DLLVERSIONINFO size from app\n");
	  return E_INVALIDARG;
	}

	pdvi->dwMajorVersion = 5;
	pdvi->dwMinorVersion = 0;
	pdvi->dwBuildNumber = 2314;
	pdvi->dwPlatformID = 1000;

	TRACE("%lu.%lu.%lu.%lu\n",
	   pdvi->dwMajorVersion, pdvi->dwMinorVersion,
	   pdvi->dwBuildNumber, pdvi->dwPlatformID);

	return S_OK;
}
