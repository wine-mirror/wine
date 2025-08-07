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

#include "wine/debug.h"
#include "wine/rbtree.h"
#include "winreg.h"
#include "cfgmgr32.h"
#include "winuser.h"
#include "dbt.h"
#include "wine/plugplay.h"
#include "setupapi.h"
#include "devfiltertypes.h"
#include "devquery.h"

#include "initguid.h"
#include "devpkey.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

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

struct cm_notify_context
{
    HDEVNOTIFY notify;
    void *user_data;
    PCM_NOTIFY_CALLBACK callback;
};

CALLBACK DWORD devnotify_callback( HANDLE handle, DWORD flags, DEV_BROADCAST_HDR *header )
{
    struct cm_notify_context *ctx = handle;
    CM_NOTIFY_EVENT_DATA *event_data;
    CM_NOTIFY_ACTION action;
    DWORD size, ret;

    TRACE( "(%p, %#lx, %p)\n", handle, flags, header );

    switch (flags)
    {
    case DBT_DEVICEARRIVAL:
        action = CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL;
        break;
    case DBT_DEVICEREMOVECOMPLETE:
        FIXME( "CM_NOTIFY_ACTION_DEVICEREMOVECOMPLETE not implemented\n" );
        action = CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL;
        break;
    case DBT_CUSTOMEVENT:
        action = CM_NOTIFY_ACTION_DEVICECUSTOMEVENT;
        break;
    default:
        FIXME( "Unexpected flags value: %#lx\n", flags );
        return 0;
    }

    switch (header->dbch_devicetype)
    {
    case DBT_DEVTYP_DEVICEINTERFACE:
    {
        const DEV_BROADCAST_DEVICEINTERFACE_W *iface = (DEV_BROADCAST_DEVICEINTERFACE_W *)header;
        UINT data_size = wcslen( iface->dbcc_name ) + 1;

        size = offsetof( CM_NOTIFY_EVENT_DATA, u.DeviceInterface.SymbolicLink[data_size] );
        if (!(event_data = calloc( 1, size ))) return 0;

        event_data->FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
        event_data->u.DeviceInterface.ClassGuid = iface->dbcc_classguid;
        memcpy( event_data->u.DeviceInterface.SymbolicLink, iface->dbcc_name, data_size * sizeof(WCHAR) );
        break;
    }
    case DBT_DEVTYP_HANDLE:
    {
        const DEV_BROADCAST_HANDLE *handle = (DEV_BROADCAST_HANDLE *)header;
        UINT data_size = handle->dbch_size - 2 * sizeof(WCHAR) - offsetof( DEV_BROADCAST_HANDLE, dbch_data );

        size = offsetof( CM_NOTIFY_EVENT_DATA, u.DeviceHandle.Data[data_size] );
        if (!(event_data = calloc( 1, size ))) return 0;

        event_data->FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEHANDLE;
        event_data->u.DeviceHandle.EventGuid = handle->dbch_eventguid;
        event_data->u.DeviceHandle.NameOffset = handle->dbch_nameoffset;
        event_data->u.DeviceHandle.DataSize = data_size;
        memcpy( event_data->u.DeviceHandle.Data, handle->dbch_data, data_size );
        break;
    }
    default:
        FIXME( "Unexpected devicetype value: %#lx\n", header->dbch_devicetype );
        return 0;
    }

    ret = ctx->callback( ctx, ctx->user_data, action, event_data, size );
    free( event_data );
    return ret;
}

static const char *debugstr_CM_NOTIFY_FILTER( const CM_NOTIFY_FILTER *filter )
{
    if (!filter) return "(null)";
    switch (filter->FilterType)
    {
    case CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE:
        return wine_dbg_sprintf( "{%#lx %lx CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE %lu {{%s}}}", filter->cbSize,
                                 filter->Flags, filter->Reserved,
                                 debugstr_guid( &filter->u.DeviceInterface.ClassGuid ) );
    case CM_NOTIFY_FILTER_TYPE_DEVICEHANDLE:
        return wine_dbg_sprintf( "{%#lx %lx CM_NOTIFY_FILTER_TYPE_DEVICEHANDLE %lu {{%p}}}", filter->cbSize,
                                 filter->Flags, filter->Reserved, filter->u.DeviceHandle.hTarget );
    case CM_NOTIFY_FILTER_TYPE_DEVICEINSTANCE:
        return wine_dbg_sprintf( "{%#lx %lx CM_NOTIFY_FILTER_TYPE_DEVICEINSTANCE %lu {{%s}}}", filter->cbSize,
                                 filter->Flags, filter->Reserved, debugstr_w( filter->u.DeviceInstance.InstanceId ) );
    default:
        return wine_dbg_sprintf( "{%#lx %lx (unknown FilterType %d) %lu}", filter->cbSize, filter->Flags,
                                 filter->FilterType, filter->Reserved );
    }
}

static CONFIGRET create_notify_context( const CM_NOTIFY_FILTER *filter, HCMNOTIFICATION *notify_handle,
                                        PCM_NOTIFY_CALLBACK callback, void *user_data )
{
    union
    {
        DEV_BROADCAST_HDR header;
        DEV_BROADCAST_DEVICEINTERFACE_W iface;
        DEV_BROADCAST_HANDLE handle;
    } notify_filter = {0};
    struct cm_notify_context *ctx;
    static const GUID GUID_NULL;

    switch (filter->FilterType)
    {
    case CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE:
        notify_filter.iface.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        if (filter->Flags & CM_NOTIFY_FILTER_FLAG_ALL_INTERFACE_CLASSES)
        {
            if (!IsEqualGUID( &filter->u.DeviceInterface.ClassGuid, &GUID_NULL )) return CR_INVALID_DATA;
            notify_filter.iface.dbcc_size = offsetof( DEV_BROADCAST_DEVICEINTERFACE_W, dbcc_classguid );
        }
        else
        {
            notify_filter.iface.dbcc_size = offsetof( DEV_BROADCAST_DEVICEINTERFACE_W, dbcc_name );
            notify_filter.iface.dbcc_classguid = filter->u.DeviceInterface.ClassGuid;
        }
        break;
    case CM_NOTIFY_FILTER_TYPE_DEVICEHANDLE:
        notify_filter.handle.dbch_devicetype = DBT_DEVTYP_HANDLE;
        notify_filter.handle.dbch_size = sizeof(notify_filter.handle);
        notify_filter.handle.dbch_handle = filter->u.DeviceHandle.hTarget;
        break;
    case CM_NOTIFY_FILTER_TYPE_DEVICEINSTANCE:
        FIXME( "CM_NOTIFY_FILTER_TYPE_DEVICEINSTANCE is not supported!\n" );
        return CR_CALL_NOT_IMPLEMENTED;
    default:
        return CR_INVALID_DATA;
    }

    if (!(ctx = calloc( 1, sizeof(*ctx) ))) return CR_OUT_OF_MEMORY;

    ctx->user_data = user_data;
    ctx->callback = callback;
    if (!(ctx->notify = I_ScRegisterDeviceNotification( ctx, &notify_filter.header, devnotify_callback )))
    {
        free( ctx );
        switch (GetLastError())
        {
        case ERROR_NOT_ENOUGH_MEMORY: return CR_OUT_OF_MEMORY;
        case ERROR_INVALID_PARAMETER: return CR_INVALID_DATA;
        default: return CR_FAILURE;
        }
    }
    *notify_handle = ctx;
    return CR_SUCCESS;
}

/***********************************************************************
 *           CM_Register_Notification (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Register_Notification( CM_NOTIFY_FILTER *filter, void *context,
                                           PCM_NOTIFY_CALLBACK callback, HCMNOTIFICATION *notify_context )
{
    TRACE( "(%s %p %p %p)\n", debugstr_CM_NOTIFY_FILTER( filter ), context, callback, notify_context );

    if (!notify_context) return CR_FAILURE;
    if (!filter || !callback || filter->cbSize != sizeof(*filter)) return CR_INVALID_DATA;

    return create_notify_context( filter, notify_context, callback, context );
}

/***********************************************************************
 *           CM_Unregister_Notification (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Unregister_Notification( HCMNOTIFICATION notify )
{
    struct cm_notify_context *ctx = notify;

    TRACE( "(%p)\n", notify );

    if (!notify) return CR_INVALID_DATA;

    I_ScUnregisterDeviceNotification( ctx->notify );
    free( ctx );

    return CR_SUCCESS;
}

/***********************************************************************
 *           CM_Get_Device_Interface_PropertyW (cfgmgr32.@)
 */
CONFIGRET WINAPI CM_Get_Device_Interface_PropertyW( LPCWSTR device_interface, const DEVPROPKEY *property_key,
                                                    DEVPROPTYPE *property_type, BYTE *property_buffer,
                                                    ULONG *property_buffer_size, ULONG flags )
{
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    HDEVINFO set;
    DWORD err;
    BOOL ret;

    TRACE( "%s %p %p %p %p %ld.\n", debugstr_w(device_interface), property_key, property_type, property_buffer,
           property_buffer_size, flags);

    if (!property_key) return CR_FAILURE;
    if (!device_interface || !property_type || !property_buffer_size) return CR_INVALID_POINTER;
    if (*property_buffer_size && !property_buffer) return CR_INVALID_POINTER;
    if (flags) return CR_INVALID_FLAG;

    set = SetupDiCreateDeviceInfoListExW( NULL, NULL, NULL, NULL );
    if (set == INVALID_HANDLE_VALUE) return CR_OUT_OF_MEMORY;
    if (!SetupDiOpenDeviceInterfaceW( set, device_interface, 0, &iface ))
    {
        SetupDiDestroyDeviceInfoList( set );
        TRACE( "No interface %s, err %lu.\n", debugstr_w( device_interface ), GetLastError());
        return CR_NO_SUCH_DEVICE_INTERFACE;
    }

    ret = SetupDiGetDeviceInterfacePropertyW( set, &iface, property_key, property_type, property_buffer,
                                              *property_buffer_size, property_buffer_size, 0 );
    err = ret ? 0 : GetLastError();
    SetupDiDestroyDeviceInfoList( set );
    switch (err)
    {
    case ERROR_SUCCESS:
        return CR_SUCCESS;
    case ERROR_INSUFFICIENT_BUFFER:
        return CR_BUFFER_SMALL;
    case ERROR_NOT_FOUND:
        return CR_NO_SUCH_VALUE;
    default:
        return CR_FAILURE;
    }
}

static BOOL dev_properties_append( DEVPROPERTY **properties, ULONG *props_len, const DEVPROPKEY *key, DEVPROPTYPE type,
                                   ULONG buf_size, void *buf )
{
    DEVPROPERTY *tmp;

    if (!(tmp = realloc( *properties, (*props_len + 1) * sizeof( **properties ))))
        return FALSE;
    *properties = tmp;

    tmp = &tmp[*props_len];
    tmp->CompKey.Key = *key;
    tmp->CompKey.Store = DEVPROP_STORE_SYSTEM;
    tmp->CompKey.LocaleName = NULL;
    tmp->Type = type;
    tmp->BufferSize = buf_size;
    tmp->Buffer = buf;

    *props_len += 1;
    return TRUE;
}

static HRESULT dev_object_iface_get_props( DEV_OBJECT *obj, HDEVINFO set, SP_DEVICE_INTERFACE_DATA *iface_data,
                                           ULONG props_len, const DEVPROPCOMPKEY *props, BOOL all_props )
{
    DEVPROPKEY *all_keys = NULL;
    DWORD keys_len = 0, i = 0;
    HRESULT hr = S_OK;

    obj->cPropertyCount = 0;
    obj->pProperties = NULL;
    if (!props && !all_props)
        return S_OK;

    if (all_props)
    {
        DWORD req = 0;
        if (SetupDiGetDeviceInterfacePropertyKeys( set, iface_data, NULL, 0, &req, 0 )
            || GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return HRESULT_FROM_WIN32( GetLastError() );

        keys_len = req;
        if (!(all_keys = calloc( keys_len, sizeof( *all_keys ) )))
            return E_OUTOFMEMORY;
        if (!SetupDiGetDeviceInterfacePropertyKeys( set, iface_data, all_keys, keys_len, &req, 0 ))
        {
            free( all_keys );
            return HRESULT_FROM_WIN32( GetLastError() );
        }
    }
    else
        keys_len = props_len;

    for (i = 0; i < keys_len; i++)
    {
        const DEVPROPKEY *key = all_keys ? &all_keys[i] : &props[i].Key;
        DWORD req = 0, size;
        DEVPROPTYPE type;
        BYTE *buf;

        if (props && props[i].Store != DEVPROP_STORE_SYSTEM)
        {
            FIXME( "Unsupported Store value: %d\n", props[i].Store );
            continue;
        }
        if (SetupDiGetDeviceInterfacePropertyW( set, iface_data, key, &type, NULL, 0, &req, 0 )
            || GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            if (props && !dev_properties_append( (DEVPROPERTY **)&obj->pProperties, &obj->cPropertyCount, key,
                                                 DEVPROP_TYPE_EMPTY, 0, NULL ))
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }
            continue;
        }

        size = req;
        if (!(buf = calloc( 1, size )))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        if (!SetupDiGetDeviceInterfacePropertyW( set, iface_data, key, &type, buf, size, &req, 0 ))
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            free( buf );
            goto done;
        }
        if (!dev_properties_append( (DEVPROPERTY **)&obj->pProperties, &obj->cPropertyCount, key, type, size, buf ))
        {
            free( buf );
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }

done:
    free( all_keys );
    if (FAILED( hr ))
    {
        DevFreeObjectProperties( obj->cPropertyCount, obj->pProperties );
        obj->cPropertyCount = 0;
        obj->pProperties = NULL;
    }
    return hr;
}

typedef HRESULT (*enum_device_object_cb)( DEV_OBJECT object, void *context );

static HRESULT enum_dev_objects( DEV_OBJECT_TYPE type, ULONG props_len, const DEVPROPCOMPKEY *props,
                                 BOOL all_props, enum_device_object_cb callback, void *data )
{
    HKEY iface_key;
    HRESULT hr = S_OK;

    DWORD i;

    switch (type)
    {
    case DevObjectTypeDeviceInterface:
    case DevObjectTypeDeviceInterfaceDisplay:
        break;
    case DevObjectTypeDeviceContainer:
    case DevObjectTypeDevice:
    case DevObjectTypeDeviceInterfaceClass:
    case DevObjectTypeAEP:
    case DevObjectTypeAEPContainer:
    case DevObjectTypeDeviceInstallerClass:
    case DevObjectTypeDeviceContainerDisplay:
    case DevObjectTypeAEPService:
    case DevObjectTypeDevicePanel:
    case DevObjectTypeAEPProtocol:
        FIXME("Unsupported DEV_OJBECT_TYPE: %d\n", type );
    default:
        return S_OK;
    }

    if (!(iface_key = SetupDiOpenClassRegKeyExW( NULL, KEY_ENUMERATE_SUB_KEYS, DIOCR_INTERFACE, NULL, NULL )))
        return HRESULT_FROM_WIN32( GetLastError() );

    for (i = 0; SUCCEEDED( hr ); i++)
    {
        char buffer[sizeof( SP_DEVICE_INTERFACE_DETAIL_DATA_W ) + MAX_PATH * sizeof( WCHAR )];
        SP_DEVICE_INTERFACE_DATA iface = {.cbSize = sizeof( iface )};
        SP_DEVICE_INTERFACE_DETAIL_DATA_W *detail = (void *)buffer;
        HDEVINFO set = INVALID_HANDLE_VALUE;
        WCHAR iface_guid_str[40];
        DWORD ret, len, j;
        GUID iface_class;

        len = ARRAY_SIZE( iface_guid_str );
        ret = RegEnumKeyExW( iface_key, i, iface_guid_str, &len, NULL, NULL, NULL, NULL );
        if (ret)
        {
            hr = (ret == ERROR_NO_MORE_ITEMS) ? S_OK : HRESULT_FROM_WIN32( ret );
            break;
        }

        iface_guid_str[37] = '\0';
        if (!UuidFromStringW( &iface_guid_str[1], &iface_class ))
        {
            set = SetupDiGetClassDevsW( &iface_class, NULL, NULL, DIGCF_DEVICEINTERFACE );
            if (set == INVALID_HANDLE_VALUE) hr = HRESULT_FROM_WIN32( GetLastError() );
        }
        else
        {
            ERR( "Could not parse device interface GUID %s\n", debugstr_w( iface_guid_str ) );
            continue;
        }

        for (j = 0; SUCCEEDED( hr ) && SetupDiEnumDeviceInterfaces( set, NULL, &iface_class, j, &iface ); j++)
        {
            DEV_OBJECT obj = {0};

            detail->cbSize = sizeof( *detail );
            if (!SetupDiGetDeviceInterfaceDetailW( set, &iface, detail, sizeof( buffer ), NULL, NULL )) continue;

            obj.ObjectType = type;
            obj.pszObjectId = detail->DevicePath;
            if (SUCCEEDED( (hr = dev_object_iface_get_props( &obj, set, &iface, props_len, props, all_props )) ))
                hr = callback( obj, data );
        }

        if (set != INVALID_HANDLE_VALUE)
            SetupDiDestroyDeviceInfoList( set );
    }
    RegCloseKey( iface_key );
    return hr;
}

struct objects_list
{
    DEV_OBJECT *objects;
    ULONG len;
};

static HRESULT dev_objects_append( DEV_OBJECT obj, void *data )
{
    struct objects_list *objects = data;
    WCHAR *id;
    DEV_OBJECT *tmp;

    if (!(id = wcsdup( obj.pszObjectId )))
        return E_OUTOFMEMORY;
    if (!(tmp = realloc( objects->objects, (objects->len + 1) * sizeof( obj ) )))
    {
        free( id );
        return E_OUTOFMEMORY;
    }

    objects->objects = tmp;
    tmp = &tmp[objects->len++];
    *tmp = obj;
    tmp->pszObjectId = id;
    return S_OK;
}

HRESULT WINAPI DevGetObjects( DEV_OBJECT_TYPE type, ULONG flags, ULONG props_len, const DEVPROPCOMPKEY *props,
                              ULONG filters_len, const DEVPROP_FILTER_EXPRESSION *filters, ULONG *objs_len,
                              const DEV_OBJECT **objs )
{
    TRACE( "(%d, %#lx, %lu, %p, %lu, %p, %p, %p)\n", type, flags, props_len, props, filters_len, filters, objs_len, objs );
    return DevGetObjectsEx( type, flags, props_len, props, filters_len, filters, 0, NULL, objs_len, objs );
}

HRESULT WINAPI DevGetObjectsEx( DEV_OBJECT_TYPE type, ULONG flags, ULONG props_len, const DEVPROPCOMPKEY *props,
                                ULONG filters_len, const DEVPROP_FILTER_EXPRESSION *filters, ULONG params_len,
                                const DEV_QUERY_PARAMETER *params, ULONG *objs_len, const DEV_OBJECT **objs )
{
    ULONG valid_flags = DevQueryFlagAllProperties | DevQueryFlagLocalize;
    struct objects_list objects = {0};
    HRESULT hr = S_OK;

    TRACE( "(%d, %#lx, %lu, %p, %lu, %p, %lu, %p, %p, %p)\n", type, flags, props_len, props, filters_len, filters,
           params_len, params, objs_len, objs );

    if (!!props_len != !!props || !!filters_len != !!filters || !!params_len != !!params || (flags & ~valid_flags)
        || (props_len && (flags & DevQueryFlagAllProperties)))
        return E_INVALIDARG;
    if (filters)
        FIXME( "Query filters are not supported!\n" );
    if (params)
        FIXME( "Query parameters are not supported!\n" );

    *objs = NULL;
    *objs_len = 0;

    hr = enum_dev_objects( type, props_len, props, !!(flags & DevQueryFlagAllProperties), dev_objects_append, &objects );
    if (SUCCEEDED( hr ))
    {
        *objs = objects.objects;
        *objs_len = objects.len;
    }
    else
        DevFreeObjects( objects.len, objects.objects );

    return hr;
}

void WINAPI DevFreeObjects( ULONG objs_len, const DEV_OBJECT *objs )
{
    DEV_OBJECT *objects = (DEV_OBJECT *)objs;
    ULONG i;

    TRACE( "(%lu, %p)\n", objs_len, objs );

    for (i = 0; i < objs_len; i++)
    {
        DevFreeObjectProperties( objects[i].cPropertyCount, objects[i].pProperties );
        free( (void *)objects[i].pszObjectId );
    }
    free( objects );
    return;
}

struct device_iface_path
{
    struct rb_entry entry;
    WCHAR *path;
};

static int device_iface_path_cmp( const void *key, const struct rb_entry *entry )
{
    const struct device_iface_path *iface = RB_ENTRY_VALUE( entry, struct device_iface_path, entry );
    return wcsicmp( key, iface->path );
}

struct device_query_context
{
    LONG ref;
    DEV_OBJECT_TYPE type;
    ULONG flags;
    ULONG prop_keys_len;
    DEVPROPCOMPKEY *prop_keys;
    BOOL filters;

    CRITICAL_SECTION cs;
    PDEV_QUERY_RESULT_CALLBACK callback;
    void *user_data;
    DEV_QUERY_STATE state;
    HANDLE closed;
    struct rb_tree known_ifaces;
    HCMNOTIFICATION notify;
};

static HRESULT device_query_context_add_object( DEV_OBJECT obj, void *data )
{
    DEV_QUERY_RESULT_ACTION_DATA action_data = {0};
    struct device_query_context *ctx = data;
    struct device_iface_path *iface_entry = NULL;
    HRESULT hr = S_OK;

    TRACE( "(%s, %p)\n", debugstr_w( obj.pszObjectId ), data );

    action_data.Action = DevQueryResultAdd;
    action_data.Data.DeviceObject = obj;
    ctx->callback( (HDEVQUERY)ctx, ctx->user_data, &action_data );

    EnterCriticalSection( &ctx->cs );
    if (ctx->state == DevQueryStateClosed)
        hr = E_CHANGED_STATE;
    else if (obj.ObjectType == DevObjectTypeDeviceInterface || obj.ObjectType == DevObjectTypeDeviceInterfaceDisplay)
    {
        if (!(iface_entry = calloc( 1, sizeof( *iface_entry ) )) || !(iface_entry->path = wcsdup( obj.pszObjectId )))
        {
            if (iface_entry) free( iface_entry->path );
            free( iface_entry );
            hr = E_OUTOFMEMORY;
        }
        else if (rb_put( &ctx->known_ifaces, iface_entry->path, &iface_entry->entry ))
        {
            free( iface_entry->path );
            free( iface_entry );
        }
    }
    LeaveCriticalSection( &ctx->cs );

    DevFreeObjectProperties( obj.cPropertyCount, obj.pProperties );
    return hr;
}

static HRESULT device_query_context_create( struct device_query_context **query, DEV_OBJECT_TYPE type, ULONG flags,
                                            ULONG props_len, const DEVPROPCOMPKEY *props,
                                            PDEV_QUERY_RESULT_CALLBACK callback, void *user_data )
{
    struct device_query_context *ctx;
    ULONG i;

    if (!(ctx = calloc( 1, sizeof( *ctx ))))
        return E_OUTOFMEMORY;
    ctx->ref = 1;
    if (!(flags & DevQueryFlagAsyncClose))
    {
        ctx->closed = CreateEventW( NULL, FALSE, FALSE, NULL );
        if (ctx->closed == INVALID_HANDLE_VALUE)
        {
            free( ctx );
            return HRESULT_FROM_WIN32( GetLastError() );
        }
    }
    ctx->prop_keys_len = props_len;
    if (props_len && !(ctx->prop_keys = calloc( props_len, sizeof( *props ) )))
    {
        if (ctx->closed) CloseHandle( ctx->closed );
        free( ctx );
        return E_OUTOFMEMORY;
    }
    for (i = 0; i < props_len; i++)
    {
        ctx->prop_keys[i].Key = props[i].Key;
        ctx->prop_keys[i].Store = props[i].Store;
    }
    InitializeCriticalSectionEx( &ctx->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );
    ctx->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": device_query_context.cs");
    ctx->type = type;
    ctx->flags = flags;
    ctx->callback = callback;
    ctx->user_data = user_data;
    ctx->state = DevQueryStateInitialized;
    rb_init( &ctx->known_ifaces, device_iface_path_cmp );

    *query = ctx;
    return S_OK;
}

static void device_query_context_addref( struct device_query_context *ctx )
{
    InterlockedIncrement( &ctx->ref );
}

static void device_iface_path_free( struct rb_entry *entry, void *data )
{
    struct device_iface_path *path = RB_ENTRY_VALUE( entry, struct device_iface_path, entry );
    free( path->path );
    free( path );
}

static void device_query_context_release( struct device_query_context *ctx )
{
    if (!InterlockedDecrement( &ctx->ref ))
    {
        free( ctx->prop_keys );
        ctx->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection( &ctx->cs );
        if (ctx->closed) CloseHandle( ctx->closed );
        rb_destroy( &ctx->known_ifaces, device_iface_path_free, NULL );
        free( ctx );
    }
}

static void device_query_context_notify_state_change( struct device_query_context *ctx, DEV_QUERY_STATE state )
{
    DEV_QUERY_RESULT_ACTION_DATA action_data = {0};

    action_data.Action = DevQueryResultStateChange;
    action_data.Data.State = state;
    ctx->callback( (HDEVQUERY)ctx, ctx->user_data, &action_data );
}

static void CALLBACK device_query_context_notify_enum_completed_async( TP_CALLBACK_INSTANCE *instance, void *data )
{
    device_query_context_notify_state_change( data, DevQueryStateEnumCompleted );
    device_query_context_release( data );
}

static void CALLBACK device_query_context_notify_closed_async( TP_CALLBACK_INSTANCE *instance, void *data )
{
    device_query_context_notify_state_change( data, DevQueryStateClosed );
    device_query_context_release( data );
}

static void CALLBACK device_query_context_notify_aborted_async( TP_CALLBACK_INSTANCE *instance, void *data )
{
    device_query_context_notify_state_change( data, DevQueryStateAborted );
    device_query_context_release( data );
}

struct devquery_notify_data
{
    DEV_QUERY_RESULT_ACTION_DATA action_data;
    struct device_query_context *ctx;
};

static void CALLBACK device_query_notify_dev_async( TP_CALLBACK_INSTANCE *instance, void *notify_data )
{
    struct devquery_notify_data *data = notify_data;

    data->ctx->callback( (HDEVQUERY)data->ctx, data->ctx->user_data, &data->action_data );
    if (data->action_data.Action != DevQueryResultStateChange)
        DevFreeObjectProperties( data->action_data.Data.DeviceObject.cPropertyCount,
                                 data->action_data.Data.DeviceObject.pProperties );
    device_query_context_release( data->ctx );
    free( data );
}

static const char *debugstr_CM_NOTIFY_ACTION( CM_NOTIFY_ACTION action )
{
    static const char *str[] = {
        "CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL",
        "CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL",
        "CM_NOTIFY_ACTION_DEVICEQUERYREMOVE",
        "CM_NOTIFY_ACTION_DEVICEQUERYREMOVEFAILED",
        "CM_NOTIFY_ACTION_DEVICEREMOVEPENDING",
        "CM_NOTIFY_ACTION_DEVICEREMOVECOMPLETE",
        "CM_NOTIFY_ACTION_DEVICECUSTOMEVENT",
        "CM_NOTIFY_ACTION_DEVICEINSTANCEENUMERATED",
        "CM_NOTIFY_ACTION_DEVICEINSTANCESTARTED",
        "CM_NOTIFY_ACTION_DEVICEINSTANCEREMOVED",
    };

    return action < ARRAY_SIZE( str ) ? str[action] : wine_dbg_sprintf( "(unknown %d)", action );
}

static const char *debugstr_CM_NOTIFY_EVENT_DATA( const CM_NOTIFY_EVENT_DATA *event )
{
    if (!event) return wine_dbg_sprintf( "(null)" );
    switch (event->FilterType)
    {
    case CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE:
        return wine_dbg_sprintf( "{CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE %lu {{%s %s}}}", event->Reserved,
                                 debugstr_guid( &event->u.DeviceInterface.ClassGuid ),
                                 debugstr_w( event->u.DeviceInterface.SymbolicLink ) );
    case CM_NOTIFY_FILTER_TYPE_DEVICEHANDLE:
        if (event->u.DeviceHandle.NameOffset == -1)
        {
            return wine_dbg_sprintf( "{CM_NOTIFY_FILTER_TYPE_DEVICEHANDLE %lu {{%s -1 %lu %p}}}", event->Reserved,
                                     debugstr_guid( &event->u.DeviceHandle.EventGuid ),
                                     event->u.DeviceHandle.DataSize, event->u.DeviceHandle.Data );
        }
        return wine_dbg_sprintf( "{CM_NOTIFY_FILTER_TYPE_DEVICEHANDLE %lu {{%s %ld %lu %p %s}}}", event->Reserved,
                                 debugstr_guid( &event->u.DeviceHandle.EventGuid ), event->u.DeviceHandle.NameOffset,
                                 event->u.DeviceHandle.DataSize, event->u.DeviceHandle.Data,
                                 debugstr_w( (WCHAR *)&event->u.DeviceHandle.Data[event->u.DeviceHandle.NameOffset] ) );
    case CM_NOTIFY_FILTER_TYPE_DEVICEINSTANCE:
        return wine_dbg_sprintf( "{CM_NOTIFY_FILTER_TYPE_DEVICEINSTANCE %lu %s}", event->Reserved,
                                 debugstr_w( event->u.DeviceInstance.InstanceId ) );
    default:
        return wine_dbg_sprintf( "{(unknown %d) %lu}", event->FilterType, event->Reserved );
    }
}

static DWORD CALLBACK device_query_context_cm_notify_callback( HCMNOTIFICATION notify, void *data,
                                                               CM_NOTIFY_ACTION action,
                                                               CM_NOTIFY_EVENT_DATA *event, DWORD event_size )
{
    struct device_query_context *ctx = data;
    const ULONG prop_flags = ctx->flags & (DevQueryFlagAllProperties | DevQueryFlagLocalize);
    DEV_QUERY_RESULT_ACTION_DATA *action_data;
    struct devquery_notify_data *notify_data;
    struct device_iface_path *iface_entry;
    const WCHAR *iface_path;
    struct rb_entry *entry;
    HRESULT hr = S_OK;

    TRACE( "(%p, %p, %s, %s, %lu)\n", notify, data, debugstr_CM_NOTIFY_ACTION( action ),
           debugstr_CM_NOTIFY_EVENT_DATA( event ), event_size );

    if (action != CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL && action != CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL)
    {
        FIXME( "Unexpected CM_NOTIFY_ACTION value: %d\n", action );
        return 0;
    }

    iface_path = event->u.DeviceInterface.SymbolicLink;
    EnterCriticalSection( &ctx->cs );
    if (ctx->state == DevQueryStateClosed || ctx->state == DevQueryStateAborted)
    {
        LeaveCriticalSection( &ctx->cs );
        return 0;
    }

    if (!(notify_data = calloc( 1, sizeof ( *notify_data ) )))
        goto abort;
    action_data = &notify_data->action_data;
    notify_data->ctx = ctx;
    action_data->Data.DeviceObject.ObjectType = ctx->type;

    /* Interface removal and arrival for objects that have already been enumerated. */
    if ((entry = rb_get( &ctx->known_ifaces, iface_path )))
    {
        const DEVPROP_BOOLEAN enabled = action == CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL ? DEVPROP_TRUE : DEVPROP_FALSE;
        DEVPROPERTY *prop;

        if (!(prop = calloc( 1, sizeof( *prop ))))
            goto abort;
        if (!(prop->Buffer = calloc( 1, sizeof( enabled ) )))
        {
            free( prop );
            goto abort;
        }
        prop->CompKey.Key = DEVPKEY_DeviceInterface_Enabled;
        prop->Type = DEVPROP_TYPE_BOOLEAN;
        prop->BufferSize = sizeof( enabled );
        memcpy( prop->Buffer, &enabled, sizeof( enabled ) );
        iface_entry =  RB_ENTRY_VALUE( entry, struct device_iface_path, entry );
        action_data->Action = DevQueryResultUpdate;
        action_data->Data.DeviceObject.cPropertyCount = 1;
        action_data->Data.DeviceObject.pProperties = prop;
    }
    else
    {
        if (!(iface_entry = calloc( 1, sizeof( *iface_entry ) )))
            goto abort;
        if (!(iface_entry->path = wcsdup( iface_path )))
        {
            free( iface_entry );
            goto abort;
        }
        rb_put( &ctx->known_ifaces, iface_path, &iface_entry->entry );
        hr = DevGetObjectProperties( ctx->type, iface_path, prop_flags, ctx->prop_keys_len, ctx->prop_keys,
                                     &action_data->Data.DeviceObject.cPropertyCount,
                                     &action_data->Data.DeviceObject.pProperties );
        if (FAILED( hr ))
            goto abort;
        action_data->Action = DevQueryResultAdd;
    }

    action_data->Data.DeviceObject.pszObjectId = iface_entry->path;
    device_query_context_addref( ctx );
    if (!TrySubmitThreadpoolCallback( device_query_notify_dev_async, notify_data, NULL ))
        device_query_context_release( ctx );
    LeaveCriticalSection( &ctx->cs );
    return 0;
abort:
    free( notify_data );
    ctx->state = DevQueryStateAborted;
    device_query_context_addref( ctx );
    if (!TrySubmitThreadpoolCallback( device_query_context_notify_aborted_async, ctx, NULL ))
        device_query_context_release( ctx );
    LeaveCriticalSection( &ctx->cs );
    return 0;
}

static void CALLBACK device_query_enum_objects_async( TP_CALLBACK_INSTANCE *instance, void *data )
{
    struct device_query_context *ctx = data;
    BOOL success;
    HRESULT hr = S_OK;

    if (!ctx->filters)
        hr = enum_dev_objects( ctx->type, ctx->prop_keys_len, ctx->prop_keys, !!(ctx->flags & DevQueryFlagAllProperties),
                               device_query_context_add_object, ctx );

    EnterCriticalSection( &ctx->cs );
    if (ctx->state == DevQueryStateClosed)
        hr = E_CHANGED_STATE;

    switch (hr)
    {
    case S_OK:
        ctx->state = DevQueryStateEnumCompleted;
        success = TrySubmitThreadpoolCallback( device_query_context_notify_enum_completed_async, ctx, NULL );
        if (ctx->filters || !(ctx->flags & DevQueryFlagUpdateResults))
            break;
        switch (ctx->type)
        {
        case DevObjectTypeDeviceInterface:
        case DevObjectTypeDeviceInterfaceDisplay:
        {
            CM_NOTIFY_FILTER filter = { 0 };
            CONFIGRET ret;

            filter.cbSize = sizeof( filter );
            filter.Flags = CM_NOTIFY_FILTER_FLAG_ALL_INTERFACE_CLASSES;
            filter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
            device_query_context_addref( ctx );
            ret = CM_Register_Notification( &filter, ctx, device_query_context_cm_notify_callback, &ctx->notify );
            if (ret)
            {
                ERR( "CM_Register_Notification failed: %lu\n", ret );
                device_query_context_release( ctx );
                goto abort;
            }
            break;
        }
        default:
            FIXME( "Device updates not supported for object type %d\n", ctx->type );
            break;
        }
        break;
    case E_CHANGED_STATE:
        if (!(ctx->flags & DevQueryFlagAsyncClose))
        {
            LeaveCriticalSection( &ctx->cs );
            SetEvent( ctx->closed );
            device_query_context_release( ctx );
            return;
        }
        success = TrySubmitThreadpoolCallback( device_query_context_notify_closed_async, ctx, NULL );
        break;
    default:
        goto abort;
    }

    LeaveCriticalSection( &ctx->cs );
    if (!success)
        device_query_context_release( ctx );
    return;
abort:
    ctx->state = DevQueryStateAborted;
    success = TrySubmitThreadpoolCallback( device_query_context_notify_aborted_async, ctx, NULL );
    LeaveCriticalSection( &ctx->cs );
    if (!success)
        device_query_context_release( ctx );
}

HRESULT WINAPI DevCreateObjectQuery( DEV_OBJECT_TYPE type, ULONG flags, ULONG props_len, const DEVPROPCOMPKEY *props,
                                     ULONG filters_len, const DEVPROP_FILTER_EXPRESSION *filters,
                                     PDEV_QUERY_RESULT_CALLBACK callback, void *user_data, HDEVQUERY *devquery )
{
    TRACE( "(%d, %#lx, %lu, %p, %lu, %p, %p, %p, %p)\n", type, flags, props_len, props, filters_len, filters, callback,
           user_data, devquery );
    return DevCreateObjectQueryEx( type, flags, props_len, props, filters_len, filters, 0, NULL, callback, user_data,
                                   devquery );
}

HRESULT WINAPI DevCreateObjectQueryEx( DEV_OBJECT_TYPE type, ULONG flags, ULONG props_len, const DEVPROPCOMPKEY *props,
                                       ULONG filters_len, const DEVPROP_FILTER_EXPRESSION *filters, ULONG params_len,
                                       const DEV_QUERY_PARAMETER *params, PDEV_QUERY_RESULT_CALLBACK callback,
                                       void *user_data, HDEVQUERY *devquery )
{
    ULONG valid_flags = DevQueryFlagUpdateResults | DevQueryFlagAllProperties | DevQueryFlagLocalize | DevQueryFlagAsyncClose;
    struct device_query_context *ctx = NULL;
    HRESULT hr;

    TRACE( "(%d, %#lx, %lu, %p, %lu, %p, %lu, %p, %p, %p, %p)\n", type, flags, props_len, props, filters_len,
           filters, params_len, params, callback, user_data, devquery );

    if (!!props_len != !!props || !!filters_len != !!filters || !!params_len != !!params || (flags & ~valid_flags) || !callback
        || (props_len && (flags & DevQueryFlagAllProperties)))
        return E_INVALIDARG;
    if (filters)
        FIXME( "Query filters are not supported!\n" );
    if (params)
        FIXME( "Query parameters are not supported!\n" );

    hr = device_query_context_create( &ctx, type, flags, props_len, props, callback, user_data );
    if (FAILED( hr ))
        return hr;

    ctx->filters = !!filters;
    device_query_context_addref( ctx );
    if (!TrySubmitThreadpoolCallback( device_query_enum_objects_async, ctx, NULL ))
        hr = HRESULT_FROM_WIN32( GetLastError() );
    if (FAILED( hr ))
    {
        device_query_context_release( ctx );
        ctx = NULL;
    }

    *devquery = (HDEVQUERY)ctx;
    return hr;
}

void WINAPI DevCloseObjectQuery( HDEVQUERY query )
{
    struct device_query_context *ctx = (struct device_query_context *)query;
    BOOL async = ctx->flags & DevQueryFlagAsyncClose;
    DEV_QUERY_STATE old;

    TRACE( "(%p)\n", query );

    if (!query)
        return;

    EnterCriticalSection( &ctx->cs );
    old = ctx->state;
    ctx->state = DevQueryStateClosed;

    if (ctx->notify)
    {
        CM_Unregister_Notification( ctx->notify );
        device_query_context_release( ctx ); /* Reference held by CM_Register_Notification. */
    }
    if (async && old == DevQueryStateEnumCompleted)
    {
        /* No asynchronous operation involving this query is active, so we need to notify DevQueryStateClosed. */
        BOOL success = TrySubmitThreadpoolCallback( device_query_context_notify_closed_async, ctx, NULL );
        LeaveCriticalSection( &ctx->cs );
        if (success)
            return;
    }
    else if (!async && old == DevQueryStateInitialized)
    {
        LeaveCriticalSection( &ctx->cs );
        /* Wait for the active async operation to end. */
        WaitForSingleObject( ctx->closed, INFINITE );
    }
    else
        LeaveCriticalSection( &ctx->cs );

    device_query_context_release( ctx );
    return;
}

HRESULT WINAPI DevGetObjectProperties( DEV_OBJECT_TYPE type, const WCHAR *id, ULONG flags, ULONG props_len,
                                       const DEVPROPCOMPKEY *props, ULONG *buf_len, const DEVPROPERTY **buf )
{
    TRACE( "(%d, %s, %#lx, %lu, %p, %p, %p)\n", type, debugstr_w( id ), flags, props_len, props, buf_len, buf );
    return DevGetObjectPropertiesEx( type, id, flags, props_len, props, 0, NULL, buf_len, buf );
}

HRESULT WINAPI DevGetObjectPropertiesEx( DEV_OBJECT_TYPE type, const WCHAR *id, ULONG flags, ULONG props_len,
                                         const DEVPROPCOMPKEY *props, ULONG params_len,
                                         const DEV_QUERY_PARAMETER *params, ULONG *buf_len, const DEVPROPERTY **buf )
{
    HRESULT hr;
    ULONG valid_flags = DevQueryFlagAllProperties | DevQueryFlagLocalize;

    TRACE( "(%d, %s, %#lx, %lu, %p, %lu, %p, %p, %p)\n", type, debugstr_w( id ), flags, props_len, props,
           params_len, params, buf_len, buf );

    if (flags & ~valid_flags)
        return E_INVALIDARG;
    if (type == DevObjectTypeUnknown || type > DevObjectTypeAEPProtocol)
        return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
    if (!id || !!props_len != !!props || !!params_len != !!params
        || (props_len && (flags & DevQueryFlagAllProperties)))
        return E_INVALIDARG;

    switch (type)
    {
    case DevObjectTypeDeviceInterface:
    case DevObjectTypeDeviceInterfaceDisplay:
    {
        SP_DEVICE_INTERFACE_DATA iface = {.cbSize = sizeof( iface )};
        DEV_OBJECT obj = {0};
        HDEVINFO set;

        set = SetupDiCreateDeviceInfoListExW( NULL, NULL, NULL, NULL );
        if (set == INVALID_HANDLE_VALUE) return HRESULT_FROM_WIN32( GetLastError() );
        if (!SetupDiOpenDeviceInterfaceW( set, id, 0, &iface ))
        {
            DWORD err = GetLastError();
            SetupDiDestroyDeviceInfoList( set );
            return HRESULT_FROM_WIN32(err == ERROR_NO_SUCH_DEVICE_INTERFACE ? ERROR_FILE_NOT_FOUND : err);
        }

        hr = dev_object_iface_get_props( &obj, set, &iface, props_len, props, !!(flags & DevQueryFlagAllProperties) );
        *buf = obj.pProperties;
        *buf_len = obj.cPropertyCount;
        break;
    }
    default:
        FIXME( "Unsupported DEV_OBJECT_TYPE: %d\n", type );
        hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
    }

    return hr;
}

void WINAPI DevFreeObjectProperties( ULONG len, const DEVPROPERTY *props )
{
    DEVPROPERTY *properties = (DEVPROPERTY *)props;
    ULONG i;

    TRACE( "(%lu, %p)\n", len, props );

    for (i = 0; i < len; i++)
        free( properties[i].Buffer );
    free( properties );
}
