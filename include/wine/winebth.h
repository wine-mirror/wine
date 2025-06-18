/*
 * Wine-specific IOCTL definitions for interfacing with winebth.sys
 *
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

#ifndef __WINEBTH_H__
#define __WINEBTH_H__

/* Set the discoverability or connectable flag for a local radio. Enabling discoverability will also enable incoming
 * connections, while disabling incoming connections disables discoverability as well. */
#define IOCTL_WINEBTH_RADIO_SET_FLAG           CTL_CODE(FILE_DEVICE_BLUETOOTH, 0xa3, METHOD_BUFFERED, FILE_ANY_ACCESS)
/* Start device inquiry for a local radio. */
#define IOCTL_WINEBTH_RADIO_START_DISCOVERY    CTL_CODE(FILE_DEVICE_BLUETOOTH, 0xa6, METHOD_BUFFERED, FILE_ANY_ACCESS)
/* Stop device inquiry for a local radio. */
#define IOCTL_WINEBTH_RADIO_STOP_DISCOVERY     CTL_CODE(FILE_DEVICE_BLUETOOTH, 0xa7, METHOD_BUFFERED, FILE_ANY_ACCESS)
/* Ask the system's Bluetooth service to send all incoming authentication requests to Wine. */
#define IOCTL_WINEBTH_AUTH_REGISTER            CTL_CODE(FILE_DEVICE_BLUETOOTH, 0xa8, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WINEBTH_RADIO_SEND_AUTH_RESPONSE CTL_CODE(FILE_DEVICE_BLUETOOTH, 0xa9, METHOD_BUFFERED, FILE_ANY_ACCESS)
/* Initiate the authentication procedure with a remote device. */
#define IOCTL_WINEBTH_RADIO_START_AUTH         CTL_CODE(FILE_DEVICE_BLUETOOTH, 0xaa, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WINEBTH_RADIO_REMOVE_DEVICE      CTL_CODE(FILE_DEVICE_BLUETOOTH, 0xab, METHOD_BUFFERED, FILE_ANY_ACCESS)

DEFINE_GUID( GUID_WINEBTH_AUTHENTICATION_REQUEST, 0xca67235f, 0xf621, 0x4c27, 0x85, 0x65, 0xa4,
             0xd5, 0x5e, 0xa1, 0x26, 0xe8 );

#define WINEBTH_AUTH_DEVICE_PATH L"\\??\\WINEBTHAUTH"

#pragma pack(push,1)

#define LOCAL_RADIO_DISCOVERABLE 0x0001
#define LOCAL_RADIO_CONNECTABLE  0x0002

struct winebth_radio_set_flag_params
{
    unsigned int flag: 2;
    unsigned int enable : 1;
};

/* Associated data for GUID_WINEBTH_AUTHENTICATION_REQUEST events. */
struct winebth_authentication_request
{
    BTH_DEVICE_INFO device_info;
    BLUETOOTH_AUTHENTICATION_METHOD auth_method;
    ULONG numeric_value_or_passkey;
};

struct winebth_radio_send_auth_response_params
{
    BTH_ADDR address;
    BLUETOOTH_AUTHENTICATION_METHOD method;
    UINT32 numeric_value_or_passkey;
    unsigned int negative : 1;

    unsigned int authenticated : 1;
};

struct winebth_radio_start_auth_params
{
    BTH_ADDR address;
};

#pragma pack(pop)

#endif /* __WINEBTH_H__ */
