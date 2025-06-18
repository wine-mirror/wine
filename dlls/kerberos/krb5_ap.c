/*
 * Copyright 2017 Dmitry Timoshkov
 * Copyright 2017 George Popoff
 * Copyright 2008 Robert Shearman for CodeWeavers
 * Copyright 2017 Hans Leidekker for CodeWeavers
 *
 * Kerberos5 Authentication Package
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
#include "winnls.h"
#include "rpc.h"
#include "sspi.h"
#include "ntsecapi.h"
#include "ntsecpkg.h"
#include "winternl.h"

#include "wine/debug.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(kerberos);

#define KERBEROS_CAPS \
    ( SECPKG_FLAG_INTEGRITY \
    | SECPKG_FLAG_PRIVACY \
    | SECPKG_FLAG_TOKEN_ONLY \
    | SECPKG_FLAG_DATAGRAM \
    | SECPKG_FLAG_CONNECTION \
    | SECPKG_FLAG_MULTI_REQUIRED \
    | SECPKG_FLAG_EXTENDED_ERROR \
    | SECPKG_FLAG_IMPERSONATION \
    | SECPKG_FLAG_ACCEPT_WIN32_NAME \
    | SECPKG_FLAG_NEGOTIABLE \
    | SECPKG_FLAG_GSS_COMPATIBLE \
    | SECPKG_FLAG_LOGON \
    | SECPKG_FLAG_MUTUAL_AUTH \
    | SECPKG_FLAG_DELEGATION \
    | SECPKG_FLAG_READONLY_WITH_CHECKSUM \
    | SECPKG_FLAG_RESTRICTED_TOKENS \
    | SECPKG_FLAG_APPCONTAINER_CHECKS)

static WCHAR kerberos_name_W[] = L"Kerberos";
static WCHAR kerberos_comment_W[] = L"Microsoft Kerberos V1.0";
static const SecPkgInfoW infoW =
{
    KERBEROS_CAPS,
    1,
    RPC_C_AUTHN_GSS_KERBEROS,
    KERBEROS_MAX_BUF,
    kerberos_name_W,
    kerberos_comment_W
};

static ULONG kerberos_package_id;
static LSA_DISPATCH_TABLE lsa_dispatch;

struct cred_handle
{
    UINT64 handle;
};

struct context_handle
{
    UINT64 handle;
};

static LSA_SEC_HANDLE create_context_handle( struct context_handle *ctx, UINT64 new_context )
{
    UINT64 context = ctx ? ctx->handle : 0;
    if (new_context && new_context != context)
    {
        struct context_handle *new_ctx = malloc(sizeof(*new_ctx));
        new_ctx->handle = new_context;
        return (LSA_SEC_HANDLE)new_ctx;
    }
    else
        return (LSA_SEC_HANDLE)ctx;
}

static int get_buffer_index( const SecBufferDesc *desc, DWORD type )
{
    UINT i;
    if (!desc) return -1;
    for (i = 0; i < desc->cBuffers; i++)
    {
        if (desc->pBuffers[i].BufferType == type) return i;
    }
    return -1;
}

static const char *debugstr_us( const UNICODE_STRING *us )
{
    if (!us) return "<null>";
    return debugstr_wn( us->Buffer, us->Length / sizeof(WCHAR) );
}

static void expiry_to_timestamp( ULONG expiry, TimeStamp *timestamp )
{
    LARGE_INTEGER time;

    if (!timestamp) return;
    NtQuerySystemTime( &time );
    RtlSystemTimeToLocalTime( &time, &time );
    time.QuadPart += expiry * (ULONGLONG)10000000;
    timestamp->LowPart  = time.QuadPart;
    timestamp->HighPart = time.QuadPart >> 32;
}

static NTSTATUS NTAPI kerberos_LsaApInitializePackage(ULONG package_id, PLSA_DISPATCH_TABLE dispatch,
    PLSA_STRING database, PLSA_STRING confidentiality, PLSA_STRING *package_name)
{
    char *kerberos_name;

    if (!__wine_unixlib_handle)
    {
        if (__wine_init_unix_call() || KRB5_CALL( process_attach, NULL ))
            ERR( "no Kerberos support, expect problems\n" );
    }

    kerberos_package_id = package_id;
    lsa_dispatch = *dispatch;

    kerberos_name = lsa_dispatch.AllocateLsaHeap(sizeof(MICROSOFT_KERBEROS_NAME_A));
    if (!kerberos_name) return STATUS_NO_MEMORY;

    memcpy(kerberos_name, MICROSOFT_KERBEROS_NAME_A, sizeof(MICROSOFT_KERBEROS_NAME_A));

    *package_name = lsa_dispatch.AllocateLsaHeap(sizeof(**package_name));
    if (!*package_name)
    {
        lsa_dispatch.FreeLsaHeap(kerberos_name);
        return STATUS_NO_MEMORY;
    }

    RtlInitString(*package_name, kerberos_name);

    return STATUS_SUCCESS;
}

static NTSTATUS copy_to_client( PLSA_CLIENT_REQUEST lsa_req, KERB_QUERY_TKT_CACHE_EX_RESPONSE *resp,
                                void **out, ULONG size )
{
    NTSTATUS status;
    ULONG i;
    char *client_str;
    KERB_QUERY_TKT_CACHE_RESPONSE *client_resp;

    status = lsa_dispatch.AllocateClientBuffer( lsa_req, size, out );
    if (status != STATUS_SUCCESS) return status;

    client_resp = *out;
    status = lsa_dispatch.CopyToClientBuffer(lsa_req, offsetof(KERB_QUERY_TKT_CACHE_RESPONSE, Tickets),
                                             client_resp, resp);
    if (status != STATUS_SUCCESS) goto fail;

    client_str = (char *)&client_resp->Tickets[resp->CountOfTickets];

    for (i = 0; i < resp->CountOfTickets; i++)
    {
        KERB_TICKET_CACHE_INFO ticket = {
            .ServerName = resp->Tickets[i].ServerName,
            .RealmName =  resp->Tickets[i].ServerRealm,
            .StartTime = resp->Tickets[i].StartTime,
            .EndTime = resp->Tickets[i].EndTime,
            .RenewTime = resp->Tickets[i].RenewTime,
            .EncryptionType = resp->Tickets[i].EncryptionType,
            .TicketFlags = resp->Tickets[i].TicketFlags,
        };

        RtlSecondsSince1970ToTime( resp->Tickets[i].StartTime.QuadPart, &ticket.StartTime );
        RtlSecondsSince1970ToTime( resp->Tickets[i].EndTime.QuadPart, &ticket.EndTime );
        RtlSecondsSince1970ToTime( resp->Tickets[i].RenewTime.QuadPart, &ticket.RenewTime );

        status = lsa_dispatch.CopyToClientBuffer(lsa_req, ticket.RealmName.MaximumLength,
                                                 client_str, ticket.RealmName.Buffer);
        if (status != STATUS_SUCCESS) goto fail;
        ticket.RealmName.Buffer = (WCHAR *)client_str;
        client_str += ticket.RealmName.MaximumLength;

        status = lsa_dispatch.CopyToClientBuffer(lsa_req, ticket.ServerName.MaximumLength,
                                                 client_str, ticket.ServerName.Buffer);
        if (status != STATUS_SUCCESS) goto fail;
        ticket.ServerName.Buffer = (WCHAR *)client_str;
        client_str += ticket.ServerName.MaximumLength;

        status = lsa_dispatch.CopyToClientBuffer(lsa_req, sizeof(ticket), &client_resp->Tickets[i], &ticket);
        if (status != STATUS_SUCCESS) goto fail;
    }
    return STATUS_SUCCESS;

fail:
    lsa_dispatch.FreeClientBuffer(lsa_req, client_resp);
    return status;
}

static NTSTATUS copy_to_client_ex( PLSA_CLIENT_REQUEST lsa_req, KERB_QUERY_TKT_CACHE_EX_RESPONSE *resp,
                                void **out, ULONG size )
{
    NTSTATUS status;
    ULONG i;
    char *client_str;
    KERB_QUERY_TKT_CACHE_EX_RESPONSE *client_resp;

    status = lsa_dispatch.AllocateClientBuffer( lsa_req, size, out );
    if (status != STATUS_SUCCESS) return status;

    client_resp = *out;
    status = lsa_dispatch.CopyToClientBuffer(lsa_req, offsetof(KERB_QUERY_TKT_CACHE_EX_RESPONSE, Tickets),
                                             client_resp, resp);
    if (status != STATUS_SUCCESS) goto fail;

    client_str = (char *)&client_resp->Tickets[resp->CountOfTickets];

    for (i = 0; i < resp->CountOfTickets; i++)
    {
        KERB_TICKET_CACHE_INFO_EX ticket = resp->Tickets[i];

        RtlSecondsSince1970ToTime( resp->Tickets[i].StartTime.QuadPart, &ticket.StartTime );
        RtlSecondsSince1970ToTime( resp->Tickets[i].EndTime.QuadPart, &ticket.EndTime );
        RtlSecondsSince1970ToTime( resp->Tickets[i].RenewTime.QuadPart, &ticket.RenewTime );

        status = lsa_dispatch.CopyToClientBuffer(lsa_req, ticket.ClientRealm.MaximumLength,
                                                 client_str, ticket.ClientRealm.Buffer);
        if (status != STATUS_SUCCESS) goto fail;
        ticket.ClientRealm.Buffer = (WCHAR *)client_str;
        client_str += ticket.ClientRealm.MaximumLength;

        status = lsa_dispatch.CopyToClientBuffer(lsa_req, ticket.ClientName.MaximumLength,
                                                 client_str, ticket.ClientName.Buffer);
        if (status != STATUS_SUCCESS) goto fail;
        ticket.ClientName.Buffer = (WCHAR *)client_str;
        client_str += ticket.ClientName.MaximumLength;

        status = lsa_dispatch.CopyToClientBuffer(lsa_req, ticket.ServerRealm.MaximumLength,
                                                 client_str, ticket.ServerRealm.Buffer);
        if (status != STATUS_SUCCESS) goto fail;
        ticket.ServerRealm.Buffer = (WCHAR *)client_str;
        client_str += ticket.ServerRealm.MaximumLength;

        status = lsa_dispatch.CopyToClientBuffer(lsa_req, ticket.ServerName.MaximumLength,
                                                 client_str, ticket.ServerName.Buffer);
        if (status != STATUS_SUCCESS) goto fail;
        ticket.ServerName.Buffer = (WCHAR *)client_str;
        client_str += ticket.ServerName.MaximumLength;

        status = lsa_dispatch.CopyToClientBuffer(lsa_req, sizeof(ticket), &client_resp->Tickets[i], &ticket);
        if (status != STATUS_SUCCESS) goto fail;
    }
    return STATUS_SUCCESS;

fail:
    lsa_dispatch.FreeClientBuffer(lsa_req, client_resp);
    return status;
}

static NTSTATUS NTAPI kerberos_LsaApCallPackageUntrusted(PLSA_CLIENT_REQUEST req, void *in_buf,
    void *client_buf_base, ULONG in_buf_len, void **out_buf, ULONG *out_buf_len, NTSTATUS *ret_status)
{
    KERB_PROTOCOL_MESSAGE_TYPE msg;

    TRACE("%p, %p, %p, %lu, %p, %p, %p\n", req, in_buf, client_buf_base, in_buf_len, out_buf, out_buf_len, ret_status);

    if (!in_buf || in_buf_len < sizeof(msg)) return STATUS_INVALID_PARAMETER;

    msg = *(KERB_PROTOCOL_MESSAGE_TYPE *)in_buf;
    switch (msg)
    {
    case KerbQueryTicketCacheMessage:
    {
        KERB_QUERY_TKT_CACHE_REQUEST *query = (KERB_QUERY_TKT_CACHE_REQUEST *)in_buf;
        NTSTATUS status;

        if (!in_buf || in_buf_len != sizeof(*query) || !out_buf || !out_buf_len) return STATUS_INVALID_PARAMETER;
        if (query->LogonId.HighPart || query->LogonId.LowPart) return STATUS_ACCESS_DENIED;

        *out_buf_len = 1024;
        for (;;)
        {
            KERB_QUERY_TKT_CACHE_EX_RESPONSE *resp = malloc( *out_buf_len );
            struct query_ticket_cache_params params = { resp, out_buf_len };
            if (!resp)
            {
                status = STATUS_NO_MEMORY;
                break;
            }
            status = KRB5_CALL( query_ticket_cache, &params );
            if (status == STATUS_SUCCESS) status = copy_to_client( req, resp, out_buf, *out_buf_len );
            free( resp );
            if (status != STATUS_BUFFER_TOO_SMALL) break;
        }
        *ret_status = status;
        break;
    }
    case KerbQueryTicketCacheExMessage:
    {
        KERB_QUERY_TKT_CACHE_REQUEST *query = (KERB_QUERY_TKT_CACHE_REQUEST *)in_buf;
        NTSTATUS status;

        if (!in_buf || in_buf_len != sizeof(*query) || !out_buf || !out_buf_len) return STATUS_INVALID_PARAMETER;
        if (query->LogonId.HighPart || query->LogonId.LowPart) return STATUS_ACCESS_DENIED;

        *out_buf_len = 1024;
        for (;;)
        {
            KERB_QUERY_TKT_CACHE_EX_RESPONSE *resp = malloc( *out_buf_len );
            struct query_ticket_cache_params params = { resp, out_buf_len };
            if (!resp)
            {
                status = STATUS_NO_MEMORY;
                break;
            }
            status = KRB5_CALL( query_ticket_cache, &params );
            if (status == STATUS_SUCCESS) status = copy_to_client_ex( req, resp, out_buf, *out_buf_len );
            free( resp );
            if (status != STATUS_BUFFER_TOO_SMALL) break;
        }
        *ret_status = status;
        break;
    }
    case KerbRetrieveTicketMessage:
        FIXME("KerbRetrieveTicketMessage stub\n");
        *ret_status = STATUS_NOT_IMPLEMENTED;
        break;

    case KerbPurgeTicketCacheMessage:
        FIXME("KerbPurgeTicketCacheMessage stub\n");
        *ret_status = STATUS_NOT_IMPLEMENTED;
        break;

    default: /* All other requests should call LsaApCallPackage */
        WARN("%u => access denied\n", msg);
        *ret_status = STATUS_ACCESS_DENIED;
        break;
    }

    return *ret_status;
}

static NTSTATUS NTAPI kerberos_SpGetInfo(SecPkgInfoW *info)
{
    TRACE("%p\n", info);

    /* LSA will make a copy before forwarding the structure, so
     * it's safe to put pointers to dynamic or constant data there.
     */
    *info = infoW;

    return STATUS_SUCCESS;
}

static char *get_str_unixcp( const UNICODE_STRING *str )
{
    char *ret;
    int len = WideCharToMultiByte( CP_UNIXCP, 0, str->Buffer, str->Length / sizeof(WCHAR), NULL, 0, NULL, NULL );
    if (!(ret = malloc( len + 1 ))) return NULL;
    WideCharToMultiByte( CP_UNIXCP, 0, str->Buffer, str->Length / sizeof(WCHAR), ret, len, NULL, NULL );
    ret[len] = 0;
    return ret;
}

static char *get_username_unixcp( const WCHAR *user, ULONG user_len, const WCHAR *domain, ULONG domain_len )
{
    int len_user, len_domain;
    char *ret;

    len_user = WideCharToMultiByte( CP_UNIXCP, 0, user, user_len, NULL, 0, NULL, NULL );
    len_domain = WideCharToMultiByte( CP_UNIXCP, 0, domain, domain_len, NULL, 0, NULL, NULL );
    if (!(ret = malloc( len_user + len_domain + 2 ))) return NULL;

    WideCharToMultiByte( CP_UNIXCP, 0, user, user_len, ret, len_user, NULL, NULL );
    ret[len_user] = '@';
    WideCharToMultiByte( CP_UNIXCP, 0, domain, domain_len, ret + len_user + 1, len_domain, NULL, NULL );
    ret[len_user + len_domain + 1] = 0;
    return ret;
}

static char *get_password_unixcp( const WCHAR *passwd, ULONG passwd_len )
{
    int len;
    char *ret;

    len = WideCharToMultiByte( CP_UNIXCP, WC_NO_BEST_FIT_CHARS, passwd, passwd_len, NULL, 0, NULL, NULL );
    if (!(ret = malloc( len + 1 ))) return NULL;
    WideCharToMultiByte( CP_UNIXCP, 0, passwd, passwd_len, ret, len, NULL, NULL );
    ret[len] = 0;
    return ret;
}

static NTSTATUS NTAPI kerberos_SpAcquireCredentialsHandle(
    UNICODE_STRING *principal_us, ULONG credential_use, LUID *logon_id, void *auth_data,
    void *get_key_fn, void *get_key_arg, LSA_SEC_HANDLE *credential, TimeStamp *expiry )
{
    char *principal = NULL, *username = NULL,  *password = NULL;
    SEC_WINNT_AUTH_IDENTITY_W *id = auth_data;
    NTSTATUS status = SEC_E_INSUFFICIENT_MEMORY;
    struct cred_handle *cred_handle;
    ULONG exptime;

    TRACE( "%s, %#lx, %p, %p, %p, %p, %p, %p\n", debugstr_us(principal_us), credential_use,
           logon_id, auth_data, get_key_fn, get_key_arg, credential, expiry );

    if (principal_us && !(principal = get_str_unixcp( principal_us ))) return SEC_E_INSUFFICIENT_MEMORY;
    if (id)
    {
        if (id->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI)
        {
            FIXME( "ANSI identity not supported\n" );
            status = SEC_E_UNSUPPORTED_FUNCTION;
            goto done;
        }
        if (!(username = get_username_unixcp( id->User, id->UserLength, id->Domain, id->DomainLength ))) goto done;
        if (!(password = get_password_unixcp( id->Password, id->PasswordLength ))) goto done;
    }

    if (!(cred_handle = calloc( 1, sizeof(*cred_handle) )))
    {
        status = SEC_E_INSUFFICIENT_MEMORY;
        goto done;
    }

    {
        struct acquire_credentials_handle_params params = { principal, credential_use, username, password,
                                                            &cred_handle->handle, &exptime };
        if (!(status = KRB5_CALL( acquire_credentials_handle, &params )))
            *credential = (LSA_SEC_HANDLE)cred_handle;
        else
            free( cred_handle );
        expiry_to_timestamp( exptime, expiry );
    }

done:
    free( principal );
    free( username );
    free( password );
    return status;
}

static NTSTATUS NTAPI kerberos_SpFreeCredentialsHandle( LSA_SEC_HANDLE credential )
{
    struct cred_handle *cred_handle = (void *)credential;
    struct free_credentials_handle_params params;
    NTSTATUS status;

    TRACE( "%Ix\n", credential );

    if (!cred_handle) return SEC_E_INVALID_HANDLE;

    params.credential = cred_handle->handle;
    status = KRB5_CALL( free_credentials_handle, &params );
    free(cred_handle);
    return status;
}

static NTSTATUS NTAPI kerberos_SpInitLsaModeContext( LSA_SEC_HANDLE credential, LSA_SEC_HANDLE context,
    UNICODE_STRING *target_name, ULONG context_req, ULONG target_data_rep, SecBufferDesc *input,
    LSA_SEC_HANDLE *new_context, SecBufferDesc *output, ULONG *context_attr, TimeStamp *expiry,
    BOOLEAN *mapped_context, SecBuffer *context_data )
{
    static const ULONG supported = ISC_REQ_CONFIDENTIALITY | ISC_REQ_INTEGRITY | ISC_REQ_SEQUENCE_DETECT |
                                   ISC_REQ_REPLAY_DETECT | ISC_REQ_MUTUAL_AUTH | ISC_REQ_USE_DCE_STYLE |
                                   ISC_REQ_IDENTIFY | ISC_REQ_CONNECTION | ISC_REQ_DELEGATE | ISC_REQ_ALLOCATE_MEMORY;
    char *target = NULL;
    NTSTATUS status;
    ULONG exptime;

    TRACE( "%Ix, %Ix, %s, %#lx, %lu, %p, %p, %p, %p, %p, %p, %p\n", credential, context, debugstr_us(target_name),
           context_req, target_data_rep, input, new_context, output, context_attr, expiry,
           mapped_context, context_data );
    if (context_req & ~supported) FIXME( "flags %#lx not supported\n", context_req & ~supported );

    if (!context && !input && !credential) return SEC_E_INVALID_HANDLE;
    if (target_name && !(target = get_str_unixcp( target_name ))) return SEC_E_INSUFFICIENT_MEMORY;
    else
    {
        struct cred_handle *cred_handle = (struct cred_handle *)credential;
        struct context_handle *context_handle = (struct context_handle *)context;
        struct initialize_context_params params = { 0 };
        UINT64 new_context_handle = 0;
        int idx;

        params.credential = cred_handle ? cred_handle->handle : 0;
        params.context = context_handle ? context_handle->handle : 0;
        params.target_name = target;
        params.context_req = context_req;
        params.new_context = &new_context_handle;
        params.context_attr = context_attr;
        params.expiry = &exptime;

        idx = get_buffer_index( input, SECBUFFER_TOKEN );
        if (idx != -1)
        {
            params.input_token = input->pBuffers[idx].pvBuffer;
            params.input_token_length = input->pBuffers[idx].cbBuffer;
        }

        if ((idx = get_buffer_index( output, SECBUFFER_TOKEN )) == -1)
        {
            free( target );
            return SEC_E_INVALID_TOKEN;
        }
        if (context_req & ISC_REQ_ALLOCATE_MEMORY)
        {
            output->pBuffers[idx].pvBuffer = RtlAllocateHeap( GetProcessHeap(), 0, KERBEROS_MAX_BUF );
            if (!output->pBuffers[idx].pvBuffer)
            {
                free( target );
                return STATUS_NO_MEMORY;
            }
            output->pBuffers[idx].cbBuffer = KERBEROS_MAX_BUF;
        }
        params.output_token = output->pBuffers[idx].pvBuffer;
        params.output_token_length = &output->pBuffers[idx].cbBuffer;

        status = KRB5_CALL( initialize_context, &params );
        if (status == SEC_E_OK || status == SEC_I_CONTINUE_NEEDED)
        {
            *new_context = create_context_handle( context_handle, new_context_handle );
            if (context_attr && (context_req & ISC_REQ_ALLOCATE_MEMORY))
                *context_attr |= ISC_RET_ALLOCATED_MEMORY;

            if (status == SEC_E_OK)
            {
                *mapped_context = TRUE;
                expiry_to_timestamp( exptime, expiry );
            }
        }
        else
        {
            if (context_req & ISC_REQ_ALLOCATE_MEMORY)
                RtlFreeHeap( GetProcessHeap(), 0, output->pBuffers[idx].pvBuffer );
        }
    }
    /* FIXME: initialize context_data */
    free( target );
    return status;
}

static NTSTATUS NTAPI kerberos_SpAcceptLsaModeContext( LSA_SEC_HANDLE credential, LSA_SEC_HANDLE context,
    SecBufferDesc *input, ULONG context_req, ULONG target_data_rep, LSA_SEC_HANDLE *new_context,
    SecBufferDesc *output, ULONG *context_attr, TimeStamp *expiry, BOOLEAN *mapped_context, SecBuffer *context_data )
{
    NTSTATUS status = SEC_E_INVALID_HANDLE;
    ULONG exptime;
    int idx;

    TRACE( "%Ix, %Ix, %#lx, %lu, %p, %p, %p, %p, %p, %p, %p\n", credential, context, context_req, target_data_rep,
           input, new_context, output, context_attr, expiry, mapped_context, context_data );
    if (context_req) FIXME( "ignoring flags %#lx\n", context_req );

    if (context || input || credential)
    {
        struct cred_handle *cred_handle = (struct cred_handle *)credential;
        struct context_handle *context_handle = (struct context_handle *)context;
        struct accept_context_params params = { 0 };
        UINT64 new_context_handle = 0;

        params.credential = cred_handle ? cred_handle->handle : 0;
        params.context = context_handle ? context_handle->handle : 0;
        params.new_context = &new_context_handle;
        params.context_attr = context_attr;
        params.expiry = &exptime;

        if (input)
        {
            if ((idx = get_buffer_index( input, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;
            params.input_token  = input->pBuffers[idx].pvBuffer;
            params.input_token_length = input->pBuffers[idx].cbBuffer;
        }
        if ((idx = get_buffer_index( output, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;
        params.output_token = output->pBuffers[idx].pvBuffer;
        params.output_token_length = &output->pBuffers[idx].cbBuffer;

        /* FIXME: check if larger output buffer exists */
        status = KRB5_CALL( accept_context, &params );
        if (status == SEC_E_OK || status == SEC_I_CONTINUE_NEEDED)
            *new_context = create_context_handle( context_handle, new_context_handle );
        if (!status)
        {
            *mapped_context = TRUE;
            expiry_to_timestamp( exptime, expiry );
        }
        /* FIXME: initialize context_data */
    }
    return status;
}

static NTSTATUS NTAPI kerberos_SpDeleteContext( LSA_SEC_HANDLE context )
{
    struct context_handle *context_handle = (void *)context;
    struct delete_context_params params;
    NTSTATUS status;

    TRACE( "%Ix\n", context );

    if (!context) return SEC_E_INVALID_HANDLE;

    params.context = context_handle->handle;
    status = KRB5_CALL( delete_context, &params );
    free( context_handle );
    return status;
}

static SecPkgInfoW *build_package_info( const SecPkgInfoW *info )
{
    SecPkgInfoW *ret;
    DWORD size_name = (wcslen(info->Name) + 1) * sizeof(WCHAR);
    DWORD size_comment = (wcslen(info->Comment) + 1) * sizeof(WCHAR);

    if (!(ret = malloc( sizeof(*ret) + size_name + size_comment ))) return NULL;
    ret->fCapabilities = info->fCapabilities;
    ret->wVersion      = info->wVersion;
    ret->wRPCID        = info->wRPCID;
    ret->cbMaxToken    = info->cbMaxToken;
    ret->Name          = (SEC_WCHAR *)(ret + 1);
    memcpy( ret->Name, info->Name, size_name );
    ret->Comment       = (SEC_WCHAR *)((char *)ret->Name + size_name);
    memcpy( ret->Comment, info->Comment, size_comment );
    return ret;
}

static NTSTATUS NTAPI kerberos_SpQueryContextAttributes( LSA_SEC_HANDLE context, ULONG attribute, void *buffer )
{
    struct context_handle *context_handle = (void *)context;

    TRACE( "%Ix, %lu, %p\n", context, attribute, buffer );

    if (!context) return SEC_E_INVALID_HANDLE;

    switch (attribute)
    {
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
    X(SECPKG_ATTR_STREAM_SIZES);
    X(SECPKG_ATTR_TARGET_INFORMATION);
#undef X
    case SECPKG_ATTR_SESSION_KEY:
    {
        SecPkgContext_SessionKey key = { 128 };
        struct query_context_attributes_params params = { context_handle->handle, attribute, &key };
        NTSTATUS status;

        if (!(key.SessionKey = RtlAllocateHeap( GetProcessHeap(), 0, key.SessionKeyLength ))) return STATUS_NO_MEMORY;

        if ((status = KRB5_CALL( query_context_attributes, &params )))
        {
            RtlFreeHeap( GetProcessHeap(), 0, key.SessionKey );
            return status;
        }

        *(SecPkgContext_SessionKey *)buffer = key;
        return SEC_E_OK;
    }
    case SECPKG_ATTR_SIZES:
    {
        struct query_context_attributes_params params = { context_handle->handle, attribute, buffer };
        return KRB5_CALL( query_context_attributes, &params );
    }
    case SECPKG_ATTR_NEGOTIATION_INFO:
    {
        SecPkgContext_NegotiationInfoW *info = (SecPkgContext_NegotiationInfoW *)buffer;
        if (!(info->PackageInfo = build_package_info( &infoW ))) return SEC_E_INSUFFICIENT_MEMORY;
        info->NegotiationState = SECPKG_NEGOTIATION_COMPLETE;
        return SEC_E_OK;
    }
    default:
        FIXME( "unknown attribute %lu\n", attribute );
        break;
    }

    return SEC_E_UNSUPPORTED_FUNCTION;
}

static NTSTATUS NTAPI kerberos_SpInitialize(ULONG_PTR package_id, SECPKG_PARAMETERS *params,
    LSA_SECPKG_FUNCTION_TABLE *lsa_function_table)
{
    TRACE("%Iu, %p, %p\n", package_id, params, lsa_function_table);

    if (!__wine_unixlib_handle)
    {
        if (__wine_init_unix_call() || KRB5_CALL( process_attach, NULL ))
            WARN( "no Kerberos support\n" );
        return STATUS_UNSUCCESSFUL;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI kerberos_SpShutdown(void)
{
    TRACE("\n");
    return STATUS_SUCCESS;
}

static SECPKG_FUNCTION_TABLE kerberos_table =
{
    kerberos_LsaApInitializePackage, /* InitializePackage */
    NULL, /* LsaLogonUser */
    NULL, /* CallPackage */
    NULL, /* LogonTerminated */
    kerberos_LsaApCallPackageUntrusted, /* CallPackageUntrusted */
    NULL, /* CallPackagePassthrough */
    NULL, /* LogonUserEx */
    NULL, /* LogonUserEx2 */
    kerberos_SpInitialize,
    kerberos_SpShutdown,
    kerberos_SpGetInfo,
    NULL, /* AcceptCredentials */
    kerberos_SpAcquireCredentialsHandle,
    NULL, /* SpQueryCredentialsAttributes */
    kerberos_SpFreeCredentialsHandle,
    NULL, /* SaveCredentials */
    NULL, /* GetCredentials */
    NULL, /* DeleteCredentials */
    kerberos_SpInitLsaModeContext,
    kerberos_SpAcceptLsaModeContext,
    kerberos_SpDeleteContext,
    NULL, /* ApplyControlToken */
    NULL, /* GetUserInfo */
    NULL, /* GetExtendedInformation */
    kerberos_SpQueryContextAttributes,
    NULL, /* SpAddCredentials */
    NULL, /* SetExtendedInformation */
    NULL, /* SetContextAttributes */
    NULL, /* SetCredentialsAttributes */
    NULL, /* ChangeAccountPassword */
    NULL, /* QueryMetaData */
    NULL, /* ExchangeMetaData */
    NULL, /* GetCredUIContext */
    NULL, /* UpdateCredentials */
    NULL, /* ValidateTargetInfo */
    NULL, /* PostLogonUser */
};

NTSTATUS NTAPI SpLsaModeInitialize(ULONG lsa_version, PULONG package_version,
    PSECPKG_FUNCTION_TABLE *table, PULONG table_count)
{
    TRACE("%#lx, %p, %p, %p\n", lsa_version, package_version, table, table_count);

    *package_version = SECPKG_INTERFACE_VERSION;
    *table = &kerberos_table;
    *table_count = 1;
    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI kerberos_SpInstanceInit(ULONG version, SECPKG_DLL_FUNCTIONS *dll_function_table, void **user_functions)
{
    TRACE("%#lx, %p, %p\n", version, dll_function_table, user_functions);
    return STATUS_SUCCESS;
}

static NTSTATUS SEC_ENTRY kerberos_SpMakeSignature( LSA_SEC_HANDLE context, ULONG quality_of_protection,
    SecBufferDesc *message, ULONG message_seq_no )
{
    TRACE( "%Ix, %#lx, %p, %lu\n", context, quality_of_protection, message, message_seq_no );
    if (quality_of_protection) FIXME( "ignoring quality_of_protection %#lx\n", quality_of_protection );
    if (message_seq_no) FIXME( "ignoring message_seq_no %lu\n", message_seq_no );

    if (context)
    {
        struct context_handle *context_handle = (void *)context;
        struct make_signature_params params;
        int data_idx, token_idx;

        /* FIXME: multiple data buffers, read-only buffers */
        if ((data_idx = get_buffer_index( message, SECBUFFER_DATA )) == -1) return SEC_E_INVALID_TOKEN;
        if ((token_idx = get_buffer_index( message, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;

        params.context = context_handle->handle;
        params.data_length = message->pBuffers[data_idx].cbBuffer;
        params.data = message->pBuffers[data_idx].pvBuffer;
        params.token_length = &message->pBuffers[token_idx].cbBuffer;
        params.token = message->pBuffers[token_idx].pvBuffer;

        return KRB5_CALL( make_signature, &params );
    }
    else return SEC_E_INVALID_HANDLE;
}

static NTSTATUS NTAPI kerberos_SpVerifySignature( LSA_SEC_HANDLE context, SecBufferDesc *message,
    ULONG message_seq_no, ULONG *quality_of_protection )
{
    TRACE( "%Ix, %p, %lu, %p\n", context, message, message_seq_no, quality_of_protection );
    if (message_seq_no) FIXME( "ignoring message_seq_no %lu\n", message_seq_no );

    if (context)
    {
        struct context_handle *context_handle = (void *)context;
        struct verify_signature_params params;
        int data_idx, token_idx;

        if ((data_idx = get_buffer_index( message, SECBUFFER_DATA )) == -1) return SEC_E_INVALID_TOKEN;
        if ((token_idx = get_buffer_index( message, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;

        params.context = context_handle->handle;
        params.data_length = message->pBuffers[data_idx].cbBuffer;
        params.data = message->pBuffers[data_idx].pvBuffer;
        params.token_length = message->pBuffers[token_idx].cbBuffer;
        params.token = message->pBuffers[token_idx].pvBuffer;
        params.qop = quality_of_protection;

        return KRB5_CALL( verify_signature, &params );
    }
    else return SEC_E_INVALID_HANDLE;
}

static NTSTATUS NTAPI kerberos_SpSealMessage( LSA_SEC_HANDLE context, ULONG quality_of_protection,
    SecBufferDesc *message, ULONG message_seq_no )
{
    TRACE( "%Ix, %#lx, %p, %lu\n", context, quality_of_protection, message, message_seq_no );
    if (message_seq_no) FIXME( "ignoring message_seq_no %lu\n", message_seq_no );

    if (context)
    {
        struct context_handle *context_handle = (void *)context;
        struct seal_message_params params;
        int data_idx, token_idx;

        /* FIXME: multiple data buffers, read-only buffers */
        if ((data_idx = get_buffer_index( message, SECBUFFER_DATA )) == -1) return SEC_E_INVALID_TOKEN;
        if ((token_idx = get_buffer_index( message, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;

        params.context = context_handle->handle;
        params.data_length = message->pBuffers[data_idx].cbBuffer;
        params.data = message->pBuffers[data_idx].pvBuffer;
        params.token_length = &message->pBuffers[token_idx].cbBuffer;
        params.token = message->pBuffers[token_idx].pvBuffer;
        params.qop = quality_of_protection;

        return KRB5_CALL( seal_message, &params );
    }
    else return SEC_E_INVALID_HANDLE;
}

static NTSTATUS NTAPI kerberos_SpUnsealMessage( LSA_SEC_HANDLE context, SecBufferDesc *message,
    ULONG message_seq_no, ULONG *quality_of_protection )
{
    TRACE( "%Ix, %p, %lu, %p\n", context, message, message_seq_no, quality_of_protection );
    if (message_seq_no) FIXME( "ignoring message_seq_no %lu\n", message_seq_no );

    if (context)
    {
        struct context_handle *context_handle = (void *)context;
        struct unseal_message_params params;
        int stream_idx, data_idx, token_idx = -1;

        if ((stream_idx = get_buffer_index( message, SECBUFFER_STREAM )) == -1 &&
            (token_idx = get_buffer_index( message, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;
        if ((data_idx = get_buffer_index( message, SECBUFFER_DATA )) == -1) return SEC_E_INVALID_TOKEN;

        params.context = context_handle->handle;

        if (token_idx != -1)
        {
            params.stream_length = 0;
            params.stream = NULL;
            params.token_length = message->pBuffers[token_idx].cbBuffer;
            params.token = message->pBuffers[token_idx].pvBuffer;
        }
        else
        {
            params.stream_length = message->pBuffers[stream_idx].cbBuffer;
            params.stream = message->pBuffers[stream_idx].pvBuffer;
            params.token_length = 0;
            params.token = NULL;
        }
        params.data_length = &message->pBuffers[data_idx].cbBuffer;
        params.data = (BYTE **)&message->pBuffers[data_idx].pvBuffer;
        params.qop = quality_of_protection;

        return KRB5_CALL( unseal_message, &params );
    }
    else return SEC_E_INVALID_HANDLE;
}

static SECPKG_USER_FUNCTION_TABLE kerberos_user_table =
{
    kerberos_SpInstanceInit,
    NULL, /* SpInitUserModeContext */
    kerberos_SpMakeSignature,
    kerberos_SpVerifySignature,
    kerberos_SpSealMessage,
    kerberos_SpUnsealMessage,
    NULL, /* SpGetContextToken */
    NULL, /* SpQueryContextAttributes */
    NULL, /* SpCompleteAuthToken */
    NULL, /* SpDeleteContext */
    NULL, /* SpFormatCredentialsFn */
    NULL, /* SpMarshallSupplementalCreds */
    NULL, /* SpExportSecurityContext */
    NULL  /* SpImportSecurityContext */
};

NTSTATUS NTAPI SpUserModeInitialize(ULONG lsa_version, PULONG package_version,
    PSECPKG_USER_FUNCTION_TABLE *table, PULONG table_count)
{
    TRACE("%#lx, %p, %p, %p\n", lsa_version, package_version, table, table_count);

    *package_version = SECPKG_INTERFACE_VERSION;
    *table = &kerberos_user_table;
    *table_count = 1;
    return STATUS_SUCCESS;
}
