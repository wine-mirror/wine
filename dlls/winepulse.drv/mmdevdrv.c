/*
 * Copyright 2011-2012 Maarten Lankhorst
 * Copyright 2010-2011 Maarten Lankhorst for CodeWeavers
 * Copyright 2011 Andrew Eikum for CodeWeavers
 * Copyright 2022 Huw Davies
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

#define COBJMACROS

#include <stdarg.h>
#include <assert.h>
#include <wchar.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/debug.h"
#include "wine/list.h"
#include "wine/unixlib.h"

#include "ole2.h"
#include "mimeole.h"
#include "dshow.h"
#include "dsound.h"
#include "propsys.h"
#include "propkey.h"

#include "initguid.h"
#include "ks.h"
#include "ksmedia.h"
#include "mmdeviceapi.h"
#include "audioclient.h"
#include "endpointvolume.h"
#include "audiopolicy.h"

#include "../mmdevapi/unixlib.h"
#include "../mmdevapi/mmdevdrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(pulse);

#define MAX_PULSE_NAME_LEN 256

static GUID pulse_render_guid =
{ 0xfd47d9cc, 0x4218, 0x4135, { 0x9c, 0xe2, 0x0c, 0x19, 0x5c, 0x87, 0x40, 0x5b } };
static GUID pulse_capture_guid =
{ 0x25da76d0, 0x033c, 0x4235, { 0x90, 0x02, 0x19, 0xf4, 0x88, 0x94, 0xac, 0x6f } };

static WCHAR drv_key_devicesW[256];

BOOL WINAPI DllMain(HINSTANCE dll, DWORD reason, void *reserved)
{
    WCHAR buf[MAX_PATH];
    WCHAR *filename;

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(dll);
        if (__wine_init_unix_call())
            return FALSE;

        GetModuleFileNameW(dll, buf, ARRAY_SIZE(buf));

        filename = wcsrchr(buf, '\\');
        filename = filename ? filename + 1 : buf;

        swprintf(drv_key_devicesW, ARRAY_SIZE(drv_key_devicesW),
                 L"Software\\Wine\\Drivers\\%s\\devices", filename);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

void WINAPI get_device_guid(EDataFlow flow, const char *pulse_name, GUID *guid)
{
    WCHAR key_name[MAX_PULSE_NAME_LEN + 2];
    DWORD type, size = sizeof(*guid);
    LSTATUS status;
    HKEY drv_key, dev_key;

    if (!pulse_name[0]) {
        *guid = (flow == eRender) ? pulse_render_guid : pulse_capture_guid;
        return;
    }

    status = RegCreateKeyExW(HKEY_CURRENT_USER, drv_key_devicesW, 0, NULL, 0,
                             KEY_WRITE | KEY_WOW64_64KEY, NULL, &drv_key, NULL);
    if (status != ERROR_SUCCESS) {
        ERR("Failed to open devices registry key: %lu\n", status);
        CoCreateGuid(guid);
        return;
    }

    key_name[0] = (flow == eRender) ? '0' : '1';
    key_name[1] = ',';
    MultiByteToWideChar(CP_UNIXCP, 0, pulse_name, -1, key_name + 2, ARRAY_SIZE(key_name) - 2);

    status = RegCreateKeyExW(drv_key, key_name, 0, NULL, 0, KEY_READ | KEY_WRITE | KEY_WOW64_64KEY,
                             NULL, &dev_key, NULL);
    if (status != ERROR_SUCCESS) {
        ERR("Failed to open registry key for device %s: %lu\n", pulse_name, status);
        RegCloseKey(drv_key);
        CoCreateGuid(guid);
        return;
    }

    status = RegQueryValueExW(dev_key, L"guid", 0, &type, (BYTE*)guid, &size);
    if (status != ERROR_SUCCESS || type != REG_BINARY || size != sizeof(*guid)) {
        CoCreateGuid(guid);
        status = RegSetValueExW(dev_key, L"guid", 0, REG_BINARY, (BYTE*)guid, sizeof(*guid));
        if (status != ERROR_SUCCESS)
            ERR("Failed to store device GUID for %s to registry: %lu\n", pulse_name, status);
    }
    RegCloseKey(dev_key);
    RegCloseKey(drv_key);
}
