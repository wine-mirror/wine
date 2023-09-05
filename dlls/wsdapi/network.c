/*
 * Web Services on Devices
 *
 * Copyright 2017-2018 Owen Rudge for CodeWeavers
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

#define COBJMACROS

#include "wsdapi_internal.h"
#include "wine/debug.h"
#include "iphlpapi.h"
#include "bcrypt.h"

WINE_DEFAULT_DEBUG_CHANNEL(wsdapi);

#define SEND_ADDRESS_IPV4   0xEFFFFFFA /* 239.255.255.250 */
#define SEND_PORT           3702

static UCHAR send_address_ipv6[] = {0xFF,0x02,0,0,0,0,0,0,0,0,0,0,0,0,0,0xC}; /* FF02::C */

#define UNICAST_UDP_REPEAT     1
#define MULTICAST_UDP_REPEAT   2
#define UDP_MIN_DELAY         50
#define UDP_MAX_DELAY        250
#define UDP_UPPER_DELAY      500

static void send_message(SOCKET s, char *data, int length, SOCKADDR_STORAGE *dest, int max_initial_delay, int repeat)
{
    UINT delay;
    int len;

    /* Sleep for a random amount of time before sending the message */
    if (max_initial_delay > 0)
    {
        BCryptGenRandom(NULL, (BYTE*) &delay, sizeof(UINT), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
        Sleep(delay % max_initial_delay);
    }

    len = (dest->ss_family == AF_INET6) ? sizeof(SOCKADDR_IN6) : sizeof(SOCKADDR_IN);

    if (sendto(s, data, length, 0, (SOCKADDR *) dest, len) == SOCKET_ERROR)
        WARN("Unable to send data to socket: %d\n", WSAGetLastError());

    if (repeat-- <= 0) return;

    BCryptGenRandom(NULL, (BYTE*) &delay, sizeof(UINT), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    delay = delay % (UDP_MAX_DELAY - UDP_MIN_DELAY + 1) + UDP_MIN_DELAY;

    for (;;)
    {
        Sleep(delay);

        if (sendto(s, data, length, 0, (SOCKADDR *) dest, len) == SOCKET_ERROR)
            WARN("Unable to send data to socket: %d\n", WSAGetLastError());

        if (repeat-- <= 0) break;
        delay = min(delay * 2, UDP_UPPER_DELAY);
    }
}

typedef struct sending_thread_params
{
    char *data;
    int length;
    SOCKET sock;
    SOCKADDR_STORAGE dest;
    int max_initial_delay;
} sending_thread_params;

static DWORD WINAPI sending_thread(LPVOID lpParam)
{
    sending_thread_params *params = (sending_thread_params *) lpParam;

    send_message(params->sock, params->data, params->length, &params->dest, params->max_initial_delay,
        MULTICAST_UDP_REPEAT);
    closesocket(params->sock);

    free(params->data);
    free(params);

    return 0;
}

static IP_ADAPTER_ADDRESSES *get_adapters(ULONG family)
{
    ULONG err, size = 4096;
    IP_ADAPTER_ADDRESSES *tmp, *ret;

    if (!(ret = malloc( size ))) return NULL;
    err = GetAdaptersAddresses( family, 0, NULL, ret, &size );
    while (err == ERROR_BUFFER_OVERFLOW)
    {
        if (!(tmp = realloc( ret, size ))) break;
        ret = tmp;
        err = GetAdaptersAddresses( family, 0, NULL, ret, &size );
    }
    if (err == ERROR_SUCCESS) return ret;
    free( ret );
    return NULL;
}

static BOOL send_udp_multicast_of_type(char *data, int length, int max_initial_delay, ULONG family)
{
    IP_ADAPTER_ADDRESSES *adapter_addresses, *adapter_addr;
    static const struct in6_addr i_addr_zero;
    sending_thread_params *send_params;
    LPSOCKADDR sockaddr;
    HANDLE thread_handle;
    const char ttl = 8;
    SOCKET s;

    adapter_addresses = get_adapters(family);
    if (!adapter_addresses)
        return FALSE;

    for (adapter_addr = adapter_addresses; adapter_addr != NULL; adapter_addr = adapter_addr->Next)
    {
        if (adapter_addr->FirstUnicastAddress == NULL)
        {
            TRACE("No address found for adaptor '%s' (%p)\n", debugstr_a(adapter_addr->AdapterName), adapter_addr);
            continue;
        }

        sockaddr = adapter_addr->FirstUnicastAddress->Address.lpSockaddr;

        /* Create a socket and bind to the adapter address */
        s = socket(family, SOCK_DGRAM, IPPROTO_UDP);

        if (s == INVALID_SOCKET)
        {
            WARN("Unable to create socket: %d\n", WSAGetLastError());
            continue;
        }

        if (bind(s, sockaddr, adapter_addr->FirstUnicastAddress->Address.iSockaddrLength) == SOCKET_ERROR)
        {
            WARN("Unable to bind to socket (adaptor '%s' (%p)): %d\n", debugstr_a(adapter_addr->AdapterName),
                adapter_addr, WSAGetLastError());
            closesocket(s);
            continue;
        }

        /* Set the multicast interface and TTL value */
        setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, (char *) &i_addr_zero,
            (family == AF_INET6) ? sizeof(struct in6_addr) : sizeof(struct in_addr));
        setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

        /* Set up the thread parameters */
        send_params = malloc(sizeof(*send_params));

        send_params->data = malloc(length);
        memcpy(send_params->data, data, length);
        send_params->length = length;
        send_params->sock = s;
        send_params->max_initial_delay = max_initial_delay;

        memset(&send_params->dest, 0, sizeof(SOCKADDR_STORAGE));
        send_params->dest.ss_family = family;

        if (family == AF_INET)
        {
            SOCKADDR_IN *sockaddr4 = (SOCKADDR_IN *)&send_params->dest;

            sockaddr4->sin_port = htons(SEND_PORT);
            sockaddr4->sin_addr.S_un.S_addr = htonl(SEND_ADDRESS_IPV4);
        }
        else
        {
            SOCKADDR_IN6 *sockaddr6 = (SOCKADDR_IN6 *)&send_params->dest;

            sockaddr6->sin6_port = htons(SEND_PORT);
            memcpy(&sockaddr6->sin6_addr, &send_address_ipv6, sizeof(send_address_ipv6));
        }

        thread_handle = CreateThread(NULL, 0, sending_thread, send_params, 0, NULL);

        if (thread_handle == NULL)
        {
            WARN("CreateThread failed (error %ld)\n", GetLastError());
            closesocket(s);

            free(send_params->data);
            free(send_params);

            continue;
        }

        CloseHandle(thread_handle);
    }

    free(adapter_addresses);
    return TRUE;
}

BOOL send_udp_multicast(IWSDiscoveryPublisherImpl *impl, char *data, int length, int max_initial_delay)
{
    if ((impl->addressFamily & WSDAPI_ADDRESSFAMILY_IPV4) &&
        (!send_udp_multicast_of_type(data, length,max_initial_delay, AF_INET))) return FALSE;

    if ((impl->addressFamily & WSDAPI_ADDRESSFAMILY_IPV6) &&
        (!send_udp_multicast_of_type(data, length, max_initial_delay, AF_INET6))) return FALSE;

    return TRUE;
}

static int join_multicast_group(SOCKET s, SOCKADDR_STORAGE *group, SOCKADDR_STORAGE *iface)
{
    int level, optname, optlen;
    struct ipv6_mreq mreqv6;
    struct ip_mreq mreqv4;
    char *optval;

    if (iface->ss_family == AF_INET6)
    {
        level = IPPROTO_IPV6;
        optname = IPV6_ADD_MEMBERSHIP;
        optval = (char *)&mreqv6;
        optlen = sizeof(mreqv6);

        mreqv6.ipv6mr_multiaddr = ((SOCKADDR_IN6 *)group)->sin6_addr;
        mreqv6.ipv6mr_interface = ((SOCKADDR_IN6 *)iface)->sin6_scope_id;
    }
    else
    {
        level = IPPROTO_IP;
        optname = IP_ADD_MEMBERSHIP;
        optval = (char *)&mreqv4;
        optlen = sizeof(mreqv4);

        mreqv4.imr_multiaddr.s_addr = ((SOCKADDR_IN *)group)->sin_addr.s_addr;
        mreqv4.imr_interface.s_addr = ((SOCKADDR_IN *)iface)->sin_addr.s_addr;
    }

    return setsockopt(s, level, optname, optval, optlen);
}

static int set_send_interface(SOCKET s, SOCKADDR_STORAGE *iface)
{
    int level, optname, optlen;
    char *optval = NULL;

    if (iface->ss_family == AF_INET6)
    {
        level = IPPROTO_IPV6;
        optname = IPV6_MULTICAST_IF;
        optval = (char *) &((SOCKADDR_IN6 *)iface)->sin6_scope_id;
        optlen = sizeof(((SOCKADDR_IN6 *)iface)->sin6_scope_id);
    }
    else
    {
        level = IPPROTO_IP;
        optname = IP_MULTICAST_IF;
        optval = (char *) &((SOCKADDR_IN *)iface)->sin_addr.s_addr;
        optlen = sizeof(((SOCKADDR_IN *)iface)->sin_addr.s_addr);
    }

    return setsockopt(s, level, optname, optval, optlen);
}

typedef struct listener_thread_params
{
    IWSDiscoveryPublisherImpl *impl;
    SOCKET listening_socket;
    BOOL ipv6;
} listener_thread_params;

static HRESULT process_received_message(listener_thread_params *params, char *message, int message_len,
    SOCKADDR_STORAGE *source_addr)
{
    IWSDUdpMessageParameters *msg_params = NULL;
    IWSDUdpAddress *remote_addr = NULL;
    struct notificationSink *sink;
    WSD_SOAP_MESSAGE *msg = NULL;
    int msg_type;
    HRESULT ret;

    ret = read_message(params->impl, message, message_len, &msg, &msg_type);
    if (FAILED(ret)) return ret;

    switch (msg_type)
    {
        case MSGTYPE_PROBE:
            TRACE("Received probe message\n");

            ret = WSDCreateUdpMessageParameters(&msg_params);

            if (FAILED(ret))
            {
                ERR("Unable to create IWSDUdpMessageParameters, not processing message.\n");
                goto cleanup;
            }

            ret = WSDCreateUdpAddress(&remote_addr);

            if (FAILED(ret))
            {
                ERR("Unable to create IWSDUdpAddress, not processing message.\n");
                goto cleanup;
            }

            IWSDUdpAddress_SetSockaddr(remote_addr, source_addr);
            IWSDUdpMessageParameters_SetRemoteAddress(msg_params, (IWSDAddress *)remote_addr);

            EnterCriticalSection(&params->impl->notification_sink_critical_section);

            LIST_FOR_EACH_ENTRY(sink, &params->impl->notificationSinks, struct notificationSink, entry)
            {
                IWSDiscoveryPublisherNotify_ProbeHandler(sink->notificationSink, msg, (IWSDMessageParameters *)msg_params);
            }

            LeaveCriticalSection(&params->impl->notification_sink_critical_section);

            break;
    }

cleanup:
    WSDFreeLinkedMemory(msg);

    if (remote_addr != NULL) IWSDUdpAddress_Release(remote_addr);
    if (msg_params != NULL) IWSDUdpMessageParameters_Release(msg_params);

    return ret;
}

#define RECEIVE_BUFFER_SIZE        65536

static DWORD WINAPI listening_thread(LPVOID params)
{
    listener_thread_params *parameter = (listener_thread_params *)params;
    int bytes_received, address_len, err;
    SOCKADDR_STORAGE source_addr;
    char *buffer;

    buffer = malloc(RECEIVE_BUFFER_SIZE);
    address_len = parameter->ipv6 ? sizeof(SOCKADDR_IN6) : sizeof(SOCKADDR_IN);

    while (parameter->impl->publisherStarted)
    {
        bytes_received = recvfrom(parameter->listening_socket, buffer, RECEIVE_BUFFER_SIZE, 0,
            (LPSOCKADDR) &source_addr, &address_len);

        if (bytes_received == SOCKET_ERROR)
        {
            err = WSAGetLastError();

            if (err != WSAETIMEDOUT)
            {
                WARN("Received error when trying to read from socket: %d. Stopping listener.\n", err);
                return 0;
            }
        }
        else
        {
            process_received_message(parameter, buffer, bytes_received, &source_addr);
        }
    }

    /* The publisher has been stopped */
    closesocket(parameter->listening_socket);

    free(buffer);
    free(parameter);

    return 0;
}

static int start_listening(IWSDiscoveryPublisherImpl *impl, SOCKADDR_STORAGE *bind_address)
{
    SOCKADDR_STORAGE multicast_addr, bind_addr, interface_addr;
    listener_thread_params *parameter = NULL;
    const DWORD receive_timeout = 5000;
    const UINT reuse_addr = 1;
    HANDLE thread_handle;
    int address_length;
    SOCKET s = 0;

    TRACE("(%p, %p) family %d\n", impl, bind_address, bind_address->ss_family);

    /* Populate the multicast address */
    ZeroMemory(&multicast_addr, sizeof(SOCKADDR_STORAGE));

    if (bind_address->ss_family == AF_INET)
    {
        SOCKADDR_IN *sockaddr4 = (SOCKADDR_IN *)&multicast_addr;

        sockaddr4->sin_port = htons(SEND_PORT);
        sockaddr4->sin_addr.S_un.S_addr = htonl(SEND_ADDRESS_IPV4);
        address_length = sizeof(SOCKADDR_IN);
    }
    else
    {
        SOCKADDR_IN6 *sockaddr6 = (SOCKADDR_IN6 *)&multicast_addr;

        sockaddr6->sin6_port = htons(SEND_PORT);
        memcpy(&sockaddr6->sin6_addr, &send_address_ipv6, sizeof(send_address_ipv6));
        address_length = sizeof(SOCKADDR_IN6);
    }

    /* Update the port for the binding address */
    memcpy(&bind_addr, bind_address, address_length);
    ((SOCKADDR_IN *)&bind_addr)->sin_port = htons(SEND_PORT);

    /* Update the port for the interface address */
    memcpy(&interface_addr, bind_address, address_length);
    ((SOCKADDR_IN *)&interface_addr)->sin_port = htons(0);

    /* Create the socket */
    s = socket(bind_address->ss_family, SOCK_DGRAM, IPPROTO_UDP);

    if (s == INVALID_SOCKET)
    {
        WARN("socket() failed (error %d)\n", WSAGetLastError());
        goto cleanup;
    }

    /* Ensure the socket can be reused */
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse_addr, sizeof(reuse_addr)) == SOCKET_ERROR)
    {
        WARN("setsockopt(SO_REUSEADDR) failed (error %d)\n", WSAGetLastError());
        goto cleanup;
    }

    /* Bind the socket to the local interface so we can receive data */
    if (bind(s, (struct sockaddr *)&bind_addr, address_length) == SOCKET_ERROR)
    {
        WARN("bind() failed (error %d)\n", WSAGetLastError());
        goto cleanup;
    }

    /* Join the multicast group */
    if (join_multicast_group(s, &multicast_addr, &interface_addr) == SOCKET_ERROR)
    {
        WARN("Unable to join multicast group (error %d)\n", WSAGetLastError());
        goto cleanup;
    }

    /* Set the outgoing interface */
    if (set_send_interface(s, &interface_addr) == SOCKET_ERROR)
    {
        WARN("Unable to set outgoing interface (error %d)\n", WSAGetLastError());
        goto cleanup;
    }

    /* Set a 5-second receive timeout */
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&receive_timeout, sizeof(receive_timeout)) == SOCKET_ERROR)
    {
        WARN("setsockopt(SO_RCVTIME0) failed (error %d)\n", WSAGetLastError());
        goto cleanup;
    }

    /* Allocate memory for thread parameters */
    parameter = malloc(sizeof(*parameter));

    parameter->impl = impl;
    parameter->listening_socket = s;
    parameter->ipv6 = (bind_address->ss_family == AF_INET6);

    thread_handle = CreateThread(NULL, 0, listening_thread, parameter, 0, NULL);

    if (thread_handle == NULL)
    {
        WARN("CreateThread failed (error %ld)\n", GetLastError());
        goto cleanup;
    }

    impl->thread_handles[impl->num_thread_handles] = thread_handle;
    impl->num_thread_handles++;

    return 1;

cleanup:
    closesocket(s);
    free(parameter);

    return 0;
}

static BOOL start_listening_on_all_addresses(IWSDiscoveryPublisherImpl *impl, ULONG family)
{
    IP_ADAPTER_ADDRESSES *adapter_addresses, *adapter_address;
    int valid_listeners = 0;

    adapter_addresses = get_adapters(family); /* family should be AF_INET or AF_INET6 */
    if (!adapter_addresses)
        return FALSE;

    for (adapter_address = adapter_addresses; adapter_address != NULL; adapter_address = adapter_address->Next)
    {
        if (impl->num_thread_handles >= MAX_WSD_THREADS)
        {
            WARN("Exceeded maximum number of supported listener threads; too many network interfaces.\n");
            goto cleanup;
        }

        if (adapter_address->FirstUnicastAddress == NULL)
        {
            TRACE("No address found for adaptor '%s' (%p)\n", adapter_address->AdapterName, adapter_address);
            continue;
        }

        valid_listeners += start_listening(impl, (SOCKADDR_STORAGE *)adapter_address->FirstUnicastAddress->Address.lpSockaddr);
    }

cleanup:
    free(adapter_addresses);
    return valid_listeners > 0;
}

HRESULT send_udp_unicast(char *data, int length, IWSDUdpAddress *remote_addr, int max_initial_delay)
{
    SOCKADDR_STORAGE address;
    HRESULT ret;
    SOCKET s;

    ZeroMemory(&address, sizeof(SOCKADDR_STORAGE));

    ret = IWSDUdpAddress_GetSockaddr(remote_addr, &address);

    if (FAILED(ret))
    {
        WARN("No sockaddr specified in send_udp_unicast\n");
        return ret;
    }

    /* Create a socket and bind to the adapter address */
    s = socket(address.ss_family, SOCK_DGRAM, IPPROTO_UDP);

    if (s == INVALID_SOCKET)
    {
        int error = WSAGetLastError();
        WARN("Unable to create socket: %d\n", error);
        return HRESULT_FROM_WIN32(error);
    }

    send_message(s, data, length, &address, max_initial_delay, UNICAST_UDP_REPEAT);
    closesocket(s);

    return S_OK;
}

void terminate_networking(IWSDiscoveryPublisherImpl *impl)
{
    BOOL needsCleanup = impl->publisherStarted;
    int i;

    impl->publisherStarted = FALSE;
    WaitForMultipleObjects(impl->num_thread_handles, impl->thread_handles, TRUE, INFINITE);

    for (i = 0; i < impl->num_thread_handles; i++)
    {
        CloseHandle(impl->thread_handles[i]);
    }

    if (needsCleanup)
        WSACleanup();
}

BOOL init_networking(IWSDiscoveryPublisherImpl *impl)
{
    WSADATA wsaData;
    int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (ret != 0)
    {
        WARN("WSAStartup failed with error: %d\n", ret);
        return FALSE;
    }

    impl->publisherStarted = TRUE;

    if ((impl->addressFamily & WSDAPI_ADDRESSFAMILY_IPV4) && (!start_listening_on_all_addresses(impl, AF_INET)))
        goto cleanup;

    if ((impl->addressFamily & WSDAPI_ADDRESSFAMILY_IPV6) && (!start_listening_on_all_addresses(impl, AF_INET6)))
        goto cleanup;

    return TRUE;

cleanup:
    terminate_networking(impl);
    return FALSE;
}
