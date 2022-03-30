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
  FIXME("(%p %s 0x%08lx 0x%08lx) stub\n", pdnDevInst, pDeviceID, dnParent, ulFlags);
  return CR_SUCCESS;
}

/***********************************************************************
 *              CM_Create_DevNodeW  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Create_DevNodeW(PDEVINST pdnDevInst, DEVINSTID_W pDeviceID, DEVINST dnParent, ULONG ulFlags)
{
  FIXME("(%p %s 0x%08lx 0x%08lx) stub\n", pdnDevInst, debugstr_w(pDeviceID), dnParent, ulFlags);
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
 *             CM_Open_DevNode_Key  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Open_DevNode_Key(
    DEVINST dnDevInst, REGSAM access, ULONG ulHardwareProfile, REGDISPOSITION disposition,
    PHKEY phkDevice, ULONG ulFlags)
{
    FIXME("0x%08lx 0x%08lx 0x%08lx 0x%08lx %p 0x%08lx : stub\n", dnDevInst, access, ulHardwareProfile,
          disposition, phkDevice, ulFlags);
    return CR_SUCCESS;
}

/***********************************************************************
 *             CM_Get_Child  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Child(
    PDEVINST pdnDevInst, DEVINST dnDevInst, ULONG ulFlags)
{
    FIXME("%p 0x%08lx 0x%08lx: stub\n", pdnDevInst, dnDevInst, ulFlags);
    return CR_SUCCESS;
}

/***********************************************************************
 *             CM_Get_Child_Ex  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Child_Ex(
    PDEVINST pdnDevInst, DEVINST dnDevInst, ULONG ulFlags, HMACHINE hMachine)
{
    FIXME("%p 0x%08lx 0x%08lx %p: stub\n", pdnDevInst, dnDevInst, ulFlags, hMachine);
    return CR_SUCCESS;
}

/***********************************************************************
 *             CM_Get_Device_ID_ExA  (SETUPAPI.@)
 */
DWORD WINAPI CM_Get_Device_ID_ExA(
    DEVINST dnDevInst, PCHAR Buffer, ULONG BufferLen, ULONG ulFlags, HMACHINE hMachine)
{
    FIXME("0x%08lx %p 0x%08lx 0x%08lx %p: stub\n", dnDevInst, Buffer, BufferLen, ulFlags, hMachine);
    return CR_SUCCESS;
}

/***********************************************************************
 *             CM_Get_Device_ID_ExW  (SETUPAPI.@)
 */
DWORD WINAPI CM_Get_Device_ID_ExW(
    DEVINST dnDevInst, PWCHAR Buffer, ULONG BufferLen, ULONG ulFlags, HMACHINE hMachine)
{
    FIXME("0x%08lx %p 0x%08lx 0x%08lx %p: stub\n", dnDevInst, Buffer, BufferLen, ulFlags, hMachine);
    return CR_SUCCESS;
}

/***********************************************************************
 *             CM_Get_Device_ID_ListA  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_ID_ListA(
    PCSTR pszFilter, PCHAR Buffer, ULONG BufferLen, ULONG ulFlags )
{
    FIXME("%s %p %ld 0x%08lx: stub\n", debugstr_a(pszFilter), Buffer, BufferLen, ulFlags);

    if (BufferLen >= 2) Buffer[0] = Buffer[1] = 0;
    return CR_SUCCESS;
}

/***********************************************************************
 *             CM_Get_Device_ID_ListW  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_ID_ListW(
    PCWSTR pszFilter, PWCHAR Buffer, ULONG BufferLen, ULONG ulFlags )
{
    FIXME("%s %p %ld 0x%08lx: stub\n", debugstr_w(pszFilter), Buffer, BufferLen, ulFlags);

    if (BufferLen >= 2) Buffer[0] = Buffer[1] = 0;
    return CR_SUCCESS;
}

/***********************************************************************
 *             CM_Get_Device_ID_List_SizeA  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_ID_List_SizeA( PULONG  pulLen, PCSTR  pszFilter, ULONG  ulFlags )
{
    FIXME("%p %s 0x%08lx: stub\n", pulLen, debugstr_a(pszFilter), ulFlags);

    return CR_SUCCESS;
}

/***********************************************************************
 *             CM_Get_Device_ID_List_SizeW  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_ID_List_SizeW( PULONG  pulLen, PCWSTR  pszFilter, ULONG  ulFlags )
{
    FIXME("%p %s 0x%08lx: stub\n", pulLen, debugstr_w(pszFilter), ulFlags);

    return CR_SUCCESS;
}

/***********************************************************************
 *              CM_Get_Parent (SETUPAPI.@)
 */
DWORD WINAPI CM_Get_Parent(PDEVINST pdnDevInst, DEVINST dnDevInst, ULONG ulFlags)
{
    FIXME("%p 0x%08lx 0x%08lx stub\n", pdnDevInst, dnDevInst, ulFlags);
    if(pdnDevInst)
        *pdnDevInst = 0;
    return CR_NO_SUCH_DEVNODE;
}

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
 *      CM_Locate_DevNodeA (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Locate_DevNodeA(PDEVINST pdnDevInst, DEVINSTID_A pDeviceID, ULONG ulFlags)
{
    FIXME("%p %s 0x%08lx: stub\n", pdnDevInst, debugstr_a(pDeviceID), ulFlags);

    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Locate_DevNodeW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Locate_DevNodeW(PDEVINST pdnDevInst, DEVINSTID_W pDeviceID, ULONG ulFlags)
{
    FIXME("%p %s 0x%08lx: stub\n", pdnDevInst, debugstr_w(pDeviceID), ulFlags);

    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Locate_DevNode_ExA (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Locate_DevNode_ExA(PDEVINST pdnDevInst, DEVINSTID_A pDeviceID, ULONG ulFlags, HMACHINE hMachine)
{
    FIXME("%p %s 0x%08lx %p: stub\n", pdnDevInst, debugstr_a(pDeviceID), ulFlags, hMachine);

    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Locate_DevNode_ExW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Locate_DevNode_ExW(PDEVINST pdnDevInst, DEVINSTID_W pDeviceID, ULONG ulFlags, HMACHINE hMachine)
{
    FIXME("%p %s 0x%08lx %p: stub\n", pdnDevInst, debugstr_w(pDeviceID), ulFlags, hMachine);

    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Get_Device_Interface_List_SizeA (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_List_SizeA(PULONG len, LPGUID class, DEVINSTID_A id,
                                                    ULONG flags)
{
    FIXME("%p %p %s 0x%08lx: stub\n", len, class, debugstr_a(id), flags);
    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Get_Device_Interface_List_SizeW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_List_SizeW(PULONG len, LPGUID class, DEVINSTID_W id,
                                                    ULONG flags)
{
    FIXME("%p %p %s 0x%08lx: stub\n", len, class, debugstr_w(id), flags);
    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Get_Device_Interface_List_Size_ExA (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_List_Size_ExA(PULONG len, LPGUID class, DEVINSTID_A id,
                                                       ULONG flags, HMACHINE machine)
{
    FIXME("%p %p %s 0x%08lx %p: stub\n", len, class, debugstr_a(id), flags, machine);
    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Get_Device_Interface_List_Size_ExW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_List_Size_ExW(PULONG len, LPGUID class, DEVINSTID_W id,
                                                       ULONG flags, HMACHINE machine)
{
    FIXME("%p %p %s 0x%08lx %p: stub\n", len, class, debugstr_w(id), flags, machine);
    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Get_Device_Interface_AliasA (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_AliasA(const char *interface, GUID *class,
                                                char *name, ULONG *len, ULONG flags)
{
    FIXME("%s %p %p %p 0x%08lx: stub\n", debugstr_a(interface), class, name, len, flags);
    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Get_Device_Interface_AliasW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_AliasW(const WCHAR *interface, GUID *class,
                                                WCHAR *name, ULONG *len, ULONG flags)
{
    FIXME("%s %p %p %p 0x%08lx: stub\n", debugstr_w(interface), class, name, len, flags);
    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Get_DevNode_Registry_Property_ExA (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_DevNode_Registry_Property_ExA(DEVINST dev, ULONG prop, PULONG regdatatype,
    PVOID buf, PULONG len, ULONG flags, HMACHINE machine)
{
    FIXME("0x%08lx %lu %p %p %p 0x%08lx %p: stub\n", dev, prop, regdatatype, buf, len, flags, machine);
    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Get_DevNode_Registry_Property_ExW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_DevNode_Registry_Property_ExW(DEVINST dev, ULONG prop, PULONG regdatatype,
    PVOID buf, PULONG len, ULONG flags, HMACHINE machine)
{
    FIXME("0x%08lx %lu %p %p %p 0x%08lx %p: stub\n", dev, prop, regdatatype, buf, len, flags, machine);
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
 *      CM_Get_DevNode_Status (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_DevNode_Status(ULONG *status, ULONG *problem, DEVINST dev,
                                       ULONG flags)
{
    return CM_Get_DevNode_Status_Ex(status, problem, dev, flags, NULL);
}

/***********************************************************************
 *      CM_Get_DevNode_Status_Ex (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_DevNode_Status_Ex(ULONG *status, ULONG *problem, DEVINST dev,
                                          ULONG flags, HMACHINE machine)
{
    FIXME("%p %p 0x%08lx 0x%08lx %p: stub\n", status, problem, dev, flags, machine);
    return CR_SUCCESS;
}

/***********************************************************************
 *      CM_Enumerate_Classes (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Enumerate_Classes(ULONG index, LPGUID class, ULONG flags)
{
    FIXME("%lu %p 0x%08lx: stub\n", index, class, flags);
    return CR_NO_SUCH_VALUE;
}

/***********************************************************************
 *      CM_Get_Class_Registry_PropertyA (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Class_Registry_PropertyA(LPGUID class, ULONG prop, PULONG regdatatype,
                                                 PVOID buf, ULONG len, ULONG flags, HMACHINE machine)
{
    FIXME("%p %lu %p %p %lu 0x%08lx %p: stub\n", class, prop, regdatatype, buf, len, flags, machine);
    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Get_Class_Registry_PropertyW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Class_Registry_PropertyW(LPGUID class, ULONG prop, PULONG regdatatype,
                                                 PVOID buf, ULONG len, ULONG flags, HMACHINE machine)
{
    FIXME("%p %lu %p %p %lu 0x%08lx %p: stub\n", class, prop, regdatatype, buf, len, flags, machine);
    return CR_FAILURE;
}

/***********************************************************************
 *             CM_Get_Sibling  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Sibling(
    PDEVINST pdnDevInst, DEVINST DevInst, ULONG ulFlags)
{
    FIXME("%p 0x%08lx 0x%08lx: stub\n", pdnDevInst, DevInst, ulFlags);
    return CR_FAILURE;
}

/***********************************************************************
 *             CM_Get_Sibling_Ex  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Sibling_Ex(
    PDEVINST pdnDevInst, DEVINST DevInst, ULONG ulFlags, HMACHINE hMachine)
{
    FIXME("%p 0x%08lx 0x%08lx %p: stub\n", pdnDevInst, DevInst, ulFlags, hMachine);
    return CR_FAILURE;
}

CONFIGRET WINAPI CM_Reenumerate_DevNode(DEVINST dnDevInst, ULONG ulFlags)
{
    FIXME("0x%08lx 0x%08lx: stub\n", dnDevInst, ulFlags);
    return CR_FAILURE;
}

CONFIGRET WINAPI CM_Reenumerate_DevNode_Ex(DEVINST dnDevInst, ULONG ulFlags, HMACHINE machine)
{
    FIXME("0x%08lx 0x%08lx %p: stub\n", dnDevInst, ulFlags, machine);
    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Set_Class_Registry_PropertyA (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Set_Class_Registry_PropertyA(LPGUID class, ULONG prop, LPCVOID buf, ULONG len,
                                                 ULONG flags, HMACHINE machine)
{
    FIXME("%p %lu %p %lu 0x%08lx %p: stub\n", class, prop, buf, len, flags, machine);
    return CR_FAILURE;
}

/***********************************************************************
 *      CM_Set_Class_Registry_PropertyW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Set_Class_Registry_PropertyW(LPGUID class, ULONG prop, LPCVOID buf, ULONG len,
                                                 ULONG flags, HMACHINE machine)
{
    FIXME("%p %lu %p %lu 0x%08lx %p: stub\n", class, prop, buf, len, flags, machine);
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

/***********************************************************************
 *              CM_Request_Device_EjectA  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Request_Device_EjectA(DEVINST dev, PPNP_VETO_TYPE type, LPSTR name, ULONG length, ULONG flags)
{
    FIXME("(0x%08lx, %p, %p, %lu, 0x%08lx) stub\n", dev, type, name, length, flags);
    return CR_SUCCESS;
}

/***********************************************************************
 *              CM_Request_Device_EjectW  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Request_Device_EjectW(DEVINST dev, PPNP_VETO_TYPE type, LPWSTR name, ULONG length, ULONG flags)
{
    FIXME("(0x%08lx, %p, %p, %lu, 0x%08lx) stub\n", dev, type, name, length, flags);
    return CR_SUCCESS;
}

BOOL WINAPI SetupDiGetClassRegistryPropertyW(const GUID *class, DWORD prop, DWORD *datatype, BYTE *buff, DWORD size,
    DWORD *req_size, const WCHAR *name, VOID *reserved)
{
    FIXME("class %s, prop %ld, datatype %p, buff %p, size %ld, req_size %p, name %s, reserved %p stub!\n",
        debugstr_guid(class), prop, datatype, buff, size, req_size, debugstr_w(name), reserved);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
