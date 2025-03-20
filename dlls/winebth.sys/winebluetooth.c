/*
 * Wine bluetooth APIs
 *
 * Copyright 2024 Vibhav Pant
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

#include <ntstatus.h>
#define WIN32_NO_STATUS

#include <windef.h>
#include <winbase.h>

#include <wine/debug.h>
#include <wine/heap.h>
#include <wine/unixlib.h>

#include "winebth_priv.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL( winebth );

NTSTATUS winebluetooth_radio_get_unique_name( winebluetooth_radio_t radio, char *name,
                                              SIZE_T *size )
{
    struct bluetooth_adapter_get_unique_name_params params = {0};
    NTSTATUS status;

    TRACE( "(%p, %p, %p)\n", (void *)radio.handle, name, size );

    params.adapter = radio.handle;
    params.buf = name;
    params.buf_size = *size;
    status = UNIX_BLUETOOTH_CALL( bluetooth_adapter_get_unique_name, &params );
    if (status == STATUS_BUFFER_TOO_SMALL)
        *size = params.buf_size;
    return status;
}

NTSTATUS winebluetooth_radio_set_property( winebluetooth_radio_t radio,
                                           ULONG prop_flag,
                                           union winebluetooth_property *property )
{
    struct bluetooth_adapter_set_prop_params params = { 0 };

    TRACE( "(%p, %#lx, %p)\n", (void *)radio.handle, prop_flag, property );

    params.adapter = radio.handle;
    params.prop_flag = prop_flag;
    params.prop = property;
    return UNIX_BLUETOOTH_CALL( bluetooth_adapter_set_prop, &params );
}

NTSTATUS winebluetooth_radio_start_discovery( winebluetooth_radio_t radio )
{
    struct bluetooth_adapter_start_discovery_params params = {0};

    TRACE( "(%p)\n", (void *)radio.handle );

    params.adapter = radio.handle;
    return UNIX_BLUETOOTH_CALL( bluetooth_adapter_start_discovery, &params );
}

NTSTATUS winebluetooth_radio_stop_discovery( winebluetooth_radio_t radio )
{
    struct bluetooth_adapter_stop_discovery_params params = {0};

    TRACE( "(%p)\n", (void *)radio.handle );
    params.adapter = radio.handle;
    return UNIX_BLUETOOTH_CALL(bluetooth_adapter_stop_discovery, &params);
}

NTSTATUS winebluetooth_auth_agent_enable_incoming( void )
{
    return UNIX_BLUETOOTH_CALL( bluetooth_auth_agent_enable_incoming, NULL );
}

void winebluetooth_radio_free( winebluetooth_radio_t radio )
{
    struct bluetooth_adapter_free_params args = { 0 };
    TRACE( "(%p)\n", (void *)radio.handle );

    args.adapter = radio.handle;
    UNIX_BLUETOOTH_CALL( bluetooth_adapter_free, &args );
}

void winebluetooth_device_free( winebluetooth_device_t device )
{
    struct bluetooth_device_free_params args = {0};
    TRACE( "(%p)\n", (void *)device.handle );

    args.device = device.handle;
    UNIX_BLUETOOTH_CALL( bluetooth_device_free, &args );
}

void winebluetooth_device_properties_to_info( winebluetooth_device_props_mask_t props_mask,
                                              const struct winebluetooth_device_properties *props,
                                              BTH_DEVICE_INFO *info )
{
    if (props_mask & WINEBLUETOOTH_DEVICE_PROPERTY_NAME)
    {
        info->flags |= BDIF_NAME;
        memcpy( info->name, props->name, sizeof( info->name ) );
    }
    if (props_mask & WINEBLUETOOTH_DEVICE_PROPERTY_ADDRESS)
    {
        info->flags |= BDIF_ADDRESS;
        info->address = RtlUlonglongByteSwap( props->address.ullLong ) >> 16;
    }
    if (props_mask & WINEBLUETOOTH_DEVICE_PROPERTY_CONNECTED && props->connected)
        info->flags |= BDIF_CONNECTED;
    if (props_mask & WINEBLUETOOTH_DEVICE_PROPERTY_PAIRED && props->paired)
        info->flags |= (BDIF_PAIRED | BDIF_PERSONAL);
    if (props_mask & WINEBLUETOOTH_DEVICE_PROPERTY_LEGACY_PAIRING && !props->legacy_pairing)
    {
        info->flags |= BDIF_SSP_SUPPORTED;
        if (props_mask & WINEBLUETOOTH_DEVICE_PROPERTY_PAIRED && props->paired)
            info->flags |= BDIF_SSP_PAIRED;
    }
    if (props_mask & WINEBLUETOOTH_DEVICE_PROPERTY_TRUSTED && props->trusted)
        info->flags |= BDIF_PERSONAL;
    if (props_mask & WINEBLUETOOTH_DEVICE_PROPERTY_CLASS)
    {
        info->classOfDevice = props->class;
        info->flags |= BDIF_COD;
    }
}

NTSTATUS winebluetooth_get_event( struct winebluetooth_event *result )
{
    struct bluetooth_get_event_params params = {0};
    NTSTATUS status;

    TRACE( "(%p)\n", result );

    status = UNIX_BLUETOOTH_CALL( bluetooth_get_event, &params );
    *result = params.result;
    return status;
}

NTSTATUS winebluetooth_init( void )
{
    NTSTATUS status;

    status = __wine_init_unix_call();
    if (status != STATUS_SUCCESS)
        return status;

    return UNIX_BLUETOOTH_CALL( bluetooth_init, NULL );
}

NTSTATUS winebluetooth_shutdown( void )
{
    return UNIX_BLUETOOTH_CALL( bluetooth_shutdown, NULL );
}
