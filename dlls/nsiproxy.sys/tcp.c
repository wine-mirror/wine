/*
 * nsiproxy.sys tcp module
 *
 * Copyright 2003, 2006, 2011 Juan Lang
 * Copyright 2007 TransGaming Technologies Inc.
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
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_SYS_SOCKETVAR_H
#include <sys/socketvar.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_NETINET_IP_VAR_H
#include <netinet/ip_var.h>
#endif

#ifdef HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif

#ifdef HAVE_NETINET_TCP_VAR_H
#include <netinet/tcp_var.h>
#endif

#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif

#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif

#ifdef HAVE_LIBPROCSTAT_H
#include <libprocstat.h>
#endif

#ifdef HAVE_LIBPROC_H
#include <libproc.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#define USE_WS_PREFIX
#include "winsock2.h"
#include "ifdef.h"
#include "netiodef.h"
#include "ws2ipdef.h"
#include "tcpmib.h"
#include "wine/nsi.h"
#include "wine/debug.h"
#include "wine/server.h"

#include "unix_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(nsi);

static NTSTATUS tcp_stats_get_all_parameters( const void *key, UINT key_size, void *rw_data, UINT rw_size,
                                              void *dynamic_data, UINT dynamic_size, void *static_data, UINT static_size )
{
    struct nsi_tcp_stats_dynamic dyn;
    struct nsi_tcp_stats_static stat;
    const USHORT *family = key;

    TRACE( "%p %d %p %d %p %d %p %d\n", key, key_size, rw_data, rw_size, dynamic_data, dynamic_size,
           static_data, static_size );

    if (*family != WS_AF_INET && *family != WS_AF_INET6) return STATUS_NOT_SUPPORTED;

    memset( &dyn, 0, sizeof(dyn) );
    memset( &stat, 0, sizeof(stat) );

#ifdef __linux__
    {
        NTSTATUS status = STATUS_NOT_SUPPORTED;
        static const char hdr[] = "Tcp:";
        char buf[512], *ptr;
        FILE *fp;

        /* linux merges tcp4 and tcp6 stats, so simply supply that for either family */
        if (!(fp = fopen( "/proc/net/snmp", "r" ))) return STATUS_NOT_SUPPORTED;

        while ((ptr = fgets( buf, sizeof(buf), fp )))
        {
            if (ascii_strncasecmp( buf, hdr, sizeof(hdr) - 1 )) continue;
            /* last line was a header, get another */
            if (!(ptr = fgets( buf, sizeof(buf), fp ))) break;
            if (!ascii_strncasecmp( buf, hdr, sizeof(hdr) - 1 ))
            {
                UINT in_segs, out_segs;
                ptr += sizeof(hdr);
                sscanf( ptr, "%u %u %u %u %u %u %u %u %u %u %u %u %u %u",
                        &stat.rto_algo,
                        &stat.rto_min,
                        &stat.rto_max,
                        &stat.max_conns,
                        &dyn.active_opens,
                        &dyn.passive_opens,
                        &dyn.attempt_fails,
                        &dyn.est_rsts,
                        &dyn.cur_est,
                        &in_segs,
                        &out_segs,
                        &dyn.retrans_segs,
                        &dyn.in_errs,
                        &dyn.out_rsts );
                dyn.in_segs = in_segs;
                dyn.out_segs = out_segs;
                if (dynamic_data) *(struct nsi_tcp_stats_dynamic *)dynamic_data = dyn;
                if (static_data) *(struct nsi_tcp_stats_static *)static_data = stat;
                status = STATUS_SUCCESS;
                break;
            }
        }
        fclose( fp );
        return status;
    }
#elif defined(HAVE_SYS_SYSCTL_H) && defined(TCPCTL_STATS) && (defined(HAVE_STRUCT_TCPSTAT_TCPS_CONNATTEMPT) || defined(HAVE_STRUCT_TCP_STATS_TCPS_CONNATTEMPT))
    {
#ifndef TCPTV_MIN  /* got removed in Mac OS X for some reason */
#define TCPTV_MIN 2
#define TCPTV_REXMTMAX 128
#endif
        int mib[] = { CTL_NET, PF_INET, IPPROTO_TCP, TCPCTL_STATS };
#define hz 1000
#if defined(HAVE_STRUCT_TCPSTAT_TCPS_CONNATTEMPT)
        struct tcpstat tcp_stat;
#elif defined(HAVE_STRUCT_TCP_STATS_TCPS_CONNATTEMPT)
        struct tcp_stats tcp_stat;
#endif
        size_t needed = sizeof(tcp_stat);

        if (sysctl( mib, ARRAY_SIZE(mib), &tcp_stat, &needed, NULL, 0 ) == -1) return STATUS_NOT_SUPPORTED;

        stat.rto_algo = MIB_TCP_RTO_VANJ;
        stat.rto_min = TCPTV_MIN;
        stat.rto_max = TCPTV_REXMTMAX;
        stat.max_conns = -1;
        dyn.active_opens = tcp_stat.tcps_connattempt;
        dyn.passive_opens = tcp_stat.tcps_accepts;
        dyn.attempt_fails = tcp_stat.tcps_conndrops;
        dyn.est_rsts = tcp_stat.tcps_drops;
        dyn.cur_est = 0;
        dyn.pad = 0;
        dyn.in_segs = tcp_stat.tcps_rcvtotal;
        dyn.out_segs = tcp_stat.tcps_sndtotal - tcp_stat.tcps_sndrexmitpack;
        dyn.retrans_segs = tcp_stat.tcps_sndrexmitpack;
        dyn.out_rsts = tcp_stat.tcps_sndctrl - tcp_stat.tcps_closed;
        dyn.in_errs = tcp_stat.tcps_rcvbadsum + tcp_stat.tcps_rcvbadoff + tcp_stat.tcps_rcvmemdrop + tcp_stat.tcps_rcvshort;
        dyn.num_conns = tcp_stat.tcps_connects;
        if (dynamic_data) *(struct nsi_tcp_stats_dynamic *)dynamic_data = dyn;
        if (static_data) *(struct nsi_tcp_stats_static *)static_data = stat;
        return STATUS_SUCCESS;
    }
#else
    FIXME( "not implemented\n" );
    return STATUS_NOT_IMPLEMENTED;
#endif
}

struct ipv6_addr_scope *get_ipv6_addr_scope_table( unsigned int *size )
{
    struct ipv6_addr_scope *table = NULL, *new_table;
    unsigned int table_size = 0, num = 0;

#ifdef __linux__
    {
        char buf[512], *ptr;
        FILE *fp;

        if (!(fp = fopen( "/proc/net/if_inet6", "r" ))) goto failed;

        while ((ptr = fgets( buf, sizeof(buf), fp )))
        {
            WORD a[8];
            UINT scope;
            struct ipv6_addr_scope *entry;
            unsigned int i;

            if (sscanf( ptr, "%4hx%4hx%4hx%4hx%4hx%4hx%4hx%4hx %*s %*s %x",
                        a, a + 1, a + 2, a + 3, a + 4, a + 5, a + 6, a + 7, &scope ) != 9)
                continue;

            if (++num > table_size)
            {
                if (!table_size) table_size = 4;
                else table_size *= 2;
                if (!(new_table = realloc( table, table_size * sizeof(table[0]) )))
                {
                    fclose( fp );
                    goto failed;
                }
                table = new_table;
            }

            entry = table + num - 1;
            for (i = 0; i < 8; i++)
                entry->addr.u.Word[i] = htons( a[i] );
            entry->scope = htons( scope );
        }

        fclose( fp );
    }
#elif defined(HAVE_GETIFADDRS)
    {
        struct ifaddrs *addrs, *cur;

        if (getifaddrs( &addrs ) == -1)  goto failed;

        for (cur = addrs; cur; cur = cur->ifa_next)
        {
            struct sockaddr_in6 *sin6;
            struct ipv6_addr_scope *entry;

            if (cur->ifa_addr->sa_family != AF_INET6) continue;

            if (++num > table_size)
            {
                if (!table_size) table_size = 4;
                else table_size *= 2;
                if (!(new_table = realloc( table, table_size * sizeof(table[0]) )))
                {
                    freeifaddrs( addrs );
                    goto failed;
                }
                table = new_table;
            }

            sin6 = (struct sockaddr_in6 *)cur->ifa_addr;
            entry = table + num - 1;
            memcpy( &entry->addr, &sin6->sin6_addr, sizeof(entry->addr) );
            entry->scope = sin6->sin6_scope_id;
        }

        freeifaddrs( addrs );
    }
#else
    FIXME( "not implemented\n" );
    goto failed;
#endif

    *size = num;
    return table;

failed:
    free( table );
    return NULL;
}

UINT find_ipv6_addr_scope( const IN6_ADDR *addr, const struct ipv6_addr_scope *table, unsigned int size )
{
    const BYTE multicast_scope_mask = 0x0F;
    const BYTE multicast_scope_shift = 0;
    unsigned int i;

    if (WS_IN6_IS_ADDR_UNSPECIFIED( addr )) return 0;

    if (WS_IN6_IS_ADDR_MULTICAST( addr ))
        return htons( (addr->u.Byte[1] & multicast_scope_mask) >> multicast_scope_shift );

    if (!table) return -1;

    for (i = 0; i < size; i++)
        if (!memcmp( &table[i].addr, addr, sizeof(table[i].addr) ))
            return table[i].scope;

    return -1;
}

struct pid_map *get_pid_map( unsigned int *num_entries )
{
    struct pid_map *map;
    unsigned int i = 0, buffer_len = 4096, process_count, pos = 0;
    NTSTATUS ret;
    char *buffer = NULL, *new_buffer;

    if (!(buffer = malloc( buffer_len ))) return NULL;

    for (;;)
    {
        SERVER_START_REQ( list_processes )
        {
            wine_server_set_reply( req, buffer, buffer_len );
            ret = wine_server_call( req );
            buffer_len = reply->info_size;
            process_count = reply->process_count;
        }
        SERVER_END_REQ;

        if (ret != STATUS_INFO_LENGTH_MISMATCH) break;

        if (!(new_buffer = realloc( buffer, buffer_len )))
        {
            free( buffer );
            return NULL;
        }
        buffer = new_buffer;
    }

    if (!(map = malloc( process_count * sizeof(*map) )))
    {
        free( buffer );
        return NULL;
    }

    for (i = 0; i < process_count; ++i)
    {
        const struct process_info *process;

        pos = (pos + 7) & ~7;
        process = (const struct process_info *)(buffer + pos);

        map[i].pid = process->pid;
        map[i].unix_pid = process->unix_pid;

        pos += sizeof(struct process_info) + process->name_len;
        pos = (pos + 7) & ~7;
        pos += process->thread_count * sizeof(struct thread_info);
    }

    free( buffer );
    *num_entries = process_count;
    return map;
}

unsigned int find_owning_pid( struct pid_map *map, unsigned int num_entries, UINT_PTR inode )
{
#ifdef __linux__
    unsigned int i, len_socket;
    char socket[32];

    sprintf( socket, "socket:[%zu]", inode );
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
                if ((len = readlink( link, name, sizeof(name) - 1 )) > 0) name[len] = 0;
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
#elif defined(HAVE_LIBPROCSTAT)
    struct procstat *pstat;
    struct kinfo_proc *proc;
    struct filestat_list *fds;
    struct filestat *fd;
    struct sockstat sock;
    unsigned int i, proc_count;

    pstat = procstat_open_sysctl();
    if (!pstat) return 0;

    for (i = 0; i < num_entries; i++)
    {
        proc = procstat_getprocs( pstat, KERN_PROC_PID, map[i].unix_pid, &proc_count );
        if (!proc || proc_count < 1) continue;

        fds = procstat_getfiles( pstat, proc, 0 );
        if (!fds)
        {
            procstat_freeprocs( pstat, proc );
            continue;
        }

        STAILQ_FOREACH( fd, fds, next )
        {
            char errbuf[_POSIX2_LINE_MAX];

            if (fd->fs_type != PS_FST_TYPE_SOCKET) continue;

            procstat_get_socket_info( pstat, fd, &sock, errbuf );

            if (sock.so_pcb == inode)
            {
                procstat_freefiles( pstat, fds );
                procstat_freeprocs( pstat, proc );
                procstat_close( pstat );
                return map[i].pid;
            }
        }

        procstat_freefiles( pstat, fds );
        procstat_freeprocs( pstat, proc );
    }

    procstat_close( pstat );
    return 0;
#elif defined(HAVE_PROC_PIDINFO)
    struct proc_fdinfo *fds;
    struct socket_fdinfo sock;
    unsigned int i, j, n;

    for (i = 0; i < num_entries; i++)
    {
        int fd_len = proc_pidinfo( map[i].unix_pid, PROC_PIDLISTFDS, 0, NULL, 0 );
        if (fd_len <= 0) continue;

        fds = malloc( fd_len );
        if (!fds) continue;

        proc_pidinfo( map[i].unix_pid, PROC_PIDLISTFDS, 0, fds, fd_len );
        n = fd_len / sizeof(struct proc_fdinfo);
        for (j = 0; j < n; j++)
        {
            if (fds[j].proc_fdtype != PROX_FDTYPE_SOCKET) continue;

            proc_pidfdinfo( map[i].unix_pid, fds[j].proc_fd, PROC_PIDFDSOCKETINFO, &sock, sizeof(sock) );
            if (sock.psi.soi_pcb == inode)
            {
                free( fds );
                return map[i].pid;
            }
        }

        free( fds );
    }
    return 0;
#else
    FIXME( "not implemented\n" );
    return 0;
#endif
}

static NTSTATUS tcp_conns_enumerate_all( UINT filter, struct nsi_tcp_conn_key *key_data, UINT key_size,
                                         void *rw, UINT rw_size,
                                         struct nsi_tcp_conn_dynamic *dynamic_data, UINT dynamic_size,
                                         struct nsi_tcp_conn_static *static_data, UINT static_size, UINT_PTR *count )
{
    BOOL want_data = key_size || rw_size || dynamic_size || static_size;
    struct nsi_tcp_conn_key key;
    struct nsi_tcp_conn_dynamic dyn;
    struct nsi_tcp_conn_static stat;
    struct ipv6_addr_scope *addr_scopes = NULL;
    unsigned int addr_scopes_size = 0;
    NTSTATUS ret = STATUS_SUCCESS;
    tcp_connection *connections = NULL;

    if (want_data)
    {
        connections = malloc( sizeof(*connections) * (*count) );
        if (!connections) return STATUS_NO_MEMORY;
    }

    SERVER_START_REQ( get_tcp_connections )
    {
        req->state_filter = filter;
        wine_server_set_reply( req, connections, want_data ? (sizeof(*connections) * (*count)) : 0 );
        if (!(ret = wine_server_call( req )))
            *count = reply->count;
        else if (ret == STATUS_BUFFER_TOO_SMALL)
        {
            *count = reply->count;
            if (want_data)
            {
                free( connections );
                return STATUS_BUFFER_OVERFLOW;
            }
            return STATUS_SUCCESS;
        }
    }
    SERVER_END_REQ;

    addr_scopes = get_ipv6_addr_scope_table( &addr_scopes_size );

    for (unsigned int i = 0; i < *count; i++)
    {
        tcp_connection *conn = &connections[i];

        if (key_data)
        {
            memset( &key, 0, sizeof(key) );
            if (conn->common.family == WS_AF_INET)
            {
                key.local.Ipv4.sin_family = key.remote.Ipv4.sin_family = WS_AF_INET;
                key.local.Ipv4.sin_addr.WS_s_addr = conn->ipv4.local_addr;
                key.local.Ipv4.sin_port = conn->ipv4.local_port;
                key.remote.Ipv4.sin_addr.WS_s_addr = conn->ipv4.remote_addr;
                key.remote.Ipv4.sin_port = conn->ipv4.remote_port;
            }
            else
            {
                key.local.Ipv6.sin6_family = key.remote.Ipv6.sin6_family = WS_AF_INET6;
                memcpy( &key.local.Ipv6.sin6_addr, &conn->ipv6.local_addr, 16 );
                key.local.Ipv6.sin6_port = conn->ipv6.local_port;
                key.local.Ipv6.sin6_scope_id = find_ipv6_addr_scope( &key.local.Ipv6.sin6_addr, addr_scopes,
                                                                     addr_scopes_size );
                memcpy( &key.remote.Ipv6.sin6_addr, &conn->ipv6.remote_addr, 16 );
                key.remote.Ipv6.sin6_port = conn->ipv6.remote_port;
                key.remote.Ipv6.sin6_scope_id = find_ipv6_addr_scope( &key.remote.Ipv6.sin6_addr, addr_scopes,
                                                                      addr_scopes_size );
            }
            *key_data++ = key;
        }

        if (dynamic_data)
        {
            memset( &dyn, 0, sizeof(dyn) );
            dyn.state = conn->common.state;
            *dynamic_data++ = dyn;
        }

        if (static_data)
        {
            memset( &stat, 0, sizeof(stat) );
            stat.pid = conn->common.owner;
            stat.create_time = 0; /* FIXME */
            stat.mod_info = 0; /* FIXME */
            *static_data++ = stat;
        }
    }

    free( connections );
    return STATUS_SUCCESS;
}

static NTSTATUS tcp_all_enumerate_all( void *key_data, UINT key_size, void *rw_data, UINT rw_size,
                                       void *dynamic_data, UINT dynamic_size,
                                       void *static_data, UINT static_size, UINT_PTR *count )
{
    TRACE( "%p %d %p %d %p %d %p %d %p\n", key_data, key_size, rw_data, rw_size,
           dynamic_data, dynamic_size, static_data, static_size, count );

    return tcp_conns_enumerate_all( 0, key_data, key_size, rw_data, rw_size,
                                    dynamic_data, dynamic_size, static_data, static_size, count );
}

static NTSTATUS tcp_estab_enumerate_all( void *key_data, UINT key_size, void *rw_data, UINT rw_size,
                                         void *dynamic_data, UINT dynamic_size,
                                         void *static_data, UINT static_size, UINT_PTR *count )
{
    TRACE( "%p %d %p %d %p %d %p %d %p\n", key_data, key_size, rw_data, rw_size,
           dynamic_data, dynamic_size, static_data, static_size, count );

    return tcp_conns_enumerate_all( MIB_TCP_STATE_ESTAB, key_data, key_size, rw_data, rw_size,
                                    dynamic_data, dynamic_size, static_data, static_size, count );
}

static NTSTATUS tcp_listen_enumerate_all( void *key_data, UINT key_size, void *rw_data, UINT rw_size,
                                          void *dynamic_data, UINT dynamic_size,
                                          void *static_data, UINT static_size, UINT_PTR *count )
{
    TRACE( "%p %d %p %d %p %d %p %d %p\n", key_data, key_size, rw_data, rw_size,
           dynamic_data, dynamic_size, static_data, static_size, count );

    return tcp_conns_enumerate_all( MIB_TCP_STATE_LISTEN, key_data, key_size, rw_data, rw_size,
                                    dynamic_data, dynamic_size, static_data, static_size, count );
}

static struct module_table tcp_tables[] =
{
    {
        NSI_TCP_STATS_TABLE,
        {
            sizeof(USHORT), 0,
            sizeof(struct nsi_tcp_stats_dynamic), sizeof(struct nsi_tcp_stats_static)
        },
        NULL,
        tcp_stats_get_all_parameters,
    },
    {
        NSI_TCP_ALL_TABLE,
        {
            sizeof(struct nsi_tcp_conn_key), 0,
            sizeof(struct nsi_tcp_conn_dynamic), sizeof(struct nsi_tcp_conn_static)
        },
        tcp_all_enumerate_all,
    },
    {
        NSI_TCP_ESTAB_TABLE,
        {
            sizeof(struct nsi_tcp_conn_key), 0,
            sizeof(struct nsi_tcp_conn_dynamic), sizeof(struct nsi_tcp_conn_static)
        },
        tcp_estab_enumerate_all,
    },
    {
        NSI_TCP_LISTEN_TABLE,
        {
            sizeof(struct nsi_tcp_conn_key), 0,
            sizeof(struct nsi_tcp_conn_dynamic), sizeof(struct nsi_tcp_conn_static)
        },
        tcp_listen_enumerate_all,
    },
    {
        ~0u
    }
};

const struct module tcp_module =
{
    &NPI_MS_TCP_MODULEID,
    tcp_tables
};
