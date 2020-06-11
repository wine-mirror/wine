/*
 * WSK (Winsock Kernel) driver library.
 *
 * Copyright 2020 Paul Gofman <pgofman@codeweavers.com> for Codeweavers
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
#define NONAMELESSSTRUCT

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winioctl.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "ddk/wsk.h"
#include "wine/debug.h"

#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(netio);

struct _WSK_CLIENT
{
    WSK_REGISTRATION *registration;
    WSK_CLIENT_NPI *client_npi;
};

static NTSTATUS WINAPI wsk_socket(WSK_CLIENT *client, ADDRESS_FAMILY address_family, USHORT socket_type,
        ULONG protocol, ULONG Flags, void *socket_context, const void *dispatch, PEPROCESS owning_process,
        PETHREAD owning_thread, SECURITY_DESCRIPTOR *security_descriptor, IRP *irp)
{
    FIXME("client %p, address_family %#x, socket_type %#x, protocol %#x, Flags %#x, socket_context %p, dispatch %p, "
            "owning_process %p, owning_thread %p, security_descriptor %p, irp %p stub.\n",
            client, address_family, socket_type, protocol, Flags, socket_context, dispatch, owning_process,
            owning_thread, security_descriptor, irp);

    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS WINAPI wsk_socket_connect(WSK_CLIENT *client, USHORT socket_type, ULONG protocol,
        SOCKADDR *local_address, SOCKADDR *remote_address, ULONG flags, void *socket_context,
        const WSK_CLIENT_CONNECTION_DISPATCH *dispatch, PEPROCESS owning_process, PETHREAD owning_thread,
        SECURITY_DESCRIPTOR *security_descriptor, IRP *irp)
{
    FIXME("client %p, socket_type %#x, protocol %#x, local_address %p, remote_address %p, "
            "flags %#x, socket_context %p, dispatch %p, owning_process %p, owning_thread %p, "
            "security_descriptor %p, irp %p stub.\n",
            client, socket_type, protocol, local_address, remote_address, flags, socket_context,
            dispatch, owning_process, owning_thread, security_descriptor, irp);

    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS WINAPI wsk_control_client(WSK_CLIENT *client, ULONG control_code, SIZE_T input_size,
        void *input_buffer, SIZE_T output_size, void *output_buffer, SIZE_T *output_size_returned,
        IRP *irp
)
{
    FIXME("client %p, control_code %#x, input_size %lu, input_buffer %p, output_size %lu, "
            "output_buffer %p, output_size_returned %p, irp %p, stub.\n",
            client, control_code, input_size, input_buffer, output_size, output_buffer,
            output_size_returned, irp);

    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS WINAPI wsk_get_address_info(WSK_CLIENT *client, UNICODE_STRING *node_name,
        UNICODE_STRING *service_name, ULONG name_space, GUID *provider, ADDRINFOEXW *hints,
        ADDRINFOEXW **result, PEPROCESS owning_process, PETHREAD owning_thread, IRP *irp)
{
    FIXME("client %p, node_name %p, service_name %p, name_space %#x, provider %p, hints %p, "
            "result %p, owning_process %p, owning_thread %p, irp %p stub.\n",
            client, node_name, service_name, name_space, provider, hints, result,
            owning_process, owning_thread, irp);

    return STATUS_NOT_IMPLEMENTED;
}

static void WINAPI wsk_free_address_info(WSK_CLIENT *client, ADDRINFOEXW *addr_info)
{
    FIXME("client %p, addr_info %p stub.\n", client, addr_info);
}

static NTSTATUS WINAPI wsk_get_name_info(WSK_CLIENT *client, SOCKADDR *sock_addr, ULONG sock_addr_length,
        UNICODE_STRING *node_name, UNICODE_STRING *service_name, ULONG flags, PEPROCESS owning_process,
        PETHREAD owning_thread, IRP *irp)
{
    FIXME("client %p, sock_addr %p, sock_addr_length %u, node_name %p, service_name %p, "
            "flags %#x, owning_process %p, owning_thread %p, irp %p stub.\n",
            client, sock_addr, sock_addr_length, node_name, service_name, flags,
            owning_process, owning_thread, irp);

    return STATUS_NOT_IMPLEMENTED;
}

static const WSK_PROVIDER_DISPATCH wsk_dispatch =
{
    MAKE_WSK_VERSION(1, 0), 0,
    wsk_socket,
    wsk_socket_connect,
    wsk_control_client,
    wsk_get_address_info,
    wsk_free_address_info,
    wsk_get_name_info,
};

NTSTATUS WINAPI WskCaptureProviderNPI(WSK_REGISTRATION *wsk_registration, ULONG wait_timeout,
        WSK_PROVIDER_NPI *wsk_provider_npi)
{
    WSK_CLIENT *client = wsk_registration->ReservedRegistrationContext;

    TRACE("wsk_registration %p, wait_timeout %u, wsk_provider_npi %p.\n",
            wsk_registration, wait_timeout, wsk_provider_npi);

    wsk_provider_npi->Client = client;
    wsk_provider_npi->Dispatch = &wsk_dispatch;
    return STATUS_SUCCESS;
}

void WINAPI WskReleaseProviderNPI(WSK_REGISTRATION *wsk_registration)
{
    TRACE("wsk_registration %p.\n", wsk_registration);

}

NTSTATUS WINAPI WskRegister(WSK_CLIENT_NPI *wsk_client_npi, WSK_REGISTRATION *wsk_registration)
{
    WSK_CLIENT *client;

    TRACE("wsk_client_npi %p, wsk_registration %p.\n", wsk_client_npi, wsk_registration);

    if (!(client = heap_alloc(sizeof(*client))))
    {
        ERR("No memory.\n");
        return STATUS_NO_MEMORY;
    }

    client->registration = wsk_registration;
    client->client_npi = wsk_client_npi;
    wsk_registration->ReservedRegistrationContext = client;

    return STATUS_SUCCESS;
}

void WINAPI WskDeregister(WSK_REGISTRATION *wsk_registration)
{
    TRACE("wsk_registration %p.\n", wsk_registration);

    heap_free(wsk_registration->ReservedRegistrationContext);
}

static void WINAPI driver_unload(DRIVER_OBJECT *driver)
{
    TRACE("driver %p.\n", driver);
}

NTSTATUS WINAPI DriverEntry(DRIVER_OBJECT *driver, UNICODE_STRING *path)
{
    TRACE("driver %p, path %s.\n", driver, debugstr_w(path->Buffer));

    driver->DriverUnload = driver_unload;
    return STATUS_SUCCESS;
}
