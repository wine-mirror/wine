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
#include "iprtrmib.h"

#define MAX_INTERFACE_PHYSADDR    8
#define MAX_INTERFACE_DESCRIPTION 256

DWORD getNumInterfaces(void);
DWORD getNumNonLoopbackInterfaces(void);

/* A table of interface indexes, see get*InterfaceTable(). */
typedef struct _InterfaceIndexTable {
  DWORD numIndexes;
  DWORD indexes[1];
} InterfaceIndexTable;

/* Returns a table with all known interface indexes, or NULL if one could not
 * be allocated.  HeapFree() the returned table.
 */
InterfaceIndexTable *getInterfaceIndexTable(void);

/* Like getInterfaceIndexTable, but filters out loopback interfaces. */
InterfaceIndexTable *getNonLoopbackInterfaceIndexTable(void);

/* ByName/ByIndex versions of various getter functions. */

/* can be used as quick check to see if you've got a valid index, returns NULL
 * if not.  Overwrites your buffer, which should be at least of size
 * MAX_ADAPTER_NAME.
 */
char *getInterfaceNameByIndex(DWORD index, char *name);

/* Fills index with the index of name, if found.  Returns
 * ERROR_INVALID_PARAMETER if name or index is NULL, ERROR_INVALID_DATA if name
 * is not found, and NO_ERROR on success.
 */
DWORD getInterfaceIndexByName(const char *name, PDWORD index);

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
DWORD getInterfacePhysicalByName(const char *name, PDWORD len, PBYTE addr,
 PDWORD type);
DWORD getInterfacePhysicalByIndex(DWORD index, PDWORD len, PBYTE addr,
 PDWORD type);

/* Fills in the MIB_IFROW by name/index.  Doesn't fill in interface statistics,
 * see ipstats.h for that.
 * Returns ERROR_INVALID_PARAMETER if name or entry is NULL, ERROR_INVALID_DATA
 * if name/index isn't valid, and NO_ERROR otherwise.
 */
DWORD getInterfaceEntryByName(const char *name, PMIB_IFROW entry);
DWORD getInterfaceEntryByIndex(DWORD index, PMIB_IFROW entry);

DWORD getNumIPAddresses(void);

/* Gets the configured IP addresses for the system, and sets *ppIpAddrTable to
 * a table of them allocated from heap, or NULL if out of memory.  Returns
 * NO_ERROR on success, something else on failure.  Note there may be more than
 * one IP address may exist per interface.
 */
DWORD getIPAddrTable(PMIB_IPADDRTABLE *ppIpAddrTable, HANDLE heap, DWORD flags);

/* Converts the network-order bytes in addr to a printable string.  Returns
 * string.
 */
char *toIPAddressString(unsigned int addr, char string[16]);

#endif /* ndef WINE_IFENUM_H_ */
