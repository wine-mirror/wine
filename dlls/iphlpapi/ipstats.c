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
#include "ipstats.h"
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

static void *append_table_row( HANDLE heap, DWORD flags, void *table, DWORD *table_size, DWORD *table_capacity,
                               const void *row, DWORD row_size )
{
    DWORD *num_entries = table; /* this must be the first field */
    if (*num_entries == *table_capacity)
    {
        void *new_table;
        *table_size += *table_capacity * row_size;
        if (!(new_table = HeapReAlloc( heap, flags, table, *table_size )))
        {
            HeapFree( heap, 0, table );
            return NULL;
        }
        num_entries = table = new_table;
        *table_capacity *= 2;
    }
    memcpy( (char *)table + *table_size - (*table_capacity - *num_entries) * row_size, row, row_size );
    (*num_entries)++;
    return table;
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
    case TCP_TABLE_OWNER_MODULE_LISTENER:
    case TCP_TABLE_OWNER_MODULE_CONNECTIONS:
    case TCP_TABLE_OWNER_MODULE_ALL:
    {
        table_size = FIELD_OFFSET(MIB_TCPTABLE_OWNER_MODULE, table[row_count]);
        if (row_size) *row_size = sizeof(MIB_TCPROW_OWNER_MODULE);
        break;
    }
    default:
        ERR("unhandled class %u\n", class);
        return 0;
    }
    return table_size;
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
    struct pid_map *map;
    unsigned int i = 0, map_count = 16, buffer_len = 4096, process_count, pos = 0;
    NTSTATUS ret;
    char *buffer = NULL, *new_buffer;

    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, buffer_len ))) return NULL;

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

        if (!(new_buffer = HeapReAlloc( GetProcessHeap(), 0, buffer, buffer_len )))
        {
            HeapFree( GetProcessHeap(), 0, buffer );
            return NULL;
        }
        buffer = new_buffer;
    }

    if (!(map = HeapAlloc( GetProcessHeap(), 0, map_count * sizeof(*map) )))
    {
        HeapFree( GetProcessHeap(), 0, buffer );
        return NULL;
    }

    for (i = 0; i < process_count; ++i)
    {
        const struct process_info *process;

        pos = (pos + 7) & ~7;
        process = (const struct process_info *)(buffer + pos);

        if (i >= map_count)
        {
            struct pid_map *new_map;
            map_count *= 2;
            if (!(new_map = HeapReAlloc( GetProcessHeap(), 0, map, map_count * sizeof(*map))))
            {
                HeapFree( GetProcessHeap(), 0, map );
                HeapFree( GetProcessHeap(), 0, buffer );
                return NULL;
            }
            map = new_map;
        }

        map[i].pid = process->pid;
        map[i].unix_pid = process->unix_pid;

        pos += sizeof(struct process_info) + process->name_len;
        pos = (pos + 7) & ~7;
        pos += process->thread_count * sizeof(struct thread_info);
    }

    HeapFree( GetProcessHeap(), 0, buffer );
    *num_entries = process_count;
    return map;
}

static unsigned int find_owning_pid( struct pid_map *map, unsigned int num_entries, UINT_PTR inode )
{
#ifdef __linux__
    unsigned int i, len_socket;
    char socket[32];

    sprintf( socket, "socket:[%lu]", inode );
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

        fds = HeapAlloc( GetProcessHeap(), 0, fd_len );
        if (!fds) continue;

        proc_pidinfo( map[i].unix_pid, PROC_PIDLISTFDS, 0, fds, fd_len );
        n = fd_len / sizeof(struct proc_fdinfo);
        for (j = 0; j < n; j++)
        {
            if (fds[j].proc_fdtype != PROX_FDTYPE_SOCKET) continue;

            proc_pidfdinfo( map[i].unix_pid, fds[j].proc_fd, PROC_PIDFDSOCKETINFO, &sock, sizeof(sock) );
            if (sock.psi.soi_pcb == inode)
            {
                HeapFree( GetProcessHeap(), 0, fds );
                return map[i].pid;
            }
        }

        HeapFree( GetProcessHeap(), 0, fds );
    }
    return 0;
#else
    FIXME( "not implemented\n" );
    return 0;
#endif
}

static BOOL match_class( TCP_TABLE_CLASS class, MIB_TCP_STATE state )
{
    switch (class)
    {
    case TCP_TABLE_BASIC_ALL:
    case TCP_TABLE_OWNER_PID_ALL:
    case TCP_TABLE_OWNER_MODULE_ALL:
        return TRUE;

    case TCP_TABLE_BASIC_LISTENER:
    case TCP_TABLE_OWNER_PID_LISTENER:
    case TCP_TABLE_OWNER_MODULE_LISTENER:
        if (state == MIB_TCP_STATE_LISTEN) return TRUE;
        return FALSE;

    case TCP_TABLE_BASIC_CONNECTIONS:
    case TCP_TABLE_OWNER_PID_CONNECTIONS:
    case TCP_TABLE_OWNER_MODULE_CONNECTIONS:
        if (state == MIB_TCP_STATE_ESTAB) return TRUE;
        return FALSE;

    default:
        ERR( "unhandled class %u\n", class );
        return FALSE;
    }
}

DWORD build_tcp_table( TCP_TABLE_CLASS class, void **tablep, BOOL order, HANDLE heap, DWORD flags,
                       DWORD *size )
{
    MIB_TCPTABLE *table;
    MIB_TCPROW_OWNER_MODULE row;
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
            unsigned int num_entries = 0;
            int inode;

            if (class >= TCP_TABLE_OWNER_PID_LISTENER) map = get_pid_map( &num_entries );

            /* skip header line */
            ptr = fgets(buf, sizeof(buf), fp);
            while ((ptr = fgets(buf, sizeof(buf), fp)))
            {
                if (sscanf( ptr, "%*x: %x:%x %x:%x %x %*s %*s %*s %*s %*s %d",
                            &row.dwLocalAddr, &row.dwLocalPort, &row.dwRemoteAddr,
                            &row.dwRemotePort, &row.dwState, &inode ) != 6)
                    continue;
                row.dwLocalPort = htons( row.dwLocalPort );
                row.dwRemotePort = htons( row.dwRemotePort );
                row.dwState = TCPStateToMIBState( row.dwState );
                if (!match_class( class, row.dwState )) continue;

                if (class >= TCP_TABLE_OWNER_PID_LISTENER)
                    row.dwOwningPid = find_owning_pid( map, num_entries, inode );
                if (class >= TCP_TABLE_OWNER_MODULE_LISTENER)
                {
                    row.liCreateTimestamp.QuadPart = 0; /* FIXME */
                    memset( &row.OwningModuleInfo, 0, sizeof(row.OwningModuleInfo) );
                }
                if (!(table = append_table_row( heap, flags, table, &table_size, &count, &row, row_size )))
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
                    if (!match_class( class, row.dwState )) continue;
                    if (!(table = append_table_row( heap, flags, table, &table_size, &count, &row, row_size )))
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
        struct pid_map *pMap = NULL;
        unsigned NumEntries;

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

        if (class >= TCP_TABLE_OWNER_PID_LISTENER) pMap = get_pid_map( &NumEntries );

        /* Might be nothing here; first entry is just a header it seems */
        if (Len <= sizeof (struct xinpgen)) goto done;

        pOrigXIG = (struct xinpgen *)Buf;
        pXIG = pOrigXIG;

        for (pXIG = (struct xinpgen *)((char *)pXIG + pXIG->xig_len);
             pXIG->xig_len > sizeof (struct xinpgen);
             pXIG = (struct xinpgen *)((char *)pXIG + pXIG->xig_len))
        {
#if __FreeBSD_version >= 1200026
            struct xtcpcb *pTCPData = (struct xtcpcb *)pXIG;
            struct xinpcb *pINData = &pTCPData->xt_inp;
            struct xsocket *pSockData = &pINData->xi_socket;
#else
            struct tcpcb *pTCPData = &((struct xtcpcb *)pXIG)->xt_tp;
            struct inpcb *pINData = &((struct xtcpcb *)pXIG)->xt_inp;
            struct xsocket *pSockData = &((struct xtcpcb *)pXIG)->xt_socket;
#endif

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
            if (!match_class( class, row.dwState )) continue;
            if (class >= TCP_TABLE_OWNER_PID_LISTENER)
                row.dwOwningPid = find_owning_pid( pMap, NumEntries, (UINT_PTR)pSockData->so_pcb );
            if (class >= TCP_TABLE_OWNER_MODULE_LISTENER)
            {
                row.liCreateTimestamp.QuadPart = 0; /* FIXME */
                memset( &row.OwningModuleInfo, 0, sizeof(row.OwningModuleInfo) );
            }
            if (!(table = append_table_row( heap, flags, table, &table_size, &count, &row, row_size )))
                break;
        }

    done:
        HeapFree( GetProcessHeap(), 0, pMap );
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

/******************************************************************
 *    AllocateAndGetTcpExTableFromStack (IPHLPAPI.@)
 *
 * Get the TCP connection table.
 * Like GetTcpTable(), but allocate the returned table from heap.
 *
 * PARAMS
 *  ppTcpTable [Out] pointer into which the MIB_TCPTABLE_EX is
 *                   allocated and returned.
 *  bOrder     [In]  whether to sort the table
 *  heap       [In]  heap from which the table is allocated
 *  flags      [In]  flags to HeapAlloc
 *  family     [In]  address family [AF_INET|AF_INET6]
 *
 * RETURNS
 *  ERROR_INVALID_PARAMETER if ppTcpTable is NULL, whatever GetTcpTable()
 *  returns otherwise.
 */
DWORD WINAPI AllocateAndGetTcpExTableFromStack( VOID **ppTcpTable, BOOL bOrder,
                                                HANDLE heap, DWORD flags, DWORD family )
{
    TRACE("table %p, bOrder %d, heap %p, flags 0x%08x, family %u\n",
          ppTcpTable, bOrder, heap, flags, family);

    if (!ppTcpTable || !family)
        return ERROR_INVALID_PARAMETER;

    if (family != WS_AF_INET)
    {
        FIXME( "family = %u not supported\n", family );
        return ERROR_NOT_SUPPORTED;
    }

    return build_tcp_table( TCP_TABLE_OWNER_PID_ALL, ppTcpTable, bOrder, heap, flags, NULL );
}

static DWORD get_udp_table_sizes( UDP_TABLE_CLASS class, DWORD row_count, DWORD *row_size )
{
    DWORD table_size;

    switch (class)
    {
    case UDP_TABLE_BASIC:
    {
        table_size = FIELD_OFFSET(MIB_UDPTABLE, table[row_count]);
        if (row_size) *row_size = sizeof(MIB_UDPROW);
        break;
    }
    case UDP_TABLE_OWNER_PID:
    {
        table_size = FIELD_OFFSET(MIB_UDPTABLE_OWNER_PID, table[row_count]);
        if (row_size) *row_size = sizeof(MIB_UDPROW_OWNER_PID);
        break;
    }
    case UDP_TABLE_OWNER_MODULE:
    {
        table_size = FIELD_OFFSET(MIB_UDPTABLE_OWNER_MODULE, table[row_count]);
        if (row_size) *row_size = sizeof(MIB_UDPROW_OWNER_MODULE);
        break;
    }
    default:
        ERR("unhandled class %u\n", class);
        return 0;
    }
    return table_size;
}

static int compare_udp_rows(const void *a, const void *b)
{
    const MIB_UDPROW *rowA = a;
    const MIB_UDPROW *rowB = b;
    int ret;

    if ((ret = rowA->dwLocalAddr - rowB->dwLocalAddr) != 0) return ret;
    return rowA->dwLocalPort - rowB->dwLocalPort;
}

DWORD build_udp_table( UDP_TABLE_CLASS class, void **tablep, BOOL order, HANDLE heap, DWORD flags,
                       DWORD *size )
{
    MIB_UDPTABLE *table;
    MIB_UDPROW_OWNER_MODULE row;
    DWORD ret = NO_ERROR, count = 16, table_size, row_size;

    if (!(table_size = get_udp_table_sizes( class, count, &row_size )))
        return ERROR_INVALID_PARAMETER;

    if (!(table = HeapAlloc( heap, flags, table_size )))
         return ERROR_OUTOFMEMORY;

    table->dwNumEntries = 0;
    memset( &row, 0, sizeof(row) );

#ifdef __linux__
    {
        FILE *fp;

        if ((fp = fopen( "/proc/net/udp", "r" )))
        {
            char buf[512], *ptr;
            struct pid_map *map = NULL;
            unsigned int num_entries = 0;
            int inode;

            if (class >= UDP_TABLE_OWNER_PID) map = get_pid_map( &num_entries );

            /* skip header line */
            ptr = fgets( buf, sizeof(buf), fp );
            while ((ptr = fgets( buf, sizeof(buf), fp )))
            {
                if (sscanf( ptr, "%*u: %x:%x %*s %*s %*s %*s %*s %*s %*s %d",
                    &row.dwLocalAddr, &row.dwLocalPort, &inode ) != 3)
                    continue;
                row.dwLocalPort = htons( row.dwLocalPort );

                if (class >= UDP_TABLE_OWNER_PID)
                    row.dwOwningPid = find_owning_pid( map, num_entries, inode );
                if (class >= UDP_TABLE_OWNER_MODULE)
                {
                    row.liCreateTimestamp.QuadPart = 0; /* FIXME */
                    row.dwFlags = 0;
                    memset( &row.OwningModuleInfo, 0, sizeof(row.OwningModuleInfo) );
                }
                if (!(table = append_table_row( heap, flags, table, &table_size, &count, &row, row_size )))
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
        mib2_udpEntry_t *entry;

        if ((fd = open_streams_mib( "udp" )) != -1)
        {
            if ((data = read_mib_entry( fd, MIB2_UDP, MIB2_UDP_ENTRY, &len )))
            {
                for (entry = data; (char *)(entry + 1) <= (char *)data + len; entry++)
                {
                    row.dwLocalAddr = entry->udpLocalAddress;
                    row.dwLocalPort = htons( entry->udpLocalPort );
                    if (!(table = append_table_row( heap, flags, table, &table_size, &count, &row, row_size )))
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
        struct pid_map *pMap = NULL;
        unsigned NumEntries;

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

        if (class >= UDP_TABLE_OWNER_PID)
            pMap = get_pid_map( &NumEntries );

        /* Might be nothing here; first entry is just a header it seems */
        if (Len <= sizeof (struct xinpgen)) goto done;

        pOrigXIG = (struct xinpgen *)Buf;
        pXIG = pOrigXIG;

        for (pXIG = (struct xinpgen *)((char *)pXIG + pXIG->xig_len);
             pXIG->xig_len > sizeof (struct xinpgen);
             pXIG = (struct xinpgen *)((char *)pXIG + pXIG->xig_len))
        {
#if __FreeBSD_version >= 1200026
            struct xinpcb *pINData = (struct xinpcb *)pXIG;
            struct xsocket *pSockData = &pINData->xi_socket;
#else
            struct inpcb *pINData = &((struct xinpcb *)pXIG)->xi_inp;
            struct xsocket *pSockData = &((struct xinpcb *)pXIG)->xi_socket;
#endif

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
            if (class >= UDP_TABLE_OWNER_PID)
                row.dwOwningPid = find_owning_pid( pMap, NumEntries, (UINT_PTR)pSockData->so_pcb );
            if (class >= UDP_TABLE_OWNER_MODULE)
            {
                row.liCreateTimestamp.QuadPart = 0; /* FIXME */
                row.dwFlags = 0;
                row.SpecificPortBind = !(pINData->inp_flags & INP_ANONPORT);
                memset( &row.OwningModuleInfo, 0, sizeof(row.OwningModuleInfo) );
            }
            if (!(table = append_table_row( heap, flags, table, &table_size, &count, &row, row_size )))
                break;
        }

    done:
        HeapFree( GetProcessHeap(), 0, pMap );
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
            qsort( table->table, table->dwNumEntries, row_size, compare_udp_rows );
        *tablep = table;
    }
    else HeapFree( heap, flags, table );
    if (size) *size = get_udp_table_sizes( class, count, NULL );
    TRACE( "returning ret %u table %p\n", ret, table );
    return ret;
}

static DWORD get_tcp6_table_sizes( TCP_TABLE_CLASS class, DWORD row_count, DWORD *row_size )
{
    DWORD table_size;

    switch (class)
    {
    case TCP_TABLE_BASIC_LISTENER:
    case TCP_TABLE_BASIC_CONNECTIONS:
    case TCP_TABLE_BASIC_ALL:
    {
        table_size = FIELD_OFFSET(MIB_TCP6TABLE, table[row_count]);
        if (row_size) *row_size = sizeof(MIB_TCP6ROW);
        break;
    }
    case TCP_TABLE_OWNER_PID_LISTENER:
    case TCP_TABLE_OWNER_PID_CONNECTIONS:
    case TCP_TABLE_OWNER_PID_ALL:
    {
        table_size = FIELD_OFFSET(MIB_TCP6TABLE_OWNER_PID, table[row_count]);
        if (row_size) *row_size = sizeof(MIB_TCP6ROW_OWNER_PID);
        break;
    }
    case TCP_TABLE_OWNER_MODULE_LISTENER:
    case TCP_TABLE_OWNER_MODULE_CONNECTIONS:
    case TCP_TABLE_OWNER_MODULE_ALL:
    {
        table_size = FIELD_OFFSET(MIB_TCP6TABLE_OWNER_MODULE, table[row_count]);
        if (row_size) *row_size = sizeof(MIB_TCP6ROW_OWNER_MODULE);
        break;
    }
    default:
        ERR("unhandled class %u\n", class);
        return 0;
    }
    return table_size;
}

static int compare_tcp6_basic_rows(const void *a, const void *b)
{
    const MIB_TCP6ROW *rowA = a;
    const MIB_TCP6ROW *rowB = b;
    int ret;

    if ((ret = memcmp(&rowA->LocalAddr, &rowB->LocalAddr, sizeof(rowA->LocalAddr))) != 0) return ret;
    if ((ret = rowA->dwLocalScopeId - rowB->dwLocalScopeId) != 0) return ret;
    if ((ret = rowA->dwLocalPort - rowB->dwLocalPort) != 0) return ret;
    if ((ret = memcmp(&rowA->RemoteAddr, &rowB->RemoteAddr, sizeof(rowA->RemoteAddr))) != 0) return ret;
    if ((ret = rowA->dwRemoteScopeId - rowB->dwRemoteScopeId) != 0) return ret;
    return rowA->dwRemotePort - rowB->dwRemotePort;
}

static int compare_tcp6_owner_rows(const void *a, const void *b)
{
    const MIB_TCP6ROW_OWNER_PID *rowA = a;
    const MIB_TCP6ROW_OWNER_PID *rowB = b;
    int ret;

    if ((ret = memcmp(&rowA->ucLocalAddr, &rowB->ucLocalAddr, sizeof(rowA->ucLocalAddr))) != 0) return ret;
    if ((ret = rowA->dwLocalScopeId - rowB->dwLocalScopeId) != 0) return ret;
    if ((ret = rowA->dwLocalPort - rowB->dwLocalPort) != 0) return ret;
    if ((ret = memcmp(&rowA->ucRemoteAddr, &rowB->ucRemoteAddr, sizeof(rowA->ucRemoteAddr))) != 0) return ret;
    if ((ret = rowA->dwRemoteScopeId - rowB->dwRemoteScopeId) != 0) return ret;
    return rowA->dwRemotePort - rowB->dwRemotePort;
}

static DWORD get_udp6_table_sizes( UDP_TABLE_CLASS class, DWORD row_count, DWORD *row_size )
{
    DWORD table_size;

    switch (class)
    {
    case UDP_TABLE_BASIC:
    {
        table_size = FIELD_OFFSET(MIB_UDP6TABLE, table[row_count]);
        if (row_size) *row_size = sizeof(MIB_UDP6ROW);
        break;
    }
    case UDP_TABLE_OWNER_PID:
    {
        table_size = FIELD_OFFSET(MIB_UDP6TABLE_OWNER_PID, table[row_count]);
        if (row_size) *row_size = sizeof(MIB_UDP6ROW_OWNER_PID);
        break;
    }
    case UDP_TABLE_OWNER_MODULE:
    {
        table_size = FIELD_OFFSET(MIB_UDP6TABLE_OWNER_MODULE, table[row_count]);
        if (row_size) *row_size = sizeof(MIB_UDP6ROW_OWNER_MODULE);
        break;
    }
    default:
        ERR("unhandled class %u\n", class);
        return 0;
    }
    return table_size;
}

static int compare_udp6_rows(const void *a, const void *b)
{
    const MIB_UDP6ROW *rowA = a;
    const MIB_UDP6ROW *rowB = b;
    int ret;

    if ((ret = memcmp(&rowA->dwLocalAddr, &rowB->dwLocalAddr, sizeof(rowA->dwLocalAddr))) != 0) return ret;
    if ((ret = rowA->dwLocalScopeId - rowB->dwLocalScopeId) != 0) return ret;
    return rowA->dwLocalPort - rowB->dwLocalPort;
}

#if defined(__linux__) || (defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_STRUCT_XINPGEN))
struct ipv6_addr_scope
{
    IN6_ADDR addr;
    DWORD scope;
};

static struct ipv6_addr_scope *get_ipv6_addr_scope_table(unsigned int *size)
{
    struct ipv6_addr_scope *table = NULL;
    unsigned int table_size = 0;
#ifdef __linux__
    char buf[512], *ptr;
    FILE *fp;
#elif defined(HAVE_GETIFADDRS)
    struct ifaddrs *addrs, *cur;
#endif

    if (!(table = HeapAlloc( GetProcessHeap(), 0, sizeof(table[0]) )))
        return NULL;

#ifdef __linux__
    if (!(fp = fopen( "/proc/net/if_inet6", "r" )))
        goto failed;

    while ((ptr = fgets( buf, sizeof(buf), fp )))
    {
        WORD a[8];
        DWORD scope;
        struct ipv6_addr_scope *new_table;
        struct ipv6_addr_scope *entry;
        unsigned int i;

        if (sscanf( ptr, "%4hx%4hx%4hx%4hx%4hx%4hx%4hx%4hx %*s %*s %x",
            &a[0], &a[1], &a[2], &a[3], &a[4], &a[5], &a[6], &a[7], &scope ) != 9)
            continue;

        table_size++;
        if (!(new_table = HeapReAlloc( GetProcessHeap(), 0, table, table_size * sizeof(table[0]) )))
        {
            fclose(fp);
            goto failed;
        }

        table = new_table;
        entry = &table[table_size - 1];

        i = 0;
        while (i < 8)
        {
            entry->addr.u.Word[i] = htons(a[i]);
            i++;
        }

        entry->scope = htons(scope);
    }

    fclose(fp);
#elif defined(HAVE_GETIFADDRS)
    if (getifaddrs(&addrs) == -1)
        goto failed;

    for (cur = addrs; cur; cur = cur->ifa_next)
    {
        struct sockaddr_in6 *sin6;
        struct ipv6_addr_scope *new_table;
        struct ipv6_addr_scope *entry;

        if (cur->ifa_addr->sa_family != AF_INET6)
            continue;

        sin6 = (struct sockaddr_in6 *)cur->ifa_addr;

        table_size++;
        if (!(new_table = HeapReAlloc( GetProcessHeap(), 0, table, table_size * sizeof(table[0]) )))
        {
            freeifaddrs(addrs);
            goto failed;
        }

        table = new_table;
        entry = &table[table_size - 1];

        memcpy(&entry->addr, &sin6->sin6_addr, sizeof(entry->addr));
        entry->scope = sin6->sin6_scope_id;
    }

    freeifaddrs(addrs);
#else
    FIXME( "not implemented\n" );
    goto failed;
#endif

    *size = table_size;
    return table;

failed:
    HeapFree( GetProcessHeap(), 0, table );
    return NULL;
}

static DWORD find_ipv6_addr_scope(const IN6_ADDR *addr, const struct ipv6_addr_scope *table, unsigned int size)
{
    const BYTE multicast_scope_mask = 0x0F;
    const BYTE multicast_scope_shift = 0;
    unsigned int i = 0;

    if (WS_IN6_IS_ADDR_UNSPECIFIED(addr))
        return 0;

    if (WS_IN6_IS_ADDR_MULTICAST(addr))
        return htons((addr->u.Byte[1] & multicast_scope_mask) >> multicast_scope_shift);

    if (!table)
        return -1;

    while (i < size)
    {
        if (memcmp(&table[i].addr, addr, sizeof(table[i].addr)) == 0)
            return table[i].scope;
        i++;
    }

    return -1;
}
#endif

DWORD build_tcp6_table( TCP_TABLE_CLASS class, void **tablep, BOOL order, HANDLE heap, DWORD flags,
                        DWORD *size )
{
    MIB_TCP6TABLE *table;
    DWORD ret = NO_ERROR, count = 16, table_size, row_size;

    if (!(table_size = get_tcp6_table_sizes( class, count, &row_size )))
        return ERROR_INVALID_PARAMETER;

    if (!(table = HeapAlloc( heap, flags, table_size )))
        return ERROR_OUTOFMEMORY;

    table->dwNumEntries = 0;

#ifdef __linux__
    {
        MIB_TCP6ROW_OWNER_MODULE row;
        FILE *fp;

        if ((fp = fopen( "/proc/net/tcp6", "r" )))
        {
            char buf[512], *ptr;
            struct pid_map *map = NULL;
            unsigned int num_entries = 0;
            struct ipv6_addr_scope *addr_scopes;
            unsigned int addr_scopes_size = 0;
            int inode;

            addr_scopes = get_ipv6_addr_scope_table(&addr_scopes_size);

            if (class >= TCP_TABLE_OWNER_PID_LISTENER) map = get_pid_map( &num_entries );

            /* skip header line */
            ptr = fgets( buf, sizeof(buf), fp );
            while ((ptr = fgets( buf, sizeof(buf), fp )))
            {
                DWORD *local_addr = (DWORD *)&row.ucLocalAddr;
                DWORD *remote_addr = (DWORD *)&row.ucRemoteAddr;

                if (sscanf( ptr, "%*u: %8x%8x%8x%8x:%x %8x%8x%8x%8x:%x %x %*s %*s %*s %*s %*s %*s %*s %d",
                            &local_addr[0], &local_addr[1], &local_addr[2], &local_addr[3], &row.dwLocalPort,
                            &remote_addr[0], &remote_addr[1], &remote_addr[2], &remote_addr[3], &row.dwRemotePort,
                            &row.dwState, &inode ) != 12)
                    continue;
                row.dwState = TCPStateToMIBState( row.dwState );
                if (!match_class( class, row.dwState )) continue;
                row.dwLocalScopeId = find_ipv6_addr_scope((const IN6_ADDR *)&row.ucLocalAddr, addr_scopes, addr_scopes_size);
                row.dwLocalPort = htons( row.dwLocalPort );
                row.dwRemoteScopeId = find_ipv6_addr_scope((const IN6_ADDR *)&row.ucRemoteAddr, addr_scopes, addr_scopes_size);
                row.dwRemotePort = htons( row.dwRemotePort );

                if (class <= TCP_TABLE_BASIC_ALL)
                {
                    /* MIB_TCP6ROW has a different field order */
                    MIB_TCP6ROW basic_row;
                    basic_row.State = row.dwState;
                    memcpy( &basic_row.LocalAddr, &row.ucLocalAddr, sizeof(row.ucLocalAddr) );
                    basic_row.dwLocalScopeId = row.dwLocalScopeId;
                    basic_row.dwLocalPort = row.dwLocalPort;
                    memcpy( &basic_row.RemoteAddr, &row.ucRemoteAddr, sizeof(row.ucRemoteAddr) );
                    basic_row.dwRemoteScopeId = row.dwRemoteScopeId;
                    basic_row.dwRemotePort = row.dwRemotePort;
                    if (!(table = append_table_row( heap, flags, table, &table_size, &count, &basic_row, row_size )))
                        break;
                    continue;
                }

                row.dwOwningPid = find_owning_pid( map, num_entries, inode );
                if (class >= TCP_TABLE_OWNER_MODULE_LISTENER)
                {
                    row.liCreateTimestamp.QuadPart = 0; /* FIXME */
                    memset( &row.OwningModuleInfo, 0, sizeof(row.OwningModuleInfo) );
                }
                if (!(table = append_table_row( heap, flags, table, &table_size, &count, &row, row_size )))
                    break;
            }
            HeapFree( GetProcessHeap(), 0, map );
            HeapFree( GetProcessHeap(), 0, addr_scopes );
            fclose( fp );
        }
        else ret = ERROR_NOT_SUPPORTED;
    }
#elif defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_STRUCT_XINPGEN)
    {
        static const char zero[sizeof(IN6_ADDR)] = {0};

        MIB_TCP6ROW_OWNER_MODULE row;
        size_t len = 0;
        char *buf = NULL;
        struct xinpgen *xig, *orig_xig;
        struct pid_map *map = NULL;
        unsigned num_entries;
        struct ipv6_addr_scope *addr_scopes = NULL;
        unsigned int addr_scopes_size = 0;

        if (sysctlbyname( "net.inet.tcp.pcblist", NULL, &len, NULL, 0 ) < 0)
        {
            ERR( "Failure to read net.inet.tcp.pcblist via sysctlbyname!\n" );
            ret = ERROR_NOT_SUPPORTED;
            goto done;
        }

        buf = HeapAlloc( GetProcessHeap(), 0, len );
        if (!buf)
        {
            ret = ERROR_OUTOFMEMORY;
            goto done;
        }

        if (sysctlbyname( "net.inet.tcp.pcblist", buf, &len, NULL, 0 ) < 0)
        {
            ERR( "Failure to read net.inet.tcp.pcblist via sysctlbyname!\n" );
            ret = ERROR_NOT_SUPPORTED;
            goto done;
        }

        addr_scopes = get_ipv6_addr_scope_table( &addr_scopes_size );
        if (!addr_scopes)
        {
            ret = ERROR_OUTOFMEMORY;
            goto done;
        }

        if (class >= TCP_TABLE_OWNER_PID_LISTENER) map = get_pid_map( &num_entries );

        /* Might be nothing here; first entry is just a header it seems */
        if (len <= sizeof (struct xinpgen)) goto done;

        orig_xig = (struct xinpgen *)buf;
        xig = orig_xig;

        for (xig = (struct xinpgen *)((char *)xig + xig->xig_len);
             xig->xig_len > sizeof (struct xinpgen);
             xig = (struct xinpgen *)((char *)xig + xig->xig_len))
        {
#if __FreeBSD_version >= 1200026
            struct xtcpcb *tcp = (struct xtcpcb *)xig;
            struct xinpcb *in = &tcp->xt_inp;
            struct xsocket *sock = &in->xi_socket;
#else
            struct tcpcb *tcp = &((struct xtcpcb *)xig)->xt_tp;
            struct inpcb *in = &((struct xtcpcb *)xig)->xt_inp;
            struct xsocket *sock = &((struct xtcpcb *)xig)->xt_socket;
#endif

            /* Ignore sockets for other protocols */
            if (sock->xso_protocol != IPPROTO_TCP)
                continue;

            /* Ignore PCBs that were freed while generating the data */
            if (in->inp_gencnt > orig_xig->xig_gen)
                continue;

            /* we're only interested in IPv6 addresses */
            if (!(in->inp_vflag & INP_IPV6) ||
                (in->inp_vflag & INP_IPV4))
                continue;

            /* If all 0's, skip it */
            if (!memcmp( &in->in6p_laddr, zero, sizeof(zero) ) && !in->inp_lport &&
                !memcmp( &in->in6p_faddr, zero, sizeof(zero) ) && !in->inp_fport)
                continue;

            /* Fill in structure details */
            memcpy( &row.ucLocalAddr, &in->in6p_laddr.s6_addr, sizeof(row.ucLocalAddr) );
            row.dwLocalPort = in->inp_lport;
            row.dwLocalScopeId = find_ipv6_addr_scope( (const IN6_ADDR *)&row.ucLocalAddr, addr_scopes, addr_scopes_size );
            memcpy( &row.ucRemoteAddr, &in->in6p_faddr.s6_addr, sizeof(row.ucRemoteAddr) );
            row.dwRemotePort = in->inp_fport;
            row.dwRemoteScopeId = find_ipv6_addr_scope( (const IN6_ADDR *)&row.ucRemoteAddr, addr_scopes, addr_scopes_size );
            row.dwState = TCPStateToMIBState( tcp->t_state );
            if (!match_class( class, row.dwState )) continue;

            if (class <= TCP_TABLE_BASIC_ALL)
            {
                /* MIB_TCP6ROW has a different field order */
                MIB_TCP6ROW basic_row;
                basic_row.State = row.dwState;
                memcpy( &basic_row.LocalAddr, &row.ucLocalAddr, sizeof(row.ucLocalAddr) );
                basic_row.dwLocalScopeId = row.dwLocalScopeId;
                basic_row.dwLocalPort = row.dwLocalPort;
                memcpy( &basic_row.RemoteAddr, &row.ucRemoteAddr, sizeof(row.ucRemoteAddr) );
                basic_row.dwRemoteScopeId = row.dwRemoteScopeId;
                basic_row.dwRemotePort = row.dwRemotePort;
                if (!(table = append_table_row( heap, flags, table, &table_size, &count, &basic_row, row_size )))
                    break;
                continue;
            }

            row.dwOwningPid = find_owning_pid( map, num_entries, (UINT_PTR)sock->so_pcb );
            if (class >= TCP_TABLE_OWNER_MODULE_LISTENER)
            {
                row.liCreateTimestamp.QuadPart = 0; /* FIXME */
                memset( &row.OwningModuleInfo, 0, sizeof(row.OwningModuleInfo) );
            }
            if (!(table = append_table_row( heap, flags, table, &table_size, &count, &row, row_size )))
                break;
        }

    done:
        HeapFree( GetProcessHeap(), 0, map );
        HeapFree( GetProcessHeap(), 0, buf );
        HeapFree( GetProcessHeap(), 0, addr_scopes );
    }
#else
    FIXME( "not implemented\n" );
    ret = ERROR_NOT_SUPPORTED;
#endif

    if (!table) return ERROR_OUTOFMEMORY;
    if (!ret)
    {
        if (order && table->dwNumEntries)
        {
            qsort( table->table, table->dwNumEntries, row_size,
                   class <= TCP_TABLE_BASIC_ALL ? compare_tcp6_basic_rows : compare_tcp6_owner_rows );
        }
        *tablep = table;
    }
    else HeapFree( heap, flags, table );
    if (size) *size = get_tcp6_table_sizes( class, count, NULL );
    TRACE( "returning ret %u table %p\n", ret, table );
    return ret;
}

DWORD build_udp6_table( UDP_TABLE_CLASS class, void **tablep, BOOL order, HANDLE heap, DWORD flags,
                        DWORD *size )
{
    MIB_UDP6TABLE *table;
    MIB_UDP6ROW_OWNER_MODULE row;
    DWORD ret = NO_ERROR, count = 16, table_size, row_size;

    if (!(table_size = get_udp6_table_sizes( class, count, &row_size )))
        return ERROR_INVALID_PARAMETER;

    if (!(table = HeapAlloc( heap, flags, table_size )))
         return ERROR_OUTOFMEMORY;

    table->dwNumEntries = 0;
    memset( &row, 0, sizeof(row) );

#ifdef __linux__
    {
        FILE *fp;

        if ((fp = fopen( "/proc/net/udp6", "r" )))
        {
            char buf[512], *ptr;
            struct pid_map *map = NULL;
            unsigned int num_entries = 0;
            struct ipv6_addr_scope *addr_scopes;
            unsigned int addr_scopes_size = 0;
            int inode;

            addr_scopes = get_ipv6_addr_scope_table(&addr_scopes_size);

            if (class >= UDP_TABLE_OWNER_PID) map = get_pid_map( &num_entries );

            /* skip header line */
            ptr = fgets( buf, sizeof(buf), fp );
            while ((ptr = fgets( buf, sizeof(buf), fp )))
            {
                DWORD *local_addr = (DWORD *)&row.ucLocalAddr;

                if (sscanf( ptr, "%*u: %8x%8x%8x%8x:%x %*s %*s %*s %*s %*s %*s %*s %d",
                    &local_addr[0], &local_addr[1], &local_addr[2], &local_addr[3],
                    &row.dwLocalPort, &inode ) != 6)
                    continue;
                row.dwLocalScopeId = find_ipv6_addr_scope((const IN6_ADDR *)&row.ucLocalAddr, addr_scopes, addr_scopes_size);
                row.dwLocalPort = htons( row.dwLocalPort );

                if (class >= UDP_TABLE_OWNER_PID)
                    row.dwOwningPid = find_owning_pid( map, num_entries, inode );
                if (class >= UDP_TABLE_OWNER_MODULE)
                {
                    row.liCreateTimestamp.QuadPart = 0; /* FIXME */
                    row.dwFlags = 0;
                    memset( &row.OwningModuleInfo, 0, sizeof(row.OwningModuleInfo) );
                }
                if (!(table = append_table_row( heap, flags, table, &table_size, &count, &row, row_size )))
                    break;
            }
            HeapFree( GetProcessHeap(), 0, map );
            HeapFree( GetProcessHeap(), 0, addr_scopes );
            fclose( fp );
        }
        else ret = ERROR_NOT_SUPPORTED;
    }
#elif defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_STRUCT_XINPGEN)
    {
        static const char zero[sizeof(IN6_ADDR)] = {0};

        size_t len = 0;
        char *buf = NULL;
        struct xinpgen *xig, *orig_xig;
        struct pid_map *map = NULL;
        unsigned num_entries;
        struct ipv6_addr_scope *addr_scopes = NULL;
        unsigned int addr_scopes_size = 0;

        if (sysctlbyname( "net.inet.udp.pcblist", NULL, &len, NULL, 0 ) < 0)
        {
            ERR( "Failure to read net.inet.udp.pcblist via sysctlbyname!\n" );
            ret = ERROR_NOT_SUPPORTED;
            goto done;
        }

        buf = HeapAlloc( GetProcessHeap(), 0, len );
        if (!buf)
        {
            ret = ERROR_OUTOFMEMORY;
            goto done;
        }

        if (sysctlbyname( "net.inet.udp.pcblist", buf, &len, NULL, 0 ) < 0)
        {
            ERR ("Failure to read net.inet.udp.pcblist via sysctlbyname!\n");
            ret = ERROR_NOT_SUPPORTED;
            goto done;
        }

        addr_scopes = get_ipv6_addr_scope_table( &addr_scopes_size );
        if (!addr_scopes)
        {
            ret = ERROR_OUTOFMEMORY;
            goto done;
        }

        if (class >= UDP_TABLE_OWNER_PID) map = get_pid_map( &num_entries );

        /* Might be nothing here; first entry is just a header it seems */
        if (len <= sizeof (struct xinpgen)) goto done;

        orig_xig = (struct xinpgen *)buf;
        xig = orig_xig;

        for (xig = (struct xinpgen *)((char *)xig + xig->xig_len);
             xig->xig_len > sizeof (struct xinpgen);
             xig = (struct xinpgen *)((char *)xig + xig->xig_len))
        {
#if __FreeBSD_version >= 1200026
            struct xinpcb *in = (struct xinpcb *)xig;
            struct xsocket *sock = &in->xi_socket;
#else
            struct inpcb *in = &((struct xinpcb *)xig)->xi_inp;
            struct xsocket *sock = &((struct xinpcb *)xig)->xi_socket;
#endif

            /* Ignore sockets for other protocols */
            if (sock->xso_protocol != IPPROTO_UDP)
                continue;

            /* Ignore PCBs that were freed while generating the data */
            if (in->inp_gencnt > orig_xig->xig_gen)
                continue;

            /* we're only interested in IPv6 addresses */
            if (!(in->inp_vflag & INP_IPV6) ||
                (in->inp_vflag & INP_IPV4))
                continue;

            /* If all 0's, skip it */
            if (!memcmp( &in->in6p_laddr.s6_addr, zero, sizeof(zero) ) && !in->inp_lport)
                continue;

            /* Fill in structure details */
            memcpy(row.ucLocalAddr, &in->in6p_laddr.s6_addr, sizeof(row.ucLocalAddr));
            row.dwLocalPort = in->inp_lport;
            row.dwLocalScopeId = find_ipv6_addr_scope((const IN6_ADDR *)&row.ucLocalAddr, addr_scopes, addr_scopes_size);
            if (class >= UDP_TABLE_OWNER_PID)
                row.dwOwningPid = find_owning_pid( map, num_entries, (UINT_PTR)sock->so_pcb );
            if (class >= UDP_TABLE_OWNER_MODULE)
            {
                row.liCreateTimestamp.QuadPart = 0; /* FIXME */
                row.dwFlags = 0;
                row.SpecificPortBind = !(in->inp_flags & INP_ANONPORT);
                memset( &row.OwningModuleInfo, 0, sizeof(row.OwningModuleInfo) );
            }
            if (!(table = append_table_row( heap, flags, table, &table_size, &count, &row, row_size )))
                break;
        }

    done:
        HeapFree( GetProcessHeap(), 0, map );
        HeapFree( GetProcessHeap(), 0, buf );
        HeapFree( GetProcessHeap(), 0, addr_scopes );
    }
#else
    FIXME( "not implemented\n" );
    ret = ERROR_NOT_SUPPORTED;
#endif

    if (!table) return ERROR_OUTOFMEMORY;
    if (!ret)
    {
        if (order && table->dwNumEntries)
            qsort( table->table, table->dwNumEntries, row_size, compare_udp6_rows );
        *tablep = table;
    }
    else HeapFree( heap, flags, table );
    if (size) *size = get_udp6_table_sizes( class, count, NULL );
    TRACE( "returning ret %u table %p\n", ret, table );
    return ret;
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
    TRACE("table %p, bOrder %d, heap %p, flags 0x%08x\n", ppUdpTable, bOrder, heap, flags);

    if (!ppUdpTable) return ERROR_INVALID_PARAMETER;
    return build_udp_table( UDP_TABLE_BASIC, (void **)ppUdpTable, bOrder, heap, flags, NULL );
}
