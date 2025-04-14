/*
 * Copyright 2011 Andrew Eikum for CodeWeavers
 *           2022 Huw Davies
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
#include <wchar.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winnls.h"
#include "winreg.h"

#include "ole2.h"
#include "mmdeviceapi.h"
#include "devpkey.h"
#include "dshow.h"
#include "dsound.h"

#include "initguid.h"
#include "endpointvolume.h"
#include "audiopolicy.h"
#include "audioclient.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "wine/unixlib.h"

#include "unixlib.h"

#include "../mmdevapi/mmdevdrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(oss);

static WCHAR drv_key_devicesW[256];
static const WCHAR guidW[] = {'g','u','i','d',0};

BOOL WINAPI DllMain(HINSTANCE dll, DWORD reason, void *reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
    {
        WCHAR buf[MAX_PATH];
        WCHAR *filename;

        if(__wine_init_unix_call()) return FALSE;

        GetModuleFileNameW(dll, buf, ARRAY_SIZE(buf));

        filename = wcsrchr(buf, '\\');
        filename = filename ? filename + 1 : buf;

        swprintf(drv_key_devicesW, ARRAY_SIZE(drv_key_devicesW),
                 L"Software\\Wine\\Drivers\\%s\\devices", filename);

        break;
    }
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

static void set_device_guid(EDataFlow flow, HKEY drv_key, const WCHAR *key_name,
        GUID *guid)
{
    HKEY key;
    BOOL opened = FALSE;
    LONG lr;

    if(!drv_key){
        lr = RegCreateKeyExW(HKEY_CURRENT_USER, drv_key_devicesW, 0, NULL, 0, KEY_WRITE,
                    NULL, &drv_key, NULL);
        if(lr != ERROR_SUCCESS){
            ERR("RegCreateKeyEx(drv_key) failed: %lu\n", lr);
            return;
        }
        opened = TRUE;
    }

    lr = RegCreateKeyExW(drv_key, key_name, 0, NULL, 0, KEY_WRITE,
                NULL, &key, NULL);
    if(lr != ERROR_SUCCESS){
        ERR("RegCreateKeyEx(%s) failed: %lu\n", wine_dbgstr_w(key_name), lr);
        goto exit;
    }

    lr = RegSetValueExW(key, guidW, 0, REG_BINARY, (BYTE*)guid,
                sizeof(GUID));
    if(lr != ERROR_SUCCESS)
        ERR("RegSetValueEx(%s\\guid) failed: %lu\n", wine_dbgstr_w(key_name), lr);

    RegCloseKey(key);
exit:
    if(opened)
        RegCloseKey(drv_key);
}

void WINAPI get_device_guid(EDataFlow flow, const char *device, GUID *guid)
{
    HKEY key = NULL, dev_key;
    DWORD type, size = sizeof(*guid);
    WCHAR key_name[256];

    if(flow == eCapture)
        key_name[0] = '1';
    else
        key_name[0] = '0';
    key_name[1] = ',';
    MultiByteToWideChar(CP_UNIXCP, 0, device, -1, key_name + 2, ARRAY_SIZE(key_name) - 2);

    if(RegOpenKeyExW(HKEY_CURRENT_USER, drv_key_devicesW, 0, KEY_WRITE|KEY_READ, &key) == ERROR_SUCCESS){
        if(RegOpenKeyExW(key, key_name, 0, KEY_READ, &dev_key) == ERROR_SUCCESS){
            if(RegQueryValueExW(dev_key, guidW, 0, &type,
                        (BYTE*)guid, &size) == ERROR_SUCCESS){
                if(type == REG_BINARY){
                    RegCloseKey(dev_key);
                    RegCloseKey(key);
                    goto exit;
                }
                ERR("Invalid type for device %s GUID: %lu; ignoring and overwriting\n",
                        wine_dbgstr_w(key_name), type);
            }
            RegCloseKey(dev_key);
        }
    }

    CoCreateGuid(guid);

    set_device_guid(flow, key, key_name, guid);
exit:
    if(key)
        RegCloseKey(key);
}
