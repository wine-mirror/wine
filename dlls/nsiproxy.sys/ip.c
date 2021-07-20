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

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
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

static struct module_table ipv4_tables[] =
{
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
        NSI_IP_UNICAST_TABLE,
        {
            sizeof(struct nsi_ipv6_unicast_key), sizeof(struct nsi_ip_unicast_rw),
            sizeof(struct nsi_ip_unicast_dynamic), sizeof(struct nsi_ip_unicast_static)
        },
        ip_unicast_enumerate_all,
        ip_unicast_get_all_parameters,
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
