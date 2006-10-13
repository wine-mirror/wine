/*
 * Implementation of mscoree.dll
 * Microsoft Component Object Runtime Execution Engine
 *
 * Copyright 2006 Paul Chitescu
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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL( mscoree );

HRESULT WINAPI CorBindToRuntimeHost(LPCWSTR pwszVersion, LPCWSTR pwszBuildFlavor,
                                    LPCWSTR pwszHostConfigFile, VOID *pReserved,
                                    DWORD startupFlags, REFCLSID rclsid,
                                    REFIID riid, LPVOID *ppv)
{
    FIXME("(%s, %s, %s, %p, %d, %p, %p, %p): stub!\n", debugstr_w(pwszVersion),
          debugstr_w(pwszBuildFlavor), debugstr_w(pwszHostConfigFile), pReserved,
          startupFlags, rclsid, riid, ppv);

    return E_FAIL;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

BOOL WINAPI _CorDllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    FIXME("(%p, %d, %p): stub\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

int WINAPI _CorExeMain(void)
{
    FIXME("Directly running .NET applications not supported.\n");
    return -1;
}

int WINAPI _CorExeMain2(PBYTE ptrMemory, DWORD cntMemory, LPCWSTR imageName, LPCWSTR loaderName, LPCWSTR cmdLine)
{
    TRACE("(%p, %u, %s, %s, %s)\n", ptrMemory, cntMemory, debugstr_w(imageName), debugstr_w(loaderName), debugstr_w(cmdLine));
    FIXME("Directly running .NET applications not supported.\n");
    return -1;
}

void WINAPI _CorImageUnloading(LPCVOID* imageBase)
{
    TRACE("(%p): stub\n", imageBase);
}

DWORD _CorValidateImage(LPCVOID* imageBase, LPCWSTR imageName)
{
    TRACE("(%p, %s): stub\n", imageBase, debugstr_w(imageName));
    return E_FAIL;
}

HRESULT WINAPI GetCORVersion(LPWSTR pbuffer, DWORD cchBuffer, DWORD *dwLength)
{
    static const WCHAR version[] = {'1','.','1','.','4','3','2','2',0};

    FIXME("(%p, %d, %p): semi-stub!\n", pbuffer, cchBuffer, dwLength);

    if (!dwLength)
        return E_POINTER;

    *dwLength = lstrlenW(version);

    if (cchBuffer < *dwLength)
        return ERROR_INSUFFICIENT_BUFFER;

    if (pbuffer)
        lstrcpyW(pbuffer, version);

    return S_OK;
}
