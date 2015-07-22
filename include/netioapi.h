/*
 * Copyright 2015 Hans Leidekker for CodeWeavers
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

#ifndef __WINE_NETIOAPI_H
#define __WINE_NETIOAPI_H

DWORD WINAPI ConvertInterfaceGuidToLuid(const GUID*,NET_LUID*);
DWORD WINAPI ConvertInterfaceIndexToLuid(NET_IFINDEX,NET_LUID*);
DWORD WINAPI ConvertInterfaceLuidToGuid(const NET_LUID*,GUID*);
DWORD WINAPI ConvertInterfaceLuidToIndex(const NET_LUID*,NET_IFINDEX*);
DWORD WINAPI ConvertInterfaceLuidToNameA(const NET_LUID*,char*,SIZE_T);
DWORD WINAPI ConvertInterfaceLuidToNameW(const NET_LUID*,WCHAR*,SIZE_T);
DWORD WINAPI ConvertInterfaceNameToLuidA(const char*,NET_LUID*);
DWORD WINAPI ConvertInterfaceNameToLuidW(const WCHAR*,NET_LUID*);
void WINAPI FreeMibTable(void*);

#endif /* __WINE_NETIOAPI_H */
