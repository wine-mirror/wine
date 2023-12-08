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

static struct list g_devices_cache = LIST_INIT(g_devices_cache);

struct device_cache {
    struct list entry;
    GUID guid;
    EDataFlow dataflow;
    char pulse_name[0];
};

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
        if (!reserved)
        {
            struct device_cache *device, *device_next;

            LIST_FOR_EACH_ENTRY_SAFE(device, device_next, &g_devices_cache, struct device_cache, entry)
                free(device);
        }
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

BOOL WINAPI get_device_name_from_guid(GUID *guid, char **name, EDataFlow *flow)
{
    struct device_cache *device;
    WCHAR key_name[MAX_PULSE_NAME_LEN + 2];
    DWORD key_name_size;
    DWORD index = 0;
    HKEY key;

    *name = NULL;

    /* Return empty string for default PulseAudio device */
    if (IsEqualGUID(guid, &pulse_render_guid)) {
        *flow = eRender;
        if (!(*name = malloc(1)))
            return FALSE;
    } else if (IsEqualGUID(guid, &pulse_capture_guid)) {
        *flow = eCapture;
        if (!(*name = malloc(1)))
            return FALSE;
    }

    if (*name) {
        *name[0] = '\0';
        return TRUE;
    }

    /* Check the cache first */
    LIST_FOR_EACH_ENTRY(device, &g_devices_cache, struct device_cache, entry) {
        if (!IsEqualGUID(guid, &device->guid))
            continue;
        *flow = device->dataflow;
        if ((*name = strdup(device->pulse_name)))
            return TRUE;

        return FALSE;
    }

    if (RegOpenKeyExW(HKEY_CURRENT_USER, drv_key_devicesW, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS) {
        WARN("No devices found in registry\n");
        return FALSE;
    }

    for (;;) {
        DWORD size, type;
        LSTATUS status;
        GUID reg_guid;
        HKEY dev_key;
        int len;

        key_name_size = ARRAY_SIZE(key_name);
        if (RegEnumKeyExW(key, index++, key_name, &key_name_size, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;

        if (RegOpenKeyExW(key, key_name, 0, KEY_READ | KEY_WOW64_64KEY, &dev_key) != ERROR_SUCCESS) {
            ERR("Couldn't open key: %s\n", wine_dbgstr_w(key_name));
            continue;
        }

        size = sizeof(reg_guid);
        status = RegQueryValueExW(dev_key, L"guid", 0, &type, (BYTE *)&reg_guid, &size);
        RegCloseKey(dev_key);

        if (status == ERROR_SUCCESS && type == REG_BINARY && size == sizeof(reg_guid) && IsEqualGUID(&reg_guid, guid)) {
            RegCloseKey(key);

            TRACE("Found matching device key: %s\n", wine_dbgstr_w(key_name));

            if (key_name[0] == '0')
                *flow = eRender;
            else if (key_name[0] == '1')
                *flow = eCapture;
            else {
                WARN("Unknown device type: %c\n", key_name[0]);
                return FALSE;
            }

            if (!(len = WideCharToMultiByte(CP_UNIXCP, 0, key_name + 2, -1, NULL, 0, NULL, NULL)))
                    return FALSE;

            if (!(*name = malloc(len)))
                return FALSE;

            if (!WideCharToMultiByte(CP_UNIXCP, 0, key_name + 2, -1, *name, len, NULL, NULL)) {
                free(*name);
                return FALSE;
            }

            if ((device = malloc(FIELD_OFFSET(struct device_cache, pulse_name[len])))) {
                device->guid = reg_guid;
                device->dataflow = *flow;
                memcpy(device->pulse_name, *name, len);
                list_add_tail(&g_devices_cache, &device->entry);
            }
            return TRUE;
        }
    }

    RegCloseKey(key);
    WARN("No matching device in registry for GUID %s\n", debugstr_guid(guid));
    return FALSE;
}
