/*
 * Unix library interface
 *
 * Copyright 2021 Zebediah Figura for CodeWeavers
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

struct samba_funcs
{
    NET_API_STATUS (WINAPI *server_getinfo)( const WCHAR *server, DWORD level, BYTE **buffer );
    NET_API_STATUS (WINAPI *share_add)( const WCHAR *server, DWORD level, const BYTE *buffer, DWORD *err );
    NET_API_STATUS (WINAPI *share_del)( const WCHAR *server, const WCHAR *share, DWORD reserved );
    NET_API_STATUS (WINAPI *wksta_getinfo)( const WCHAR *server, DWORD level, BYTE **buffer );
    NET_API_STATUS (WINAPI *change_password)( const WCHAR *domain, const WCHAR *user,
                                              const WCHAR *old, const WCHAR *new );
};
