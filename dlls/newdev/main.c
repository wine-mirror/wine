/*
 * NewDev
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
#include "winuser.h"
#include "winreg.h"
#include "setupapi.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(newdev);

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
BOOL WINAPI UpdateDriverForPlugAndPlayDevicesA(HWND hwndParent, LPCSTR HardwareId,
    LPCSTR FullInfPath, DWORD InstallFlags, PBOOL bRebootRequired OPTIONAL)
{
    FIXME("Stub! %s %s 0x%08x\n", HardwareId, FullInfPath, InstallFlags);
    return TRUE;
}


/***********************************************************************
 *           UpdateDriverForPlugAndPlayDevicesW (NEWDEV.@)
 */
BOOL WINAPI UpdateDriverForPlugAndPlayDevicesW(HWND hwndParent, LPCWSTR HardwareId,
    LPCWSTR FullInfPath, DWORD InstallFlags, PBOOL bRebootRequired OPTIONAL)
{
    FIXME("Stub! %s %s 0x%08x\n", debugstr_w(HardwareId), debugstr_w(FullInfPath), InstallFlags);
    return TRUE;
}


/***********************************************************************
 *           DiInstallDriverA (NEWDEV.@)
 */
BOOL WINAPI DiInstallDriverA(HWND parent, HDEVINFO deviceinfo, PSP_DEVINFO_DATA devicedata,
    PSP_DRVINFO_DATA_A driverdata, DWORD flags, BOOL *reboot)
{
    FIXME("Stub! %p %p %p %p 0x%08x %p\n", parent, deviceinfo, devicedata, driverdata, flags, reboot);
    return TRUE;
}

/***********************************************************************
 *           DiInstallDriverW (NEWDEV.@)
 */
BOOL WINAPI DiInstallDriverW(HWND parent, HDEVINFO deviceinfo, PSP_DEVINFO_DATA devicedata,
    PSP_DRVINFO_DATA_W driverdata, DWORD flags, BOOL *reboot)
{
    FIXME("Stub! %p %p %p %p 0x%08x %p\n", parent, deviceinfo, devicedata, driverdata, flags, reboot);
    return TRUE;
}
