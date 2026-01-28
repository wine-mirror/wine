/*
 * ping program
 *
 * Copyright (C) 2010 Trey Hunner
 * Copyright (C) 2018 Isira Seneviratne
 * Copyright (C) 2026 Pawel Kapeluszny
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
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <icmpapi.h>
#include <limits.h>
#include <windows.h>

/* Constants to avoid magic numbers */
#define DEFAULT_COUNT 4
#define DEFAULT_TIMEOUT 4000
#define DEFAULT_DATA_LEN 32
#define MAX_DATA_LEN 65500 /* Safe limit for WORD (65535) */
#define ICMP_HEADER_EXTRA 8

/* Wine debug macros (commented out for standalone build) */
// #include "wine/debug.h"
// WINE_DEFAULT_DEBUG_CHANNEL(ping);

static void usage(void)
{
    printf("Usage: ping [-n count] [-w timeout] [-l buffer_length] target_name\n\n"
           "Options:\n"
           "    -n  Number of echo requests to send.\n"
           "    -w  Timeout in milliseconds to wait for each reply.\n"
           "    -l  Length of send buffer (0 - %d).\n", MAX_DATA_LEN);
}

int __cdecl main(int argc, char** argv)
{
    unsigned int n = DEFAULT_COUNT;
    unsigned int w = DEFAULT_TIMEOUT;
    unsigned int l = DEFAULT_DATA_LEN;
    unsigned int i;
    int res;
    
    /* Stats variables */
    int rec = 0, lost = 0;
    unsigned long min = ULONG_MAX, max = 0;
    double avg = 0.0; 

    WSADATA wsa;
    HANDLE icmp_file = INVALID_HANDLE_VALUE;
    unsigned long ipaddr = INADDR_NONE;
    DWORD retval, reply_size;
    
    char *send_data = NULL;
    const char *hostname = NULL;
    char rtt[32];
    char ip_str[INET_ADDRSTRLEN];
    
    void *reply_buffer = NULL;
    struct in_addr addr;
    ICMP_ECHO_REPLY *reply;
    struct hostent *remote_host;
    char *endptr;

    if (argc == 1)
    {
        usage();
        return 1;
    }

    /* Argument parsing with validation */
    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-' || argv[i][0] == '/')
        {
            if (i == argc - 1) { usage(); return 1; }
            
            unsigned long val = strtoul(argv[++i], &endptr, 10);
            if (*endptr != '\0') {
                printf("Invalid number format: %s\n", argv[i]);
                return 1;
            }

            switch (argv[i-1][1])
            {
            case 'n':
                if (val == 0) {
                    printf("Bad value for option -n.\n");
                    return 1;
                }
                n = (unsigned int)val;
                break;
            case 'w':
                if (val == 0) {
                    printf("Bad value for option -w.\n");
                    return 1;
                }
                w = (unsigned int)val;
                break;
            case 'l':
                if (val > MAX_DATA_LEN) {
                    printf("Bad value for option -l, valid range is from 0 to %d.\n", MAX_DATA_LEN);
                    return 1;
                }
                l = (unsigned int)val;
                break;
            case '?':
                usage();
                return 0;
            default:
                usage();
                /* WINE_FIXME(...) */
                return 1;
            }
        }
        else
        {
            if (hostname) {
                printf("Bad argument %s\n", argv[i]);
                return 1;
            }
            hostname = argv[i];
        }
    }

    if (!hostname)
    {
        printf("Pass a host name.\n");
        return 1;
    }

    res = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (res != 0)
    {
        printf("WSAStartup failed: %d\n", res);
        return 1;
    }

    /* Resolve hostname */
    remote_host = gethostbyname(hostname);
    
    /* FIX: Added check for empty address list */
    if (remote_host == NULL || !remote_host->h_addr_list || !remote_host->h_addr_list[0])
    {
        printf("Ping request could not find host %s. Please check the name and try again.\n", hostname);
        WSACleanup();
        return 1;
    }

    ipaddr = *(u_long *) remote_host->h_addr_list[0];
    addr.s_addr = ipaddr;

    if (InetNtopA(AF_INET, &addr, ip_str, sizeof(ip_str)) == NULL) {
        printf("Could not resolve IP address string.\n");
        WSACleanup();
        return 1;
    }

    if (ipaddr == INADDR_NONE)
    {
        printf("Could not get IP address of host %s.\n", hostname);
        WSACleanup();
        return 1;
    }

    icmp_file = IcmpCreateFile();
    if (icmp_file == INVALID_HANDLE_VALUE) {
        printf("Unable to open ICMP handle. Error: %ld\n", GetLastError());
        WSACleanup();
        return 1;
    }

    if (l > 0) {
        send_data = malloc(l); /* Changed to malloc as we fill it immediately */
        if (!send_data) {
            printf("Memory allocation failed.\n");
            IcmpCloseHandle(icmp_file);
            WSACleanup();
            return 1;
        }
        /* FIX: Windows-style pattern fill (alphabet sequence) */
        unsigned int k;
        for (k = 0; k < l; k++) {
            send_data[k] = 'a' + (k % 23);
        }
    }

    reply_size = sizeof(ICMP_ECHO_REPLY) + l + ICMP_HEADER_EXTRA;
    reply_buffer = malloc(reply_size);
    if (reply_buffer == NULL)
    {
        printf("Unable to allocate memory to reply buffer.\n");
        free(send_data);
        IcmpCloseHandle(icmp_file);
        WSACleanup();
        return 1;
    }

    printf("Pinging %s [%s] with %d bytes of data:\n", hostname, ip_str, l);

    for (i = 0; i < n; i++)
    {
        SetLastError(0);
        /* FIX: Explicit cast to WORD for safety */
        retval = IcmpSendEcho(icmp_file, ipaddr, send_data, (WORD)l,
            NULL, reply_buffer, reply_size, w);

        if (retval != 0)
        {
            reply = (ICMP_ECHO_REPLY *) reply_buffer;
            
            if (reply->RoundTripTime >= 1)
                snprintf(rtt, sizeof(rtt), "=%ld", reply->RoundTripTime);
            else
                snprintf(rtt, sizeof(rtt), "<1");

            /* FIX: Simplified TTL logic */
            unsigned char ttl = reply->Options.Ttl;

            printf("Reply from %s: bytes=%d time%sms TTL=%d\n", ip_str, l,
                rtt, ttl);

            if (rec == 0) {
                min = max = reply->RoundTripTime;
            } else {
                if (reply->RoundTripTime > max) max = reply->RoundTripTime;
                if (reply->RoundTripTime < min) min = reply->RoundTripTime;
            }
            
            avg += reply->RoundTripTime;
            rec++;
        }
        else
        {
            if (GetLastError() == IP_REQ_TIMED_OUT)
                puts("Request timed out.");
            else
                puts("PING: transmit failed. General failure.");
            lost++;
        }
        
        if (i < n - 1) Sleep(1000);
    }

    printf("\nPing statistics for %s\n", ip_str);
    
    /* FIX: Consistent double types for stats */
    double loss_pct = (n > 0) ? (double)lost / n * 100.0 : 0.0;
    
    printf("\tPackets: Sent = %d, Received = %d, Lost = %d (%.0f%% loss)\n",
        n, rec, lost, loss_pct);

    if (rec != 0)
    {
        avg /= rec;
        printf("Approximate round trip times in milli-seconds:\n");
        /* FIX: Corrected printf format string */
        printf("\tMinimum = %lums, Maximum = %lums, Average = %.0fms\n",
               min, max, avg);
    }

    free(reply_buffer);
    free(send_data);
    IcmpCloseHandle(icmp_file);
    WSACleanup();

    return 0;
}
