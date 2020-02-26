/*
 * Copyright (C) 2019 Alex Henrie
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

#ifndef __WINE_IP2STRING_H
#define __WINE_IP2STRING_H

#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS WINAPI RtlIpv4StringToAddressA(const char *str, BOOLEAN strict, const char **terminator, IN_ADDR *address);
NTSTATUS WINAPI RtlIpv4StringToAddressW(const WCHAR *str, BOOLEAN strict, const WCHAR **terminator, IN_ADDR *address);
#define RtlIpv4StringToAddress WINELIB_NAME_AW(RtlIpv4StringToAddress)
NTSTATUS WINAPI RtlIpv4StringToAddressExA(const char *str, BOOLEAN strict, IN_ADDR *address, USHORT *port);
NTSTATUS WINAPI RtlIpv4StringToAddressExW(const WCHAR *str, BOOLEAN strict, IN_ADDR *address, USHORT *port);
#define RtlIpv4StringToAddressEx WINELIB_NAME_AW(RtlIpv4StringToAddressEx)

#ifdef __cplusplus
}
#endif

#endif
