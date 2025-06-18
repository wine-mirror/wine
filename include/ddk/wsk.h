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
#include <mswsock.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct _WSK_CLIENT;

typedef struct _WSK_CLIENT WSK_CLIENT, *PWSK_CLIENT;

typedef struct _WSK_SOCKET
{
    const void *Dispatch;
} WSK_SOCKET, *PWSK_SOCKET;

#define MAKE_WSK_VERSION(major, minor) ((USHORT)((major) << 8) | (USHORT)((minor) & 0xff))
#define WSK_NO_WAIT 0
#define WSK_INFINITE_WAIT 0xffffffff

#define WSK_FLAG_BASIC_SOCKET      0x00000000
#define WSK_FLAG_LISTEN_SOCKET     0x00000001
#define WSK_FLAG_CONNECTION_SOCKET 0x00000002
#define WSK_FLAG_DATAGRAM_SOCKET   0x00000004
#define WSK_FLAG_STREAM_SOCKET     0x00000008

typedef enum _WSK_CONTROL_SOCKET_TYPE
{
     WskSetOption,
     WskGetOption,
     WskIoctl,
} WSK_CONTROL_SOCKET_TYPE;

typedef enum _WSK_INSPECT_ACTION
{
    WskInspectReject,
    WskInspectAccept,
} WSK_INSPECT_ACTION;

typedef struct _WSK_CLIENT_CONNECTION_DISPATCH WSK_CLIENT_CONNECTION_DISPATCH, *PWSK_CLIENT_CONNECTION_DISPATCH;

typedef struct _WSK_BUF
{
    PMDL Mdl;
    ULONG Offset;
    SIZE_T Length;
} WSK_BUF, *PWSK_BUF;

typedef struct _WSK_BUF_LIST
{
    struct _WSK_BUF_LIST *Next;
    WSK_BUF Buffer;
} WSK_BUF_LIST, *PWSK_BUF_LIST;

typedef struct _WSK_DATA_INDICATION
{
    struct _WSK_DATA_INDICATION *Next;
    WSK_BUF Buffer;
} WSK_DATA_INDICATION, *PWSK_DATA_INDICATION;

typedef struct _WSK_INSPECT_ID
{
    ULONG_PTR Key;
    ULONG SerialNumber;
} WSK_INSPECT_ID, *PWSK_INSPECT_ID;

typedef struct _WSK_DATAGRAM_INDICATION
{
    struct _WSK_DATAGRAM_INDICATION *Next;
    WSK_BUF Buffer;
    PCMSGHDR ControlInfo;
    ULONG ControlInfoLength;
    PSOCKADDR RemoteAddress;
} WSK_DATAGRAM_INDICATION, *PWSK_DATAGRAM_INDICATION;

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

typedef NTSTATUS (WINAPI *PFN_WSK_ACCEPT_EVENT)(void *socket_context, ULONG flags, SOCKADDR *local_address,
        SOCKADDR *remote_address, WSK_SOCKET *accept_socket, void *accept_socket_context,
        const WSK_CLIENT_CONNECTION_DISPATCH **accept_socket_dispatch);
typedef WSK_INSPECT_ACTION (WINAPI *PFN_WSK_INSPECT_EVENT)(void *socket_context, SOCKADDR *local_address,
        SOCKADDR *remote_address, WSK_INSPECT_ID *inspect_id);
typedef NTSTATUS (WINAPI *PFN_WSK_ABORT_EVENT)(void *socket_context, WSK_INSPECT_ID *inspect_id);

typedef struct _WSK_CLIENT_LISTEN_DISPATCH
{
  PFN_WSK_ACCEPT_EVENT WskAcceptEvent;
  PFN_WSK_INSPECT_EVENT WskInspectEvent;
  PFN_WSK_ABORT_EVENT WskAbortEvent;
} WSK_CLIENT_LISTEN_DISPATCH, *PWSK_CLIENT_LISTEN_DISPATCH;

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

typedef NTSTATUS (WINAPI *PFN_WSK_CONTROL_SOCKET)(WSK_SOCKET *socket, WSK_CONTROL_SOCKET_TYPE request_type,
        ULONG control_code, ULONG level, SIZE_T input_size, void *input_buffer, SIZE_T output_size,
        void *output_buffer, SIZE_T *output_size_returned, IRP *irp);
typedef NTSTATUS (WINAPI *PFN_WSK_CLOSE_SOCKET)(WSK_SOCKET *socket, IRP *irp);
typedef NTSTATUS (WINAPI *PFN_WSK_BIND)(WSK_SOCKET *socket, SOCKADDR *local_address, ULONG flags, IRP *irp);
typedef NTSTATUS (WINAPI *PFN_WSK_ACCEPT)(WSK_SOCKET *listen_socket, ULONG flags, void *accept_socket_context,
        const WSK_CLIENT_CONNECTION_DISPATCH *accept_socket_dispatch, SOCKADDR *local_address,
        SOCKADDR *remote_address, IRP *irp);
typedef NTSTATUS (WINAPI *PFN_WSK_CONNECT)(WSK_SOCKET *socket, SOCKADDR *remote_address, ULONG flags, IRP *irp);
typedef NTSTATUS (WINAPI *PFN_WSK_LISTEN)(WSK_SOCKET *socket, IRP *irp);
typedef NTSTATUS (WINAPI *PFN_WSK_SEND)(WSK_SOCKET *socket, WSK_BUF *buffer, ULONG flags, IRP *irp);
typedef NTSTATUS (WINAPI *PFN_WSK_RECEIVE)(WSK_SOCKET *socket, WSK_BUF *buffer, ULONG flags, IRP *irp);
typedef NTSTATUS (WINAPI *PFN_WSK_DISCONNECT)(WSK_SOCKET *socket, WSK_BUF *buffer, ULONG flags, IRP *irp);
typedef NTSTATUS (WINAPI *PFN_WSK_GET_LOCAL_ADDRESS)(WSK_SOCKET *socket, SOCKADDR *local_address, IRP *irp);
typedef NTSTATUS (WINAPI *PFN_WSK_GET_REMOTE_ADDRESS)(WSK_SOCKET *socket, SOCKADDR *remote_address, IRP *irp);
typedef NTSTATUS (WINAPI *PFN_WSK_CONNECT_EX)(WSK_SOCKET *socket, SOCKADDR *remote_address, WSK_BUF *buffer,
        ULONG flags, IRP *irp);
typedef NTSTATUS (WINAPI *PFN_WSK_RELEASE_DATA_INDICATION_LIST)(WSK_SOCKET *socket,
        WSK_DATA_INDICATION *data_indication);
typedef NTSTATUS (WINAPI *PFN_WSK_SEND_MESSAGES) (WSK_SOCKET *socket, WSK_BUF_LIST *buffer_list, ULONG flags,
        SOCKADDR *remote_address, ULONG control_info_length, CMSGHDR *control_info, IRP *irp);
typedef NTSTATUS (WINAPI *PFN_WSK_SEND_TO)(WSK_SOCKET *socket, WSK_BUF *buffer, ULONG flags, SOCKADDR *remote_address,
        ULONG control_info_length, CMSGHDR *control_info, IRP *irp);
typedef NTSTATUS (WINAPI *PFN_WSK_RECEIVE_FROM)(WSK_SOCKET *socket, WSK_BUF *buffer, ULONG flags,
        SOCKADDR *remote_address, ULONG *control_length, CMSGHDR *control_info, ULONG *control_flags, IRP *irp);
typedef NTSTATUS (WINAPI *PFN_WSK_RELEASE_DATAGRAM_INDICATION_LIST)(WSK_SOCKET *socket,
        WSK_DATAGRAM_INDICATION *datagram_indication);
typedef NTSTATUS (WINAPI *PFN_WSK_INSPECT_COMPLETE)(WSK_SOCKET *listen_socket, WSK_INSPECT_ID *inspect_id,
        WSK_INSPECT_ACTION action, IRP *irp);

/* PFN_WSK_SEND_EX, PFN_WSK_RECEIVE_EX functions are undocumented and reserved for system use. */
typedef void *PFN_WSK_SEND_EX;
typedef void *PFN_WSK_RECEIVE_EX;

typedef struct _WSK_PROVIDER_BASIC_DISPATCH
{
    PFN_WSK_CONTROL_SOCKET WskControlSocket;
    PFN_WSK_CLOSE_SOCKET WskCloseSocket;
} WSK_PROVIDER_BASIC_DISPATCH, *PWSK_PROVIDER_BASIC_DISPATCH;

typedef struct _WSK_PROVIDER_STREAM_DISPATCH
{
    WSK_PROVIDER_BASIC_DISPATCH Basic;
    PFN_WSK_BIND WskBind;
    PFN_WSK_ACCEPT WskAccept;
    PFN_WSK_CONNECT WskConnect;
    PFN_WSK_LISTEN WskListen;
    PFN_WSK_SEND WskSend;
    PFN_WSK_RECEIVE WskReceive;
    PFN_WSK_DISCONNECT WskDisconnect;
    PFN_WSK_RELEASE_DATA_INDICATION_LIST WskRelease;
    PFN_WSK_GET_LOCAL_ADDRESS WskGetLocalAddress;
    PFN_WSK_GET_REMOTE_ADDRESS WskGetRemoteAddress;
    PFN_WSK_CONNECT_EX WskConnectEx;
    PFN_WSK_SEND_EX WskSendEx;
    PFN_WSK_RECEIVE_EX WskReceiveEx;
} WSK_PROVIDER_STREAM_DISPATCH, *PWSK_PROVIDER_STREAM_DISPATCH;

typedef struct _WSK_PROVIDER_CONNECTION_DISPATCH
{
    WSK_PROVIDER_BASIC_DISPATCH Basic;
    PFN_WSK_BIND WskBind;
    PFN_WSK_CONNECT WskConnect;
    PFN_WSK_GET_LOCAL_ADDRESS WskGetLocalAddress;
    PFN_WSK_GET_REMOTE_ADDRESS WskGetRemoteAddress;
    PFN_WSK_SEND WskSend;
    PFN_WSK_RECEIVE WskReceive;
    PFN_WSK_DISCONNECT WskDisconnect;
    PFN_WSK_RELEASE_DATA_INDICATION_LIST WskRelease;
    PFN_WSK_CONNECT_EX WskConnectEx;
    PFN_WSK_SEND_EX WskSendEx;
    PFN_WSK_RECEIVE_EX WskReceiveEx;
} WSK_PROVIDER_CONNECTION_DISPATCH, *PWSK_PROVIDER_CONNECTION_DISPATCH;

typedef struct _WSK_PROVIDER_DATAGRAM_DISPATCH
{
    WSK_PROVIDER_BASIC_DISPATCH Basic;
    PFN_WSK_BIND WskBind;
    PFN_WSK_SEND_TO WskSendTo;
    PFN_WSK_RECEIVE_FROM WskReceiveFrom;
    PFN_WSK_RELEASE_DATAGRAM_INDICATION_LIST WskRelease;
    PFN_WSK_GET_LOCAL_ADDRESS WskGetLocalAddress;
    PFN_WSK_SEND_MESSAGES WskSendMessages;
} WSK_PROVIDER_DATAGRAM_DISPATCH, *PWSK_PROVIDER_DATAGRAM_DISPATCH;

typedef struct _WSK_PROVIDER_LISTEN_DISPATCH
{
    WSK_PROVIDER_BASIC_DISPATCH Basic;
    PFN_WSK_BIND WskBind;
    PFN_WSK_ACCEPT WskAccept;
    PFN_WSK_INSPECT_COMPLETE WskInspectComplete;
    PFN_WSK_GET_LOCAL_ADDRESS WskGetLocalAddress;
} WSK_PROVIDER_LISTEN_DISPATCH, *PWSK_PROVIDER_LISTEN_DISPATCH;

NTSTATUS WINAPI WskRegister(WSK_CLIENT_NPI *wsk_client_npi, WSK_REGISTRATION *wsk_registration);
void WINAPI WskDeregister(WSK_REGISTRATION *wsk_registration);
NTSTATUS WINAPI WskCaptureProviderNPI(WSK_REGISTRATION *wsk_registration, ULONG wait_timeout,
        WSK_PROVIDER_NPI *wsk_provider_npi);
void WINAPI WskReleaseProviderNPI(WSK_REGISTRATION *wsk_registration);

#ifdef __cplusplus
}
#endif
#endif
