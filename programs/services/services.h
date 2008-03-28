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
    LONG ref_count;                    /* number of references - if goes to zero and the service is deleted the structure will be freed */
    LPWSTR name;
    SERVICE_STATUS_PROCESS status;
    QUERY_SERVICE_CONFIGW config;
    LPWSTR description;
    LPWSTR dependOnServices;
    LPWSTR dependOnGroups;
};

BOOL validate_service_name(LPCWSTR name);
BOOL validate_service_config(struct service_entry *entry);
struct service_entry *find_service(LPCWSTR name);
struct service_entry *find_service_by_displayname(LPCWSTR name);
DWORD add_service(struct service_entry *entry);
DWORD remove_service(struct service_entry *entry);
DWORD save_service_config(struct service_entry *entry);
void free_service_entry(struct service_entry *entry);
void release_service(struct service_entry *service);

DWORD lock_service_database(void);
void unlock_service_database(void);

void lock_services(void);
void unlock_services(void);

extern HANDLE g_hStartedEvent;

DWORD RPC_MainLoop(void);

/* from utils.c */
LPWSTR strdupW(LPCWSTR str);

BOOL check_multisz(LPCWSTR lpMultiSz, DWORD cbSize);

DWORD load_reg_string(HKEY hKey, LPCWSTR szValue, BOOL bExpand, LPWSTR *output);
DWORD load_reg_multisz(HKEY hKey, LPCWSTR szValue, LPWSTR *output);
DWORD load_reg_dword(HKEY hKey, LPCWSTR szValue, DWORD *output);

static inline LPCWSTR get_display_name(struct service_entry *service)
{
    return service->config.lpDisplayName ? service->config.lpDisplayName : service->name;
}

static inline BOOL is_marked_for_delete(struct service_entry *service)
{
    return service->entry.next == NULL;
}

#endif /*WINE_PROGRAMS_UTILS_H_*/
