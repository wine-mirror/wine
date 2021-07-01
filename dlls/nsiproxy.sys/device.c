/*
 * nsiproxy.sys
 *
 * Copyright 2021 Huw Davies
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
#include "winternl.h"
#include "winioctl.h"
#include "ddk/wdm.h"
#include "ifdef.h"
#include "netiodef.h"
#include "wine/nsi.h"
#include "wine/debug.h"
#include "nsiproxy_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(nsi);

static void nsiproxy_enumerate_all( IRP *irp )
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    struct nsiproxy_enumerate_all *in = (struct nsiproxy_enumerate_all *)irp->AssociatedIrp.SystemBuffer;
    DWORD in_len = irpsp->Parameters.DeviceIoControl.InputBufferLength;
    void *out = irp->AssociatedIrp.SystemBuffer;
    DWORD out_len = irpsp->Parameters.DeviceIoControl.OutputBufferLength;
    struct nsi_enumerate_all_ex enum_all;

    if (in_len != sizeof(*in))
    {
        irp->IoStatus.u.Status = STATUS_INVALID_PARAMETER;
        return;
    }

    if (out_len < sizeof(DWORD) + (in->key_size + in->rw_size + in->dynamic_size + in->static_size) * in->count)
    {
        irp->IoStatus.u.Status = STATUS_INVALID_PARAMETER;
        return;
    }

    enum_all.unknown[0] = 0;
    enum_all.unknown[1] = 0;
    enum_all.first_arg = in->first_arg;
    enum_all.second_arg = in->second_arg;
    enum_all.module = &in->module;
    enum_all.table = in->table;
    enum_all.key_data = (BYTE *)out + sizeof(DWORD);
    enum_all.key_size = in->key_size;
    enum_all.rw_data = (BYTE *)enum_all.key_data + in->key_size * in->count;
    enum_all.rw_size = in->rw_size;
    enum_all.dynamic_data = (BYTE *)enum_all.rw_data + in->rw_size * in->count;
    enum_all.dynamic_size = in->dynamic_size;
    enum_all.static_data = (BYTE *)enum_all.dynamic_data + in->dynamic_size * in->count;
    enum_all.static_size = in->static_size;
    enum_all.count = in->count;

    irp->IoStatus.u.Status = nsi_enumerate_all_ex( &enum_all );
    if (irp->IoStatus.u.Status == STATUS_SUCCESS || irp->IoStatus.u.Status == STATUS_MORE_ENTRIES)
    {
        irp->IoStatus.Information = out_len;
        *(DWORD *)out = enum_all.count;
    }
    else irp->IoStatus.Information = 0;
}

static void nsiproxy_get_all_parameters( IRP *irp )
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    struct nsiproxy_get_all_parameters *in = (struct nsiproxy_get_all_parameters *)irp->AssociatedIrp.SystemBuffer;
    DWORD in_len = irpsp->Parameters.DeviceIoControl.InputBufferLength;
    BYTE *out = irp->AssociatedIrp.SystemBuffer;
    DWORD out_len = irpsp->Parameters.DeviceIoControl.OutputBufferLength;
    struct nsi_get_all_parameters_ex get_all;

    if (in_len < FIELD_OFFSET(struct nsiproxy_get_all_parameters, key[0]) ||
        in_len < FIELD_OFFSET(struct nsiproxy_get_all_parameters, key[in->key_size]))
    {
        irp->IoStatus.u.Status = STATUS_INVALID_PARAMETER;
        return;
    }

    if (out_len < in->rw_size + in->dynamic_size + in->static_size)
    {
        irp->IoStatus.u.Status = STATUS_INVALID_PARAMETER;
        return;
    }

    get_all.unknown[0] = 0;
    get_all.unknown[1] = 0;
    get_all.first_arg = in->first_arg;
    get_all.unknown2 = 0;
    get_all.module = &in->module;
    get_all.table = in->table;
    get_all.key = in->key;
    get_all.key_size = in->key_size;
    get_all.rw_data = out;
    get_all.rw_size = in->rw_size;
    get_all.dynamic_data = out + in->rw_size;
    get_all.dynamic_size = in->dynamic_size;
    get_all.static_data = out + in->rw_size + in->dynamic_size;
    get_all.static_size = in->static_size;

    irp->IoStatus.u.Status = nsi_get_all_parameters_ex( &get_all );
    irp->IoStatus.Information = (irp->IoStatus.u.Status == STATUS_SUCCESS) ? out_len : 0;
}

static void nsiproxy_get_parameter( IRP *irp )
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    struct nsiproxy_get_parameter *in = (struct nsiproxy_get_parameter *)irp->AssociatedIrp.SystemBuffer;
    DWORD in_len = irpsp->Parameters.DeviceIoControl.InputBufferLength;
    void *out = irp->AssociatedIrp.SystemBuffer;
    DWORD out_len = irpsp->Parameters.DeviceIoControl.OutputBufferLength;
    struct nsi_get_parameter_ex get_param;

    if (in_len < FIELD_OFFSET(struct nsiproxy_get_parameter, key[0]) ||
        in_len < FIELD_OFFSET(struct nsiproxy_get_parameter, key[in->key_size]))
    {
        irp->IoStatus.u.Status = STATUS_INVALID_PARAMETER;
        return;
    }

    get_param.unknown[0] = 0;
    get_param.unknown[1] = 0;
    get_param.first_arg = in->first_arg;
    get_param.unknown2 = 0;
    get_param.module = &in->module;
    get_param.table = in->table;
    get_param.key = in->key;
    get_param.key_size = in->key_size;
    get_param.param_type = in->param_type;
    get_param.data = out;
    get_param.data_size = out_len;
    get_param.data_offset = in->data_offset;

    irp->IoStatus.u.Status = nsi_get_parameter_ex( &get_param );
    irp->IoStatus.Information = irp->IoStatus.u.Status == STATUS_SUCCESS ? out_len : 0;
}

static NTSTATUS WINAPI nsi_ioctl( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );

    TRACE( "ioctl %x insize %u outsize %u\n",
           irpsp->Parameters.DeviceIoControl.IoControlCode,
           irpsp->Parameters.DeviceIoControl.InputBufferLength,
           irpsp->Parameters.DeviceIoControl.OutputBufferLength );

    switch (irpsp->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_NSIPROXY_WINE_ENUMERATE_ALL:
        nsiproxy_enumerate_all( irp );
        break;

    case IOCTL_NSIPROXY_WINE_GET_ALL_PARAMETERS:
        nsiproxy_get_all_parameters( irp );
        break;

    case IOCTL_NSIPROXY_WINE_GET_PARAMETER:
        nsiproxy_get_parameter( irp );
        break;

    default:
        FIXME( "ioctl %x not supported\n", irpsp->Parameters.DeviceIoControl.IoControlCode );
        irp->IoStatus.u.Status = STATUS_NOT_SUPPORTED;
        break;
    }

    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}

static int add_device( DRIVER_OBJECT *driver )
{
    static const WCHAR name_str[] = {'\\','D','e','v','i','c','e','\\','N','s','i',0};
    static const WCHAR link_str[] = {'\\','?','?','\\','N','s','i',0};
    UNICODE_STRING name, link;
    DEVICE_OBJECT *device;
    NTSTATUS status;

    RtlInitUnicodeString( &name, name_str );
    RtlInitUnicodeString( &link, link_str );

    if (!(status = IoCreateDevice( driver, 0, &name, FILE_DEVICE_NETWORK, FILE_DEVICE_SECURE_OPEN, FALSE, &device )))
        status = IoCreateSymbolicLink( &link, &name );
    if (status)
    {
        FIXME( "failed to create device error %x\n", status );
        return 0;
    }

    return 1;
}

NTSTATUS WINAPI DriverEntry( DRIVER_OBJECT *driver, UNICODE_STRING *path )
{
    TRACE( "(%p, %s)\n", driver, debugstr_w( path->Buffer ) );

    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = nsi_ioctl;

    add_device( driver );

    return STATUS_SUCCESS;
}
