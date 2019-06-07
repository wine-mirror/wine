/*
 * Plug and Play
 *
 * Copyright 2016 Sebastian Lackner
 * Copyright 2016 Aric Stewart for CodeWeavers
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

#define NONAMELESSUNION

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winioctl.h"
#include "winreg.h"
#include "winuser.h"
#include "winsvc.h"
#include "winternl.h"
#include "setupapi.h"
#include "cfgmgr32.h"
#include "dbt.h"
#include "ddk/wdm.h"
#include "ddk/ntifs.h"
#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/rbtree.h"
#include "wine/unicode.h"

#include "ntoskrnl_private.h"

#include "initguid.h"
DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

WINE_DEFAULT_DEBUG_CHANNEL(plugplay);

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

static inline BOOL is_valid_hex(WCHAR c)
{
    if (!(((c >= '0') && (c <= '9'))  ||
          ((c >= 'a') && (c <= 'f'))  ||
          ((c >= 'A') && (c <= 'F'))))
        return FALSE;
    return TRUE;
}

static const BYTE guid_conv_table[256] =
{
  0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x00 */
  0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x10 */
  0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x20 */
  0,   1,   2,   3,   4,   5,   6, 7, 8, 9, 0, 0, 0, 0, 0, 0, /* 0x30 */
  0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x40 */
  0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x50 */
  0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf                             /* 0x60 */
};

static BOOL guid_from_string(const WCHAR *s, GUID *id)
{
    int	i;

    if (!s || s[0] != '{')
    {
        memset( id, 0, sizeof (CLSID) );
        return FALSE;
    }

    id->Data1 = 0;
    for (i = 1; i < 9; i++)
    {
        if (!is_valid_hex(s[i])) return FALSE;
        id->Data1 = (id->Data1 << 4) | guid_conv_table[s[i]];
    }
    if (s[9] != '-') return FALSE;

    id->Data2 = 0;
    for (i = 10; i < 14; i++)
    {
        if (!is_valid_hex(s[i])) return FALSE;
        id->Data2 = (id->Data2 << 4) | guid_conv_table[s[i]];
    }
    if (s[14] != '-') return FALSE;

    id->Data3 = 0;
    for (i = 15; i < 19; i++)
    {
        if (!is_valid_hex(s[i])) return FALSE;
        id->Data3 = (id->Data3 << 4) | guid_conv_table[s[i]];
    }
    if (s[19] != '-') return FALSE;

    for (i = 20; i < 37; i += 2)
    {
        if (i == 24)
        {
            if (s[i] != '-') return FALSE;
            i++;
        }
        if (!is_valid_hex(s[i]) || !is_valid_hex(s[i+1])) return FALSE;
        id->Data4[(i-20)/2] = guid_conv_table[s[i]] << 4 | guid_conv_table[s[i+1]];
    }

    if (s[37] == '}')
        return TRUE;

    return FALSE;
}

static NTSTATUS WINAPI internal_complete( DEVICE_OBJECT *device, IRP *irp, void *context )
{
    HANDLE event = context;
    SetEvent( event );
    return STATUS_MORE_PROCESSING_REQUIRED;
}

static NTSTATUS send_device_irp( DEVICE_OBJECT *device, IRP *irp, ULONG_PTR *info )
{
    HANDLE event = CreateEventA( NULL, FALSE, FALSE, NULL );
    DEVICE_OBJECT *toplevel_device;
    NTSTATUS status;

    irp->IoStatus.u.Status = STATUS_NOT_SUPPORTED;
    IoSetCompletionRoutine( irp, internal_complete, event, TRUE, TRUE, TRUE );

    toplevel_device = IoGetAttachedDeviceReference( device );
    status = IoCallDriver( toplevel_device, irp );

    if (status == STATUS_PENDING)
        WaitForSingleObject( event, INFINITE );

    status = irp->IoStatus.u.Status;
    if (info)
        *info = irp->IoStatus.Information;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    ObDereferenceObject( toplevel_device );
    CloseHandle( event );
    return status;
}

static NTSTATUS get_device_id( DEVICE_OBJECT *device, BUS_QUERY_ID_TYPE type, WCHAR **id )
{
    IO_STACK_LOCATION *irpsp;
    IO_STATUS_BLOCK irp_status;
    IRP *irp;

    device = IoGetAttachedDevice( device );

    if (!(irp = IoBuildSynchronousFsdRequest( IRP_MJ_PNP, device, NULL, 0, NULL, NULL, &irp_status )))
        return STATUS_NO_MEMORY;

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->MinorFunction = IRP_MN_QUERY_ID;
    irpsp->Parameters.QueryId.IdType = type;

    return send_device_irp( device, irp, (ULONG_PTR *)id );
}

static NTSTATUS send_pnp_irp( DEVICE_OBJECT *device, UCHAR minor )
{
    IO_STACK_LOCATION *irpsp;
    IO_STATUS_BLOCK irp_status;
    IRP *irp;

    device = IoGetAttachedDevice( device );

    if (!(irp = IoBuildSynchronousFsdRequest( IRP_MJ_PNP, device, NULL, 0, NULL, NULL, &irp_status )))
        return STATUS_NO_MEMORY;

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->MinorFunction = minor;

    irpsp->Parameters.StartDevice.AllocatedResources = NULL;
    irpsp->Parameters.StartDevice.AllocatedResourcesTranslated = NULL;

    return send_device_irp( device, irp, NULL );
}

static NTSTATUS get_device_instance_id( DEVICE_OBJECT *device, WCHAR *buffer )
{
    static const WCHAR backslashW[] = {'\\',0};
    NTSTATUS status;
    WCHAR *id;

    if ((status = get_device_id( device, BusQueryDeviceID, &id )))
    {
        ERR("Failed to get device ID, status %#x.\n", status);
        return status;
    }

    strcpyW( buffer, id );
    ExFreePool( id );

    if ((status = get_device_id( device, BusQueryInstanceID, &id )))
    {
        ERR("Failed to get instance ID, status %#x.\n", status);
        return status;
    }

    strcatW( buffer, backslashW );
    strcatW( buffer, id );
    ExFreePool( id );

    TRACE("Returning ID %s.\n", debugstr_w(buffer));

    return STATUS_SUCCESS;
}

static NTSTATUS send_power_irp( DEVICE_OBJECT *device, DEVICE_POWER_STATE power )
{
    IO_STATUS_BLOCK irp_status;
    IO_STACK_LOCATION *irpsp;
    IRP *irp;

    device = IoGetAttachedDevice( device );

    if (!(irp = IoBuildSynchronousFsdRequest( IRP_MJ_POWER, device, NULL, 0, NULL, NULL, &irp_status )))
        return STATUS_NO_MEMORY;

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->MinorFunction = IRP_MN_SET_POWER;

    irpsp->Parameters.Power.Type = DevicePowerState;
    irpsp->Parameters.Power.State.DeviceState = power;

    return send_device_irp( device, irp, NULL );
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

    strcpyW( buffer, servicesW );
    strcatW( buffer, driver );
    RtlInitUnicodeString( &string, buffer );
    status = ZwLoadDriver( &string );
    if (status != STATUS_SUCCESS && status != STATUS_IMAGE_ALREADY_LOADED)
    {
        ERR("Failed to load driver %s, status %#x.\n", debugstr_w(driver), status);
        return;
    }

    strcpyW( buffer, driverW );
    strcatW( buffer, driver );
    RtlInitUnicodeString( &string, buffer );
    if (ObReferenceObjectByName( &string, OBJ_CASE_INSENSITIVE, NULL,
                                 0, NULL, KernelMode, NULL, (void **)&driver_obj ) != STATUS_SUCCESS)
    {
        ERR("Failed to locate loaded driver %s.\n", debugstr_w(driver));
        return;
    }

    if (driver_obj->DriverExtension->AddDevice)
        status = driver_obj->DriverExtension->AddDevice( driver_obj, device );
    else
        status = STATUS_NOT_IMPLEMENTED;

    ObDereferenceObject( driver_obj );

    if (status != STATUS_SUCCESS)
        ERR("AddDevice failed for driver %s, status %#x.\n", debugstr_w(driver), status);
}

/* Return the total number of characters in a REG_MULTI_SZ string, including
 * the final terminating null. */
static size_t sizeof_multiszW( const WCHAR *str )
{
    const WCHAR *p;
    for (p = str; *p; p += strlenW(p) + 1);
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

    NTSTATUS status;
    unsigned int i;
    WCHAR *ids;

    if ((status = get_device_id( device, BusQueryHardwareIDs, &ids )) || !ids)
    {
        ERR("Failed to get hardware IDs, status %#x.\n", status);
        return FALSE;
    }

    SetupDiSetDeviceRegistryPropertyW( set, sp_device, SPDRP_HARDWAREID, (BYTE *)ids,
            sizeof_multiszW( ids ) * sizeof(WCHAR) );
    ExFreePool( ids );

    if ((status = get_device_id( device, BusQueryCompatibleIDs, &ids )) || !ids)
    {
        ERR("Failed to get compatible IDs, status %#x.\n", status);
        return FALSE;
    }

    SetupDiSetDeviceRegistryPropertyW( set, sp_device, SPDRP_COMPATIBLEIDS, (BYTE *)ids,
            sizeof_multiszW( ids ) * sizeof(WCHAR) );
    ExFreePool( ids );

    if (!SetupDiBuildDriverInfoList( set, sp_device, SPDIT_COMPATDRIVER ))
    {
        ERR("Failed to build compatible driver list, error %#x.\n", GetLastError());
        return FALSE;
    }

    for (i = 0; i < ARRAY_SIZE(dif_list); ++i)
    {
        if (!SetupDiCallClassInstaller(dif_list[i], set, sp_device) && GetLastError() != ERROR_DI_DO_DEFAULT)
        {
            ERR("Install function %#x failed, error %#x.\n", dif_list[i], GetLastError());
            return FALSE;
        }
    }

    return TRUE;
}

static void handle_bus_relations( DEVICE_OBJECT *device )
{
    static const WCHAR infpathW[] = {'I','n','f','P','a','t','h',0};

    SP_DEVINFO_DATA sp_device = {sizeof(sp_device)};
    WCHAR device_instance_id[MAX_DEVICE_ID_LEN];
    BOOL need_driver = TRUE;
    HDEVINFO set;
    HKEY key;

    /* We could (should?) do a full IRP_MN_QUERY_DEVICE_RELATIONS query,
     * but we don't have to, we have the DEVICE_OBJECT of the new device
     * so we can simply handle the process here */

    if (get_device_instance_id( device, device_instance_id ))
        return;

    set = SetupDiCreateDeviceInfoList( NULL, NULL );

    if (!SetupDiCreateDeviceInfoW( set, device_instance_id, &GUID_NULL, NULL, NULL, 0, &sp_device )
            && !SetupDiOpenDeviceInfoW( set, device_instance_id, NULL, 0, &sp_device ))
    {
        ERR("Failed to create or open device %s, error %#x.\n", debugstr_w(device_instance_id), GetLastError());
        SetupDiDestroyDeviceInfoList( set );
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

    if (need_driver && !install_device_driver( device, set, &sp_device ))
    {
        SetupDiDestroyDeviceInfoList( set );
        return;
    }

    load_function_driver( device, set, &sp_device );
    if (device->DriverObject)
    {
        send_pnp_irp( device, IRP_MN_START_DEVICE );
        send_power_irp( device, PowerDeviceD0 );
    }

    SetupDiDestroyDeviceInfoList( set );
}

static void handle_removal_relations( DEVICE_OBJECT *device )
{
    TRACE( "(%p)\n", device );

    send_power_irp( device, PowerDeviceD3 );
    send_pnp_irp( device, IRP_MN_SURPRISE_REMOVAL );
    send_pnp_irp( device, IRP_MN_REMOVE_DEVICE );
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
            handle_bus_relations( device_object );
            break;
        case RemovalRelations:
            handle_removal_relations( device_object );
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
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    TRACE("device %p, property %u, length %u, buffer %p, needed %p.\n",
            device, property, length, buffer, needed);

    switch (property)
    {
        case DevicePropertyEnumeratorName:
        {
            WCHAR *id, *ptr;

            status = get_device_id( device, BusQueryInstanceID, &id );
            if (status != STATUS_SUCCESS)
            {
                ERR("Failed to get instance ID, status %#x.\n", status);
                break;
            }

            struprW( id );
            ptr = strchrW( id, '\\' );
            if (ptr) *ptr = 0;

            *needed = sizeof(WCHAR) * (strlenW(id) + 1);
            if (length >= *needed)
                memcpy( buffer, id, *needed );
            else
                status = STATUS_BUFFER_TOO_SMALL;

            ExFreePool( id );
            break;
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
            break;
        }
        default:
            FIXME("Unhandled property %u.\n", property);
    }
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
    static const WCHAR controlW[] = {'C','o','n','t','r','o','l',0};
    static const WCHAR linkedW[] = {'L','i','n','k','e','d',0};
    static const WCHAR slashW[] = {'\\',0};
    static const WCHAR hashW[] = {'#',0};

    size_t namelen = name->Length / sizeof(WCHAR);
    DEV_BROADCAST_DEVICEINTERFACE_W *broadcast;
    struct device_interface *iface;
    HANDLE iface_key, control_key;
    OBJECT_ATTRIBUTES attr = {0};
    struct wine_rb_entry *entry;
    WCHAR *path, *refstr, *p;
    UNICODE_STRING string;
    DWORD data = enable;
    NTSTATUS ret;
    GUID class;
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

    refstr = memrchrW(name->Buffer + 4, '\\', namelen - 4);

    if (!guid_from_string( (refstr ? refstr : name->Buffer + namelen) - 38, &class ))
        return STATUS_INVALID_PARAMETER;

    len = strlenW(DeviceClassesW) + 38 + 1 + namelen + 2 + 1;

    if (!(path = heap_alloc( len * sizeof(WCHAR) )))
        return STATUS_NO_MEMORY;

    strcpyW( path, DeviceClassesW );
    lstrcpynW( path + strlenW( path ), (refstr ? refstr : name->Buffer + namelen) - 38, 39 );
    strcatW( path, slashW );
    p = path + strlenW( path );
    lstrcpynW( path + strlenW( path ), name->Buffer, (refstr ? (refstr - name->Buffer) : namelen) + 1 );
    p[0] = p[1] = p[3] = '#';
    strcatW( path, slashW );
    strcatW( path, hashW );
    if (refstr)
        lstrcpynW( path + strlenW( path ), refstr, name->Buffer + namelen - refstr + 1 );

    attr.Length = sizeof(attr);
    attr.ObjectName = &string;
    RtlInitUnicodeString( &string, path );
    ret = NtOpenKey( &iface_key, KEY_CREATE_SUB_KEY, &attr );
    heap_free(path);
    if (ret)
        return ret;

    attr.RootDirectory = iface_key;
    RtlInitUnicodeString( &string, controlW );
    ret = NtCreateKey( &control_key, KEY_SET_VALUE, &attr, 0, NULL, 0, NULL );
    NtClose( iface_key );
    if (ret)
        return ret;

    RtlInitUnicodeString( &string, linkedW );
    ret = NtSetValueKey( control_key, &string, 0, REG_DWORD, &data, sizeof(data) );
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
        NtDeleteValueKey( control_key, &string );
        NtClose( control_key );
        return ret;
    }

    iface->enabled = enable;

    len = offsetof(DEV_BROADCAST_DEVICEINTERFACE_W, dbcc_name[namelen + 1]);

    if ((broadcast = heap_alloc( len )))
    {
        broadcast->dbcc_size = len;
        broadcast->dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        broadcast->dbcc_classguid = class;
        lstrcpynW( broadcast->dbcc_name, name->Buffer, namelen + 1 );
        BroadcastSystemMessageW( BSF_FORCEIFHUNG | BSF_QUERY, NULL, WM_DEVICECHANGE,
            enable ? DBT_DEVICEARRIVAL : DBT_DEVICEREMOVECOMPLETE, (LPARAM)broadcast );

        heap_free( broadcast );
    }
    return ret;
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
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *data;
    NTSTATUS status = STATUS_SUCCESS;
    struct device_interface *iface;
    DWORD required;
    HDEVINFO set;

    TRACE("device %p, class_guid %s, refstr %s, symbolic_link %p.\n",
            device, debugstr_guid(class_guid), debugstr_us(refstr), symbolic_link);

    if ((status = get_device_instance_id( device, device_instance_id )))
        return status;

    set = SetupDiGetClassDevsW( class_guid, NULL, NULL, DIGCF_DEVICEINTERFACE );
    if (set == INVALID_HANDLE_VALUE) return STATUS_UNSUCCESSFUL;

    if (!SetupDiCreateDeviceInfoW( set, device_instance_id, class_guid, NULL, NULL, 0, &sp_device )
            && !SetupDiOpenDeviceInfoW( set, device_instance_id, NULL, 0, &sp_device ))
    {
        ERR("Failed to create device %s, error %#x.\n", debugstr_w(device_instance_id), GetLastError());
        return GetLastError();
    }

    if (!SetupDiCreateDeviceInterfaceW( set, &sp_device, class_guid, refstr ? refstr->Buffer : NULL, 0, &sp_iface ))
        return STATUS_UNSUCCESSFUL;

    required = 0;
    SetupDiGetDeviceInterfaceDetailW( set, &sp_iface, NULL, 0, &required, NULL );
    if (required == 0) return STATUS_UNSUCCESSFUL;

    data = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, required );
    data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

    if (!SetupDiGetDeviceInterfaceDetailW( set, &sp_iface, data, required, NULL, NULL ))
    {
        HeapFree( GetProcessHeap(), 0, data );
        return STATUS_UNSUCCESSFUL;
    }

    data->DevicePath[1] = '?';
    TRACE("Returning path %s.\n", debugstr_w(data->DevicePath));

    iface = heap_alloc_zero( sizeof(struct device_interface) );
    iface->device = device;
    iface->interface_class = *class_guid;
    RtlCreateUnicodeString(&iface->symbolic_link, data->DevicePath);
    if (symbolic_link)
        RtlCreateUnicodeString( symbolic_link, data->DevicePath);

    if (wine_rb_put( &device_interfaces, &iface->symbolic_link, &iface->entry ))
        ERR("Failed to insert interface %s into tree.\n", debugstr_us(&iface->symbolic_link));

    HeapFree( GetProcessHeap(), 0, data );

    return status;
}

/***********************************************************************
 *           PoSetPowerState   (NTOSKRNL.EXE.@)
 */
POWER_STATE WINAPI PoSetPowerState( DEVICE_OBJECT *device, POWER_STATE_TYPE type, POWER_STATE state)
{
    FIXME("device %p, type %u, state %u, stub!\n", device, type, state.DeviceState);
    return state;
}
