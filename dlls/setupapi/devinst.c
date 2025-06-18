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

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winreg.h"
#include "winternl.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winsvc.h"
#include "setupapi.h"
#include "softpub.h"
#include "mscat.h"
#include "shlwapi.h"
#include "wine/debug.h"
#include "wine/list.h"
#include "cfgmgr32.h"
#include "winioctl.h"
#include "rpc.h"
#include "rpcdce.h"
#include "cguid.h"

#include "setupapi_private.h"


WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

/* Unicode constants */
#ifdef __i386__
static const WCHAR NtPlatformExtension[] = L".NTx86";
#elif defined(__x86_64__)
static const WCHAR NtPlatformExtension[] = L".NTamd64";
#elif defined(__arm__)
static const WCHAR NtPlatformExtension[] = L".NTarm";
#elif defined(__aarch64__)
static const WCHAR NtPlatformExtension[] = L".NTarm64";
#endif

/* Registry key names */
static const WCHAR ControlClass[] = L"System\\CurrentControlSet\\Control\\Class";
static const WCHAR DeviceClasses[] = L"System\\CurrentControlSet\\Control\\DeviceClasses";
static const WCHAR Enum[] = L"System\\CurrentControlSet\\Enum";

#define SERVICE_CONTROL_REENUMERATE_ROOT_DEVICES 128

struct driver
{
    DWORD rank;
    WCHAR inf_path[MAX_PATH];
    WCHAR manufacturer[LINE_LEN];
    WCHAR mfg_key[LINE_LEN];
    WCHAR description[LINE_LEN];
    WCHAR section[LINE_LEN];
};

/* is used to identify if a DeviceInfoSet pointer is
valid or not */
#define SETUP_DEVICE_INFO_SET_MAGIC 0xd00ff056

struct DeviceInfoSet
{
    DWORD magic;        /* if is equal to SETUP_DEVICE_INFO_SET_MAGIC struct is okay */
    GUID ClassGuid;
    HWND hwndParent;
    struct list devices;
};

struct device
{
    struct DeviceInfoSet *set;
    HKEY                  key;
    BOOL                  phantom;
    WCHAR                *instanceId;
    struct list           interfaces;
    GUID                  class;
    DEVINST               devnode;
    struct list           entry;
    BOOL                  removed;
    SP_DEVINSTALL_PARAMS_W params;
    struct driver        *drivers;
    unsigned int          driver_count;
    struct driver        *selected_driver;
};

struct device_iface
{
    WCHAR           *refstr;
    WCHAR           *symlink;
    struct device   *device;
    GUID             class;
    DWORD            flags;
    HKEY             class_key;
    HKEY             refstr_key;
    struct list      entry;
};

bool array_reserve(void **elements, size_t *capacity, size_t count, size_t size)
{
    unsigned int new_capacity, max_capacity;
    void *new_elements;

    if (count <= *capacity)
        return true;

    max_capacity = ~(size_t)0 / size;
    if (count > max_capacity)
        return false;

    new_capacity = max(4, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = max_capacity;

    if (!(new_elements = realloc(*elements, new_capacity * size)))
        return false;

    *elements = new_elements;
    *capacity = new_capacity;

    return true;
}

static WCHAR *sprintf_path(const WCHAR *format, ...)
{
    va_list args, args_copy;
    WCHAR *buffer;
    size_t len;

    va_start(args, format);

    va_copy(args_copy, args);
    len = _vsnwprintf(NULL, 0, format, args_copy) + 1;
    va_end(args_copy);

    buffer = malloc(len * sizeof(WCHAR));
    _vsnwprintf(buffer, len, format, args);
    va_end(args);
    return buffer;
}

static WCHAR *concat_path(const WCHAR *root, const WCHAR *path)
{
    return sprintf_path(L"%s\\%s", root, path);
}

static struct DeviceInfoSet *get_device_set(HDEVINFO devinfo)
{
    struct DeviceInfoSet *set = devinfo;

    if (!devinfo || devinfo == INVALID_HANDLE_VALUE || set->magic != SETUP_DEVICE_INFO_SET_MAGIC)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }

    return set;
}

static struct device *get_device(HDEVINFO devinfo, const SP_DEVINFO_DATA *data)
{
    struct DeviceInfoSet *set;
    struct device *device;

    if (!(set = get_device_set(devinfo)))
        return FALSE;

    if (!data || data->cbSize != sizeof(*data) || !data->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    device = (struct device *)data->Reserved;

    if (device->set != set)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (device->removed)
    {
        SetLastError(ERROR_NO_SUCH_DEVINST);
        return NULL;
    }

    return device;
}

static struct device_iface *get_device_iface(HDEVINFO devinfo, const SP_DEVICE_INTERFACE_DATA *data)
{
    if (!get_device_set(devinfo))
        return FALSE;

    if (!data || data->cbSize != sizeof(*data) || !data->Reserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    return (struct device_iface *)data->Reserved;
}

static inline void copy_device_data(SP_DEVINFO_DATA *data, const struct device *device)
{
    data->ClassGuid = device->class;
    data->DevInst = device->devnode;
    data->Reserved = (ULONG_PTR)device;
}

static inline void copy_device_iface_data(SP_DEVICE_INTERFACE_DATA *data,
    const struct device_iface *iface)
{
    data->InterfaceClassGuid = iface->class;
    data->Flags = iface->flags;
    data->Reserved = (ULONG_PTR)iface;
}

static WCHAR **devinst_table;
static unsigned int devinst_table_size;

static DEVINST get_devinst_for_device_id(const WCHAR *id)
{
    unsigned int i;

    for (i = 0; i < devinst_table_size; ++i)
    {
        if (!devinst_table[i])
            break;
        if (!wcsicmp(devinst_table[i], id))
            return i;
    }
    return i;
}

static DEVINST alloc_devinst_for_device_id(const WCHAR *id)
{
    DEVINST ret;

    ret = get_devinst_for_device_id(id);
    if (ret == devinst_table_size)
    {
        if (devinst_table)
        {
            devinst_table = realloc(devinst_table, devinst_table_size * 2 * sizeof(*devinst_table));
            memset(devinst_table + devinst_table_size, 0, devinst_table_size * sizeof(*devinst_table));
            devinst_table_size *= 2;
        }
        else
        {
            devinst_table_size = 256;
            devinst_table = calloc(devinst_table_size, sizeof(*devinst_table));
        }
    }
    if (!devinst_table[ret])
        devinst_table[ret] = wcsdup(id);
    return ret;
}

static void SETUPDI_GuidToString(const GUID *guid, LPWSTR guidStr)
{
    swprintf(guidStr, 39, L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        guid->Data1, guid->Data2, guid->Data3,
        guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
        guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
}

static WCHAR *get_iface_key_path(struct device_iface *iface)
{
    WCHAR *path, *ptr;
    size_t len = lstrlenW(DeviceClasses) + 1 + 38 + 1 + lstrlenW(iface->symlink);

    if (!(path = malloc((len + 1) * sizeof(WCHAR))))
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }

    lstrcpyW(path, DeviceClasses);
    lstrcatW(path, L"\\");
    SETUPDI_GuidToString(&iface->class, path + lstrlenW(path));
    lstrcatW(path, L"\\");
    ptr = path + lstrlenW(path);
    lstrcatW(path, iface->symlink);
    if (lstrlenW(iface->symlink) > 3)
        ptr[0] = ptr[1] = ptr[3] = '#';

    ptr = wcschr(ptr, '\\');
    if (ptr) *ptr = 0;

    return path;
}

static WCHAR *get_refstr_key_path(struct device_iface *iface)
{
    WCHAR *path, *ptr;
    size_t len = lstrlenW(DeviceClasses) + 1 + 38 + 1 + lstrlenW(iface->symlink) + 1 + 1;

    if (iface->refstr)
        len += lstrlenW(iface->refstr);

    if (!(path = malloc((len + 1) * sizeof(WCHAR))))
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }

    lstrcpyW(path, DeviceClasses);
    lstrcatW(path, L"\\");
    SETUPDI_GuidToString(&iface->class, path + lstrlenW(path));
    lstrcatW(path, L"\\");
    ptr = path + lstrlenW(path);
    lstrcatW(path, iface->symlink);
    if (lstrlenW(iface->symlink) > 3)
        ptr[0] = ptr[1] = ptr[3] = '#';

    ptr = wcschr(ptr, '\\');
    if (ptr) *ptr = 0;

    lstrcatW(path, L"\\#");

    if (iface->refstr)
        lstrcatW(path, iface->refstr);

    return path;
}

static BOOL is_valid_property_type(DEVPROPTYPE prop_type)
{
    DWORD type = prop_type & DEVPROP_MASK_TYPE;
    DWORD typemod = prop_type & DEVPROP_MASK_TYPEMOD;

    if (type > MAX_DEVPROP_TYPE)
        return FALSE;
    if (typemod > MAX_DEVPROP_TYPEMOD)
        return FALSE;

    if (typemod == DEVPROP_TYPEMOD_ARRAY
        && (type == DEVPROP_TYPE_EMPTY || type == DEVPROP_TYPE_NULL || type == DEVPROP_TYPE_STRING
            || type == DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING))
        return FALSE;

    if (typemod == DEVPROP_TYPEMOD_LIST
        && !(type == DEVPROP_TYPE_STRING || type == DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING))
        return FALSE;

    return TRUE;
}

static LPWSTR SETUPDI_CreateSymbolicLinkPath(LPCWSTR instanceId,
        const GUID *InterfaceClassGuid, LPCWSTR ReferenceString)
{
    static const WCHAR fmt[] = L"\\\\?\\%s#%s";
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
    ret = malloc(len * sizeof(WCHAR));
    if (ret)
    {
        int printed = swprintf(ret, len, fmt, instanceId, guidStr);
        LPWSTR ptr;

        /* replace '\\' with '#' after the "\\\\?\\" beginning */
        for (ptr = wcschr(ret + 4, '\\'); ptr; ptr = wcschr(ptr + 1, '\\'))
            *ptr = '#';
        if (ReferenceString && *ReferenceString)
        {
            ret[printed] = '\\';
            lstrcpyW(ret + printed + 1, ReferenceString);
        }
    }

    CharLowerW(ret);

    return ret;
}

static BOOL is_linked(HKEY key)
{
    DWORD linked, type, size;
    HKEY control_key;
    BOOL ret = FALSE;

    if (!RegOpenKeyW(key, L"Control", &control_key))
    {
        size = sizeof(DWORD);
        if (!RegQueryValueExW(control_key, L"Linked", NULL, &type, (BYTE *)&linked, &size)
                && type == REG_DWORD && linked)
            ret = TRUE;

        RegCloseKey(control_key);
    }

    return ret;
}

static struct device_iface *SETUPDI_CreateDeviceInterface(struct device *device,
        const GUID *class, const WCHAR *refstr)
{
    struct device_iface *iface = NULL;
    WCHAR *refstr2 = NULL, *symlink = NULL, *path = NULL;
    HKEY key;
    LONG ret;

    TRACE("%p %s %s\n", device, debugstr_guid(class), debugstr_w(refstr));

    /* check if it already exists */
    LIST_FOR_EACH_ENTRY(iface, &device->interfaces, struct device_iface, entry)
    {
        if (IsEqualGUID(&iface->class, class) && !lstrcmpiW(iface->refstr, refstr))
            return iface;
    }

    iface = malloc(sizeof(*iface));
    symlink = SETUPDI_CreateSymbolicLinkPath(device->instanceId, class, refstr);

    if (!iface || !symlink)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto err;
    }

    if (refstr && !(refstr2 = wcsdup(refstr)))
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto err;
    }
    iface->refstr = refstr2;
    iface->symlink = symlink;
    iface->device = device;
    iface->class = *class;
    iface->flags = 0;

    if (!(path = get_iface_key_path(iface)))
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto err;
    }

    if ((ret = RegCreateKeyW(HKEY_LOCAL_MACHINE, path, &key)))
    {
        SetLastError(ret);
        goto err;
    }
    RegSetValueExW(key, L"DeviceInstance", 0, REG_SZ, (BYTE *)device->instanceId,
        lstrlenW(device->instanceId) * sizeof(WCHAR));
    free(path);

    iface->class_key = key;

    if (!(path = get_refstr_key_path(iface)))
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto err;
    }

    if ((ret = RegCreateKeyW(HKEY_LOCAL_MACHINE, path, &key)))
    {
        SetLastError(ret);
        goto err;
    }
    RegSetValueExW(key, L"SymbolicLink", 0, REG_SZ, (BYTE *)iface->symlink,
        lstrlenW(iface->symlink) * sizeof(WCHAR));

    if (is_linked(key))
        iface->flags |= SPINT_ACTIVE;

    free(path);

    iface->refstr_key = key;

    list_add_tail(&device->interfaces, &iface->entry);
    return iface;

err:
    free(iface);
    free(refstr2);
    free(symlink);
    free(path);
    return NULL;
}

static BOOL SETUPDI_SetInterfaceSymbolicLink(struct device_iface *iface,
    const WCHAR *symlink)
{
    free(iface->symlink);
    if ((iface->symlink = wcsdup(symlink)))
        return TRUE;
    return FALSE;
}

static HKEY SETUPDI_CreateDevKey(struct device *device)
{
    HKEY enumKey, key = INVALID_HANDLE_VALUE;
    LONG l;

    l = RegCreateKeyExW(HKEY_LOCAL_MACHINE, Enum, 0, NULL, 0, KEY_ALL_ACCESS,
            NULL, &enumKey, NULL);
    if (!l)
    {
        RegCreateKeyExW(enumKey, device->instanceId, 0, NULL, 0,
                KEY_READ | KEY_WRITE, NULL, &key, NULL);
        RegCloseKey(enumKey);
    }
    return key;
}

static LONG open_driver_key(struct device *device, REGSAM access, HKEY *key)
{
    HKEY class_key;
    WCHAR path[50];
    DWORD size = sizeof(path);
    LONG l;

    if ((l = RegCreateKeyExW(HKEY_LOCAL_MACHINE, ControlClass, 0, NULL, 0,
            KEY_CREATE_SUB_KEY, NULL, &class_key, NULL)))
    {
        ERR("Failed to open driver class root key, error %lu.\n", l);
        return l;
    }

    if (!(l = RegGetValueW(device->key, NULL, L"Driver", RRF_RT_REG_SZ, NULL, path, &size)))
    {
        if (!(l = RegOpenKeyExW(class_key, path, 0, access, key)))
        {
            RegCloseKey(class_key);
            return l;
        }
        TRACE("Failed to open driver key, error %lu.\n", l);
    }

    RegCloseKey(class_key);
    return l;
}

static LONG create_driver_key(struct device *device, HKEY *key)
{
    unsigned int i = 0;
    WCHAR path[50];
    HKEY class_key;
    DWORD dispos;
    LONG l;

    if (!open_driver_key(device, KEY_READ | KEY_WRITE, key))
        return ERROR_SUCCESS;

    if ((l = RegCreateKeyExW(HKEY_LOCAL_MACHINE, ControlClass, 0, NULL, 0,
            KEY_CREATE_SUB_KEY, NULL, &class_key, NULL)))
    {
        ERR("Failed to open driver class root key, error %lu.\n", l);
        return l;
    }

    SETUPDI_GuidToString(&device->class, path);
    lstrcatW(path, L"\\");
    /* Allocate a new driver key, by finding the first integer value that's not
     * already taken. */
    for (;;)
    {
        swprintf(path + 39, ARRAY_SIZE(path) - 39, L"%04u", i++);
        if ((l = RegCreateKeyExW(class_key, path, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, key, &dispos)))
            break;
        else if (dispos == REG_CREATED_NEW_KEY)
        {
            RegSetValueExW(device->key, L"Driver", 0, REG_SZ, (BYTE *)path, lstrlenW(path) * sizeof(WCHAR));
            RegCloseKey(class_key);
            return ERROR_SUCCESS;
        }
        RegCloseKey(*key);
    }
    ERR("Failed to create driver key, error %lu.\n", l);
    RegCloseKey(class_key);
    return l;
}

static LONG delete_driver_key(struct device *device)
{
    HKEY key;
    LONG l;

    if (!(l = open_driver_key(device, KEY_READ | KEY_WRITE, &key)))
    {
        l = RegDeleteKeyW(key, L"");
        RegCloseKey(key);
    }

    return l;
}

struct PropertyMapEntry
{
    DWORD   regType;
    LPCSTR  nameA;
    LPCWSTR nameW;
};

#define PROPERTY_MAP_ENTRY(type, name) { type, name, L##name }
static const struct PropertyMapEntry PropertyMap[] = {
    PROPERTY_MAP_ENTRY(REG_SZ, "DeviceDesc"),
    PROPERTY_MAP_ENTRY(REG_MULTI_SZ, "HardwareId"),
    PROPERTY_MAP_ENTRY(REG_MULTI_SZ, "CompatibleIDs"),
    { 0, NULL, NULL }, /* SPDRP_UNUSED0 */
    PROPERTY_MAP_ENTRY(REG_SZ, "Service"),
    { 0, NULL, NULL }, /* SPDRP_UNUSED1 */
    { 0, NULL, NULL }, /* SPDRP_UNUSED2 */
    PROPERTY_MAP_ENTRY(REG_SZ, "Class"),
    PROPERTY_MAP_ENTRY(REG_SZ, "ClassGUID"),
    PROPERTY_MAP_ENTRY(REG_SZ, "Driver"),
    PROPERTY_MAP_ENTRY(REG_DWORD, "ConfigFlags"),
    PROPERTY_MAP_ENTRY(REG_SZ, "Mfg"),
    PROPERTY_MAP_ENTRY(REG_SZ, "FriendlyName"),
    PROPERTY_MAP_ENTRY(REG_SZ, "LocationInformation"),
    { 0, NULL, NULL }, /* SPDRP_PHYSICAL_DEVICE_OBJECT_NAME */
    PROPERTY_MAP_ENTRY(REG_DWORD, "Capabilities"),
    PROPERTY_MAP_ENTRY(REG_DWORD, "UINumber"),
    PROPERTY_MAP_ENTRY(REG_MULTI_SZ, "UpperFilters"),
    PROPERTY_MAP_ENTRY(REG_MULTI_SZ, "LowerFilters"),
    [SPDRP_BASE_CONTAINERID] = PROPERTY_MAP_ENTRY(REG_SZ, "ContainerId"),
};
#undef PROPERTY_MAP_ENTRY

static BOOL SETUPDI_SetDeviceRegistryPropertyW(struct device *device,
    DWORD prop, const BYTE *buffer, DWORD size)
{
    if (prop < ARRAY_SIZE(PropertyMap) && PropertyMap[prop].nameW)
    {
        LONG ret = RegSetValueExW(device->key, PropertyMap[prop].nameW, 0,
                PropertyMap[prop].regType, buffer, size);
        if (!ret)
            return TRUE;

        SetLastError(ret);
    }
    return FALSE;
}

static void remove_device_iface(struct device_iface *iface)
{
    RegDeleteTreeW(iface->refstr_key, NULL);
    RegDeleteKeyW(iface->refstr_key, L"");
    RegCloseKey(iface->refstr_key);
    iface->refstr_key = NULL;
    /* Also remove the class key if it's empty. */
    RegDeleteKeyW(iface->class_key, L"");
    RegCloseKey(iface->class_key);
    iface->class_key = NULL;
    iface->flags |= SPINT_REMOVED;
}

static void delete_device_iface(struct device_iface *iface)
{
    list_remove(&iface->entry);
    RegCloseKey(iface->refstr_key);
    RegCloseKey(iface->class_key);
    free(iface->refstr);
    free(iface->symlink);
    free(iface);
}

/* remove all interfaces associated with the device, including those not
 * enumerated in the set */
static void remove_all_device_ifaces(struct device *device)
{
    HKEY classes_key;
    DWORD i, len;
    LONG ret;

    if ((ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, DeviceClasses, 0, KEY_READ, &classes_key)))
    {
        WARN("Failed to open classes key, error %lu.\n", ret);
        return;
    }

    for (i = 0; ; ++i)
    {
        WCHAR class_name[40];
        HKEY class_key;
        DWORD j;

        len = ARRAY_SIZE(class_name);
        if ((ret = RegEnumKeyExW(classes_key, i, class_name, &len, NULL, NULL, NULL, NULL)))
        {
            if (ret != ERROR_NO_MORE_ITEMS) ERR("Failed to enumerate classes, error %lu.\n", ret);
            break;
        }

        if ((ret = RegOpenKeyExW(classes_key, class_name, 0, KEY_READ, &class_key)))
        {
            ERR("Failed to open class %s, error %lu.\n", debugstr_w(class_name), ret);
            continue;
        }

        for (j = 0; ; ++j)
        {
            WCHAR iface_name[MAX_DEVICE_ID_LEN + 39], device_name[MAX_DEVICE_ID_LEN];
            HKEY iface_key;

            len = ARRAY_SIZE(iface_name);
            if ((ret = RegEnumKeyExW(class_key, j, iface_name, &len, NULL, NULL, NULL, NULL)))
            {
                if (ret != ERROR_NO_MORE_ITEMS) ERR("Failed to enumerate interfaces, error %lu.\n", ret);
                break;
            }

            if ((ret = RegOpenKeyExW(class_key, iface_name, 0, KEY_ALL_ACCESS, &iface_key)))
            {
                ERR("Failed to open interface %s, error %lu.\n", debugstr_w(iface_name), ret);
                continue;
            }

            len = sizeof(device_name);
            if ((ret = RegQueryValueExW(iface_key, L"DeviceInstance", NULL, NULL, (BYTE *)device_name, &len)))
            {
                ERR("Failed to query device instance, error %lu.\n", ret);
                RegCloseKey(iface_key);
                continue;
            }

            if (!wcsicmp(device_name, device->instanceId))
            {
                if ((ret = RegDeleteTreeW(iface_key, NULL)))
                    ERR("Failed to delete interface %s subkeys, error %lu.\n", debugstr_w(iface_name), ret);
                if ((ret = RegDeleteKeyW(iface_key, L"")))
                    ERR("Failed to delete interface %s, error %lu.\n", debugstr_w(iface_name), ret);
            }

            RegCloseKey(iface_key);
        }
        RegCloseKey(class_key);
    }

    RegCloseKey(classes_key);
}

static void remove_device(struct device *device)
{
    WCHAR id[MAX_DEVICE_ID_LEN], *p;
    struct device_iface *iface;
    HKEY enum_key;

    delete_driver_key(device);

    LIST_FOR_EACH_ENTRY(iface, &device->interfaces, struct device_iface, entry)
    {
        remove_device_iface(iface);
    }

    RegDeleteTreeW(device->key, NULL);
    RegDeleteKeyW(device->key, L"");

    /* delete all empty parents of the key */
    if (!RegOpenKeyExW(HKEY_LOCAL_MACHINE, Enum, 0, KEY_ENUMERATE_SUB_KEYS, &enum_key))
    {
        lstrcpyW(id, device->instanceId);

        while ((p = wcsrchr(id, '\\')))
        {
            *p = 0;
            RegDeleteKeyW(enum_key, id);
        }

        RegCloseKey(enum_key);
    }

    RegCloseKey(device->key);
    device->key = NULL;
    device->removed = TRUE;
}

static void delete_device(struct device *device)
{
    struct device_iface *iface, *next;
    SP_DEVINFO_DATA device_data;

    device_data.cbSize = sizeof(device_data);
    copy_device_data(&device_data, device);
    SetupDiCallClassInstaller(DIF_DESTROYPRIVATEDATA, device->set, &device_data);

    if (device->phantom)
    {
        remove_device(device);
        remove_all_device_ifaces(device);
    }

    RegCloseKey(device->key);
    free(device->instanceId);
    free(device->drivers);

    LIST_FOR_EACH_ENTRY_SAFE(iface, next, &device->interfaces,
            struct device_iface, entry)
    {
        delete_device_iface(iface);
    }
    list_remove(&device->entry);
    free(device);
}

/* Create a new device, or return a device already in the set. */
static struct device *create_device(struct DeviceInfoSet *set,
    const GUID *class, const WCHAR *instanceid, BOOL phantom)
{
    const DWORD one = 1;
    struct device *device;
    WCHAR guidstr[MAX_GUID_STRING_LEN];
    WCHAR class_name[MAX_CLASS_NAME_LEN];
    DWORD size;

    TRACE("%p, %s, %s, %d\n", set, debugstr_guid(class),
        debugstr_w(instanceid), phantom);

    LIST_FOR_EACH_ENTRY(device, &set->devices, struct device, entry)
    {
        if (!wcsicmp(instanceid, device->instanceId))
        {
            TRACE("Found device %p already in set.\n", device);
            return device;
        }
    }

    if (!(device = calloc(1, sizeof(*device))))
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }

    if (!(device->instanceId = wcsdup(instanceid)))
    {
        SetLastError(ERROR_OUTOFMEMORY);
        free(device);
        return NULL;
    }

    wcsupr(device->instanceId);
    device->set = set;
    device->key = SETUPDI_CreateDevKey(device);
    device->phantom = phantom;
    list_init(&device->interfaces);
    device->class = *class;
    device->devnode = alloc_devinst_for_device_id(device->instanceId);
    device->removed = FALSE;
    list_add_tail(&set->devices, &device->entry);
    device->params.cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);

    if (phantom)
        RegSetValueExW(device->key, L"Phantom", 0, REG_DWORD, (const BYTE *)&one, sizeof(one));

    SETUPDI_GuidToString(class, guidstr);
    SETUPDI_SetDeviceRegistryPropertyW(device, SPDRP_CLASSGUID,
        (const BYTE *)guidstr, sizeof(guidstr));

    if (SetupDiClassNameFromGuidW(class, class_name, ARRAY_SIZE(class_name), NULL))
    {
        size = (lstrlenW(class_name) + 1) * sizeof(WCHAR);
        SETUPDI_SetDeviceRegistryPropertyW(device, SPDRP_CLASS, (const BYTE *)class_name, size);
    }

    TRACE("Created new device %p.\n", device);
    return device;
}

static struct device *get_devnode_device(DEVINST devnode, HDEVINFO *set)
{
    SP_DEVINFO_DATA data = { sizeof(data) };

    *set = NULL;
    if (devnode >= devinst_table_size || !devinst_table[devnode])
    {
        WARN("device node %lu not found\n", devnode);
        return NULL;
    }

    *set = SetupDiCreateDeviceInfoListExW(NULL, NULL, NULL, NULL);
    if (*set == INVALID_HANDLE_VALUE) return NULL;
    if (!SetupDiOpenDeviceInfoW(*set, devinst_table[devnode], NULL, 0, &data))
    {
        SetupDiDestroyDeviceInfoList(*set);
        *set = NULL;
        return NULL;
    }
    return get_device(*set, &data);
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
	TRACE("RegEnumKeyExW() returns %ld\n", lError);
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
				  L"NoUseClass",
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
				   L"NoInstallClass",
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
				   L"NoDisplayClass",
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
        dwLength = ARRAY_SIZE(szKeyName);
	lError = RegEnumKeyExW(hClassesKey,
			       dwIndex,
			       szKeyName,
			       &dwLength,
			       NULL,
			       NULL,
			       NULL,
			       NULL);
	TRACE("RegEnumKeyExW() returns %ld\n", lError);
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

	    dwLength = sizeof(szClassName);
	    if (!RegQueryValueExW(hClassKey,
				  L"Class",
				  NULL,
				  NULL,
				  (LPBYTE)szClassName,
				  &dwLength))
	    {
		TRACE("Class name: %p\n", szClassName);

		if (wcsicmp(szClassName, ClassName) == 0)
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
			     L"Class",
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
			 L"Class",
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
 *   MachineName [I] name of machine to create empty DeviceInfoSet list, if NULL
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

    list = malloc(size);
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

    TRACE("%p %p %ld %ld %ld %p %s\n", DeviceInfoSet, DeviceInfoData, Scope,
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
HKEY WINAPI SetupDiCreateDevRegKeyW(HDEVINFO devinfo, SP_DEVINFO_DATA *device_data, DWORD Scope,
        DWORD HwProfile, DWORD KeyType, HINF InfHandle, const WCHAR *InfSectionName)
{
    struct device *device;
    HKEY key = INVALID_HANDLE_VALUE;
    LONG l;

    TRACE("devinfo %p, device_data %p, scope %ld, profile %ld, type %ld, inf_handle %p, inf_section %s.\n",
            devinfo, device_data, Scope, HwProfile, KeyType, InfHandle, debugstr_w(InfSectionName));

    if (!(device = get_device(devinfo, device_data)))
        return INVALID_HANDLE_VALUE;

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
    if (device->phantom)
    {
        SetLastError(ERROR_DEVINFO_NOT_REGISTERED);
        return INVALID_HANDLE_VALUE;
    }
    if (Scope != DICS_FLAG_GLOBAL)
        FIXME("unimplemented for scope %ld\n", Scope);
    switch (KeyType)
    {
        case DIREG_DEV:
            l = RegCreateKeyExW(device->key, L"Device Parameters", 0, NULL, 0,
                    KEY_READ | KEY_WRITE, NULL, &key, NULL);
            break;
        case DIREG_DRV:
            l = create_driver_key(device, &key);
            break;
        default:
            FIXME("Unhandled type %#lx.\n", KeyType);
            l = ERROR_CALL_NOT_IMPLEMENTED;
    }
    if (InfHandle)
        SetupInstallFromInfSectionW(NULL, InfHandle, InfSectionName, SPINST_ALL,
                NULL, NULL, SP_COPY_NEWER_ONLY, NULL, NULL, devinfo, device_data);
    SetLastError(l);
    return l ? INVALID_HANDLE_VALUE : key;
}

/***********************************************************************
 *              SetupDiCreateDeviceInfoA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiCreateDeviceInfoA(HDEVINFO DeviceInfoSet, const char *name,
        const GUID *ClassGuid, PCSTR DeviceDescription, HWND hwndParent, DWORD CreationFlags,
        PSP_DEVINFO_DATA DeviceInfoData)
{
    WCHAR nameW[MAX_DEVICE_ID_LEN];
    BOOL ret = FALSE;
    LPWSTR DeviceDescriptionW = NULL;

    if (!name || strlen(name) >= MAX_DEVICE_ID_LEN)
    {
        SetLastError(ERROR_INVALID_DEVINST_NAME);
        return FALSE;
    }

    MultiByteToWideChar(CP_ACP, 0, name, -1, nameW, ARRAY_SIZE(nameW));

    if (DeviceDescription)
    {
        DeviceDescriptionW = MultiByteToUnicode(DeviceDescription, CP_ACP);
        if (DeviceDescriptionW == NULL)
            return FALSE;
    }

    ret = SetupDiCreateDeviceInfoW(DeviceInfoSet, nameW, ClassGuid, DeviceDescriptionW,
            hwndParent, CreationFlags, DeviceInfoData);

    MyFree(DeviceDescriptionW);

    return ret;
}

/***********************************************************************
 *              SetupDiCreateDeviceInfoW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiCreateDeviceInfoW(HDEVINFO devinfo, const WCHAR *name, const GUID *class,
        const WCHAR *description, HWND parent, DWORD flags, SP_DEVINFO_DATA *device_data)
{
    WCHAR id[MAX_DEVICE_ID_LEN];
    struct DeviceInfoSet *set;
    HKEY enum_hkey;
    HKEY instance_hkey;
    struct device *device;
    LONG l;

    TRACE("devinfo %p, name %s, class %s, description %s, hwnd %p, flags %#lx, device_data %p.\n",
            devinfo, debugstr_w(name), debugstr_guid(class), debugstr_w(description),
            parent, flags, device_data);

    if (!name || lstrlenW(name) >= MAX_DEVICE_ID_LEN)
    {
        SetLastError(ERROR_INVALID_DEVINST_NAME);
        return FALSE;
    }

    if (!(set = get_device_set(devinfo)))
        return FALSE;

    if (!class)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!IsEqualGUID(&set->ClassGuid, &GUID_NULL) && !IsEqualGUID(class, &set->ClassGuid))
    {
        SetLastError(ERROR_CLASS_MISMATCH);
        return FALSE;
    }
    if ((flags & DICD_GENERATE_ID))
    {
        unsigned int instance_id;

        if (wcschr(name, '\\'))
        {
            SetLastError(ERROR_INVALID_DEVINST_NAME);
            return FALSE;
        }

        for (instance_id = 0; ; ++instance_id)
        {
            if (swprintf(id, ARRAY_SIZE(id), L"ROOT\\%s\\%04u", name, instance_id) == -1)
            {
                SetLastError(ERROR_INVALID_DEVINST_NAME);
                return FALSE;
            }

            RegCreateKeyExW(HKEY_LOCAL_MACHINE, Enum, 0, NULL, 0, KEY_READ, NULL, &enum_hkey, NULL);
            if (!(l = RegOpenKeyExW(enum_hkey, id, 0, KEY_READ, &instance_hkey)))
                RegCloseKey(instance_hkey);
            if (l == ERROR_FILE_NOT_FOUND)
                break;
            RegCloseKey(enum_hkey);
        }
    }
    else
    {
        /* Check if instance is already in registry */
        RegCreateKeyExW(HKEY_LOCAL_MACHINE, Enum, 0, NULL, 0, KEY_READ, NULL, &enum_hkey, NULL);
        if (!RegOpenKeyExW(enum_hkey, name, 0, KEY_READ, &instance_hkey))
        {
            RegCloseKey(instance_hkey);
            RegCloseKey(enum_hkey);
            SetLastError(ERROR_DEVINST_ALREADY_EXISTS);
            return FALSE;
        }
        RegCloseKey(enum_hkey);

        /* Check if instance is already in set */
        lstrcpyW(id, name);
        LIST_FOR_EACH_ENTRY(device, &set->devices, struct device, entry)
        {
            if (!lstrcmpiW(name, device->instanceId))
            {
                SetLastError(ERROR_DEVINST_ALREADY_EXISTS);
                return FALSE;
            }
        }
    }

    if (!(device = create_device(set, class, id, TRUE)))
        return FALSE;

    if (description)
    {
        SETUPDI_SetDeviceRegistryPropertyW(device, SPDRP_DEVICEDESC,
                (const BYTE *)description, lstrlenW(description) * sizeof(WCHAR));
    }

    if (device_data)
    {
        if (device_data->cbSize != sizeof(SP_DEVINFO_DATA))
        {
            SetLastError(ERROR_INVALID_USER_BUFFER);
            return FALSE;
        }
        else
            copy_device_data(device_data, device);
    }

    return TRUE;
}

/***********************************************************************
 *		SetupDiRegisterDeviceInfo (SETUPAPI.@)
 */
BOOL WINAPI SetupDiRegisterDeviceInfo(HDEVINFO devinfo, SP_DEVINFO_DATA *device_data, DWORD flags,
        PSP_DETSIG_CMPPROC compare_proc, void *context, SP_DEVINFO_DATA *duplicate_data)
{
    struct device *device;

    TRACE("devinfo %p, data %p, flags %#lx, compare_proc %p, context %p, duplicate_data %p.\n",
            devinfo, device_data, flags, compare_proc, context, duplicate_data);

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

    if (device->phantom)
    {
        device->phantom = FALSE;
        RegDeleteValueW(device->key, L"Phantom");
    }
    return TRUE;
}

/***********************************************************************
 *              SetupDiRemoveDevice (SETUPAPI.@)
 */
BOOL WINAPI SetupDiRemoveDevice(HDEVINFO devinfo, SP_DEVINFO_DATA *device_data)
{
    SC_HANDLE manager = NULL, service = NULL;
    struct device *device;
    WCHAR *service_name = NULL;
    DWORD size;

    TRACE("devinfo %p, device_data %p.\n", devinfo, device_data);

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

    if (!(manager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT)))
        return FALSE;

    if (!RegGetValueW(device->key, NULL, L"Service", RRF_RT_REG_SZ, NULL, NULL, &size))
    {
        service_name = malloc(size);
        if (!RegGetValueW(device->key, NULL, L"Service", RRF_RT_REG_SZ, NULL, service_name, &size))
            service = OpenServiceW(manager, service_name, SERVICE_USER_DEFINED_CONTROL);
    }

    remove_device(device);

    if (service)
    {
        SERVICE_STATUS status;
        if (!ControlService(service, SERVICE_CONTROL_REENUMERATE_ROOT_DEVICES, &status))
            ERR("Failed to control service %s, error %lu.\n", debugstr_w(service_name), GetLastError());
        CloseServiceHandle(service);
    }
    CloseServiceHandle(manager);

    free(service_name);

    remove_all_device_ifaces(device);

    return TRUE;
}

/***********************************************************************
 *              SetupDiDeleteDeviceInfo (SETUPAPI.@)
 */
BOOL WINAPI SetupDiDeleteDeviceInfo(HDEVINFO devinfo, SP_DEVINFO_DATA *device_data)
{
    struct device *device;

    TRACE("devinfo %p, device_data %p.\n", devinfo, device_data);

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

    delete_device(device);

    return TRUE;
}

/***********************************************************************
 *              SetupDiRemoveDeviceInterface (SETUPAPI.@)
 */
BOOL WINAPI SetupDiRemoveDeviceInterface(HDEVINFO devinfo, SP_DEVICE_INTERFACE_DATA *iface_data)
{
    struct device_iface *iface;

    TRACE("devinfo %p, iface_data %p.\n", devinfo, iface_data);

    if (!(iface = get_device_iface(devinfo, iface_data)))
        return FALSE;

    remove_device_iface(iface);

    return TRUE;
}

/***********************************************************************
 *              SetupDiDeleteDeviceInterfaceData (SETUPAPI.@)
 */
BOOL WINAPI SetupDiDeleteDeviceInterfaceData(HDEVINFO devinfo, SP_DEVICE_INTERFACE_DATA *iface_data)
{
    struct device_iface *iface;

    TRACE("devinfo %p, iface_data %p.\n", devinfo, iface_data);

    if (!(iface = get_device_iface(devinfo, iface_data)))
        return FALSE;

    delete_device_iface(iface);

    return TRUE;
}

/***********************************************************************
 *		SetupDiEnumDeviceInfo (SETUPAPI.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetupDiEnumDeviceInfo(HDEVINFO devinfo, DWORD index, SP_DEVINFO_DATA *device_data)
{
    struct DeviceInfoSet *set;
    struct device *device;
    DWORD i = 0;

    TRACE("devinfo %p, index %ld, device_data %p\n", devinfo, index, device_data);

    if (!(set = get_device_set(devinfo)))
        return FALSE;

    if (!device_data)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (device_data->cbSize != sizeof(SP_DEVINFO_DATA))
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    LIST_FOR_EACH_ENTRY(device, &set->devices, struct device, entry)
    {
        if (i++ == index)
        {
            copy_device_data(device_data, device);
            return TRUE;
        }
    }

    SetLastError(ERROR_NO_MORE_ITEMS);
    return FALSE;
}

/***********************************************************************
 *		SetupDiGetDeviceInstanceIdA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInstanceIdA(HDEVINFO devinfo, SP_DEVINFO_DATA *device_data,
        char *id, DWORD size, DWORD *needed)
{
    WCHAR idW[MAX_DEVICE_ID_LEN];

    TRACE("devinfo %p, device_data %p, id %p, size %ld, needed %p.\n",
            devinfo, device_data, id, size, needed);

    if (!SetupDiGetDeviceInstanceIdW(devinfo, device_data, idW, ARRAY_SIZE(idW), NULL))
        return FALSE;

    if (needed)
        *needed = WideCharToMultiByte(CP_ACP, 0, idW, -1, NULL, 0, NULL, NULL);

    if (size && WideCharToMultiByte(CP_ACP, 0, idW, -1, id, size, NULL, NULL))
        return TRUE;

    SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return FALSE;
}

/***********************************************************************
 *		SetupDiGetDeviceInstanceIdW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInstanceIdW(HDEVINFO devinfo, SP_DEVINFO_DATA *device_data,
        WCHAR *DeviceInstanceId, DWORD DeviceInstanceIdSize, DWORD *RequiredSize)
{
    struct device *device;

    TRACE("devinfo %p, device_data %p, DeviceInstanceId %p, DeviceInstanceIdSize %ld, RequiredSize %p.\n",
            devinfo, device_data, DeviceInstanceId, DeviceInstanceIdSize, RequiredSize);

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

    TRACE("instance ID: %s\n", debugstr_w(device->instanceId));
    if (DeviceInstanceIdSize < lstrlenW(device->instanceId) + 1)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        if (RequiredSize)
            *RequiredSize = lstrlenW(device->instanceId) + 1;
        return FALSE;
    }
    lstrcpyW(DeviceInstanceId, device->instanceId);
    if (RequiredSize)
        *RequiredSize = lstrlenW(device->instanceId) + 1;
    return TRUE;
}

/***********************************************************************
 *              SetupDiGetActualSectionToInstallExA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetActualSectionToInstallExA(HINF hinf, const char *section, SP_ALTPLATFORM_INFO *altplatform,
        char *section_ext, DWORD size, DWORD *needed, char **extptr, void *reserved)
{
    WCHAR sectionW[LINE_LEN], section_extW[LINE_LEN], *extptrW;
    BOOL ret;

    MultiByteToWideChar(CP_ACP, 0, section, -1, sectionW, ARRAY_SIZE(sectionW));

    ret = SetupDiGetActualSectionToInstallExW(hinf, sectionW, altplatform, section_extW,
            ARRAY_SIZE(section_extW), NULL, &extptrW, reserved);
    if (ret)
    {
        if (needed)
            *needed = WideCharToMultiByte(CP_ACP, 0, section_extW, -1, NULL, 0, NULL, NULL);

        if (section_ext)
            ret = !!WideCharToMultiByte(CP_ACP, 0, section_extW, -1, section_ext, size, NULL, NULL);

        if (extptr)
        {
            if (extptrW)
                *extptr = section_ext + WideCharToMultiByte(CP_ACP, 0, section_extW,
                        extptrW - section_extW, NULL, 0, NULL, NULL);
            else
                *extptr = NULL;
        }
    }

    return ret;
}

/***********************************************************************
 *              SetupDiGetActualSectionToInstallA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetActualSectionToInstallA(HINF hinf, const char *section, char *section_ext,
        DWORD size, DWORD *needed, char **extptr)
{
    return SetupDiGetActualSectionToInstallExA(hinf, section, NULL, section_ext, size,
        needed, extptr, NULL);
}

/***********************************************************************
 *              SetupDiGetActualSectionToInstallExW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetActualSectionToInstallExW(HINF hinf, const WCHAR *section, SP_ALTPLATFORM_INFO *altplatform,
        WCHAR *section_ext, DWORD size, DWORD *needed, WCHAR **extptr, void *reserved)
{
    WCHAR buffer[MAX_PATH];
    DWORD len;
    DWORD full_len;
    LONG line_count = -1;

    TRACE("hinf %p, section %s, altplatform %p, ext %p, size %ld, needed %p, extptr %p, reserved %p.\n",
            hinf, debugstr_w(section), altplatform, section_ext, size, needed, extptr, reserved);

    if (altplatform)
        FIXME("SP_ALTPLATFORM_INFO unsupported\n");

    lstrcpyW(buffer, section);
    len = lstrlenW(buffer);

    if (OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        /* Test section name with '.NTx86' extension */
        lstrcpyW(&buffer[len], NtPlatformExtension);
        line_count = SetupGetLineCountW(hinf, buffer);

        if (line_count == -1)
        {
            /* Test section name with '.NT' extension */
            lstrcpyW(&buffer[len], L".NT");
            line_count = SetupGetLineCountW(hinf, buffer);
        }
    }
    else
    {
        /* Test section name with '.Win' extension */
        lstrcpyW(&buffer[len], L".Win");
        line_count = SetupGetLineCountW(hinf, buffer);
    }

    if (line_count == -1)
        buffer[len] = 0;

    full_len = lstrlenW(buffer);

    if (section_ext != NULL && size != 0)
    {
        if (size < (full_len + 1))
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        lstrcpyW(section_ext, buffer);
        if (extptr != NULL)
        {
            *extptr = (len == full_len) ? NULL : &section_ext[len];
        }
    }

    if (needed != NULL)
    {
        *needed = full_len + 1;
    }

    return TRUE;
}

/***********************************************************************
 *              SetupDiGetActualSectionToInstallW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetActualSectionToInstallW(HINF hinf, const WCHAR *section, WCHAR *section_ext,
        DWORD size, DWORD *needed, WCHAR **extptr)
{
    return SetupDiGetActualSectionToInstallExW(hinf, section, NULL, section_ext, size,
            needed, extptr, NULL);
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
    LSTATUS ls;

    hKey = SetupDiOpenClassRegKeyExA(ClassGuid,
                                     KEY_ALL_ACCESS,
                                     DIOCR_INSTALLER,
                                     MachineName,
                                     Reserved);
    if (hKey == INVALID_HANDLE_VALUE)
    {
	WARN("SetupDiOpenClassRegKeyExA() failed (Error %lu)\n", GetLastError());
	return FALSE;
    }

    dwLength = ClassDescriptionSize;
    ls = RegQueryValueExA(hKey, NULL, NULL, NULL, (BYTE *)ClassDescription, &dwLength);
    RegCloseKey(hKey);
    if ((!ls || ls == ERROR_MORE_DATA) && RequiredSize)
        *RequiredSize = dwLength;
    return !ls;
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
    LSTATUS ls;

    hKey = SetupDiOpenClassRegKeyExW(ClassGuid,
                                     KEY_ALL_ACCESS,
                                     DIOCR_INSTALLER,
                                     MachineName,
                                     Reserved);
    if (hKey == INVALID_HANDLE_VALUE)
    {
	WARN("SetupDiOpenClassRegKeyExW() failed (Error %lu)\n", GetLastError());
	return FALSE;
    }

    dwLength = ClassDescriptionSize * sizeof(WCHAR);
    ls = RegQueryValueExW(hKey, NULL, NULL, NULL, (BYTE *)ClassDescription, &dwLength);
    RegCloseKey(hKey);
    if ((!ls || ls == ERROR_MORE_DATA) && RequiredSize)
        *RequiredSize = dwLength / sizeof(WCHAR);
    return !ls;
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
        enumstrW = malloc(len * sizeof(WCHAR));
        if (!enumstrW)
        {
            ret = INVALID_HANDLE_VALUE;
            goto end;
        }
        MultiByteToWideChar(CP_ACP, 0, enumstr, -1, enumstrW, len);
    }
    ret = SetupDiGetClassDevsExW(class, enumstrW, parent, flags, NULL, NULL,
            NULL);
    free(enumstrW);

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
        enumstrW = malloc(len * sizeof(WCHAR));
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
        machineW = malloc(len * sizeof(WCHAR));
        if (!machineW)
        {
            free(enumstrW);
            ret = INVALID_HANDLE_VALUE;
            goto end;
        }
        MultiByteToWideChar(CP_ACP, 0, machine, -1, machineW, len);
    }
    ret = SetupDiGetClassDevsExW(class, enumstrW, parent, flags, deviceset,
            machineW, reserved);
    free(enumstrW);
    free(machineW);

end:
    return ret;
}

static void SETUPDI_AddDeviceInterfaces(struct device *device, HKEY key,
    const GUID *guid, DWORD flags)
{
    DWORD i, len;
    WCHAR subKeyName[MAX_PATH];
    LONG l = ERROR_SUCCESS;

    for (i = 0; !l; i++)
    {
        len = ARRAY_SIZE(subKeyName);
        l = RegEnumKeyExW(key, i, subKeyName, &len, NULL, NULL, NULL, NULL);
        if (!l)
        {
            HKEY subKey;
            struct device_iface *iface;

            if (*subKeyName == '#')
            {
                /* The subkey name is the reference string, with a '#' prepended */
                l = RegOpenKeyExW(key, subKeyName, 0, KEY_READ, &subKey);
                if (!l)
                {
                    WCHAR symbolicLink[MAX_PATH];
                    DWORD dataType;

                    if (!(flags & DIGCF_PRESENT) || is_linked(subKey))
                    {
                        iface = SETUPDI_CreateDeviceInterface(device, guid, subKeyName + 1);

                        len = sizeof(symbolicLink);
                        l = RegQueryValueExW(subKey, L"SymbolicLink", NULL, &dataType,
                                (BYTE *)symbolicLink, &len);
                        if (!l && dataType == REG_SZ)
                            SETUPDI_SetInterfaceSymbolicLink(iface, symbolicLink);
                    }
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
        HKEY key, const GUID *guid, const WCHAR *enumstr, DWORD flags)
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
        len = ARRAY_SIZE(subKeyName);
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
                l = RegQueryValueExW(subKey, L"DeviceInstance", NULL, &dataType,
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
                            l = RegQueryValueExW(deviceKey, L"ClassGUID", NULL,
                                    &dataType, (BYTE *)deviceClassStr, &len);
                            if (!l && dataType == REG_SZ &&
                                    deviceClassStr[0] == '{' &&
                                    deviceClassStr[37] == '}')
                            {
                                GUID deviceClass;
                                struct device *device;

                                deviceClassStr[37] = 0;
                                UuidFromStringW(&deviceClassStr[1],
                                        &deviceClass);
                                if ((device = create_device(set, &deviceClass, deviceInst, FALSE)))
                                    SETUPDI_AddDeviceInterfaces(device, subKey, guid, flags);
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

    TRACE("%p, %s, %s, %08lx\n", DeviceInfoSet, debugstr_guid(guid),
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
                len = ARRAY_SIZE(interfaceGuidStr);
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
                                    interfaceKey, &interfaceGuid, enumstr, flags);
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
                    interfacesKey, guid, enumstr, flags);
        }
        RegCloseKey(interfacesKey);
    }
}

static void SETUPDI_EnumerateMatchingDeviceInstances(struct DeviceInfoSet *set,
        LPCWSTR enumerator, LPCWSTR deviceName, HKEY deviceKey,
        const GUID *class, DWORD flags)
{
    WCHAR id[MAX_DEVICE_ID_LEN];
    DWORD i, len;
    WCHAR deviceInstance[MAX_PATH];
    LONG l = ERROR_SUCCESS;

    TRACE("%s %s\n", debugstr_w(enumerator), debugstr_w(deviceName));

    for (i = 0; !l; i++)
    {
        len = ARRAY_SIZE(deviceInstance);
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
                l = RegQueryValueExW(subKey, L"ClassGUID", NULL, &dataType,
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

                            if (swprintf(id, ARRAY_SIZE(id), fmt, enumerator,
                                    deviceName, deviceInstance) != -1)
                            {
                                create_device(set, &deviceClass, id, FALSE);
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
        len = ARRAY_SIZE(subKeyName);
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

    TRACE("%p, %s, %s, %08lx\n", DeviceInfoSet, debugstr_guid(class),
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
                WCHAR *bus, *device;

                if (!wcschr(enumstr, '\\'))
                {
                    SETUPDI_EnumerateMatchingDevices(DeviceInfoSet, enumstr, enumStrKey, class, flags);
                }
                else if ((bus = wcsdup(enumstr)))
                {
                    device = wcschr(bus, '\\');
                    *device++ = 0;

                    SETUPDI_EnumerateMatchingDeviceInstances(DeviceInfoSet, bus, device, enumStrKey, class, flags);
                    free(bus);
                }

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
                len = ARRAY_SIZE(subKeyName);
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
    static const DWORD unsupportedFlags = DIGCF_DEFAULT | DIGCF_PROFILE;
    HDEVINFO set;

    TRACE("%s %s %p 0x%08lx %p %s %p\n", debugstr_guid(class),
            debugstr_w(enumstr), parent, flags, deviceset, debugstr_w(machine),
            reserved);

    if (!(flags & DIGCF_ALLCLASSES) && !class)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    if (flags & DIGCF_ALLCLASSES)
        class = NULL;

    if (flags & unsupportedFlags)
        WARN("unsupported flags %08lx\n", flags & unsupportedFlags);
    if (deviceset)
        set = deviceset;
    else
        set = SetupDiCreateDeviceInfoListExW((flags & DIGCF_DEVICEINTERFACE) ? NULL : class, parent, machine, reserved);
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
BOOL WINAPI SetupDiGetDeviceInfoListDetailA(HDEVINFO devinfo, SP_DEVINFO_LIST_DETAIL_DATA_A *DevInfoData)
{
    struct DeviceInfoSet *set;

    TRACE("devinfo %p, detail_data %p.\n", devinfo, DevInfoData);

    if (!(set = get_device_set(devinfo)))
        return FALSE;

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
BOOL WINAPI SetupDiGetDeviceInfoListDetailW(HDEVINFO devinfo, SP_DEVINFO_LIST_DETAIL_DATA_W *DevInfoData)
{
    struct DeviceInfoSet *set;

    TRACE("devinfo %p, detail_data %p.\n", devinfo, DevInfoData);

    if (!(set = get_device_set(devinfo)))
        return FALSE;

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

    TRACE("%p %p %s %s %08lx %p\n", DeviceInfoSet, DeviceInfoData,
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
BOOL WINAPI SetupDiCreateDeviceInterfaceW(HDEVINFO devinfo, SP_DEVINFO_DATA *device_data,
        const GUID *class, const WCHAR *refstr, DWORD flags, SP_DEVICE_INTERFACE_DATA *iface_data)
{
    struct device *device;
    struct device_iface *iface;

    TRACE("devinfo %p, device_data %p, class %s, refstr %s, flags %#lx, iface_data %p.\n",
            devinfo, device_data, debugstr_guid(class), debugstr_w(refstr), flags, iface_data);

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

    if (!class)
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if (!(iface = SETUPDI_CreateDeviceInterface(device, class, refstr)))
        return FALSE;

    if (iface_data)
    {
        if (iface_data->cbSize != sizeof(SP_DEVICE_INTERFACE_DATA))
        {
            SetLastError(ERROR_INVALID_USER_BUFFER);
            return FALSE;
        }

        copy_device_iface_data(iface_data, iface);
    }
    return TRUE;
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

    TRACE("%p %p %ld %08lx %p %p\n", DeviceInfoSet, DeviceInterfaceData, Reserved,
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

static LONG create_iface_key(const struct device_iface *iface, REGSAM access, HKEY *key)
{
    return RegCreateKeyExW(iface->refstr_key, L"Device Parameters", 0, NULL, 0, access, NULL, key, NULL);
}

/***********************************************************************
 *		SetupDiCreateDeviceInterfaceRegKeyW (SETUPAPI.@)
 */
HKEY WINAPI SetupDiCreateDeviceInterfaceRegKeyW(HDEVINFO devinfo,
    SP_DEVICE_INTERFACE_DATA *iface_data, DWORD reserved, REGSAM access,
    HINF hinf, const WCHAR *section)
{
    struct device_iface *iface;
    HKEY params_key;
    LONG ret;

    TRACE("devinfo %p, iface_data %p, reserved %ld, access %#lx, hinf %p, section %s.\n",
            devinfo, iface_data, reserved, access, hinf, debugstr_w(section));

    if (!(iface = get_device_iface(devinfo, iface_data)))
        return INVALID_HANDLE_VALUE;

    if (hinf && !section)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    ret = create_iface_key(iface, access, &params_key);
    if (ret)
    {
        SetLastError(ret);
        return INVALID_HANDLE_VALUE;
    }

    return params_key;
}

/***********************************************************************
 *		SetupDiDeleteDeviceInterfaceRegKey (SETUPAPI.@)
 */
BOOL WINAPI SetupDiDeleteDeviceInterfaceRegKey(HDEVINFO devinfo,
    SP_DEVICE_INTERFACE_DATA *iface_data, DWORD reserved)
{
    struct device_iface *iface;
    LONG ret;

    TRACE("devinfo %p, iface_data %p, reserved %ld.\n", devinfo, iface_data, reserved);

    if (!(iface = get_device_iface(devinfo, iface_data)))
        return FALSE;

    ret = RegDeleteKeyW(iface->refstr_key, L"Device Parameters");
    if (ret)
    {
        SetLastError(ret);
        return FALSE;
    }

    return TRUE;
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
BOOL WINAPI SetupDiEnumDeviceInterfaces(HDEVINFO devinfo,
    SP_DEVINFO_DATA *device_data, const GUID *class, DWORD index,
    SP_DEVICE_INTERFACE_DATA *iface_data)
{
    struct DeviceInfoSet *set;
    struct device *device;
    struct device_iface *iface;
    DWORD i = 0;

    TRACE("devinfo %p, device_data %p, class %s, index %lu, iface_data %p.\n",
            devinfo, device_data, debugstr_guid(class), index, iface_data);

    if (!iface_data || iface_data->cbSize != sizeof(SP_DEVICE_INTERFACE_DATA))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* In case application fails to check return value, clear output */
    memset(iface_data, 0, sizeof(*iface_data));
    iface_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    if (device_data)
    {
        if (!(device = get_device(devinfo, device_data)))
            return FALSE;

        LIST_FOR_EACH_ENTRY(iface, &device->interfaces, struct device_iface, entry)
        {
            if (IsEqualGUID(&iface->class, class))
            {
                if (i == index)
                {
                    copy_device_iface_data(iface_data, iface);
                    return TRUE;
                }
                i++;
            }
        }
    }
    else
    {
        if (!(set = get_device_set(devinfo)))
            return FALSE;

        LIST_FOR_EACH_ENTRY(device, &set->devices, struct device, entry)
        {
            LIST_FOR_EACH_ENTRY(iface, &device->interfaces, struct device_iface, entry)
            {
                if (IsEqualGUID(&iface->class, class))
                {
                    if (i == index)
                    {
                        copy_device_iface_data(iface_data, iface);
                        return TRUE;
                    }
                    i++;
                }
            }
        }
    }

    SetLastError(ERROR_NO_MORE_ITEMS);
    return FALSE;
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
    struct DeviceInfoSet *set;
    struct device *device, *device2;

    TRACE("devinfo %p.\n", devinfo);

    if (!(set = get_device_set(devinfo)))
        return FALSE;

    LIST_FOR_EACH_ENTRY_SAFE(device, device2, &set->devices, struct device, entry)
    {
        delete_device(device);
    }
    free(set);

    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

/***********************************************************************
 *		SetupDiGetDeviceInterfaceDetailA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInterfaceDetailA(HDEVINFO devinfo, SP_DEVICE_INTERFACE_DATA *iface_data,
        SP_DEVICE_INTERFACE_DETAIL_DATA_A *DeviceInterfaceDetailData,
        DWORD DeviceInterfaceDetailDataSize, DWORD *ret_size, SP_DEVINFO_DATA *device_data)
{
    struct device_iface *iface;
    DWORD bytesNeeded = FIELD_OFFSET(SP_DEVICE_INTERFACE_DETAIL_DATA_A, DevicePath[1]);
    BOOL ret = FALSE;

    TRACE("devinfo %p, iface_data %p, detail_data %p, size %ld, ret_size %p, device_data %p.\n",
            devinfo, iface_data, DeviceInterfaceDetailData, DeviceInterfaceDetailDataSize,
            ret_size, device_data);

    if (!(iface = get_device_iface(devinfo, iface_data)))
        return FALSE;

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

    if (iface->symlink)
        bytesNeeded += WideCharToMultiByte(CP_ACP, 0, iface->symlink, -1,
                NULL, 0, NULL, NULL) - 1;

    if (ret_size)
        *ret_size = bytesNeeded;

    if (DeviceInterfaceDetailDataSize >= bytesNeeded)
    {
        if (iface->symlink)
            WideCharToMultiByte(CP_ACP, 0, iface->symlink, -1,
                    DeviceInterfaceDetailData->DevicePath,
                    DeviceInterfaceDetailDataSize -
                    offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA_A, DevicePath),
                    NULL, NULL);
        else
            DeviceInterfaceDetailData->DevicePath[0] = '\0';

        ret = TRUE;
    }
    else
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }

    if (device_data && device_data->cbSize == sizeof(SP_DEVINFO_DATA))
        copy_device_data(device_data, iface->device);

    return ret;
}

/***********************************************************************
 *		SetupDiGetDeviceInterfaceDetailW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInterfaceDetailW(HDEVINFO devinfo, SP_DEVICE_INTERFACE_DATA *iface_data,
        SP_DEVICE_INTERFACE_DETAIL_DATA_W *DeviceInterfaceDetailData,
        DWORD DeviceInterfaceDetailDataSize, DWORD *ret_size, SP_DEVINFO_DATA *device_data)
{
    struct device_iface *iface;
    DWORD bytesNeeded = offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA_W, DevicePath)
        + sizeof(WCHAR); /* include NULL terminator */
    BOOL ret = FALSE;

    TRACE("devinfo %p, iface_data %p, detail_data %p, size %ld, ret_size %p, device_data %p.\n",
            devinfo, iface_data, DeviceInterfaceDetailData, DeviceInterfaceDetailDataSize,
            ret_size, device_data);

    if (!(iface = get_device_iface(devinfo, iface_data)))
        return FALSE;

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

    if (iface->symlink)
        bytesNeeded += sizeof(WCHAR) * lstrlenW(iface->symlink);

    if (ret_size)
        *ret_size = bytesNeeded;

    if (DeviceInterfaceDetailDataSize >= bytesNeeded)
    {
        if (iface->symlink)
            lstrcpyW(DeviceInterfaceDetailData->DevicePath, iface->symlink);
        else
            DeviceInterfaceDetailData->DevicePath[0] = '\0';

        ret = TRUE;
    }
    else
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }

    if (device_data && device_data->cbSize == sizeof(SP_DEVINFO_DATA))
        copy_device_data(device_data, iface->device);

    return ret;
}

BOOL WINAPI SetupDiGetDeviceInterfacePropertyW( HDEVINFO devinfo, SP_DEVICE_INTERFACE_DATA *iface_data,
                                                const DEVPROPKEY *key, DEVPROPTYPE *type, BYTE *buf, DWORD buf_size,
                                                DWORD *req_size, DWORD flags )
{
    FIXME( "devinfo %p, iface_data %p, key %p, type %p, buf %p, buf_size %lu, req_size %p, flags %#lx: stub!\n",
           devinfo, iface_data, key, type, buf, buf_size, req_size, flags );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

BOOL WINAPI SetupDiSetDeviceInterfacePropertyW( HDEVINFO devinfo, SP_DEVICE_INTERFACE_DATA *iface_data,
                                                const DEVPROPKEY *key, DEVPROPTYPE type, const BYTE *buf,
                                                DWORD buf_size, DWORD flags )
{
    FIXME( "devinfo %p, iface_data %p, key %p, type %#lx, buf %p, buf_size %lu, flags %#lx: stub!\n", devinfo,
           iface_data, key, type, buf, buf_size, flags );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *		SetupDiGetDeviceRegistryPropertyA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceRegistryPropertyA(HDEVINFO devinfo,
        SP_DEVINFO_DATA *device_data, DWORD Property, DWORD *PropertyRegDataType,
        BYTE *PropertyBuffer, DWORD PropertyBufferSize, DWORD *RequiredSize)
{
    BOOL ret = FALSE;
    struct device *device;

    TRACE("devinfo %p, device_data %p, property %ld, type %p, buffer %p, size %ld, required %p\n",
            devinfo, device_data, Property, PropertyRegDataType, PropertyBuffer, PropertyBufferSize, RequiredSize);

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

    if (PropertyBufferSize && PropertyBuffer == NULL)
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    if (Property < ARRAY_SIZE(PropertyMap) && PropertyMap[Property].nameA)
    {
        DWORD size = PropertyBufferSize;
        LONG l = RegQueryValueExA(device->key, PropertyMap[Property].nameA,
                NULL, PropertyRegDataType, PropertyBuffer, &size);

        if (l == ERROR_FILE_NOT_FOUND)
            SetLastError(ERROR_INVALID_DATA);
        else if (l == ERROR_MORE_DATA || !PropertyBufferSize)
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
BOOL WINAPI SetupDiGetDeviceRegistryPropertyW(HDEVINFO devinfo,
        SP_DEVINFO_DATA *device_data, DWORD Property, DWORD *PropertyRegDataType,
        BYTE *PropertyBuffer, DWORD PropertyBufferSize, DWORD *RequiredSize)
{
    BOOL ret = FALSE;
    struct device *device;

    TRACE("devinfo %p, device_data %p, prop %ld, type %p, buffer %p, size %ld, required %p\n",
            devinfo, device_data, Property, PropertyRegDataType, PropertyBuffer, PropertyBufferSize, RequiredSize);

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

    if (PropertyBufferSize && PropertyBuffer == NULL)
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    if (Property < ARRAY_SIZE(PropertyMap) && PropertyMap[Property].nameW)
    {
        DWORD size = PropertyBufferSize;
        LONG l = RegQueryValueExW(device->key, PropertyMap[Property].nameW,
                NULL, PropertyRegDataType, PropertyBuffer, &size);

        if (l == ERROR_FILE_NOT_FOUND)
            SetLastError(ERROR_INVALID_DATA);
        else if (l == ERROR_MORE_DATA || !PropertyBufferSize)
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
BOOL WINAPI SetupDiSetDeviceRegistryPropertyA(HDEVINFO devinfo, SP_DEVINFO_DATA *device_data,
        DWORD Property, const BYTE *PropertyBuffer, DWORD PropertyBufferSize)
{
    BOOL ret = FALSE;
    struct device *device;

    TRACE("devinfo %p, device_data %p, prop %ld, buffer %p, size %ld.\n",
            devinfo, device_data, Property, PropertyBuffer, PropertyBufferSize);

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

    if (Property < ARRAY_SIZE(PropertyMap) && PropertyMap[Property].nameA)
    {
        LONG l = RegSetValueExA(device->key, PropertyMap[Property].nameA, 0,
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
BOOL WINAPI SetupDiSetDeviceRegistryPropertyW(HDEVINFO devinfo,
    SP_DEVINFO_DATA *device_data, DWORD prop, const BYTE *buffer, DWORD size)
{
    struct device *device;

    TRACE("devinfo %p, device_data %p, prop %ld, buffer %p, size %ld.\n",
            devinfo, device_data, prop, buffer, size);

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

    return SETUPDI_SetDeviceRegistryPropertyW(device, prop, buffer, size);
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
    WCHAR FullBuffer[MAX_PATH];
    WCHAR Buffer[MAX_PATH];
    DWORD RequiredSize;
    HKEY hClassKey;

    if (!SetupGetLineTextW(NULL,
			   hInf,
			   L"Version",
			   L"ClassGUID",
			   Buffer,
			   MAX_PATH,
			   &RequiredSize))
    {
	return INVALID_HANDLE_VALUE;
    }

    lstrcpyW(FullBuffer, ControlClass);
    lstrcatW(FullBuffer, L"\\");
    lstrcatW(FullBuffer, Buffer);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
		      FullBuffer,
		      0,
		      KEY_ALL_ACCESS,
		      &hClassKey))
    {
	if (!SetupGetLineTextW(NULL,
			       hInf,
			       L"Version",
			       L"Class",
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
		       L"Class",
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
				      L"ClassInstall32",
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
 *              SetupDiOpenDeviceInfoA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiOpenDeviceInfoA(HDEVINFO devinfo, PCSTR instance_id, HWND hwnd_parent, DWORD flags,
                                   PSP_DEVINFO_DATA device_data)
{
    WCHAR instance_idW[MAX_DEVICE_ID_LEN];

    TRACE("%p %s %p 0x%08lx %p\n", devinfo, debugstr_a(instance_id), hwnd_parent, flags, device_data);

    if (!instance_id || strlen(instance_id) >= MAX_DEVICE_ID_LEN)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    MultiByteToWideChar(CP_ACP, 0, instance_id, -1, instance_idW, ARRAY_SIZE(instance_idW));
    return SetupDiOpenDeviceInfoW(devinfo, instance_idW, hwnd_parent, flags, device_data);
}

/***********************************************************************
 *              SetupDiOpenDeviceInfoW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiOpenDeviceInfoW(HDEVINFO devinfo, PCWSTR instance_id, HWND hwnd_parent, DWORD flags,
                                   PSP_DEVINFO_DATA device_data)
{
    struct DeviceInfoSet *set;
    struct device *device;
    WCHAR classW[40];
    GUID guid;
    HKEY enumKey = NULL;
    HKEY instanceKey = NULL;
    DWORD phantom;
    DWORD size;
    DWORD error = ERROR_NO_SUCH_DEVINST;

    TRACE("%p %s %p 0x%08lx %p\n", devinfo, debugstr_w(instance_id), hwnd_parent, flags, device_data);

    if (!(set = get_device_set(devinfo)))
        return FALSE;

    if (!instance_id)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (hwnd_parent)
        FIXME("hwnd_parent unsupported\n");

    if (flags)
        FIXME("flags unsupported: 0x%08lx\n", flags);

    RegCreateKeyExW(HKEY_LOCAL_MACHINE, Enum, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &enumKey, NULL);
    /* Instance needs to be already existent in registry, if not, report ERROR_NO_SUCH_DEVINST */
    if (RegOpenKeyExW(enumKey, instance_id, 0, KEY_READ, &instanceKey))
        goto done;

    /* If it's an unregistered instance, aka phantom instance, report ERROR_NO_SUCH_DEVINST */
    size = sizeof(phantom);
    if (!RegQueryValueExW(instanceKey, L"Phantom", NULL, NULL, (BYTE *)&phantom, &size))
        goto done;

    /* Check class GUID */
    size = sizeof(classW);
    if (RegQueryValueExW(instanceKey, L"ClassGUID", NULL, NULL, (BYTE *)classW, &size))
        goto done;

    classW[37] = 0;
    UuidFromStringW(&classW[1], &guid);

    if (!IsEqualGUID(&set->ClassGuid, &GUID_NULL) && !IsEqualGUID(&guid, &set->ClassGuid))
    {
        error = ERROR_CLASS_MISMATCH;
        goto done;
    }

    if (!(device = create_device(set, &guid, instance_id, FALSE)))
        goto done;

    if (!device_data || device_data->cbSize == sizeof(SP_DEVINFO_DATA))
    {
        if (device_data)
            copy_device_data(device_data, device);
        error = NO_ERROR;
    }
    else
        error = ERROR_INVALID_USER_BUFFER;

done:
    RegCloseKey(instanceKey);
    RegCloseKey(enumKey);
    SetLastError(error);
    return !error;
}

/***********************************************************************
 *		SetupDiOpenDeviceInterfaceW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiOpenDeviceInterfaceW (HDEVINFO devinfo, const WCHAR *device_path,
       DWORD flags, SP_DEVICE_INTERFACE_DATA *iface_data)
{
    SP_DEVINFO_DATA device_data = {.cbSize = sizeof(device_data)};
    WCHAR *instance_id = NULL, *tmp;
    struct device_iface *iface;
    struct device *device;
    unsigned int len;

    TRACE("%p %s %#lx %p\n", devinfo, debugstr_w(device_path), flags, iface_data);

    if (flags)
        FIXME("flags %#lx not implemented\n", flags);

    if (!device_path)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if ((len = wcslen(device_path)) < 4)
        goto error;
    if (!(instance_id = malloc((len - 4 + 1) * sizeof(*instance_id))))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    wcscpy(instance_id, device_path + 4);
    if ((tmp = wcsrchr(instance_id, '#'))) *tmp = 0;
    while ((tmp = wcschr(instance_id, '#'))) *tmp = '\\';

    if (!SetupDiGetClassDevsExW(NULL, instance_id, NULL, DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES,
            devinfo, NULL, NULL))
        goto error;
    if (!SetupDiOpenDeviceInfoW(devinfo, instance_id, NULL, 0, &device_data))
        goto error;

    if (!(device = get_device(devinfo, &device_data)))
        goto error;
    LIST_FOR_EACH_ENTRY(iface, &device->interfaces, struct device_iface, entry)
    {
        if (iface->symlink && !wcsicmp(device_path, iface->symlink))
        {
            if (iface_data)
                copy_device_iface_data(iface_data, iface);
            free(instance_id);
            return TRUE;
        }
    }

error:
    free(instance_id);
    SetLastError(ERROR_NO_SUCH_DEVICE_INTERFACE);
    return FALSE;
}

/***********************************************************************
 *		SetupDiOpenDeviceInterfaceA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiOpenDeviceInterfaceA(HDEVINFO devinfo, const char *device_path,
       DWORD flags, SP_DEVICE_INTERFACE_DATA *iface_data)
{
    WCHAR *device_pathW;
    BOOL ret;
    int len;

    TRACE("%p %s %#lx %p\n", devinfo, debugstr_a(device_path), flags, iface_data);

    if (!device_path)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!(len = MultiByteToWideChar(CP_ACP, 0, device_path, -1, NULL, 0)))
    {
        SetLastError(ERROR_NO_SUCH_DEVICE_INTERFACE);
        return FALSE;
    }
    if (!(device_pathW = malloc(len * sizeof(*device_pathW))))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    MultiByteToWideChar(CP_ACP, 0, device_path, -1, device_pathW, len);
    ret = SetupDiOpenDeviceInterfaceW(devinfo, device_pathW, flags, iface_data);
    free(device_pathW);
    return ret;
}

/***********************************************************************
 *              SetupDiOpenDeviceInterfaceRegKey (SETUPAPI.@)
 */
HKEY WINAPI SetupDiOpenDeviceInterfaceRegKey(HDEVINFO devinfo, PSP_DEVICE_INTERFACE_DATA iface_data,
        DWORD reserved, REGSAM access)
{
    struct device_iface *iface;
    LSTATUS lr;
    HKEY key;

    TRACE("devinfo %p, iface_data %p, reserved %ld, access %#lx.\n", devinfo, iface_data, reserved, access);

    if (!(iface = get_device_iface(devinfo, iface_data)))
        return INVALID_HANDLE_VALUE;

    lr = RegOpenKeyExW(iface->refstr_key, L"Device Parameters", 0, access, &key);
    if (lr)
    {
        SetLastError(lr);
        return INVALID_HANDLE_VALUE;
    }

    return key;
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
    FIXME("%p %p %x %lu\n",DeviceInfoSet, DeviceInfoData,
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
    FIXME("%p %p %x %lu\n",DeviceInfoSet, DeviceInfoData,
          ClassInstallParams->InstallFunction, ClassInstallParamsSize);
    return FALSE;
}

static BOOL call_coinstallers(WCHAR *list, DI_FUNCTION function, HDEVINFO devinfo, SP_DEVINFO_DATA *device_data)
{
    DWORD (CALLBACK *coinst_proc)(DI_FUNCTION, HDEVINFO, SP_DEVINFO_DATA *, COINSTALLER_CONTEXT_DATA *);
    COINSTALLER_CONTEXT_DATA coinst_ctx;
    WCHAR *p, *procnameW;
    HMODULE module;
    char *procname;
    DWORD ret;

    for (p = list; *p; p += lstrlenW(p) + 1)
    {
        TRACE("Found co-installer %s.\n", debugstr_w(p));
        if ((procnameW = wcschr(p, ',')))
            *procnameW = 0;

        if ((module = LoadLibraryExW(p, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32)))
        {
            if (procnameW)
            {
                procname = strdupWtoA(procnameW + 1);
                coinst_proc = (void *)GetProcAddress(module, procname);
                free(procname);
            }
            else
                coinst_proc = (void *)GetProcAddress(module, "CoDeviceInstall");
            if (coinst_proc)
            {
                memset(&coinst_ctx, 0, sizeof(coinst_ctx));
                TRACE("Calling co-installer %p.\n", coinst_proc);
                ret = coinst_proc(function, devinfo, device_data, &coinst_ctx);
                TRACE("Co-installer %p returned %#lx.\n", coinst_proc, ret);
                if (ret == ERROR_DI_POSTPROCESSING_REQUIRED)
                    FIXME("Co-installer postprocessing not implemented.\n");
                else if (ret)
                {
                    ERR("Co-installer returned error %#lx.\n", ret);
                    FreeLibrary(module);
                    SetLastError(ret);
                    return FALSE;
                }
            }
            FreeLibrary(module);
        }
    }

    return TRUE;
}

/***********************************************************************
 *              SetupDiCallClassInstaller (SETUPAPI.@)
 */
BOOL WINAPI SetupDiCallClassInstaller(DI_FUNCTION function, HDEVINFO devinfo, SP_DEVINFO_DATA *device_data)
{
    static const WCHAR class_coinst_pathW[] = L"System\\CurrentControlSet\\Control\\CoDeviceInstallers";
    DWORD (CALLBACK *classinst_proc)(DI_FUNCTION, HDEVINFO, SP_DEVINFO_DATA *);
    DWORD ret = ERROR_DI_DO_DEFAULT;
    HKEY class_key, coinst_key;
    WCHAR *path, *procnameW;
    struct device *device;
    WCHAR guidstr[39];
    BOOL coret = TRUE;
    HMODULE module;
    char *procname;
    DWORD size;

    TRACE("function %#x, devinfo %p, device_data %p.\n", function, devinfo, device_data);

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

    if (!RegOpenKeyExW(HKEY_LOCAL_MACHINE, class_coinst_pathW, 0, KEY_READ, &coinst_key))
    {
        SETUPDI_GuidToString(&device->class, guidstr);
        if (!RegGetValueW(coinst_key, NULL, guidstr, RRF_RT_REG_MULTI_SZ, NULL, NULL, &size))
        {
            path = malloc(size);
            if (!RegGetValueW(coinst_key, NULL, guidstr, RRF_RT_REG_MULTI_SZ, NULL, path, &size))
                coret = call_coinstallers(path, function, devinfo, device_data);
            free(path);
        }
        RegCloseKey(coinst_key);
    }

    if (!coret)
        return FALSE;

    if (!open_driver_key(device, KEY_READ, &coinst_key))
    {
        if (!RegGetValueW(coinst_key, NULL, L"CoInstallers32", RRF_RT_REG_MULTI_SZ, NULL, NULL, &size))
        {
            path = malloc(size);
            if (!RegGetValueW(coinst_key, NULL, L"CoInstallers32", RRF_RT_REG_MULTI_SZ, NULL, path, &size))
                coret = call_coinstallers(path, function, devinfo, device_data);
            free(path);
        }
        RegCloseKey(coinst_key);
    }

    if ((class_key = SetupDiOpenClassRegKey(&device->class, KEY_READ)) != INVALID_HANDLE_VALUE)
    {
        if (!RegGetValueW(class_key, NULL, L"Installer32", RRF_RT_REG_SZ, NULL, NULL, &size))
        {
            path = malloc(size);
            if (!RegGetValueW(class_key, NULL, L"Installer32", RRF_RT_REG_SZ, NULL, path, &size))
            {
                TRACE("Found class installer %s.\n", debugstr_w(path));
                if ((procnameW = wcschr(path, ',')))
                    *procnameW = 0;

                if ((module = LoadLibraryExW(path, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32)))
                {
                    if (procnameW)
                    {
                        procname = strdupWtoA(procnameW + 1);
                        classinst_proc = (void *)GetProcAddress(module, procname);
                        free(procname);
                    }
                    else
                        classinst_proc = (void *)GetProcAddress(module, "ClassInstall");
                    if (classinst_proc)
                    {
                        TRACE("Calling class installer %p.\n", classinst_proc);
                        ret = classinst_proc(function, devinfo, device_data);
                        TRACE("Class installer %p returned %#lx.\n", classinst_proc, ret);
                    }
                    FreeLibrary(module);
                }
            }
            free(path);
        }
        RegCloseKey(class_key);
    }

    if (ret == ERROR_DI_DO_DEFAULT)
    {
        switch (function)
        {
        case DIF_REGISTERDEVICE:
            return SetupDiRegisterDeviceInfo(devinfo, device_data, 0, NULL, NULL, NULL);
        case DIF_REMOVE:
            return SetupDiRemoveDevice(devinfo, device_data);
        case DIF_SELECTBESTCOMPATDRV:
            return SetupDiSelectBestCompatDrv(devinfo, device_data);
        case DIF_REGISTER_COINSTALLERS:
            return SetupDiRegisterCoDeviceInstallers(devinfo, device_data);
        case DIF_INSTALLDEVICEFILES:
            return SetupDiInstallDriverFiles(devinfo, device_data);
        case DIF_INSTALLINTERFACES:
            return SetupDiInstallDeviceInterfaces(devinfo, device_data);
        case DIF_INSTALLDEVICE:
            return SetupDiInstallDevice(devinfo, device_data);
        case DIF_FINISHINSTALL_ACTION:
        case DIF_PROPERTYCHANGE:
        case DIF_SELECTDEVICE:
        case DIF_UNREMOVE:
            FIXME("Unhandled function %#x.\n", function);
        }
    }

    if (ret) SetLastError(ret);
    return !ret;
}

/***********************************************************************
 *              SetupDiGetDeviceInstallParamsW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInstallParamsW(HDEVINFO devinfo,
        SP_DEVINFO_DATA *device_data, SP_DEVINSTALL_PARAMS_W *params)
{
    struct device *device;

    TRACE("devinfo %p, device_data %p, params %p.\n", devinfo, device_data, params);

    if (params->cbSize != sizeof(SP_DEVINSTALL_PARAMS_W))
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

    *params = device->params;

    return TRUE;
}

/***********************************************************************
 *              SetupDiGetDeviceInstallParamsA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInstallParamsA(HDEVINFO devinfo,
        SP_DEVINFO_DATA *device_data, SP_DEVINSTALL_PARAMS_A *params)
{
    SP_DEVINSTALL_PARAMS_W paramsW;
    BOOL ret;

    if (params->cbSize != sizeof(SP_DEVINSTALL_PARAMS_A))
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    paramsW.cbSize = sizeof(paramsW);
    ret = SetupDiGetDeviceInstallParamsW(devinfo, device_data, &paramsW);
    params->Flags = paramsW.Flags;
    params->FlagsEx = paramsW.FlagsEx;
    params->hwndParent = paramsW.hwndParent;
    params->InstallMsgHandler = paramsW.InstallMsgHandler;
    params->InstallMsgHandlerContext = paramsW.InstallMsgHandlerContext;
    params->FileQueue = paramsW.FileQueue;
    params->ClassInstallReserved = paramsW.ClassInstallReserved;
    params->Reserved = paramsW.Reserved;
    WideCharToMultiByte(CP_ACP, 0, paramsW.DriverPath, -1, params->DriverPath, sizeof(params->DriverPath), NULL, NULL);

    return ret;
}

/***********************************************************************
 *              SetupDiSetDeviceInstallParamsA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiSetDeviceInstallParamsA(HDEVINFO devinfo,
        SP_DEVINFO_DATA *device_data, SP_DEVINSTALL_PARAMS_A *params)
{
    SP_DEVINSTALL_PARAMS_W paramsW;

    if (params->cbSize != sizeof(SP_DEVINSTALL_PARAMS_A))
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    paramsW.cbSize = sizeof(paramsW);
    paramsW.Flags = params->Flags;
    paramsW.FlagsEx = params->FlagsEx;
    paramsW.hwndParent = params->hwndParent;
    paramsW.InstallMsgHandler = params->InstallMsgHandler;
    paramsW.InstallMsgHandlerContext = params->InstallMsgHandlerContext;
    paramsW.FileQueue = params->FileQueue;
    paramsW.ClassInstallReserved = params->ClassInstallReserved;
    paramsW.Reserved = params->Reserved;
    MultiByteToWideChar(CP_ACP, 0, params->DriverPath, -1, paramsW.DriverPath, ARRAY_SIZE(paramsW.DriverPath));

    return SetupDiSetDeviceInstallParamsW(devinfo, device_data, &paramsW);
}

/***********************************************************************
 *              SetupDiSetDeviceInstallParamsW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiSetDeviceInstallParamsW(HDEVINFO devinfo,
        SP_DEVINFO_DATA *device_data, SP_DEVINSTALL_PARAMS_W *params)
{
    struct device *device;

    TRACE("devinfo %p, device_data %p, params %p.\n", devinfo, device_data, params);

    if (params->cbSize != sizeof(SP_DEVINSTALL_PARAMS_W))
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

    device->params = *params;

    return TRUE;
}

BOOL WINAPI SetupDiSetDevicePropertyW(HDEVINFO devinfo, PSP_DEVINFO_DATA device_data, const DEVPROPKEY *key,
                                      DEVPROPTYPE type, const BYTE *buffer, DWORD size, DWORD flags)
{
    struct device *device;
    HKEY properties_hkey, property_hkey;
    WCHAR property_hkey_path[44];
    LSTATUS ls;

    TRACE("%p %p %p %#lx %p %ld %#lx\n", devinfo, device_data, key, type, buffer, size, flags);

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

    if (!key || !is_valid_property_type(type)
        || (buffer && !size && !(type == DEVPROP_TYPE_EMPTY || type == DEVPROP_TYPE_NULL))
        || (buffer && size && (type == DEVPROP_TYPE_EMPTY || type == DEVPROP_TYPE_NULL)))
    {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    if (size && !buffer)
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if (flags)
    {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    ls = RegCreateKeyExW(device->key, L"Properties", 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &properties_hkey, NULL);
    if (ls)
    {
        SetLastError(ls);
        return FALSE;
    }

    SETUPDI_GuidToString(&key->fmtid, property_hkey_path);
    swprintf(property_hkey_path + 38, ARRAY_SIZE(property_hkey_path) - 38, L"\\%04X", key->pid);

    if (type == DEVPROP_TYPE_EMPTY)
    {
        ls = RegDeleteKeyW(properties_hkey, property_hkey_path);
        RegCloseKey(properties_hkey);
        SetLastError(ls == ERROR_FILE_NOT_FOUND ? ERROR_NOT_FOUND : ls);
        return !ls;
    }
    else if (type == DEVPROP_TYPE_NULL)
    {
        if (!(ls = RegOpenKeyW(properties_hkey, property_hkey_path, &property_hkey)))
        {
            ls = RegDeleteValueW(property_hkey, NULL);
            RegCloseKey(property_hkey);
        }

        RegCloseKey(properties_hkey);
        SetLastError(ls == ERROR_FILE_NOT_FOUND ? ERROR_NOT_FOUND : ls);
        return !ls;
    }
    else
    {
        if (!(ls = RegCreateKeyExW(properties_hkey, property_hkey_path, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL,
                                  &property_hkey, NULL)))
        {
            ls = RegSetValueExW(property_hkey, NULL, 0, 0xffff0000 | (0xffff & type), buffer, size);
            RegCloseKey(property_hkey);
        }

        RegCloseKey(properties_hkey);
        SetLastError(ls);
        return !ls;
    }
}

/***********************************************************************
 *		SetupDiOpenDevRegKey (SETUPAPI.@)
 */
HKEY WINAPI SetupDiOpenDevRegKey(HDEVINFO devinfo, SP_DEVINFO_DATA *device_data,
        DWORD Scope, DWORD HwProfile, DWORD KeyType, REGSAM samDesired)
{
    struct device *device;
    HKEY key = INVALID_HANDLE_VALUE;
    LONG l;

    TRACE("devinfo %p, device_data %p, scope %ld, profile %ld, type %ld, access %#lx.\n",
            devinfo, device_data, Scope, HwProfile, KeyType, samDesired);

    if (!(device = get_device(devinfo, device_data)))
        return INVALID_HANDLE_VALUE;

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

    if (device->phantom)
    {
        SetLastError(ERROR_DEVINFO_NOT_REGISTERED);
        return INVALID_HANDLE_VALUE;
    }
    if (Scope != DICS_FLAG_GLOBAL)
        FIXME("unimplemented for scope %ld\n", Scope);
    switch (KeyType)
    {
        case DIREG_DEV:
            l = RegOpenKeyExW(device->key, L"Device Parameters", 0, samDesired, &key);
            break;
        case DIREG_DRV:
            l = open_driver_key(device, samDesired, &key);
            break;
        default:
            FIXME("Unhandled type %#lx.\n", KeyType);
            l = ERROR_CALL_NOT_IMPLEMENTED;
    }
    SetLastError(l == ERROR_FILE_NOT_FOUND ? ERROR_KEY_DOES_NOT_EXIST : l);
    return l ? INVALID_HANDLE_VALUE : key;
}

/***********************************************************************
 *		SetupDiDeleteDevRegKey (SETUPAPI.@)
 */
BOOL WINAPI SetupDiDeleteDevRegKey(HDEVINFO devinfo, SP_DEVINFO_DATA *device_data,
        DWORD Scope, DWORD HwProfile, DWORD KeyType)
{
    struct device *device;
    LONG l;

    TRACE("devinfo %p, device_data %p, scope %ld, profile %ld, type %ld.\n",
            devinfo, device_data, Scope, HwProfile, KeyType);

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

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

    if (device->phantom)
    {
        SetLastError(ERROR_DEVINFO_NOT_REGISTERED);
        return FALSE;
    }
    if (Scope != DICS_FLAG_GLOBAL)
        FIXME("unimplemented for scope %ld\n", Scope);
    switch (KeyType)
    {
        case DIREG_DRV:
            l = delete_driver_key(device);
            break;
        case DIREG_BOTH:
            if ((l = delete_driver_key(device)))
                break;
            /* fall through */
        case DIREG_DEV:
            l = RegDeleteKeyW(device->key, L"Device Parameters");
            break;
        default:
            FIXME("Unhandled type %#lx.\n", KeyType);
            l = ERROR_CALL_NOT_IMPLEMENTED;
    }
    SetLastError(l);
    return !l;
}

/***********************************************************************
 *              CM_Get_Device_IDA  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_IDA(DEVINST devnode, char *buffer, ULONG len, ULONG flags)
{
    TRACE("%lu, %p, %lu, %#lx\n", devnode, buffer, len, flags);

    if (devnode >= devinst_table_size || !devinst_table[devnode])
        return CR_NO_SUCH_DEVINST;

    WideCharToMultiByte(CP_ACP, 0, devinst_table[devnode], -1, buffer, len, 0, 0);
    TRACE("Returning %s\n", debugstr_a(buffer));
    return CR_SUCCESS;
}

/***********************************************************************
 *              CM_Get_Device_IDW  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_IDW(DEVINST devnode, WCHAR *buffer, ULONG len, ULONG flags)
{
    TRACE("%lu, %p, %lu, %#lx\n", devnode, buffer, len, flags);

    if (devnode >= devinst_table_size || !devinst_table[devnode])
        return CR_NO_SUCH_DEVINST;

    lstrcpynW(buffer, devinst_table[devnode], len);
    TRACE("Returning %s\n", debugstr_w(buffer));
    return CR_SUCCESS;
}

/***********************************************************************
 *              CM_Get_Device_ID_Size  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_ID_Size(ULONG *len, DEVINST devnode, ULONG flags)
{
    TRACE("%p, %lu, %#lx\n", len, devnode, flags);

    if (devnode >= devinst_table_size || !devinst_table[devnode])
        return CR_NO_SUCH_DEVINST;

    *len = lstrlenW(devinst_table[devnode]);
    return CR_SUCCESS;
}

/***********************************************************************
 *      CM_Locate_DevNodeA (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Locate_DevNodeA(DEVINST *devinst, DEVINSTID_A device_id, ULONG flags)
{
    TRACE("%p %s %#lx.\n", devinst, debugstr_a(device_id), flags);

    return CM_Locate_DevNode_ExA(devinst, device_id, flags, NULL);
}

/***********************************************************************
 *      CM_Locate_DevNodeW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Locate_DevNodeW(DEVINST *devinst, DEVINSTID_W device_id, ULONG flags)
{
    TRACE("%p %s %#lx.\n", devinst, debugstr_w(device_id), flags);

    return CM_Locate_DevNode_ExW(devinst, device_id, flags, NULL);
}

/***********************************************************************
 *      CM_Locate_DevNode_ExA (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Locate_DevNode_ExA(DEVINST *devinst, DEVINSTID_A device_id, ULONG flags, HMACHINE machine)
{
    CONFIGRET ret;
    DEVINSTID_W device_idw;
    unsigned int slen;

    TRACE("%p %s %#lx %p.\n", devinst, debugstr_a(device_id), flags, machine);

    if (!device_id)
    {
        FIXME("NULL device_id unsupported.\n");
        return CR_CALL_NOT_IMPLEMENTED;
    }

    slen = strlen(device_id) + 1;
    if (!(device_idw = malloc(slen * sizeof(*device_idw))))
        return CR_OUT_OF_MEMORY;

    MultiByteToWideChar(CP_ACP, 0, device_id, slen, device_idw, slen);
    ret = CM_Locate_DevNode_ExW(devinst, device_idw, flags, NULL);
    free(device_idw);
    return ret;
}

/***********************************************************************
 *      CM_Locate_DevNode_ExW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Locate_DevNode_ExW(DEVINST *devinst, DEVINSTID_W device_id, ULONG flags, HMACHINE machine)
{
    DEVINST ret;

    TRACE("%p %s %#lx %p.\n", devinst, debugstr_w(device_id), flags, machine);

    if (!devinst)
        return CR_INVALID_POINTER;

    *devinst = 0;

    if (machine)
        FIXME("machine %p not supported.\n", machine);
    if (flags)
        FIXME("flags %#lx are not supported.\n", flags);

    if (!device_id)
    {
        FIXME("NULL device_id unsupported.\n");
        return CR_CALL_NOT_IMPLEMENTED;
    }

    if ((ret = get_devinst_for_device_id(device_id)) < devinst_table_size && devinst_table[ret])
    {
        *devinst = ret;
        return CR_SUCCESS;
    }

    return CR_NO_SUCH_DEVNODE;
}

static CONFIGRET get_device_id_list(const WCHAR *filter, WCHAR *buffer, ULONG *len, ULONG flags)
{
    const ULONG supported_flags = CM_GETIDLIST_FILTER_NONE | CM_GETIDLIST_FILTER_CLASS | CM_GETIDLIST_FILTER_PRESENT;
    SP_DEVINFO_DATA device = { sizeof(device) };
    CONFIGRET ret = CR_SUCCESS;
    GUID guid, *pguid = NULL;
    unsigned int i, id_len;
    ULONG query_flags = 0;
    HDEVINFO set;
    WCHAR id[256];
    ULONG needed;
    WCHAR *p;

    if (!len || (buffer && !*len))
        return CR_INVALID_POINTER;

    needed = 1;

    if (buffer)
        *buffer = 0;
    if (flags & ~supported_flags)
    {
        FIXME("Flags %#lx are not supported.\n", flags);
        *len = needed;
        return CR_SUCCESS;
    }

    if (!buffer)
        *len = 0;

    if (flags & CM_GETIDLIST_FILTER_CLASS)
    {
        if (!filter)
            return CR_INVALID_POINTER;
        if (IIDFromString((WCHAR *)filter, &guid))
            return CR_INVALID_DATA;
        pguid = &guid;
    }

    if (!buffer)
        *len = needed;

    if (!pguid)
        query_flags |= DIGCF_ALLCLASSES;
    if (flags & CM_GETIDLIST_FILTER_PRESENT)
        query_flags |= DIGCF_PRESENT;

    set = SetupDiGetClassDevsW(pguid, NULL, NULL, query_flags);
    if (set == INVALID_HANDLE_VALUE)
        return CR_SUCCESS;

    p = buffer;
    for (i = 0; SetupDiEnumDeviceInfo(set, i, &device); ++i)
    {
        ret = SetupDiGetDeviceInstanceIdW(set, &device, id, ARRAY_SIZE(id), NULL);
        if (!ret) continue;
        id_len = wcslen(id) + 1;
        needed += id_len;
        if (buffer)
        {
            if (needed > *len)
            {
                SetupDiDestroyDeviceInfoList(set);
                *buffer = 0;
                return CR_BUFFER_SMALL;
            }
            memcpy(p, id, sizeof(*p) * id_len);
            p += id_len;
        }
    }
    SetupDiDestroyDeviceInfoList(set);
    *len = needed;
    if (buffer)
        *p = 0;
    return CR_SUCCESS;
}

/***********************************************************************
 *             CM_Get_Device_ID_List_ExW  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_ID_List_ExW(const WCHAR *filter, WCHAR *buffer, ULONG len, ULONG flags, HMACHINE machine)
{
    TRACE("%s %p %ld %#lx %p.\n", debugstr_w(filter), buffer, len, flags, machine);

    if (machine)
        FIXME("machine %p.\n", machine);

    if (!buffer)
        return CR_INVALID_POINTER;

    return get_device_id_list(filter, buffer, &len, flags);
}

/***********************************************************************
 *             CM_Get_Device_ID_ListW  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_ID_ListW(const WCHAR *filter, WCHAR *buffer, ULONG len, ULONG flags)
{
    return CM_Get_Device_ID_List_ExW(filter, buffer, len, flags, NULL);
}

/***********************************************************************
 *             CM_Get_Device_ID_List_Size_ExW  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_ID_List_Size_ExW(ULONG *len, const WCHAR *filter, ULONG flags, HMACHINE machine)
{
    TRACE("%p %s %#lx, machine %p.\n", len, debugstr_w(filter), flags, machine);

    if (machine)
        FIXME("machine %p.\n", machine);

    return get_device_id_list(filter, NULL, len, flags);
}

/***********************************************************************
 *             CM_Get_Device_ID_List_SizeW  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_ID_List_SizeW(ULONG *len, const WCHAR *filter, ULONG flags)
{
    TRACE("%p %s %#lx.\n", len, debugstr_w(filter), flags);

    return get_device_id_list(filter, NULL, len, flags);
}

/***********************************************************************
 *             CM_Get_Device_ID_List_ExA  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_ID_List_ExA(const char *filter, char *buffer, ULONG len, ULONG flags, HMACHINE machine)
{
    WCHAR *wbuffer, *wfilter = NULL, *p;
    unsigned int slen;
    CONFIGRET ret;

    TRACE("%s %p %ld %#lx.\n", debugstr_a(filter), buffer, len, flags);

    if (machine)
        FIXME("machine %p.\n", machine);

    if (!buffer || !len)
        return CR_INVALID_POINTER;

    if (!(wbuffer = malloc(len * sizeof(*wbuffer))))
        return CR_OUT_OF_MEMORY;

    if (filter)
    {
        slen = strlen(filter) + 1;
        if (!(wfilter = malloc(slen * sizeof(*wfilter))))
        {
            free(wbuffer);
            return CR_OUT_OF_MEMORY;
        }
        MultiByteToWideChar(CP_ACP, 0, filter, slen, wfilter, slen);
    }

    if (!(ret = CM_Get_Device_ID_ListW(wfilter, wbuffer, len, flags)))
    {
        p = wbuffer;
        while (*p)
        {
            slen = wcslen(p) + 1;
            WideCharToMultiByte(CP_ACP, 0, p, slen, buffer, slen, NULL, NULL);
            p += slen;
            buffer += slen;
        }
        *buffer = 0;
    }
    free(wfilter);
    free(wbuffer);
    return ret;
}

/***********************************************************************
 *             CM_Get_Device_ID_ListA  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_ID_ListA(const char *filter, char *buffer, ULONG len, ULONG flags)
{
    return CM_Get_Device_ID_List_ExA(filter, buffer, len, flags, NULL);
}

/***********************************************************************
 *             CM_Get_Device_ID_List_Size_ExA  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_ID_List_Size_ExA(ULONG *len, const char *filter, ULONG flags, HMACHINE machine)
{
    WCHAR *wfilter = NULL;
    unsigned int slen;
    CONFIGRET ret;

    TRACE("%p %s %#lx.\n", len, debugstr_a(filter), flags);

    if (machine)
        FIXME("machine %p.\n", machine);

    if (filter)
    {
        slen = strlen(filter) + 1;
        if (!(wfilter = malloc(slen * sizeof(*wfilter))))
            return CR_OUT_OF_MEMORY;
        MultiByteToWideChar(CP_ACP, 0, filter, slen, wfilter, slen);
    }
    ret = CM_Get_Device_ID_List_SizeW(len, wfilter, flags);
    free(wfilter);
    return ret;
}

/***********************************************************************
 *             CM_Get_Device_ID_List_SizeA  (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_ID_List_SizeA(ULONG *len, const char *filter, ULONG flags)
{
    return CM_Get_Device_ID_List_Size_ExA(len, filter, flags, NULL);
}

static CONFIGRET get_device_interface_list(const GUID *class_guid, DEVINSTID_W device_id, WCHAR *buffer, ULONG *len,
        ULONG flags)
{
    const ULONG supported_flags = CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES;

    BYTE iface_detail_buffer[sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) + 256 * sizeof(WCHAR)];
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *iface_data;
    SP_DEVINFO_DATA device = { sizeof(device) };
    ULONG query_flags = DIGCF_DEVICEINTERFACE;
    CONFIGRET ret = CR_SUCCESS;
    unsigned int i, id_len;
    HDEVINFO set;
    ULONG needed;
    WCHAR *p;

    if (!len || (buffer && !*len))
        return CR_INVALID_POINTER;

    needed = 1;

    if (buffer)
        *buffer = 0;
    if (flags & ~supported_flags)
        FIXME("Flags %#lx are not supported.\n", flags);

    if (!buffer)
        *len = 0;

    if (!buffer)
        *len = needed;

    if (!(flags & CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES)) query_flags |= DIGCF_PRESENT;
    set = SetupDiGetClassDevsW(class_guid, device_id, NULL, query_flags);
    if (set == INVALID_HANDLE_VALUE)
        return CR_SUCCESS;

    iface_data = (SP_DEVICE_INTERFACE_DETAIL_DATA_W *)iface_detail_buffer;
    iface_data->cbSize = sizeof(*iface_data);

    p = buffer;
    for (i = 0; SetupDiEnumDeviceInterfaces(set, NULL, class_guid, i, &iface); ++i)
    {
        ret = SetupDiGetDeviceInterfaceDetailW(set, &iface, iface_data, sizeof(iface_detail_buffer), NULL, &device);
        if (!ret) continue;
        id_len = wcslen(iface_data->DevicePath) + 1;
        needed += id_len;
        if (buffer)
        {
            if (needed > *len)
            {
                SetupDiDestroyDeviceInfoList(set);
                *buffer = 0;
                return CR_BUFFER_SMALL;
            }
            memcpy(p, iface_data->DevicePath, sizeof(*p) * id_len);
            p += id_len;
        }
    }
    SetupDiDestroyDeviceInfoList(set);
    *len = needed;
    if (buffer)
        *p = 0;
    return CR_SUCCESS;
}

/***********************************************************************
 *      CM_Get_Device_Interface_List_Size_ExW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_List_Size_ExW(PULONG len, LPGUID class, DEVINSTID_W id,
                                                       ULONG flags, HMACHINE machine)
{
    TRACE("%p %s %s 0x%08lx %p\n", len, debugstr_guid(class), debugstr_w(id), flags, machine);

    if (machine)
        FIXME("machine %p.\n", machine);

    return get_device_interface_list(class, id, NULL, len, flags);
}

/***********************************************************************
 *      CM_Get_Device_Interface_List_SizeW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_List_SizeW(PULONG len, LPGUID class, DEVINSTID_W id, ULONG flags)
{
    TRACE("%p %s %s 0x%08lx\n", len, debugstr_guid(class), debugstr_w(id), flags);
    return get_device_interface_list(class, id, NULL, len, flags);
}

/***********************************************************************
 *      CM_Get_Device_Interface_List_W (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_List_ExW(LPGUID class, DEVINSTID_W id, PZZWSTR buffer, ULONG len, ULONG flags,
        HMACHINE machine)
{
    TRACE("%s %s %p %lu %#lx\n", debugstr_guid(class), debugstr_w(id), buffer, len, flags);

    if (machine)
        FIXME("machine %p.\n", machine);

    return get_device_interface_list(class, id, buffer, &len, flags);
}

/***********************************************************************
 *      CM_Get_Device_Interface_List_W (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_ListW(LPGUID class, DEVINSTID_W id, PZZWSTR buffer, ULONG len, ULONG flags)
{
    TRACE("%s %s %p %lu %#lx\n", debugstr_guid(class), debugstr_w(id), buffer, len, flags);

    return get_device_interface_list(class, id, buffer, &len, flags);
}

/***********************************************************************
 *      CM_Get_Device_Interface_List_SizeA (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_List_SizeA(PULONG len, LPGUID class, DEVINSTID_A id,
        ULONG flags)
{
    return CM_Get_Device_Interface_List_Size_ExA(len, class, id, flags, NULL);
}

/***********************************************************************
 *      CM_Get_Device_Interface_List_Size_ExA (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_List_Size_ExA(PULONG len, LPGUID class, DEVINSTID_A id,
                                                       ULONG flags, HMACHINE machine)
{
    WCHAR *wid = NULL;
    unsigned int slen;
    CONFIGRET ret;

    TRACE("%p %s %s 0x%08lx %p\n", len, debugstr_guid(class), debugstr_a(id), flags, machine);

    if (machine)
        FIXME("machine %p.\n", machine);

    if (id)
    {
        slen = strlen(id) + 1;
        if (!(wid = malloc(slen * sizeof(*wid))))
            return CR_OUT_OF_MEMORY;
        MultiByteToWideChar(CP_ACP, 0, id, slen, wid, slen);
    }
    ret = CM_Get_Device_Interface_List_SizeW(len, class, wid, flags);
    free(wid);
    return ret;
}

/***********************************************************************
 *      CM_Get_Device_Interface_List_ExA (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_List_ExA(LPGUID class, DEVINSTID_A id, PZZSTR buffer, ULONG len, ULONG flags,
        HMACHINE machine)
{
    WCHAR *wbuffer, *wid = NULL, *p;
    unsigned int slen;
    CONFIGRET ret;

    TRACE("%s %s %p %lu 0x%08lx %p\n", debugstr_guid(class), debugstr_a(id), buffer, len, flags, machine);

    if (machine)
        FIXME("machine %p.\n", machine);

    if (!buffer || !len)
        return CR_INVALID_POINTER;

    if (!(wbuffer = malloc(len * sizeof(*wbuffer))))
        return CR_OUT_OF_MEMORY;

    if (id)
    {
        slen = strlen(id) + 1;
        if (!(wid = malloc(slen * sizeof(*wid))))
        {
            free(wbuffer);
            return CR_OUT_OF_MEMORY;
        }
        MultiByteToWideChar(CP_ACP, 0, id, slen, wid, slen);
    }

    if (!(ret = CM_Get_Device_Interface_List_ExW(class, wid, wbuffer, len, flags, machine)))
    {
        p = wbuffer;
        while (*p)
        {
            slen = wcslen(p) + 1;
            WideCharToMultiByte(CP_ACP, 0, p, slen, buffer, slen, NULL, NULL);
            p += slen;
            buffer += slen;
        }
        *buffer = 0;
    }
    free(wid);
    free(wbuffer);
    return ret;
}

/***********************************************************************
 *      CM_Get_Device_Interface_ListA (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_ListA(LPGUID class, DEVINSTID_A id, PZZSTR buffer, ULONG len, ULONG flags)
{
    return CM_Get_Device_Interface_List_ExA(class, id, buffer, len, flags, NULL);
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
        if (!(class_nameW = malloc(size * sizeof(WCHAR))))
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

    free(class_nameW);
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
    DWORD class_name_len;
    WCHAR buffer[MAX_PATH];
    INFCONTEXT inf_ctx;
    HINF hinf;
    BOOL retval = FALSE;

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

    if ((hinf = SetupOpenInfFileW(inf, NULL, INF_STYLE_WIN4, NULL)) == INVALID_HANDLE_VALUE)
    {
        ERR("failed to open INF file %s\n", debugstr_w(inf));
        return FALSE;
    }

    if (!SetupFindFirstLineW(hinf, L"Version", L"Signature", &inf_ctx))
    {
        ERR("INF file %s does not have mandatory [Version].Signature\n", debugstr_w(inf));
        goto out;
    }

    if (!SetupGetStringFieldW(&inf_ctx, 1, buffer, ARRAY_SIZE(buffer), NULL))
    {
        ERR("failed to get [Version].Signature string from %s\n", debugstr_w(inf));
        goto out;
    }

    if (lstrcmpiW(buffer, L"$Chicago$") && lstrcmpiW(buffer, L"$Windows NT$"))
    {
        ERR("INF file %s has invalid [Version].Signature: %s\n", debugstr_w(inf), debugstr_w(buffer));
        goto out;
    }

    have_guid = SetupFindFirstLineW(hinf, L"Version", L"ClassGUID", &inf_ctx);

    if (have_guid)
    {
        if (!SetupGetStringFieldW(&inf_ctx, 1, buffer, ARRAY_SIZE(buffer), NULL))
        {
            ERR("failed to get [Version].ClassGUID as a string from '%s'\n", debugstr_w(inf));
            goto out;
        }

        buffer[lstrlenW(buffer)-1] = 0;
        if (RPC_S_OK != UuidFromStringW(buffer + 1, class_guid))
        {
            ERR("INF file %s has invalid [Version].ClassGUID: %s\n", debugstr_w(inf), debugstr_w(buffer));
            SetLastError(ERROR_INVALID_PARAMETER);
            goto out;
        }
    }

    have_name = SetupFindFirstLineW(hinf, L"Version", L"Class", &inf_ctx);

    class_name_len = 0;
    if (have_name)
    {
        if (!SetupGetStringFieldW(&inf_ctx, 1, buffer, ARRAY_SIZE(buffer), NULL))
        {
            ERR("failed to get [Version].Class as a string from '%s'\n", debugstr_w(inf));
            goto out;
        }

        class_name_len = lstrlenW(buffer);
    }

    if (have_guid && !have_name)
    {
        class_name[0] = '\0';
        FIXME("class name lookup via guid not implemented\n");
    }

    if (have_name)
    {
        if (class_name_len < size) lstrcpyW(class_name, buffer);
        else
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            have_name = FALSE;
        }
    }

    if (required_size) *required_size = class_name_len + ((class_name_len) ? 1 : 0);

    retval = (have_guid || have_name);
out:
    SetupCloseInfFile(hinf);
    return retval;
}

static LSTATUS get_device_property(struct device *device, const DEVPROPKEY *prop_key, DEVPROPTYPE *prop_type,
                BYTE *prop_buff, DWORD prop_buff_size, DWORD *required_size, DWORD flags)
{
    WCHAR key_path[55] = L"Properties\\";
    HKEY hkey;
    DWORD value_type;
    DWORD value_size = 0;
    LSTATUS ls;

    if (!prop_key)
        return ERROR_INVALID_DATA;

    if (!prop_type || (!prop_buff && prop_buff_size))
        return ERROR_INVALID_USER_BUFFER;

    if (flags)
        return ERROR_INVALID_FLAGS;

    SETUPDI_GuidToString(&prop_key->fmtid, key_path + 11);
    swprintf(key_path + 49, ARRAY_SIZE(key_path) - 49, L"\\%04X", prop_key->pid);

    ls = RegOpenKeyExW(device->key, key_path, 0, KEY_QUERY_VALUE, &hkey);
    if (!ls)
    {
        value_size = prop_buff_size;
        ls = RegQueryValueExW(hkey, NULL, NULL, &value_type, prop_buff, &value_size);
        RegCloseKey(hkey);
    }

    switch (ls)
    {
    case NO_ERROR:
    case ERROR_MORE_DATA:
        *prop_type = 0xffff & value_type;
        ls = (ls == ERROR_MORE_DATA || !prop_buff) ? ERROR_INSUFFICIENT_BUFFER : NO_ERROR;
        break;
    case ERROR_FILE_NOT_FOUND:
        *prop_type = DEVPROP_TYPE_EMPTY;
        value_size = 0;
        ls = ERROR_NOT_FOUND;
        break;
    default:
        *prop_type = DEVPROP_TYPE_EMPTY;
        value_size = 0;
        FIXME("Unhandled error %#lx\n", ls);
        break;
    }

    if (required_size)
        *required_size = value_size;

    return ls;
}

BOOL WINAPI SetupDiGetDevicePropertyKeys( HDEVINFO devinfo, PSP_DEVINFO_DATA device_data,
                                          DEVPROPKEY *prop_keys, DWORD prop_keys_len,
                                          DWORD *required_prop_keys, DWORD flags )
{
    struct device *device;
    DWORD count = 0, i;
    HKEY hkey;
    LSTATUS ls;
    DEVPROPKEY *keys_buf = NULL;

    TRACE( "%p, %p, %p, %lu, %p, %#lx\n", devinfo, device_data, prop_keys, prop_keys_len,
           required_prop_keys, flags);

    if (flags)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return FALSE;
    }
    if (!prop_keys && prop_keys_len)
    {
        SetLastError( ERROR_INVALID_USER_BUFFER );
        return FALSE;
    }

    device = get_device( devinfo, device_data );
    if (!device)
        return FALSE;

    ls = RegOpenKeyExW( device->key, L"Properties", 0, KEY_ENUMERATE_SUB_KEYS, &hkey );
    if (ls)
    {
        SetLastError( ls );
        return FALSE;
    }

    keys_buf = malloc( sizeof( *keys_buf ) * prop_keys_len );
    if (!keys_buf && prop_keys_len)
    {
        RegCloseKey( hkey );
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    for (i = 0; ;i++)
    {
        WCHAR guid_str[39];
        HKEY propkey;
        DWORD len, j;
        GUID prop_guid;

        len = ARRAY_SIZE( guid_str );
        ls = RegEnumKeyExW( hkey, i, guid_str, &len, NULL, NULL, NULL, NULL );
        if (ls)
        {
            if (ls == ERROR_NO_MORE_ITEMS)
                ls = ERROR_SUCCESS;
            else
                ERR( "Could not enumerate subkeys for device %s: %lu\n",
                     debugstr_w( device->instanceId ), ls );
            break;
        }
        ls = RegOpenKeyExW( hkey, guid_str, 0, KEY_ENUMERATE_SUB_KEYS, &propkey );
        if (ls)
            break;
        guid_str[37] = 0;
        if (UuidFromStringW( &guid_str[1], &prop_guid ))
        {
            ERR( "Could not parse propkey GUID string %s\n", debugstr_w( &guid_str[1] ) );
            RegCloseKey( propkey );
            continue;
        }
        for (j = 0; ;j++)
        {
            DEVPROPID pid;
            WCHAR key_name[6];

            len = 5;
            ls = RegEnumKeyExW( propkey, j, key_name, &len, NULL, NULL, NULL, NULL );
            if (ls)
            {
                if (ls != ERROR_NO_MORE_ITEMS)
                    ERR( "Could not enumerate subkeys for device %s under %s: %lu\n", debugstr_w( device->instanceId ),
                         debugstr_guid( &prop_guid ), ls );
                break;
            }
            swscanf( key_name, L"%04X", &pid );
            if (++count <= prop_keys_len)
            {
                keys_buf[count-1].fmtid = prop_guid;
                keys_buf[count-1].pid = pid;
            }
        }
        RegCloseKey( propkey );
    }

    RegCloseKey( hkey );
    if (!ls)
    {
        if (required_prop_keys)
            *required_prop_keys = count;

        if (prop_keys_len < count)
        {
            free( keys_buf );
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }
        memcpy( prop_keys, keys_buf, count * sizeof( *keys_buf ) );
    }
    free( keys_buf );
    SetLastError( ls );
    return !ls;
}

/***********************************************************************
 *              SetupDiGetDevicePropertyW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDevicePropertyW(HDEVINFO devinfo, PSP_DEVINFO_DATA device_data,
                const DEVPROPKEY *prop_key, DEVPROPTYPE *prop_type, BYTE *prop_buff,
                DWORD prop_buff_size, DWORD *required_size, DWORD flags)
{
    struct device *device;
    LSTATUS ls;

    TRACE("%p, %p, %p, %p, %p, %ld, %p, %#lx\n", devinfo, device_data, prop_key, prop_type, prop_buff, prop_buff_size,
          required_size, flags);

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

    ls = get_device_property(device, prop_key, prop_type, prop_buff, prop_buff_size, required_size, flags);

    SetLastError(ls);
    return !ls;
}

/***********************************************************************
 *              CM_Get_DevNode_Property_ExW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_DevNode_Property_ExW(DEVINST devnode, const DEVPROPKEY *prop_key, DEVPROPTYPE *prop_type,
    BYTE *prop_buff, ULONG *prop_buff_size, ULONG flags, HMACHINE machine)
{
    HDEVINFO set;
    struct device *device;
    LSTATUS ls;

    TRACE("%lu, %p, %p, %p, %p, %#lx, %p\n", devnode, prop_key, prop_type, prop_buff, prop_buff_size,
          flags, machine);

    if (machine)
        return CR_MACHINE_UNAVAILABLE;

    if (!prop_buff_size)
        return CR_INVALID_POINTER;

    if (!(device = get_devnode_device(devnode, &set)))
        return CR_NO_SUCH_DEVINST;

    ls = get_device_property(device, prop_key, prop_type, prop_buff, *prop_buff_size, prop_buff_size, flags);
    SetupDiDestroyDeviceInfoList(set);
    switch (ls)
    {
    case NO_ERROR:
        return CR_SUCCESS;
    case ERROR_INVALID_DATA:
        return CR_INVALID_DATA;
    case ERROR_INVALID_USER_BUFFER:
        return CR_INVALID_POINTER;
    case ERROR_INVALID_FLAGS:
        return CR_INVALID_FLAG;
    case ERROR_INSUFFICIENT_BUFFER:
        return CR_BUFFER_SMALL;
    case ERROR_NOT_FOUND:
        return CR_NO_SUCH_VALUE;
    }
    return CR_FAILURE;
}

/***********************************************************************
 *              CM_Get_DevNode_PropertyW (SETUPAPI.@)
 */
CONFIGRET WINAPI CM_Get_DevNode_PropertyW(DEVINST dev, const DEVPROPKEY *key, DEVPROPTYPE *type,
    PVOID buf, PULONG len, ULONG flags)
{
    return CM_Get_DevNode_Property_ExW(dev, key, type, buf, len, flags, NULL);
}

/***********************************************************************
 *              SetupDiInstallDeviceInterfaces (SETUPAPI.@)
 */
BOOL WINAPI SetupDiInstallDeviceInterfaces(HDEVINFO devinfo, SP_DEVINFO_DATA *device_data)
{
    WCHAR section_ext[LINE_LEN], iface_section[LINE_LEN], refstr[LINE_LEN], guidstr[39];
    UINT install_flags = SPINST_ALL;
    struct device_iface *iface;
    struct device *device;
    struct driver *driver;
    void *callback_ctx;
    GUID iface_guid;
    INFCONTEXT ctx;
    HKEY iface_key;
    HINF hinf;
    LONG l;

    TRACE("devinfo %p, device_data %p.\n", devinfo, device_data);

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

    if (!(driver = device->selected_driver))
    {
        ERR("No driver selected for device %p.\n", devinfo);
        SetLastError(ERROR_NO_DRIVER_SELECTED);
        return FALSE;
    }

    if ((hinf = SetupOpenInfFileW(driver->inf_path, NULL, INF_STYLE_WIN4, NULL)) == INVALID_HANDLE_VALUE)
        return FALSE;

    SetupDiGetActualSectionToInstallW(hinf, driver->section, section_ext, ARRAY_SIZE(section_ext), NULL, NULL);

    if (device->params.Flags & DI_NOFILECOPY)
        install_flags &= ~SPINST_FILES;

    callback_ctx = SetupInitDefaultQueueCallback(NULL);

    lstrcatW(section_ext, L".Interfaces");
    if (SetupFindFirstLineW(hinf, section_ext, L"AddInterface", &ctx))
    {
        do {
            SetupGetStringFieldW(&ctx, 1, guidstr, ARRAY_SIZE(guidstr), NULL);
            SetupGetStringFieldW(&ctx, 2, refstr, ARRAY_SIZE(refstr), NULL);
            guidstr[37] = 0;
            UuidFromStringW(&guidstr[1], &iface_guid);

            if (!(iface = SETUPDI_CreateDeviceInterface(device, &iface_guid, refstr)))
            {
                ERR("Failed to create device interface, error %#lx.\n", GetLastError());
                continue;
            }

            if ((l = create_iface_key(iface, KEY_ALL_ACCESS, &iface_key)))
            {
                ERR("Failed to create interface key, error %lu.\n", l);
                continue;
            }

            SetupGetStringFieldW(&ctx, 3, iface_section, ARRAY_SIZE(iface_section), NULL);
            SetupInstallFromInfSectionW(NULL, hinf, iface_section, install_flags, iface_key,
                    NULL, SP_COPY_NEWER_ONLY, SetupDefaultQueueCallbackW, callback_ctx, NULL, NULL);

            RegCloseKey(iface_key);
        } while (SetupFindNextMatchLineW(&ctx, L"AddInterface", &ctx));
    }

    SetupTermDefaultQueueCallback(callback_ctx);

    SetupCloseInfFile(hinf);
    return TRUE;
}

/***********************************************************************
 *              SetupDiRegisterCoDeviceInstallers (SETUPAPI.@)
 */
BOOL WINAPI SetupDiRegisterCoDeviceInstallers(HDEVINFO devinfo, SP_DEVINFO_DATA *device_data)
{
    WCHAR coinst_key_ext[LINE_LEN];
    struct device *device;
    struct driver *driver;
    void *callback_ctx;
    HKEY driver_key;
    HINF hinf;
    LONG l;

    TRACE("devinfo %p, device_data %p.\n", devinfo, device_data);

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

    if (!(driver = device->selected_driver))
    {
        ERR("No driver selected for device %p.\n", devinfo);
        SetLastError(ERROR_NO_DRIVER_SELECTED);
        return FALSE;
    }

    if ((hinf = SetupOpenInfFileW(driver->inf_path, NULL, INF_STYLE_WIN4, NULL)) == INVALID_HANDLE_VALUE)
        return FALSE;

    SetupDiGetActualSectionToInstallW(hinf, driver->section, coinst_key_ext, ARRAY_SIZE(coinst_key_ext), NULL, NULL);
    lstrcatW(coinst_key_ext, L".CoInstallers");

    if ((l = create_driver_key(device, &driver_key)))
    {
        SetLastError(l);
        SetupCloseInfFile(hinf);
        return FALSE;
    }

    callback_ctx = SetupInitDefaultQueueCallback(NULL);
    SetupInstallFromInfSectionW(NULL, hinf, coinst_key_ext, SPINST_ALL, driver_key, NULL,
            SP_COPY_NEWER_ONLY, SetupDefaultQueueCallbackW, callback_ctx, NULL, NULL);
    SetupTermDefaultQueueCallback(callback_ctx);

    RegCloseKey(driver_key);
    SetupCloseInfFile(hinf);
    return TRUE;
}

/* Check whether the given hardware or compatible ID matches any of the device's
 * own hardware or compatible IDs. */
static BOOL device_matches_id(const struct device *device, const WCHAR *id_type, const WCHAR *id,
                              DWORD *driver_rank)
{
    WCHAR *device_ids;
    const WCHAR *p;
    DWORD i, size;

    if (!RegGetValueW(device->key, NULL, id_type, RRF_RT_REG_MULTI_SZ, NULL, NULL, &size))
    {
        device_ids = malloc(size);
        if (!RegGetValueW(device->key, NULL, id_type, RRF_RT_REG_MULTI_SZ, NULL, device_ids, &size))
        {
            for (p = device_ids, i = 0; *p; p += lstrlenW(p) + 1, i++)
            {
                if (!wcsicmp(p, id))
                {
                    *driver_rank += min(i, 0xff);
                    free(device_ids);
                    return TRUE;
                }
            }
        }
        free(device_ids);
    }

    return FALSE;
}

static BOOL version_is_compatible(const WCHAR *version)
{
    const WCHAR *machine_ext = NtPlatformExtension + 1, *p;
    size_t len = lstrlenW(version);
    BOOL wow64;

    /* We are only concerned with architecture. */
    if ((p = wcschr(version, '.')))
        len = p - version;

    if (!wcsnicmp(version, L"NT", len))
        return TRUE;

    if (IsWow64Process(GetCurrentProcess(), &wow64) && wow64)
    {
#ifdef __i386__
        static const WCHAR wow_ext[] = L"NTamd64";
        machine_ext = wow_ext;
#elif defined(__arm__)
        static const WCHAR wow_ext[] = L"NTarm64";
        machine_ext = wow_ext;
#endif
    }

    return !wcsnicmp(version, machine_ext, len);
}

static bool any_version_is_compatible(INFCONTEXT *ctx)
{
    WCHAR version[LINE_LEN];
    DWORD j;

    if (SetupGetFieldCount(ctx) < 2)
        return true;

    for (j = 2; SetupGetStringFieldW(ctx, j, version, ARRAY_SIZE(version), NULL); ++j)
    {
        if (version_is_compatible(version))
            return true;
    }

    return false;
}

static void enum_compat_drivers_from_file(struct device *device, const WCHAR *path)
{
    WCHAR mfg_key[LINE_LEN], id[MAX_DEVICE_ID_LEN];
    DWORD i, j, k, driver_count = device->driver_count;
    struct driver driver, *drivers = device->drivers;
    INFCONTEXT ctx;
    BOOL found;
    HINF hinf;

    TRACE("Enumerating drivers from %s.\n", debugstr_w(path));

    if ((hinf = SetupOpenInfFileW(path, NULL, INF_STYLE_WIN4, NULL)) == INVALID_HANDLE_VALUE)
        return;

    lstrcpyW(driver.inf_path, path);

    for (i = 0; SetupGetLineByIndexW(hinf, L"Manufacturer", i, &ctx); ++i)
    {
        SetupGetStringFieldW(&ctx, 0, driver.manufacturer, ARRAY_SIZE(driver.manufacturer), NULL);
        if (!SetupGetStringFieldW(&ctx, 1, mfg_key, ARRAY_SIZE(mfg_key), NULL))
            lstrcpyW(mfg_key, driver.manufacturer);

        if (!any_version_is_compatible(&ctx))
            continue;

        if (!SetupDiGetActualSectionToInstallW(hinf, mfg_key, driver.mfg_key,
                ARRAY_SIZE(driver.mfg_key), NULL, NULL))
        {
            WARN("Failed to find section for %s, skipping.\n", debugstr_w(mfg_key));
            continue;
        }

        for (j = 0; SetupGetLineByIndexW(hinf, driver.mfg_key, j, &ctx); ++j)
        {
            driver.rank = 0;
            for (k = 2, found = FALSE; SetupGetStringFieldW(&ctx, k, id, ARRAY_SIZE(id), NULL); ++k)
            {
                if ((found = device_matches_id(device, L"HardwareId", id, &driver.rank))) break;
                driver.rank += 0x2000;
                if ((found = device_matches_id(device, L"CompatibleIDs", id, &driver.rank))) break;
                driver.rank = 0x1000 + min(0x0100 * (k - 2), 0xf00);
            }

            if (found)
            {
                SetupGetStringFieldW(&ctx, 0, driver.description, ARRAY_SIZE(driver.description), NULL);
                SetupGetStringFieldW(&ctx, 1, driver.section, ARRAY_SIZE(driver.section), NULL);

                TRACE("Found compatible driver: rank %#lx manufacturer %s, desc %s.\n",
                        driver.rank, debugstr_w(driver.manufacturer), debugstr_w(driver.description));

                driver_count++;
                drivers = realloc(drivers, driver_count * sizeof(*drivers));
                drivers[driver_count - 1] = driver;
            }
        }
    }

    SetupCloseInfFile(hinf);

    device->drivers = drivers;
    device->driver_count = driver_count;
}

/***********************************************************************
 *              SetupDiBuildDriverInfoList (SETUPAPI.@)
 */
BOOL WINAPI SetupDiBuildDriverInfoList(HDEVINFO devinfo, SP_DEVINFO_DATA *device_data, DWORD type)
{
    struct device *device;

    TRACE("devinfo %p, device_data %p, type %#lx.\n", devinfo, device_data, type);

    if (type != SPDIT_COMPATDRIVER)
    {
        FIXME("Unhandled type %#lx.\n", type);
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

    if (device->params.Flags & DI_ENUMSINGLEINF)
    {
        enum_compat_drivers_from_file(device, device->params.DriverPath);
    }
    else
    {
        WCHAR dir[MAX_PATH], file[MAX_PATH];
        WIN32_FIND_DATAW find_data;
        HANDLE find_handle;

        if (device->params.DriverPath[0])
            lstrcpyW(dir, device->params.DriverPath);
        else
            lstrcpyW(dir, L"C:/windows/inf");
        lstrcatW(dir, L"\\*");

        TRACE("Searching for drivers in %s.\n", debugstr_w(dir));

        if ((find_handle = FindFirstFileW(dir, &find_data)) != INVALID_HANDLE_VALUE)
        {
            do
            {
                lstrcpyW(file, dir);
                lstrcpyW(file + lstrlenW(file) - 1, find_data.cFileName);
                enum_compat_drivers_from_file(device, file);
            } while (FindNextFileW(find_handle, &find_data));

            FindClose(find_handle);
        }
    }

    if (device->driver_count)
    {
        WCHAR classname[MAX_CLASS_NAME_LEN], guidstr[39];
        GUID class;

        if (SetupDiGetINFClassW(device->drivers[0].inf_path, &class, classname, ARRAY_SIZE(classname), NULL))
        {
            device_data->ClassGuid = device->class = class;
            SETUPDI_GuidToString(&class, guidstr);
            RegSetValueExW(device->key, L"ClassGUID", 0, REG_SZ, (BYTE *)guidstr, sizeof(guidstr));
            RegSetValueExW(device->key, L"Class", 0, REG_SZ, (BYTE *)classname, wcslen(classname) * sizeof(WCHAR));
        }
    }

    return TRUE;
}

static BOOL copy_driver_data(SP_DRVINFO_DATA_W *data, const struct driver *driver)
{
    INFCONTEXT ctx;
    HINF hinf;

    if ((hinf = SetupOpenInfFileW(driver->inf_path, NULL, INF_STYLE_WIN4, NULL)) == INVALID_HANDLE_VALUE)
        return FALSE;

    data->ProviderName[0] = 0;
    if (SetupFindFirstLineW(hinf, L"Version", L"Provider", &ctx))
        SetupGetStringFieldW(&ctx, 1, data->ProviderName, ARRAY_SIZE(data->ProviderName), NULL);
    wcscpy(data->Description, driver->description);
    wcscpy(data->MfgName, driver->manufacturer);
    data->DriverType = SPDIT_COMPATDRIVER;
    data->Reserved = (ULONG_PTR)driver;

    SetupCloseInfFile(hinf);

    return TRUE;
}

static void driver_data_wtoa(SP_DRVINFO_DATA_A *a, const SP_DRVINFO_DATA_W *w)
{
    a->DriverType = w->DriverType;
    a->Reserved = w->Reserved;
    WideCharToMultiByte(CP_ACP, 0, w->Description, -1, a->Description, sizeof(a->Description), NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, w->MfgName, -1, a->MfgName, sizeof(a->MfgName), NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, w->ProviderName, -1, a->ProviderName, sizeof(a->ProviderName), NULL, NULL);
}

/***********************************************************************
 *              SetupDiEnumDriverInfoW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiEnumDriverInfoW(HDEVINFO devinfo, SP_DEVINFO_DATA *device_data,
        DWORD type, DWORD index, SP_DRVINFO_DATA_W *driver_data)
{
    struct device *device;

    TRACE("devinfo %p, device_data %p, type %#lx, index %lu, driver_data %p.\n",
            devinfo, device_data, type, index, driver_data);

    if (type != SPDIT_COMPATDRIVER)
    {
        FIXME("Unhandled type %#lx.\n", type);
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

    if (index >= device->driver_count)
    {
        SetLastError(ERROR_NO_MORE_ITEMS);
        return FALSE;
    }

    return copy_driver_data(driver_data, &device->drivers[index]);
}

/***********************************************************************
 *              SetupDiEnumDriverInfoA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiEnumDriverInfoA(HDEVINFO devinfo, SP_DEVINFO_DATA *device_data,
        DWORD type, DWORD index, SP_DRVINFO_DATA_A *driver_data)
{
    SP_DRVINFO_DATA_W driver_dataW;
    BOOL ret;

    driver_dataW.cbSize = sizeof(driver_dataW);
    ret = SetupDiEnumDriverInfoW(devinfo, device_data, type, index, &driver_dataW);
    if (ret) driver_data_wtoa(driver_data, &driver_dataW);

    return ret;
}

/***********************************************************************
 *              SetupDiSelectBestCompatDrv (SETUPAPI.@)
 */
BOOL WINAPI SetupDiSelectBestCompatDrv(HDEVINFO devinfo, SP_DEVINFO_DATA *device_data)
{
    struct device *device;
    struct driver *best;
    DWORD i;

    TRACE("devinfo %p, device_data %p.\n", devinfo, device_data);

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

    if (!device->driver_count)
    {
        WARN("No compatible drivers were enumerated for device %s.\n", debugstr_w(device->instanceId));
        SetLastError(ERROR_NO_COMPAT_DRIVERS);
        return FALSE;
    }

    best = device->drivers;
    for (i = 1; i < device->driver_count; ++i)
    {
        if (device->drivers[i].rank >= best->rank) continue;
        best = device->drivers + i;
    }

    TRACE("selected driver: rank %#lx manufacturer %s, desc %s.\n",
            best->rank, debugstr_w(best->manufacturer), debugstr_w(best->description));

    device->selected_driver = best;
    return TRUE;
}

/***********************************************************************
 *              SetupDiGetSelectedDriverW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetSelectedDriverW(HDEVINFO devinfo, SP_DEVINFO_DATA *device_data, SP_DRVINFO_DATA_W *driver_data)
{
    struct device *device;

    TRACE("devinfo %p, device_data %p, driver_data %p.\n", devinfo, device_data, driver_data);

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

    if (!device->selected_driver)
    {
        SetLastError(ERROR_NO_DRIVER_SELECTED);
        return FALSE;
    }

    return copy_driver_data(driver_data, device->selected_driver);
}

/***********************************************************************
 *              SetupDiGetSelectedDriverA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetSelectedDriverA(HDEVINFO devinfo, SP_DEVINFO_DATA *device_data, SP_DRVINFO_DATA_A *driver_data)
{
    SP_DRVINFO_DATA_W driver_dataW;
    BOOL ret;

    driver_dataW.cbSize = sizeof(driver_dataW);
    if ((ret = SetupDiGetSelectedDriverW(devinfo, device_data, &driver_dataW)))
        driver_data_wtoa(driver_data, &driver_dataW);
    return ret;
}

/***********************************************************************
 *              SetupDiGetDriverInfoDetailW (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDriverInfoDetailW(HDEVINFO devinfo, SP_DEVINFO_DATA *device_data,
        SP_DRVINFO_DATA_W *driver_data, SP_DRVINFO_DETAIL_DATA_W *detail_data, const DWORD size, DWORD *ret_size)
{
    struct driver *driver = (struct driver *)driver_data->Reserved;
    DWORD size_needed, i, id_size = 1;
    WCHAR id[MAX_DEVICE_ID_LEN];
    INFCONTEXT ctx;
    HANDLE file;
    HINF hinf;

    TRACE("devinfo %p, device_data %p, driver_data %p, detail_data %p, size %lu, ret_size %p.\n",
            devinfo, device_data, driver_data, detail_data, size, ret_size);

    if ((detail_data || size) && size < sizeof(SP_DRVINFO_DETAIL_DATA_W))
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if ((hinf = SetupOpenInfFileW(driver->inf_path, NULL, INF_STYLE_WIN4, NULL)) == INVALID_HANDLE_VALUE)
        return FALSE;

    SetupFindFirstLineW(hinf, driver->mfg_key, driver->description, &ctx);
    for (i = 2; SetupGetStringFieldW(&ctx, i, id, ARRAY_SIZE(id), NULL); ++i)
        id_size += wcslen(id) + 1;

    size_needed = FIELD_OFFSET(SP_DRVINFO_DETAIL_DATA_W, HardwareID[id_size]);
    if (ret_size)
        *ret_size = size_needed;
    if (!detail_data)
        return TRUE;

    detail_data->CompatIDsLength = detail_data->CompatIDsOffset = 0;
    detail_data->HardwareID[0] = 0;

    if (size >= size_needed)
    {
        id_size = 0;
        for (i = 2; SetupGetStringFieldW(&ctx, i, id, ARRAY_SIZE(id), NULL); ++i)
        {
            wcscpy(&detail_data->HardwareID[id_size], id);
            if (i == 3)
                detail_data->CompatIDsOffset = id_size;
            id_size += wcslen(id) + 1;
        }
        detail_data->HardwareID[id_size++] = 0;
        if (i > 3)
            detail_data->CompatIDsLength = id_size - detail_data->CompatIDsOffset;
    }

    SetupCloseInfFile(hinf);

    if ((file = CreateFileW(driver->inf_path, 0, 0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
        return FALSE;
    GetFileTime(file, NULL, NULL, &detail_data->InfDate);
    CloseHandle(file);

    wcscpy(detail_data->SectionName, driver->section);
    wcscpy(detail_data->InfFileName, driver->inf_path);
    wcscpy(detail_data->DrvDescription, driver->description);

    if (size < size_needed)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *              SetupDiGetDriverInfoDetailA (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDriverInfoDetailA(HDEVINFO devinfo, SP_DEVINFO_DATA *device_data,
        SP_DRVINFO_DATA_A *driver_data, SP_DRVINFO_DETAIL_DATA_A *detail_data, const DWORD size, DWORD *ret_size)
{
    struct driver *driver = (struct driver *)driver_data->Reserved;
    DWORD size_needed, i, id_size = 1;
    char id[MAX_DEVICE_ID_LEN];
    INFCONTEXT ctx;
    HANDLE file;
    HINF hinf;

    TRACE("devinfo %p, device_data %p, driver_data %p, detail_data %p, size %lu, ret_size %p.\n",
            devinfo, device_data, driver_data, detail_data, size, ret_size);

    if ((detail_data || size) && size < sizeof(SP_DRVINFO_DETAIL_DATA_A))
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if ((hinf = SetupOpenInfFileW(driver->inf_path, NULL, INF_STYLE_WIN4, NULL)) == INVALID_HANDLE_VALUE)
        return FALSE;

    SetupFindFirstLineW(hinf, driver->mfg_key, driver->description, &ctx);
    for (i = 2; SetupGetStringFieldA(&ctx, i, id, ARRAY_SIZE(id), NULL); ++i)
        id_size += strlen(id) + 1;

    size_needed = FIELD_OFFSET(SP_DRVINFO_DETAIL_DATA_A, HardwareID[id_size]);
    if (ret_size)
        *ret_size = size_needed;
    if (!detail_data)
    {
        SetupCloseInfFile(hinf);
        return TRUE;
    }

    detail_data->CompatIDsLength = detail_data->CompatIDsOffset = 0;
    detail_data->HardwareID[0] = 0;

    if (size >= size_needed)
    {
        id_size = 0;
        for (i = 2; SetupGetStringFieldA(&ctx, i, id, ARRAY_SIZE(id), NULL); ++i)
        {
            strcpy(&detail_data->HardwareID[id_size], id);
            if (i == 3)
                detail_data->CompatIDsOffset = id_size;
            id_size += strlen(id) + 1;
        }
        detail_data->HardwareID[id_size++] = 0;
        if (i > 3)
            detail_data->CompatIDsLength = id_size - detail_data->CompatIDsOffset;
    }

    SetupCloseInfFile(hinf);

    if ((file = CreateFileW(driver->inf_path, 0, 0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
        return FALSE;
    GetFileTime(file, NULL, NULL, &detail_data->InfDate);
    CloseHandle(file);

    WideCharToMultiByte(CP_ACP, 0, driver->section, -1, detail_data->SectionName,
            sizeof(detail_data->SectionName), NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, driver->inf_path, -1, detail_data->InfFileName,
            sizeof(detail_data->InfFileName), NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, driver->description, -1, detail_data->DrvDescription,
            sizeof(detail_data->InfFileName), NULL, NULL);

    if (size < size_needed)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *              SetupDiInstallDriverFiles (SETUPAPI.@)
 */
BOOL WINAPI SetupDiInstallDriverFiles(HDEVINFO devinfo, SP_DEVINFO_DATA *device_data)
{
    WCHAR section[LINE_LEN], section_ext[LINE_LEN], iface_section[LINE_LEN];
    struct device *device;
    struct driver *driver;
    void *callback_ctx;
    INFCONTEXT ctx;
    HINF hinf;

    TRACE("devinfo %p, device_data %p.\n", devinfo, device_data);

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

    if (!(driver = device->selected_driver))
    {
        ERR("No driver selected for device %p.\n", devinfo);
        SetLastError(ERROR_NO_DRIVER_SELECTED);
        return FALSE;
    }

    if ((hinf = SetupOpenInfFileW(driver->inf_path, NULL, INF_STYLE_WIN4, NULL)) == INVALID_HANDLE_VALUE)
        return FALSE;

    SetupFindFirstLineW(hinf, driver->mfg_key, driver->description, &ctx);
    SetupGetStringFieldW(&ctx, 1, section, ARRAY_SIZE(section), NULL);
    SetupDiGetActualSectionToInstallW(hinf, section, section_ext, ARRAY_SIZE(section_ext), NULL, NULL);

    callback_ctx = SetupInitDefaultQueueCallback(NULL);

    SetupInstallFromInfSectionW(NULL, hinf, section_ext, SPINST_FILES, NULL, NULL,
            SP_COPY_NEWER_ONLY, SetupDefaultQueueCallbackW, callback_ctx, NULL, NULL);

    lstrcatW(section_ext, L".Interfaces");
    if (SetupFindFirstLineW(hinf, section_ext, L"AddInterface", &ctx))
    {
        do {
            SetupGetStringFieldW(&ctx, 3, iface_section, ARRAY_SIZE(iface_section), NULL);
            SetupInstallFromInfSectionW(NULL, hinf, iface_section, SPINST_FILES, NULL, NULL,
                    SP_COPY_NEWER_ONLY, SetupDefaultQueueCallbackW, callback_ctx, NULL, NULL);
        } while (SetupFindNextMatchLineW(&ctx, L"AddInterface", &ctx));
    }

    SetupTermDefaultQueueCallback(callback_ctx);

    SetupCloseInfFile(hinf);
    return TRUE;
}

/***********************************************************************
 *              SetupDiInstallDevice (SETUPAPI.@)
 */
BOOL WINAPI SetupDiInstallDevice(HDEVINFO devinfo, SP_DEVINFO_DATA *device_data)
{
    WCHAR section_ext[LINE_LEN], subsection[LINE_LEN], inf_path[MAX_PATH], *extptr, *filepart;
    static const DWORD config_flags = 0;
    UINT install_flags = SPINST_ALL;
    HKEY driver_key, device_key;
    SC_HANDLE manager, service;
    WCHAR svc_name[LINE_LEN];
    struct device *device;
    struct driver *driver;
    void *callback_ctx;
    INFCONTEXT ctx;
    HINF hinf;
    LONG l;

    TRACE("devinfo %p, device_data %p.\n", devinfo, device_data);

    if (!(device = get_device(devinfo, device_data)))
        return FALSE;

    if (!(driver = device->selected_driver))
    {
        ERR("No driver selected for device %p.\n", devinfo);
        SetLastError(ERROR_NO_DRIVER_SELECTED);
        return FALSE;
    }

    if ((hinf = SetupOpenInfFileW(driver->inf_path, NULL, INF_STYLE_WIN4, NULL)) == INVALID_HANDLE_VALUE)
        return FALSE;

    RegSetValueExW(device->key, L"DeviceDesc", 0, REG_SZ, (BYTE *)driver->description,
            wcslen(driver->description) * sizeof(WCHAR));

    SetupDiGetActualSectionToInstallW(hinf, driver->section, section_ext, ARRAY_SIZE(section_ext), NULL, &extptr);

    if ((l = create_driver_key(device, &driver_key)))
    {
        SetLastError(l);
        SetupCloseInfFile(hinf);
        return FALSE;
    }

    if ((l = RegCreateKeyExW(device->key, L"Device Parameters", 0, NULL, 0,
            KEY_READ | KEY_WRITE, NULL, &device_key, NULL)))
    {
        SetLastError(l);
        RegCloseKey(driver_key);
        SetupCloseInfFile(hinf);
        return FALSE;
    }

    if (!SETUPDI_SetDeviceRegistryPropertyW(device, SPDRP_CONFIGFLAGS,
            (BYTE *)&config_flags, sizeof(config_flags)))
        ERR("Failed to set config flags, error %#lx.\n", GetLastError());

    if (device->params.Flags & DI_NOFILECOPY)
        install_flags &= ~SPINST_FILES;

    callback_ctx = SetupInitDefaultQueueCallback(NULL);

    SetupInstallFromInfSectionW(NULL, hinf, section_ext, install_flags, driver_key, NULL,
            SP_COPY_NEWER_ONLY, SetupDefaultQueueCallbackW, callback_ctx, NULL, NULL);

    lstrcpyW(subsection, section_ext);
    lstrcatW(subsection, L".HW");

    SetupInstallFromInfSectionW(NULL, hinf, subsection, install_flags, device_key, NULL,
            SP_COPY_NEWER_ONLY, SetupDefaultQueueCallbackW, callback_ctx, NULL, NULL);

    lstrcpyW(subsection, section_ext);
    lstrcatW(subsection, L".Services");
    SetupInstallServicesFromInfSectionW(hinf, subsection, 0);

    svc_name[0] = 0;
    if (SetupFindFirstLineW(hinf, subsection, L"AddService", &ctx))
    {
        do
        {
            INT flags;

            if (SetupGetIntField(&ctx, 2, &flags) && (flags & SPSVCINST_ASSOCSERVICE))
            {
                if (SetupGetStringFieldW(&ctx, 1, svc_name, ARRAY_SIZE(svc_name), NULL) && svc_name[0])
                    RegSetValueExW(device->key, L"Service", 0, REG_SZ, (BYTE *)svc_name, lstrlenW(svc_name) * sizeof(WCHAR));
                break;
            }
        } while (SetupFindNextMatchLineW(&ctx, L"AddService", &ctx));
    }

    SetupTermDefaultQueueCallback(callback_ctx);
    SetupCloseInfFile(hinf);

    SetupCopyOEMInfW(driver->inf_path, NULL, SPOST_NONE, 0, inf_path, ARRAY_SIZE(inf_path), NULL, &filepart);
    TRACE("Copied INF file %s to %s.\n", debugstr_w(driver->inf_path), debugstr_w(inf_path));

    RegSetValueExW(driver_key, L"InfPath", 0, REG_SZ, (BYTE *)filepart, lstrlenW(filepart) * sizeof(WCHAR));
    RegSetValueExW(driver_key, L"InfSection", 0, REG_SZ, (BYTE *)driver->section, lstrlenW(driver->section) * sizeof(WCHAR));
    if (extptr)
        RegSetValueExW(driver_key, L"InfSectionExt", 0, REG_SZ, (BYTE *)extptr, lstrlenW(extptr) * sizeof(WCHAR));

    RegCloseKey(device_key);
    RegCloseKey(driver_key);

    if (!wcsnicmp(device->instanceId, L"root\\", strlen("root\\")) && svc_name[0]
            && (manager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT)))
    {
        if ((service = OpenServiceW(manager, svc_name, SERVICE_START | SERVICE_USER_DEFINED_CONTROL)))
        {
            SERVICE_STATUS status;

            if (!StartServiceW(service, 0, NULL) && GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
            {
                ERR("Failed to start service %s for device %s, error %lu.\n",
                        debugstr_w(svc_name), debugstr_w(device->instanceId), GetLastError());
            }
            if (!ControlService(service, SERVICE_CONTROL_REENUMERATE_ROOT_DEVICES, &status))
            {
                ERR("Failed to control service %s for device %s, error %lu.\n",
                        debugstr_w(svc_name), debugstr_w(device->instanceId), GetLastError());
            }
            CloseServiceHandle(service);
        }
        else
            ERR("Failed to open service %s for device %s.\n", debugstr_w(svc_name), debugstr_w(device->instanceId));
        CloseServiceHandle(manager);
    }

    return TRUE;
}

BOOL WINAPI SetupDiGetCustomDevicePropertyA(HDEVINFO devinfo, SP_DEVINFO_DATA *data, const char *name, DWORD flags,
        DWORD *reg_type, BYTE *buffer, DWORD bufsize, DWORD *required)
{
    FIXME("devinfo %p, data %p, name %s, flags %#lx, reg_type %p, buffer %p, bufsize %lu, required %p stub.\n",
            devinfo, data, debugstr_a(name), flags, reg_type, buffer, bufsize, required);

    SetLastError(ERROR_INVALID_DATA);
    return FALSE;
}

BOOL WINAPI SetupDiGetCustomDevicePropertyW(HDEVINFO devinfo, SP_DEVINFO_DATA *data, const WCHAR *name, DWORD flags,
        DWORD *reg_type, BYTE *buffer, DWORD bufsize, DWORD *required)
{
    FIXME("devinfo %p, data %p, name %s, flags %#lx, reg_type %p, buffer %p, bufsize %lu, required %p stub.\n",
            devinfo, data, debugstr_w(name), flags, reg_type, buffer, bufsize, required);

    SetLastError(ERROR_INVALID_DATA);
    return FALSE;
}

/***********************************************************************
 *      SetupCopyOEMInfA  (SETUPAPI.@)
 */
BOOL WINAPI SetupCopyOEMInfA( PCSTR source, PCSTR location,
                              DWORD media_type, DWORD style, PSTR dest,
                              DWORD buffer_size, PDWORD required_size, PSTR *component )
{
    BOOL ret = FALSE;
    LPWSTR destW = NULL, sourceW = NULL, locationW = NULL;
    DWORD size;

    TRACE("%s, %s, %ld, %ld, %p, %ld, %p, %p\n", debugstr_a(source), debugstr_a(location),
          media_type, style, dest, buffer_size, required_size, component);

    if (dest && !(destW = MyMalloc( buffer_size * sizeof(WCHAR) ))) return FALSE;
    if (source && !(sourceW = strdupAtoW( source ))) goto done;
    if (location && !(locationW = strdupAtoW( location ))) goto done;

    ret = SetupCopyOEMInfW( sourceW, locationW, media_type, style, destW, buffer_size, &size, NULL );

    if (required_size) *required_size = size;

    if (dest)
    {
        if (buffer_size >= size)
        {
            WideCharToMultiByte( CP_ACP, 0, destW, -1, dest, buffer_size, NULL, NULL );
            if (component) *component = strrchr( dest, '\\' ) + 1;
        }
        else
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
    }

done:
    MyFree( destW );
    free( sourceW );
    free( locationW );
    if (ret) SetLastError(ERROR_SUCCESS);
    return ret;
}

static int compare_files(HANDLE file1, HANDLE file2)
{
    char buffer1[2048];
    char buffer2[2048];
    DWORD size1;
    DWORD size2;

    while (ReadFile(file1, buffer1, sizeof(buffer1), &size1, NULL)
            && ReadFile(file2, buffer2, sizeof(buffer2), &size2, NULL))
    {
        int ret;
        if (size1 != size2)
            return size1 > size2 ? 1 : -1;
        if (!size1)
            return 0;
        ret = memcmp( buffer1, buffer2, size1 );
        if (ret)
            return ret;
    }

    return 0;
}

struct driver_package
{
    const WCHAR *inf_name;
    HINF hinf;
    WCHAR *src_root, *dst_root;
    bool already_installed;

    struct file
    {
        WCHAR *desc, *tag, *subdir, *filename;
    } *files;
    size_t file_count, files_size;
};

static void driver_package_cleanup(struct driver_package *package)
{
    free(package->src_root);
    free(package->dst_root);
    for (size_t i = 0; i < package->file_count; ++i)
    {
        free(package->files[i].desc);
        free(package->files[i].tag);
        free(package->files[i].subdir);
        free(package->files[i].filename);
    }
    free(package->files);
    SetupCloseInfFile(package->hinf);
}

static WCHAR *get_string_field(INFCONTEXT *ctx, DWORD index)
{
    WCHAR *ret;
    DWORD len;

    if (!SetupGetStringFieldW(ctx, index, NULL, 0, &len) || len <= 1)
        return NULL;

    ret = malloc(len * sizeof(WCHAR));
    SetupGetStringFieldW(ctx, index, ret, len, NULL);
    return ret;
}

static bool get_source_info(HINF hinf, const WCHAR *filename, WCHAR **desc, WCHAR **tag, WCHAR **subdir)
{
    WCHAR *file_subdir = NULL, *disk_subdir = NULL;
    UINT diskid;
    DWORD len;

    if (!SetupGetSourceFileLocationW(hinf, NULL, filename, &diskid, NULL, 0, &len))
    {
        ERR("Failed to get location for %s, error %lu.\n", debugstr_w(filename), GetLastError());
        return false;
    }

    if (len > 1)
    {
        if (!(file_subdir = malloc(len * sizeof(WCHAR))))
            return false;
        SetupGetSourceFileLocationW(hinf, NULL, filename, &diskid, file_subdir, len, NULL);
    }

    if (SetupGetSourceInfoW(hinf, diskid, SRCINFO_DESCRIPTION, NULL, 0, &len) && len > 1
            && (*desc = malloc(len * sizeof(WCHAR))))
        SetupGetSourceInfoW(hinf, diskid, SRCINFO_DESCRIPTION, *desc, len, NULL);

    if (SetupGetSourceInfoW(hinf, diskid, SRCINFO_TAGFILE, NULL, 0, &len) && len > 1
            && (*tag = malloc(len * sizeof(WCHAR))))
        SetupGetSourceInfoW(hinf, diskid, SRCINFO_TAGFILE, *tag, len, NULL);

    if (SetupGetSourceInfoW(hinf, diskid, SRCINFO_PATH, NULL, 0, &len) && len > 1
            && (disk_subdir = malloc(len * sizeof(WCHAR))))
        SetupGetSourceInfoW(hinf, diskid, SRCINFO_PATH, disk_subdir, len, NULL);

    if (disk_subdir)
    {
        if (file_subdir)
        {
            *subdir = concat_path(disk_subdir, file_subdir);
            free(disk_subdir);
            free(file_subdir);
        }
        else
        {
            *subdir = disk_subdir;
        }
    }
    else
    {
        *subdir = file_subdir;
    }

    return true;
}

static void add_file(struct driver_package *package,
        WCHAR *filename, WCHAR *desc, WCHAR *tag, WCHAR *subdir)
{
    struct file *file;

    array_reserve((void **)&package->files, &package->files_size,
            package->file_count + 1, sizeof(*package->files));

    file = &package->files[package->file_count++];
    file->filename = filename;
    file->desc = desc;
    file->tag = tag;
    file->subdir = subdir;

    TRACE("Adding file %s, desc %s, tag %s, subdir %s.\n",
            debugstr_w(filename), debugstr_w(desc), debugstr_w(tag), debugstr_w(subdir));
}

static void add_file_from_copy_section(struct driver_package *package,
        const WCHAR *dst_filename, const WCHAR *src_filename)
{
    WCHAR *desc = NULL, *tag = NULL, *subdir = NULL;

    if (get_source_info(package->hinf, src_filename, &desc, &tag, &subdir))
        add_file(package, wcsdup(src_filename), desc, tag, subdir);
}

static void add_copy_section(struct driver_package *package, const WCHAR *section)
{
    TRACE("Building file list from CopyFiles section %s.\n", debugstr_w(section));

    if (section[0] == '@')
    {
        add_file_from_copy_section(package, section + 1, section + 1);
    }
    else
    {
        INFCONTEXT context;

        if (!SetupFindFirstLineW(package->hinf, section, NULL, &context))
            return;
        do
        {
            WCHAR *dst_filename = get_string_field(&context, 1);
            WCHAR *src_filename = get_string_field(&context, 2);

            add_file_from_copy_section(package, dst_filename, src_filename ? src_filename : dst_filename);

            free(dst_filename);
            free(src_filename);
        } while (SetupFindNextLine(&context, &context));
    }
}

static void add_driver_files(struct driver_package *package, const WCHAR *driver_section)
{
    INFCONTEXT ctx;
    BOOL found;

    TRACE("Building file list for driver section %s.\n", debugstr_w(driver_section));

    found = SetupFindFirstLineW(package->hinf, driver_section, L"CopyFiles", &ctx);
    while (found)
    {
        DWORD count = SetupGetFieldCount(&ctx);

        for (DWORD i = 1; i <= count; ++i)
        {
            WCHAR *section = get_string_field(&ctx, i);

            add_copy_section(package, section);
            free(section);
        }

        found = SetupFindNextMatchLineW(&ctx, L"CopyFiles", &ctx);
    }
}

static bool driver_store_files_are_equal(const WCHAR *src_path, const WCHAR *store_path)
{
    HANDLE src_file, store_file;
    bool ret;

    src_file = CreateFileW(src_path, FILE_READ_DATA,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (src_file == INVALID_HANDLE_VALUE)
    {
        ERR("Source file %s doesn't exist.\n", debugstr_w(src_path));
        return false;
    }

    store_file = CreateFileW(store_path, FILE_READ_DATA,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (store_file == INVALID_HANDLE_VALUE)
    {
        TRACE("File %s doesn't exist in driver store; this is the wrong package.\n", debugstr_w(store_path));
        CloseHandle(src_file);
        return false;
    }

    ret = !compare_files(src_file, store_file);

    CloseHandle(src_file);
    CloseHandle(store_file);
    return ret;
}

static void find_driver_store_path(struct driver_package *package, const WCHAR *inf_path)
{
    static const WCHAR file_repository[] = L"C:\\windows\\system32\\driverstore\\filerepository";
    WCHAR *search_path = sprintf_path(L"%s\\%s_*", file_repository, package->inf_name);
    unsigned int index = 1;
    WIN32_FIND_DATAW data;
    HANDLE handle;

    /* Windows names directories using the inf name and what appears to be a
     * hash, separated by an underscore. For simplicity we don't implement the
     * hash; instead we just use an integer to discriminate different packages
     * with the same name. */

    handle = FindFirstFileW(search_path, &data);
    free(search_path);

    if (handle != INVALID_HANDLE_VALUE)
    {
        do
        {
            const size_t prefix_len = wcslen(package->inf_name) + 1;
            WCHAR *store_root, *end_ptr, *store_inf;
            unsigned int dir_index;

            /* FindFirstFile() should have given us a name at least as long as
             * the INF name followed by an underscore. */
            assert(wcslen(data.cFileName) >= prefix_len);
            if (!(dir_index = wcstoul(data.cFileName + prefix_len, &end_ptr, 10)))
                ERR("Malformed directory name %s.\n", debugstr_w(data.cFileName));
            index = max(index, dir_index + 1);

            store_root = concat_path(file_repository, data.cFileName);
            store_inf = concat_path(store_root, package->inf_name);

            if (driver_store_files_are_equal(inf_path, store_inf))
            {
                TRACE("Found matching driver package %s.\n", debugstr_w(store_root));
                free(store_inf);
                FindClose(handle);
                package->already_installed = true;
                package->dst_root = store_root;
                return;
            }

            free(store_root);
        } while (FindNextFileW(handle, &data));

        FindClose(handle);
    }
    else
    {
        if (GetLastError() != ERROR_PATH_NOT_FOUND && GetLastError() != ERROR_FILE_NOT_FOUND)
        {
            ERR("Failed to enumerate file repository, error %lu.\n", GetLastError());
            return;
        }
    }

    package->dst_root = sprintf_path(L"%s\\%s_%u", file_repository, package->inf_name, index);
    TRACE("No matching driver package found; using new path %s.\n", debugstr_w(package->dst_root));
}

static DWORD parse_inf(struct driver_package *package, const WCHAR *filename)
{
    WCHAR mfg_key[LINE_LEN], manufacturer[LINE_LEN];
    WCHAR *filename_abs, *file_part, *catalog;
    INFCONTEXT ctx;
    DWORD len;

    memset(package, 0, sizeof(*package));

    if (!*filename)
        return ERROR_FILE_NOT_FOUND;

    len = GetFullPathNameW(filename, 0, NULL, NULL);
    filename_abs = malloc(len * sizeof(WCHAR));
    GetFullPathNameW(filename, len, filename_abs, NULL);

    TRACE("Parsing %s.\n", debugstr_w(filename_abs));

    if ((package->hinf = SetupOpenInfFileW(filename_abs, NULL, INF_STYLE_WIN4, NULL)) == INVALID_HANDLE_VALUE)
    {
        ERR("Failed to open %s, error %lu.\n", debugstr_w(filename_abs), GetLastError());
        driver_package_cleanup(package);
        return GetLastError();
    }

    file_part = wcsrchr(filename_abs, '\\');
    assert(file_part);
    package->inf_name = file_part + 1;

    *file_part = 0;
    package->src_root = filename_abs;

    add_file(package, wcsdup(package->inf_name), NULL, NULL, NULL);

    if (SetupFindFirstLineW(package->hinf, L"Version", L"CatalogFile", &ctx)
            && (catalog = get_string_field(&ctx, 1)))
        add_file(package, catalog, NULL, NULL, NULL);

    for (DWORD i = 0; SetupGetLineByIndexW(package->hinf, L"Manufacturer", i, &ctx); ++i)
    {
        SetupGetStringFieldW(&ctx, 0, manufacturer, ARRAY_SIZE(manufacturer), NULL);
        if (!SetupGetStringFieldW(&ctx, 1, mfg_key, ARRAY_SIZE(mfg_key), NULL))
            wcscpy(mfg_key, manufacturer);

        if (!any_version_is_compatible(&ctx))
            continue;

        if (!SetupDiGetActualSectionToInstallW(package->hinf, mfg_key, mfg_key, ARRAY_SIZE(mfg_key), NULL, NULL))
        {
            WARN("Failed to find section for %s, skipping.\n", debugstr_w(mfg_key));
            continue;
        }

        for (DWORD j = 0; SetupGetLineByIndexW(package->hinf, mfg_key, j, &ctx); ++j)
        {
            WCHAR *driver_section = get_string_field(&ctx, 1);
            WCHAR arch_driver_section[LINE_LEN];

            if (SetupDiGetActualSectionToInstallW(package->hinf, driver_section,
                    arch_driver_section, ARRAY_SIZE(arch_driver_section), NULL, NULL))
            {
                WCHAR coinst_section[LINE_LEN];

                add_driver_files(package, arch_driver_section);

                swprintf(coinst_section, ARRAY_SIZE(coinst_section), L"%s.CoInstallers", arch_driver_section);
                add_driver_files(package, coinst_section);
            }
            else
            {
                WARN("Failed to find driver section for %s, skipping.\n", debugstr_w(driver_section));
            }

            free(driver_section);
        }
    }

    find_driver_store_path(package, filename);

    return ERROR_SUCCESS;
}

static BOOL find_existing_inf(const WCHAR *source, WCHAR *target)
{
    LARGE_INTEGER source_file_size, dest_file_size;
    HANDLE source_file, dest_file;
    WIN32_FIND_DATAW find_data;
    HANDLE find_handle;

    source_file = CreateFileW(source, FILE_READ_DATA | FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (source_file == INVALID_HANDLE_VALUE)
        return FALSE;

    if (!GetFileSizeEx(source_file, &source_file_size))
    {
        CloseHandle(source_file);
        return FALSE;
    }

    GetWindowsDirectoryW(target, MAX_PATH);
    wcscat(target, L"\\inf\\*");
    if ((find_handle = FindFirstFileW(target, &find_data)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            GetWindowsDirectoryW(target, MAX_PATH);
            wcscat(target, L"\\inf\\");
            wcscat(target, find_data.cFileName);
            dest_file = CreateFileW(target, FILE_READ_DATA | FILE_READ_ATTRIBUTES,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
            if (dest_file == INVALID_HANDLE_VALUE)
                continue;

            SetFilePointer(source_file, 0, NULL, FILE_BEGIN);

            if (GetFileSizeEx(dest_file, &dest_file_size)
                    && dest_file_size.QuadPart == source_file_size.QuadPart
                    && !compare_files(source_file, dest_file))
            {
                CloseHandle(dest_file);
                CloseHandle(source_file);
                FindClose(find_handle);
                TRACE("Found matching INF %s.\n", debugstr_w(target));
                return TRUE;
            }
            CloseHandle(dest_file);
        } while (FindNextFileW(find_handle, &find_data));

        FindClose(find_handle);
    }

    CloseHandle(source_file);
    TRACE("No matching INF found.\n");
    return FALSE;
}

/* arbitrary limit not related to what native actually uses */
#define OEM_INDEX_LIMIT 999

static DWORD copy_inf(const WCHAR *source, DWORD style, WCHAR *ret_path)
{
    WCHAR target[MAX_PATH], catalog_file[MAX_PATH], pnf_path[MAX_PATH], *p;
    FILE *pnf_file;
    unsigned int i;
    HINF hinf;

    if (find_existing_inf(source, target))
    {
        TRACE("Found existing INF %s.\n", debugstr_w(target));

        if (ret_path)
            wcscpy(ret_path, target);
        if (style & SP_COPY_NOOVERWRITE)
            return ERROR_FILE_EXISTS;
        else
            return ERROR_SUCCESS;
    }

    GetWindowsDirectoryW(target, ARRAY_SIZE(target));
    wcscat(target, L"\\inf\\");
    wcscat(target, wcsrchr(source, '\\') + 1);
    if (GetFileAttributesW(target) != INVALID_FILE_ATTRIBUTES)
    {
        for (i = 0; i < OEM_INDEX_LIMIT; i++)
        {
            GetWindowsDirectoryW(target, ARRAY_SIZE(target));
            wcscat(target, L"\\inf\\");
            swprintf(target + wcslen(target), ARRAY_SIZE(target) - wcslen(target), L"oem%u.inf", i);

            if (GetFileAttributesW(target) == INVALID_FILE_ATTRIBUTES)
                break;
        }
        if (i == OEM_INDEX_LIMIT)
            return ERROR_FILENAME_EXCED_RANGE;
    }

    hinf = SetupOpenInfFileW(source, NULL, INF_STYLE_WIN4, NULL);
    if (hinf == INVALID_HANDLE_VALUE)
        return GetLastError();

    if (SetupGetLineTextW(NULL, hinf, L"Version", L"CatalogFile",
            catalog_file, ARRAY_SIZE(catalog_file), NULL))
    {
        GUID msguid = DRIVER_ACTION_VERIFY;
        WCHAR source_cat[MAX_PATH];
        HCATADMIN handle;
        HCATINFO cat;

        SetupCloseInfFile(hinf);

        wcscpy(source_cat, source);
        p = wcsrchr(source_cat, '\\');
        if (p)
            p++;
        else
            p = source_cat;
        wcscpy(p, catalog_file);

        TRACE("Installing catalog file %s.\n", debugstr_w(source_cat));

        if (!CryptCATAdminAcquireContext(&handle, &msguid, 0))
        {
            ERR("Failed to acquire security context, error %lu.\n", GetLastError());
            return GetLastError();
        }

        if (!(cat = CryptCATAdminAddCatalog(handle, source_cat, catalog_file, 0)))
        {
            ERR("Failed to add catalog, error %lu.\n", GetLastError());
            CryptCATAdminReleaseContext(handle, 0);
            return GetLastError();
        }

        CryptCATAdminReleaseCatalogContext(handle, cat, 0);
        CryptCATAdminReleaseContext(handle, 0);
    }
    else
    {
        SetupCloseInfFile(hinf);
    }

    if (!CopyFileW(source, target, TRUE))
        return GetLastError();

    wcscpy(pnf_path, target);
    PathRemoveExtensionW(pnf_path);
    PathAddExtensionW(pnf_path, L".pnf");
    if ((pnf_file = _wfopen(pnf_path, L"w")))
    {
        fputws(PNF_HEADER, pnf_file);
        fputws(source, pnf_file);
        fclose(pnf_file);
    }

    if (ret_path)
        wcscpy(ret_path, target);
    return ERROR_SUCCESS;
}

static void queue_copy_file(const struct driver_package *package, HSPFILEQ queue, const struct file *file)
{
    SP_FILE_COPY_PARAMS_W params =
    {
        .cbSize = sizeof(params),
        .QueueHandle = queue,
        .SourceRootPath = package->src_root,
        .CopyStyle = SP_COPY_NODECOMP,
        .SourceFilename = file->filename,
        .TargetFilename = file->filename,
    };

    params.SourceDescription = file->desc;
    params.SourceTagfile = file->tag;
    params.SourcePath = file->subdir;

    TRACE("Queueing copy from subdir %s, filename %s.\n",
            debugstr_w(file->subdir), debugstr_w(file->filename));

    if (file->subdir)
    {
        WCHAR *dst_dir = concat_path(package->dst_root, file->subdir);

        params.TargetDirectory = dst_dir;
        if (!SetupQueueCopyIndirectW(&params))
            ERR("Failed to queue copy, error %lu.\n", GetLastError());
        free(dst_dir);
    }
    else
    {
        params.TargetDirectory = package->dst_root;
        if (!SetupQueueCopyIndirectW(&params))
            ERR("Failed to queue copy, error %lu.\n", GetLastError());
    }
}

static DWORD driver_package_install_to_store(const struct driver_package *package, DWORD style, WCHAR *infdir_path)
{
    HSPFILEQ queue = SetupOpenFileQueue();
    DWORD ret = ERROR_SUCCESS;
    void *setupapi_ctx;
    WCHAR *store_inf;

    for (size_t i = 0; i < package->file_count; ++i)
        queue_copy_file(package, queue, &package->files[i]);

    setupapi_ctx = SetupInitDefaultQueueCallback(NULL);
    if (!SetupCommitFileQueueW(NULL, queue, SetupDefaultQueueCallbackW, setupapi_ctx))
    {
        ERR("Failed to commit queue, error %lu.\n", GetLastError());
        ret = GetLastError();
    }

    SetupTermDefaultQueueCallback(setupapi_ctx);
    SetupCloseFileQueue(queue);

    if (!ret)
    {
        store_inf = concat_path(package->dst_root, package->inf_name);
        if ((ret = copy_inf(store_inf, style, infdir_path)))
            ERR("Failed to copy INF %s, error %lu.\n", debugstr_w(store_inf), GetLastError());
        free(store_inf);
    }

    return ret;
}

static DWORD driver_package_delete(const struct driver_package *package)
{
    WCHAR infdir_path[MAX_PATH];
    WCHAR *inf_path;

    inf_path = concat_path(package->dst_root, package->inf_name);

    if (find_existing_inf(inf_path, infdir_path))
    {
        if (!DeleteFileW(infdir_path))
            ERR("Failed to delete %s, error %lu.\n", debugstr_w(infdir_path), GetLastError());
        PathRemoveExtensionW(infdir_path);
        PathAddExtensionW(infdir_path, L".pnf");
        if (!DeleteFileW(infdir_path))
            ERR("Failed to delete %s, error %lu.\n", debugstr_w(infdir_path), GetLastError());
    }
    else
    {
        ERR("Driver package INF %s not found in INF directory!\n", debugstr_w(inf_path));
    }

    free(inf_path);

    for (size_t i = 0; i < package->file_count; ++i)
    {
        const struct file *file = &package->files[i];
        WCHAR *path;

        if (file->subdir)
            path = sprintf_path(L"%s\\%s\\%s", package->dst_root, file->subdir, file->filename);
        else
            path = sprintf_path(L"%s\\%s", package->dst_root, file->filename);

        if (DeleteFileW(path))
        {
            for (;;)
            {
                *wcsrchr(path, '\\') = 0;
                if (wcslen(path) == wcslen(package->dst_root))
                    break;
                if (!RemoveDirectoryW(path))
                {
                    if (GetLastError() != ERROR_DIR_NOT_EMPTY)
                        ERR("Failed to remove %s, error %lu.\n", debugstr_w(path), GetLastError());
                    break;
                }
            }
        }
        else
        {
            ERR("Failed to delete %s, error %lu.\n", debugstr_w(path), GetLastError());
        }

        free(path);
    }

    if (!RemoveDirectoryW(package->dst_root))
        ERR("Failed to remove %s, error %lu.\n", debugstr_w(package->dst_root), GetLastError());

    return ERROR_SUCCESS;
}

/***********************************************************************
 *      SetupCopyOEMInfW  (SETUPAPI.@)
 */
BOOL WINAPI SetupCopyOEMInfW(const WCHAR *source, const WCHAR *location, DWORD media_type,
        DWORD style, WCHAR *dest, DWORD buffer_size, DWORD *required_size, WCHAR **filepart)
{
    struct driver_package package;
    WCHAR target[MAX_PATH];
    DWORD size, ret;

    TRACE("source %s, location %s, media_type %lu, style %#lx, dest %p, buffer_size %lu, required_size %p, filepart %p.\n",
            debugstr_w(source), debugstr_w(location), media_type, style, dest, buffer_size, required_size, filepart);

    if (!source)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ((ret = parse_inf(&package, source)))
    {
        SetLastError(ret);
        return FALSE;
    }

    if (package.already_installed)
    {
        if (find_existing_inf(source, target))
        {
            if (style & SP_COPY_NOOVERWRITE)
                ret = ERROR_FILE_EXISTS;
            else
                ret = ERROR_SUCCESS;
        }
        else
        {
            ERR("Inf %s is already installed to driver store, but not found in C:\\windows\\inf!\n",
                    debugstr_w(source));
            ret = ERROR_FILE_NOT_FOUND;
        }
    }
    else
    {
        ret = driver_package_install_to_store(&package, style, target);
    }

    if (style & SP_COPY_DELETESOURCE)
        DeleteFileW(source);

    size = wcslen(target) + 1;
    if (required_size)
        *required_size = size;

    if ((!ret || ret == ERROR_FILE_EXISTS) && dest)
    {
        if (buffer_size >= size)
        {
            wcscpy(dest, target);
            if (filepart)
                *filepart = wcsrchr(dest, '\\') + 1;
        }
        else
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }
    }

    SetLastError(ret);
    return !ret;
}

/***********************************************************************
 *      SetupUninstallOEMInfA  (SETUPAPI.@)
 */
BOOL WINAPI SetupUninstallOEMInfA(const char *inf_file, DWORD flags, void *reserved)
{
    WCHAR *inf_fileW = NULL;
    BOOL ret;

    TRACE("inf_file %s, flags %#lx, reserved %p.\n", debugstr_a(inf_file), flags, reserved);

    if (inf_file && !(inf_fileW = strdupAtoW(inf_file)))
        return FALSE;
    ret = SetupUninstallOEMInfW(inf_fileW, flags, reserved);
    free(inf_fileW);
    return ret;
}

/***********************************************************************
 *      SetupUninstallOEMInfW  (SETUPAPI.@)
 */
BOOL WINAPI SetupUninstallOEMInfW(const WCHAR *inf_file, DWORD flags, void *reserved)
{
    struct driver_package package;
    WCHAR target[MAX_PATH];
    DWORD ret;

    TRACE("inf_file %s, flags %#lx, reserved %p.\n", debugstr_w(inf_file), flags, reserved);

    if (!inf_file)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!GetWindowsDirectoryW( target, ARRAY_SIZE( target )))
        return FALSE;

    wcscat(target, L"\\inf\\");
    wcscat(target, inf_file);

    if ((ret = parse_inf(&package, target)))
    {
        SetLastError(ret);
        return FALSE;
    }

    if (package.already_installed)
        ret = driver_package_delete(&package);
    else
        ret = ERROR_FILE_NOT_FOUND;

    driver_package_cleanup(&package);

    SetLastError(ret);
    return !ret;
}

HRESULT WINAPI DriverStoreFindDriverPackageW(const WCHAR *inf_path, void *unk1,
        void *unk2, WORD architecture, void *unk4, WCHAR *ret_path, DWORD *ret_len)
{
    struct driver_package package;
    SYSTEM_INFO system_info;
    HRESULT hr;
    DWORD ret;

    TRACE("inf_path %s, unk1 %p, unk2 %p, architecture %#x, unk4 %p, ret_path %p, ret_len %p.\n",
            debugstr_w(inf_path), unk1, unk2, architecture, unk4, ret_path, ret_len);

    if (unk1)
        FIXME("Ignoring unk1 %p.\n", unk1);
    if (unk2)
        FIXME("Ignoring unk2 %p.\n", unk2);
    if (unk4)
        FIXME("Ignoring unk4 %p.\n", unk4);

    if (*ret_len < MAX_PATH)
    {
        FIXME("Length %lu too short, returning E_INVALIDARG.\n", *ret_len);
        return E_INVALIDARG;
    }

    GetSystemInfo(&system_info);
    if (architecture != system_info.wProcessorArchitecture)
    {
        FIXME("Wrong architecture %#x, expected %#x.\n", architecture, system_info.wProcessorArchitecture);
        return E_INVALIDARG;
    }

    if ((ret = parse_inf(&package, inf_path)))
        return HRESULT_FROM_WIN32(ret);

    if (package.already_installed)
    {
        DWORD len = wcslen(package.dst_root) + 1 + wcslen(package.inf_name) + 1;

        if (len > *ret_len)
        {
            FIXME("Buffer too small.\n");
            /* FIXME: What do we return here? */
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
        else
        {
            swprintf(ret_path, len, L"%s\\%s", package.dst_root, package.inf_name);
            hr = S_OK;
        }
        *ret_len = len;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        *ret_path = 0;
        *ret_len = 1;
    }

    driver_package_cleanup(&package);
    return hr;
}

HRESULT WINAPI DriverStoreFindDriverPackageA(const char *inf_path, void *unk1,
        void *unk2, WORD architecture, void *unk4, char *ret_path, DWORD *ret_len)
{
    WCHAR ret_pathW[MAX_PATH];
    WCHAR *inf_pathW;
    DWORD lenW, len;
    HRESULT hr;

    TRACE("inf_path %s, unk1 %p, unk2 %p, architecture %#x, unk4 %p, ret_path %p, ret_len %p.\n",
            debugstr_a(inf_path), unk1, unk2, architecture, unk4, ret_path, ret_len);

    if (*ret_len < MAX_PATH)
    {
        FIXME("Length %lu too short, returning E_INVALIDARG.\n", *ret_len);
        return E_INVALIDARG;
    }

    if (!(inf_pathW = strdupAtoW(inf_path)))
        return E_OUTOFMEMORY;

    lenW = ARRAY_SIZE(ret_pathW);
    hr = DriverStoreFindDriverPackageW(inf_pathW, unk1, unk2, architecture, unk4, ret_pathW, &lenW);
    if (!hr)
    {
        len = WideCharToMultiByte(CP_ACP, 0, ret_pathW, lenW, NULL, 0, NULL, NULL);
        if (len > *ret_len)
        {
            FIXME("Buffer too small.\n");
            /* FIXME: What do we return here? */
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
        else
        {
            WideCharToMultiByte(CP_ACP, 0, ret_pathW, lenW, ret_path, len, NULL, NULL);
        }
        *ret_len = len;
    }
    else
    {
        *ret_path = 0;
        *ret_len = 1;
    }
    free(inf_pathW);
    return hr;
}

HRESULT WINAPI DriverStoreAddDriverPackageW(const WCHAR *inf_path, void *unk1,
        void *unk2, WORD architecture, WCHAR *ret_path, DWORD *ret_len)
{
    struct driver_package package;
    SYSTEM_INFO system_info;
    DWORD ret, len;
    HRESULT hr;

    TRACE("inf_path %s, unk1 %p, unk2 %p, architecture %#x, ret_path %p, ret_len %p.\n",
            debugstr_w(inf_path), unk1, unk2, architecture, ret_path, ret_len);

    if (unk1)
        FIXME("Ignoring unk1 %p.\n", unk1);
    if (unk2)
        FIXME("Ignoring unk2 %p.\n", unk2);

    if (*ret_len < MAX_PATH)
    {
        FIXME("Length %lu too short, returning E_INVALIDARG.\n", *ret_len);
        return E_INVALIDARG;
    }

    GetSystemInfo(&system_info);
    if (architecture != system_info.wProcessorArchitecture)
    {
        FIXME("Wrong architecture %#x, expected %#x.\n", architecture, system_info.wProcessorArchitecture);
        return E_INVALIDARG;
    }

    if ((ret = parse_inf(&package, inf_path)))
        return HRESULT_FROM_WIN32(ret);

    if (!package.already_installed)
    {
        if ((ret = driver_package_install_to_store(&package, 0, NULL)))
        {
            driver_package_cleanup(&package);
            return HRESULT_FROM_WIN32(ret);
        }
    }

    len = wcslen(package.dst_root) + 1 + wcslen(package.inf_name) + 1;

    if (len > *ret_len)
    {
        FIXME("Buffer too small.\n");
        /* FIXME: What do we return here? */
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }
    else
    {
        swprintf(ret_path, len, L"%s\\%s", package.dst_root, package.inf_name);
        hr = S_OK;
    }
    *ret_len = len;

    driver_package_cleanup(&package);
    return hr;
}

HRESULT WINAPI DriverStoreAddDriverPackageA(const char *inf_path, void *unk1,
        void *unk2, WORD architecture, char *ret_path, DWORD *ret_len)
{
    WCHAR ret_pathW[MAX_PATH];
    WCHAR *inf_pathW;
    DWORD lenW, len;
    HRESULT hr;

    TRACE("inf_path %s, unk1 %p, unk2 %p, architecture %#x, ret_path %p, ret_len %p.\n",
            debugstr_a(inf_path), unk1, unk2, architecture, ret_path, ret_len);

    if (*ret_len < MAX_PATH)
    {
        FIXME("Length %lu too short, returning E_INVALIDARG.\n", *ret_len);
        return E_INVALIDARG;
    }

    if (!(inf_pathW = strdupAtoW(inf_path)))
        return E_OUTOFMEMORY;

    lenW = ARRAY_SIZE(ret_pathW);
    hr = DriverStoreAddDriverPackageW(inf_pathW, unk1, unk2, architecture, ret_pathW, &lenW);
    if (!hr)
    {
        len = WideCharToMultiByte(CP_ACP, 0, ret_pathW, lenW, NULL, 0, NULL, NULL);
        if (len > *ret_len)
        {
            FIXME("Buffer too small.\n");
            /* FIXME: What do we return here? */
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
        else
        {
            WideCharToMultiByte(CP_ACP, 0, ret_pathW, lenW, ret_path, len, NULL, NULL);
        }
        *ret_len = len;
    }
    free(inf_pathW);
    return hr;
}

HRESULT WINAPI DriverStoreDeleteDriverPackageW(const WCHAR *inf_path, void *unk1, void *unk2)
{
    struct driver_package package;
    DWORD ret;

    TRACE("inf_path %s, unk1 %p, unk2 %p.\n", debugstr_w(inf_path), unk1, unk2);

    if (unk1)
        FIXME("Ignoring unk1 %p.\n", unk1);
    if (unk2)
        FIXME("Ignoring unk2 %p.\n", unk2);

    if ((ret = parse_inf(&package, inf_path)))
        return HRESULT_FROM_WIN32(ret);

    if (package.already_installed)
        ret = driver_package_delete(&package);
    else
        ret = ERROR_FILE_NOT_FOUND;

    driver_package_cleanup(&package);
    return HRESULT_FROM_WIN32(ret);
}

HRESULT WINAPI DriverStoreDeleteDriverPackageA(const char *inf_path, void *unk1, void *unk2)
{
    WCHAR *inf_pathW;
    HRESULT hr;

    TRACE("inf_path %s, unk1 %p, unk2 %p.\n", debugstr_a(inf_path), unk1, unk2);

    if (!(inf_pathW = strdupAtoW(inf_path)))
        return E_OUTOFMEMORY;

    hr = DriverStoreDeleteDriverPackageW(inf_pathW, unk1, unk2);
    free(inf_pathW);
    return hr;
}
