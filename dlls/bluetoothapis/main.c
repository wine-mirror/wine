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
#include "bluetoothapis.h"
#include "setupapi.h"
#include "winioctl.h"
#include "wine/winebth.h"

#include "initguid.h"
#include "bthdef.h"
#include "bthioctl.h"

#include "wine/debug.h"

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

/*********************************************************************
 *  BluetoothRegisterForAuthenticationEx
 */
DWORD WINAPI BluetoothRegisterForAuthenticationEx(const BLUETOOTH_DEVICE_INFO *info, HBLUETOOTH_AUTHENTICATION_REGISTRATION *out,
                                                  PFN_AUTHENTICATION_CALLBACK_EX callback, void *param)
{
    FIXME("(%p, %p, %p, %p): stub!\n", info, out, callback, param);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/*********************************************************************
 *  BluetoothUnregisterAuthentication
 */
BOOL WINAPI BluetoothUnregisterAuthentication(HBLUETOOTH_AUTHENTICATION_REGISTRATION handle)
{
    FIXME("(%p): stub!\n", handle);
    if (!handle) SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
}
