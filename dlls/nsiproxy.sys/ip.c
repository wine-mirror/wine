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
#include "config.h"
#include <stdarg.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif

#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_NETINET_IP_VAR_H
#include <netinet/ip_var.h>
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
#include "netiodef.h"
#include "wine/heap.h"
#include "wine/nsi.h"
#include "wine/debug.h"

#include "nsiproxy_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(nsi);

static inline DWORD nsi_popcount( DWORD m )
{
#ifdef HAVE___BUILTIN_POPCOUNT
    return __builtin_popcount( m );
#else
    m -= m >> 1 & 0x55555555;
    m = (m & 0x33333333) + (m >> 2 & 0x33333333);
    return ((m + (m >> 4)) & 0x0f0f0f0f) * 0x01010101 >> 24;
#endif
}

static DWORD mask_v4_to_prefix( struct in_addr *addr )
{
    return nsi_popcount( addr->s_addr );
}

static DWORD mask_v6_to_prefix( struct in6_addr *addr )
{
    DWORD ret;

    ret = nsi_popcount( *(DWORD *)addr->s6_addr );
    ret += nsi_popcount( *(DWORD *)(addr->s6_addr + 4) );
    ret += nsi_popcount( *(DWORD *)(addr->s6_addr + 8) );
    ret += nsi_popcount( *(DWORD *)(addr->s6_addr + 12) );
    return ret;
}

static ULONG64 get_boot_time( void )
{
    SYSTEM_TIMEOFDAY_INFORMATION ti;

    NtQuerySystemInformation( SystemTimeOfDayInformation, &ti, sizeof(ti), NULL );
    return ti.BootTime.QuadPart;
}

static NTSTATUS ipv4_ipstats_get_all_parameters( const void *key, DWORD key_size, void *rw_data, DWORD rw_size,
                                                 void *dynamic_data, DWORD dynamic_size, void *static_data, DWORD static_size )
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
            if (_strnicmp( buf, hdr, sizeof(hdr) - 1 )) continue;
            /* last line was a header, get another */
            if (!(ptr = fgets( buf, sizeof(buf), fp ))) break;
            if (!_strnicmp( buf, hdr, sizeof(hdr) - 1 ))
            {
                DWORD in_recv, in_hdr_errs, fwd_dgrams, in_delivers, out_reqs;
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

static NTSTATUS ipv6_ipstats_get_all_parameters( const void *key, DWORD key_size, void *rw_data, DWORD rw_size,
                                                 void *dynamic_data, DWORD dynamic_size, void *static_data, DWORD static_size )
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
        DWORD i;
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
                if (!_strnicmp( buf, ipstatlist[i].name, -1 ))
                {
                    if (ipstatlist[i].size == sizeof(long))
                        *(long *)ipstatlist[i].elem = strtoul( value, NULL, 10 );
                    else
                        *(long long *)ipstatlist[i].elem = strtoull( value, NULL, 10 );
                    status = STATUS_SUCCESS;
                }
        }
        fclose( fp );
        if (dynamic_data) *(struct nsi_ip_ipstats_dynamic *)dynamic_data = dyn;
        if (static_data) *(struct nsi_ip_ipstats_static *)static_data = stat;
        return status;
    }
#else
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
#endif
}

static void unicast_fill_entry( struct ifaddrs *entry, void *key, struct nsi_ip_unicast_rw *rw,
                                struct nsi_ip_unicast_dynamic *dyn, struct nsi_ip_unicast_static *stat )
{
    struct nsi_ipv6_unicast_key placeholder, *key6 = key;
    struct nsi_ipv4_unicast_key *key4 = key;
    DWORD scope_id = 0;

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

static NTSTATUS ip_unicast_enumerate_all( void *key_data, DWORD key_size, void *rw_data, DWORD rw_size,
                                          void *dynamic_data, DWORD dynamic_size,
                                          void *static_data, DWORD static_size, DWORD_PTR *count )
{
    DWORD num = 0;
    NTSTATUS status = STATUS_SUCCESS;
    BOOL want_data = key_size || rw_size || dynamic_size || static_size;
    struct ifaddrs *addrs, *entry;
    int family = (key_size == sizeof(struct nsi_ipv4_unicast_key)) ? AF_INET : AF_INET6;

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
    else status = STATUS_MORE_ENTRIES;

    return status;
}

static NTSTATUS ip_unicast_get_all_parameters( const void *key, DWORD key_size, void *rw_data, DWORD rw_size,
                                               void *dynamic_data, DWORD dynamic_size,
                                               void *static_data, DWORD static_size )
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
    DWORD if_index;
    struct in_addr addr;
    BYTE phys_addr[IF_MAX_PHYS_ADDRESS_LENGTH];
    DWORD state;
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

static NTSTATUS ipv4_neighbour_enumerate_all( void *key_data, DWORD key_size, void *rw_data, DWORD rw_size,
                                              void *dynamic_data, DWORD dynamic_size,
                                              void *static_data, DWORD static_size, DWORD_PTR *count )
{
    DWORD num = 0;
    NTSTATUS status = STATUS_SUCCESS;
    BOOL want_data = key_size || rw_size || dynamic_size || static_size;
    struct ipv4_neighbour_data entry;

    TRACE( "%p %d %p %d %p %d %p %d %p\n", key_data, key_size, rw_data, rw_size,
           dynamic_data, dynamic_size, static_data, static_size, count );

#ifdef __linux__
    {
        char buf[512], *ptr;
        DWORD atf_flags;
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

            if (!convert_unix_name_to_luid( ptr, &entry.luid )) continue;
            if (!convert_luid_to_index( &entry.luid, &entry.if_index )) continue;

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

        buf = heap_alloc( needed );
        if (!buf) return STATUS_NO_MEMORY;

        if (sysctl( mib, ARRAY_SIZE(mib), buf, &needed, NULL, 0 ) == -1)
        {
            heap_free( buf );
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
                entry.is_router = sinarp->sin_other & SIN_ROUTER;
                entry.is_unreachable = 0; /* FIXME */

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
        heap_free( buf );
    }
#else
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
#endif

    if (!want_data || num <= *count) *count = num;
    else status = STATUS_MORE_ENTRIES;

    return status;
}

static NTSTATUS ipv6_neighbour_enumerate_all( void *key_data, DWORD key_size, void *rw_data, DWORD rw_size,
                                              void *dynamic_data, DWORD dynamic_size,
                                              void *static_data, DWORD static_size, DWORD_PTR *count )
{
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
}

struct ipv4_route_data
{
    NET_LUID luid;
    DWORD if_index;
    struct in_addr prefix;
    DWORD prefix_len;
    struct in_addr next_hop;
    DWORD metric;
    DWORD protocol;
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

static NTSTATUS ipv4_forward_enumerate_all( void *key_data, DWORD key_size, void *rw_data, DWORD rw_size,
                                            void *dynamic_data, DWORD dynamic_size,
                                            void *static_data, DWORD static_size, DWORD_PTR *count )
{
    DWORD num = 0;
    NTSTATUS status = STATUS_SUCCESS;
    BOOL want_data = key_size || rw_size || dynamic_size || static_size;
    struct ipv4_route_data entry;

    TRACE( "%p %d %p %d %p %d %p %d %p\n", key_data, key_size, rw_data, rw_size,
           dynamic_data, dynamic_size, static_data, static_size, count );

#ifdef __linux__
    {
        char buf[512], *ptr;
        struct in_addr mask;
        DWORD rtf_flags;
        FILE *fp;

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

        buf = heap_alloc( needed );
        if (!buf) return STATUS_NO_MEMORY;

        if (sysctl( mib, 6, buf, &needed, NULL, 0 ) < 0)
        {
            heap_free( buf );
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
        HeapFree( GetProcessHeap (), 0, buf );
    }
#else
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
#endif

    if (!want_data || num <= *count) *count = num;
    else status = STATUS_MORE_ENTRIES;

    return status;
}

static NTSTATUS ipv6_forward_enumerate_all( void *key_data, DWORD key_size, void *rw_data, DWORD rw_size,
                                            void *dynamic_data, DWORD dynamic_size,
                                            void *static_data, DWORD static_size, DWORD_PTR *count )
{
    FIXME( "not implemented\n" );
    *count = 0;
    return STATUS_SUCCESS;
}

static struct module_table ipv4_tables[] =
{
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
        NSI_IP_UNICAST_TABLE,
        {
            sizeof(struct nsi_ipv4_unicast_key), sizeof(struct nsi_ip_unicast_rw),
            sizeof(struct nsi_ip_unicast_dynamic), sizeof(struct nsi_ip_unicast_static)
        },
        ip_unicast_enumerate_all,
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
        NSI_IP_IPSTATS_TABLE,
        {
            0, 0,
            sizeof(struct nsi_ip_ipstats_dynamic), sizeof(struct nsi_ip_ipstats_static)
        },
        NULL,
        ipv6_ipstats_get_all_parameters,
    },
    {
        NSI_IP_UNICAST_TABLE,
        {
            sizeof(struct nsi_ipv6_unicast_key), sizeof(struct nsi_ip_unicast_rw),
            sizeof(struct nsi_ip_unicast_dynamic), sizeof(struct nsi_ip_unicast_static)
        },
        ip_unicast_enumerate_all,
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
