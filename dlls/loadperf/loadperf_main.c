/*
 * Implementation of loadperf.dll
 *
 * Copyright 2009 Andrey Turkin
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
#include "winerror.h"
#include "winnls.h"
#include "wine/debug.h"

#include "loadperf.h"

WINE_DEFAULT_DEBUG_CHANNEL(loadperf);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n",hinstDLL,fdwReason,lpvReserved);

    switch(fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE; /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}

/*************************************************************
 *     LoadPerfCounterTextStringsA (loadperf.@)
 *
 * NOTES
 *   See LoadPerfCounterTextStringsW
 */
DWORD WINAPI LoadPerfCounterTextStringsA(LPCSTR cmdline, BOOL quiet)
{
    DWORD ret;
    LPWSTR cmdlineW = NULL;

    if (cmdline)
    {
        INT len = MultiByteToWideChar(CP_ACP, 0, cmdline, -1, NULL, 0);
        cmdlineW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!cmdlineW) return ERROR_NOT_ENOUGH_MEMORY;
        MultiByteToWideChar(CP_ACP, 0, cmdline, -1, cmdlineW, len);
    }

    ret = LoadPerfCounterTextStringsW(cmdlineW, quiet);

    HeapFree(GetProcessHeap(), 0, cmdlineW);

    return ret;
}

/*************************************************************
 *     LoadPerfCounterTextStringsW (loadperf.@)
 *
 * PARAMS
 *   cmdline [in] Last argument in command line - ini file to be used
 *   quiet   [in] FALSE - the function may write to stdout
 *
 */
DWORD WINAPI LoadPerfCounterTextStringsW(LPCWSTR cmdline, BOOL quiet)
{
    FIXME("(%s, %d): stub\n", debugstr_w(cmdline), quiet);

    return ERROR_SUCCESS;
}

/*************************************************************
 *     UnloadPerfCounterTextStringsA (loadperf.@)
 *
 * NOTES
 *   See UnloadPerfCounterTextStringsW
 */
DWORD WINAPI UnloadPerfCounterTextStringsA(LPCSTR cmdline, BOOL verbose)
{
    DWORD ret;
    LPWSTR cmdlineW = NULL;

    if (cmdline)
    {
        INT len = MultiByteToWideChar(CP_ACP, 0, cmdline, -1, NULL, 0);
        cmdlineW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!cmdlineW) return ERROR_NOT_ENOUGH_MEMORY;
        MultiByteToWideChar(CP_ACP, 0, cmdline, -1, cmdlineW, len);
    }

    ret = UnloadPerfCounterTextStringsW(cmdlineW, verbose);

    HeapFree(GetProcessHeap(), 0, cmdlineW);

    return ret;
}

/*************************************************************
 *     UnloadPerfCounterTextStringsW (loadperf.@)
 *
 * PARAMS
 *   cmdline [in] Last argument in command line - application counters to be removed
 *   verbose [in] TRUE - the function may write to stdout
 *
 */
DWORD WINAPI UnloadPerfCounterTextStringsW(LPCWSTR cmdline, BOOL verbose)
{
    FIXME("(%s, %d): stub\n", debugstr_w(cmdline), verbose);

    return ERROR_SUCCESS;
}
