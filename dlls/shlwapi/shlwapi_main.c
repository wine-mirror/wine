/*
 * SHLWAPI initialisation
 *
 *  Copyright 1998 Marcus Meissner
 *  Copyright 1998 Juergen Schmied (jsch)
 */

#include "winbase.h"
#include "winerror.h"
#include "debugtools.h"

#include "initguid.h"
#include "wine/obj_storage.h"

DEFAULT_DEBUG_CHANNEL(shell);

HINSTANCE shlwapi_hInstance = 0; 

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
	    break;
	}
	return TRUE;
}

/***********************************************************************
 * DllGetVersion [SHLWAPI]
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
	  WARN("wrong DLLVERSIONINFO size from app");
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
