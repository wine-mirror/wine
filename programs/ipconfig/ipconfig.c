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

#include <windows.h>
#include <iphlpapi.h>
#include <wine/debug.h>
#include <wine/unicode.h>

#include "ipconfig.h"

WINE_DEFAULT_DEBUG_CHANNEL(ipconfig);

static int ipconfig_vprintfW(const WCHAR *msg, va_list va_args)
{
    int wlen;
    DWORD count, ret;
    WCHAR msg_buffer[8192];

    wlen = vsprintfW(msg_buffer, msg, va_args);

    ret = WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), msg_buffer, wlen, &count, NULL);
    if (!ret)
    {
        DWORD len;
        char *msgA;

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

static int ipconfig_printfW(const WCHAR *msg, ...)
{
    va_list va_args;
    int len;

    va_start(va_args, msg);
    len = ipconfig_vprintfW(msg, va_args);
    va_end(va_args);

    return len;
}

static int ipconfig_message_printfW(int msg, ...)
{
    va_list va_args;
    WCHAR msg_buffer[8192];
    int len;

    LoadStringW(GetModuleHandleW(NULL), msg, msg_buffer,
        sizeof(msg_buffer)/sizeof(WCHAR));

    va_start(va_args, msg);
    len = ipconfig_vprintfW(msg_buffer, va_args);
    va_end(va_args);

    return len;
}

static int ipconfig_message(int msg)
{
    static const WCHAR formatW[] = {'%','s',0};
    WCHAR msg_buffer[8192];

    LoadStringW(GetModuleHandleW(NULL), msg, msg_buffer,
        sizeof(msg_buffer)/sizeof(WCHAR));

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

    LoadStringW(GetModuleHandleW(NULL), msg, msg_buffer,
        sizeof(msg_buffer)/sizeof(WCHAR));

    return msg_buffer;
}

static void print_field(int msg, const WCHAR *value)
{
    static const WCHAR formatW[] = {' ',' ',' ',' ','%','s',':',' ','%','s','\n',0};

    WCHAR field[] = {'.',' ','.',' ','.',' ','.',' ','.',' ','.',' ','.',' ','.',' ','.',
                     ' ','.',' ','.',' ','.',' ','.',' ','.',' ','.',' ','.',' ','.',' ',0};
    WCHAR name_buffer[sizeof(field)/sizeof(WCHAR)];

    LoadStringW(GetModuleHandleW(NULL), msg, name_buffer, sizeof(name_buffer)/sizeof(WCHAR));
    memcpy(field, name_buffer, sizeof(WCHAR) * min(strlenW(name_buffer), sizeof(field)/sizeof(WCHAR) - 1));

    ipconfig_printfW(formatW, field, value);
}

static void print_basic_information(void)
{
    IP_ADAPTER_ADDRESSES *adapters;
    ULONG out = 0;

    if (GetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, &out) == ERROR_BUFFER_OVERFLOW)
    {
        adapters = HeapAlloc(GetProcessHeap(), 0, out);
        if (!adapters)
            exit(1);

        if (GetAdaptersAddresses(AF_UNSPEC, 0, NULL, adapters, &out) == ERROR_SUCCESS)
        {
            IP_ADAPTER_ADDRESSES *p;

            for (p = adapters; p; p = p->Next)
            {
                static const WCHAR newlineW[] = {'\n',0};

                IP_ADAPTER_UNICAST_ADDRESS *addr;

                ipconfig_message_printfW(STRING_ADAPTER_FRIENDLY, iftype_to_string(p->IfType), p->FriendlyName);
                ipconfig_printfW(newlineW);
                print_field(STRING_CONN_DNS_SUFFIX, p->DnsSuffix);

                for (addr = p->FirstUnicastAddress; addr; addr = addr->Next)
                {
                    WCHAR addr_buf[54];
                    DWORD len = sizeof(addr_buf)/sizeof(WCHAR);

                    if (WSAAddressToStringW(addr->Address.lpSockaddr,
                                            addr->Address.iSockaddrLength, NULL,
                                            addr_buf, &len) == 0)
                        print_field(STRING_IP_ADDRESS, addr_buf);
                    /* FIXME: Output corresponding subnet mask. */
                }

                /* FIXME: Output default gateway address. */
                ipconfig_printfW(newlineW);
            }
        }

        HeapFree(GetProcessHeap(), 0, adapters);
    }
}

int wmain(int argc, WCHAR *argv[])
{
    static const WCHAR slashHelp[] = {'/','?',0};
    static const WCHAR slashAll[] = {'/','a','l','l',0};

    WSADATA data;

    if (WSAStartup(MAKEWORD(2, 2), &data))
        return 1;

    if (argc > 1)
    {
        if (!strcmpW(slashHelp, argv[1]))
        {
            ipconfig_message(STRING_USAGE);
            WSACleanup();
            return 1;
        }
        else if (!strcmpiW(slashAll, argv[1]))
        {
            if (argv[2])
            {
                ipconfig_message(STRING_INVALID_CMDLINE);
                ipconfig_message(STRING_USAGE);
                WSACleanup();
                return 1;
            }

            WINE_FIXME("/all option is not currently handled\n");
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
