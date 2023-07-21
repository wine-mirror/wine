/*
 * Services - controls services keeps track of their state
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

#define WIN32_LEAN_AND_MEAN

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <windows.h>
#include <winsvc.h>
#include <winternl.h>
#include <rpc.h>
#include <userenv.h>
#include <setupapi.h>

#include "wine/debug.h"
#include "svcctl.h"

#include "services.h"

#define MAX_SERVICE_NAME 260

WINE_DEFAULT_DEBUG_CHANNEL(service);

struct scmdatabase *active_database;

DWORD service_pipe_timeout = 10000;
DWORD service_kill_timeout = 60000;
static DWORD default_preshutdown_timeout = 180000;
static DWORD autostart_delay = 120000;
static void *environment = NULL;
static HKEY service_current_key = NULL;
static HANDLE job_object, job_completion_port;

static const BOOL is_win64 = (sizeof(void *) > sizeof(int));

static const WCHAR SZ_LOCAL_SYSTEM[] = {'L','o','c','a','l','S','y','s','t','e','m',0};

/* Registry constants */
static const WCHAR SZ_SERVICES_KEY[] = { 'S','y','s','t','e','m','\\',
      'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
      'S','e','r','v','i','c','e','s',0 };

/* Service key values names */
static const WCHAR SZ_DISPLAY_NAME[]      = {'D','i','s','p','l','a','y','N','a','m','e',0 };
static const WCHAR SZ_TYPE[]              = {'T','y','p','e',0 };
static const WCHAR SZ_START[]             = {'S','t','a','r','t',0 };
static const WCHAR SZ_ERROR[]             = {'E','r','r','o','r','C','o','n','t','r','o','l',0 };
static const WCHAR SZ_IMAGE_PATH[]        = {'I','m','a','g','e','P','a','t','h',0};
static const WCHAR SZ_GROUP[]             = {'G','r','o','u','p',0};
static const WCHAR SZ_DEPEND_ON_SERVICE[] = {'D','e','p','e','n','d','O','n','S','e','r','v','i','c','e',0};
static const WCHAR SZ_DEPEND_ON_GROUP[]   = {'D','e','p','e','n','d','O','n','G','r','o','u','p',0};
static const WCHAR SZ_OBJECT_NAME[]       = {'O','b','j','e','c','t','N','a','m','e',0};
static const WCHAR SZ_TAG[]               = {'T','a','g',0};
static const WCHAR SZ_DESCRIPTION[]       = {'D','e','s','c','r','i','p','t','i','o','n',0};
static const WCHAR SZ_PRESHUTDOWN[]       = {'P','r','e','s','h','u','t','d','o','w','n','T','i','m','e','o','u','t',0};
static const WCHAR SZ_WOW64[]             = {'W','O','W','6','4',0};
static const WCHAR SZ_DELAYED_AUTOSTART[] = {'D','e','l','a','y','e','d','A','u','t','o','S','t','a','r','t',0};

static DWORD process_create(const WCHAR *name, struct process_entry **entry)
{
    DWORD err;

    *entry = calloc(1, sizeof(**entry));
    if (!*entry)
        return ERROR_NOT_ENOUGH_SERVER_MEMORY;
    (*entry)->ref_count = 1;
    (*entry)->control_mutex = CreateMutexW(NULL, TRUE, NULL);
    if (!(*entry)->control_mutex)
        goto error;
    (*entry)->overlapped_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!(*entry)->overlapped_event)
        goto error;
    (*entry)->control_pipe = CreateNamedPipeW(name, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                              PIPE_TYPE_BYTE|PIPE_WAIT, 1, 256, 256, 10000, NULL);
    if ((*entry)->control_pipe == INVALID_HANDLE_VALUE)
        goto error;
    /* all other fields are zero */
    return ERROR_SUCCESS;

error:
    err = GetLastError();
    if ((*entry)->control_mutex)
        CloseHandle((*entry)->control_mutex);
    if ((*entry)->overlapped_event)
        CloseHandle((*entry)->overlapped_event);
    free(*entry);
    return err;
}

static void free_process_entry(struct process_entry *entry)
{
    CloseHandle(entry->process);
    CloseHandle(entry->control_mutex);
    CloseHandle(entry->control_pipe);
    CloseHandle(entry->overlapped_event);
    free(entry);
}

DWORD service_create(LPCWSTR name, struct service_entry **entry)
{
    *entry = calloc(1, sizeof(**entry));
    if (!*entry)
        return ERROR_NOT_ENOUGH_SERVER_MEMORY;
    (*entry)->name = wcsdup(name);
    list_init(&(*entry)->handles);
    if (!(*entry)->name)
    {
        free(*entry);
        return ERROR_NOT_ENOUGH_SERVER_MEMORY;
    }
    (*entry)->status_changed_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!(*entry)->status_changed_event)
    {
        free((*entry)->name);
        free(*entry);
        return GetLastError();
    }
    (*entry)->ref_count = 1;
    (*entry)->status.dwCurrentState = SERVICE_STOPPED;
    (*entry)->status.dwWin32ExitCode = ERROR_SERVICE_NEVER_STARTED;
    (*entry)->preshutdown_timeout = default_preshutdown_timeout;
    /* all other fields are zero */
    return ERROR_SUCCESS;
}

void free_service_entry(struct service_entry *entry)
{
    assert(list_empty(&entry->handles));
    CloseHandle(entry->status_changed_event);
    free(entry->name);
    free(entry->config.lpBinaryPathName);
    free(entry->config.lpDependencies);
    free(entry->config.lpLoadOrderGroup);
    free(entry->config.lpServiceStartName);
    free(entry->config.lpDisplayName);
    free(entry->description);
    free(entry->dependOnServices);
    free(entry->dependOnGroups);
    if (entry->process) release_process(entry->process);
    free(entry);
}

static DWORD load_service_config(HKEY hKey, struct service_entry *entry)
{
    DWORD err, value = 0;
    WCHAR *wptr;

    if ((err = load_reg_string(hKey, SZ_IMAGE_PATH,   TRUE, &entry->config.lpBinaryPathName)) != 0)
        return err;
    if ((err = load_reg_string(hKey, SZ_GROUP,        0,    &entry->config.lpLoadOrderGroup)) != 0)
        return err;
    if ((err = load_reg_string(hKey, SZ_OBJECT_NAME,  TRUE, &entry->config.lpServiceStartName)) != 0)
        return err;
    if ((err = load_reg_string(hKey, SZ_DISPLAY_NAME, 0,    &entry->config.lpDisplayName)) != 0)
        return err;
    if ((err = load_reg_string(hKey, SZ_DESCRIPTION,  0,    &entry->description)) != 0)
        return err;
    if ((err = load_reg_multisz(hKey, SZ_DEPEND_ON_SERVICE, TRUE, &entry->dependOnServices)) != 0)
        return err;
    if ((err = load_reg_multisz(hKey, SZ_DEPEND_ON_GROUP, FALSE, &entry->dependOnGroups)) != 0)
        return err;

    if ((err = load_reg_dword(hKey, SZ_TYPE,  &entry->config.dwServiceType)) != 0)
        return err;
    if ((err = load_reg_dword(hKey, SZ_START, &entry->config.dwStartType)) != 0)
        return err;
    if ((err = load_reg_dword(hKey, SZ_ERROR, &entry->config.dwErrorControl)) != 0)
        return err;
    if ((err = load_reg_dword(hKey, SZ_TAG,   &entry->config.dwTagId)) != 0)
        return err;
    if ((err = load_reg_dword(hKey, SZ_PRESHUTDOWN, &entry->preshutdown_timeout)) != 0)
        return err;

    if (load_reg_dword(hKey, SZ_WOW64, &value) == 0 && value == 1)
        entry->is_wow64 = TRUE;
    if (load_reg_dword(hKey, SZ_DELAYED_AUTOSTART, &value) == 0 && value == 1)
        entry->delayed_autostart = TRUE;

    WINE_TRACE("Image path           = %s\n", wine_dbgstr_w(entry->config.lpBinaryPathName) );
    WINE_TRACE("Group                = %s\n", wine_dbgstr_w(entry->config.lpLoadOrderGroup) );
    WINE_TRACE("Service account name = %s\n", wine_dbgstr_w(entry->config.lpServiceStartName) );
    WINE_TRACE("Display name         = %s\n", wine_dbgstr_w(entry->config.lpDisplayName) );
    WINE_TRACE("Service dependencies : %s\n", entry->dependOnServices[0] ? "" : "(none)");
    for (wptr = entry->dependOnServices; *wptr; wptr += lstrlenW(wptr) + 1)
        WINE_TRACE("    * %s\n", wine_dbgstr_w(wptr));
    WINE_TRACE("Group dependencies   : %s\n", entry->dependOnGroups[0] ? "" : "(none)");
    for (wptr = entry->dependOnGroups; *wptr; wptr += lstrlenW(wptr) + 1)
        WINE_TRACE("    * %s\n", wine_dbgstr_w(wptr));

    return ERROR_SUCCESS;
}

static DWORD reg_set_string_value(HKEY hKey, LPCWSTR value_name, LPCWSTR string)
{
    if (!string)
    {
        DWORD err;
        err = RegDeleteValueW(hKey, value_name);
        if (err != ERROR_FILE_NOT_FOUND)
            return err;

        return ERROR_SUCCESS;
    }

    return RegSetValueExW(hKey, value_name, 0, REG_SZ, (const BYTE*)string, sizeof(WCHAR)*(lstrlenW(string) + 1));
}

static DWORD reg_set_multisz_value(HKEY hKey, LPCWSTR value_name, LPCWSTR string)
{
    const WCHAR *ptr;

    if (!string)
    {
        DWORD err;
        err = RegDeleteValueW(hKey, value_name);
        if (err != ERROR_FILE_NOT_FOUND)
            return err;

        return ERROR_SUCCESS;
    }

    ptr = string;
    while (*ptr) ptr += lstrlenW(ptr) + 1;
    return RegSetValueExW(hKey, value_name, 0, REG_MULTI_SZ, (const BYTE*)string, sizeof(WCHAR)*(ptr - string + 1));
}

DWORD save_service_config(struct service_entry *entry)
{
    DWORD err;
    HKEY hKey = NULL;

    err = RegCreateKeyW(entry->db->root_key, entry->name, &hKey);
    if (err != ERROR_SUCCESS)
        goto cleanup;

    if ((err = reg_set_string_value(hKey, SZ_DISPLAY_NAME, entry->config.lpDisplayName)) != 0)
        goto cleanup;
    if ((err = reg_set_string_value(hKey, SZ_IMAGE_PATH, entry->config.lpBinaryPathName)) != 0)
        goto cleanup;
    if ((err = reg_set_string_value(hKey, SZ_GROUP, entry->config.lpLoadOrderGroup)) != 0)
        goto cleanup;
    if ((err = reg_set_string_value(hKey, SZ_OBJECT_NAME, entry->config.lpServiceStartName)) != 0)
        goto cleanup;
    if ((err = reg_set_string_value(hKey, SZ_DESCRIPTION, entry->description)) != 0)
        goto cleanup;
    if ((err = reg_set_multisz_value(hKey, SZ_DEPEND_ON_SERVICE, entry->dependOnServices)) != 0)
        goto cleanup;
    if ((err = reg_set_multisz_value(hKey, SZ_DEPEND_ON_GROUP, entry->dependOnGroups)) != 0)
        goto cleanup;
    if ((err = RegSetValueExW(hKey, SZ_START, 0, REG_DWORD, (LPBYTE)&entry->config.dwStartType, sizeof(DWORD))) != 0)
        goto cleanup;
    if ((err = RegSetValueExW(hKey, SZ_ERROR, 0, REG_DWORD, (LPBYTE)&entry->config.dwErrorControl, sizeof(DWORD))) != 0)
        goto cleanup;
    if ((err = RegSetValueExW(hKey, SZ_TYPE, 0, REG_DWORD, (LPBYTE)&entry->config.dwServiceType, sizeof(DWORD))) != 0)
        goto cleanup;
    if ((err = RegSetValueExW(hKey, SZ_PRESHUTDOWN, 0, REG_DWORD, (LPBYTE)&entry->preshutdown_timeout, sizeof(DWORD))) != 0)
        goto cleanup;
    if ((err = RegSetValueExW(hKey, SZ_PRESHUTDOWN, 0, REG_DWORD, (LPBYTE)&entry->preshutdown_timeout, sizeof(DWORD))) != 0)
        goto cleanup;
    if (entry->is_wow64)
    {
        const DWORD is_wow64 = 1;
        if ((err = RegSetValueExW(hKey, SZ_WOW64, 0, REG_DWORD, (LPBYTE)&is_wow64, sizeof(DWORD))) != 0)
            goto cleanup;
    }

    if (entry->config.dwTagId)
        err = RegSetValueExW(hKey, SZ_TAG, 0, REG_DWORD, (LPBYTE)&entry->config.dwTagId, sizeof(DWORD));
    else
        err = RegDeleteValueW(hKey, SZ_TAG);

    if (err != 0 && err != ERROR_FILE_NOT_FOUND)
        goto cleanup;

    err = ERROR_SUCCESS;
cleanup:
    RegCloseKey(hKey);
    return err;
}

static void scmdatabase_add_process(struct scmdatabase *db, struct process_entry *process)
{
    process->db = db;
    list_add_tail(&db->processes, &process->entry);
}

static void scmdatabase_remove_process(struct scmdatabase *db, struct process_entry *process)
{
    list_remove(&process->entry);
    process->entry.next = process->entry.prev = NULL;
}

DWORD scmdatabase_add_service(struct scmdatabase *db, struct service_entry *service)
{
    int err;
    service->db = db;
    if ((err = save_service_config(service)) != ERROR_SUCCESS)
    {
        WINE_ERR("Couldn't store service configuration: error %u\n", err);
        return ERROR_GEN_FAILURE;
    }

    list_add_tail(&db->services, &service->entry);
    return ERROR_SUCCESS;
}

static void scmdatabase_remove_service(struct scmdatabase *db, struct service_entry *service)
{
    RegDeleteTreeW(db->root_key, service->name);
    list_remove(&service->entry);
    service->entry.next = service->entry.prev = NULL;
}

static int __cdecl compare_tags(const void *a, const void *b)
{
    struct service_entry *service_a = *(struct service_entry **)a;
    struct service_entry *service_b = *(struct service_entry **)b;
    return service_a->config.dwTagId - service_b->config.dwTagId;
}

static PTP_CLEANUP_GROUP delayed_autostart_cleanup;

struct delayed_autostart_params
{
    unsigned int count;
    struct service_entry **services;
};

static void CALLBACK delayed_autostart_cancel_callback(void *object, void *userdata)
{
    struct delayed_autostart_params *params = object;
    while(params->count--)
        release_service(params->services[params->count]);
    free(params->services);
    free(params);
}

static void CALLBACK delayed_autostart_callback(TP_CALLBACK_INSTANCE *instance, void *context,
                                                TP_TIMER *timer)
{
    struct delayed_autostart_params *params = context;
    struct service_entry *service;
    unsigned int i;
    DWORD err;

    scmdatabase_lock_startup(active_database, INFINITE);

    for (i = 0; i < params->count; i++)
    {
        service = params->services[i];
        if (service->status.dwCurrentState == SERVICE_STOPPED)
        {
            TRACE("Starting delayed auto-start service %s\n", debugstr_w(service->name));
            err = service_start(service, 0, NULL);
            if (err != ERROR_SUCCESS)
                FIXME("Delayed auto-start service %s failed to start: %ld\n",
                      wine_dbgstr_w(service->name), err);
        }
        release_service(service);
    }

    scmdatabase_unlock_startup(active_database);

    free(params->services);
    free(params);
    CloseThreadpoolTimer(timer);
}

static BOOL schedule_delayed_autostart(struct service_entry **services, unsigned int count)
{
    struct delayed_autostart_params *params;
    TP_CALLBACK_ENVIRON environment;
    LARGE_INTEGER timestamp;
    TP_TIMER *timer;
    FILETIME ft;

    if (!(delayed_autostart_cleanup = CreateThreadpoolCleanupGroup()))
    {
        ERR("CreateThreadpoolCleanupGroup failed with error %lu\n", GetLastError());
        return FALSE;
    }

    if (!(params = malloc(sizeof(*params)))) return FALSE;
    params->count = count;
    params->services = services;

    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.CleanupGroup = delayed_autostart_cleanup;
    environment.CleanupGroupCancelCallback = delayed_autostart_cancel_callback;

    timestamp.QuadPart = (ULONGLONG)autostart_delay * -10000;
    ft.dwLowDateTime   = timestamp.u.LowPart;
    ft.dwHighDateTime  = timestamp.u.HighPart;

    if (!(timer = CreateThreadpoolTimer(delayed_autostart_callback, params, &environment)))
    {
        ERR("CreateThreadpoolWait failed: %lu\n", GetLastError());
        free(params);
        return FALSE;
    }

    SetThreadpoolTimer(timer, &ft, 0, 0);
    return TRUE;
}

static BOOL is_root_pnp_service(HDEVINFO set, const struct service_entry *service)
{
    SP_DEVINFO_DATA device = {sizeof(device)};
    WCHAR name[MAX_SERVICE_NAME];
    unsigned int i;

    for (i = 0; SetupDiEnumDeviceInfo(set, i, &device); ++i)
    {
        if (SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_SERVICE, NULL,
                                              (BYTE *)name, sizeof(name), NULL)
                && !wcsicmp(name, service->name))
        {
            return TRUE;
        }
    }

    return FALSE;
}

static void scmdatabase_autostart_services(struct scmdatabase *db)
{
    static const WCHAR rootW[] = {'R','O','O','T',0};
    struct service_entry **services_list;
    unsigned int i = 0;
    unsigned int size = 32;
    unsigned int delayed_cnt = 0;
    struct service_entry *service;
    HDEVINFO set;

    services_list = malloc(size * sizeof(services_list[0]));
    if (!services_list)
        return;

    if ((set = SetupDiGetClassDevsW( NULL, rootW, NULL, DIGCF_ALLCLASSES )) == INVALID_HANDLE_VALUE)
        WINE_ERR("Failed to enumerate devices, error %#lx.\n", GetLastError());

    scmdatabase_lock(db);

    LIST_FOR_EACH_ENTRY(service, &db->services, struct service_entry, entry)
    {
        if (service->config.dwStartType == SERVICE_BOOT_START ||
            service->config.dwStartType == SERVICE_SYSTEM_START ||
            service->config.dwStartType == SERVICE_AUTO_START ||
            (set != INVALID_HANDLE_VALUE && is_root_pnp_service(set, service)))
        {
            if (i+1 >= size)
            {
                struct service_entry **slist_new;
                size *= 2;
                slist_new = realloc(services_list, size * sizeof(services_list[0]));
                if (!slist_new)
                    break;
                services_list = slist_new;
            }
            services_list[i++] = grab_service(service);
        }
    }
    size = i;

    scmdatabase_unlock(db);
    qsort(services_list, size, sizeof(services_list[0]), compare_tags);
    scmdatabase_lock_startup(db, INFINITE);

    for (i = 0; i < size; i++)
    {
        DWORD err;
        service = services_list[i];
        if (service->delayed_autostart)
        {
            TRACE("delayed starting %s\n", wine_dbgstr_w(service->name));
            services_list[delayed_cnt++] = service;
            continue;
        }
        err = service_start(service, 0, NULL);
        if (err != ERROR_SUCCESS)
            WINE_FIXME("Auto-start service %s failed to start: %ld\n",
                       wine_dbgstr_w(service->name), err);
        release_service(service);
    }

    scmdatabase_unlock_startup(db);

    if (!delayed_cnt || !schedule_delayed_autostart(services_list, delayed_cnt))
        free(services_list);
    SetupDiDestroyDeviceInfoList(set);
}

static void scmdatabase_wait_terminate(struct scmdatabase *db)
{
    struct list pending = LIST_INIT(pending);
    void *ptr;

    scmdatabase_lock(db);
    list_move_tail(&pending, &db->processes);
    while ((ptr = list_head(&pending)))
    {
        struct process_entry *process = grab_process(LIST_ENTRY(ptr, struct process_entry, entry));

        process_terminate(process);
        scmdatabase_unlock(db);
        WaitForSingleObject(process->process, INFINITE);
        scmdatabase_lock(db);

        list_remove(&process->entry);
        list_add_tail(&db->processes, &process->entry);
        release_process(process);
    }
    scmdatabase_unlock(db);
}

BOOL validate_service_name(LPCWSTR name)
{
    return (name && name[0] && !wcschr(name, '/') && !wcschr(name, '\\'));
}

BOOL validate_service_config(struct service_entry *entry)
{
    if (entry->config.dwServiceType & SERVICE_WIN32 && (entry->config.lpBinaryPathName == NULL || !entry->config.lpBinaryPathName[0]))
    {
        WINE_ERR("Service %s is Win32 but has no image path set\n", wine_dbgstr_w(entry->name));
        return FALSE;
    }

    switch (entry->config.dwServiceType)
    {
    case SERVICE_KERNEL_DRIVER:
    case SERVICE_FILE_SYSTEM_DRIVER:
    case SERVICE_WIN32_OWN_PROCESS:
    case SERVICE_WIN32_SHARE_PROCESS:
        /* No problem */
        break;
    case SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS:
    case SERVICE_WIN32_SHARE_PROCESS | SERVICE_INTERACTIVE_PROCESS:
        /* These can be only run as LocalSystem */
        if (entry->config.lpServiceStartName && wcsicmp(entry->config.lpServiceStartName, SZ_LOCAL_SYSTEM) != 0)
        {
            WINE_ERR("Service %s is interactive but has a start name\n", wine_dbgstr_w(entry->name));
            return FALSE;
        }
        break;
    default:
        WINE_ERR("Service %s has an unknown service type (0x%lx)\n", wine_dbgstr_w(entry->name), entry->config.dwServiceType);
        return FALSE;
    }

    /* StartType can only be a single value (if several values are mixed the result is probably not what was intended) */
    if (entry->config.dwStartType > SERVICE_DISABLED)
    {
        WINE_ERR("Service %s has an unknown start type\n", wine_dbgstr_w(entry->name));
        return FALSE;
    }

    /* SERVICE_BOOT_START and SERVICE_SYSTEM_START are only allowed for driver services */
    if (((entry->config.dwStartType == SERVICE_BOOT_START) || (entry->config.dwStartType == SERVICE_SYSTEM_START)) &&
        ((entry->config.dwServiceType & SERVICE_WIN32_OWN_PROCESS) || (entry->config.dwServiceType & SERVICE_WIN32_SHARE_PROCESS)))
    {
        WINE_ERR("Service %s - SERVICE_BOOT_START and SERVICE_SYSTEM_START are only allowed for driver services\n", wine_dbgstr_w(entry->name));
        return FALSE;
    }

    if (entry->config.lpServiceStartName == NULL)
        entry->config.lpServiceStartName = wcsdup(SZ_LOCAL_SYSTEM);

    return TRUE;
}


struct service_entry *scmdatabase_find_service(struct scmdatabase *db, LPCWSTR name)
{
    struct service_entry *service;

    LIST_FOR_EACH_ENTRY(service, &db->services, struct service_entry, entry)
    {
        if (wcsicmp(name, service->name) == 0)
            return service;
    }

    return NULL;
}

struct service_entry *scmdatabase_find_service_by_displayname(struct scmdatabase *db, LPCWSTR name)
{
    struct service_entry *service;

    LIST_FOR_EACH_ENTRY(service, &db->services, struct service_entry, entry)
    {
        if (service->config.lpDisplayName && wcsicmp(name, service->config.lpDisplayName) == 0)
            return service;
    }

    return NULL;
}

struct process_entry *grab_process(struct process_entry *process)
{
    if (process)
        InterlockedIncrement(&process->ref_count);
    return process;
}

void release_process(struct process_entry *process)
{
    struct scmdatabase *db = process->db;

    scmdatabase_lock(db);
    if (InterlockedDecrement(&process->ref_count) == 0)
    {
        scmdatabase_remove_process(db, process);
        free_process_entry(process);
    }
    scmdatabase_unlock(db);
}

struct service_entry *grab_service(struct service_entry *service)
{
    if (service)
        InterlockedIncrement(&service->ref_count);
    return service;
}

void release_service(struct service_entry *service)
{
    struct scmdatabase *db = service->db;

    scmdatabase_lock(db);
    if (InterlockedDecrement(&service->ref_count) == 0 && is_marked_for_delete(service))
    {
        scmdatabase_remove_service(db, service);
        free_service_entry(service);
    }
    scmdatabase_unlock(db);
}

static DWORD scmdatabase_create(struct scmdatabase **db)
{
    DWORD err;

    *db = malloc(sizeof(**db));
    if (!*db)
        return ERROR_NOT_ENOUGH_SERVER_MEMORY;

    (*db)->service_start_lock = FALSE;
    list_init(&(*db)->processes);
    list_init(&(*db)->services);

    InitializeCriticalSection(&(*db)->cs);
    (*db)->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": scmdatabase");

    err = RegCreateKeyExW(HKEY_LOCAL_MACHINE, SZ_SERVICES_KEY, 0, NULL,
                          REG_OPTION_NON_VOLATILE, MAXIMUM_ALLOWED, NULL,
                          &(*db)->root_key, NULL);
    if (err != ERROR_SUCCESS)
        free(*db);

    return err;
}

static void scmdatabase_destroy(struct scmdatabase *db)
{
    RegCloseKey(db->root_key);
    db->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&db->cs);
    free(db);
}

static DWORD scmdatabase_load_services(struct scmdatabase *db)
{
    DWORD err;
    int i;

    for (i = 0; TRUE; i++)
    {
        WCHAR szName[MAX_SERVICE_NAME];
        struct service_entry *entry;
        HKEY hServiceKey;

        err = RegEnumKeyW(db->root_key, i, szName, MAX_SERVICE_NAME);
        if (err == ERROR_NO_MORE_ITEMS)
            break;

        if (err != 0)
        {
            WINE_ERR("Error %ld reading key %d name - skipping\n", err, i);
            continue;
        }

        err = service_create(szName, &entry);
        if (err != ERROR_SUCCESS)
            break;

        WINE_TRACE("Loading service %s\n", wine_dbgstr_w(szName));
        err = RegOpenKeyExW(db->root_key, szName, 0, KEY_READ, &hServiceKey);
        if (err == ERROR_SUCCESS)
        {
            err = load_service_config(hServiceKey, entry);
            RegCloseKey(hServiceKey);
        }

        if (err != ERROR_SUCCESS)
        {
            WINE_ERR("Error %ld reading registry key for service %s - skipping\n", err, wine_dbgstr_w(szName));
            free_service_entry(entry);
            continue;
        }

        if (entry->config.dwServiceType == 0)
        {
            /* Maybe an application only wrote some configuration in the service key. Continue silently */
            WINE_TRACE("Even the service type not set for service %s - skipping\n", wine_dbgstr_w(szName));
            free_service_entry(entry);
            continue;
        }

        if (!validate_service_config(entry))
        {
            WINE_ERR("Invalid configuration of service %s - skipping\n", wine_dbgstr_w(szName));
            free_service_entry(entry);
            continue;
        }

        entry->status.dwServiceType = entry->config.dwServiceType;
        entry->db = db;

        list_add_tail(&db->services, &entry->entry);
        release_service(entry);
    }
    return ERROR_SUCCESS;
}

BOOL scmdatabase_lock_startup(struct scmdatabase *db, int timeout)
{
    while (InterlockedCompareExchange(&db->service_start_lock, TRUE, FALSE))
    {
        if (timeout != INFINITE)
        {
            timeout -= 10;
            if (timeout <= 0) return FALSE;
        }
        Sleep(10);
    }
    return TRUE;
}

void scmdatabase_unlock_startup(struct scmdatabase *db)
{
    InterlockedCompareExchange(&db->service_start_lock, FALSE, TRUE);
}

void scmdatabase_lock(struct scmdatabase *db)
{
    EnterCriticalSection(&db->cs);
}

void scmdatabase_unlock(struct scmdatabase *db)
{
    LeaveCriticalSection(&db->cs);
}

void service_lock(struct service_entry *service)
{
    EnterCriticalSection(&service->db->cs);
}

void service_unlock(struct service_entry *service)
{
    LeaveCriticalSection(&service->db->cs);
}

/* only one service started at a time, so there is no race on the registry
 * value here */
static LPWSTR service_get_pipe_name(void)
{
    static const WCHAR format[] = { '\\','\\','.','\\','p','i','p','e','\\',
        'n','e','t','\\','N','t','C','o','n','t','r','o','l','P','i','p','e','%','u',0};
    static WCHAR name[ARRAY_SIZE(format) + 10]; /* lstrlenW("4294967295") */
    static DWORD service_current = 0;
    DWORD len, value = -1;
    LONG ret;
    DWORD type;

    len = sizeof(value);
    ret = RegQueryValueExW(service_current_key, NULL, NULL, &type,
        (BYTE *)&value, &len);
    if (ret == ERROR_SUCCESS && type == REG_DWORD)
        service_current = max(service_current, value + 1);
    RegSetValueExW(service_current_key, NULL, 0, REG_DWORD,
        (BYTE *)&service_current, sizeof(service_current));
    swprintf(name, ARRAY_SIZE(name), format, service_current);
    service_current++;
    return name;
}

static DWORD get_service_binary_path(const struct service_entry *service_entry, WCHAR **path)
{
    DWORD size = ExpandEnvironmentStringsW(service_entry->config.lpBinaryPathName, NULL, 0);

    *path = malloc(size * sizeof(WCHAR));
    if (!*path)
        return ERROR_NOT_ENOUGH_SERVER_MEMORY;

    ExpandEnvironmentStringsW(service_entry->config.lpBinaryPathName, *path, size);

    /* if service image is configured to systemdir, redirect it to wow64 systemdir */
    if (service_entry->is_wow64 && !(service_entry->config.dwServiceType & (SERVICE_FILE_SYSTEM_DRIVER | SERVICE_KERNEL_DRIVER)))
    {
        WCHAR system_dir[MAX_PATH], *redirected;
        DWORD len;

        GetSystemDirectoryW( system_dir, MAX_PATH );
        len = lstrlenW( system_dir );

        if (wcsnicmp( system_dir, *path, len ))
            return ERROR_SUCCESS;

        GetSystemWow64DirectoryW( system_dir, MAX_PATH );

        redirected = malloc( (wcslen( *path ) + wcslen( system_dir )) * sizeof(WCHAR) );
        if (!redirected)
        {
            free( *path );
            return ERROR_NOT_ENOUGH_SERVER_MEMORY;
        }

        lstrcpyW( redirected, system_dir );
        lstrcatW( redirected, &(*path)[len] );
        free( *path );
        *path = redirected;
        TRACE("redirected to %s\n", debugstr_w(redirected));
    }

    return ERROR_SUCCESS;
}

static DWORD get_winedevice_binary_path(struct service_entry *service_entry, WCHAR **path, BOOL *is_wow64)
{
    static const WCHAR winedeviceW[] = {'\\','w','i','n','e','d','e','v','i','c','e','.','e','x','e',0};
    WCHAR system_dir[MAX_PATH];
    DWORD type;

    if (!is_win64)
        *is_wow64 = FALSE;
    else if (GetBinaryTypeW(*path, &type))
        *is_wow64 = (type == SCS_32BIT_BINARY);
    else
        *is_wow64 = service_entry->is_wow64;

    GetSystemDirectoryW(system_dir, MAX_PATH);
    free(*path);
    if (!(*path = malloc(wcslen(system_dir) * sizeof(WCHAR) + sizeof(winedeviceW))))
       return ERROR_NOT_ENOUGH_SERVER_MEMORY;

    lstrcpyW(*path, system_dir);
    lstrcatW(*path, winedeviceW);
    return ERROR_SUCCESS;
}

static struct process_entry *get_winedevice_process(struct service_entry *service_entry, WCHAR *path, BOOL is_wow64)
{
    struct service_entry *winedevice_entry;

    if (!service_entry->config.lpLoadOrderGroup)
        return NULL;

    LIST_FOR_EACH_ENTRY(winedevice_entry, &service_entry->db->services, struct service_entry, entry)
    {
        if (winedevice_entry->status.dwCurrentState != SERVICE_START_PENDING &&
            winedevice_entry->status.dwCurrentState != SERVICE_RUNNING) continue;
        if (!winedevice_entry->process) continue;

        if (winedevice_entry->is_wow64 != is_wow64) continue;
        if (!winedevice_entry->config.lpBinaryPathName) continue;
        if (lstrcmpW(winedevice_entry->config.lpBinaryPathName, path)) continue;

        if (!winedevice_entry->config.lpLoadOrderGroup) continue;
        if (lstrcmpW(winedevice_entry->config.lpLoadOrderGroup, service_entry->config.lpLoadOrderGroup)) continue;

        return grab_process(winedevice_entry->process);
    }

    return NULL;
}

static DWORD add_winedevice_service(const struct service_entry *service, WCHAR *path, BOOL is_wow64,
                                    struct service_entry **entry)
{
    static const WCHAR format[] = {'W','i','n','e','d','e','v','i','c','e','%','u',0};
    static WCHAR name[ARRAY_SIZE(format) + 10]; /* lstrlenW("4294967295") */
    static DWORD current = 0;
    struct scmdatabase *db = service->db;
    DWORD err;

    for (;;)
    {
        swprintf(name, ARRAY_SIZE(name), format, ++current);
        if (!scmdatabase_find_service(db, name)) break;
    }

    err = service_create(name, entry);
    if (err != ERROR_SUCCESS)
        return err;

    (*entry)->is_wow64                  = is_wow64;
    (*entry)->config.dwServiceType      = SERVICE_WIN32_OWN_PROCESS;
    (*entry)->config.dwStartType        = SERVICE_DEMAND_START;
    (*entry)->status.dwServiceType      = (*entry)->config.dwServiceType;

    if (!((*entry)->config.lpBinaryPathName = wcsdup(path)))
        goto error;
    if (!((*entry)->config.lpServiceStartName = wcsdup(SZ_LOCAL_SYSTEM)))
        goto error;
    if (!((*entry)->config.lpDisplayName = wcsdup(name)))
        goto error;
    if (service->config.lpLoadOrderGroup &&
        !((*entry)->config.lpLoadOrderGroup = wcsdup(service->config.lpLoadOrderGroup)))
        goto error;

    (*entry)->db = db;

    list_add_tail(&db->services, &(*entry)->entry);
    mark_for_delete(*entry);
    return ERROR_SUCCESS;

error:
    free_service_entry(*entry);
    return ERROR_NOT_ENOUGH_SERVER_MEMORY;
}

static DWORD service_start_process(struct service_entry *service_entry, struct process_entry **new_process,
                                   BOOL *shared_process)
{
    struct process_entry *process;
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    BOOL is_wow64 = FALSE;
    HANDLE token;
    WCHAR *path;
    DWORD err;
    BOOL r;

    service_lock(service_entry);

    if ((process = service_entry->process))
    {
        if (WaitForSingleObject(process->process, 0) == WAIT_TIMEOUT)
        {
            service_unlock(service_entry);
            return ERROR_SERVICE_ALREADY_RUNNING;
        }
        service_entry->process = NULL;
        process->use_count--;
        release_process(process);
    }

    service_entry->force_shutdown = FALSE;

    if ((err = get_service_binary_path(service_entry, &path)))
    {
        service_unlock(service_entry);
        return err;
    }

    if (service_entry->config.dwServiceType == SERVICE_KERNEL_DRIVER ||
        service_entry->config.dwServiceType == SERVICE_FILE_SYSTEM_DRIVER)
    {
        struct service_entry *winedevice_entry;
        WCHAR *group;

        if ((err = get_winedevice_binary_path(service_entry, &path, &is_wow64)))
        {
            service_unlock(service_entry);
            free(path);
            return err;
        }

        if ((process = get_winedevice_process(service_entry, path, is_wow64)))
        {
            free(path);
            goto found;
        }

        err = add_winedevice_service(service_entry, path, is_wow64, &winedevice_entry);
        free(path);
        if (err != ERROR_SUCCESS)
        {
            service_unlock(service_entry);
            return err;
        }

        group = wcsdup(winedevice_entry->config.lpLoadOrderGroup);
        service_unlock(service_entry);

        err = service_start(winedevice_entry, group != NULL, (const WCHAR **)&group);
        free(group);
        if (err != ERROR_SUCCESS)
        {
            release_service(winedevice_entry);
            return err;
        }

        service_lock(service_entry);
        process = grab_process(winedevice_entry->process);
        release_service(winedevice_entry);

        if (!process)
        {
            service_unlock(service_entry);
            return ERROR_SERVICE_REQUEST_TIMEOUT;
        }

found:
        service_entry->status.dwCurrentState = SERVICE_START_PENDING;
        service_entry->status.dwControlsAccepted = 0;
        ResetEvent(service_entry->status_changed_event);

        service_entry->process = grab_process(process);
        service_entry->shared_process = *shared_process = TRUE;
        process->use_count++;
        service_unlock(service_entry);

        err = WaitForSingleObject(process->control_mutex, 30000);
        if (err != WAIT_OBJECT_0)
        {
            release_process(process);
            return ERROR_SERVICE_REQUEST_TIMEOUT;
        }

        *new_process = process;
        return ERROR_SUCCESS;
    }

    if ((err = process_create(service_get_pipe_name(), &process)))
    {
        WINE_ERR("failed to create process object for %s, error = %lu\n",
                 wine_dbgstr_w(service_entry->name), err);
        service_unlock(service_entry);
        free(path);
        return err;
    }

    ZeroMemory(&si, sizeof(STARTUPINFOW));
    si.cb = sizeof(STARTUPINFOW);
    if (!(service_entry->config.dwServiceType & SERVICE_INTERACTIVE_PROCESS))
    {
        static WCHAR desktopW[] = {'_','_','w','i','n','e','s','e','r','v','i','c','e','_','w','i','n','s','t','a','t','i','o','n','\\','D','e','f','a','u','l','t',0};
        si.lpDesktop = desktopW;
    }

    if (!environment && OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE, &token))
    {
        WCHAR val[16];
        CreateEnvironmentBlock(&environment, token, FALSE);
        if (GetEnvironmentVariableW( L"WINEBOOTSTRAPMODE", val, ARRAY_SIZE(val) ))
        {
            UNICODE_STRING name = RTL_CONSTANT_STRING(L"WINEBOOTSTRAPMODE");
            UNICODE_STRING value;

            RtlInitUnicodeString( &value, val );
            RtlSetEnvironmentVariable( (WCHAR **)&environment, &name, &value );
        }
        CloseHandle(token);
    }

    service_entry->status.dwCurrentState = SERVICE_START_PENDING;
    service_entry->status.dwControlsAccepted = 0;
    ResetEvent(service_entry->status_changed_event);

    scmdatabase_add_process(service_entry->db, process);
    service_entry->process = grab_process(process);
    service_entry->shared_process = *shared_process = FALSE;
    process->use_count++;
    service_unlock(service_entry);

    r = CreateProcessW(NULL, path, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT | DETACHED_PROCESS, environment, NULL, &si, &pi);
    free(path);
    if (!r)
    {
        err = GetLastError();
        process_terminate(process);
        release_process(process);
        return err;
    }
    if (!AssignProcessToJobObject(job_object, pi.hProcess))
        WINE_ERR("Could not add object to job.\n");

    process->process_id = pi.dwProcessId;
    process->process = pi.hProcess;
    CloseHandle( pi.hThread );

    *new_process = process;
    return ERROR_SUCCESS;
}

static DWORD service_wait_for_startup(struct service_entry *service, struct process_entry *process)
{
    HANDLE handles[2] = { service->status_changed_event, process->process };
    DWORD result;

    result = WaitForMultipleObjects( 2, handles, FALSE, service_pipe_timeout );
    if (result != WAIT_OBJECT_0)
        return ERROR_SERVICE_REQUEST_TIMEOUT;

    service_lock(service);
    result = service->status.dwCurrentState;
    service_unlock(service);

    return (result == SERVICE_START_PENDING || result == SERVICE_RUNNING) ?
           ERROR_SUCCESS : ERROR_SERVICE_REQUEST_TIMEOUT;
}

/******************************************************************************
 * process_send_start_message
 */
static DWORD process_send_start_message(struct process_entry *process, BOOL shared_process,
                                        const WCHAR *name, const WCHAR **argv, DWORD argc)
{
    OVERLAPPED overlapped;
    DWORD i, len, result;
    WCHAR *str, *p;

    WINE_TRACE("%p %s %p %ld\n", process, wine_dbgstr_w(name), argv, argc);

    overlapped.hEvent = process->overlapped_event;
    if (!ConnectNamedPipe(process->control_pipe, &overlapped))
    {
        if (GetLastError() == ERROR_IO_PENDING)
        {
            HANDLE handles[2];
            handles[0] = process->overlapped_event;
            handles[1] = process->process;
            if (WaitForMultipleObjects( 2, handles, FALSE, service_pipe_timeout ) != WAIT_OBJECT_0)
                CancelIo(process->control_pipe);
            if (!GetOverlappedResult(process->control_pipe, &overlapped, &len, FALSE))
            {
                WINE_ERR("service %s failed to start\n", wine_dbgstr_w(name));
                return ERROR_SERVICE_REQUEST_TIMEOUT;
            }
        }
        else if (GetLastError() != ERROR_PIPE_CONNECTED)
        {
            WINE_ERR("pipe connect failed\n");
            return ERROR_SERVICE_REQUEST_TIMEOUT;
        }
    }

    len = lstrlenW(name) + 1;
    for (i = 0; i < argc; i++)
        len += lstrlenW(argv[i])+1;
    len = (len + 1) * sizeof(WCHAR);

    if (!(str = malloc(len)))
        return ERROR_NOT_ENOUGH_SERVER_MEMORY;

    p = str;
    lstrcpyW(p, name);
    p += lstrlenW(name) + 1;
    for (i = 0; i < argc; i++)
    {
        lstrcpyW(p, argv[i]);
        p += lstrlenW(p) + 1;
    }
    *p = 0;

    if (!process_send_control(process, shared_process, name,
                              SERVICE_CONTROL_START, (const BYTE *)str, len, &result))
        result = ERROR_SERVICE_REQUEST_TIMEOUT;

    free(str);
    return result;
}

DWORD service_start(struct service_entry *service, DWORD service_argc, LPCWSTR *service_argv)
{
    struct process_entry *process = NULL;
    BOOL shared_process;
    DWORD err;

    err = service_start_process(service, &process, &shared_process);
    if (err == ERROR_SUCCESS)
    {
        err = process_send_start_message(process, shared_process, service->name, service_argv, service_argc);

        if (err == ERROR_SUCCESS)
            err = service_wait_for_startup(service, process);

        if (err != ERROR_SUCCESS)
        {
            service_lock(service);
            if (service->process)
            {
                service->status.dwCurrentState = SERVICE_STOPPED;
                service->process = NULL;
                if (!--process->use_count) process_terminate(process);
                release_process(process);
            }
            service_unlock(service);
        }

        ReleaseMutex(process->control_mutex);
        release_process(process);
    }

    WINE_TRACE("returning %ld\n", err);
    return err;
}

void process_terminate(struct process_entry *process)
{
    struct scmdatabase *db = process->db;
    struct service_entry *service;

    scmdatabase_lock(db);
    TerminateProcess(process->process, 0);
    LIST_FOR_EACH_ENTRY(service, &db->services, struct service_entry, entry)
    {
        if (service->process != process) continue;
        service->status.dwCurrentState = SERVICE_STOPPED;
        service->process = NULL;
        process->use_count--;
        release_process(process);
    }
    scmdatabase_unlock(db);
}

static void load_registry_parameters(void)
{
    static const WCHAR controlW[] =
        { 'S','y','s','t','e','m','\\',
          'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
          'C','o','n','t','r','o','l',0 };
    static const WCHAR pipetimeoutW[] =
        {'S','e','r','v','i','c','e','s','P','i','p','e','T','i','m','e','o','u','t',0};
    static const WCHAR killtimeoutW[] =
        {'W','a','i','t','T','o','K','i','l','l','S','e','r','v','i','c','e','T','i','m','e','o','u','t',0};
    static const WCHAR autostartdelayW[] =
        {'A','u','t','o','S','t','a','r','t','D','e','l','a','y',0};
    HKEY key;
    WCHAR buffer[64];
    DWORD type, count, val;

    if (RegOpenKeyW( HKEY_LOCAL_MACHINE, controlW, &key )) return;

    count = sizeof(buffer);
    if (!RegQueryValueExW( key, pipetimeoutW, NULL, &type, (BYTE *)buffer, &count ) &&
        type == REG_SZ && (val = wcstol( buffer, NULL, 10 )))
        service_pipe_timeout = val;

    count = sizeof(buffer);
    if (!RegQueryValueExW( key, killtimeoutW, NULL, &type, (BYTE *)buffer, &count ) &&
        type == REG_SZ && (val = wcstol( buffer, NULL, 10 )))
        service_kill_timeout = val;

    count = sizeof(val);
    if (!RegQueryValueExW( key, autostartdelayW, NULL, &type, (BYTE *)&val, &count ) && type == REG_DWORD)
        autostart_delay = val;

    RegCloseKey( key );
}

static DWORD WINAPI process_monitor_thread_proc( void *arg )
{
    struct scmdatabase *db = active_database;
    struct service_entry *service;
    struct process_entry *process;
    OVERLAPPED *overlapped;
    ULONG_PTR value;
    DWORD key, pid;

    while (GetQueuedCompletionStatus(job_completion_port, &key, &value, &overlapped, INFINITE))
    {
        if (!key)
            break;
        if (key != JOB_OBJECT_MSG_EXIT_PROCESS)
            continue;
        pid = (ULONG_PTR)overlapped;
        WINE_TRACE("pid %04lx exited.\n", pid);
        scmdatabase_lock(db);
        LIST_FOR_EACH_ENTRY(service, &db->services, struct service_entry, entry)
        {
            if (service->status.dwCurrentState != SERVICE_RUNNING || !service->process
                    || service->process->process_id != pid) continue;

            WINE_TRACE("Stopping service %s.\n", debugstr_w(service->config.lpBinaryPathName));
            service->status.dwCurrentState = SERVICE_STOPPED;
            service->status.dwControlsAccepted = 0;
            service->status.dwWin32ExitCode = ERROR_PROCESS_ABORTED;
            service->status.dwServiceSpecificExitCode = 0;
            service->status.dwCheckPoint = 0;
            service->status.dwWaitHint = 0;
            SetEvent(service->status_changed_event);

            process = service->process;
            service->process = NULL;
            process->use_count--;
            release_process(process);
            notify_service_state(service);
        }
        scmdatabase_unlock(db);
    }
    WINE_TRACE("Terminating.\n");
    return 0;
}

int __cdecl main(int argc, char *argv[])
{
    static const WCHAR service_current_key_str[] = { 'S','Y','S','T','E','M','\\',
        'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
        'C','o','n','t','r','o','l','\\',
        'S','e','r','v','i','c','e','C','u','r','r','e','n','t',0};
    static const WCHAR svcctl_started_event[] = SVCCTL_STARTED_EVENT;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION job_limit;
    JOBOBJECT_ASSOCIATE_COMPLETION_PORT port_info;
    HANDLE started_event, process_monitor_thread;
    DWORD err;

    job_object = CreateJobObjectW(NULL, NULL);
    job_limit.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_BREAKAWAY_OK | JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
    if (!SetInformationJobObject(job_object, JobObjectExtendedLimitInformation, &job_limit, sizeof(job_limit)))
    {
        WINE_ERR("Failed to initialized job object, err %lu.\n", GetLastError());
        return GetLastError();
    }
    job_completion_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
    port_info.CompletionPort = job_completion_port;
    port_info.CompletionKey = job_object;
    if (!SetInformationJobObject(job_object, JobObjectAssociateCompletionPortInformation,
            &port_info, sizeof(port_info)))
    {
        WINE_ERR("Failed to set completion port for job, err %lu.\n", GetLastError());
        return GetLastError();
    }

    started_event = CreateEventW(NULL, TRUE, FALSE, svcctl_started_event);

    err = RegCreateKeyExW(HKEY_LOCAL_MACHINE, service_current_key_str, 0,
        NULL, REG_OPTION_VOLATILE, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL,
        &service_current_key, NULL);
    if (err != ERROR_SUCCESS)
        return err;

    load_registry_parameters();
    err = scmdatabase_create(&active_database);
    if (err != ERROR_SUCCESS)
        return err;
    if ((err = scmdatabase_load_services(active_database)) != ERROR_SUCCESS)
        return err;
    if ((err = RPC_Init()) == ERROR_SUCCESS)
    {
        scmdatabase_autostart_services(active_database);
        process_monitor_thread = CreateThread(NULL, 0, process_monitor_thread_proc, NULL, 0, NULL);
        SetEvent(started_event);
        WaitForSingleObject(exit_event, INFINITE);
        PostQueuedCompletionStatus(job_completion_port, 0, 0, NULL);
        WaitForSingleObject(process_monitor_thread, INFINITE);
        scmdatabase_wait_terminate(active_database);
        if (delayed_autostart_cleanup)
        {
            CloseThreadpoolCleanupGroupMembers(delayed_autostart_cleanup, TRUE, NULL);
            CloseThreadpoolCleanupGroup(delayed_autostart_cleanup);
        }
        RPC_Stop();
    }
    scmdatabase_destroy(active_database);
    if (environment)
        DestroyEnvironmentBlock(environment);

    WINE_TRACE("services.exe exited with code %ld\n", err);
    return err;
}
