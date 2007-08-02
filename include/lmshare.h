/*
 * Copyright (C) 2003 Juan Lang
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
#ifndef _LMSHARE_H
#define _LMSHARE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lmcons.h>

typedef struct _SHARE_INFO_0 {
    LMSTR shi0_netname;
} SHARE_INFO_0, *PSHARE_INFO_0, *LPSHARE_INFO_0;

typedef struct _SHARE_INFO_1 {
    LMSTR shi1_netname;
    DWORD shi1_type;
    LMSTR shi1_remark;
} SHARE_INFO_1, *PSHARE_INFO_1, *LPSHARE_INFO_1;

NET_API_STATUS WINAPI NetShareDel(LMSTR servername, LMSTR netname, DWORD reserved);
NET_API_STATUS WINAPI NetShareEnum(LMSTR servername, DWORD level,
 LPBYTE *bufptr, DWORD prefmaxlen, LPDWORD entriesread, LPDWORD totalentries,
 LPDWORD resume_handle);

#define STYPE_DISKTREE 0
#define STYPE_PRINTQ   1
#define STYPE_DEVICE   2
#define STYPE_IPC      3
#define STYPE_SPECIAL  0x80000000

NET_API_STATUS NET_API_FUNCTION NetSessionEnum(LMSTR servername, LMSTR UncClientName,
    LMSTR username, DWORD level, LPBYTE *bufptr, DWORD prefmaxlen, LPDWORD entriesread,
    LPDWORD totalentries, LPDWORD resume_handle);

#ifdef __cplusplus
}
#endif

#endif /* ndef _LMSHARE_H */
