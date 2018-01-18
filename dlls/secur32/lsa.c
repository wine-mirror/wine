/*
 * Copyright (C) 2004 Juan Lang
 * Copyright (C) 2007 Kai Blin
 * Copyright (C) 2017, 2018 Dmitry Timoshkov
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
#include "rpc.h"
#include "secur32_priv.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(secur32);

#define LSA_MAGIC ('L' << 24 | 'S' << 16 | 'A' << 8 | ' ')

struct lsa_package
{
    ULONG package_id;
    HMODULE mod;
    LSA_STRING *name;
    ULONG lsa_api_version, lsa_table_count, user_api_version, user_table_count;
    SECPKG_FUNCTION_TABLE *lsa_api;
    SECPKG_USER_FUNCTION_TABLE *user_api;
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
            if (loaded_packages[i].lsa_api->CallPackageUntrusted)
                return loaded_packages[i].lsa_api->CallPackageUntrusted(NULL /* FIXME*/,
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

static NTSTATUS NTAPI lsa_RegisterCallback(ULONG callback_id, PLSA_CALLBACK_FUNCTION callback)
{
    FIXME("%u,%p: stub\n", callback_id, callback);
    return STATUS_NOT_IMPLEMENTED;
}

static SECPKG_DLL_FUNCTIONS lsa_dll_dispatch =
{
    lsa_AllocateLsaHeap,
    lsa_FreeLsaHeap,
    lsa_RegisterCallback
};

static const SecurityFunctionTableW lsa_sspi_tableW =
{
    1,
    NULL, /* EnumerateSecurityPackagesW */
    NULL, /* QueryCredentialsAttributesW */
    NULL, /* AcquireCredentialsHandleW */
    NULL, /* FreeCredentialsHandle */
    NULL, /* Reserved2 */
    NULL, /* InitializeSecurityContextW */
    NULL, /* AcceptSecurityContext */
    NULL, /* CompleteAuthToken */
    NULL, /* DeleteSecurityContext */
    NULL, /* ApplyControlToken */
    NULL, /* QueryContextAttributesW */
    NULL, /* ImpersonateSecurityContext */
    NULL, /* RevertSecurityContext */
    NULL, /* MakeSignature */
    NULL, /* VerifySignature */
    NULL, /* FreeContextBuffer */
    NULL, /* QuerySecurityPackageInfoW */
    NULL, /* Reserved3 */
    NULL, /* Reserved4 */
    NULL, /* ExportSecurityContext */
    NULL, /* ImportSecurityContextW */
    NULL, /* AddCredentialsW */
    NULL, /* Reserved8 */
    NULL, /* QuerySecurityContextToken */
    NULL, /* EncryptMessage */
    NULL, /* DecryptMessage */
    NULL, /* SetContextAttributesW */
};

static const SecurityFunctionTableA lsa_sspi_tableA =
{
    1,
    NULL, /* EnumerateSecurityPackagesA */
    NULL, /* QueryCredentialsAttributesA */
    NULL, /* AcquireCredentialsHandleA */
    NULL, /* FreeCredentialsHandle */
    NULL, /* Reserved2 */
    NULL, /* InitializeSecurityContextA */
    NULL, /* AcceptSecurityContext */
    NULL, /* CompleteAuthToken */
    NULL, /* DeleteSecurityContext */
    NULL, /* ApplyControlToken */
    NULL, /* QueryContextAttributesA */
    NULL, /* ImpersonateSecurityContext */
    NULL, /* RevertSecurityContext */
    NULL, /* MakeSignature */
    NULL, /* VerifySignature */
    NULL, /* FreeContextBuffer */
    NULL, /* QuerySecurityPackageInfoA */
    NULL, /* Reserved3 */
    NULL, /* Reserved4 */
    NULL, /* ExportSecurityContext */
    NULL, /* ImportSecurityContextA */
    NULL, /* AddCredentialsA */
    NULL, /* Reserved8 */
    NULL, /* QuerySecurityContextToken */
    NULL, /* EncryptMessage */
    NULL, /* DecryptMessage */
    NULL, /* SetContextAttributesA */
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
    NTSTATUS (NTAPI *pSpLsaModeInitialize)(ULONG, PULONG, PSECPKG_FUNCTION_TABLE *, PULONG);
    NTSTATUS (NTAPI *pSpUserModeInitialize)(ULONG, PULONG, PSECPKG_USER_FUNCTION_TABLE *, PULONG);

    memset(package, 0, sizeof(*package));

    package->mod = LoadLibraryW(name);
    if (!package->mod) return FALSE;

    pSpLsaModeInitialize = (void *)GetProcAddress(package->mod, "SpLsaModeInitialize");
    if (pSpLsaModeInitialize)
    {
        NTSTATUS status;

        status = pSpLsaModeInitialize(SECPKG_INTERFACE_VERSION, &package->lsa_api_version, &package->lsa_api, &package->lsa_table_count);
        if (status == STATUS_SUCCESS)
        {
            status = package->lsa_api->InitializePackage(package_id, &lsa_dispatch, NULL, NULL, &package->name);
            if (status == STATUS_SUCCESS)
            {
                TRACE("%s => %p, name %s, version %#x, api table %p, table count %u\n",
                    debugstr_w(name), package->mod, debugstr_an(package->name->Buffer, package->name->Length),
                    package->lsa_api_version, package->lsa_api, package->lsa_table_count);
                package->package_id = package_id;

                status = package->lsa_api->Initialize(package_id, NULL /* FIXME: params */, NULL);
                if (status == STATUS_SUCCESS)
                {
                    pSpUserModeInitialize = (void *)GetProcAddress(package->mod, "SpUserModeInitialize");
                    if (pSpUserModeInitialize)
                    {
                        status = pSpUserModeInitialize(SECPKG_INTERFACE_VERSION, &package->user_api_version, &package->user_api, &package->user_table_count);
                        if (status == STATUS_SUCCESS)
                            package->user_api->InstanceInit(SECPKG_INTERFACE_VERSION, &lsa_dll_dispatch, NULL);
                    }
                }
                return TRUE;
            }
        }
    }

    FreeLibrary(package->mod);
    return FALSE;
}

#define MAX_SERVICE_NAME 260

void load_auth_packages(void)
{
    static const WCHAR LSA_KEY[] = { 'S','y','s','t','e','m','\\',
        'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
        'C','o','n','t','r','o','l','\\','L','s','a',0 };
    DWORD err, i;
    HKEY root;
    SecureProvider *provider;

    err = RegOpenKeyExW(HKEY_LOCAL_MACHINE, LSA_KEY, 0, KEY_READ, &root);
    if (err != ERROR_SUCCESS) return;

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

    if (!loaded_packages_count) return;

    provider = SECUR32_addProvider(&lsa_sspi_tableA, &lsa_sspi_tableW, NULL);
    if (!provider)
    {
        ERR("Failed to add SSP/AP provider\n");
        return;
    }

    for (i = 0; i < loaded_packages_count; i++)
    {
        SecPkgInfoW *info;

        info = HeapAlloc(GetProcessHeap(), 0, loaded_packages[i].lsa_table_count * sizeof(*info));
        if (info)
        {
            NTSTATUS status;

            status = loaded_packages[i].lsa_api->GetInfo(info);
            if (status == STATUS_SUCCESS)
                SECUR32_addPackages(provider, loaded_packages[i].lsa_table_count, NULL, info);

            HeapFree(GetProcessHeap(), 0, info);
        }
    }
}

NTSTATUS WINAPI LsaLookupAuthenticationPackage(HANDLE lsa_handle,
        PLSA_STRING package_name, PULONG package_id)
{
    ULONG i;

    TRACE("%p %p %p\n", lsa_handle, package_name, package_id);

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
