/*
 * Copyright (C) 2003,2006 Juan Lang
 * Copyright (C) 2007 TransGaming Technologies Inc.
 * Copyright (C) 2009 Alexandre Julliard
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef HAVE_ALIAS_H
#include <alias.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_SOCKETVAR_H
#include <sys/socketvar.h>
#endif
#ifdef HAVE_SYS_TIMEOUT_H
#include <sys/timeout.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif
#ifdef HAVE_NET_IF_TYPES_H
#include <net/if_types.h>
#endif
#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif
#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif
#ifdef HAVE_NETINET_IF_ETHER_H
#include <netinet/if_ether.h>
#endif
#ifdef HAVE_NETINET_IF_INARP_H
#include <netinet/if_inarp.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#ifdef HAVE_NETINET_IP_VAR_H
#include <netinet/ip_var.h>
#endif
#ifdef HAVE_NETINET_TCP_FSM_H
#include <netinet/tcp_fsm.h>
#endif
#ifdef HAVE_NETINET_IN_PCB_H
#include <netinet/in_pcb.h>
#endif
#ifdef HAVE_NETINET_TCP_TIMER_H
#include <netinet/tcp_timer.h>
#endif
#ifdef HAVE_NETINET_TCP_VAR_H
#include <netinet/tcp_var.h>
#endif
#ifdef HAVE_NETINET_IP_ICMP_H
#include <netinet/ip_icmp.h>
#endif
#ifdef HAVE_NETINET_ICMP_VAR_H
#include <netinet/icmp_var.h>
#endif
#ifdef HAVE_NETINET_UDP_H
#include <netinet/udp.h>
#endif
#ifdef HAVE_NETINET_UDP_VAR_H
#include <netinet/udp_var.h>
#endif
#ifdef HAVE_SYS_PROTOSW_H
#include <sys/protosw.h>
#endif
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#ifdef HAVE_KSTAT_H
#include <kstat.h>
#endif
#ifdef HAVE_INET_MIB2_H
#include <inet/mib2.h>
#endif
#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif
#ifdef HAVE_SYS_TIHDR_H
#include <sys/tihdr.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif
#ifdef HAVE_SYS_USER_H
/* Make sure the definitions of struct kinfo_proc are the same. */
#include <sys/user.h>
#endif
#ifdef HAVE_LIBPROCSTAT_H
#include <libprocstat.h>
#endif
#ifdef HAVE_LIBPROC_H
#include <libproc.h>
#endif
#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif

#ifndef ROUNDUP
#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(int) - 1))) : sizeof(int))
#endif
#ifndef ADVANCE
#define ADVANCE(x, n) (x += ROUNDUP(((struct sockaddr *)n)->sa_len))
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#define USE_WS_PREFIX
#include "winsock2.h"
#include "ws2ipdef.h"
#include "ifenum.h"
#include "iphlpapi.h"

#include "wine/debug.h"
#include "wine/server.h"
#include "wine/unicode.h"

#ifndef HAVE_NETINET_TCP_FSM_H
#define TCPS_ESTABLISHED  1
#define TCPS_SYN_SENT     2
#define TCPS_SYN_RECEIVED 3
#define TCPS_FIN_WAIT_1   4
#define TCPS_FIN_WAIT_2   5
#define TCPS_TIME_WAIT    6
#define TCPS_CLOSED       7
#define TCPS_CLOSE_WAIT   8
#define TCPS_LAST_ACK     9
#define TCPS_LISTEN      10
#define TCPS_CLOSING     11
#endif

#ifndef RTF_MULTICAST
#define RTF_MULTICAST 0 /* Not available on NetBSD/OpenBSD */
#endif

#ifndef RTF_LLINFO
#define RTF_LLINFO 0 /* Not available on FreeBSD 8 and above */
#endif

WINE_DEFAULT_DEBUG_CHANNEL(iphlpapi);

#ifdef HAVE_LIBKSTAT
static DWORD kstat_get_ui32( kstat_t *ksp, const char *name )
{
    unsigned int i;
    kstat_named_t *data = ksp->ks_data;

    for (i = 0; i < ksp->ks_ndata; i++)
        if (!strcmp( data[i].name, name )) return data[i].value.ui32;
    return 0;
}
#endif

/******************************************************************
 *    GetUdpStatistics (IPHLPAPI.@)
 *
 * Get the IPv4 and IPv6 UDP statistics for the local computer.
 *
 * PARAMS
 *  stats [Out] buffer for UDP statistics
 *  family [In] specifies whether IPv4 or IPv6 statistics are returned
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 */
DWORD WINAPI GetUdpStatisticsEx(PMIB_UDPSTATS stats, DWORD family)
{
    DWORD ret = ERROR_NOT_SUPPORTED;

    if (!stats) return ERROR_INVALID_PARAMETER;
    if (family != WS_AF_INET && family != WS_AF_INET6) return ERROR_INVALID_PARAMETER;
    memset( stats, 0, sizeof(*stats) );

    stats->dwNumAddrs = get_interface_indices( FALSE, NULL );

    if (family == WS_AF_INET6)
    {
#ifdef __linux__
        {
            FILE *fp;

            if ((fp = fopen("/proc/net/snmp6", "r")))
            {
                struct {
                    const char *name;
                    DWORD *elem;
                } udpstatlist[] = {
                    { "Udp6InDatagrams",  &stats->dwInDatagrams },
                    { "Udp6NoPorts",      &stats->dwNoPorts },
                    { "Udp6InErrors",     &stats->dwInErrors },
                    { "Udp6OutDatagrams", &stats->dwOutDatagrams },
                };
                char buf[512], *ptr, *value;
                DWORD res, i;

                while ((ptr = fgets(buf, sizeof(buf), fp)))
                {
                    if (!(value = strchr(buf, ' ')))
                        continue;

                    /* terminate the valuename */
                    ptr = value - 1;
                    *(ptr + 1) = '\0';

                    /* and strip leading spaces from value */
                    value += 1;
                    while (*value==' ') value++;
                    if ((ptr = strchr(value, '\n')))
                        *ptr='\0';

                    for (i = 0; i < ARRAY_SIZE(udpstatlist); i++)
                        if (!_strnicmp(buf, udpstatlist[i].name, -1) && sscanf(value, "%d", &res))
                            *udpstatlist[i].elem = res;
                }
                fclose(fp);
                ret = NO_ERROR;
            }
        }
#else
        FIXME( "unimplemented for IPv6\n" );
#endif
        return ret;
    }

#ifdef __linux__
    {
        FILE *fp;

        if ((fp = fopen("/proc/net/snmp", "r")))
        {
            static const char hdr[] = "Udp:";
            char buf[512], *ptr;

            while ((ptr = fgets(buf, sizeof(buf), fp)))
            {
                if (_strnicmp(buf, hdr, sizeof(hdr) - 1)) continue;
                /* last line was a header, get another */
                if (!(ptr = fgets(buf, sizeof(buf), fp))) break;
                if (!_strnicmp(buf, hdr, sizeof(hdr) - 1))
                {
                    ptr += sizeof(hdr);
                    sscanf( ptr, "%u %u %u %u %u",
                            &stats->dwInDatagrams, &stats->dwNoPorts,
                            &stats->dwInErrors, &stats->dwOutDatagrams, &stats->dwNumAddrs );
                    break;
                }
            }
            fclose(fp);
            ret = NO_ERROR;
        }
    }
#elif defined(HAVE_LIBKSTAT)
    {
        static char udp[] = "udp";
        kstat_ctl_t *kc;
        kstat_t *ksp;
        MIB_UDPTABLE *udp_table;

        if ((kc = kstat_open()) &&
            (ksp = kstat_lookup( kc, udp, 0, udp )) &&
            kstat_read( kc, ksp, NULL ) != -1 &&
            ksp->ks_type == KSTAT_TYPE_NAMED)
        {
            stats->dwInDatagrams  = kstat_get_ui32( ksp, "inDatagrams" );
            stats->dwNoPorts      = 0;  /* FIXME */
            stats->dwInErrors     = kstat_get_ui32( ksp, "inErrors" );
            stats->dwOutDatagrams = kstat_get_ui32( ksp, "outDatagrams" );
            if (!AllocateAndGetUdpTableFromStack( &udp_table, FALSE, GetProcessHeap(), 0 ))
            {
                stats->dwNumAddrs = udp_table->dwNumEntries;
                HeapFree( GetProcessHeap(), 0, udp_table );
            }
            ret = NO_ERROR;
        }
        if (kc) kstat_close( kc );
    }
#elif defined(HAVE_SYS_SYSCTL_H) && defined(UDPCTL_STATS) && defined(HAVE_STRUCT_UDPSTAT_UDPS_IPACKETS)
    {
        int mib[] = {CTL_NET, PF_INET, IPPROTO_UDP, UDPCTL_STATS};
        struct udpstat udp_stat;
        MIB_UDPTABLE *udp_table;
        size_t needed = sizeof(udp_stat);

        if(sysctl(mib, ARRAY_SIZE(mib), &udp_stat, &needed, NULL, 0) != -1)
        {
            stats->dwInDatagrams = udp_stat.udps_ipackets;
            stats->dwOutDatagrams = udp_stat.udps_opackets;
            stats->dwNoPorts = udp_stat.udps_noport;
            stats->dwInErrors = udp_stat.udps_hdrops + udp_stat.udps_badsum + udp_stat.udps_fullsock + udp_stat.udps_badlen;
            if (!AllocateAndGetUdpTableFromStack( &udp_table, FALSE, GetProcessHeap(), 0 ))
            {
                stats->dwNumAddrs = udp_table->dwNumEntries;
                HeapFree( GetProcessHeap(), 0, udp_table );
            }
            ret = NO_ERROR;
        }
        else ERR ("failed to get udpstat\n");
    }
#else
    FIXME( "unimplemented for IPv4\n" );
#endif
    return ret;
}

/******************************************************************
 *    GetUdpStatistics (IPHLPAPI.@)
 *
 * Get the UDP statistics for the local computer.
 *
 * PARAMS
 *  stats [Out] buffer for UDP statistics
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 */
DWORD WINAPI GetUdpStatistics(PMIB_UDPSTATS stats)
{
    return GetUdpStatisticsEx(stats, WS_AF_INET);
}
