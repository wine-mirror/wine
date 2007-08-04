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
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"

#include "cor.h"
#include "mscoree.h"

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

static LPWSTR get_mono_exe(void)
{
    static const WCHAR mono_exe[] = {'b','i','n','\\','m','o','n','o','.','e','x','e',' ',0};
    static const WCHAR mono_key[] = {'S','o','f','t','w','a','r','e','\\','N','o','v','e','l','l','\\','M','o','n','o',0};
    static const WCHAR defaul_clr[] = {'D','e','f','a','u','l','t','C','L','R',0};
    static const WCHAR install_root[] = {'S','d','k','I','n','s','t','a','l','l','R','o','o','t',0};
    static const WCHAR slash[] = {'\\',0};

    WCHAR version[64], version_key[MAX_PATH], root[MAX_PATH], *ret;
    DWORD len, size;
    HKEY key;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, mono_key, 0, KEY_READ, &key))
        return NULL;

    len = sizeof(version);
    if (RegQueryValueExW(key, defaul_clr, 0, NULL, (LPBYTE)version, &len))
    {
        RegCloseKey(key);
        return NULL;
    }
    RegCloseKey(key);

    lstrcpyW(version_key, mono_key);
    lstrcatW(version_key, slash);
    lstrcatW(version_key, version);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, version_key, 0, KEY_READ, &key))
        return NULL;

    len = sizeof(root);
    if (RegQueryValueExW(key, install_root, 0, NULL, (LPBYTE)root, &len))
    {
        RegCloseKey(key);
        return NULL;
    }
    RegCloseKey(key);

    size = len + sizeof(slash) + sizeof(mono_exe);
    if (!(ret = HeapAlloc(GetProcessHeap(), 0, size))) return NULL;

    lstrcpyW(ret, root);
    lstrcatW(ret, slash);
    lstrcatW(ret, mono_exe);

    return ret;
}

__int32 WINAPI _CorExeMain(void)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    WCHAR *mono_exe, *cmd_line;
    DWORD size, exit_code;

    if (!(mono_exe = get_mono_exe()))
    {
        MESSAGE("install the Windows version of Mono to run .NET executables\n");
        return -1;
    }

    size = (lstrlenW(mono_exe) + lstrlenW(GetCommandLineW()) + 1) * sizeof(WCHAR);
    if (!(cmd_line = HeapAlloc(GetProcessHeap(), 0, size)))
    {
        HeapFree(GetProcessHeap(), 0, mono_exe);
        return -1;
    }

    lstrcpyW(cmd_line, mono_exe);
    HeapFree(GetProcessHeap(), 0, mono_exe);
    lstrcatW(cmd_line, GetCommandLineW());

    TRACE("new command line: %s\n", debugstr_w(cmd_line));

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    if (!CreateProcessW(NULL, cmd_line, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        HeapFree(GetProcessHeap(), 0, cmd_line);
        return -1;
    }
    HeapFree(GetProcessHeap(), 0, cmd_line);

    /* wait for the process to exit */
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return (int)exit_code;
}

__int32 WINAPI _CorExeMain2(PBYTE ptrMemory, DWORD cntMemory, LPWSTR imageName, LPWSTR loaderName, LPWSTR cmdLine)
{
    TRACE("(%p, %u, %s, %s, %s)\n", ptrMemory, cntMemory, debugstr_w(imageName), debugstr_w(loaderName), debugstr_w(cmdLine));
    FIXME("Directly running .NET applications not supported.\n");
    return -1;
}

void WINAPI CorExitProcess(int exitCode)
{
    FIXME("(%x) stub\n", exitCode);
    ExitProcess(exitCode);
}

VOID WINAPI _CorImageUnloading(PVOID imageBase)
{
    TRACE("(%p): stub\n", imageBase);
}

HRESULT WINAPI _CorValidateImage(PVOID* imageBase, LPCWSTR imageName)
{
    TRACE("(%p, %s): stub\n", imageBase, debugstr_w(imageName));
    return E_FAIL;
}

HRESULT WINAPI GetCORSystemDirectory(LPWSTR pbuffer, DWORD cchBuffer, DWORD *dwLength)
{
    FIXME("(%p, %d, %p): stub!\n", pbuffer, cchBuffer, dwLength);

    if (!dwLength)
        return E_POINTER;

    *dwLength = 0;

    return S_OK;
}

HRESULT WINAPI GetCORVersion(LPWSTR pbuffer, DWORD cchBuffer, DWORD *dwLength)
{
    static const WCHAR version[] = {'v','1','.','1','.','4','3','2','2',0};

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

HRESULT WINAPI GetRequestedRuntimeInfo(LPCWSTR pExe, LPCWSTR pwszVersion, LPCWSTR pConfigurationFile,
    DWORD startupFlags, DWORD runtimeInfoFlags, LPWSTR pDirectory, DWORD dwDirectory, DWORD *dwDirectoryLength,
    LPWSTR pVersion, DWORD cchBuffer, DWORD *dwlength)
{
    FIXME("(%s, %s, %s, 0x%08x, 0x%08x, %p, 0x%08x, %p, %p, 0x%08x, %p) stub\n", debugstr_w(pExe),
          debugstr_w(pwszVersion), debugstr_w(pConfigurationFile), startupFlags, runtimeInfoFlags, pDirectory,
          dwDirectory, dwDirectoryLength, pVersion, cchBuffer, dwlength);
    return GetCORVersion(pVersion, cchBuffer, dwlength);
}

HRESULT WINAPI LoadLibraryShim( LPCWSTR szDllName, LPCWSTR szVersion, LPVOID pvReserved, HMODULE * phModDll)
{
    FIXME("(%p %s, %p, %p, %p): semi-stub\n", szDllName, debugstr_w(szDllName), szVersion, pvReserved, phModDll);

    if (phModDll) *phModDll = LoadLibraryW(szDllName);
    return S_OK;
}

HRESULT WINAPI CoInitializeCor(DWORD fFlags)
{
    FIXME("(0x%08x): stub\n", fFlags);
    return S_OK;
}

HRESULT WINAPI GetAssemblyMDImport(LPCWSTR szFileName, REFIID riid, IUnknown **ppIUnk)
{
    FIXME("(%p %s, %p, %p): stub\n", szFileName, debugstr_w(szFileName), riid, *ppIUnk);
    return ERROR_CALL_NOT_IMPLEMENTED;
}
