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
#include "winsock2.h"
#include "ws2tcpip.h"

#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(netio);

struct _WSK_CLIENT
{
    WSK_REGISTRATION *registration;
    WSK_CLIENT_NPI *client_npi;
};

static NTSTATUS sock_error_to_ntstatus(DWORD err)
{
    switch (err)
    {
        case 0:                    return STATUS_SUCCESS;
        case WSAEBADF:             return STATUS_INVALID_HANDLE;
        case WSAEACCES:            return STATUS_ACCESS_DENIED;
        case WSAEFAULT:            return STATUS_NO_MEMORY;
        case WSAEINVAL:            return STATUS_INVALID_PARAMETER;
        case WSAEMFILE:            return STATUS_TOO_MANY_OPENED_FILES;
        case WSAEWOULDBLOCK:       return STATUS_CANT_WAIT;
        case WSAEINPROGRESS:       return STATUS_PENDING;
        case WSAEALREADY:          return STATUS_NETWORK_BUSY;
        case WSAENOTSOCK:          return STATUS_OBJECT_TYPE_MISMATCH;
        case WSAEDESTADDRREQ:      return STATUS_INVALID_PARAMETER;
        case WSAEMSGSIZE:          return STATUS_BUFFER_OVERFLOW;
        case WSAEPROTONOSUPPORT:
        case WSAESOCKTNOSUPPORT:
        case WSAEPFNOSUPPORT:
        case WSAEAFNOSUPPORT:
        case WSAEPROTOTYPE:        return STATUS_NOT_SUPPORTED;
        case WSAENOPROTOOPT:       return STATUS_INVALID_PARAMETER;
        case WSAEOPNOTSUPP:        return STATUS_NOT_SUPPORTED;
        case WSAEADDRINUSE:        return STATUS_ADDRESS_ALREADY_ASSOCIATED;
        case WSAEADDRNOTAVAIL:     return STATUS_INVALID_PARAMETER;
        case WSAECONNREFUSED:      return STATUS_CONNECTION_REFUSED;
        case WSAESHUTDOWN:         return STATUS_PIPE_DISCONNECTED;
        case WSAENOTCONN:          return STATUS_CONNECTION_DISCONNECTED;
        case WSAETIMEDOUT:         return STATUS_IO_TIMEOUT;
        case WSAENETUNREACH:       return STATUS_NETWORK_UNREACHABLE;
        case WSAENETDOWN:          return STATUS_NETWORK_BUSY;
        case WSAECONNRESET:        return STATUS_CONNECTION_RESET;
        case WSAECONNABORTED:      return STATUS_CONNECTION_ABORTED;
        case WSAHOST_NOT_FOUND:    return STATUS_NOT_FOUND;
        default:
            FIXME("Unmapped error %u.\n", err);
            return STATUS_UNSUCCESSFUL;
    }
}

static void dispatch_irp(IRP *irp, NTSTATUS status)
{
    irp->IoStatus.u.Status = status;
    --irp->CurrentLocation;
    --irp->Tail.Overlay.s.u2.CurrentStackLocation;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

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

struct wsk_get_address_info_context
{
    UNICODE_STRING *node_name;
    UNICODE_STRING *service_name;
    ULONG namespace;
    GUID *provider;
    ADDRINFOEXW *hints;
    ADDRINFOEXW **result;
    IRP *irp;
};

static void WINAPI get_address_info_callback(TP_CALLBACK_INSTANCE *instance, void *context_)
{
    struct wsk_get_address_info_context *context = context_;
    INT ret;

    TRACE("instance %p, context %p.\n", instance, context);

    ret = GetAddrInfoExW( context->node_name ? context->node_name->Buffer : NULL,
            context->service_name ? context->service_name->Buffer : NULL, context->namespace,
            context->provider, context->hints, context->result, NULL, NULL, NULL, NULL);

    context->irp->IoStatus.Information = 0;
    dispatch_irp(context->irp, sock_error_to_ntstatus(ret));
    heap_free(context);
}

static NTSTATUS WINAPI wsk_get_address_info(WSK_CLIENT *client, UNICODE_STRING *node_name,
        UNICODE_STRING *service_name, ULONG name_space, GUID *provider, ADDRINFOEXW *hints,
        ADDRINFOEXW **result, PEPROCESS owning_process, PETHREAD owning_thread, IRP *irp)
{
    struct wsk_get_address_info_context *context;
    NTSTATUS status;

    TRACE("client %p, node_name %p, service_name %p, name_space %#x, provider %p, hints %p, "
            "result %p, owning_process %p, owning_thread %p, irp %p.\n",
            client, node_name, service_name, name_space, provider, hints, result,
            owning_process, owning_thread, irp);

    if (!irp)
        return STATUS_INVALID_PARAMETER;

    if (!(context = heap_alloc(sizeof(*context))))
    {
        ERR("No memory.\n");
        status = STATUS_NO_MEMORY;
        dispatch_irp(irp, status);
        return status;
    }

    context->node_name = node_name;
    context->service_name = service_name;
    context->namespace = name_space;
    context->provider = provider;
    context->hints = hints;
    context->result = result;
    context->irp = irp;

    if (!TrySubmitThreadpoolCallback(get_address_info_callback, context, NULL))
    {
        ERR("Could not submit thread pool callback.\n");
        status = STATUS_UNSUCCESSFUL;
        dispatch_irp(irp, status);
        heap_free(context);
        return status;
    }
    TRACE("Submitted threadpool callback, context %p.\n", context);
    return STATUS_PENDING;
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
    static const WORD version = MAKEWORD( 2, 2 );
    WSADATA data;

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

    if (WSAStartup(version, &data))
        return STATUS_INTERNAL_ERROR;

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
