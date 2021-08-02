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

DWORD build_tcp_table(TCP_TABLE_CLASS, void **, BOOL, HANDLE, DWORD, DWORD *) DECLSPEC_HIDDEN;
DWORD build_tcp6_table(TCP_TABLE_CLASS, void **, BOOL, HANDLE, DWORD, DWORD *) DECLSPEC_HIDDEN;
DWORD build_udp_table(UDP_TABLE_CLASS, void **, BOOL, HANDLE, DWORD, DWORD *) DECLSPEC_HIDDEN;
DWORD build_udp6_table(UDP_TABLE_CLASS, void **, BOOL, HANDLE, DWORD, DWORD *) DECLSPEC_HIDDEN;

#endif /* ndef WINE_IPSTATS_H_ */
