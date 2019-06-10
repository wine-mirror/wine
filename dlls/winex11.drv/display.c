/*
 * X11DRV display device functions
 *
 * Copyright 2019 Zhiyi Zhang for CodeWeavers
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
#include "winuser.h"
#include "rpc.h"
#include "winreg.h"
#include "initguid.h"
#include "devguid.h"
#include "devpkey.h"
#include "setupapi.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "x11drv.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

static const WCHAR driver_descW[] = {'D','r','i','v','e','r','D','e','s','c',0};
static const WCHAR video_idW[] = {'V','i','d','e','o','I','D',0};
static const WCHAR guid_fmtW[] = {
    '{','%','0','8','x','-','%','0','4','x','-','%','0','4','x','-','%','0','2','x','%','0','2','x','-',
    '%','0','2','x','%','0','2','x','%','0','2','x','%','0','2','x','%','0','2','x','%','0','2','x','}',0};
static const WCHAR gpu_instance_fmtW[] = {
    'P','C','I','\\',
    'V','E','N','_','%','0','4','X','&',
    'D','E','V','_','%','0','4','X','&',
    'S','U','B','S','Y','S','_','%','0','8','X','&',
    'R','E','V','_','%','0','2','X','\\',
    '%','0','8','X',0};
static const WCHAR gpu_hardware_id_fmtW[] = {
    'P','C','I','\\',
    'V','E','N','_','%','0','4','X','&',
    'D','E','V','_','%','0','4','X','&',
    'S','U','B','S','Y','S','_','0','0','0','0','0','0','0','0','&',
    'R','E','V','_','0','0',0};
static const WCHAR video_keyW[] = {
    'H','A','R','D','W','A','R','E','\\',
    'D','E','V','I','C','E','M','A','P','\\',
    'V','I','D','E','O',0};

static struct x11drv_display_device_handler handler;

void X11DRV_DisplayDevices_SetHandler(const struct x11drv_display_device_handler *new_handler)
{
    if (new_handler->priority > handler.priority)
    {
        handler = *new_handler;
        TRACE("Display device functions are now handled by: %s\n", handler.name);
    }
}

/* Initialize a GPU instance */
static BOOL X11DRV_InitGpu(HDEVINFO devinfo, const struct x11drv_gpu *gpu, INT gpu_index)
{
    static const BOOL present = TRUE;
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    WCHAR instanceW[MAX_PATH];
    WCHAR bufferW[1024];
    HKEY hkey = NULL;
    GUID guid;
    INT written;
    DWORD size;
    BOOL ret = FALSE;

    sprintfW(instanceW, gpu_instance_fmtW, gpu->vendor_id, gpu->device_id, gpu->subsys_id, gpu->revision_id, gpu_index);
    if (!SetupDiOpenDeviceInfoW(devinfo, instanceW, NULL, 0, &device_data))
    {
        SetupDiCreateDeviceInfoW(devinfo, instanceW, &GUID_DEVCLASS_DISPLAY, gpu->name, NULL, 0, &device_data);
        if (!SetupDiRegisterDeviceInfo(devinfo, &device_data, 0, NULL, NULL, NULL))
            goto done;
    }

    /* Write HardwareID registry property, REG_MULTI_SZ */
    written = sprintfW(bufferW, gpu_hardware_id_fmtW, gpu->vendor_id, gpu->device_id);
    bufferW[written + 1] = 0;
    if (!SetupDiSetDeviceRegistryPropertyW(devinfo, &device_data, SPDRP_HARDWAREID, (const BYTE *)bufferW,
                                           (written + 2) * sizeof(WCHAR)))
        goto done;

    /* Write DEVPKEY_Device_IsPresent property */
    if (!SetupDiSetDevicePropertyW(devinfo, &device_data, &DEVPKEY_Device_IsPresent, DEVPROP_TYPE_BOOLEAN,
                                   (const BYTE *)&present, sizeof(present), 0))
        goto done;

    /* Open driver key.
     * This is where HKLM\System\CurrentControlSet\Control\Video\{GPU GUID}\{Adapter Index} links to */
    hkey = SetupDiCreateDevRegKeyW(devinfo, &device_data, DICS_FLAG_GLOBAL, 0, DIREG_DRV, NULL, NULL);

    /* Write DriverDesc value */
    if (RegSetValueExW(hkey, driver_descW, 0, REG_SZ, (const BYTE *)gpu->name,
                       (strlenW(gpu->name) + 1) * sizeof(WCHAR)))
        goto done;
    RegCloseKey(hkey);

    /* Write GUID in VideoID in .../instance/Device Parameters, reuse the GUID if it's existent */
    hkey = SetupDiCreateDevRegKeyW(devinfo, &device_data, DICS_FLAG_GLOBAL, 0, DIREG_DEV, NULL, NULL);

    size = sizeof(bufferW);
    if (RegQueryValueExW(hkey, video_idW, 0, NULL, (BYTE *)bufferW, &size))
    {
        UuidCreate(&guid);
        sprintfW(bufferW, guid_fmtW, guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2],
                 guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
        if (RegSetValueExW(hkey, video_idW, 0, REG_SZ, (const BYTE *)bufferW, (strlenW(bufferW) + 1) * sizeof(WCHAR)))
            goto done;
    }

    ret = TRUE;
done:
    RegCloseKey(hkey);
    if (!ret)
        ERR("Failed to initialize GPU\n");
    return ret;
}

static void prepare_devices(void)
{
    static const BOOL not_present = FALSE;
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    HDEVINFO devinfo;
    DWORD i = 0;

    /* FIXME:
     * Currently SetupDiGetClassDevsW with DIGCF_PRESENT is unsupported, So we need to clean up not present devices in
     * case application uses SetupDiGetClassDevsW to enumerate devices. Wrong devices could exist in registry as a result
     * of prefix copying or having devices unplugged. But then we couldn't simply delete GPUs because we need to retain
     * the same GUID for the same GPU. */
    devinfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_DISPLAY, NULL, NULL, 0);
    while (SetupDiEnumDeviceInfo(devinfo, i++, &device_data))
    {
        if (!SetupDiSetDevicePropertyW(devinfo, &device_data, &DEVPKEY_Device_IsPresent, DEVPROP_TYPE_BOOLEAN,
                                       (const BYTE *)&not_present, sizeof(not_present), 0))
            ERR("Failed to set GPU present property\n");
    }
    SetupDiDestroyDeviceInfoList(devinfo);
}

static void cleanup_devices(void)
{
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    HDEVINFO devinfo;
    DWORD type;
    DWORD i = 0;
    BOOL present;

    devinfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_DISPLAY, NULL, NULL, 0);
    while (SetupDiEnumDeviceInfo(devinfo, i++, &device_data))
    {
        present = FALSE;
        SetupDiGetDevicePropertyW(devinfo, &device_data, &DEVPKEY_Device_IsPresent, &type, (BYTE *)&present,
                                  sizeof(present), NULL, 0);
        if (!present && !SetupDiRemoveDevice(devinfo, &device_data))
            ERR("Failed to remove GPU\n");
    }
    SetupDiDestroyDeviceInfoList(devinfo);
}

void X11DRV_DisplayDevices_Init(void)
{
    static const WCHAR init_mutexW[] = {'d','i','s','p','l','a','y','_','d','e','v','i','c','e','_','i','n','i','t',0};
    HANDLE mutex;
    struct x11drv_gpu *gpus = NULL;
    INT gpu_count;
    INT gpu;
    HDEVINFO gpu_devinfo = NULL;
    HKEY video_hkey = NULL;
    DWORD disposition = 0;

    mutex = CreateMutexW(NULL, FALSE, init_mutexW);
    WaitForSingleObject(mutex, INFINITE);

    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, video_keyW, 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &video_hkey,
                        &disposition))
    {
        ERR("Failed to create video device key\n");
        goto done;
    }

    /* Avoid unnecessary reinit */
    if (disposition != REG_CREATED_NEW_KEY)
        goto done;

    TRACE("via %s\n", wine_dbgstr_a(handler.name));

    prepare_devices();

    gpu_devinfo = SetupDiCreateDeviceInfoList(&GUID_DEVCLASS_DISPLAY, NULL);

    /* Initialize GPUs */
    if (!handler.pGetGpus(&gpus, &gpu_count))
        goto done;

    for (gpu = 0; gpu < gpu_count; gpu++)
    {
        if (!X11DRV_InitGpu(gpu_devinfo, &gpus[gpu], gpu))
            goto done;
    }

done:
    cleanup_devices();
    SetupDiDestroyDeviceInfoList(gpu_devinfo);
    RegCloseKey(video_hkey);
    ReleaseMutex(mutex);
    CloseHandle(mutex);
    if (gpus)
        handler.pFreeGpus(gpus);
}
