/*
 * SetupAPI device installer
 *
 * Copyright 2000 Andreas Mohr for Codeweavers
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "windef.h"
#include "winbase.h"
#include "setupx16.h"
#include "wine/debug.h"
#include "setupapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

/***********************************************************************
 *		DiGetClassDevs (SETUPX.304)
 * Return a list of installed system devices.
 * Uses HKLM\\ENUM to list devices.
 */
RETERR16 WINAPI DiGetClassDevs16(LPLPDEVICE_INFO16 lplpdi,
                                 LPCSTR lpszClassName, HWND16 hwndParent, INT16 iFlags)
{
    LPDEVICE_INFO16 lpdi;

    FIXME("(%p, '%s', %04x, %04x), semi-stub.\n",
          lplpdi, lpszClassName, hwndParent, iFlags);
    lpdi = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DEVICE_INFO16));
    lpdi->cbSize = sizeof(DEVICE_INFO16);
    *lplpdi = (LPDEVICE_INFO16)MapLS(lpdi);
    return OK;
}

/***********************************************************************
 *		DiBuildCompatDrvList (SETUPX.300)
 */
RETERR16 WINAPI DiBuildCompatDrvList16(LPDEVICE_INFO16 lpdi)
{
    FIXME("(%p): stub\n", lpdi);
    lpdi->lpCompatDrvList = NULL;
    return FALSE;
}

/***********************************************************************
 *		DiCallClassInstaller (SETUPX.308)
 */
RETERR16 WINAPI DiCallClassInstaller16(/*DI_FUNCTIONS*/WORD diFctn, LPDEVICE_INFO16 lpdi)
{
    FIXME("(%x, %p): stub\n", diFctn, lpdi);
    return FALSE;
}

/***********************************************************************
 *		DiCreateDevRegKey (SETUPX.318)
 */
RETERR16 WINAPI DiCreateDevRegKey16(LPDEVICE_INFO16 lpdi,
                                    VOID* p2, WORD w3,
                                    LPCSTR s4, WORD w5)
{
    FIXME("(%p, %p, %x, %s, %x): stub\n", lpdi, p2, w3, debugstr_a(s4), w5);
    return FALSE;
}

/***********************************************************************
 *		DiDeleteDevRegKey (SETUPX.344)
 */
RETERR16 WINAPI DiDeleteDevRegKey16(LPDEVICE_INFO16 lpdi, INT16 iFlags)
{
    FIXME("(%p, %x): stub\n", lpdi, iFlags);
    return FALSE;
}

/***********************************************************************
 *		DiCreateDeviceInfo (SETUPX.303)
 */
RETERR16 WINAPI DiCreateDeviceInfo16(LPLPDEVICE_INFO16 lplpdi,
                                     LPCSTR lpszDescription, DWORD dnDevnode,
                                     HKEY16 hkey, LPCSTR lpszRegsubkey,
                                     LPCSTR lpszClassName, HWND16 hwndParent)
{
    LPDEVICE_INFO16 lpdi;
    FIXME("(%p %s %08lx %x %s %s %x): stub\n", lplpdi,
          debugstr_a(lpszDescription), dnDevnode, hkey,
          debugstr_a(lpszRegsubkey), debugstr_a(lpszClassName), hwndParent);
    lpdi = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DEVICE_INFO16));
    lpdi->cbSize = sizeof(DEVICE_INFO16);
    strcpy(lpdi->szClassName, lpszClassName);
    lpdi->hwndParent = hwndParent;
    *lplpdi = (LPDEVICE_INFO16)MapLS(lpdi);
    return OK;
}

/***********************************************************************
 *		DiDestroyDeviceInfoList (SETUPX.305)
 */
RETERR16 WINAPI DiDestroyDeviceInfoList16(LPDEVICE_INFO16 lpdi)
{
    FIXME("(%p): stub\n", lpdi);
    return FALSE;
}

/***********************************************************************
 *		DiOpenDevRegKey (SETUPX.319)
 */
RETERR16 WINAPI DiOpenDevRegKey16(LPDEVICE_INFO16 lpdi,
                                  LPHKEY16 lphk,INT16 iFlags)
{
    FIXME("(%p %p %d): stub\n", lpdi, lphk, iFlags);
    return FALSE;
}

/***********************************************************************
 *		SetupDiGetDeviceInterfaceDetailA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInterfaceDetailA(
      HDEVINFO DeviceInfoSet,
      PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
      PSP_DEVICE_INTERFACE_DETAIL_DATAA DeviceInterfaceDetailData,
      DWORD DeviceInterfaceDetailDataSize,
      PDWORD RequiredSize,
      PSP_DEVINFO_DATA DeviceInfoData )
{
    FIXME("\n");
    return FALSE;
}

/***********************************************************************
 *		SetupDiGetDeviceInterfaceDetailW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInterfaceDetailW(
      HDEVINFO DeviceInfoSet,
      PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
      PSP_DEVICE_INTERFACE_DETAIL_DATAW DeviceInterfaceDetailData,
      DWORD DeviceInterfaceDetailDataSize,
      PDWORD RequiredSize,
      PSP_DEVINFO_DATA DeviceInfoData )
{
    FIXME("\n");
    return FALSE;
}

/***********************************************************************
 *		SetupDiEnumDeviceInterfaces (SETUPAPI.@)
 */
BOOL WINAPI SetupDiEnumDeviceInterfaces(
       HDEVINFO DeviceInfoSet,
       PSP_DEVINFO_DATA DeviceInfoData,
       CONST GUID * InterfaceClassGuid,
       DWORD MemberIndex,
       PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData)
{
    FIXME("\n");
    return FALSE;
}

/***********************************************************************
 *		SetupDiGetClassDevsA (SETUPAPI.@)
 */
HDEVINFO WINAPI SetupDiGetClassDevsA(
       CONST GUID *class,
       LPCSTR enumstr,
       HWND parent,
       DWORD flags)
{
    FIXME("%s %s %p %08lx\n",debugstr_guid(class),enumstr,parent,flags);

    return (HDEVINFO) INVALID_HANDLE_VALUE;
}

/***********************************************************************
 *		SetupDiGetClassDevsW (SETUPAPI.@)
 */
HDEVINFO WINAPI SetupDiGetClassDevsW(
       CONST GUID *class,
       LPCWSTR enumstr,
       HWND parent,
       DWORD flags)
{
    FIXME("%s %s %p %08lx\n",debugstr_guid(class),debugstr_w(enumstr),parent,flags);

    return (HDEVINFO) INVALID_HANDLE_VALUE;
}
/***********************************************************************
 *		SetupDiEnumDeviceInfo (SETUPAPI.@)
 */
BOOL WINAPI SetupDiEnumDeviceInfo(
        HDEVINFO  devinfo,
        DWORD  index,
        PSP_DEVINFO_DATA info)
{
    FIXME("%p %ld %p\n", devinfo, index, info );

    if(info==NULL)
        return FALSE;
    if(info->cbSize < sizeof *info)
        return FALSE;

    return FALSE;
}

/***********************************************************************
 *		SetupDiDestroyDeviceInfoList (SETUPAPI.@)
 */
BOOL WINAPI SetupDiDestroyDeviceInfoList( HDEVINFO devinfo )
{
    FIXME("%04lx\n", (DWORD)devinfo);
    return FALSE;
}

/***********************************************************************
 *		SetupDiGetDeviceRegistryPropertyA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceRegistryPropertyA(
        HDEVINFO  devinfo,
        PSP_DEVINFO_DATA  DeviceInfoData,
        DWORD   Property,
        PDWORD  PropertyRegDataType,
        PBYTE   PropertyBuffer,
        DWORD   PropertyBufferSize,
        PDWORD  RequiredSize)
{
    FIXME("%04lx %p %ld %p %p %ld %p\n", (DWORD)devinfo, DeviceInfoData,
        Property, PropertyRegDataType, PropertyBuffer, PropertyBufferSize,
        RequiredSize);
    return FALSE;
}
