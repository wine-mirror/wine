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

#ifndef ROUNDUP
#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#endif
#ifndef ADVANCE
#define ADVANCE(x, n) (x += ROUNDUP(((struct sockaddr *)n)->sa_len))
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#include "ifenum.h"
#include "ipstats.h"

#include "wine/debug.h"
#include "wine/server.h"

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

static ULONGLONG kstat_get_ui64( kstat_t *ksp, const char *name )
{
    unsigned int i;
    kstat_named_t *data = ksp->ks_data;

    for (i = 0; i < ksp->ks_ndata; i++)
        if (!strcmp( data[i].name, name )) return data[i].value.ui64;
    return 0;
}
#endif

#if defined(HAVE_SYS_TIHDR_H) && defined(T_OPTMGMT_ACK)
static int open_streams_mib( const char *proto )
{
    int fd;
    struct strbuf buf;
    struct request
    {
        struct T_optmgmt_req req_header;
        struct opthdr        opt_header;
    } request;

    if ((fd = open( "/dev/arp", O_RDWR )) == -1)
    {
        WARN( "could not open /dev/arp: %s\n", strerror(errno) );
        return -1;
    }
    if (proto) ioctl( fd, I_PUSH, proto );

    request.req_header.PRIM_type  = T_SVR4_OPTMGMT_REQ;
    request.req_header.OPT_length = sizeof(request.opt_header);
    request.req_header.OPT_offset = FIELD_OFFSET( struct request, opt_header );
    request.req_header.MGMT_flags = T_CURRENT;
    request.opt_header.level      = MIB2_IP;
    request.opt_header.name       = 0;
    request.opt_header.len        = 0;

    buf.len = sizeof(request);
    buf.buf = (caddr_t)&request;
    if (putmsg( fd, &buf, NULL, 0 ) == -1)
    {
        WARN( "putmsg: %s\n", strerror(errno) );
        close( fd );
        fd = -1;
    }
    return fd;
}

static void *read_mib_entry( int fd, int level, int name, int *len )
{
    struct strbuf buf;
    void *data;
    int ret, flags = 0;

    struct reply
    {
        struct T_optmgmt_ack ack_header;
        struct opthdr        opt_header;
    } reply;

    for (;;)
    {
        buf.maxlen = sizeof(reply);
        buf.buf = (caddr_t)&reply;
        if ((ret = getmsg( fd, &buf, NULL, &flags )) < 0) return NULL;
        if (!(ret & MOREDATA)) return NULL;
        if (reply.ack_header.PRIM_type != T_OPTMGMT_ACK) return NULL;
        if (buf.len < sizeof(reply.ack_header)) return NULL;
        if (reply.ack_header.OPT_length < sizeof(reply.opt_header)) return NULL;

        if (!(data = HeapAlloc( GetProcessHeap(), 0, reply.opt_header.len ))) return NULL;
        buf.maxlen = reply.opt_header.len;
        buf.buf = (caddr_t)data;
        flags = 0;
        if (getmsg( fd, NULL, &buf, &flags ) >= 0 &&
            reply.opt_header.level == level &&
            reply.opt_header.name == name)
        {
            *len = buf.len;
            return data;
        }
        HeapFree( GetProcessHeap(), 0, data );
    }
}
#endif /* HAVE_SYS_TIHDR_H && T_OPTMGMT_ACK */

DWORD getInterfaceStatsByName(const char *name, PMIB_IFROW entry)
{
    DWORD ret = ERROR_NOT_SUPPORTED;

    if (!name || !entry) return ERROR_INVALID_PARAMETER;

#ifdef __linux__
    {
        FILE *fp;

        if ((fp = fopen("/proc/net/dev", "r")))
        {
            DWORD skip;
            char buf[512], *ptr;
            int nameLen = strlen(name);

            while ((ptr = fgets(buf, sizeof(buf), fp)))
            {
                while (*ptr && isspace(*ptr)) ptr++;
                if (strncasecmp(ptr, name, nameLen) == 0 && *(ptr + nameLen) == ':')
                {
                    ptr += nameLen + 1;
                    sscanf( ptr, "%u %u %u %u %u %u %u %u %u %u %u %u",
                            &entry->dwInOctets, &entry->dwInUcastPkts,
                            &entry->dwInErrors, &entry->dwInDiscards,
                            &skip, &skip, &skip,
                            &entry->dwInNUcastPkts, &entry->dwOutOctets,
                            &entry->dwOutUcastPkts, &entry->dwOutErrors,
                            &entry->dwOutDiscards );
                    break;
                }
            }
            fclose(fp);
            ret = NO_ERROR;
        }
    }
#elif defined(HAVE_LIBKSTAT)
    {
        kstat_ctl_t *kc;
        kstat_t *ksp;

        if ((kc = kstat_open()) &&
            (ksp = kstat_lookup( kc, NULL, -1, (char *)name )) &&
            kstat_read( kc, ksp, NULL ) != -1 &&
            ksp->ks_type == KSTAT_TYPE_NAMED)
        {
            entry->dwMtu             = 1500;  /* FIXME */
            entry->dwSpeed           = min( kstat_get_ui64( ksp, "ifspeed" ), ~0u );
            entry->dwInOctets        = kstat_get_ui32( ksp, "rbytes" );
            entry->dwInNUcastPkts    = kstat_get_ui32( ksp, "multircv" );
            entry->dwInNUcastPkts   += kstat_get_ui32( ksp, "brdcstrcv" );
            entry->dwInUcastPkts     = kstat_get_ui32( ksp, "ipackets" ) - entry->dwInNUcastPkts;
            entry->dwInDiscards      = kstat_get_ui32( ksp, "norcvbuf" );
            entry->dwInErrors        = kstat_get_ui32( ksp, "ierrors" );
            entry->dwInUnknownProtos = kstat_get_ui32( ksp, "unknowns" );
            entry->dwOutOctets       = kstat_get_ui32( ksp, "obytes" );
            entry->dwOutNUcastPkts   = kstat_get_ui32( ksp, "multixmt" );
            entry->dwOutNUcastPkts  += kstat_get_ui32( ksp, "brdcstxmt" );
            entry->dwOutUcastPkts    = kstat_get_ui32( ksp, "opackets" ) - entry->dwOutNUcastPkts;
            entry->dwOutDiscards     = 0;  /* FIXME */
            entry->dwOutErrors       = kstat_get_ui32( ksp, "oerrors" );
            entry->dwOutQLen         = kstat_get_ui32( ksp, "noxmtbuf" );
            ret = NO_ERROR;
        }
        if (kc) kstat_close( kc );
    }
#elif defined(HAVE_SYS_SYSCTL_H) && defined(NET_RT_IFLIST)
    {
        int mib[] = {CTL_NET, PF_ROUTE, 0, AF_INET, NET_RT_IFLIST, if_nametoindex(name)};
#define MIB_LEN (sizeof(mib) / sizeof(mib[0]))

        size_t needed;
        char *buf = NULL, *end;
        struct if_msghdr *ifm;
        struct if_data ifdata;

        if(sysctl(mib, MIB_LEN, NULL, &needed, NULL, 0) == -1)
        {
            ERR ("failed to get size of iflist\n");
            goto done;
        }
        buf = HeapAlloc (GetProcessHeap (), 0, needed);
        if (!buf)
        {
            ret = ERROR_OUTOFMEMORY;
            goto done;
        }
        if(sysctl(mib, MIB_LEN, buf, &needed, NULL, 0) == -1)
        {
            ERR ("failed to get iflist\n");
            goto done;
        }
        for ( end = buf + needed; buf < end; buf += ifm->ifm_msglen)
        {
            ifm = (struct if_msghdr *) buf;
            if(ifm->ifm_type == RTM_IFINFO)
            {
                ifdata = ifm->ifm_data;
                entry->dwMtu = ifdata.ifi_mtu;
                entry->dwSpeed = ifdata.ifi_baudrate;
                entry->dwInOctets = ifdata.ifi_ibytes;
                entry->dwInErrors = ifdata.ifi_ierrors;
                entry->dwInDiscards = ifdata.ifi_iqdrops;
                entry->dwInUcastPkts = ifdata.ifi_ipackets;
                entry->dwInNUcastPkts = ifdata.ifi_imcasts;
                entry->dwOutOctets = ifdata.ifi_obytes;
                entry->dwOutUcastPkts = ifdata.ifi_opackets;
                entry->dwOutErrors = ifdata.ifi_oerrors;
                ret = NO_ERROR;
                break;
            }
        }
    done:
        HeapFree (GetProcessHeap (), 0, buf);
    }
#else
    FIXME( "unimplemented\n" );
#endif
    return ret;
}


/******************************************************************
 *    GetIcmpStatistics (IPHLPAPI.@)
 *
 * Get the ICMP statistics for the local computer.
 *
 * PARAMS
 *  stats [Out] buffer for ICMP statistics
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 */
DWORD WINAPI GetIcmpStatistics(PMIB_ICMP stats)
{
    DWORD ret = ERROR_NOT_SUPPORTED;

    if (!stats) return ERROR_INVALID_PARAMETER;
    memset( stats, 0, sizeof(MIB_ICMP) );

#ifdef __linux__
    {
        FILE *fp;

        if ((fp = fopen("/proc/net/snmp", "r")))
        {
            static const char hdr[] = "Icmp:";
            char buf[512], *ptr;

            while ((ptr = fgets(buf, sizeof(buf), fp)))
            {
                if (strncasecmp(buf, hdr, sizeof(hdr) - 1)) continue;
                /* last line was a header, get another */
                if (!(ptr = fgets(buf, sizeof(buf), fp))) break;
                if (!strncasecmp(buf, hdr, sizeof(hdr) - 1))
                {
                    ptr += sizeof(hdr);
                    sscanf( ptr, "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u",
                            &stats->stats.icmpInStats.dwMsgs,
                            &stats->stats.icmpInStats.dwErrors,
                            &stats->stats.icmpInStats.dwDestUnreachs,
                            &stats->stats.icmpInStats.dwTimeExcds,
                            &stats->stats.icmpInStats.dwParmProbs,
                            &stats->stats.icmpInStats.dwSrcQuenchs,
                            &stats->stats.icmpInStats.dwRedirects,
                            &stats->stats.icmpInStats.dwEchoReps,
                            &stats->stats.icmpInStats.dwTimestamps,
                            &stats->stats.icmpInStats.dwTimestampReps,
                            &stats->stats.icmpInStats.dwAddrMasks,
                            &stats->stats.icmpInStats.dwAddrMaskReps,
                            &stats->stats.icmpOutStats.dwMsgs,
                            &stats->stats.icmpOutStats.dwErrors,
                            &stats->stats.icmpOutStats.dwDestUnreachs,
                            &stats->stats.icmpOutStats.dwTimeExcds,
                            &stats->stats.icmpOutStats.dwParmProbs,
                            &stats->stats.icmpOutStats.dwSrcQuenchs,
                            &stats->stats.icmpOutStats.dwRedirects,
                            &stats->stats.icmpOutStats.dwEchoReps,
                            &stats->stats.icmpOutStats.dwTimestamps,
                            &stats->stats.icmpOutStats.dwTimestampReps,
                            &stats->stats.icmpOutStats.dwAddrMasks,
                            &stats->stats.icmpOutStats.dwAddrMaskReps );
                    break;
                }
            }
            fclose(fp);
            ret = NO_ERROR;
        }
    }
#elif defined(HAVE_LIBKSTAT)
    {
        static char ip[] = "ip", icmp[] = "icmp";
        kstat_ctl_t *kc;
        kstat_t *ksp;

        if ((kc = kstat_open()) &&
            (ksp = kstat_lookup( kc, ip, 0, icmp )) &&
            kstat_read( kc, ksp, NULL ) != -1 &&
            ksp->ks_type == KSTAT_TYPE_NAMED)
        {
            stats->stats.icmpInStats.dwMsgs           = kstat_get_ui32( ksp, "inMsgs" );
            stats->stats.icmpInStats.dwErrors         = kstat_get_ui32( ksp, "inErrors" );
            stats->stats.icmpInStats.dwDestUnreachs   = kstat_get_ui32( ksp, "inDestUnreachs" );
            stats->stats.icmpInStats.dwTimeExcds      = kstat_get_ui32( ksp, "inTimeExcds" );
            stats->stats.icmpInStats.dwParmProbs      = kstat_get_ui32( ksp, "inParmProbs" );
            stats->stats.icmpInStats.dwSrcQuenchs     = kstat_get_ui32( ksp, "inSrcQuenchs" );
            stats->stats.icmpInStats.dwRedirects      = kstat_get_ui32( ksp, "inRedirects" );
            stats->stats.icmpInStats.dwEchos          = kstat_get_ui32( ksp, "inEchos" );
            stats->stats.icmpInStats.dwEchoReps       = kstat_get_ui32( ksp, "inEchoReps" );
            stats->stats.icmpInStats.dwTimestamps     = kstat_get_ui32( ksp, "inTimestamps" );
            stats->stats.icmpInStats.dwTimestampReps  = kstat_get_ui32( ksp, "inTimestampReps" );
            stats->stats.icmpInStats.dwAddrMasks      = kstat_get_ui32( ksp, "inAddrMasks" );
            stats->stats.icmpInStats.dwAddrMaskReps   = kstat_get_ui32( ksp, "inAddrMaskReps" );
            stats->stats.icmpOutStats.dwMsgs          = kstat_get_ui32( ksp, "outMsgs" );
            stats->stats.icmpOutStats.dwErrors        = kstat_get_ui32( ksp, "outErrors" );
            stats->stats.icmpOutStats.dwDestUnreachs  = kstat_get_ui32( ksp, "outDestUnreachs" );
            stats->stats.icmpOutStats.dwTimeExcds     = kstat_get_ui32( ksp, "outTimeExcds" );
            stats->stats.icmpOutStats.dwParmProbs     = kstat_get_ui32( ksp, "outParmProbs" );
            stats->stats.icmpOutStats.dwSrcQuenchs    = kstat_get_ui32( ksp, "outSrcQuenchs" );
            stats->stats.icmpOutStats.dwRedirects     = kstat_get_ui32( ksp, "outRedirects" );
            stats->stats.icmpOutStats.dwEchos         = kstat_get_ui32( ksp, "outEchos" );
            stats->stats.icmpOutStats.dwEchoReps      = kstat_get_ui32( ksp, "outEchoReps" );
            stats->stats.icmpOutStats.dwTimestamps    = kstat_get_ui32( ksp, "outTimestamps" );
            stats->stats.icmpOutStats.dwTimestampReps = kstat_get_ui32( ksp, "outTimestampReps" );
            stats->stats.icmpOutStats.dwAddrMasks     = kstat_get_ui32( ksp, "outAddrMasks" );
            stats->stats.icmpOutStats.dwAddrMaskReps  = kstat_get_ui32( ksp, "outAddrMaskReps" );
            ret = NO_ERROR;
        }
        if (kc) kstat_close( kc );
    }
#elif defined(HAVE_SYS_SYSCTL_H) && defined(ICMPCTL_STATS) && defined(HAVE_STRUCT_ICMPSTAT_ICPS_INHIST)
    {
        int mib[] = {CTL_NET, PF_INET, IPPROTO_ICMP, ICMPCTL_STATS};
#define MIB_LEN (sizeof(mib) / sizeof(mib[0]))
        struct icmpstat icmp_stat;
        size_t needed  = sizeof(icmp_stat);
        int i;

        if(sysctl(mib, MIB_LEN, &icmp_stat, &needed, NULL, 0) != -1)
        {
            /*in stats */
            stats->stats.icmpInStats.dwMsgs = icmp_stat.icps_badcode + icmp_stat.icps_checksum + icmp_stat.icps_tooshort + icmp_stat.icps_badlen;
            for(i = 0; i <= ICMP_MAXTYPE; i++)
                stats->stats.icmpInStats.dwMsgs += icmp_stat.icps_inhist[i];

            stats->stats.icmpInStats.dwErrors = icmp_stat.icps_badcode + icmp_stat.icps_tooshort + icmp_stat.icps_checksum + icmp_stat.icps_badlen;

            stats->stats.icmpInStats.dwDestUnreachs = icmp_stat.icps_inhist[ICMP_UNREACH];
            stats->stats.icmpInStats.dwTimeExcds = icmp_stat.icps_inhist[ICMP_TIMXCEED];
            stats->stats.icmpInStats.dwParmProbs = icmp_stat.icps_inhist[ICMP_PARAMPROB];
            stats->stats.icmpInStats.dwSrcQuenchs = icmp_stat.icps_inhist[ICMP_SOURCEQUENCH];
            stats->stats.icmpInStats.dwRedirects = icmp_stat.icps_inhist[ICMP_REDIRECT];
            stats->stats.icmpInStats.dwEchos = icmp_stat.icps_inhist[ICMP_ECHO];
            stats->stats.icmpInStats.dwEchoReps = icmp_stat.icps_inhist[ICMP_ECHOREPLY];
            stats->stats.icmpInStats.dwTimestamps = icmp_stat.icps_inhist[ICMP_TSTAMP];
            stats->stats.icmpInStats.dwTimestampReps = icmp_stat.icps_inhist[ICMP_TSTAMPREPLY];
            stats->stats.icmpInStats.dwAddrMasks = icmp_stat.icps_inhist[ICMP_MASKREQ];
            stats->stats.icmpInStats.dwAddrMaskReps = icmp_stat.icps_inhist[ICMP_MASKREPLY];

#ifdef HAVE_STRUCT_ICMPSTAT_ICPS_OUTHIST
            /* out stats */
            stats->stats.icmpOutStats.dwMsgs = icmp_stat.icps_oldshort + icmp_stat.icps_oldicmp;
            for(i = 0; i <= ICMP_MAXTYPE; i++)
                stats->stats.icmpOutStats.dwMsgs += icmp_stat.icps_outhist[i];

            stats->stats.icmpOutStats.dwErrors = icmp_stat.icps_oldshort + icmp_stat.icps_oldicmp;

            stats->stats.icmpOutStats.dwDestUnreachs = icmp_stat.icps_outhist[ICMP_UNREACH];
            stats->stats.icmpOutStats.dwTimeExcds = icmp_stat.icps_outhist[ICMP_TIMXCEED];
            stats->stats.icmpOutStats.dwParmProbs = icmp_stat.icps_outhist[ICMP_PARAMPROB];
            stats->stats.icmpOutStats.dwSrcQuenchs = icmp_stat.icps_outhist[ICMP_SOURCEQUENCH];
            stats->stats.icmpOutStats.dwRedirects = icmp_stat.icps_outhist[ICMP_REDIRECT];
            stats->stats.icmpOutStats.dwEchos = icmp_stat.icps_outhist[ICMP_ECHO];
            stats->stats.icmpOutStats.dwEchoReps = icmp_stat.icps_outhist[ICMP_ECHOREPLY];
            stats->stats.icmpOutStats.dwTimestamps = icmp_stat.icps_outhist[ICMP_TSTAMP];
            stats->stats.icmpOutStats.dwTimestampReps = icmp_stat.icps_outhist[ICMP_TSTAMPREPLY];
            stats->stats.icmpOutStats.dwAddrMasks = icmp_stat.icps_outhist[ICMP_MASKREQ];
            stats->stats.icmpOutStats.dwAddrMaskReps = icmp_stat.icps_outhist[ICMP_MASKREPLY];
#endif /* HAVE_STRUCT_ICMPSTAT_ICPS_OUTHIST */
            ret = NO_ERROR;
        }
    }
#else /* ICMPCTL_STATS */
    FIXME( "unimplemented\n" );
#endif
    return ret;
}


/******************************************************************
 *    GetIpStatistics (IPHLPAPI.@)
 *
 * Get the IP statistics for the local computer.
 *
 * PARAMS
 *  stats [Out] buffer for IP statistics
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 */
DWORD WINAPI GetIpStatistics(PMIB_IPSTATS stats)
{
    DWORD ret = ERROR_NOT_SUPPORTED;
    MIB_IPFORWARDTABLE *fwd_table;

    if (!stats) return ERROR_INVALID_PARAMETER;
    memset( stats, 0, sizeof(*stats) );

    stats->dwNumIf = stats->dwNumAddr = getNumInterfaces();
    if (!AllocateAndGetIpForwardTableFromStack( &fwd_table, FALSE, GetProcessHeap(), 0 ))
    {
        stats->dwNumRoutes = fwd_table->dwNumEntries;
        HeapFree( GetProcessHeap(), 0, fwd_table );
    }

#ifdef __linux__
    {
        FILE *fp;

        if ((fp = fopen("/proc/net/snmp", "r")))
        {
            static const char hdr[] = "Ip:";
            char buf[512], *ptr;

            while ((ptr = fgets(buf, sizeof(buf), fp)))
            {
                if (strncasecmp(buf, hdr, sizeof(hdr) - 1)) continue;
                /* last line was a header, get another */
                if (!(ptr = fgets(buf, sizeof(buf), fp))) break;
                if (!strncasecmp(buf, hdr, sizeof(hdr) - 1))
                {
                    ptr += sizeof(hdr);
                    sscanf( ptr, "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u",
                            &stats->u.dwForwarding,
                            &stats->dwDefaultTTL,
                            &stats->dwInReceives,
                            &stats->dwInHdrErrors,
                            &stats->dwInAddrErrors,
                            &stats->dwForwDatagrams,
                            &stats->dwInUnknownProtos,
                            &stats->dwInDiscards,
                            &stats->dwInDelivers,
                            &stats->dwOutRequests,
                            &stats->dwOutDiscards,
                            &stats->dwOutNoRoutes,
                            &stats->dwReasmTimeout,
                            &stats->dwReasmReqds,
                            &stats->dwReasmOks,
                            &stats->dwReasmFails,
                            &stats->dwFragOks,
                            &stats->dwFragFails,
                            &stats->dwFragCreates );
                    /* hmm, no routingDiscards */
                    break;
                }
            }
            fclose(fp);
            ret = NO_ERROR;
        }
    }
#elif defined(HAVE_LIBKSTAT)
    {
        static char ip[] = "ip";
        kstat_ctl_t *kc;
        kstat_t *ksp;

        if ((kc = kstat_open()) &&
            (ksp = kstat_lookup( kc, ip, 0, ip )) &&
            kstat_read( kc, ksp, NULL ) != -1 &&
            ksp->ks_type == KSTAT_TYPE_NAMED)
        {
            stats->u.dwForwarding    = kstat_get_ui32( ksp, "forwarding" );
            stats->dwDefaultTTL      = kstat_get_ui32( ksp, "defaultTTL" );
            stats->dwInReceives      = kstat_get_ui32( ksp, "inReceives" );
            stats->dwInHdrErrors     = kstat_get_ui32( ksp, "inHdrErrors" );
            stats->dwInAddrErrors    = kstat_get_ui32( ksp, "inAddrErrors" );
            stats->dwForwDatagrams   = kstat_get_ui32( ksp, "forwDatagrams" );
            stats->dwInUnknownProtos = kstat_get_ui32( ksp, "inUnknownProtos" );
            stats->dwInDiscards      = kstat_get_ui32( ksp, "inDiscards" );
            stats->dwInDelivers      = kstat_get_ui32( ksp, "inDelivers" );
            stats->dwOutRequests     = kstat_get_ui32( ksp, "outRequests" );
            stats->dwRoutingDiscards = kstat_get_ui32( ksp, "routingDiscards" );
            stats->dwOutDiscards     = kstat_get_ui32( ksp, "outDiscards" );
            stats->dwOutNoRoutes     = kstat_get_ui32( ksp, "outNoRoutes" );
            stats->dwReasmTimeout    = kstat_get_ui32( ksp, "reasmTimeout" );
            stats->dwReasmReqds      = kstat_get_ui32( ksp, "reasmReqds" );
            stats->dwReasmOks        = kstat_get_ui32( ksp, "reasmOKs" );
            stats->dwReasmFails      = kstat_get_ui32( ksp, "reasmFails" );
            stats->dwFragOks         = kstat_get_ui32( ksp, "fragOKs" );
            stats->dwFragFails       = kstat_get_ui32( ksp, "fragFails" );
            stats->dwFragCreates     = kstat_get_ui32( ksp, "fragCreates" );
            ret = NO_ERROR;
        }
        if (kc) kstat_close( kc );
    }
#elif defined(HAVE_SYS_SYSCTL_H) && defined(IPCTL_STATS) && (defined(HAVE_STRUCT_IPSTAT_IPS_TOTAL) || defined(HAVE_STRUCT_IP_STATS_IPS_TOTAL))
    {
        int mib[] = {CTL_NET, PF_INET, IPPROTO_IP, IPCTL_STATS};
#define MIB_LEN (sizeof(mib) / sizeof(mib[0]))
        int ip_ttl, ip_forwarding;
#if defined(HAVE_STRUCT_IPSTAT_IPS_TOTAL)
        struct ipstat ip_stat;
#elif defined(HAVE_STRUCT_IP_STATS_IPS_TOTAL)
        struct ip_stats ip_stat;
#endif
        size_t needed;

        needed = sizeof(ip_stat);
        if(sysctl(mib, MIB_LEN, &ip_stat, &needed, NULL, 0) == -1)
        {
            ERR ("failed to get ipstat\n");
            return ERROR_NOT_SUPPORTED;
        }

        needed = sizeof(ip_ttl);
        if (sysctlbyname ("net.inet.ip.ttl", &ip_ttl, &needed, NULL, 0) == -1)
        {
            ERR ("failed to get ip Default TTL\n");
            return ERROR_NOT_SUPPORTED;
        }

        needed = sizeof(ip_forwarding);
        if (sysctlbyname ("net.inet.ip.forwarding", &ip_forwarding, &needed, NULL, 0) == -1)
        {
            ERR ("failed to get ip forwarding\n");
            return ERROR_NOT_SUPPORTED;
        }

        stats->u.dwForwarding = ip_forwarding;
        stats->dwDefaultTTL = ip_ttl;
        stats->dwInDelivers = ip_stat.ips_delivered;
        stats->dwInHdrErrors = ip_stat.ips_badhlen + ip_stat.ips_badsum + ip_stat.ips_tooshort + ip_stat.ips_badlen;
        stats->dwInAddrErrors = ip_stat.ips_cantforward;
        stats->dwInReceives = ip_stat.ips_total;
        stats->dwForwDatagrams = ip_stat.ips_forward;
        stats->dwInUnknownProtos = ip_stat.ips_noproto;
        stats->dwInDiscards = ip_stat.ips_fragdropped;
        stats->dwOutDiscards = ip_stat.ips_odropped;
        stats->dwReasmOks = ip_stat.ips_reassembled;
        stats->dwFragOks = ip_stat.ips_fragmented;
        stats->dwFragFails = ip_stat.ips_cantfrag;
        stats->dwReasmTimeout = ip_stat.ips_fragtimeout;
        stats->dwOutNoRoutes = ip_stat.ips_noroute;
        stats->dwOutRequests = ip_stat.ips_localout;
        stats->dwReasmReqds = ip_stat.ips_fragments;
        ret = NO_ERROR;
    }
#else
    FIXME( "unimplemented\n" );
#endif
    return ret;
}


/******************************************************************
 *    GetTcpStatistics (IPHLPAPI.@)
 *
 * Get the TCP statistics for the local computer.
 *
 * PARAMS
 *  stats [Out] buffer for TCP statistics
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 */
DWORD WINAPI GetTcpStatistics(PMIB_TCPSTATS stats)
{
    DWORD ret = ERROR_NOT_SUPPORTED;

    if (!stats) return ERROR_INVALID_PARAMETER;
    memset( stats, 0, sizeof(*stats) );

#ifdef __linux__
    {
        FILE *fp;

        if ((fp = fopen("/proc/net/snmp", "r")))
        {
            static const char hdr[] = "Tcp:";
            MIB_TCPTABLE *tcp_table;
            char buf[512], *ptr;

            while ((ptr = fgets(buf, sizeof(buf), fp)))
            {
                if (strncasecmp(buf, hdr, sizeof(hdr) - 1)) continue;
                /* last line was a header, get another */
                if (!(ptr = fgets(buf, sizeof(buf), fp))) break;
                if (!strncasecmp(buf, hdr, sizeof(hdr) - 1))
                {
                    ptr += sizeof(hdr);
                    sscanf( ptr, "%u %u %u %u %u %u %u %u %u %u %u %u %u %u",
                            &stats->u.dwRtoAlgorithm,
                            &stats->dwRtoMin,
                            &stats->dwRtoMax,
                            &stats->dwMaxConn,
                            &stats->dwActiveOpens,
                            &stats->dwPassiveOpens,
                            &stats->dwAttemptFails,
                            &stats->dwEstabResets,
                            &stats->dwCurrEstab,
                            &stats->dwInSegs,
                            &stats->dwOutSegs,
                            &stats->dwRetransSegs,
                            &stats->dwInErrs,
                            &stats->dwOutRsts );
                    break;
                }
            }
            if (!AllocateAndGetTcpTableFromStack( &tcp_table, FALSE, GetProcessHeap(), 0 ))
            {
                stats->dwNumConns = tcp_table->dwNumEntries;
                HeapFree( GetProcessHeap(), 0, tcp_table );
            }
            fclose(fp);
            ret = NO_ERROR;
        }
    }
#elif defined(HAVE_LIBKSTAT)
    {
        static char tcp[] = "tcp";
        kstat_ctl_t *kc;
        kstat_t *ksp;

        if ((kc = kstat_open()) &&
            (ksp = kstat_lookup( kc, tcp, 0, tcp )) &&
            kstat_read( kc, ksp, NULL ) != -1 &&
            ksp->ks_type == KSTAT_TYPE_NAMED)
        {
            stats->u.dwRtoAlgorithm = kstat_get_ui32( ksp, "rtoAlgorithm" );
            stats->dwRtoMin       = kstat_get_ui32( ksp, "rtoMin" );
            stats->dwRtoMax       = kstat_get_ui32( ksp, "rtoMax" );
            stats->dwMaxConn      = kstat_get_ui32( ksp, "maxConn" );
            stats->dwActiveOpens  = kstat_get_ui32( ksp, "activeOpens" );
            stats->dwPassiveOpens = kstat_get_ui32( ksp, "passiveOpens" );
            stats->dwAttemptFails = kstat_get_ui32( ksp, "attemptFails" );
            stats->dwEstabResets  = kstat_get_ui32( ksp, "estabResets" );
            stats->dwCurrEstab    = kstat_get_ui32( ksp, "currEstab" );
            stats->dwInSegs       = kstat_get_ui32( ksp, "inSegs" );
            stats->dwOutSegs      = kstat_get_ui32( ksp, "outSegs" );
            stats->dwRetransSegs  = kstat_get_ui32( ksp, "retransSegs" );
            stats->dwInErrs       = kstat_get_ui32( ksp, "inErrs" );
            stats->dwOutRsts      = kstat_get_ui32( ksp, "outRsts" );
            stats->dwNumConns     = kstat_get_ui32( ksp, "connTableSize" );
            ret = NO_ERROR;
        }
        if (kc) kstat_close( kc );
    }
#elif defined(HAVE_SYS_SYSCTL_H) && defined(TCPCTL_STATS) && (defined(HAVE_STRUCT_TCPSTAT_TCPS_CONNATTEMPT) || defined(HAVE_STRUCT_TCP_STATS_TCPS_CONNATTEMPT))
    {
#ifndef TCPTV_MIN  /* got removed in Mac OS X for some reason */
#define TCPTV_MIN 2
#define TCPTV_REXMTMAX 128
#endif
        int mib[] = {CTL_NET, PF_INET, IPPROTO_TCP, TCPCTL_STATS};
#define MIB_LEN (sizeof(mib) / sizeof(mib[0]))
#define hz 1000
#if defined(HAVE_STRUCT_TCPSTAT_TCPS_CONNATTEMPT)
        struct tcpstat tcp_stat;
#elif defined(HAVE_STRUCT_TCP_STATS_TCPS_CONNATTEMPT)
        struct tcp_stats tcp_stat;
#endif
        size_t needed = sizeof(tcp_stat);

        if(sysctl(mib, MIB_LEN, &tcp_stat, &needed, NULL, 0) != -1)
        {
            stats->u.RtoAlgorithm = MIB_TCP_RTO_VANJ;
            stats->dwRtoMin = TCPTV_MIN;
            stats->dwRtoMax = TCPTV_REXMTMAX;
            stats->dwMaxConn = -1;
            stats->dwActiveOpens = tcp_stat.tcps_connattempt;
            stats->dwPassiveOpens = tcp_stat.tcps_accepts;
            stats->dwAttemptFails = tcp_stat.tcps_conndrops;
            stats->dwEstabResets = tcp_stat.tcps_drops;
            stats->dwCurrEstab = 0;
            stats->dwInSegs = tcp_stat.tcps_rcvtotal;
            stats->dwOutSegs = tcp_stat.tcps_sndtotal - tcp_stat.tcps_sndrexmitpack;
            stats->dwRetransSegs = tcp_stat.tcps_sndrexmitpack;
            stats->dwInErrs = tcp_stat.tcps_rcvbadsum + tcp_stat.tcps_rcvbadoff + tcp_stat.tcps_rcvmemdrop + tcp_stat.tcps_rcvshort;
            stats->dwOutRsts = tcp_stat.tcps_sndctrl - tcp_stat.tcps_closed;
            stats->dwNumConns = tcp_stat.tcps_connects;
            ret = NO_ERROR;
        }
        else ERR ("failed to get tcpstat\n");
    }
#else
    FIXME( "unimplemented\n" );
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
    DWORD ret = ERROR_NOT_SUPPORTED;

    if (!stats) return ERROR_INVALID_PARAMETER;
    memset( stats, 0, sizeof(*stats) );

#ifdef __linux__
    {
        FILE *fp;

        if ((fp = fopen("/proc/net/snmp", "r")))
        {
            static const char hdr[] = "Udp:";
            char buf[512], *ptr;

            while ((ptr = fgets(buf, sizeof(buf), fp)))
            {
                if (strncasecmp(buf, hdr, sizeof(hdr) - 1)) continue;
                /* last line was a header, get another */
                if (!(ptr = fgets(buf, sizeof(buf), fp))) break;
                if (!strncasecmp(buf, hdr, sizeof(hdr) - 1))
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
#define MIB_LEN (sizeof(mib) / sizeof(mib[0]))
        struct udpstat udp_stat;
        MIB_UDPTABLE *udp_table;
        size_t needed = sizeof(udp_stat);

        if(sysctl(mib, MIB_LEN, &udp_stat, &needed, NULL, 0) != -1)
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
    FIXME( "unimplemented\n" );
#endif
    return ret;
}


static MIB_IPFORWARDTABLE *append_ipforward_row( HANDLE heap, DWORD flags, MIB_IPFORWARDTABLE *table,
                                                 DWORD *count, const MIB_IPFORWARDROW *row )
{
    if (table->dwNumEntries >= *count)
    {
        MIB_IPFORWARDTABLE *new_table;
        DWORD new_count = table->dwNumEntries * 2;

        if (!(new_table = HeapReAlloc( heap, flags, table,
                                       FIELD_OFFSET(MIB_IPFORWARDTABLE, table[new_count] ))))
        {
            HeapFree( heap, 0, table );
            return NULL;
        }
        *count = new_count;
        table = new_table;
    }
    memcpy( &table->table[table->dwNumEntries++], row, sizeof(*row) );
    return table;
}

static int compare_ipforward_rows(const void *a, const void *b)
{
    const MIB_IPFORWARDROW *rowA = a;
    const MIB_IPFORWARDROW *rowB = b;
    int ret;

    if ((ret = rowA->dwForwardDest - rowB->dwForwardDest) != 0) return ret;
    if ((ret = rowA->u2.dwForwardProto - rowB->u2.dwForwardProto) != 0) return ret;
    if ((ret = rowA->dwForwardPolicy - rowB->dwForwardPolicy) != 0) return ret;
    return rowA->dwForwardNextHop - rowB->dwForwardNextHop;
}

/******************************************************************
 *    AllocateAndGetIpForwardTableFromStack (IPHLPAPI.@)
 *
 * Get the route table.
 * Like GetIpForwardTable(), but allocate the returned table from heap.
 *
 * PARAMS
 *  ppIpForwardTable [Out] pointer into which the MIB_IPFORWARDTABLE is
 *                         allocated and returned.
 *  bOrder           [In]  whether to sort the table
 *  heap             [In]  heap from which the table is allocated
 *  flags            [In]  flags to HeapAlloc
 *
 * RETURNS
 *  ERROR_INVALID_PARAMETER if ppIfTable is NULL, other error codes
 *  on failure, NO_ERROR on success.
 */
DWORD WINAPI AllocateAndGetIpForwardTableFromStack(PMIB_IPFORWARDTABLE *ppIpForwardTable, BOOL bOrder,
                                                   HANDLE heap, DWORD flags)
{
    MIB_IPFORWARDTABLE *table;
    MIB_IPFORWARDROW row;
    DWORD ret = NO_ERROR, count = 16;

    TRACE("table %p, bOrder %d, heap %p, flags 0x%08x\n", ppIpForwardTable, bOrder, heap, flags);

    if (!ppIpForwardTable) return ERROR_INVALID_PARAMETER;

    if (!(table = HeapAlloc( heap, flags, FIELD_OFFSET(MIB_IPFORWARDTABLE, table[count] ))))
        return ERROR_OUTOFMEMORY;

    table->dwNumEntries = 0;

#ifdef __linux__
    {
        FILE *fp;

        if ((fp = fopen("/proc/net/route", "r")))
        {
            char buf[512], *ptr;
            DWORD flags;

            /* skip header line */
            ptr = fgets(buf, sizeof(buf), fp);
            while ((ptr = fgets(buf, sizeof(buf), fp)))
            {
                memset( &row, 0, sizeof(row) );

                while (!isspace(*ptr)) ptr++;
                *ptr++ = 0;
                if (getInterfaceIndexByName(buf, &row.dwForwardIfIndex) != NO_ERROR)
                    continue;

                row.dwForwardDest = strtoul(ptr, &ptr, 16);
                row.dwForwardNextHop = strtoul(ptr + 1, &ptr, 16);
                flags = strtoul(ptr + 1, &ptr, 16);

                if (!(flags & RTF_UP)) row.u1.ForwardType = MIB_IPROUTE_TYPE_INVALID;
                else if (flags & RTF_GATEWAY) row.u1.ForwardType = MIB_IPROUTE_TYPE_INDIRECT;
                else row.u1.ForwardType = MIB_IPROUTE_TYPE_DIRECT;

                strtoul(ptr + 1, &ptr, 16); /* refcount, skip */
                strtoul(ptr + 1, &ptr, 16); /* use, skip */
                row.dwForwardMetric1 = strtoul(ptr + 1, &ptr, 16);
                row.dwForwardMask = strtoul(ptr + 1, &ptr, 16);
                /* FIXME: other protos might be appropriate, e.g. the default
                 * route is typically set with MIB_IPPROTO_NETMGMT instead */
                row.u2.ForwardProto = MIB_IPPROTO_LOCAL;

                if (!(table = append_ipforward_row( heap, flags, table, &count, &row )))
                    break;
            }
            fclose(fp);
        }
        else ret = ERROR_NOT_SUPPORTED;
    }
#elif defined(HAVE_SYS_TIHDR_H) && defined(T_OPTMGMT_ACK)
    {
        void *data;
        int fd, len, namelen;
        mib2_ipRouteEntry_t *entry;
        char name[64];

        if ((fd = open_streams_mib( NULL )) != -1)
        {
            if ((data = read_mib_entry( fd, MIB2_IP, MIB2_IP_ROUTE, &len )))
            {
                for (entry = data; (char *)(entry + 1) <= (char *)data + len; entry++)
                {
                    row.dwForwardDest      = entry->ipRouteDest;
                    row.dwForwardMask      = entry->ipRouteMask;
                    row.dwForwardPolicy    = 0;
                    row.dwForwardNextHop   = entry->ipRouteNextHop;
                    row.u1.dwForwardType   = entry->ipRouteType;
                    row.u2.dwForwardProto  = entry->ipRouteProto;
                    row.dwForwardAge       = entry->ipRouteAge;
                    row.dwForwardNextHopAS = 0;
                    row.dwForwardMetric1   = entry->ipRouteMetric1;
                    row.dwForwardMetric2   = entry->ipRouteMetric2;
                    row.dwForwardMetric3   = entry->ipRouteMetric3;
                    row.dwForwardMetric4   = entry->ipRouteMetric4;
                    row.dwForwardMetric5   = entry->ipRouteMetric5;
                    namelen = min( sizeof(name) - 1, entry->ipRouteIfIndex.o_length );
                    memcpy( name, entry->ipRouteIfIndex.o_bytes, namelen );
                    name[namelen] = 0;
                    getInterfaceIndexByName( name, &row.dwForwardIfIndex );
                    if (!(table = append_ipforward_row( heap, flags, table, &count, &row ))) break;
                }
                HeapFree( GetProcessHeap(), 0, data );
            }
            close( fd );
        }
        else ret = ERROR_NOT_SUPPORTED;
    }
#elif defined(HAVE_SYS_SYSCTL_H) && defined(NET_RT_DUMP)
    {
       int mib[6] = {CTL_NET, PF_ROUTE, 0, PF_INET, NET_RT_DUMP, 0};
       size_t needed;
       char *buf = NULL, *lim, *next, *addrPtr;
       struct rt_msghdr *rtm;

       if (sysctl (mib, 6, NULL, &needed, NULL, 0) < 0)
       {
          ERR ("sysctl 1 failed!\n");
          ret = ERROR_NOT_SUPPORTED;
          goto done;
       }

       buf = HeapAlloc (GetProcessHeap (), 0, needed);
       if (!buf)
       {
          ret = ERROR_OUTOFMEMORY;
          goto done;
       }

       if (sysctl (mib, 6, buf, &needed, NULL, 0) < 0)
       {
          ret = ERROR_NOT_SUPPORTED;
          goto done;
       }

       lim = buf + needed;
       for (next = buf; next < lim; next += rtm->rtm_msglen)
       {
          int i;

          rtm = (struct rt_msghdr *)next;

          if (rtm->rtm_type != RTM_GET)
          {
             WARN ("Got unexpected message type 0x%x!\n",
                   rtm->rtm_type);
             continue;
          }

          /* Ignore all entries except for gateway routes which aren't
             multicast */
          if (!(rtm->rtm_flags & RTF_GATEWAY) ||
              (rtm->rtm_flags & RTF_MULTICAST))
             continue;

          memset( &row, 0, sizeof(row) );
          row.dwForwardIfIndex = rtm->rtm_index;
          row.u1.ForwardType = MIB_IPROUTE_TYPE_INDIRECT;
          row.dwForwardMetric1 = rtm->rtm_rmx.rmx_hopcount;
          row.u2.ForwardProto = MIB_IPPROTO_LOCAL;

          addrPtr = (char *)(rtm + 1);

          for (i = 1; i; i <<= 1)
          {
             struct sockaddr *sa;
             DWORD addr;

             if (!(i & rtm->rtm_addrs))
                continue;

             sa = (struct sockaddr *)addrPtr;
             ADVANCE (addrPtr, sa);

             /* default routes are encoded by length-zero sockaddr */
             if (sa->sa_len == 0)
                addr = 0;
             else if (sa->sa_family != AF_INET)
             {
                WARN ("Received unsupported sockaddr family 0x%x\n",
                     sa->sa_family);
                addr = 0;
             }
             else
             {
                struct sockaddr_in *sin = (struct sockaddr_in *)sa;

                addr = sin->sin_addr.s_addr;
             }

             switch (i)
             {
                case RTA_DST:     row.dwForwardDest = addr; break;
                case RTA_GATEWAY: row.dwForwardNextHop = addr; break;
                case RTA_NETMASK: row.dwForwardMask = addr; break;
                default:
                   WARN ("Unexpected address type 0x%x\n", i);
             }
          }

          if (!(table = append_ipforward_row( heap, flags, table, &count, &row )))
              break;
       }
done:
      HeapFree( GetProcessHeap (), 0, buf );
    }
#else
    FIXME( "not implemented\n" );
    ret = ERROR_NOT_SUPPORTED;
#endif

    if (!table) return ERROR_OUTOFMEMORY;
    if (!ret)
    {
        if (bOrder && table->dwNumEntries)
            qsort( table->table, table->dwNumEntries, sizeof(row), compare_ipforward_rows );
        *ppIpForwardTable = table;
    }
    else HeapFree( heap, flags, table );
    TRACE( "returning ret %u table %p\n", ret, table );
    return ret;
}

static MIB_IPNETTABLE *append_ipnet_row( HANDLE heap, DWORD flags, MIB_IPNETTABLE *table,
                                         DWORD *count, const MIB_IPNETROW *row )
{
    if (table->dwNumEntries >= *count)
    {
        MIB_IPNETTABLE *new_table;
        DWORD new_count = table->dwNumEntries * 2;

        if (!(new_table = HeapReAlloc( heap, flags, table,
                                       FIELD_OFFSET(MIB_IPNETTABLE, table[new_count] ))))
        {
            HeapFree( heap, 0, table );
            return NULL;
        }
        *count = new_count;
        table = new_table;
    }
    memcpy( &table->table[table->dwNumEntries++], row, sizeof(*row) );
    return table;
}

static int compare_ipnet_rows(const void *a, const void *b)
{
    const MIB_IPNETROW *rowA = a;
    const MIB_IPNETROW *rowB = b;

    return ntohl(rowA->dwAddr) - ntohl(rowB->dwAddr);
}


/******************************************************************
 *    AllocateAndGetIpNetTableFromStack (IPHLPAPI.@)
 *
 * Get the IP-to-physical address mapping table.
 * Like GetIpNetTable(), but allocate the returned table from heap.
 *
 * PARAMS
 *  ppIpNetTable [Out] pointer into which the MIB_IPNETTABLE is
 *                     allocated and returned.
 *  bOrder       [In]  whether to sort the table
 *  heap         [In]  heap from which the table is allocated
 *  flags        [In]  flags to HeapAlloc
 *
 * RETURNS
 *  ERROR_INVALID_PARAMETER if ppIpNetTable is NULL, other error codes
 *  on failure, NO_ERROR on success.
 */
DWORD WINAPI AllocateAndGetIpNetTableFromStack(PMIB_IPNETTABLE *ppIpNetTable, BOOL bOrder,
                                               HANDLE heap, DWORD flags)
{
    MIB_IPNETTABLE *table;
    MIB_IPNETROW row;
    DWORD ret = NO_ERROR, count = 16;

    TRACE("table %p, bOrder %d, heap %p, flags 0x%08x\n", ppIpNetTable, bOrder, heap, flags);

    if (!ppIpNetTable) return ERROR_INVALID_PARAMETER;

    if (!(table = HeapAlloc( heap, flags, FIELD_OFFSET(MIB_IPNETTABLE, table[count] ))))
        return ERROR_OUTOFMEMORY;

    table->dwNumEntries = 0;

#ifdef __linux__
    {
        FILE *fp;

        if ((fp = fopen("/proc/net/arp", "r")))
        {
            char buf[512], *ptr;
            DWORD flags;

            /* skip header line */
            ptr = fgets(buf, sizeof(buf), fp);
            while ((ptr = fgets(buf, sizeof(buf), fp)))
            {
                memset( &row, 0, sizeof(row) );

                row.dwAddr = inet_addr(ptr);
                while (*ptr && !isspace(*ptr)) ptr++;
                strtoul(ptr + 1, &ptr, 16); /* hw type (skip) */
                flags = strtoul(ptr + 1, &ptr, 16);

#ifdef ATF_COM
                if (flags & ATF_COM) row.u.Type = MIB_IPNET_TYPE_DYNAMIC;
                else
#endif
#ifdef ATF_PERM
                if (flags & ATF_PERM) row.u.Type = MIB_IPNET_TYPE_STATIC;
                else
#endif
                    row.u.Type = MIB_IPNET_TYPE_OTHER;

                while (*ptr && isspace(*ptr)) ptr++;
                while (*ptr && !isspace(*ptr))
                {
                    row.bPhysAddr[row.dwPhysAddrLen++] = strtoul(ptr, &ptr, 16);
                    if (*ptr) ptr++;
                }
                while (*ptr && isspace(*ptr)) ptr++;
                while (*ptr && !isspace(*ptr)) ptr++;   /* mask (skip) */
                while (*ptr && isspace(*ptr)) ptr++;
                getInterfaceIndexByName(ptr, &row.dwIndex);

                if (!(table = append_ipnet_row( heap, flags, table, &count, &row )))
                    break;
            }
            fclose(fp);
        }
        else ret = ERROR_NOT_SUPPORTED;
    }
#elif defined(HAVE_SYS_TIHDR_H) && defined(T_OPTMGMT_ACK)
    {
        void *data;
        int fd, len, namelen;
        mib2_ipNetToMediaEntry_t *entry;
        char name[64];

        if ((fd = open_streams_mib( NULL )) != -1)
        {
            if ((data = read_mib_entry( fd, MIB2_IP, MIB2_IP_MEDIA, &len )))
            {
                for (entry = data; (char *)(entry + 1) <= (char *)data + len; entry++)
                {
                    row.dwPhysAddrLen = min( entry->ipNetToMediaPhysAddress.o_length, MAXLEN_PHYSADDR );
                    memcpy( row.bPhysAddr, entry->ipNetToMediaPhysAddress.o_bytes, row.dwPhysAddrLen );
                    row.dwAddr = entry->ipNetToMediaNetAddress;
                    row.u.Type = entry->ipNetToMediaType;
                    namelen = min( sizeof(name) - 1, entry->ipNetToMediaIfIndex.o_length );
                    memcpy( name, entry->ipNetToMediaIfIndex.o_bytes, namelen );
                    name[namelen] = 0;
                    getInterfaceIndexByName( name, &row.dwIndex );
                    if (!(table = append_ipnet_row( heap, flags, table, &count, &row ))) break;
                }
                HeapFree( GetProcessHeap(), 0, data );
            }
            close( fd );
        }
        else ret = ERROR_NOT_SUPPORTED;
    }
#elif defined(HAVE_SYS_SYSCTL_H) && defined(NET_RT_DUMP)
    {
      int mib[] = {CTL_NET, PF_ROUTE, 0, AF_INET, NET_RT_FLAGS, RTF_LLINFO};
#define MIB_LEN (sizeof(mib) / sizeof(mib[0]))
      size_t needed;
      char *buf = NULL, *lim, *next;
      struct rt_msghdr *rtm;
      struct sockaddr_inarp *sinarp;
      struct sockaddr_dl *sdl;

      if (sysctl (mib, MIB_LEN,  NULL, &needed, NULL, 0) == -1)
      {
         ERR ("failed to get arp table\n");
         ret = ERROR_NOT_SUPPORTED;
         goto done;
      }

      buf = HeapAlloc (GetProcessHeap (), 0, needed);
      if (!buf)
      {
          ret = ERROR_OUTOFMEMORY;
          goto done;
      }

      if (sysctl (mib, MIB_LEN, buf, &needed, NULL, 0) == -1)
      {
         ret = ERROR_NOT_SUPPORTED;
         goto done;
      }

      lim = buf + needed;
      next = buf;
      while(next < lim)
      {
          rtm = (struct rt_msghdr *)next;
          sinarp=(struct sockaddr_inarp *)(rtm + 1);
          sdl = (struct sockaddr_dl *)((char *)sinarp + ROUNDUP(sinarp->sin_len));
          if(sdl->sdl_alen) /* arp entry */
          {
              memset( &row, 0, sizeof(row) );
              row.dwAddr = sinarp->sin_addr.s_addr;
              row.dwIndex = sdl->sdl_index;
              row.dwPhysAddrLen = min( 8, sdl->sdl_alen );
              memcpy( row.bPhysAddr, &sdl->sdl_data[sdl->sdl_nlen], row.dwPhysAddrLen );
              if(rtm->rtm_rmx.rmx_expire == 0) row.u.Type = MIB_IPNET_TYPE_STATIC;
              else if(sinarp->sin_other & SIN_PROXY) row.u.Type = MIB_IPNET_TYPE_OTHER;
              else if(rtm->rtm_rmx.rmx_expire != 0) row.u.Type = MIB_IPNET_TYPE_DYNAMIC;
              else row.u.Type = MIB_IPNET_TYPE_INVALID;

              if (!(table = append_ipnet_row( heap, flags, table, &count, &row )))
                  break;
          }
          next += rtm->rtm_msglen;
      }
done:
      HeapFree( GetProcessHeap (), 0, buf );
    }
#else
    FIXME( "not implemented\n" );
    ret = ERROR_NOT_SUPPORTED;
#endif

    if (!table) return ERROR_OUTOFMEMORY;
    if (!ret)
    {
        if (bOrder && table->dwNumEntries)
            qsort( table->table, table->dwNumEntries, sizeof(row), compare_ipnet_rows );
        *ppIpNetTable = table;
    }
    else HeapFree( heap, flags, table );
    TRACE( "returning ret %u table %p\n", ret, table );
    return ret;
}


static MIB_UDPTABLE *append_udp_row( HANDLE heap, DWORD flags, MIB_UDPTABLE *table,
                                     DWORD *count, const MIB_UDPROW *row )
{
    if (table->dwNumEntries >= *count)
    {
        MIB_UDPTABLE *new_table;
        DWORD new_count = table->dwNumEntries * 2;

        if (!(new_table = HeapReAlloc( heap, flags, table, FIELD_OFFSET(MIB_UDPTABLE, table[new_count] ))))
        {
            HeapFree( heap, 0, table );
            return NULL;
        }
        *count = new_count;
        table = new_table;
    }
    memcpy( &table->table[table->dwNumEntries++], row, sizeof(*row) );
    return table;
}

static int compare_udp_rows(const void *a, const void *b)
{
    const MIB_UDPROW *rowA = a;
    const MIB_UDPROW *rowB = b;
    int ret;

    if ((ret = rowA->dwLocalAddr - rowB->dwLocalAddr) != 0) return ret;
    return rowA->dwLocalPort - rowB->dwLocalPort;
}


/******************************************************************
 *    AllocateAndGetUdpTableFromStack (IPHLPAPI.@)
 *
 * Get the UDP listener table.
 * Like GetUdpTable(), but allocate the returned table from heap.
 *
 * PARAMS
 *  ppUdpTable [Out] pointer into which the MIB_UDPTABLE is
 *                   allocated and returned.
 *  bOrder     [In]  whether to sort the table
 *  heap       [In]  heap from which the table is allocated
 *  flags      [In]  flags to HeapAlloc
 *
 * RETURNS
 *  ERROR_INVALID_PARAMETER if ppUdpTable is NULL, whatever GetUdpTable()
 *  returns otherwise.
 */
DWORD WINAPI AllocateAndGetUdpTableFromStack(PMIB_UDPTABLE *ppUdpTable, BOOL bOrder,
                                             HANDLE heap, DWORD flags)
{
    MIB_UDPTABLE *table;
    MIB_UDPROW row;
    DWORD ret = NO_ERROR, count = 16;

    TRACE("table %p, bOrder %d, heap %p, flags 0x%08x\n", ppUdpTable, bOrder, heap, flags);

    if (!ppUdpTable) return ERROR_INVALID_PARAMETER;

    if (!(table = HeapAlloc( heap, flags, FIELD_OFFSET(MIB_UDPTABLE, table[count] ))))
        return ERROR_OUTOFMEMORY;

    table->dwNumEntries = 0;

#ifdef __linux__
    {
        FILE *fp;

        if ((fp = fopen("/proc/net/udp", "r")))
        {
            char buf[512], *ptr;
            DWORD dummy;

            /* skip header line */
            ptr = fgets(buf, sizeof(buf), fp);
            while ((ptr = fgets(buf, sizeof(buf), fp)))
            {
                if (sscanf( ptr, "%u: %x:%x", &dummy, &row.dwLocalAddr, &row.dwLocalPort ) != 3)
                    continue;
                row.dwLocalPort = htons( row.dwLocalPort );
                if (!(table = append_udp_row( heap, flags, table, &count, &row )))
                    break;
            }
            fclose(fp);
        }
        else ret = ERROR_NOT_SUPPORTED;
    }
#elif defined(HAVE_SYS_TIHDR_H) && defined(T_OPTMGMT_ACK)
    {
        void *data;
        int fd, len;
        mib2_udpEntry_t *entry;

        if ((fd = open_streams_mib( "udp" )) != -1)
        {
            if ((data = read_mib_entry( fd, MIB2_UDP, MIB2_UDP_ENTRY, &len )))
            {
                for (entry = data; (char *)(entry + 1) <= (char *)data + len; entry++)
                {
                    row.dwLocalAddr = entry->udpLocalAddress;
                    row.dwLocalPort = htons( entry->udpLocalPort );
                    if (!(table = append_udp_row( heap, flags, table, &count, &row ))) break;
                }
                HeapFree( GetProcessHeap(), 0, data );
            }
            close( fd );
        }
        else ret = ERROR_NOT_SUPPORTED;
    }
#elif defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_STRUCT_XINPGEN)
    {
        size_t Len = 0;
        char *Buf = NULL;
        struct xinpgen *pXIG, *pOrigXIG;

        if (sysctlbyname ("net.inet.udp.pcblist", NULL, &Len, NULL, 0) < 0)
        {
            ERR ("Failure to read net.inet.udp.pcblist via sysctlbyname!\n");
            ret = ERROR_NOT_SUPPORTED;
            goto done;
        }

        Buf = HeapAlloc (GetProcessHeap (), 0, Len);
        if (!Buf)
        {
            ret = ERROR_OUTOFMEMORY;
            goto done;
        }

        if (sysctlbyname ("net.inet.udp.pcblist", Buf, &Len, NULL, 0) < 0)
        {
            ERR ("Failure to read net.inet.udp.pcblist via sysctlbyname!\n");
            ret = ERROR_NOT_SUPPORTED;
            goto done;
        }

        /* Might be nothing here; first entry is just a header it seems */
        if (Len <= sizeof (struct xinpgen)) goto done;

        pOrigXIG = (struct xinpgen *)Buf;
        pXIG = pOrigXIG;

        for (pXIG = (struct xinpgen *)((char *)pXIG + pXIG->xig_len);
             pXIG->xig_len > sizeof (struct xinpgen);
             pXIG = (struct xinpgen *)((char *)pXIG + pXIG->xig_len))
        {
            struct inpcb *pINData;
            struct xsocket *pSockData;

            pINData = &((struct xinpcb *)pXIG)->xi_inp;
            pSockData = &((struct xinpcb *)pXIG)->xi_socket;

            /* Ignore sockets for other protocols */
            if (pSockData->xso_protocol != IPPROTO_UDP)
                continue;

            /* Ignore PCBs that were freed while generating the data */
            if (pINData->inp_gencnt > pOrigXIG->xig_gen)
                continue;

            /* we're only interested in IPv4 addresses */
            if (!(pINData->inp_vflag & INP_IPV4) ||
                (pINData->inp_vflag & INP_IPV6))
                continue;

            /* If all 0's, skip it */
            if (!pINData->inp_laddr.s_addr &&
                !pINData->inp_lport)
                continue;

            /* Fill in structure details */
            row.dwLocalAddr = pINData->inp_laddr.s_addr;
            row.dwLocalPort = pINData->inp_lport;
            if (!(table = append_udp_row( heap, flags, table, &count, &row ))) break;
        }

    done:
        HeapFree (GetProcessHeap (), 0, Buf);
    }
#else
    FIXME( "not implemented\n" );
    ret = ERROR_NOT_SUPPORTED;
#endif

    if (!table) return ERROR_OUTOFMEMORY;
    if (!ret)
    {
        if (bOrder && table->dwNumEntries)
            qsort( table->table, table->dwNumEntries, sizeof(row), compare_udp_rows );
        *ppUdpTable = table;
    }
    else HeapFree( heap, flags, table );
    TRACE( "returning ret %u table %p\n", ret, table );
    return ret;
}

static DWORD get_tcp_table_sizes( TCP_TABLE_CLASS class, DWORD row_count, DWORD *row_size )
{
    DWORD table_size;

    switch (class)
    {
    case TCP_TABLE_BASIC_LISTENER:
    case TCP_TABLE_BASIC_CONNECTIONS:
    case TCP_TABLE_BASIC_ALL:
    {
        table_size = FIELD_OFFSET(MIB_TCPTABLE, table[row_count]);
        if (row_size) *row_size = sizeof(MIB_TCPROW);
        break;
    }
    case TCP_TABLE_OWNER_PID_LISTENER:
    case TCP_TABLE_OWNER_PID_CONNECTIONS:
    case TCP_TABLE_OWNER_PID_ALL:
    {
        table_size = FIELD_OFFSET(MIB_TCPTABLE_OWNER_PID, table[row_count]);
        if (row_size) *row_size = sizeof(MIB_TCPROW_OWNER_PID);
        break;
    }
    default:
        ERR("unhandled class %u\n", class);
        return 0;
    }
    return table_size;
}

static MIB_TCPTABLE *append_tcp_row( TCP_TABLE_CLASS class, HANDLE heap, DWORD flags,
                                     MIB_TCPTABLE *table, DWORD *count,
                                     const MIB_TCPROW_OWNER_PID *row, DWORD row_size )
{
    if (table->dwNumEntries >= *count)
    {
        MIB_TCPTABLE *new_table;
        DWORD new_count = table->dwNumEntries * 2, new_table_size;

        new_table_size = get_tcp_table_sizes( class, new_count, NULL );
        if (!(new_table = HeapReAlloc( heap, flags, table, new_table_size )))
        {
            HeapFree( heap, 0, table );
            return NULL;
        }
        *count = new_count;
        table = new_table;
    }
    memcpy( (char *)table->table + (table->dwNumEntries * row_size), row, row_size );
    table->dwNumEntries++;
    return table;
}


/* Why not a lookup table? Because the TCPS_* constants are different
   on different platforms */
static inline MIB_TCP_STATE TCPStateToMIBState (int state)
{
   switch (state)
   {
      case TCPS_ESTABLISHED: return MIB_TCP_STATE_ESTAB;
      case TCPS_SYN_SENT: return MIB_TCP_STATE_SYN_SENT;
      case TCPS_SYN_RECEIVED: return MIB_TCP_STATE_SYN_RCVD;
      case TCPS_FIN_WAIT_1: return MIB_TCP_STATE_FIN_WAIT1;
      case TCPS_FIN_WAIT_2: return MIB_TCP_STATE_FIN_WAIT2;
      case TCPS_TIME_WAIT: return MIB_TCP_STATE_TIME_WAIT;
      case TCPS_CLOSE_WAIT: return MIB_TCP_STATE_CLOSE_WAIT;
      case TCPS_LAST_ACK: return MIB_TCP_STATE_LAST_ACK;
      case TCPS_LISTEN: return MIB_TCP_STATE_LISTEN;
      case TCPS_CLOSING: return MIB_TCP_STATE_CLOSING;
      default:
      case TCPS_CLOSED: return MIB_TCP_STATE_CLOSED;
   }
}

static int compare_tcp_rows(const void *a, const void *b)
{
    const MIB_TCPROW *rowA = a;
    const MIB_TCPROW *rowB = b;
    int ret;

    if ((ret = ntohl (rowA->dwLocalAddr) - ntohl (rowB->dwLocalAddr)) != 0) return ret;
    if ((ret = ntohs ((unsigned short)rowA->dwLocalPort) -
               ntohs ((unsigned short)rowB->dwLocalPort)) != 0) return ret;
    if ((ret = ntohl (rowA->dwRemoteAddr) - ntohl (rowB->dwRemoteAddr)) != 0) return ret;
    return ntohs ((unsigned short)rowA->dwRemotePort) - ntohs ((unsigned short)rowB->dwRemotePort);
}

struct pid_map
{
    unsigned int pid;
    unsigned int unix_pid;
};

static struct pid_map *get_pid_map( unsigned int *num_entries )
{
    HANDLE snapshot = NULL;
    struct pid_map *map;
    unsigned int i = 0, count = 16, size = count * sizeof(*map);
    NTSTATUS ret;

    if (!(map = HeapAlloc( GetProcessHeap(), 0, size ))) return NULL;

    SERVER_START_REQ( create_snapshot )
    {
        req->flags      = SNAP_PROCESS;
        req->attributes = 0;
        if (!(ret = wine_server_call( req )))
            snapshot = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    *num_entries = 0;
    while (ret == STATUS_SUCCESS)
    {
        SERVER_START_REQ( next_process )
        {
            req->handle = wine_server_obj_handle( snapshot );
            req->reset = (i == 0);
            if (!(ret = wine_server_call( req )))
            {
                if (i >= count)
                {
                    struct pid_map *new_map;
                    count *= 2;
                    size = count * sizeof(*new_map);

                    if (!(new_map = HeapReAlloc( GetProcessHeap(), 0, map, size )))
                    {
                        HeapFree( GetProcessHeap(), 0, map );
                        map = NULL;
                        goto done;
                    }
                    map = new_map;
                }
                map[i].pid = reply->pid;
                map[i].unix_pid = reply->unix_pid;
                (*num_entries)++;
                i++;
            }
        }
        SERVER_END_REQ;
    }

done:
    NtClose( snapshot );
    return map;
}

static unsigned int find_owning_pid( struct pid_map *map, unsigned int num_entries, int inode )
{
#ifdef __linux__
    unsigned int i, len_socket;
    char socket[32];

    sprintf( socket, "socket:[%d]", inode );
    len_socket = strlen( socket );
    for (i = 0; i < num_entries; i++)
    {
        char dir[32];
        struct dirent *dirent;
        DIR *dirfd;

        sprintf( dir, "/proc/%u/fd", map[i].unix_pid );
        if ((dirfd = opendir( dir )))
        {
            while ((dirent = readdir( dirfd )))
            {
                char link[sizeof(dirent->d_name) + 32], name[32];
                int len;

                sprintf( link, "/proc/%u/fd/%s", map[i].unix_pid, dirent->d_name );
                if ((len = readlink( link, name, 32 )) > 0) name[len] = 0;
                if (len == len_socket && !strcmp( socket, name ))
                {
                    closedir( dirfd );
                    return map[i].pid;
                }
            }
            closedir( dirfd );
        }
    }
    return 0;
#else
    FIXME( "not implemented\n" );
    return 0;
#endif
}

DWORD build_tcp_table( TCP_TABLE_CLASS class, void **tablep, BOOL order, HANDLE heap, DWORD flags,
                       DWORD *size )
{
    MIB_TCPTABLE *table;
    MIB_TCPROW_OWNER_PID row;
    DWORD ret = NO_ERROR, count = 16, table_size, row_size;

    if (!(table_size = get_tcp_table_sizes( class, count, &row_size )))
        return ERROR_INVALID_PARAMETER;

    if (!(table = HeapAlloc( heap, flags, table_size )))
        return ERROR_OUTOFMEMORY;

    table->dwNumEntries = 0;

#ifdef __linux__
    {
        FILE *fp;

        if ((fp = fopen("/proc/net/tcp", "r")))
        {
            char buf[512], *ptr;
            struct pid_map *map = NULL;
            unsigned int dummy, num_entries = 0;
            int inode;

            if (class == TCP_TABLE_OWNER_PID_ALL) map = get_pid_map( &num_entries );

            /* skip header line */
            ptr = fgets(buf, sizeof(buf), fp);
            while ((ptr = fgets(buf, sizeof(buf), fp)))
            {
                if (sscanf( ptr, "%x: %x:%x %x:%x %x %*s %*s %*s %*s %*s %d", &dummy,
                            &row.dwLocalAddr, &row.dwLocalPort, &row.dwRemoteAddr,
                            &row.dwRemotePort, &row.dwState, &inode ) != 7)
                    continue;
                row.dwLocalPort = htons( row.dwLocalPort );
                row.dwRemotePort = htons( row.dwRemotePort );
                row.dwState = TCPStateToMIBState( row.dwState );
                if (class == TCP_TABLE_OWNER_PID_ALL)
                    row.dwOwningPid = find_owning_pid( map, num_entries, inode );

                if (!(table = append_tcp_row( class, heap, flags, table, &count, &row, row_size )))
                    break;
            }
            HeapFree( GetProcessHeap(), 0, map );
            fclose( fp );
        }
        else ret = ERROR_NOT_SUPPORTED;
    }
#elif defined(HAVE_SYS_TIHDR_H) && defined(T_OPTMGMT_ACK)
    {
        void *data;
        int fd, len;
        mib2_tcpConnEntry_t *entry;

        if ((fd = open_streams_mib( "tcp" )) != -1)
        {
            if ((data = read_mib_entry( fd, MIB2_TCP, MIB2_TCP_CONN, &len )))
            {
                for (entry = data; (char *)(entry + 1) <= (char *)data + len; entry++)
                {
                    row.dwLocalAddr = entry->tcpConnLocalAddress;
                    row.dwLocalPort = htons( entry->tcpConnLocalPort );
                    row.dwRemoteAddr = entry->tcpConnRemAddress;
                    row.dwRemotePort = htons( entry->tcpConnRemPort );
                    row.dwState = entry->tcpConnState;
                    if (!(table = append_tcp_row( class, heap, flags, table, &count, &row, row_size )))
                        break;
                }
                HeapFree( GetProcessHeap(), 0, data );
            }
            close( fd );
        }
        else ret = ERROR_NOT_SUPPORTED;
    }
#elif defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_STRUCT_XINPGEN)
    {
        size_t Len = 0;
        char *Buf = NULL;
        struct xinpgen *pXIG, *pOrigXIG;

        if (sysctlbyname ("net.inet.tcp.pcblist", NULL, &Len, NULL, 0) < 0)
        {
            ERR ("Failure to read net.inet.tcp.pcblist via sysctlbyname!\n");
            ret = ERROR_NOT_SUPPORTED;
            goto done;
        }

        Buf = HeapAlloc (GetProcessHeap (), 0, Len);
        if (!Buf)
        {
            ret = ERROR_OUTOFMEMORY;
            goto done;
        }

        if (sysctlbyname ("net.inet.tcp.pcblist", Buf, &Len, NULL, 0) < 0)
        {
            ERR ("Failure to read net.inet.tcp.pcblist via sysctlbyname!\n");
            ret = ERROR_NOT_SUPPORTED;
            goto done;
        }

        /* Might be nothing here; first entry is just a header it seems */
        if (Len <= sizeof (struct xinpgen)) goto done;

        pOrigXIG = (struct xinpgen *)Buf;
        pXIG = pOrigXIG;

        for (pXIG = (struct xinpgen *)((char *)pXIG + pXIG->xig_len);
             pXIG->xig_len > sizeof (struct xinpgen);
             pXIG = (struct xinpgen *)((char *)pXIG + pXIG->xig_len))
        {
            struct tcpcb *pTCPData = NULL;
            struct inpcb *pINData;
            struct xsocket *pSockData;

            pTCPData = &((struct xtcpcb *)pXIG)->xt_tp;
            pINData = &((struct xtcpcb *)pXIG)->xt_inp;
            pSockData = &((struct xtcpcb *)pXIG)->xt_socket;

            /* Ignore sockets for other protocols */
            if (pSockData->xso_protocol != IPPROTO_TCP)
                continue;

            /* Ignore PCBs that were freed while generating the data */
            if (pINData->inp_gencnt > pOrigXIG->xig_gen)
                continue;

            /* we're only interested in IPv4 addresses */
            if (!(pINData->inp_vflag & INP_IPV4) ||
                (pINData->inp_vflag & INP_IPV6))
                continue;

            /* If all 0's, skip it */
            if (!pINData->inp_laddr.s_addr &&
                !pINData->inp_lport &&
                !pINData->inp_faddr.s_addr &&
                !pINData->inp_fport)
                continue;

            /* Fill in structure details */
            row.dwLocalAddr = pINData->inp_laddr.s_addr;
            row.dwLocalPort = pINData->inp_lport;
            row.dwRemoteAddr = pINData->inp_faddr.s_addr;
            row.dwRemotePort = pINData->inp_fport;
            row.dwState = TCPStateToMIBState (pTCPData->t_state);
            if (!(table = append_tcp_row( class, heap, flags, table, &count, &row, row_size )))
                break;
        }

    done:
        HeapFree (GetProcessHeap (), 0, Buf);
    }
#else
    FIXME( "not implemented\n" );
    ret = ERROR_NOT_SUPPORTED;
#endif

    if (!table) return ERROR_OUTOFMEMORY;
    if (!ret)
    {
        if (order && table->dwNumEntries)
            qsort( table->table, table->dwNumEntries, row_size, compare_tcp_rows );
        *tablep = table;
    }
    else HeapFree( heap, flags, table );
    if (size) *size = get_tcp_table_sizes( class, count, NULL );
    TRACE( "returning ret %u table %p\n", ret, table );
    return ret;
}

/******************************************************************
 *    AllocateAndGetTcpTableFromStack (IPHLPAPI.@)
 *
 * Get the TCP connection table.
 * Like GetTcpTable(), but allocate the returned table from heap.
 *
 * PARAMS
 *  ppTcpTable [Out] pointer into which the MIB_TCPTABLE is
 *                   allocated and returned.
 *  bOrder     [In]  whether to sort the table
 *  heap       [In]  heap from which the table is allocated
 *  flags      [In]  flags to HeapAlloc
 *
 * RETURNS
 *  ERROR_INVALID_PARAMETER if ppTcpTable is NULL, whatever GetTcpTable()
 *  returns otherwise.
 */
DWORD WINAPI AllocateAndGetTcpTableFromStack( PMIB_TCPTABLE *ppTcpTable, BOOL bOrder,
                                              HANDLE heap, DWORD flags )
{
    TRACE("table %p, bOrder %d, heap %p, flags 0x%08x\n", ppTcpTable, bOrder, heap, flags);

    if (!ppTcpTable) return ERROR_INVALID_PARAMETER;
    return build_tcp_table( TCP_TABLE_BASIC_ALL, (void **)ppTcpTable, bOrder, heap, flags, NULL );
}
