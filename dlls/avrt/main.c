/* Avrt dll implementation
 *
 * Copyright (C) 2009 Maarten Lankhorst
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wine/debug.h"
#include "avrt.h"

WINE_DEFAULT_DEBUG_CHANNEL(avrt);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

    return TRUE;
}

HANDLE WINAPI AvSetMmThreadCharacteristicsA(LPCSTR TaskName, LPDWORD TaskIndex)
{
    HANDLE ret;
    LPWSTR str = NULL;

    if (TaskName)
    {
        DWORD len = (lstrlenA(TaskName)+1);
        str = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
        if (!str)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return NULL;
        }
        MultiByteToWideChar(CP_ACP, 0, TaskName, len, str, len);
    }
    ret = AvSetMmThreadCharacteristicsW(str, TaskIndex);
    HeapFree(GetProcessHeap(), 0, str);
    return ret;
}

HANDLE WINAPI AvSetMmThreadCharacteristicsW(LPCWSTR TaskName, LPDWORD TaskIndex)
{
    FIXME("(%s,%p): stub\n", debugstr_w(TaskName), TaskIndex);

    if (!TaskName)
    {
        SetLastError(ERROR_INVALID_TASK_NAME);
        return NULL;
    }
    if (!TaskIndex)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }
    return (HANDLE)0x12345678;
}

BOOL WINAPI AvQuerySystemResponsiveness(HANDLE AvrtHandle, ULONG *value)
{
    FIXME("(%p, %p): stub\n", AvrtHandle, value);
    return FALSE;
}

BOOL WINAPI AvRevertMmThreadCharacteristics(HANDLE AvrtHandle)
{
    FIXME("(%p): stub\n", AvrtHandle);
    return TRUE;
}

BOOL WINAPI AvSetMmThreadPriority(HANDLE AvrtHandle, AVRT_PRIORITY prio)
{
    FIXME("(%p)->(%u) stub\n", AvrtHandle, prio);
    return TRUE;
}
