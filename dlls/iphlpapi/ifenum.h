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

/* Fills index with the index of name, if found.  Returns
 * ERROR_INVALID_PARAMETER if name or index is NULL, ERROR_INVALID_DATA if name
 * is not found, and NO_ERROR on success.
 */
DWORD getInterfaceIndexByName(const char *name, IF_INDEX *index) DECLSPEC_HIDDEN;

#endif /* ndef WINE_IFENUM_H_ */
