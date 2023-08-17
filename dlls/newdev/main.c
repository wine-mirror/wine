/*
 * New Device installation API
 *
 * Copyright 2003 Ulrich Czekalla
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
#include "winerror.h"
#include "winbase.h"
#include "winnls.h"
#include "winuser.h"
#include "winreg.h"
#include "cfgmgr32.h"
#include "newdev.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

/***********************************************************************
 *           InstallNewDevice (NEWDEV.@)
 */
BOOL WINAPI InstallNewDevice(HWND hwndParent, LPGUID ClassGuid, PDWORD pReboot)
{
    FIXME("Stub!\n");
    return TRUE;
}

/***********************************************************************
 *           InstallSelectedDriver (NEWDEV.@)
 */
BOOL WINAPI InstallSelectedDriver(HWND parent, HDEVINFO info, const WCHAR *reserved, BOOL backup, DWORD *reboot)
{
    FIXME("Stub! %p %p %s %u %p\n", parent, info, debugstr_w(reserved), backup, reboot);
    return TRUE;
}

/***********************************************************************
 *           UpdateDriverForPlugAndPlayDevicesA (NEWDEV.@)
 */
BOOL WINAPI UpdateDriverForPlugAndPlayDevicesA(HWND parent, const char *hardware_id,
        const char *inf_path, DWORD flags, BOOL *reboot)
{
    WCHAR hardware_idW[MAX_DEVICE_ID_LEN];
    WCHAR inf_pathW[MAX_PATH];

    MultiByteToWideChar(CP_ACP, 0, hardware_id, -1, hardware_idW, ARRAY_SIZE(hardware_idW));
    MultiByteToWideChar(CP_ACP, 0, inf_path, -1, inf_pathW, ARRAY_SIZE(inf_pathW));

    return UpdateDriverForPlugAndPlayDevicesW(parent, hardware_idW, inf_pathW, flags, reboot);
}

static BOOL hardware_id_matches(const WCHAR *id, const WCHAR *device_ids)
{
    while (*device_ids)
    {
        if (!wcscmp(id, device_ids))
            return TRUE;
        device_ids += lstrlenW(device_ids) + 1;
    }
    return FALSE;
}

/***********************************************************************
 *           UpdateDriverForPlugAndPlayDevicesW (NEWDEV.@)
 */
BOOL WINAPI UpdateDriverForPlugAndPlayDevicesW(HWND parent, const WCHAR *hardware_id,
        const WCHAR *inf_path, DWORD flags, BOOL *reboot)
{
    SP_DEVINSTALL_PARAMS_W params = {sizeof(params)};
    SP_DEVINFO_DATA device = {sizeof(device)};
    WCHAR *device_ids = NULL;
    DWORD size = 0, i, j;
    HDEVINFO set;

    static const DWORD dif_list[] =
    {
        DIF_SELECTBESTCOMPATDRV,
        DIF_ALLOW_INSTALL,
        DIF_INSTALLDEVICEFILES,
        DIF_REGISTER_COINSTALLERS,
        DIF_INSTALLINTERFACES,
        DIF_INSTALLDEVICE,
        DIF_NEWDEVICEWIZARD_FINISHINSTALL,
    };

    TRACE("parent %p, hardware_id %s, inf_path %s, flags %#lx, reboot %p.\n",
            parent, debugstr_w(hardware_id), debugstr_w(inf_path), flags, reboot);

    if (flags)
        FIXME("Unhandled flags %#lx.\n", flags);

    if (reboot) *reboot = FALSE;

    if ((set = SetupDiGetClassDevsW(NULL, NULL, 0, DIGCF_ALLCLASSES)) == INVALID_HANDLE_VALUE)
        return FALSE;

    for (i = 0; SetupDiEnumDeviceInfo(set, i, &device); ++i)
    {
        if (!SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_HARDWAREID, NULL, (BYTE *)device_ids, size, &size))
        {
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                continue;
            device_ids = realloc(device_ids, size);
            SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_HARDWAREID, NULL, (BYTE *)device_ids, size, NULL);
        }

        if (!hardware_id_matches(hardware_id, device_ids))
            continue;

        if (!SetupDiGetDeviceInstallParamsW(set, &device, &params))
            continue;

        lstrcpyW(params.DriverPath, inf_path);
        params.Flags |= DI_ENUMSINGLEINF;
        if (!SetupDiSetDeviceInstallParamsW(set, &device, &params))
            continue;

        if (!SetupDiBuildDriverInfoList(set, &device, SPDIT_COMPATDRIVER))
            continue;

        for (j = 0; j < ARRAY_SIZE(dif_list); ++j)
        {
            if (!SetupDiCallClassInstaller(dif_list[j], set, &device) && GetLastError() != ERROR_DI_DO_DEFAULT)
                break;
        }
    }

    SetupDiDestroyDeviceInfoList(set);
    free(device_ids);
    return TRUE;
}


/***********************************************************************
 *           DiInstallDriverA (NEWDEV.@)
 */
BOOL WINAPI DiInstallDriverA(HWND parent, const char *inf_path, DWORD flags, BOOL *reboot)
{
    FIXME("parent %p, inf_path %s, flags %#lx, reboot %p, stub!\n", parent, debugstr_a(inf_path), flags, reboot);
    return TRUE;
}

/***********************************************************************
 *           DiInstallDriverW (NEWDEV.@)
 */
BOOL WINAPI DiInstallDriverW(HWND parent, const WCHAR *inf_path, DWORD flags, BOOL *reboot)
{
    FIXME("parent %p, inf_path %s, flags %#lx, reboot %p, stub!\n", parent, debugstr_w(inf_path), flags, reboot);
    return TRUE;
}
