/* ifenum.h
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
 * This module implements network interface and address enumeration.  It's
 * meant to hide some problematic defines like socket(), and make iphlpapi
 * more portable.
 *
 * Like Windows, it uses a numeric index to identify an interface uniquely.
 * As implemented, an interface represents a UNIX network interface, virtual
 * or real, and thus can have 0 or 1 IP addresses associated with it.  (This
 * only supports IPv4.)
 * The indexes returned are not guaranteed to be contiguous, so don't call
 * getNumInterfaces() and assume the values [0,getNumInterfaces() - 1] will be
 * valid indexes; use getInterfaceIndexTable() instead.
 *
 * See also the companion file, ipstats.h, for functions related to getting
 * statistics.
 */
#ifndef WINE_IFENUM_H_
#define WINE_IFENUM_H_

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#define USE_WS_PREFIX
#include "iprtrmib.h"
#include "winsock2.h"

#define MAX_INTERFACE_PHYSADDR    8
#define MAX_INTERFACE_DESCRIPTION 256

BOOL isIfIndexLoopback(ULONG idx) DECLSPEC_HIDDEN;

/* A table of interface indexes, see get_interface_indices(). */
typedef struct _InterfaceIndexTable {
  DWORD numIndexes;
  IF_INDEX indexes[1];
} InterfaceIndexTable;

/* Returns the count of all interface indexes and optionally a ptr to an interface table.
 * HeapFree() the returned table.  Will optionally ignore loopback devices.
 */
DWORD get_interface_indices( BOOL skip_loopback, InterfaceIndexTable **table ) DECLSPEC_HIDDEN;

/* ByName/ByIndex versions of various getter functions. */

/* can be used as quick check to see if you've got a valid index, returns NULL
 * if not.  Overwrites your buffer, which should be at least of size
 * MAX_ADAPTER_NAME.
 */
char *getInterfaceNameByIndex(IF_INDEX index, char *name) DECLSPEC_HIDDEN;

/* Fills index with the index of name, if found.  Returns
 * ERROR_INVALID_PARAMETER if name or index is NULL, ERROR_INVALID_DATA if name
 * is not found, and NO_ERROR on success.
 */
DWORD getInterfaceIndexByName(const char *name, IF_INDEX *index) DECLSPEC_HIDDEN;

/* Gets a few physical characteristics of a device:  MAC addr len, MAC addr,
 * and type as one of the MIB_IF_TYPEs.
 * len's in-out: on in, needs to say how many bytes are available in addr,
 * which to be safe should be MAX_INTERFACE_PHYSADDR.  On out, it's how many
 * bytes were set, or how many were required if addr isn't big enough.
 * Returns ERROR_INVALID_PARAMETER if name, len, addr, or type is NULL.
 * Returns ERROR_INVALID_DATA if name/index isn't valid.
 * Returns ERROR_INSUFFICIENT_BUFFER if addr isn't large enough for the
 * physical address; *len will contain the required size.
 * May return other errors, e.g. ERROR_OUTOFMEMORY or ERROR_NO_MORE_FILES,
 * if internal errors occur.
 * Returns NO_ERROR on success.
 */
DWORD getInterfacePhysicalByIndex(IF_INDEX index, PDWORD len, PBYTE addr,
 PDWORD type) DECLSPEC_HIDDEN;

/* Fills in the MIB_IFROW by name.  Doesn't fill in interface statistics,
 * see ipstats.h for that.
 * Returns ERROR_INVALID_PARAMETER if name is NULL, ERROR_INVALID_DATA
 * if name isn't valid, and NO_ERROR otherwise.
 */
DWORD getInterfaceEntryByName(const char *name, PMIB_IFROW entry) DECLSPEC_HIDDEN;

DWORD getNumIPAddresses(void) DECLSPEC_HIDDEN;

/* Gets the configured IP addresses for the system, and sets *ppIpAddrTable to
 * a table of them allocated from heap, or NULL if out of memory.  Returns
 * NO_ERROR on success, something else on failure.  Note there may be more than
 * one IP address may exist per interface.
 */
DWORD getIPAddrTable(PMIB_IPADDRTABLE *ppIpAddrTable, HANDLE heap, DWORD flags) DECLSPEC_HIDDEN;

/* Returns the IPv6 addresses for a particular interface index.
 * Returns NO_ERROR on success, something else on failure.
 */
ULONG v6addressesFromIndex(IF_INDEX index, SOCKET_ADDRESS **addrs, ULONG *num_addrs,
 SOCKET_ADDRESS **masks) DECLSPEC_HIDDEN;

DWORD getInterfaceMtuByName(const char *name, PDWORD mtu) DECLSPEC_HIDDEN;
DWORD getInterfaceStatusByName(const char *name, INTERNAL_IF_OPER_STATUS *status) DECLSPEC_HIDDEN;

#endif /* ndef WINE_IFENUM_H_ */
