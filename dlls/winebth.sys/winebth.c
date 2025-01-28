/*
 * Bluetooth bus driver
 *
 * Copyright 2024-2025 Vibhav Pant
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

#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winternl.h>
#include <winnls.h>
#include <initguid.h>
#include <devpkey.h>
#include <bthsdpdef.h>
#include <bluetoothapis.h>
#include <bthdef.h>
#include <winioctl.h>
#include <bthioctl.h>
#include <ddk/wdm.h>

#include <wine/winebth.h>
#include <wine/debug.h>
#include <wine/list.h>

#include "winebth_priv.h"

WINE_DEFAULT_DEBUG_CHANNEL( winebth );

static DRIVER_OBJECT *driver_obj;

static DEVICE_OBJECT *bus_fdo, *bus_pdo;

#define DECLARE_CRITICAL_SECTION( cs )                                                             \
    static CRITICAL_SECTION cs;                                                                    \
    static CRITICAL_SECTION_DEBUG cs##_debug = {                                                   \
        0,                                                                                         \
        0,                                                                                         \
        &( cs ),                                                                                   \
        { &cs##_debug.ProcessLocksList, &cs##_debug.ProcessLocksList },                            \
        0,                                                                                         \
        0,                                                                                         \
        { (DWORD_PTR)( __FILE__ ": " #cs ) } };                                                    \
    static CRITICAL_SECTION cs = { &cs##_debug, -1, 0, 0, 0, 0 };

DECLARE_CRITICAL_SECTION( device_list_cs );

static struct list device_list = LIST_INIT( device_list );

struct bluetooth_radio
{
    struct list entry;
    BOOL removed;

    DEVICE_OBJECT *device_obj;
    CRITICAL_SECTION props_cs;
    winebluetooth_radio_props_mask_t props_mask; /* Guarded by props_cs */
    struct winebluetooth_radio_properties props; /* Guarded by props_cs */
    winebluetooth_radio_t radio;
    WCHAR *hw_name;
    UNICODE_STRING bthport_symlink_name;
    UNICODE_STRING bthradio_symlink_name;
};

static NTSTATUS WINAPI dispatch_bluetooth( DEVICE_OBJECT *device, IRP *irp )
{
    struct bluetooth_radio *ext = (struct bluetooth_radio *)device->DeviceExtension;
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    ULONG insize = stack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outsize = stack->Parameters.DeviceIoControl.OutputBufferLength;
    NTSTATUS status = irp->IoStatus.Status;

    TRACE( "device %p irp %p code %#lx\n", device, irp, code );

    switch (code)
    {
    case IOCTL_BTH_GET_LOCAL_INFO:
    {
        BTH_LOCAL_RADIO_INFO *info = (BTH_LOCAL_RADIO_INFO *)irp->AssociatedIrp.SystemBuffer;

        if (!info || outsize < sizeof(*info))
        {
            status = STATUS_INVALID_USER_BUFFER;
            break;
        }

        memset( info, 0, sizeof( *info ) );

        EnterCriticalSection( &ext->props_cs );
        if (ext->props_mask & WINEBLUETOOTH_RADIO_PROPERTY_ADDRESS)
        {
            info->localInfo.flags |= BDIF_ADDRESS;
            info->localInfo.address = RtlUlonglongByteSwap( ext->props.address.ullLong );
        }
        if (ext->props_mask & WINEBLUETOOTH_RADIO_PROPERTY_NAME)
        {
            info->localInfo.flags |= BDIF_NAME;
            strcpy( info->localInfo.name, ext->props.name );
        }
        if (ext->props_mask & WINEBLUETOOTH_RADIO_PROPERTY_CLASS)
        {
            info->localInfo.flags |= BDIF_COD;
            info->localInfo.classOfDevice = ext->props.class;
        }
        if (ext->props_mask & WINEBLUETOOTH_RADIO_PROPERTY_VERSION)
            info->hciVersion = info->radioInfo.lmpVersion = ext->props.version;
        if (ext->props.connectable)
            info->flags |= LOCAL_RADIO_CONNECTABLE;
        if (ext->props.discoverable)
            info->flags |= LOCAL_RADIO_DISCOVERABLE;
        if (ext->props_mask & WINEBLUETOOTH_RADIO_PROPERTY_MANUFACTURER)
            info->radioInfo.mfg = ext->props.manufacturer;
        LeaveCriticalSection( &ext->props_cs );

        irp->IoStatus.Information = sizeof( *info );
        status = STATUS_SUCCESS;
        break;
    }
    case IOCTL_WINEBTH_RADIO_SET_FLAG:
    {
        const struct winebth_radio_set_flag_params *params = irp->AssociatedIrp.SystemBuffer;
        union winebluetooth_property prop_value = {0};

        if (!params)
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        if (insize < sizeof( *params ))
        {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        prop_value.boolean = !!params->enable;
        status = winebluetooth_radio_set_property( ext->radio, params->flag, &prop_value );
        break;
    }
    default:
        FIXME( "Unimplemented IOCTL code: %#lx\n", code );
        break;
    }

    irp->IoStatus.Status = status;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return status;
}

void WINAPIV append_id( struct string_buffer *buffer, const WCHAR *format, ... )
{
    va_list args;
    WCHAR *string;
    int len;

    va_start( args, format );

    len = _vsnwprintf( NULL, 0, format, args ) + 1;
    if (!(string = ExAllocatePool( PagedPool, (buffer->len + len) * sizeof( WCHAR ) )))
    {
        if (buffer->string)
            ExFreePool( buffer->string );
        buffer->string = NULL;
        return;
    }
    if (buffer->string)
    {
        memcpy( string, buffer->string, buffer->len * sizeof( WCHAR ) );
        ExFreePool( buffer->string );
    }
    _vsnwprintf( string + buffer->len, len, format, args );
    buffer->string = string;
    buffer->len += len;

    va_end( args );
}


static HANDLE event_loop_thread;
static NTSTATUS radio_get_hw_name_w( winebluetooth_radio_t radio, WCHAR **name )
{
    char *name_a;
    SIZE_T size = sizeof( char ) *  256;
    NTSTATUS status;

    name_a = malloc( size );
    if (!name_a)
        return STATUS_NO_MEMORY;

    status = winebluetooth_radio_get_unique_name( radio, name_a, &size );
    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        void *ptr = realloc( name_a, size );
        if (!ptr)
        {
            free( name_a );
            return STATUS_NO_MEMORY;
        }
        name_a = ptr;
        status = winebluetooth_radio_get_unique_name( radio, name_a, &size );
    }
    if (status != STATUS_SUCCESS)
    {
        free( name_a );
        return status;
    }

    *name = malloc( (mbstowcs( NULL, name_a, 0 ) + 1) * sizeof( WCHAR ));
    if (!*name)
    {
        free( name_a );
        return status;
    }

    mbstowcs( *name, name_a, strlen( name_a ) + 1 );
    free( name_a );
    return STATUS_SUCCESS;
}
static void add_bluetooth_radio( struct winebluetooth_watcher_event_radio_added event )
{
    struct bluetooth_radio *device;
    DEVICE_OBJECT *device_obj;
    UNICODE_STRING string;
    NTSTATUS status;
    WCHAR name[256];
    WCHAR *hw_name;
    static unsigned int radio_index;

    swprintf( name, ARRAY_SIZE( name ), L"\\Device\\WINEBTH-RADIO-%d", radio_index++ );
    TRACE( "Adding new bluetooth radio %p: %s\n", (void *)event.radio.handle, debugstr_w( name ) );

    status = radio_get_hw_name_w( event.radio, &hw_name );
    if (status)
    {
        ERR( "Failed to get hardware name for radio %p, status %#lx\n", (void *)event.radio.handle, status );
        return;
    }

    RtlInitUnicodeString( &string, name );
    status = IoCreateDevice( driver_obj, sizeof( *device ), &string, FILE_DEVICE_BLUETOOTH, 0,
                             FALSE, &device_obj );
    if (status)
    {
        ERR( "Failed to create device, status %#lx\n", status );
        return;
    }

    device = device_obj->DeviceExtension;
    device->device_obj = device_obj;
    device->radio = event.radio;
    device->removed = FALSE;
    device->hw_name = hw_name;
    device->props = event.props;
    device->props_mask = event.props_mask;

    InitializeCriticalSection( &device->props_cs );

    EnterCriticalSection( &device_list_cs );
    list_add_tail( &device_list, &device->entry );
    LeaveCriticalSection( &device_list_cs );

    IoInvalidateDeviceRelations( bus_pdo, BusRelations );
}

static void remove_bluetooth_radio( winebluetooth_radio_t radio )
{
    struct bluetooth_radio *device;

    EnterCriticalSection( &device_list_cs );
    LIST_FOR_EACH_ENTRY( device, &device_list, struct bluetooth_radio, entry )
    {
        if (winebluetooth_radio_equal( radio, device->radio ) && !device->removed)
        {
            TRACE( "Removing bluetooth radio %p\n", (void *)radio.handle );
            device->removed = TRUE;
            list_remove( &device->entry );
            IoInvalidateDeviceRelations( device->device_obj, BusRelations );
            break;
        }
    }
    LeaveCriticalSection( &device_list_cs );

    IoInvalidateDeviceRelations( bus_pdo, BusRelations );
    winebluetooth_radio_free( radio );
}

static void bluetooth_radio_set_properties( DEVICE_OBJECT *obj,
                                            winebluetooth_radio_props_mask_t mask,
                                            struct winebluetooth_radio_properties *props );

static void update_bluetooth_radio_properties( struct winebluetooth_watcher_event_radio_props_changed event )
{
    struct bluetooth_radio *device;
    winebluetooth_radio_t radio = event.radio;
    winebluetooth_radio_props_mask_t mask = event.changed_props_mask;
    struct winebluetooth_radio_properties props = event.props;

    EnterCriticalSection( &device_list_cs );
    LIST_FOR_EACH_ENTRY( device, &device_list, struct bluetooth_radio, entry )
    {
        if (winebluetooth_radio_equal( radio, device->radio ) && !device->removed)
        {
            EnterCriticalSection( &device->props_cs );
            device->props_mask = mask;
            device->props = props;
            bluetooth_radio_set_properties( device->device_obj, device->props_mask,
                                            &device->props );
            LeaveCriticalSection( &device->props_cs );
            break;
        }
    }
    LeaveCriticalSection( &device_list_cs );
}

static DWORD CALLBACK bluetooth_event_loop_thread_proc( void *arg )
{
    NTSTATUS status;
    while (TRUE)
    {
        struct winebluetooth_event result = {0};

        status = winebluetooth_get_event( &result );
        if (status != STATUS_PENDING) break;

        switch (result.status)
        {
            case WINEBLUETOOTH_EVENT_WATCHER_EVENT:
            {
                struct winebluetooth_watcher_event *event = &result.data.watcher_event;
                switch (event->event_type)
                {
                    case BLUETOOTH_WATCHER_EVENT_TYPE_RADIO_ADDED:
                        add_bluetooth_radio( event->event_data.radio_added );
                        break;
                    case BLUETOOTH_WATCHER_EVENT_TYPE_RADIO_REMOVED:
                        remove_bluetooth_radio( event->event_data.radio_removed );
                        break;
                    case BLUETOOTH_WATCHER_EVENT_TYPE_RADIO_PROPERTIES_CHANGED:
                        update_bluetooth_radio_properties( event->event_data.radio_props_changed );
                        break;
                    default:
                        FIXME( "Unknown bluetooth watcher event code: %#x\n", event->event_type );
                }
                break;
            }
            default:
                FIXME( "Unknown bluetooth event loop status code: %#x\n", result.status );
        }
    }

    if (status != STATUS_SUCCESS)
        ERR( "Bluetooth event loop terminated with %#lx", status );
    else
        TRACE( "Exiting bluetooth event loop\n" );
    return 0;
}

static NTSTATUS WINAPI fdo_pnp( DEVICE_OBJECT *device_obj, IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );

    TRACE( "irp %p, minor function %s.\n", irp, debugstr_minor_function_code( stack->MinorFunction ) );

    switch (stack->MinorFunction)
    {
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            struct bluetooth_radio *radio;
            DEVICE_RELATIONS *devices;
            SIZE_T i = 0;

            if (stack->Parameters.QueryDeviceRelations.Type != BusRelations)
            {
                FIXME( "Unhandled Device Relation %x\n",
                       stack->Parameters.QueryDeviceRelations.Type );
                break;
            }

            EnterCriticalSection( &device_list_cs );
            devices = ExAllocatePool(
                PagedPool, offsetof( DEVICE_RELATIONS, Objects[list_count( &device_list )] ) );
            if (devices == NULL)
            {
                LeaveCriticalSection( &device_list_cs );
                irp->IoStatus.Status = STATUS_NO_MEMORY;
                break;
            }

            LIST_FOR_EACH_ENTRY(radio, &device_list, struct bluetooth_radio, entry)
            {
                devices->Objects[i++] = radio->device_obj;
                call_fastcall_func1( ObfReferenceObject, radio->device_obj );
            }
            LeaveCriticalSection( &device_list_cs );

            devices->Count = i;
            irp->IoStatus.Information = (ULONG_PTR)devices;
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_START_DEVICE:
            event_loop_thread =
                CreateThread( NULL, 0, bluetooth_event_loop_thread_proc, NULL, 0, NULL );
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        case IRP_MN_SURPRISE_REMOVAL:
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        case IRP_MN_REMOVE_DEVICE:
        {
            struct bluetooth_radio *device, *cur;
            NTSTATUS ret;
            winebluetooth_shutdown();
            WaitForSingleObject( event_loop_thread, INFINITE );
            CloseHandle( event_loop_thread );
            EnterCriticalSection( &device_list_cs );
            LIST_FOR_EACH_ENTRY_SAFE( device, cur, &device_list, struct bluetooth_radio, entry )
            {
                assert( !device->removed );
                winebluetooth_radio_free( device->radio );
                list_remove( &device->entry );
                IoDeleteDevice( device->device_obj );
            }
            LeaveCriticalSection( &device_list_cs );
            IoSkipCurrentIrpStackLocation( irp );
            ret = IoCallDriver( bus_pdo, irp );
            IoDetachDevice( bus_pdo );
            IoDeleteDevice( bus_fdo );
            return ret;
        }
        default:
            FIXME( "Unhandled minor function %s.\n", debugstr_minor_function_code( stack->MinorFunction ) );
    }

    IoSkipCurrentIrpStackLocation( irp );
    return IoCallDriver( bus_pdo, irp );
}

static NTSTATUS query_id(const struct bluetooth_radio *ext, IRP *irp, BUS_QUERY_ID_TYPE type )
{
    struct string_buffer buf = {0};

    TRACE( "(%p, %p, %s)\n", ext, irp, debugstr_BUS_QUERY_ID_TYPE( type ) );
    switch (type)
    {
    case BusQueryDeviceID:
        append_id( &buf, L"WINEBTH\\RADIO" );
        break;
    case BusQueryInstanceID:
        append_id( &buf, L"%s", ext->hw_name );
        break;
    case BusQueryHardwareIDs:
    case BusQueryCompatibleIDs:
        append_id( &buf, L"" );
        break;
    default:
        return irp->IoStatus.Status;
    }

    if (!buf.string)
        return STATUS_NO_MEMORY;

    irp->IoStatus.Information = (ULONG_PTR)buf.string;
    return STATUS_SUCCESS;
}

/* Caller must hold props_cs */
static void bluetooth_radio_set_properties( DEVICE_OBJECT *obj,
                                            winebluetooth_radio_props_mask_t mask,
                                            struct winebluetooth_radio_properties *props )
{
    if (mask & WINEBLUETOOTH_RADIO_PROPERTY_ADDRESS)
    {
        BTH_ADDR addr = RtlUlonglongByteSwap( props->address.ullLong );
        IoSetDevicePropertyData( obj, &DEVPKEY_BluetoothRadio_Address, LOCALE_NEUTRAL, 0,
                                 DEVPROP_TYPE_UINT64, sizeof( addr ), &addr );
    }
    if (mask & WINEBLUETOOTH_RADIO_PROPERTY_MANUFACTURER)
    {
        UINT16 manufacturer = props->manufacturer;
        IoSetDevicePropertyData( obj, &DEVPKEY_BluetoothRadio_Manufacturer, LOCALE_NEUTRAL,
                                 0, DEVPROP_TYPE_UINT16, sizeof( manufacturer ), &manufacturer );
    }
    if (mask & WINEBLUETOOTH_RADIO_PROPERTY_NAME)
    {
        WCHAR buf[BLUETOOTH_MAX_NAME_SIZE * sizeof(WCHAR)];
        INT ret;

        if ((ret = MultiByteToWideChar( CP_ACP, 0, props->name, -1, buf, BLUETOOTH_MAX_NAME_SIZE)))
            IoSetDevicePropertyData( obj, &DEVPKEY_NAME, LOCALE_NEUTRAL, 0, DEVPROP_TYPE_STRING, ret, buf );
    }
    if (mask & WINEBLUETOOTH_RADIO_PROPERTY_VERSION)
        IoSetDevicePropertyData( obj, &DEVPKEY_BluetoothRadio_LMPVersion, LOCALE_NEUTRAL, 0, DEVPROP_TYPE_BYTE,
                                 sizeof( props->version ), &props->version );
}

static NTSTATUS WINAPI pdo_pnp( DEVICE_OBJECT *device_obj, IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    struct bluetooth_radio *device = device_obj->DeviceExtension;
    NTSTATUS ret = irp->IoStatus.Status;

    TRACE( "device_obj %p, irp %p, minor function %s\n", device_obj, irp, debugstr_minor_function_code( stack->MinorFunction ) );
    switch (stack->MinorFunction)
    {
        case IRP_MN_QUERY_ID:
            ret = query_id( device, irp, stack->Parameters.QueryId.IdType );
            break;
        case IRP_MN_QUERY_CAPABILITIES:
        {
            DEVICE_CAPABILITIES *caps = stack->Parameters.DeviceCapabilities.Capabilities;
            caps->Removable = TRUE;
            caps->SurpriseRemovalOK = TRUE;
            caps->RawDeviceOK = TRUE;
            ret = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_START_DEVICE:
            EnterCriticalSection( &device->props_cs );
            bluetooth_radio_set_properties( device_obj, device->props_mask, &device->props );
            LeaveCriticalSection( &device->props_cs );

            if (IoRegisterDeviceInterface( device_obj, &GUID_BTHPORT_DEVICE_INTERFACE, NULL,
                                          &device->bthport_symlink_name ) == STATUS_SUCCESS)
                IoSetDeviceInterfaceState( &device->bthport_symlink_name, TRUE );

            if (IoRegisterDeviceInterface( device_obj, &GUID_BLUETOOTH_RADIO_INTERFACE, NULL,
                                          &device->bthradio_symlink_name ) == STATUS_SUCCESS)
                IoSetDeviceInterfaceState( &device->bthradio_symlink_name, TRUE );
            ret = STATUS_SUCCESS;
            break;
        case IRP_MN_REMOVE_DEVICE:
            assert( device->removed );
            if (device->bthport_symlink_name.Buffer)
            {
                IoSetDeviceInterfaceState(&device->bthport_symlink_name, FALSE);
                RtlFreeUnicodeString( &device->bthport_symlink_name );
            }
            if (device->bthradio_symlink_name.Buffer)
            {
                IoSetDeviceInterfaceState(&device->bthradio_symlink_name, FALSE);
                RtlFreeUnicodeString( &device->bthradio_symlink_name );
            }
            free( device->hw_name );
            winebluetooth_radio_free( device->radio );
            IoDeleteDevice( device->device_obj );
            ret = STATUS_SUCCESS;
            break;
        case IRP_MN_SURPRISE_REMOVAL:
            EnterCriticalSection( &device_list_cs );
            if (!device->removed)
            {
                device->removed = TRUE;
                list_remove( &device->entry );
            }
            LeaveCriticalSection( &device_list_cs );
            ret = STATUS_SUCCESS;
            break;
        default:
            FIXME( "Unhandled minor function %s.\n", debugstr_minor_function_code( stack->MinorFunction ) );
            break;
    }

    irp->IoStatus.Status = ret;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return ret;
}

static NTSTATUS WINAPI bluetooth_pnp( DEVICE_OBJECT *device, IRP *irp )
{
    if (device == bus_fdo)
        return fdo_pnp( device, irp );
    return pdo_pnp( device, irp );
}

static NTSTATUS WINAPI driver_add_device( DRIVER_OBJECT *driver, DEVICE_OBJECT *pdo )
{
    NTSTATUS ret;

    TRACE( "(%p, %p)\n", driver, pdo );
    ret = IoCreateDevice( driver, 0, NULL, FILE_DEVICE_BUS_EXTENDER, 0, FALSE, &bus_fdo );
    if (ret != STATUS_SUCCESS)
    {
        ERR( "failed to create FDO: %#lx\n", ret );
        return ret;
    }

    IoAttachDeviceToDeviceStack( bus_fdo, pdo );
    bus_pdo = pdo;
    bus_fdo->Flags &= ~DO_DEVICE_INITIALIZING;
    return STATUS_SUCCESS;
}

static void WINAPI driver_unload( DRIVER_OBJECT *driver ) {}

NTSTATUS WINAPI DriverEntry( DRIVER_OBJECT *driver, UNICODE_STRING *path )
{
    NTSTATUS status;
    TRACE( "(%p, %s)\n", driver, debugstr_w( path->Buffer ) );

    status = winebluetooth_init();
    if (status)
        return status;

    driver_obj = driver;

    driver->DriverExtension->AddDevice = driver_add_device;
    driver->DriverUnload = driver_unload;
    driver->MajorFunction[IRP_MJ_PNP] = bluetooth_pnp;
    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = dispatch_bluetooth;
    return STATUS_SUCCESS;
}
