/* ipstats.h
 * Copyright (C) 2003,2006 Juan Lang
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
 * This module implements functions that get network-related statistics.
 * It's meant to hide some platform-specificisms.
 */
#ifndef WINE_IPSTATS_H_
#define WINE_IPSTATS_H_

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "iprtrmib.h"

/* Fills in entry's interface stats, using name to find them.
 * Returns ERROR_INVALID_PARAMETER if name or entry is NULL, NO_ERROR otherwise.
 */
DWORD getInterfaceStatsByName(const char *name, PMIB_IFROW entry);

/* Gets ICMP statistics into stats.  Returns ERROR_INVALID_PARAMETER if stats is
 * NULL, NO_ERROR otherwise.
 */
DWORD getICMPStats(MIB_ICMP *stats);

/* Gets IP statistics into stats.  Returns ERROR_INVALID_PARAMETER if stats is
 * NULL, NO_ERROR otherwise.
 */
DWORD getIPStats(PMIB_IPSTATS stats);

/* Gets TCP statistics into stats.  Returns ERROR_INVALID_PARAMETER if stats is
 * NULL, NO_ERROR otherwise.
 */
DWORD getTCPStats(MIB_TCPSTATS *stats);

/* Gets UDP statistics into stats.  Returns ERROR_INVALID_PARAMETER if stats is
 * NULL, NO_ERROR otherwise.
 */
DWORD getUDPStats(MIB_UDPSTATS *stats);

/* Returns the number of entries in the route table. */
DWORD getNumRoutes(void);

/* Allocates the route table from heap and returns it to you in
 * *ppIpForwardTable.  Returns NO_ERROR on success, something else on failure.
 */
DWORD getRouteTable(PMIB_IPFORWARDTABLE *ppIpForwardTable, HANDLE heap,
 DWORD flags);

/* Returns the number of entries in the arp table. */
DWORD getNumArpEntries(void);

/* Allocates the arp table from heap and returns it to you in *ppIpNetTable.
 * Returns NO_ERROR on success, something else on failure.
 */
DWORD getArpTable(PMIB_IPNETTABLE *ppIpNetTable, HANDLE heap, DWORD flags);

/* Returns the number of entries in the UDP state table. */
DWORD getNumUdpEntries(void);

/* Allocates the UDP state table from heap and returns it to you in *ppUdpTable.
 * Returns NO_ERROR on success, something else on failure.
 */
DWORD getUdpTable(PMIB_UDPTABLE *ppUdpTable, HANDLE heap, DWORD flags);

/* Returns the number of entries in the TCP state table. */
DWORD getNumTcpEntries(void);

/* Allocates the TCP state table from heap and returns it to you in *ppTcpTable.
 * Returns NO_ERROR on success, something else on failure.
 */
DWORD getTcpTable(PMIB_TCPTABLE *ppTcpTable, DWORD maxEntries, HANDLE heap,
                  DWORD flags);

#endif /* ndef WINE_IPSTATS_H_ */
