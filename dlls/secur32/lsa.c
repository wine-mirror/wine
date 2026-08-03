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
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "sspi.h"
#include "ntsecapi.h"
#include "ntsecpkg.h"
#include "winternl.h"
#include "ddk/ntddk.h"
#include "rpc.h"

#include "wine/debug.h"
#include "secur32_priv.h"

WINE_DEFAULT_DEBUG_CHANNEL(secur32);

#define LSA_MAGIC_CONNECTION  ('L' << 24 | 'S' << 16 | 'A' << 8 | '0')
#define LSA_MAGIC_CREDENTIALS ('L' << 24 | 'S' << 16 | 'A' << 8 | '1')
#define LSA_MAGIC_CONTEXT     ('L' << 24 | 'S' << 16 | 'A' << 8 | '2')

static const WCHAR *default_authentication_package = L"Negotiate";

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

struct lsa_handle
{
    DWORD magic;
    struct lsa_package *package;
    LSA_SEC_HANDLE handle;
};

static const char *debugstr_as(const LSA_STRING *str)
{
    if (!str) return "<null>";
    return debugstr_an(str->Buffer, str->Length);
}

SECPKG_FUNCTION_TABLE *lsa_find_package(const char *name, SECPKG_USER_FUNCTION_TABLE **user_api)
{
    LSA_STRING package_name;
    ULONG i;

    RtlInitString(&package_name, name);

    for (i = 0; i < loaded_packages_count; i++)
    {
        if (!RtlCompareString(loaded_packages[i].name, &package_name, FALSE))
        {
            *user_api = loaded_packages[i].user_api;
            return loaded_packages[i].lsa_api;
        }
    }
    return NULL;
}

NTSTATUS WINAPI LsaCallAuthenticationPackage(HANDLE lsa_handle, ULONG package_id,
        PVOID in_buffer, ULONG in_buffer_length,
        PVOID *out_buffer, PULONG out_buffer_length, PNTSTATUS status)
{
    ULONG i;

    TRACE("%p,%lu,%p,%lu,%p,%p,%p\n", lsa_handle, package_id, in_buffer,
        in_buffer_length, out_buffer, out_buffer_length, status);

    if (out_buffer) *out_buffer = NULL;
    if (out_buffer_length) *out_buffer_length = 0;
    if (status) *status = STATUS_SUCCESS;

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

    return STATUS_NO_SUCH_PACKAGE;
}

static struct lsa_handle *alloc_lsa_handle(ULONG magic)
{
    struct lsa_handle *ret;
    if (!(ret = calloc(1, sizeof(*ret)))) return NULL;
    ret->magic = magic;
    return ret;
}

NTSTATUS WINAPI LsaConnectUntrusted(PHANDLE LsaHandle)
{
    struct lsa_handle *lsa_conn;

    TRACE("%p\n", LsaHandle);

    if (!(lsa_conn = alloc_lsa_handle(LSA_MAGIC_CONNECTION))) return STATUS_NO_MEMORY;
    *LsaHandle = lsa_conn;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI LsaRegisterLogonProcess(PLSA_STRING LogonProcessName,
        PHANDLE LsaHandle, PLSA_OPERATIONAL_MODE SecurityMode)
{
    struct lsa_handle *lsa_conn;

    FIXME("%s %p %p stub\n", debugstr_as(LogonProcessName), LsaHandle, SecurityMode);

    if (!(lsa_conn = alloc_lsa_handle(LSA_MAGIC_CONNECTION))) return STATUS_NO_MEMORY;
    *LsaHandle = lsa_conn;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI LsaDeregisterLogonProcess(HANDLE LsaHandle)
{
    struct lsa_handle *lsa_conn = (struct lsa_handle *)LsaHandle;

    TRACE("%p\n", LsaHandle);

    if (!lsa_conn || lsa_conn->magic != LSA_MAGIC_CONNECTION) return STATUS_INVALID_HANDLE;
    /* Ensure compiler doesn't optimize out the assignment with 0. */
    SecureZeroMemory(&lsa_conn->magic, sizeof(lsa_conn->magic));
    free(lsa_conn);

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

NTSTATUS WINAPI LsaFreeReturnBuffer(PVOID buffer)
{
    TRACE("%p\n", buffer);
    free(buffer);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI LsaGetLogonSessionData(PLUID LogonId,
        PSECURITY_LOGON_SESSION_DATA* ppLogonSessionData)
{
    SECURITY_LOGON_SESSION_DATA *data;
    int authpkg_len;
    WCHAR *end;

    FIXME("%p %p semi-stub\n", LogonId, ppLogonSessionData);

    authpkg_len = wcslen(default_authentication_package) * sizeof(WCHAR);

    data = calloc(1, sizeof(*data) + authpkg_len + sizeof(WCHAR));
    if (!data) return STATUS_NO_MEMORY;

    data->Size = sizeof(*data);
    data->LogonId = *LogonId;

    end = (WCHAR *)(data + 1);
    wcscpy(end, default_authentication_package);

    data->AuthenticationPackage.Length = authpkg_len;
    data->AuthenticationPackage.MaximumLength = authpkg_len + sizeof(WCHAR);
    data->AuthenticationPackage.Buffer = end;

    *ppLogonSessionData = data;

    return STATUS_SUCCESS;
}

NTSTATUS WINAPI LsaLogonUser(HANDLE LsaHandle, PLSA_STRING OriginName,
        SECURITY_LOGON_TYPE LogonType, ULONG AuthenticationPackage,
        PVOID AuthenticationInformation, ULONG AuthenticationInformationLength,
        PTOKEN_GROUPS LocalGroups, PTOKEN_SOURCE SourceContext,
        PVOID* ProfileBuffer, PULONG ProfileBufferLength, PLUID LogonId,
        PHANDLE Token, PQUOTA_LIMITS Quotas, PNTSTATUS SubStatus)
{
    FIXME("%p %s %d %ld %p %ld %p %p %p %p %p %p %p %p stub\n", LsaHandle,
            debugstr_as(OriginName), LogonType, AuthenticationPackage,
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
    FIXME("%p,%lu,%s,%s: stub\n", logon_id, package_id,
        debugstr_as(primary_key), debugstr_as(credentials));
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS NTAPI lsa_GetCredentials(LUID *logon_id, ULONG package_id, ULONG *context,
    BOOLEAN retrieve_all, LSA_STRING *primary_key, ULONG *primary_key_len, LSA_STRING *credentials)
{
    FIXME("%p,%#lx,%p,%d,%p,%p,%p: stub\n", logon_id, package_id, context,
        retrieve_all, primary_key, primary_key_len, credentials);
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS NTAPI lsa_DeleteCredential(LUID *logon_id, ULONG package_id, LSA_STRING *primary_key)
{
    FIXME("%p,%#lx,%s: stub\n", logon_id, package_id, debugstr_as(primary_key));
    return STATUS_NOT_IMPLEMENTED;
}

static void * NTAPI lsa_AllocateLsaHeap(ULONG size)
{
    TRACE("%lu\n", size);
    return malloc(size);
}

static void NTAPI lsa_FreeLsaHeap(void *p)
{
    TRACE("%p\n", p);
    free(p);
}

static NTSTATUS NTAPI lsa_AllocateClientBuffer(PLSA_CLIENT_REQUEST req, ULONG size, void **p)
{
    TRACE("%p,%lu,%p\n", req, size, p);
    *p = malloc(size);
    return *p ? STATUS_SUCCESS : STATUS_NO_MEMORY;
}

static NTSTATUS NTAPI lsa_FreeClientBuffer(PLSA_CLIENT_REQUEST req, void *p)
{
    TRACE("%p,%p\n", req, p);
    free(p);
    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI lsa_CopyToClientBuffer(PLSA_CLIENT_REQUEST req, ULONG size, void *client, void *buf)
{
    TRACE("%p,%lu,%p,%p\n", req, size, client, buf);
    memcpy(client, buf, size);
    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI lsa_CopyFromClientBuffer(PLSA_CLIENT_REQUEST req, ULONG size, void *buf, void *client)
{
    TRACE("%p,%lu,%p,%p\n", req, size, buf, client);
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
    FIXME("%lu,%p: stub\n", callback_id, callback);
    return STATUS_NOT_IMPLEMENTED;
}

static SECPKG_DLL_FUNCTIONS lsa_dll_dispatch =
{
    lsa_AllocateLsaHeap,
    lsa_FreeLsaHeap,
    lsa_RegisterCallback
};

static SECURITY_STATUS lsa_lookup_package(SEC_WCHAR *nameW, struct lsa_package **lsa_package)
{
    ULONG i;
    UNICODE_STRING package_name, name;

    for (i = 0; i < loaded_packages_count; i++)
    {
        if (RtlAnsiStringToUnicodeString(&package_name, loaded_packages[i].name, TRUE))
            return SEC_E_INSUFFICIENT_MEMORY;

        RtlInitUnicodeString(&name, nameW);

        if (RtlEqualUnicodeString(&package_name, &name, TRUE))
        {
            RtlFreeUnicodeString(&package_name);
            *lsa_package = &loaded_packages[i];
            return SEC_E_OK;
        }

        RtlFreeUnicodeString(&package_name);
    }

    return SEC_E_SECPKG_NOT_FOUND;
}

static SECURITY_STATUS WINAPI lsa_AcquireCredentialsHandleW(
    SEC_WCHAR *principal, SEC_WCHAR *package, ULONG credentials_use,
    LUID *logon_id, void *auth_data, SEC_GET_KEY_FN get_key_fn,
    void *get_key_arg, CredHandle *credential, TimeStamp *ts_expiry)
{
    SECURITY_STATUS status;
    struct lsa_package *lsa_package;
    struct lsa_handle *lsa_handle;
    UNICODE_STRING principal_us;
    LSA_SEC_HANDLE lsa_credential;

    TRACE("%s %s %#lx %p %p %p %p %p\n", debugstr_w(principal), debugstr_w(package),
        credentials_use, auth_data, get_key_fn, get_key_arg, credential, ts_expiry);

    if (!credential) return SEC_E_INVALID_HANDLE;
    if (!package) return SEC_E_SECPKG_NOT_FOUND;

    status = lsa_lookup_package(package, &lsa_package);
    if (status != SEC_E_OK) return status;

    if (!lsa_package->lsa_api || !lsa_package->lsa_api->SpAcquireCredentialsHandle)
        return SEC_E_UNSUPPORTED_FUNCTION;

    if (principal)
        RtlInitUnicodeString(&principal_us, principal);

    status = lsa_package->lsa_api->SpAcquireCredentialsHandle(principal ? &principal_us : NULL,
        credentials_use, logon_id, auth_data, get_key_fn, get_key_arg, &lsa_credential, ts_expiry);
    if (status == SEC_E_OK)
    {
        if (!(lsa_handle = alloc_lsa_handle(LSA_MAGIC_CREDENTIALS))) return STATUS_NO_MEMORY;
        lsa_handle->package = lsa_package;
        lsa_handle->handle = lsa_credential;
        credential->dwLower = (ULONG_PTR)lsa_handle;
        credential->dwUpper = 0;
    }
    return status;
}

static SECURITY_STATUS WINAPI lsa_AcquireCredentialsHandleA(
    SEC_CHAR *principal, SEC_CHAR *package, ULONG credentials_use,
    LUID *logon_id, void *auth_data, SEC_GET_KEY_FN get_key_fn,
    void *get_key_arg, CredHandle *credential, TimeStamp *ts_expiry)
{
    SECURITY_STATUS status = SEC_E_INSUFFICIENT_MEMORY;
    SEC_WCHAR *principalW = NULL, *packageW = NULL;
    SEC_WINNT_AUTH_IDENTITY_A *id = auth_data;
    SEC_WINNT_AUTH_IDENTITY_W idW = {};

    TRACE("%s %s %#lx %p %p %p %p %p\n", debugstr_a(principal), debugstr_a(package),
          credentials_use, auth_data, get_key_fn, get_key_arg, credential, ts_expiry);

    if (principal)
    {
        int len = MultiByteToWideChar( CP_ACP, 0, principal, -1, NULL, 0 );
        if (!(principalW = malloc( len * sizeof(SEC_WCHAR) ))) goto done;
        MultiByteToWideChar( CP_ACP, 0, principal, -1, principalW, len );
    }
    if (package)
    {
        int len = MultiByteToWideChar( CP_ACP, 0, package, -1, NULL, 0 );
        if (!(packageW = malloc( len * sizeof(SEC_WCHAR) ))) goto done;
        MultiByteToWideChar( CP_ACP, 0, package, -1, packageW, len );
    }
    if (id && (id->Flags == SEC_WINNT_AUTH_IDENTITY_ANSI))
    {
        if (id->UserLength)
        {
            idW.UserLength = MultiByteToWideChar( CP_ACP, 0, (char *)id->User, id->UserLength, NULL, 0 );
            if (!(idW.User = malloc( idW.UserLength * sizeof(SEC_WCHAR) ))) goto done;
            MultiByteToWideChar( CP_ACP, 0, (char *)id->User, id->UserLength, idW.User, idW.UserLength );
        }
        if (id->DomainLength)
        {
            idW.DomainLength = MultiByteToWideChar( CP_ACP, 0, (char *)id->Domain, id->DomainLength, NULL, 0 );
            if (!(idW.Domain = malloc( idW.DomainLength * sizeof(SEC_WCHAR) ))) goto done;
            MultiByteToWideChar( CP_ACP, 0, (char *)id->Domain, id->DomainLength, idW.Domain, idW.DomainLength );
        }
        if (id->PasswordLength)
        {
            idW.PasswordLength = MultiByteToWideChar( CP_ACP, 0, (char *)id->Password, id->PasswordLength, NULL, 0 );
            if (!(idW.Password = malloc( idW.PasswordLength * sizeof(SEC_WCHAR) ))) goto done;
            MultiByteToWideChar( CP_ACP, 0, (char *)id->Password, id->PasswordLength, idW.Password, idW.PasswordLength );
        }
        idW.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
        auth_data = &idW;
    }

    status = lsa_AcquireCredentialsHandleW( principalW, packageW, credentials_use, logon_id, auth_data, get_key_fn,
                                            get_key_arg, credential, ts_expiry );
done:
    free( packageW );
    free( principalW );
    free( idW.User );
    free( idW.Domain );
    free( idW.Password );
    return status;
}

static SECURITY_STATUS WINAPI lsa_FreeCredentialsHandle(CredHandle *credential)
{
    struct lsa_handle *lsa_cred;
    SECURITY_STATUS status;

    TRACE("%p\n", credential);
    if (!credential) return SEC_E_INVALID_HANDLE;

    lsa_cred = (struct lsa_handle *)credential->dwLower;
    if (!lsa_cred || lsa_cred->magic != LSA_MAGIC_CREDENTIALS) return SEC_E_INVALID_HANDLE;

    if (!lsa_cred->package->lsa_api || !lsa_cred->package->lsa_api->FreeCredentialsHandle)
        return SEC_E_UNSUPPORTED_FUNCTION;

    status = lsa_cred->package->lsa_api->FreeCredentialsHandle(lsa_cred->handle);

    /* Ensure compiler doesn't optimize out the assignment with 0. */
    SecureZeroMemory(&lsa_cred->magic, sizeof(lsa_cred->magic));
    free(lsa_cred);
    return status;
}

static SECURITY_STATUS WINAPI lsa_InitializeSecurityContextW(
    CredHandle *credential, CtxtHandle *context, SEC_WCHAR *target_name, ULONG context_req,
    ULONG reserved1, ULONG target_data_rep, SecBufferDesc *input, ULONG reserved2,
    CtxtHandle *new_context, SecBufferDesc *output, ULONG *context_attr, TimeStamp *ts_expiry)
{
    SECURITY_STATUS status;
    struct lsa_handle *lsa_cred = NULL, *lsa_ctx = NULL, *new_lsa_ctx;
    struct lsa_package *package = NULL;
    UNICODE_STRING target_name_us;
    BOOLEAN mapped_context;
    LSA_SEC_HANDLE new_handle;

    TRACE("%p %p %s %#lx %ld %ld %p %ld %p %p %p %p\n", credential, context,
        debugstr_w(target_name), context_req, reserved1, target_data_rep, input,
        reserved2, new_context, output, context_attr, ts_expiry);

    if (context)
    {
        lsa_ctx = (struct lsa_handle *)context->dwLower;
        if (lsa_ctx->magic != LSA_MAGIC_CONTEXT) return SEC_E_INVALID_HANDLE;
        package = lsa_ctx->package;
    }
    else if (credential)
    {
        lsa_cred = (struct lsa_handle *)credential->dwLower;
        if (lsa_cred->magic != LSA_MAGIC_CREDENTIALS) return SEC_E_INVALID_HANDLE;
        package = lsa_cred->package;
    }
    if (!package || !new_context) return SEC_E_INVALID_HANDLE;

    if (!package->lsa_api || !package->lsa_api->InitLsaModeContext)
        return SEC_E_UNSUPPORTED_FUNCTION;

    if (target_name)
        RtlInitUnicodeString(&target_name_us, target_name);

    status = package->lsa_api->InitLsaModeContext(lsa_cred ? lsa_cred->handle : 0,
        lsa_ctx ? lsa_ctx->handle : 0, target_name ? &target_name_us : NULL, context_req, target_data_rep,
        input, &new_handle, output, context_attr, ts_expiry, &mapped_context, NULL /* FIXME */);
    if (status == SEC_E_OK || status == SEC_I_CONTINUE_NEEDED)
    {
        if (!(new_lsa_ctx = alloc_lsa_handle(LSA_MAGIC_CONTEXT))) return STATUS_NO_MEMORY;
        new_lsa_ctx->package = package;
        new_lsa_ctx->handle = new_handle;
        new_context->dwLower = (ULONG_PTR)new_lsa_ctx;
        new_context->dwUpper = 0;
    }
    return status;
}

static SECURITY_STATUS WINAPI lsa_InitializeSecurityContextA(
    CredHandle *credential, CtxtHandle *context, SEC_CHAR *target_name, ULONG context_req,
    ULONG reserved1, ULONG target_data_rep, SecBufferDesc *input, ULONG reserved2,
    CtxtHandle *new_context, SecBufferDesc *output, ULONG *context_attr, TimeStamp *ts_expiry)
{
    SECURITY_STATUS status;
    SEC_WCHAR *targetW = NULL;

    TRACE("%p %p %s %#lx %ld %ld %p %ld %p %p %p %p\n", credential, context,
        debugstr_a(target_name), context_req, reserved1, target_data_rep, input,
        reserved2, new_context, output, context_attr, ts_expiry);

    if (target_name)
    {
        int len = MultiByteToWideChar( CP_ACP, 0, target_name, -1, NULL, 0 );
        if (!(targetW = malloc( len * sizeof(SEC_WCHAR) ))) return SEC_E_INSUFFICIENT_MEMORY;
        MultiByteToWideChar( CP_ACP, 0, target_name, -1, targetW, len );
    }

    status = lsa_InitializeSecurityContextW( credential, context, targetW, context_req, reserved1, target_data_rep,
                                             input, reserved2, new_context, output, context_attr, ts_expiry );
    free( targetW );
    return status;
}

static SECURITY_STATUS WINAPI lsa_AcceptSecurityContext(
    CredHandle *credential, CtxtHandle *context, SecBufferDesc *input,
    ULONG context_req, ULONG target_data_rep, CtxtHandle *new_context,
    SecBufferDesc *output, ULONG *context_attr, TimeStamp *ts_expiry)
{
    SECURITY_STATUS status;
    struct lsa_package *package = NULL;
    struct lsa_handle *lsa_cred = NULL, *lsa_ctx = NULL, *new_lsa_ctx;
    BOOLEAN mapped_context;
    LSA_SEC_HANDLE new_handle;

    TRACE("%p %p %p %#lx %#lx %p %p %p %p\n", credential, context, input,
        context_req, target_data_rep, new_context, output, context_attr, ts_expiry);

    if (context)
    {
        lsa_ctx = (struct lsa_handle *)context->dwLower;
        if (lsa_ctx->magic != LSA_MAGIC_CONTEXT) return SEC_E_INVALID_HANDLE;
        package = lsa_ctx->package;
    }
    else if (credential)
    {
        lsa_cred = (struct lsa_handle *)credential->dwLower;
        if (lsa_cred->magic != LSA_MAGIC_CREDENTIALS) return SEC_E_INVALID_HANDLE;
        package = lsa_cred->package;
    }
    if (!package || !new_context) return SEC_E_INVALID_HANDLE;

    if (!package->lsa_api || !package->lsa_api->AcceptLsaModeContext)
        return SEC_E_UNSUPPORTED_FUNCTION;

    status = package->lsa_api->AcceptLsaModeContext(lsa_cred ? lsa_cred->handle : 0,
        lsa_ctx ? lsa_ctx->handle : 0, input, context_req, target_data_rep, &new_handle, output,
        context_attr, ts_expiry, &mapped_context, NULL /* FIXME */);
    if (status == SEC_E_OK || status == SEC_I_CONTINUE_NEEDED)
    {
        if (!(new_lsa_ctx = alloc_lsa_handle(LSA_MAGIC_CONTEXT))) return STATUS_NO_MEMORY;
        new_lsa_ctx->package = package;
        new_lsa_ctx->handle = new_handle;
        new_context->dwLower = (ULONG_PTR)new_lsa_ctx;
        new_context->dwUpper = 0;
    }
    return status;
}

static SECURITY_STATUS WINAPI lsa_DeleteSecurityContext(CtxtHandle *context)
{
    struct lsa_handle *lsa_ctx;
    SECURITY_STATUS status;

    TRACE("%p\n", context);

    if (!context) return SEC_E_INVALID_HANDLE;
    lsa_ctx = (struct lsa_handle *)context->dwLower;
    if (!lsa_ctx || lsa_ctx->magic != LSA_MAGIC_CONTEXT) return SEC_E_INVALID_HANDLE;

    if (!lsa_ctx->package->lsa_api || !lsa_ctx->package->lsa_api->DeleteContext)
        return SEC_E_UNSUPPORTED_FUNCTION;

    status = lsa_ctx->package->lsa_api->DeleteContext(lsa_ctx->handle);
    free(lsa_ctx);
    return status;
}

static SECURITY_STATUS WINAPI lsa_QueryContextAttributesW(CtxtHandle *context, ULONG attribute, void *buffer)
{
    struct lsa_handle *lsa_ctx;

    TRACE("%p %ld %p\n", context, attribute, buffer);

    if (!context) return SEC_E_INVALID_HANDLE;
    lsa_ctx = (struct lsa_handle *)context->dwLower;
    if (!lsa_ctx || lsa_ctx->magic != LSA_MAGIC_CONTEXT) return SEC_E_INVALID_HANDLE;

    if (!lsa_ctx->package->lsa_api || !lsa_ctx->package->lsa_api->SpQueryContextAttributes)
        return SEC_E_UNSUPPORTED_FUNCTION;

    return lsa_ctx->package->lsa_api->SpQueryContextAttributes(lsa_ctx->handle, attribute, buffer);
}

static SecPkgInfoA *package_infoWtoA( const SecPkgInfoW *info )
{
    SecPkgInfoA *ret;
    int size_name = WideCharToMultiByte( CP_ACP, 0, info->Name, -1, NULL, 0, NULL, NULL );
    int size_comment = WideCharToMultiByte( CP_ACP, 0, info->Comment, -1, NULL, 0, NULL, NULL );

    /* freed with FreeContextBuffer */
    if (!(ret = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*ret) + size_name + size_comment ))) return NULL;
    ret->fCapabilities = info->fCapabilities;
    ret->wVersion      = info->wVersion;
    ret->wRPCID        = info->wRPCID;
    ret->cbMaxToken    = info->cbMaxToken;
    ret->Name          = (SEC_CHAR *)(ret + 1);
    WideCharToMultiByte( CP_ACP, 0, info->Name, -1, ret->Name, size_name, NULL, NULL );
    ret->Comment       = ret->Name + size_name;
    WideCharToMultiByte( CP_ACP, 0, info->Comment, -1, ret->Comment, size_comment, NULL, NULL );
    return ret;
}

static SECURITY_STATUS nego_info_WtoA( const SecPkgContext_NegotiationInfoW *infoW,
                                       SecPkgContext_NegotiationInfoA *infoA )
{
    infoA->NegotiationState = infoW->NegotiationState;
    if (!(infoA->PackageInfo = package_infoWtoA( infoW->PackageInfo ))) return SEC_E_INSUFFICIENT_MEMORY;
    return SEC_E_OK;
}

static SECURITY_STATUS key_info_WtoA( const SecPkgContext_KeyInfoW *infoW, SecPkgContext_KeyInfoA *infoA )
{
    int size;

    size = WideCharToMultiByte( CP_ACP, 0, infoW->sSignatureAlgorithmName, -1, NULL, 0, NULL, NULL );
    if (!(infoA->sSignatureAlgorithmName = RtlAllocateHeap( GetProcessHeap(), 0, size )))
        return SEC_E_INSUFFICIENT_MEMORY;
    WideCharToMultiByte( CP_ACP, 0, infoW->sSignatureAlgorithmName, -1, infoA->sSignatureAlgorithmName,
                         size, NULL, NULL );

    size = WideCharToMultiByte( CP_ACP, 0, infoW->sEncryptAlgorithmName, -1, NULL, 0, NULL, NULL );
    if (!(infoA->sEncryptAlgorithmName = RtlAllocateHeap( GetProcessHeap(), 0, size )))
    {
        RtlFreeHeap( GetProcessHeap(), 0, infoA->sSignatureAlgorithmName );
        return SEC_E_INSUFFICIENT_MEMORY;
    }
    WideCharToMultiByte( CP_ACP, 0, infoW->sEncryptAlgorithmName, -1, infoA->sEncryptAlgorithmName,
                         size, NULL, NULL );

    infoA->KeySize = infoW->KeySize;
    infoA->SignatureAlgorithm = infoW->SignatureAlgorithm;
    infoA->EncryptAlgorithm = infoW->EncryptAlgorithm;
    return SEC_E_OK;
}

static SECURITY_STATUS WINAPI lsa_QueryContextAttributesA(CtxtHandle *context, ULONG attribute, void *buffer)
{
    TRACE("%p %ld %p\n", context, attribute, buffer);

    if (!context) return SEC_E_INVALID_HANDLE;

    switch (attribute)
    {
    case SECPKG_ATTR_SIZES:
    case SECPKG_ATTR_SESSION_KEY:
        return lsa_QueryContextAttributesW( context, attribute, buffer );

    case SECPKG_ATTR_NEGOTIATION_INFO:
    {
        SecPkgContext_NegotiationInfoW infoW;
        SecPkgContext_NegotiationInfoA *infoA = (SecPkgContext_NegotiationInfoA *)buffer;
        SECURITY_STATUS status = lsa_QueryContextAttributesW( context, SECPKG_ATTR_NEGOTIATION_INFO, &infoW );

        if (status != SEC_E_OK) return status;
        status = nego_info_WtoA( &infoW, infoA );
        FreeContextBuffer( infoW.PackageInfo );
        return status;
    }
    case SECPKG_ATTR_KEY_INFO:
    {
        SecPkgContext_KeyInfoW infoW;
        SecPkgContext_KeyInfoA *infoA = (SecPkgContext_KeyInfoA *)buffer;

        SECURITY_STATUS status = lsa_QueryContextAttributesW( context, SECPKG_ATTR_KEY_INFO, &infoW );

        if (status != SEC_E_OK) return status;
        status = key_info_WtoA( &infoW, infoA );
        FreeContextBuffer( infoW.sSignatureAlgorithmName );
        FreeContextBuffer( infoW.sEncryptAlgorithmName );
        return status;
    }

#define X(x) case (x) : FIXME(#x" stub\n"); break
    X(SECPKG_ATTR_ACCESS_TOKEN);
    X(SECPKG_ATTR_AUTHORITY);
    X(SECPKG_ATTR_DCE_INFO);
    X(SECPKG_ATTR_LIFESPAN);
    X(SECPKG_ATTR_NAMES);
    X(SECPKG_ATTR_NATIVE_NAMES);
    X(SECPKG_ATTR_PACKAGE_INFO);
    X(SECPKG_ATTR_PASSWORD_EXPIRY);
    X(SECPKG_ATTR_STREAM_SIZES);
    X(SECPKG_ATTR_TARGET_INFORMATION);
#undef X
    default:
        FIXME( "unknown attribute %lu\n", attribute );
        break;
    }

    return SEC_E_UNSUPPORTED_FUNCTION;
}

static SECURITY_STATUS WINAPI lsa_MakeSignature(CtxtHandle *context, ULONG quality_of_protection,
    SecBufferDesc *message, ULONG message_seq_no)
{
    struct lsa_handle *lsa_ctx;

    TRACE("%p %#lx %p %lu)\n", context, quality_of_protection, message, message_seq_no);

    if (!context) return SEC_E_INVALID_HANDLE;
    lsa_ctx = (struct lsa_handle *)context->dwLower;
    if (!lsa_ctx || lsa_ctx->magic != LSA_MAGIC_CONTEXT) return SEC_E_INVALID_HANDLE;

    if (!lsa_ctx->package->user_api || !lsa_ctx->package->user_api->MakeSignature)
        return SEC_E_UNSUPPORTED_FUNCTION;

    return lsa_ctx->package->user_api->MakeSignature(lsa_ctx->handle, quality_of_protection, message, message_seq_no);
}

static SECURITY_STATUS WINAPI lsa_VerifySignature(CtxtHandle *context, SecBufferDesc *message,
    ULONG message_seq_no, ULONG *quality_of_protection)
{
    struct lsa_handle *lsa_ctx;

    TRACE("%p %p %lu %p)\n", context, message, message_seq_no, quality_of_protection);

    if (!context) return SEC_E_INVALID_HANDLE;
    lsa_ctx = (struct lsa_handle *)context->dwLower;
    if (!lsa_ctx || lsa_ctx->magic != LSA_MAGIC_CONTEXT) return SEC_E_INVALID_HANDLE;

    if (!lsa_ctx->package->user_api || !lsa_ctx->package->user_api->VerifySignature)
        return SEC_E_UNSUPPORTED_FUNCTION;

    return lsa_ctx->package->user_api->VerifySignature(lsa_ctx->handle, message, message_seq_no, quality_of_protection);
}

static SECURITY_STATUS WINAPI lsa_EncryptMessage(CtxtHandle *context, ULONG quality_of_protection,
    SecBufferDesc *message, ULONG message_seq_no)
{
    struct lsa_handle *lsa_ctx;

    TRACE("%p %#lx %p %lu)\n", context, quality_of_protection, message, message_seq_no);

    if (!context) return SEC_E_INVALID_HANDLE;
    lsa_ctx = (struct lsa_handle *)context->dwLower;
    if (!lsa_ctx || lsa_ctx->magic != LSA_MAGIC_CONTEXT) return SEC_E_INVALID_HANDLE;

    if (!lsa_ctx->package->user_api || !lsa_ctx->package->user_api->SealMessage)
        return SEC_E_UNSUPPORTED_FUNCTION;

    return lsa_ctx->package->user_api->SealMessage(lsa_ctx->handle, quality_of_protection, message, message_seq_no);
}

static SECURITY_STATUS WINAPI lsa_DecryptMessage(CtxtHandle *context, SecBufferDesc *message,
    ULONG message_seq_no, ULONG *quality_of_protection)
{
    struct lsa_handle *lsa_ctx;

    TRACE("%p %p %lu %p)\n", context, message, message_seq_no, quality_of_protection);

    if (!context) return SEC_E_INVALID_HANDLE;
    lsa_ctx = (struct lsa_handle *)context->dwLower;
    if (!lsa_ctx || lsa_ctx->magic != LSA_MAGIC_CONTEXT) return SEC_E_INVALID_HANDLE;

    if (!lsa_ctx->package->user_api || !lsa_ctx->package->user_api->UnsealMessage)
        return SEC_E_UNSUPPORTED_FUNCTION;

    return lsa_ctx->package->user_api->UnsealMessage(lsa_ctx->handle, message, message_seq_no, quality_of_protection);
}

static const SecurityFunctionTableW lsa_sspi_tableW =
{
    1,
    NULL, /* EnumerateSecurityPackagesW */
    NULL, /* QueryCredentialsAttributesW */
    lsa_AcquireCredentialsHandleW,
    lsa_FreeCredentialsHandle,
    NULL, /* Reserved2 */
    lsa_InitializeSecurityContextW,
    lsa_AcceptSecurityContext,
    NULL, /* CompleteAuthToken */
    lsa_DeleteSecurityContext,
    NULL, /* ApplyControlToken */
    lsa_QueryContextAttributesW,
    NULL, /* ImpersonateSecurityContext */
    NULL, /* RevertSecurityContext */
    lsa_MakeSignature,
    lsa_VerifySignature,
    NULL, /* FreeContextBuffer */
    NULL, /* QuerySecurityPackageInfoW */
    NULL, /* Reserved3 */
    NULL, /* Reserved4 */
    NULL, /* ExportSecurityContext */
    NULL, /* ImportSecurityContextW */
    NULL, /* AddCredentialsW */
    NULL, /* Reserved8 */
    NULL, /* QuerySecurityContextToken */
    lsa_EncryptMessage,
    lsa_DecryptMessage,
    NULL, /* SetContextAttributesW */
};

static const SecurityFunctionTableA lsa_sspi_tableA =
{
    1,
    NULL, /* EnumerateSecurityPackagesA */
    NULL, /* QueryCredentialsAttributesA */
    lsa_AcquireCredentialsHandleA,
    lsa_FreeCredentialsHandle,
    NULL, /* Reserved2 */
    lsa_InitializeSecurityContextA,
    lsa_AcceptSecurityContext,
    NULL, /* CompleteAuthToken */
    lsa_DeleteSecurityContext,
    NULL, /* ApplyControlToken */
    lsa_QueryContextAttributesA,
    NULL, /* ImpersonateSecurityContext */
    NULL, /* RevertSecurityContext */
    lsa_MakeSignature,
    lsa_VerifySignature,
    NULL, /* FreeContextBuffer */
    NULL, /* QuerySecurityPackageInfoA */
    NULL, /* Reserved3 */
    NULL, /* Reserved4 */
    NULL, /* ExportSecurityContext */
    NULL, /* ImportSecurityContextA */
    NULL, /* AddCredentialsA */
    NULL, /* Reserved8 */
    NULL, /* QuerySecurityContextToken */
    lsa_EncryptMessage,
    lsa_DecryptMessage,
    NULL, /* SetContextAttributesA */
};

static void add_package(struct lsa_package *package)
{
    struct lsa_package *new_loaded_packages;

    if (!loaded_packages)
        new_loaded_packages = malloc(sizeof(*new_loaded_packages));
    else
        new_loaded_packages = realloc(loaded_packages, sizeof(*new_loaded_packages) * (loaded_packages_count + 1));

    if (new_loaded_packages)
    {
        loaded_packages = new_loaded_packages;
        loaded_packages[loaded_packages_count] = *package;
        loaded_packages_count++;
    }
}

static BOOL initialize_package(struct lsa_package *package,
                               NTSTATUS (NTAPI *pSpLsaModeInitialize)(ULONG, PULONG, PSECPKG_FUNCTION_TABLE *, PULONG),
                               NTSTATUS (NTAPI *pSpUserModeInitialize)(ULONG, PULONG, PSECPKG_USER_FUNCTION_TABLE *, PULONG))
{
    NTSTATUS status;

    if (!pSpLsaModeInitialize || !pSpUserModeInitialize)
        return FALSE;

    status = pSpLsaModeInitialize(SECPKG_INTERFACE_VERSION, &package->lsa_api_version, &package->lsa_api, &package->lsa_table_count);
    if (status == STATUS_SUCCESS)
    {
        status = package->lsa_api->InitializePackage(package->package_id, &lsa_dispatch, NULL, NULL, &package->name);
        if (status == STATUS_SUCCESS)
        {
            TRACE("name %s, version %#lx, api table %p, table count %lu\n",
                  debugstr_an(package->name->Buffer, package->name->Length),
                  package->lsa_api_version, package->lsa_api, package->lsa_table_count);

            status = package->lsa_api->Initialize(package->package_id, NULL /* FIXME: params */, NULL);
            if (status == STATUS_SUCCESS)
            {
                status = pSpUserModeInitialize(SECPKG_INTERFACE_VERSION, &package->user_api_version, &package->user_api, &package->user_table_count);
                if (status == STATUS_SUCCESS)
                {
                    package->user_api->InstanceInit(SECPKG_INTERFACE_VERSION, &lsa_dll_dispatch, NULL);
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

static BOOL load_package(const WCHAR *name, struct lsa_package *package, ULONG package_id)
{
    NTSTATUS (NTAPI *pSpLsaModeInitialize)(ULONG, PULONG, PSECPKG_FUNCTION_TABLE *, PULONG);
    NTSTATUS (NTAPI *pSpUserModeInitialize)(ULONG, PULONG, PSECPKG_USER_FUNCTION_TABLE *, PULONG);

    memset(package, 0, sizeof(*package));

    package->package_id = package_id;
    package->mod = LoadLibraryW(name);
    if (!package->mod) return FALSE;

    pSpLsaModeInitialize = (void *)GetProcAddress(package->mod, "SpLsaModeInitialize");
    pSpUserModeInitialize = (void *)GetProcAddress(package->mod, "SpUserModeInitialize");

    if (initialize_package(package, pSpLsaModeInitialize, pSpUserModeInitialize))
        return TRUE;

    FreeLibrary(package->mod);
    return FALSE;
}

#define MAX_SERVICE_NAME 260

void load_auth_packages(void)
{
    DWORD err, i;
    HKEY root;
    SecureProvider *provider;
    struct lsa_package package;

    memset(&package, 0, sizeof(package));

    /* "Negotiate" has package id 0, .Net depends on this. */
    package.package_id = 0;
    if (initialize_package(&package, nego_SpLsaModeInitialize, nego_SpUserModeInitialize))
        add_package(&package);

    err = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\Lsa", 0, KEY_READ, &root);
    if (err != ERROR_SUCCESS) return;

    i = 0;
    for (;;)
    {
        WCHAR name[MAX_SERVICE_NAME];

        err = RegEnumKeyW(root, i, name, MAX_SERVICE_NAME);
        if (err == ERROR_NO_MORE_ITEMS)
            break;

        if (err != ERROR_SUCCESS)
            continue;

        if (load_package(name, &package, i + 1))
            add_package(&package);

        i++;
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

        info = malloc(loaded_packages[i].lsa_table_count * sizeof(*info));
        if (info)
        {
            NTSTATUS status;

            status = loaded_packages[i].lsa_api->GetInfo(info);
            if (status == STATUS_SUCCESS)
                SECUR32_addPackages(provider, loaded_packages[i].lsa_table_count, NULL, info);

            free(info);
        }
    }
}

NTSTATUS WINAPI LsaLookupAuthenticationPackage(HANDLE lsa_handle,
        PLSA_STRING package_name, PULONG package_id)
{
    ULONG i;

    TRACE("%p %s %p\n", lsa_handle, debugstr_as(package_name), package_id);

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
