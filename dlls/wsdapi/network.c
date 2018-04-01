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
#include "wine/heap.h"
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

    heap_free(params->data);
    heap_free(params);

    return 0;
}

BOOL send_udp_multicast_of_type(char *data, int length, int max_initial_delay, ULONG family)
{
    IP_ADAPTER_ADDRESSES *adapter_addresses = NULL, *adapter_addr;
    static const struct in6_addr i_addr_zero;
    sending_thread_params *send_params;
    ULONG bufferSize = 0;
    LPSOCKADDR sockaddr;
    BOOL ret = FALSE;
    HANDLE thread_handle;
    const char ttl = 8;
    ULONG retval;
    SOCKET s;

    /* Get size of buffer for adapters */
    retval = GetAdaptersAddresses(family, 0, NULL, NULL, &bufferSize);

    if (retval != ERROR_BUFFER_OVERFLOW)
    {
        WARN("GetAdaptorsAddresses failed with error %08x\n", retval);
        goto cleanup;
    }

    adapter_addresses = (IP_ADAPTER_ADDRESSES *) heap_alloc(bufferSize);

    if (adapter_addresses == NULL)
    {
        WARN("Out of memory allocating space for adapter information\n");
        goto cleanup;
    }

    /* Get list of adapters */
    retval = GetAdaptersAddresses(family, 0, NULL, adapter_addresses, &bufferSize);

    if (retval != ERROR_SUCCESS)
    {
        WARN("GetAdaptorsAddresses failed with error %08x\n", retval);
        goto cleanup;
    }

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
        send_params = heap_alloc(sizeof(*send_params));

        send_params->data = heap_alloc(length);
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
            WARN("CreateThread failed (error %d)\n", GetLastError());
            closesocket(s);

            heap_free(send_params->data);
            heap_free(send_params);

            continue;
        }

        CloseHandle(thread_handle);
    }

    ret = TRUE;

cleanup:
    heap_free(adapter_addresses);

    return ret;
}

BOOL send_udp_multicast(IWSDiscoveryPublisherImpl *impl, char *data, int length, int max_initial_delay)
{
    if ((impl->addressFamily & WSDAPI_ADDRESSFAMILY_IPV4) &&
        (!send_udp_multicast_of_type(data, length,max_initial_delay, AF_INET))) return FALSE;

    if ((impl->addressFamily & WSDAPI_ADDRESSFAMILY_IPV6) &&
        (!send_udp_multicast_of_type(data, length, max_initial_delay, AF_INET6))) return FALSE;

    return TRUE;
}

void terminate_networking(IWSDiscoveryPublisherImpl *impl)
{
    BOOL needsCleanup = impl->publisherStarted;

    impl->publisherStarted = FALSE;

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

    /* TODO: Start listening */
    return TRUE;
}
