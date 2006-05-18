/* Copyright (C) 2003,2006 Juan Lang
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
 *
 * This file implements statistics getting using the /proc filesystem exported
 * by Linux, and maybe other OSes.
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif
#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#ifdef HAVE_NETINET_TCP_FSM_H
#include <netinet/tcp_fsm.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "iprtrmib.h"
#include "ifenum.h"
#include "ipstats.h"

#ifdef linux
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

DWORD getInterfaceStatsByName(const char *name, PMIB_IFROW entry)
{
  FILE *fp;

  if (!name)
    return ERROR_INVALID_PARAMETER;
  if (!entry)
    return ERROR_INVALID_PARAMETER;

  /* get interface stats from /proc/net/dev, no error if can't
     no inUnknownProtos, outNUcastPkts, outQLen */
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
  return NO_ERROR;
}

DWORD getICMPStats(MIB_ICMP *stats)
{
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
  return NO_ERROR;
}

DWORD getIPStats(PMIB_IPSTATS stats)
{
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
  return NO_ERROR;
}

DWORD getTCPStats(MIB_TCPSTATS *stats)
{
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
          stats->dwRtoMin = strtoul(ptr, &endPtr, 10);
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
  return NO_ERROR;
}

DWORD getUDPStats(MIB_UDPSTATS *stats)
{
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
  return NO_ERROR;
}

static DWORD getNumWithOneHeader(const char *filename)
{
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
  return ret;
}

DWORD getNumRoutes(void)
{
  return getNumWithOneHeader("/proc/net/route");
}

DWORD getRouteTable(PMIB_IPFORWARDTABLE *ppIpForwardTable, HANDLE heap,
 DWORD flags)
{
  DWORD ret;

  if (!ppIpForwardTable)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD numRoutes = getNumRoutes();
    PMIB_IPFORWARDTABLE table = HeapAlloc(heap, flags,
     sizeof(MIB_IPFORWARDTABLE) + (numRoutes - 1) * sizeof(MIB_IPFORWARDROW));

    if (table) {
      FILE *fp;

      ret = NO_ERROR;
      *ppIpForwardTable = table;
      table->dwNumEntries = 0;
      /* get from /proc/net/route, no error if can't */
      fp = fopen("/proc/net/route", "r");
      if (fp) {
        char buf[512] = { 0 }, *ptr;

        /* skip header line */
        ptr = fgets(buf, sizeof(buf), fp);
        while (ptr && table->dwNumEntries < numRoutes) {
          memset(&table->table[table->dwNumEntries], 0,
           sizeof(MIB_IPFORWARDROW));
          ptr = fgets(buf, sizeof(buf), fp);
          if (ptr) {
            DWORD index;

            while (!isspace(*ptr))
              ptr++;
            *ptr = '\0';
            ptr++;
            if (getInterfaceIndexByName(buf, &index) == NO_ERROR) {
              char *endPtr;

              table->table[table->dwNumEntries].dwForwardIfIndex = index;
              if (*ptr) {
                table->table[table->dwNumEntries].dwForwardDest =
                 strtoul(ptr, &endPtr, 16);
                ptr = endPtr;
              }
              if (ptr && *ptr) {
                table->table[table->dwNumEntries].dwForwardNextHop =
                 strtoul(ptr, &endPtr, 16);
                ptr = endPtr;
              }
              if (ptr && *ptr) {
                DWORD flags = strtoul(ptr, &endPtr, 16);

                if (!(flags & RTF_UP))
                  table->table[table->dwNumEntries].dwForwardType =
                   MIB_IPROUTE_TYPE_INVALID;
                else if (flags & RTF_GATEWAY)
                  table->table[table->dwNumEntries].dwForwardType =
                   MIB_IPROUTE_TYPE_INDIRECT;
                else
                  table->table[table->dwNumEntries].dwForwardType =
                   MIB_IPROUTE_TYPE_DIRECT;
                ptr = endPtr;
              }
              if (ptr && *ptr) {
                strtoul(ptr, &endPtr, 16); /* refcount, skip */
                ptr = endPtr;
              }
              if (ptr && *ptr) {
                strtoul(ptr, &endPtr, 16); /* use, skip */
                ptr = endPtr;
              }
              if (ptr && *ptr) {
                table->table[table->dwNumEntries].dwForwardMetric1 =
                 strtoul(ptr, &endPtr, 16);
                ptr = endPtr;
              }
              if (ptr && *ptr) {
                table->table[table->dwNumEntries].dwForwardMask =
                 strtoul(ptr, &endPtr, 16);
                ptr = endPtr;
              }
              /* FIXME: other protos might be appropriate, e.g. the default
               * route is typically set with MIB_IPPROTO_NETMGMT instead */
              table->table[table->dwNumEntries].dwForwardProto =
               MIB_IPPROTO_LOCAL;
              table->dwNumEntries++;
            }
          }
        }
        fclose(fp);
      }
    }
    else
      ret = ERROR_OUTOFMEMORY;
  }
  return ret;
}

DWORD getNumArpEntries(void)
{
  return getNumWithOneHeader("/proc/net/arp");
}

DWORD getArpTable(PMIB_IPNETTABLE *ppIpNetTable, HANDLE heap, DWORD flags)
{
  DWORD ret;

  if (!ppIpNetTable)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD numEntries = getNumArpEntries();
    PMIB_IPNETTABLE table = HeapAlloc(heap, flags,
     sizeof(MIB_IPNETTABLE) + (numEntries - 1) * sizeof(MIB_IPNETROW));

    if (table) {
      FILE *fp;

      ret = NO_ERROR;
      *ppIpNetTable = table;
      table->dwNumEntries = 0;
      /* get from /proc/net/arp, no error if can't */
      fp = fopen("/proc/net/arp", "r");
      if (fp) {
        char buf[512] = { 0 }, *ptr;

        /* skip header line */
        ptr = fgets(buf, sizeof(buf), fp);
        while (ptr && table->dwNumEntries < numEntries) {
          ptr = fgets(buf, sizeof(buf), fp);
          if (ptr) {
            char *endPtr;

            memset(&table->table[table->dwNumEntries], 0, sizeof(MIB_IPNETROW));
            table->table[table->dwNumEntries].dwAddr = inet_addr(ptr);
            while (ptr && *ptr && !isspace(*ptr))
              ptr++;

            if (ptr && *ptr) {
              strtoul(ptr, &endPtr, 16); /* hw type (skip) */
              ptr = endPtr;
            }
            if (ptr && *ptr) {
              DWORD flags = strtoul(ptr, &endPtr, 16);

#ifdef ATF_COM
              if (flags & ATF_COM)
                table->table[table->dwNumEntries].dwType =
                 MIB_IPNET_TYPE_DYNAMIC;
              else
#endif
#ifdef ATF_PERM
              if (flags & ATF_PERM)
                table->table[table->dwNumEntries].dwType =
                 MIB_IPNET_TYPE_STATIC;
              else
#endif
                table->table[table->dwNumEntries].dwType = MIB_IPNET_TYPE_OTHER;

              ptr = endPtr;
            }
            while (ptr && *ptr && isspace(*ptr))
              ptr++;
            while (ptr && *ptr && !isspace(*ptr)) {
              DWORD byte = strtoul(ptr, &endPtr, 16);

              if (endPtr && *endPtr) {
                endPtr++;
                table->table[table->dwNumEntries].bPhysAddr[
                 table->table[table->dwNumEntries].dwPhysAddrLen++] =
                 byte & 0x0ff;
              }
              ptr = endPtr;
            }
            if (ptr && *ptr) {
              strtoul(ptr, &endPtr, 16); /* mask (skip) */
              ptr = endPtr;
            }
            getInterfaceIndexByName(ptr,
             &table->table[table->dwNumEntries].dwIndex);
            table->dwNumEntries++;
          }
        }
        fclose(fp);
      }
    }
    else
      ret = ERROR_OUTOFMEMORY;
  }
  return ret;
}

DWORD getNumUdpEntries(void)
{
  return getNumWithOneHeader("/proc/net/udp");
}

DWORD getUdpTable(PMIB_UDPTABLE *ppUdpTable, HANDLE heap, DWORD flags)
{
  DWORD ret;

  if (!ppUdpTable)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD numEntries = getNumUdpEntries();
    PMIB_UDPTABLE table = HeapAlloc(heap, flags,
     sizeof(MIB_UDPTABLE) + (numEntries - 1) * sizeof(MIB_UDPROW));

    if (table) {
      FILE *fp;

      ret = NO_ERROR;
      *ppUdpTable = table;
      table->dwNumEntries = 0;
      /* get from /proc/net/udp, no error if can't */
      fp = fopen("/proc/net/udp", "r");
      if (fp) {
        char buf[512] = { 0 }, *ptr;

        /* skip header line */
        ptr = fgets(buf, sizeof(buf), fp);
        while (ptr && table->dwNumEntries < numEntries) {
          memset(&table->table[table->dwNumEntries], 0, sizeof(MIB_UDPROW));
          ptr = fgets(buf, sizeof(buf), fp);
          if (ptr) {
            char *endPtr;

            if (ptr && *ptr) {
              strtoul(ptr, &endPtr, 16); /* skip */
              ptr = endPtr;
            }
            if (ptr && *ptr) {
              ptr++;
              table->table[table->dwNumEntries].dwLocalAddr = strtoul(ptr,
               &endPtr, 16);
              ptr = endPtr;
            }
            if (ptr && *ptr) {
              ptr++;
              table->table[table->dwNumEntries].dwLocalPort = strtoul(ptr,
               &endPtr, 16);
              ptr = endPtr;
            }
            table->dwNumEntries++;
          }
        }
        fclose(fp);
      }
    }
    else
      ret = ERROR_OUTOFMEMORY;
  }
  return ret;
}

DWORD getNumTcpEntries(void)
{
  return getNumWithOneHeader("/proc/net/tcp");
}

DWORD getTcpTable(PMIB_TCPTABLE *ppTcpTable, HANDLE heap, DWORD flags)
{
  DWORD ret;

  if (!ppTcpTable)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD numEntries = getNumTcpEntries();
    PMIB_TCPTABLE table = HeapAlloc(heap, flags,
     sizeof(MIB_TCPTABLE) + (numEntries - 1) * sizeof(MIB_TCPROW));

    if (table) {
      FILE *fp;

      ret = NO_ERROR;
      *ppTcpTable = table;
      table->dwNumEntries = 0;
      /* get from /proc/net/tcp, no error if can't */
      fp = fopen("/proc/net/tcp", "r");
      if (fp) {
        char buf[512] = { 0 }, *ptr;

        /* skip header line */
        ptr = fgets(buf, sizeof(buf), fp);
        while (ptr && table->dwNumEntries < numEntries) {
          memset(&table->table[table->dwNumEntries], 0, sizeof(MIB_TCPROW));
          ptr = fgets(buf, sizeof(buf), fp);
          if (ptr) {
            char *endPtr;

            while (ptr && *ptr && *ptr != ':')
              ptr++;
            if (ptr && *ptr)
              ptr++;
            if (ptr && *ptr) {
              table->table[table->dwNumEntries].dwLocalAddr = strtoul(ptr,
               &endPtr, 16);
              ptr = endPtr;
            }
            if (ptr && *ptr) {
              ptr++;
              table->table[table->dwNumEntries].dwLocalPort = strtoul(ptr,
               &endPtr, 16);
              ptr = endPtr;
            }
            if (ptr && *ptr) {
              table->table[table->dwNumEntries].dwRemoteAddr = strtoul(ptr,
               &endPtr, 16);
              ptr = endPtr;
            }
            if (ptr && *ptr) {
              ptr++;
              table->table[table->dwNumEntries].dwRemotePort = strtoul(ptr,
               &endPtr, 16);
              ptr = endPtr;
            }
            if (ptr && *ptr) {
              DWORD state = strtoul(ptr, &endPtr, 16);

              switch (state)
              {
                case TCPS_ESTABLISHED:
                  table->table[table->dwNumEntries].dwState =
                   MIB_TCP_STATE_ESTAB;
                  break;
                case TCPS_SYN_SENT:
                  table->table[table->dwNumEntries].dwState =
                   MIB_TCP_STATE_SYN_SENT;
                  break;
                case TCPS_SYN_RECEIVED:
                  table->table[table->dwNumEntries].dwState =
                   MIB_TCP_STATE_SYN_RCVD;
                  break;
                case TCPS_FIN_WAIT_1:
                  table->table[table->dwNumEntries].dwState =
                   MIB_TCP_STATE_FIN_WAIT1;
                  break;
                case TCPS_FIN_WAIT_2:
                  table->table[table->dwNumEntries].dwState =
                   MIB_TCP_STATE_FIN_WAIT2;
                  break;
                case TCPS_TIME_WAIT:
                  table->table[table->dwNumEntries].dwState =
                   MIB_TCP_STATE_TIME_WAIT;
                  break;
                case TCPS_CLOSED:
                  table->table[table->dwNumEntries].dwState =
                   MIB_TCP_STATE_CLOSED;
                  break;
                case TCPS_CLOSE_WAIT:
                  table->table[table->dwNumEntries].dwState =
                   MIB_TCP_STATE_CLOSE_WAIT;
                  break;
                case TCPS_LAST_ACK:
                  table->table[table->dwNumEntries].dwState =
                   MIB_TCP_STATE_LAST_ACK;
                  break;
                case TCPS_LISTEN:
                  table->table[table->dwNumEntries].dwState =
                   MIB_TCP_STATE_LISTEN;
                  break;
                case TCPS_CLOSING:
                  table->table[table->dwNumEntries].dwState =
                   MIB_TCP_STATE_CLOSING;
                  break;
              }
              ptr = endPtr;
            }
            table->dwNumEntries++;
          }
        }
        fclose(fp);
      }
    }
    else
      ret = ERROR_OUTOFMEMORY;
  }
  return ret;
}
