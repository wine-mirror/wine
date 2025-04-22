/*
 * Bluetooth APIs
 *
 * Copyright 2016 Austin English
 * Copyright 2025 Vibhav Pant
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
 *
 */

#include <stdarg.h>
#include <winternl.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winreg.h>
#include <winnls.h>

#include "bthsdpdef.h"
#include "cfgmgr32.h"
#include "setupapi.h"
#include "winioctl.h"

#include "initguid.h"
#include "bluetoothapis.h"
#include "bthdef.h"
#include "bthioctl.h"
#include "wine/winebth.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(bluetoothapis);

struct bluetooth_find_radio_handle
{
    HDEVINFO devinfo;
    DWORD idx;
};

struct bluetooth_find_device
{
    BLUETOOTH_DEVICE_SEARCH_PARAMS params;
    BTH_DEVICE_INFO_LIST *device_list;
    SIZE_T idx;
};

static const char *debugstr_BLUETOOTH_DEVICE_SEARCH_PARAMS( const BLUETOOTH_DEVICE_SEARCH_PARAMS *params )
{
    if (!params || params->dwSize != sizeof( *params ))
        return wine_dbg_sprintf("%p", params );

    return wine_dbg_sprintf( "{ %ld %d %d %d %d %d %u %p }", params->dwSize, params->fReturnAuthenticated,
                             params->fReturnRemembered, params->fReturnUnknown, params->fReturnConnected,
                             params->fIssueInquiry, params->cTimeoutMultiplier, params->hRadio );
}

static const char *debugstr_addr( const BYTE *addr )
{
    if (!addr)
        return wine_dbg_sprintf( "(null)" );

    return wine_dbg_sprintf( "%02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5] );
}

static BOOL radio_set_inquiry( HANDLE radio, BOOL enable )
{
    DWORD bytes;

    return DeviceIoControl( radio, enable ? IOCTL_WINEBTH_RADIO_START_DISCOVERY : IOCTL_WINEBTH_RADIO_STOP_DISCOVERY,
                            NULL, 0, NULL, 0, &bytes, NULL );
}

static BTH_DEVICE_INFO_LIST *radio_get_devices( HANDLE radio )
{
    DWORD num_devices = 1;

    for (;;)
    {
        BTH_DEVICE_INFO_LIST *list;
        DWORD size, bytes;

        size = sizeof( *list ) + (num_devices - 1) * sizeof( list->deviceList[0] );
        list = calloc( 1, size );
        if (!list)
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return NULL;
        }
        if (!DeviceIoControl( radio, IOCTL_BTH_GET_DEVICE_INFO, NULL, 0, list, size, &bytes, NULL )
            && GetLastError() != ERROR_INVALID_USER_BUFFER)
        {
            free( list );
            return NULL;
        }
        if (!list->numOfDevices)
        {
            free( list );
            SetLastError( ERROR_NO_MORE_ITEMS );
            return NULL;
        }
        if (num_devices != list->numOfDevices)
        {
            /* The buffer is incorrectly sized, try again. */
            num_devices = list->numOfDevices;
            free( list );
            continue;
        }
        return list;
    }
}

static void device_info_from_bth_info( BLUETOOTH_DEVICE_INFO *info, const BTH_DEVICE_INFO *bth_info )
{
    memset( info, 0, sizeof( *info ));
    info->dwSize = sizeof( *info );

    if (bth_info->flags & BDIF_ADDRESS)
        info->Address.ullLong = bth_info->address;
    info->ulClassofDevice = bth_info->classOfDevice;
    info->fConnected = !!(bth_info->flags & BDIF_CONNECTED);
    info->fRemembered = !!(bth_info->flags & BDIF_PERSONAL);
    info->fAuthenticated = !!(bth_info->flags & BDIF_PAIRED);

    if (bth_info->flags & BDIF_NAME)
        MultiByteToWideChar( CP_ACP, 0, bth_info->name, -1, info->szName, ARRAY_SIZE( info->szName ) );
}

static BOOL device_find_next( struct bluetooth_find_device *find, BLUETOOTH_DEVICE_INFO *info )
{
    while (find->idx < find->device_list->numOfDevices)
    {
        const BTH_DEVICE_INFO *bth_info;
        BOOL matches;

        bth_info = &find->device_list->deviceList[find->idx++];
        matches = (find->params.fReturnAuthenticated && bth_info->flags & BDIF_PAIRED) ||
            (find->params.fReturnRemembered && bth_info->flags & BDIF_PERSONAL) ||
            (find->params.fReturnUnknown && !(bth_info->flags & BDIF_PERSONAL)) ||
            (find->params.fReturnConnected && bth_info->flags & BDIF_CONNECTED);

        if (!matches)
            continue;

        device_info_from_bth_info( info, bth_info );
        /* If the user is looking for unknown devices as part of device inquiry, or wants connected devices,
         * we can set stLastSeen to the current UTC time. */
        if ((find->params.fIssueInquiry && find->params.fReturnUnknown && !info->fRemembered)
            || (find->params.fReturnConnected && info->fConnected))
            GetSystemTime( &info->stLastSeen );
        else
            FIXME( "semi-stub!\n" );
        return TRUE;
    }

    return FALSE;
}

/*********************************************************************
 *  BluetoothFindFirstDevice
 */
HBLUETOOTH_DEVICE_FIND WINAPI BluetoothFindFirstDevice( BLUETOOTH_DEVICE_SEARCH_PARAMS *params,
                                                        BLUETOOTH_DEVICE_INFO *info )
{
    struct bluetooth_find_device *hfind;
    BTH_DEVICE_INFO_LIST *device_list;

    TRACE( "(%s %p)\n", debugstr_BLUETOOTH_DEVICE_SEARCH_PARAMS( params ), info );

    if (!params || !info)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    if (params->dwSize != sizeof( *params ) || info->dwSize != sizeof( *info ))
    {
        SetLastError( ERROR_REVISION_MISMATCH );
        return NULL;
    }
    if (params->fIssueInquiry && params->cTimeoutMultiplier > 48)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    if (!params->hRadio)
    {
        /* TODO: Perform device inquiry on all local radios */
        FIXME( "semi-stub!\n" );
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    if (params->fIssueInquiry)
    {
        if (!radio_set_inquiry( params->hRadio, TRUE ))
            return NULL;
        Sleep( params->cTimeoutMultiplier * 1280 );
        radio_set_inquiry( params->hRadio, FALSE );
    }

    if (!(device_list = radio_get_devices( params->hRadio )))
        return NULL;

    hfind = malloc( sizeof( *hfind ) );
    if (!hfind)
    {
        free( device_list );
        SetLastError( ERROR_OUTOFMEMORY );
        return NULL;
    }

    hfind->params = *params;
    hfind->device_list = device_list;
    hfind->idx = 0;
    if (!BluetoothFindNextDevice( hfind, info ))
    {
        free( hfind->device_list );
        free( hfind );
        SetLastError( ERROR_NO_MORE_ITEMS );
        return NULL;
    }
    SetLastError( ERROR_SUCCESS );
    return hfind;
}

/*********************************************************************
 *  BluetoothFindFirstRadio
 */
HBLUETOOTH_RADIO_FIND WINAPI BluetoothFindFirstRadio( BLUETOOTH_FIND_RADIO_PARAMS *params, HANDLE *radio )
{
    struct bluetooth_find_radio_handle *find;
    HANDLE device_ret;
    DWORD err;

    TRACE( "(%p, %p)\n", params, radio );

    if (!params)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    if (params->dwSize != sizeof( *params ))
    {
        SetLastError( ERROR_REVISION_MISMATCH );
        return NULL;
    }
    if (!(find = calloc( 1, sizeof( *find ) )))
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return NULL;
    }

    find->devinfo = SetupDiGetClassDevsW( &GUID_BTHPORT_DEVICE_INTERFACE, NULL, NULL,
                                          DIGCF_PRESENT | DIGCF_DEVICEINTERFACE );
    if (find->devinfo == INVALID_HANDLE_VALUE)
    {
        free( find );
        return NULL;
    }

    if (BluetoothFindNextRadio( find, &device_ret ))
    {
        *radio = device_ret;
        return find;
    }

    err = GetLastError();
    BluetoothFindRadioClose( find );
    SetLastError( err );
    return NULL;
}

/*********************************************************************
 *  BluetoothFindRadioClose
 */
BOOL WINAPI BluetoothFindRadioClose( HBLUETOOTH_RADIO_FIND find_handle )
{
    struct bluetooth_find_radio_handle *find = find_handle;

    TRACE( "(%p)\n", find_handle );

    if (!find_handle)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    SetupDiDestroyDeviceInfoList( find->devinfo );
    free( find );
    SetLastError( ERROR_SUCCESS );
    return TRUE;
}

/*********************************************************************
 *  BluetoothFindDeviceClose
 */
BOOL WINAPI BluetoothFindDeviceClose( HBLUETOOTH_DEVICE_FIND find_handle )
{
    struct bluetooth_find_device *find = find_handle;

    TRACE( "(%p)\n", find_handle );

    if (!find_handle)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    free( find->device_list );
    free( find );
    SetLastError( ERROR_SUCCESS );
    return TRUE;
}

/*********************************************************************
 *  BluetoothFindNextRadio
 */
BOOL WINAPI BluetoothFindNextRadio( HBLUETOOTH_RADIO_FIND find_handle, HANDLE *radio )
{
    char buffer[sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) + MAX_PATH * sizeof( WCHAR )];
    struct bluetooth_find_radio_handle *find = find_handle;
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *iface_detail = (SP_DEVICE_INTERFACE_DETAIL_DATA_W *)buffer;
    SP_DEVICE_INTERFACE_DATA iface_data;
    HANDLE device_ret;
    BOOL found;

    TRACE( "(%p, %p)\n", find_handle, radio );

    if (!find_handle)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    iface_detail->cbSize = sizeof( *iface_detail );
    iface_data.cbSize = sizeof( iface_data );
    found = FALSE;
    while (SetupDiEnumDeviceInterfaces( find->devinfo, NULL, &GUID_BTHPORT_DEVICE_INTERFACE, find->idx++,
                                        &iface_data ))
    {
        if (!SetupDiGetDeviceInterfaceDetailW( find->devinfo, &iface_data, iface_detail, sizeof( buffer ), NULL,
                                               NULL ))
            continue;
        device_ret = CreateFileW( iface_detail->DevicePath, GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );
        if (device_ret != INVALID_HANDLE_VALUE)
        {
            found = TRUE;
            break;
        }
    }

    if (found)
        *radio = device_ret;

    return found;
}

/*********************************************************************
 *  BluetoothGetRadioInfo
 */
DWORD WINAPI BluetoothGetRadioInfo( HANDLE radio, PBLUETOOTH_RADIO_INFO info )
{
    BOOL ret;
    DWORD bytes = 0;
    BTH_LOCAL_RADIO_INFO radio_info = {0};

    TRACE( "(%p, %p)\n", radio, info );

    if (!radio || !info)
        return ERROR_INVALID_PARAMETER;
    if (info->dwSize != sizeof( *info ))
        return ERROR_REVISION_MISMATCH;

    ret = DeviceIoControl( radio, IOCTL_BTH_GET_LOCAL_INFO, NULL, 0, &radio_info, sizeof( radio_info ), &bytes, NULL );
    if (!ret)
        return GetLastError();

    if (radio_info.localInfo.flags & BDIF_ADDRESS)
        info->address.ullLong = RtlUlonglongByteSwap( radio_info.localInfo.address ) >> 16;
    if (radio_info.localInfo.flags & BDIF_COD)
        info->ulClassofDevice = radio_info.localInfo.classOfDevice;
    if (radio_info.localInfo.flags & BDIF_NAME)
        MultiByteToWideChar( CP_ACP, 0, radio_info.localInfo.name, -1, info->szName, ARRAY_SIZE( info->szName ) );

    info->lmpSubversion = radio_info.radioInfo.lmpSubversion;
    info->manufacturer = radio_info.radioInfo.mfg;

    return ERROR_SUCCESS;
}

/*********************************************************************
 *  BluetoothGetDeviceInfo
 */
DWORD WINAPI BluetoothGetDeviceInfo( HANDLE radio, BLUETOOTH_DEVICE_INFO *info )
{
    const static BYTE addr_zero[6];
    BTH_DEVICE_INFO_LIST *devices;
    DWORD i, ret = ERROR_NOT_FOUND;

    TRACE( "(%p, %p)\n", radio, info );

    if (!radio)
        return E_HANDLE;
    if (!info)
        return ERROR_INVALID_PARAMETER;
    if (info->dwSize != sizeof( *info ))
        return ERROR_REVISION_MISMATCH;
    if (!memcmp( info->Address.rgBytes, addr_zero, sizeof( addr_zero ) ))
        return ERROR_NOT_FOUND;

    devices = radio_get_devices( radio );
    if (!devices)
        return GetLastError();
    for (i = 0; i < devices->numOfDevices; i++)
    {
        if (devices->deviceList[i].address == info->Address.ullLong)
        {
            device_info_from_bth_info( info, &devices->deviceList[i] );
            if (info->fConnected)
                GetSystemTime( &info->stLastSeen );
            else
                FIXME( "semi-stub!\n" );
            ret = ERROR_SUCCESS;
            break;
        }
    }

    free( devices );
    return ret;
}

/*********************************************************************
 *  BluetoothIsConnectable
 */
BOOL WINAPI BluetoothIsConnectable( HANDLE radio )
{
    TRACE( "(%p)\n", radio );

    if (!radio)
    {
        BLUETOOTH_FIND_RADIO_PARAMS params = {.dwSize = sizeof( params )};
        HBLUETOOTH_RADIO_FIND find = BluetoothFindFirstRadio( &params, &radio );

        if (!find)
            return FALSE;
        for (;;)
        {
            if (BluetoothIsConnectable( radio ))
            {
                CloseHandle( radio );
                BluetoothFindRadioClose( find );
                return TRUE;
            }

            CloseHandle( radio );
            if (!BluetoothFindNextRadio( find, &radio ))
            {
                BluetoothFindRadioClose( find );
                return FALSE;
            }
        }
    }
    else
    {
        BTH_LOCAL_RADIO_INFO radio_info = {0};
        DWORD bytes;
        DWORD ret;

        ret = DeviceIoControl( radio, IOCTL_BTH_GET_LOCAL_INFO, NULL, 0, &radio_info, sizeof( radio_info ), &bytes,
                               NULL );
        if (!ret)
        {
            ERR( "DeviceIoControl failed: %#lx\n", GetLastError() );
            return FALSE;
        }
        return !!(radio_info.flags & LOCAL_RADIO_CONNECTABLE);
    }
}

/*********************************************************************
 *  BluetoothEnableIncomingConnections
 */
BOOL WINAPI BluetoothEnableIncomingConnections( HANDLE radio, BOOL enable )
{
    TRACE( "(%p, %d)\n", radio, enable );
    if (!radio)
    {
        BLUETOOTH_FIND_RADIO_PARAMS params = {.dwSize = sizeof( params )};
        HBLUETOOTH_RADIO_FIND find = BluetoothFindFirstRadio( &params, &radio );

        if (!find)
            return FALSE;
        for (;;)
        {
            if (BluetoothEnableIncomingConnections( radio, enable ))
            {
                CloseHandle( radio );
                BluetoothFindRadioClose( find );
                return TRUE;
            }

            CloseHandle(radio );
            if (!BluetoothFindNextRadio( find, &radio ))
            {
                BluetoothFindRadioClose( find );
                return FALSE;
            }
        }
    }
    else if (!enable && !BluetoothIsDiscoverable( radio ))
        /* The local radio can only be made non-connectable if it is non-discoverable. */
        return FALSE;
    else
    {
        struct winebth_radio_set_flag_params params = {0};
        BOOL ret;
        DWORD bytes;

        params.flag = LOCAL_RADIO_CONNECTABLE;
        params.enable = !!enable;
        ret = DeviceIoControl( radio, IOCTL_WINEBTH_RADIO_SET_FLAG, &params, sizeof( params ), NULL, 0, &bytes, NULL );
        if (!ret)
            ERR("DeviceIoControl failed: %#lx\n", GetLastError());
        return ret;
    }
}

/*********************************************************************
 *  BluetoothIsDiscoverable
 */
BOOL WINAPI BluetoothIsDiscoverable( HANDLE radio )
{
    TRACE( "(%p)\n", radio );

    if (!radio)
    {
        BLUETOOTH_FIND_RADIO_PARAMS params = {.dwSize = sizeof( params )};
        HBLUETOOTH_RADIO_FIND find = BluetoothFindFirstRadio( &params, &radio );

        if (!find)
            return FALSE;
        for (;;)
        {
            if (BluetoothIsDiscoverable( radio ))
            {
                CloseHandle( radio );
                BluetoothFindRadioClose( find );
                return TRUE;
            }

            CloseHandle( radio );
            if (!BluetoothFindNextRadio( find, &radio ))
            {
                BluetoothFindRadioClose( find );
                return FALSE;
            }
        }
    }
    else
    {
        BTH_LOCAL_RADIO_INFO radio_info = {0};
        DWORD bytes;
        DWORD ret;

        ret = DeviceIoControl( radio, IOCTL_BTH_GET_LOCAL_INFO, NULL, 0, &radio_info, sizeof( radio_info ), &bytes,
                               NULL );
        if (!ret)
        {
            ERR( "DeviceIoControl failed: %#lx\n", GetLastError() );
            return FALSE;
        }
        return !!(radio_info.flags & LOCAL_RADIO_DISCOVERABLE);
    }
}

/*********************************************************************
 *  BluetoothEnableDiscovery
 */
BOOL WINAPI BluetoothEnableDiscovery( HANDLE radio, BOOL enabled )
{
    TRACE( "(%p, %d)\n", radio, enabled );
    if (!radio)
    {
        BLUETOOTH_FIND_RADIO_PARAMS params = {.dwSize = sizeof( params )};
        HBLUETOOTH_RADIO_FIND find = BluetoothFindFirstRadio( &params, &radio );

        if (!find)
            return FALSE;
        for (;;)
        {
            if (BluetoothEnableDiscovery( radio, enabled ))
            {
                CloseHandle( radio );
                BluetoothFindRadioClose( find );
                return TRUE;
            }

            CloseHandle(radio );
            if (!BluetoothFindNextRadio( find, &radio ))
            {
                BluetoothFindRadioClose( find );
                return FALSE;
            }
        }
    }
    else if (enabled && !BluetoothIsConnectable( radio ))
        /* The local radio can only be made discoverable if it is connectable. */
        return FALSE;
    else
    {
        struct winebth_radio_set_flag_params params = {0};
        BOOL ret;
        DWORD bytes;

        params.flag = LOCAL_RADIO_DISCOVERABLE;
        params.enable = !!enabled;
        ret = DeviceIoControl( radio, IOCTL_WINEBTH_RADIO_SET_FLAG, &params, sizeof( params ), NULL, 0, &bytes, NULL );
        if (!ret)
            ERR("DeviceIoControl failed: %#lx\n", GetLastError());
        return ret;
    }
}

/*********************************************************************
 *  BluetoothFindNextDevice
 */
BOOL WINAPI BluetoothFindNextDevice( HBLUETOOTH_DEVICE_FIND find, BLUETOOTH_DEVICE_INFO *info )
{
    BOOL success;

    TRACE( "(%p, %p)\n", find, info );

    /* This method doesn't perform any validation for info, for some reason. */
    if (!find)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    success = device_find_next( find, info );
    if (!success)
        SetLastError( ERROR_NO_MORE_ITEMS );
    else
        SetLastError( ERROR_SUCCESS );
    return success;
}

struct bluetooth_auth_listener
{
    struct list entry;

    unsigned int all_devices : 1;
    BLUETOOTH_ADDRESS addr;

    PFN_AUTHENTICATION_CALLBACK_EX callback;

    void *user_data;
};

static const char *debugstr_bluetooth_auth_listener( const struct bluetooth_auth_listener *listener )
{
    if (!listener)
        return wine_dbg_sprintf( "(null)" );
    if (listener->all_devices)
        return wine_dbg_sprintf( "{ all_devices=1 %p %p }", listener->callback, listener->user_data );
    return wine_dbg_sprintf( "{ all_devices=0 %s %p %p }", debugstr_addr( listener->addr.rgBytes ),
                             listener->callback, listener->user_data );
}

static SRWLOCK bluetooth_auth_lock = SRWLOCK_INIT;
static struct list bluetooth_auth_listeners = LIST_INIT( bluetooth_auth_listeners ); /* Guarded by bluetooth_auth_lock */
static HCMNOTIFICATION bluetooth_auth_event_notify; /* Guarded by bluetooth_auth_lock */

struct auth_callback_data
{
    PFN_AUTHENTICATION_CALLBACK_EX callback;
    struct winebth_authentication_request request;
    void *user_data;
};

static CALLBACK void tp_auth_callback_proc( TP_CALLBACK_INSTANCE *instance, void *ctx )
{
    BLUETOOTH_AUTHENTICATION_CALLBACK_PARAMS params;
    struct auth_callback_data *data = ctx;

    device_info_from_bth_info( &params.deviceInfo, &data->request.device_info );
    /* For some reason, Windows uses the *local* time here. */
    GetLocalTime( &params.deviceInfo.stLastSeen );
    GetLocalTime( &params.deviceInfo.stLastUsed );
    params.authenticationMethod = data->request.auth_method;
    params.ioCapability = BLUETOOTH_IO_CAPABILITY_UNDEFINED;
    params.authenticationRequirements = BLUETOOTH_MITM_ProtectionNotDefined;
    params.Numeric_Value = data->request.numeric_value_or_passkey;
    data->callback( data->user_data, &params );
    free( data );
}

static CALLBACK DWORD bluetooth_auth_event_callback( HCMNOTIFICATION notify, void *ctx, CM_NOTIFY_ACTION action,
                                                     CM_NOTIFY_EVENT_DATA *event_data, DWORD size )
{
    struct winebth_authentication_request *request = (struct winebth_authentication_request *)event_data->u.DeviceHandle.Data;
    struct bluetooth_auth_listener *listener;

    TRACE( "(%p, %p, %d, %p, %lu)\n", notify, ctx, action, event_data, size );

    switch (action)
    {
    case CM_NOTIFY_ACTION_DEVICECUSTOMEVENT:
        if (!IsEqualGUID( &event_data->u.DeviceHandle.EventGuid, &GUID_WINEBTH_AUTHENTICATION_REQUEST ))
        {
            FIXME( "Unexpected EventGUID: %s\n", debugstr_guid(&event_data->u.DeviceHandle.EventGuid) );
            break;
        }

        AcquireSRWLockShared( &bluetooth_auth_lock );
        LIST_FOR_EACH_ENTRY( listener, &bluetooth_auth_listeners, struct bluetooth_auth_listener, entry )
        {
            struct auth_callback_data *data;

            if (!(listener->all_devices || listener->addr.ullLong == request->device_info.address))
                continue;
            data = calloc( 1, sizeof( *data ) );
            if (!data)
                continue;

            data->request = *request;
            data->callback = listener->callback;
            data->user_data = listener->user_data;
            if (!TrySubmitThreadpoolCallback( tp_auth_callback_proc, data, NULL ))
            {
                ERR( "TrySubmitThreadpoolCallback failed: %lu\n", GetLastError() );
                free( data );
                continue;
            }
        }
        ReleaseSRWLockShared( &bluetooth_auth_lock );
        break;
    default:
        FIXME( "Unexpected CM_NOTIFY_ACTION: %d\n", action );
        break;
    }

    return 0;
}

static DWORD bluetooth_auth_register( void )
{
    DWORD ret;
    CM_NOTIFY_FILTER handle_filter = {0};
    HANDLE winebth_auth;
    DWORD bytes;


    winebth_auth = CreateFileW( WINEBTH_AUTH_DEVICE_PATH, GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );
    if (winebth_auth == INVALID_HANDLE_VALUE)
        return GetLastError();

    handle_filter.cbSize = sizeof( handle_filter );
    handle_filter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEHANDLE;
    handle_filter.u.DeviceHandle.hTarget = winebth_auth;

    ret = CM_Register_Notification( &handle_filter, NULL, bluetooth_auth_event_callback,
                                    &bluetooth_auth_event_notify );
    if (ret)
    {
        ERR( "CM_Register_Notification failed: %#lx\n", ret );
        CloseHandle( winebth_auth );
        return CM_MapCrToWin32Err( ret, ERROR_INTERNAL_ERROR );
    }

    if (!DeviceIoControl( winebth_auth, IOCTL_WINEBTH_AUTH_REGISTER, NULL, 0, NULL, 0, &bytes, NULL ))
        ret = GetLastError();

    CloseHandle( winebth_auth );
    return ret;
}

/*********************************************************************
 *  BluetoothRegisterForAuthenticationEx
 */
DWORD WINAPI BluetoothRegisterForAuthenticationEx( const BLUETOOTH_DEVICE_INFO *device_info,
                                                   HBLUETOOTH_AUTHENTICATION_REGISTRATION *out,
                                                   PFN_AUTHENTICATION_CALLBACK_EX callback, void *param )
{
    DWORD ret;
    struct bluetooth_auth_listener *listener;

    TRACE( "(%p, %p, %p, %p)\n", device_info, out, callback, param );

    if (!out || (device_info && device_info->dwSize != sizeof ( *device_info )) || !callback)
        return ERROR_INVALID_PARAMETER;

    listener = calloc( 1, sizeof( *listener ) );
    if (!listener)
        return ERROR_OUTOFMEMORY;
    listener->all_devices = !device_info;
    if (!listener->all_devices)
    {
        TRACE( "Registering for authentication events from %s\n", debugstr_addr( device_info->Address.rgBytes ) );
        listener->addr = device_info->Address;
    }
    listener->callback = callback;
    listener->user_data = param;

    AcquireSRWLockExclusive( &bluetooth_auth_lock );
    if (list_empty( &bluetooth_auth_listeners ))
    {
        ret = bluetooth_auth_register();
        if (ret)
        {
            free( listener );
            ReleaseSRWLockExclusive( &bluetooth_auth_lock );
            return ret;
        }
    }
    /* The MSDN documentation for BluetoothRegisterForAuthentication states if applications call this method
     * multiple times, only the first callback registered is invoked during an authentication session.
     * This is incorrect, at least for Windows 11, where all registered callbacks do get called. */
    list_add_tail( &bluetooth_auth_listeners, &listener->entry );
    ReleaseSRWLockExclusive( &bluetooth_auth_lock );

    *out = listener;
    ret = ERROR_SUCCESS;
    return ret;
}

/*********************************************************************
 *  BluetoothUnregisterAuthentication
 */
BOOL WINAPI BluetoothUnregisterAuthentication( HBLUETOOTH_AUTHENTICATION_REGISTRATION handle )
{
    struct bluetooth_auth_listener *listener = handle;
    DWORD ret = ERROR_SUCCESS;

    TRACE( "(%s)\n", debugstr_bluetooth_auth_listener( handle ) );

    if (!handle)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    AcquireSRWLockExclusive( &bluetooth_auth_lock );
    list_remove( &listener->entry );
    if (list_empty( &bluetooth_auth_listeners ))
    {
        ret = CM_Unregister_Notification( bluetooth_auth_event_notify );
        if (ret)
            ERR( "CM_Unregister_Notification failed: %#lx\n", ret );
    }
    ReleaseSRWLockExclusive( &bluetooth_auth_lock );

    SetLastError( ret );
    return !ret;
}

static const char *debugstr_BLUETOOTH_AUTHENTICATE_RESPONSE( const BLUETOOTH_AUTHENTICATE_RESPONSE *resp )
{
    if (!resp)
        return wine_dbg_sprintf("(null)");
    return wine_dbg_sprintf("{ bthAddressRemote: %s, authMethod: %d, negativeResponse: %d }",
                            debugstr_addr( resp->bthAddressRemote.rgBytes ), resp->authMethod, resp->negativeResponse );
}

DWORD WINAPI BluetoothSendAuthenticationResponseEx( HANDLE handle_radio, BLUETOOTH_AUTHENTICATE_RESPONSE *auth_response )
{
    struct winebth_radio_send_auth_response_params params = {0};
    DWORD ret, bytes;

    TRACE( "(%p, %s)\n", handle_radio, debugstr_BLUETOOTH_AUTHENTICATE_RESPONSE( auth_response ) );

    if (!auth_response)
        return ERROR_INVALID_PARAMETER;
    switch (auth_response->authMethod)
    {
    case BLUETOOTH_AUTHENTICATION_METHOD_NUMERIC_COMPARISON:
        break;
    case BLUETOOTH_AUTHENTICATION_METHOD_LEGACY:
    case BLUETOOTH_AUTHENTICATION_METHOD_OOB:
    case BLUETOOTH_AUTHENTICATION_METHOD_PASSKEY_NOTIFICATION:
    case BLUETOOTH_AUTHENTICATION_METHOD_PASSKEY:
        FIXME( "Unsupported authMethod: %d\n", auth_response->authMethod );
        return ERROR_CALL_NOT_IMPLEMENTED;
    default:
        return ERROR_INVALID_PARAMETER;
    }
    if (!handle_radio)
    {
        HBLUETOOTH_RADIO_FIND radio_find;
        BLUETOOTH_FIND_RADIO_PARAMS find_params = {.dwSize = sizeof( find_params ) };

        radio_find = BluetoothFindFirstRadio( &find_params, &handle_radio );
        if (!radio_find)
            return GetLastError();
        for(;;)
        {
            ret = BluetoothSendAuthenticationResponseEx( handle_radio, auth_response );
            CloseHandle( handle_radio );
            if (!ret)
            {
                BluetoothFindRadioClose( radio_find );
                return ERROR_SUCCESS;
            }

            if (!BluetoothFindNextRadio( radio_find, &handle_radio ))
            {
                BluetoothFindRadioClose( radio_find );
                return ret;
            }
        }
    }

    AcquireSRWLockShared( &bluetooth_auth_lock );
    if (list_empty( &bluetooth_auth_listeners ))
    {
        ret = ERROR_INVALID_PARAMETER;
        goto done;
    }
    params.address = RtlUlonglongByteSwap( auth_response->bthAddressRemote.ullLong ) >> 16;
    params.method = auth_response->authMethod;
    params.numeric_value_or_passkey = auth_response->numericCompInfo.NumericValue;
    params.negative = !!auth_response->negativeResponse;

    if (!DeviceIoControl( handle_radio, IOCTL_WINEBTH_RADIO_SEND_AUTH_RESPONSE, &params, sizeof( params ),
                          &params, sizeof( params ), &bytes, NULL ))
    {
        ret = GetLastError();
        goto done;
    }
    ret = params.authenticated ? ERROR_SUCCESS : ERROR_NOT_AUTHENTICATED;

 done:
    ReleaseSRWLockShared( &bluetooth_auth_lock );
    return ret;
 }

DWORD WINAPI BluetoothAuthenticateDeviceEx( HWND parent, HANDLE handle_radio, BLUETOOTH_DEVICE_INFO *device_info,
                                            BLUETOOTH_OOB_DATA_INFO *oob_info,
                                            AUTHENTICATION_REQUIREMENTS auth_req )
{
    OVERLAPPED ovl = {0};
    BTH_ADDR device_addr;
    BOOL success;
    DWORD ret, bytes;

    FIXME( "(%p, %p, %p, %p, %d): semi-stub!\n", parent, handle_radio, device_info, oob_info, auth_req );

    if (!device_info || auth_req < MITMProtectionNotRequired || auth_req > MITMProtectionNotDefined)
        return ERROR_INVALID_PARAMETER;
    if (device_info->dwSize != sizeof( *device_info ))
        return ERROR_REVISION_MISMATCH;

    TRACE( "Initiating pairing with %s\n", debugstr_addr( device_info->Address.rgBytes ) );
    if (!handle_radio)
    {
        BLUETOOTH_FIND_RADIO_PARAMS find_params = {.dwSize = sizeof( find_params )};
        HBLUETOOTH_RADIO_FIND radio_find;

        radio_find = BluetoothFindFirstRadio( &find_params, &handle_radio );
        ret = GetLastError();
        if (!radio_find)
            return ret == ERROR_NO_MORE_ITEMS ? ERROR_NOT_FOUND : ret;
        do {
            ret = BluetoothAuthenticateDeviceEx( parent, handle_radio, device_info, oob_info, auth_req );
            CloseHandle( handle_radio );
            if (!ret || ret == ERROR_NO_MORE_ITEMS)
                break;
        } while (BluetoothFindNextRadio( radio_find, &handle_radio ));

        BluetoothFindRadioClose( radio_find );
        return ret;
    }

    ovl.hEvent = CreateEventW( NULL, TRUE, FALSE, NULL );
    device_addr = RtlUlonglongByteSwap( device_info->Address.ullLong ) >> 16;
    success = DeviceIoControl( handle_radio, IOCTL_WINEBTH_RADIO_START_AUTH, &device_addr, sizeof( device_addr ),
                               NULL, 0, &bytes, &ovl );
    ret = ERROR_SUCCESS;
    if (!success)
    {
        ret = GetLastError();
        if (ret == ERROR_IO_PENDING)
            ret = GetOverlappedResult( handle_radio, &ovl, &bytes, TRUE );
        CloseHandle( ovl.hEvent );
    }

    return ret;
}

DWORD WINAPI BluetoothRemoveDevice( BLUETOOTH_ADDRESS *addr )
{
    BLUETOOTH_FIND_RADIO_PARAMS find_params = {0};
    HBLUETOOTH_RADIO_FIND radio_find;
    const static BYTE addr_zero[6];
    BOOL success = FALSE;
    HANDLE radio;
    DWORD ret;

    TRACE( "(%s)\n", debugstr_addr( (BYTE *)addr ) );

    if (!memcmp( addr_zero, &addr->rgBytes, sizeof( addr_zero ) ))
        return ERROR_NOT_FOUND;

    find_params.dwSize = sizeof( find_params );
    radio_find = BluetoothFindFirstRadio( &find_params, &radio );
    if (!radio_find)
    {
        ret = GetLastError();
        return ret == ERROR_NO_MORE_ITEMS ? ERROR_NOT_FOUND : ret;
    }

    do {
        DWORD bytes;
        success |= DeviceIoControl( radio, IOCTL_WINEBTH_RADIO_REMOVE_DEVICE, addr, sizeof( *addr ), NULL, 0, &bytes, NULL );
        CloseHandle( radio );
    } while (BluetoothFindNextRadio( radio_find, &radio ));

    BluetoothFindRadioClose( radio_find );
    return success ? ERROR_SUCCESS : ERROR_NOT_FOUND;
}
