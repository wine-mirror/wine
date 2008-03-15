/*
 * Services.exe - include file
 *
 * Copyright 2007 Google (Mikolaj Zalewski)
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

#ifndef WINE_PROGRAMS_UTILS_H_
#define WINE_PROGRAMS_UTILS_H_

#include "wine/list.h"

struct service_entry
{
    struct list entry;
    LPWSTR name;
    SERVICE_STATUS_PROCESS status;
    QUERY_SERVICE_CONFIGW config;
    LPWSTR description;
    LPWSTR dependOnServices;
    LPWSTR dependOnGroups;
};

extern HANDLE g_hStartedEvent;

DWORD RPC_MainLoop(void);

/* from utils.c */
LPWSTR strdupW(LPCWSTR str);

DWORD load_reg_string(HKEY hKey, LPCWSTR szValue, BOOL bExpand, LPWSTR *output);
DWORD load_reg_multisz(HKEY hKey, LPCWSTR szValue, LPWSTR *output);
DWORD load_reg_dword(HKEY hKey, LPCWSTR szValue, DWORD *output);

#endif /*WINE_PROGRAMS_UTILS_H_*/
