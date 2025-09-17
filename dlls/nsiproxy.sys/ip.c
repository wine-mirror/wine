/*
 * nsiproxy.sys ipv4 and ipv6 modules
 *
 * Copyright 2003, 2006, 2011 Juan Lang
 * Copyright 2021 Huw Davies
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
#if 0
#pragma makedep unix
#endif

#include "config.h"
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>

#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif

#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif

#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif

#ifdef HAVE_NETINET_IP_ICMP_H
#include <netinet/ip_icmp.h>
#endif

#ifdef HAVE_NETINET_IP_VAR_H
#include <netinet/ip_var.h>
#endif

#ifdef HAVE_NETINET6_IP6_VAR_H
#include <netinet6/ip6_var.h>
#endif

#ifdef HAVE_NET_IF_H
# include <net/if.h>
#endif

#ifdef __APPLE__
/* For reasons unknown, Mac OS doesn't export <netinet6/ip6_var.h> to user-
 * space. We'll have to define the needed struct ourselves.
 */
struct ip6stat {
    u_quad_t ip6s_total;
    u_quad_t ip6s_tooshort;
    u_quad_t ip6s_toosmall;
    u_quad_t ip6s_fragments;
    u_quad_t ip6s_fragdropped;
    u_quad_t ip6s_fragtimeout;
    u_quad_t ip6s_fragoverflow;
    u_quad_t ip6s_forward;
    u_quad_t ip6s_cantforward;
    u_quad_t ip6s_redirectsent;
    u_quad_t ip6s_delivered;
    u_quad_t ip6s_localout;
    u_quad_t ip6s_odropped;
    u_quad_t ip6s_reassembled;
    u_quad_t ip6s_atmfrag_rcvd;
    u_quad_t ip6s_fragmented;
    u_quad_t ip6s_ofragments;
    u_quad_t ip6s_cantfrag;
    u_quad_t ip6s_badoptions;
    u_quad_t ip6s_noroute;
    u_quad_t ip6s_badvers;
    u_quad_t ip6s_rawout;
    u_quad_t ip6s_badscope;
    u_quad_t ip6s_notmember;
    u_quad_t ip6s_nxthist[256];
    u_quad_t ip6s_m1;
    u_quad_t ip6s_m2m[32];
    u_quad_t ip6s_mext1;
    u_quad_t ip6s_mext2m;
    u_quad_t ip6s_exthdrtoolong;
    u_quad_t ip6s_nogif;
    u_quad_t ip6s_toomanyhdr;
};
#endif

#ifdef HAVE_NETINET_ICMP_VAR_H
#include <netinet/icmp_var.h>
#endif

#ifdef HAVE_NETINET_ICMP6_H
#include <netinet/icmp6.h>
#undef ICMP6_DST_UNREACH
#undef ICMP6_PACKET_TOO_BIG
#undef ICMP6_TIME_EXCEEDED
#undef ICMP6_PARAM_PROB
#undef ICMP6_ECHO_REQUEST
#undef ICMP6_ECHO_REPLY
#undef ICMP6_MEMBERSHIP_QUERY
#undef ICMP6_MEMBERSHIP_REPORT
#undef ICMP6_MEMBERSHIP_REDUCTION
#undef ND_ROUTER_SOLICIT
#undef ND_ROUTER_ADVERT
#undef ND_NEIGHBOR_SOLICIT
#undef ND_NEIGHBOR_ADVERT
#undef ND_REDIRECT
#endif

#ifdef HAVE_NETINET_IF_ETHER_H
#include <netinet/if_ether.h>
#endif

#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif

#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif

#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#define USE_WS_PREFIX
#include "winsock2.h"
#include "ws2ipdef.h"
#include "nldef.h"
#include "ifdef.h"
#include "ipmib.h"
#include "netiodef.h"
#include "wine/nsi.h"
#include "wine/debug.h"

#include "unix_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(nsi);

static inline UINT nsi_popcount( UINT m )
{
#ifdef HAVE___BUILTIN_POPCOUNT
    return __builtin_popcount( m );
#else
    m -= m >> 1 & 0x55555555;
    m = (m & 0x33333333) + (m >> 2 & 0x33333333);
    return ((m + (m >> 4)) & 0x0f0f0f0f) * 0x01010101 >> 24;
#endif
}

static UINT mask_v4_to_prefix( struct in_addr *addr )
{
    return nsi_popcount( addr->s_addr );
}

static UINT mask_v6_to_prefix( struct in6_addr *addr )
{
    UINT ret;

    ret = nsi_popcount( *(UINT *)addr->s6_addr );
    ret += nsi_popcount( *(UINT *)(addr->s6_addr + 4) );
    ret += nsi_popcount( *(UINT *)(addr->s6_addr + 8) );
    ret += nsi_popcount( *(UINT *)(addr->s6_addr + 12) );
    return ret;
}

static ULONG64 get_boot_time( void )
{
    SYSTEM_TIMEOFDAY_INFORMATION ti;

    NtQuerySystemInformation( SystemTimeOfDayInformation, &ti, sizeof(ti), NULL );
    return ti.BootTime.QuadPart;
}

#if __linux__
static NTSTATUS read_sysctl_int( const char *file, int *val )
{
    FILE *fp;
    char buf[128], *endptr = buf;

    fp = fopen( file, "r" );
    if (!fp) return STATUS_NOT_SUPPORTED;

    if (fgets( buf, sizeof(buf), fp ))
        *val = strtol( buf, &endptr, 10 );

    fclose( fp );
    return (endptr == buf) ? STATUS_NOT_SUPPORTED : STATUS_SUCCESS;
}
#endif

static NTSTATUS ip_cmpt_get_all_parameters( UINT fam, const UINT *key, UINT key_size,
                                            struct nsi_ip_cmpt_rw *rw_data, UINT rw_size,
                                            struct nsi_ip_cmpt_dynamic *dynamic_data, UINT dynamic_size,
                                            void *static_data, UINT static_size )
{
    const NPI_MODULEID *ip_mod = (fam == AF_INET) ? &NPI_MS_IPV4_MODULEID : &NPI_MS_IPV6_MODULEID;
    struct nsi_ip_cmpt_rw rw;
    struct nsi_ip_cmpt_dynamic dyn;
    UINT count;

    memset( &rw, 0, sizeof(rw) );
    memset( &dyn, 0, sizeof(dyn) );

    if (*key != 1) return STATUS_NOT_SUPPORTED;

#if __linux__
    {
        const char *fwd = (fam == AF_INET) ? "/proc/sys/net/ipv4/conf/default/forwarding" :
            "/proc/sys/net/ipv6/conf/default/forwarding";
        const char *ttl = (fam == AF_INET) ? "/proc/sys/net/ipv4/ip_default_ttl" :
            "/proc/sys/net/ipv6/conf/default/hop_limit";
        int value;

        if (!read_sysctl_int( fwd, &value )) rw.not_forwarding = !value;
        if (!read_sysctl_int( ttl, &value )) rw.default_ttl = value;
    }
#elif defined(HAVE_SYS_SYSCTL_H)
    {
        int fwd_4[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_FORWARDING };
        int fwd_6[] = { CTL_NET, PF_INET6, IPPROTO_IPV6, IPV6CTL_FORWARDING };
        int ttl_4[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_DEFTTL };
        int ttl_6[] = { CTL_NET, PF_INET6, IPPROTO_IPV6, IPV6CTL_DEFHLIM };
        int value;
        size_t needed;

        needed = sizeof(value);
        if (!sysctl( (fam == AF_INET) ? fwd_4 : fwd_6, ARRAY_SIZE(fwd_4), &value, &needed, NULL, 0 ))
            rw.not_forwarding = value;

        needed = sizeof(value);
        if (!sysctl( (fam == AF_INET) ? ttl_4 : ttl_6, ARRAY_SIZE(ttl_4), &value, &needed, NULL, 0 ))
            rw.default_ttl = value;
    }
#else
    FIXME( "forwarding and default ttl not implemented\n" );
#endif

    count = 0;
    if (!nsi_enumerate_all( 1, 0, &NPI_MS_NDIS_MODULEID, NSI_NDIS_IFINFO_TABLE, NULL, 0, NULL, 0,
                            NULL, 0, NULL, 0, &count ))
        dyn.num_ifs = count;

    count = 0;
    if (!nsi_enumerate_all( 1, 0, ip_mod, NSI_IP_FORWARD_TABLE, NULL, 0, NULL, 0,
                            NULL, 0, NULL, 0, &count ))
        dyn.num_routes = count;

    count = 0;
    if (!nsi_enumerate_all( 1, 0, ip_mod, NSI_IP_UNICAST_TABLE, NULL, 0, NULL, 0,
                            NULL, 0, NULL, 0, &count ))
        dyn.num_addrs = count;

    if (rw_data) *rw_data = rw;
    if (dynamic_data) *dynamic_data = dyn;

    return STATUS_SUCCESS;
}

static NTSTATUS ipv4_cmpt_get_all_parameters( const void *key, UINT key_size, void *rw_data, UINT rw_size,
                                              void *dynamic_data, UINT dynamic_size, void *static_data, UINT static_size )
{
    TRACE( "%p %d %p %d %p %d %p %d\n", key, key_size, rw_data, rw_size, dynamic_data, dynamic_size,
           static_data, static_size );
    return ip_cmpt_get_all_parameters( AF_INET, key, key_size, rw_data, rw_size,
                                       dynamic_data, dynamic_size, static_data, static_size );
}

static NTSTATUS ipv6_cmpt_get_all_parameters( const void *key, UINT key_size, void *rw_data, UINT rw_size,
                                              void *dynamic_data, UINT dynamic_size, void *static_data, UINT static_size )
{
    TRACE( "%p %d %p %d %p %d %p %d\n", key, key_size, rw_data, rw_size, dynamic_data, dynamic_size,
           static_data, static_size );
    return ip_cmpt_get_all_parameters( AF_INET6, key, key_size, rw_data, rw_size,
                                       dynamic_data, dynamic_size, static_data, static_size );
}

static NTSTATUS ipv4_icmpstats_get_all_parameters( const void *key, UINT key_size, void *rw_data, UINT rw_size,
                                                   void *dynamic_data, UINT dynamic_size, void *static_data, UINT static_size )
{
    struct nsi_ip_icmpstats_dynamic dyn;

    TRACE( "%p %d %p %d %p %d %p %d\n", key, key_size, rw_data, rw_size, dynamic_data, dynamic_size,
           static_data, static_size );

    memset( &dyn, 0, sizeof(dyn) );

#ifdef __linux__
    {
        NTSTATUS status = STATUS_NOT_SUPPORTED;
        static const char hdr[] = "Icmp:";
        char buf[512], *ptr;
        FILE *fp;

        if (!(fp = fopen( "/proc/net/snmp", "r" ))) return STATUS_NOT_SUPPORTED;

        while ((ptr = fgets( buf, sizeof(buf), fp )))
        {
            if (ascii_strncasecmp( buf, hdr, sizeof(hdr) - 1 )) continue;
            /* last line was a header, get another */
            if (!(ptr = fgets( buf, sizeof(buf), fp ))) break;
            if (!ascii_strncasecmp( buf, hdr, sizeof(hdr) - 1 ))
            {
                ptr += sizeof(hdr);
                sscanf( ptr, "%u %u %*u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u",
                        &dyn.in_msgs,
                        &dyn.in_errors,
                        &dyn.in_type_counts[ICMP4_DST_UNREACH],
                        &dyn.in_type_counts[ICMP4_TIME_EXCEEDED],
                        &dyn.in_type_counts[ICMP4_PARAM_PROB],
                        &dyn.in_type_counts[ICMP4_SOURCE_QUENCH],
                        &dyn.in_type_counts[ICMP4_REDIRECT],
                        &dyn.in_type_counts[ICMP4_ECHO_REQUEST],
                        &dyn.in_type_counts[ICMP4_ECHO_REPLY],
                        &dyn.in_type_counts[ICMP4_TIMESTAMP_REQUEST],
                        &dyn.in_type_counts[ICMP4_TIMESTAMP_REPLY],
                        &dyn.in_type_counts[ICMP4_MASK_REQUEST],
                        &dyn.in_type_counts[ICMP4_MASK_REPLY],
                        &dyn.out_msgs,
                        &dyn.out_errors,
                        &dyn.out_type_counts[ICMP4_DST_UNREACH],
                        &dyn.out_type_counts[ICMP4_TIME_EXCEEDED],
                        &dyn.out_type_counts[ICMP4_PARAM_PROB],
                        &dyn.out_type_counts[ICMP4_SOURCE_QUENCH],
                        &dyn.out_type_counts[ICMP4_REDIRECT],
                        &dyn.out_type_counts[ICMP4_ECHO_REQUEST],
                        &dyn.out_type_counts[ICMP4_ECHO_REPLY],
                        &dyn.out_type_counts[ICMP4_TIMESTAMP_REQUEST],
                        &dyn.out_type_counts[ICMP4_TIMESTAMP_REPLY],
                        &dyn.out_type_counts[ICMP4_MASK_REQUEST],
                        &dyn.out_type_counts[ICMP4_MASK_REPLY] );
                status = STATUS_SUCCESS;
                if (dynamic_data) *(struct nsi_ip_icmpstats_dynamic *)dynamic_data = dyn;
                break;
            }
        }
        fclose( fp );
        return status;
    }
#elif defined(HAVE_SYS_SYSCTL_H) && defined(ICMPCTL_STATS) && defined(HAVE_STRUCT_ICMPSTAT_ICPS_ERROR)
    {
        int mib[] = { CTL_NET, PF_INET, IPPROTO_ICMP, ICMPCTL_STATS };
        struct icmpstat icmp_stat;
        size_t needed = sizeof(icmp_stat);
        int i;

        if (sysctl( mib, ARRAY_SIZE(mib), &icmp_stat, &needed, NULL, 0 ) == -1) return STATUS_NOT_SUPPORTED;

        dyn.in_msgs = icmp_stat.icps_badcode + icmp_stat.icps_checksum + icmp_stat.icps_tooshort + icmp_stat.icps_badlen;
        for (i = 0; i <= ICMP_MAXTYPE; i++)
            dyn.in_msgs += icmp_stat.icps_inhist[i];

        dyn.in_errors = icmp_stat.icps_badcode + icmp_stat.icps_tooshort + icmp_stat.icps_checksum + icmp_stat.icps_badlen;

        dyn.in_type_counts[ICMP4_DST_UNREACH] = icmp_stat.icps_inhist[ICMP_UNREACH];
        dyn.in_type_counts[ICMP4_TIME_EXCEEDED] = icmp_stat.icps_inhist[ICMP_TIMXCEED];
        dyn.in_type_counts[ICMP4_PARAM_PROB] = icmp_stat.icps_inhist[ICMP_PARAMPROB];
        dyn.in_type_counts[ICMP4_SOURCE_QUENCH] = icmp_stat.icps_inhist[ICMP_SOURCEQUENCH];
        dyn.in_type_counts[ICMP4_REDIRECT] = icmp_stat.icps_inhist[ICMP_REDIRECT];
        dyn.in_type_counts[ICMP4_ECHO_REQUEST] = icmp_stat.icps_inhist[ICMP_ECHO];
        dyn.in_type_counts[ICMP4_ECHO_REPLY] = icmp_stat.icps_inhist[ICMP_ECHOREPLY];
        dyn.in_type_counts[ICMP4_TIMESTAMP_REQUEST] = icmp_stat.icps_inhist[ICMP_TSTAMP];
        dyn.in_type_counts[ICMP4_TIMESTAMP_REPLY] = icmp_stat.icps_inhist[ICMP_TSTAMPREPLY];
        dyn.in_type_counts[ICMP4_MASK_REQUEST] = icmp_stat.icps_inhist[ICMP_MASKREQ];
        dyn.in_type_counts[ICMP4_MASK_REPLY] = icmp_stat.icps_inhist[ICMP_MASKREPLY];

        dyn.out_msgs = icmp_stat.icps_oldshort + icmp_stat.icps_oldicmp;
        for (i = 0; i <= ICMP_MAXTYPE; i++)
            dyn.out_msgs += icmp_stat.icps_outhist[i];

        dyn.out_errors = icmp_stat.icps_oldshort + icmp_stat.icps_oldicmp;

        dyn.out_type_counts[ICMP4_DST_UNREACH] = icmp_stat.icps_outhist[ICMP_UNREACH];
        dyn.out_type_counts[ICMP4_TIME_EXCEEDED] = icmp_stat.icps_outhist[ICMP_TIMXCEED];
        dyn.out_type_counts[ICMP4_PARAM_PROB] = icmp_stat.icps_outhist[ICMP_PARAMPROB];
        dyn.out_type_counts[ICMP4_SOURCE_QUENCH] = icmp_stat.icps_outhist[ICMP_SOURCEQUENCH];
        dyn.out_type_counts[ICMP4_REDIRECT] = icmp_stat.icps_outhist[ICMP_REDIRECT];
        dyn.out_type_counts[ICMP4_ECHO_REQUEST] = icmp_stat.icps_outhist[ICMP_ECHO];
        dyn.out_type_counts[ICMP4_ECHO_REPLY] = icmp_stat.icps_outhist[ICMP_ECHOREPLY];
        dyn.out_type_counts[ICMP4_TIMESTAMP_REQUEST] = icmp_stat.icps_outhist[ICMP_TSTAMP];
        dyn.out_type_counts[ICMP4_TIMESTAMP_REPLY] = icmp_stat.icps_outhist[ICMP_TSTAMPREPLY];
        dyn.out_type_counts[ICMP4_MASK_REQUEST] = icmp_stat.icps_outhist[ICMP_MASKREQ];
        dyn.out_type_counts[ICMP4_MASK_REPLY] = icmp_stat.icps_outhist[ICMP_MASKREPLY];
        if (dynamic_data) *(struct nsi_ip_icmpstats_dynamic *)dynamic_data = dyn;
        return STATUS_SUCCESS;
    }
#else
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
#endif
}

static NTSTATUS ipv6_icmpstats_get_all_parameters( const void *key, UINT key_size, void *rw_data, UINT rw_size,
                                                   void *dynamic_data, UINT dynamic_size, void *static_data, UINT static_size )
{
    struct nsi_ip_icmpstats_dynamic dyn;

    TRACE( "%p %d %p %d %p %d %p %d\n", key, key_size, rw_data, rw_size, dynamic_data, dynamic_size,
           static_data, static_size );

    memset( &dyn, 0, sizeof(dyn) );

#ifdef __linux__
    {
        struct data
        {
            const char *name;
            UINT pos;
        };
        static const struct data in_list[] =
        {
            { "Icmp6InDestUnreachs",           ICMP6_DST_UNREACH },
            { "Icmp6InPktTooBigs",             ICMP6_PACKET_TOO_BIG },
            { "Icmp6InTimeExcds",              ICMP6_TIME_EXCEEDED },
            { "Icmp6InParmProblems",           ICMP6_PARAM_PROB },
            { "Icmp6InEchos",                  ICMP6_ECHO_REQUEST },
            { "Icmp6InEchoReplies",            ICMP6_ECHO_REPLY },
            { "Icmp6InGroupMembQueries",       ICMP6_MEMBERSHIP_QUERY },
            { "Icmp6InGroupMembResponses",     ICMP6_MEMBERSHIP_REPORT },
            { "Icmp6InGroupMembReductions",    ICMP6_MEMBERSHIP_REDUCTION },
            { "Icmp6InRouterSolicits",         ND_ROUTER_SOLICIT },
            { "Icmp6InRouterAdvertisements",   ND_ROUTER_ADVERT },
            { "Icmp6InNeighborSolicits",       ND_NEIGHBOR_SOLICIT },
            { "Icmp6InNeighborAdvertisements", ND_NEIGHBOR_ADVERT },
            { "Icmp6InRedirects",              ND_REDIRECT },
            { "Icmp6InMLDv2Reports",           ICMP6_V2_MEMBERSHIP_REPORT },
        };
        static const struct data out_list[] =
        {
            { "Icmp6OutDestUnreachs",           ICMP6_DST_UNREACH },
            { "Icmp6OutPktTooBigs",             ICMP6_PACKET_TOO_BIG },
            { "Icmp6OutTimeExcds",              ICMP6_TIME_EXCEEDED },
            { "Icmp6OutParmProblems",           ICMP6_PARAM_PROB },
            { "Icmp6OutEchos",                  ICMP6_ECHO_REQUEST },
            { "Icmp6OutEchoReplies",            ICMP6_ECHO_REPLY },
            { "Icmp6OutGroupMembQueries",       ICMP6_MEMBERSHIP_QUERY },
            { "Icmp6OutGroupMembResponses",     ICMP6_MEMBERSHIP_REPORT },
            { "Icmp6OutGroupMembReductions",    ICMP6_MEMBERSHIP_REDUCTION },
            { "Icmp6OutRouterSolicits",         ND_ROUTER_SOLICIT },
            { "Icmp6OutRouterAdvertisements",   ND_ROUTER_ADVERT },
            { "Icmp6OutNeighborSolicits",       ND_NEIGHBOR_SOLICIT },
            { "Icmp6OutNeighborAdvertisements", ND_NEIGHBOR_ADVERT },
            { "Icmp6OutRedirects",              ND_REDIRECT },
            { "Icmp6OutMLDv2Reports",           ICMP6_V2_MEMBERSHIP_REPORT },
        };
        char buf[512], *ptr, *value;
        UINT res, i;
        FILE *fp;

        if (!(fp = fopen( "/proc/net/snmp6", "r" ))) return STATUS_NOT_SUPPORTED;

        while ((ptr = fgets( buf, sizeof(buf), fp )))
        {
            if (!(value = strchr( buf, ' ' ))) continue;

            /* terminate the valuename */
            ptr = value - 1;
            *(ptr + 1) = '\0';

            /* and strip leading spaces from value */
            value += 1;
            while (*value == ' ') value++;
            if ((ptr = strchr( value, '\n' ))) *ptr='\0';

            if (!ascii_strcasecmp( buf, "Icmp6InMsgs" ))
            {
                if (sscanf( value, "%d", &res )) dyn.in_msgs = res;
                continue;
            }

            if (!ascii_strcasecmp( buf, "Icmp6InErrors" ))
            {
                if (sscanf( value, "%d", &res )) dyn.in_errors = res;
                continue;
            }

            for (i = 0; i < ARRAY_SIZE(in_list); i++)
            {
                if (!ascii_strcasecmp( buf, in_list[i].name ))
                {
                    if (sscanf( value, "%d", &res ))
                        dyn.in_type_counts[in_list[i].pos] = res;
                    break;
                }
            }

            if (!ascii_strcasecmp( buf, "Icmp6OutMsgs" ))
            {
                if (sscanf( value, "%d", &res )) dyn.out_msgs = res;
                continue;
            }

            if (!ascii_strcasecmp( buf, "Icmp6OutErrors" ))
            {
                if (sscanf( value, "%d", &res )) dyn.out_errors = res;
                continue;
            }

            for (i = 0; i < ARRAY_SIZE(out_list); i++)
            {
                if (!ascii_strcasecmp( buf, out_list[i].name ))
                {
                    if (sscanf( value, "%d", &res ))
                        dyn.out_type_counts[out_list[i].pos] = res;
                    break;
                }
            }
        }
        fclose( fp );
        if (dynamic_data) *(struct nsi_ip_icmpstats_dynamic *)dynamic_data = dyn;
        return STATUS_SUCCESS;
    }
#elif defined(HAVE_SYS_SYSCTL_H) && defined(ICMPV6CTL_STATS) && defined(HAVE_STRUCT_ICMP6STAT_ICP6S_ERROR)
    {
        int mib[] = { CTL_NET, PF_INET6, IPPROTO_ICMPV6, ICMPV6CTL_STATS };
        struct icmp6stat icmp_stat;
        size_t needed = sizeof(icmp_stat);
        int i;

        if (sysctl( mib, ARRAY_SIZE(mib), &icmp_stat, &needed, NULL, 0 ) == -1) return STATUS_NOT_SUPPORTED;

        dyn.in_msgs = icmp_stat.icp6s_badcode + icmp_stat.icp6s_checksum + icmp_stat.icp6s_tooshort +
            icmp_stat.icp6s_badlen + icmp_stat.icp6s_nd_toomanyopt;
        for (i = 0; i <= ICMP6_MAXTYPE; i++)
            dyn.in_msgs += icmp_stat.icp6s_inhist[i];

        dyn.in_errors = icmp_stat.icp6s_badcode + icmp_stat.icp6s_checksum + icmp_stat.icp6s_tooshort +
            icmp_stat.icp6s_badlen + icmp_stat.icp6s_nd_toomanyopt;

        dyn.in_type_counts[ICMP6_DST_UNREACH] = icmp_stat.icp6s_inhist[ICMP6_DST_UNREACH];
        dyn.in_type_counts[ICMP6_PACKET_TOO_BIG] = icmp_stat.icp6s_inhist[ICMP6_PACKET_TOO_BIG];
        dyn.in_type_counts[ICMP6_TIME_EXCEEDED] = icmp_stat.icp6s_inhist[ICMP6_TIME_EXCEEDED];
        dyn.in_type_counts[ICMP6_PARAM_PROB] = icmp_stat.icp6s_inhist[ICMP6_PARAM_PROB];
        dyn.in_type_counts[ICMP6_ECHO_REQUEST] = icmp_stat.icp6s_inhist[ICMP6_ECHO_REQUEST];
        dyn.in_type_counts[ICMP6_ECHO_REPLY] = icmp_stat.icp6s_inhist[ICMP6_ECHO_REPLY];
        dyn.in_type_counts[ICMP6_MEMBERSHIP_QUERY] = icmp_stat.icp6s_inhist[ICMP6_MEMBERSHIP_QUERY];
        dyn.in_type_counts[ICMP6_MEMBERSHIP_REPORT] = icmp_stat.icp6s_inhist[ICMP6_MEMBERSHIP_REPORT];
        dyn.in_type_counts[ICMP6_MEMBERSHIP_REDUCTION] = icmp_stat.icp6s_inhist[ICMP6_MEMBERSHIP_REDUCTION];
        dyn.in_type_counts[ND_ROUTER_SOLICIT] = icmp_stat.icp6s_inhist[ND_ROUTER_SOLICIT];
        dyn.in_type_counts[ND_ROUTER_ADVERT] = icmp_stat.icp6s_inhist[ND_ROUTER_ADVERT];
        dyn.in_type_counts[ND_NEIGHBOR_SOLICIT] = icmp_stat.icp6s_inhist[ND_NEIGHBOR_SOLICIT];
        dyn.in_type_counts[ND_NEIGHBOR_ADVERT] = icmp_stat.icp6s_inhist[ND_NEIGHBOR_ADVERT];
        dyn.in_type_counts[ND_REDIRECT] = icmp_stat.icp6s_inhist[ND_REDIRECT];
        dyn.in_type_counts[ICMP6_V2_MEMBERSHIP_REPORT] = icmp_stat.icp6s_inhist[MLDV2_LISTENER_REPORT];

        dyn.out_msgs = icmp_stat.icp6s_canterror + icmp_stat.icp6s_toofreq;
        for (i = 0; i <= ICMP6_MAXTYPE; i++)
            dyn.out_msgs += icmp_stat.icp6s_outhist[i];

        dyn.out_errors = icmp_stat.icp6s_canterror + icmp_stat.icp6s_toofreq;

        dyn.out_type_counts[ICMP6_DST_UNREACH] = icmp_stat.icp6s_outhist[ICMP6_DST_UNREACH];
        dyn.out_type_counts[ICMP6_PACKET_TOO_BIG] = icmp_stat.icp6s_outhist[ICMP6_PACKET_TOO_BIG];
        dyn.out_type_counts[ICMP6_TIME_EXCEEDED] = icmp_stat.icp6s_outhist[ICMP6_TIME_EXCEEDED];
        dyn.out_type_counts[ICMP6_PARAM_PROB] = icmp_stat.icp6s_outhist[ICMP6_PARAM_PROB];
        dyn.out_type_counts[ICMP6_ECHO_REQUEST] = icmp_stat.icp6s_outhist[ICMP6_ECHO_REQUEST];
        dyn.out_type_counts[ICMP6_ECHO_REPLY] = icmp_stat.icp6s_outhist[ICMP6_ECHO_REPLY];
        dyn.out_type_counts[ICMP6_MEMBERSHIP_QUERY] = icmp_stat.icp6s_outhist[ICMP6_MEMBERSHIP_QUERY];
        dyn.out_type_counts[ICMP6_MEMBERSHIP_REPORT] = icmp_stat.icp6s_outhist[ICMP6_MEMBERSHIP_REPORT];
        dyn.out_type_counts[ICMP6_MEMBERSHIP_REDUCTION] = icmp_stat.icp6s_outhist[ICMP6_MEMBERSHIP_REDUCTION];
        dyn.out_type_counts[ND_ROUTER_SOLICIT] = icmp_stat.icp6s_outhist[ND_ROUTER_SOLICIT];
        dyn.out_type_counts[ND_ROUTER_ADVERT] = icmp_stat.icp6s_outhist[ND_ROUTER_ADVERT];
        dyn.out_type_counts[ND_NEIGHBOR_SOLICIT] = icmp_stat.icp6s_outhist[ND_NEIGHBOR_SOLICIT];
        dyn.out_type_counts[ND_NEIGHBOR_ADVERT] = icmp_stat.icp6s_outhist[ND_NEIGHBOR_ADVERT];
        dyn.out_type_counts[ND_REDIRECT] = icmp_stat.icp6s_outhist[ND_REDIRECT];
        dyn.out_type_counts[ICMP6_V2_MEMBERSHIP_REPORT] = icmp_stat.icp6s_outhist[MLDV2_LISTENER_REPORT];
        if (dynamic_data) *(struct nsi_ip_icmpstats_dynamic *)dynamic_data = dyn;
        return STATUS_SUCCESS;
    }
#else
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
#endif
}

static NTSTATUS ipv4_ipstats_get_all_parameters( const void *key, UINT key_size, void *rw_data, UINT rw_size,
                                                 void *dynamic_data, UINT dynamic_size, void *static_data, UINT static_size )
{
    struct nsi_ip_ipstats_dynamic dyn;
    struct nsi_ip_ipstats_static stat;

    TRACE( "%p %d %p %d %p %d %p %d\n", key, key_size, rw_data, rw_size, dynamic_data, dynamic_size,
           static_data, static_size );

    memset( &dyn, 0, sizeof(dyn) );
    memset( &stat, 0, sizeof(stat) );

#ifdef __linux__
    {
        NTSTATUS status = STATUS_NOT_SUPPORTED;
        static const char hdr[] = "Ip:";
        char buf[512], *ptr;
        FILE *fp;

        if (!(fp = fopen( "/proc/net/snmp", "r" ))) return STATUS_NOT_SUPPORTED;

        while ((ptr = fgets( buf, sizeof(buf), fp )))
        {
            if (ascii_strncasecmp( buf, hdr, sizeof(hdr) - 1 )) continue;
            /* last line was a header, get another */
            if (!(ptr = fgets( buf, sizeof(buf), fp ))) break;
            if (!ascii_strncasecmp( buf, hdr, sizeof(hdr) - 1 ))
            {
                UINT in_recv, in_hdr_errs, fwd_dgrams, in_delivers, out_reqs;
                ptr += sizeof(hdr);
                sscanf( ptr, "%*u %*u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u",
                        &in_recv,
                        &in_hdr_errs,
                        &dyn.in_addr_errs,
                        &fwd_dgrams,
                        &dyn.in_unk_protos,
                        &dyn.in_discards,
                        &in_delivers,
                        &out_reqs,
                        &dyn.out_discards,
                        &dyn.out_no_routes,
                        &stat.reasm_timeout,
                        &dyn.reasm_reqds,
                        &dyn.reasm_oks,
                        &dyn.reasm_fails,
                        &dyn.frag_oks,
                        &dyn.frag_fails,
                        &dyn.frag_creates );
                /* no routingDiscards */
                dyn.in_recv = in_recv;
                dyn.in_hdr_errs = in_hdr_errs;
                dyn.fwd_dgrams = fwd_dgrams;
                dyn.in_delivers = in_delivers;
                dyn.out_reqs = out_reqs;
                if (dynamic_data) *(struct nsi_ip_ipstats_dynamic *)dynamic_data = dyn;
                if (static_data) *(struct nsi_ip_ipstats_static *)static_data = stat;
                status = STATUS_SUCCESS;
                break;
            }
        }
        fclose( fp );
        return status;
    }
#elif defined(HAVE_SYS_SYSCTL_H) && defined(IPCTL_STATS) && (defined(HAVE_STRUCT_IPSTAT_IPS_TOTAL) || defined(HAVE_STRUCT_IP_STATS_IPS_TOTAL))
    {
        int mib[] = { CTL_NET, PF_INET, IPPROTO_IP, IPCTL_STATS };
#if defined(HAVE_STRUCT_IPSTAT_IPS_TOTAL)
        struct ipstat ip_stat;
#elif defined(HAVE_STRUCT_IP_STATS_IPS_TOTAL)
        struct ip_stats ip_stat;
#endif
        size_t needed;

        needed = sizeof(ip_stat);
        if (sysctl( mib, ARRAY_SIZE(mib), &ip_stat, &needed, NULL, 0 ) == -1) return STATUS_NOT_SUPPORTED;

        dyn.in_recv = ip_stat.ips_total;
        dyn.in_hdr_errs = ip_stat.ips_badhlen + ip_stat.ips_badsum + ip_stat.ips_tooshort + ip_stat.ips_badlen +
            ip_stat.ips_badvers + ip_stat.ips_badoptions;
        /* ips_badaddr also includes outgoing packets with a bad address, but we can't account for that right now */
        dyn.in_addr_errs = ip_stat.ips_cantforward + ip_stat.ips_badaddr + ip_stat.ips_notmember;
        dyn.fwd_dgrams = ip_stat.ips_forward;
        dyn.in_unk_protos = ip_stat.ips_noproto;
        dyn.in_discards = ip_stat.ips_fragdropped;
        dyn.in_delivers = ip_stat.ips_delivered;
        dyn.out_reqs = ip_stat.ips_localout;
        dyn.out_discards = ip_stat.ips_odropped;
        dyn.out_no_routes = ip_stat.ips_noroute;
        stat.reasm_timeout = ip_stat.ips_fragtimeout;
        dyn.reasm_reqds = ip_stat.ips_fragments;
        dyn.reasm_oks = ip_stat.ips_reassembled;
        dyn.reasm_fails = ip_stat.ips_fragments - ip_stat.ips_reassembled;
        dyn.frag_oks = ip_stat.ips_fragmented;
        dyn.frag_fails = ip_stat.ips_cantfrag;
        dyn.frag_creates = ip_stat.ips_ofragments;

        if (dynamic_data) *(struct nsi_ip_ipstats_dynamic *)dynamic_data = dyn;
        if (static_data) *(struct nsi_ip_ipstats_static *)static_data = stat;
        return STATUS_SUCCESS;
    }
#else
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
#endif
}

static NTSTATUS ipv6_ipstats_get_all_parameters( const void *key, UINT key_size, void *rw_data, UINT rw_size,
                                                 void *dynamic_data, UINT dynamic_size, void *static_data, UINT static_size )
{
    struct nsi_ip_ipstats_dynamic dyn;
    struct nsi_ip_ipstats_static stat;

    memset( &dyn, 0, sizeof(dyn) );
    memset( &stat, 0, sizeof(stat) );

#ifdef __linux__
    {
        struct
        {
            const char *name;
            void *elem;
            int size;
        } ipstatlist[] =
        {
#define X(x) &x, sizeof(x)
            { "Ip6InReceives",       X( dyn.in_recv ) },
            { "Ip6InHdrErrors",      X( dyn.in_hdr_errs ) },
            { "Ip6InAddrErrors",     X( dyn.in_addr_errs ) },
            { "Ip6OutForwDatagrams", X( dyn.fwd_dgrams ) },
            { "Ip6InUnknownProtos",  X( dyn.in_unk_protos ) },
            { "Ip6InDiscards",       X( dyn.in_discards ) },
            { "Ip6InDelivers",       X( dyn.in_delivers ) },
            { "Ip6OutRequests",      X( dyn.out_reqs ) },
            { "Ip6OutDiscards",      X( dyn.out_discards ) },
            { "Ip6OutNoRoutes",      X( dyn.out_no_routes ) },
            { "Ip6ReasmTimeout",     X( stat.reasm_timeout ) },
            { "Ip6ReasmReqds",       X( dyn.reasm_reqds ) },
            { "Ip6ReasmOKs",         X( dyn.reasm_oks ) },
            { "Ip6ReasmFails",       X( dyn.reasm_fails ) },
            { "Ip6FragOKs",          X( dyn.frag_oks ) },
            { "Ip6FragFails",        X( dyn.frag_fails ) },
            { "Ip6FragCreates",      X( dyn.frag_creates ) },
            /* no routingDiscards */
#undef X
        };
        NTSTATUS status = STATUS_NOT_SUPPORTED;
        char buf[512], *ptr, *value;
        UINT i;
        FILE *fp;

        if (!(fp = fopen( "/proc/net/snmp6", "r" ))) return STATUS_NOT_SUPPORTED;

        while ((ptr = fgets( buf, sizeof(buf), fp )))
        {
            if (!(value = strchr( buf, ' ' ))) continue;
            /* terminate the valuename */
            *value++ = '\0';
            /* and strip leading spaces from value */
            while (*value == ' ') value++;
            if ((ptr = strchr( value, '\n' ))) *ptr = '\0';

            for (i = 0; i < ARRAY_SIZE(ipstatlist); i++)
                if (!ascii_strcasecmp( buf, ipstatlist[i].name ))
                {
                    if (ipstatlist[i].size == sizeof(UINT))
                        *(UINT *)ipstatlist[i].elem = strtoul( value, NULL, 10 );
                    else
                        *(ULONGLONG *)ipstatlist[i].elem = strtoull( value, NULL, 10 );
                    status = STATUS_SUCCESS;
                }
        }
        fclose( fp );
        if (dynamic_data) *(struct nsi_ip_ipstats_dynamic *)dynamic_data = dyn;
        if (static_data) *(struct nsi_ip_ipstats_static *)static_data = stat;
        return status;
    }
#elif defined(HAVE_SYS_SYSCTL_H) && defined(IPV6CTL_STATS) && (defined(HAVE_STRUCT_IP6STAT_IP6S_TOTAL) || defined(__APPLE__))
    {
        int mib[] = { CTL_NET, PF_INET6, IPPROTO_IPV6, IPV6CTL_STATS };
        struct ip6stat ip_stat;
        size_t needed;

        needed = sizeof(ip_stat);
        if (sysctl( mib, ARRAY_SIZE(mib), &ip_stat, &needed, NULL, 0 ) == -1) return STATUS_NOT_SUPPORTED;

        dyn.in_recv = ip_stat.ip6s_total;
        dyn.in_hdr_errs = ip_stat.ip6s_tooshort + ip_stat.ip6s_toosmall + ip_stat.ip6s_badvers +
            ip_stat.ip6s_badoptions + ip_stat.ip6s_exthdrtoolong + ip_stat.ip6s_toomanyhdr;
        dyn.in_addr_errs = ip_stat.ip6s_cantforward + ip_stat.ip6s_badscope + ip_stat.ip6s_notmember;
        dyn.fwd_dgrams = ip_stat.ip6s_forward;
        dyn.in_discards = ip_stat.ip6s_fragdropped;
        dyn.in_delivers = ip_stat.ip6s_delivered;
        dyn.out_reqs = ip_stat.ip6s_localout;
        dyn.out_discards = ip_stat.ip6s_odropped;
        dyn.out_no_routes = ip_stat.ip6s_noroute;
        stat.reasm_timeout = ip_stat.ip6s_fragtimeout;
        dyn.reasm_reqds = ip_stat.ip6s_fragments;
        dyn.reasm_oks = ip_stat.ip6s_reassembled;
        dyn.reasm_fails = ip_stat.ip6s_fragments - ip_stat.ip6s_reassembled;
        dyn.frag_oks = ip_stat.ip6s_fragmented;
        dyn.frag_fails = ip_stat.ip6s_cantfrag;
        dyn.frag_creates = ip_stat.ip6s_ofragments;

        if (dynamic_data) *(struct nsi_ip_ipstats_dynamic *)dynamic_data = dyn;
        if (static_data) *(struct nsi_ip_ipstats_static *)static_data = stat;
        return STATUS_SUCCESS;
    }
#else
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
#endif
}

static NTSTATUS ip_interface_fill( UINT fam, const char *unix_name, void *key_data, UINT key_size,
                                   void *rw_data, UINT rw_size, void *dynamic_data, UINT dynamic_size,
                                   void *static_data, UINT static_size, UINT_PTR *count )
{
    BOOL want_data = key_size || rw_size || dynamic_size || static_size;
    int base_reachable_time, dad_transmits, site_prefix_len;
    struct nsi_ip_interface_dynamic *dyn = dynamic_data;
    struct nsi_ip_interface_static *stat = static_data;
    struct nsi_ndis_ifinfo_dynamic iface_dynamic;
    struct nsi_ip_interface_key *key = key_data;
    struct nsi_ndis_ifinfo_static iface_static;
    struct ipv6_addr_scope *addr_scopes = NULL;
    unsigned int addr_scopes_size = 0;
    struct nsi_ip_interface_rw *rw = rw_data;
    struct ifaddrs *addrs, *entry, *entry2;
    UINT num = 0, scope_id = 0xffffffff;
    NET_LUID luid;

    if (getifaddrs( &addrs )) return STATUS_NO_MORE_ENTRIES;

    if (fam == AF_INET6) addr_scopes = get_ipv6_addr_scope_table( &addr_scopes_size );

    rw = rw_data;
    for (entry = addrs; entry; entry = entry->ifa_next)
    {
        if (!entry->ifa_addr || entry->ifa_addr->sa_family != fam) continue;
        if (unix_name && strcmp( entry->ifa_name, unix_name )) continue;
        if (fam == AF_INET6)
        {
            scope_id = find_ipv6_addr_scope( (IN6_ADDR*)&((struct sockaddr_in6 *)entry->ifa_addr)->sin6_addr, addr_scopes,
                                             addr_scopes_size );
            /* Info in the IP interface table entry corresponds to link local IPv6 address, while reported info for
             * loopback is different on Windows. */
            if (scope_id != 0xffffffff && scope_id != 0x1000 /* loopback */ && scope_id != 0x2000 /* link_local */)
                continue;
        }
        if (!unix_name)
        {
            /* getifaddrs may return multipe entries for the same interface having different IP addresses.
             * IP interface table being returned has only one entry per network interface. */
            for (entry2 = addrs; entry2 && entry2 != entry; entry2 = entry2->ifa_next)
            {
                if (!entry2->ifa_addr || entry2->ifa_addr->sa_family != fam) continue;
                if (fam == AF_INET6)
                {
                    scope_id = find_ipv6_addr_scope( (IN6_ADDR*)&((struct sockaddr_in6 *)entry2->ifa_addr)->sin6_addr,
                                                     addr_scopes, addr_scopes_size );
                    if (scope_id != 0xffffffff && scope_id != 0x1000 /* loopback */
                        && scope_id != 0x2000 /* link_local */) continue;
                }
                if (!strcmp( entry2->ifa_name, entry->ifa_name ))
                    break;
            }
            if (entry2 != entry) continue;
        }
        if (!convert_unix_name_to_luid( entry->ifa_name, &luid )) continue;
        if (!nsi_get_all_parameters( &NPI_MS_NDIS_MODULEID, NSI_NDIS_IFINFO_TABLE, &luid, sizeof(luid),
                                     NULL, 0, &iface_dynamic, sizeof(iface_dynamic),
                                     &iface_static, sizeof(iface_static) ))
        {
            ERR( "Could not get iface parameters.\n" );
            continue;
        }

        if (!count || num < *count)
        {
            if (key && count) memcpy( key, &luid, sizeof(luid) );
            if (stat) memset( stat, 0, sizeof(*stat) );

            base_reachable_time = 0;
            if (iface_static.type == MIB_IF_TYPE_LOOPBACK) dad_transmits = 0;
            else                                           dad_transmits = (fam == AF_INET6) ? 1 : 3;
#if __linux__
            if (rw || dyn)
            {
                char path[256];
                sprintf( path, "/proc/sys/net/%s/neigh/%s/base_reachable_time_ms",
                         (fam == AF_INET) ? "ipv4" : "ipv6",  entry->ifa_name);
                read_sysctl_int( path, &base_reachable_time );
            }
            if (rw)
            {
                if (fam == AF_INET6 && iface_static.type != MIB_IF_TYPE_LOOPBACK)
                {
                    char path[256];
                    sprintf( path, "/proc/sys/net/ipv6/conf/%s/dad_transmits", entry->ifa_name);
                    read_sysctl_int( path, &dad_transmits );
                }
            }
#endif
            if (rw)
            {
                site_prefix_len = 64;
                if (fam == AF_INET6 && iface_static.type != MIB_IF_TYPE_LOOPBACK)
                {
                    /* For some reason prefix length reported on ipv4 is 64 for ipv4 addresses on Windows and
                     * prefix len is 64 for loopback device. */
                    site_prefix_len = mask_v6_to_prefix( &((struct sockaddr_in6 *)entry->ifa_netmask)->sin6_addr );
                }
                memset( rw, 0, sizeof(*rw) );
                rw->mtu = iface_dynamic.mtu;
                rw->site_prefix_len = site_prefix_len;
                rw->base_reachable_time = base_reachable_time;
                rw->dad_transmits = dad_transmits;
                rw->retransmit_time = 1000;
                rw->path_mtu_discovery_timeout = 600000;
                rw->link_local_address_behavior = (fam == AF_INET6) ? LinkLocalAlwaysOn : LinkLocalDelayed;
                rw->link_local_address_timeout = (fam == AF_INET6) ? 0 : 6500;
            }
            if (dyn)
            {
                memset( dyn, 0, sizeof(*dyn) );
                dyn->if_index = iface_static.if_index;
                dyn->connected = (iface_dynamic.media_conn_state == MediaConnectStateConnected);
                dyn->reachable_time = base_reachable_time;
            }
        }
        if (!count)
        {
            freeifaddrs( addrs );
            free( addr_scopes );
            return STATUS_SUCCESS;
        }
        ++num;
        if (key) ++key;
        if (rw) ++rw;
        if (dyn) ++dyn;
        if (stat) ++stat;
    }
    freeifaddrs( addrs );
    free( addr_scopes );

    if (!count) return STATUS_NOT_FOUND;
    if (want_data && num > *count) return STATUS_BUFFER_OVERFLOW;
    *count = num;
    return STATUS_SUCCESS;
}

static NTSTATUS ipv4_interface_enumerate_all( void *key_data, UINT key_size, void *rw_data, UINT rw_size,
                                              void *dynamic_data, UINT dynamic_size,
                                              void *static_data, UINT static_size, UINT_PTR *count )
{
    return ip_interface_fill( AF_INET, NULL, key_data, key_size, rw_data, rw_size, dynamic_data, dynamic_size,
                              static_data, static_size, count );
}

static NTSTATUS ipv4_interface_get_all_parameters( const void *key, UINT key_size, void *rw_data, UINT rw_size,
                                                   void *dynamic_data, UINT dynamic_size, void *static_data, UINT static_size )
{
    struct nsi_ip_interface_key *ip_key = (void *)key;
    const char *unix_name;

    if (!convert_luid_to_unix_name( &ip_key->luid, &unix_name )) return STATUS_NOT_FOUND;

    return ip_interface_fill( AF_INET, unix_name, ip_key, key_size, rw_data, rw_size, dynamic_data, dynamic_size,
                              static_data, static_size, NULL );
}

static NTSTATUS ipv6_interface_enumerate_all( void *key_data, UINT key_size, void *rw_data, UINT rw_size,
                                              void *dynamic_data, UINT dynamic_size,
                                              void *static_data, UINT static_size, UINT_PTR *count )
{
    return ip_interface_fill( AF_INET6, NULL, key_data, key_size, rw_data, rw_size, dynamic_data, dynamic_size,
                              static_data, static_size, count );
}

static NTSTATUS ipv6_interface_get_all_parameters( const void *key, UINT key_size, void *rw_data, UINT rw_size,
                                                   void *dynamic_data, UINT dynamic_size, void *static_data, UINT static_size )
{
    struct nsi_ip_interface_key *ip_key = (void *)key;
    const char *unix_name;

    if (!convert_luid_to_unix_name( &ip_key->luid, &unix_name )) return STATUS_NOT_FOUND;

    return ip_interface_fill( AF_INET6, unix_name, ip_key, key_size, rw_data, rw_size, dynamic_data, dynamic_size,
                              static_data, static_size, NULL );
}

static void unicast_fill_entry( struct ifaddrs *entry, void *key, struct nsi_ip_unicast_rw *rw,
                                struct nsi_ip_unicast_dynamic *dyn, struct nsi_ip_unicast_static *stat )
{
    struct nsi_ipv6_unicast_key placeholder, *key6 = key;
    struct nsi_ipv4_unicast_key *key4 = key;
    UINT scope_id = 0;

    if (!key)
    {
        key6 = &placeholder;
        key4 = (struct nsi_ipv4_unicast_key *)&placeholder;
    }

    convert_unix_name_to_luid( entry->ifa_name, &key6->luid );

    if (entry->ifa_addr->sa_family == AF_INET)
    {
        memcpy( &key4->addr, &((struct sockaddr_in *)entry->ifa_addr)->sin_addr, sizeof(key4->addr) );
        key4->pad = 0;
    }
    else if (entry->ifa_addr->sa_family == AF_INET6)
    {
        memcpy( &key6->addr, &((struct sockaddr_in6 *)entry->ifa_addr)->sin6_addr, sizeof(key6->addr) );
        scope_id = ((struct sockaddr_in6 *)entry->ifa_addr)->sin6_scope_id;
    }

    if (rw)
    {
        rw->preferred_lifetime = 60000;
        rw->valid_lifetime = 60000;

        if (key6->luid.Info.IfType != IF_TYPE_SOFTWARE_LOOPBACK)
        {
            rw->prefix_origin = IpPrefixOriginDhcp;
            rw->suffix_origin = IpSuffixOriginDhcp;
        }
        else
        {
            rw->prefix_origin = IpPrefixOriginManual;
            rw->suffix_origin = IpSuffixOriginManual;
        }
        if (entry->ifa_netmask && entry->ifa_netmask->sa_family == AF_INET)
            rw->on_link_prefix = mask_v4_to_prefix( &((struct sockaddr_in *)entry->ifa_netmask)->sin_addr );
        else if (entry->ifa_netmask && entry->ifa_netmask->sa_family == AF_INET6)
            rw->on_link_prefix = mask_v6_to_prefix( &((struct sockaddr_in6 *)entry->ifa_netmask)->sin6_addr );
        else rw->on_link_prefix = 0;
        rw->unk[0] = 0;
        rw->unk[1] = 0;
    }

    if (dyn)
    {
        dyn->scope_id = scope_id;
        dyn->dad_state = IpDadStatePreferred;
    }

    if (stat) stat->creation_time = get_boot_time();
}

static NTSTATUS ip_unicast_enumerate_all( int family, void *key_data, UINT key_size, void *rw_data, UINT rw_size,
                                          void *dynamic_data, UINT dynamic_size,
                                          void *static_data, UINT static_size, UINT_PTR *count )
{
    UINT num = 0;
    NTSTATUS status = STATUS_SUCCESS;
    BOOL want_data = key_size || rw_size || dynamic_size || static_size;
    struct ifaddrs *addrs, *entry;

    TRACE( "%p %d %p %d %p %d %p %d %p\n", key_data, key_size, rw_data, rw_size,
           dynamic_data, dynamic_size, static_data, static_size, count );

    if (getifaddrs( &addrs )) return STATUS_NO_MORE_ENTRIES;

    for (entry = addrs; entry; entry = entry->ifa_next)
    {
        if (!entry->ifa_addr || entry->ifa_addr->sa_family != family) continue;

        if (num < *count)
        {
            unicast_fill_entry( entry, key_data, rw_data, dynamic_data, static_data );
            key_data = (BYTE *)key_data + key_size;
            rw_data = (BYTE *)rw_data + rw_size;
            dynamic_data = (BYTE *)dynamic_data + dynamic_size;
            static_data = (BYTE *)static_data + static_size;
        }
        num++;
    }

    freeifaddrs( addrs );

    if (!want_data || num <= *count) *count = num;
    else status = STATUS_BUFFER_OVERFLOW;

    return status;
}

static NTSTATUS ipv4_unicast_enumerate_all( void *key_data, UINT key_size, void *rw_data, UINT rw_size,
                                            void *dynamic_data, UINT dynamic_size,
                                            void *static_data, UINT static_size, UINT_PTR *count )
{
    return ip_unicast_enumerate_all( AF_INET, key_data, key_size, rw_data, rw_size,
                                     dynamic_data, dynamic_size, static_data, static_size, count );
}

static NTSTATUS ipv6_unicast_enumerate_all( void *key_data, UINT key_size, void *rw_data, UINT rw_size,
                                            void *dynamic_data, UINT dynamic_size,
                                            void *static_data, UINT static_size, UINT_PTR *count )
{
    return ip_unicast_enumerate_all( AF_INET6, key_data, key_size, rw_data, rw_size,
                                     dynamic_data, dynamic_size, static_data, static_size, count );
}

static NTSTATUS ip_unicast_get_all_parameters( const void *key, UINT key_size, void *rw_data, UINT rw_size,
                                               void *dynamic_data, UINT dynamic_size,
                                               void *static_data, UINT static_size )
{
    int family = (key_size == sizeof(struct nsi_ipv4_unicast_key)) ? AF_INET : AF_INET6;
    NTSTATUS status = STATUS_NOT_FOUND;
    const struct nsi_ipv6_unicast_key *key6 = key;
    const struct nsi_ipv4_unicast_key *key4 = key;
    struct ifaddrs *addrs, *entry;
    const char *unix_name;

    TRACE( "%p %d %p %d %p %d %p %d\n", key, key_size, rw_data, rw_size, dynamic_data, dynamic_size,
           static_data, static_size );

    if (!convert_luid_to_unix_name( &key6->luid, &unix_name )) return STATUS_NOT_FOUND;

    if (getifaddrs( &addrs )) return STATUS_NO_MORE_ENTRIES;

    for (entry = addrs; entry; entry = entry->ifa_next)
    {
        if (!entry->ifa_addr || entry->ifa_addr->sa_family != family) continue;
        if (strcmp( entry->ifa_name, unix_name )) continue;

        if (family == AF_INET &&
            memcmp( &key4->addr, &((struct sockaddr_in *)entry->ifa_addr)->sin_addr, sizeof(key4->addr) )) continue;
        if (family == AF_INET6 &&
            memcmp( &key6->addr, &((struct sockaddr_in6 *)entry->ifa_addr)->sin6_addr, sizeof(key6->addr) )) continue;

        unicast_fill_entry( entry, NULL, rw_data, dynamic_data, static_data );
        status = STATUS_SUCCESS;
        break;
    }

    freeifaddrs( addrs );
    return status;
}

struct ipv4_neighbour_data
{
    NET_LUID luid;
    UINT if_index;
    struct in_addr addr;
    BYTE phys_addr[IF_MAX_PHYS_ADDRESS_LENGTH];
    UINT state;
    USHORT phys_addr_len;
    BOOL is_router;
    BOOL is_unreachable;
};

static void ipv4_neighbour_fill_entry( struct ipv4_neighbour_data *entry, struct nsi_ipv4_neighbour_key *key, struct nsi_ip_neighbour_rw *rw,
                                       struct nsi_ip_neighbour_dynamic *dyn, void *stat )
{
    USHORT phys_addr_len = entry->phys_addr_len > sizeof(rw->phys_addr) ? 0 : entry->phys_addr_len;

    if (key)
    {
        key->luid = entry->luid;
        key->luid2 = entry->luid;
        key->addr.WS_s_addr = entry->addr.s_addr;
        key->pad = 0;
    }

    if (rw)
    {
        memcpy( rw->phys_addr, entry->phys_addr, phys_addr_len );
        memset( rw->phys_addr + entry->phys_addr_len, 0, sizeof(rw->phys_addr) - phys_addr_len );
    }

    if (dyn)
    {
        memset( dyn, 0, sizeof(*dyn) );
        dyn->state = entry->state;
        dyn->flags.is_router = entry->is_router;
        dyn->flags.is_unreachable = entry->is_unreachable;
        dyn->phys_addr_len = phys_addr_len;
    }
}

/* ARP entries for these multicast addresses are always present on Windows for each interface. */
static DWORD ipv4_multicast_addresses[] =
{
    IPV4_ADDR(224, 0, 0, 22),
    IPV4_ADDR(239, 255, 255, 250),
};

static void update_static_address_found( DWORD address, UINT if_index, struct nsi_ndis_ifinfo_static *iface,
                                         unsigned int iface_count )
{
    unsigned int i, j;

    for (i = 0; i < ARRAY_SIZE(ipv4_multicast_addresses); ++i)
        if (ipv4_multicast_addresses[i] == address) break;

    if (i == ARRAY_SIZE(ipv4_multicast_addresses)) return;

    for (j = 0; j < iface_count; ++j)
    {
        if (iface[j].if_index == if_index)
        {
            iface[j].unk |= 1 << i;
            return;
        }
    }
}

static NTSTATUS ipv4_neighbour_enumerate_all( void *key_data, UINT key_size, void *rw_data, UINT rw_size,
                                              void *dynamic_data, UINT dynamic_size,
                                              void *static_data, UINT static_size, UINT_PTR *count )
{
    UINT num = 0, iface_count;
    NTSTATUS status = STATUS_SUCCESS;
    BOOL want_data = key_size || rw_size || dynamic_size || static_size;
    struct nsi_ndis_ifinfo_static *iface_static;
    struct ipv4_neighbour_data entry;
    NET_LUID *luid_tbl;
    unsigned int i, j;

    TRACE( "%p %d %p %d %p %d %p %d %p\n", key_data, key_size, rw_data, rw_size,
           dynamic_data, dynamic_size, static_data, static_size, count );

    iface_count = 0;
    if ((status = nsi_enumerate_all( 1, 0, &NPI_MS_NDIS_MODULEID, NSI_NDIS_IFINFO_TABLE, NULL, 0, NULL, 0,
                                     NULL, 0, NULL, 0, &iface_count )))
        return status;

    if (!(luid_tbl = malloc( iface_count * sizeof(*luid_tbl) )))
        return STATUS_NO_MEMORY;

    if (!(iface_static = malloc( iface_count * sizeof(*iface_static) )))
    {
        free( luid_tbl );
        return STATUS_NO_MEMORY;
    }

    if ((status = nsi_enumerate_all( 1, 0, &NPI_MS_NDIS_MODULEID, NSI_NDIS_IFINFO_TABLE, luid_tbl, sizeof(*luid_tbl),
                                     NULL, 0, NULL, 0, iface_static, sizeof(*iface_static), &iface_count ))
        && status != STATUS_BUFFER_OVERFLOW)
    {
        free( luid_tbl );
        free( iface_static );
        return status;
    }

    /* Use unk field to indicate whether we found mandatory multicast addresses in the host ARP table. */
    for (i = 0; i < iface_count; ++i)
        iface_static[i].unk = 0;

#ifdef __linux__
    {
        char buf[512], *ptr, *s;
        UINT atf_flags;
        FILE *fp;

        if (!(fp = fopen( "/proc/net/arp", "r" ))) return STATUS_NOT_SUPPORTED;

        /* skip header line */
        ptr = fgets( buf, sizeof(buf), fp );
        while ((ptr = fgets( buf, sizeof(buf), fp )))
        {
            entry.addr.s_addr = inet_addr( ptr );
            while (*ptr && !isspace( *ptr )) ptr++;
            strtoul( ptr + 1, &ptr, 16 ); /* hw type (skip) */
            atf_flags = strtoul( ptr + 1, &ptr, 16 );

            if (atf_flags & ATF_PERM) entry.state = NlnsPermanent;
            else if (atf_flags & ATF_COM) entry.state = NlnsReachable;
            else entry.state = NlnsStale;

            entry.is_router = 0;
            entry.is_unreachable = !(atf_flags & (ATF_PERM | ATF_COM));

            while (*ptr && isspace( *ptr )) ptr++;
            entry.phys_addr_len = 0;
            while (*ptr && !isspace( *ptr ))
            {
                if (entry.phys_addr_len >= sizeof(entry.phys_addr))
                {
                    entry.phys_addr_len = 0;
                    while (*ptr && !isspace( *ptr )) ptr++;
                    break;
                }
                entry.phys_addr[entry.phys_addr_len++] = strtoul( ptr, &ptr, 16 );
                if (*ptr) ptr++;
            }
            while (*ptr && isspace( *ptr )) ptr++;
            while (*ptr && !isspace( *ptr )) ptr++;   /* mask (skip) */
            while (*ptr && isspace( *ptr )) ptr++;

            s = ptr;
            while (*s && !isspace(*s)) s++;
            *s = 0;
            if (!convert_unix_name_to_luid( ptr, &entry.luid )) continue;
            if (!convert_luid_to_index( &entry.luid, &entry.if_index )) continue;

            update_static_address_found( entry.addr.s_addr, entry.if_index, iface_static, iface_count );

            if (num < *count)
            {
                ipv4_neighbour_fill_entry( &entry, key_data, rw_data, dynamic_data, static_data );

                if (key_data) key_data = (BYTE *)key_data + key_size;
                if (rw_data) rw_data = (BYTE *)rw_data + rw_size;
                if (dynamic_data) dynamic_data = (BYTE *)dynamic_data + dynamic_size;
                if (static_data) static_data = (BYTE *)static_data + static_size;
            }
            num++;
        }
        fclose( fp );
    }
#elif defined(HAVE_SYS_SYSCTL_H)
    {
        int mib[] = { CTL_NET, PF_ROUTE, 0, AF_INET, NET_RT_FLAGS, RTF_LLINFO }, sinarp_len;
        size_t needed;
        char *buf = NULL, *lim, *next;
        struct rt_msghdr *rtm;
        struct sockaddr_inarp *sinarp;
        struct sockaddr_dl *sdl;

        if (sysctl( mib, ARRAY_SIZE(mib), NULL, &needed, NULL, 0 ) == -1) return STATUS_NOT_SUPPORTED;

        buf = malloc( needed );
        if (!buf) return STATUS_NO_MEMORY;

        if (sysctl( mib, ARRAY_SIZE(mib), buf, &needed, NULL, 0 ) == -1)
        {
            free( buf );
            return STATUS_NOT_SUPPORTED;
        }

        lim = buf + needed;
        next = buf;
        while (next < lim)
        {
            rtm = (struct rt_msghdr *)next;
            sinarp = (struct sockaddr_inarp *)(rtm + 1);
            if (sinarp->sin_len) sinarp_len = (sinarp->sin_len + sizeof(int)-1) & ~(sizeof(int)-1);
            else sinarp_len = sizeof(int);
            sdl = (struct sockaddr_dl *)((char *)sinarp + sinarp_len);

            if (sdl->sdl_alen) /* arp entry */
            {
                entry.addr = sinarp->sin_addr;
                entry.if_index = sdl->sdl_index;
                if (!convert_index_to_luid( entry.if_index, &entry.luid )) break;
                entry.phys_addr_len = min( 8, sdl->sdl_alen );
                if (entry.phys_addr_len > sizeof(entry.phys_addr)) entry.phys_addr_len = 0;
                memcpy( entry.phys_addr, &sdl->sdl_data[sdl->sdl_nlen], entry.phys_addr_len );
                if (rtm->rtm_rmx.rmx_expire == 0) entry.state = NlnsPermanent;
                else entry.state = NlnsReachable;
#ifdef SIN_ROUTER
                entry.is_router = sinarp->sin_other & SIN_ROUTER;
#else
                entry.is_router = 0;
#endif
                entry.is_unreachable = 0; /* FIXME */

                update_static_address_found( entry.addr.s_addr, entry.if_index, iface_static, iface_count );

                if (num < *count)
                {
                    ipv4_neighbour_fill_entry( &entry, key_data, rw_data, dynamic_data, static_data );

                    if (key_data) key_data = (BYTE *)key_data + key_size;
                    if (rw_data) rw_data = (BYTE *)rw_data + rw_size;
                    if (dynamic_data) dynamic_data = (BYTE *)dynamic_data + dynamic_size;
                    if (static_data) static_data = (BYTE *)static_data + static_size;
                }
                num++;
            }
            next += rtm->rtm_msglen;
        }
        free( buf );
    }
#else
    FIXME( "not implemented\n" );
    free( luid_tbl );
    free( iface_static );
    return STATUS_NOT_IMPLEMENTED;
#endif

    if (!want_data || num <= *count)
    {
        /* Certain ipv4 multicast addresses are always present on Windows for each interface.
         * Add those if they weren't added already. */
        memset( &entry, 0, sizeof(entry) );
        entry.state = NlnsPermanent;
        for (i = 0; i < iface_count; ++i)
        {
            entry.if_index = iface_static[i].if_index;
            entry.luid = luid_tbl[i];
            for (j = 0; j < ARRAY_SIZE(ipv4_multicast_addresses); ++j)
            {
                if (iface_static[i].unk & (1 << j)) continue;
                if (num < *count)
                {
                    entry.addr.s_addr = ipv4_multicast_addresses[j];
                    ipv4_neighbour_fill_entry( &entry, key_data, rw_data, dynamic_data, static_data );

                    if (key_data) key_data = (BYTE *)key_data + key_size;
                    if (rw_data) rw_data = (BYTE *)rw_data + rw_size;
                    if (dynamic_data) dynamic_data = (BYTE *)dynamic_data + dynamic_size;
                    if (static_data) static_data = (BYTE *)static_data + static_size;
                }
                ++num;
            }
        }
    }

    free( luid_tbl );
    free( iface_static );

    if (!want_data || num <= *count) *count = num;
    else status = STATUS_BUFFER_OVERFLOW;

    return status;
}

static NTSTATUS ipv6_neighbour_enumerate_all( void *key_data, UINT key_size, void *rw_data, UINT rw_size,
                                              void *dynamic_data, UINT dynamic_size,
                                              void *static_data, UINT static_size, UINT_PTR *count )
{
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
}

struct ipv4_route_data
{
    NET_LUID luid;
    UINT if_index;
    struct in_addr prefix;
    UINT prefix_len;
    struct in_addr next_hop;
    UINT metric;
    UINT protocol;
    BYTE loopback;
};

static void ipv4_forward_fill_entry( struct ipv4_route_data *entry, struct nsi_ipv4_forward_key *key,
                                     struct nsi_ip_forward_rw *rw, struct nsi_ipv4_forward_dynamic *dyn,
                                     struct nsi_ip_forward_static *stat )
{
    if (key)
    {
        key->unk = 0;
        key->prefix.WS_s_addr = entry->prefix.s_addr;
        key->prefix_len = entry->prefix_len;
        memset( key->unk2, 0, sizeof(key->unk2) );
        memset( key->unk3, 0, sizeof(key->unk3) );
        key->luid = entry->luid;
        key->luid2 = entry->luid;
        key->next_hop.WS_s_addr = entry->next_hop.s_addr;
        key->pad = 0;
    }

    if (rw)
    {
        rw->site_prefix_len = 0;
        rw->valid_lifetime = ~0u;
        rw->preferred_lifetime = ~0u;
        rw->metric = entry->metric;
        rw->protocol = entry->protocol;
        rw->loopback = entry->loopback;
        rw->autoconf = 1;
        rw->publish = 0;
        rw->immortal = 1;
        memset( rw->unk, 0, sizeof(rw->unk) );
        rw->unk2 = 0;
    }

    if (dyn)
    {
        memset( dyn, 0, sizeof(*dyn) );
    }

    if (stat)
    {
        stat->origin = NlroManual;
        stat->if_index = entry->if_index;
    }
}

static NTSTATUS ipv4_forward_enumerate_all( void *key_data, UINT key_size, void *rw_data, UINT rw_size,
                                            void *dynamic_data, UINT dynamic_size,
                                            void *static_data, UINT static_size, UINT_PTR *count )
{
    UINT num = 0;
    NTSTATUS status = STATUS_SUCCESS;
    BOOL want_data = key_size || rw_size || dynamic_size || static_size;
    struct ipv4_route_data entry;

    TRACE( "%p %d %p %d %p %d %p %d %p\n", key_data, key_size, rw_data, rw_size,
           dynamic_data, dynamic_size, static_data, static_size, count );

#ifdef __linux__
    {
        struct ifaddrs *addrs, *ifentry;
        char buf[512], *ptr;
        struct in_addr mask;
        UINT rtf_flags;
        FILE *fp;

        /* Loopback routes are not present in /proc/net/routes, add those explicitly. */
        if (getifaddrs( &addrs )) return STATUS_NO_MORE_ENTRIES;
        for (ifentry = addrs; ifentry; ifentry = ifentry->ifa_next)
        {
            if (!(ifentry->ifa_flags & IFF_LOOPBACK)) continue;
            if (!convert_unix_name_to_luid( ifentry->ifa_name, &entry.luid )) continue;
            if (!convert_luid_to_index( &entry.luid, &entry.if_index )) continue;

            if (num < *count)
            {
                entry.prefix.s_addr = htonl( 0x7f000000 );
                entry.next_hop.s_addr = 0;
                entry.metric = 256;
                entry.prefix_len = 8;
                entry.protocol = MIB_IPPROTO_LOCAL;
                entry.loopback = 1;
                ipv4_forward_fill_entry( &entry, key_data, rw_data, dynamic_data, static_data );
                key_data = (BYTE *)key_data + key_size;
                rw_data = (BYTE *)rw_data + rw_size;
                dynamic_data = (BYTE *)dynamic_data + dynamic_size;
                static_data = (BYTE *)static_data + static_size;
            }
            num++;
            if (num < *count)
            {
                entry.prefix.s_addr = htonl( INADDR_LOOPBACK );
                entry.prefix_len = 32;
                ipv4_forward_fill_entry( &entry, key_data, rw_data, dynamic_data, static_data );
                key_data = (BYTE *)key_data + key_size;
                rw_data = (BYTE *)rw_data + rw_size;
                dynamic_data = (BYTE *)dynamic_data + dynamic_size;
                static_data = (BYTE *)static_data + static_size;
            }
            num++;
            break;
        }
        freeifaddrs( addrs );

        if (!(fp = fopen( "/proc/net/route", "r" ))) return STATUS_NOT_SUPPORTED;

        /* skip header line */
        fgets( buf, sizeof(buf), fp );
        while ((ptr = fgets( buf, sizeof(buf), fp )))
        {
            while (!isspace( *ptr )) ptr++;
            *ptr++ = '\0';

            if (!convert_unix_name_to_luid( buf, &entry.luid )) continue;
            if (!convert_luid_to_index( &entry.luid, &entry.if_index )) continue;

            entry.prefix.s_addr = strtoul( ptr, &ptr, 16 );
            entry.next_hop.s_addr = strtoul( ptr + 1, &ptr, 16 );
            rtf_flags = strtoul( ptr + 1, &ptr, 16 );
            strtoul( ptr + 1, &ptr, 16 ); /* refcount, skip */
            strtoul( ptr + 1, &ptr, 16 ); /* use, skip */
            entry.metric = strtoul( ptr + 1, &ptr, 16 );
            mask.s_addr = strtoul( ptr + 1, &ptr, 16 );
            entry.prefix_len = mask_v4_to_prefix( &mask );
            entry.protocol = (rtf_flags & RTF_GATEWAY) ? MIB_IPPROTO_NETMGMT : MIB_IPPROTO_LOCAL;
            entry.loopback = entry.protocol == MIB_IPPROTO_LOCAL && entry.prefix_len == 32;

            if (num < *count)
            {
                ipv4_forward_fill_entry( &entry, key_data, rw_data, dynamic_data, static_data );
                key_data = (BYTE *)key_data + key_size;
                rw_data = (BYTE *)rw_data + rw_size;
                dynamic_data = (BYTE *)dynamic_data + dynamic_size;
                static_data = (BYTE *)static_data + static_size;
            }
            num++;
        }
        fclose( fp );
    }
#elif defined(HAVE_SYS_SYSCTL_H) && defined(NET_RT_DUMP)
    {
        int mib[6] = { CTL_NET, PF_ROUTE, 0, PF_INET, NET_RT_DUMP, 0 };
        size_t needed;
        char *buf = NULL, *lim, *next, *addr_ptr;
        struct rt_msghdr *rtm;

        if (sysctl( mib, ARRAY_SIZE(mib), NULL, &needed, NULL, 0 ) < 0) return STATUS_NOT_SUPPORTED;

        buf = malloc( needed );
        if (!buf) return STATUS_NO_MEMORY;

        if (sysctl( mib, 6, buf, &needed, NULL, 0 ) < 0)
        {
            free( buf );
            return STATUS_NOT_SUPPORTED;
        }

        lim = buf + needed;
        for (next = buf; next < lim; next += rtm->rtm_msglen)
        {
            int i;
            sa_family_t dst_family = AF_UNSPEC;

            rtm = (struct rt_msghdr *)next;

            if (rtm->rtm_type != RTM_GET)
            {
                WARN( "Got unexpected message type 0x%x!\n", rtm->rtm_type );
                continue;
            }

            /* Ignore gateway routes which are multicast */
            if ((rtm->rtm_flags & RTF_GATEWAY) && (rtm->rtm_flags & RTF_MULTICAST)) continue;

            entry.if_index = rtm->rtm_index;
            if (!convert_index_to_luid( entry.if_index, &entry.luid )) continue;
            entry.protocol = (rtm->rtm_flags & RTF_GATEWAY) ? MIB_IPPROTO_NETMGMT : MIB_IPPROTO_LOCAL;
            entry.metric = rtm->rtm_rmx.rmx_hopcount;

            addr_ptr = (char *)(rtm + 1);

            for (i = 1; i; i <<= 1)
            {
                struct sockaddr *sa;
                struct in_addr addr;

                if (!(i & rtm->rtm_addrs)) continue;

                sa = (struct sockaddr *)addr_ptr;
                if (addr_ptr + sa->sa_len > next + rtm->rtm_msglen)
                {
                    ERR( "struct sockaddr extends beyond the route message, %p > %p\n",
                         addr_ptr + sa->sa_len, next + rtm->rtm_msglen );
                }

                if (sa->sa_len) addr_ptr += (sa->sa_len + sizeof(int)-1) & ~(sizeof(int)-1);
                else addr_ptr += sizeof(int);
                /* Apple's netstat prints the netmask together with the destination
                 * and only looks at the destination's address family. The netmask's
                 * sa_family sometimes contains the non-existent value 0xff. */
                switch (i == RTA_NETMASK ? dst_family : sa->sa_family)
                {
                case AF_INET:
                {
                    /* Netmasks (and possibly other addresses) have only enough size
                     * to represent the non-zero bits, e.g. a netmask of 255.0.0.0 has
                     * 5 bytes (1 sa_len, 1 sa_family, 2 sa_port and 1 for the first
                     * byte of sin_addr). */
                    struct sockaddr_in sin = {0};
                    memcpy( &sin, sa, sa->sa_len );
                    addr = sin.sin_addr;
                    break;
                }
#ifdef AF_LINK
                case AF_LINK:
                    if (i == RTA_GATEWAY && entry.protocol ==  MIB_IPPROTO_NETMGMT)
                    {
                        /* For direct route we may simply use dest addr as next hop */
                        C_ASSERT(RTA_DST < RTA_GATEWAY);
                        addr = entry.prefix;
                        break;
                    }
                    /* fallthrough */
#endif
                default:
                    WARN( "Received unsupported sockaddr family 0x%x\n", sa->sa_family );
                    addr.s_addr = 0;
                }
                switch (i)
                {
                case RTA_DST:
                    entry.prefix = addr;
                    dst_family = sa->sa_family;
                    break;
                case RTA_GATEWAY: entry.next_hop = addr; break;
                case RTA_NETMASK: entry.prefix_len = mask_v4_to_prefix( &addr ); break;
                default:
                    WARN( "Unexpected address type 0x%x\n", i );
                }
            }

            if (num < *count)
            {
                ipv4_forward_fill_entry( &entry, key_data, rw_data, dynamic_data, static_data );
                key_data = (BYTE *)key_data + key_size;
                rw_data = (BYTE *)rw_data + rw_size;
                dynamic_data = (BYTE *)dynamic_data + dynamic_size;
                static_data = (BYTE *)static_data + static_size;
            }
            num++;
        }
        free( buf );
    }
#else
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
#endif

    if (!want_data || num <= *count) *count = num;
    else status = STATUS_BUFFER_OVERFLOW;

    return status;
}

#ifdef __linux__
struct ipv6_route_data
{
    NET_LUID luid;
    UINT if_index;
    struct in6_addr prefix;
    UINT prefix_len;
    struct in6_addr next_hop;
    UINT metric;
    UINT protocol;
    BYTE loopback;
};

static void ipv6_forward_fill_entry( struct ipv6_route_data *entry, struct nsi_ipv6_forward_key *key,
                                     struct nsi_ip_forward_rw *rw, struct nsi_ipv6_forward_dynamic *dyn,
                                     struct nsi_ip_forward_static *stat )
{
    if (key)
    {
        key->unk = 0;
        memcpy( key->prefix.u.Byte, entry->prefix.s6_addr, sizeof(entry->prefix.s6_addr) );
        key->prefix_len = entry->prefix_len;
        memset( key->unk2, 0, sizeof(key->unk2) );
        memset( key->unk3, 0, sizeof(key->unk3) );
        key->luid = entry->luid;
        key->luid2 = entry->luid;
        memcpy( key->next_hop.u.Byte, entry->next_hop.s6_addr, sizeof(entry->next_hop.s6_addr) );
        key->pad = 0;
    }

    if (rw)
    {
        rw->site_prefix_len = 0;
        rw->valid_lifetime = ~0u;
        rw->preferred_lifetime = ~0u;
        rw->metric = entry->metric;
        rw->protocol = entry->protocol;
        rw->loopback = entry->loopback;
        rw->autoconf = 1;
        rw->publish = 0;
        rw->immortal = 1;
        memset( rw->unk, 0, sizeof(rw->unk) );
        rw->unk2 = 0;
    }

    if (dyn)
    {
        memset( dyn, 0, sizeof(*dyn) );
    }

    if (stat)
    {
        stat->origin = NlroManual;
        stat->if_index = entry->if_index;
    }
}
#endif

struct in6_addr str_to_in6_addr(char *nptr, char **endptr)
{
    struct in6_addr ret;

    for (int i = 0; i < sizeof(ret); i++)
    {
        if (!isxdigit( *nptr ) || !isxdigit( *(nptr + 1) ))
        {
            /* invalid hex string */
            if (endptr) *endptr = nptr;
            return ret;
        }

        sscanf( nptr, "%2hhx", &ret.s6_addr[i] );
        nptr += 2;
    }

    if (endptr) *endptr = nptr;

    return ret;
}

static NTSTATUS ipv6_forward_enumerate_all( void *key_data, UINT key_size, void *rw_data, UINT rw_size,
                                            void *dynamic_data, UINT dynamic_size,
                                            void *static_data, UINT static_size, UINT_PTR *count )
{
    UINT num = 0;
    NTSTATUS status = STATUS_SUCCESS;
    BOOL want_data = key_size || rw_size || dynamic_size || static_size;

    TRACE( "%p %d %p %d %p %d %p %d %p\n" , key_data, key_size, rw_data, rw_size,
        dynamic_data, dynamic_size, static_data, static_size, count );

#ifdef __linux__
    {
        struct ipv6_route_data entry;
        char buf[512], *ptr, *end;
        UINT rtf_flags;
        FILE *fp;

        if (!(fp = fopen( "/proc/net/ipv6_route", "r" )))
        {
            *count = 0;
            return STATUS_SUCCESS;
        }

        while ((ptr = fgets( buf, sizeof(buf), fp )))
        {
            entry.prefix = str_to_in6_addr( ptr, &ptr );
            entry.prefix_len = strtoul( ptr + 1, &ptr, 16 );
            str_to_in6_addr( ptr + 1, &ptr ); /* source network, skip */
            strtoul( ptr + 1, &ptr, 16 ); /* source prefix length, skip */
            entry.next_hop = str_to_in6_addr( ptr + 1, &ptr );
            entry.metric = strtoul( ptr + 1, &ptr, 16 );
            strtoul( ptr + 1, &ptr, 16 ); /* refcount, skip */
            strtoul( ptr + 1, &ptr, 16 ); /* use, skip */
            rtf_flags = strtoul( ptr + 1, &ptr, 16);
            if (!(rtf_flags & RTF_UP)) continue;
            entry.protocol = (rtf_flags & RTF_GATEWAY) ? MIB_IPPROTO_NETMGMT : MIB_IPPROTO_LOCAL;
            entry.loopback = entry.protocol == MIB_IPPROTO_LOCAL && entry.prefix_len == 32;

            while (isspace( *ptr )) ptr++;
            end = ptr;
            while (*end && !isspace(*end)) ++end;
            *end = 0;
            if (!convert_unix_name_to_luid( ptr, &entry.luid )) continue;
            if (!convert_luid_to_index( &entry.luid, &entry.if_index )) continue;

            if (num < *count)
            {
                ipv6_forward_fill_entry( &entry, key_data, rw_data, dynamic_data, static_data );
                key_data = (BYTE *)key_data + key_size;
                rw_data = (BYTE *)rw_data + rw_size;
                dynamic_data = (BYTE *)dynamic_data + dynamic_size;
                static_data = (BYTE *)static_data + static_size;
            }
            num++;
        }
        fclose(fp);
    }
#else
    FIXME( "not implemented\n" );
    *count = 0;
    return STATUS_SUCCESS;
#endif

    if (!want_data || num <= *count) *count = num;
    else status = STATUS_BUFFER_OVERFLOW;

    return status;
}

static struct module_table ipv4_tables[] =
{
    {
        NSI_IP_COMPARTMENT_TABLE,
        {
            sizeof(UINT), sizeof(struct nsi_ip_cmpt_rw),
            sizeof(struct nsi_ip_cmpt_dynamic), 0
        },
        NULL,
        ipv4_cmpt_get_all_parameters,
    },
    {
        NSI_IP_ICMPSTATS_TABLE,
        {
            0, 0,
            sizeof(struct nsi_ip_icmpstats_dynamic), 0
        },
        NULL,
        ipv4_icmpstats_get_all_parameters,
    },
    {
        NSI_IP_IPSTATS_TABLE,
        {
            0, 0,
            sizeof(struct nsi_ip_ipstats_dynamic), sizeof(struct nsi_ip_ipstats_static)
        },
        NULL,
        ipv4_ipstats_get_all_parameters,
    },
    {
        NSI_IP_INTERFACE_TABLE,
        {
            sizeof(struct nsi_ip_interface_key), sizeof(struct nsi_ip_interface_rw),
            sizeof(struct nsi_ip_interface_dynamic), sizeof(struct nsi_ip_interface_static)
        },
        ipv4_interface_enumerate_all,
        ipv4_interface_get_all_parameters,
    },
    {
        NSI_IP_UNICAST_TABLE,
        {
            sizeof(struct nsi_ipv4_unicast_key), sizeof(struct nsi_ip_unicast_rw),
            sizeof(struct nsi_ip_unicast_dynamic), sizeof(struct nsi_ip_unicast_static)
        },
        ipv4_unicast_enumerate_all,
        ip_unicast_get_all_parameters,
    },
    {
        NSI_IP_NEIGHBOUR_TABLE,
        {
            sizeof(struct nsi_ipv4_neighbour_key), sizeof(struct nsi_ip_neighbour_rw),
            sizeof(struct nsi_ip_neighbour_dynamic), 0
        },
        ipv4_neighbour_enumerate_all,
    },
    {
        NSI_IP_FORWARD_TABLE,
        {
            sizeof(struct nsi_ipv4_forward_key), sizeof(struct nsi_ip_forward_rw),
            sizeof(struct nsi_ipv4_forward_dynamic), sizeof(struct nsi_ip_forward_static)
        },
        ipv4_forward_enumerate_all,
    },
    {
        ~0u
    }
};

const struct module ipv4_module =
{
    &NPI_MS_IPV4_MODULEID,
    ipv4_tables
};

static struct module_table ipv6_tables[] =
{
    {
        NSI_IP_COMPARTMENT_TABLE,
        {
            sizeof(UINT), sizeof(struct nsi_ip_cmpt_rw),
            sizeof(struct nsi_ip_cmpt_dynamic), 0
        },
        NULL,
        ipv6_cmpt_get_all_parameters,
    },
    {
        NSI_IP_ICMPSTATS_TABLE,
        {
            0, 0,
            sizeof(struct nsi_ip_icmpstats_dynamic), 0
        },
        NULL,
        ipv6_icmpstats_get_all_parameters,
    },
    {
        NSI_IP_IPSTATS_TABLE,
        {
            0, 0,
            sizeof(struct nsi_ip_ipstats_dynamic), sizeof(struct nsi_ip_ipstats_static)
        },
        NULL,
        ipv6_ipstats_get_all_parameters,
    },
    {
        NSI_IP_INTERFACE_TABLE,
        {
            sizeof(struct nsi_ip_interface_key), sizeof(struct nsi_ip_interface_rw),
            sizeof(struct nsi_ip_interface_dynamic), sizeof(struct nsi_ip_interface_static)
        },
        ipv6_interface_enumerate_all,
        ipv6_interface_get_all_parameters,
    },
    {
        NSI_IP_UNICAST_TABLE,
        {
            sizeof(struct nsi_ipv6_unicast_key), sizeof(struct nsi_ip_unicast_rw),
            sizeof(struct nsi_ip_unicast_dynamic), sizeof(struct nsi_ip_unicast_static)
        },
        ipv6_unicast_enumerate_all,
        ip_unicast_get_all_parameters,
    },
    {
        NSI_IP_NEIGHBOUR_TABLE,
        {
            sizeof(struct nsi_ipv6_neighbour_key), sizeof(struct nsi_ip_neighbour_rw),
            sizeof(struct nsi_ip_neighbour_dynamic), 0
        },
        ipv6_neighbour_enumerate_all,
    },
    {
        NSI_IP_FORWARD_TABLE,
        {
            sizeof(struct nsi_ipv6_forward_key), sizeof(struct nsi_ip_forward_rw),
            sizeof(struct nsi_ipv6_forward_dynamic), sizeof(struct nsi_ip_forward_static)
        },
        ipv6_forward_enumerate_all,
    },
    {
        ~0u
    }
};

const struct module ipv6_module =
{
    &NPI_MS_IPV6_MODULEID,
    ipv6_tables
};
