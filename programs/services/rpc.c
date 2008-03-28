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

static CRITICAL_SECTION g_handle_table_cs;
static CRITICAL_SECTION_DEBUG g_handle_table_cs_debug =
{
    0, 0, &g_handle_table_cs,
    { &g_handle_table_cs_debug.ProcessLocksList,
      &g_handle_table_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": g_handle_table_cs") }
};
static CRITICAL_SECTION g_handle_table_cs = { &g_handle_table_cs_debug, -1, 0, 0, 0, 0 };

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

struct sc_manager       /* service control manager handle */
{
    struct sc_handle hdr;
};

struct sc_service       /* service handle */
{
    struct sc_handle hdr;
    struct service_entry *service_entry;
};

struct sc_lock
{
    char dummy; /* no state currently used */
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

static DWORD validate_scm_handle(SC_RPC_HANDLE handle, DWORD needed_access, struct sc_manager **manager)
{
    struct sc_handle *hdr;
    DWORD err = validate_context_handle(handle, SC_HTYPE_MANAGER, needed_access, &hdr);
    if (err == ERROR_SUCCESS)
        *manager = (struct sc_manager *)hdr;
    return err;
}

static DWORD validate_service_handle(SC_RPC_HANDLE handle, DWORD needed_access, struct sc_service **service)
{
    struct sc_handle *hdr;
    DWORD err = validate_context_handle(handle, SC_HTYPE_SERVICE, needed_access, &hdr);
    if (err == ERROR_SUCCESS)
        *service = (struct sc_service *)hdr;
    return err;
}

DWORD svcctl_OpenSCManagerW(
    MACHINE_HANDLEW MachineName, /* Note: this parameter is ignored */
    LPCWSTR DatabaseName,
    DWORD dwAccessMask,
    SC_RPC_HANDLE *handle)
{
    struct sc_manager *manager;

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
            struct sc_manager *manager = (struct sc_manager *)hdr;
            HeapFree(GetProcessHeap(), 0, manager);
            break;
        }
        case SC_HTYPE_SERVICE:
        {
            struct sc_service *service = (struct sc_service *)hdr;
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
    struct sc_manager *manager;
    struct service_entry *entry;
    DWORD err;

    WINE_TRACE("(%s, %d)\n", wine_dbgstr_w(lpServiceName), cchBufSize);

    if ((err = validate_scm_handle(hSCManager, 0, &manager)) != ERROR_SUCCESS)
        return err;

    lock_services();

    entry = find_service(lpServiceName);
    if (entry != NULL)
    {
        LPCWSTR name = get_display_name(entry);
        *cchLength = strlenW(name);
        if (*cchLength < cchBufSize)
        {
            err = ERROR_SUCCESS;
            lstrcpyW(lpBuffer, name);
        }
        else
            err = ERROR_INSUFFICIENT_BUFFER;
    }
    else
    {
        *cchLength = 1;
        err = ERROR_SERVICE_DOES_NOT_EXIST;
    }

    if (err != ERROR_SUCCESS && cchBufSize > 0)
        lpBuffer[0] = 0;
    unlock_services();

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
    struct sc_manager *manager;
    DWORD err;

    WINE_TRACE("(%s, %d)\n", wine_dbgstr_w(lpServiceDisplayName), cchBufSize);

    if ((err = validate_scm_handle(hSCManager, 0, &manager)) != ERROR_SUCCESS)
        return err;

    lock_services();

    entry = find_service_by_displayname(lpServiceDisplayName);
    if (entry != NULL)
    {
        *cchLength = strlenW(entry->name);
        if (*cchLength < cchBufSize)
        {
            err = ERROR_SUCCESS;
            lstrcpyW(lpBuffer, entry->name);
        }
        else
            err = ERROR_INSUFFICIENT_BUFFER;
    }
    else
    {
        *cchLength = 1;
        err = ERROR_SERVICE_DOES_NOT_EXIST;
    }

    if (err != ERROR_SUCCESS && cchBufSize > 0)
        lpBuffer[0] = 0;
    unlock_services();

    return err;
}

static DWORD create_handle_for_service(struct service_entry *entry, DWORD dwDesiredAccess, SC_RPC_HANDLE *phService)
{
    struct sc_service *service;

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
    struct sc_manager *manager;
    struct service_entry *entry;
    DWORD err;

    WINE_TRACE("(%s, 0x%x)\n", wine_dbgstr_w(lpServiceName), dwDesiredAccess);

    if ((err = validate_scm_handle(hSCManager, 0, &manager)) != ERROR_SUCCESS)
        return err;
    if (!validate_service_name(lpServiceName))
        return ERROR_INVALID_NAME;

    lock_services();
    entry = find_service(lpServiceName);
    if (entry != NULL)
        entry->ref_count++;
    unlock_services();

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
    struct sc_manager *manager;
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

    entry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*entry));
    entry->name = strdupW(lpServiceName);
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

    lock_services();

    if (find_service(lpServiceName))
    {
        unlock_services();
        free_service_entry(entry);
        return ERROR_SERVICE_EXISTS;
    }

    if (find_service_by_displayname(get_display_name(entry)))
    {
        unlock_services();
        free_service_entry(entry);
        return ERROR_DUPLICATE_SERVICE_NAME;
    }

    err = add_service(entry);
    if (err != ERROR_SUCCESS)
    {
        unlock_services();
        free_service_entry(entry);
        return err;
    }
    unlock_services();

    return create_handle_for_service(entry, dwDesiredAccess, phService);
}

DWORD svcctl_DeleteService(
    SC_RPC_HANDLE hService)
{
    struct sc_service *service;
    DWORD err;

    if ((err = validate_service_handle(hService, DELETE, &service)) != ERROR_SUCCESS)
        return err;

    lock_services();

    if (!is_marked_for_delete(service->service_entry))
        err = remove_service(service->service_entry);
    else
        err = ERROR_SERVICE_MARKED_FOR_DELETE;

    unlock_services();

    return err;
}

DWORD svcctl_QueryServiceConfigW(
        SC_RPC_HANDLE hService,
        QUERY_SERVICE_CONFIGW *config)
{
    struct sc_service *service;
    DWORD err;

    WINE_TRACE("(%p)\n", config);

    if ((err = validate_service_handle(hService, SERVICE_QUERY_CONFIG, &service)) != 0)
        return err;

    lock_services();
    config->dwServiceType = service->service_entry->config.dwServiceType;
    config->dwStartType = service->service_entry->config.dwStartType;
    config->dwErrorControl = service->service_entry->config.dwErrorControl;
    config->lpBinaryPathName = strdupW(service->service_entry->config.lpBinaryPathName);
    config->lpLoadOrderGroup = strdupW(service->service_entry->config.lpLoadOrderGroup);
    config->dwTagId = service->service_entry->config.dwTagId;
    config->lpDependencies = NULL; /* TODO */
    config->lpServiceStartName = strdupW(service->service_entry->config.lpServiceStartName);
    config->lpDisplayName = strdupW(service->service_entry->config.lpDisplayName);
    unlock_services();

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
    struct sc_service *service;
    DWORD err;

    WINE_TRACE("\n");

    if ((err = validate_service_handle(hService, SERVICE_CHANGE_CONFIG, &service)) != 0)
        return err;

    if (!check_multisz((LPCWSTR)lpDependencies, dwDependenciesSize))
        return ERROR_INVALID_PARAMETER;

    /* first check if the new configuration is correct */
    lock_services();

    if (is_marked_for_delete(service->service_entry))
    {
        unlock_services();
        return ERROR_SERVICE_MARKED_FOR_DELETE;
    }

    if (lpDisplayName != NULL && find_service_by_displayname(lpDisplayName))
    {
        unlock_services();
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
        unlock_services();
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
    unlock_services();

    return ERROR_SUCCESS;
}

DWORD svcctl_SetServiceStatus(
    SC_RPC_HANDLE hServiceStatus,
    LPSERVICE_STATUS lpServiceStatus)
{
    struct sc_service *service;
    DWORD err;

    WINE_TRACE("(%p, %p)\n", hServiceStatus, lpServiceStatus);

    if ((err = validate_service_handle(hServiceStatus, SERVICE_SET_STATUS, &service)) != 0)
        return err;

    lock_services();
    /* FIXME: be a bit more discriminant about what parts of the status we set
     * and check that fields are valid */
    service->service_entry->status.dwServiceType = lpServiceStatus->dwServiceType;
    service->service_entry->status.dwCurrentState = lpServiceStatus->dwCurrentState;
    service->service_entry->status.dwControlsAccepted = lpServiceStatus->dwControlsAccepted;
    service->service_entry->status.dwWin32ExitCode = lpServiceStatus->dwWin32ExitCode;
    service->service_entry->status.dwServiceSpecificExitCode = lpServiceStatus->dwServiceSpecificExitCode;
    service->service_entry->status.dwCheckPoint = lpServiceStatus->dwCheckPoint;
    service->service_entry->status.dwWaitHint = lpServiceStatus->dwWaitHint;
    unlock_services();

    return ERROR_SUCCESS;
}

DWORD svcctl_QueryServiceStatusEx(
    SC_RPC_HANDLE hService,
    SC_STATUS_TYPE InfoLevel,
    BYTE *lpBuffer,
    DWORD cbBufSize,
    LPDWORD pcbBytesNeeded)
{
    struct sc_service *service;
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

    lock_services();

    pSvcStatusData->dwServiceType = service->service_entry->status.dwServiceType;
    pSvcStatusData->dwCurrentState = service->service_entry->status.dwCurrentState;
    pSvcStatusData->dwControlsAccepted = service->service_entry->status.dwControlsAccepted;
    pSvcStatusData->dwWin32ExitCode = service->service_entry->status.dwWin32ExitCode;
    pSvcStatusData->dwServiceSpecificExitCode = service->service_entry->status.dwServiceSpecificExitCode;
    pSvcStatusData->dwCheckPoint = service->service_entry->status.dwCheckPoint;
    pSvcStatusData->dwWaitHint = service->service_entry->status.dwWaitHint;
    pSvcStatusData->dwProcessId = service->service_entry->status.dwProcessId;
    pSvcStatusData->dwServiceFlags = service->service_entry->status.dwServiceFlags;

    unlock_services();

    return ERROR_SUCCESS;
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
    unlock_service_database();
    HeapFree(GetProcessHeap(), 0, hLock);
}

void __RPC_USER SC_RPC_LOCK_rundown(SC_RPC_LOCK hLock)
{
    SC_RPC_LOCK_destroy(hLock);
}

DWORD svcctl_LockServiceDatabase(
    SC_RPC_HANDLE hSCManager,
    SC_RPC_LOCK *phLock)
{
    struct sc_manager *manager;
    DWORD err;

    WINE_TRACE("(%p, %p)\n", hSCManager, phLock);

    if ((err = validate_scm_handle(hSCManager, SC_MANAGER_LOCK, &manager)) != ERROR_SUCCESS)
        return err;

    err = lock_service_database();
    if (err != ERROR_SUCCESS)
        return err;

    *phLock = HeapAlloc(GetProcessHeap(), 0, sizeof(struct sc_lock));
    if (!*phLock)
        return ERROR_NOT_ENOUGH_SERVER_MEMORY;

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
