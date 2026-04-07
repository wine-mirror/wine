/*
 * SetupAPI stubs
 *
 * Copyright 2000 James Hatheway
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

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "cfgmgr32.h"
#include "setupapi.h"
#include "winnls.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

/***********************************************************************
 *		SetupInitializeFileLogW(SETUPAPI.@)
 */
HSPFILELOG WINAPI SetupInitializeFileLogW(LPCWSTR LogFileName, DWORD Flags)
{
    FIXME("Stub %s, 0x%lx\n",debugstr_w(LogFileName),Flags);
    return INVALID_HANDLE_VALUE;
}

/***********************************************************************
 *		SetupInitializeFileLogA(SETUPAPI.@)
 */
HSPFILELOG WINAPI SetupInitializeFileLogA(LPCSTR LogFileName, DWORD Flags)
{
    FIXME("Stub %s, 0x%lx\n",debugstr_a(LogFileName),Flags);
    return INVALID_HANDLE_VALUE;
}

/***********************************************************************
 *		SetupTerminateFileLog(SETUPAPI.@)
 */
BOOL WINAPI SetupTerminateFileLog(HANDLE FileLogHandle)
{
    FIXME ("Stub %p\n",FileLogHandle);
    return TRUE;
}

/***********************************************************************
 *		RegistryDelnode(SETUPAPI.@)
 */
BOOL WINAPI RegistryDelnode(DWORD x, DWORD y)
{
    FIXME("%08lx %08lx: stub\n", x, y);
    return FALSE;
}

/***********************************************************************
 *      SetupPromptReboot(SETUPAPI.@)
 */
INT WINAPI SetupPromptReboot( HSPFILEQ file_queue, HWND owner, BOOL scan_only )
{
    FIXME("%p, %p, %d: stub\n", file_queue, owner, scan_only);
    return 0;
}

/***********************************************************************
 *      SetupQueryDrivesInDiskSpaceListA (SETUPAPI.@)
 */
BOOL WINAPI SetupQueryDrivesInDiskSpaceListA(HDSKSPC disk_space, PSTR return_buffer, DWORD return_buffer_size, PDWORD required_size)
{
    FIXME("%p, %p, %ld, %p: stub\n", disk_space, return_buffer, return_buffer_size, required_size);
    return FALSE;
}

/***********************************************************************
 *      SetupQueryDrivesInDiskSpaceListW (SETUPAPI.@)
 */
BOOL WINAPI SetupQueryDrivesInDiskSpaceListW(HDSKSPC disk_space, PWSTR return_buffer, DWORD return_buffer_size, PDWORD required_size)
{
    FIXME("%p, %p, %ld, %p: stub\n", disk_space, return_buffer, return_buffer_size, required_size);
    return FALSE;
}

/***********************************************************************
 *      SetupAddToSourceListA (SETUPAPI.@)
 */
BOOL WINAPI SetupAddToSourceListA(DWORD flags, PCSTR source)
{
    FIXME("0x%08lx %s: stub\n", flags, debugstr_a(source));
    return TRUE;
}

/***********************************************************************
 *      SetupAddToSourceListW (SETUPAPI.@)
 */
BOOL WINAPI SetupAddToSourceListW(DWORD flags, PCWSTR source)
{
    FIXME("0x%08lx %s: stub\n", flags, debugstr_w(source));
    return TRUE;
}

/***********************************************************************
 *      SetupSetSourceListA (SETUPAPI.@)
 */
BOOL WINAPI SetupSetSourceListA(DWORD flags, PCSTR *list, UINT count)
{
    FIXME("0x%08lx %p %d: stub\n", flags, list, count);
    return FALSE;
}

/***********************************************************************
 *      SetupSetSourceListW (SETUPAPI.@)
 */
BOOL WINAPI SetupSetSourceListW(DWORD flags, PCWSTR *list, UINT count)
{
    FIXME("0x%08lx %p %d: stub\n", flags, list, count);
    return FALSE;
}

/***********************************************************************
 *      SetupDiDestroyClassImageList (SETUPAPI.@)
 */
BOOL WINAPI SetupDiDestroyClassImageList(PSP_CLASSIMAGELIST_DATA ClassListImageData)
{
    FIXME("(%p) stub\n", ClassListImageData);
    return TRUE;
}

/***********************************************************************
 *      SetupDiGetClassImageList (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetClassImageList(PSP_CLASSIMAGELIST_DATA ClassImageListData)
{
    FIXME("(%p) stub\n", ClassImageListData);
    return FALSE;
}

/***********************************************************************
 *      SetupDiGetClassImageList (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetClassImageIndex(PSP_CLASSIMAGELIST_DATA ClassImageListData, const GUID *class, PINT index)
{
    FIXME("%p %p %p\n", ClassImageListData, class, index);
    return FALSE;
}

/***********************************************************************
 *              SetupLogFileW  (SETUPAPI.@)
 */
BOOL WINAPI SetupLogFileW(
    HSPFILELOG FileLogHandle,
    PCWSTR LogSectionName,
    PCWSTR SourceFileName,
    PCWSTR TargetFileName,
    DWORD Checksum,
    PCWSTR DiskTagfile,
    PCWSTR DiskDescription,
    PCWSTR OtherInfo,
    DWORD Flags )
{
    FIXME("(%p, %s, '%s', '%s', %ld, %s, %s, %s, %ld): stub\n", FileLogHandle,
        debugstr_w(LogSectionName), debugstr_w(SourceFileName),
        debugstr_w(TargetFileName), Checksum, debugstr_w(DiskTagfile),
        debugstr_w(DiskDescription), debugstr_w(OtherInfo), Flags);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *              SetupLogFileA  (SETUPAPI.@)
 */
BOOL WINAPI SetupLogFileA(
    HSPFILELOG FileLogHandle,
    PCSTR LogSectionName,
    PCSTR SourceFileName,
    PCSTR TargetFileName,
    DWORD Checksum,
    PCSTR DiskTagfile,
    PCSTR DiskDescription,
    PCSTR OtherInfo,
    DWORD Flags )
{
    FIXME("(%p, %p, '%s', '%s', %ld, %p, %p, %p, %ld): stub\n", FileLogHandle,
        LogSectionName, SourceFileName, TargetFileName, Checksum, DiskTagfile,
        DiskDescription, OtherInfo, Flags);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *              SetupDiDestroyDriverInfoList  (SETUPAPI.@)
 */

BOOL WINAPI SetupDiDestroyDriverInfoList(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, DWORD DriverType)
{
    FIXME("%p %p %ld\n", DeviceInfoSet, DeviceInfoData, DriverType);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *              SetupDiDrawMiniIcon  (SETUPAPI.@)
 */
INT WINAPI SetupDiDrawMiniIcon(HDC hdc, RECT rc, INT MiniIconIndex, DWORD Flags)
{
    FIXME("(%p, %s, %d, %lx) stub\n", hdc, wine_dbgstr_rect(&rc), MiniIconIndex, Flags);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/***********************************************************************
 *              SetupDiGetClassBitmapIndex  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetClassBitmapIndex(const GUID *ClassGuid, PINT MiniIconIndex)
{
    FIXME("(%s, %p) stub\n", debugstr_guid(ClassGuid), MiniIconIndex);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *              SetupDiLoadClassIcon  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiLoadClassIcon(const GUID *ClassGuid, HICON *LargeIcon, PINT MiniIconIndex)
{
    FIXME(": stub %s, %p, %p\n", debugstr_guid(ClassGuid), LargeIcon, MiniIconIndex);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *              SetupDiSetSelectedDevice  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiSetSelectedDevice(HDEVINFO SetupDiSetSelectedDevice, PSP_DEVINFO_DATA DeviceInfoData)
{
    FIXME("(%p, %p) stub\n", SetupDiSetSelectedDevice, DeviceInfoData);

    return TRUE;
}

BOOL WINAPI SetupDiGetClassRegistryPropertyW(const GUID *class, DWORD prop, DWORD *datatype, BYTE *buff, DWORD size,
    DWORD *req_size, const WCHAR *name, VOID *reserved)
{
    FIXME("class %s, prop %ld, datatype %p, buff %p, size %ld, req_size %p, name %s, reserved %p stub!\n",
        debugstr_guid(class), prop, datatype, buff, size, req_size, debugstr_w(name), reserved);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
