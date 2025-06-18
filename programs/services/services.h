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
    struct list processes;
    struct list services;
    CRITICAL_SECTION cs;
};

struct process_entry
{
    struct list entry;
    struct scmdatabase *db;
    LONG ref_count;
    LONG use_count;
    DWORD process_id;
    HANDLE process;
    HANDLE control_mutex;
    HANDLE control_pipe;
    HANDLE overlapped_event;
};

struct service_entry
{
    struct list entry;
    struct scmdatabase *db;
    LONG ref_count;                    /* number of references - if goes to zero and the service is deleted the structure will be freed */
    LPWSTR name;
    SERVICE_STATUS status;
    HANDLE status_changed_event;
    QUERY_SERVICE_CONFIGW config;
    DWORD preshutdown_timeout;
    LPWSTR description;
    LPWSTR dependOnServices;
    LPWSTR dependOnGroups;
    struct process_entry *process;
    BOOL shared_process;
    BOOL force_shutdown;
    BOOL marked_for_delete;
    BOOL is_wow64;
    BOOL delayed_autostart;
    struct list handles;
};

extern struct scmdatabase *active_database;

/* SCM database functions */

struct service_entry *scmdatabase_find_service(struct scmdatabase *db, LPCWSTR name);
struct service_entry *scmdatabase_find_service_by_displayname(struct scmdatabase *db, LPCWSTR name);
DWORD scmdatabase_add_service(struct scmdatabase *db, struct service_entry *entry);

BOOL scmdatabase_lock_startup(struct scmdatabase *db, int timeout);
void scmdatabase_unlock_startup(struct scmdatabase *db);

void scmdatabase_lock(struct scmdatabase *db);
void scmdatabase_unlock(struct scmdatabase *db);

/* Service functions */

DWORD service_create(LPCWSTR name, struct service_entry **entry);
BOOL validate_service_name(LPCWSTR name);
BOOL validate_service_config(struct service_entry *entry);
DWORD save_service_config(struct service_entry *entry);
void free_service_entry(struct service_entry *entry);
struct service_entry *grab_service(struct service_entry *service);
void release_service(struct service_entry *service);
void service_lock(struct service_entry *service);
void service_unlock(struct service_entry *service);
DWORD service_start(struct service_entry *service, DWORD service_argc, LPCWSTR *service_argv);
void notify_service_state(struct service_entry *service);

/* Process functions */

struct process_entry *grab_process(struct process_entry *process);
void release_process(struct process_entry *process);
BOOL process_send_control(struct process_entry *process, BOOL winedevice, const WCHAR *name,
                          DWORD control, const BYTE *data, DWORD data_size, DWORD *result);
void process_terminate(struct process_entry *process);

extern DWORD service_pipe_timeout;
extern DWORD service_kill_timeout;
extern HANDLE exit_event;

DWORD RPC_Init(void);
void RPC_Stop(void);

/* from utils.c */

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
