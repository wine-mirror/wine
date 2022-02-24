/*
 * Copyright 2022 Dmitry Timoshkov
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

#include <windef.h>
#include <winbase.h>
#include <objbase.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dllhost);

int WINAPI wWinMain(HINSTANCE hinst, HINSTANCE previnst, LPWSTR cmdline, int showcmd)
{
    HRESULT hr;
    CLSID clsid;

    if (wcsnicmp(cmdline, L"/PROCESSID:", 11))
        return 0;

    cmdline += 11;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = CLSIDFromString(cmdline, &clsid);
    if (hr == S_OK)
    {
        FIXME("hosting object %s is not implemented\n", wine_dbgstr_guid(&clsid));
    }

    CoUninitialize();

    return 0;
}
