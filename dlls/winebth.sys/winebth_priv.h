/*
 * Private winebth.sys defs
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
 */

#ifndef __WINE_WINEBTH_WINEBTH_H_
#define __WINE_WINEBTH_WINEBTH_H_

#include <bthsdpdef.h>
#include <bluetoothapis.h>
#include <bthdef.h>
#include <ddk/wdm.h>

#include <wine/debug.h>

#ifdef __ASM_USE_FASTCALL_WRAPPER
extern void * WINAPI wrap_fastcall_func1(void *func, const void *a);
__ASM_STDCALL_FUNC(wrap_fastcall_func1, 8,
                   "popl %ecx\n\t"
                   "popl %eax\n\t"
                   "xchgl (%esp),%ecx\n\t"
                   "jmp *%eax");
#define call_fastcall_func1(func,a) wrap_fastcall_func1(func,a)
#else
#define call_fastcall_func1(func,a) func(a)
#endif
struct string_buffer
{
    WCHAR *string;
    size_t len;
};

#define XX(i) case (i): return #i

static inline const char *debugstr_BUS_QUERY_ID_TYPE( BUS_QUERY_ID_TYPE type )
{
    switch (type)
    {
        XX(BusQueryDeviceID);
        XX(BusQueryHardwareIDs);
        XX(BusQueryCompatibleIDs);
        XX(BusQueryInstanceID);
        XX(BusQueryDeviceSerialNumber);
        XX(BusQueryContainerID);
    default:
        return wine_dbg_sprintf( "(unknown %d)", type );
    }
}

static inline const char *debugstr_minor_function_code( UCHAR code )
{
    switch(code)
    {
        XX(IRP_MN_START_DEVICE);
        XX(IRP_MN_QUERY_REMOVE_DEVICE);
        XX(IRP_MN_REMOVE_DEVICE);
        XX(IRP_MN_CANCEL_REMOVE_DEVICE);
        XX(IRP_MN_STOP_DEVICE);
        XX(IRP_MN_QUERY_STOP_DEVICE);
        XX(IRP_MN_CANCEL_STOP_DEVICE);
        XX(IRP_MN_QUERY_DEVICE_RELATIONS);
        XX(IRP_MN_QUERY_INTERFACE);
        XX(IRP_MN_QUERY_CAPABILITIES);
        XX(IRP_MN_QUERY_RESOURCES);
        XX(IRP_MN_QUERY_RESOURCE_REQUIREMENTS);
        XX(IRP_MN_QUERY_DEVICE_TEXT);
        XX(IRP_MN_FILTER_RESOURCE_REQUIREMENTS);
        XX(IRP_MN_READ_CONFIG);
        XX(IRP_MN_WRITE_CONFIG);
        XX(IRP_MN_EJECT);
        XX(IRP_MN_SET_LOCK);
        XX(IRP_MN_QUERY_ID);
        XX(IRP_MN_QUERY_PNP_DEVICE_STATE);
        XX(IRP_MN_QUERY_BUS_INFORMATION);
        XX(IRP_MN_DEVICE_USAGE_NOTIFICATION);
        XX(IRP_MN_SURPRISE_REMOVAL);
        XX(IRP_MN_QUERY_LEGACY_BUS_INFORMATION);
        default:
            return wine_dbg_sprintf( "(unknown %#x)", code );
    }
}
#undef XX

/* Undocumented device properties for Bluetooth radios. */
#define DEFINE_BTH_RADIO_DEVPROPKEY( d, i )                                                        \
    DEFINE_DEVPROPKEY( DEVPKEY_BluetoothRadio_##d, 0xa92f26ca, 0xeda7, 0x4b1d, 0x9d, 0xb2, 0x27,   \
                       0xb6, 0x8a, 0xa5, 0xa2, 0xeb, (i) )

DEFINE_BTH_RADIO_DEVPROPKEY( Address, 1 );                         /* DEVPROP_TYPE_UINT64 */
DEFINE_BTH_RADIO_DEVPROPKEY( Manufacturer, 2 );                    /* DEVPROP_TYPE_UINT16 */
DEFINE_BTH_RADIO_DEVPROPKEY( LMPSupportedFeatures, 3 );            /* DEVPROP_TYPE_UINT64 */
DEFINE_BTH_RADIO_DEVPROPKEY( LMPVersion, 4 );                      /* DEVPROP_TYPE_BYTE */
DEFINE_BTH_RADIO_DEVPROPKEY( HCIVendorFeatures, 8 );               /* DEVPROP_TYPE_UINT64 */
DEFINE_BTH_RADIO_DEVPROPKEY( MaximumAdvertisementDataLength, 17 ); /* DEVPROP_TYPE_UINT16 */
DEFINE_BTH_RADIO_DEVPROPKEY( LELocalSupportedFeatures, 22 );       /* DEVPROP_TYPE_UINT64 */

typedef struct
{
    UINT_PTR handle;
} winebluetooth_radio_t;

typedef UINT16 winebluetooth_radio_props_mask_t;

#define WINEBLUETOOTH_RADIO_PROPERTY_NAME         (1)
#define WINEBLUETOOTH_RADIO_PROPERTY_ADDRESS      (1 << 2)
#define WINEBLUETOOTH_RADIO_PROPERTY_DISCOVERABLE (1 << 3)
#define WINEBLUETOOTH_RADIO_PROPERTY_CONNECTABLE  (1 << 4)
#define WINEBLUETOOTH_RADIO_PROPERTY_CLASS        (1 << 5)
#define WINEBLUETOOTH_RADIO_PROPERTY_MANUFACTURER (1 << 6)
#define WINEBLUETOOTH_RADIO_PROPERTY_VERSION      (1 << 7)
#define WINEBLUETOOTH_RADIO_PROPERTY_DISCOVERING  (1 << 8)
#define WINEBLUETOOTH_RADIO_PROPERTY_PAIRABLE     (1 << 9)

#define WINEBLUETOOTH_RADIO_ALL_PROPERTIES                                                         \
    (WINEBLUETOOTH_RADIO_PROPERTY_NAME | WINEBLUETOOTH_RADIO_PROPERTY_ADDRESS |                    \
     WINEBLUETOOTH_RADIO_PROPERTY_DISCOVERABLE | WINEBLUETOOTH_RADIO_PROPERTY_CONNECTABLE |        \
     WINEBLUETOOTH_RADIO_PROPERTY_CLASS | WINEBLUETOOTH_RADIO_PROPERTY_MANUFACTURER |              \
     WINEBLUETOOTH_RADIO_PROPERTY_VERSION | WINEBLUETOOTH_RADIO_PROPERTY_DISCOVERING |             \
     WINEBLUETOOTH_RADIO_PROPERTY_PAIRABLE)

typedef struct
{
    UINT_PTR handle;
} winebluetooth_device_t;

typedef UINT16 winebluetooth_device_props_mask_t;

#define WINEBLUETOOTH_DEVICE_PROPERTY_NAME           (1)
#define WINEBLUETOOTH_DEVICE_PROPERTY_ADDRESS        (1 << 1)
#define WINEBLUETOOTH_DEVICE_PROPERTY_CONNECTED      (1 << 2)
#define WINEBLUETOOTH_DEVICE_PROPERTY_PAIRED         (1 << 3)
#define WINEBLUETOOTH_DEVICE_PROPERTY_LEGACY_PAIRING (1 << 4)
#define WINEBLUETOOTH_DEVICE_PROPERTY_TRUSTED        (1 << 5)
#define WINEBLUETOOTH_DEVICE_PROPERTY_CLASS          (1 << 6)

#define WINEBLUETOOTH_DEVICE_ALL_PROPERTIES                                                 \
    (WINEBLUETOOTH_DEVICE_PROPERTY_NAME | WINEBLUETOOTH_DEVICE_PROPERTY_ADDRESS |           \
     WINEBLUETOOTH_DEVICE_PROPERTY_CONNECTED | WINEBLUETOOTH_DEVICE_PROPERTY_PAIRED |       \
     WINEBLUETOOTH_DEVICE_PROPERTY_LEGACY_PAIRING | WINEBLUETOOTH_DEVICE_PROPERTY_TRUSTED | \
     WINEBLUETOOTH_DEVICE_PROPERTY_CLASS)

union winebluetooth_property
{
    BOOL boolean;
    ULONG ulong;
    BLUETOOTH_ADDRESS address;
    WCHAR name[BLUETOOTH_MAX_NAME_SIZE];
};

struct winebluetooth_radio_properties
{
    BOOL discoverable;
    BOOL connectable;
    BOOL discovering;
    BOOL pairable;
    BLUETOOTH_ADDRESS address;
    CHAR name[BLUETOOTH_MAX_NAME_SIZE];
    ULONG class;
    USHORT manufacturer;
    BYTE version;
};

struct winebluetooth_device_properties
{
    BLUETOOTH_ADDRESS address;
    CHAR name[BLUETOOTH_MAX_NAME_SIZE];
    BOOL connected;
    BOOL paired;
    BOOL legacy_pairing;
    BOOL trusted;
    UINT32 class;
};

NTSTATUS winebluetooth_radio_get_unique_name( winebluetooth_radio_t radio, char *name,
                                              SIZE_T *size );
void winebluetooth_radio_free( winebluetooth_radio_t radio );
static inline BOOL winebluetooth_radio_equal( winebluetooth_radio_t r1, winebluetooth_radio_t r2 )
{
    return r1.handle == r2.handle;
}
NTSTATUS winebluetooth_radio_set_property( winebluetooth_radio_t radio,
                                           ULONG prop_flag,
                                           union winebluetooth_property *property );
NTSTATUS winebluetooth_radio_start_discovery( winebluetooth_radio_t radio );
NTSTATUS winebluetooth_radio_stop_discovery( winebluetooth_radio_t radio );
NTSTATUS winebluetooth_radio_remove_device( winebluetooth_radio_t radio, winebluetooth_device_t device );
NTSTATUS winebluetooth_auth_agent_enable_incoming( void );

void winebluetooth_device_free( winebluetooth_device_t device );
static inline BOOL winebluetooth_device_equal( winebluetooth_device_t d1, winebluetooth_device_t d2 )
{
    return d1.handle == d2.handle;
}
void winebluetooth_device_properties_to_info( winebluetooth_device_props_mask_t props_mask,
                                              const struct winebluetooth_device_properties *props,
                                              BTH_DEVICE_INFO *info );
NTSTATUS winebluetooth_device_disconnect( winebluetooth_device_t device );

NTSTATUS winebluetooth_auth_send_response( winebluetooth_device_t device, BLUETOOTH_AUTHENTICATION_METHOD method,
                                           UINT32 numeric_or_passkey, BOOL negative, BOOL *authenticated );
NTSTATUS winebluetooth_device_start_pairing( winebluetooth_device_t device, IRP *irp );
enum winebluetooth_watcher_event_type
{
    BLUETOOTH_WATCHER_EVENT_TYPE_RADIO_ADDED,
    BLUETOOTH_WATCHER_EVENT_TYPE_RADIO_REMOVED,
    BLUETOOTH_WATCHER_EVENT_TYPE_RADIO_PROPERTIES_CHANGED,
    BLUETOOTH_WATCHER_EVENT_TYPE_DEVICE_ADDED,
    BLUETOOTH_WATCHER_EVENT_TYPE_DEVICE_REMOVED,
    BLUETOOTH_WATCHER_EVENT_TYPE_DEVICE_PROPERTIES_CHANGED,
    BLUETOOTH_WATCHER_EVENT_TYPE_PAIRING_FINISHED,
};

struct winebluetooth_watcher_event_radio_added
{
    winebluetooth_radio_props_mask_t props_mask;
    struct winebluetooth_radio_properties props;
    winebluetooth_radio_t radio;
};

struct winebluetooth_watcher_event_radio_props_changed
{
    winebluetooth_radio_props_mask_t changed_props_mask;
    struct winebluetooth_radio_properties props;

    winebluetooth_radio_props_mask_t invalid_props_mask;
    winebluetooth_radio_t radio;
};

struct winebluetooth_watcher_event_device_added
{
    winebluetooth_device_props_mask_t known_props_mask;
    struct winebluetooth_device_properties props;
    winebluetooth_device_t device;
    winebluetooth_radio_t radio;
    BOOL init_entry;
};

struct winebluetooth_watcher_event_device_props_changed
{
    winebluetooth_device_props_mask_t changed_props_mask;
    struct winebluetooth_device_properties props;

    winebluetooth_device_props_mask_t invalid_props_mask;
    winebluetooth_device_t device;
};

struct winebluetooth_watcher_event_device_removed
{
    winebluetooth_device_t device;
};

struct winebluetooth_watcher_event_pairing_finished
{
    IRP *irp;
    NTSTATUS result;
};

union winebluetooth_watcher_event_data
{
    struct winebluetooth_watcher_event_radio_added radio_added;
    winebluetooth_radio_t radio_removed;
    struct winebluetooth_watcher_event_radio_props_changed radio_props_changed;
    struct winebluetooth_watcher_event_device_added device_added;
    struct winebluetooth_watcher_event_device_removed device_removed;
    struct winebluetooth_watcher_event_device_props_changed device_props_changed;
    struct winebluetooth_watcher_event_pairing_finished pairing_finished;
};

struct winebluetooth_watcher_event
{
    enum winebluetooth_watcher_event_type event_type;
    union winebluetooth_watcher_event_data event_data;
};

enum winebluetooth_event_type
{
    WINEBLUETOOTH_EVENT_WATCHER_EVENT,
    WINEBLUETOOTH_EVENT_AUTH_EVENT
};

struct winebluetooth_auth_event
{
    winebluetooth_device_t device;
    BLUETOOTH_AUTHENTICATION_METHOD method;
    UINT32 numeric_value_or_passkey;
};

struct winebluetooth_event
{
    enum winebluetooth_event_type status;
    union {
        struct winebluetooth_watcher_event watcher_event;
        struct winebluetooth_auth_event auth_event;
    } data;
};

NTSTATUS winebluetooth_get_event( struct winebluetooth_event *result );
NTSTATUS winebluetooth_init( void );
NTSTATUS winebluetooth_shutdown( void );

#endif /* __WINE_WINEBTH_WINEBTH_H_ */
