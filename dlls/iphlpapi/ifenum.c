/* Copyright (C) 2003,2006,2011 Juan Lang
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
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

#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif

#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif

#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif

#ifdef HAVE_NET_IF_TYPES_H
#include <net/if_types.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif

#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif

#include "ifenum.h"
#include "ws2ipdef.h"

#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
#define ifreq_len(ifr) \
 max(sizeof(struct ifreq), sizeof((ifr)->ifr_name)+(ifr)->ifr_addr.sa_len)
#else
#define ifreq_len(ifr) sizeof(struct ifreq)
#endif

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

#ifndef IF_NAMESIZE
#define IF_NAMESIZE 16
#endif

#ifndef INADDR_NONE
#define INADDR_NONE (~0U)
#endif

#define INITIAL_INTERFACES_ASSUMED 4

/* Functions */

static BOOL isLoopbackInterface(int fd, const char *name)
{
  BOOL ret = FALSE;

  if (name) {
    struct ifreq ifr;

    lstrcpynA(ifr.ifr_name, name, IFNAMSIZ);
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0)
      ret = ifr.ifr_flags & IFF_LOOPBACK;
  }
  return !!ret;
}

/* The comments say MAX_ADAPTER_NAME is required, but really only IF_NAMESIZE
 * bytes are necessary.
 */
static char *getInterfaceNameByIndex(IF_INDEX index, char *name)
{
  return if_indextoname(index, name);
}

DWORD getInterfaceIndexByName(const char *name, IF_INDEX *index)
{
  DWORD ret;
  unsigned int idx;

  if (!name)
    return ERROR_INVALID_PARAMETER;
  if (!index)
    return ERROR_INVALID_PARAMETER;
  idx = if_nametoindex(name);
  if (idx) {
    *index = idx;
    ret = NO_ERROR;
  }
  else
    ret = ERROR_INVALID_DATA;
  return ret;
}

static BOOL isIfIndexLoopback(ULONG idx)
{
  BOOL ret = FALSE;
  char name[IFNAMSIZ];
  int fd;

  getInterfaceNameByIndex(idx, name);
  fd = socket(PF_INET, SOCK_DGRAM, 0);
  if (fd != -1) {
    ret = isLoopbackInterface(fd, name);
    close(fd);
  }
  return ret;
}

#ifdef HAVE_IF_NAMEINDEX
DWORD get_interface_indices( BOOL skip_loopback, InterfaceIndexTable **table )
{
    DWORD count = 0, i;
    struct if_nameindex *p, *indices = if_nameindex();
    InterfaceIndexTable *ret;

    if (table) *table = NULL;
    if (!indices) return 0;

    for (p = indices; p->if_name; p++)
    {
        if (skip_loopback && isIfIndexLoopback( p->if_index )) continue;
        count++;
    }

    if (table)
    {
        ret = HeapAlloc( GetProcessHeap(), 0, FIELD_OFFSET(InterfaceIndexTable, indexes[count]) );
        if (!ret)
        {
            count = 0;
            goto end;
        }
        for (p = indices, i = 0; p->if_name && i < count; p++)
        {
            if (skip_loopback && isIfIndexLoopback( p->if_index )) continue;
            ret->indexes[i++] = p->if_index;
        }
        ret->numIndexes = count = i;
        *table = ret;
    }

end:
    if_freenameindex( indices );
    return count;
}

#elif defined(HAVE_LINUX_RTNETLINK_H)
static int open_netlink( int *pid )
{
    int fd = socket( AF_NETLINK, SOCK_RAW, NETLINK_ROUTE );
    struct sockaddr_nl addr;
    socklen_t len;

    if (fd < 0) return fd;

    memset( &addr, 0, sizeof(addr) );
    addr.nl_family = AF_NETLINK;

    if (bind( fd, (struct sockaddr *)&addr, sizeof(addr) ) < 0)
        goto fail;

    len = sizeof(addr);
    if (getsockname( fd, (struct sockaddr *)&addr, &len ) < 0)
        goto fail;

    *pid = addr.nl_pid;
    return fd;
fail:
    close( fd );
    return -1;
}

static int send_netlink_req( int fd, int pid, int type, int *seq_no )
{
    static LONG seq;
    struct request
    {
        struct nlmsghdr hdr;
        struct rtgenmsg gen;
    } req;
    struct sockaddr_nl addr;

    req.hdr.nlmsg_len = sizeof(req);
    req.hdr.nlmsg_type = type;
    req.hdr.nlmsg_flags = NLM_F_ROOT | NLM_F_MATCH | NLM_F_REQUEST;
    req.hdr.nlmsg_pid = pid;
    req.hdr.nlmsg_seq = InterlockedIncrement( &seq );
    req.gen.rtgen_family = AF_UNSPEC;

    memset( &addr, 0, sizeof(addr) );
    addr.nl_family = AF_NETLINK;
    if (sendto( fd, &req, sizeof(req), 0, (struct sockaddr *)&addr, sizeof(addr) ) != sizeof(req))
        return -1;
    *seq_no = req.hdr.nlmsg_seq;
    return 0;
}

struct netlink_reply
{
    struct netlink_reply *next;
    int size;
    struct nlmsghdr *hdr;
};

static void free_netlink_reply( struct netlink_reply *data )
{
    struct netlink_reply *ptr;
    while( data )
    {
        ptr = data->next;
        HeapFree( GetProcessHeap(), 0, data );
        data = ptr;
    }
}

static int recv_netlink_reply( int fd, int pid, int seq, struct netlink_reply **data )
{
    int bufsize = getpagesize();
    int left, read;
    BOOL done = FALSE;
    socklen_t sa_len;
    struct sockaddr_nl addr;
    struct netlink_reply *cur, *last = NULL;
    struct nlmsghdr *hdr;
    char *buf;

    *data = NULL;
    buf = HeapAlloc( GetProcessHeap(), 0, bufsize );
    if (!buf) return -1;

    do
    {
        left = read = recvfrom( fd, buf, bufsize, 0, (struct sockaddr *)&addr, &sa_len );
        if (read < 0) goto fail;
        if (addr.nl_pid != 0) continue; /* not from kernel */

        for (hdr = (struct nlmsghdr *)buf; NLMSG_OK(hdr, left); hdr = NLMSG_NEXT(hdr, left))
        {
            if (hdr->nlmsg_pid != pid || hdr->nlmsg_seq != seq) continue;
            if (hdr->nlmsg_type == NLMSG_DONE)
            {
                done = TRUE;
                break;
            }
        }

        cur = HeapAlloc( GetProcessHeap(), 0, sizeof(*cur) + read );
        if (!cur) goto fail;
        cur->next = NULL;
        cur->size = read;
        cur->hdr = (struct nlmsghdr *)(cur + 1);
        memcpy( cur->hdr, buf, read );
        if (last) last->next = cur;
        else *data = cur;
        last = cur;
    } while (!done);

    HeapFree( GetProcessHeap(), 0, buf );
    return 0;

fail:
    free_netlink_reply( *data );
    HeapFree( GetProcessHeap(), 0, buf );
    return -1;
}


static DWORD get_indices_from_reply( struct netlink_reply *reply, int pid, int seq,
                                     BOOL skip_loopback, InterfaceIndexTable *table )
{
    struct nlmsghdr *hdr;
    struct netlink_reply *r;
    int count = 0;

    for (r = reply; r; r = r->next)
    {
        int size = r->size;
        for (hdr = r->hdr; NLMSG_OK(hdr, size); hdr = NLMSG_NEXT(hdr, size))
        {
            if (hdr->nlmsg_pid != pid || hdr->nlmsg_seq != seq) continue;
            if (hdr->nlmsg_type == NLMSG_DONE) break;

            if (hdr->nlmsg_type == RTM_NEWLINK)
            {
                struct ifinfomsg *info = NLMSG_DATA(hdr);

                if (skip_loopback && (info->ifi_flags & IFF_LOOPBACK)) continue;
                if (table) table->indexes[count] = info->ifi_index;
                count++;
            }
        }
    }
    return count;
}

DWORD get_interface_indices( BOOL skip_loopback, InterfaceIndexTable **table )
{
    int fd, pid, seq;
    struct netlink_reply *reply = NULL;
    DWORD count = 0;

    if (table) *table = NULL;
    fd = open_netlink( &pid );
    if (fd < 0) return 0;

    if (send_netlink_req( fd, pid, RTM_GETLINK, &seq ) < 0)
        goto end;

    if (recv_netlink_reply( fd, pid, seq, &reply ) < 0)
        goto end;

    count = get_indices_from_reply( reply, pid, seq, skip_loopback, NULL );

    if (table)
    {
        InterfaceIndexTable *ret = HeapAlloc( GetProcessHeap(), 0, FIELD_OFFSET(InterfaceIndexTable, indexes[count]) );
        if (!ret)
        {
            count = 0;
            goto end;
        }

        ret->numIndexes = count;
        get_indices_from_reply( reply, pid, seq, skip_loopback, ret );
        *table = ret;
    }

end:
    free_netlink_reply( reply );
    close( fd );
    return count;
}

#else
DWORD get_interface_indices( BOOL skip_loopback, InterfaceIndexTable **table )
{
    if (table) *table = NULL;
    return 0;
}
#endif
