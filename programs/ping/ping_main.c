/*
 * ping program
 *
 * Copyright (C) 2010 Trey Hunner
 * Copyright (C) 2018 Isira Seneviratne
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

#include "winsock2.h"
#include "ws2tcpip.h"
#include "iphlpapi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <icmpapi.h>
#include <limits.h>

#include <windows.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ping);

static void usage(void)
{
    printf("Usage: ping [-n count] [-w timeout] [-l buffer_length] target_name\n\n"
           "Options:\n"
           "    -n  Number of echo requests to send.\n"
           "    -w  Timeout in milliseconds to wait for each reply.\n"
           "    -l  Length of send buffer.\n");
}

int __cdecl main(int argc, char** argv)
{
    unsigned int n = 4, i, w = 4000, l = 32;
    int res;
    int rec = 0, lost = 0, min = INT_MAX, max = 0;
    WSADATA wsa;
    HANDLE icmp_file = INVALID_HANDLE_VALUE;
    unsigned long ipaddr = INADDR_NONE;
    DWORD retval, reply_size;
    char *send_data = NULL, *hostname = NULL, rtt[16];
    char ip_str[INET_ADDRSTRLEN];
    void *reply_buffer = NULL;
    struct in_addr addr;
    ICMP_ECHO_REPLY *reply;
    float avg = 0;
    struct hostent *remote_host;

    if (argc == 1)
    {
        usage();
        return 1;
    }

    /* Parse command line arguments */
    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-' || argv[i][0] == '/')
        {
            switch (argv[i][1])
            {
            case 'n':
                if (i == argc - 1) return 1;
                n = atoi(argv[++i]);
                if (n == 0) return 1; /* Prevent 0 packet count */
                break;
            case 'w':
                if (i == argc - 1) return 1;
                w = atoi(argv[++i]);
                if (w == 0) return 1;
                break;
            case 'l':
                if (i == argc - 1) return 1;
                l = atoi(argv[++i]);
                if (l == 0) return 1;
                break;
            case '?':
                usage();
                return 0;
            default:
                usage();
                WINE_FIXME( "this command currently only supports the -n, -w and -l parameters.\n" );
                return 1;
            }
        }
        else
        {
            if (hostname)
            {
                printf( "Bad argument %s\n", argv[i] );
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

    /* Initialize Winsock */
    res = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (res != 0)
    {
        printf("WSAStartup failed: %d\n", res);
        return 1;
    }

    /* Resolve hostname to IP */
    remote_host = gethostbyname(hostname);
    if (remote_host == NULL)
    {
        printf("Ping request could not find host %s. Please check the name and try again.\n",
               hostname);
        WSACleanup();
        return 1;
    }

    /* Extract the IP address directly from the host entry */
    ipaddr = *(u_long *) remote_host->h_addr_list[0];

    /* Convert to string only for display purposes */
    addr.s_addr = ipaddr;
    strcpy(ip_str, inet_ntoa(addr));

    if (ipaddr == INADDR_NONE)
    {
        printf("Could not get IP address of host %s.\n", hostname);
        WSACleanup();
        return 1;
    }

    /* Create a handle for ICMP requests */
    icmp_file = IcmpCreateFile();
    if (icmp_file == INVALID_HANDLE_VALUE)
    {
        printf("Unable to open ICMP handle. Error: %ld\n", GetLastError());
        WSACleanup();
        return 1;
    }

    /* Allocate the send buffer */
    send_data = calloc(1, l);
    if (!send_data)
    {
        printf("Memory allocation failed for send buffer.\n");
        IcmpCloseHandle(icmp_file);
        WSACleanup();
        return 1;
    }

    /* Calculate reply buffer size (Reply struct + Data + 8 bytes for ICMP error) */
    reply_size = sizeof(ICMP_ECHO_REPLY) + l + 8;
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
        /* Send the ICMP Echo Request */
        retval = IcmpSendEcho(icmp_file, ipaddr, send_data, l,
            NULL, reply_buffer, reply_size, w);

        if (retval != 0)
        {
            reply = (ICMP_ECHO_REPLY *) reply_buffer;
            
            /* Format RTT display */
            if (reply->RoundTripTime >= 1)
                sprintf(rtt, "=%ld", reply->RoundTripTime);
            else
                strcpy(rtt, "<1");

            printf("Reply from %s: bytes=%d time%sms TTL=%d\n", ip_str, l,
                rtt, reply->Options.Ttl);

            /* Update statistics */
            if (reply->RoundTripTime > max)
                max = reply->RoundTripTime;
            if (reply->RoundTripTime < min)
                min = reply->RoundTripTime;
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

        /* Wait 1 second between pings, but not after the last one */
        if (i < n - 1) Sleep(1000);
    }

    /* Display final statistics */
    printf("\nPing statistics for %s\n", ip_str);
    
    /* Calculate loss percentage (protected against division by zero) */
    float loss_pct = (n > 0) ? (float)lost / n * 100 : 0;
    
    printf("\tPackets: Sent = %d, Received = %d, Lost = %d (%.0f%% loss)\n",
        n, rec, lost, loss_pct);

    if (rec != 0)
    {
        avg /= rec;
        printf("Approximate round trip times in milli-seconds:\n");
        printf("\tMinimum = %dms, Maximum = %dms, Average = %.0fms\n",
               min, max, avg);
    }

    /* Cleanup resources */
    free(reply_buffer);
    free(send_data);
    IcmpCloseHandle(icmp_file);
    WSACleanup();

    return 0;
}
