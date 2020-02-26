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
#include "wine/heap.h"

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
    HANDLE icmp_file;
    unsigned long ipaddr;
    DWORD retval, reply_size;
    char *send_data, ip[100], *hostname = NULL, rtt[16];
    void *reply_buffer;
    struct in_addr addr;
    ICMP_ECHO_REPLY *reply;
    float avg = 0;
    struct hostent *remote_host;

    if (argc == 1)
    {
        usage();
        exit(1);
    }

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-' || argv[i][0] == '/')
        {
            switch (argv[i][1])
            {
            case 'n':
                if (i == argc - 1)
                {
                    printf( "Missing value for option %s\n", argv[i] );
                    exit(1);
                }
                n = atoi(argv[++i]);
                if (n == 0)
                {
                  printf("Bad value for option -n, valid range is from 1 to 4294967295.\n");
                  exit(1);
                }
                break;
            case 'w':
                if (i == argc - 1)
                {
                    printf( "Missing value for option %s\n", argv[i] );
                    exit(1);
                }
                w = atoi(argv[++i]);
                if (w == 0)
                {
                    printf("Bad value for option -w.\n");
                    exit(1);
                }
                break;
            case 'l':
                if (i == argc - 1)
                {
                    printf( "Missing value for option %s\n", argv[i] );
                    exit(1);
                }
                l = atoi(argv[++i]);
                if (l == 0)
                {
                    printf("Bad value for option -l.\n");
                    exit(1);
                }
                break;
            case '?':
                usage();
                exit(1);
            default:
                usage();
                WINE_FIXME( "this command currently only supports the -n, -w and -l parameters.\n" );
                exit(1);
            }
        }
        else
        {
            if (hostname)
            {
                printf( "Bad argument %s\n", argv[i] );
                exit(1);
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

    remote_host = gethostbyname(hostname);
    if (remote_host == NULL)
    {
        printf("Ping request could not find host %s. Please check the name and try again.\n",
               hostname);
        return 1;
    }

    addr.s_addr = *(u_long *) remote_host->h_addr_list[0];
    strcpy(ip, inet_ntoa(addr));
    ipaddr = inet_addr(ip);
    if (ipaddr == INADDR_NONE)
    {
        printf("Could not get IP address of host %s.", hostname);
        return 1;
    }

    icmp_file = IcmpCreateFile();

    send_data = heap_alloc_zero(l);
    reply_size = sizeof(ICMP_ECHO_REPLY) + l + 8;
    /* The buffer has to hold 8 more bytes of data (the size of an ICMP error message). */
    reply_buffer = heap_alloc(reply_size);
    if (reply_buffer == NULL)
    {
        printf("Unable to allocate memory to reply buffer.\n");
        return 1;
    }

    printf("Pinging %s [%s] with %d bytes of data:\n", hostname, ip, l);
    for (i = 0; i < n; i++)
    {
        SetLastError(0);
        retval = IcmpSendEcho(icmp_file, ipaddr, send_data, l,
            NULL, reply_buffer, reply_size, w);
        if (retval != 0)
        {
            reply = (ICMP_ECHO_REPLY *) reply_buffer;
            if (reply->RoundTripTime >= 1)
                sprintf(rtt, "=%d", reply->RoundTripTime);
            else
                strcpy(rtt, "<1");
            printf("Reply from %s: bytes=%d time%sms TTL=%d\n", ip, l,
                rtt, reply->Options.Ttl);
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
        if (i < n - 1) Sleep(1000);
    }

    printf("\nPing statistics for %s\n", ip);
    printf("\tPackets: Sent = %d, Received = %d, Lost = %d (%.0f%% loss)\n",
        n, rec, lost, (float) lost / n * 100);
    if (rec != 0)
    {
        avg /= rec;
        printf("Approximate round trip times in milli-seconds:\n");
        printf("\tMinimum = %dms, Maximum = %dms, Average = %.0fms\n",
               min, max, avg);
    }

    heap_free(reply_buffer);
    return 0;
}
