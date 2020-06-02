/*
 * Copyright 2020 Alistair Leslie-Hughes
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
#ifndef _INC_WSK
#define _INC_WSK

#include <winsock2.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct _WSK_CLIENT;
typedef struct _WSK_CLIENT WSK_CLIENT, *PWSK_CLIENT;
typedef struct _WSK_CLIENT_CONNECTION_DISPATCH WSK_CLIENT_CONNECTION_DISPATCH, *PWSK_CLIENT_CONNECTION_DISPATCH;

typedef struct _WSK_BUF
{
    PMDL Mdl;
    ULONG Offset;
    SIZE_T Length;
} WSK_BUF, *PWSK_BUF;

typedef struct _WSK_DATA_INDICATION
{
    struct _WSK_DATA_INDICATION *Next;
    WSK_BUF                      Buffer;
} WSK_DATA_INDICATION, *PWSK_DATA_INDICATION;

typedef NTSTATUS (WINAPI *PFN_WSK_CLIENT_EVENT)(void *context, ULONG event, void *info, SIZE_T length);
typedef NTSTATUS (WINAPI *PFN_WSK_DISCONNECT_EVENT)(void *context, ULONG flags);
typedef NTSTATUS (WINAPI *PFN_WSK_SEND_BACKLOG_EVENT)(void *socket_context, SIZE_T ideal_backlog_size);
typedef NTSTATUS (WINAPI *PFN_WSK_SOCKET)(WSK_CLIENT *client, ADDRESS_FAMILY family,
        USHORT type, ULONG protocol, ULONG flags, void *context, const void *dispatch,
        PEPROCESS process, PETHREAD thread, SECURITY_DESCRIPTOR *security, IRP *irp);
typedef NTSTATUS (WINAPI *PFN_WSK_SOCKET_CONNECT)(WSK_CLIENT *client, USHORT type,
        ULONG protocol, SOCKADDR *local, SOCKADDR *remote, ULONG flags, void *context,
        const WSK_CLIENT_CONNECTION_DISPATCH *dispatch, PEPROCESS process, PETHREAD owning,
        SECURITY_DESCRIPTOR *descriptor, IRP *irp);
typedef NTSTATUS (WINAPI *PFN_WSK_CONTROL_CLIENT)(WSK_CLIENT *client, ULONG control,
        SIZE_T input_size, void *input, SIZE_T output_size, void *output, SIZE_T *returned,
        IRP *irp);
typedef NTSTATUS (WINAPI *PFN_WSK_GET_ADDRESS_INFO)(WSK_CLIENT *client, UNICODE_STRING *node_name,
        UNICODE_STRING *service_name, ULONG name_space, GUID *provider, ADDRINFOEXW *hints,
        ADDRINFOEXW **result, PEPROCESS process, PETHREAD thread, IRP *irp);
typedef void (WINAPI *PFN_WSK_FREE_ADDRESS_INFO)(WSK_CLIENT *client, ADDRINFOEXW *addrinfo);
typedef NTSTATUS (WINAPI *PFN_WSK_GET_NAME_INFO)(WSK_CLIENT *client, SOCKADDR *addr,
        ULONG length, UNICODE_STRING *node_name, UNICODE_STRING *service_name,
        ULONG flags, PEPROCESS process, PETHREAD thread, IRP *irp);
typedef NTSTATUS (WINAPI* PFN_WSK_RECEIVE_EVENT)(void *context, ULONG flags,
        WSK_DATA_INDICATION *indication, SIZE_T size, SIZE_T *accepted);

typedef struct _WSK_PROVIDER_DISPATCH
{
    USHORT Version;
    USHORT Reserved;
    PFN_WSK_SOCKET WskSocket;
    PFN_WSK_SOCKET_CONNECT WskSocketConnect;
    PFN_WSK_CONTROL_CLIENT WskControlClient;
    PFN_WSK_GET_ADDRESS_INFO WskGetAddressInfo;
    PFN_WSK_FREE_ADDRESS_INFO WskFreeAddressInfo;
    PFN_WSK_GET_NAME_INFO WskGetNameInfo;
} WSK_PROVIDER_DISPATCH, *PWSK_PROVIDER_DISPATCH;

typedef struct _WSK_CLIENT_CONNECTION_DISPATCH
{
    PFN_WSK_RECEIVE_EVENT WskReceiveEvent;
    PFN_WSK_DISCONNECT_EVENT WskDisconnectEvent;
    PFN_WSK_SEND_BACKLOG_EVENT WskSendBacklogEvent;
} WSK_CLIENT_CONNECTION_DISPATCH, *PWSK_CLIENT_CONNECTION_DISPATCH;

typedef struct _WSK_CLIENT_DISPATCH
{
    USHORT Version;
    USHORT Reserved;
    PFN_WSK_CLIENT_EVENT WskClientEvent;
} WSK_CLIENT_DISPATCH, *PWSK_CLIENT_DISPATCH;

typedef struct _WSK_CLIENT_NPI
{
    void *ClientContext;
    const WSK_CLIENT_DISPATCH *Dispatch;
} WSK_CLIENT_NPI, *PWSK_CLIENT_NPI;

typedef struct _WSK_REGISTRATION
{
    ULONGLONG ReservedRegistrationState;
    void *ReservedRegistrationContext;
    KSPIN_LOCK ReservedRegistrationLock;
} WSK_REGISTRATION, *PWSK_REGISTRATION;

typedef struct _WSK_PROVIDER_NPI
{
    PWSK_CLIENT Client;
    const WSK_PROVIDER_DISPATCH *Dispatch;
} WSK_PROVIDER_NPI, *PWSK_PROVIDER_NPI;

NTSTATUS WINAPI WskRegister(WSK_CLIENT_NPI *wsk_client_npi, WSK_REGISTRATION *wsk_registration);
void WINAPI WskDeregister(WSK_REGISTRATION *wsk_registration);
NTSTATUS WINAPI WskCaptureProviderNPI(WSK_REGISTRATION *wsk_registration, ULONG wait_timeout,
        WSK_PROVIDER_NPI *wsk_provider_npi);
void WINAPI WskReleaseProviderNPI(WSK_REGISTRATION *wsk_registration);

#ifdef __cplusplus
}
#endif
#endif
