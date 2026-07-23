/*
 * IP configuration utility
 *
 * Copyright 2008 Andrew Riedi
 * Copyright 2010 Andrew Nguyen
 * Copyright 2026 Pawel Kapeluszny
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

#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>

#include "ipconfig.h"


static int ipconfig_vprintfW(const WCHAR *msg, va_list va_args)
{
    int wlen;
    DWORD count;
    WCHAR msg_buffer[8192];

    wlen = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, msg, 0, 0, msg_buffer,
                          ARRAY_SIZE(msg_buffer), &va_args);

    if (!WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), msg_buffer, wlen, &count, NULL))
    {
        DWORD len;
        char *msgA;

        len = WideCharToMultiByte(GetOEMCP(), 0, msg_buffer, wlen,
            NULL, 0, NULL, NULL);
        msgA = malloc(len);
        if (!msgA)
            return 0;

        WideCharToMultiByte(GetOEMCP(), 0, msg_buffer, wlen, msgA, len, NULL, NULL);
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), msgA, len, &count, FALSE);
        free(msgA);
    }

    return count;
}

static int WINAPIV ipconfig_printfW(const WCHAR *msg, ...)
{
    va_list va_args;
    int len;

    va_start(va_args, msg);
    len = ipconfig_vprintfW(msg, va_args);
    va_end(va_args);

    return len;
}

static int WINAPIV ipconfig_message_printfW(int msg, ...)
{
    va_list va_args;
    WCHAR msg_buffer[8192];
    int len;

    LoadStringW(GetModuleHandleW(NULL), msg, msg_buffer, ARRAY_SIZE(msg_buffer));

    va_start(va_args, msg);
    len = ipconfig_vprintfW(msg_buffer, va_args);
    va_end(va_args);

    return len;
}

static int ipconfig_message(int msg)
{
    WCHAR msg_buffer[8192];

    LoadStringW(GetModuleHandleW(NULL), msg, msg_buffer, ARRAY_SIZE(msg_buffer));

    return ipconfig_printfW(L"%1", msg_buffer);
}

static const WCHAR *iftype_to_string(DWORD type)
{
    static WCHAR msg_buffer[50]; /* NOTE: Static buffer is not thread-safe, but OK for single-threaded CLI */
    int msg;

    switch (type)
    {
    case IF_TYPE_ETHERNET_CSMACD:
    case IF_TYPE_SOFTWARE_LOOPBACK:
        msg = STRING_ETHERNET;
        break;
    default:
        msg = STRING_UNKNOWN;
    }

    LoadStringW(GetModuleHandleW(NULL), msg, msg_buffer, ARRAY_SIZE(msg_buffer));

    return msg_buffer;
}

static void print_field(int msg, const WCHAR *value)
{
    WCHAR field[] = L". . . . . . . . . . . . . . . . . ";
    WCHAR name_buffer[ARRAY_SIZE(field)];

    LoadStringW(GetModuleHandleW(NULL), msg, name_buffer, ARRAY_SIZE(name_buffer));
    memcpy(field, name_buffer, sizeof(WCHAR) * min(lstrlenW(name_buffer), ARRAY_SIZE(field) - 1));

    ipconfig_printfW(L"    %1: %2\n", field, value);
}

static void print_value(const WCHAR *value)
{
    ipconfig_printfW(L"                                        %1\n", value);
}

static BOOL socket_address_to_string(WCHAR *buf, DWORD len, SOCKET_ADDRESS *addr)
{
    /* FIX: Use local variable for length pointer to avoid semantic confusion */
    DWORD buflen = len;
    return WSAAddressToStringW(addr->lpSockaddr,
                               addr->iSockaddrLength, NULL,
                               buf, &buflen) == 0;
}

/* Helper function for calculating and printing the subnet mask */
static void print_subnet_mask(UINT8 prefix_len, ADDRESS_FAMILY family)
{
    WCHAR buf[64];

    if (family == AF_INET)
    {
        ULONG mask = 0;
        /* FIX: Use full socket structure to provide correct sin_family */
        struct sockaddr_in sa; 
        DWORD buf_len = ARRAY_SIZE(buf);

        if (prefix_len > 0 && prefix_len <= 32)
            mask = htonl(prefix_len == 32 ? 0xffffffff : ~(0xffffffff >> prefix_len));
        else if (prefix_len == 0)
            mask = 0;

        /* Critical FIX: Zero memory and set family explicitly */
        memset(&sa, 0, sizeof(sa));
        sa.sin_family = AF_INET;
        sa.sin_addr.S_un.S_addr = mask;

        /* Cast strictly to LPSOCKADDR */
        if (WSAAddressToStringW((LPSOCKADDR)&sa, sizeof(sa), NULL, buf, &buf_len) == 0)
        {
            print_field(STRING_SUBNET_MASK, buf);
        }
    }
}

static IP_ADAPTER_ADDRESSES *get_adapters(void)
{
    ULONG err, size = 4096;
    IP_ADAPTER_ADDRESSES *tmp, *ret;

    if (!(ret = malloc( size ))) return NULL;
    
    err = GetAdaptersAddresses( AF_UNSPEC, GAA_FLAG_INCLUDE_GATEWAYS | GAA_FLAG_INCLUDE_PREFIX, NULL, ret, &size );
    while (err == ERROR_BUFFER_OVERFLOW)
    {
        if (!(tmp = realloc( ret, size ))) break;
        ret = tmp;
        err = GetAdaptersAddresses( AF_UNSPEC, GAA_FLAG_INCLUDE_GATEWAYS | GAA_FLAG_INCLUDE_PREFIX, NULL, ret, &size );
    }
    if (err == ERROR_SUCCESS) return ret;
    free( ret );
    return NULL;
}

static void print_basic_information(void)
{
    IP_ADAPTER_ADDRESSES *adapters, *p;

    adapters = get_adapters();
    /* FIX: replaced exit(1) with return */
    if (!adapters)
        return;

    for (p = adapters; p; p = p->Next)
    {
        IP_ADAPTER_UNICAST_ADDRESS *addr;
        IP_ADAPTER_GATEWAY_ADDRESS_LH *gateway;
        WCHAR addr_buf[54];

        ipconfig_message_printfW(STRING_ADAPTER_FRIENDLY, iftype_to_string(p->IfType), p->FriendlyName);
        ipconfig_printfW(L"\n");
        print_field(STRING_CONN_DNS_SUFFIX, p->DnsSuffix);

        for (addr = p->FirstUnicastAddress; addr; addr = addr->Next)
        {
            if (addr->Address.lpSockaddr->sa_family == AF_INET &&
                socket_address_to_string(addr_buf, ARRAY_SIZE(addr_buf), &addr->Address))
            {
                print_field(STRING_IP_ADDRESS, addr_buf);
                print_subnet_mask(addr->OnLinkPrefixLength, AF_INET);
            }
            else if (addr->Address.lpSockaddr->sa_family == AF_INET6 &&
                     socket_address_to_string(addr_buf, ARRAY_SIZE(addr_buf), &addr->Address))
                print_field(STRING_IP6_ADDRESS, addr_buf);
        }

        if (p->FirstGatewayAddress)
        {
            if (socket_address_to_string(addr_buf, ARRAY_SIZE(addr_buf), &p->FirstGatewayAddress->Address))
                print_field(STRING_DEFAULT_GATEWAY, addr_buf);

            for (gateway = p->FirstGatewayAddress->Next; gateway; gateway = gateway->Next)
            {
                if (socket_address_to_string(addr_buf, ARRAY_SIZE(addr_buf), &gateway->Address))
                    print_value(addr_buf);
            }
        }
        else
            print_field(STRING_DEFAULT_GATEWAY, L"");

        ipconfig_printfW(L"\n");
    }
    free(adapters);
}

static const WCHAR *nodetype_to_string(DWORD type)
{
    static WCHAR msg_buffer[50]; /* NOTE: Static buffer */
    int msg;

    switch (type)
    {
    case BROADCAST_NODETYPE:
        msg = STRING_BROADCAST;
        break;
    case PEER_TO_PEER_NODETYPE:
        msg = STRING_PEER_TO_PEER;
        break;
    case MIXED_NODETYPE:
        msg = STRING_MIXED;
        break;
    case HYBRID_NODETYPE:
        msg = STRING_HYBRID;
        break;
    default:
        msg = STRING_UNKNOWN;
    }

    LoadStringW(GetModuleHandleW(NULL), msg, msg_buffer, ARRAY_SIZE(msg_buffer));

    return msg_buffer;
}

static WCHAR *physaddr_to_string(WCHAR *buf, BYTE *addr, DWORD len)
{
    if (!len)
        *buf = '\0';
    else
    {
        WCHAR *p = buf;
        DWORD i;

        for (i = 0; i < len - 1; i++)
        {
            swprintf(p, 4, L"%02X-", addr[i]);
            p += 3;
        }
        swprintf(p, 3, L"%02X", addr[i]);
    }

    return buf;
}

static const WCHAR *boolean_to_string(int value)
{
    static WCHAR msg_buffer[15]; /* NOTE: Static buffer */

    LoadStringW(GetModuleHandleW(NULL), value ? STRING_YES : STRING_NO,
        msg_buffer, ARRAY_SIZE(msg_buffer));

    return msg_buffer;
}

static void print_full_information(void)
{
    FIXED_INFO *info;
    IP_ADAPTER_ADDRESSES *adapters, *p;
    ULONG out;

    if (GetNetworkParams(NULL, &out) == ERROR_BUFFER_OVERFLOW)
    {
        info = malloc(out);
        /* FIX: replaced exit(1) with return */
        if (!info)
            return;

        if (GetNetworkParams(info, &out) == ERROR_SUCCESS)
        {
            WCHAR hostnameW[MAX_HOSTNAME_LEN + 4];
            WCHAR dnssuffixW[MAX_DOMAIN_NAME_LEN + 4];

            MultiByteToWideChar(CP_ACP, 0, info->HostName, -1, hostnameW, ARRAY_SIZE(hostnameW));
            print_field(STRING_HOSTNAME, hostnameW);

            MultiByteToWideChar(CP_ACP, 0, info->DomainName, -1, dnssuffixW, ARRAY_SIZE(dnssuffixW));
            print_field(STRING_PRIMARY_DNS_SUFFIX, dnssuffixW);

            print_field(STRING_NODE_TYPE, nodetype_to_string(info->NodeType));
            print_field(STRING_IP_ROUTING, boolean_to_string(info->EnableRouting));

            print_field(STRING_WINS_PROXY_ENABLED, boolean_to_string(info->EnableProxy));

            ipconfig_printfW(L"\n");
        }

        free(info);
    }

    adapters = get_adapters();
    /* FIX: replaced exit(1) with return */
    if (!adapters)
        return;

    for (p = adapters; p; p = p->Next)
    {
        IP_ADAPTER_UNICAST_ADDRESS *addr;
        WCHAR physaddr_buf[3 * MAX_ADAPTER_ADDRESS_LENGTH], addr_buf[54];
        IP_ADAPTER_GATEWAY_ADDRESS_LH *gateway;
        IP_ADAPTER_DNS_SERVER_ADDRESS_XP *dhcp;

        ipconfig_message_printfW(STRING_ADAPTER_FRIENDLY, iftype_to_string(p->IfType), p->FriendlyName);
        ipconfig_printfW(L"\n");
        print_field(STRING_CONN_DNS_SUFFIX, p->DnsSuffix);
        print_field(STRING_DESCRIPTION, p->Description);
        print_field(STRING_PHYS_ADDR, physaddr_to_string(physaddr_buf, p->PhysicalAddress, p->PhysicalAddressLength));
        print_field(STRING_DHCP_ENABLED, boolean_to_string(p->Flags & IP_ADAPTER_DHCP_ENABLED));

        for (addr = p->FirstUnicastAddress; addr; addr = addr->Next)
        {
            if (addr->Address.lpSockaddr->sa_family == AF_INET &&
                socket_address_to_string(addr_buf, ARRAY_SIZE(addr_buf), &addr->Address))
            {
                print_field(STRING_IP_ADDRESS, addr_buf);
                print_subnet_mask(addr->OnLinkPrefixLength, AF_INET);
            }
            else if (addr->Address.lpSockaddr->sa_family == AF_INET6 &&
                     socket_address_to_string(addr_buf, ARRAY_SIZE(addr_buf), &addr->Address))
                print_field(STRING_IP6_ADDRESS, addr_buf);
        }

        if (p->FirstGatewayAddress)
        {
            if (socket_address_to_string(addr_buf, ARRAY_SIZE(addr_buf), &p->FirstGatewayAddress->Address))
                print_field(STRING_DEFAULT_GATEWAY, addr_buf);

            for (gateway = p->FirstGatewayAddress->Next; gateway; gateway = gateway->Next)
            {
                if (socket_address_to_string(addr_buf, ARRAY_SIZE(addr_buf), &gateway->Address))
                    print_value(addr_buf);
            }
        }
        else
            print_field(STRING_DEFAULT_GATEWAY, L"");

        if (p->Flags & IP_ADAPTER_DHCP_ENABLED && p->FirstDhcpServerAddress)
        {
            if (socket_address_to_string(addr_buf, ARRAY_SIZE(addr_buf), &p->FirstDhcpServerAddress->Address))
                print_field(STRING_DHCP_SERVER, addr_buf);

            for (dhcp = p->FirstDhcpServerAddress->Next; dhcp; dhcp = dhcp->Next)
            {
                if (socket_address_to_string(addr_buf, ARRAY_SIZE(addr_buf), &dhcp->Address))
                    print_value(addr_buf);
            }
        }

        ipconfig_printfW(L"\n");
    }
    free(adapters);
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    WSADATA data;

    if (WSAStartup(MAKEWORD(2, 2), &data))
        return 1;

    if (argc > 1)
    {
        if (!lstrcmpW(L"/?", argv[1]))
        {
            ipconfig_message(STRING_USAGE);
            WSACleanup();
            return 1;
        }
        else if (!wcsicmp(L"/all", argv[1]))
        {
            if (argv[2])
            {
                ipconfig_message(STRING_INVALID_CMDLINE);
                ipconfig_message(STRING_USAGE);
                WSACleanup();
                return 1;
            }

            print_full_information();
        }
        else
        {
            ipconfig_message(STRING_INVALID_CMDLINE);
            ipconfig_message(STRING_USAGE);
            WSACleanup();
            return 1;
        }
    }
    else
        print_basic_information();

    WSACleanup();
    return 0;
}
