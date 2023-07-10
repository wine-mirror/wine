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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#define NO_SHLWAPI_REG
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

HINSTANCE shlwapi_hInstance = 0;

/*************************************************************************
 * SHLWAPI {SHLWAPI}
 *
 * The Shell Light-Weight API dll provides a large number of utility functions
 * which are commonly required by Win32 programs. Originally distributed with
 * Internet Explorer as a free download, it became a core part of Windows when
 * Internet Explorer was 'integrated' into the O/S with the release of Win98.
 *
 * All functions exported by ordinal are undocumented by MS. The vast majority
 * of these are wrappers for Unicode functions that may not exist on early 16
 * bit platforms. The remainder perform various small tasks and presumably were
 * added to facilitate code reuse amongst the MS developers.
 */

/*************************************************************************
 * SHLWAPI DllMain
 *
 * NOTES
 *  calling oleinitialize here breaks some apps.
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
	TRACE("%p 0x%lx %p\n", hinstDLL, fdwReason, fImpLoad);
	switch (fdwReason)
	{
	  case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
	    shlwapi_hInstance = hinstDLL;
	    break;
	}
	return TRUE;
}

/***********************************************************************
 * DllGetVersion [SHLWAPI.@]
 *
 * Retrieve "shlwapi.dll" version information.
 *
 * PARAMS
 *     pdvi [O] pointer to version information structure.
 *
 * RETURNS
 *     Success: S_OK. pdvi is updated with the version information
 *     Failure: E_INVALIDARG, if pdvi->cbSize is not set correctly.
 *
 * NOTES
 *     You may pass either a DLLVERSIONINFO of DLLVERSIONINFO2 structure
 *     as pdvi, provided that the size is set correctly.
 *     Returns version as shlwapi.dll from IE5.01.
 */
HRESULT WINAPI DllGetVersion (DLLVERSIONINFO *pdvi)
{
  DLLVERSIONINFO2 *pdvi2 = (DLLVERSIONINFO2*)pdvi;

  TRACE("(%p)\n",pdvi);

  if (!pdvi)
    return E_INVALIDARG;

  switch (pdvi2->info1.cbSize)
  {
  case sizeof(DLLVERSIONINFO2):
    pdvi2->dwFlags = 0;
    pdvi2->ullVersion = MAKEDLLVERULL(6, 0, 2800, 1612);
    /* Fall through */
  case sizeof(DLLVERSIONINFO):
    pdvi2->info1.dwMajorVersion = 6;
    pdvi2->info1.dwMinorVersion = 0;
    pdvi2->info1.dwBuildNumber = 2800;
    pdvi2->info1.dwPlatformID = DLLVER_PLATFORM_WINDOWS;
    return S_OK;
 }

 WARN("pdvi->cbSize = %ld, unhandled\n", pdvi2->info1.cbSize);
 return E_INVALIDARG;
}

/*************************************************************************
 *      WhichPlatform()        [SHLWAPI.276]
 */
UINT WINAPI WhichPlatform(void)
{
    static const char szIntegratedBrowser[] = "IntegratedBrowser";
    static DWORD state = PLATFORM_UNKNOWN;
    DWORD ret, data, size;
    HMODULE hshell32;
    HKEY hKey;

    if (state)
        return state;

    /* If shell32 exports DllGetVersion(), the browser is integrated */
    state = PLATFORM_BROWSERONLY;
    hshell32 = LoadLibraryA("shell32.dll");
    if (hshell32)
    {
        FARPROC pDllGetVersion;
        pDllGetVersion = GetProcAddress(hshell32, "DllGetVersion");
        state = pDllGetVersion ? PLATFORM_INTEGRATED : PLATFORM_BROWSERONLY;
        FreeLibrary(hshell32);
    }

    /* Set or delete the key accordingly */
    ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Internet Explorer", 0, KEY_ALL_ACCESS, &hKey);
    if (!ret)
    {
        size = sizeof(data);
        ret = RegQueryValueExA(hKey, szIntegratedBrowser, 0, 0, (BYTE *)&data, &size);
        if (!ret && state == PLATFORM_BROWSERONLY)
        {
            /* Value exists but browser is not integrated */
            RegDeleteValueA(hKey, szIntegratedBrowser);
        }
        else if (ret && state == PLATFORM_INTEGRATED)
        {
            /* Browser is integrated but value does not exist */
            data = TRUE;
            RegSetValueExA(hKey, szIntegratedBrowser, 0, REG_DWORD, (BYTE *)&data, sizeof(data));
        }
        RegCloseKey(hKey);
    }

    return state;
}

/***********************************************************************
 *             SHGetViewStatePropertyBag [SHLWAPI.515]
 */
HRESULT WINAPI SHGetViewStatePropertyBag(PCIDLIST_ABSOLUTE pidl, PCWSTR bag_name, DWORD flags, REFIID riid, void **ppv)
{
    FIXME("%p, %s, %#lx, %s, %p stub.\n", pidl, debugstr_w(bag_name), flags, debugstr_guid(riid), ppv);

    return E_NOTIMPL;
}

/*************************************************************************
 *      SHIsLowMemoryMachine    [SHLWAPI.@]
 */
BOOL WINAPI SHIsLowMemoryMachine(DWORD type)
{
    FIXME("%ld stub\n", type);

    return FALSE;
}
