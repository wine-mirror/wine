/* Xbox toolhelpx.dll stubs
 *
 * Based on XWine1/SlimEra toolhelpx.cpp (MIT)
 * Provides Xbox-specific extensions to process/module enumeration.
 * Standard Win32 functions are forwarded to their kernel32/advapi32 equivalents.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(toolhelpx);

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(inst);
    return TRUE;
}

/* Xbox-specific: resolve source line from address - not available on Linux */
HRESULT WINAPI GetSourceLineFromAddress(int disposition, DWORD frameCount,
        ULONG_PTR *addresses, BOOL (*callback)(void*, ULONG_PTR, HRESULT, LPCWSTR, ULONG),
        void *context)
{
    FIXME("(%d, %ld, %p, %p, %p) stub\n", disposition, frameCount, addresses, callback, context);
    return E_NOTIMPL;
}

/* Xbox-specific: resolve symbol from address */
HRESULT WINAPI GetSymbolFromAddress(int disposition, DWORD frameCount,
        ULONG_PTR *addresses, BOOL (*callback)(void*, ULONG_PTR, HRESULT, LPCWSTR, ULONG),
        void *context)
{
    FIXME("(%d, %ld, %p, %p, %p) stub\n", disposition, frameCount, addresses, callback, context);
    return E_NOTIMPL;
}

/* Xbox-specific hardware info query */
BOOL WINAPI QuerySystemHardwareInfo(void)
{
    FIXME("stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/* Thread naming forwarded to kernelx */
BOOL WINAPI SetThreadName(HANDLE hThread, LPCWSTR name)
{
    TRACE("(%p, %s)\n", hThread, debugstr_w(name));
    return SetThreadDescription(hThread, name);
}

BOOL WINAPI GetThreadName(HANDLE hThread, LPWSTR buf, SIZE_T len, SIZE_T *pRet)
{
    PWSTR desc = NULL;
    int n;
    TRACE("(%p)\n", hThread);
    if (!pRet) { SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }
    if (FAILED(GetThreadDescription(hThread, &desc))) { *pRet = 0; return FALSE; }
    n = lstrlenW(desc);
    *pRet = n;
    if (!buf || (SIZE_T)n >= len) { LocalFree(desc); SetLastError(ERROR_INSUFFICIENT_BUFFER); return FALSE; }
    memcpy(buf, desc, (n+1)*sizeof(WCHAR));
    LocalFree(desc);
    return TRUE;
}
