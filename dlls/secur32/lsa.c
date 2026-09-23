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
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winsvc.h"
#include "sspi.h"
#include "ntsecapi.h"
#include "ntsecpkg.h"
#include "winternl.h"
#include "ddk/ntddk.h"
#include "rpc.h"
#include "lsass.h"

#include "wine/debug.h"
#include "wine/exception.h"
#include "secur32_priv.h"

WINE_DEFAULT_DEBUG_CHANNEL(secur32);

#define LSA_MAGIC_CONNECTION  ('L' << 24 | 'S' << 16 | 'A' << 8 | '0')
#define LSA_MAGIC_CREDENTIALS ('L' << 24 | 'S' << 16 | 'A' << 8 | '1')
#define LSA_MAGIC_CONTEXT     ('L' << 24 | 'S' << 16 | 'A' << 8 | '2')

static const WCHAR *default_authentication_package = L"Negotiate";

struct lsa_package
{
    HMODULE mod;
    SecPkgInfoW info;
    SECPKG_USER_FUNCTION_TABLE *user_api;
};

static struct lsa_package *loaded_packages;
static ULONG loaded_packages_count;

#define LSA_USER_HANDLE(x) (LSA_SEC_HANDLE)(&(x))
struct lsa_handle
{
    DWORD magic;
    struct lsa_package *package;
    ULONG64 handle;
};

static char *strdupWA( const WCHAR *str )
{
    char *ret = NULL;
    if (str)
    {
        int len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
        if ((ret = RtlAllocateHeap( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    }
    return ret;
}

static const char *debugstr_as(const LSA_STRING *str)
{
    if (!str) return "<null>";
    return debugstr_an(str->Buffer, str->Length);
}

void* __RPC_USER MIDL_user_allocate( SIZE_T size )
{
    return malloc( size );
}

void __RPC_USER MIDL_user_free( void *p )
{
    free( p );
}

ULONG __RPC_USER CLIENT_PTR_UserSize( ULONG *flags, ULONG pos, CLIENT_PTR *client_ptr )
{
    return sizeof(ULONG64);
}

unsigned char* __RPC_USER CLIENT_PTR_UserMarshal( ULONG *flags, unsigned char *buf, CLIENT_PTR *client_ptr )
{
    ULONG64 data = (ULONG_PTR)*client_ptr;

    memcpy( buf, &data, sizeof(data) );
    return buf + sizeof(data);
}

unsigned char* __RPC_USER CLIENT_PTR_UserUnmarshal( ULONG *flags, unsigned char *buf, CLIENT_PTR *client_ptr )
{
    ULONG64 data;

    memcpy( &data, buf, sizeof(data) );
    *(ULONG_PTR*)client_ptr = data;
    return buf + sizeof(data);
}

void __RPC_USER CLIENT_PTR_UserFree( ULONG *flags, CLIENT_PTR *client_ptr )
{
}

static LONG WINAPI rpc_filter(EXCEPTION_POINTERS *eptr)
{
    return I_RpcExceptionFilter(eptr->ExceptionRecord->ExceptionCode);
}

static BOOL start_samss(void)
{
    SERVICE_STATUS_PROCESS status;
    SC_HANDLE scm, service;
    BOOL ret = FALSE;

    TRACE("\n");

    if (!(scm = OpenSCManagerW(NULL, NULL, 0)))
    {
        ERR("Failed to open service manager\n");
        return FALSE;
    }

    if (!(service = OpenServiceW(scm, L"SamSs", SERVICE_START | SERVICE_QUERY_STATUS)))
    {
        ERR("Failed to open SamSs service\n");
        CloseServiceHandle( scm );
        return FALSE;
    }

    if (StartServiceW(service, 0, NULL) || GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
    {
        ULONGLONG start_time = GetTickCount64();
        do
        {
            DWORD dummy;

            if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (BYTE *)&status, sizeof(status), &dummy))
                break;
            if (status.dwCurrentState == SERVICE_RUNNING)
            {
                ret = TRUE;
                break;
            }
            if (GetTickCount64() - start_time > 30000) break;
            Sleep( 100 );

        } while (status.dwCurrentState == SERVICE_START_PENDING);

        if (status.dwCurrentState != SERVICE_RUNNING)
            WARN("SamSs failed to start %lu\n", status.dwCurrentState);
    }
    else
        ERR("Failed to start SamSs service\n");

    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return ret;
}

#define LSASS_CALL_START \
    for (;;) { \
        DWORD err = 0; \
        __TRY {

#define LSASS_CALL_END \
        } __EXCEPT(rpc_filter) { \
            err = GetExceptionCode(); \
            status = SEC_E_INTERNAL_ERROR; \
        } \
        __ENDTRY \
        if (err == RPC_S_SERVER_UNAVAILABLE) { \
            if (start_samss()) \
                continue; \
        } \
        break; \
    }

static RPC_BINDING_HANDLE get_lsass_handle(void)
{
    static RPC_BINDING_HANDLE irpcss_handle;

    if (!irpcss_handle)
    {
        unsigned short protseq[] = LSASS_PROTSEQ;
        unsigned short endpoint[] = LSASS_ENDPOINT;
        RPC_BINDING_HANDLE handle;
        RPC_STATUS status;
        RPC_WSTR binding;

        status = RpcStringBindingComposeW(NULL, protseq, NULL, endpoint, NULL, &binding);
        if (status != RPC_S_OK)
            return NULL;

        status = RpcBindingFromStringBindingW(binding, &handle);
        RpcStringFreeW(&binding);
        if (status != RPC_S_OK)
            return NULL;

        if (InterlockedCompareExchangePointer(&irpcss_handle, handle, NULL))
            /* another thread beat us to it */
            RpcBindingFree(&handle);
    }
    return irpcss_handle;
}

SECPKG_USER_FUNCTION_TABLE *lsa_find_func_table( const WCHAR *name )
{
    ULONG i;

    for (i = 0; i < loaded_packages_count; i++)
    {
        if (!wcscmp( loaded_packages[i].info.Name, name ))
            return loaded_packages[i].user_api;
    }
    return NULL;
}

NTSTATUS WINAPI LsaCallAuthenticationPackage(HANDLE lsa_handle, ULONG package_id,
        PVOID in_buffer, ULONG in_buffer_length,
        PVOID *out_buffer, PULONG out_buffer_length, PNTSTATUS prot_status)
{
    NTSTATUS status;

    TRACE("%p,%lu,%p,%lu,%p,%p,%p\n", lsa_handle, package_id, in_buffer,
        in_buffer_length, out_buffer, out_buffer_length, prot_status);

    if (out_buffer) *out_buffer = NULL;
    if (out_buffer_length) *out_buffer_length = 0;
    if (prot_status) *prot_status = STATUS_SUCCESS;

    if (package_id >= loaded_packages_count)
        return STATUS_NO_SUCH_PACKAGE;

    LSASS_CALL_START
    status = call_package_untrusted(get_lsass_handle(), GetCurrentThreadId(),
            0, package_id, in_buffer, in_buffer, in_buffer_length,
            out_buffer, out_buffer_length, prot_status);
    LSASS_CALL_END
    return status;
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
    SIZE_T size = 0;

    TRACE("%p\n", buffer);
    return NtFreeVirtualMemory(GetCurrentProcess(), &buffer, &size, MEM_RELEASE);
}

NTSTATUS WINAPI LsaGetLogonSessionData(PLUID LogonId,
        PSECURITY_LOGON_SESSION_DATA* ppLogonSessionData)
{
    SECURITY_LOGON_SESSION_DATA *data;
    int authpkg_len;
    WCHAR *end;

    FIXME("%p %p semi-stub\n", LogonId, ppLogonSessionData);

    authpkg_len = wcslen(default_authentication_package) * sizeof(WCHAR);

    data = VirtualAlloc(NULL, sizeof(*data) + authpkg_len + sizeof(WCHAR),
            MEM_COMMIT, PAGE_READWRITE);
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

static SECURITY_STATUS WINAPI lsa_QueryCredentialsAttributesW(
        CredHandle *credential, ULONG attr, void *buf)
{
    struct lsa_handle *lsa_cred;
    SECURITY_STATUS status;

    TRACE("%p %lu %p\n", credential, attr, buf);
    if (!credential) return SEC_E_INVALID_HANDLE;

    lsa_cred = (struct lsa_handle *)credential->dwLower;
    if (!lsa_cred || lsa_cred->magic != LSA_MAGIC_CREDENTIALS) return SEC_E_INVALID_HANDLE;

    LSASS_CALL_START
    status = query_credentials_attr(get_lsass_handle(), GetCurrentThreadId(),
            lsa_cred->package - loaded_packages, lsa_cred->handle, attr, buf);
    LSASS_CALL_END
    return status;
}

static SECURITY_STATUS WINAPI lsa_QueryCredentialsAttributesA(
        CredHandle *credential, ULONG attr, void *buf)
{
    SECURITY_STATUS status;

    TRACE("%p %lu %p\n", credential, attr, buf);

    switch (attr)
    {
    case SECPKG_CRED_ATTR_NAMES:
    {
        SecPkgCredentials_NamesA *namesA = buf;
        SecPkgCredentials_NamesW namesW;

        status = lsa_QueryCredentialsAttributesW(credential, attr, &namesW);
        if (status) return status;

        namesA->sUserName = strdupWA(namesW.sUserName);
        FreeContextBuffer(namesW.sUserName);
        if (!namesA->sUserName) return STATUS_NO_MEMORY;
        return SEC_E_OK;
    }
    default:
        FIXME("unsupported attribute: %lu\n", attr);
        return STATUS_NOT_IMPLEMENTED;
    }
}

static SECURITY_STATUS WINAPI lsa_AcquireCredentialsHandleW(
    SEC_WCHAR *principal, SEC_WCHAR *package, ULONG credentials_use,
    LUID *logon_id, void *auth_data, SEC_GET_KEY_FN get_key_fn,
    void *get_key_arg, CredHandle *credential, TimeStamp *ts_expiry)
{
    struct lsa_handle *lsa_handle;
    SECURITY_STATUS status;
    ULONG package_id;

    TRACE("%s %s %#lx %p %p %p %p %p\n", debugstr_w(principal), debugstr_w(package),
        credentials_use, auth_data, get_key_fn, get_key_arg, credential, ts_expiry);

    if (!credential) return SEC_E_INVALID_HANDLE;
    if (!package) return SEC_E_SECPKG_NOT_FOUND;
    if (!(lsa_handle = alloc_lsa_handle(LSA_MAGIC_CREDENTIALS))) return STATUS_NO_MEMORY;

    LSASS_CALL_START
    status = acquire_credentials_handle(get_lsass_handle(), GetCurrentThreadId(),
            principal, package, credentials_use, logon_id, auth_data, get_key_fn,
            get_key_arg, &package_id, &lsa_handle->handle, ts_expiry);
    LSASS_CALL_END

    if (status != SEC_E_OK)
    {
        SecureZeroMemory(&lsa_handle->magic, sizeof(lsa_handle->magic));
        free(lsa_handle);
        return status;
    }

    lsa_handle->package = loaded_packages + package_id;
    credential->dwLower = (ULONG_PTR)lsa_handle;
    credential->dwUpper = 0;
    return status;
}

static SECURITY_STATUS WINAPI lsa_AcquireCredentialsHandleA(
    SEC_CHAR *principal, SEC_CHAR *package, ULONG credentials_use,
    LUID *logon_id, void *auth_data, SEC_GET_KEY_FN get_key_fn,
    void *get_key_arg, CredHandle *credential, TimeStamp *ts_expiry)
{
    SECURITY_STATUS status = SEC_E_INSUFFICIENT_MEMORY;
    SEC_WCHAR *principalW = NULL, *packageW = NULL;

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

    status = lsa_AcquireCredentialsHandleW( principalW, packageW, credentials_use, logon_id, auth_data, get_key_fn,
                                            get_key_arg, credential, ts_expiry );
done:
    free( packageW );
    free( principalW );
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

    LSASS_CALL_START
    status = free_credentials_handle(get_lsass_handle(), GetCurrentThreadId(),
            lsa_cred->package - loaded_packages, lsa_cred->handle);
    LSASS_CALL_END

    /* Ensure compiler doesn't optimize out the assignment with 0. */
    SecureZeroMemory(&lsa_cred->magic, sizeof(lsa_cred->magic));
    free(lsa_cred);
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

    LSASS_CALL_START
    status = delete_security_context(get_lsass_handle(), GetCurrentThreadId(),
            lsa_ctx->package - loaded_packages, lsa_ctx->handle);
    LSASS_CALL_END
    free(lsa_ctx);
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
    BOOLEAN mapped_context = FALSE;
    SecBuffer ctx_data = { 0 };

    TRACE("%p %p %s %#lx %ld %ld %p %ld %p %p %p %p\n", credential, context,
        debugstr_w(target_name), context_req, reserved1, target_data_rep, input,
        reserved2, new_context, output, context_attr, ts_expiry);

    if (input && input->cBuffers > MAX_SEC_BUFFERS) return SEC_E_INVALID_TOKEN;
    if (output && output->cBuffers > MAX_SEC_BUFFERS) return SEC_E_INVALID_TOKEN;

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

    if (!(new_lsa_ctx = alloc_lsa_handle(LSA_MAGIC_CONTEXT))) return STATUS_NO_MEMORY;

    LSASS_CALL_START
    status = initialize_security_context(get_lsass_handle(), GetCurrentThreadId(),
            package - loaded_packages, lsa_cred ? lsa_cred->handle : 0,
            lsa_ctx ? lsa_ctx->handle : 0, target_name, context_req,
            target_data_rep, input, &new_lsa_ctx->handle, output,
            context_attr, ts_expiry, &mapped_context, &ctx_data);
    LSASS_CALL_END
    if (status == SEC_E_OK || status == SEC_I_CONTINUE_NEEDED)
    {
        new_lsa_ctx->package = package;
        new_context->dwLower = (ULONG_PTR)new_lsa_ctx;
        new_context->dwUpper = 0;

        if (mapped_context)
        {
            NTSTATUS ret = package->user_api->InitUserModeContext(
                    LSA_USER_HANDLE(new_lsa_ctx->handle), &ctx_data );
            FreeContextBuffer( ctx_data.pvBuffer );
            if (ret)
            {
                lsa_DeleteSecurityContext( new_context );
                return ret;
            }
        }
    }
    else
    {
        free( new_lsa_ctx );
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
    BOOLEAN mapped_context = FALSE;
    SecBuffer ctx_data = { 0 };

    TRACE("%p %p %p %#lx %#lx %p %p %p %p\n", credential, context, input,
        context_req, target_data_rep, new_context, output, context_attr, ts_expiry);

    if (input && input->cBuffers > MAX_SEC_BUFFERS) return SEC_E_INVALID_TOKEN;
    if (output && output->cBuffers > MAX_SEC_BUFFERS) return SEC_E_INVALID_TOKEN;

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

    if (!(new_lsa_ctx = alloc_lsa_handle(LSA_MAGIC_CONTEXT))) return STATUS_NO_MEMORY;

    LSASS_CALL_START
    status = accept_security_context(get_lsass_handle(), GetCurrentThreadId(),
            package - loaded_packages, lsa_cred ? lsa_cred->handle : 0,
            lsa_ctx ? lsa_ctx->handle : 0, input, context_req,
            target_data_rep, &new_lsa_ctx->handle, output, context_attr,
            ts_expiry, &mapped_context, &ctx_data);
    LSASS_CALL_END
    if (status == SEC_E_OK || status == SEC_I_CONTINUE_NEEDED)
    {
        new_lsa_ctx->package = package;
        new_context->dwLower = (ULONG_PTR)new_lsa_ctx;
        new_context->dwUpper = 0;

        if (mapped_context)
        {
            NTSTATUS ret = package->user_api->InitUserModeContext(
                    LSA_USER_HANDLE(new_lsa_ctx->handle), &ctx_data );
            FreeContextBuffer( ctx_data.pvBuffer );
            if (ret)
            {
                lsa_DeleteSecurityContext( new_context );
                free( new_lsa_ctx );
                return ret;
            }
        }
    }
    else
    {
        free( new_lsa_ctx );
    }
    return status;
}

static SECURITY_STATUS WINAPI lsa_QueryContextAttributesW(CtxtHandle *context, ULONG attribute, void *buffer)
{
    struct lsa_handle *lsa_ctx;
    NTSTATUS status;

    TRACE("%p %ld %p\n", context, attribute, buffer);

    if (!context) return SEC_E_INVALID_HANDLE;
    lsa_ctx = (struct lsa_handle *)context->dwLower;
    if (!lsa_ctx || lsa_ctx->magic != LSA_MAGIC_CONTEXT) return SEC_E_INVALID_HANDLE;

    LSASS_CALL_START
    status = query_context_attr(get_lsass_handle(), GetCurrentThreadId(),
            lsa_ctx->package - loaded_packages, lsa_ctx->handle, attribute, buffer);
    LSASS_CALL_END
    return status;
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

    case SECPKG_ATTR_PACKAGE_INFO:
    {
        SecPkgContext_PackageInfoW infoW;
        SecPkgContext_PackageInfoA *infoA = buffer;
        SECURITY_STATUS status = lsa_QueryContextAttributesW( context, SECPKG_ATTR_PACKAGE_INFO, &infoW );

        if (status != SEC_E_OK) return status;
        infoA->PackageInfo = package_infoWtoA( infoW.PackageInfo );
        FreeContextBuffer( infoW.PackageInfo );
        return infoA->PackageInfo ? SEC_E_OK : SEC_E_INSUFFICIENT_MEMORY;
    }
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

    return lsa_ctx->package->user_api->MakeSignature(LSA_USER_HANDLE(lsa_ctx->handle),
            quality_of_protection, message, message_seq_no);
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

    return lsa_ctx->package->user_api->VerifySignature(LSA_USER_HANDLE(lsa_ctx->handle),
            message, message_seq_no, quality_of_protection);
}

static SECURITY_STATUS WINAPI lsa_QuerySecurityContextToken(CtxtHandle *context, HANDLE *token)
{
    HANDLE primary;
    BOOL r;

    FIXME("%p %p): stub\n", context, token);

    if (!OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &primary))
        return GetLastError();
    r = DuplicateToken(primary, SecurityImpersonation, token);
    CloseHandle(primary);
    return r ? SEC_E_OK : GetLastError();
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

    return lsa_ctx->package->user_api->SealMessage(LSA_USER_HANDLE(lsa_ctx->handle),
            quality_of_protection, message, message_seq_no);
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

    return lsa_ctx->package->user_api->UnsealMessage(LSA_USER_HANDLE(lsa_ctx->handle),
            message, message_seq_no, quality_of_protection);
}

static const SecurityFunctionTableW lsa_sspi_tableW =
{
    1,
    NULL, /* EnumerateSecurityPackagesW */
    lsa_QueryCredentialsAttributesW,
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
    lsa_QuerySecurityContextToken,
    lsa_EncryptMessage,
    lsa_DecryptMessage,
    NULL, /* SetContextAttributesW */
};

static const SecurityFunctionTableA lsa_sspi_tableA =
{
    1,
    NULL, /* EnumerateSecurityPackagesA */
    lsa_QueryCredentialsAttributesA,
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

static BOOL initialize_package(ULONG package_id, HMODULE hmod, ULONG table_no,
                               SecPkgInfoW *info, SpUserModeInitializeFn pSpUserModeInitialize)
{
    ULONG api_version, table_count;
    SECPKG_USER_FUNCTION_TABLE *user_api;
    struct lsa_package *package;
    NTSTATUS status;

    if (!pSpUserModeInitialize)
        return FALSE;

    status = pSpUserModeInitialize(SECPKG_INTERFACE_VERSION, &api_version, &user_api, &table_count);
    if (status || table_no >= table_count)
        return FALSE;
    user_api += table_no;
    user_api->InstanceInit(SECPKG_INTERFACE_VERSION, &lsa_dll_dispatch, NULL);

    package = loaded_packages + package_id;
    package->mod = hmod;
    package->info = *info;
    package->user_api = user_api;
    return TRUE;
}

void load_auth_packages(void)
{
    SecureProvider *provider;
    package_info *packages;
    ULONG i, count;
    NTSTATUS status = SEC_E_INTERNAL_ERROR;

    LSASS_CALL_START
    packages = NULL;
    status = get_packages(get_lsass_handle(), &count, &packages);
    LSASS_CALL_END
    if (status)
    {
        ERR("Failed to get security packages list: %lx\n", status);
        return;
    }

    provider = SECUR32_addProvider(&lsa_sspi_tableA, &lsa_sspi_tableW, NULL);
    if (!provider)
    {
        ERR("Failed to add SSP/AP provider\n");
        return;
    }

    loaded_packages = malloc(sizeof(*loaded_packages) * count);
    if (!loaded_packages)
    {
        for (i = 0; i < count; i++)
        {
            MIDL_user_free(packages[i].module_name);
            MIDL_user_free(packages[i].info.Name);
            MIDL_user_free(packages[i].info.Comment);
        }
        MIDL_user_free(packages);
        return;
    }
    for (i = 0; i < count; i++)
    {
        SpUserModeInitializeFn user_init;
        HMODULE hmod;

        if (!packages[i].module_name)
        {
            hmod = NULL;
            user_init = nego_SpUserModeInitialize;
        }
        else
        {
            hmod = LoadLibraryW(packages[i].module_name);
            user_init = (void *)GetProcAddress(hmod, "SpUserModeInitialize");
        }

        MIDL_user_free(packages[i].module_name);

        if (!initialize_package(i, hmod, packages[i].table_no, &packages[i].info, user_init))
        {
            MIDL_user_free(packages[i].info.Name);
            MIDL_user_free(packages[i].info.Comment);
            FreeLibrary(hmod);
        }
        else
        {
            SECUR32_addPackages(provider, 1, NULL, &loaded_packages[i].info);
        }
    }
    MIDL_user_free(packages);
    loaded_packages_count = count;
}

NTSTATUS WINAPI LsaLookupAuthenticationPackage(HANDLE lsa_handle,
        PLSA_STRING package_name, PULONG package_id)
{
    UNICODE_STRING package_name_us, str;
    ULONG i;

    TRACE("%p %s %p\n", lsa_handle, debugstr_as(package_name), package_id);

    if (RtlAnsiStringToUnicodeString(&package_name_us, package_name, TRUE))
        return STATUS_NO_MEMORY;

    for (i = 0; i < loaded_packages_count; i++)
    {
        RtlInitUnicodeString(&str, loaded_packages[i].info.Name);

        if (RtlEqualUnicodeString(&package_name_us, &str, TRUE))
        {
            RtlFreeUnicodeString(&package_name_us);
            *package_id = i;
            return STATUS_SUCCESS;
        }
    }
    RtlFreeUnicodeString(&package_name_us);

    return STATUS_UNSUCCESSFUL; /* FIXME */
}
