/*
 * nsiproxy.sys ndis module
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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif

#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_NETINET_IF_ETHER_H
#include <netinet/if_ether.h>
#endif

#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif

#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif

#ifdef HAVE_NET_IF_TYPES_H
#include <net/if_types.h>
#endif

#define NONAMELESSUNION
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
#include "ddk/wdm.h"
#include "wine/nsi.h"
#include "wine/list.h"
#include "wine/heap.h"
#include "wine/debug.h"

#include "nsiproxy_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(nsi);

struct if_entry
{
    struct list entry;
    GUID if_guid;
    NET_LUID if_luid;
    WCHAR *if_name;
    char if_unix_name[IFNAMSIZ];
    IF_PHYSICAL_ADDRESS if_phys_addr;
    DWORD if_index;
    DWORD if_type;
};

static struct list if_list = LIST_INIT( if_list );

static CRITICAL_SECTION if_list_cs;
static CRITICAL_SECTION_DEBUG if_list_cs_debug =
{
    0, 0, &if_list_cs,
    { &if_list_cs_debug.ProcessLocksList, &if_list_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": if_list_cs") }
};
static CRITICAL_SECTION if_list_cs = { &if_list_cs_debug, -1, 0, 0, 0, 0 };

static struct if_entry *find_entry_from_index( DWORD index )
{
    struct if_entry *entry;

    LIST_FOR_EACH_ENTRY( entry, &if_list, struct if_entry, entry )
        if (entry->if_index == index) return entry;

    return NULL;
}

static struct if_entry *find_entry_from_luid( const NET_LUID *luid )
{
    struct if_entry *entry;

    LIST_FOR_EACH_ENTRY( entry, &if_list, struct if_entry, entry )
        if (entry->if_luid.Value == luid->Value) return entry;

    return NULL;
}

#if defined (SIOCGIFHWADDR) && defined (HAVE_STRUCT_IFREQ_IFR_HWADDR)
static NTSTATUS if_get_physical( const char *name, DWORD *type, IF_PHYSICAL_ADDRESS *phys_addr )
{
    int fd, size, i;
    struct ifreq ifr;
    NTSTATUS ret = STATUS_SUCCESS;
    static const struct type_lookup
    {
        unsigned short ifi_type;
        IFTYPE mib_type;
        DWORD addr_len;
    } types[] =
    {
        { ARPHRD_LOOPBACK, MIB_IF_TYPE_LOOPBACK, 0 },
        { ARPHRD_ETHER, MIB_IF_TYPE_ETHERNET, ETH_ALEN },
        { ARPHRD_FDDI, MIB_IF_TYPE_FDDI, ETH_ALEN },
        { ARPHRD_IEEE802, MIB_IF_TYPE_TOKENRING, ETH_ALEN },
        { ARPHRD_IEEE802_TR, MIB_IF_TYPE_TOKENRING, ETH_ALEN },
        { ARPHRD_SLIP, MIB_IF_TYPE_SLIP, 0 },
        { ARPHRD_PPP, MIB_IF_TYPE_PPP, 0 }
    };

    *type = MIB_IF_TYPE_OTHER;
    memset( phys_addr, 0, sizeof(*phys_addr) );

    size = strlen( name ) + 1;
    if (size > sizeof(ifr.ifr_name)) return STATUS_NAME_TOO_LONG;
    memset( &ifr, 0, sizeof(ifr) );
    memcpy( ifr.ifr_name, name, size );

    fd = socket( PF_INET, SOCK_DGRAM, 0 );
    if (fd == -1) return STATUS_TOO_MANY_OPENED_FILES;

    if (ioctl( fd, SIOCGIFHWADDR, &ifr ))
    {
        ret = STATUS_DEVICE_DATA_ERROR;
        goto err;
    }

    for (i = 0; i < ARRAY_SIZE(types); i++)
        if (ifr.ifr_hwaddr.sa_family == types[i].ifi_type)
        {
            *type = types[i].mib_type;
            phys_addr->Length = types[i].addr_len;
            memcpy( phys_addr->Address, ifr.ifr_hwaddr.sa_data, phys_addr->Length );
            break;
        }

err:
    close( fd );
    return ret;
}

#elif defined (HAVE_SYS_SYSCTL_H) && defined (HAVE_NET_IF_DL_H)

static NTSTATUS if_get_physical( const char *name, DWORD *type, IF_PHYSICAL_ADDRESS *phys_addr )
{
    struct if_msghdr *ifm;
    struct sockaddr_dl *sdl;
    u_char *p, *buf;
    size_t mib_len;
    int mib[] = { CTL_NET, AF_ROUTE, 0, AF_LINK, NET_RT_IFLIST, 0 }, i;
    static const struct type_lookup
    {
        u_char sdl_type;
        IFTYPE mib_type;
    } types[] =
    {
        { IFT_ETHER, MIB_IF_TYPE_ETHERNET },
        { IFT_FDDI, MIB_IF_TYPE_FDDI },
        { IFT_ISO88024, MIB_IF_TYPE_TOKENRING },
        { IFT_ISO88025, MIB_IF_TYPE_TOKENRING },
        { IFT_PPP, MIB_IF_TYPE_PPP },
        { IFT_SLIP, MIB_IF_TYPE_SLIP },
        { IFT_LOOP, MIB_IF_TYPE_LOOPBACK }
    };

    *type = MIB_IF_TYPE_OTHER;
    memset( phys_addr, 0, sizeof(*phys_addr) );

    if (sysctl( mib, 6, NULL, &mib_len, NULL, 0 ) < 0) return STATUS_TOO_MANY_OPENED_FILES;

    buf = malloc( mib_len );
    if (!buf) return STATUS_NO_MEMORY;

    if (sysctl( mib, 6, buf, &mib_len, NULL, 0 ) < 0)
    {
        free( buf );
        return STATUS_TOO_MANY_OPENED_FILES;
    }

    for (p = buf; p < buf + mib_len; p += ifm->ifm_msglen)
    {
        ifm = (struct if_msghdr *)p;
        sdl = (struct sockaddr_dl *)(ifm + 1);

        if (ifm->ifm_type != RTM_IFINFO || (ifm->ifm_addrs & RTA_IFP) == 0) continue;

        if (sdl->sdl_family != AF_LINK || sdl->sdl_nlen == 0 ||
            memcmp( sdl->sdl_data, name, max( sdl->sdl_nlen, strlen( name ) ) ))
            continue;

        for (i = 0; i < ARRAY_SIZE(types); i++)
            if (sdl->sdl_type == types[i].sdl_type)
            {
                *type = types[i].mib_type;
                break;
            }

        phys_addr->Length = sdl->sdl_alen;
        if (phys_addr->Length > sizeof(phys_addr->Address)) phys_addr->Length = 0;
        memcpy( phys_addr->Address, LLADDR(sdl), phys_addr->Length );
        break;
    }

    free( buf );
    return STATUS_SUCCESS;
}
#endif

static WCHAR *strdupAtoW( const char *str )
{
    WCHAR *ret = NULL;
    DWORD len;

    if (!str) return ret;
    len = MultiByteToWideChar( CP_UNIXCP, 0, str, -1, NULL, 0 );
    ret = heap_alloc( len * sizeof(WCHAR) );
    if (ret) MultiByteToWideChar( CP_UNIXCP, 0, str, -1, ret, len );
    return ret;
}

static struct if_entry *add_entry( DWORD index, char *name )
{
    struct if_entry *entry;
    int name_len = strlen( name );

    if (name_len >= IFNAMSIZ - 1) return NULL;
    entry = heap_alloc( sizeof(*entry) );
    if (!entry) return NULL;

    entry->if_index = index;
    memcpy( entry->if_unix_name, name, name_len + 1 );
    entry->if_name = strdupAtoW( name );
    if (!entry->if_name)
    {
        heap_free( entry );
        return NULL;
    }

    if_get_physical( name, &entry->if_type, &entry->if_phys_addr );

    entry->if_luid.Info.Reserved = 0;
    entry->if_luid.Info.NetLuidIndex = index;
    entry->if_luid.Info.IfType = entry->if_type;

    memset( &entry->if_guid, 0, sizeof(entry->if_guid) );
    entry->if_guid.Data1 = index;
    memcpy( entry->if_guid.Data4 + 2, "NetDev", 6 );

    list_add_tail( &if_list, &entry->entry );
    return entry;
}

static void update_if_table( void )
{
    struct if_nameindex *indices = if_nameindex(), *entry;

    for (entry = indices; entry->if_index; entry++)
    {
        if (!find_entry_from_index( entry->if_index ))
            add_entry( entry->if_index, entry->if_name );
    }

    if_freenameindex( indices );
}

static void if_counted_string_init( IF_COUNTED_STRING *str, const WCHAR *value )
{
    str->Length = value ? min( lstrlenW( value ), ARRAY_SIZE(str->String) - 1 ) * sizeof(WCHAR) : 0;
    if (str->Length) memcpy( str->String, value, str->Length );
    memset( (char *)str->String + str->Length, 0, sizeof(str->String) - str->Length );
}

static void ifinfo_fill_dynamic( struct if_entry *entry, struct nsi_ndis_ifinfo_dynamic *data )
{
    int fd, name_len = strlen( entry->if_unix_name );
    struct ifreq req;

    memset( data, 0, sizeof(*data) );

    if (name_len >= sizeof(req.ifr_name)) return;
    memcpy( req.ifr_name, entry->if_unix_name, name_len + 1 );

    fd = socket( PF_INET, SOCK_DGRAM, 0 );
    if (fd == -1) return;

    if (!ioctl( fd, SIOCGIFFLAGS, &req ))
    {
        if (req.ifr_flags & IFF_UP) data->oper_status = IfOperStatusUp;
#ifdef IFF_DORMANT
        else if (req.ifr_flags & IFF_DORMANT) data->oper_status = IfOperStatusDormant;
#endif
        else data->oper_status = IfOperStatusDown;
    } else data->oper_status = IfOperStatusUnknown;

    data->flags.unk = 0;
    data->flags.not_media_conn = 0;
    data->flags.unk2 = 0;
    data->media_conn_state = MediaConnectStateConnected;
    data->unk = 0;

    if (!ioctl( fd, SIOCGIFMTU, &req )) data->mtu = req.ifr_mtu;
    else data->mtu = 0;

    close( fd );

#ifdef __linux__
    {
        FILE *fp;

        if ((fp = fopen( "/proc/net/dev", "r" )))
        {
            char buf[512], *ptr;

            while ((ptr = fgets( buf, sizeof(buf), fp )))
            {
                while (*ptr && isspace( *ptr )) ptr++;
                if (!_strnicmp( ptr, entry->if_unix_name, name_len ) && ptr[name_len] == ':')
                {
                    unsigned long long values[9];
                    ptr += name_len + 1;
                    sscanf( ptr, "%llu %llu %llu %llu %*u %*u %*u %llu %llu %llu %llu %llu",
                            values, values + 1, values + 2, values + 3, values + 4,
                            values + 5, values + 6, values + 7, values + 8 );
                    data->in_octets      = values[0];
                    data->in_ucast_pkts  = values[1];
                    data->in_errors      = values[2];
                    data->in_discards    = values[3];
                    data->in_mcast_pkts  = values[4];
                    data->out_octets     = values[5];
                    data->out_ucast_pkts = values[6];
                    data->out_errors     = values[7];
                    data->out_discards   = values[8];
                    break;
                }
            }
            fclose( fp );
        }
    }
#elif defined(HAVE_SYS_SYSCTL_H) && defined(NET_RT_IFLIST)
    {
        int mib[] = { CTL_NET, PF_ROUTE, 0, AF_INET, NET_RT_IFLIST, entry->if_index };
        size_t needed;
        char *buf = NULL, *end;
        struct if_msghdr *ifm;
        struct if_data ifdata;

        if (sysctl( mib, ARRAY_SIZE(mib), NULL, &needed, NULL, 0 ) == -1) goto done;
        buf = heap_alloc( needed );
        if (!buf) goto done;
        if (sysctl( mib, ARRAY_SIZE(mib), buf, &needed, NULL, 0 ) == -1) goto done;
        for (end = buf + needed; buf < end; buf += ifm->ifm_msglen)
        {
            ifm = (struct if_msghdr *) buf;
            if (ifm->ifm_type == RTM_IFINFO)
            {
                ifdata = ifm->ifm_data;
                data->xmit_speed = data->rcv_speed = ifdata.ifi_baudrate;
                data->in_octets = ifdata.ifi_ibytes;
                data->in_errors = ifdata.ifi_ierrors;
                data->in_discards = ifdata.ifi_iqdrops;
                data->in_ucast_pkts = ifdata.ifi_ipackets;
                data->in_mcast_pkts = ifdata.ifi_imcasts;
                data->out_octets = ifdata.ifi_obytes;
                data->out_ucast_pkts = ifdata.ifi_opackets;
                data->out_mcast_pkts = ifdata.ifi_omcasts;
                data->out_errors = ifdata.ifi_oerrors;
                break;
            }
        }
    done:
        heap_free( buf );
    }
#endif
}

static void ifinfo_fill_entry( struct if_entry *entry, NET_LUID *key, struct nsi_ndis_ifinfo_rw *rw,
                               struct nsi_ndis_ifinfo_dynamic *dyn, struct nsi_ndis_ifinfo_static *stat )
{
    if (key) memcpy( key, &entry->if_luid, sizeof(entry->if_luid) );

    if (rw)
    {
        memset( &rw->network_guid, 0, sizeof(entry->if_guid) );
        rw->admin_status = MIB_IF_ADMIN_STATUS_UP;
        if_counted_string_init( &rw->alias, entry->if_name );
        memcpy( &rw->phys_addr, &entry->if_phys_addr, sizeof(entry->if_phys_addr) );
        rw->pad = 0;
        if_counted_string_init( &rw->name2, NULL );
        rw->unk = 0;
    }

    if (dyn) ifinfo_fill_dynamic( entry, dyn );

    if (stat)
    {
        stat->if_index = entry->if_index;
        if_counted_string_init( &stat->descr, entry->if_name ); /* get a more descriptive name */
        stat->type = entry->if_type;
        stat->access_type = (entry->if_type == MIB_IF_TYPE_LOOPBACK) ? NET_IF_ACCESS_LOOPBACK : NET_IF_ACCESS_BROADCAST;
        stat->unk = 0;
        stat->conn_type = NET_IF_CONNECTION_DEDICATED;
        memcpy( &stat->if_guid, &entry->if_guid, sizeof(entry->if_guid) );
        stat->conn_present = entry->if_type != MIB_IF_TYPE_LOOPBACK;
        memcpy( &stat->perm_phys_addr, &entry->if_phys_addr, sizeof(entry->if_phys_addr) );
        stat->flags.hw = entry->if_type != MIB_IF_TYPE_LOOPBACK;
        stat->flags.filter = 0;
        stat->flags.unk = 0;
        stat->media_type = 0;
        stat->phys_medium_type = 0;
    }
}

static NTSTATUS ifinfo_enumerate_all( void *key_data, DWORD key_size, void *rw_data, DWORD rw_size,
                                      void *dynamic_data, DWORD dynamic_size,
                                      void *static_data, DWORD static_size, DWORD_PTR *count )
{
    struct if_entry *entry;
    DWORD num = 0;
    NTSTATUS status = STATUS_SUCCESS;
    BOOL want_data = key_size || rw_size || dynamic_size || static_size;

    TRACE( "%p %d %p %d %p %d %p %d %p\n", key_data, key_size, rw_data, rw_size,
           dynamic_data, dynamic_size, static_data, static_size, count );

    EnterCriticalSection( &if_list_cs );

    update_if_table();

    LIST_FOR_EACH_ENTRY( entry, &if_list, struct if_entry, entry )
    {
        if (num < *count)
        {
            ifinfo_fill_entry( entry, key_data, rw_data, dynamic_data, static_data );
            key_data = (BYTE *)key_data + key_size;
            rw_data = (BYTE *)rw_data + rw_size;
            dynamic_data = (BYTE *)dynamic_data + dynamic_size;
            static_data = (BYTE *)static_data + static_size;
        }
        num++;
    }

    LeaveCriticalSection( &if_list_cs );

    if (!want_data || num <= *count) *count = num;
    else status = STATUS_MORE_ENTRIES;

    return status;
}

static NTSTATUS ifinfo_get_all_parameters( const void *key, DWORD key_size, void *rw_data, DWORD rw_size,
                                           void *dynamic_data, DWORD dynamic_size,
                                           void *static_data, DWORD static_size )
{
    struct if_entry *entry;
    NTSTATUS status = STATUS_OBJECT_NAME_NOT_FOUND;

    TRACE( "%p %d %p %d %p %d %p %d\n", key, key_size, rw_data, rw_size,
           dynamic_data, dynamic_size, static_data, static_size );

    EnterCriticalSection( &if_list_cs );

    update_if_table();

    entry = find_entry_from_luid( (const NET_LUID *)key );
    if (entry)
    {
        ifinfo_fill_entry( entry, NULL, rw_data, dynamic_data, static_data );
        status = STATUS_SUCCESS;
    }

    LeaveCriticalSection( &if_list_cs );

    return status;
}

static NTSTATUS ifinfo_get_rw_parameter( struct if_entry *entry, void *data, DWORD data_size, DWORD data_offset )
{
    switch (data_offset)
    {
    case FIELD_OFFSET( struct nsi_ndis_ifinfo_rw, alias ):
    {
        IF_COUNTED_STRING *str = (IF_COUNTED_STRING *)data;
        if (data_size != sizeof(*str)) return STATUS_INVALID_PARAMETER;
        if_counted_string_init( str, entry->if_name );
        return STATUS_SUCCESS;
    }
    default:
        FIXME( "Offset %#x not handled\n", data_offset );
    }

    return STATUS_INVALID_PARAMETER;
}

static NTSTATUS ifinfo_get_static_parameter( struct if_entry *entry, void *data, DWORD data_size, DWORD data_offset )
{
    switch (data_offset)
    {
    case FIELD_OFFSET( struct nsi_ndis_ifinfo_static, if_index ):
        if (data_size != sizeof(DWORD)) return STATUS_INVALID_PARAMETER;
        *(DWORD *)data = entry->if_index;
        return STATUS_SUCCESS;

    case FIELD_OFFSET( struct nsi_ndis_ifinfo_static, if_guid ):
        if (data_size != sizeof(GUID)) return STATUS_INVALID_PARAMETER;
        *(GUID *)data = entry->if_guid;
        return STATUS_SUCCESS;

    default:
        FIXME( "Offset %#x not handled\n", data_offset );
    }
    return STATUS_INVALID_PARAMETER;
}

static NTSTATUS ifinfo_get_parameter( const void *key, DWORD key_size, DWORD param_type,
                                      void *data, DWORD data_size, DWORD data_offset )
{
    struct if_entry *entry;
    NTSTATUS status = STATUS_OBJECT_NAME_NOT_FOUND;

    TRACE( "%p %d %d %p %d %d\n", key, key_size, param_type, data, data_size, data_offset );

    EnterCriticalSection( &if_list_cs );

    update_if_table();

    entry = find_entry_from_luid( (const NET_LUID *)key );
    if (entry)
    {
        switch (param_type)
        {
        case NSI_PARAM_TYPE_RW:
            status = ifinfo_get_rw_parameter( entry, data, data_size, data_offset );
            break;
        case NSI_PARAM_TYPE_STATIC:
            status = ifinfo_get_static_parameter( entry, data, data_size, data_offset );
            break;
        }
    }

    LeaveCriticalSection( &if_list_cs );

    return status;
}

static NTSTATUS index_luid_get_parameter( const void *key, DWORD key_size, DWORD param_type,
                                          void *data, DWORD data_size, DWORD data_offset )
{
    struct if_entry *entry;
    NTSTATUS status = STATUS_OBJECT_NAME_NOT_FOUND;

    TRACE( "%p %d %d %p %d %d\n", key, key_size, param_type, data, data_size, data_offset );

    if (param_type != NSI_PARAM_TYPE_STATIC || data_size != sizeof(NET_LUID) || data_offset != 0)
        return STATUS_INVALID_PARAMETER;

    EnterCriticalSection( &if_list_cs );

    update_if_table();

    entry = find_entry_from_index( *(DWORD *)key );
    if (entry)
    {
        *(NET_LUID *)data = entry->if_luid;
        status = STATUS_SUCCESS;
    }
    LeaveCriticalSection( &if_list_cs );
    return status;
}

BOOL convert_unix_name_to_luid( const char *unix_name, NET_LUID *luid )
{
    struct if_entry *entry;
    BOOL ret = FALSE;

    EnterCriticalSection( &if_list_cs );

    update_if_table();

    LIST_FOR_EACH_ENTRY( entry, &if_list, struct if_entry, entry )
        if (!strcmp( entry->if_unix_name, unix_name ))
        {
            *luid = entry->if_luid;
            ret = TRUE;
            break;
        }
    LeaveCriticalSection( &if_list_cs );

    return ret;
}

BOOL convert_luid_to_unix_name( const NET_LUID *luid, const char **unix_name )
{
    struct if_entry *entry;
    BOOL ret = FALSE;

    EnterCriticalSection( &if_list_cs );

    update_if_table();

    LIST_FOR_EACH_ENTRY( entry, &if_list, struct if_entry, entry )
        if (entry->if_luid.Value == luid->Value)
        {
            *unix_name = entry->if_unix_name;
            ret = TRUE;

        }
    LeaveCriticalSection( &if_list_cs );

    return ret;
}

static const struct module_table tables[] =
{
    {
        NSI_NDIS_IFINFO_TABLE,
        {
            sizeof(NET_LUID), sizeof(struct nsi_ndis_ifinfo_rw),
            sizeof(struct nsi_ndis_ifinfo_dynamic), sizeof(struct nsi_ndis_ifinfo_static)
        },
        ifinfo_enumerate_all,
        ifinfo_get_all_parameters,
        ifinfo_get_parameter
    },
    {
        NSI_NDIS_INDEX_LUID_TABLE,
        {
            sizeof(DWORD), 0,
            0, sizeof(NET_LUID)
        },
        NULL,
        NULL,
        index_luid_get_parameter
    },
    { ~0u }
};

const struct module ndis_module =
{
    &NPI_MS_NDIS_MODULEID,
    tables
};
