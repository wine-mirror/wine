/*
 * Bluetooth APIs
 *
 * Copyright 2016 Austin English
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
#include <windef.h>
#include <winbase.h>

#include "wine/debug.h"
#include "bthsdpdef.h"
#include "bluetoothapis.h"

WINE_DEFAULT_DEBUG_CHANNEL(bluetoothapis);

/*********************************************************************
 *  BluetoothFindFirstDevice
 */
HBLUETOOTH_DEVICE_FIND WINAPI BluetoothFindFirstDevice(BLUETOOTH_DEVICE_SEARCH_PARAMS *params,
                                                       BLUETOOTH_DEVICE_INFO *info)
{
    FIXME("(%p %p): stub!\n", params, info);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}

/*********************************************************************
 *  BluetoothFindFirstRadio
 */
HBLUETOOTH_RADIO_FIND WINAPI BluetoothFindFirstRadio(BLUETOOTH_FIND_RADIO_PARAMS *params, HANDLE *radio)
{
    FIXME("(%p %p): stub!\n", params, radio);
    *radio = NULL;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}

/*********************************************************************
 *  BluetoothFindRadioClose
 */
BOOL WINAPI BluetoothFindRadioClose(HBLUETOOTH_RADIO_FIND find)
{
    FIXME("(%p): stub!\n", find);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*********************************************************************
 *  BluetoothFindDeviceClose
 */
BOOL WINAPI BluetoothFindDeviceClose(HBLUETOOTH_DEVICE_FIND find)
{
    FIXME("(%p): stub!\n", find);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*********************************************************************
 *  BluetoothFindNextRadio
 */
BOOL WINAPI BluetoothFindNextRadio(HBLUETOOTH_RADIO_FIND find, HANDLE *radio)
{
    FIXME("(%p, %p): stub!\n", find, radio);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*********************************************************************
 *  BluetoothGetRadioInfo
 */
DWORD WINAPI BluetoothGetRadioInfo(HANDLE radio, PBLUETOOTH_RADIO_INFO info)
{
    FIXME("(%p, %p): stub!\n", radio, info);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/*********************************************************************
 *  BluetoothFindNextDevice
 */
BOOL WINAPI BluetoothFindNextDevice(HBLUETOOTH_DEVICE_FIND find, BLUETOOTH_DEVICE_INFO *info)
{
    FIXME("(%p, %p): stub!\n", find, info);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
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
