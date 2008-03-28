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

extern HANDLE __wine_make_process_system(void);

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

/* Check if the given handle is of the required type and allows the requested access. */
static DWORD validate_context_handle(SC_RPC_HANDLE handle, DWORD type, DWORD needed_access, struct sc_handle **out_hdr)
{
    struct sc_handle *hdr = (struct sc_handle *)handle;

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

DWORD svcctl_OpenSCManagerW(
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
    struct sc_handle *hdr = (struct sc_handle *)handle;
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

DWORD svcctl_GetServiceDisplayNameW(
    SC_RPC_HANDLE hSCManager,
    LPCWSTR lpServiceName,
    WCHAR *lpBuffer,
    DWORD cchBufSize,
    DWORD *cchLength)
{
    struct sc_manager_handle *manager;
    struct service_entry *entry;
    DWORD err;

    WINE_TRACE("(%s, %d)\n", wine_dbgstr_w(lpServiceName), cchBufSize);

    if ((err = validate_scm_handle(hSCManager, 0, &manager)) != ERROR_SUCCESS)
        return err;

    scmdatabase_lock_shared(manager->db);

    entry = scmdatabase_find_service(manager->db, lpServiceName);
    if (entry != NULL)
    {
        LPCWSTR name;
        service_lock_shared(entry);
        name = get_display_name(entry);
        *cchLength = strlenW(name);
        if (*cchLength < cchBufSize)
        {
            err = ERROR_SUCCESS;
            lstrcpyW(lpBuffer, name);
        }
        else
            err = ERROR_INSUFFICIENT_BUFFER;
        service_unlock(entry);
    }
    else
    {
        *cchLength = 1;
        err = ERROR_SERVICE_DOES_NOT_EXIST;
    }

    scmdatabase_unlock(manager->db);

    if (err != ERROR_SUCCESS && cchBufSize > 0)
        lpBuffer[0] = 0;

    return err;
}

DWORD svcctl_GetServiceKeyNameW(
    SC_RPC_HANDLE hSCManager,
    LPCWSTR lpServiceDisplayName,
    WCHAR *lpBuffer,
    DWORD cchBufSize,
    DWORD *cchLength)
{
    struct service_entry *entry;
    struct sc_manager_handle *manager;
    DWORD err;

    WINE_TRACE("(%s, %d)\n", wine_dbgstr_w(lpServiceDisplayName), cchBufSize);

    if ((err = validate_scm_handle(hSCManager, 0, &manager)) != ERROR_SUCCESS)
        return err;

    scmdatabase_lock_shared(manager->db);

    entry = scmdatabase_find_service_by_displayname(manager->db, lpServiceDisplayName);
    if (entry != NULL)
    {
        service_lock_shared(entry);
        *cchLength = strlenW(entry->name);
        if (*cchLength < cchBufSize)
        {
            err = ERROR_SUCCESS;
            lstrcpyW(lpBuffer, entry->name);
        }
        else
            err = ERROR_INSUFFICIENT_BUFFER;
        service_unlock(entry);
    }
    else
    {
        *cchLength = 1;
        err = ERROR_SERVICE_DOES_NOT_EXIST;
    }

    scmdatabase_unlock(manager->db);

    if (err != ERROR_SUCCESS && cchBufSize > 0)
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

DWORD svcctl_OpenServiceW(
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
        entry->ref_count++;
    scmdatabase_unlock(manager->db);

    if (entry == NULL)
        return ERROR_SERVICE_DOES_NOT_EXIST;

    return create_handle_for_service(entry, dwDesiredAccess, phService);
}

DWORD svcctl_CreateServiceW(
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
    struct sc_manager_handle *manager;
    struct service_entry *entry;
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
    if (lpDependencies)
        WINE_FIXME("Dependencies not supported yet\n");

    err = service_create(lpServiceName, &entry);
    if (err != ERROR_SUCCESS)
        return err;
    entry->config.dwServiceType = dwServiceType;
    entry->config.dwStartType = dwStartType;
    entry->config.dwErrorControl = dwErrorControl;
    entry->config.lpBinaryPathName = strdupW(lpBinaryPathName);
    entry->config.lpLoadOrderGroup = strdupW(lpLoadOrderGroup);
    entry->config.lpServiceStartName = strdupW(lpServiceStartName);
    entry->config.lpDisplayName = strdupW(lpDisplayName);

    if (lpdwTagId)      /* TODO: in most situations a non-NULL tagid will generate a ERROR_INVALID_PARAMETER */
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

    if (scmdatabase_find_service(manager->db, lpServiceName))
    {
        scmdatabase_unlock(manager->db);
        free_service_entry(entry);
        return ERROR_SERVICE_EXISTS;
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

DWORD svcctl_DeleteService(
    SC_RPC_HANDLE hService)
{
    struct sc_service_handle *service;
    DWORD err;

    if ((err = validate_service_handle(hService, DELETE, &service)) != ERROR_SUCCESS)
        return err;

    scmdatabase_lock_exclusive(service->service_entry->db);
    service_lock_exclusive(service->service_entry);

    if (!is_marked_for_delete(service->service_entry))
        err = scmdatabase_remove_service(service->service_entry->db, service->service_entry);
    else
        err = ERROR_SERVICE_MARKED_FOR_DELETE;

    service_unlock(service->service_entry);
    scmdatabase_unlock(service->service_entry->db);

    return err;
}

DWORD svcctl_QueryServiceConfigW(
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

DWORD svcctl_ChangeServiceConfigW(
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
    struct service_entry new_entry;
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
        scmdatabase_find_service_by_displayname(service->service_entry->db, lpDisplayName))
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

    if (lpDependencies != NULL)
        WINE_FIXME("Chainging dependencies not supported\n");

    if (lpServiceStartName != NULL)
        new_entry.config.lpServiceStartName = (LPWSTR)lpServiceStartName;

    if (lpPassword != NULL)
        WINE_FIXME("Setting password not supported\n");

    if (lpDisplayName != NULL)
        new_entry.config.lpDisplayName = (LPWSTR)lpDisplayName;

    if (!validate_service_config(&new_entry))
    {
        WINE_ERR("The configuration after the change wouldn't be valid");
        service_unlock(service->service_entry);
        return ERROR_INVALID_PARAMETER;
    }

    /* configuration OK. The strings needs to be duplicated */
    if (lpBinaryPathName != NULL)
    {
        HeapFree(GetProcessHeap(), 0, service->service_entry->config.lpBinaryPathName);
        new_entry.config.lpBinaryPathName = strdupW(lpBinaryPathName);
    }

    if (lpLoadOrderGroup != NULL)
    {
        HeapFree(GetProcessHeap(), 0, service->service_entry->config.lpLoadOrderGroup);
        new_entry.config.lpLoadOrderGroup = strdupW(lpLoadOrderGroup);
    }

    if (lpServiceStartName != NULL)
    {
        HeapFree(GetProcessHeap(), 0, service->service_entry->config.lpServiceStartName);
        new_entry.config.lpServiceStartName = strdupW(lpServiceStartName);
    }

    if (lpDisplayName != NULL)
    {
        HeapFree(GetProcessHeap(), 0, service->service_entry->config.lpDisplayName);
        new_entry.config.lpDisplayName = strdupW(lpDisplayName);
    }

    *service->service_entry = new_entry;
    save_service_config(service->service_entry);
    service_unlock(service->service_entry);

    return ERROR_SUCCESS;
}

DWORD svcctl_SetServiceStatus(
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

    if (service->service_entry->status_changed_event)
        SetEvent(service->service_entry->status_changed_event);

    return ERROR_SUCCESS;
}

DWORD svcctl_QueryServiceStatusEx(
    SC_RPC_HANDLE hService,
    SC_STATUS_TYPE InfoLevel,
    BYTE *lpBuffer,
    DWORD cbBufSize,
    LPDWORD pcbBytesNeeded)
{
    struct sc_service_handle *service;
    DWORD err;
    LPSERVICE_STATUS_PROCESS pSvcStatusData;

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

/* only one service started at a time, so there is no race on the registry
 * value here */
static LPWSTR service_get_pipe_name(void)
{
    static const WCHAR format[] = { '\\','\\','.','\\','p','i','p','e','\\',
        'n','e','t','\\','N','t','C','o','n','t','r','o','l','P','i','p','e','%','u',0};
    static const WCHAR service_current_key_str[] = { 'S','Y','S','T','E','M','\\',
        'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
        'C','o','n','t','r','o','l','\\',
        'S','e','r','v','i','c','e','C','u','r','r','e','n','t',0};
    LPWSTR name;
    DWORD len;
    HKEY service_current_key;
    DWORD service_current = -1;
    LONG ret;
    DWORD type;

    ret = RegCreateKeyExW(HKEY_LOCAL_MACHINE, service_current_key_str, 0,
        NULL, REG_OPTION_VOLATILE, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL,
        &service_current_key, NULL);
    if (ret != ERROR_SUCCESS)
        return NULL;
    len = sizeof(service_current);
    ret = RegQueryValueExW(service_current_key, NULL, NULL, &type,
        (BYTE *)&service_current, &len);
    if ((ret == ERROR_SUCCESS && type == REG_DWORD) || ret == ERROR_FILE_NOT_FOUND)
    {
        service_current++;
        RegSetValueExW(service_current_key, NULL, 0, REG_DWORD,
            (BYTE *)&service_current, sizeof(service_current));
    }
    RegCloseKey(service_current_key);
    if ((ret != ERROR_SUCCESS || type != REG_DWORD) && (ret != ERROR_FILE_NOT_FOUND))
        return NULL;
    len = sizeof(format)/sizeof(WCHAR) + 10 /* strlenW("4294967295") */;
    name = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!name)
        return NULL;
    snprintfW(name, len, format, service_current);
    return name;
}

static DWORD service_start_process(struct service_entry *service_entry, HANDLE *process)
{
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    LPWSTR path = NULL;
    DWORD size;
    BOOL r;

    service_lock_exclusive(service_entry);

    if (service_entry->config.dwServiceType == SERVICE_KERNEL_DRIVER)
    {
        static const WCHAR winedeviceW[] = {'\\','w','i','n','e','d','e','v','i','c','e','.','e','x','e',' ',0};
        DWORD len = GetSystemDirectoryW( NULL, 0 ) + sizeof(winedeviceW)/sizeof(WCHAR) + strlenW(service_entry->name);

        if (!(path = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
            return ERROR_NOT_ENOUGH_SERVER_MEMORY;
        GetSystemDirectoryW( path, len );
        lstrcatW( path, winedeviceW );
        lstrcatW( path, service_entry->name );
    }
    else
    {
        size = ExpandEnvironmentStringsW(service_entry->config.lpBinaryPathName,NULL,0);
        path = HeapAlloc(GetProcessHeap(),0,size*sizeof(WCHAR));
        if (!path)
            return ERROR_NOT_ENOUGH_SERVER_MEMORY;
        ExpandEnvironmentStringsW(service_entry->config.lpBinaryPathName,path,size);
    }

    ZeroMemory(&si, sizeof(STARTUPINFOW));
    si.cb = sizeof(STARTUPINFOW);
    if (!(service_entry->config.dwServiceType & SERVICE_INTERACTIVE_PROCESS))
    {
        static WCHAR desktopW[] = {'_','_','w','i','n','e','s','e','r','v','i','c','e','_','w','i','n','s','t','a','t','i','o','n','\\','D','e','f','a','u','l','t',0};
        si.lpDesktop = desktopW;
    }

    service_entry->status.dwCurrentState = SERVICE_START_PENDING;
    service_entry->status.dwProcessId = pi.dwProcessId;

    service_unlock(service_entry);

    r = CreateProcessW(NULL, path, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    HeapFree(GetProcessHeap(),0,path);
    if (!r)
    {
        service_lock_exclusive(service_entry);
        service_entry->status.dwCurrentState = SERVICE_STOPPED;
        service_entry->status.dwProcessId = 0;
        service_unlock(service_entry);
        return GetLastError();
    }

    *process = pi.hProcess;
    CloseHandle( pi.hThread );

    return ERROR_SUCCESS;
}

static DWORD service_wait_for_startup(struct service_entry *service_entry, HANDLE process_handle)
{
    WINE_TRACE("%p\n", service_entry);

    for (;;)
    {
        DWORD dwCurrentStatus;
        HANDLE handles[2] = { service_entry->status_changed_event, process_handle };
        DWORD ret;
        ret = WaitForMultipleObjects(sizeof(handles)/sizeof(handles[0]), handles, FALSE, 20000);
        if (ret != WAIT_OBJECT_0)
            return ERROR_SERVICE_REQUEST_TIMEOUT;
        service_lock_shared(service_entry);
        dwCurrentStatus = service_entry->status.dwCurrentState;
        service_unlock(service_entry);
        if (dwCurrentStatus == SERVICE_RUNNING)
        {
            WINE_TRACE("Service started successfully\n");
            return ERROR_SUCCESS;
        }
        if (dwCurrentStatus != SERVICE_START_PENDING)
            return ERROR_SERVICE_REQUEST_TIMEOUT;
    }
}

/******************************************************************************
 * service_send_start_message
 */
static BOOL service_send_start_message(HANDLE pipe, LPCWSTR *argv, DWORD argc)
{
    DWORD i, len, count, result;
    service_start_info *ssi;
    LPWSTR p;
    BOOL r;

    WINE_TRACE("%p %p %d\n", pipe, argv, argc);

    /* FIXME: this can block so should be done in another thread */
    r = ConnectNamedPipe(pipe, NULL);
    if (!r && GetLastError() != ERROR_PIPE_CONNECTED)
    {
        WINE_ERR("pipe connect failed\n");
        return FALSE;
    }

    /* calculate how much space do we need to send the startup info */
    len = 1;
    for (i=0; i<argc; i++)
        len += strlenW(argv[i])+1;

    ssi = HeapAlloc(GetProcessHeap(),0,FIELD_OFFSET(service_start_info, str[len]));
    ssi->cmd = WINESERV_STARTINFO;
    ssi->size = len;

    /* copy service args into a single buffer*/
    p = &ssi->str[0];
    for (i=0; i<argc; i++)
    {
        strcpyW(p, argv[i]);
        p += strlenW(p) + 1;
    }
    *p=0;

    r = WriteFile(pipe, ssi, FIELD_OFFSET(service_start_info, str[len]), &count, NULL);
    if (r)
    {
        r = ReadFile(pipe, &result, sizeof result, &count, NULL);
        if (r && result)
        {
            SetLastError(result);
            r = FALSE;
        }
    }

    HeapFree(GetProcessHeap(),0,ssi);

    return r;
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
 * service_send_control
 */
static BOOL service_send_control(HANDLE pipe, DWORD dwControl, DWORD *result)
{
    DWORD cmd[2], count = 0;
    BOOL r;

    cmd[0] = WINESERV_SENDCONTROL;
    cmd[1] = dwControl;
    r = WriteFile(pipe, cmd, sizeof cmd, &count, NULL);
    if (!r || count != sizeof cmd)
    {
        WINE_ERR("service protocol error - failed to write pipe!\n");
        return r;
    }
    r = ReadFile(pipe, result, sizeof *result, &count, NULL);
    if (!r || count != sizeof *result)
        WINE_ERR("service protocol error - failed to read pipe "
            "r = %d  count = %d!\n", r, count);
    return r;
}

DWORD svcctl_StartServiceW(
    SC_RPC_HANDLE hService,
    DWORD dwNumServiceArgs,
    LPCWSTR *lpServiceArgVectors)
{
    struct sc_service_handle *service;
    DWORD err;
    LPWSTR name;
    HANDLE process_handle = NULL;

    WINE_TRACE("(%p, %d, %p)\n", hService, dwNumServiceArgs, lpServiceArgVectors);

    if ((err = validate_service_handle(hService, SERVICE_START, &service)) != 0)
        return err;

    err = scmdatabase_lock_startup(service->service_entry->db);
    if (err != ERROR_SUCCESS)
        return err;

    if (service->service_entry->control_pipe != INVALID_HANDLE_VALUE)
    {
        scmdatabase_unlock_startup(service->service_entry->db);
        return ERROR_SERVICE_ALREADY_RUNNING;
    }

    service->service_entry->control_mutex = CreateMutexW(NULL, TRUE, NULL);

    if (!service->service_entry->status_changed_event)
        service->service_entry->status_changed_event = CreateEventW(NULL, TRUE, FALSE, NULL);

    name = service_get_pipe_name();
    service->service_entry->control_pipe = CreateNamedPipeW(name, PIPE_ACCESS_DUPLEX,
                  PIPE_TYPE_BYTE|PIPE_WAIT, 1, 256, 256, 10000, NULL );
    HeapFree(GetProcessHeap(), 0, name);
    if (service->service_entry->control_pipe==INVALID_HANDLE_VALUE)
    {
        WINE_ERR("failed to create pipe for %s, error = %d\n",
            wine_dbgstr_w(service->service_entry->name), GetLastError());
        scmdatabase_unlock_startup(service->service_entry->db);
        return GetLastError();
    }

    err = service_start_process(service->service_entry, &process_handle);

    if (err == ERROR_SUCCESS)
    {
        if (!service_send_start_message(service->service_entry->control_pipe,
                lpServiceArgVectors, dwNumServiceArgs))
            err = ERROR_SERVICE_REQUEST_TIMEOUT;
    }

    WINE_TRACE("returning %d\n", err);

    if (err == ERROR_SUCCESS)
        err = service_wait_for_startup(service->service_entry, process_handle);

    if (process_handle)
        CloseHandle(process_handle);

    ReleaseMutex(service->service_entry->control_mutex);
    scmdatabase_unlock_startup(service->service_entry->db);

    return err;
}

DWORD svcctl_ControlService(
    SC_RPC_HANDLE hService,
    DWORD dwControl,
    SERVICE_STATUS *lpServiceStatus)
{
    DWORD access_required;
    struct sc_service_handle *service;
    DWORD err;
    BOOL ret;
    HANDLE control_mutex;
    HANDLE control_pipe;

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

    if ((err = validate_service_handle(hService, access_required, &service)) != 0)
        return err;

    service_lock_exclusive(service->service_entry);

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

    if (!service_accepts_control(service->service_entry, dwControl))
    {
        service_unlock(service->service_entry);
        return ERROR_INVALID_SERVICE_CONTROL;
    }

    switch (service->service_entry->status.dwCurrentState)
    {
    case SERVICE_STOPPED:
        service_unlock(service->service_entry);
        return ERROR_SERVICE_NOT_ACTIVE;
    case SERVICE_START_PENDING:
        if (dwControl==SERVICE_CONTROL_STOP)
            break;
        /* fall thru */
    case SERVICE_STOP_PENDING:
        service_unlock(service->service_entry);
        return ERROR_SERVICE_CANNOT_ACCEPT_CTRL;
    }

    /* prevent races by caching these variables and clearing them on
     * stop here instead of outside the services lock */
    control_mutex = service->service_entry->control_mutex;
    control_pipe = service->service_entry->control_pipe;
    if (dwControl == SERVICE_CONTROL_STOP)
    {
        service->service_entry->control_mutex = NULL;
        service->service_entry->control_pipe = NULL;
    }

    service_unlock(service->service_entry);

    ret = WaitForSingleObject(control_mutex, 30000);
    if (ret)
    {
        DWORD result = ERROR_SUCCESS;

        ret = service_send_control(control_pipe, dwControl, &result);

        if (dwControl == SERVICE_CONTROL_STOP)
        {
            CloseHandle(control_mutex);
            CloseHandle(control_pipe);
        }
        else
            ReleaseMutex(control_mutex);

        return result;
    }
    else
    {
        if (dwControl == SERVICE_CONTROL_STOP)
        {
            CloseHandle(control_mutex);
            CloseHandle(control_pipe);
        }
        return ERROR_SERVICE_REQUEST_TIMEOUT;
    }
}

DWORD svcctl_CloseServiceHandle(
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

DWORD svcctl_LockServiceDatabase(
    SC_RPC_HANDLE hSCManager,
    SC_RPC_LOCK *phLock)
{
    struct sc_manager_handle *manager;
    DWORD err;

    WINE_TRACE("(%p, %p)\n", hSCManager, phLock);

    if ((err = validate_scm_handle(hSCManager, SC_MANAGER_LOCK, &manager)) != ERROR_SUCCESS)
        return err;

    err = scmdatabase_lock_startup(manager->db);
    if (err != ERROR_SUCCESS)
        return err;

    *phLock = HeapAlloc(GetProcessHeap(), 0, sizeof(struct sc_lock));
    if (!*phLock)
    {
        scmdatabase_unlock_startup(manager->db);
        return ERROR_NOT_ENOUGH_SERVER_MEMORY;
    }

    return ERROR_SUCCESS;
}

DWORD svcctl_UnlockServiceDatabase(
    SC_RPC_LOCK *phLock)
{
    WINE_TRACE("(&%p)\n", *phLock);

    SC_RPC_LOCK_destroy(*phLock);
    *phLock = NULL;

    return ERROR_SUCCESS;
}

DWORD RPC_MainLoop(void)
{
    WCHAR transport[] = SVCCTL_TRANSPORT;
    WCHAR endpoint[] = SVCCTL_ENDPOINT;
    HANDLE hSleepHandle;
    DWORD err;

    if ((err = RpcServerUseProtseqEpW(transport, 0, endpoint, NULL)) != ERROR_SUCCESS)
    {
        WINE_ERR("RpcServerUseProtseq failed with error %u\n", err);
        return err;
    }

    if ((err = RpcServerRegisterIf(svcctl_v2_0_s_ifspec, 0, 0)) != ERROR_SUCCESS)
    {
        WINE_ERR("RpcServerRegisterIf failed with error %u", err);
        return err;
    }

    if ((err = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE)) != ERROR_SUCCESS)
    {
        WINE_ERR("RpcServerListen failed with error %u\n", err);
        return err;
    }

    WINE_TRACE("Entered main loop\n");
    hSleepHandle = __wine_make_process_system();
    SetEvent(g_hStartedEvent);
    do
    {
        err = WaitForSingleObjectEx(hSleepHandle, INFINITE, TRUE);
        WINE_TRACE("Wait returned %d\n", err);
    } while (err != WAIT_OBJECT_0);

    WINE_TRACE("Object signaled - wine shutdown\n");
    return ERROR_SUCCESS;
}

void __RPC_USER SC_RPC_HANDLE_rundown(SC_RPC_HANDLE handle)
{
    SC_RPC_HANDLE_destroy(handle);
}

void  __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

void __RPC_USER MIDL_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}
