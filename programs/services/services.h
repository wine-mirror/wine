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

struct scmdatabase
{
    HKEY root_key;
    LONG service_start_lock;
    struct list services;
    CRITICAL_SECTION cs;
};

struct service_entry
{
    struct list entry;
    struct scmdatabase *db;
    LONG ref_count;                    /* number of references - if goes to zero and the service is deleted the structure will be freed */
    LPWSTR name;
    SERVICE_STATUS_PROCESS status;
    QUERY_SERVICE_CONFIGW config;
    DWORD preshutdown_timeout;
    LPWSTR description;
    LPWSTR dependOnServices;
    LPWSTR dependOnGroups;
    HANDLE process;
    HANDLE control_mutex;
    HANDLE control_pipe;
    HANDLE overlapped_event;
    HANDLE status_changed_event;
    BOOL marked_for_delete;
};

extern struct scmdatabase *active_database;

/* SCM database functions */

struct service_entry *scmdatabase_find_service(struct scmdatabase *db, LPCWSTR name);
struct service_entry *scmdatabase_find_service_by_displayname(struct scmdatabase *db, LPCWSTR name);
DWORD scmdatabase_add_service(struct scmdatabase *db, struct service_entry *entry);

DWORD scmdatabase_lock_startup(struct scmdatabase *db);
void scmdatabase_unlock_startup(struct scmdatabase *db);

void scmdatabase_lock_shared(struct scmdatabase *db);
void scmdatabase_lock_exclusive(struct scmdatabase *db);
void scmdatabase_unlock(struct scmdatabase *db);

/* Service functions */

DWORD service_create(LPCWSTR name, struct service_entry **entry);
BOOL validate_service_name(LPCWSTR name);
BOOL validate_service_config(struct service_entry *entry);
DWORD save_service_config(struct service_entry *entry);
void free_service_entry(struct service_entry *entry);
void release_service(struct service_entry *service);
void service_lock_shared(struct service_entry *service);
void service_lock_exclusive(struct service_entry *service);
void service_unlock(struct service_entry *service);
DWORD service_start(struct service_entry *service, DWORD service_argc, LPCWSTR *service_argv);
void service_terminate(struct service_entry *service);
BOOL service_send_command( struct service_entry *service, HANDLE pipe,
                           const void *data, DWORD size, DWORD *result );

extern HANDLE g_hStartedEvent;

extern DWORD service_pipe_timeout;
extern DWORD service_kill_timeout;

DWORD RPC_Init(void);
DWORD events_loop(void);

/* from utils.c */
LPWSTR strdupW(LPCWSTR str);

BOOL check_multisz(LPCWSTR lpMultiSz, DWORD cbSize);

DWORD load_reg_string(HKEY hKey, LPCWSTR szValue, BOOL bExpand, LPWSTR *output);
DWORD load_reg_multisz(HKEY hKey, LPCWSTR szValue, BOOL bAllowSingle, LPWSTR *output);
DWORD load_reg_dword(HKEY hKey, LPCWSTR szValue, DWORD *output);

static inline LPCWSTR get_display_name(struct service_entry *service)
{
    return service->config.lpDisplayName ? service->config.lpDisplayName : service->name;
}

static inline BOOL is_marked_for_delete(struct service_entry *service)
{
    return service->marked_for_delete;
}

static inline DWORD mark_for_delete(struct service_entry *service)
{
    service->marked_for_delete = TRUE;
    return ERROR_SUCCESS;
}

#endif /*WINE_PROGRAMS_UTILS_H_*/
