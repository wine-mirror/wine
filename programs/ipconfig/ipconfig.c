/*
 * IP configuration utility
 *
 * Copyright 2008 Andrew Riedi
 * Copyright 2010 Andrew Nguyen
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

#define NONAMELESSUNION

#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>

#include "ipconfig.h"

static int ipconfig_vprintfW(const WCHAR *msg, __ms_va_list va_args)
{
    int wlen;
    DWORD count, ret;
    WCHAR msg_buffer[8192];

    wlen = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, msg, 0, 0, msg_buffer,
                          ARRAY_SIZE(msg_buffer), &va_args);

    ret = WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), msg_buffer, wlen, &count, NULL);
    if (!ret)
    {
        DWORD len;
        char *msgA;

        /* On Windows WriteConsoleW() fails if the output is redirected. So fall
         * back to WriteFile(), assuming the console encoding is still the right
         * one in that case.
         */
        len = WideCharToMultiByte(GetConsoleOutputCP(), 0, msg_buffer, wlen,
            NULL, 0, NULL, NULL);
        msgA = HeapAlloc(GetProcessHeap(), 0, len);
        if (!msgA)
            return 0;

        WideCharToMultiByte(GetConsoleOutputCP(), 0, msg_buffer, wlen, msgA, len,
            NULL, NULL);
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), msgA, len, &count, FALSE);
        HeapFree(GetProcessHeap(), 0, msgA);
    }

    return count;
}

static int WINAPIV ipconfig_printfW(const WCHAR *msg, ...)
{
    __ms_va_list va_args;
    int len;

    __ms_va_start(va_args, msg);
    len = ipconfig_vprintfW(msg, va_args);
    __ms_va_end(va_args);

    return len;
}

static int WINAPIV ipconfig_message_printfW(int msg, ...)
{
    __ms_va_list va_args;
    WCHAR msg_buffer[8192];
    int len;

    LoadStringW(GetModuleHandleW(NULL), msg, msg_buffer, ARRAY_SIZE(msg_buffer));

    __ms_va_start(va_args, msg);
    len = ipconfig_vprintfW(msg_buffer, va_args);
    __ms_va_end(va_args);

    return len;
}

static int ipconfig_message(int msg)
{
    static const WCHAR formatW[] = {'%','1',0};
    WCHAR msg_buffer[8192];

    LoadStringW(GetModuleHandleW(NULL), msg, msg_buffer, ARRAY_SIZE(msg_buffer));

    return ipconfig_printfW(formatW, msg_buffer);
}

static const WCHAR *iftype_to_string(DWORD type)
{
    static WCHAR msg_buffer[50];

    int msg;

    switch (type)
    {
    case IF_TYPE_ETHERNET_CSMACD:
    /* The loopback adapter appears as an Ethernet device. */
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
    static const WCHAR formatW[] = {' ',' ',' ',' ','%','1',':',' ','%','2','\n',0};

    WCHAR field[] = {'.',' ','.',' ','.',' ','.',' ','.',' ','.',' ','.',' ','.',' ','.',
                     ' ','.',' ','.',' ','.',' ','.',' ','.',' ','.',' ','.',' ','.',' ',0};
    WCHAR name_buffer[ARRAY_SIZE(field)];

    LoadStringW(GetModuleHandleW(NULL), msg, name_buffer, ARRAY_SIZE(name_buffer));
    memcpy(field, name_buffer, sizeof(WCHAR) * min(lstrlenW(name_buffer), ARRAY_SIZE(field) - 1));

    ipconfig_printfW(formatW, field, value);
}

static void print_value(const WCHAR *value)
{
    static const WCHAR formatW[] = {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
                                    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
                                    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
                                    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
                                    '%','1','\n',0};

    ipconfig_printfW(formatW, value);
}

static BOOL socket_address_to_string(WCHAR *buf, DWORD len, SOCKET_ADDRESS *addr)
{
    return WSAAddressToStringW(addr->lpSockaddr,
                               addr->iSockaddrLength, NULL,
                               buf, &len) == 0;
}

static void print_basic_information(void)
{
    IP_ADAPTER_ADDRESSES *adapters;
    ULONG out = 0;

    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_GATEWAYS,
                             NULL, NULL, &out) == ERROR_BUFFER_OVERFLOW)
    {
        adapters = HeapAlloc(GetProcessHeap(), 0, out);
        if (!adapters)
            exit(1);

        if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_GATEWAYS,
                                 NULL, adapters, &out) == ERROR_SUCCESS)
        {
            IP_ADAPTER_ADDRESSES *p;

            for (p = adapters; p; p = p->Next)
            {
                static const WCHAR newlineW[] = {'\n',0};
                static const WCHAR emptyW[] = {0};

                IP_ADAPTER_UNICAST_ADDRESS *addr;
                IP_ADAPTER_GATEWAY_ADDRESS_LH *gateway;
                WCHAR addr_buf[54];

                ipconfig_message_printfW(STRING_ADAPTER_FRIENDLY, iftype_to_string(p->IfType), p->FriendlyName);
                ipconfig_printfW(newlineW);
                print_field(STRING_CONN_DNS_SUFFIX, p->DnsSuffix);

                for (addr = p->FirstUnicastAddress; addr; addr = addr->Next)
                {
                    if (addr->Address.lpSockaddr->sa_family == AF_INET &&
                        socket_address_to_string(addr_buf, ARRAY_SIZE(addr_buf), &addr->Address))
                        print_field(STRING_IP_ADDRESS, addr_buf);
                    else if (addr->Address.lpSockaddr->sa_family == AF_INET6 &&
                             socket_address_to_string(addr_buf, ARRAY_SIZE(addr_buf), &addr->Address))
                        print_field(STRING_IP6_ADDRESS, addr_buf);
                    /* FIXME: Output corresponding subnet mask. */
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
                    print_field(STRING_DEFAULT_GATEWAY, emptyW);

                ipconfig_printfW(newlineW);
            }
        }

        HeapFree(GetProcessHeap(), 0, adapters);
    }
}

static const WCHAR *nodetype_to_string(DWORD type)
{
    static WCHAR msg_buffer[50];

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
    static const WCHAR fmtW[] = {'%','0','2','X','-',0};
    static const WCHAR fmt2W[] = {'%','0','2','X',0};

    if (!len)
        *buf = '\0';
    else
    {
        WCHAR *p = buf;
        DWORD i;

        for (i = 0; i < len - 1; i++)
        {
            swprintf(p, 4, fmtW, addr[i]);
            p += 3;
        }
        swprintf(p, 3, fmt2W, addr[i]);
    }

    return buf;
}

static const WCHAR *boolean_to_string(int value)
{
    static WCHAR msg_buffer[15];

    LoadStringW(GetModuleHandleW(NULL), value ? STRING_YES : STRING_NO,
        msg_buffer, ARRAY_SIZE(msg_buffer));

    return msg_buffer;
}

static void print_full_information(void)
{
    static const WCHAR newlineW[] = {'\n',0};
    static const WCHAR emptyW[] = {0};

    FIXED_INFO *info;
    IP_ADAPTER_ADDRESSES *adapters;
    ULONG out = 0;

    if (GetNetworkParams(NULL, &out) == ERROR_BUFFER_OVERFLOW)
    {
        info = HeapAlloc(GetProcessHeap(), 0, out);
        if (!info)
            exit(1);

        if (GetNetworkParams(info, &out) == ERROR_SUCCESS)
        {
            WCHAR hostnameW[MAX_HOSTNAME_LEN + 4];

            MultiByteToWideChar(CP_ACP, 0, info->HostName, -1, hostnameW, ARRAY_SIZE(hostnameW));
            print_field(STRING_HOSTNAME, hostnameW);

            /* FIXME: Output primary DNS suffix. */

            print_field(STRING_NODE_TYPE, nodetype_to_string(info->NodeType));
            print_field(STRING_IP_ROUTING, boolean_to_string(info->EnableRouting));

            /* FIXME: Output WINS proxy status and DNS suffix search list. */

            ipconfig_printfW(newlineW);
        }

        HeapFree(GetProcessHeap(), 0, info);
    }

    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_GATEWAYS,
                             NULL, NULL, &out) == ERROR_BUFFER_OVERFLOW)
    {
        adapters = HeapAlloc(GetProcessHeap(), 0, out);
        if (!adapters)
            exit(1);

        if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_GATEWAYS,
                                 NULL, adapters, &out) == ERROR_SUCCESS)
        {
            IP_ADAPTER_ADDRESSES *p;

            for (p = adapters; p; p = p->Next)
            {
                IP_ADAPTER_UNICAST_ADDRESS *addr;
                WCHAR physaddr_buf[3 * MAX_ADAPTER_ADDRESS_LENGTH];
                IP_ADAPTER_GATEWAY_ADDRESS_LH *gateway;
                WCHAR addr_buf[54];

                ipconfig_message_printfW(STRING_ADAPTER_FRIENDLY, iftype_to_string(p->IfType), p->FriendlyName);
                ipconfig_printfW(newlineW);
                print_field(STRING_CONN_DNS_SUFFIX, p->DnsSuffix);
                print_field(STRING_DESCRIPTION, p->Description);
                print_field(STRING_PHYS_ADDR, physaddr_to_string(physaddr_buf, p->PhysicalAddress, p->PhysicalAddressLength));
                print_field(STRING_DHCP_ENABLED, boolean_to_string(p->u1.Flags & IP_ADAPTER_DHCP_ENABLED));

                /* FIXME: Output autoconfiguration status. */

                for (addr = p->FirstUnicastAddress; addr; addr = addr->Next)
                {
                    if (addr->Address.lpSockaddr->sa_family == AF_INET &&
                        socket_address_to_string(addr_buf, ARRAY_SIZE(addr_buf), &addr->Address))
                        print_field(STRING_IP_ADDRESS, addr_buf);
                    else if (addr->Address.lpSockaddr->sa_family == AF_INET6 &&
                             socket_address_to_string(addr_buf, ARRAY_SIZE(addr_buf), &addr->Address))
                        print_field(STRING_IP6_ADDRESS, addr_buf);
                    /* FIXME: Output corresponding subnet mask. */
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
                    print_field(STRING_DEFAULT_GATEWAY, emptyW);

                ipconfig_printfW(newlineW);
            }
        }

        HeapFree(GetProcessHeap(), 0, adapters);
    }
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    static const WCHAR slashHelp[] = {'/','?',0};
    static const WCHAR slashAll[] = {'/','a','l','l',0};

    WSADATA data;

    if (WSAStartup(MAKEWORD(2, 2), &data))
        return 1;

    if (argc > 1)
    {
        if (!lstrcmpW(slashHelp, argv[1]))
        {
            ipconfig_message(STRING_USAGE);
            WSACleanup();
            return 1;
        }
        else if (!wcsicmp(slashAll, argv[1]))
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
