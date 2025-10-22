/*
 * Copyright (C) 2023 Mohamad Al-Jaf
 * Copyright (C) 2025 Vibhav Pant
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

#include "cfgmgr32_private.h"
#include "initguid.h"
#include "devpkey.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

static LSTATUS guid_from_string( const WCHAR *str, GUID *guid )
{
    UNICODE_STRING guid_str;
    RtlInitUnicodeString( &guid_str, str );
    if (RtlGUIDFromString( &guid_str, guid )) return ERROR_INVALID_DATA;
    return ERROR_SUCCESS;
}

static const WCHAR *guid_string( const GUID *guid, WCHAR *buffer, UINT length )
{
    swprintf( buffer, length, L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
              guid->Data1, guid->Data2, guid->Data3, guid->Data4[0], guid->Data4[1], guid->Data4[2],
              guid->Data4[3], guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7] );
    return buffer;
}

static LSTATUS propkey_from_string( const WCHAR *str, DEVPROPKEY *key )
{
    const WCHAR *pid;
    LSTATUS err;

    if (!(pid = wcsrchr( str, '\\' ))) return ERROR_INVALID_DATA;
    if ((err = guid_from_string( str, &key->fmtid ))) return err;
    if (swscanf( pid + 1, L"%04X", &key->pid ) != 1) return ERROR_INVALID_DATA;
    return ERROR_SUCCESS;
}

static const WCHAR *propkey_string( const DEVPROPKEY *key, const WCHAR *prefix, WCHAR *buffer, UINT length )
{
    swprintf( buffer, length, L"%s{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\\%04X", prefix,
              key->fmtid.Data1, key->fmtid.Data2, key->fmtid.Data3, key->fmtid.Data4[0], key->fmtid.Data4[1], key->fmtid.Data4[2],
              key->fmtid.Data4[3], key->fmtid.Data4[4], key->fmtid.Data4[5], key->fmtid.Data4[6], key->fmtid.Data4[7], key->pid );
    return buffer;
}

static const WCHAR control_classW[]  = L"System\\CurrentControlSet\\Control\\Class\\";
static const WCHAR device_classesW[] = L"System\\CurrentControlSet\\Control\\DeviceClasses\\";
static const WCHAR enum_rootW[]      = L"System\\CurrentControlSet\\Enum\\";

static struct key_cache
{
    HKEY root;
    const WCHAR *prefix;
    UINT prefix_len;
    HKEY hkey;
} cache[] =
{
    { HKEY_LOCAL_MACHINE, control_classW, ARRAY_SIZE(control_classW) - 1, (HKEY)-1 },
    { HKEY_LOCAL_MACHINE, device_classesW, ARRAY_SIZE(device_classesW) - 1, (HKEY)-1 },
    { HKEY_LOCAL_MACHINE, enum_rootW, ARRAY_SIZE(enum_rootW) - 1, (HKEY)-1 },
};

static HKEY cache_root_key( HKEY root, const WCHAR *key, const WCHAR **path )
{
    HKEY hkey;

    for (struct key_cache *entry = cache; entry < cache + ARRAY_SIZE(cache); entry++)
    {
        if (entry->root != root) continue;
        if (wcsnicmp( key, entry->prefix, entry->prefix_len )) continue;
        if (path) *path = key + entry->prefix_len;

        if (entry->hkey != (HKEY)-1 || RegOpenKeyExW( root, entry->prefix, 0, KEY_ALL_ACCESS, &hkey )) return entry->hkey;
        if (InterlockedCompareExchangePointer( (void *)&entry->hkey, hkey, (HKEY)-1 ) != (HKEY)-1) RegCloseKey( hkey );
        return entry->hkey;
    }

    if (path) *path = key;
    return root;
}

static LSTATUS open_key( HKEY root, const WCHAR *key, REGSAM access, BOOL open, HKEY *hkey )
{
    if ((root = cache_root_key( root, key, &key )) == (HKEY)-1) return ERROR_FILE_NOT_FOUND;
    if (open) return RegOpenKeyExW( root, key, 0, access, hkey );
    return RegCreateKeyExW( root, key, 0, NULL, 0, access, NULL, hkey, NULL );
}

static LSTATUS query_value( HKEY hkey, const WCHAR *value, WCHAR *buffer, DWORD len )
{
    return RegQueryValueExW( hkey, value, NULL, NULL, (BYTE *)buffer, &len );
}

static LSTATUS open_class_key( HKEY root, const WCHAR *key, REGSAM access, BOOL open, HKEY *hkey )
{
    WCHAR path[MAX_PATH];
    swprintf( path, ARRAY_SIZE(path), L"%s%s", control_classW, key );
    return open_key( root, path, access, open, hkey );
}

static LSTATUS open_device_classes_key( HKEY root, const WCHAR *key, REGSAM access, BOOL open, HKEY *hkey )
{
    WCHAR path[MAX_PATH];
    swprintf( path, ARRAY_SIZE(path), L"%s%s", device_classesW, key );
    return open_key( root, path, access, open, hkey );
}

LSTATUS init_property( struct property *prop, const DEVPROPKEY *key, DEVPROPTYPE *type, void *buffer, DWORD *size )
{
    if (!key) return ERROR_INVALID_PARAMETER;
    if (!(prop->type = type) || !(prop->size = size)) return ERROR_INVALID_USER_BUFFER;
    if (!(prop->buffer = buffer) && (*prop->size)) return ERROR_INVALID_USER_BUFFER;
    prop->ansi = FALSE;
    prop->key = *key;
    prop->reg_type = NULL;
    return ERROR_SUCCESS;
}

static LSTATUS init_registry_property( struct property *prop, const DEVPROPKEY *base, UINT property, DWORD *type, void *buffer, DWORD *size, BOOL ansi )
{
    if (!(prop->size = size)) return ERROR_INVALID_USER_BUFFER;
    if (!(prop->buffer = buffer) && (*prop->size)) return ERROR_INVALID_USER_BUFFER;
    prop->type = NULL;
    prop->ansi = ansi;
    memcpy( &prop->key, base, sizeof(prop->key) );
    prop->key.pid = property + 1;
    prop->reg_type = type;
    return ERROR_SUCCESS;
}

static LSTATUS query_property( HKEY root, const WCHAR *prefix, DEVPROPTYPE type, struct property *prop )
{
    WCHAR path[MAX_PATH];
    ULONG reg_type;
    LSTATUS err;

    err = RegQueryValueExW( root, propkey_string( &prop->key, prefix, path, ARRAY_SIZE(path) ),
                            NULL, &reg_type, prop->buffer, prop->size );
    if (type == DEVPROP_TYPE_EMPTY) type = reg_type & 0xffff;

    if (!err && !prop->buffer) err = ERROR_MORE_DATA;
    if ((!err || err == ERROR_MORE_DATA) && prop->type) *prop->type = type;
    if (err == ERROR_FILE_NOT_FOUND) return ERROR_NOT_FOUND;
    return err;
}

static LSTATUS query_named_property( HKEY hkey, const WCHAR *nameW, DEVPROPTYPE type, struct property *prop )
{
    LSTATUS err;

    if (!prop->ansi) err = RegQueryValueExW( hkey, nameW, NULL, prop->reg_type, prop->buffer, prop->size );
    else
    {
        char nameA[MAX_PATH];
        if (nameW) WideCharToMultiByte( CP_ACP, 0, nameW, -1, nameA, sizeof(nameA), NULL, NULL );
        err = RegQueryValueExA( hkey, nameW ? nameA : NULL, NULL, prop->reg_type, prop->buffer, prop->size );
    }

    if (!err && !prop->buffer) err = ERROR_MORE_DATA;
    if ((!err || err == ERROR_MORE_DATA) && prop->type) *prop->type = type;
    if (err == ERROR_FILE_NOT_FOUND) return ERROR_NOT_FOUND;
    return err;
}

static LSTATUS return_property( struct property *prop, DEVPROPTYPE type, const void *buffer, UINT size )
{
    LSTATUS err = *prop->size >= size ? ERROR_SUCCESS : ERROR_MORE_DATA;
    if (!err && prop->buffer) memcpy( prop->buffer, buffer, size );
    *prop->size = size;
    *prop->type = type;
    return err;
}

static LSTATUS return_property_bool( struct property *prop, DEVPROP_BOOLEAN value )
{
    return return_property( prop, DEVPROP_TYPE_BOOLEAN, &value, sizeof(value) );
}

static LSTATUS enum_objects_size( HKEY hkey, const void *object, const WCHAR *path, UINT path_len, void *context )
{
    UINT *total = context;
    *total += WideCharToMultiByte( CP_ACP, 0, path, path_len, NULL, 0, 0, 0 );
    return ERROR_SUCCESS;
}

struct enum_objects_append_params
{
    WCHAR *buffer;
    UINT len;
};

static LSTATUS enum_objects_append( HKEY hkey, const void *object, const WCHAR *path, UINT path_len, void *context )
{
    struct enum_objects_append_params *params = context;

    if (path_len > params->len) return ERROR_MORE_DATA;
    memcpy( params->buffer, path, path_len * sizeof(WCHAR) );
    params->buffer += path_len;
    params->len -= path_len;

    return ERROR_SUCCESS;
}

struct property_desc
{
    const DEVPROPKEY *key;
    DEVPROPTYPE       type;
    const WCHAR      *name;
};

static const struct property_desc class_properties[] =
{
    { &DEVPKEY_DeviceClass_ClassName,          DEVPROP_TYPE_STRING,                     L"Class" },
    { &DEVPKEY_DeviceClass_Name,               DEVPROP_TYPE_STRING,                     L"" },
    { &DEVPKEY_NAME,                           DEVPROP_TYPE_STRING,                     L"" },
    /* ansi-compatible CM_CRP properties */
    { &DEVPKEY_DeviceClass_UpperFilters,       DEVPROP_TYPE_STRING,                     L"UpperFilters" },
    { &DEVPKEY_DeviceClass_LowerFilters,       DEVPROP_TYPE_STRING,                     L"LowerFilters" },
    { &DEVPKEY_DeviceClass_Security,           DEVPROP_TYPE_SECURITY_DESCRIPTOR,        L"Security" },
    { &DEVPKEY_DeviceClass_SecuritySDS,        DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING, L"SecuritySDS" },
    { &DEVPKEY_DeviceClass_DevType,            DEVPROP_TYPE_UINT32,                     L"DevType" },
    { &DEVPKEY_DeviceClass_Exclusive,          DEVPROP_TYPE_BOOLEAN,                    L"Exclusive" },
    { &DEVPKEY_DeviceClass_Characteristics,    DEVPROP_TYPE_INT32,                      L"Characteristics" },
    /* unicode-only properties */
    { &DEVPKEY_DeviceClass_Icon,               DEVPROP_TYPE_STRING },
    { &DEVPKEY_DeviceClass_ClassInstaller,     DEVPROP_TYPE_STRING,                     L"Installer32" },
    { &DEVPKEY_DeviceClass_DefaultService,     DEVPROP_TYPE_STRING,                     L"Default Service" },
    { &DEVPKEY_DeviceClass_IconPath,           DEVPROP_TYPE_STRING_LIST,                L"IconPath" },
    { &DEVPKEY_DeviceClass_NoDisplayClass,     DEVPROP_TYPE_BOOLEAN,                    L"NoDisplayClass" },
    { &DEVPKEY_DeviceClass_NoInstallClass,     DEVPROP_TYPE_BOOLEAN,                    L"NoInstallClass" },
    { &DEVPKEY_DeviceClass_NoUseClass,         DEVPROP_TYPE_BOOLEAN,                    L"NoUseClass" },
    { &DEVPKEY_DeviceClass_PropPageProvider,   DEVPROP_TYPE_STRING,                     L"EnumPropPages32" },
    { &DEVPKEY_DeviceClass_SilentInstall,      DEVPROP_TYPE_BOOLEAN,                    L"SilentInstall" },
    { &DEVPKEY_DeviceClass_DHPRebalanceOptOut, DEVPROP_TYPE_BOOLEAN },
    { &DEVPKEY_DeviceClass_ClassCoInstallers,  DEVPROP_TYPE_STRING_LIST },
};

static LSTATUS query_class_property( HKEY hkey, struct property *prop )
{
    for (UINT i = 0; i < ARRAY_SIZE(class_properties); i++)
    {
        const struct property_desc *desc = class_properties + i;
        if (memcmp( desc->key, &prop->key, sizeof(prop->key) )) continue;
        if (!desc->name) return query_property( hkey, L"Properties\\", desc->type, prop );
        return query_named_property( hkey, desc->name, desc->type, prop );
    }

    if (!memcmp( &DEVPKEY_DeviceClass_UpperFilters, &prop->key, sizeof(prop->key.fmtid) ))
    {
        FIXME( "property %#lx not implemented\n", prop->key.pid - 1 );
        return ERROR_UNKNOWN_PROPERTY;
    }

    return query_property( hkey, L"Properties\\", DEVPROP_TYPE_EMPTY, prop );
}

static LSTATUS get_class_property( const GUID *class, struct property *prop )
{
    WCHAR path[39];
    LSTATUS err;
    HKEY hkey;

    guid_string( class, path, ARRAY_SIZE(path) );
    if (!(err = open_class_key( HKEY_LOCAL_MACHINE, path, KEY_QUERY_VALUE, TRUE, &hkey )))
    {
        err = query_class_property( hkey, prop );
        RegCloseKey( hkey );
    }

    if (err && err != ERROR_MORE_DATA) *prop->size = 0;
    return err;
}

static LSTATUS enum_class_property_keys( HKEY hkey, DEVPROPKEY *buffer, ULONG *size )
{
    ULONG capacity = *size, count = 0;
    LSTATUS err = ERROR_SUCCESS;
    HKEY props_key;

    for (UINT i = 0; i < ARRAY_SIZE(class_properties); i++)
    {
        const struct property_desc *desc = class_properties + i;
        if (desc->name && !RegQueryValueExW( hkey, desc->name, NULL, NULL, NULL, NULL ))
        {
            if (capacity < ++count || !buffer) err = ERROR_MORE_DATA;
            else buffer[count - 1] = *desc->key;
        }
    }

    if (!open_key( hkey, L"Properties", KEY_ENUMERATE_SUB_KEYS, TRUE, &props_key ))
    {
        WCHAR name[MAX_PATH];
        for (ULONG i = 0, len = ARRAY_SIZE(name); !RegEnumValueW( props_key, i, name, &len, 0, NULL, NULL, NULL ); i++, len = ARRAY_SIZE(name))
        {
            if (capacity < ++count || !buffer) err = ERROR_MORE_DATA;
            else err = propkey_from_string( name, buffer + count - 1 );
        }
        RegCloseKey( props_key );
    }

    *size = count;
    return err;
}

static LSTATUS get_class_property_keys( const GUID *class, DEVPROPKEY *buffer, ULONG *size )
{
    WCHAR path[39];
    LSTATUS err;
    HKEY hkey;

    guid_string( class, path, ARRAY_SIZE(path) );
    if ((err = open_class_key( HKEY_LOCAL_MACHINE, path, KEY_ALL_ACCESS, TRUE, &hkey ))) return err;
    err = enum_class_property_keys( hkey, buffer, size );
    RegCloseKey( hkey );

    return err;
}

LSTATUS init_device_interface( struct device_interface *iface, const WCHAR *name )
{
    WCHAR *tmp;
    UINT len;

    if (wcsncmp( name, L"\\\\?\\", 4 )) return ERROR_FILE_NOT_FOUND;
    len = swprintf( iface->name, MAX_PATH, L"##?#%s", name + 4 );

    if ((tmp = wcschr( iface->name, '\\' ))) *tmp++ = 0;
    else tmp = iface->name + len;
    swprintf( iface->refstr, MAX_PATH, L"#%s", tmp );

    if (!(tmp = wcsrchr( iface->name, '#' ))) return ERROR_FILE_NOT_FOUND;
    return guid_from_string( wcscpy( iface->class, tmp + 1 ), &iface->class_guid );
}

LSTATUS open_device_interface_key( const struct device_interface *iface, REGSAM access, BOOL open, HKEY *hkey )
{
    WCHAR path[MAX_PATH];
    swprintf( path, ARRAY_SIZE(path), L"%s\\%s", iface->class, iface->name );
    return open_device_classes_key( HKEY_LOCAL_MACHINE, path, access, open, hkey );
}

static DEVPROP_BOOLEAN device_interface_enabled( HKEY hkey, const struct device_interface *iface )
{
    DWORD linked, linked_size = sizeof(linked);
    WCHAR control[MAX_PATH];

    swprintf( control, ARRAY_SIZE(control), L"%s\\Control", iface->refstr );
    if (RegGetValueW( hkey, control, L"Linked", RRF_RT_DWORD, NULL, &linked, &linked_size )) return DEVPROP_FALSE;
    return linked ? DEVPROP_TRUE : DEVPROP_FALSE;
}

static LSTATUS enum_class_device_interfaces( HKEY root, struct device_interface *iface, DEVINSTID_W instance_id, BOOL all,
                                             enum_objects_cb callback, void *context )
{
    LSTATUS err = ERROR_SUCCESS;
    HKEY iface_key;

    for (UINT i = 0; !err && !(err = RegEnumKeyW( root, i, iface->name, ARRAY_SIZE(iface->name) )); i++)
    {
        WCHAR *tmp, instance[MAX_PATH], path[MAX_PATH];

        if ((err = open_key( root, iface->name, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, TRUE, &iface_key ))) continue;

        if (!(err = query_value( iface_key, L"DeviceInstance", instance, ARRAY_SIZE(instance) )) &&
            (!instance_id || !wcsicmp( instance_id, instance )))
        {
            for (tmp = wcschr( instance, '\\' ); tmp; tmp = wcschr( tmp, '\\' )) *tmp = '#';
            for (UINT j = 0; !err && !(err = RegEnumKeyW( iface_key, j, iface->refstr, ARRAY_SIZE(iface->refstr) )); j++)
            {
                ULONG len = swprintf( path, ARRAY_SIZE(path), L"\\\\?\\%s%s%s", instance, iface->refstr, iface->class );
                if (all || device_interface_enabled( iface_key, iface )) err = callback( iface_key, iface, path, len + 1, context );
            }
            if (err == ERROR_NO_MORE_ITEMS) err = ERROR_SUCCESS;
        }

        RegCloseKey( iface_key );
    }
    if (err == ERROR_NO_MORE_ITEMS) err = ERROR_SUCCESS;

    return err;
}

LSTATUS enum_device_interfaces( BOOL all, enum_objects_cb callback, void *context )
{
    struct device_interface iface;
    LSTATUS err = ERROR_SUCCESS;
    HKEY root, class_key;

    if ((root = cache_root_key( HKEY_LOCAL_MACHINE, device_classesW, NULL )) == (HKEY)-1) return ERROR_FILE_NOT_FOUND;

    for (UINT i = 0; !err && !(err = RegEnumKeyW( root, i, iface.class, ARRAY_SIZE(iface.class) )); i++)
    {
        if ((err = guid_from_string( iface.class, &iface.class_guid ))) continue;
        if ((err = open_key( root, iface.class, KEY_ENUMERATE_SUB_KEYS, TRUE, &class_key ))) continue;
        err = enum_class_device_interfaces( class_key, &iface, NULL, all, callback, context );
        RegCloseKey( class_key );
    }
    if (err == ERROR_NO_MORE_ITEMS) err = ERROR_SUCCESS;

    return err;
}

static LSTATUS enum_device_interface_list( GUID *class, DEVINSTID_W instance_id, BOOL all, enum_objects_cb callback, void *context )
{
    struct device_interface iface;
    HKEY class_key;
    LSTATUS err;

    if (instance_id && !*instance_id) instance_id = NULL;

    guid_string( class, iface.class, ARRAY_SIZE(iface.class) );
    if ((err = open_device_classes_key( HKEY_LOCAL_MACHINE, iface.class, KEY_ENUMERATE_SUB_KEYS, TRUE, &class_key ))) return err;
    err = enum_class_device_interfaces( class_key, &iface, instance_id, all, callback, context );
    RegCloseKey( class_key );

    if (!err) callback( NULL, NULL, L"", 1, context );
    return err;
}

static const struct property_desc device_interface_properties[] =
{
    { &DEVPKEY_Device_ContainerId,                          DEVPROP_TYPE_GUID },
    { &DEVPKEY_DeviceInterface_FriendlyName,                DEVPROP_TYPE_STRING,        L"FriendlyName" },
};

LSTATUS query_device_interface_property( HKEY hkey, const struct device_interface *iface, struct property *prop )
{
    WCHAR prefix[MAX_PATH];

    if (!memcmp( &DEVPKEY_DeviceInterface_Enabled, &prop->key, sizeof(prop->key) ))
        return return_property_bool( prop, device_interface_enabled( hkey, iface ) );
    if (!memcmp( &DEVPKEY_Device_InstanceId, &prop->key, sizeof(prop->key) ))
        return query_named_property( hkey, L"DeviceInstance", DEVPROP_TYPE_STRING, prop );
    if (!memcmp( &DEVPKEY_DeviceInterface_ClassGuid, &prop->key, sizeof(prop->key) ))
        return return_property( prop, DEVPROP_TYPE_GUID, &iface->class_guid, sizeof(iface->class_guid) );

    swprintf( prefix, ARRAY_SIZE(prefix), L"%s\\Properties\\", iface->refstr );
    for (UINT i = 0; i < ARRAY_SIZE(device_interface_properties); i++)
    {
        const struct property_desc *desc = device_interface_properties + i;
        if (memcmp( desc->key, &prop->key, sizeof(prop->key) )) continue;
        if (!desc->name) return query_property( hkey, prefix, desc->type, prop );
        return query_named_property( hkey, desc->name, desc->type, prop );
    }

    return query_property( hkey, prefix, DEVPROP_TYPE_EMPTY, prop );
}

static LSTATUS get_device_interface_property( const struct device_interface *iface, struct property *prop )
{
    LSTATUS err;
    HKEY hkey;

    if (!(err = open_device_interface_key( iface, KEY_QUERY_VALUE, TRUE, &hkey )))
    {
        err = query_device_interface_property( hkey, iface, prop );
        RegCloseKey( hkey );
    }

    if (err && err != ERROR_MORE_DATA) *prop->size = 0;
    return err;
}

LSTATUS enum_device_interface_property_keys( HKEY hkey, const struct device_interface *iface, DEVPROPKEY *buffer, ULONG *size )
{
    ULONG capacity = *size, count = 0;
    LSTATUS err = ERROR_SUCCESS;
    WCHAR path[MAX_PATH];
    HKEY props_key;

    if (capacity < ++count || !buffer) err = ERROR_MORE_DATA;
    else buffer[count - 1] = DEVPKEY_DeviceInterface_Enabled;
    if (capacity < ++count || !buffer) err = ERROR_MORE_DATA;
    else buffer[count - 1] = DEVPKEY_Device_InstanceId;
    if (capacity < ++count || !buffer) err = ERROR_MORE_DATA;
    else buffer[count - 1] = DEVPKEY_DeviceInterface_ClassGuid;

    for (UINT i = 0; i < ARRAY_SIZE(device_interface_properties); i++)
    {
        const struct property_desc *desc = device_interface_properties + i;
        if (desc->name && !RegQueryValueExW( hkey, desc->name, NULL, NULL, NULL, NULL ))
        {
            if (capacity < ++count || !buffer) err = ERROR_MORE_DATA;
            else buffer[count - 1] = *desc->key;
        }
    }

    swprintf( path, ARRAY_SIZE(path), L"%s\\Properties", iface->refstr );
    if (!open_key( hkey, path, KEY_ENUMERATE_SUB_KEYS, TRUE, &props_key ))
    {
        WCHAR name[MAX_PATH];
        for (ULONG i = 0, len = ARRAY_SIZE(name); !RegEnumValueW( props_key, i, name, &len, 0, NULL, NULL, NULL ); i++, len = ARRAY_SIZE(name))
        {
            if (capacity < ++count || !buffer) err = ERROR_MORE_DATA;
            else err = propkey_from_string( name, buffer + count - 1 );
        }
        RegCloseKey( props_key );
    }

    *size = count;
    return err;
}

static LSTATUS get_device_interface_property_keys( const struct device_interface *iface, DEVPROPKEY *buffer, ULONG *size )
{
    LSTATUS err;
    HKEY hkey;

    if ((err = open_device_interface_key( iface, KEY_ALL_ACCESS, TRUE, &hkey ))) return err;
    err = enum_device_interface_property_keys( hkey, iface, buffer, size );
    RegCloseKey( hkey );

    return err;
}

static CONFIGRET map_error( LSTATUS err )
{
    switch (err)
    {
    case ERROR_INVALID_PARAMETER:                 return CR_FAILURE;
    case ERROR_INVALID_USER_BUFFER:               return CR_INVALID_POINTER;
    case ERROR_FILE_NOT_FOUND:                    return CR_NO_SUCH_REGISTRY_KEY;
    case ERROR_MORE_DATA:                         return CR_BUFFER_SMALL;
    case ERROR_NO_MORE_ITEMS:                     return CR_NO_SUCH_VALUE;
    case ERROR_NOT_FOUND:                         return CR_NO_SUCH_VALUE;
    case ERROR_SUCCESS:                           return CR_SUCCESS;
    case ERROR_UNKNOWN_PROPERTY:                  return CR_INVALID_PROPERTY;
    default: WARN( "unmapped error %lu\n", err ); return CR_FAILURE;
    }
}

/***********************************************************************
 *           CM_MapCrToWin32Err (cfgmgr32.@)
 */
DWORD WINAPI CM_MapCrToWin32Err( CONFIGRET code, DWORD default_error )
{
    TRACE( "code: %#lx, default_error: %ld\n", code, default_error );

    switch (code)
    {
    case CR_SUCCESS:                  return ERROR_SUCCESS;
    case CR_OUT_OF_MEMORY:            return ERROR_NOT_ENOUGH_MEMORY;
    case CR_INVALID_POINTER:          return ERROR_INVALID_USER_BUFFER;
    case CR_INVALID_FLAG:             return ERROR_INVALID_FLAGS;
    case CR_INVALID_DEVNODE:
    case CR_INVALID_DEVICE_ID:
    case CR_INVALID_MACHINENAME:
    case CR_INVALID_PROPERTY:
    case CR_INVALID_REFERENCE_STRING: return ERROR_INVALID_DATA;
    case CR_NO_SUCH_DEVNODE:
    case CR_NO_SUCH_VALUE:
    case CR_NO_SUCH_DEVICE_INTERFACE: return ERROR_NOT_FOUND;
    case CR_ALREADY_SUCH_DEVNODE:     return ERROR_ALREADY_EXISTS;
    case CR_BUFFER_SMALL:             return ERROR_INSUFFICIENT_BUFFER;
    case CR_NO_REGISTRY_HANDLE:       return ERROR_INVALID_HANDLE;
    case CR_REGISTRY_ERROR:           return ERROR_REGISTRY_CORRUPT;
    case CR_NO_SUCH_REGISTRY_KEY:     return ERROR_FILE_NOT_FOUND;
    case CR_REMOTE_COMM_FAILURE:
    case CR_MACHINE_UNAVAILABLE:
    case CR_NO_CM_SERVICES:           return ERROR_SERVICE_NOT_ACTIVE;
    case CR_ACCESS_DENIED:            return ERROR_ACCESS_DENIED;
    case CR_CALL_NOT_IMPLEMENTED:     return ERROR_CALL_NOT_IMPLEMENTED;
    }

    return default_error;
}

/***********************************************************************
 *           CM_Enumerate_Classes_Ex (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Enumerate_Classes_Ex( ULONG index, GUID *class, ULONG flags, HMACHINE machine )
{
    WCHAR buffer[39];
    LSTATUS err;
    HKEY root;

    TRACE( "index %lu, class %s, flags %#lx, machine %p\n", index, debugstr_guid(class), flags, machine );
    if (machine) FIXME( "machine %p not implemented!\n", machine );

    if (!class) return CR_INVALID_POINTER;
    if (flags & ~CM_ENUMERATE_CLASSES_BITS) return CR_INVALID_FLAG;

    if (flags == CM_ENUMERATE_CLASSES_INSTALLER) root = cache_root_key( HKEY_LOCAL_MACHINE, control_classW, NULL );
    else root = cache_root_key( HKEY_LOCAL_MACHINE, device_classesW, NULL );
    if (root == (HKEY)-1) return CR_NO_SUCH_REGISTRY_KEY;

    if ((err = RegEnumKeyW( root, index, buffer, ARRAY_SIZE(buffer) ))) return map_error( err );
    return map_error( guid_from_string( buffer, class ) );
}

/***********************************************************************
 *           CM_Enumerate_Classes (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Enumerate_Classes( ULONG index, GUID *class, ULONG flags )
{
    return CM_Enumerate_Classes_Ex( index, class, flags, NULL );
}

/***********************************************************************
 *           CM_Enumerate_Enumerators_ExW (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Enumerate_Enumerators_ExW( ULONG index, WCHAR *buffer, ULONG *len, ULONG flags, HMACHINE machine )
{
    LSTATUS err;
    HKEY root;

    TRACE( "index %lu, buffer %p, len %p, flags %#lx, machine %p\n", index, buffer, len, flags, machine );
    if (machine) FIXME( "machine %p not implemented!\n", machine );

    if (!buffer || !len) return CR_INVALID_POINTER;
    if (!*len && buffer) return CR_INVALID_DATA;
    if (flags) return CR_INVALID_FLAG;

    root = cache_root_key( HKEY_LOCAL_MACHINE, enum_rootW, NULL );
    if (root == (HKEY)-1) return CR_NO_SUCH_REGISTRY_KEY;

    if (!(err = RegEnumKeyExW( root, index, buffer, len, NULL, NULL, NULL, NULL ))) *len += 1;
    return map_error( err );
}

/***********************************************************************
 *           CM_Enumerate_Enumerators_ExA (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Enumerate_Enumerators_ExA( ULONG index, char *bufferA, ULONG *lenA, ULONG flags, HMACHINE machine )
{
    WCHAR bufferW[MAX_PATH];
    DWORD lenW = ARRAY_SIZE(bufferW), maxA;
    CONFIGRET ret;

    TRACE( "index %lu, bufferA %p, lenA %p, flags %#lx, machine %p\n", index, bufferA, lenA, flags, machine );

    if (!bufferA || !lenA) return CR_INVALID_POINTER;
    if (!(maxA = *lenA) && bufferA) return CR_INVALID_DATA;

    if ((ret = CM_Enumerate_Enumerators_ExW( index, bufferW, &lenW, flags, NULL ))) return ret;
    if ((*lenA = WideCharToMultiByte( CP_ACP, 0, bufferW, lenW, NULL, 0, NULL, NULL )) > maxA || !bufferA) return CR_BUFFER_SMALL;
    WideCharToMultiByte( CP_ACP, 0, bufferW, lenW, bufferA, maxA, NULL, NULL );

    return CR_SUCCESS;
}

/***********************************************************************
 *           CM_Enumerate_EnumeratorsW (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Enumerate_EnumeratorsW( ULONG index, WCHAR *buffer, ULONG *len, ULONG flags )
{
    return CM_Enumerate_Enumerators_ExW( index, buffer, len, flags, NULL );
}

/***********************************************************************
 *           CM_Enumerate_EnumeratorsA (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Enumerate_EnumeratorsA( ULONG index, char *buffer, ULONG *len, ULONG flags )
{
    return CM_Enumerate_Enumerators_ExA( index, buffer, len, flags, NULL );
}

/***********************************************************************
 *           CM_Get_Class_Key_Name_ExW (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Get_Class_Key_Name_ExW( GUID *guid, WCHAR *name, ULONG *len, ULONG flags, HMACHINE machine )
{
    UINT capacity;

    TRACE( "guid %s, name %p, len %p, flags %#lx, machine %p\n", debugstr_guid(guid), name, len, flags, machine );
    if (machine) FIXME( "machine %p not implemented!\n", machine );
    if (flags) FIXME( "flags %#lx not implemented!\n", flags );

    if (!guid || !len) return CR_INVALID_POINTER;
    if ((capacity = *len) && !name) return CR_INVALID_POINTER;

    *len = 39;
    if (capacity < *len) return CR_BUFFER_SMALL;
    guid_string( guid, name, capacity );
    return CR_SUCCESS;
}

/***********************************************************************
 *           CM_Get_Class_Key_Name_ExA (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Get_Class_Key_Name_ExA( GUID *class, char *nameA, ULONG *len, ULONG flags, HMACHINE machine )
{
    WCHAR nameW[39];
    CONFIGRET ret;

    if ((ret = CM_Get_Class_Key_Name_ExW( class, nameA ? nameW : NULL, len, flags, machine ))) return ret;
    if (nameA) WideCharToMultiByte( CP_ACP, 0, nameW, 39, nameA, 39, NULL, NULL );

    return CR_SUCCESS;
}

/***********************************************************************
 *           CM_Get_Class_Key_NameW (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Get_Class_Key_NameW( GUID *class, WCHAR *name, ULONG *len, ULONG flags )
{
    return CM_Get_Class_Key_Name_ExW( class, name, len, flags, NULL );
}

/***********************************************************************
 *           CM_Get_Class_Key_NameA (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Get_Class_Key_NameA( GUID *class, char *name, ULONG *len, ULONG flags )
{
    return CM_Get_Class_Key_Name_ExA( class, name, len, flags, NULL );
}

/***********************************************************************
 *           CM_Open_Class_Key_ExW (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Open_Class_Key_ExW( GUID *class, const WCHAR *name, REGSAM access, REGDISPOSITION disposition, HKEY *hkey, ULONG flags, HMACHINE machine )
{
    BOOL open = disposition == RegDisposition_OpenExisting;
    WCHAR buffer[39];

    TRACE( "class %s, name %s, access %#lx, disposition %#lx, hkey %p, flags %#lx\n", debugstr_guid(class), debugstr_w(name), access, disposition, hkey, flags );
    if (machine) FIXME( "machine %p not implemented!\n", machine );

    if (name) return CR_INVALID_DATA;
    if (!hkey) return CR_INVALID_POINTER;
    if (flags & ~CM_OPEN_CLASS_KEY_BITS) return CR_INVALID_FLAG;

    if (!class) *buffer = 0;
    else guid_string( class, buffer, ARRAY_SIZE(buffer) );

    if (flags == CM_OPEN_CLASS_KEY_INSTALLER) return map_error( open_class_key( HKEY_LOCAL_MACHINE, buffer, access, open, hkey ) );
    return map_error( open_device_classes_key( HKEY_LOCAL_MACHINE, buffer, access, open, hkey ) );
}

/***********************************************************************
 *           CM_Open_Class_Key_ExA (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Open_Class_Key_ExA( GUID *class, const char *nameA, REGSAM access, REGDISPOSITION disposition, HKEY *hkey, ULONG flags, HMACHINE machine )
{
    WCHAR nameW[MAX_PATH];

    TRACE( "guid %s, nameA %s, access %#lx, disposition %#lx, hkey %p, flags %#lx\n", debugstr_guid(class), debugstr_a(nameA), access, disposition, hkey, flags );

    if (nameA) MultiByteToWideChar( CP_ACP, 0, nameA, -1, nameW, ARRAY_SIZE(nameW) );
    return CM_Open_Class_Key_ExW( class, nameA ? nameW : NULL, access, disposition, hkey, flags, machine );
}

/***********************************************************************
 *           CM_Open_Class_KeyW (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Open_Class_KeyW( GUID *class, const WCHAR *name, REGSAM access, REGDISPOSITION disposition, HKEY *hkey, ULONG flags )
{
    return CM_Open_Class_Key_ExW( class, name, access, disposition, hkey, flags, NULL );
}

/***********************************************************************
 *           CM_Open_Class_KeyA (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Open_Class_KeyA( GUID *class, const char *name, REGSAM access, REGDISPOSITION disposition, HKEY *hkey, ULONG flags )
{
    return CM_Open_Class_Key_ExA( class, name, access, disposition, hkey, flags, NULL );
}

/***********************************************************************
 *           CM_Get_Class_Registry_PropertyW (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Get_Class_Registry_PropertyW( GUID *class, ULONG property, ULONG *type, void *buffer, ULONG *len, ULONG flags, HMACHINE machine )
{
    struct property prop;
    LSTATUS err;

    TRACE( "class %s, property %#lx, type %p, buffer %p, len %p, flags %#lx, machine %p\n", debugstr_guid(class), property, type, buffer, len, flags, machine );
    if (machine) FIXME( "machine %p not implemented!\n", machine );
    if (flags) FIXME( "flags %#lx not implemented!\n", flags );

    if (!class) return CR_INVALID_POINTER;
    if ((err = init_registry_property( &prop, &DEVPKEY_DeviceClass_UpperFilters, property, type, buffer, len, FALSE ))) return map_error( err );

    return map_error( get_class_property( class, &prop ) );
}

/***********************************************************************
 *           CM_Get_Class_Registry_PropertyA (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Get_Class_Registry_PropertyA( GUID *class, ULONG property, ULONG *type, void *buffer, ULONG *len, ULONG flags, HMACHINE machine )
{
    struct property prop;
    LSTATUS err;

    TRACE( "class %s, property %#lx, type %p, buffer %p, len %p, flags %#lx, machine %p\n", debugstr_guid(class), property, type, buffer, len, flags, machine );
    if (machine) FIXME( "machine %p not implemented!\n", machine );
    if (flags) FIXME( "flags %#lx not implemented!\n", flags );

    if (!class) return CR_INVALID_POINTER;
    if ((err = init_registry_property( &prop, &DEVPKEY_DeviceClass_UpperFilters, property, type, buffer, len, TRUE ))) return map_error( err );

    return map_error( get_class_property( class, &prop ) );
}

/***********************************************************************
 *           CM_Get_Class_Property_ExW (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Get_Class_Property_ExW( const GUID *class, const DEVPROPKEY *key, DEVPROPTYPE *type, BYTE *buffer, ULONG *size, ULONG flags, HMACHINE machine )
{
    struct property prop;
    LSTATUS err;

    TRACE( "class %s, key %s, type %p, buffer %p, size %p, flags %#lx, machine %p\n", debugstr_guid(class), debugstr_DEVPROPKEY(key), type, buffer, size, flags, machine );
    if (machine) FIXME( "machine %p not implemented!\n", machine );
    if (flags) FIXME( "flags %#lx not implemented!\n", flags );

    if (!class) return CR_INVALID_POINTER;
    if ((err = init_property( &prop, key, type, buffer, size ))) return map_error( err );
    return map_error( get_class_property( class, &prop ) );
}

/***********************************************************************
 *           CM_Get_Class_PropertyW (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Get_Class_PropertyW( const GUID *class, const DEVPROPKEY *key, DEVPROPTYPE *type, BYTE *buffer, ULONG *size, ULONG flags )
{
    return CM_Get_Class_Property_ExW( class, key, type, buffer, size, flags, NULL );
}

/***********************************************************************
 *           CM_Get_Class_Property_Keys_Ex (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Get_Class_Property_Keys_Ex( const GUID *class, DEVPROPKEY *keys, ULONG *count, ULONG flags, HMACHINE machine )
{
    LSTATUS err;

    TRACE( "class %s, keys %p, size %p, flags %#lx, machine %p\n", debugstr_guid(class), keys, count, flags, machine );
    if (machine) FIXME( "machine %p not implemented!\n", machine );
    if (flags) FIXME( "flags %#lx not implemented!\n", flags );

    if (!class) return CR_INVALID_POINTER;
    if (!count) return CR_INVALID_POINTER;
    if (*count && !keys) return CR_INVALID_POINTER;

    err = get_class_property_keys( class, keys, count );
    if (err && err != ERROR_MORE_DATA) *count = 0;
    return map_error( err );
}

/***********************************************************************
 *           CM_Get_Class_Property_Keys (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Get_Class_Property_Keys( const GUID *class, DEVPROPKEY *keys, ULONG *count, ULONG flags )
{
    return CM_Get_Class_Property_Keys_Ex( class, keys, count, flags, NULL );
}

/***********************************************************************
 *           CM_Get_Device_Interface_List_Size_ExW (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_List_Size_ExW( ULONG *len, GUID *class, DEVINSTID_W instance_id, ULONG flags, HMACHINE machine )
{
    BOOL all = flags == CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES;

    TRACE( "len %p, class %s, instance %s, flags %#lx, machine %p\n", len, debugstr_guid(class), debugstr_w(instance_id), flags, machine );
    if (machine) FIXME( "machine %p not implemented!\n", machine );

    if (!class) return CR_FAILURE;
    if (!len) return CR_INVALID_POINTER;
    if (flags & ~CM_GET_DEVICE_INTERFACE_LIST_BITS) return CR_INVALID_FLAG;

    *len = 0;
    return map_error( enum_device_interface_list( class, instance_id, all, enum_objects_size, len ) );
}

/***********************************************************************
 *           CM_Get_Device_Interface_List_Size_ExA (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_List_Size_ExA( ULONG *len, GUID *class, DEVINSTID_A instance_idA, ULONG flags, HMACHINE machine )
{
    WCHAR instance_idW[MAX_PATH];

    if (instance_idA) MultiByteToWideChar( CP_ACP, 0, instance_idA, -1, instance_idW, ARRAY_SIZE(instance_idW) );
    return CM_Get_Device_Interface_List_Size_ExW( len, class, instance_idA ? instance_idW : NULL, flags, machine );
}

/***********************************************************************
 *           CM_Get_Device_Interface_List_SizeW (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_List_SizeW( ULONG *len, GUID *class, DEVINSTID_W instance_id, ULONG flags )
{
    return CM_Get_Device_Interface_List_Size_ExW( len, class, instance_id, flags, NULL );
}

/***********************************************************************
 *           CM_Get_Device_Interface_List_SizeA (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_List_SizeA( ULONG *len, GUID *class, DEVINSTID_A instance_id, ULONG flags )
{
    return CM_Get_Device_Interface_List_Size_ExA( len, class, instance_id, flags, NULL );
}

/***********************************************************************
 *           CM_Get_Device_Interface_List_ExW (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_List_ExW( GUID *class, DEVINSTID_W instance_id, WCHAR *buffer, ULONG len, ULONG flags, HMACHINE machine )
{
    struct enum_objects_append_params params = {.buffer = buffer, .len = len};
    BOOL all = flags == CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES;

    TRACE( "class %s, instance %s, buffer %p, len %lu, flags %#lx, machine %p\n", debugstr_guid(class), debugstr_w(instance_id), buffer, len, flags, machine );
    if (machine) FIXME( "machine %p not implemented!\n", machine );

    if (!class) return CR_FAILURE;
    if (!buffer) return CR_INVALID_POINTER;
    if (!len) return CR_BUFFER_SMALL;
    if (flags & ~CM_GET_DEVICE_INTERFACE_LIST_BITS) return CR_INVALID_FLAG;

    memset( buffer, 0, len * sizeof(WCHAR) );
    return map_error( enum_device_interface_list( class, instance_id, all, enum_objects_append, &params ) );
}

/***********************************************************************
 *           CM_Get_Device_Interface_List_ExA (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_List_ExA( GUID *class, DEVINSTID_A instance_idA, char *bufferA, ULONG len, ULONG flags, HMACHINE machine )
{
    WCHAR instance_idW[MAX_PATH], *bufferW;
    CONFIGRET ret;

    bufferW = bufferA ? malloc( len * sizeof(WCHAR) ) : NULL;
    if (instance_idA) MultiByteToWideChar( CP_ACP, 0, instance_idA, -1, instance_idW, ARRAY_SIZE(instance_idW) );
    ret = CM_Get_Device_Interface_List_ExW( class, instance_idA ? instance_idW : NULL, bufferA ? bufferW : NULL, len, flags, machine );
    if (!ret && bufferA && !WideCharToMultiByte( CP_ACP, 0, bufferW, len, bufferA, len, 0, 0 ))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) ret = CR_BUFFER_SMALL;
        else ret = CR_FAILURE;
    }
    free( bufferW );

    return ret;
}

/***********************************************************************
 *           CM_Get_Device_Interface_ListW (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_ListW( GUID *class, DEVINSTID_W instance_id, WCHAR *buffer, ULONG len, ULONG flags )
{
    return CM_Get_Device_Interface_List_ExW( class, instance_id, buffer, len, flags, NULL );
}

/***********************************************************************
 *           CM_Get_Device_Interface_ListA (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_ListA( GUID *class, DEVINSTID_A instance_id, char *buffer, ULONG len, ULONG flags )
{
    return CM_Get_Device_Interface_List_ExA( class, instance_id, buffer, len, flags, NULL );
}

/***********************************************************************
 *           CM_Open_Device_Interface_Key_ExW (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Open_Device_Interface_Key_ExW( const WCHAR *name, REGSAM access, REGDISPOSITION disposition, HKEY *hkey, ULONG flags, HMACHINE machine )
{
    BOOL open = disposition == RegDisposition_OpenExisting;
    struct device_interface iface;
    WCHAR path[MAX_PATH];
    HKEY iface_key;
    LSTATUS err;

    TRACE( "name %s, access %#lx, disposition %#lx, hkey %p, flags %#lx\n", debugstr_w(name), access, disposition, hkey, flags );
    if (machine) FIXME( "machine %p not implemented!\n", machine );

    if (!name) return CR_INVALID_POINTER;
    if (init_device_interface( &iface, name )) return CR_INVALID_DATA;
    if (!hkey) return CR_INVALID_POINTER;
    if (flags) return CR_INVALID_FLAG;

    if (open_device_interface_key( &iface, KEY_ALL_ACCESS, TRUE, &iface_key )) return CR_NO_SUCH_DEVICE_INTERFACE;
    swprintf( path, ARRAY_SIZE(path), L"%s\\Device Parameters", iface.refstr );
    err = open_key( iface_key, path, access, open, hkey );
    RegCloseKey( iface_key );

    return map_error( err );
}

/***********************************************************************
 *           CM_Open_Device_Interface_Key_ExA (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Open_Device_Interface_Key_ExA( const char *ifaceA, REGSAM access, REGDISPOSITION disposition, HKEY *hkey, ULONG flags, HMACHINE machine )
{
    WCHAR ifaceW[MAX_PATH];

    TRACE( "ifaceA %s, access %#lx, disposition %#lx, hkey %p, flags %#lx\n", debugstr_a(ifaceA), access, disposition, hkey, flags );

    if (ifaceA) MultiByteToWideChar( CP_ACP, 0, ifaceA, -1, ifaceW, ARRAY_SIZE(ifaceW) );
    return CM_Open_Device_Interface_Key_ExW( ifaceA ? ifaceW : NULL, access, disposition, hkey, flags, machine );
}

/***********************************************************************
 *           CM_Open_Device_Interface_KeyW (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Open_Device_Interface_KeyW( const WCHAR *iface, REGSAM access, REGDISPOSITION disposition, HKEY *hkey, ULONG flags )
{
    return CM_Open_Device_Interface_Key_ExW( iface, access, disposition, hkey, flags, NULL );
}

/***********************************************************************
 *           CM_Open_Device_Interface_KeyA (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Open_Device_Interface_KeyA( const char *iface, REGSAM access, REGDISPOSITION disposition, HKEY *hkey, ULONG flags )
{
    return CM_Open_Device_Interface_Key_ExA( iface, access, disposition, hkey, flags, NULL );
}

/***********************************************************************
 *           CM_Get_Device_Interface_Property_ExW (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_Property_ExW( const WCHAR *name, const DEVPROPKEY *key, DEVPROPTYPE *type,
                                                       BYTE *buffer, ULONG *size, ULONG flags, HMACHINE machine )
{
    struct device_interface iface;
    struct property prop;
    LSTATUS err;

    TRACE( "name %s, key %p, type %p, buffer %p, size %p, flags %#lx\n", debugstr_w(name), key, type, buffer, size, flags);
    if (machine) FIXME( "machine %p not implemented!\n", machine );

    if (!name) return CR_INVALID_POINTER;
    if (init_device_interface( &iface, name )) return CR_NO_SUCH_DEVICE_INTERFACE;
    if ((err = init_property( &prop, key, type, buffer, size ))) return map_error( err );
    if (flags) return CR_INVALID_FLAG;

    return map_error( get_device_interface_property( &iface, &prop ) );
}

/***********************************************************************
 *           CM_Get_Device_Interface_PropertyW (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_PropertyW( const WCHAR *iface, const DEVPROPKEY *key, DEVPROPTYPE *type, BYTE *buffer, ULONG *size, ULONG flags )
{
    return CM_Get_Device_Interface_Property_ExW( iface, key, type, buffer, size, flags, NULL );
}

/***********************************************************************
 *           CM_Get_Device_Interface_Property_Keys_ExW (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_Property_Keys_ExW( const WCHAR *name, DEVPROPKEY *buffer, ULONG *size, ULONG flags, HMACHINE machine )
{
    struct device_interface iface;
    LSTATUS err;

    TRACE( "name %s, buffer %p, size %p, flags %#lx\n", debugstr_w(name), buffer, size, flags);
    if (machine) FIXME( "machine %p not implemented!\n", machine );
    if (flags) FIXME( "flags %#lx not implemented!\n", flags );

    if (!name || !size) return CR_INVALID_POINTER;
    if (*size && !buffer) return CR_INVALID_POINTER;
    if (init_device_interface( &iface, name ))
    {
        *size = 0;
        return CR_INVALID_DATA;
    }

    err = get_device_interface_property_keys( &iface, buffer, size );
    if (err && err != ERROR_MORE_DATA) *size = 0;
    if (err == ERROR_FILE_NOT_FOUND) return CR_NO_SUCH_DEVICE_INTERFACE;
    return map_error( err );
}

/***********************************************************************
 *           CM_Get_Device_Interface_Property_KeysW (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_Property_KeysW( const WCHAR *iface, DEVPROPKEY *keys, ULONG *count, ULONG flags )
{
    return CM_Get_Device_Interface_Property_Keys_ExW( iface, keys, count, flags, NULL );
}
