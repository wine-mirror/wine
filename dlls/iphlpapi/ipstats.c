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
#include <sys/types.h>
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

#ifndef ROUNDUP
#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#endif
#ifndef ADVANCE
#define ADVANCE(x, n) (x += ROUNDUP(((struct sockaddr *)n)->sa_len))
#endif

#include "windef.h"
#include "winbase.h"
#include "iprtrmib.h"
#include "ifenum.h"
#include "ipstats.h"

#include "wine/debug.h"

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

DWORD getInterfaceStatsByName(const char *name, PMIB_IFROW entry)
{
#if defined(HAVE_SYS_SYSCTL_H) && defined(NET_RT_IFLIST)
  int mib[] = {CTL_NET, PF_ROUTE, 0, AF_INET, NET_RT_IFLIST, if_nametoindex(name)};
#define MIB_LEN (sizeof(mib) / sizeof(mib[0]))

  size_t needed;
  char *buf, *end;
  struct if_msghdr *ifm;
  struct if_data ifdata;
  if (!name || !entry)
    return ERROR_INVALID_PARAMETER;

  if(sysctl(mib, MIB_LEN, NULL, &needed, NULL, 0) == -1)
  {
      ERR ("failed to get size of iflist\n");
      return ERROR_NOT_SUPPORTED;
  }
  buf = HeapAlloc (GetProcessHeap (), 0, needed);
  if (!buf) return ERROR_NOT_SUPPORTED;
  if(sysctl(mib, MIB_LEN, buf, &needed, NULL, 0) == -1)
  {
      ERR ("failed to get iflist\n");
      HeapFree (GetProcessHeap (), 0, buf);
      return ERROR_NOT_SUPPORTED;
  }
  else
      for ( end = buf + needed; buf < end; buf += ifm->ifm_msglen)
      {
          ifm = (struct if_msghdr *) buf;
          if(ifm->ifm_type == RTM_IFINFO && ifm->ifm_data.ifi_type == IFT_ETHER)
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
              HeapFree (GetProcessHeap (), 0, buf);
              return NO_ERROR;
          }
      }
      HeapFree (GetProcessHeap (), 0, buf);
      return ERROR_NOT_SUPPORTED;
#else
  /* get interface stats from /proc/net/dev, no error if can't
     no inUnknownProtos, outNUcastPkts, outQLen */
  FILE *fp;

  if (!name || !entry)
    return ERROR_INVALID_PARAMETER;
  fp = fopen("/proc/net/dev", "r");
  if (fp) {
    char buf[512] = { 0 }, *ptr;
    int nameLen = strlen(name), nameFound = 0;


    ptr = fgets(buf, sizeof(buf), fp);
    while (ptr && !nameFound) {
      while (*ptr && isspace(*ptr))
        ptr++;
      if (strncasecmp(ptr, name, nameLen) == 0 && *(ptr + nameLen) == ':')
        nameFound = 1;
      else
        ptr = fgets(buf, sizeof(buf), fp);
    }
    if (nameFound) {
      char *endPtr;

      ptr += nameLen + 1;
      if (ptr && *ptr) {
        entry->dwInOctets = strtoul(ptr, &endPtr, 10);
        ptr = endPtr;
      }
      if (ptr && *ptr) {
        entry->dwInUcastPkts = strtoul(ptr, &endPtr, 10);
        ptr = endPtr;
      }
      if (ptr && *ptr) {
        entry->dwInErrors = strtoul(ptr, &endPtr, 10);
        ptr = endPtr;
      }
      if (ptr && *ptr) {
        entry->dwInDiscards = strtoul(ptr, &endPtr, 10);
        ptr = endPtr;
      }
      if (ptr && *ptr) {
        strtoul(ptr, &endPtr, 10); /* skip */
        ptr = endPtr;
      }
      if (ptr && *ptr) {
        strtoul(ptr, &endPtr, 10); /* skip */
        ptr = endPtr;
      }
      if (ptr && *ptr) {
        strtoul(ptr, &endPtr, 10); /* skip */
        ptr = endPtr;
      }
      if (ptr && *ptr) {
        entry->dwInNUcastPkts = strtoul(ptr, &endPtr, 10);
        ptr = endPtr;
      }
      if (ptr && *ptr) {
        entry->dwOutOctets = strtoul(ptr, &endPtr, 10);
        ptr = endPtr;
      }
      if (ptr && *ptr) {
        entry->dwOutUcastPkts = strtoul(ptr, &endPtr, 10);
        ptr = endPtr;
      }
      if (ptr && *ptr) {
        entry->dwOutErrors = strtoul(ptr, &endPtr, 10);
        ptr = endPtr;
      }
      if (ptr && *ptr) {
        entry->dwOutDiscards = strtoul(ptr, &endPtr, 10);
        ptr = endPtr;
      }
    }
    fclose(fp);
  }
  else
  {
     ERR ("unimplemented!\n");
     return ERROR_NOT_SUPPORTED;
  }

  return NO_ERROR;
#endif
}

DWORD getICMPStats(MIB_ICMP *stats)
{
#if defined(HAVE_SYS_SYSCTL_H) && defined(ICMPCTL_STATS)
  int mib[] = {CTL_NET, PF_INET, IPPROTO_ICMP, ICMPCTL_STATS};
#define MIB_LEN (sizeof(mib) / sizeof(mib[0]))
  size_t needed;
  struct icmpstat icmp_stat;
  int i;

  if (!stats)
    return ERROR_INVALID_PARAMETER;

  needed = sizeof(icmp_stat);
  if(sysctl(mib, MIB_LEN, &icmp_stat, &needed, NULL, 0) == -1)
  {
      ERR ("failed to get icmpstat\n");
      return ERROR_NOT_SUPPORTED;
  }


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

#ifdef HAVE_ICPS_OUTHIST
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
#else /* ICPS_OUTHIST */
  memset( &stats->stats.icmpOutStats, 0, sizeof(stats->stats.icmpOutStats) );
#endif /* ICPS_OUTHIST */

  return NO_ERROR;

#else /* ICMPCTL_STATS */
  FILE *fp;

  if (!stats)
    return ERROR_INVALID_PARAMETER;

  memset(stats, 0, sizeof(MIB_ICMP));
  /* get most of these stats from /proc/net/snmp, no error if can't */
  fp = fopen("/proc/net/snmp", "r");
  if (fp) {
    static const char hdr[] = "Icmp:";
    char buf[512] = { 0 }, *ptr;

    do {
      ptr = fgets(buf, sizeof(buf), fp);
    } while (ptr && strncasecmp(buf, hdr, sizeof(hdr) - 1));
    if (ptr) {
      /* last line was a header, get another */
      ptr = fgets(buf, sizeof(buf), fp);
      if (ptr && strncasecmp(buf, hdr, sizeof(hdr) - 1) == 0) {
        char *endPtr;

        ptr += sizeof(hdr);
        if (ptr && *ptr) {
          stats->stats.icmpInStats.dwMsgs = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpInStats.dwErrors = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpInStats.dwDestUnreachs = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpInStats.dwTimeExcds = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpInStats.dwParmProbs = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpInStats.dwSrcQuenchs = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpInStats.dwRedirects = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpInStats.dwEchoReps = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpInStats.dwTimestamps = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpInStats.dwTimestampReps = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpInStats.dwAddrMasks = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpInStats.dwAddrMaskReps = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpOutStats.dwMsgs = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpOutStats.dwErrors = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpOutStats.dwDestUnreachs = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpOutStats.dwTimeExcds = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpOutStats.dwParmProbs = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpOutStats.dwSrcQuenchs = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpOutStats.dwRedirects = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpOutStats.dwEchoReps = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpOutStats.dwTimestamps = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpOutStats.dwTimestampReps = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpOutStats.dwAddrMasks = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpOutStats.dwAddrMaskReps = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
      }
    }
    fclose(fp);
  }
  else
  {
     ERR ("unimplemented!\n");
     return ERROR_NOT_SUPPORTED;
  }

  return NO_ERROR;
#endif
}

DWORD getIPStats(PMIB_IPSTATS stats)
{
#if defined(HAVE_SYS_SYSCTL_H) && defined(IPCTL_STATS)
  int mib[] = {CTL_NET, PF_INET, IPPROTO_IP, IPCTL_STATS};
#define MIB_LEN (sizeof(mib) / sizeof(mib[0]))
  int ip_ttl, ip_forwarding;
  struct ipstat ip_stat;
  size_t needed;

  if (!stats)
      return ERROR_INVALID_PARAMETER;

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

  stats->dwForwarding = ip_forwarding;
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

  return NO_ERROR;
#else
  FILE *fp;

  if (!stats)
    return ERROR_INVALID_PARAMETER;

  memset(stats, 0, sizeof(MIB_IPSTATS));
  stats->dwNumIf = stats->dwNumAddr = getNumInterfaces();
  stats->dwNumRoutes = getNumRoutes();

  /* get most of these stats from /proc/net/snmp, no error if can't */
  fp = fopen("/proc/net/snmp", "r");
  if (fp) {
    static const char hdr[] = "Ip:";
    char buf[512] = { 0 }, *ptr;

    do {
      ptr = fgets(buf, sizeof(buf), fp);
    } while (ptr && strncasecmp(buf, hdr, sizeof(hdr) - 1));
    if (ptr) {
      /* last line was a header, get another */
      ptr = fgets(buf, sizeof(buf), fp);
      if (ptr && strncasecmp(buf, hdr, sizeof(hdr) - 1) == 0) {
        char *endPtr;

        ptr += sizeof(hdr);
        if (ptr && *ptr) {
          stats->dwForwarding = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwDefaultTTL = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwInReceives = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwInHdrErrors = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwInAddrErrors = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwForwDatagrams = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwInUnknownProtos = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwInDiscards = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwInDelivers = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwOutRequests = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwOutDiscards = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwOutNoRoutes = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwReasmTimeout = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwReasmReqds = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwReasmOks = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwReasmFails = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwFragOks = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwFragFails = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwFragCreates = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        /* hmm, no routingDiscards */
      }
    }
    fclose(fp);
  }
  else
  {
     ERR ("unimplemented!\n");
     return ERROR_NOT_SUPPORTED;
  }

  return NO_ERROR;
#endif
}

DWORD getTCPStats(MIB_TCPSTATS *stats)
{
#if defined(HAVE_SYS_SYSCTL_H) && defined(UDPCTL_STATS)
#ifndef TCPTV_MIN  /* got removed in Mac OS X for some reason */
#define TCPTV_MIN 2
#define TCPTV_REXMTMAX 128
#endif
  int mib[] = {CTL_NET, PF_INET, IPPROTO_TCP, TCPCTL_STATS};
#define MIB_LEN (sizeof(mib) / sizeof(mib[0]))
#define hz 1000
  struct tcpstat tcp_stat;
  size_t needed;

  if (!stats)
    return ERROR_INVALID_PARAMETER;
  needed = sizeof(tcp_stat);

  if(sysctl(mib, MIB_LEN, &tcp_stat, &needed, NULL, 0) == -1)
  {
      ERR ("failed to get tcpstat\n");
      return ERROR_NOT_SUPPORTED;
  }

  stats->dwRtoAlgorithm = MIB_TCP_RTO_VANJ;
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

  return NO_ERROR;

#else
  FILE *fp;

  if (!stats)
    return ERROR_INVALID_PARAMETER;

  memset(stats, 0, sizeof(MIB_TCPSTATS));

  /* get from /proc/net/snmp, no error if can't */
  fp = fopen("/proc/net/snmp", "r");
  if (fp) {
    static const char hdr[] = "Tcp:";
    char buf[512] = { 0 }, *ptr;


    do {
      ptr = fgets(buf, sizeof(buf), fp);
    } while (ptr && strncasecmp(buf, hdr, sizeof(hdr) - 1));
    if (ptr) {
      /* last line was a header, get another */
      ptr = fgets(buf, sizeof(buf), fp);
      if (ptr && strncasecmp(buf, hdr, sizeof(hdr) - 1) == 0) {
        char *endPtr;

        ptr += sizeof(hdr);
        if (ptr && *ptr) {
          stats->dwRtoAlgorithm = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwRtoMin = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwRtoMax = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwMaxConn = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwActiveOpens = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwPassiveOpens = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwAttemptFails = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwEstabResets = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwCurrEstab = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwInSegs = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwOutSegs = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwRetransSegs = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwInErrs = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwOutRsts = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        stats->dwNumConns = getNumTcpEntries();
      }
    }
    fclose(fp);
  }
  else
  {
     ERR ("unimplemented!\n");
     return ERROR_NOT_SUPPORTED;
  }

  return NO_ERROR;
#endif
}

DWORD getUDPStats(MIB_UDPSTATS *stats)
{
#if defined(HAVE_SYS_SYSCTL_H) && defined(UDPCTL_STATS)
  int mib[] = {CTL_NET, PF_INET, IPPROTO_UDP, UDPCTL_STATS};
#define MIB_LEN (sizeof(mib) / sizeof(mib[0]))
  struct udpstat udp_stat;
  MIB_UDPTABLE *udp_table;
  size_t needed;
  if (!stats)
      return ERROR_INVALID_PARAMETER;

  needed = sizeof(udp_stat);

  if(sysctl(mib, MIB_LEN, &udp_stat, &needed, NULL, 0) == -1)
  {
      ERR ("failed to get udpstat\n");
      return ERROR_NOT_SUPPORTED;
  }

  stats->dwInDatagrams = udp_stat.udps_ipackets;
  stats->dwOutDatagrams = udp_stat.udps_opackets;
  stats->dwNoPorts = udp_stat.udps_noport;
  stats->dwInErrors = udp_stat.udps_hdrops + udp_stat.udps_badsum + udp_stat.udps_fullsock + udp_stat.udps_badlen;
  if (!AllocateAndGetUdpTableFromStack( &udp_table, FALSE, GetProcessHeap(), 0 ))
  {
      stats->dwNumAddrs = udp_table->dwNumEntries;
      HeapFree( GetProcessHeap(), 0, udp_table );
  }
  else stats->dwNumAddrs = 0;

  return NO_ERROR;
#else
  FILE *fp;

  if (!stats)
    return ERROR_INVALID_PARAMETER;

  memset(stats, 0, sizeof(MIB_UDPSTATS));

  /* get from /proc/net/snmp, no error if can't */
  fp = fopen("/proc/net/snmp", "r");
  if (fp) {
    static const char hdr[] = "Udp:";
    char buf[512] = { 0 }, *ptr;


    do {
      ptr = fgets(buf, sizeof(buf), fp);
    } while (ptr && strncasecmp(buf, hdr, sizeof(hdr) - 1));
    if (ptr) {
      /* last line was a header, get another */
      ptr = fgets(buf, sizeof(buf), fp);
      if (ptr && strncasecmp(buf, hdr, sizeof(hdr) - 1) == 0) {
        char *endPtr;

        ptr += sizeof(hdr);
        if (ptr && *ptr) {
          stats->dwInDatagrams = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwNoPorts = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwInErrors = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwOutDatagrams = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->dwNumAddrs = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
      }
    }
    fclose(fp);
  }
  else
  {
     ERR ("unimplemented!\n");
     return ERROR_NOT_SUPPORTED;
  }

  return NO_ERROR;
#endif
}

static DWORD getNumWithOneHeader(const char *filename)
{
#if defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_STRUCT_XINPGEN)
   size_t Len = 0;
   char *Buf;
   struct xinpgen *pXIG, *pOrigXIG;
   int Protocol;
   DWORD NumEntries = 0;

   if (!strcmp (filename, "net.inet.tcp.pcblist"))
      Protocol = IPPROTO_TCP;
   else if (!strcmp (filename, "net.inet.udp.pcblist"))
      Protocol = IPPROTO_UDP;
   else
   {
      ERR ("Unsupported mib '%s', needs protocol mapping\n",
           filename);
      return 0;
   }

   if (sysctlbyname (filename, NULL, &Len, NULL, 0) < 0)
   {
      WARN ("Unable to read '%s' via sysctlbyname\n", filename);
      return 0;
   }

   Buf = HeapAlloc (GetProcessHeap (), 0, Len);
   if (!Buf)
   {
      ERR ("Out of memory!\n");
      return 0;
   }

   if (sysctlbyname (filename, Buf, &Len, NULL, 0) < 0)
   {
      ERR ("Failure to read '%s' via sysctlbyname!\n", filename);
      HeapFree (GetProcessHeap (), 0, Buf);
      return 0;
   }

   /* Might be nothing here; first entry is just a header it seems */
   if (Len <= sizeof (struct xinpgen))
   {
      HeapFree (GetProcessHeap (), 0, Buf);
      return 0;
   }

   pOrigXIG = (struct xinpgen *)Buf;
   pXIG = pOrigXIG;

   for (pXIG = (struct xinpgen *)((char *)pXIG + pXIG->xig_len);
        pXIG->xig_len > sizeof (struct xinpgen);
        pXIG = (struct xinpgen *)((char *)pXIG + pXIG->xig_len))
   {
      struct tcpcb *pTCPData = NULL;
      struct inpcb *pINData;
      struct xsocket *pSockData;

      if (Protocol == IPPROTO_TCP)
      {
         pTCPData = &((struct xtcpcb *)pXIG)->xt_tp;
         pINData = &((struct xtcpcb *)pXIG)->xt_inp;
         pSockData = &((struct xtcpcb *)pXIG)->xt_socket;
      }
      else
      {
         pINData = &((struct xinpcb *)pXIG)->xi_inp;
         pSockData = &((struct xinpcb *)pXIG)->xi_socket;
      }

      /* Ignore sockets for other protocols */
      if (pSockData->xso_protocol != Protocol)
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

      NumEntries++;
   }

   HeapFree (GetProcessHeap (), 0, Buf);
   return NumEntries;
#else
  FILE *fp;
  int ret = 0;

  fp = fopen(filename, "r");
  if (fp) {
    char buf[512] = { 0 }, *ptr;


    ptr = fgets(buf, sizeof(buf), fp);
    if (ptr) {
      do {
        ptr = fgets(buf, sizeof(buf), fp);
        if (ptr)
          ret++;
      } while (ptr);
    }
    fclose(fp);
  }
  else
     ERR ("Unable to open '%s' to count entries!\n", filename);

  return ret;
#endif
}

DWORD getNumRoutes(void)
{
#if defined(HAVE_SYS_SYSCTL_H) && defined(NET_RT_DUMP)
   int mib[6] = {CTL_NET, PF_ROUTE, 0, PF_INET, NET_RT_DUMP, 0};
   size_t needed;
   char *buf, *lim, *next;
   struct rt_msghdr *rtm;
   DWORD RouteCount = 0;

   if (sysctl (mib, 6, NULL, &needed, NULL, 0) < 0)
   {
      ERR ("sysctl 1 failed!\n");
      return 0;
   }

   buf = HeapAlloc (GetProcessHeap (), 0, needed);
   if (!buf) return 0;

   if (sysctl (mib, 6, buf, &needed, NULL, 0) < 0)
   {
      ERR ("sysctl 2 failed!\n");
      HeapFree (GetProcessHeap (), 0, buf);
      return 0;
   }

   lim = buf + needed;
   for (next = buf; next < lim; next += rtm->rtm_msglen)
   {
      rtm = (struct rt_msghdr *)next;

      if (rtm->rtm_type != RTM_GET)
      {
         WARN ("Got unexpected message type 0x%x!\n",
               rtm->rtm_type);
         continue;
      }

      /* Ignore all entries except for gateway routes which aren't
         multicast */
      if (!(rtm->rtm_flags & RTF_GATEWAY) || (rtm->rtm_flags & RTF_MULTICAST))
         continue;

      RouteCount++;
   }

   HeapFree (GetProcessHeap (), 0, buf);
   return RouteCount;
#else
   return getNumWithOneHeader("/proc/net/route");
#endif
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
    if ((ret = rowA->dwForwardProto - rowB->dwForwardProto) != 0) return ret;
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

                if (!(flags & RTF_UP)) row.dwForwardType = MIB_IPROUTE_TYPE_INVALID;
                else if (flags & RTF_GATEWAY) row.dwForwardType = MIB_IPROUTE_TYPE_INDIRECT;
                else row.dwForwardType = MIB_IPROUTE_TYPE_DIRECT;

                strtoul(ptr + 1, &ptr, 16); /* refcount, skip */
                strtoul(ptr + 1, &ptr, 16); /* use, skip */
                row.dwForwardMetric1 = strtoul(ptr + 1, &ptr, 16);
                row.dwForwardMask = strtoul(ptr + 1, &ptr, 16);
                /* FIXME: other protos might be appropriate, e.g. the default
                 * route is typically set with MIB_IPPROTO_NETMGMT instead */
                row.dwForwardProto = MIB_IPPROTO_LOCAL;

                if (!(table = append_ipforward_row( heap, flags, table, &count, &row )))
                    break;
            }
            fclose(fp);
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
          row.dwForwardType = MIB_IPROUTE_TYPE_INDIRECT;
          row.dwForwardMetric1 = rtm->rtm_rmx.rmx_hopcount;
          row.dwForwardProto = MIB_IPPROTO_LOCAL;

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
                if (flags & ATF_COM) row.dwType = MIB_IPNET_TYPE_DYNAMIC;
                else
#endif
#ifdef ATF_PERM
                if (flags & ATF_PERM) row.dwType = MIB_IPNET_TYPE_STATIC;
                else
#endif
                    row.dwType = MIB_IPNET_TYPE_OTHER;

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
              if(rtm->rtm_rmx.rmx_expire == 0) row.dwType = MIB_IPNET_TYPE_STATIC;
              else if(sinarp->sin_other & SIN_PROXY) row.dwType = MIB_IPNET_TYPE_OTHER;
              else if(rtm->rtm_rmx.rmx_expire != 0) row.dwType = MIB_IPNET_TYPE_DYNAMIC;
              else row.dwType = MIB_IPNET_TYPE_INVALID;

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


DWORD getNumTcpEntries(void)
{
#if defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_NETINET_IN_PCB_H)
   return getNumWithOneHeader ("net.inet.tcp.pcblist");
#else
   return getNumWithOneHeader ("/proc/net/tcp");
#endif
}

static MIB_TCPTABLE *append_tcp_row( HANDLE heap, DWORD flags, MIB_TCPTABLE *table,
                                     DWORD *count, const MIB_TCPROW *row )
{
    if (table->dwNumEntries >= *count)
    {
        MIB_TCPTABLE *new_table;
        DWORD new_count = table->dwNumEntries * 2;

        if (!(new_table = HeapReAlloc( heap, flags, table, FIELD_OFFSET(MIB_TCPTABLE, table[new_count] ))))
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


/* Why not a lookup table? Because the TCPS_* constants are different
   on different platforms */
static DWORD TCPStateToMIBState (int state)
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
                                              HANDLE heap, DWORD flags)
{
    MIB_TCPTABLE *table;
    MIB_TCPROW row;
    DWORD ret = NO_ERROR, count = 16;

    TRACE("table %p, bOrder %d, heap %p, flags 0x%08x\n", ppTcpTable, bOrder, heap, flags);

    if (!ppTcpTable) return ERROR_INVALID_PARAMETER;

    if (!(table = HeapAlloc( heap, flags, FIELD_OFFSET(MIB_TCPTABLE, table[count] ))))
        return ERROR_OUTOFMEMORY;

    table->dwNumEntries = 0;

#ifdef __linux__
    {
        FILE *fp;

        if ((fp = fopen("/proc/net/tcp", "r")))
        {
            char buf[512], *ptr;
            DWORD dummy;

            /* skip header line */
            ptr = fgets(buf, sizeof(buf), fp);
            while ((ptr = fgets(buf, sizeof(buf), fp)))
            {
                if (sscanf( ptr, "%x: %x:%x %x:%x %x", &dummy, &row.dwLocalAddr, &row.dwLocalPort,
                            &row.dwRemoteAddr, &row.dwRemotePort, &row.dwState ) != 6)
                    continue;
                row.dwLocalPort = htons( row.dwLocalPort );
                row.dwRemotePort = htons( row.dwRemotePort );
                row.dwState = TCPStateToMIBState( row.dwState );
                if (!(table = append_tcp_row( heap, flags, table, &count, &row )))
                    break;
            }
            fclose( fp );
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
            if (!(table = append_tcp_row( heap, flags, table, &count, &row ))) break;
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
            qsort( table->table, table->dwNumEntries, sizeof(row), compare_tcp_rows );
        *ppTcpTable = table;
    }
    else HeapFree( heap, flags, table );
    TRACE( "returning ret %u table %p\n", ret, table );
    return ret;
}
