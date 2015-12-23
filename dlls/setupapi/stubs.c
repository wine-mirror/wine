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
 *              CM_Connect_MachineA  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Connect_MachineA(PCSTR name, PHMACHINE machine)
{
  FIXME("(%s %p) stub\n", name, machine);
  return CR_ACCESS_DENIED;
}

/***********************************************************************
 *		CM_Connect_MachineW  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Connect_MachineW(PCWSTR name, PHMACHINE machine)
{
  FIXME("stub\n");
  return  CR_ACCESS_DENIED;
}

/***********************************************************************
 *              CM_Create_DevNodeA  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Create_DevNodeA(PDEVINST pdnDevInst, DEVINSTID_A pDeviceID, DEVINST dnParent, ULONG ulFlags)
{
  FIXME("(%p %s 0x%08x 0x%08x) stub\n", pdnDevInst, pDeviceID, dnParent, ulFlags);
  return CR_SUCCESS;
}

/***********************************************************************
 *              CM_Create_DevNodeW  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Create_DevNodeW(PDEVINST pdnDevInst, DEVINSTID_W pDeviceID, DEVINST dnParent, ULONG ulFlags)
{
  FIXME("(%p %s 0x%08x 0x%08x) stub\n", pdnDevInst, debugstr_w(pDeviceID), dnParent, ulFlags);
  return CR_SUCCESS;
}

/***********************************************************************
 *		CM_Disconnect_Machine  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Disconnect_Machine(HMACHINE handle)
{
  FIXME("stub\n");
  return  CR_SUCCESS;

}

/***********************************************************************
 *             CM_Get_Device_ID_ExA  (SETUPAPI.@)
 */
DWORD WINAPI CM_Get_Device_ID_ExA(
    DEVINST dnDevInst, PCHAR Buffer, ULONG BufferLen, ULONG ulFlags, HMACHINE hMachine)
{
    FIXME("0x%08x %p 0x%08x 0x%08x %p: stub\n", dnDevInst, Buffer, BufferLen, ulFlags, hMachine);
    return CR_SUCCESS;
}

/***********************************************************************
 *             CM_Get_Device_ID_ExW  (SETUPAPI.@)
 */
DWORD WINAPI CM_Get_Device_ID_ExW(
    DEVINST dnDevInst, PWCHAR Buffer, ULONG BufferLen, ULONG ulFlags, HMACHINE hMachine)
{
    FIXME("0x%08x %p 0x%08x 0x%08x %p: stub\n", dnDevInst, Buffer, BufferLen, ulFlags, hMachine);
    return CR_SUCCESS;
}

/***********************************************************************
 *             CM_Get_Device_ID_ListA  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_ID_ListA(
    PCSTR pszFilter, PCHAR Buffer, ULONG BufferLen, ULONG ulFlags )
{
    FIXME("%s %p %d 0x%08x: stub\n", debugstr_a(pszFilter), Buffer, BufferLen, ulFlags);

    if (BufferLen >= 2) Buffer[0] = Buffer[1] = 0;
    return CR_SUCCESS;
}

/***********************************************************************
 *             CM_Get_Device_ID_ListW  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_ID_ListW(
    PCWSTR pszFilter, PWCHAR Buffer, ULONG BufferLen, ULONG ulFlags )
{
    FIXME("%s %p %d 0x%08x: stub\n", debugstr_w(pszFilter), Buffer, BufferLen, ulFlags);

    if (BufferLen >= 2) Buffer[0] = Buffer[1] = 0;
    return CR_SUCCESS;
}

/***********************************************************************
 *             CM_Get_Device_ID_List_SizeA  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_ID_List_SizeA( PULONG  pulLen, PCSTR  pszFilter, ULONG  ulFlags )
{
    FIXME("%p %s 0x%08x: stub\n", pulLen, debugstr_a(pszFilter), ulFlags);

    return CR_SUCCESS;
}

/***********************************************************************
 *             CM_Get_Device_ID_List_SizeW  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_ID_List_SizeW( PULONG  pulLen, PCWSTR  pszFilter, ULONG  ulFlags )
{
    FIXME("%p %s 0x%08x: stub\n", pulLen, debugstr_w(pszFilter), ulFlags);

    return CR_SUCCESS;
}

/***********************************************************************
 *              CM_Get_Parent (SETUPAPI.@)
 */
DWORD WINAPI CM_Get_Parent(PDEVINST pdnDevInst, DEVINST dnDevInst, ULONG ulFlags)
{
    FIXME("%p 0x%08x 0x%08x stub\n", pdnDevInst, dnDevInst, ulFlags);
    *pdnDevInst = dnDevInst;
    return CR_SUCCESS;
}

/***********************************************************************
 *		SetupInitializeFileLogW(SETUPAPI.@)
 */
HSPFILELOG WINAPI SetupInitializeFileLogW(LPCWSTR LogFileName, DWORD Flags)
{
    FIXME("Stub %s, 0x%x\n",debugstr_w(LogFileName),Flags);
    return INVALID_HANDLE_VALUE;
}

/***********************************************************************
 *		SetupInitializeFileLogA(SETUPAPI.@)
 */
HSPFILELOG WINAPI SetupInitializeFileLogA(LPCSTR LogFileName, DWORD Flags)
{
    FIXME("Stub %s, 0x%x\n",debugstr_a(LogFileName),Flags);
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
    FIXME("%08x %08x: stub\n", x, y);
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
    FIXME("%p, %p, %d, %p: stub\n", disk_space, return_buffer, return_buffer_size, required_size);
    return FALSE;
}

/***********************************************************************
 *      SetupQueryDrivesInDiskSpaceListW (SETUPAPI.@)
 */
BOOL WINAPI SetupQueryDrivesInDiskSpaceListW(HDSKSPC disk_space, PWSTR return_buffer, DWORD return_buffer_size, PDWORD required_size)
{
    FIXME("%p, %p, %d, %p: stub\n", disk_space, return_buffer, return_buffer_size, required_size);
    return FALSE;
}

/***********************************************************************
 *      SetupAddToSourceListA (SETUPAPI.@)
 */
BOOL WINAPI SetupAddToSourceListA(DWORD flags, PCSTR source)
{
    FIXME("0x%08x %s: stub\n", flags, debugstr_a(source));
    return TRUE;
}

/***********************************************************************
 *      SetupAddToSourceListW (SETUPAPI.@)
 */
BOOL WINAPI SetupAddToSourceListW(DWORD flags, PCWSTR source)
{
    FIXME("0x%08x %s: stub\n", flags, debugstr_w(source));
    return TRUE;
}

/***********************************************************************
 *      SetupSetSourceListA (SETUPAPI.@)
 */
BOOL WINAPI SetupSetSourceListA(DWORD flags, PCSTR *list, UINT count)
{
    FIXME("0x%08x %p %d: stub\n", flags, list, count);
    return FALSE;
}

/***********************************************************************
 *      SetupSetSourceListW (SETUPAPI.@)
 */
BOOL WINAPI SetupSetSourceListW(DWORD flags, PCWSTR *list, UINT count)
{
    FIXME("0x%08x %p %d: stub\n", flags, list, count);
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
 *      SetupDiOpenDeviceInfoA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiOpenDeviceInfoA(HDEVINFO DeviceInfoSet, PCSTR DeviceInstanceId,
        HWND hwndParent, DWORD OpenFlags, PSP_DEVINFO_DATA DeviceInfoData)
{
    FIXME("%p %s %p 0x%08x %p: stub\n", DeviceInfoSet, debugstr_a(DeviceInstanceId),
          hwndParent, OpenFlags, DeviceInfoData);
    return FALSE;
}

/***********************************************************************
 *      SetupDiOpenDeviceInfoW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiOpenDeviceInfoW(HDEVINFO DeviceInfoSet, PCWSTR DeviceInstanceId,
        HWND hwndParent, DWORD OpenFlags, PSP_DEVINFO_DATA DeviceInfoData)
{
    FIXME("%p %s %p 0x%08x %p: stub\n", DeviceInfoSet, debugstr_w(DeviceInstanceId),
          hwndParent, OpenFlags, DeviceInfoData);
    return FALSE;
}

/***********************************************************************
 *      CM_Locate_DevNodeA (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Locate_DevNodeA(PDEVINST pdnDevInst, DEVINSTID_A pDeviceID, ULONG ulFlags)
{
    FIXME("%p %s 0x%08x: stub\n", pdnDevInst, debugstr_a(pDeviceID), ulFlags);

    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Locate_DevNodeW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Locate_DevNodeW(PDEVINST pdnDevInst, DEVINSTID_W pDeviceID, ULONG ulFlags)
{
    FIXME("%p %s 0x%08x: stub\n", pdnDevInst, debugstr_w(pDeviceID), ulFlags);

    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Locate_DevNode_ExA (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Locate_DevNode_ExA(PDEVINST pdnDevInst, DEVINSTID_A pDeviceID, ULONG ulFlags, HMACHINE hMachine)
{
    FIXME("%p %s 0x%08x %p: stub\n", pdnDevInst, debugstr_a(pDeviceID), ulFlags, hMachine);

    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Locate_DevNode_ExW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Locate_DevNode_ExW(PDEVINST pdnDevInst, DEVINSTID_W pDeviceID, ULONG ulFlags, HMACHINE hMachine)
{
    FIXME("%p %s 0x%08x %p: stub\n", pdnDevInst, debugstr_w(pDeviceID), ulFlags, hMachine);

    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Get_Device_Interface_List_SizeA (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_List_SizeA(PULONG len, LPGUID class, DEVINSTID_A id,
                                                    ULONG flags)
{
    FIXME("%p %p %s 0x%08x: stub\n", len, class, debugstr_a(id), flags);
    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Get_Device_Interface_List_SizeW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_List_SizeW(PULONG len, LPGUID class, DEVINSTID_W id,
                                                    ULONG flags)
{
    FIXME("%p %p %s 0x%08x: stub\n", len, class, debugstr_w(id), flags);
    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Get_Device_Interface_List_Size_ExA (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_List_Size_ExA(PULONG len, LPGUID class, DEVINSTID_A id,
                                                       ULONG flags, HMACHINE machine)
{
    FIXME("%p %p %s 0x%08x %p: stub\n", len, class, debugstr_a(id), flags, machine);
    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Get_Device_Interface_List_Size_ExW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_List_Size_ExW(PULONG len, LPGUID class, DEVINSTID_W id,
                                                       ULONG flags, HMACHINE machine)
{
    FIXME("%p %p %s 0x%08x %p: stub\n", len, class, debugstr_w(id), flags, machine);
    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Get_DevNode_Registry_Property_ExA (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_DevNode_Registry_Property_ExA(DEVINST dev, ULONG prop, PULONG regdatatype,
    PVOID buf, PULONG len, ULONG flags, HMACHINE machine)
{
    FIXME("0x%08x %u %p %p %p 0x%08x %p: stub\n", dev, prop, regdatatype, buf, len, flags, machine);
    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Get_DevNode_Registry_Property_ExW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_DevNode_Registry_Property_ExW(DEVINST dev, ULONG prop, PULONG regdatatype,
    PVOID buf, PULONG len, ULONG flags, HMACHINE machine)
{
    FIXME("0x%08x %u %p %p %p 0x%08x %p: stub\n", dev, prop, regdatatype, buf, len, flags, machine);
    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Get_DevNode_Registry_PropertyA (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_DevNode_Registry_PropertyA(DEVINST dev, ULONG prop, PULONG regdatatype,
    PVOID buf, PULONG len, ULONG flags)
{
    return CM_Get_DevNode_Registry_Property_ExA(dev, prop, regdatatype, buf, len, flags, NULL);
}

/***********************************************************************
 *      CM_Get_DevNode_Registry_PropertyW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_DevNode_Registry_PropertyW(DEVINST dev, ULONG prop, PULONG regdatatype,
    PVOID buf, PULONG len, ULONG flags)
{
    return CM_Get_DevNode_Registry_Property_ExW(dev, prop, regdatatype, buf, len, flags, NULL);
}

/***********************************************************************
 *      CM_Enumerate_Classes (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Enumerate_Classes(ULONG index, LPGUID class, ULONG flags)
{
    FIXME("%u %p 0x%08x: stub\n", index, class, flags);
    return CR_NO_SUCH_VALUE;
}

/***********************************************************************
 *      CM_Get_Class_Registry_PropertyA (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Class_Registry_PropertyA(LPGUID class, ULONG prop, PULONG regdatatype,
                                                 PVOID buf, ULONG len, ULONG flags, HMACHINE machine)
{
    FIXME("%p %u %p %p %u 0x%08x %p: stub\n", class, prop, regdatatype, buf, len, flags, machine);
    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Get_Class_Registry_PropertyW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Class_Registry_PropertyW(LPGUID class, ULONG prop, PULONG regdatatype,
                                                 PVOID buf, ULONG len, ULONG flags, HMACHINE machine)
{
    FIXME("%p %u %p %p %u 0x%08x %p: stub\n", class, prop, regdatatype, buf, len, flags, machine);
    return CR_FAILURE;
}

CONFIGRET WINAPI CM_Reenumerate_DevNode(DEVINST dnDevInst, ULONG ulFlags)
{
    FIXME("0x%08x 0x%08x: stub\n", dnDevInst, ulFlags);
    return CR_FAILURE;
}

CONFIGRET WINAPI CM_Reenumerate_DevNode_Ex(DEVINST dnDevInst, ULONG ulFlags, HMACHINE machine)
{
    FIXME("0x%08x 0x%08x %p: stub\n", dnDevInst, ulFlags, machine);
    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Set_Class_Registry_PropertyA (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Set_Class_Registry_PropertyA(LPGUID class, ULONG prop, PVOID buf, ULONG len,
                                                 ULONG flags, HMACHINE machine)
{
    FIXME("%p %u %p %u 0x%08x %p: stub\n", class, prop, buf, len, flags, machine);
    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Set_Class_Registry_PropertyW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Set_Class_Registry_PropertyW(LPGUID class, ULONG prop, PVOID buf, ULONG len,
                                                 ULONG flags, HMACHINE machine)
{
    FIXME("%p %u %p %u 0x%08x %p: stub\n", class, prop, buf, len, flags, machine);
    return CR_FAILURE;
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
    FIXME("(%p, %s, '%s', '%s', %d, %s, %s, %s, %d): stub\n", FileLogHandle,
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
    FIXME("(%p, %p, '%s', '%s', %d, %p, %p, %p, %d): stub\n", FileLogHandle,
        LogSectionName, SourceFileName, TargetFileName, Checksum, DiskTagfile,
        DiskDescription, OtherInfo, Flags);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *              SetupDiBuildDriverInfoList  (SETUPAPI.@)
 */

BOOL WINAPI SetupDiBuildDriverInfoList(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, DWORD DriverType)
 {
    FIXME(": stub %p, %p, %d\n", DeviceInfoSet, DeviceInfoData, DriverType);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
 }

/***********************************************************************
 *              SetupDiDestroyDriverInfoList  (SETUPAPI.@)
 */

BOOL WINAPI SetupDiDestroyDriverInfoList(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, DWORD DriverType)
{
    FIXME("%p %p %d\n", DeviceInfoSet, DeviceInfoData, DriverType);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *              SetupDiDeleteDeviceInfo  (SETUPAPI.@)
 */

BOOL WINAPI SetupDiDeleteDeviceInfo(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData)
 {
    FIXME(": stub %p, %p\n", DeviceInfoSet, DeviceInfoData);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
 }

/***********************************************************************
 *              SetupDiDrawMiniIcon  (SETUPAPI.@)
 */
INT WINAPI SetupDiDrawMiniIcon(HDC hdc, RECT rc, INT MiniIconIndex, DWORD Flags)
{
    FIXME("(%p, %s, %d, %x) stub\n", hdc, wine_dbgstr_rect(&rc), MiniIconIndex, Flags);

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
 *              SetupDiSelectBestCompatDrv (SETUPAPI.@)
 */
BOOL WINAPI SetupDiSelectBestCompatDrv(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData)
{
    FIXME(": stub %p, %p\n", DeviceInfoSet, DeviceInfoData);

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
