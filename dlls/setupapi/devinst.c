/*
 * SetupAPI device installer
 *
 * Copyright 2000 Andreas Mohr for CodeWeavers
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
#include "wine/port.h"
 
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winreg.h"
#include "winternl.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "setupapi.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "cfgmgr32.h"
#include "initguid.h"
#include "winioctl.h"
#include "rpc.h"
#include "rpcdce.h"

#include "setupapi_private.h"


WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

/* Unicode constants */
static const WCHAR ClassGUID[]  = {'C','l','a','s','s','G','U','I','D',0};
static const WCHAR Class[]  = {'C','l','a','s','s',0};
static const WCHAR ClassInstall32[]  = {'C','l','a','s','s','I','n','s','t','a','l','l','3','2',0};
static const WCHAR NoDisplayClass[]  = {'N','o','D','i','s','p','l','a','y','C','l','a','s','s',0};
static const WCHAR NoInstallClass[]  = {'N','o','I','n','s','t','a','l','l','C','l','a','s','s',0};
static const WCHAR NoUseClass[]  = {'N','o','U','s','e','C','l','a','s','s',0};
static const WCHAR NtExtension[]  = {'.','N','T',0};
static const WCHAR NtPlatformExtension[]  = {'.','N','T','x','8','6',0};
static const WCHAR Version[]  = {'V','e','r','s','i','o','n',0};
static const WCHAR WinExtension[]  = {'.','W','i','n',0};

/* Registry key and value names */
static const WCHAR ControlClass[] = {'S','y','s','t','e','m','\\',
                                  'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                  'C','o','n','t','r','o','l','\\',
                                  'C','l','a','s','s',0};

static const WCHAR DeviceClasses[] = {'S','y','s','t','e','m','\\',
                                  'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                  'C','o','n','t','r','o','l','\\',
                                  'D','e','v','i','c','e','C','l','a','s','s','e','s',0};
static const WCHAR Enum[] = {'S','y','s','t','e','m','\\',
                                  'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
				  'E','n','u','m',0};
static const WCHAR DeviceDesc[] = {'D','e','v','i','c','e','D','e','s','c',0};
static const WCHAR DeviceInstance[] = {'D','e','v','i','c','e','I','n','s','t','a','n','c','e',0};
static const WCHAR HardwareId[] = {'H','a','r','d','w','a','r','e','I','D',0};
static const WCHAR CompatibleIDs[] = {'C','o','m','p','a','t','i','b','l','e','I','d','s',0};
static const WCHAR Service[] = {'S','e','r','v','i','c','e',0};
static const WCHAR Driver[] = {'D','r','i','v','e','r',0};
static const WCHAR ConfigFlags[] = {'C','o','n','f','i','g','F','l','a','g','s',0};
static const WCHAR Mfg[] = {'M','f','g',0};
static const WCHAR FriendlyName[] = {'F','r','i','e','n','d','l','y','N','a','m','e',0};
static const WCHAR LocationInformation[] = {'L','o','c','a','t','i','o','n','I','n','f','o','r','m','a','t','i','o','n',0};
static const WCHAR Capabilities[] = {'C','a','p','a','b','i','l','i','t','i','e','s',0};
static const WCHAR UINumber[] = {'U','I','N','u','m','b','e','r',0};
static const WCHAR UpperFilters[] = {'U','p','p','e','r','F','i','l','t','e','r','s',0};
static const WCHAR LowerFilters[] = {'L','o','w','e','r','F','i','l','t','e','r','s',0};
static const WCHAR Phantom[] = {'P','h','a','n','t','o','m',0};

/* is used to identify if a DeviceInfoSet pointer is
valid or not */
#define SETUP_DEVICE_INFO_SET_MAGIC 0xd00ff056

struct DeviceInfoSet
{
    DWORD magic;        /* if is equal to SETUP_DEVICE_INFO_SET_MAGIC struct is okay */
    GUID ClassGuid;
    HWND hwndParent;
    DWORD cDevices;
    SP_DEVINFO_DATA *devices;
};

/* Pointed to by SP_DEVINFO_DATA's Reserved member */
struct DeviceInfo
{
    HKEY   key;
    BOOL   phantom;
    LPWSTR instanceId;
};

static struct DeviceInfo *SETUPDI_AllocateDeviceInfo(LPCWSTR instanceId,
        BOOL phantom)
{
    struct DeviceInfo *devInfo = HeapAlloc(GetProcessHeap(), 0,
            sizeof(struct DeviceInfo));

    if (devInfo)
    {
        devInfo->instanceId = HeapAlloc(GetProcessHeap(), 0,
                (lstrlenW(instanceId) + 1) * sizeof(WCHAR));
        if (devInfo->instanceId)
        {
            HKEY enumKey;
            LONG l;

            devInfo->key = INVALID_HANDLE_VALUE;
            devInfo->phantom = phantom;
            lstrcpyW(devInfo->instanceId, instanceId);
            struprW(devInfo->instanceId);
            l = RegCreateKeyExW(HKEY_LOCAL_MACHINE, Enum, 0, NULL, 0,
                    KEY_ALL_ACCESS, NULL, &enumKey, NULL);
            if (!l)
            {
                RegCreateKeyExW(enumKey, devInfo->instanceId, 0, NULL, 0,
                        KEY_ALL_ACCESS, NULL, &devInfo->key, NULL);
                if (phantom)
                    RegSetValueExW(devInfo->key, Phantom, 0, REG_DWORD,
                            (LPBYTE)&phantom, sizeof(phantom));
                RegCloseKey(enumKey);
            }
        }
        else
        {
            HeapFree(GetProcessHeap(), 0, devInfo);
            devInfo = NULL;
        }
    }
    return devInfo;
}

static void SETUPDI_FreeDeviceInfo(struct DeviceInfo *devInfo)
{
    if (devInfo->key != INVALID_HANDLE_VALUE)
        RegCloseKey(devInfo->key);
    if (devInfo->phantom)
    {
        HKEY enumKey;
        LONG l;

        l = RegCreateKeyExW(HKEY_LOCAL_MACHINE, Enum, 0, NULL, 0,
                KEY_ALL_ACCESS, NULL, &enumKey, NULL);
        if (!l)
        {
            RegDeleteTreeW(enumKey, devInfo->instanceId);
            RegCloseKey(enumKey);
        }
    }
    HeapFree(GetProcessHeap(), 0, devInfo->instanceId);
    HeapFree(GetProcessHeap(), 0, devInfo);
}

static void SETUPDI_GuidToString(const GUID *guid, LPWSTR guidStr)
{
    static const WCHAR fmt[] = {'{','%','0','8','X','-','%','0','4','X','-',
        '%','0','4','X','-','%','0','2','X','%','0','2','X','-','%','0','2',
        'X','%','0','2','X','%','0','2','X','%','0','2','X','%','0','2','X','%',
        '0','2','X','}',0};

    sprintfW(guidStr, fmt, guid->Data1, guid->Data2, guid->Data3,
        guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
        guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
}

/* Adds a device with GUID guid and identifer devInst to set.  Allocates a
 * struct DeviceInfo, and points the returned device info's Reserved member
 * to it.  "Phantom" devices are deleted from the registry when closed.
 * Returns a pointer to the newly allocated device info.
 */
static BOOL SETUPDI_AddDeviceToSet(struct DeviceInfoSet *set,
        const GUID *guid,
        DWORD devInst,
        LPCWSTR instanceId,
        BOOL phantom,
        SP_DEVINFO_DATA **dev)
{
    BOOL ret = FALSE;
    struct DeviceInfo *devInfo = SETUPDI_AllocateDeviceInfo(instanceId,
            phantom);

    TRACE("%p, %s, %d, %s, %d\n", set, debugstr_guid(guid), devInst,
            debugstr_w(instanceId), phantom);

    if (devInfo)
    {
        if (set->devices)
            set->devices = HeapReAlloc(GetProcessHeap(), 0, set->devices,
                    (set->cDevices + 1) * sizeof(SP_DEVINFO_DATA));
        else
            set->devices = HeapAlloc(GetProcessHeap(), 0,
                    sizeof(SP_DEVINFO_DATA));
        if (set->devices)
        {
            WCHAR classGuidStr[39];

            *dev = &set->devices[set->cDevices++];
            (*dev)->cbSize = sizeof(SP_DEVINFO_DATA);
            memcpy(&(*dev)->ClassGuid, guid, sizeof(GUID));
            (*dev)->DevInst = devInst;
            (*dev)->Reserved = (ULONG_PTR)devInfo;
            SETUPDI_GuidToString(guid, classGuidStr);
            SetupDiSetDeviceRegistryPropertyW((HDEVINFO)set,
                *dev, SPDRP_CLASSGUID, (const BYTE *)classGuidStr,
                lstrlenW(classGuidStr) * sizeof(WCHAR));
            ret = TRUE;
        }
        else
        {
            HeapFree(GetProcessHeap(), 0, devInfo);
            SetLastError(ERROR_OUTOFMEMORY);
        }
    }
    return ret;
}

/***********************************************************************
 *              SetupDiBuildClassInfoList  (SETUPAPI.@)
 *
 * Returns a list of setup class GUIDs that identify the classes
 * that are installed on a local machine.
 *
 * PARAMS
 *   Flags [I] control exclusion of classes from the list.
 *   ClassGuidList [O] pointer to a GUID-typed array that receives a list of setup class GUIDs.
 *   ClassGuidListSize [I] The number of GUIDs in the array (ClassGuidList).
 *   RequiredSize [O] pointer, which receives the number of GUIDs that are returned.
 *
 * RETURNS
 *   Success: TRUE.
 *   Failure: FALSE.
 */
BOOL WINAPI SetupDiBuildClassInfoList(
        DWORD Flags,
        LPGUID ClassGuidList,
        DWORD ClassGuidListSize,
        PDWORD RequiredSize)
{
    TRACE("\n");
    return SetupDiBuildClassInfoListExW(Flags, ClassGuidList,
                                        ClassGuidListSize, RequiredSize,
                                        NULL, NULL);
}

/***********************************************************************
 *              SetupDiBuildClassInfoListExA  (SETUPAPI.@)
 *
 * Returns a list of setup class GUIDs that identify the classes
 * that are installed on a local or remote macine.
 *
 * PARAMS
 *   Flags [I] control exclusion of classes from the list.
 *   ClassGuidList [O] pointer to a GUID-typed array that receives a list of setup class GUIDs.
 *   ClassGuidListSize [I] The number of GUIDs in the array (ClassGuidList).
 *   RequiredSize [O] pointer, which receives the number of GUIDs that are returned.
 *   MachineName [I] name of a remote machine.
 *   Reserved [I] must be NULL.
 *
 * RETURNS
 *   Success: TRUE.
 *   Failure: FALSE.
 */
BOOL WINAPI SetupDiBuildClassInfoListExA(
        DWORD Flags,
        LPGUID ClassGuidList,
        DWORD ClassGuidListSize,
        PDWORD RequiredSize,
        LPCSTR MachineName,
        PVOID Reserved)
{
    LPWSTR MachineNameW = NULL;
    BOOL bResult;

    TRACE("\n");

    if (MachineName)
    {
        MachineNameW = MultiByteToUnicode(MachineName, CP_ACP);
        if (MachineNameW == NULL) return FALSE;
    }

    bResult = SetupDiBuildClassInfoListExW(Flags, ClassGuidList,
                                           ClassGuidListSize, RequiredSize,
                                           MachineNameW, Reserved);

    MyFree(MachineNameW);

    return bResult;
}

/***********************************************************************
 *              SetupDiBuildClassInfoListExW  (SETUPAPI.@)
 *
 * Returns a list of setup class GUIDs that identify the classes
 * that are installed on a local or remote macine.
 *
 * PARAMS
 *   Flags [I] control exclusion of classes from the list.
 *   ClassGuidList [O] pointer to a GUID-typed array that receives a list of setup class GUIDs.
 *   ClassGuidListSize [I] The number of GUIDs in the array (ClassGuidList).
 *   RequiredSize [O] pointer, which receives the number of GUIDs that are returned.
 *   MachineName [I] name of a remote machine.
 *   Reserved [I] must be NULL.
 *
 * RETURNS
 *   Success: TRUE.
 *   Failure: FALSE.
 */
BOOL WINAPI SetupDiBuildClassInfoListExW(
        DWORD Flags,
        LPGUID ClassGuidList,
        DWORD ClassGuidListSize,
        PDWORD RequiredSize,
        LPCWSTR MachineName,
        PVOID Reserved)
{
    WCHAR szKeyName[40];
    HKEY hClassesKey;
    HKEY hClassKey;
    DWORD dwLength;
    DWORD dwIndex;
    LONG lError;
    DWORD dwGuidListIndex = 0;

    TRACE("\n");

    if (RequiredSize != NULL)
	*RequiredSize = 0;

    hClassesKey = SetupDiOpenClassRegKeyExW(NULL,
                                            KEY_ALL_ACCESS,
                                            DIOCR_INSTALLER,
                                            MachineName,
                                            Reserved);
    if (hClassesKey == INVALID_HANDLE_VALUE)
    {
	return FALSE;
    }

    for (dwIndex = 0; ; dwIndex++)
    {
	dwLength = 40;
	lError = RegEnumKeyExW(hClassesKey,
			       dwIndex,
			       szKeyName,
			       &dwLength,
			       NULL,
			       NULL,
			       NULL,
			       NULL);
	TRACE("RegEnumKeyExW() returns %d\n", lError);
	if (lError == ERROR_SUCCESS || lError == ERROR_MORE_DATA)
	{
	    TRACE("Key name: %p\n", szKeyName);

	    if (RegOpenKeyExW(hClassesKey,
			      szKeyName,
			      0,
			      KEY_ALL_ACCESS,
			      &hClassKey))
	    {
		RegCloseKey(hClassesKey);
		return FALSE;
	    }

	    if (!RegQueryValueExW(hClassKey,
				  NoUseClass,
				  NULL,
				  NULL,
				  NULL,
				  NULL))
	    {
		TRACE("'NoUseClass' value found!\n");
		RegCloseKey(hClassKey);
		continue;
	    }

	    if ((Flags & DIBCI_NOINSTALLCLASS) &&
		(!RegQueryValueExW(hClassKey,
				   NoInstallClass,
				   NULL,
				   NULL,
				   NULL,
				   NULL)))
	    {
		TRACE("'NoInstallClass' value found!\n");
		RegCloseKey(hClassKey);
		continue;
	    }

	    if ((Flags & DIBCI_NODISPLAYCLASS) &&
		(!RegQueryValueExW(hClassKey,
				   NoDisplayClass,
				   NULL,
				   NULL,
				   NULL,
				   NULL)))
	    {
		TRACE("'NoDisplayClass' value found!\n");
		RegCloseKey(hClassKey);
		continue;
	    }

	    RegCloseKey(hClassKey);

	    TRACE("Guid: %p\n", szKeyName);
	    if (dwGuidListIndex < ClassGuidListSize)
	    {
		if (szKeyName[0] == '{' && szKeyName[37] == '}')
		{
		    szKeyName[37] = 0;
		}
		TRACE("Guid: %p\n", &szKeyName[1]);

		UuidFromStringW(&szKeyName[1],
				&ClassGuidList[dwGuidListIndex]);
	    }

	    dwGuidListIndex++;
	}

	if (lError != ERROR_SUCCESS)
	    break;
    }

    RegCloseKey(hClassesKey);

    if (RequiredSize != NULL)
	*RequiredSize = dwGuidListIndex;

    if (ClassGuidListSize < dwGuidListIndex)
    {
	SetLastError(ERROR_INSUFFICIENT_BUFFER);
	return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *		SetupDiClassGuidsFromNameA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiClassGuidsFromNameA(
        LPCSTR ClassName,
        LPGUID ClassGuidList,
        DWORD ClassGuidListSize,
        PDWORD RequiredSize)
{
  return SetupDiClassGuidsFromNameExA(ClassName, ClassGuidList,
                                      ClassGuidListSize, RequiredSize,
                                      NULL, NULL);
}

/***********************************************************************
 *		SetupDiClassGuidsFromNameW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiClassGuidsFromNameW(
        LPCWSTR ClassName,
        LPGUID ClassGuidList,
        DWORD ClassGuidListSize,
        PDWORD RequiredSize)
{
  return SetupDiClassGuidsFromNameExW(ClassName, ClassGuidList,
                                      ClassGuidListSize, RequiredSize,
                                      NULL, NULL);
}

/***********************************************************************
 *		SetupDiClassGuidsFromNameExA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiClassGuidsFromNameExA(
        LPCSTR ClassName,
        LPGUID ClassGuidList,
        DWORD ClassGuidListSize,
        PDWORD RequiredSize,
        LPCSTR MachineName,
        PVOID Reserved)
{
    LPWSTR ClassNameW = NULL;
    LPWSTR MachineNameW = NULL;
    BOOL bResult;

    FIXME("\n");

    ClassNameW = MultiByteToUnicode(ClassName, CP_ACP);
    if (ClassNameW == NULL)
        return FALSE;

    if (MachineName)
    {
        MachineNameW = MultiByteToUnicode(MachineName, CP_ACP);
        if (MachineNameW == NULL)
        {
            MyFree(ClassNameW);
            return FALSE;
        }
    }

    bResult = SetupDiClassGuidsFromNameExW(ClassNameW, ClassGuidList,
                                           ClassGuidListSize, RequiredSize,
                                           MachineNameW, Reserved);

    MyFree(MachineNameW);
    MyFree(ClassNameW);

    return bResult;
}

/***********************************************************************
 *		SetupDiClassGuidsFromNameExW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiClassGuidsFromNameExW(
        LPCWSTR ClassName,
        LPGUID ClassGuidList,
        DWORD ClassGuidListSize,
        PDWORD RequiredSize,
        LPCWSTR MachineName,
        PVOID Reserved)
{
    WCHAR szKeyName[40];
    WCHAR szClassName[256];
    HKEY hClassesKey;
    HKEY hClassKey;
    DWORD dwLength;
    DWORD dwIndex;
    LONG lError;
    DWORD dwGuidListIndex = 0;

    if (RequiredSize != NULL)
	*RequiredSize = 0;

    hClassesKey = SetupDiOpenClassRegKeyExW(NULL,
                                            KEY_ALL_ACCESS,
                                            DIOCR_INSTALLER,
                                            MachineName,
                                            Reserved);
    if (hClassesKey == INVALID_HANDLE_VALUE)
    {
	return FALSE;
    }

    for (dwIndex = 0; ; dwIndex++)
    {
	dwLength = 40;
	lError = RegEnumKeyExW(hClassesKey,
			       dwIndex,
			       szKeyName,
			       &dwLength,
			       NULL,
			       NULL,
			       NULL,
			       NULL);
	TRACE("RegEnumKeyExW() returns %d\n", lError);
	if (lError == ERROR_SUCCESS || lError == ERROR_MORE_DATA)
	{
	    TRACE("Key name: %p\n", szKeyName);

	    if (RegOpenKeyExW(hClassesKey,
			      szKeyName,
			      0,
			      KEY_ALL_ACCESS,
			      &hClassKey))
	    {
		RegCloseKey(hClassesKey);
		return FALSE;
	    }

	    dwLength = 256 * sizeof(WCHAR);
	    if (!RegQueryValueExW(hClassKey,
				  Class,
				  NULL,
				  NULL,
				  (LPBYTE)szClassName,
				  &dwLength))
	    {
		TRACE("Class name: %p\n", szClassName);

		if (strcmpiW(szClassName, ClassName) == 0)
		{
		    TRACE("Found matching class name\n");

		    TRACE("Guid: %p\n", szKeyName);
		    if (dwGuidListIndex < ClassGuidListSize)
		    {
			if (szKeyName[0] == '{' && szKeyName[37] == '}')
			{
			    szKeyName[37] = 0;
			}
			TRACE("Guid: %p\n", &szKeyName[1]);

			UuidFromStringW(&szKeyName[1],
					&ClassGuidList[dwGuidListIndex]);
		    }

		    dwGuidListIndex++;
		}
	    }

	    RegCloseKey(hClassKey);
	}

	if (lError != ERROR_SUCCESS)
	    break;
    }

    RegCloseKey(hClassesKey);

    if (RequiredSize != NULL)
	*RequiredSize = dwGuidListIndex;

    if (ClassGuidListSize < dwGuidListIndex)
    {
	SetLastError(ERROR_INSUFFICIENT_BUFFER);
	return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *              SetupDiClassNameFromGuidA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiClassNameFromGuidA(
        const GUID* ClassGuid,
        PSTR ClassName,
        DWORD ClassNameSize,
        PDWORD RequiredSize)
{
  return SetupDiClassNameFromGuidExA(ClassGuid, ClassName,
                                     ClassNameSize, RequiredSize,
                                     NULL, NULL);
}

/***********************************************************************
 *              SetupDiClassNameFromGuidW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiClassNameFromGuidW(
        const GUID* ClassGuid,
        PWSTR ClassName,
        DWORD ClassNameSize,
        PDWORD RequiredSize)
{
  return SetupDiClassNameFromGuidExW(ClassGuid, ClassName,
                                     ClassNameSize, RequiredSize,
                                     NULL, NULL);
}

/***********************************************************************
 *              SetupDiClassNameFromGuidExA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiClassNameFromGuidExA(
        const GUID* ClassGuid,
        PSTR ClassName,
        DWORD ClassNameSize,
        PDWORD RequiredSize,
        PCSTR MachineName,
        PVOID Reserved)
{
    WCHAR ClassNameW[MAX_CLASS_NAME_LEN];
    LPWSTR MachineNameW = NULL;
    BOOL ret;

    if (MachineName)
        MachineNameW = MultiByteToUnicode(MachineName, CP_ACP);
    ret = SetupDiClassNameFromGuidExW(ClassGuid, ClassNameW, MAX_CLASS_NAME_LEN,
     NULL, MachineNameW, Reserved);
    if (ret)
    {
        int len = WideCharToMultiByte(CP_ACP, 0, ClassNameW, -1, ClassName,
         ClassNameSize, NULL, NULL);

        if (!ClassNameSize && RequiredSize)
            *RequiredSize = len;
    }
    MyFree(MachineNameW);
    return ret;
}

/***********************************************************************
 *		SetupDiClassNameFromGuidExW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiClassNameFromGuidExW(
        const GUID* ClassGuid,
        PWSTR ClassName,
        DWORD ClassNameSize,
        PDWORD RequiredSize,
        PCWSTR MachineName,
        PVOID Reserved)
{
    HKEY hKey;
    DWORD dwLength;

    hKey = SetupDiOpenClassRegKeyExW(ClassGuid,
                                     KEY_ALL_ACCESS,
                                     DIOCR_INSTALLER,
                                     MachineName,
                                     Reserved);
    if (hKey == INVALID_HANDLE_VALUE)
    {
	return FALSE;
    }

    if (RequiredSize != NULL)
    {
	dwLength = 0;
	if (RegQueryValueExW(hKey,
			     Class,
			     NULL,
			     NULL,
			     NULL,
			     &dwLength))
	{
	    RegCloseKey(hKey);
	    return FALSE;
	}

	*RequiredSize = dwLength / sizeof(WCHAR);
    }

    dwLength = ClassNameSize * sizeof(WCHAR);
    if (RegQueryValueExW(hKey,
			 Class,
			 NULL,
			 NULL,
			 (LPBYTE)ClassName,
			 &dwLength))
    {
	RegCloseKey(hKey);
	return FALSE;
    }

    RegCloseKey(hKey);

    return TRUE;
}

/***********************************************************************
 *		SetupDiCreateDeviceInfoList (SETUPAPI.@)
 */
HDEVINFO WINAPI
SetupDiCreateDeviceInfoList(const GUID *ClassGuid,
			    HWND hwndParent)
{
  return SetupDiCreateDeviceInfoListExW(ClassGuid, hwndParent, NULL, NULL);
}

/***********************************************************************
 *		SetupDiCreateDeviceInfoListExA (SETUPAPI.@)
 */
HDEVINFO WINAPI
SetupDiCreateDeviceInfoListExA(const GUID *ClassGuid,
			       HWND hwndParent,
			       PCSTR MachineName,
			       PVOID Reserved)
{
    LPWSTR MachineNameW = NULL;
    HDEVINFO hDevInfo;

    TRACE("\n");

    if (MachineName)
    {
        MachineNameW = MultiByteToUnicode(MachineName, CP_ACP);
        if (MachineNameW == NULL)
            return (HDEVINFO)INVALID_HANDLE_VALUE;
    }

    hDevInfo = SetupDiCreateDeviceInfoListExW(ClassGuid, hwndParent,
                                              MachineNameW, Reserved);

    MyFree(MachineNameW);

    return hDevInfo;
}

/***********************************************************************
 *		SetupDiCreateDeviceInfoListExW (SETUPAPI.@)
 *
 * Create an empty DeviceInfoSet list.
 *
 * PARAMS
 *   ClassGuid [I] if not NULL only devices with GUID ClcassGuid are associated
 *                 with this list.
 *   hwndParent [I] hwnd needed for interface related actions.
 *   MachineName [I] name of machine to create emtpy DeviceInfoSet list, if NULL
 *                   local registry will be used.
 *   Reserved [I] must be NULL
 *
 * RETURNS
 *   Success: empty list.
 *   Failure: INVALID_HANDLE_VALUE.
 */
HDEVINFO WINAPI
SetupDiCreateDeviceInfoListExW(const GUID *ClassGuid,
			       HWND hwndParent,
			       PCWSTR MachineName,
			       PVOID Reserved)
{
    struct DeviceInfoSet *list = NULL;
    DWORD size = sizeof(struct DeviceInfoSet);

    TRACE("%s %p %s %p\n", debugstr_guid(ClassGuid), hwndParent,
      debugstr_w(MachineName), Reserved);

    if (MachineName != NULL)
    {
        FIXME("remote support is not implemented\n");
        SetLastError(ERROR_INVALID_MACHINENAME);
        return (HDEVINFO)INVALID_HANDLE_VALUE;
    }

    if (Reserved != NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (HDEVINFO)INVALID_HANDLE_VALUE;
    }

    list = HeapAlloc(GetProcessHeap(), 0, size);
    if (!list)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return (HDEVINFO)INVALID_HANDLE_VALUE;
    }

    list->magic = SETUP_DEVICE_INFO_SET_MAGIC;
    list->hwndParent = hwndParent;
    memcpy(&list->ClassGuid,
            ClassGuid ? ClassGuid : &GUID_NULL,
            sizeof(list->ClassGuid));
    list->cDevices = 0;
    list->devices = NULL;

    return (HDEVINFO)list;
}

/***********************************************************************
 *              SetupDiCreateDeviceInfoA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiCreateDeviceInfoA(
       HDEVINFO DeviceInfoSet,
       PCSTR DeviceName,
       CONST GUID *ClassGuid,
       PCSTR DeviceDescription,
       HWND hwndParent,
       DWORD CreationFlags,
       PSP_DEVINFO_DATA DeviceInfoData)
{
    BOOL ret = FALSE;
    LPWSTR DeviceNameW = NULL;
    LPWSTR DeviceDescriptionW = NULL;

    if (DeviceName)
    {
        DeviceNameW = MultiByteToUnicode(DeviceName, CP_ACP);
        if (DeviceNameW == NULL) return FALSE;
    }
    if (DeviceDescription)
    {
        DeviceDescriptionW = MultiByteToUnicode(DeviceDescription, CP_ACP);
        if (DeviceDescriptionW == NULL)
        {
            MyFree(DeviceNameW);
            return FALSE;
        }
    }

    ret = SetupDiCreateDeviceInfoW(DeviceInfoSet, DeviceNameW, ClassGuid, DeviceDescriptionW,
            hwndParent, CreationFlags, DeviceInfoData);

    MyFree(DeviceNameW);
    MyFree(DeviceDescriptionW);

    return ret;
}

static DWORD SETUPDI_DevNameToDevID(LPCWSTR devName)
{
    LPCWSTR ptr;
    DWORD devNameLen = lstrlenW(devName), devInst = 0;
    BOOL valid = TRUE;

    TRACE("%s\n", debugstr_w(devName));
    for (ptr = devName; valid && *ptr && ptr - devName < devNameLen; )
    {
	if (isdigitW(*ptr))
	{
	    devInst *= 10;
	    devInst |= *ptr - '0';
	    ptr++;
	}
	else
	    valid = FALSE;
    }
    TRACE("%d\n", valid ? devInst : 0xffffffff);
    return valid ? devInst : 0xffffffff;
}

/***********************************************************************
 *              SetupDiCreateDeviceInfoW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiCreateDeviceInfoW(
       HDEVINFO DeviceInfoSet,
       PCWSTR DeviceName,
       CONST GUID *ClassGuid,
       PCWSTR DeviceDescription,
       HWND hwndParent,
       DWORD CreationFlags,
       PSP_DEVINFO_DATA DeviceInfoData)
{
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    BOOL ret = FALSE, allocatedInstanceId = FALSE;
    LPCWSTR instanceId = NULL;

    TRACE("%p %s %s %s %p %x %p\n", DeviceInfoSet, debugstr_w(DeviceName),
        debugstr_guid(ClassGuid), debugstr_w(DeviceDescription),
        hwndParent, CreationFlags, DeviceInfoData);

    if (!DeviceName)
    {
        SetLastError(ERROR_INVALID_DEVINST_NAME);
        return FALSE;
    }
    if (!DeviceInfoSet || DeviceInfoSet == (HDEVINFO)INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!ClassGuid)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!IsEqualGUID(&set->ClassGuid, &GUID_NULL) &&
        !IsEqualGUID(ClassGuid, &set->ClassGuid))
    {
        SetLastError(ERROR_CLASS_MISMATCH);
        return FALSE;
    }
    if ((CreationFlags & DICD_GENERATE_ID))
    {
        if (strchrW(DeviceName, '\\'))
            SetLastError(ERROR_INVALID_DEVINST_NAME);
        else
        {
            static const WCHAR newDeviceFmt[] = {'R','O','O','T','\\','%','s',
                '\\','%','0','4','d',0};
            DWORD devId;

            if (set->cDevices)
            {
                DWORD i, highestDevID = 0;

                for (i = 0; i < set->cDevices; i++)
                {
                    struct DeviceInfo *devInfo =
                        (struct DeviceInfo *)set->devices[i].Reserved;
                    LPCWSTR devName = strrchrW(devInfo->instanceId, '\\');
                    DWORD id;

                    if (devName)
                        devName++;
                    else
                        devName = devInfo->instanceId;
                    id = SETUPDI_DevNameToDevID(devName);
                    if (id != 0xffffffff && id > highestDevID)
                        highestDevID = id;
                }
                devId = highestDevID + 1;
            }
            else
                devId = 0;
            /* 17 == lstrlenW(L"Root\\") + lstrlenW("\\") + 1 + %d max size */
            instanceId = HeapAlloc(GetProcessHeap(), 0,
                    (17 + lstrlenW(DeviceName)) * sizeof(WCHAR));
            if (instanceId)
            {
                sprintfW((LPWSTR)instanceId, newDeviceFmt, DeviceName,
                        devId);
                allocatedInstanceId = TRUE;
                ret = TRUE;
            }
            else
                ret = FALSE;
        }
    }
    else
    {
        DWORD i;

        ret = TRUE;
        instanceId = DeviceName;
        for (i = 0; ret && i < set->cDevices; i++)
        {
            struct DeviceInfo *devInfo =
                (struct DeviceInfo *)set->devices[i].Reserved;

            if (!lstrcmpiW(DeviceName, devInfo->instanceId))
            {
                SetLastError(ERROR_DEVINST_ALREADY_EXISTS);
                ret = FALSE;
            }
        }
    }
    if (ret)
    {
        SP_DEVINFO_DATA *dev = NULL;

        ret = SETUPDI_AddDeviceToSet(set, ClassGuid, 0 /* FIXME: DevInst */,
                instanceId, TRUE, &dev);
        if (ret)
        {
            if (DeviceDescription)
                SetupDiSetDeviceRegistryPropertyW(DeviceInfoSet,
                    dev, SPDRP_DEVICEDESC, (const BYTE *)DeviceDescription,
                    lstrlenW(DeviceDescription) * sizeof(WCHAR));
            if (DeviceInfoData)
            {
                if (DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA))
                {
                    SetLastError(ERROR_INVALID_USER_BUFFER);
                    ret = FALSE;
                }
                else
                    memcpy(DeviceInfoData, dev, sizeof(SP_DEVINFO_DATA));
            }
        }
    }
    if (allocatedInstanceId)
        HeapFree(GetProcessHeap(), 0, (LPWSTR)instanceId);

    return ret;
}

/***********************************************************************
 *		SetupDiRegisterDeviceInfo (SETUPAPI.@)
 */
BOOL WINAPI SetupDiRegisterDeviceInfo(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_DATA DeviceInfoData,
        DWORD Flags,
        PSP_DETSIG_CMPPROC CompareProc,
        PVOID CompareContext,
        PSP_DEVINFO_DATA DupDeviceInfoData)
{
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    struct DeviceInfo *devInfo;

    TRACE("%p %p %08x %p %p %p\n", DeviceInfoSet, DeviceInfoData, Flags,
            CompareProc, CompareContext, DupDeviceInfoData);

    if (!DeviceInfoSet || DeviceInfoSet == (HDEVINFO)INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!DeviceInfoData || DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA)
            || !DeviceInfoData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    devInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;
    if (devInfo->phantom)
    {
        devInfo->phantom = FALSE;
        RegDeleteValueW(devInfo->key, Phantom);
    }
    return TRUE;
}

/***********************************************************************
 *		SetupDiEnumDeviceInfo (SETUPAPI.@)
 */
BOOL WINAPI SetupDiEnumDeviceInfo(
        HDEVINFO  devinfo,
        DWORD  index,
        PSP_DEVINFO_DATA info)
{
    BOOL ret = FALSE;

    TRACE("%p %d %p\n", devinfo, index, info);

    if(info==NULL)
        return FALSE;
    if(info->cbSize < sizeof(*info))
        return FALSE;
    if (devinfo && devinfo != (HDEVINFO)INVALID_HANDLE_VALUE)
    {
        struct DeviceInfoSet *list = (struct DeviceInfoSet *)devinfo;
        if (list->magic == SETUP_DEVICE_INFO_SET_MAGIC)
        {
            if (index < list->cDevices)
            {
                if (info->cbSize == sizeof(SP_DEVINFO_DATA))
                {
                    memcpy(info, &list->devices[index], info->cbSize);
                    ret = TRUE;
                }
                else
                    SetLastError(ERROR_INVALID_USER_BUFFER);
            }
            else
                SetLastError(ERROR_NO_MORE_ITEMS);
        }
        else
            SetLastError(ERROR_INVALID_HANDLE);
    }
    else
        SetLastError(ERROR_INVALID_HANDLE);
    return ret;
}

/***********************************************************************
 *		SetupDiGetDeviceInstanceIdA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInstanceIdA(
	HDEVINFO DeviceInfoSet,
	PSP_DEVINFO_DATA DeviceInfoData,
	PSTR DeviceInstanceId,
	DWORD DeviceInstanceIdSize,
	PDWORD RequiredSize)
{
    BOOL ret = FALSE;
    DWORD size;
    PWSTR instanceId;

    TRACE("%p %p %p %d %p\n", DeviceInfoSet, DeviceInfoData, DeviceInstanceId,
	    DeviceInstanceIdSize, RequiredSize);

    SetupDiGetDeviceInstanceIdW(DeviceInfoSet,
                                DeviceInfoData,
                                NULL,
                                0,
                                &size);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return FALSE;
    instanceId = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
    if (instanceId)
    {
        ret = SetupDiGetDeviceInstanceIdW(DeviceInfoSet,
                                          DeviceInfoData,
                                          instanceId,
                                          size,
                                          &size);
        if (ret)
        {
            int len = WideCharToMultiByte(CP_ACP, 0, instanceId, -1,
                                          DeviceInstanceId,
                                          DeviceInstanceIdSize, NULL, NULL);

            if (!len)
                ret = FALSE;
            else
            {
                if (len > DeviceInstanceIdSize)
                {
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    ret = FALSE;
                }
                if (RequiredSize)
                    *RequiredSize = len;
            }
        }
        HeapFree(GetProcessHeap(), 0, instanceId);
    }
    return ret;
}

/***********************************************************************
 *		SetupDiGetDeviceInstanceIdW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInstanceIdW(
	HDEVINFO DeviceInfoSet,
	PSP_DEVINFO_DATA DeviceInfoData,
	PWSTR DeviceInstanceId,
	DWORD DeviceInstanceIdSize,
	PDWORD RequiredSize)
{
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    struct DeviceInfo *devInfo;

    TRACE("%p %p %p %d %p\n", DeviceInfoSet, DeviceInfoData, DeviceInstanceId,
	    DeviceInstanceIdSize, RequiredSize);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!DeviceInfoData || DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA)
            || !DeviceInfoData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    devInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;
    TRACE("instance ID: %s\n", debugstr_w(devInfo->instanceId));
    if (DeviceInstanceIdSize < lstrlenW(devInfo->instanceId) + 1)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        if (RequiredSize)
            *RequiredSize = lstrlenW(devInfo->instanceId) + 1;
        return FALSE;
    }
    lstrcpyW(DeviceInstanceId, devInfo->instanceId);
    if (RequiredSize)
        *RequiredSize = lstrlenW(devInfo->instanceId) + 1;
    return TRUE;
}

/***********************************************************************
 *		SetupDiGetActualSectionToInstallA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetActualSectionToInstallA(
        HINF InfHandle,
        PCSTR InfSectionName,
        PSTR InfSectionWithExt,
        DWORD InfSectionWithExtSize,
        PDWORD RequiredSize,
        PSTR *Extension)
{
    FIXME("\n");
    return FALSE;
}

/***********************************************************************
 *		SetupDiGetActualSectionToInstallW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetActualSectionToInstallW(
        HINF InfHandle,
        PCWSTR InfSectionName,
        PWSTR InfSectionWithExt,
        DWORD InfSectionWithExtSize,
        PDWORD RequiredSize,
        PWSTR *Extension)
{
    WCHAR szBuffer[MAX_PATH];
    DWORD dwLength;
    DWORD dwFullLength;
    LONG lLineCount = -1;

    lstrcpyW(szBuffer, InfSectionName);
    dwLength = lstrlenW(szBuffer);

    if (OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
	/* Test section name with '.NTx86' extension */
	lstrcpyW(&szBuffer[dwLength], NtPlatformExtension);
	lLineCount = SetupGetLineCountW(InfHandle, szBuffer);

	if (lLineCount == -1)
	{
	    /* Test section name with '.NT' extension */
	    lstrcpyW(&szBuffer[dwLength], NtExtension);
	    lLineCount = SetupGetLineCountW(InfHandle, szBuffer);
	}
    }
    else
    {
	/* Test section name with '.Win' extension */
	lstrcpyW(&szBuffer[dwLength], WinExtension);
	lLineCount = SetupGetLineCountW(InfHandle, szBuffer);
    }

    if (lLineCount == -1)
    {
	/* Test section name without extension */
	szBuffer[dwLength] = 0;
	lLineCount = SetupGetLineCountW(InfHandle, szBuffer);
    }

    if (lLineCount == -1)
    {
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }

    dwFullLength = lstrlenW(szBuffer);

    if (InfSectionWithExt != NULL && InfSectionWithExtSize != 0)
    {
	if (InfSectionWithExtSize < (dwFullLength + 1))
	{
	    SetLastError(ERROR_INSUFFICIENT_BUFFER);
	    return FALSE;
	}

	lstrcpyW(InfSectionWithExt, szBuffer);
	if (Extension != NULL)
	{
	    *Extension = (dwLength == dwFullLength) ? NULL : &InfSectionWithExt[dwLength];
	}
    }

    if (RequiredSize != NULL)
    {
	*RequiredSize = dwFullLength + 1;
    }

    return TRUE;
}

/***********************************************************************
 *		SetupDiGetClassDescriptionA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetClassDescriptionA(
        const GUID* ClassGuid,
        PSTR ClassDescription,
        DWORD ClassDescriptionSize,
        PDWORD RequiredSize)
{
  return SetupDiGetClassDescriptionExA(ClassGuid, ClassDescription,
                                       ClassDescriptionSize,
                                       RequiredSize, NULL, NULL);
}

/***********************************************************************
 *		SetupDiGetClassDescriptionW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetClassDescriptionW(
        const GUID* ClassGuid,
        PWSTR ClassDescription,
        DWORD ClassDescriptionSize,
        PDWORD RequiredSize)
{
  return SetupDiGetClassDescriptionExW(ClassGuid, ClassDescription,
                                       ClassDescriptionSize,
                                       RequiredSize, NULL, NULL);
}

/***********************************************************************
 *		SetupDiGetClassDescriptionExA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetClassDescriptionExA(
        const GUID* ClassGuid,
        PSTR ClassDescription,
        DWORD ClassDescriptionSize,
        PDWORD RequiredSize,
        PCSTR MachineName,
        PVOID Reserved)
{
    HKEY hKey;
    DWORD dwLength;
    BOOL ret;

    hKey = SetupDiOpenClassRegKeyExA(ClassGuid,
                                     KEY_ALL_ACCESS,
                                     DIOCR_INSTALLER,
                                     MachineName,
                                     Reserved);
    if (hKey == INVALID_HANDLE_VALUE)
    {
	WARN("SetupDiOpenClassRegKeyExA() failed (Error %u)\n", GetLastError());
	return FALSE;
    }

    dwLength = ClassDescriptionSize;
    ret = !RegQueryValueExA( hKey, NULL, NULL, NULL,
                             (LPBYTE)ClassDescription, &dwLength );
    if (RequiredSize) *RequiredSize = dwLength;
    RegCloseKey(hKey);
    return ret;
}

/***********************************************************************
 *		SetupDiGetClassDescriptionExW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetClassDescriptionExW(
        const GUID* ClassGuid,
        PWSTR ClassDescription,
        DWORD ClassDescriptionSize,
        PDWORD RequiredSize,
        PCWSTR MachineName,
        PVOID Reserved)
{
    HKEY hKey;
    DWORD dwLength;
    BOOL ret;

    hKey = SetupDiOpenClassRegKeyExW(ClassGuid,
                                     KEY_ALL_ACCESS,
                                     DIOCR_INSTALLER,
                                     MachineName,
                                     Reserved);
    if (hKey == INVALID_HANDLE_VALUE)
    {
	WARN("SetupDiOpenClassRegKeyExW() failed (Error %u)\n", GetLastError());
	return FALSE;
    }

    dwLength = ClassDescriptionSize * sizeof(WCHAR);
    ret = !RegQueryValueExW( hKey, NULL, NULL, NULL,
                             (LPBYTE)ClassDescription, &dwLength );
    if (RequiredSize) *RequiredSize = dwLength / sizeof(WCHAR);
    RegCloseKey(hKey);
    return ret;
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
    HDEVINFO ret;
    LPWSTR enumstrW = NULL;

    if (enumstr)
    {
        int len = MultiByteToWideChar(CP_ACP, 0, enumstr, -1, NULL, 0);
        enumstrW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!enumstrW)
        {
            ret = (HDEVINFO)INVALID_HANDLE_VALUE;
            goto end;
        }
        MultiByteToWideChar(CP_ACP, 0, enumstr, -1, enumstrW, len);
    }
    ret = SetupDiGetClassDevsW(class, enumstrW, parent, flags);
    HeapFree(GetProcessHeap(), 0, enumstrW);

end:
    return ret;
}

static void SETUPDI_EnumerateMatchingInterfaces(HDEVINFO DeviceInfoSet,
        HKEY key, const GUID *interface, LPCWSTR enumstr)
{
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    DWORD i, len;
    WCHAR subKeyName[MAX_PATH];
    LONG l;
    HKEY enumKey = INVALID_HANDLE_VALUE;

    TRACE("%s\n", debugstr_w(enumstr));

    l = RegCreateKeyExW(HKEY_LOCAL_MACHINE, Enum, 0, NULL, 0, KEY_READ, NULL,
            &enumKey, NULL);
    for (i = 0; !l; i++)
    {
        len = sizeof(subKeyName) / sizeof(subKeyName[0]);
        l = RegEnumKeyExW(key, i, subKeyName, &len, NULL, NULL, NULL, NULL);
        if (!l)
        {
            HKEY subKey;

            l = RegOpenKeyExW(key, subKeyName, 0, KEY_READ, &subKey);
            if (!l)
            {
                WCHAR deviceInst[MAX_PATH * 3];
                DWORD dataType;

                len = sizeof(deviceInst);
                l = RegQueryValueExW(subKey, DeviceInstance, NULL, &dataType,
                        (BYTE *)deviceInst, &len);
                if (!l && dataType == REG_SZ)
                {
                    TRACE("found instance ID %s\n", debugstr_w(deviceInst));
                    if (!enumstr || !lstrcmpiW(enumstr, deviceInst))
                    {
                        HKEY deviceKey;

                        l = RegOpenKeyExW(enumKey, deviceInst, 0, KEY_READ,
                                &deviceKey);
                        if (!l)
                        {
                            WCHAR deviceClassStr[40];

                            len = sizeof(deviceClassStr);
                            l = RegQueryValueExW(deviceKey, ClassGUID, NULL,
                                    &dataType, (BYTE *)deviceClassStr, &len);
                            if (!l && dataType == REG_SZ &&
                                    deviceClassStr[0] == '{' &&
                                    deviceClassStr[37] == '}')
                            {
                                GUID deviceClass;
                                SP_DEVINFO_DATA *dev;

                                deviceClassStr[37] = 0;
                                UuidFromStringW(&deviceClassStr[1],
                                        &deviceClass);
                                SETUPDI_AddDeviceToSet(set, &deviceClass,
                                        0 /* FIXME: DevInst */, deviceInst,
                                        FALSE, &dev);
                                /* FIXME: add this interface, and all the
                                 * device's interfaces, to the device
                                 */
                            }
                            RegCloseKey(deviceKey);
                        }
                    }
                }
                RegCloseKey(subKey);
            }
        }
    }
    if (enumKey != INVALID_HANDLE_VALUE)
        RegCloseKey(enumKey);
}

static void SETUPDI_EnumerateInterfaces(HDEVINFO DeviceInfoSet,
        const GUID *interface, LPCWSTR enumstr, DWORD flags)
{
    HKEY interfacesKey = SetupDiOpenClassRegKeyExW(interface, KEY_READ,
            DIOCR_INTERFACE, NULL, NULL);

    TRACE("%p, %s, %s, %08x\n", DeviceInfoSet, debugstr_guid(interface),
            debugstr_w(enumstr), flags);

    if (interfacesKey != INVALID_HANDLE_VALUE)
    {
        if (flags & DIGCF_ALLCLASSES)
        {
            DWORD i, len;
            WCHAR interfaceGuidStr[40];
            LONG l = ERROR_SUCCESS;

            for (i = 0; !l; i++)
            {
                len = sizeof(interfaceGuidStr) / sizeof(interfaceGuidStr[0]);
                l = RegEnumKeyExW(interfacesKey, i, interfaceGuidStr, &len,
                        NULL, NULL, NULL, NULL);
                if (!l)
                {
                    if (interfaceGuidStr[0] == '{' &&
                            interfaceGuidStr[37] == '}')
                    {
                        HKEY interfaceKey;
                        GUID interfaceGuid;

                        interfaceGuidStr[37] = 0;
                        UuidFromStringW(&interfaceGuidStr[1], &interfaceGuid);
                        l = RegOpenKeyExW(interfacesKey, interfaceGuidStr, 0,
                                KEY_READ, &interfaceKey);
                        if (!l)
                        {
                            SETUPDI_EnumerateMatchingInterfaces(DeviceInfoSet,
                                    interfaceKey, &interfaceGuid, enumstr);
                            RegCloseKey(interfaceKey);
                        }
                    }
                }
            }
        }
        else
        {
            /* In this case, SetupDiOpenClassRegKeyExW opened the specific
             * interface's key, so just pass that long
             */
            SETUPDI_EnumerateMatchingInterfaces(DeviceInfoSet,
                    interfacesKey, interface, enumstr);
        }
        RegCloseKey(interfacesKey);
    }
}

static void SETUPDI_EnumerateMatchingDevices(HDEVINFO DeviceInfoSet,
        LPCWSTR parent, HKEY key, const GUID *class, DWORD flags)
{
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)set;
    DWORD i, len;
    WCHAR subKeyName[MAX_PATH];
    LONG l = ERROR_SUCCESS;

    TRACE("%s\n", debugstr_w(parent));

    for (i = 0; !l; i++)
    {
        len = sizeof(subKeyName) / sizeof(subKeyName[0]);
        l = RegEnumKeyExW(key, i, subKeyName, &len, NULL, NULL, NULL, NULL);
        if (!l)
        {
            HKEY subKey;

            l = RegOpenKeyExW(key, subKeyName, 0, KEY_READ, &subKey);
            if (!l)
            {
                WCHAR classGuid[40];
                DWORD dataType;

                len = sizeof(classGuid);
                l = RegQueryValueExW(subKey, ClassGUID, NULL, &dataType,
                        (BYTE *)classGuid, &len);
                if (!l && dataType == REG_SZ)
                {
                    if (classGuid[0] == '{' && classGuid[37] == '}')
                    {
                        GUID deviceClass;

                        classGuid[37] = 0;
                        UuidFromStringW(&classGuid[1], &deviceClass);
                        if ((flags & DIGCF_ALLCLASSES) ||
                                IsEqualGUID(class, &deviceClass))
                        {
                            static const WCHAR fmt[] = {'%','s','\\','%','s',0};
                            LPWSTR instanceId;

                            instanceId = HeapAlloc(GetProcessHeap(), 0,
                                (lstrlenW(parent) + lstrlenW(subKeyName) + 2)
                                * sizeof(WCHAR));
                            if (instanceId)
                            {
                                SP_DEVINFO_DATA *dev;

                                sprintfW(instanceId, fmt, parent, subKeyName);
                                SETUPDI_AddDeviceToSet(set, &deviceClass,
                                        0 /* FIXME: DevInst */, instanceId,
                                        FALSE, &dev);
                                HeapFree(GetProcessHeap(), 0, instanceId);
                            }
                        }
                    }
                }
                RegCloseKey(subKey);
            }
        }
    }
}

static void SETUPDI_EnumerateDevices(HDEVINFO DeviceInfoSet, const GUID *class,
        LPCWSTR enumstr, DWORD flags)
{
    HKEY classesKey = SetupDiOpenClassRegKeyExW(class, KEY_READ,
            DIOCR_INSTALLER, NULL, NULL);

    TRACE("%p, %s, %s, %08x\n", DeviceInfoSet, debugstr_guid(class),
            debugstr_w(enumstr), flags);

    if (classesKey != INVALID_HANDLE_VALUE)
    {
        if (enumstr)
        {
            HKEY enumKey;
            LONG l = RegOpenKeyExW(classesKey, enumstr, 0, KEY_READ,
                    &enumKey);

            if (!l)
            {
                SETUPDI_EnumerateMatchingDevices(DeviceInfoSet, enumstr,
                        enumKey, class, flags);
                RegCloseKey(enumKey);
            }
        }
        else
        {
            DWORD i, len;
            WCHAR subKeyName[MAX_PATH];
            LONG l = ERROR_SUCCESS;

            for (i = 0; !l; i++)
            {
                len = sizeof(subKeyName) / sizeof(subKeyName[0]);
                l = RegEnumKeyExW(classesKey, i, subKeyName, &len, NULL,
                        NULL, NULL, NULL);
                if (!l)
                {
                    HKEY subKey;

                    l = RegOpenKeyExW(classesKey, subKeyName, 0, KEY_READ,
                            &subKey);
                    if (!l)
                    {
                        SETUPDI_EnumerateMatchingDevices(DeviceInfoSet,
                                subKeyName, subKey, class, flags);
                        RegCloseKey(subKey);
                    }
                }
            }
        }
        RegCloseKey(classesKey);
    }
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
    static const DWORD unsupportedFlags = DIGCF_DEFAULT | DIGCF_PRESENT |
        DIGCF_PROFILE;
    HDEVINFO set;

    TRACE("%s %s %p 0x%08x\n", debugstr_guid(class), debugstr_w(enumstr), parent, flags);

    if (!(flags & DIGCF_ALLCLASSES) && !class)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    if (flags & unsupportedFlags)
        WARN("unsupported flags %08x\n", flags & unsupportedFlags);
    /* WinXP always succeeds, returns empty list for unknown classes */
    set = SetupDiCreateDeviceInfoList(class, parent);
    if (set)
    {
        if (flags & DIGCF_DEVICEINTERFACE)
            SETUPDI_EnumerateInterfaces(set, class, enumstr, flags);
        else
            SETUPDI_EnumerateDevices(set, class, enumstr, flags);
    }
    return set;
}

/***********************************************************************
 *              SetupDiGetClassDevsExW (SETUPAPI.@)
 */
HDEVINFO WINAPI SetupDiGetClassDevsExW(
        CONST GUID *class,
        PCWSTR enumstr,
        HWND parent,
        DWORD flags,
        HDEVINFO deviceset,
        PCWSTR machine,
        PVOID reserved)
{
    FIXME("stub\n");
    return NULL;
}

/***********************************************************************
 *		SetupDiEnumDeviceInterfaces (SETUPAPI.@)
 */
BOOL WINAPI SetupDiEnumDeviceInterfaces(
       HDEVINFO devinfo,
       PSP_DEVINFO_DATA DeviceInfoData,
       CONST GUID * InterfaceClassGuid,
       DWORD MemberIndex,
       PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData)
{
    BOOL ret = FALSE;

    FIXME("%p, %p, %s, 0x%08x, %p\n", devinfo, DeviceInfoData,
     debugstr_guid(InterfaceClassGuid), MemberIndex, DeviceInterfaceData);

    if (devinfo && devinfo != (HDEVINFO)INVALID_HANDLE_VALUE)
    {
        struct DeviceInfoSet *list = (struct DeviceInfoSet *)devinfo;
        if (list->magic == SETUP_DEVICE_INFO_SET_MAGIC)
            SetLastError(ERROR_NO_MORE_ITEMS);
        else
            SetLastError(ERROR_INVALID_HANDLE);
    }
    else
        SetLastError(ERROR_INVALID_HANDLE);
    return ret;
}

/***********************************************************************
 *		SetupDiDestroyDeviceInfoList (SETUPAPI.@)
  *
 * Destroy a DeviceInfoList and free all used memory of the list.
 *
 * PARAMS
 *   devinfo [I] DeviceInfoList pointer to list to destroy
 *
 * RETURNS
 *   Success: non zero value.
 *   Failure: zero value.
 */
BOOL WINAPI SetupDiDestroyDeviceInfoList(HDEVINFO devinfo)
{
    BOOL ret = FALSE;

    TRACE("%p\n", devinfo);
    if (devinfo && devinfo != (HDEVINFO)INVALID_HANDLE_VALUE)
    {
        struct DeviceInfoSet *list = (struct DeviceInfoSet *)devinfo;

        if (list->magic == SETUP_DEVICE_INFO_SET_MAGIC)
        {
            DWORD i;

            for (i = 0; i < list->cDevices; i++)
                SETUPDI_FreeDeviceInfo(
                        (struct DeviceInfo *)list->devices[i].Reserved);
            HeapFree(GetProcessHeap(), 0, list->devices);
            HeapFree(GetProcessHeap(), 0, list);
            ret = TRUE;
        }
    }

    if (ret == FALSE)
        SetLastError(ERROR_INVALID_HANDLE);

    return ret;
}

/***********************************************************************
 *		SetupDiGetDeviceInterfaceDetailA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInterfaceDetailA(
      HDEVINFO DeviceInfoSet,
      PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
      PSP_DEVICE_INTERFACE_DETAIL_DATA_A DeviceInterfaceDetailData,
      DWORD DeviceInterfaceDetailDataSize,
      PDWORD RequiredSize,
      PSP_DEVINFO_DATA DeviceInfoData)
{
    BOOL ret = FALSE;

    FIXME("(%p, %p, %p, %d, %p, %p)\n", DeviceInfoSet,
     DeviceInterfaceData, DeviceInterfaceDetailData,
     DeviceInterfaceDetailDataSize, RequiredSize, DeviceInfoData);

    SetLastError(ERROR_INVALID_HANDLE);
    return ret;
}

/***********************************************************************
 *		SetupDiGetDeviceInterfaceDetailW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInterfaceDetailW(
      HDEVINFO DeviceInfoSet,
      PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
      PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailData,
      DWORD DeviceInterfaceDetailDataSize,
      PDWORD RequiredSize,
      PSP_DEVINFO_DATA DeviceInfoData)
{
    FIXME("(%p, %p, %p, %d, %p, %p): stub\n", DeviceInfoSet,
     DeviceInterfaceData, DeviceInterfaceDetailData,
     DeviceInterfaceDetailDataSize, RequiredSize, DeviceInfoData);
    return FALSE;
}

struct PropertyMapEntry
{
    DWORD   regType;
    LPCSTR  nameA;
    LPCWSTR nameW;
};

static struct PropertyMapEntry PropertyMap[] = {
    { REG_SZ, "DeviceDesc", DeviceDesc },
    { REG_MULTI_SZ, "HardwareId", HardwareId },
    { REG_MULTI_SZ, "CompatibleIDs", CompatibleIDs },
    { 0, NULL, NULL }, /* SPDRP_UNUSED0 */
    { REG_SZ, "Service", Service },
    { 0, NULL, NULL }, /* SPDRP_UNUSED1 */
    { 0, NULL, NULL }, /* SPDRP_UNUSED2 */
    { REG_SZ, "Class", Class },
    { REG_SZ, "ClassGUID", ClassGUID },
    { REG_SZ, "Driver", Driver },
    { REG_DWORD, "ConfigFlags", ConfigFlags },
    { REG_SZ, "Mfg", Mfg },
    { REG_SZ, "FriendlyName", FriendlyName },
    { REG_SZ, "LocationInformation", LocationInformation },
    { 0, NULL, NULL }, /* SPDRP_PHYSICAL_DEVICE_OBJECT_NAME */
    { REG_DWORD, "Capabilities", Capabilities },
    { REG_DWORD, "UINumber", UINumber },
    { REG_MULTI_SZ, "UpperFilters", UpperFilters },
    { REG_MULTI_SZ, "LowerFilters", LowerFilters },
};

/***********************************************************************
 *		SetupDiGetDeviceRegistryPropertyA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceRegistryPropertyA(
        HDEVINFO  DeviceInfoSet,
        PSP_DEVINFO_DATA  DeviceInfoData,
        DWORD   Property,
        PDWORD  PropertyRegDataType,
        PBYTE   PropertyBuffer,
        DWORD   PropertyBufferSize,
        PDWORD  RequiredSize)
{
    BOOL ret = FALSE;
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    struct DeviceInfo *devInfo;

    TRACE("%04x %p %d %p %p %d %p\n", (DWORD)DeviceInfoSet, DeviceInfoData,
        Property, PropertyRegDataType, PropertyBuffer, PropertyBufferSize,
        RequiredSize);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!DeviceInfoData || DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA)
            || !DeviceInfoData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    devInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;
    if (Property < sizeof(PropertyMap) / sizeof(PropertyMap[0])
        && PropertyMap[Property].nameA)
    {
        DWORD size = PropertyBufferSize;
        LONG l = RegQueryValueExA(devInfo->key, PropertyMap[Property].nameA,
                NULL, PropertyRegDataType, PropertyBuffer, &size);

        if (RequiredSize)
            *RequiredSize = size;
        if (!l)
            ret = TRUE;
        else
            SetLastError(l);
    }
    return ret;
}

/***********************************************************************
 *		SetupDiGetDeviceRegistryPropertyW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceRegistryPropertyW(
        HDEVINFO  DeviceInfoSet,
        PSP_DEVINFO_DATA  DeviceInfoData,
        DWORD   Property,
        PDWORD  PropertyRegDataType,
        PBYTE   PropertyBuffer,
        DWORD   PropertyBufferSize,
        PDWORD  RequiredSize)
{
    BOOL ret = FALSE;
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    struct DeviceInfo *devInfo;

    TRACE("%04x %p %d %p %p %d %p\n", (DWORD)DeviceInfoSet, DeviceInfoData,
        Property, PropertyRegDataType, PropertyBuffer, PropertyBufferSize,
        RequiredSize);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!DeviceInfoData || DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA)
            || !DeviceInfoData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    devInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;
    if (Property < sizeof(PropertyMap) / sizeof(PropertyMap[0])
        && PropertyMap[Property].nameW)
    {
        DWORD size = PropertyBufferSize;
        LONG l = RegQueryValueExW(devInfo->key, PropertyMap[Property].nameW,
                NULL, PropertyRegDataType, PropertyBuffer, &size);

        if (RequiredSize)
            *RequiredSize = size;
        if (!l)
            ret = TRUE;
        else
            SetLastError(l);
    }
    return ret;
}

/***********************************************************************
 *		SetupDiSetDeviceRegistryPropertyA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiSetDeviceRegistryPropertyA(
	HDEVINFO DeviceInfoSet,
	PSP_DEVINFO_DATA DeviceInfoData,
	DWORD Property,
	const BYTE *PropertyBuffer,
	DWORD PropertyBufferSize)
{
    BOOL ret = FALSE;
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    struct DeviceInfo *devInfo;

    TRACE("%p %p %d %p %d\n", DeviceInfoSet, DeviceInfoData, Property,
        PropertyBuffer, PropertyBufferSize);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!DeviceInfoData || DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA)
            || !DeviceInfoData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    devInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;
    if (Property < sizeof(PropertyMap) / sizeof(PropertyMap[0])
        && PropertyMap[Property].nameA)
    {
        LONG l = RegSetValueExA(devInfo->key, PropertyMap[Property].nameA, 0,
                PropertyMap[Property].regType, PropertyBuffer,
                PropertyBufferSize);
        if (!l)
            ret = TRUE;
        else
            SetLastError(l);
    }
    return ret;
}

/***********************************************************************
 *		SetupDiSetDeviceRegistryPropertyW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiSetDeviceRegistryPropertyW(
	HDEVINFO DeviceInfoSet,
	PSP_DEVINFO_DATA DeviceInfoData,
	DWORD Property,
	const BYTE *PropertyBuffer,
	DWORD PropertyBufferSize)
{
    BOOL ret = FALSE;
    struct DeviceInfoSet *set = (struct DeviceInfoSet *)DeviceInfoSet;
    struct DeviceInfo *devInfo;

    TRACE("%p %p %d %p %d\n", DeviceInfoSet, DeviceInfoData, Property,
        PropertyBuffer, PropertyBufferSize);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!DeviceInfoData || DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA)
            || !DeviceInfoData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    devInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;
    if (Property < sizeof(PropertyMap) / sizeof(PropertyMap[0])
        && PropertyMap[Property].nameW)
    {
        LONG l = RegSetValueExW(devInfo->key, PropertyMap[Property].nameW, 0,
                PropertyMap[Property].regType, PropertyBuffer,
                PropertyBufferSize);
        if (!l)
            ret = TRUE;
        else
            SetLastError(l);
    }
    return ret;
}

/***********************************************************************
 *		SetupDiInstallClassA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiInstallClassA(
        HWND hwndParent,
        PCSTR InfFileName,
        DWORD Flags,
        HSPFILEQ FileQueue)
{
    UNICODE_STRING FileNameW;
    BOOL Result;

    if (!RtlCreateUnicodeStringFromAsciiz(&FileNameW, InfFileName))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    Result = SetupDiInstallClassW(hwndParent, FileNameW.Buffer, Flags, FileQueue);

    RtlFreeUnicodeString(&FileNameW);

    return Result;
}

static HKEY CreateClassKey(HINF hInf)
{
    WCHAR FullBuffer[MAX_PATH];
    WCHAR Buffer[MAX_PATH];
    DWORD RequiredSize;
    HKEY hClassKey;

    if (!SetupGetLineTextW(NULL,
			   hInf,
			   Version,
			   ClassGUID,
			   Buffer,
			   MAX_PATH,
			   &RequiredSize))
    {
	return INVALID_HANDLE_VALUE;
    }

    lstrcpyW(FullBuffer, ControlClass);
    lstrcatW(FullBuffer, Buffer);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
		      FullBuffer,
		      0,
		      KEY_ALL_ACCESS,
		      &hClassKey))
    {
	if (!SetupGetLineTextW(NULL,
			       hInf,
			       Version,
			       Class,
			       Buffer,
			       MAX_PATH,
			       &RequiredSize))
	{
	    return INVALID_HANDLE_VALUE;
	}

	if (RegCreateKeyExW(HKEY_LOCAL_MACHINE,
			    FullBuffer,
			    0,
			    NULL,
			    REG_OPTION_NON_VOLATILE,
			    KEY_ALL_ACCESS,
			    NULL,
			    &hClassKey,
			    NULL))
	{
	    return INVALID_HANDLE_VALUE;
	}

    }

    if (RegSetValueExW(hClassKey,
		       Class,
		       0,
		       REG_SZ,
		       (LPBYTE)Buffer,
		       RequiredSize * sizeof(WCHAR)))
    {
	RegCloseKey(hClassKey);
	RegDeleteKeyW(HKEY_LOCAL_MACHINE,
		      FullBuffer);
	return INVALID_HANDLE_VALUE;
    }

    return hClassKey;
}

/***********************************************************************
 *		SetupDiInstallClassW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiInstallClassW(
        HWND hwndParent,
        PCWSTR InfFileName,
        DWORD Flags,
        HSPFILEQ FileQueue)
{
    WCHAR SectionName[MAX_PATH];
    DWORD SectionNameLength = 0;
    HINF hInf;
    BOOL bFileQueueCreated = FALSE;
    HKEY hClassKey;


    FIXME("\n");

    if ((Flags & DI_NOVCP) && (FileQueue == NULL || FileQueue == INVALID_HANDLE_VALUE))
    {
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }

    /* Open the .inf file */
    hInf = SetupOpenInfFileW(InfFileName,
			     NULL,
			     INF_STYLE_WIN4,
			     NULL);
    if (hInf == INVALID_HANDLE_VALUE)
    {

	return FALSE;
    }

    /* Create or open the class registry key 'HKLM\\CurrentControlSet\\Class\\{GUID}' */
    hClassKey = CreateClassKey(hInf);
    if (hClassKey == INVALID_HANDLE_VALUE)
    {
	SetupCloseInfFile(hInf);
	return FALSE;
    }


    /* Try to append a layout file */
#if 0
    SetupOpenAppendInfFileW(NULL, hInf, NULL);
#endif

    /* Retrieve the actual section name */
    SetupDiGetActualSectionToInstallW(hInf,
				      ClassInstall32,
				      SectionName,
				      MAX_PATH,
				      &SectionNameLength,
				      NULL);

#if 0
    if (!(Flags & DI_NOVCP))
    {
	FileQueue = SetupOpenFileQueue();
	if (FileQueue == INVALID_HANDLE_VALUE)
	{
	    SetupCloseInfFile(hInf);
	    return FALSE;
	}

	bFileQueueCreated = TRUE;

    }
#endif

    SetupInstallFromInfSectionW(NULL,
				hInf,
				SectionName,
				SPINST_REGISTRY,
				hClassKey,
				NULL,
				0,
				NULL,
				NULL,
				INVALID_HANDLE_VALUE,
				NULL);

    /* FIXME: More code! */

    if (bFileQueueCreated)
	SetupCloseFileQueue(FileQueue);

    SetupCloseInfFile(hInf);

    return TRUE;
}


/***********************************************************************
 *		SetupDiOpenClassRegKey  (SETUPAPI.@)
 */
HKEY WINAPI SetupDiOpenClassRegKey(
        const GUID* ClassGuid,
        REGSAM samDesired)
{
    return SetupDiOpenClassRegKeyExW(ClassGuid, samDesired,
                                     DIOCR_INSTALLER, NULL, NULL);
}


/***********************************************************************
 *		SetupDiOpenClassRegKeyExA  (SETUPAPI.@)
 */
HKEY WINAPI SetupDiOpenClassRegKeyExA(
        const GUID* ClassGuid,
        REGSAM samDesired,
        DWORD Flags,
        PCSTR MachineName,
        PVOID Reserved)
{
    PWSTR MachineNameW = NULL;
    HKEY hKey;

    TRACE("\n");

    if (MachineName)
    {
        MachineNameW = MultiByteToUnicode(MachineName, CP_ACP);
        if (MachineNameW == NULL)
            return INVALID_HANDLE_VALUE;
    }

    hKey = SetupDiOpenClassRegKeyExW(ClassGuid, samDesired,
                                     Flags, MachineNameW, Reserved);

    MyFree(MachineNameW);

    return hKey;
}


/***********************************************************************
 *		SetupDiOpenClassRegKeyExW  (SETUPAPI.@)
 */
HKEY WINAPI SetupDiOpenClassRegKeyExW(
        const GUID* ClassGuid,
        REGSAM samDesired,
        DWORD Flags,
        PCWSTR MachineName,
        PVOID Reserved)
{
    HKEY hClassesKey;
    HKEY key;
    LPCWSTR lpKeyName;
    LONG l;

    if (MachineName != NULL)
    {
        FIXME("Remote access not supported yet!\n");
        return INVALID_HANDLE_VALUE;
    }

    if (Flags == DIOCR_INSTALLER)
    {
        lpKeyName = ControlClass;
    }
    else if (Flags == DIOCR_INTERFACE)
    {
        lpKeyName = DeviceClasses;
    }
    else
    {
        ERR("Invalid Flags parameter!\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    if (!ClassGuid)
    {
        if ((l = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          lpKeyName,
                          0,
                          samDesired,
                          &hClassesKey)))
        {
            SetLastError(l);
            hClassesKey = INVALID_HANDLE_VALUE;
        }
        key = hClassesKey;
    }
    else
    {
        WCHAR bracedGuidString[39];

        SETUPDI_GuidToString(ClassGuid, bracedGuidString);

        if (!(l = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          lpKeyName,
                          0,
                          samDesired,
                          &hClassesKey)))
        {
            if ((l = RegOpenKeyExW(hClassesKey,
                              bracedGuidString,
                              0,
                              samDesired,
                              &key)))
            {
                SetLastError(l);
                key = INVALID_HANDLE_VALUE;
            }
            RegCloseKey(hClassesKey);
        }
        else
        {
            SetLastError(l);
            key = INVALID_HANDLE_VALUE;
        }
    }
    return key;
}

/***********************************************************************
 *		SetupDiOpenDeviceInterfaceW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiOpenDeviceInterfaceW(
       HDEVINFO DeviceInfoSet,
       PCWSTR DevicePath,
       DWORD OpenFlags,
       PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData)
{
    FIXME("%p %s %08x %p\n",
        DeviceInfoSet, debugstr_w(DevicePath), OpenFlags, DeviceInterfaceData);
    return FALSE;
}

/***********************************************************************
 *		SetupDiOpenDeviceInterfaceA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiOpenDeviceInterfaceA(
       HDEVINFO DeviceInfoSet,
       PCSTR DevicePath,
       DWORD OpenFlags,
       PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData)
{
    FIXME("%p %s %08x %p\n", DeviceInfoSet,
        debugstr_a(DevicePath), OpenFlags, DeviceInterfaceData);
    return FALSE;
}

/***********************************************************************
 *		SetupDiSetClassInstallParamsA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiSetClassInstallParamsA(
       HDEVINFO  DeviceInfoSet,
       PSP_DEVINFO_DATA DeviceInfoData,
       PSP_CLASSINSTALL_HEADER ClassInstallParams,
       DWORD ClassInstallParamsSize)
{
    FIXME("%p %p %x %u\n",DeviceInfoSet, DeviceInfoData,
          ClassInstallParams->InstallFunction, ClassInstallParamsSize);
    return FALSE;
}

/***********************************************************************
 *		SetupDiCallClassInstaller (SETUPAPI.@)
 */
BOOL WINAPI SetupDiCallClassInstaller(
       DI_FUNCTION InstallFunction,
       HDEVINFO DeviceInfoSet,
       PSP_DEVINFO_DATA DeviceInfoData)
{
    FIXME("%d %p %p\n", InstallFunction, DeviceInfoSet, DeviceInfoData);
    return FALSE;
}

/***********************************************************************
 *		SetupDiGetDeviceInstallParamsA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInstallParamsA(
       HDEVINFO DeviceInfoSet,
       PSP_DEVINFO_DATA DeviceInfoData,
       PSP_DEVINSTALL_PARAMS_A DeviceInstallParams)
{
    FIXME("%p %p %p\n", DeviceInfoSet, DeviceInfoData, DeviceInstallParams);
    return FALSE;
}

/***********************************************************************
 *		SetupDiOpenDevRegKey (SETUPAPI.@)
 */
HKEY WINAPI SetupDiOpenDevRegKey(
       HDEVINFO DeviceInfoSet,
       PSP_DEVINFO_DATA DeviceInfoData,
       DWORD Scope,
       DWORD HwProfile,
       DWORD KeyType,
       REGSAM samDesired)
{
    FIXME("%p %p %d %d %d %x\n", DeviceInfoSet, DeviceInfoData,
          Scope, HwProfile, KeyType, samDesired);
    return INVALID_HANDLE_VALUE;
}
