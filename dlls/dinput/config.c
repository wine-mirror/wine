/*
 * Copyright (c) 2011 Lucas Fialho Zawacki
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

#include "wine/debug.h"
#include "wine/unicode.h"
#include "objbase.h"
#include "dinput_private.h"
#include "device_private.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

HRESULT _configure_devices(IDirectInput8W *iface,
                           LPDICONFIGUREDEVICESCALLBACK lpdiCallback,
                           LPDICONFIGUREDEVICESPARAMSW lpdiCDParams,
                           DWORD dwFlags,
                           LPVOID pvRefData
)
{
    InitCommonControls();

    DialogBoxParamW(GetModuleHandleA("dinput.dll"), (LPCWSTR) MAKEINTRESOURCE(IDD_CONFIGUREDEVICES), lpdiCDParams->hwnd, 0, 0);

    return DI_OK;
}
