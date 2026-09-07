/*
 * Copyright 2004 Juan Lang
 * Copyright 2007 Kai Blin
 * Copyright 2017, 2018 Dmitry Timoshkov
 * Copyright 2026 Piotr Caban
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

#include "wtypes.h"
#include "sspi.h"
#include "ntstatus.h"
#include "lsass_private.h"
#include "lsass.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(secur32);

struct package
{
    WCHAR module[MAX_PATH];
    ULONG table_no;
    SecPkgInfoW info;
    SECPKG_FUNCTION_TABLE *funcs;
};

static struct package *packages;
static ULONG packages_count, packages_size;

static DWORD tls_index;

static const char *debugstr_as( const LSA_STRING *str )
{
    if (!str) return "<null>";
    return debugstr_an( str->Buffer, str->Length );
}

static void init_call_info( handle_t binding, DWORD thread_id )
{
    SECPKG_CALL_INFO *call_info = TlsGetValue( tls_index );

    if (!call_info)
    {
        call_info = malloc( sizeof(*call_info) );
        if (!call_info) return;
        TlsSetValue( tls_index, call_info );
    }

    call_info->ThreadId = thread_id;
    if (!binding)
    {
        call_info->ProcessId = GetCurrentProcessId();
        call_info->Attributes = SECPKG_CALL_IN_PROC;
    }
    else
    {
        if (I_RpcBindingInqLocalClientPID( binding, &call_info->ProcessId ))
            call_info->ProcessId = 0;
        call_info->Attributes = 0;

        if (call_info->ProcessId)
        {
            HANDLE hproc = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, call_info->ProcessId );
            BOOL wow64 = FALSE;

            if (hproc) IsWow64Process( hproc, &wow64 );
            if (wow64) call_info->Attributes |= SECPKG_CALL_WOWCLIENT;
        }
    }
}

static SECPKG_CALL_INFO *get_call_info( void )
{
    return TlsGetValue( tls_index );
}

static struct package * lsa_lookup_package( const WCHAR *name )
{
    ULONG i;

    for (i = 0; i < packages_count; i++)
    {
        if (!wcscmp( packages[i].info.Name, name ))
            return packages + i;
    }
    return NULL;
}

SECPKG_FUNCTION_TABLE *lsa_find_func_table( const WCHAR *name )
{
    struct package *package = lsa_lookup_package( name );

    if (package) return package->funcs;
    return NULL;
}

static NTSTATUS NTAPI lsa_CreateLogonSession( LUID *logon_id )
{
    FIXME( "%p: stub\n", logon_id );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS NTAPI lsa_DeleteLogonSession( LUID *logon_id )
{
    FIXME( "%p: stub\n", logon_id );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS NTAPI lsa_AddCredential( LUID *logon_id, ULONG package_id,
        LSA_STRING *primary_key, LSA_STRING *credentials )
{
    FIXME( "%p,%lu,%s,%s: stub\n", logon_id, package_id,
            debugstr_as(primary_key), debugstr_as(credentials) );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS NTAPI lsa_GetCredentials( LUID *logon_id, ULONG package_id,
        ULONG *context, BOOLEAN retrieve_all, LSA_STRING *primary_key,
        ULONG *primary_key_len, LSA_STRING *credentials )
{
    FIXME( "%p,%#lx,%p,%d,%p,%p,%p: stub\n", logon_id, package_id, context,
            retrieve_all, primary_key, primary_key_len, credentials );
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS NTAPI lsa_DeleteCredential( LUID *logon_id,
        ULONG package_id, LSA_STRING *primary_key )
{
    FIXME( "%p,%#lx,%s: stub\n", logon_id, package_id, debugstr_as(primary_key) );
    return STATUS_NOT_IMPLEMENTED;
}

static void * NTAPI lsa_AllocateLsaHeap( ULONG size )
{
    TRACE( "%lu\n", size );
    return malloc( size );
}

static void NTAPI lsa_FreeLsaHeap( void *p )
{
    TRACE( "%p\n", p );
    free( p );
}

static NTSTATUS NTAPI lsa_AllocateClientBuffer( PLSA_CLIENT_REQUEST req, ULONG size, void **p )
{
    SECPKG_CALL_INFO *call_info = get_call_info();
    NTSTATUS status;
    HANDLE process;
    CLIENT_ID cid;
    SIZE_T s = size;

    TRACE("%p,%lu,%p\n", req, size, p);

    if (!call_info) return STATUS_NO_MEMORY;
    if (call_info->Attributes & SECPKG_CALL_IN_PROC)
    {
        *p = malloc(size);
        return *p ? STATUS_SUCCESS : STATUS_NO_MEMORY;
    }

    cid.UniqueProcess = ULongToHandle(call_info->ProcessId);
    cid.UniqueThread  = 0;
    status = NtOpenProcess(&process, PROCESS_VM_OPERATION, NULL, &cid);
    if (status) return status;

    *p = NULL;
    status = NtAllocateVirtualMemory(process, p, 0, &s, MEM_COMMIT, PAGE_READWRITE);
    CloseHandle(process);
    return status;
}

static NTSTATUS NTAPI lsa_FreeClientBuffer( PLSA_CLIENT_REQUEST req, void *p )
{
    SECPKG_CALL_INFO *call_info = get_call_info();
    NTSTATUS status;
    HANDLE process;
    CLIENT_ID cid;
    SIZE_T size = 0;

    TRACE("%p,%p\n", req, p);

    if (!call_info) return STATUS_NO_MEMORY;
    if (call_info->Attributes & SECPKG_CALL_IN_PROC)
    {
        free(p);
        return STATUS_SUCCESS;
    }

    cid.UniqueProcess = ULongToHandle(call_info->ProcessId);
    cid.UniqueThread  = 0;
    status = NtOpenProcess(&process, PROCESS_VM_OPERATION, NULL, &cid);
    if (status) return status;

    status = NtFreeVirtualMemory(process, p, &size, MEM_RELEASE);
    CloseHandle(process);
    return status;
}

static NTSTATUS NTAPI lsa_CopyToClientBuffer( PLSA_CLIENT_REQUEST req, ULONG size, void *client, void *buf )
{
    SECPKG_CALL_INFO *call_info = get_call_info();
    NTSTATUS status;
    HANDLE process;
    CLIENT_ID cid;

    TRACE("%p,%lu,%p,%p\n", req, size, client, buf);

    if (!call_info) return STATUS_NO_MEMORY;
    if (call_info->Attributes & SECPKG_CALL_IN_PROC)
    {
        memcpy(client, buf, size);
        return STATUS_SUCCESS;
    }

    cid.UniqueProcess = ULongToHandle(call_info->ProcessId);
    cid.UniqueThread  = 0;
    status = NtOpenProcess(&process, PROCESS_VM_WRITE, NULL, &cid);
    if (status) return status;

    status = NtWriteVirtualMemory(process, client, buf, size, NULL);
    CloseHandle(process);
    return status;
}

static NTSTATUS NTAPI lsa_CopyFromClientBuffer( PLSA_CLIENT_REQUEST req, ULONG size, void *buf, void *client )
{
    SECPKG_CALL_INFO *call_info = get_call_info();
    NTSTATUS status;
    HANDLE process;
    CLIENT_ID cid;

    TRACE("%p,%lu,%p,%p\n", req, size, buf, client);

    if (!call_info) return STATUS_NO_MEMORY;
    if (call_info->Attributes & SECPKG_CALL_IN_PROC)
    {
        memcpy(buf, client, size);
        return STATUS_SUCCESS;
    }

    cid.UniqueProcess = ULongToHandle(call_info->ProcessId);
    cid.UniqueThread  = 0;
    status = NtOpenProcess(&process, PROCESS_VM_READ, NULL, &cid);
    if (status) return status;

    status = NtReadVirtualMemory(process, client, buf, size, NULL);
    CloseHandle(process);
    return status;
}

static NTSTATUS NTAPI lsa_MapBuffer( SecBuffer *in, SecBuffer *out )
{
    SecBuffer tmp;
    NTSTATUS status;

    if (in->BufferType & SECBUFFER_UNMAPPED)
    {
        if (out != in) *out = *in;
        return SEC_E_OK;
    }

    tmp.pvBuffer = lsa_AllocateLsaHeap( in->cbBuffer );
    if (!tmp.pvBuffer) return SEC_E_INSUFFICIENT_MEMORY;
    status = lsa_CopyFromClientBuffer( NULL, in->cbBuffer, tmp.pvBuffer, in->pvBuffer );
    if (status)
    {
        lsa_FreeLsaHeap( tmp.pvBuffer );
        return SEC_E_INTERNAL_ERROR;
    }
    tmp.cbBuffer = in->cbBuffer;
    tmp.BufferType = in->BufferType | SECBUFFER_UNMAPPED;
    *out = tmp;
    return SEC_E_OK;
}

static BOOLEAN NTAPI lsa_GetCallInfo( SECPKG_CALL_INFO *info )
{
    SECPKG_CALL_INFO *call_info = get_call_info();

    if (!call_info || !call_info->ProcessId) return FALSE;
    *info = *call_info;
    return TRUE;
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

static LSA_SECPKG_FUNCTION_TABLE lsa_functions =
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
    lsa_CopyFromClientBuffer,
    NULL, /* ImpersonateClient */
    NULL, /* UnloadPackage */
    NULL, /* DuplicateHandle */
    NULL, /* SaveSupplementalCredentials */
    NULL, /* CreateThread */
    NULL, /* GetClientInfo */
    NULL, /* RegisterNotification */
    NULL, /* CancelNotification */
    lsa_MapBuffer,
    NULL, /* CreateToken */
    NULL, /* AuditLogon */
    NULL, /* CallPackage */
    NULL, /* FreeReturnBuffer */
    lsa_GetCallInfo,
    NULL, /* CallPackageEx */
    NULL, /* CreateSharedMemory */
    NULL, /* AllocateSharedMemory */
    NULL, /* FreeSharedMemory */
    NULL, /* DeleteSharedMemory */
    NULL, /* OpenSamUser */
    NULL, /* GetUserCredentials */
    NULL, /* GetUserAuthData */
    NULL, /* CloseSamUser */
    NULL, /* ConvertAuthDataToToken */
    NULL, /* ClientCallback */
    NULL, /* UpdateCredentials */
    NULL, /* GetAuthDataForUser */
    NULL, /* CrackSingleName */
    NULL, /* AuditAccountLogon */
    NULL, /* CallPackagePassthrough */
};

static BOOL init_package( const WCHAR *module, SpLsaModeInitializeFn init )
{
    SECPKG_FUNCTION_TABLE *tables;
    ULONG api_version, count, i;
    SecPkgInfoW info;
    LSA_STRING *name;
    BOOL ret = FALSE;

    if (!init) return FALSE;
    if (init( SECPKG_INTERFACE_VERSION, &api_version, &tables, &count ))
        return FALSE;

    if (!packages_size)
    {
        packages = malloc( sizeof(*packages) * max(count, 8) );
        if (!packages) return FALSE;
        packages_size = max( count, 8 );
    }
    else if (packages_count + count > packages_size)
    {
        struct package *new_packages;

        new_packages = realloc( packages, sizeof(*packages) *
                max(packages_size * 2, packages_count + count) );
        if (!new_packages) return FALSE;
        packages = new_packages;
        packages_size = max( packages_size * 2, packages_count + count );
    }

    for (i = 0; i < count; i++)
    {
        init_call_info( 0, GetCurrentThreadId() );
        if (tables[i].InitializePackage( packages_count, &lsa_dispatch, NULL, NULL, &name ))
            continue;

        TRACE( "name %s, version %#lx, api table %p\n", debugstr_as(name), api_version, &tables[i] );
        lsa_FreeLsaHeap( name );

        if (tables[i].Initialize( packages_count, NULL /* FIXME: params */, &lsa_functions ))
            continue;
        if (tables[i].GetInfo( &info )) continue;

        wcscpy( packages[packages_count].module, module );
        packages[packages_count].table_no = i;
        packages[packages_count].funcs = tables + i;
        packages[packages_count].info = info;
        packages_count++;
        ret = TRUE;
    }

    return ret;
}

void load_auth_packages( void )
{
    DWORD err, i;
    HKEY root;

    tls_index = TlsAlloc();

    /* "Negotiate" has package id 0, .Net depends on this. */
    init_package( L"", nego_SpLsaModeInitialize );

    err = RegOpenKeyExW( HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\Lsa", 0, KEY_READ, &root );
    if (err != ERROR_SUCCESS) return;

    for (i = 0;; i++)
    {
        WCHAR name[MAX_PATH];
        SpLsaModeInitializeFn init;
        HMODULE hmod;

        err = RegEnumKeyW( root, i, name, MAX_PATH );
        if (err == ERROR_NO_MORE_ITEMS) break;
        if (err != ERROR_SUCCESS) continue;

        hmod = LoadLibraryW( name );
        if (!hmod) continue;
        init = (void *)GetProcAddress( hmod, "SpLsaModeInitialize" );
        if (!init || !init_package( name, init ))
            FreeLibrary( hmod );
    }

    RegCloseKey( root );
}

NTSTATUS __cdecl get_packages( handle_t binding, ULONG *count, package_info **out )
{
    ULONG i;

    *count = 0;
    *out = MIDL_user_allocate( sizeof(*(*out)) * packages_count );
    if (!*out) return SEC_E_INSUFFICIENT_MEMORY;

    *count = packages_count;
    for (i = 0; i < packages_count; i++)
    {
        (*out)[i].module_name = packages[i].module[0] ? packages[i].module : NULL;
        (*out)[i].table_no = packages[i].table_no;
        (*out)[i].info = packages[i].info;
    }
    return SEC_E_OK;
}

NTSTATUS __cdecl call_package_untrusted( handle_t binding, DWORD thread_id,
        ULONG64 handle, ULONG package_id, BYTE *in_buf, void *client_buf_base,
        ULONG in_buf_len, void **out_buf, ULONG *out_buf_len, NTSTATUS *prot_status )
{
    NTSTATUS status;

    if (package_id >= packages_count)
        return STATUS_NO_SUCH_PACKAGE;
    if (!packages[package_id].funcs->CallPackageUntrusted)
        return SEC_E_UNSUPPORTED_FUNCTION;

    init_call_info( binding, thread_id );
    status = packages[package_id].funcs->CallPackageUntrusted( NULL /* FIXME*/, in_buf,
            client_buf_base, in_buf_len, out_buf, out_buf_len, prot_status );
    return status;
}

NTSTATUS __cdecl acquire_credentials_handle( handle_t binding, DWORD thread_id,
        WCHAR *principal, WCHAR *package_name, ULONG cred_use, LUID *logon_id,
        void *auth_data, void *get_key_fn, void *get_key_arg,
        ULONG *package_id, ULONG64 *cred_handle, TimeStamp *expiry )
{
    LSA_SEC_HANDLE lsa_credential;
    UNICODE_STRING principal_us;
    struct package *package;
    NTSTATUS status;

    if (get_key_fn || get_key_arg)
        FIXME("unsupported get_key_* arguments\n");

    if (!(package = lsa_lookup_package( package_name )))
        return SEC_E_SECPKG_NOT_FOUND;
    if (!package->funcs->SpAcquireCredentialsHandle)
        return SEC_E_UNSUPPORTED_FUNCTION;

    if (principal)
        RtlInitUnicodeString(&principal_us, principal);

    init_call_info( binding, thread_id );
    status = package->funcs->SpAcquireCredentialsHandle( principal ? &principal_us : NULL,
            cred_use, logon_id, auth_data, NULL, NULL, &lsa_credential, expiry );
    if (!status)
    {
        *cred_handle = lsa_credential;
        *package_id = package - packages;
    }
    return status;
}

NTSTATUS __cdecl free_credentials_handle( handle_t binding,
        DWORD thread_id, ULONG package_id, ULONG64 handle )
{
    if (package_id >= packages_count)
        return STATUS_NO_SUCH_PACKAGE;
    if (!packages[package_id].funcs->FreeCredentialsHandle)
        return SEC_E_UNSUPPORTED_FUNCTION;

    init_call_info( binding, thread_id );
    return packages[package_id].funcs->FreeCredentialsHandle( handle );
}

NTSTATUS __cdecl query_credentials_attr( handle_t binding, DWORD thread_id,
        ULONG package_id, ULONG64 handle, ULONG attr, void *buf )
{
    if (package_id >= packages_count)
        return STATUS_NO_SUCH_PACKAGE;
    if (!packages[package_id].funcs->SpQueryCredentialsAttributes)
        return SEC_E_UNSUPPORTED_FUNCTION;

    init_call_info( binding, thread_id );
    return packages[package_id].funcs->SpQueryCredentialsAttributes(
            handle, attr, buf );
}

static void init_sec_buffers( BOOL alloc, SecBufferDesc *desc, void **client_bufs )
{
    ULONG i;

    if (!desc) return;

    for (i = 0; i < desc->cBuffers; i++)
    {
        if (alloc) desc->pBuffers[i].pvBuffer = NULL;
        client_bufs[i] = desc->pBuffers[i].pvBuffer;
    }
}

static NTSTATUS remap_buffer( SecBuffer *buf, void *client_buf )
{
    NTSTATUS status = SEC_E_OK;

    if (!client_buf && buf->pvBuffer)
    {
        status = lsa_AllocateClientBuffer( NULL, buf->cbBuffer, &client_buf );
    }
    else if (!(buf->BufferType & SECBUFFER_UNMAPPED))
    {
        return SEC_E_OK;
    }

    if (!status)
        status = lsa_CopyToClientBuffer( NULL, buf->cbBuffer, client_buf, buf->pvBuffer );
    lsa_FreeLsaHeap( buf->pvBuffer );

    buf->BufferType &= ~SECBUFFER_UNMAPPED;
    buf->pvBuffer = client_buf;
    return status;
}

NTSTATUS __cdecl initialize_security_context( handle_t binding, DWORD thread_id,
        ULONG package_id, ULONG64 cred_handle, ULONG64 ctx_handle, WCHAR *target,
        ULONG context_req, ULONG data_rep, SecBufferDesc *input,
        ULONG64 *new_ctx_handle, SecBufferDesc *output, ULONG *context_attr,
        TimeStamp *expiry, BOOLEAN *mapped_ctx, SecBuffer *ctx_data )
{
    CLIENT_PTR output_ptrs[MAX_SEC_BUFFERS];
    LSA_SEC_HANDLE lsa_ctx_handle;
    UNICODE_STRING target_us;
    NTSTATUS status, status2;
    UINT i;

    if (package_id >= packages_count)
        return STATUS_NO_SUCH_PACKAGE;
    if (!packages[package_id].funcs->InitLsaModeContext)
        return SEC_E_UNSUPPORTED_FUNCTION;

    if (target)
        RtlInitUnicodeString(&target_us, target);

    init_call_info( binding, thread_id );
    init_sec_buffers( context_req & ISC_REQ_ALLOCATE_MEMORY, output, output_ptrs );

    status = packages[package_id].funcs->InitLsaModeContext( cred_handle, ctx_handle,
            target ? &target_us : NULL, context_req, data_rep, input,
            &lsa_ctx_handle, output, context_attr, expiry, mapped_ctx, ctx_data );
    for (i = 0; input && i < input->cBuffers; i++)
    {
        if (input->pBuffers[i].BufferType & SECBUFFER_UNMAPPED)
            lsa_FreeLsaHeap( input->pBuffers[i].pvBuffer );
    }
    if (status != SEC_E_OK && status != SEC_I_CONTINUE_NEEDED)
    {
        for (i = 0; output && i < output->cBuffers; i++)
        {
            if (output->pBuffers[i].BufferType & SECBUFFER_UNMAPPED)
            {
                lsa_FreeLsaHeap( output->pBuffers[i].pvBuffer );
                output->pBuffers[i].pvBuffer = output_ptrs[i];
            }
        }
        return status;
    }

    for (i = 0; output && i < output->cBuffers; i++)
    {
        status2 = remap_buffer( output->pBuffers + i, output_ptrs[i] );
        if (status2) status = status2;
    }
    status2 = remap_buffer( ctx_data, NULL );
    if (status2) status = status2;

    if (status != SEC_E_OK && status != SEC_I_CONTINUE_NEEDED)
    {
        if (packages[package_id].funcs->DeleteContext)
            packages[package_id].funcs->DeleteContext( lsa_ctx_handle );
    }
    else
    {
        *new_ctx_handle = lsa_ctx_handle;
    }
    return status;
}

NTSTATUS __cdecl accept_security_context( handle_t binding, DWORD thread_id,
        ULONG package_id, ULONG64 cred_handle, ULONG64 ctx_handle,
        SecBufferDesc *input, ULONG context_req, ULONG data_rep,
        ULONG64 *new_ctx_handle, SecBufferDesc *output, ULONG *context_attr,
        TimeStamp *expiry, BOOLEAN *mapped_ctx, SecBuffer *ctx_data )
{
    CLIENT_PTR output_ptrs[MAX_SEC_BUFFERS];
    LSA_SEC_HANDLE lsa_ctx_handle;
    NTSTATUS status, status2;
    UINT i;

    if (package_id >= packages_count)
        return STATUS_NO_SUCH_PACKAGE;
    if (!packages[package_id].funcs->AcceptLsaModeContext)
        return SEC_E_UNSUPPORTED_FUNCTION;

    init_call_info( binding, thread_id );
    init_sec_buffers( context_req & ASC_REQ_ALLOCATE_MEMORY, output, output_ptrs );

    status = packages[package_id].funcs->AcceptLsaModeContext( cred_handle,
            ctx_handle, input, context_req, data_rep, &lsa_ctx_handle, output,
            context_attr, expiry, mapped_ctx, ctx_data );
    for (i = 0; input && i < input->cBuffers; i++)
    {
        if (input->pBuffers[i].BufferType & SECBUFFER_UNMAPPED)
            lsa_FreeLsaHeap( input->pBuffers[i].pvBuffer );
    }
    if (status != SEC_E_OK && status != SEC_I_CONTINUE_NEEDED)
    {
        for (i = 0; output && i < output->cBuffers; i++)
        {
            if (output->pBuffers[i].BufferType & SECBUFFER_UNMAPPED)
            {
                lsa_FreeLsaHeap( output->pBuffers[i].pvBuffer );
                output->pBuffers[i].pvBuffer = output_ptrs[i];
            }
        }
        return status;
    }

    for (i = 0; output && i < output->cBuffers; i++)
    {
        status2 = remap_buffer( output->pBuffers + i, output_ptrs[i] );
        if (status2) status = status2;
    }
    status2 = remap_buffer( ctx_data, NULL );
    if (status2) status = status2;

    if (status != SEC_E_OK && status != SEC_I_CONTINUE_NEEDED)
    {
        if (packages[package_id].funcs->DeleteContext)
            packages[package_id].funcs->DeleteContext( lsa_ctx_handle );
    }
    else
    {
        *new_ctx_handle = lsa_ctx_handle;
    }
    return status;
}

NTSTATUS __cdecl delete_security_context( handle_t binding,
        DWORD thread_id, ULONG package_id, ULONG64 handle )
{
    if (package_id >= packages_count)
        return STATUS_NO_SUCH_PACKAGE;
    if (!packages[package_id].funcs->DeleteContext)
        return SEC_E_UNSUPPORTED_FUNCTION;

    init_call_info( binding, thread_id );
    return packages[package_id].funcs->DeleteContext( handle );
}

NTSTATUS __cdecl query_context_attr( handle_t binding, DWORD thread_id,
        ULONG package_id, ULONG64 handle, ULONG attr, void *buf )
{
    if (package_id >= packages_count)
        return STATUS_NO_SUCH_PACKAGE;
    if (!packages[package_id].funcs->SpQueryContextAttributes)
        return SEC_E_UNSUPPORTED_FUNCTION;

    init_call_info( binding, thread_id );
    return packages[package_id].funcs->SpQueryContextAttributes(
            handle, attr, (void *)buf );
}
