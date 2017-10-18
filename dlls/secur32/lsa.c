/*
 * Copyright (C) 2004 Juan Lang
 * Copyright (C) 2007 Kai Blin
 * Copyright (C) 2017 Dmitry Timoshkov
 *
 * Local Security Authority functions, as far as secur32 has them.
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

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "sspi.h"
#include "ntsecapi.h"
#include "ntsecpkg.h"
#include "winternl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(lsa);

#define LSA_MAGIC ('L' << 24 | 'S' << 16 | 'A' << 8 | ' ')

struct lsa_package
{
    ULONG package_id;
    HMODULE mod;
    ULONG version;
    LSA_STRING *name;
    SECPKG_FUNCTION_TABLE *api;
    ULONG table_count;
};

static struct lsa_package *loaded_packages;
static ULONG loaded_packages_count;

struct lsa_connection
{
    DWORD magic;
};

NTSTATUS WINAPI LsaCallAuthenticationPackage(HANDLE lsa_handle, ULONG package_id,
        PVOID in_buffer, ULONG in_buffer_length,
        PVOID *out_buffer, PULONG out_buffer_length, PNTSTATUS status)
{
    ULONG i;

    TRACE("%p,%u,%p,%u,%p,%p,%p\n", lsa_handle, package_id, in_buffer,
        in_buffer_length, out_buffer, out_buffer_length, status);

    for (i = 0; i < loaded_packages_count; i++)
    {
        if (loaded_packages[i].package_id == package_id)
        {
            if (loaded_packages[i].api->CallPackageUntrusted)
                return loaded_packages[i].api->CallPackageUntrusted(NULL /* FIXME*/,
                    in_buffer, NULL, in_buffer_length, out_buffer, out_buffer_length, status);

            return SEC_E_UNSUPPORTED_FUNCTION;
        }
    }

    return STATUS_INVALID_PARAMETER;
}

NTSTATUS WINAPI LsaConnectUntrusted(PHANDLE LsaHandle)
{
    struct lsa_connection *lsa_conn;

    TRACE("%p\n", LsaHandle);

    lsa_conn = HeapAlloc(GetProcessHeap(), 0, sizeof(*lsa_conn));
    if (!lsa_conn) return STATUS_NO_MEMORY;

    lsa_conn->magic = LSA_MAGIC;
    *LsaHandle = lsa_conn;

    return STATUS_SUCCESS;
}

NTSTATUS WINAPI LsaDeregisterLogonProcess(HANDLE LsaHandle)
{
    FIXME("%p stub\n", LsaHandle);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI LsaEnumerateLogonSessions(PULONG LogonSessionCount,
        PLUID* LogonSessionList)
{
    FIXME("%p %p stub\n", LogonSessionCount, LogonSessionList);
    *LogonSessionCount = 0;
    *LogonSessionList = NULL;

    return STATUS_SUCCESS;
}

NTSTATUS WINAPI LsaFreeReturnBuffer(PVOID Buffer)
{
    FIXME("%p stub\n", Buffer);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI LsaGetLogonSessionData(PLUID LogonId,
        PSECURITY_LOGON_SESSION_DATA* ppLogonSessionData)
{
    FIXME("%p %p stub\n", LogonId, ppLogonSessionData);
    *ppLogonSessionData = NULL;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI LsaLogonUser(HANDLE LsaHandle, PLSA_STRING OriginName,
        SECURITY_LOGON_TYPE LogonType, ULONG AuthenticationPackage,
        PVOID AuthenticationInformation, ULONG AuthenticationInformationLength,
        PTOKEN_GROUPS LocalGroups, PTOKEN_SOURCE SourceContext,
        PVOID* ProfileBuffer, PULONG ProfileBufferLength, PLUID LogonId,
        PHANDLE Token, PQUOTA_LIMITS Quotas, PNTSTATUS SubStatus)
{
    FIXME("%p %p %d %d %p %d %p %p %p %p %p %p %p %p stub\n", LsaHandle,
            OriginName, LogonType, AuthenticationPackage,
            AuthenticationInformation, AuthenticationInformationLength,
            LocalGroups, SourceContext, ProfileBuffer, ProfileBufferLength,
            LogonId, Token, Quotas, SubStatus);
    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI lsa_CreateLogonSession(LUID *logon_id)
{
    FIXME("%p: stub\n", logon_id);
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS NTAPI lsa_DeleteLogonSession(LUID *logon_id)
{
    FIXME("%p: stub\n", logon_id);
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS NTAPI lsa_AddCredential(LUID *logon_id, ULONG package_id,
    LSA_STRING *primary_key, LSA_STRING *credentials)
{
    FIXME("%p,%u,%p,%p: stub\n", logon_id, package_id, primary_key, credentials);
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS NTAPI lsa_GetCredentials(LUID *logon_id, ULONG package_id, ULONG *context,
    BOOLEAN retrieve_all, LSA_STRING *primary_key, ULONG *primary_key_len, LSA_STRING *credentials)
{
    FIXME("%p,%#x,%p,%d,%p,%p,%p: stub\n", logon_id, package_id, context,
        retrieve_all, primary_key, primary_key_len, credentials);
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS NTAPI lsa_DeleteCredential(LUID *logon_id, ULONG package_id, LSA_STRING *primary_key)
{
    FIXME("%p,%#x,%p: stub\n", logon_id, package_id, primary_key);
    return STATUS_NOT_IMPLEMENTED;
}

static void * NTAPI lsa_AllocateLsaHeap(ULONG size)
{
    TRACE("%u\n", size);
    return HeapAlloc(GetProcessHeap(), 0, size);
}

static void NTAPI lsa_FreeLsaHeap(void *p)
{
    TRACE("%p\n", p);
    HeapFree(GetProcessHeap(), 0, p);
}

static NTSTATUS NTAPI lsa_AllocateClientBuffer(PLSA_CLIENT_REQUEST req, ULONG size, void **p)
{
    TRACE("%p,%u,%p\n", req, size, p);
    *p = HeapAlloc(GetProcessHeap(), 0, size);
    return *p ? STATUS_SUCCESS : STATUS_NO_MEMORY;
}

static NTSTATUS NTAPI lsa_FreeClientBuffer(PLSA_CLIENT_REQUEST req, void *p)
{
    TRACE("%p,%p\n", req, p);
    HeapFree(GetProcessHeap(), 0, p);
    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI lsa_CopyToClientBuffer(PLSA_CLIENT_REQUEST req, ULONG size, void *client, void *buf)
{
    TRACE("%p,%u,%p,%p\n", req, size, client, buf);
    memcpy(client, buf, size);
    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI lsa_CopyFromClientBuffer(PLSA_CLIENT_REQUEST req, ULONG size, void *buf, void *client)
{
    TRACE("%p,%u,%p,%p\n", req, size, buf, client);
    memcpy(buf, client, size);
    return STATUS_SUCCESS;
}

static LSA_DISPATCH_TABLE lsa_dispatch =
{
    lsa_CreateLogonSession,
    lsa_DeleteLogonSession,
    lsa_AddCredential,
    lsa_GetCredentials,
    lsa_DeleteCredential,
    lsa_AllocateLsaHeap,
    lsa_FreeLsaHeap,
    lsa_AllocateClientBuffer,
    lsa_FreeClientBuffer,
    lsa_CopyToClientBuffer,
    lsa_CopyFromClientBuffer
};

static void add_package(struct lsa_package *package)
{
    struct lsa_package *new_loaded_packages;

    if (!loaded_packages)
        new_loaded_packages = HeapAlloc(GetProcessHeap(), 0, sizeof(*new_loaded_packages));
    else
        new_loaded_packages = HeapReAlloc(GetProcessHeap(), 0, loaded_packages, sizeof(*new_loaded_packages) * (loaded_packages_count + 1));

    if (new_loaded_packages)
    {
        loaded_packages = new_loaded_packages;
        loaded_packages[loaded_packages_count] = *package;
        loaded_packages_count++;
    }
}

static BOOL load_package(const WCHAR *name, struct lsa_package *package, ULONG package_id)
{
    NTSTATUS (NTAPI *lsa_mode_init)(ULONG,PULONG,PSECPKG_FUNCTION_TABLE *, PULONG);

    package->mod = LoadLibraryW(name);
    if (!package->mod) return FALSE;

    lsa_mode_init = (void *)GetProcAddress(package->mod, "SpLsaModeInitialize");
    if (lsa_mode_init)
    {
        NTSTATUS status;

        status = lsa_mode_init(SECPKG_INTERFACE_VERSION, &package->version, &package->api, &package->table_count);
        if (status == STATUS_SUCCESS)
        {
            status = package->api->InitializePackage(package_id, &lsa_dispatch, NULL, NULL, &package->name);
            if (status == STATUS_SUCCESS)
            {
                TRACE("%s => %p, name %s, version %#x, api table %p, table count %u\n",
                    debugstr_w(name), package->mod, debugstr_an(package->name->Buffer, package->name->Length),
                    package->version, package->api, package->table_count);
                package->package_id = package_id;
                return TRUE;
            }
        }
    }

    FreeLibrary(package->mod);
    return FALSE;
}

#define MAX_SERVICE_NAME 260

static BOOL WINAPI load_auth_packages(INIT_ONCE *init_once, void *param, void **context)
{
    static const WCHAR LSA_KEY[] = { 'S','y','s','t','e','m','\\',
        'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
        'C','o','n','t','r','o','l','\\','L','s','a',0 };
    DWORD err, i;
    HKEY root;

    err = RegOpenKeyExW(HKEY_LOCAL_MACHINE, LSA_KEY, 0, KEY_READ, &root);
    if (err != ERROR_SUCCESS) return FALSE;

    i = 0;
    for (;;)
    {
        WCHAR name[MAX_SERVICE_NAME];
        struct lsa_package package;

        err = RegEnumKeyW(root, i++, name, MAX_SERVICE_NAME);
        if (err == ERROR_NO_MORE_ITEMS)
            break;

        if (err != ERROR_SUCCESS)
            continue;

        if (!load_package(name, &package, i))
            continue;

        add_package(&package);
    }

    RegCloseKey(root);

    return TRUE;
}

NTSTATUS WINAPI LsaLookupAuthenticationPackage(HANDLE lsa_handle,
        PLSA_STRING package_name, PULONG package_id)
{
    static INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;
    ULONG i;

    TRACE("%p %p %p\n", lsa_handle, package_name, package_id);

    InitOnceExecuteOnce(&init_once, load_auth_packages, NULL, NULL);

    for (i = 0; i < loaded_packages_count; i++)
    {
        if (!RtlCompareString(loaded_packages[i].name, package_name, FALSE))
        {
            *package_id = loaded_packages[i].package_id;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_UNSUCCESSFUL; /* FIXME */
}

NTSTATUS WINAPI LsaRegisterLogonProcess(PLSA_STRING LogonProcessName,
        PHANDLE LsaHandle, PLSA_OPERATIONAL_MODE SecurityMode)
{
    FIXME("%p %p %p stub\n", LogonProcessName, LsaHandle, SecurityMode);
    return STATUS_SUCCESS;
}
