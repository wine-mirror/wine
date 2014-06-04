/*
 * Services.exe - RPC functions
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
#include <windows.h>
#include <winternl.h>
#include <winsvc.h>
#include <ntsecapi.h>
#include <rpc.h>

#include "wine/list.h"
#include "wine/unicode.h"
#include "wine/debug.h"

#include "services.h"
#include "svcctl.h"

extern HANDLE CDECL __wine_make_process_system(void);

WINE_DEFAULT_DEBUG_CHANNEL(service);

static const GENERIC_MAPPING g_scm_generic =
{
    (STANDARD_RIGHTS_READ | SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_QUERY_LOCK_STATUS),
    (STANDARD_RIGHTS_WRITE | SC_MANAGER_CREATE_SERVICE | SC_MANAGER_MODIFY_BOOT_CONFIG),
    (STANDARD_RIGHTS_EXECUTE | SC_MANAGER_CONNECT | SC_MANAGER_LOCK),
    SC_MANAGER_ALL_ACCESS
};

static const GENERIC_MAPPING g_svc_generic =
{
    (STANDARD_RIGHTS_READ | SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS | SERVICE_INTERROGATE | SERVICE_ENUMERATE_DEPENDENTS),
    (STANDARD_RIGHTS_WRITE | SERVICE_CHANGE_CONFIG),
    (STANDARD_RIGHTS_EXECUTE | SERVICE_START | SERVICE_STOP | SERVICE_PAUSE_CONTINUE | SERVICE_USER_DEFINED_CONTROL),
    SERVICE_ALL_ACCESS
};

typedef enum
{
    SC_HTYPE_DONT_CARE = 0,
    SC_HTYPE_MANAGER,
    SC_HTYPE_SERVICE
} SC_HANDLE_TYPE;

struct sc_handle
{
    SC_HANDLE_TYPE type;
    DWORD access;
};

struct sc_manager_handle       /* service control manager handle */
{
    struct sc_handle hdr;
    struct scmdatabase *db;
};

struct sc_service_handle       /* service handle */
{
    struct sc_handle hdr;
    struct service_entry *service_entry;
};

struct sc_lock
{
    struct scmdatabase *db;
};

static HANDLE timeout_queue_event;
static CRITICAL_SECTION timeout_queue_cs;
static CRITICAL_SECTION_DEBUG timeout_queue_cs_debug =
{
    0, 0, &timeout_queue_cs,
    { &timeout_queue_cs_debug.ProcessLocksList, &timeout_queue_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": timeout_queue_cs") }
};
static CRITICAL_SECTION timeout_queue_cs = { &timeout_queue_cs_debug, -1, 0, 0, 0, 0 };
static struct list timeout_queue = LIST_INIT(timeout_queue);
struct timeout_queue_elem
{
    struct list entry;

    FILETIME time;
    void (*func)(struct service_entry*);
    struct service_entry *service_entry;
};

static void run_after_timeout(void (*func)(struct service_entry*), struct service_entry *service, DWORD timeout)
{
    struct timeout_queue_elem *elem = HeapAlloc(GetProcessHeap(), 0, sizeof(struct timeout_queue_elem));
    ULARGE_INTEGER time;

    if(!elem) {
        func(service);
        return;
    }

    service->ref_count++;
    elem->func = func;
    elem->service_entry = service;

    GetSystemTimeAsFileTime(&elem->time);
    time.u.LowPart = elem->time.dwLowDateTime;
    time.u.HighPart = elem->time.dwHighDateTime;
    time.QuadPart += timeout*10000000;
    elem->time.dwLowDateTime = time.u.LowPart;
    elem->time.dwHighDateTime = time.u.HighPart;

    EnterCriticalSection(&timeout_queue_cs);
    list_add_head(&timeout_queue, &elem->entry);
    LeaveCriticalSection(&timeout_queue_cs);

    SetEvent(timeout_queue_event);
}

static void free_service_strings(struct service_entry *old, struct service_entry *new)
{
    QUERY_SERVICE_CONFIGW *old_cfg = &old->config;
    QUERY_SERVICE_CONFIGW *new_cfg = &new->config;

    if (old_cfg->lpBinaryPathName != new_cfg->lpBinaryPathName)
        HeapFree(GetProcessHeap(), 0, old_cfg->lpBinaryPathName);

    if (old_cfg->lpLoadOrderGroup != new_cfg->lpLoadOrderGroup)
        HeapFree(GetProcessHeap(), 0, old_cfg->lpLoadOrderGroup);

    if (old_cfg->lpServiceStartName != new_cfg->lpServiceStartName)
        HeapFree(GetProcessHeap(), 0, old_cfg->lpServiceStartName);

    if (old_cfg->lpDisplayName != new_cfg->lpDisplayName)
        HeapFree(GetProcessHeap(), 0, old_cfg->lpDisplayName);

    if (old->dependOnServices != new->dependOnServices)
        HeapFree(GetProcessHeap(), 0, old->dependOnServices);

    if (old->dependOnGroups != new->dependOnGroups)
        HeapFree(GetProcessHeap(), 0, old->dependOnGroups);
}

/* Check if the given handle is of the required type and allows the requested access. */
static DWORD validate_context_handle(SC_RPC_HANDLE handle, DWORD type, DWORD needed_access, struct sc_handle **out_hdr)
{
    struct sc_handle *hdr = handle;

    if (type != SC_HTYPE_DONT_CARE && hdr->type != type)
    {
        WINE_ERR("Handle is of an invalid type (%d, %d)\n", hdr->type, type);
        return ERROR_INVALID_HANDLE;
    }

    if ((needed_access & hdr->access) != needed_access)
    {
        WINE_ERR("Access denied - handle created with access %x, needed %x\n", hdr->access, needed_access);
        return ERROR_ACCESS_DENIED;
    }

    *out_hdr = hdr;
    return ERROR_SUCCESS;
}

static DWORD validate_scm_handle(SC_RPC_HANDLE handle, DWORD needed_access, struct sc_manager_handle **manager)
{
    struct sc_handle *hdr;
    DWORD err = validate_context_handle(handle, SC_HTYPE_MANAGER, needed_access, &hdr);
    if (err == ERROR_SUCCESS)
        *manager = (struct sc_manager_handle *)hdr;
    return err;
}

static DWORD validate_service_handle(SC_RPC_HANDLE handle, DWORD needed_access, struct sc_service_handle **service)
{
    struct sc_handle *hdr;
    DWORD err = validate_context_handle(handle, SC_HTYPE_SERVICE, needed_access, &hdr);
    if (err == ERROR_SUCCESS)
        *service = (struct sc_service_handle *)hdr;
    return err;
}

DWORD __cdecl svcctl_OpenSCManagerW(
    MACHINE_HANDLEW MachineName, /* Note: this parameter is ignored */
    LPCWSTR DatabaseName,
    DWORD dwAccessMask,
    SC_RPC_HANDLE *handle)
{
    struct sc_manager_handle *manager;

    WINE_TRACE("(%s, %s, %x)\n", wine_dbgstr_w(MachineName), wine_dbgstr_w(DatabaseName), dwAccessMask);

    if (DatabaseName != NULL && DatabaseName[0])
    {
        if (strcmpW(DatabaseName, SERVICES_FAILED_DATABASEW) == 0)
            return ERROR_DATABASE_DOES_NOT_EXIST;
        if (strcmpW(DatabaseName, SERVICES_ACTIVE_DATABASEW) != 0)
            return ERROR_INVALID_NAME;
    }

    if (!(manager = HeapAlloc(GetProcessHeap(), 0, sizeof(*manager))))
        return ERROR_NOT_ENOUGH_SERVER_MEMORY;

    manager->hdr.type = SC_HTYPE_MANAGER;

    if (dwAccessMask & MAXIMUM_ALLOWED)
        dwAccessMask |= SC_MANAGER_ALL_ACCESS;
    manager->hdr.access = dwAccessMask;
    RtlMapGenericMask(&manager->hdr.access, &g_scm_generic);
    manager->db = active_database;
    *handle = &manager->hdr;

    return ERROR_SUCCESS;
}

static void SC_RPC_HANDLE_destroy(SC_RPC_HANDLE handle)
{
    struct sc_handle *hdr = handle;
    switch (hdr->type)
    {
        case SC_HTYPE_MANAGER:
        {
            struct sc_manager_handle *manager = (struct sc_manager_handle *)hdr;
            HeapFree(GetProcessHeap(), 0, manager);
            break;
        }
        case SC_HTYPE_SERVICE:
        {
            struct sc_service_handle *service = (struct sc_service_handle *)hdr;
            release_service(service->service_entry);
            HeapFree(GetProcessHeap(), 0, service);
            break;
        }
        default:
            WINE_ERR("invalid handle type %d\n", hdr->type);
            RpcRaiseException(ERROR_INVALID_HANDLE);
    }
}

DWORD __cdecl svcctl_GetServiceDisplayNameW(
    SC_RPC_HANDLE hSCManager,
    LPCWSTR lpServiceName,
    WCHAR *lpBuffer,
    DWORD *cchBufSize)
{
    struct sc_manager_handle *manager;
    struct service_entry *entry;
    DWORD err;

    WINE_TRACE("(%s, %d)\n", wine_dbgstr_w(lpServiceName), *cchBufSize);

    if ((err = validate_scm_handle(hSCManager, 0, &manager)) != ERROR_SUCCESS)
        return err;

    scmdatabase_lock_shared(manager->db);

    entry = scmdatabase_find_service(manager->db, lpServiceName);
    if (entry != NULL)
    {
        LPCWSTR name;
        int len;
        service_lock_shared(entry);
        name = get_display_name(entry);
        len = strlenW(name);
        if (len <= *cchBufSize)
        {
            err = ERROR_SUCCESS;
            memcpy(lpBuffer, name, (len + 1)*sizeof(*name));
        }
        else
            err = ERROR_INSUFFICIENT_BUFFER;
        *cchBufSize = len;
        service_unlock(entry);
    }
    else
        err = ERROR_SERVICE_DOES_NOT_EXIST;

    scmdatabase_unlock(manager->db);

    if (err != ERROR_SUCCESS)
        lpBuffer[0] = 0;

    return err;
}

DWORD __cdecl svcctl_GetServiceKeyNameW(
    SC_RPC_HANDLE hSCManager,
    LPCWSTR lpServiceDisplayName,
    WCHAR *lpBuffer,
    DWORD *cchBufSize)
{
    struct service_entry *entry;
    struct sc_manager_handle *manager;
    DWORD err;

    WINE_TRACE("(%s, %d)\n", wine_dbgstr_w(lpServiceDisplayName), *cchBufSize);

    if ((err = validate_scm_handle(hSCManager, 0, &manager)) != ERROR_SUCCESS)
        return err;

    scmdatabase_lock_shared(manager->db);

    entry = scmdatabase_find_service_by_displayname(manager->db, lpServiceDisplayName);
    if (entry != NULL)
    {
        int len;
        service_lock_shared(entry);
        len = strlenW(entry->name);
        if (len <= *cchBufSize)
        {
            err = ERROR_SUCCESS;
            memcpy(lpBuffer, entry->name, (len + 1)*sizeof(*entry->name));
        }
        else
            err = ERROR_INSUFFICIENT_BUFFER;
        *cchBufSize = len;
        service_unlock(entry);
    }
    else
        err = ERROR_SERVICE_DOES_NOT_EXIST;

    scmdatabase_unlock(manager->db);

    if (err != ERROR_SUCCESS)
        lpBuffer[0] = 0;

    return err;
}

static DWORD create_handle_for_service(struct service_entry *entry, DWORD dwDesiredAccess, SC_RPC_HANDLE *phService)
{
    struct sc_service_handle *service;

    if (!(service = HeapAlloc(GetProcessHeap(), 0, sizeof(*service))))
    {
        release_service(entry);
        return ERROR_NOT_ENOUGH_SERVER_MEMORY;
    }

    service->hdr.type = SC_HTYPE_SERVICE;
    service->hdr.access = dwDesiredAccess;
    RtlMapGenericMask(&service->hdr.access, &g_svc_generic);
    service->service_entry = entry;
    if (dwDesiredAccess & MAXIMUM_ALLOWED)
        dwDesiredAccess |= SERVICE_ALL_ACCESS;

    *phService = &service->hdr;
    return ERROR_SUCCESS;
}

DWORD __cdecl svcctl_OpenServiceW(
    SC_RPC_HANDLE hSCManager,
    LPCWSTR lpServiceName,
    DWORD dwDesiredAccess,
    SC_RPC_HANDLE *phService)
{
    struct sc_manager_handle *manager;
    struct service_entry *entry;
    DWORD err;

    WINE_TRACE("(%s, 0x%x)\n", wine_dbgstr_w(lpServiceName), dwDesiredAccess);

    if ((err = validate_scm_handle(hSCManager, 0, &manager)) != ERROR_SUCCESS)
        return err;
    if (!validate_service_name(lpServiceName))
        return ERROR_INVALID_NAME;

    scmdatabase_lock_shared(manager->db);
    entry = scmdatabase_find_service(manager->db, lpServiceName);
    if (entry != NULL)
        InterlockedIncrement(&entry->ref_count);
    scmdatabase_unlock(manager->db);

    if (entry == NULL)
        return ERROR_SERVICE_DOES_NOT_EXIST;

    return create_handle_for_service(entry, dwDesiredAccess, phService);
}

static DWORD parse_dependencies(const WCHAR *dependencies, struct service_entry *entry)
{
    WCHAR *services = NULL, *groups, *s;
    DWORD len, len_services = 0, len_groups = 0;
    const WCHAR *ptr = dependencies;

    if (!dependencies || !dependencies[0])
    {
        entry->dependOnServices = NULL;
        entry->dependOnGroups = NULL;
        return ERROR_SUCCESS;
    }

    while (*ptr)
    {
        len = strlenW(ptr) + 1;
        if (ptr[0] == '+' && ptr[1])
            len_groups += len - 1;
        else
            len_services += len;
        ptr += len;
    }
    if (!len_services) entry->dependOnServices = NULL;
    else
    {
        services = HeapAlloc(GetProcessHeap(), 0, (len_services + 1) * sizeof(WCHAR));
        if (!services)
            return ERROR_OUTOFMEMORY;

        s = services;
        ptr = dependencies;
        while (*ptr)
        {
            len = strlenW(ptr) + 1;
            if (*ptr != '+')
            {
                strcpyW(s, ptr);
                s += len;
            }
            ptr += len;
        }
        *s = 0;
        entry->dependOnServices = services;
    }
    if (!len_groups) entry->dependOnGroups = NULL;
    else
    {
        groups = HeapAlloc(GetProcessHeap(), 0, (len_groups + 1) * sizeof(WCHAR));
        if (!groups)
        {
            HeapFree(GetProcessHeap(), 0, services);
            return ERROR_OUTOFMEMORY;
        }
        s = groups;
        ptr = dependencies;
        while (*ptr)
        {
            len = strlenW(ptr) + 1;
            if (ptr[0] == '+' && ptr[1])
            {
                strcpyW(s, ptr + 1);
                s += len - 1;
            }
            ptr += len;
        }
        *s = 0;
        entry->dependOnGroups = groups;
    }

    return ERROR_SUCCESS;
}

DWORD __cdecl svcctl_CreateServiceW(
    SC_RPC_HANDLE hSCManager,
    LPCWSTR lpServiceName,
    LPCWSTR lpDisplayName,
    DWORD dwDesiredAccess,
    DWORD dwServiceType,
    DWORD dwStartType,
    DWORD dwErrorControl,
    LPCWSTR lpBinaryPathName,
    LPCWSTR lpLoadOrderGroup,
    DWORD *lpdwTagId,
    const BYTE *lpDependencies,
    DWORD dwDependenciesSize,
    LPCWSTR lpServiceStartName,
    const BYTE *lpPassword,
    DWORD dwPasswordSize,
    SC_RPC_HANDLE *phService)
{
    struct service_entry *entry, *found;
    struct sc_manager_handle *manager;
    DWORD err;

    WINE_TRACE("(%s, %s, 0x%x, %s)\n", wine_dbgstr_w(lpServiceName), wine_dbgstr_w(lpDisplayName), dwDesiredAccess, wine_dbgstr_w(lpBinaryPathName));

    if ((err = validate_scm_handle(hSCManager, SC_MANAGER_CREATE_SERVICE, &manager)) != ERROR_SUCCESS)
        return err;

    if (!validate_service_name(lpServiceName))
        return ERROR_INVALID_NAME;
    if (!check_multisz((LPCWSTR)lpDependencies, dwDependenciesSize) || !lpServiceName[0] || !lpBinaryPathName[0])
        return ERROR_INVALID_PARAMETER;

    if (lpPassword)
        WINE_FIXME("Don't know how to add a password\n");   /* I always get ERROR_GEN_FAILURE */

    err = service_create(lpServiceName, &entry);
    if (err != ERROR_SUCCESS)
        return err;

    err = parse_dependencies((LPCWSTR)lpDependencies, entry);
    if (err != ERROR_SUCCESS) {
        free_service_entry(entry);
        return err;
    }

    entry->ref_count = 1;
    entry->config.dwServiceType = entry->status.dwServiceType = dwServiceType;
    entry->config.dwStartType = dwStartType;
    entry->config.dwErrorControl = dwErrorControl;
    entry->config.lpBinaryPathName = strdupW(lpBinaryPathName);
    entry->config.lpLoadOrderGroup = strdupW(lpLoadOrderGroup);
    entry->config.lpServiceStartName = strdupW(lpServiceStartName);
    entry->config.lpDisplayName = strdupW(lpDisplayName);

    if (lpdwTagId)      /* TODO: In most situations a non-NULL TagId will generate an ERROR_INVALID_PARAMETER. */
        entry->config.dwTagId = *lpdwTagId;
    else
        entry->config.dwTagId = 0;

    /* other fields NULL*/

    if (!validate_service_config(entry))
    {
        WINE_ERR("Invalid data while trying to create service\n");
        free_service_entry(entry);
        return ERROR_INVALID_PARAMETER;
    }

    scmdatabase_lock_exclusive(manager->db);

    if ((found = scmdatabase_find_service(manager->db, lpServiceName)))
    {
        service_lock_exclusive(found);
        err = is_marked_for_delete(found) ? ERROR_SERVICE_MARKED_FOR_DELETE : ERROR_SERVICE_EXISTS;
        service_unlock(found);
        scmdatabase_unlock(manager->db);
        free_service_entry(entry);
        return err;
    }

    if (scmdatabase_find_service_by_displayname(manager->db, get_display_name(entry)))
    {
        scmdatabase_unlock(manager->db);
        free_service_entry(entry);
        return ERROR_DUPLICATE_SERVICE_NAME;
    }

    err = scmdatabase_add_service(manager->db, entry);
    if (err != ERROR_SUCCESS)
    {
        scmdatabase_unlock(manager->db);
        free_service_entry(entry);
        return err;
    }
    scmdatabase_unlock(manager->db);

    return create_handle_for_service(entry, dwDesiredAccess, phService);
}

DWORD __cdecl svcctl_DeleteService(
    SC_RPC_HANDLE hService)
{
    struct sc_service_handle *service;
    DWORD err;

    if ((err = validate_service_handle(hService, DELETE, &service)) != ERROR_SUCCESS)
        return err;

    service_lock_exclusive(service->service_entry);

    if (!is_marked_for_delete(service->service_entry))
        err = mark_for_delete(service->service_entry);
    else
        err = ERROR_SERVICE_MARKED_FOR_DELETE;

    service_unlock(service->service_entry);

    return err;
}

DWORD __cdecl svcctl_QueryServiceConfigW(
        SC_RPC_HANDLE hService,
        QUERY_SERVICE_CONFIGW *config)
{
    struct sc_service_handle *service;
    DWORD err;

    WINE_TRACE("(%p)\n", config);

    if ((err = validate_service_handle(hService, SERVICE_QUERY_CONFIG, &service)) != 0)
        return err;

    service_lock_shared(service->service_entry);
    config->dwServiceType = service->service_entry->config.dwServiceType;
    config->dwStartType = service->service_entry->config.dwStartType;
    config->dwErrorControl = service->service_entry->config.dwErrorControl;
    config->lpBinaryPathName = strdupW(service->service_entry->config.lpBinaryPathName);
    config->lpLoadOrderGroup = strdupW(service->service_entry->config.lpLoadOrderGroup);
    config->dwTagId = service->service_entry->config.dwTagId;
    config->lpDependencies = NULL; /* TODO */
    config->lpServiceStartName = strdupW(service->service_entry->config.lpServiceStartName);
    config->lpDisplayName = strdupW(service->service_entry->config.lpDisplayName);
    service_unlock(service->service_entry);

    return ERROR_SUCCESS;
}

DWORD __cdecl svcctl_ChangeServiceConfigW(
        SC_RPC_HANDLE hService,
        DWORD dwServiceType,
        DWORD dwStartType,
        DWORD dwErrorControl,
        LPCWSTR lpBinaryPathName,
        LPCWSTR lpLoadOrderGroup,
        DWORD *lpdwTagId,
        const BYTE *lpDependencies,
        DWORD dwDependenciesSize,
        LPCWSTR lpServiceStartName,
        const BYTE *lpPassword,
        DWORD dwPasswordSize,
        LPCWSTR lpDisplayName)
{
    struct service_entry new_entry, *entry;
    struct sc_service_handle *service;
    DWORD err;

    WINE_TRACE("\n");

    if ((err = validate_service_handle(hService, SERVICE_CHANGE_CONFIG, &service)) != 0)
        return err;

    if (!check_multisz((LPCWSTR)lpDependencies, dwDependenciesSize))
        return ERROR_INVALID_PARAMETER;

    /* first check if the new configuration is correct */
    service_lock_exclusive(service->service_entry);

    if (is_marked_for_delete(service->service_entry))
    {
        service_unlock(service->service_entry);
        return ERROR_SERVICE_MARKED_FOR_DELETE;
    }

    if (lpDisplayName != NULL &&
        (entry = scmdatabase_find_service_by_displayname(service->service_entry->db, lpDisplayName)) &&
        (entry != service->service_entry))
    {
        service_unlock(service->service_entry);
        return ERROR_DUPLICATE_SERVICE_NAME;
    }

    new_entry = *service->service_entry;

    if (dwServiceType != SERVICE_NO_CHANGE)
        new_entry.config.dwServiceType = dwServiceType;

    if (dwStartType != SERVICE_NO_CHANGE)
        new_entry.config.dwStartType = dwStartType;

    if (dwErrorControl != SERVICE_NO_CHANGE)
        new_entry.config.dwErrorControl = dwErrorControl;

    if (lpBinaryPathName != NULL)
        new_entry.config.lpBinaryPathName = (LPWSTR)lpBinaryPathName;

    if (lpLoadOrderGroup != NULL)
        new_entry.config.lpLoadOrderGroup = (LPWSTR)lpLoadOrderGroup;

    if (lpdwTagId != NULL)
        WINE_FIXME("Changing tag id not supported\n");

    if (lpServiceStartName != NULL)
        new_entry.config.lpServiceStartName = (LPWSTR)lpServiceStartName;

    if (lpPassword != NULL)
        WINE_FIXME("Setting password not supported\n");

    if (lpDisplayName != NULL)
        new_entry.config.lpDisplayName = (LPWSTR)lpDisplayName;

    err = parse_dependencies((LPCWSTR)lpDependencies, &new_entry);
    if (err != ERROR_SUCCESS)
    {
        service_unlock(service->service_entry);
        return err;
    }

    if (!validate_service_config(&new_entry))
    {
        WINE_ERR("The configuration after the change wouldn't be valid\n");
        service_unlock(service->service_entry);
        return ERROR_INVALID_PARAMETER;
    }

    /* configuration OK. The strings needs to be duplicated */
    if (lpBinaryPathName != NULL)
        new_entry.config.lpBinaryPathName = strdupW(lpBinaryPathName);

    if (lpLoadOrderGroup != NULL)
        new_entry.config.lpLoadOrderGroup = strdupW(lpLoadOrderGroup);

    if (lpServiceStartName != NULL)
        new_entry.config.lpServiceStartName = strdupW(lpServiceStartName);

    if (lpDisplayName != NULL)
        new_entry.config.lpDisplayName = strdupW(lpDisplayName);

    /* try to save to Registry, commit or rollback depending on success */
    err = save_service_config(&new_entry);
    if (ERROR_SUCCESS == err)
    {
        free_service_strings(service->service_entry, &new_entry);
        *service->service_entry = new_entry;
    }
    else free_service_strings(&new_entry, service->service_entry);
    service_unlock(service->service_entry);

    return err;
}

DWORD __cdecl svcctl_SetServiceStatus(
    SC_RPC_HANDLE hServiceStatus,
    LPSERVICE_STATUS lpServiceStatus)
{
    struct sc_service_handle *service;
    DWORD err;

    WINE_TRACE("(%p, %p)\n", hServiceStatus, lpServiceStatus);

    if ((err = validate_service_handle(hServiceStatus, SERVICE_SET_STATUS, &service)) != 0)
        return err;

    service_lock_exclusive(service->service_entry);
    /* FIXME: be a bit more discriminant about what parts of the status we set
     * and check that fields are valid */
    service->service_entry->status.dwServiceType = lpServiceStatus->dwServiceType;
    service->service_entry->status.dwCurrentState = lpServiceStatus->dwCurrentState;
    service->service_entry->status.dwControlsAccepted = lpServiceStatus->dwControlsAccepted;
    service->service_entry->status.dwWin32ExitCode = lpServiceStatus->dwWin32ExitCode;
    service->service_entry->status.dwServiceSpecificExitCode = lpServiceStatus->dwServiceSpecificExitCode;
    service->service_entry->status.dwCheckPoint = lpServiceStatus->dwCheckPoint;
    service->service_entry->status.dwWaitHint = lpServiceStatus->dwWaitHint;
    service_unlock(service->service_entry);

    if (lpServiceStatus->dwCurrentState == SERVICE_STOPPED)
        run_after_timeout(service_terminate, service->service_entry, service_kill_timeout);
    else if (service->service_entry->status_changed_event)
        SetEvent(service->service_entry->status_changed_event);

    return ERROR_SUCCESS;
}

DWORD __cdecl svcctl_ChangeServiceConfig2W( SC_RPC_HANDLE hService, DWORD level, SERVICE_CONFIG2W *config )
{
    struct sc_service_handle *service;
    DWORD err;

    if ((err = validate_service_handle(hService, SERVICE_CHANGE_CONFIG, &service)) != 0)
        return err;

    switch (level)
    {
    case SERVICE_CONFIG_DESCRIPTION:
        {
            WCHAR *descr = NULL;

            if (config->descr.lpDescription[0])
            {
                if (!(descr = strdupW( config->descr.lpDescription )))
                    return ERROR_NOT_ENOUGH_MEMORY;
            }

            WINE_TRACE( "changing service %p descr to %s\n", service, wine_dbgstr_w(descr) );
            service_lock_exclusive( service->service_entry );
            HeapFree( GetProcessHeap(), 0, service->service_entry->description );
            service->service_entry->description = descr;
            save_service_config( service->service_entry );
            service_unlock( service->service_entry );
        }
        break;
    case SERVICE_CONFIG_FAILURE_ACTIONS:
        WINE_FIXME( "SERVICE_CONFIG_FAILURE_ACTIONS not implemented: period %u msg %s cmd %s\n",
                    config->actions.dwResetPeriod,
                    wine_dbgstr_w(config->actions.lpRebootMsg),
                    wine_dbgstr_w(config->actions.lpCommand) );
        break;
    case SERVICE_CONFIG_PRESHUTDOWN_INFO:
        WINE_TRACE( "changing service %p preshutdown timeout to %d\n",
                service, config->preshutdown.dwPreshutdownTimeout );
        service_lock_exclusive( service->service_entry );
        service->service_entry->preshutdown_timeout = config->preshutdown.dwPreshutdownTimeout;
        save_service_config( service->service_entry );
        service_unlock( service->service_entry );
        break;
    default:
        WINE_FIXME("level %u not implemented\n", level);
        err = ERROR_INVALID_LEVEL;
        break;
    }
    return err;
}

DWORD __cdecl svcctl_QueryServiceConfig2W( SC_RPC_HANDLE hService, DWORD level,
                                           BYTE *buffer, DWORD size, LPDWORD needed )
{
    struct sc_service_handle *service;
    DWORD err;

    memset(buffer, 0, size);

    if ((err = validate_service_handle(hService, SERVICE_QUERY_STATUS, &service)) != 0)
        return err;

    switch (level)
    {
    case SERVICE_CONFIG_DESCRIPTION:
        {
            SERVICE_DESCRIPTIONW *descr = (SERVICE_DESCRIPTIONW *)buffer;

            service_lock_shared(service->service_entry);
            *needed = sizeof(*descr);
            if (service->service_entry->description)
                *needed += (strlenW(service->service_entry->description) + 1) * sizeof(WCHAR);
            if (size >= *needed)
            {
                if (service->service_entry->description)
                {
                    /* store a buffer offset instead of a pointer */
                    descr->lpDescription = (WCHAR *)((BYTE *)(descr + 1) - buffer);
                    strcpyW( (WCHAR *)(descr + 1), service->service_entry->description );
                }
                else descr->lpDescription = NULL;
            }
            else err = ERROR_INSUFFICIENT_BUFFER;
            service_unlock(service->service_entry);
        }
        break;

    case SERVICE_CONFIG_PRESHUTDOWN_INFO:
        service_lock_shared(service->service_entry);

        *needed = sizeof(SERVICE_PRESHUTDOWN_INFO);
        if (size >= *needed)
            ((LPSERVICE_PRESHUTDOWN_INFO)buffer)->dwPreshutdownTimeout =
                service->service_entry->preshutdown_timeout;
        else err = ERROR_INSUFFICIENT_BUFFER;

        service_unlock(service->service_entry);
        break;

    default:
        WINE_FIXME("level %u not implemented\n", level);
        err = ERROR_INVALID_LEVEL;
        break;
    }
    return err;
}

DWORD __cdecl svcctl_QueryServiceStatusEx(
    SC_RPC_HANDLE hService,
    SC_STATUS_TYPE InfoLevel,
    BYTE *lpBuffer,
    DWORD cbBufSize,
    LPDWORD pcbBytesNeeded)
{
    struct sc_service_handle *service;
    DWORD err;
    LPSERVICE_STATUS_PROCESS pSvcStatusData;

    memset(lpBuffer, 0, cbBufSize);

    if ((err = validate_service_handle(hService, SERVICE_QUERY_STATUS, &service)) != 0)
        return err;

    if (InfoLevel != SC_STATUS_PROCESS_INFO)
        return ERROR_INVALID_LEVEL;

    pSvcStatusData = (LPSERVICE_STATUS_PROCESS) lpBuffer;
    if (pSvcStatusData == NULL)
        return ERROR_INVALID_PARAMETER;

    if (cbBufSize < sizeof(SERVICE_STATUS_PROCESS))
    {
        if( pcbBytesNeeded != NULL)
            *pcbBytesNeeded = sizeof(SERVICE_STATUS_PROCESS);

        return ERROR_INSUFFICIENT_BUFFER;
    }

    service_lock_shared(service->service_entry);

    pSvcStatusData->dwServiceType = service->service_entry->status.dwServiceType;
    pSvcStatusData->dwCurrentState = service->service_entry->status.dwCurrentState;
    pSvcStatusData->dwControlsAccepted = service->service_entry->status.dwControlsAccepted;
    pSvcStatusData->dwWin32ExitCode = service->service_entry->status.dwWin32ExitCode;
    pSvcStatusData->dwServiceSpecificExitCode = service->service_entry->status.dwServiceSpecificExitCode;
    pSvcStatusData->dwCheckPoint = service->service_entry->status.dwCheckPoint;
    pSvcStatusData->dwWaitHint = service->service_entry->status.dwWaitHint;
    pSvcStatusData->dwProcessId = service->service_entry->status.dwProcessId;
    pSvcStatusData->dwServiceFlags = service->service_entry->status.dwServiceFlags;

    service_unlock(service->service_entry);

    return ERROR_SUCCESS;
}

/******************************************************************************
 * service_accepts_control
 */
static BOOL service_accepts_control(const struct service_entry *service, DWORD dwControl)
{
    DWORD a = service->status.dwControlsAccepted;

    switch (dwControl)
    {
    case SERVICE_CONTROL_INTERROGATE:
        return TRUE;
    case SERVICE_CONTROL_STOP:
        if (a&SERVICE_ACCEPT_STOP)
            return TRUE;
        break;
    case SERVICE_CONTROL_SHUTDOWN:
        if (a&SERVICE_ACCEPT_SHUTDOWN)
            return TRUE;
        break;
    case SERVICE_CONTROL_PAUSE:
    case SERVICE_CONTROL_CONTINUE:
        if (a&SERVICE_ACCEPT_PAUSE_CONTINUE)
            return TRUE;
        break;
    case SERVICE_CONTROL_PARAMCHANGE:
        if (a&SERVICE_ACCEPT_PARAMCHANGE)
            return TRUE;
        break;
    case SERVICE_CONTROL_NETBINDADD:
    case SERVICE_CONTROL_NETBINDREMOVE:
    case SERVICE_CONTROL_NETBINDENABLE:
    case SERVICE_CONTROL_NETBINDDISABLE:
        if (a&SERVICE_ACCEPT_NETBINDCHANGE)
            return TRUE;
    case SERVICE_CONTROL_HARDWAREPROFILECHANGE:
        if (a&SERVICE_ACCEPT_HARDWAREPROFILECHANGE)
            return TRUE;
        break;
    case SERVICE_CONTROL_POWEREVENT:
        if (a&SERVICE_ACCEPT_POWEREVENT)
            return TRUE;
        break;
    case SERVICE_CONTROL_SESSIONCHANGE:
        if (a&SERVICE_ACCEPT_SESSIONCHANGE)
            return TRUE;
        break;
    }
    return FALSE;
}

/******************************************************************************
 * service_send_command
 */
BOOL service_send_command( struct service_entry *service, HANDLE pipe,
                           const void *data, DWORD size, DWORD *result )
{
    OVERLAPPED overlapped;
    DWORD count, ret;
    BOOL r;

    overlapped.hEvent = service->overlapped_event;
    r = WriteFile(pipe, data, size, &count, &overlapped);
    if (!r && GetLastError() == ERROR_IO_PENDING)
    {
        ret = WaitForSingleObject( service->overlapped_event, service_pipe_timeout );
        if (ret == WAIT_TIMEOUT)
        {
            WINE_ERR("sending command timed out\n");
            *result = ERROR_SERVICE_REQUEST_TIMEOUT;
            return FALSE;
        }
        r = GetOverlappedResult( pipe, &overlapped, &count, FALSE );
    }
    if (!r || count != size)
    {
        WINE_ERR("service protocol error - failed to write pipe!\n");
        *result  = (!r ? GetLastError() : ERROR_WRITE_FAULT);
        return FALSE;
    }
    r = ReadFile(pipe, result, sizeof *result, &count, &overlapped);
    if (!r && GetLastError() == ERROR_IO_PENDING)
    {
        ret = WaitForSingleObject( service->overlapped_event, service_pipe_timeout );
        if (ret == WAIT_TIMEOUT)
        {
            WINE_ERR("receiving command result timed out\n");
            *result = ERROR_SERVICE_REQUEST_TIMEOUT;
            return FALSE;
        }
        r = GetOverlappedResult( pipe, &overlapped, &count, FALSE );
    }
    if (!r || count != sizeof *result)
    {
        WINE_ERR("service protocol error - failed to read pipe "
            "r = %d  count = %d!\n", r, count);
        *result = (!r ? GetLastError() : ERROR_READ_FAULT);
        return FALSE;
    }

    *result = ERROR_SUCCESS;
    return TRUE;
}

/******************************************************************************
 * service_send_control
 */
static BOOL service_send_control(struct service_entry *service, HANDLE pipe, DWORD dwControl, DWORD *result)
{
    service_start_info *ssi;
    DWORD len;
    BOOL r;

    /* calculate how much space we need to send the startup info */
    len = strlenW(service->name) + 1;

    ssi = HeapAlloc(GetProcessHeap(),0,FIELD_OFFSET(service_start_info, data[len]));
    ssi->cmd = WINESERV_SENDCONTROL;
    ssi->control = dwControl;
    ssi->total_size = FIELD_OFFSET(service_start_info, data[len]);
    ssi->name_size = strlenW(service->name) + 1;
    strcpyW( ssi->data, service->name );

    r = service_send_command( service, pipe, ssi, ssi->total_size, result );
    HeapFree( GetProcessHeap(), 0, ssi );
    return r;
}

DWORD __cdecl svcctl_StartServiceW(
    SC_RPC_HANDLE hService,
    DWORD dwNumServiceArgs,
    LPCWSTR *lpServiceArgVectors)
{
    struct sc_service_handle *service;
    DWORD err;

    WINE_TRACE("(%p, %d, %p)\n", hService, dwNumServiceArgs, lpServiceArgVectors);

    if ((err = validate_service_handle(hService, SERVICE_START, &service)) != 0)
        return err;

    if (service->service_entry->config.dwStartType == SERVICE_DISABLED)
        return ERROR_SERVICE_DISABLED;

    err = service_start(service->service_entry, dwNumServiceArgs, lpServiceArgVectors);

    return err;
}

DWORD __cdecl svcctl_ControlService(
    SC_RPC_HANDLE hService,
    DWORD dwControl,
    SERVICE_STATUS *lpServiceStatus)
{
    DWORD access_required;
    struct sc_service_handle *service;
    DWORD result;
    BOOL ret;
    HANDLE control_mutex;

    WINE_TRACE("(%p, %d, %p)\n", hService, dwControl, lpServiceStatus);

    switch (dwControl)
    {
    case SERVICE_CONTROL_CONTINUE:
    case SERVICE_CONTROL_NETBINDADD:
    case SERVICE_CONTROL_NETBINDDISABLE:
    case SERVICE_CONTROL_NETBINDENABLE:
    case SERVICE_CONTROL_NETBINDREMOVE:
    case SERVICE_CONTROL_PARAMCHANGE:
    case SERVICE_CONTROL_PAUSE:
        access_required = SERVICE_PAUSE_CONTINUE;
        break;
    case SERVICE_CONTROL_INTERROGATE:
        access_required = SERVICE_INTERROGATE;
        break;
    case SERVICE_CONTROL_STOP:
        access_required = SERVICE_STOP;
        break;
    default:
        if (dwControl >= 128 && dwControl <= 255)
            access_required = SERVICE_USER_DEFINED_CONTROL;
        else
            return ERROR_INVALID_PARAMETER;
    }

    if ((result = validate_service_handle(hService, access_required, &service)) != 0)
        return result;

    service_lock_exclusive(service->service_entry);

    result = ERROR_SUCCESS;
    switch (service->service_entry->status.dwCurrentState)
    {
    case SERVICE_STOPPED:
        result = ERROR_SERVICE_NOT_ACTIVE;
        break;
    case SERVICE_START_PENDING:
        if (dwControl==SERVICE_CONTROL_STOP)
            break;
        /* fall through */
    case SERVICE_STOP_PENDING:
        result = ERROR_SERVICE_CANNOT_ACCEPT_CTRL;
        break;
    }

    if (result==ERROR_SUCCESS && !service->service_entry->control_mutex) {
        result = ERROR_SERVICE_CANNOT_ACCEPT_CTRL;
        service_terminate(service->service_entry);
    }

    if (result != ERROR_SUCCESS)
    {
        if (lpServiceStatus)
        {
            lpServiceStatus->dwServiceType = service->service_entry->status.dwServiceType;
            lpServiceStatus->dwCurrentState = service->service_entry->status.dwCurrentState;
            lpServiceStatus->dwControlsAccepted = service->service_entry->status.dwControlsAccepted;
            lpServiceStatus->dwWin32ExitCode = service->service_entry->status.dwWin32ExitCode;
            lpServiceStatus->dwServiceSpecificExitCode = service->service_entry->status.dwServiceSpecificExitCode;
            lpServiceStatus->dwCheckPoint = service->service_entry->status.dwCheckPoint;
            lpServiceStatus->dwWaitHint = service->service_entry->status.dwWaitHint;
        }
        service_unlock(service->service_entry);
        return result;
    }

    if (!service_accepts_control(service->service_entry, dwControl))
    {
        service_unlock(service->service_entry);
        return ERROR_INVALID_SERVICE_CONTROL;
    }

    /* prevent races by caching control_mutex and clearing it on
     * stop instead of outside the services lock */
    control_mutex = service->service_entry->control_mutex;
    if (dwControl == SERVICE_CONTROL_STOP)
        service->service_entry->control_mutex = NULL;

    service_unlock(service->service_entry);

    ret = WaitForSingleObject(control_mutex, 30000);
    if (ret == WAIT_OBJECT_0)
    {
        service_send_control(service->service_entry, service->service_entry->control_pipe,
                dwControl, &result);

        if (lpServiceStatus)
        {
            service_lock_shared(service->service_entry);
            lpServiceStatus->dwServiceType = service->service_entry->status.dwServiceType;
            lpServiceStatus->dwCurrentState = service->service_entry->status.dwCurrentState;
            lpServiceStatus->dwControlsAccepted = service->service_entry->status.dwControlsAccepted;
            lpServiceStatus->dwWin32ExitCode = service->service_entry->status.dwWin32ExitCode;
            lpServiceStatus->dwServiceSpecificExitCode = service->service_entry->status.dwServiceSpecificExitCode;
            lpServiceStatus->dwCheckPoint = service->service_entry->status.dwCheckPoint;
            lpServiceStatus->dwWaitHint = service->service_entry->status.dwWaitHint;
            service_unlock(service->service_entry);
        }

        if (dwControl == SERVICE_CONTROL_STOP)
            CloseHandle(control_mutex);
        else
            ReleaseMutex(control_mutex);

        return result;
    }
    else
    {
        if (dwControl == SERVICE_CONTROL_STOP)
            CloseHandle(control_mutex);
        return ERROR_SERVICE_REQUEST_TIMEOUT;
    }
}

DWORD __cdecl svcctl_CloseServiceHandle(
    SC_RPC_HANDLE *handle)
{
    WINE_TRACE("(&%p)\n", *handle);

    SC_RPC_HANDLE_destroy(*handle);
    *handle = NULL;

    return ERROR_SUCCESS;
}

static void SC_RPC_LOCK_destroy(SC_RPC_LOCK hLock)
{
    struct sc_lock *lock = hLock;
    scmdatabase_unlock_startup(lock->db);
    HeapFree(GetProcessHeap(), 0, lock);
}

void __RPC_USER SC_RPC_LOCK_rundown(SC_RPC_LOCK hLock)
{
    SC_RPC_LOCK_destroy(hLock);
}

DWORD __cdecl svcctl_LockServiceDatabase(
    SC_RPC_HANDLE hSCManager,
    SC_RPC_LOCK *phLock)
{
    struct sc_manager_handle *manager;
    struct sc_lock *lock;
    DWORD err;

    WINE_TRACE("(%p, %p)\n", hSCManager, phLock);

    if ((err = validate_scm_handle(hSCManager, SC_MANAGER_LOCK, &manager)) != ERROR_SUCCESS)
        return err;

    err = scmdatabase_lock_startup(manager->db);
    if (err != ERROR_SUCCESS)
        return err;

    lock = HeapAlloc(GetProcessHeap(), 0, sizeof(struct sc_lock));
    if (!lock)
    {
        scmdatabase_unlock_startup(manager->db);
        return ERROR_NOT_ENOUGH_SERVER_MEMORY;
    }

    lock->db = manager->db;
    *phLock = lock;

    return ERROR_SUCCESS;
}

DWORD __cdecl svcctl_UnlockServiceDatabase(
    SC_RPC_LOCK *phLock)
{
    WINE_TRACE("(&%p)\n", *phLock);

    SC_RPC_LOCK_destroy(*phLock);
    *phLock = NULL;

    return ERROR_SUCCESS;
}

static BOOL map_state(DWORD state, DWORD mask)
{
    switch (state)
    {
    case SERVICE_START_PENDING:
    case SERVICE_STOP_PENDING:
    case SERVICE_RUNNING:
    case SERVICE_CONTINUE_PENDING:
    case SERVICE_PAUSE_PENDING:
    case SERVICE_PAUSED:
        if (SERVICE_ACTIVE & mask) return TRUE;
        break;
    case SERVICE_STOPPED:
        if (SERVICE_INACTIVE & mask) return TRUE;
        break;
    default:
        WINE_ERR("unknown state %u\n", state);
        break;
    }
    return FALSE;
}

DWORD __cdecl svcctl_EnumServicesStatusW(
    SC_RPC_HANDLE hmngr,
    DWORD type,
    DWORD state,
    BYTE *buffer,
    DWORD size,
    LPDWORD needed,
    LPDWORD returned)
{
    DWORD err, sz, total_size, num_services;
    DWORD_PTR offset;
    struct sc_manager_handle *manager;
    struct service_entry *service;
    ENUM_SERVICE_STATUSW *s;

    WINE_TRACE("(%p, 0x%x, 0x%x, %p, %u, %p, %p)\n", hmngr, type, state, buffer, size, needed, returned);

    if (!type || !state)
        return ERROR_INVALID_PARAMETER;

    if ((err = validate_scm_handle(hmngr, SC_MANAGER_ENUMERATE_SERVICE, &manager)) != ERROR_SUCCESS)
        return err;

    scmdatabase_lock_exclusive(manager->db);

    total_size = num_services = 0;
    LIST_FOR_EACH_ENTRY(service, &manager->db->services, struct service_entry, entry)
    {
        if ((service->status.dwServiceType & type) && map_state(service->status.dwCurrentState, state))
        {
            total_size += sizeof(ENUM_SERVICE_STATUSW);
            total_size += (strlenW(service->name) + 1) * sizeof(WCHAR);
            if (service->config.lpDisplayName)
            {
                total_size += (strlenW(service->config.lpDisplayName) + 1) * sizeof(WCHAR);
            }
            num_services++;
        }
    }
    *returned = 0;
    *needed = total_size;
    if (total_size > size)
    {
        scmdatabase_unlock(manager->db);
        return ERROR_MORE_DATA;
    }
    s = (ENUM_SERVICE_STATUSW *)buffer;
    offset = num_services * sizeof(ENUM_SERVICE_STATUSW);
    LIST_FOR_EACH_ENTRY(service, &manager->db->services, struct service_entry, entry)
    {
        if ((service->status.dwServiceType & type) && map_state(service->status.dwCurrentState, state))
        {
            sz = (strlenW(service->name) + 1) * sizeof(WCHAR);
            memcpy(buffer + offset, service->name, sz);
            s->lpServiceName = (WCHAR *)offset; /* store a buffer offset instead of a pointer */
            offset += sz;

            if (!service->config.lpDisplayName) s->lpDisplayName = NULL;
            else
            {
                sz = (strlenW(service->config.lpDisplayName) + 1) * sizeof(WCHAR);
                memcpy(buffer + offset, service->config.lpDisplayName, sz);
                s->lpDisplayName = (WCHAR *)offset;
                offset += sz;
            }
            memcpy(&s->ServiceStatus, &service->status, sizeof(SERVICE_STATUS));
            s++;
        }
    }
    *returned = num_services;
    *needed = 0;
    scmdatabase_unlock(manager->db);
    return ERROR_SUCCESS;
}

static struct service_entry *find_service_by_group(struct scmdatabase *db, const WCHAR *group)
{
    struct service_entry *service;
    LIST_FOR_EACH_ENTRY(service, &db->services, struct service_entry, entry)
    {
        if (service->config.lpLoadOrderGroup && !strcmpiW(group, service->config.lpLoadOrderGroup))
            return service;
    }
    return NULL;
}

static BOOL match_group(const WCHAR *g1, const WCHAR *g2)
{
    if (!g2) return TRUE;
    if (!g2[0] && (!g1 || !g1[0])) return TRUE;
    if (g1 && !strcmpW(g1, g2)) return TRUE;
    return FALSE;
}

DWORD __cdecl svcctl_EnumServicesStatusExW(
    SC_RPC_HANDLE hmngr,
    DWORD type,
    DWORD state,
    BYTE *buffer,
    DWORD size,
    LPDWORD needed,
    LPDWORD returned,
    LPCWSTR group)
{
    DWORD err, sz, total_size, num_services;
    DWORD_PTR offset;
    struct sc_manager_handle *manager;
    struct service_entry *service;
    ENUM_SERVICE_STATUS_PROCESSW *s;

    WINE_TRACE("(%p, 0x%x, 0x%x, %p, %u, %p, %p, %s)\n", hmngr, type, state, buffer, size,
               needed, returned, wine_dbgstr_w(group));

    if (!type || !state)
        return ERROR_INVALID_PARAMETER;

    if ((err = validate_scm_handle(hmngr, SC_MANAGER_ENUMERATE_SERVICE, &manager)) != ERROR_SUCCESS)
        return err;

    scmdatabase_lock_exclusive(manager->db);

    if (group && !find_service_by_group(manager->db, group))
    {
        scmdatabase_unlock(manager->db);
        return ERROR_SERVICE_DOES_NOT_EXIST;
    }

    total_size = num_services = 0;
    LIST_FOR_EACH_ENTRY(service, &manager->db->services, struct service_entry, entry)
    {
        if ((service->status.dwServiceType & type) && map_state(service->status.dwCurrentState, state)
            && match_group(service->config.lpLoadOrderGroup, group))
        {
            total_size += sizeof(ENUM_SERVICE_STATUS_PROCESSW);
            total_size += (strlenW(service->name) + 1) * sizeof(WCHAR);
            if (service->config.lpDisplayName)
            {
                total_size += (strlenW(service->config.lpDisplayName) + 1) * sizeof(WCHAR);
            }
            num_services++;
        }
    }
    *returned = 0;
    *needed = total_size;
    if (total_size > size)
    {
        scmdatabase_unlock(manager->db);
        return ERROR_MORE_DATA;
    }
    s = (ENUM_SERVICE_STATUS_PROCESSW *)buffer;
    offset = num_services * sizeof(ENUM_SERVICE_STATUS_PROCESSW);
    LIST_FOR_EACH_ENTRY(service, &manager->db->services, struct service_entry, entry)
    {
        if ((service->status.dwServiceType & type) && map_state(service->status.dwCurrentState, state)
            && match_group(service->config.lpLoadOrderGroup, group))
        {
            sz = (strlenW(service->name) + 1) * sizeof(WCHAR);
            memcpy(buffer + offset, service->name, sz);
            s->lpServiceName = (WCHAR *)offset; /* store a buffer offset instead of a pointer */
            offset += sz;

            if (!service->config.lpDisplayName) s->lpDisplayName = NULL;
            else
            {
                sz = (strlenW(service->config.lpDisplayName) + 1) * sizeof(WCHAR);
                memcpy(buffer + offset, service->config.lpDisplayName, sz);
                s->lpDisplayName = (WCHAR *)offset;
                offset += sz;
            }
            s->ServiceStatusProcess = service->status;
            s++;
        }
    }
    *returned = num_services;
    *needed = 0;
    scmdatabase_unlock(manager->db);
    return ERROR_SUCCESS;
}

DWORD __cdecl svcctl_QueryServiceObjectSecurity(void)
{
    WINE_FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD __cdecl svcctl_SetServiceObjectSecurity(void)
{
    WINE_FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD __cdecl svcctl_QueryServiceStatus(void)
{
    WINE_FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD __cdecl svcctl_NotifyBootConfigStatus(void)
{
    WINE_FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD __cdecl svcctl_SCSetServiceBitsW(void)
{
    WINE_FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD __cdecl svcctl_EnumDependentServicesW(void)
{
    WINE_FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD __cdecl svcctl_QueryServiceLockStatusW(void)
{
    WINE_FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD __cdecl svcctl_SCSetServiceBitsA(void)
{
    WINE_FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD __cdecl svcctl_ChangeServiceConfigA(void)
{
    WINE_FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD __cdecl svcctl_CreateServiceA(void)
{
    WINE_FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD __cdecl svcctl_EnumDependentServicesA(void)
{
    WINE_FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD __cdecl svcctl_EnumServicesStatusA(void)
{
    WINE_FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD __cdecl svcctl_OpenSCManagerA(void)
{
    WINE_FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD __cdecl svcctl_OpenServiceA(void)
{
    WINE_FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD __cdecl svcctl_QueryServiceConfigA(void)
{
    WINE_FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD __cdecl svcctl_QueryServiceLockStatusA(void)
{
    WINE_FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD __cdecl svcctl_StartServiceA(void)
{
    WINE_FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD __cdecl svcctl_GetServiceDisplayNameA(void)
{
    WINE_FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD __cdecl svcctl_GetServiceKeyNameA(void)
{
    WINE_FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD __cdecl svcctl_GetCurrentGroupStateW(void)
{
    WINE_FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD __cdecl svcctl_EnumServiceGroupW(void)
{
    WINE_FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD __cdecl svcctl_ChangeServiceConfig2A(void)
{
    WINE_FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD __cdecl svcctl_QueryServiceConfig2A(void)
{
    WINE_FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD RPC_Init(void)
{
    WCHAR transport[] = SVCCTL_TRANSPORT;
    WCHAR endpoint[] = SVCCTL_ENDPOINT;
    DWORD err;

    if ((err = RpcServerUseProtseqEpW(transport, 0, endpoint, NULL)) != ERROR_SUCCESS)
    {
        WINE_ERR("RpcServerUseProtseq failed with error %u\n", err);
        return err;
    }

    if ((err = RpcServerRegisterIf(svcctl_v2_0_s_ifspec, 0, 0)) != ERROR_SUCCESS)
    {
        WINE_ERR("RpcServerRegisterIf failed with error %u\n", err);
        return err;
    }

    if ((err = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE)) != ERROR_SUCCESS)
    {
        WINE_ERR("RpcServerListen failed with error %u\n", err);
        return err;
    }
    return ERROR_SUCCESS;
}

DWORD events_loop(void)
{
    struct timeout_queue_elem *iter, *iter_safe;
    DWORD err;
    HANDLE wait_handles[2];
    DWORD timeout = INFINITE;

    wait_handles[0] = __wine_make_process_system();
    wait_handles[1] = CreateEventW(NULL, FALSE, FALSE, NULL);
    timeout_queue_event = wait_handles[1];

    SetEvent(g_hStartedEvent);

    WINE_TRACE("Entered main loop\n");

    do
    {
        err = WaitForMultipleObjects(2, wait_handles, FALSE, timeout);
        WINE_TRACE("Wait returned %d\n", err);

        if(err==WAIT_OBJECT_0+1 || err==WAIT_TIMEOUT)
        {
            FILETIME cur_time;
            ULARGE_INTEGER time;

            GetSystemTimeAsFileTime(&cur_time);
            time.u.LowPart = cur_time.dwLowDateTime;
            time.u.HighPart = cur_time.dwHighDateTime;

            EnterCriticalSection(&timeout_queue_cs);
            timeout = INFINITE;
            LIST_FOR_EACH_ENTRY_SAFE(iter, iter_safe, &timeout_queue, struct timeout_queue_elem, entry)
            {
                if(CompareFileTime(&cur_time, &iter->time) >= 0)
                {
                    LeaveCriticalSection(&timeout_queue_cs);
                    iter->func(iter->service_entry);
                    EnterCriticalSection(&timeout_queue_cs);

                    release_service(iter->service_entry);
                    list_remove(&iter->entry);
                    HeapFree(GetProcessHeap(), 0, iter);
                }
                else
                {
                    ULARGE_INTEGER time_diff;

                    time_diff.u.LowPart = iter->time.dwLowDateTime;
                    time_diff.u.HighPart = iter->time.dwHighDateTime;
                    time_diff.QuadPart = (time_diff.QuadPart-time.QuadPart)/10000;

                    if(time_diff.QuadPart < timeout)
                        timeout = time_diff.QuadPart;
                }
            }
            LeaveCriticalSection(&timeout_queue_cs);

            if(timeout != INFINITE)
                timeout += 1000;
        }
    } while (err != WAIT_OBJECT_0);

    WINE_TRACE("Object signaled - wine shutdown\n");
    EnterCriticalSection(&timeout_queue_cs);
    LIST_FOR_EACH_ENTRY_SAFE(iter, iter_safe, &timeout_queue, struct timeout_queue_elem, entry)
    {
        LeaveCriticalSection(&timeout_queue_cs);
        iter->func(iter->service_entry);
        EnterCriticalSection(&timeout_queue_cs);

        release_service(iter->service_entry);
        list_remove(&iter->entry);
        HeapFree(GetProcessHeap(), 0, iter);
    }
    LeaveCriticalSection(&timeout_queue_cs);

    CloseHandle(wait_handles[0]);
    CloseHandle(wait_handles[1]);
    return ERROR_SUCCESS;
}

void __RPC_USER SC_RPC_HANDLE_rundown(SC_RPC_HANDLE handle)
{
    SC_RPC_HANDLE_destroy(handle);
}

void  __RPC_FAR * __RPC_USER MIDL_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

void __RPC_USER MIDL_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}
