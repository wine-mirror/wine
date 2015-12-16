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
#include "wine/list.h"
#include "wine/unicode.h"
#include "cfgmgr32.h"
#include "winioctl.h"
#include "rpc.h"
#include "rpcdce.h"

#include "setupapi_private.h"


WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

/* Unicode constants */
static const WCHAR Chicago[]  = {'$','C','h','i','c','a','g','o','$',0};
static const WCHAR ClassGUID[]  = {'C','l','a','s','s','G','U','I','D',0};
static const WCHAR Class[]  = {'C','l','a','s','s',0};
static const WCHAR ClassInstall32[]  = {'C','l','a','s','s','I','n','s','t','a','l','l','3','2',0};
static const WCHAR NoDisplayClass[]  = {'N','o','D','i','s','p','l','a','y','C','l','a','s','s',0};
static const WCHAR NoInstallClass[]  = {'N','o','I','n','s','t','a','l','l','C','l','a','s','s',0};
static const WCHAR NoUseClass[]  = {'N','o','U','s','e','C','l','a','s','s',0};
static const WCHAR NtExtension[]  = {'.','N','T',0};
static const WCHAR NtPlatformExtension[]  = {'.','N','T','x','8','6',0};
static const WCHAR Signature[]  = {'S','i','g','n','a','t','u','r','e',0};
static const WCHAR Version[]  = {'V','e','r','s','i','o','n',0};
static const WCHAR WinExtension[]  = {'.','W','i','n',0};
static const WCHAR WindowsNT[]  = {'$','W','i','n','d','o','w','s',' ','N','T','$',0};

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
static const WCHAR SymbolicLink[] = {'S','y','m','b','o','l','i','c','L','i','n','k',0};

/* is used to identify if a DeviceInfoSet pointer is
valid or not */
#define SETUP_DEVICE_INFO_SET_MAGIC 0xd00ff056

struct DeviceInfoSet
{
    DWORD magic;        /* if is equal to SETUP_DEVICE_INFO_SET_MAGIC struct is okay */
    GUID ClassGuid;
    HWND hwndParent;
    DWORD cDevices;
    struct list devices;
};

struct DeviceInstance
{
    struct list entry;
    SP_DEVINFO_DATA data;
};

/* Pointed to by SP_DEVICE_INTERFACE_DATA's Reserved member */
struct InterfaceInfo
{
    LPWSTR           referenceString;
    LPWSTR           symbolicLink;
    PSP_DEVINFO_DATA device;
};

/* A device may have multiple instances of the same interface, so this holds
 * each instance belonging to a particular interface.
 */
struct InterfaceInstances
{
    GUID                      guid;
    DWORD                     cInstances;
    DWORD                     cInstancesAllocated;
    SP_DEVICE_INTERFACE_DATA *instances;
    struct list               entry;
};

/* Pointed to by SP_DEVINFO_DATA's Reserved member */
struct DeviceInfo
{
    struct DeviceInfoSet *set;
    HKEY                  key;
    BOOL                  phantom;
    DWORD                 devId;
    LPWSTR                instanceId;
    struct list           interfaces;
};

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

static void SETUPDI_FreeInterfaceInstances(struct InterfaceInstances *instances)
{
    DWORD i;

    for (i = 0; i < instances->cInstances; i++)
    {
        struct InterfaceInfo *ifaceInfo =
            (struct InterfaceInfo *)instances->instances[i].Reserved;

        if (ifaceInfo->device && ifaceInfo->device->Reserved)
        {
            struct DeviceInfo *devInfo =
                (struct DeviceInfo *)ifaceInfo->device->Reserved;

            if (devInfo->phantom)
                SetupDiDeleteDeviceInterfaceRegKey(devInfo->set,
                        &instances->instances[i], 0);
        }
        HeapFree(GetProcessHeap(), 0, ifaceInfo->referenceString);
        HeapFree(GetProcessHeap(), 0, ifaceInfo->symbolicLink);
        HeapFree(GetProcessHeap(), 0, ifaceInfo);
    }
    HeapFree(GetProcessHeap(), 0, instances->instances);
}

/* Finds the interface with interface class InterfaceClassGuid in the device.
 * Returns TRUE if found, and updates *interface to point to device's
 * interfaces member where the given interface was found.
 * Returns FALSE if not found.
 */
static BOOL SETUPDI_FindInterface(const struct DeviceInfo *devInfo,
        const GUID *InterfaceClassGuid, struct InterfaceInstances **iface_ret)
{
    BOOL found = FALSE;
    struct InterfaceInstances *iface;

    TRACE("%s\n", debugstr_guid(InterfaceClassGuid));

    LIST_FOR_EACH_ENTRY(iface, &devInfo->interfaces, struct InterfaceInstances,
            entry)
    {
        if (IsEqualGUID(&iface->guid, InterfaceClassGuid))
        {
            *iface_ret = iface;
            found = TRUE;
            break;
        }
    }
    TRACE("returning %d (%p)\n", found, found ? *iface_ret : NULL);
    return found;
}

/* Finds the interface instance with reference string ReferenceString in the
 * interface instance map.  Returns TRUE if found, and updates instanceIndex to
 * the index of the interface instance's instances member
 * where the given instance was found.  Returns FALSE if not found.
 */
static BOOL SETUPDI_FindInterfaceInstance(
        const struct InterfaceInstances *instances,
        LPCWSTR ReferenceString, DWORD *instanceIndex)
{
    BOOL found = FALSE;
    DWORD i;

    TRACE("%s\n", debugstr_w(ReferenceString));

    for (i = 0; !found && i < instances->cInstances; i++)
    {
        SP_DEVICE_INTERFACE_DATA *ifaceData = &instances->instances[i];
        struct InterfaceInfo *ifaceInfo =
            (struct InterfaceInfo *)ifaceData->Reserved;

        if (!ReferenceString && !ifaceInfo->referenceString)
        {
            *instanceIndex = i;
            found = TRUE;
        }
        else if (ReferenceString && ifaceInfo->referenceString &&
                !lstrcmpiW(ifaceInfo->referenceString, ReferenceString))
        {
            *instanceIndex = i;
            found = TRUE;
        }
    }
    TRACE("returning %d (%d)\n", found, found ? *instanceIndex : 0);
    return found;
}

static LPWSTR SETUPDI_CreateSymbolicLinkPath(LPCWSTR instanceId,
        const GUID *InterfaceClassGuid, LPCWSTR ReferenceString)
{
    static const WCHAR fmt[] = {'\\','\\','?','\\','%','s','#','%','s',0};
    WCHAR guidStr[39];
    DWORD len;
    LPWSTR ret;

    SETUPDI_GuidToString(InterfaceClassGuid, guidStr);
    /* omit length of format specifiers, but include NULL terminator: */
    len = lstrlenW(fmt) - 4 + 1;
    len += lstrlenW(instanceId) + lstrlenW(guidStr);
    if (ReferenceString && *ReferenceString)
    {
        /* space for a hash between string and reference string: */
        len += lstrlenW(ReferenceString) + 1;
    }
    ret = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (ret)
    {
        int printed = sprintfW(ret, fmt, instanceId, guidStr);
        LPWSTR ptr;

        /* replace '\\' with '#' after the "\\\\?\\" beginning */
        for (ptr = strchrW(ret + 4, '\\'); ptr; ptr = strchrW(ptr + 1, '\\'))
            *ptr = '#';
        if (ReferenceString && *ReferenceString)
        {
            ret[printed] = '\\';
            lstrcpyW(ret + printed + 1, ReferenceString);
        }
    }
    return ret;
}

/* Adds an interface with the given interface class and reference string to
 * the device, if it doesn't already exist in the device.  If iface is not
 * NULL, returns a pointer to the newly added (or already existing) interface.
 */
static BOOL SETUPDI_AddInterfaceInstance(PSP_DEVINFO_DATA DeviceInfoData,
        const GUID *InterfaceClassGuid, LPCWSTR ReferenceString,
        SP_DEVICE_INTERFACE_DATA **ifaceData)
{
    struct DeviceInfo *devInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;
    BOOL newInterface = FALSE, ret;
    struct InterfaceInstances *iface = NULL;

    TRACE("%p %s %s %p\n", devInfo, debugstr_guid(InterfaceClassGuid),
            debugstr_w(ReferenceString), iface);

    if (!(ret = SETUPDI_FindInterface(devInfo, InterfaceClassGuid, &iface)))
    {
        iface = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                sizeof(struct InterfaceInstances));
        if (iface)
        {
            list_add_tail(&devInfo->interfaces, &iface->entry);
            newInterface = TRUE;
        }
    }
    if (iface)
    {
        DWORD instanceIndex = 0;

        if (!(ret = SETUPDI_FindInterfaceInstance(iface, ReferenceString,
                        &instanceIndex)))
        {
            SP_DEVICE_INTERFACE_DATA *instance = NULL;

            if (!iface->cInstancesAllocated)
            {
                iface->instances = HeapAlloc(GetProcessHeap(), 0,
                        sizeof(SP_DEVICE_INTERFACE_DATA));
                if (iface->instances)
                    instance = &iface->instances[iface->cInstancesAllocated++];
            }
            else if (iface->cInstances == iface->cInstancesAllocated)
            {
                iface->instances = HeapReAlloc(GetProcessHeap(), 0,
                        iface->instances,
                        (iface->cInstancesAllocated + 1) *
                        sizeof(SP_DEVICE_INTERFACE_DATA));
                if (iface->instances)
                    instance = &iface->instances[iface->cInstancesAllocated++];
            }
            else
                instance = &iface->instances[iface->cInstances];
            if (instance)
            {
                struct InterfaceInfo *ifaceInfo = HeapAlloc(GetProcessHeap(),
                        0, sizeof(struct InterfaceInfo));

                if (ifaceInfo)
                {
                    ret = TRUE;
                    ifaceInfo->device = DeviceInfoData;
                    ifaceInfo->symbolicLink = SETUPDI_CreateSymbolicLinkPath(
                            devInfo->instanceId, InterfaceClassGuid,
                            ReferenceString);
                    if (ReferenceString)
                    {
                        ifaceInfo->referenceString =
                            HeapAlloc(GetProcessHeap(), 0,
                                (lstrlenW(ReferenceString) + 1) *
                                sizeof(WCHAR));
                        if (ifaceInfo->referenceString)
                            lstrcpyW(ifaceInfo->referenceString,
                                    ReferenceString);
                        else
                            ret = FALSE;
                    }
                    else
                        ifaceInfo->referenceString = NULL;
                    if (ret)
                    {
                        HKEY key;

                        iface->cInstances++;
                        instance->cbSize =
                            sizeof(SP_DEVICE_INTERFACE_DATA);
                        instance->InterfaceClassGuid = *InterfaceClassGuid;
                        instance->Flags = SPINT_ACTIVE; /* FIXME */
                        instance->Reserved = (ULONG_PTR)ifaceInfo;
                        if (newInterface)
                            iface->guid = *InterfaceClassGuid;
                        key = SetupDiCreateDeviceInterfaceRegKeyW(devInfo->set,
                                instance, 0, KEY_WRITE, NULL, NULL);
                        if (key != INVALID_HANDLE_VALUE)
                        {
                            RegSetValueExW(key, SymbolicLink, 0, REG_SZ,
                                    (BYTE *)ifaceInfo->symbolicLink,
                                    lstrlenW(ifaceInfo->symbolicLink) *
                                    sizeof(WCHAR));
                            RegCloseKey(key);
                        }
                        if (ifaceData)
                            *ifaceData = instance;
                    }
                    else
                        HeapFree(GetProcessHeap(), 0, ifaceInfo);
                }
            }
        }
        else
        {
            if (ifaceData)
                *ifaceData = &iface->instances[instanceIndex];
        }
    }
    else
        ret = FALSE;
    TRACE("returning %d\n", ret);
    return ret;
}

static BOOL SETUPDI_SetInterfaceSymbolicLink(SP_DEVICE_INTERFACE_DATA *iface,
        LPCWSTR symbolicLink)
{
    struct InterfaceInfo *info = (struct InterfaceInfo *)iface->Reserved;
    BOOL ret = FALSE;

    if (info)
    {
        HeapFree(GetProcessHeap(), 0, info->symbolicLink);
        info->symbolicLink = HeapAlloc(GetProcessHeap(), 0,
                (lstrlenW(symbolicLink) + 1) * sizeof(WCHAR));
        if (info->symbolicLink)
        {
            lstrcpyW(info->symbolicLink, symbolicLink);
            ret = TRUE;
        }
    }
    return ret;
}

static HKEY SETUPDI_CreateDevKey(struct DeviceInfo *devInfo)
{
    HKEY enumKey, key = INVALID_HANDLE_VALUE;
    LONG l;

    l = RegCreateKeyExW(HKEY_LOCAL_MACHINE, Enum, 0, NULL, 0, KEY_ALL_ACCESS,
            NULL, &enumKey, NULL);
    if (!l)
    {
        RegCreateKeyExW(enumKey, devInfo->instanceId, 0, NULL, 0,
                KEY_READ | KEY_WRITE, NULL, &key, NULL);
        RegCloseKey(enumKey);
    }
    return key;
}

static HKEY SETUPDI_CreateDrvKey(struct DeviceInfo *devInfo)
{
    static const WCHAR slash[] = { '\\',0 };
    WCHAR classKeyPath[MAX_PATH];
    HKEY classKey, key = INVALID_HANDLE_VALUE;
    LONG l;

    lstrcpyW(classKeyPath, ControlClass);
    lstrcatW(classKeyPath, slash);
    SETUPDI_GuidToString(&devInfo->set->ClassGuid,
            classKeyPath + lstrlenW(classKeyPath));
    l = RegCreateKeyExW(HKEY_LOCAL_MACHINE, classKeyPath, 0, NULL, 0,
            KEY_ALL_ACCESS, NULL, &classKey, NULL);
    if (!l)
    {
        static const WCHAR fmt[] = { '%','0','4','u',0 };
        WCHAR devId[10];

        sprintfW(devId, fmt, devInfo->devId);
        RegCreateKeyExW(classKey, devId, 0, NULL, 0, KEY_READ | KEY_WRITE,
                NULL, &key, NULL);
        RegCloseKey(classKey);
    }
    return key;
}

static struct DeviceInfo *SETUPDI_AllocateDeviceInfo(struct DeviceInfoSet *set,
        DWORD devId, LPCWSTR instanceId, BOOL phantom)
{
    struct DeviceInfo *devInfo = NULL;
    HANDLE devInst = GlobalAlloc(GMEM_FIXED, sizeof(struct DeviceInfo));
    if (devInst)
        devInfo = GlobalLock(devInst);

    if (devInfo)
    {
        devInfo->set = set;
        devInfo->devId = (DWORD)devInst;

        devInfo->instanceId = HeapAlloc(GetProcessHeap(), 0,
                (lstrlenW(instanceId) + 1) * sizeof(WCHAR));
        if (devInfo->instanceId)
        {
            devInfo->key = INVALID_HANDLE_VALUE;
            devInfo->phantom = phantom;
            lstrcpyW(devInfo->instanceId, instanceId);
            struprW(devInfo->instanceId);
            devInfo->key = SETUPDI_CreateDevKey(devInfo);
            if (devInfo->key != INVALID_HANDLE_VALUE)
            {
                if (phantom)
                    RegSetValueExW(devInfo->key, Phantom, 0, REG_DWORD,
                            (LPBYTE)&phantom, sizeof(phantom));
            }
            list_init(&devInfo->interfaces);
            GlobalUnlock(devInst);
        }
        else
        {
            GlobalUnlock(devInst);
            GlobalFree(devInst);
            devInfo = NULL;
        }
    }
    return devInfo;
}

static void SETUPDI_FreeDeviceInfo(struct DeviceInfo *devInfo)
{
    struct InterfaceInstances *iface, *next;

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
    LIST_FOR_EACH_ENTRY_SAFE(iface, next, &devInfo->interfaces,
            struct InterfaceInstances, entry)
    {
        list_remove(&iface->entry);
        SETUPDI_FreeInterfaceInstances(iface);
        HeapFree(GetProcessHeap(), 0, iface);
    }
    GlobalFree((HANDLE)devInfo->devId);
}

/* Adds a device with GUID guid and identifier devInst to set.  Allocates a
 * struct DeviceInfo, and points the returned device info's Reserved member
 * to it.  "Phantom" devices are deleted from the registry when closed.
 * Returns a pointer to the newly allocated device info.
 */
static BOOL SETUPDI_AddDeviceToSet(struct DeviceInfoSet *set,
        const GUID *guid,
        DWORD dev_inst,
        LPCWSTR instanceId,
        BOOL phantom,
        SP_DEVINFO_DATA **dev)
{
    BOOL ret = FALSE;
    struct DeviceInfo *devInfo = SETUPDI_AllocateDeviceInfo(set, set->cDevices,
            instanceId, phantom);

    TRACE("%p, %s, %d, %s, %d\n", set, debugstr_guid(guid), dev_inst,
            debugstr_w(instanceId), phantom);

    if (devInfo)
    {
        struct DeviceInstance *devInst =
                HeapAlloc(GetProcessHeap(), 0, sizeof(struct DeviceInstance));

        if (devInst)
        {
            WCHAR classGuidStr[39];

            list_add_tail(&set->devices, &devInst->entry);
            set->cDevices++;
            devInst->data.cbSize = sizeof(SP_DEVINFO_DATA);
            devInst->data.ClassGuid = *guid;
            devInst->data.DevInst = devInfo->devId;
            devInst->data.Reserved = (ULONG_PTR)devInfo;
            SETUPDI_GuidToString(guid, classGuidStr);
            SetupDiSetDeviceRegistryPropertyW(set, &devInst->data,
                SPDRP_CLASSGUID, (const BYTE *)classGuidStr,
                lstrlenW(classGuidStr) * sizeof(WCHAR));
            if (dev) *dev = &devInst->data;
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
 * that are installed on a local or remote machine.
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
 * that are installed on a local or remote machine.
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
            return INVALID_HANDLE_VALUE;
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
 *   ClassGuid [I] if not NULL only devices with GUID ClassGuid are associated
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

    if (MachineName && *MachineName)
    {
        FIXME("remote support is not implemented\n");
        SetLastError(ERROR_INVALID_MACHINENAME);
        return INVALID_HANDLE_VALUE;
    }

    if (Reserved != NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    list = HeapAlloc(GetProcessHeap(), 0, size);
    if (!list)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
    }

    list->magic = SETUP_DEVICE_INFO_SET_MAGIC;
    list->hwndParent = hwndParent;
    memcpy(&list->ClassGuid,
            ClassGuid ? ClassGuid : &GUID_NULL,
            sizeof(list->ClassGuid));
    list->cDevices = 0;
    list_init(&list->devices);

    return list;
}

/***********************************************************************
 *              SetupDiCreateDevRegKeyA (SETUPAPI.@)
 */
HKEY WINAPI SetupDiCreateDevRegKeyA(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_DATA DeviceInfoData,
        DWORD Scope,
        DWORD HwProfile,
        DWORD KeyType,
        HINF InfHandle,
        PCSTR InfSectionName)
{
    PWSTR InfSectionNameW = NULL;
    HKEY key;

    TRACE("%p %p %d %d %d %p %s\n", DeviceInfoSet, DeviceInfoData, Scope,
            HwProfile, KeyType, InfHandle, debugstr_a(InfSectionName));

    if (InfHandle)
    {
        if (!InfSectionName)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
        }
        else
        {
            InfSectionNameW = MultiByteToUnicode(InfSectionName, CP_ACP);
            if (InfSectionNameW == NULL) return INVALID_HANDLE_VALUE;
        }
    }
    key = SetupDiCreateDevRegKeyW(DeviceInfoSet, DeviceInfoData, Scope,
            HwProfile, KeyType, InfHandle, InfSectionNameW);
    MyFree(InfSectionNameW);
    return key;
}

/***********************************************************************
 *              SetupDiCreateDevRegKeyW (SETUPAPI.@)
 */
HKEY WINAPI SetupDiCreateDevRegKeyW(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_DATA DeviceInfoData,
        DWORD Scope,
        DWORD HwProfile,
        DWORD KeyType,
        HINF InfHandle,
        PCWSTR InfSectionName)
{
    struct DeviceInfoSet *set = DeviceInfoSet;
    struct DeviceInfo *devInfo;
    HKEY key = INVALID_HANDLE_VALUE;

    TRACE("%p %p %d %d %d %p %s\n", DeviceInfoSet, DeviceInfoData, Scope,
            HwProfile, KeyType, InfHandle, debugstr_w(InfSectionName));

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return INVALID_HANDLE_VALUE;
    }
    if (set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return INVALID_HANDLE_VALUE;
    }
    if (!DeviceInfoData || DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA)
            || !DeviceInfoData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    devInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;
    if (devInfo->set != set)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    if (Scope != DICS_FLAG_GLOBAL && Scope != DICS_FLAG_CONFIGSPECIFIC)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return INVALID_HANDLE_VALUE;
    }
    if (KeyType != DIREG_DEV && KeyType != DIREG_DRV)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return INVALID_HANDLE_VALUE;
    }
    if (devInfo->phantom)
    {
        SetLastError(ERROR_DEVINFO_NOT_REGISTERED);
        return INVALID_HANDLE_VALUE;
    }
    if (Scope != DICS_FLAG_GLOBAL)
        FIXME("unimplemented for scope %d\n", Scope);
    switch (KeyType)
    {
        case DIREG_DEV:
            key = SETUPDI_CreateDevKey(devInfo);
            break;
        case DIREG_DRV:
            key = SETUPDI_CreateDrvKey(devInfo);
            break;
        default:
            WARN("unknown KeyType %d\n", KeyType);
    }
    if (InfHandle)
        SetupInstallFromInfSectionW(NULL, InfHandle, InfSectionName, SPINST_ALL,
                NULL, NULL, SP_COPY_NEWER_ONLY, NULL, NULL, DeviceInfoSet,
                DeviceInfoData);
    return key;
}

/***********************************************************************
 *              SetupDiCreateDeviceInfoA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiCreateDeviceInfoA(HDEVINFO DeviceInfoSet, PCSTR DeviceName,
        const GUID *ClassGuid, PCSTR DeviceDescription, HWND hwndParent, DWORD CreationFlags,
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
    int devNameLen = lstrlenW(devName);
    DWORD devInst = 0;
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
BOOL WINAPI SetupDiCreateDeviceInfoW(HDEVINFO DeviceInfoSet, PCWSTR DeviceName,
        const GUID *ClassGuid, PCWSTR DeviceDescription, HWND hwndParent, DWORD CreationFlags,
        PSP_DEVINFO_DATA DeviceInfoData)
{
    struct DeviceInfoSet *set = DeviceInfoSet;
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
    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE)
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
                DWORD highestDevID = 0;
                struct DeviceInstance *devInst;

                LIST_FOR_EACH_ENTRY(devInst, &set->devices, struct DeviceInstance, entry)
                {
                    struct DeviceInfo *devInfo = (struct DeviceInfo *)devInst->data.Reserved;
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
        struct DeviceInstance *devInst;

        ret = TRUE;
        instanceId = DeviceName;
        LIST_FOR_EACH_ENTRY(devInst, &set->devices, struct DeviceInstance, entry)
        {
            struct DeviceInfo *devInfo = (struct DeviceInfo *)devInst->data.Reserved;

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
                    *DeviceInfoData = *dev;
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
    struct DeviceInfoSet *set = DeviceInfoSet;
    struct DeviceInfo *devInfo;

    TRACE("%p %p %08x %p %p %p\n", DeviceInfoSet, DeviceInfoData, Flags,
            CompareProc, CompareContext, DupDeviceInfoData);

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
    if (devInfo->set != set)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (devInfo->phantom)
    {
        devInfo->phantom = FALSE;
        RegDeleteValueW(devInfo->key, Phantom);
    }
    return TRUE;
}

/***********************************************************************
 *              SetupDiRemoveDevice (SETUPAPI.@)
 */
BOOL WINAPI SetupDiRemoveDevice(
        HDEVINFO devinfo,
        PSP_DEVINFO_DATA info)
{
    FIXME("(%p, %p): stub\n", devinfo, info);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
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
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (devinfo && devinfo != INVALID_HANDLE_VALUE)
    {
        struct DeviceInfoSet *list = devinfo;
        if (list->magic == SETUP_DEVICE_INFO_SET_MAGIC)
        {
            if (index < list->cDevices)
            {
                if (info->cbSize == sizeof(SP_DEVINFO_DATA))
                {
                    struct DeviceInstance *devInst;
                    DWORD i = 0;

                    LIST_FOR_EACH_ENTRY(devInst, &list->devices,
                            struct DeviceInstance, entry)
                    {
                        if (i++ == index)
                        {
                            *info = devInst->data;
                            break;
                        }
                    }
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
    struct DeviceInfoSet *set = DeviceInfoSet;
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
    if (devInfo->set != set)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    TRACE("instance ID: %s\n", debugstr_w(devInfo->instanceId));
    if (DeviceInstanceIdSize < strlenW(devInfo->instanceId) + 1)
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
HDEVINFO WINAPI SetupDiGetClassDevsA(const GUID *class, LPCSTR enumstr, HWND parent, DWORD flags)
{
    HDEVINFO ret;
    LPWSTR enumstrW = NULL;

    if (enumstr)
    {
        int len = MultiByteToWideChar(CP_ACP, 0, enumstr, -1, NULL, 0);
        enumstrW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!enumstrW)
        {
            ret = INVALID_HANDLE_VALUE;
            goto end;
        }
        MultiByteToWideChar(CP_ACP, 0, enumstr, -1, enumstrW, len);
    }
    ret = SetupDiGetClassDevsExW(class, enumstrW, parent, flags, NULL, NULL,
            NULL);
    HeapFree(GetProcessHeap(), 0, enumstrW);

end:
    return ret;
}

/***********************************************************************
 *		  SetupDiGetClassDevsExA (SETUPAPI.@)
 */
HDEVINFO WINAPI SetupDiGetClassDevsExA(
        const GUID *class,
        PCSTR enumstr,
        HWND parent,
        DWORD flags,
        HDEVINFO deviceset,
        PCSTR machine,
        PVOID reserved)
{
    HDEVINFO ret;
    LPWSTR enumstrW = NULL, machineW = NULL;

    if (enumstr)
    {
        int len = MultiByteToWideChar(CP_ACP, 0, enumstr, -1, NULL, 0);
        enumstrW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!enumstrW)
        {
            ret = INVALID_HANDLE_VALUE;
            goto end;
        }
        MultiByteToWideChar(CP_ACP, 0, enumstr, -1, enumstrW, len);
    }
    if (machine)
    {
        int len = MultiByteToWideChar(CP_ACP, 0, machine, -1, NULL, 0);
        machineW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!machineW)
        {
            HeapFree(GetProcessHeap(), 0, enumstrW);
            ret = INVALID_HANDLE_VALUE;
            goto end;
        }
        MultiByteToWideChar(CP_ACP, 0, machine, -1, machineW, len);
    }
    ret = SetupDiGetClassDevsExW(class, enumstrW, parent, flags, deviceset,
            machineW, reserved);
    HeapFree(GetProcessHeap(), 0, enumstrW);
    HeapFree(GetProcessHeap(), 0, machineW);

end:
    return ret;
}

static void SETUPDI_AddDeviceInterfaces(SP_DEVINFO_DATA *dev, HKEY key,
        const GUID *guid)
{
    DWORD i, len;
    WCHAR subKeyName[MAX_PATH];
    LONG l = ERROR_SUCCESS;

    for (i = 0; !l; i++)
    {
        len = sizeof(subKeyName) / sizeof(subKeyName[0]);
        l = RegEnumKeyExW(key, i, subKeyName, &len, NULL, NULL, NULL, NULL);
        if (!l)
        {
            HKEY subKey;
            SP_DEVICE_INTERFACE_DATA *iface = NULL;

            if (*subKeyName == '#')
            {
                /* The subkey name is the reference string, with a '#' prepended */
                SETUPDI_AddInterfaceInstance(dev, guid, subKeyName + 1, &iface);
                l = RegOpenKeyExW(key, subKeyName, 0, KEY_READ, &subKey);
                if (!l)
                {
                    WCHAR symbolicLink[MAX_PATH];
                    DWORD dataType;

                    len = sizeof(symbolicLink);
                    l = RegQueryValueExW(subKey, SymbolicLink, NULL, &dataType,
                            (BYTE *)symbolicLink, &len);
                    if (!l && dataType == REG_SZ)
                        SETUPDI_SetInterfaceSymbolicLink(iface, symbolicLink);
                    RegCloseKey(subKey);
                }
            }
            /* Allow enumeration to continue */
            l = ERROR_SUCCESS;
        }
    }
    /* FIXME: find and add all the device's interfaces to the device */
}

static void SETUPDI_EnumerateMatchingInterfaces(HDEVINFO DeviceInfoSet,
        HKEY key, const GUID *guid, LPCWSTR enumstr)
{
    struct DeviceInfoSet *set = DeviceInfoSet;
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
                                if (SETUPDI_AddDeviceToSet(set, &deviceClass,
                                        0 /* FIXME: DevInst */, deviceInst,
                                        FALSE, &dev))
                                    SETUPDI_AddDeviceInterfaces(dev, subKey, guid);
                            }
                            RegCloseKey(deviceKey);
                        }
                    }
                }
                RegCloseKey(subKey);
            }
            /* Allow enumeration to continue */
            l = ERROR_SUCCESS;
        }
    }
    if (enumKey != INVALID_HANDLE_VALUE)
        RegCloseKey(enumKey);
}

static void SETUPDI_EnumerateInterfaces(HDEVINFO DeviceInfoSet,
        const GUID *guid, LPCWSTR enumstr, DWORD flags)
{
    HKEY interfacesKey = SetupDiOpenClassRegKeyExW(guid, KEY_READ,
            DIOCR_INTERFACE, NULL, NULL);

    TRACE("%p, %s, %s, %08x\n", DeviceInfoSet, debugstr_guid(guid),
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
                        interfaceGuidStr[37] = '}';
                        interfaceGuidStr[38] = 0;
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
                    interfacesKey, guid, enumstr);
        }
        RegCloseKey(interfacesKey);
    }
}

static void SETUPDI_EnumerateMatchingDeviceInstances(struct DeviceInfoSet *set,
        LPCWSTR enumerator, LPCWSTR deviceName, HKEY deviceKey,
        const GUID *class, DWORD flags)
{
    DWORD i, len;
    WCHAR deviceInstance[MAX_PATH];
    LONG l = ERROR_SUCCESS;

    TRACE("%s %s\n", debugstr_w(enumerator), debugstr_w(deviceName));

    for (i = 0; !l; i++)
    {
        len = sizeof(deviceInstance) / sizeof(deviceInstance[0]);
        l = RegEnumKeyExW(deviceKey, i, deviceInstance, &len, NULL, NULL, NULL,
                NULL);
        if (!l)
        {
            HKEY subKey;

            l = RegOpenKeyExW(deviceKey, deviceInstance, 0, KEY_READ, &subKey);
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
                            static const WCHAR fmt[] =
                             {'%','s','\\','%','s','\\','%','s',0};
                            LPWSTR instanceId;

                            instanceId = HeapAlloc(GetProcessHeap(), 0,
                                (lstrlenW(enumerator) + lstrlenW(deviceName) +
                                lstrlenW(deviceInstance) + 3) * sizeof(WCHAR));
                            if (instanceId)
                            {
                                sprintfW(instanceId, fmt, enumerator,
                                        deviceName, deviceInstance);
                                SETUPDI_AddDeviceToSet(set, &deviceClass,
                                        0 /* FIXME: DevInst */, instanceId,
                                        FALSE, NULL);
                                HeapFree(GetProcessHeap(), 0, instanceId);
                            }
                        }
                    }
                }
                RegCloseKey(subKey);
            }
            /* Allow enumeration to continue */
            l = ERROR_SUCCESS;
        }
    }
}

static void SETUPDI_EnumerateMatchingDevices(HDEVINFO DeviceInfoSet,
        LPCWSTR parent, HKEY key, const GUID *class, DWORD flags)
{
    struct DeviceInfoSet *set = DeviceInfoSet;
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
                TRACE("%s\n", debugstr_w(subKeyName));
                SETUPDI_EnumerateMatchingDeviceInstances(set, parent,
                        subKeyName, subKey, class, flags);
                RegCloseKey(subKey);
            }
            /* Allow enumeration to continue */
            l = ERROR_SUCCESS;
        }
    }
}

static void SETUPDI_EnumerateDevices(HDEVINFO DeviceInfoSet, const GUID *class,
        LPCWSTR enumstr, DWORD flags)
{
    HKEY enumKey;
    LONG l;

    TRACE("%p, %s, %s, %08x\n", DeviceInfoSet, debugstr_guid(class),
            debugstr_w(enumstr), flags);

    l = RegCreateKeyExW(HKEY_LOCAL_MACHINE, Enum, 0, NULL, 0, KEY_READ, NULL,
            &enumKey, NULL);
    if (enumKey != INVALID_HANDLE_VALUE)
    {
        if (enumstr)
        {
            HKEY enumStrKey;

            l = RegOpenKeyExW(enumKey, enumstr, 0, KEY_READ,
                    &enumStrKey);
            if (!l)
            {
                SETUPDI_EnumerateMatchingDevices(DeviceInfoSet, enumstr,
                        enumStrKey, class, flags);
                RegCloseKey(enumStrKey);
            }
        }
        else
        {
            DWORD i, len;
            WCHAR subKeyName[MAX_PATH];

            l = ERROR_SUCCESS;
            for (i = 0; !l; i++)
            {
                len = sizeof(subKeyName) / sizeof(subKeyName[0]);
                l = RegEnumKeyExW(enumKey, i, subKeyName, &len, NULL,
                        NULL, NULL, NULL);
                if (!l)
                {
                    HKEY subKey;

                    l = RegOpenKeyExW(enumKey, subKeyName, 0, KEY_READ,
                            &subKey);
                    if (!l)
                    {
                        SETUPDI_EnumerateMatchingDevices(DeviceInfoSet,
                                subKeyName, subKey, class, flags);
                        RegCloseKey(subKey);
                    }
                    /* Allow enumeration to continue */
                    l = ERROR_SUCCESS;
                }
            }
        }
        RegCloseKey(enumKey);
    }
}

/***********************************************************************
 *		SetupDiGetClassDevsW (SETUPAPI.@)
 */
HDEVINFO WINAPI SetupDiGetClassDevsW(const GUID *class, LPCWSTR enumstr, HWND parent, DWORD flags)
{
    return SetupDiGetClassDevsExW(class, enumstr, parent, flags, NULL, NULL,
            NULL);
}

/***********************************************************************
 *              SetupDiGetClassDevsExW (SETUPAPI.@)
 */
HDEVINFO WINAPI SetupDiGetClassDevsExW(const GUID *class, PCWSTR enumstr, HWND parent, DWORD flags,
        HDEVINFO deviceset, PCWSTR machine, void *reserved)
{
    static const DWORD unsupportedFlags = DIGCF_DEFAULT | DIGCF_PRESENT |
        DIGCF_PROFILE;
    HDEVINFO set;

    TRACE("%s %s %p 0x%08x %p %s %p\n", debugstr_guid(class),
            debugstr_w(enumstr), parent, flags, deviceset, debugstr_w(machine),
            reserved);

    if (!(flags & DIGCF_ALLCLASSES) && !class)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    if (flags & unsupportedFlags)
        WARN("unsupported flags %08x\n", flags & unsupportedFlags);
    if (deviceset)
        set = deviceset;
    else
        set = SetupDiCreateDeviceInfoListExW(class, parent, machine, reserved);
    if (set != INVALID_HANDLE_VALUE)
    {
        if (machine && *machine)
            FIXME("%s: unimplemented for remote machines\n",
                    debugstr_w(machine));
        else if (flags & DIGCF_DEVICEINTERFACE)
            SETUPDI_EnumerateInterfaces(set, class, enumstr, flags);
        else
            SETUPDI_EnumerateDevices(set, class, enumstr, flags);
    }
    return set;
}

/***********************************************************************
 *		SetupDiGetDeviceInfoListDetailA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInfoListDetailA(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_LIST_DETAIL_DATA_A DevInfoData )
{
    struct DeviceInfoSet *set = DeviceInfoSet;

    TRACE("%p %p\n", DeviceInfoSet, DevInfoData);

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
    if (!DevInfoData ||
            DevInfoData->cbSize != sizeof(SP_DEVINFO_LIST_DETAIL_DATA_A))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    DevInfoData->ClassGuid = set->ClassGuid;
    DevInfoData->RemoteMachineHandle = NULL;
    DevInfoData->RemoteMachineName[0] = '\0';
    return TRUE;
}

/***********************************************************************
 *		SetupDiGetDeviceInfoListDetailW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInfoListDetailW(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_LIST_DETAIL_DATA_W DevInfoData )
{
    struct DeviceInfoSet *set = DeviceInfoSet;

    TRACE("%p %p\n", DeviceInfoSet, DevInfoData);

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
    if (!DevInfoData ||
            DevInfoData->cbSize != sizeof(SP_DEVINFO_LIST_DETAIL_DATA_W))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    DevInfoData->ClassGuid = set->ClassGuid;
    DevInfoData->RemoteMachineHandle = NULL;
    DevInfoData->RemoteMachineName[0] = '\0';
    return TRUE;
}

/***********************************************************************
 *		SetupDiCreateDeviceInterfaceA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiCreateDeviceInterfaceA(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_DATA DeviceInfoData,
        const GUID *InterfaceClassGuid,
        PCSTR ReferenceString,
        DWORD CreationFlags,
        PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData)
{
    BOOL ret;
    LPWSTR ReferenceStringW = NULL;

    TRACE("%p %p %s %s %08x %p\n", DeviceInfoSet, DeviceInfoData,
            debugstr_guid(InterfaceClassGuid), debugstr_a(ReferenceString),
            CreationFlags, DeviceInterfaceData);

    if (ReferenceString)
    {
        ReferenceStringW = MultiByteToUnicode(ReferenceString, CP_ACP);
        if (ReferenceStringW == NULL) return FALSE;
    }

    ret = SetupDiCreateDeviceInterfaceW(DeviceInfoSet, DeviceInfoData,
            InterfaceClassGuid, ReferenceStringW, CreationFlags,
            DeviceInterfaceData);

    MyFree(ReferenceStringW);

    return ret;
}

/***********************************************************************
 *		SetupDiCreateDeviceInterfaceW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiCreateDeviceInterfaceW(
        HDEVINFO DeviceInfoSet,
        PSP_DEVINFO_DATA DeviceInfoData,
        const GUID *InterfaceClassGuid,
        PCWSTR ReferenceString,
        DWORD CreationFlags,
        PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData)
{
    struct DeviceInfoSet *set = DeviceInfoSet;
    struct DeviceInfo *devInfo;
    SP_DEVICE_INTERFACE_DATA *iface = NULL;
    BOOL ret;

    TRACE("%p %p %s %s %08x %p\n", DeviceInfoSet, DeviceInfoData,
            debugstr_guid(InterfaceClassGuid), debugstr_w(ReferenceString),
            CreationFlags, DeviceInterfaceData);

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
    if (devInfo->set != set)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!InterfaceClassGuid)
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }
    if ((ret = SETUPDI_AddInterfaceInstance(DeviceInfoData, InterfaceClassGuid,
                    ReferenceString, &iface)))
    {
        if (DeviceInterfaceData)
        {
            if (DeviceInterfaceData->cbSize != sizeof(SP_DEVICE_INTERFACE_DATA))
            {
                SetLastError(ERROR_INVALID_USER_BUFFER);
                ret = FALSE;
            }
            else
                *DeviceInterfaceData = *iface;
        }
    }
    return ret;
}

/***********************************************************************
 *		SetupDiCreateDeviceInterfaceRegKeyA (SETUPAPI.@)
 */
HKEY WINAPI SetupDiCreateDeviceInterfaceRegKeyA(
        HDEVINFO DeviceInfoSet,
        PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
        DWORD Reserved,
        REGSAM samDesired,
        HINF InfHandle,
        PCSTR InfSectionName)
{
    HKEY key;
    PWSTR InfSectionNameW = NULL;

    TRACE("%p %p %d %08x %p %p\n", DeviceInfoSet, DeviceInterfaceData, Reserved,
            samDesired, InfHandle, InfSectionName);
    if (InfHandle)
    {
        if (!InfSectionName)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
        }
        InfSectionNameW = MultiByteToUnicode(InfSectionName, CP_ACP);
        if (!InfSectionNameW)
            return INVALID_HANDLE_VALUE;
    }
    key = SetupDiCreateDeviceInterfaceRegKeyW(DeviceInfoSet,
            DeviceInterfaceData, Reserved, samDesired, InfHandle,
            InfSectionNameW);
    MyFree(InfSectionNameW);
    return key;
}

static PWSTR SETUPDI_GetInstancePath(struct InterfaceInfo *ifaceInfo)
{
    static const WCHAR hash[] = {'#',0};
    PWSTR instancePath = NULL;

    if (ifaceInfo->referenceString)
    {
        instancePath = HeapAlloc(GetProcessHeap(), 0,
                (lstrlenW(ifaceInfo->referenceString) + 2) * sizeof(WCHAR));
        if (instancePath)
        {
            lstrcpyW(instancePath, hash);
            lstrcatW(instancePath, ifaceInfo->referenceString);
        }
        else
            SetLastError(ERROR_OUTOFMEMORY);
    }
    else
    {
        instancePath = HeapAlloc(GetProcessHeap(), 0,
                (lstrlenW(hash) + 1) * sizeof(WCHAR));
        if (instancePath)
            lstrcpyW(instancePath, hash);
    }
    return instancePath;
}

/***********************************************************************
 *		SetupDiCreateDeviceInterfaceRegKeyW (SETUPAPI.@)
 */
HKEY WINAPI SetupDiCreateDeviceInterfaceRegKeyW(
        HDEVINFO DeviceInfoSet,
        PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
        DWORD Reserved,
        REGSAM samDesired,
        HINF InfHandle,
        PCWSTR InfSectionName)
{
    struct DeviceInfoSet *set = DeviceInfoSet;
    HKEY key = INVALID_HANDLE_VALUE, interfacesKey;
    LONG l;

    TRACE("%p %p %d %08x %p %p\n", DeviceInfoSet, DeviceInterfaceData, Reserved,
            samDesired, InfHandle, InfSectionName);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE ||
            set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return INVALID_HANDLE_VALUE;
    }
    if (!DeviceInterfaceData ||
            DeviceInterfaceData->cbSize != sizeof(SP_DEVICE_INTERFACE_DATA) ||
            !DeviceInterfaceData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    if (InfHandle && !InfSectionName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    if (!(l = RegCreateKeyExW(HKEY_LOCAL_MACHINE, DeviceClasses, 0, NULL, 0,
                    samDesired, NULL, &interfacesKey, NULL)))
    {
        HKEY parent;
        WCHAR bracedGuidString[39];

        SETUPDI_GuidToString(&DeviceInterfaceData->InterfaceClassGuid,
                bracedGuidString);
        if (!(l = RegCreateKeyExW(interfacesKey, bracedGuidString, 0, NULL, 0,
                        samDesired, NULL, &parent, NULL)))
        {
            struct InterfaceInfo *ifaceInfo =
                (struct InterfaceInfo *)DeviceInterfaceData->Reserved;
            PWSTR instancePath = SETUPDI_GetInstancePath(ifaceInfo);
            PWSTR interfKeyName = HeapAlloc(GetProcessHeap(), 0,
                    (lstrlenW(ifaceInfo->symbolicLink) + 1) * sizeof(WCHAR));
            HKEY interfKey;
            WCHAR *ptr;

            lstrcpyW(interfKeyName, ifaceInfo->symbolicLink);
            if (lstrlenW(ifaceInfo->symbolicLink) > 3)
            {
                interfKeyName[0] = '#';
                interfKeyName[1] = '#';
                interfKeyName[3] = '#';
            }
            ptr = strchrW(interfKeyName, '\\');
            if (ptr)
                *ptr = 0;
            l = RegCreateKeyExW(parent, interfKeyName, 0, NULL, 0,
                    samDesired, NULL, &interfKey, NULL);
            if (!l)
            {
                struct DeviceInfo *devInfo =
                        (struct DeviceInfo *)ifaceInfo->device->Reserved;

                l = RegSetValueExW(interfKey, DeviceInstance, 0, REG_SZ,
                        (BYTE *)devInfo->instanceId,
                        (lstrlenW(devInfo->instanceId) + 1) * sizeof(WCHAR));
                if (!l)
                {
                    if (instancePath)
                    {
                        LONG l;

                        l = RegCreateKeyExW(interfKey, instancePath, 0, NULL, 0,
                                samDesired, NULL, &key, NULL);
                        if (l)
                        {
                            SetLastError(l);
                            key = INVALID_HANDLE_VALUE;
                        }
                        else if (InfHandle)
                            FIXME("INF section installation unsupported\n");
                    }
                }
                else
                    SetLastError(l);
                RegCloseKey(interfKey);
            }
            else
                SetLastError(l);
            HeapFree(GetProcessHeap(), 0, interfKeyName);
            HeapFree(GetProcessHeap(), 0, instancePath);
            RegCloseKey(parent);
        }
        else
            SetLastError(l);
        RegCloseKey(interfacesKey);
    }
    else
        SetLastError(l);
    return key;
}

/***********************************************************************
 *		SetupDiDeleteDeviceInterfaceRegKey (SETUPAPI.@)
 */
BOOL WINAPI SetupDiDeleteDeviceInterfaceRegKey(
        HDEVINFO DeviceInfoSet,
        PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
        DWORD Reserved)
{
    struct DeviceInfoSet *set = DeviceInfoSet;
    HKEY parent;
    BOOL ret = FALSE;

    TRACE("%p %p %d\n", DeviceInfoSet, DeviceInterfaceData, Reserved);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE ||
            set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!DeviceInterfaceData ||
            DeviceInterfaceData->cbSize != sizeof(SP_DEVICE_INTERFACE_DATA) ||
            !DeviceInterfaceData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    parent = SetupDiOpenClassRegKeyExW(&DeviceInterfaceData->InterfaceClassGuid,
            KEY_ALL_ACCESS, DIOCR_INTERFACE, NULL, NULL);
    if (parent != INVALID_HANDLE_VALUE)
    {
        struct InterfaceInfo *ifaceInfo =
            (struct InterfaceInfo *)DeviceInterfaceData->Reserved;
        PWSTR instancePath = SETUPDI_GetInstancePath(ifaceInfo);

        if (instancePath)
        {
            LONG l = RegDeleteKeyW(parent, instancePath);

            if (l)
                SetLastError(l);
            else
                ret = TRUE;
            HeapFree(GetProcessHeap(), 0, instancePath);
        }
        RegCloseKey(parent);
    }
    return ret;
}

/***********************************************************************
 *		SetupDiEnumDeviceInterfaces (SETUPAPI.@)
 *
 * PARAMS
 *   DeviceInfoSet      [I]    Set of devices from which to enumerate
 *                             interfaces
 *   DeviceInfoData     [I]    (Optional) If specified, a specific device
 *                             instance from which to enumerate interfaces.
 *                             If it isn't specified, all interfaces for all
 *                             devices in the set are enumerated.
 *   InterfaceClassGuid [I]    The interface class to enumerate.
 *   MemberIndex        [I]    An index of the interface instance to enumerate.
 *                             A caller should start with MemberIndex set to 0,
 *                             and continue until the function fails with
 *                             ERROR_NO_MORE_ITEMS.
 *   DeviceInterfaceData [I/O] Returns an enumerated interface.  Its cbSize
 *                             member must be set to
 *                             sizeof(SP_DEVICE_INTERFACE_DATA).
 *
 * RETURNS
 *   Success: non-zero value.
 *   Failure: FALSE.  Call GetLastError() for more info.
 */
BOOL WINAPI SetupDiEnumDeviceInterfaces(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData,
        const GUID *InterfaceClassGuid, DWORD MemberIndex,
        PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData)
{
    struct DeviceInfoSet *set = DeviceInfoSet;
    BOOL ret = FALSE;

    TRACE("%p, %p, %s, %d, %p\n", DeviceInfoSet, DeviceInfoData,
     debugstr_guid(InterfaceClassGuid), MemberIndex, DeviceInterfaceData);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE ||
            set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (DeviceInfoData && (DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA) ||
                !DeviceInfoData->Reserved))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!DeviceInterfaceData ||
            DeviceInterfaceData->cbSize != sizeof(SP_DEVICE_INTERFACE_DATA))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* In case application fails to check return value, clear output */
    memset(DeviceInterfaceData, 0, sizeof(*DeviceInterfaceData));
    DeviceInterfaceData->cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    if (DeviceInfoData)
    {
        struct DeviceInfo *devInfo =
            (struct DeviceInfo *)DeviceInfoData->Reserved;
        struct InterfaceInstances *iface;

        if ((ret = SETUPDI_FindInterface(devInfo, InterfaceClassGuid, &iface)))
        {
            if (MemberIndex < iface->cInstances)
                *DeviceInterfaceData = iface->instances[MemberIndex];
            else
            {
                SetLastError(ERROR_NO_MORE_ITEMS);
                ret = FALSE;
            }
        }
        else
            SetLastError(ERROR_NO_MORE_ITEMS);
    }
    else
    {
        struct DeviceInstance *devInst;
        DWORD cEnumerated = 0;
        BOOL found = FALSE;

        LIST_FOR_EACH_ENTRY(devInst, &set->devices, struct DeviceInstance, entry)
        {
            struct DeviceInfo *devInfo = (struct DeviceInfo *)devInst->data.Reserved;
            struct InterfaceInstances *iface;

            if (found || cEnumerated >= MemberIndex + 1)
                break;
            if (SETUPDI_FindInterface(devInfo, InterfaceClassGuid, &iface))
            {
                if (cEnumerated + iface->cInstances < MemberIndex + 1)
                    cEnumerated += iface->cInstances;
                else
                {
                    DWORD instanceIndex = MemberIndex - cEnumerated;

                    *DeviceInterfaceData = iface->instances[instanceIndex];
                    cEnumerated += instanceIndex + 1;
                    found = TRUE;
                    ret = TRUE;
                }
            }
        }
        if (!found)
            SetLastError(ERROR_NO_MORE_ITEMS);
    }
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
    if (devinfo && devinfo != INVALID_HANDLE_VALUE)
    {
        struct DeviceInfoSet *list = devinfo;

        if (list->magic == SETUP_DEVICE_INFO_SET_MAGIC)
        {
            struct DeviceInstance *devInst, *devInst2;

            LIST_FOR_EACH_ENTRY_SAFE(devInst, devInst2, &list->devices,
                    struct DeviceInstance, entry)
            {
                SETUPDI_FreeDeviceInfo( (struct DeviceInfo *)devInst->data.Reserved );
                list_remove(&devInst->entry);
                HeapFree(GetProcessHeap(), 0, devInst);
            }
            HeapFree(GetProcessHeap(), 0, list);
            ret = TRUE;
        }
    }

    if (!ret)
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
    struct DeviceInfoSet *set = DeviceInfoSet;
    struct InterfaceInfo *info;
    DWORD bytesNeeded = FIELD_OFFSET(SP_DEVICE_INTERFACE_DETAIL_DATA_A, DevicePath[1]);
    BOOL ret = FALSE;

    TRACE("(%p, %p, %p, %d, %p, %p)\n", DeviceInfoSet,
     DeviceInterfaceData, DeviceInterfaceDetailData,
     DeviceInterfaceDetailDataSize, RequiredSize, DeviceInfoData);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE ||
            set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!DeviceInterfaceData ||
            DeviceInterfaceData->cbSize != sizeof(SP_DEVICE_INTERFACE_DATA) ||
            !DeviceInterfaceData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (DeviceInterfaceDetailData &&
        DeviceInterfaceDetailData->cbSize != sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A))
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }
    if (!DeviceInterfaceDetailData && DeviceInterfaceDetailDataSize)
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }
    info = (struct InterfaceInfo *)DeviceInterfaceData->Reserved;
    if (info->symbolicLink)
        bytesNeeded += WideCharToMultiByte(CP_ACP, 0, info->symbolicLink, -1,
                NULL, 0, NULL, NULL);
    if (DeviceInterfaceDetailDataSize >= bytesNeeded)
    {
        if (info->symbolicLink)
            WideCharToMultiByte(CP_ACP, 0, info->symbolicLink, -1,
                    DeviceInterfaceDetailData->DevicePath,
                    DeviceInterfaceDetailDataSize -
                    offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA_A, DevicePath),
                    NULL, NULL);
        else
            DeviceInterfaceDetailData->DevicePath[0] = '\0';
        if (DeviceInfoData && DeviceInfoData->cbSize == sizeof(SP_DEVINFO_DATA))
            *DeviceInfoData = *info->device;
        ret = TRUE;
    }
    else
    {
        if (RequiredSize)
            *RequiredSize = bytesNeeded;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }
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
    struct DeviceInfoSet *set = DeviceInfoSet;
    struct InterfaceInfo *info;
    DWORD bytesNeeded = offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA_W, DevicePath)
        + sizeof(WCHAR); /* include NULL terminator */
    BOOL ret = FALSE;

    TRACE("(%p, %p, %p, %d, %p, %p)\n", DeviceInfoSet,
     DeviceInterfaceData, DeviceInterfaceDetailData,
     DeviceInterfaceDetailDataSize, RequiredSize, DeviceInfoData);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE ||
            set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (!DeviceInterfaceData ||
            DeviceInterfaceData->cbSize != sizeof(SP_DEVICE_INTERFACE_DATA) ||
            !DeviceInterfaceData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (DeviceInterfaceDetailData && (DeviceInterfaceDetailData->cbSize <
            offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA_W, DevicePath) + sizeof(WCHAR) ||
            DeviceInterfaceDetailData->cbSize > sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)))
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }
    if (!DeviceInterfaceDetailData && DeviceInterfaceDetailDataSize)
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }
    info = (struct InterfaceInfo *)DeviceInterfaceData->Reserved;
    if (info->symbolicLink)
        bytesNeeded += sizeof(WCHAR)*lstrlenW(info->symbolicLink);
    if (DeviceInterfaceDetailDataSize >= bytesNeeded)
    {
        if (info->symbolicLink)
            lstrcpyW(DeviceInterfaceDetailData->DevicePath, info->symbolicLink);
        else
            DeviceInterfaceDetailData->DevicePath[0] = '\0';
        if (DeviceInfoData && DeviceInfoData->cbSize == sizeof(SP_DEVINFO_DATA))
            *DeviceInfoData = *info->device;
        ret = TRUE;
    }
    else
    {
        if (RequiredSize)
            *RequiredSize = bytesNeeded;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }
    return ret;
}

struct PropertyMapEntry
{
    DWORD   regType;
    LPCSTR  nameA;
    LPCWSTR nameW;
};

static const struct PropertyMapEntry PropertyMap[] = {
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
    struct DeviceInfoSet *set = DeviceInfoSet;
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
    if (PropertyBufferSize && PropertyBuffer == NULL)
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }
    devInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;
    if (Property < sizeof(PropertyMap) / sizeof(PropertyMap[0])
        && PropertyMap[Property].nameA)
    {
        DWORD size = PropertyBufferSize;
        LONG l = RegQueryValueExA(devInfo->key, PropertyMap[Property].nameA,
                NULL, PropertyRegDataType, PropertyBuffer, &size);

        if (l == ERROR_MORE_DATA || !PropertyBufferSize)
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        else if (!l)
            ret = TRUE;
        else
            SetLastError(l);
        if (RequiredSize)
            *RequiredSize = size;
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
    struct DeviceInfoSet *set = DeviceInfoSet;
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
    if (PropertyBufferSize && PropertyBuffer == NULL)
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }
    devInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;
    if (Property < sizeof(PropertyMap) / sizeof(PropertyMap[0])
        && PropertyMap[Property].nameW)
    {
        DWORD size = PropertyBufferSize;
        LONG l = RegQueryValueExW(devInfo->key, PropertyMap[Property].nameW,
                NULL, PropertyRegDataType, PropertyBuffer, &size);

        if (l == ERROR_MORE_DATA || !PropertyBufferSize)
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        else if (!l)
            ret = TRUE;
        else
            SetLastError(l);
        if (RequiredSize)
            *RequiredSize = size;
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
    struct DeviceInfoSet *set = DeviceInfoSet;
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
    struct DeviceInfoSet *set = DeviceInfoSet;
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

    if (!InfFileName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
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
    static const WCHAR slash[] = { '\\',0 };
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
    lstrcatW(FullBuffer, slash);
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

    if (!InfFileName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
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
    SetupOpenAppendInfFileW(NULL, hInf, NULL);

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
				SPINST_COPYINF | SPINST_FILES | SPINST_REGISTRY,
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

    if (MachineName && *MachineName)
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
 *		SetupDiSetClassInstallParamsW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiSetClassInstallParamsW(
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
 *		SetupDiGetDeviceInstallParamsW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInstallParamsW(
       HDEVINFO DeviceInfoSet,
       PSP_DEVINFO_DATA DeviceInfoData,
       PSP_DEVINSTALL_PARAMS_W DeviceInstallParams)
{
    FIXME("%p %p %p\n", DeviceInfoSet, DeviceInfoData, DeviceInstallParams);
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
 *              SetupDiSetDeviceInstallParamsA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiSetDeviceInstallParamsA(
       HDEVINFO DeviceInfoSet,
       PSP_DEVINFO_DATA DeviceInfoData,
       PSP_DEVINSTALL_PARAMS_A DeviceInstallParams)
{
    FIXME("(%p, %p, %p) stub\n", DeviceInfoSet, DeviceInfoData, DeviceInstallParams);

    return TRUE;
}

/***********************************************************************
 *              SetupDiSetDeviceInstallParamsW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiSetDeviceInstallParamsW(
       HDEVINFO DeviceInfoSet,
       PSP_DEVINFO_DATA DeviceInfoData,
       PSP_DEVINSTALL_PARAMS_W DeviceInstallParams)
{
    FIXME("(%p, %p, %p) stub\n", DeviceInfoSet, DeviceInfoData, DeviceInstallParams);

    return TRUE;
}

static HKEY SETUPDI_OpenDevKey(struct DeviceInfo *devInfo, REGSAM samDesired)
{
    HKEY enumKey, key = INVALID_HANDLE_VALUE;
    LONG l;

    l = RegCreateKeyExW(HKEY_LOCAL_MACHINE, Enum, 0, NULL, 0, KEY_ALL_ACCESS,
            NULL, &enumKey, NULL);
    if (!l)
    {
        RegOpenKeyExW(enumKey, devInfo->instanceId, 0, samDesired, &key);
        RegCloseKey(enumKey);
    }
    return key;
}

static HKEY SETUPDI_OpenDrvKey(struct DeviceInfo *devInfo, REGSAM samDesired)
{
    static const WCHAR slash[] = { '\\',0 };
    WCHAR classKeyPath[MAX_PATH];
    HKEY classKey, key = INVALID_HANDLE_VALUE;
    LONG l;

    lstrcpyW(classKeyPath, ControlClass);
    lstrcatW(classKeyPath, slash);
    SETUPDI_GuidToString(&devInfo->set->ClassGuid,
            classKeyPath + lstrlenW(classKeyPath));
    l = RegCreateKeyExW(HKEY_LOCAL_MACHINE, classKeyPath, 0, NULL, 0,
            KEY_ALL_ACCESS, NULL, &classKey, NULL);
    if (!l)
    {
        static const WCHAR fmt[] = { '%','0','4','u',0 };
        WCHAR devId[10];

        sprintfW(devId, fmt, devInfo->devId);
        l = RegOpenKeyExW(classKey, devId, 0, samDesired, &key);
        RegCloseKey(classKey);
        if (l)
        {
            SetLastError(ERROR_KEY_DOES_NOT_EXIST);
            return INVALID_HANDLE_VALUE;
        }
    }
    return key;
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
    struct DeviceInfoSet *set = DeviceInfoSet;
    struct DeviceInfo *devInfo;
    HKEY key = INVALID_HANDLE_VALUE;

    TRACE("%p %p %d %d %d %x\n", DeviceInfoSet, DeviceInfoData,
          Scope, HwProfile, KeyType, samDesired);

    if (!DeviceInfoSet || DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return INVALID_HANDLE_VALUE;
    }
    if (set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return INVALID_HANDLE_VALUE;
    }
    if (!DeviceInfoData || DeviceInfoData->cbSize != sizeof(SP_DEVINFO_DATA)
            || !DeviceInfoData->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    if (Scope != DICS_FLAG_GLOBAL && Scope != DICS_FLAG_CONFIGSPECIFIC)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return INVALID_HANDLE_VALUE;
    }
    if (KeyType != DIREG_DEV && KeyType != DIREG_DRV)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return INVALID_HANDLE_VALUE;
    }
    devInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;
    if (devInfo->set != set)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    if (devInfo->phantom)
    {
        SetLastError(ERROR_DEVINFO_NOT_REGISTERED);
        return INVALID_HANDLE_VALUE;
    }
    if (Scope != DICS_FLAG_GLOBAL)
        FIXME("unimplemented for scope %d\n", Scope);
    switch (KeyType)
    {
        case DIREG_DEV:
            key = SETUPDI_OpenDevKey(devInfo, samDesired);
            break;
        case DIREG_DRV:
            key = SETUPDI_OpenDrvKey(devInfo, samDesired);
            break;
        default:
            WARN("unknown KeyType %d\n", KeyType);
    }
    return key;
}

static BOOL SETUPDI_DeleteDevKey(struct DeviceInfo *devInfo)
{
    HKEY enumKey;
    BOOL ret = FALSE;
    LONG l;

    l = RegCreateKeyExW(HKEY_LOCAL_MACHINE, Enum, 0, NULL, 0, KEY_ALL_ACCESS,
            NULL, &enumKey, NULL);
    if (!l)
    {
        ret = RegDeleteTreeW(enumKey, devInfo->instanceId);
        RegCloseKey(enumKey);
    }
    else
        SetLastError(l);
    return ret;
}

static BOOL SETUPDI_DeleteDrvKey(struct DeviceInfo *devInfo)
{
    static const WCHAR slash[] = { '\\',0 };
    WCHAR classKeyPath[MAX_PATH];
    HKEY classKey;
    LONG l;
    BOOL ret = FALSE;

    lstrcpyW(classKeyPath, ControlClass);
    lstrcatW(classKeyPath, slash);
    SETUPDI_GuidToString(&devInfo->set->ClassGuid,
            classKeyPath + lstrlenW(classKeyPath));
    l = RegCreateKeyExW(HKEY_LOCAL_MACHINE, classKeyPath, 0, NULL, 0,
            KEY_ALL_ACCESS, NULL, &classKey, NULL);
    if (!l)
    {
        static const WCHAR fmt[] = { '%','0','4','u',0 };
        WCHAR devId[10];

        sprintfW(devId, fmt, devInfo->devId);
        ret = RegDeleteTreeW(classKey, devId);
        RegCloseKey(classKey);
    }
    else
        SetLastError(l);
    return ret;
}

/***********************************************************************
 *		SetupDiDeleteDevRegKey (SETUPAPI.@)
 */
BOOL WINAPI SetupDiDeleteDevRegKey(
       HDEVINFO DeviceInfoSet,
       PSP_DEVINFO_DATA DeviceInfoData,
       DWORD Scope,
       DWORD HwProfile,
       DWORD KeyType)
{
    struct DeviceInfoSet *set = DeviceInfoSet;
    struct DeviceInfo *devInfo;
    BOOL ret = FALSE;

    TRACE("%p %p %d %d %d\n", DeviceInfoSet, DeviceInfoData, Scope, HwProfile,
            KeyType);

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
    if (Scope != DICS_FLAG_GLOBAL && Scope != DICS_FLAG_CONFIGSPECIFIC)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }
    if (KeyType != DIREG_DEV && KeyType != DIREG_DRV && KeyType != DIREG_BOTH)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }
    devInfo = (struct DeviceInfo *)DeviceInfoData->Reserved;
    if (devInfo->set != set)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (devInfo->phantom)
    {
        SetLastError(ERROR_DEVINFO_NOT_REGISTERED);
        return FALSE;
    }
    if (Scope != DICS_FLAG_GLOBAL)
        FIXME("unimplemented for scope %d\n", Scope);
    switch (KeyType)
    {
        case DIREG_DEV:
            ret = SETUPDI_DeleteDevKey(devInfo);
            break;
        case DIREG_DRV:
            ret = SETUPDI_DeleteDrvKey(devInfo);
            break;
        case DIREG_BOTH:
            ret = SETUPDI_DeleteDevKey(devInfo);
            if (ret)
                ret = SETUPDI_DeleteDrvKey(devInfo);
            break;
        default:
            WARN("unknown KeyType %d\n", KeyType);
    }
    return ret;
}

/***********************************************************************
 *              CM_Get_Device_IDA  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_IDA( DEVINST dnDevInst, PSTR Buffer,
                                   ULONG  BufferLen, ULONG  ulFlags)
{
    struct DeviceInfo *devInfo = GlobalLock((HANDLE)dnDevInst);

    TRACE("%x->%p, %p, %u %u\n", dnDevInst, devInfo, Buffer, BufferLen, ulFlags);

    if (!devInfo)
        return CR_NO_SUCH_DEVINST;

    WideCharToMultiByte(CP_ACP, 0, devInfo->instanceId, -1, Buffer, BufferLen, 0, 0);
    TRACE("Returning %s\n", debugstr_a(Buffer));
    return CR_SUCCESS;
}

/***********************************************************************
 *              CM_Get_Device_IDW  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_IDW( DEVINST dnDevInst, LPWSTR Buffer,
                                   ULONG  BufferLen, ULONG  ulFlags)
{
    struct DeviceInfo *devInfo = GlobalLock((HANDLE)dnDevInst);

    TRACE("%x->%p, %p, %u %u\n", dnDevInst, devInfo, Buffer, BufferLen, ulFlags);

    if (!devInfo)
    {
        WARN("dev instance %d not found!\n", dnDevInst);
        return CR_NO_SUCH_DEVINST;
    }

    lstrcpynW(Buffer, devInfo->instanceId, BufferLen);
    TRACE("Returning %s\n", debugstr_w(Buffer));
    GlobalUnlock((HANDLE)dnDevInst);
    return CR_SUCCESS;
}



/***********************************************************************
 *              CM_Get_Device_ID_Size  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_ID_Size( PULONG  pulLen, DEVINST dnDevInst,
                                        ULONG  ulFlags)
{
    struct DeviceInfo *ppdevInfo = GlobalLock((HANDLE)dnDevInst);

    TRACE("%x->%p, %p, %u\n", dnDevInst, ppdevInfo, pulLen, ulFlags);

    if (!ppdevInfo)
    {
        WARN("dev instance %d not found!\n", dnDevInst);
        return CR_NO_SUCH_DEVINST;
    }

    *pulLen = lstrlenW(ppdevInfo->instanceId);
    GlobalUnlock((HANDLE)dnDevInst);
    return CR_SUCCESS;
}

/***********************************************************************
 *      SetupDiGetINFClassA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetINFClassA(PCSTR inf, LPGUID class_guid, PSTR class_name,
        DWORD size, PDWORD required_size)
{
    BOOL retval;
    DWORD required_sizeA, required_sizeW;
    PWSTR class_nameW = NULL;
    UNICODE_STRING infW;

    if (inf)
    {
        if (!RtlCreateUnicodeStringFromAsciiz(&infW, inf))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }
    else
        infW.Buffer = NULL;

    if (class_name && size)
    {
        if (!(class_nameW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR))))
        {
            RtlFreeUnicodeString(&infW);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }

    retval = SetupDiGetINFClassW(infW.Buffer, class_guid, class_nameW, size, &required_sizeW);

    if (retval)
    {
        required_sizeA = WideCharToMultiByte( CP_ACP, 0, class_nameW, required_sizeW,
                                              class_name, size, NULL, NULL);

        if(required_size) *required_size = required_sizeA;
    }
    else
        if(required_size) *required_size = required_sizeW;

    HeapFree(GetProcessHeap(), 0, class_nameW);
    RtlFreeUnicodeString(&infW);
    return retval;
}

/***********************************************************************
 *              SetupDiGetINFClassW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetINFClassW(PCWSTR inf, LPGUID class_guid, PWSTR class_name,
        DWORD size, PDWORD required_size)
{
    BOOL have_guid, have_name;
    DWORD dret;
    WCHAR buffer[MAX_PATH];

    if (!inf)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(inf))
    {
        FIXME("%s not found. Searching via DevicePath not implemented\n", debugstr_w(inf));
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    if (!class_guid || !class_name || !size)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!GetPrivateProfileStringW(Version, Signature, NULL, buffer, MAX_PATH, inf))
        return FALSE;

    if (lstrcmpiW(buffer, Chicago) && lstrcmpiW(buffer, WindowsNT))
        return FALSE;

    buffer[0] = '\0';
    have_guid = 0 < GetPrivateProfileStringW(Version, ClassGUID, NULL, buffer, MAX_PATH, inf);
    if (have_guid)
    {
        buffer[lstrlenW(buffer)-1] = 0;
        if (RPC_S_OK != UuidFromStringW(buffer + 1, class_guid))
        {
            FIXME("failed to convert \"%s\" into a guid\n", debugstr_w(buffer));
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    buffer[0] = '\0';
    dret = GetPrivateProfileStringW(Version, Class, NULL, buffer, MAX_PATH, inf);
    have_name = 0 < dret;

    if (dret >= MAX_PATH -1) FIXME("buffer might be too small\n");
    if (have_guid && !have_name) FIXME("class name lookup via guid not implemented\n");

    if (have_name)
    {
        if (dret < size) lstrcpyW(class_name, buffer);
        else
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            have_name = FALSE;
        }
    }

    if (required_size) *required_size = dret + ((dret) ? 1 : 0);

    return (have_guid || have_name);
}
