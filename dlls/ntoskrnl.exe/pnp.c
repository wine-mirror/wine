/*
 * Plug and Play
 *
 * Copyright 2016 Sebastian Lackner
 * Copyright 2016 Aric Stewart for CodeWeavers
 * Copyright 2019 Zebediah Figura
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

#include "ntoskrnl_private.h"
#include "winreg.h"
#include "winuser.h"
#include "setupapi.h"
#include "cfgmgr32.h"
#include "dbt.h"
#include "wine/exception.h"
#include "wine/heap.h"

#include "plugplay.h"

#include "initguid.h"
DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

WINE_DEFAULT_DEBUG_CHANNEL(plugplay);

DECLARE_CRITICAL_SECTION(invalidated_devices_cs);
static CONDITION_VARIABLE invalidated_devices_cv = CONDITION_VARIABLE_INIT;

static DEVICE_OBJECT **invalidated_devices;
static size_t invalidated_devices_count;

static inline const char *debugstr_propkey( const DEVPROPKEY *id )
{
    if (!id) return "(null)";
    return wine_dbg_sprintf( "{%s,%04lx}", wine_dbgstr_guid( &id->fmtid ), id->pid );
}

#define MAX_SERVICE_NAME 260

struct device_interface
{
    struct wine_rb_entry entry;

    UNICODE_STRING symbolic_link;
    DEVICE_OBJECT *device;
    GUID interface_class;
    BOOL enabled;
};

static int interface_rb_compare( const void *key, const struct wine_rb_entry *entry)
{
    const struct device_interface *iface = WINE_RB_ENTRY_VALUE( entry, const struct device_interface, entry );
    const UNICODE_STRING *k = key;

    return RtlCompareUnicodeString( k, &iface->symbolic_link, FALSE );
}

static struct wine_rb_tree device_interfaces = { interface_rb_compare };

static NTSTATUS get_device_id( DEVICE_OBJECT *device, BUS_QUERY_ID_TYPE type, WCHAR **id )
{
    IO_STACK_LOCATION *irpsp;
    IO_STATUS_BLOCK irp_status;
    KEVENT event;
    IRP *irp;

    device = IoGetAttachedDevice( device );

    KeInitializeEvent( &event, NotificationEvent, FALSE );
    if (!(irp = IoBuildSynchronousFsdRequest( IRP_MJ_PNP, device, NULL, 0, NULL, &event, &irp_status )))
        return STATUS_NO_MEMORY;

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->MinorFunction = IRP_MN_QUERY_ID;
    irpsp->Parameters.QueryId.IdType = type;

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    if (IoCallDriver( device, irp ) == STATUS_PENDING)
        KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );

    *id = (WCHAR *)irp_status.Information;
    return irp_status.Status;
}

static NTSTATUS send_pnp_irp( DEVICE_OBJECT *device, UCHAR minor )
{
    IO_STACK_LOCATION *irpsp;
    IO_STATUS_BLOCK irp_status;
    KEVENT event;
    IRP *irp;

    device = IoGetAttachedDevice( device );

    KeInitializeEvent( &event, NotificationEvent, FALSE );
    if (!(irp = IoBuildSynchronousFsdRequest( IRP_MJ_PNP, device, NULL, 0, NULL, NULL, &irp_status )))
        return STATUS_NO_MEMORY;

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->MinorFunction = minor;

    irpsp->Parameters.StartDevice.AllocatedResources = NULL;
    irpsp->Parameters.StartDevice.AllocatedResourcesTranslated = NULL;

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    if (IoCallDriver( device, irp ) == STATUS_PENDING)
        KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );

    return irp_status.Status;
}

static NTSTATUS get_device_instance_id( DEVICE_OBJECT *device, WCHAR *buffer )
{
    static const WCHAR backslashW[] = {'\\',0};
    NTSTATUS status;
    WCHAR *id;

    if ((status = get_device_id( device, BusQueryDeviceID, &id )))
    {
        ERR("Failed to get device ID, status %#lx.\n", status);
        return status;
    }

    lstrcpyW( buffer, id );
    ExFreePool( id );

    if ((status = get_device_id( device, BusQueryInstanceID, &id )))
    {
        ERR("Failed to get instance ID, status %#lx.\n", status);
        return status;
    }

    lstrcatW( buffer, backslashW );
    lstrcatW( buffer, id );
    ExFreePool( id );

    TRACE("Returning ID %s.\n", debugstr_w(buffer));

    return STATUS_SUCCESS;
}

static NTSTATUS get_device_caps( DEVICE_OBJECT *device, DEVICE_CAPABILITIES *caps )
{
    IO_STACK_LOCATION *irpsp;
    IO_STATUS_BLOCK irp_status;
    KEVENT event;
    IRP *irp;

    memset( caps, 0, sizeof(*caps) );
    caps->Size = sizeof(*caps);
    caps->Version = 1;
    caps->Address = 0xffffffff;
    caps->UINumber = 0xffffffff;

    device = IoGetAttachedDevice( device );

    KeInitializeEvent( &event, NotificationEvent, FALSE );
    if (!(irp = IoBuildSynchronousFsdRequest( IRP_MJ_PNP, device, NULL, 0, NULL, NULL, &irp_status )))
        return STATUS_NO_MEMORY;

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->MinorFunction = IRP_MN_QUERY_CAPABILITIES;
    irpsp->Parameters.DeviceCapabilities.Capabilities = caps;

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    if (IoCallDriver( device, irp ) == STATUS_PENDING)
        KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );

    return irp_status.Status;
}

static void load_function_driver( DEVICE_OBJECT *device, HDEVINFO set, SP_DEVINFO_DATA *sp_device )
{
    static const WCHAR driverW[] = {'\\','D','r','i','v','e','r','\\',0};
    WCHAR buffer[MAX_SERVICE_NAME + ARRAY_SIZE(servicesW)];
    WCHAR driver[MAX_SERVICE_NAME] = {0};
    DRIVER_OBJECT *driver_obj;
    UNICODE_STRING string;
    NTSTATUS status;

    if (!SetupDiGetDeviceRegistryPropertyW( set, sp_device, SPDRP_SERVICE,
            NULL, (BYTE *)driver, sizeof(driver), NULL ))
    {
        WARN("No driver registered for device %p.\n", device);
        return;
    }

    lstrcpyW( buffer, servicesW );
    lstrcatW( buffer, driver );
    RtlInitUnicodeString( &string, buffer );
    status = ZwLoadDriver( &string );
    if (status != STATUS_SUCCESS && status != STATUS_IMAGE_ALREADY_LOADED)
    {
        ERR("Failed to load driver %s, status %#lx.\n", debugstr_w(driver), status);
        return;
    }

    lstrcpyW( buffer, driverW );
    lstrcatW( buffer, driver );
    RtlInitUnicodeString( &string, buffer );
    if (ObReferenceObjectByName( &string, OBJ_CASE_INSENSITIVE, NULL,
                                 0, NULL, KernelMode, NULL, (void **)&driver_obj ) != STATUS_SUCCESS)
    {
        ERR("Failed to locate loaded driver %s.\n", debugstr_w(driver));
        return;
    }

    TRACE("Calling AddDevice routine %p.\n", driver_obj->DriverExtension->AddDevice);
    if (driver_obj->DriverExtension->AddDevice)
        status = driver_obj->DriverExtension->AddDevice( driver_obj, device );
    else
        status = STATUS_NOT_IMPLEMENTED;
    TRACE("AddDevice routine %p returned %#lx.\n", driver_obj->DriverExtension->AddDevice, status);

    ObDereferenceObject( driver_obj );

    if (status != STATUS_SUCCESS)
        ERR("AddDevice failed for driver %s, status %#lx.\n", debugstr_w(driver), status);
}

/* Return the total number of characters in a REG_MULTI_SZ string, including
 * the final terminating null. */
static size_t sizeof_multiszW( const WCHAR *str )
{
    const WCHAR *p;
    for (p = str; *p; p += lstrlenW(p) + 1);
    return p + 1 - str;
}

/* This does almost the same thing as UpdateDriverForPlugAndPlayDevices(),
 * except that we don't know the INF path beforehand. */
static BOOL install_device_driver( DEVICE_OBJECT *device, HDEVINFO set, SP_DEVINFO_DATA *sp_device )
{
    static const DWORD dif_list[] =
    {
        DIF_REGISTERDEVICE,
        DIF_SELECTBESTCOMPATDRV,
        DIF_ALLOW_INSTALL,
        DIF_INSTALLDEVICEFILES,
        DIF_REGISTER_COINSTALLERS,
        DIF_INSTALLINTERFACES,
        DIF_INSTALLDEVICE,
        DIF_NEWDEVICEWIZARD_FINISHINSTALL,
    };
    static const DWORD config_flags = 0;

    NTSTATUS status;
    unsigned int i;
    WCHAR *ids;

    if ((status = get_device_id( device, BusQueryHardwareIDs, &ids )) || !ids)
    {
        ERR("Failed to get hardware IDs, status %#lx.\n", status);
        return FALSE;
    }

    SetupDiSetDeviceRegistryPropertyW( set, sp_device, SPDRP_HARDWAREID, (BYTE *)ids,
            sizeof_multiszW( ids ) * sizeof(WCHAR) );
    ExFreePool( ids );

    if ((status = get_device_id( device, BusQueryCompatibleIDs, &ids )) || !ids)
    {
        ERR("Failed to get compatible IDs, status %#lx.\n", status);
        return FALSE;
    }

    SetupDiSetDeviceRegistryPropertyW( set, sp_device, SPDRP_COMPATIBLEIDS, (BYTE *)ids,
            sizeof_multiszW( ids ) * sizeof(WCHAR) );
    ExFreePool( ids );

    /* Set the config flags. setupapi won't do this for us if we couldn't find
     * a driver to install, but raw devices should still have this key
     * populated. */

    if (!SetupDiSetDeviceRegistryPropertyW( set, sp_device, SPDRP_CONFIGFLAGS,
            (BYTE *)&config_flags, sizeof(config_flags) ))
        ERR("Failed to set config flags, error %#lx.\n", GetLastError());

    if (!SetupDiBuildDriverInfoList( set, sp_device, SPDIT_COMPATDRIVER ))
    {
        ERR("Failed to build compatible driver list, error %#lx.\n", GetLastError());
        return FALSE;
    }

    for (i = 0; i < ARRAY_SIZE(dif_list); ++i)
    {
        if (!SetupDiCallClassInstaller(dif_list[i], set, sp_device) && GetLastError() != ERROR_DI_DO_DEFAULT)
        {
            WARN("Install function %#lx failed, error %#lx.\n", dif_list[i], GetLastError());
            return FALSE;
        }
    }

    return TRUE;
}

/* Load the function driver for a newly created PDO, if one is present, and
 * send IRPs to start the device. */
static void start_device( DEVICE_OBJECT *device, HDEVINFO set, SP_DEVINFO_DATA *sp_device )
{
    load_function_driver( device, set, sp_device );
    if (device->DriverObject)
        send_pnp_irp( device, IRP_MN_START_DEVICE );
}

static void enumerate_new_device( DEVICE_OBJECT *device, HDEVINFO set )
{
    static const WCHAR infpathW[] = {'I','n','f','P','a','t','h',0};

    SP_DEVINFO_DATA sp_device = {sizeof(sp_device)};
    WCHAR device_instance_id[MAX_DEVICE_ID_LEN];
    DEVICE_CAPABILITIES caps;
    BOOL need_driver = TRUE;
    NTSTATUS status;
    HKEY key;
    WCHAR *id;

    if (get_device_instance_id( device, device_instance_id ))
        return;

    if (!SetupDiCreateDeviceInfoW( set, device_instance_id, &GUID_NULL, NULL, NULL, 0, &sp_device )
            && !SetupDiOpenDeviceInfoW( set, device_instance_id, NULL, 0, &sp_device ))
    {
        ERR("Failed to create or open device %s, error %#lx.\n", debugstr_w(device_instance_id), GetLastError());
        return;
    }

    TRACE("Creating new device %s.\n", debugstr_w(device_instance_id));

    /* Check if the device already has a driver registered; if not, find one
     * and install it. */
    key = SetupDiOpenDevRegKey( set, &sp_device, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ );
    if (key != INVALID_HANDLE_VALUE)
    {
        if (!RegQueryValueExW( key, infpathW, NULL, NULL, NULL, NULL ))
            need_driver = FALSE;
        RegCloseKey( key );
    }

    if ((status = get_device_caps( device, &caps )))
    {
        ERR("Failed to get caps for device %s, status %#lx.\n", debugstr_w(device_instance_id), status);
        return;
    }

    if (!get_device_id(device, BusQueryContainerID, &id) && id)
    {
        SetupDiSetDeviceRegistryPropertyW( set, &sp_device, SPDRP_BASE_CONTAINERID, (BYTE *)id,
            (lstrlenW( id ) + 1) * sizeof(WCHAR) );
        ExFreePool( id );
    }

    if (need_driver && !install_device_driver( device, set, &sp_device ) && !caps.RawDeviceOK)
    {
        ERR("Unable to install a function driver for device %s.\n", debugstr_w(device_instance_id));
        return;
    }

    start_device( device, set, &sp_device );
}

static void send_remove_device_irp( DEVICE_OBJECT *device, UCHAR code )
{
    struct wine_device *wine_device = CONTAINING_RECORD(device, struct wine_device, device_obj);

    TRACE( "Removing device %p, code %x.\n", device, code );

    if (wine_device->children)
    {
        ULONG i;
        for (i = 0; i < wine_device->children->Count; ++i)
            send_remove_device_irp( wine_device->children->Objects[i], code );
    }

    send_pnp_irp( device, code );
}

static void remove_device( DEVICE_OBJECT *device )
{
    send_remove_device_irp( device, IRP_MN_SURPRISE_REMOVAL );
    send_remove_device_irp( device, IRP_MN_REMOVE_DEVICE );
}

static BOOL device_in_list( const DEVICE_RELATIONS *list, const DEVICE_OBJECT *device )
{
    ULONG i;
    for (i = 0; i < list->Count; ++i)
    {
        if (list->Objects[i] == device)
            return TRUE;
    }
    return FALSE;
}

static void handle_bus_relations( DEVICE_OBJECT *parent )
{
    struct wine_device *wine_parent = CONTAINING_RECORD(parent, struct wine_device, device_obj);
    SP_DEVINFO_DATA sp_device = {sizeof(sp_device)};
    DEVICE_RELATIONS *relations;
    IO_STATUS_BLOCK irp_status;
    IO_STACK_LOCATION *irpsp;
    HDEVINFO set;
    KEVENT event;
    IRP *irp;
    ULONG i;

    TRACE( "(%p)\n", parent );

    set = SetupDiCreateDeviceInfoList( NULL, NULL );

    parent = IoGetAttachedDevice( parent );

    KeInitializeEvent( &event, NotificationEvent, FALSE );
    if (!(irp = IoBuildSynchronousFsdRequest( IRP_MJ_PNP, parent, NULL, 0, NULL, &event, &irp_status )))
    {
        SetupDiDestroyDeviceInfoList( set );
        return;
    }

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;
    irpsp->Parameters.QueryDeviceRelations.Type = BusRelations;

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    if (IoCallDriver( parent, irp ) == STATUS_PENDING)
        KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );

    relations = (DEVICE_RELATIONS *)irp_status.Information;
    if (irp_status.Status)
    {
        ERR("Failed to enumerate child devices, status %#lx.\n", irp_status.Status);
        SetupDiDestroyDeviceInfoList( set );
        return;
    }

    TRACE("Got %lu devices.\n", relations->Count);

    for (i = 0; i < relations->Count; ++i)
    {
        DEVICE_OBJECT *child = relations->Objects[i];

        if (!wine_parent->children || !device_in_list( wine_parent->children, child ))
        {
            TRACE("Adding new device %p.\n", child);
            enumerate_new_device( child, set );
        }
    }

    if (wine_parent->children)
    {
        for (i = 0; i < wine_parent->children->Count; ++i)
        {
            DEVICE_OBJECT *child = wine_parent->children->Objects[i];

            if (!device_in_list( relations, child ))
            {
                TRACE("Removing device %p.\n", child);
                remove_device( child );
            }
            ObDereferenceObject( child );
        }
    }

    ExFreePool( wine_parent->children );
    wine_parent->children = relations;

    SetupDiDestroyDeviceInfoList( set );
}

/***********************************************************************
 *           IoInvalidateDeviceRelations (NTOSKRNL.EXE.@)
 */
void WINAPI IoInvalidateDeviceRelations( DEVICE_OBJECT *device_object, DEVICE_RELATION_TYPE type )
{
    TRACE("device %p, type %#x.\n", device_object, type);

    switch (type)
    {
        case BusRelations:
            EnterCriticalSection( &invalidated_devices_cs );
            invalidated_devices = realloc( invalidated_devices,
                    (invalidated_devices_count + 1) * sizeof(*invalidated_devices) );
            invalidated_devices[invalidated_devices_count++] = device_object;
            LeaveCriticalSection( &invalidated_devices_cs );
            WakeConditionVariable( &invalidated_devices_cv );
            break;

        default:
            FIXME("Unhandled relation %#x.\n", type);
            break;
    }
}

/***********************************************************************
 *           IoGetDeviceProperty   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoGetDeviceProperty( DEVICE_OBJECT *device, DEVICE_REGISTRY_PROPERTY property,
                                     ULONG length, void *buffer, ULONG *needed )
{
    SP_DEVINFO_DATA sp_device = {sizeof(sp_device)};
    WCHAR device_instance_id[MAX_DEVICE_ID_LEN];
    DWORD sp_property = -1;
    NTSTATUS status;
    HDEVINFO set;

    TRACE("device %p, property %u, length %lu, buffer %p, needed %p.\n",
            device, property, length, buffer, needed);

    switch (property)
    {
        case DevicePropertyEnumeratorName:
        {
            WCHAR *id, *ptr;

            status = get_device_id( device, BusQueryInstanceID, &id );
            if (status != STATUS_SUCCESS)
            {
                ERR("Failed to get instance ID, status %#lx.\n", status);
                break;
            }

            wcsupr( id );
            ptr = wcschr( id, '\\' );
            if (ptr) *ptr = 0;

            *needed = sizeof(WCHAR) * (lstrlenW(id) + 1);
            if (length >= *needed)
                memcpy( buffer, id, *needed );
            else
                status = STATUS_BUFFER_TOO_SMALL;

            ExFreePool( id );
            return status;
        }
        case DevicePropertyPhysicalDeviceObjectName:
        {
            ULONG used_len, len = length + sizeof(OBJECT_NAME_INFORMATION);
            OBJECT_NAME_INFORMATION *name = HeapAlloc(GetProcessHeap(), 0, len);
            HANDLE handle;

            status = ObOpenObjectByPointer( device, OBJ_KERNEL_HANDLE, NULL, 0, NULL, KernelMode, &handle );
            if (!status)
            {
                status = NtQueryObject( handle, ObjectNameInformation, name, len, &used_len );
                NtClose( handle );
            }
            if (status == STATUS_SUCCESS)
            {
                /* Ensure room for NULL termination */
                if (length >= name->Name.MaximumLength)
                    memcpy(buffer, name->Name.Buffer, name->Name.MaximumLength);
                else
                    status = STATUS_BUFFER_TOO_SMALL;
                *needed = name->Name.MaximumLength;
            }
            else
            {
                if (status == STATUS_INFO_LENGTH_MISMATCH ||
                    status == STATUS_BUFFER_OVERFLOW)
                {
                    status = STATUS_BUFFER_TOO_SMALL;
                    *needed = used_len - sizeof(OBJECT_NAME_INFORMATION);
                }
                else
                    *needed = 0;
            }
            HeapFree(GetProcessHeap(), 0, name);
            return status;
        }
        case DevicePropertyDeviceDescription:
            sp_property = SPDRP_DEVICEDESC;
            break;
        case DevicePropertyHardwareID:
            sp_property = SPDRP_HARDWAREID;
            break;
        case DevicePropertyCompatibleIDs:
            sp_property = SPDRP_COMPATIBLEIDS;
            break;
        case DevicePropertyClassName:
            sp_property = SPDRP_CLASS;
            break;
        case DevicePropertyClassGuid:
            sp_property = SPDRP_CLASSGUID;
            break;
        case DevicePropertyManufacturer:
            sp_property = SPDRP_MFG;
            break;
        case DevicePropertyFriendlyName:
            sp_property = SPDRP_FRIENDLYNAME;
            break;
        case DevicePropertyLocationInformation:
            sp_property = SPDRP_LOCATION_INFORMATION;
            break;
        case DevicePropertyBusTypeGuid:
            sp_property = SPDRP_BUSTYPEGUID;
            break;
        case DevicePropertyLegacyBusType:
            sp_property = SPDRP_LEGACYBUSTYPE;
            break;
        case DevicePropertyBusNumber:
            sp_property = SPDRP_BUSNUMBER;
            break;
        case DevicePropertyAddress:
            sp_property = SPDRP_ADDRESS;
            break;
        case DevicePropertyUINumber:
            sp_property = SPDRP_UI_NUMBER;
            break;
        case DevicePropertyInstallState:
            sp_property = SPDRP_INSTALL_STATE;
            break;
        case DevicePropertyRemovalPolicy:
            sp_property = SPDRP_REMOVAL_POLICY;
            break;
        default:
            FIXME("Unhandled property %u.\n", property);
            return STATUS_NOT_IMPLEMENTED;
    }

    if ((status = get_device_instance_id( device, device_instance_id )))
        return status;

    if ((set = SetupDiCreateDeviceInfoList( &GUID_NULL, NULL )) == INVALID_HANDLE_VALUE)
    {
        ERR("Failed to create device list, error %#lx.\n", GetLastError());
        return GetLastError();
    }

    if (!SetupDiOpenDeviceInfoW( set, device_instance_id, NULL, 0, &sp_device))
    {
        ERR("Failed to open device, error %#lx.\n", GetLastError());
        SetupDiDestroyDeviceInfoList( set );
        return GetLastError();
    }

    if (SetupDiGetDeviceRegistryPropertyW( set, &sp_device, sp_property, NULL, buffer, length, needed ))
        status = STATUS_SUCCESS;
    else
        status = GetLastError();

    SetupDiDestroyDeviceInfoList( set );

    return status;
}

static NTSTATUS create_device_symlink( DEVICE_OBJECT *device, UNICODE_STRING *symlink_name )
{
    UNICODE_STRING device_nameU;
    WCHAR *device_name;
    ULONG len = 0;
    NTSTATUS ret;

    ret = IoGetDeviceProperty( device, DevicePropertyPhysicalDeviceObjectName, 0, NULL, &len );
    if (ret != STATUS_BUFFER_TOO_SMALL)
        return ret;

    device_name = heap_alloc( len );
    ret = IoGetDeviceProperty( device, DevicePropertyPhysicalDeviceObjectName, len, device_name, &len );
    if (ret)
    {
        heap_free( device_name );
        return ret;
    }

    RtlInitUnicodeString( &device_nameU, device_name );
    ret = IoCreateSymbolicLink( symlink_name, &device_nameU );
    heap_free( device_name );
    return ret;
}

void  __RPC_FAR * __RPC_USER MIDL_user_allocate( SIZE_T len )
{
    return heap_alloc( len );
}

void __RPC_USER MIDL_user_free( void __RPC_FAR *ptr )
{
    heap_free( ptr );
}

static LONG WINAPI rpc_filter( EXCEPTION_POINTERS *eptr )
{
    return I_RpcExceptionFilter( eptr->ExceptionRecord->ExceptionCode );
}

static void send_devicechange( DWORD code, void *data, unsigned int size )
{
    __TRY
    {
        plugplay_send_event( code, data, size );
    }
    __EXCEPT(rpc_filter)
    {
        WARN("Failed to send event, exception %#lx.\n", GetExceptionCode());
    }
    __ENDTRY
}

/***********************************************************************
 *           IoSetDeviceInterfaceState   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoSetDeviceInterfaceState( UNICODE_STRING *name, BOOLEAN enable )
{
    static const WCHAR DeviceClassesW[] = {'\\','R','E','G','I','S','T','R','Y','\\',
        'M','a','c','h','i','n','e','\\','S','y','s','t','e','m','\\',
        'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
        'C','o','n','t','r','o','l','\\',
        'D','e','v','i','c','e','C','l','a','s','s','e','s','\\',0};
    static const WCHAR slashW[] = {'\\',0};
    static const WCHAR hashW[] = {'#',0};

    size_t namelen = name->Length / sizeof(WCHAR);
    DEV_BROADCAST_DEVICEINTERFACE_W *broadcast;
    struct device_interface *iface;
    HANDLE iface_key, control_key;
    OBJECT_ATTRIBUTES attr = {0};
    struct wine_rb_entry *entry;
    UNICODE_STRING control = RTL_CONSTANT_STRING( L"Control" );
    UNICODE_STRING linked = RTL_CONSTANT_STRING( L"Linked" );
    WCHAR *path, *refstr, *p;
    UNICODE_STRING string;
    DWORD data = enable;
    NTSTATUS ret;
    ULONG len;

    TRACE("device %s, enable %u.\n", debugstr_us(name), enable);

    entry = wine_rb_get( &device_interfaces, name );
    if (!entry)
        return STATUS_OBJECT_NAME_NOT_FOUND;

    iface = WINE_RB_ENTRY_VALUE( entry, struct device_interface, entry );

    if (!enable && !iface->enabled)
        return STATUS_OBJECT_NAME_NOT_FOUND;

    if (enable && iface->enabled)
        return STATUS_OBJECT_NAME_EXISTS;

    for (p = name->Buffer + 4, refstr = NULL; p < name->Buffer + namelen; p++)
        if (*p == '\\') refstr = p;
    if (!refstr) refstr = p;

    len = lstrlenW(DeviceClassesW) + 38 + 1 + namelen + 2 + 1;

    if (!(path = heap_alloc( len * sizeof(WCHAR) )))
        return STATUS_NO_MEMORY;

    lstrcpyW( path, DeviceClassesW );
    lstrcpynW( path + lstrlenW( path ), refstr - 38, 39 );
    lstrcatW( path, slashW );
    p = path + lstrlenW( path );
    lstrcpynW( path + lstrlenW( path ), name->Buffer, (refstr - name->Buffer) + 1 );
    p[0] = p[1] = p[3] = '#';
    lstrcatW( path, slashW );
    lstrcatW( path, hashW );
    if (refstr < name->Buffer + namelen)
        lstrcpynW( path + lstrlenW( path ), refstr, name->Buffer + namelen - refstr + 1 );

    attr.Length = sizeof(attr);
    attr.ObjectName = &string;
    RtlInitUnicodeString( &string, path );
    ret = NtOpenKey( &iface_key, KEY_CREATE_SUB_KEY, &attr );
    heap_free(path);
    if (ret)
        return ret;

    attr.RootDirectory = iface_key;
    attr.ObjectName = &control;
    ret = NtCreateKey( &control_key, KEY_SET_VALUE, &attr, 0, NULL, REG_OPTION_VOLATILE, NULL );
    NtClose( iface_key );
    if (ret)
        return ret;

    attr.ObjectName = &linked;
    ret = NtSetValueKey( control_key, &linked, 0, REG_DWORD, &data, sizeof(data) );
    if (ret)
    {
        NtClose( control_key );
        return ret;
    }

    if (enable)
        ret = create_device_symlink( iface->device, name );
    else
        ret = IoDeleteSymbolicLink( name );
    if (ret)
    {
        NtDeleteValueKey( control_key, &linked );
        NtClose( control_key );
        return ret;
    }

    iface->enabled = enable;

    len = offsetof(DEV_BROADCAST_DEVICEINTERFACE_W, dbcc_name[namelen + 1]);

    if ((broadcast = heap_alloc( len )))
    {
        broadcast->dbcc_size       = len;
        broadcast->dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        broadcast->dbcc_reserved   = 0;
        broadcast->dbcc_classguid  = iface->interface_class;
        lstrcpynW( broadcast->dbcc_name, name->Buffer, namelen + 1 );
        if (namelen > 1) broadcast->dbcc_name[1] = '\\';
        send_devicechange( enable ? DBT_DEVICEARRIVAL : DBT_DEVICEREMOVECOMPLETE, broadcast, len );
        heap_free( broadcast );
    }
    return ret;
}

/***********************************************************************
 *           IoSetDevicePropertyData (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoSetDevicePropertyData( DEVICE_OBJECT *device, const DEVPROPKEY *property_key, LCID lcid,
                                         ULONG flags, DEVPROPTYPE type, ULONG size, void *data )
{
    SP_DEVINFO_DATA sp_device = {sizeof(sp_device)};
    WCHAR device_instance_id[MAX_DEVICE_ID_LEN];
    NTSTATUS status;
    HDEVINFO set;

    TRACE( "device %p, property_key %s, lcid %#lx, flags %#lx, type %#lx, size %lu, data %p.\n",
           device, debugstr_propkey(property_key), lcid, flags, type, size, data );

    /* flags is always treated as PLUGPLAY_PROPERTY_PERSISTENT starting with Win 8 / 2012 */

    if (lcid != LOCALE_NEUTRAL) FIXME( "only LOCALE_NEUTRAL is supported\n" );

    if ((status = get_device_instance_id( device, device_instance_id ))) return status;

    if ((set = SetupDiCreateDeviceInfoList( &GUID_NULL, NULL )) == INVALID_HANDLE_VALUE)
    {
        ERR( "Failed to create device list, error %#lx.\n", GetLastError() );
        return GetLastError();
    }

    if (!SetupDiOpenDeviceInfoW( set, device_instance_id, NULL, 0, &sp_device ))
    {
        ERR( "Failed to open device, error %#lx.\n", GetLastError() );
        SetupDiDestroyDeviceInfoList( set );
        return GetLastError();
    }

    if (!SetupDiSetDevicePropertyW( set, &sp_device, property_key, type, data, size, 0 ))
    {
        ERR( "Failed to set property, error %#lx.\n", GetLastError() );
        SetupDiDestroyDeviceInfoList( set );
        return GetLastError();
    }

    SetupDiDestroyDeviceInfoList( set );

    return STATUS_SUCCESS;
}

/***********************************************************************
 *           IoRegisterDeviceInterface (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoRegisterDeviceInterface(DEVICE_OBJECT *device, const GUID *class_guid,
        UNICODE_STRING *refstr, UNICODE_STRING *symbolic_link)
{
    SP_DEVICE_INTERFACE_DATA sp_iface = {sizeof(sp_iface)};
    SP_DEVINFO_DATA sp_device = {sizeof(sp_device)};
    WCHAR device_instance_id[MAX_DEVICE_ID_LEN];
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING device_path;
    struct device_interface *iface;
    struct wine_rb_entry *entry;
    HDEVINFO set;
    WCHAR *p;

    TRACE("device %p, class_guid %s, refstr %s, symbolic_link %p.\n",
            device, debugstr_guid(class_guid), debugstr_us(refstr), symbolic_link);

    if ((status = get_device_instance_id( device, device_instance_id )))
        return status;

    set = SetupDiGetClassDevsW( class_guid, NULL, NULL, DIGCF_DEVICEINTERFACE );
    if (set == INVALID_HANDLE_VALUE) return STATUS_UNSUCCESSFUL;

    if (!SetupDiCreateDeviceInfoW( set, device_instance_id, class_guid, NULL, NULL, 0, &sp_device )
            && !SetupDiOpenDeviceInfoW( set, device_instance_id, NULL, 0, &sp_device ))
    {
        ERR("Failed to create device %s, error %#lx.\n", debugstr_w(device_instance_id), GetLastError());
        return GetLastError();
    }

    if (!SetupDiCreateDeviceInterfaceW( set, &sp_device, class_guid, refstr ? refstr->Buffer : NULL, 0, &sp_iface ))
        return STATUS_UNSUCCESSFUL;

    /* setupapi mangles the case; construct the interface path manually. */

    device_path.Length = (4 + wcslen( device_instance_id ) + 1 + 38) * sizeof(WCHAR);
    if (refstr)
        device_path.Length += sizeof(WCHAR) + refstr->Length;
    device_path.MaximumLength = device_path.Length + sizeof(WCHAR);

    device_path.Buffer = RtlAllocateHeap( GetProcessHeap(), 0, device_path.MaximumLength );
    swprintf( device_path.Buffer, device_path.MaximumLength / sizeof(WCHAR),
            L"\\??\\%s#{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
            device_instance_id, class_guid->Data1, class_guid->Data2, class_guid->Data3,
            class_guid->Data4[0], class_guid->Data4[1], class_guid->Data4[2], class_guid->Data4[3],
            class_guid->Data4[4], class_guid->Data4[5], class_guid->Data4[6], class_guid->Data4[7] );
    for (p = device_path.Buffer + 4; *p; p++)
    {
        if (*p == '\\')
            *p = '#';
    }
    if (refstr)
    {
        *p++ = '\\';
        wcscpy( p, refstr->Buffer );
    }

    TRACE("Returning path %s.\n", debugstr_us(&device_path));

    entry = wine_rb_get( &device_interfaces, &device_path );
    if (entry)
    {
        iface = WINE_RB_ENTRY_VALUE( entry, struct device_interface, entry );
        if (iface->enabled)
            ERR("Device interface %s is still enabled.\n", debugstr_us(&iface->symbolic_link));
    }
    else
    {
        iface = heap_alloc_zero( sizeof(struct device_interface) );
        RtlDuplicateUnicodeString( 1, &device_path, &iface->symbolic_link );
        if (wine_rb_put( &device_interfaces, &iface->symbolic_link, &iface->entry ))
            ERR("Failed to insert interface %s into tree.\n", debugstr_us(&iface->symbolic_link));
    }

    iface->device = device;
    iface->interface_class = *class_guid;
    if (symbolic_link)
        RtlDuplicateUnicodeString( 1, &device_path, symbolic_link );

    RtlFreeUnicodeString( &device_path );

    return status;
}

/***********************************************************************
 *           IoOpenDeviceRegistryKey   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoOpenDeviceRegistryKey( DEVICE_OBJECT *device, ULONG type, ACCESS_MASK access, HANDLE *key )
{
    SP_DEVINFO_DATA sp_device = {sizeof(sp_device)};
    WCHAR device_instance_id[MAX_DEVICE_ID_LEN];
    NTSTATUS status;
    HDEVINFO set;

    TRACE("device %p, type %#lx, access %#lx, key %p.\n", device, type, access, key);

    if ((status = get_device_instance_id( device, device_instance_id )))
    {
        ERR("Failed to get device instance ID, error %#lx.\n", status);
        return status;
    }

    set = SetupDiCreateDeviceInfoList( &GUID_NULL, NULL );

    SetupDiOpenDeviceInfoW( set, device_instance_id, NULL, 0, &sp_device );

    *key = SetupDiOpenDevRegKey( set, &sp_device, DICS_FLAG_GLOBAL, 0, type, access );
    SetupDiDestroyDeviceInfoList( set );
    if (*key == INVALID_HANDLE_VALUE)
        return GetLastError();
    return STATUS_SUCCESS;
}

/***********************************************************************
 *           PoSetPowerState   (NTOSKRNL.EXE.@)
 */
POWER_STATE WINAPI PoSetPowerState( DEVICE_OBJECT *device, POWER_STATE_TYPE type, POWER_STATE state)
{
    FIXME("device %p, type %u, state %u, stub!\n", device, type, state.DeviceState);
    return state;
}

/*****************************************************
 *           PoStartNextPowerIrp   (NTOSKRNL.EXE.@)
 */
void WINAPI PoStartNextPowerIrp( IRP *irp )
{
    FIXME("irp %p, stub!\n", irp);
}

/*****************************************************
 *           PoCallDriver   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI PoCallDriver( DEVICE_OBJECT *device, IRP *irp )
{
    TRACE("device %p, irp %p.\n", device, irp);
    return IoCallDriver( device, irp );
}

static DRIVER_OBJECT *pnp_manager;

struct root_pnp_device
{
    WCHAR id[MAX_DEVICE_ID_LEN];
    struct list entry;
    DEVICE_OBJECT *device;
};

static struct root_pnp_device *find_root_pnp_device( struct wine_driver *driver, const WCHAR *id )
{
    struct root_pnp_device *device;

    LIST_FOR_EACH_ENTRY( device, &driver->root_pnp_devices, struct root_pnp_device, entry )
    {
        if (!wcsicmp( id, device->id ))
            return device;
    }

    return NULL;
}

static NTSTATUS WINAPI pnp_manager_device_pnp( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    struct root_pnp_device *root_device = device->DeviceExtension;
    NTSTATUS status;

    TRACE("device %p, irp %p, minor function %#x.\n", device, irp, stack->MinorFunction);

    switch (stack->MinorFunction)
    {
    case IRP_MN_QUERY_DEVICE_RELATIONS:
        /* The FDO above already handled this, so return the same status. */
        break;
    case IRP_MN_START_DEVICE:
    case IRP_MN_SURPRISE_REMOVAL:
        /* Nothing to do. */
        irp->IoStatus.Status = STATUS_SUCCESS;
        break;
    case IRP_MN_REMOVE_DEVICE:
        list_remove( &root_device->entry );
        irp->IoStatus.Status = STATUS_SUCCESS;
        break;
    case IRP_MN_QUERY_CAPABILITIES:
        irp->IoStatus.Status = STATUS_SUCCESS;
        break;
    case IRP_MN_QUERY_ID:
    {
        BUS_QUERY_ID_TYPE type = stack->Parameters.QueryId.IdType;
        WCHAR *id, *p;

        TRACE("Received IRP_MN_QUERY_ID, type %#x.\n", type);

        switch (type)
        {
        case BusQueryDeviceID:
            p = wcsrchr( root_device->id, '\\' );
            if ((id = ExAllocatePool( NonPagedPool, (p - root_device->id + 1) * sizeof(WCHAR) )))
            {
                memcpy( id, root_device->id, (p - root_device->id) * sizeof(WCHAR) );
                id[p - root_device->id] = 0;
                irp->IoStatus.Information = (ULONG_PTR)id;
                irp->IoStatus.Status = STATUS_SUCCESS;
            }
            else
            {
                irp->IoStatus.Information = 0;
                irp->IoStatus.Status = STATUS_NO_MEMORY;
            }
            break;
        case BusQueryInstanceID:
            p = wcsrchr( root_device->id, '\\' );
            if ((id = ExAllocatePool( NonPagedPool, (wcslen( p + 1 ) + 1) * sizeof(WCHAR) )))
            {
                wcscpy( id, p + 1 );
                irp->IoStatus.Information = (ULONG_PTR)id;
                irp->IoStatus.Status = STATUS_SUCCESS;
            }
            else
            {
                irp->IoStatus.Information = 0;
                irp->IoStatus.Status = STATUS_NO_MEMORY;
            }
            break;
        default:
            FIXME("Unhandled IRP_MN_QUERY_ID type %#x.\n", type);
        }
        break;
    }
    default:
        FIXME("Unhandled PnP request %#x.\n", stack->MinorFunction);
    }

    status = irp->IoStatus.Status;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return status;
}

static NTSTATUS WINAPI pnp_manager_driver_entry( DRIVER_OBJECT *driver, UNICODE_STRING *keypath )
{
    pnp_manager = driver;
    driver->MajorFunction[IRP_MJ_PNP] = pnp_manager_device_pnp;
    return STATUS_SUCCESS;
}

static DWORD CALLBACK device_enum_thread_proc(void *arg)
{
    for (;;)
    {
        DEVICE_OBJECT *device;

        EnterCriticalSection( &invalidated_devices_cs );

        while (!invalidated_devices_count)
            SleepConditionVariableCS( &invalidated_devices_cv, &invalidated_devices_cs, INFINITE );

        device = invalidated_devices[--invalidated_devices_count];

        /* Don't hold the CS while enumerating the device. Tests show that
         * calling IoInvalidateDeviceRelations() from another thread shouldn't
         * block, even if this thread is blocked in an IRP handler. */
        LeaveCriticalSection( &invalidated_devices_cs );

        handle_bus_relations( device );
    }

    return 0;
}

void pnp_manager_start(void)
{
    WCHAR endpoint[] = L"\\pipe\\wine_plugplay";
    WCHAR protseq[] = L"ncacn_np";
    UNICODE_STRING driver_nameU = RTL_CONSTANT_STRING( L"\\Driver\\PnpManager" );
    RPC_WSTR binding_str;
    NTSTATUS status;
    RPC_STATUS err;

    if ((status = IoCreateDriver( &driver_nameU, pnp_manager_driver_entry )))
        ERR("Failed to create PnP manager driver, status %#lx.\n", status);

    if ((err = RpcStringBindingComposeW( NULL, protseq, NULL, endpoint, NULL, &binding_str )))
    {
        ERR("RpcStringBindingCompose() failed, error %#lx\n", err);
        return;
    }
    err = RpcBindingFromStringBindingW( binding_str, &plugplay_binding_handle );
    RpcStringFreeW( &binding_str );
    if (err)
        ERR("RpcBindingFromStringBinding() failed, error %#lx\n", err);

    CreateThread( NULL, 0, device_enum_thread_proc, NULL, 0, NULL );
}

void pnp_manager_stop_driver( struct wine_driver *driver )
{
    struct root_pnp_device *device, *next;

    LIST_FOR_EACH_ENTRY_SAFE( device, next, &driver->root_pnp_devices, struct root_pnp_device, entry )
        remove_device( device->device );
}

void pnp_manager_stop(void)
{
    IoDeleteDriver( pnp_manager );
    RpcBindingFree( &plugplay_binding_handle );
}

void CDECL wine_enumerate_root_devices( const WCHAR *driver_name )
{
    static const WCHAR driverW[] = {'\\','D','r','i','v','e','r','\\',0};
    static const WCHAR rootW[] = {'R','O','O','T',0};
    WCHAR buffer[MAX_SERVICE_NAME + ARRAY_SIZE(driverW)], id[MAX_DEVICE_ID_LEN];
    SP_DEVINFO_DATA sp_device = {sizeof(sp_device)};
    struct list new_list = LIST_INIT(new_list);
    struct root_pnp_device *pnp_device, *next;
    struct wine_driver *driver;
    DEVICE_OBJECT *device;
    NTSTATUS status;
    unsigned int i;
    HDEVINFO set;

    TRACE("Searching for new root-enumerated devices for driver %s.\n", debugstr_w(driver_name));

    driver = get_driver( driver_name );

    set = SetupDiGetClassDevsW( NULL, rootW, NULL, DIGCF_ALLCLASSES );
    if (set == INVALID_HANDLE_VALUE)
    {
        ERR("Failed to build device set, error %#lx.\n", GetLastError());
        return;
    }

    for (i = 0; SetupDiEnumDeviceInfo( set, i, &sp_device ); ++i)
    {
        if (!SetupDiGetDeviceRegistryPropertyW( set, &sp_device, SPDRP_SERVICE,
                NULL, (BYTE *)buffer, sizeof(buffer), NULL )
                || lstrcmpiW( buffer, driver_name ))
        {
            continue;
        }

        SetupDiGetDeviceInstanceIdW( set, &sp_device, id, ARRAY_SIZE(id), NULL );

        if ((pnp_device = find_root_pnp_device( driver, id )))
        {
            TRACE("Found device %s already enumerated.\n", debugstr_w(id));
            list_remove( &pnp_device->entry );
            list_add_tail( &new_list, &pnp_device->entry );
            continue;
        }

        TRACE("Adding new root-enumerated device %s.\n", debugstr_w(id));

        if ((status = IoCreateDevice( pnp_manager, sizeof(struct root_pnp_device), NULL,
                FILE_DEVICE_CONTROLLER, FILE_AUTOGENERATED_DEVICE_NAME, FALSE, &device )))
        {
            ERR("Failed to create root-enumerated PnP device %s, status %#lx.\n", debugstr_w(id), status);
            continue;
        }

        pnp_device = device->DeviceExtension;
        wcscpy( pnp_device->id, id );
        pnp_device->device = device;
        list_add_tail( &new_list, &pnp_device->entry );

        start_device( device, set, &sp_device );
    }

    LIST_FOR_EACH_ENTRY_SAFE( pnp_device, next, &driver->root_pnp_devices, struct root_pnp_device, entry )
    {
        TRACE("Removing device %s.\n", debugstr_w(pnp_device->id));
        remove_device( pnp_device->device );
    }

    list_move_head( &driver->root_pnp_devices, &new_list );

    SetupDiDestroyDeviceInfoList(set);
}
