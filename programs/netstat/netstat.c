/*
 * Copyright 2011-2013 André Hentschel
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

#include <stdio.h>
#include "netstat.h"
#include <winsock2.h>
#include <iphlpapi.h>
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(netstat);

static const WCHAR tcpstatesW[][16] = {
    L"???",
    L"CLOSED",
    L"LISTENING",
    L"SYN_SENT",
    L"SYN_RCVD",
    L"ESTABLISHED",
    L"FIN_WAIT1",
    L"FIN_WAIT2",
    L"CLOSE_WAIT",
    L"CLOSING",
    L"LAST_ACK",
    L"TIME_WAIT",
    L"DELETE_TCB",
};

/* =========================================================================
 *  Output a unicode string. Ideally this will go to the console
 *  and hence required WriteConsoleW to output it, however if file i/o is
 *  redirected, it needs to be WriteFile'd using OEM (not ANSI) format
 * ========================================================================= */
static int WINAPIV NETSTAT_wprintf(const WCHAR *format, ...)
{
    static WCHAR *output_bufW = NULL;
    static char  *output_bufA = NULL;
    static BOOL  toConsole    = TRUE;
    static BOOL  traceOutput  = FALSE;
#define MAX_WRITECONSOLE_SIZE 65535

    va_list parms;
    DWORD   nOut;
    int len;
    DWORD   res = 0;

    /*
     * Allocate buffer to use when writing to console
     * Note: Not freed - memory will be allocated once and released when
     * netstat ends
     */

    if (!output_bufW) output_bufW = HeapAlloc(GetProcessHeap(), 0,
                                              MAX_WRITECONSOLE_SIZE*sizeof(WCHAR));
    if (!output_bufW) {
        WINE_FIXME("Out of memory - could not allocate 2 x 64 KB buffers\n");
        return 0;
    }

    va_start(parms, format);
    len = wvsprintfW(output_bufW, format, parms);
    va_end(parms);

    /* Try to write as unicode all the time we think it's a console */
    if (toConsole) {
        res = WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),
                            output_bufW, len, &nOut, NULL);
    }

    /* If writing to console has failed (ever) we assume it's file
       i/o so convert to OEM codepage and output                  */
    if (!res) {
        BOOL usedDefaultChar = FALSE;
        DWORD convertedChars;

        toConsole = FALSE;

        /*
         * Allocate buffer to use when writing to file. Not freed, as above
         */
        if (!output_bufA) output_bufA = HeapAlloc(GetProcessHeap(), 0,
                                                  MAX_WRITECONSOLE_SIZE);
        if (!output_bufA) {
            WINE_FIXME("Out of memory - could not allocate 2 x 64 KB buffers\n");
            return 0;
        }

        /* Convert to OEM, then output */
        convertedChars = WideCharToMultiByte(GetOEMCP(), 0, output_bufW,
                                             len, output_bufA, MAX_WRITECONSOLE_SIZE,
                                             "?", &usedDefaultChar);
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), output_bufA, convertedChars,
                  &nOut, FALSE);
    }

    /* Trace whether screen or console */
    if (!traceOutput) {
        WINE_TRACE("Writing to console? (%d)\n", toConsole);
        traceOutput = TRUE;
    }
    return nOut;
}

static WCHAR *NETSTAT_load_message(UINT id) {
    static WCHAR msg[2048];

    if (!LoadStringW(GetModuleHandleW(NULL), id, msg, ARRAY_SIZE(msg))) {
        WINE_FIXME("LoadString failed with %ld\n", GetLastError());
        lstrcpyW(msg, L"Failed!");
    }
    return msg;
}

static WCHAR *NETSTAT_port_name(UINT port, WCHAR name[])
{
    /* FIXME: can we get the name? */
    swprintf(name, 32, L"%d", htons((WORD)port));
    return name;
}

static WCHAR *NETSTAT_host_name(UINT ip, WCHAR name[])
{
    UINT nip;

    /* FIXME: can we get the name? */
    nip = htonl(ip);
    swprintf(name, MAX_HOSTNAME_LEN, L"%d.%d.%d.%d",
             (nip >> 24) & 0xFF, (nip >> 16) & 0xFF, (nip >> 8) & 0xFF, (nip) & 0xFF);
    return name;
}

static void NETSTAT_conn_header(void)
{
    WCHAR local[22], remote[22], state[22];
    NETSTAT_wprintf(L"\n%s\n\n", NETSTAT_load_message(IDS_TCP_ACTIVE_CONN));
    lstrcpyW(local, NETSTAT_load_message(IDS_TCP_LOCAL_ADDR));
    lstrcpyW(remote, NETSTAT_load_message(IDS_TCP_REMOTE_ADDR));
    lstrcpyW(state, NETSTAT_load_message(IDS_TCP_STATE));
    NETSTAT_wprintf(L"  %-6s %-22s %-22s %s\n", NETSTAT_load_message(IDS_TCP_PROTO), local, remote, state);
}

static void NETSTAT_eth_stats(void)
{
    PMIB_IFTABLE table;
    DWORD err, size, i;
    DWORD octets[2], ucastpkts[2], nucastpkts[2], discards[2], errors[2], unknown;
    WCHAR recv[19];

    size = sizeof(MIB_IFTABLE);
    do
    {
        table = HeapAlloc(GetProcessHeap(), 0, size);
        err = GetIfTable(table, &size, FALSE);
        if (err != NO_ERROR) HeapFree(GetProcessHeap(), 0, table);
    } while (err == ERROR_INSUFFICIENT_BUFFER);

    if (err) return;

    NETSTAT_wprintf(NETSTAT_load_message(IDS_ETH_STAT));
    NETSTAT_wprintf(L"\n\n");
    lstrcpyW(recv, NETSTAT_load_message(IDS_ETH_RECV));
    NETSTAT_wprintf(L"                           %-19s %s\n\n", recv, NETSTAT_load_message(IDS_ETH_SENT));

    octets[0] = octets[1] = 0;
    ucastpkts[0] = ucastpkts[1] = 0;
    nucastpkts[0] = nucastpkts[1] = 0;
    discards[0] = discards[1] = 0;
    errors[0] = errors[1] = 0;
    unknown = 0;

    for (i = 0; i < table->dwNumEntries; i++)
    {
        octets[0] += table->table[i].dwInOctets;
        octets[1] += table->table[i].dwOutOctets;
        ucastpkts[0] += table->table[i].dwInUcastPkts;
        ucastpkts[1] += table->table[i].dwOutUcastPkts;
        nucastpkts[0] += table->table[i].dwInNUcastPkts;
        nucastpkts[1] += table->table[i].dwOutNUcastPkts;
        discards[0] += table->table[i].dwInDiscards;
        discards[1] += table->table[i].dwOutDiscards;
        errors[0] += table->table[i].dwInErrors;
        errors[1] += table->table[i].dwOutErrors;
        unknown += table->table[i].dwInUnknownProtos;
    }

    NETSTAT_wprintf(L"%-20s %14lu %15lu\n", NETSTAT_load_message(IDS_ETH_BYTES), octets[0], octets[1]);
    NETSTAT_wprintf(L"%-20s %14lu %15lu\n", NETSTAT_load_message(IDS_ETH_UNICAST), ucastpkts[0], ucastpkts[1]);
    NETSTAT_wprintf(L"%-20s %14lu %15lu\n", NETSTAT_load_message(IDS_ETH_NUNICAST), nucastpkts[0], nucastpkts[1]);
    NETSTAT_wprintf(L"%-20s %14lu %15lu\n", NETSTAT_load_message(IDS_ETH_DISCARDS), discards[0], discards[1]);
    NETSTAT_wprintf(L"%-20s %14lu %15lu\n", NETSTAT_load_message(IDS_ETH_ERRORS), errors[0], errors[1]);
    NETSTAT_wprintf(L"%-20s %14lu\n\n", NETSTAT_load_message(IDS_ETH_UNKNOWN), unknown);

    HeapFree(GetProcessHeap(), 0, table);
}

static void NETSTAT_tcp_table(void)
{
    PMIB_TCPTABLE table;
    DWORD err, size, i;
    WCHAR HostIp[MAX_HOSTNAME_LEN], HostPort[32];
    WCHAR RemoteIp[MAX_HOSTNAME_LEN], RemotePort[32];
    WCHAR Host[MAX_HOSTNAME_LEN + 32];
    WCHAR Remote[MAX_HOSTNAME_LEN + 32];

    size = sizeof(MIB_TCPTABLE);
    do
    {
        table = HeapAlloc(GetProcessHeap(), 0, size);
        err = GetTcpTable(table, &size, TRUE);
        if (err != NO_ERROR) HeapFree(GetProcessHeap(), 0, table);
    } while (err == ERROR_INSUFFICIENT_BUFFER);

    if (err) return;

    for (i = 0; i < table->dwNumEntries; i++)
    {
        if ((table->table[i].dwState ==  MIB_TCP_STATE_CLOSE_WAIT) ||
            (table->table[i].dwState ==  MIB_TCP_STATE_ESTAB) ||
            (table->table[i].dwState ==  MIB_TCP_STATE_TIME_WAIT))
        {
            NETSTAT_host_name(table->table[i].dwLocalAddr, HostIp);
            NETSTAT_port_name(table->table[i].dwLocalPort, HostPort);
            NETSTAT_host_name(table->table[i].dwRemoteAddr, RemoteIp);
            NETSTAT_port_name(table->table[i].dwRemotePort, RemotePort);

            swprintf(Host, ARRAY_SIZE(Host), L"%s:%s", HostIp, HostPort);
            swprintf(Remote, ARRAY_SIZE(Remote), L"%s:%s", RemoteIp, RemotePort);
            NETSTAT_wprintf(L"  %-6s %-22s %-22s %s\n", L"TCP", Host, Remote, tcpstatesW[table->table[i].dwState]);
        }
    }
    HeapFree(GetProcessHeap(), 0, table);
}

static void NETSTAT_tcp_stats(void)
{
    MIB_TCPSTATS stats;

    if (GetTcpStatistics(&stats) == NO_ERROR)
    {
        NETSTAT_wprintf(L"\n%s\n\n", NETSTAT_load_message(IDS_TCP_STAT));
        NETSTAT_wprintf(L"  %-35s = %lu\n", NETSTAT_load_message(IDS_TCP_ACTIVE_OPEN), stats.dwActiveOpens);
        NETSTAT_wprintf(L"  %-35s = %lu\n", NETSTAT_load_message(IDS_TCP_PASSIV_OPEN), stats.dwPassiveOpens);
        NETSTAT_wprintf(L"  %-35s = %lu\n", NETSTAT_load_message(IDS_TCP_FAILED_CONN), stats.dwAttemptFails);
        NETSTAT_wprintf(L"  %-35s = %lu\n", NETSTAT_load_message(IDS_TCP_RESET_CONN),  stats.dwEstabResets);
        NETSTAT_wprintf(L"  %-35s = %lu\n", NETSTAT_load_message(IDS_TCP_CURR_CONN),   stats.dwCurrEstab);
        NETSTAT_wprintf(L"  %-35s = %lu\n", NETSTAT_load_message(IDS_TCP_SEGM_RECV),   stats.dwInSegs);
        NETSTAT_wprintf(L"  %-35s = %lu\n", NETSTAT_load_message(IDS_TCP_SEGM_SENT),   stats.dwOutSegs);
        NETSTAT_wprintf(L"  %-35s = %lu\n", NETSTAT_load_message(IDS_TCP_SEGM_RETRAN), stats.dwRetransSegs);
    }
}

static void NETSTAT_udp_table(void)
{
    PMIB_UDPTABLE table;
    DWORD err, size, i;
    WCHAR HostIp[MAX_HOSTNAME_LEN], HostPort[32];
    WCHAR Host[MAX_HOSTNAME_LEN + 32];

    size = sizeof(MIB_UDPTABLE);
    do
    {
        table = HeapAlloc(GetProcessHeap(), 0, size);
        err = GetUdpTable(table, &size, TRUE);
        if (err != NO_ERROR) HeapFree(GetProcessHeap(), 0, table);
    } while (err == ERROR_INSUFFICIENT_BUFFER);

    if (err) return;

    for (i = 0; i < table->dwNumEntries; i++)
    {
        NETSTAT_host_name(table->table[i].dwLocalAddr, HostIp);
        NETSTAT_port_name(table->table[i].dwLocalPort, HostPort);

        swprintf(Host, ARRAY_SIZE(Host), L"%s:%s", HostIp, HostPort);
        NETSTAT_wprintf(L"  %-6s %-22s *:*\n", L"UDP", Host);
    }
    HeapFree(GetProcessHeap(), 0, table);
}

static void NETSTAT_udp_stats(void)
{
    MIB_UDPSTATS stats;

    if (GetUdpStatistics(&stats) == NO_ERROR)
    {
        NETSTAT_wprintf(L"\n%s\n\n", NETSTAT_load_message(IDS_UDP_STAT));
        NETSTAT_wprintf(L"  %-21s = %lu\n", NETSTAT_load_message(IDS_UDP_DGRAMS_RECV), stats.dwInDatagrams);
        NETSTAT_wprintf(L"  %-21s = %lu\n", NETSTAT_load_message(IDS_UDP_NO_PORTS), stats.dwNoPorts);
        NETSTAT_wprintf(L"  %-21s = %lu\n", NETSTAT_load_message(IDS_UDP_RECV_ERRORS), stats.dwInErrors);
        NETSTAT_wprintf(L"  %-21s = %lu\n", NETSTAT_load_message(IDS_UDP_DGRAMS_SENT),  stats.dwOutDatagrams);
    }
}

static NETSTATPROTOCOLS NETSTAT_get_protocol(WCHAR name[])
{
    if (!wcsicmp(name, L"IP")) return PROT_IP;
    if (!wcsicmp(name, L"IPv6")) return PROT_IPV6;
    if (!wcsicmp(name, L"ICMP")) return PROT_ICMP;
    if (!wcsicmp(name, L"ICMPv6")) return PROT_ICMPV6;
    if (!wcsicmp(name, L"TCP")) return PROT_TCP;
    if (!wcsicmp(name, L"TCPv6")) return PROT_TCPV6;
    if (!wcsicmp(name, L"UDP")) return PROT_UDP;
    if (!wcsicmp(name, L"UDPv6")) return PROT_UDPV6;
    return PROT_UNKNOWN;
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    WSADATA wsa_data;
    BOOL output_stats = FALSE;

    if (WSAStartup(MAKEWORD(2, 2), &wsa_data))
    {
        WINE_ERR("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    if (argc == 1)
    {
        /* No options */
        NETSTAT_conn_header();
        NETSTAT_tcp_table();
        return 0;
    }

    while (argv[1] && argv[1][0] == '-')
    {
        switch (argv[1][1])
        {
        case 'a':
            NETSTAT_conn_header();
            NETSTAT_tcp_table();
            NETSTAT_udp_table();
            return 0;
        case 'e':
            NETSTAT_eth_stats();
            return 0;
        case 's':
            output_stats = TRUE;
            break;
        case 'p':
            argv++; argc--;
            if (argc == 1) return 1;
            switch (NETSTAT_get_protocol(argv[1]))
            {
                case PROT_TCP:
                    if (output_stats)
                        NETSTAT_tcp_stats();
                    NETSTAT_conn_header();
                    NETSTAT_tcp_table();
                    break;
                case PROT_UDP:
                    if (output_stats)
                        NETSTAT_udp_stats();
                    NETSTAT_conn_header();
                    NETSTAT_udp_table();
                    break;
                default:
                    WINE_FIXME("Protocol not yet implemented: %s\n", debugstr_w(argv[1]));
            }
            return 0;
        default:
            WINE_FIXME("Unknown option: %s\n", debugstr_w(argv[1]));
            return 1;
        }
        argv++; argc--;
    }

    if (output_stats)
    {
        NETSTAT_tcp_stats();
        NETSTAT_udp_stats();
    }

    return 0;
}
