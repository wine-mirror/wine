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
#include "rpc.h"

#include "wine/debug.h"
#include "secur32_priv.h"

WINE_DEFAULT_DEBUG_CHANNEL(secur32);

#define LSA_MAGIC_CONNECTION  ('L' << 24 | 'S' << 16 | 'A' << 8 | '0')
#define LSA_MAGIC_CREDENTIALS ('L' << 24 | 'S' << 16 | 'A' << 8 | '1')
#define LSA_MAGIC_CONTEXT     ('L' << 24 | 'S' << 16 | 'A' << 8 | '2')

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
    lsa_conn->magic = 0;
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
    FIXME("%p %s %d %d %p %d %p %p %p %p %p %p %p %p stub\n", LsaHandle,
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
    FIXME("%p,%u,%s,%s: stub\n", logon_id, package_id,
        debugstr_as(primary_key), debugstr_as(credentials));
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
    FIXME("%p,%#x,%s: stub\n", logon_id, package_id, debugstr_as(primary_key));
    return STATUS_NOT_IMPLEMENTED;
}

static void * NTAPI lsa_AllocateLsaHeap(ULONG size)
{
    TRACE("%u\n", size);
    return malloc(size);
}

static void NTAPI lsa_FreeLsaHeap(void *p)
{
    TRACE("%p\n", p);
    free(p);
}

static NTSTATUS NTAPI lsa_AllocateClientBuffer(PLSA_CLIENT_REQUEST req, ULONG size, void **p)
{
    TRACE("%p,%u,%p\n", req, size, p);
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

    TRACE("%s %s %#x %p %p %p %p %p\n", debugstr_w(principal), debugstr_w(package),
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
    int len_user = 0, len_domain = 0, len_passwd = 0;
    SEC_WCHAR *principalW = NULL, *packageW = NULL, *user = NULL, *domain = NULL, *passwd = NULL;
    SEC_WINNT_AUTH_IDENTITY_W *auth_dataW = NULL;
    SEC_WINNT_AUTH_IDENTITY_A *id = NULL;

    TRACE("%s %s %#x %p %p %p %p %p\n", debugstr_a(principal), debugstr_a(package),
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
    if (auth_data)
    {
        id = (PSEC_WINNT_AUTH_IDENTITY_A)auth_data;

        if (id->Flags == SEC_WINNT_AUTH_IDENTITY_ANSI)
        {
            if (!(auth_dataW = malloc( sizeof(SEC_WINNT_AUTH_IDENTITY_W) ))) goto done;
            if (id->UserLength)
            {
                len_user = MultiByteToWideChar( CP_ACP, 0, (char *)id->User, id->UserLength, NULL, 0 );
                if (!(user = malloc( len_user * sizeof(SEC_WCHAR) ))) goto done;
                MultiByteToWideChar( CP_ACP, 0, (char *)id->User, id->UserLength, user, len_user );
            }
            if (id->DomainLength)
            {
                len_domain = MultiByteToWideChar( CP_ACP, 0, (char *)id->Domain, id->DomainLength, NULL, 0 );
                if (!(domain = malloc( len_domain * sizeof(SEC_WCHAR) ))) goto done;
                MultiByteToWideChar( CP_ACP, 0, (char *)id->Domain, id->DomainLength, domain, len_domain );
            }
            if (id->PasswordLength)
            {
                len_passwd = MultiByteToWideChar( CP_ACP, 0, (char *)id->Password, id->PasswordLength, NULL, 0 );
                if (!(passwd = malloc( len_passwd * sizeof(SEC_WCHAR) ))) goto done;
                MultiByteToWideChar( CP_ACP, 0, (char *)id->Password, id->PasswordLength, passwd, len_passwd );
            }
            auth_dataW->Flags          = SEC_WINNT_AUTH_IDENTITY_UNICODE;
            auth_dataW->User           = user;
            auth_dataW->UserLength     = len_user;
            auth_dataW->Domain         = domain;
            auth_dataW->DomainLength   = len_domain;
            auth_dataW->Password       = passwd;
            auth_dataW->PasswordLength = len_passwd;
        }
        else auth_dataW = (PSEC_WINNT_AUTH_IDENTITY_W)auth_data;
    }

    status = lsa_AcquireCredentialsHandleW( principalW, packageW, credentials_use, logon_id, auth_dataW, get_key_fn,
                                            get_key_arg, credential, ts_expiry );
done:
    if (auth_dataW != (SEC_WINNT_AUTH_IDENTITY_W *)id) free( auth_dataW );
    free( packageW );
    free( principalW );
    free( user );
    free( domain );
    free( passwd );
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

    lsa_cred->magic = 0;
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

    TRACE("%p %p %s %#x %d %d %p %d %p %p %p %p\n", credential, context,
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

    TRACE("%p %p %s %#x %d %d %p %d %p %p %p %p\n", credential, context,
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

    TRACE("%p %p %p %#x %#x %p %p %p %p\n", credential, context, input,
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

    TRACE("%p %d %p\n", context, attribute, buffer);

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

static SECURITY_STATUS WINAPI lsa_QueryContextAttributesA(CtxtHandle *context, ULONG attribute, void *buffer)
{
    TRACE("%p %d %p\n", context, attribute, buffer);

    if (!context) return SEC_E_INVALID_HANDLE;

    switch (attribute)
    {
    case SECPKG_ATTR_SIZES:
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

#define X(x) case (x) : FIXME(#x" stub\n"); break
    X(SECPKG_ATTR_ACCESS_TOKEN);
    X(SECPKG_ATTR_AUTHORITY);
    X(SECPKG_ATTR_DCE_INFO);
    X(SECPKG_ATTR_KEY_INFO);
    X(SECPKG_ATTR_LIFESPAN);
    X(SECPKG_ATTR_NAMES);
    X(SECPKG_ATTR_NATIVE_NAMES);
    X(SECPKG_ATTR_PACKAGE_INFO);
    X(SECPKG_ATTR_PASSWORD_EXPIRY);
    X(SECPKG_ATTR_SESSION_KEY);
    X(SECPKG_ATTR_STREAM_SIZES);
    X(SECPKG_ATTR_TARGET_INFORMATION);
#undef X
    default:
        FIXME( "unknown attribute %u\n", attribute );
        break;
    }

    return SEC_E_UNSUPPORTED_FUNCTION;
}

static SECURITY_STATUS WINAPI lsa_MakeSignature(CtxtHandle *context, ULONG quality_of_protection,
    SecBufferDesc *message, ULONG message_seq_no)
{
    struct lsa_handle *lsa_ctx;

    TRACE("%p %#x %p %u)\n", context, quality_of_protection, message, message_seq_no);

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

    TRACE("%p %p %u %p)\n", context, message, message_seq_no, quality_of_protection);

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

    TRACE("%p %#x %p %u)\n", context, quality_of_protection, message, message_seq_no);

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

    TRACE("%p %p %u %p)\n", context, message, message_seq_no, quality_of_protection);

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
    DWORD err, i;
    HKEY root;
    SecureProvider *provider;

    err = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\Lsa", 0, KEY_READ, &root);
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
