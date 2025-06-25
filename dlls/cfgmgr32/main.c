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
#include "winreg.h"
#include "cfgmgr32.h"
#include "winuser.h"
#include "dbt.h"
#include "wine/plugplay.h"
#include "setupapi.h"

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
