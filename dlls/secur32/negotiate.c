/*
 * Copyright 2005 Kai Blin
 * Copyright 2012 Hans Leidekker for CodeWeavers
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
#include "sspi.h"
#include "rpc.h"
#include "wincred.h"
#include "ntsecapi.h"
#include "ntsecpkg.h"
#include "winternl.h"

#include "wine/debug.h"
#include "secur32_priv.h"

WINE_DEFAULT_DEBUG_CHANNEL(secur32);

struct sec_handle
{
    SECPKG_FUNCTION_TABLE      *krb;
    SECPKG_FUNCTION_TABLE      *ntlm;
    LSA_SEC_HANDLE              handle_krb;
    LSA_SEC_HANDLE              handle_ntlm;
};

struct user_context_data
{
    enum
    {
        SSP_KERBEROS,
        SSP_NTLM
    } ssp;
    BOOLEAN mapped_ctx;
    /* BYTE ssp_context_data[]; */
};

struct user_ctx
{
    struct list entry;
    LSA_SEC_HANDLE handle;
    SECPKG_USER_FUNCTION_TABLE *funcs;
};

static struct list user_ctx_list = LIST_INIT(user_ctx_list);
static CRITICAL_SECTION user_ctx_cs;
static CRITICAL_SECTION_DEBUG user_ctx_debug =
{
    0, 0, &user_ctx_cs,
    { &user_ctx_debug.ProcessLocksList, &user_ctx_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": user_ctx_cs") }
};
static CRITICAL_SECTION user_ctx_cs = { &user_ctx_debug, -1, 0, 0, 0, 0 };

static LSA_SECPKG_FUNCTION_TABLE *lsa_funcs;

#define WINE_NO_CACHED_CREDENTIALS 0x10000000
#define NEGO_MAX_TOKEN 48256

static WCHAR nego_name_W[] = {'N','e','g','o','t','i','a','t','e',0};
static char nego_name_A[] = "Negotiate";
static WCHAR negotiate_comment_W[] =
    {'M','i','c','r','o','s','o','f','t',' ','P','a','c','k','a','g','e',' ',
     'N','e','g','o','t','i','a','t','o','r',0};

#define CAPS ( \
    SECPKG_FLAG_INTEGRITY  | \
    SECPKG_FLAG_PRIVACY    | \
    SECPKG_FLAG_CONNECTION | \
    SECPKG_FLAG_MULTI_REQUIRED | \
    SECPKG_FLAG_EXTENDED_ERROR | \
    SECPKG_FLAG_IMPERSONATION  | \
    SECPKG_FLAG_ACCEPT_WIN32_NAME | \
    SECPKG_FLAG_NEGOTIABLE        | \
    SECPKG_FLAG_GSS_COMPATIBLE    | \
    SECPKG_FLAG_LOGON             | \
    SECPKG_FLAG_RESTRICTED_TOKENS )

static NTSTATUS NTAPI nego_LsaApCallPackageUntrusted( PLSA_CLIENT_REQUEST req, void *in_buf,
    void *client_buf_base, ULONG in_buf_len, void **out_buf, ULONG *out_buf_len, NTSTATUS *ret_status )
{
    ULONG *MessageType;

    FIXME("%p, %p, %p, %lu, %p, %p, %p: stub\n", req, in_buf, client_buf_base, in_buf_len, out_buf, out_buf_len, ret_status);

    if (!in_buf || in_buf_len < sizeof(*MessageType) || !out_buf || !out_buf_len || !ret_status)
        return STATUS_INVALID_PARAMETER;

    MessageType = in_buf;
    switch (*MessageType)
    {
    case 1: /* NegGetCallerName */
        *ret_status = STATUS_NO_SUCH_LOGON_SESSION;
        return STATUS_SUCCESS;

    default:
        return SEC_E_UNSUPPORTED_FUNCTION;
    }
}

static NTSTATUS NTAPI nego_LsaApInitializePackage( ULONG package_id, PLSA_DISPATCH_TABLE dispatch,
    PLSA_STRING database, PLSA_STRING confidentiality, PLSA_STRING *package_name )
{
    char *name;

    name = dispatch->AllocateLsaHeap( sizeof(nego_name_A) );
    if (!name) return STATUS_NO_MEMORY;

    memcpy(name, nego_name_A, sizeof(nego_name_A));

    *package_name = dispatch->AllocateLsaHeap( sizeof(**package_name) );
    if (!*package_name)
    {
        dispatch->FreeLsaHeap( name );
        return STATUS_NO_MEMORY;
    }

    RtlInitString( *package_name, name );

    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI nego_SpInitialize( ULONG_PTR package_id, SECPKG_PARAMETERS *params,
    LSA_SECPKG_FUNCTION_TABLE *lsa_function_table )
{
    TRACE( "%Iu, %p, %p\n", package_id, params, lsa_function_table );

    lsa_funcs = lsa_function_table;
    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI nego_SpShutdown( void )
{
    TRACE( "\n" );
    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI nego_SpGetInfo( SecPkgInfoW *info )
{
    const SecPkgInfoW infoW = {CAPS, 1, RPC_C_AUTHN_GSS_NEGOTIATE, NEGO_MAX_TOKEN,
                               nego_name_W, negotiate_comment_W};

    TRACE( "%p\n", info );

    /* LSA will make a copy before forwarding the structure, so
     * it's safe to put pointers to dynamic or constant data there.
     */
    *info = infoW;
    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI nego_SpAcquireCredentialsHandle(
    UNICODE_STRING *principal_us, ULONG credential_use, LUID *logon_id, void *auth_data,
    void *get_key_fn, void *get_key_arg, LSA_SEC_HANDLE *credential, TimeStamp *expiry )
{
    NTSTATUS ret;
    struct sec_handle *cred;
    SECPKG_FUNCTION_TABLE *package;
    SECPKG_USER_FUNCTION_TABLE *user;

    TRACE( "%p, %#lx, %p, %p, %p, %p, %p, %p\n", principal_us, credential_use,
           logon_id, auth_data, get_key_fn, get_key_arg, credential, expiry );

    if (!(cred = calloc( 1, sizeof(*cred) ))) return SEC_E_INSUFFICIENT_MEMORY;

    ret = SEC_E_NO_CREDENTIALS;
    if ((package = lsa_find_package( "Kerberos", &user )))
    {
        ret = package->SpAcquireCredentialsHandle( principal_us, credential_use, logon_id, auth_data,
                                                   get_key_fn, get_key_arg, &cred->handle_krb, expiry );
        if (ret == SEC_E_OK)
            cred->krb = package;
    }

    if ((package = lsa_find_package( "NTLM", &user )))
    {
        ULONG cred_use = auth_data ? credential_use : credential_use | WINE_NO_CACHED_CREDENTIALS;

        ret = package->SpAcquireCredentialsHandle( principal_us, cred_use, logon_id, auth_data,
                                                   get_key_fn, get_key_arg, &cred->handle_ntlm, expiry );
        if (ret == SEC_E_OK)
            cred->ntlm = package;
    }

    if (cred->krb || cred->ntlm)
    {
        *credential = (LSA_SEC_HANDLE)cred;
        return SEC_E_OK;
    }

    free( cred );
    return ret;
}

static NTSTATUS NTAPI nego_SpFreeCredentialsHandle( LSA_SEC_HANDLE credential )
{
    struct sec_handle *cred;

    TRACE( "%Ix\n", credential );

    if (!credential) return SEC_E_INVALID_HANDLE;

    cred = (struct sec_handle *)credential;
    if (cred->krb) cred->krb->FreeCredentialsHandle( cred->handle_krb );
    if (cred->ntlm) cred->ntlm->FreeCredentialsHandle( cred->handle_ntlm );

    free( cred );
    return SEC_E_OK;
}

static NTSTATUS NTAPI nego_SpInitLsaModeContext( LSA_SEC_HANDLE credential, LSA_SEC_HANDLE context,
    UNICODE_STRING *target_name, ULONG context_req, ULONG target_data_rep, SecBufferDesc *input,
    LSA_SEC_HANDLE *new_context, SecBufferDesc *output, ULONG *context_attr, TimeStamp *expiry,
    BOOLEAN *mapped_context, SecBuffer *context_data )
{
    NTSTATUS ret = SEC_E_INVALID_HANDLE;
    struct sec_handle *handle = NULL, *ctxt, *new_ctxt = NULL, *cred = NULL;

    TRACE( "%Ix, %Ix, %p, %#lx, %lu, %p, %p, %p, %p, %p, %p, %p\n", credential, context, target_name,
           context_req, target_data_rep, input, new_context, output, context_attr, expiry,
           mapped_context, context_data );

    if (context)
    {
        handle = ctxt = (struct sec_handle *)context;
    }
    else if (credential)
    {
        handle = cred = (struct sec_handle *)credential;
        if (!(new_ctxt = ctxt = calloc( 1, sizeof(*ctxt) ))) return SEC_E_INSUFFICIENT_MEMORY;
        ctxt->krb  = cred->krb;
        ctxt->ntlm = cred->ntlm;
    }
    if (!handle) return SEC_E_INVALID_HANDLE;

    if (handle->krb)
    {
        ret = handle->krb->InitLsaModeContext( credential ? cred->handle_krb : 0,
                context ? ctxt->handle_krb : 0, target_name, context_req, target_data_rep, input,
                new_context ? &ctxt->handle_krb : NULL, output, context_attr, expiry, mapped_context, context_data );
        if ((ret == SEC_E_OK || ret == SEC_I_CONTINUE_NEEDED) && new_context)
        {
            ctxt->ntlm = NULL;
            *new_context = (LSA_SEC_HANDLE)ctxt;
            if (new_ctxt == ctxt) new_ctxt = NULL;
        }
        else
        {
            ctxt->krb = NULL;
        }
    }

    if (ret != SEC_E_OK && ret != SEC_I_CONTINUE_NEEDED && handle->ntlm)
    {
        ret = handle->ntlm->InitLsaModeContext( credential ? cred->handle_ntlm : 0,
                context ? ctxt->handle_ntlm : 0, target_name, context_req, target_data_rep, input,
                new_context ? &ctxt->handle_ntlm : NULL, output, context_attr, expiry, mapped_context, context_data );
        if ((ret == SEC_E_OK || ret == SEC_I_CONTINUE_NEEDED) && new_context)
        {
            ctxt->krb = NULL;
            *new_context = (LSA_SEC_HANDLE)ctxt;
            if (new_ctxt == ctxt) new_ctxt = NULL;
        }
    }

    if (ret == SEC_E_OK)
    {
        struct user_context_data *data;
        ULONG size = sizeof( *data ) + context_data->cbBuffer;
        SecBuffer negotiate_data;

        data = lsa_funcs->AllocateLsaHeap( size );
        if (!data)
        {
            lsa_funcs->FreeLsaHeap( context_data->pvBuffer );
            free( new_ctxt );
            return SEC_E_INSUFFICIENT_MEMORY;
        }
        negotiate_data.cbBuffer = size;
        negotiate_data.pvBuffer = data;
        negotiate_data.BufferType = context_data->BufferType;
        data->ssp = ctxt->krb ? SSP_KERBEROS : SSP_NTLM;
        data->mapped_ctx = *mapped_context;
        memcpy( data + 1, context_data->pvBuffer, context_data->cbBuffer );
        lsa_funcs->FreeLsaHeap( context_data->pvBuffer );


        *mapped_context = TRUE;
        *context_data = negotiate_data;
    }

    free( new_ctxt );
    return ret;
}

static NTSTATUS NTAPI nego_SpAcceptLsaModeContext( LSA_SEC_HANDLE credential, LSA_SEC_HANDLE context,
    SecBufferDesc *input, ULONG context_req, ULONG target_data_rep, LSA_SEC_HANDLE *new_context,
    SecBufferDesc *output, ULONG *context_attr, TimeStamp *expiry, BOOLEAN *mapped_context, SecBuffer *context_data )
{
    SECURITY_STATUS ret = SEC_E_INVALID_HANDLE;
    struct sec_handle *handle = NULL, *ctxt, *new_ctxt = NULL, *cred = NULL;

    TRACE( "%Ix, %Ix, %#lx, %lu, %p, %p, %p, %p, %p, %p, %p\n", credential, context, context_req, target_data_rep,
           input, new_context, output, context_attr, expiry, mapped_context, context_data );

    if (context)
    {
        handle = ctxt = (struct sec_handle *)context;
    }
    else if (credential)
    {
        handle = cred = (struct sec_handle *)credential;
        if (!(new_ctxt = ctxt = calloc( 1, sizeof(*ctxt) ))) return SEC_E_INSUFFICIENT_MEMORY;
        ctxt->krb  = cred->krb;
        ctxt->ntlm = cred->ntlm;
    }
    if (!handle) return SEC_E_INVALID_HANDLE;

    if (handle->krb)
    {
        ret = handle->krb->AcceptLsaModeContext( credential ? cred->handle_krb : 0,
                context ? ctxt->handle_krb : 0, input, context_req, target_data_rep,
                new_context ? &ctxt->handle_krb : NULL,
                output, context_attr, expiry, mapped_context, context_data );
        if ((ret == SEC_E_OK || ret == SEC_I_CONTINUE_NEEDED) && new_context)
        {
            ctxt->ntlm = NULL;
            *new_context = (LSA_SEC_HANDLE)ctxt;
            if (new_ctxt == ctxt) new_ctxt = NULL;
        }
        else
        {
            ctxt->krb = NULL;
        }
    }

    if (ret != SEC_E_OK && ret != SEC_I_CONTINUE_NEEDED && handle->ntlm)
    {
        ret = handle->ntlm->AcceptLsaModeContext( credential ? cred->handle_ntlm : 0,
                context ? ctxt->handle_ntlm : 0, input, context_req, target_data_rep,
                new_context ? &ctxt->handle_ntlm : NULL,
                output, context_attr, expiry, mapped_context, context_data );
        if ((ret == SEC_E_OK || ret == SEC_I_CONTINUE_NEEDED) && new_context)
        {
            ctxt->krb = NULL;
            *new_context = (LSA_SEC_HANDLE)ctxt;
            if (new_ctxt == ctxt) new_ctxt = NULL;
        }
    }

    if (ret == SEC_E_OK)
    {
        struct user_context_data *data;
        ULONG size = sizeof( *data ) + context_data->cbBuffer;
        SecBuffer negotiate_data;

        data = lsa_funcs->AllocateLsaHeap( size );
        if (!data)
        {
            lsa_funcs->FreeLsaHeap( context_data->pvBuffer );
            free( new_ctxt );
            return SEC_E_INSUFFICIENT_MEMORY;
        }
        negotiate_data.cbBuffer = size;
        negotiate_data.pvBuffer = data;
        negotiate_data.BufferType = context_data->BufferType;
        data->ssp = ctxt->krb ? SSP_KERBEROS : SSP_NTLM;
        data->mapped_ctx = *mapped_context;
        memcpy( data + 1, context_data->pvBuffer, context_data->cbBuffer );
        lsa_funcs->FreeLsaHeap( context_data->pvBuffer );

        *mapped_context = TRUE;
        *context_data = negotiate_data;
    }

    free( new_ctxt );
    return ret;
}

static NTSTATUS NTAPI nego_SpDeleteContext( LSA_SEC_HANDLE context )
{
    SECURITY_STATUS ret = SEC_E_INVALID_HANDLE;
    struct sec_handle *ctxt;

    TRACE( "%Ix\n", context );

    if (!context) return SEC_E_INVALID_HANDLE;

    ctxt = (struct sec_handle *)context;
    if (ctxt->krb)
    {
        ret = ctxt->krb->DeleteContext( ctxt->handle_krb );
    }
    else if (ctxt->ntlm)
    {
        ret = ctxt->ntlm->DeleteContext( ctxt->handle_ntlm );
    }
    TRACE( "freeing %p\n", ctxt );
    free( ctxt );
    return ret;
}

static NTSTATUS NTAPI nego_SpQueryContextAttributes( LSA_SEC_HANDLE context, ULONG attribute, void *buffer )
{
    SECURITY_STATUS ret = SEC_E_INVALID_HANDLE;
    struct sec_handle *ctxt;

    TRACE( "%Ix, %lu, %p\n", context, attribute, buffer );

    if (!context) return SEC_E_INVALID_HANDLE;

    ctxt = (struct sec_handle *)context;
    if (ctxt->krb)
    {
        ret = ctxt->krb->SpQueryContextAttributes( ctxt->handle_krb, attribute, buffer );
    }
    else if (ctxt->ntlm)
    {
        ret = ctxt->ntlm->SpQueryContextAttributes( ctxt->handle_ntlm, attribute, buffer );
    }
    return ret;
}

static SECPKG_FUNCTION_TABLE nego_lsa_table =
{
    nego_LsaApInitializePackage,
    NULL, /* LsaLogonUser */
    NULL, /* CallPackage */
    NULL, /* LogonTerminated */
    nego_LsaApCallPackageUntrusted,
    NULL, /* CallPackagePassthrough */
    NULL, /* LogonUserEx */
    NULL, /* LogonUserEx2 */
    nego_SpInitialize,
    nego_SpShutdown,
    nego_SpGetInfo,
    NULL, /* AcceptCredentials */
    nego_SpAcquireCredentialsHandle,
    NULL, /* SpQueryCredentialsAttributes */
    nego_SpFreeCredentialsHandle,
    NULL, /* SaveCredentials */
    NULL, /* GetCredentials */
    NULL, /* DeleteCredentials */
    nego_SpInitLsaModeContext,
    nego_SpAcceptLsaModeContext,
    nego_SpDeleteContext,
    NULL, /* ApplyControlToken */
    NULL, /* GetUserInfo */
    NULL, /* GetExtendedInformation */
    nego_SpQueryContextAttributes,
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

NTSTATUS NTAPI nego_SpLsaModeInitialize(ULONG lsa_version, PULONG package_version,
    PSECPKG_FUNCTION_TABLE *table, PULONG table_count)
{
    TRACE("%#lx, %p, %p, %p\n", lsa_version, package_version, table, table_count);

    *package_version = SECPKG_INTERFACE_VERSION;
    *table = &nego_lsa_table;
    *table_count = 1;
    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI nego_SpInstanceInit(ULONG version, SECPKG_DLL_FUNCTIONS *dll_function_table, void **user_functions)
{
    TRACE("%#lx, %p, %p\n", version, dll_function_table, user_functions);
    return STATUS_SUCCESS;
}

static struct user_ctx* find_user_ctx( LSA_SEC_HANDLE handle )
{
    struct user_ctx *ret;

    EnterCriticalSection( &user_ctx_cs );
    LIST_FOR_EACH_ENTRY( ret, &user_ctx_list, struct user_ctx, entry )
    {
        if (ret->handle == handle)
        {
            LeaveCriticalSection( &user_ctx_cs );
            return ret;
        }
    }
    LeaveCriticalSection( &user_ctx_cs );
    return NULL;
}

static NTSTATUS NTAPI nego_SpInitUserModeContext( LSA_SEC_HANDLE handle, SecBuffer *buf )
{
    struct user_context_data *data = buf->pvBuffer;
    SECPKG_FUNCTION_TABLE *package;
    struct user_ctx *ctx;
    SecBuffer ctx_data;
    NTSTATUS status = SEC_E_OK;

    TRACE( "%Ix, %p\n", handle, buf);

    if (buf->cbBuffer < sizeof( *data ))
        return SEC_E_INTERNAL_ERROR;

    EnterCriticalSection( &user_ctx_cs );
    ctx = find_user_ctx( handle );
    if (!ctx)
    {
        ctx = malloc( sizeof(*ctx) );
        if (!ctx)
        {
            LeaveCriticalSection( &user_ctx_cs );
            return SEC_E_INSUFFICIENT_MEMORY;
        }
        list_add_head( &user_ctx_list, &ctx->entry );
    }
    LeaveCriticalSection( &user_ctx_cs );

    ctx_data.cbBuffer = buf->cbBuffer - sizeof(*data);
    ctx_data.BufferType = buf->BufferType;
    ctx_data.pvBuffer = data + 1;

    ctx->handle = handle;

    if (data->ssp == SSP_KERBEROS)
        package = lsa_find_package( "Kerberos", &ctx->funcs );
    else
        package = lsa_find_package( "NTLM", &ctx->funcs );
    if (!package)
        status = SEC_E_INTERNAL_ERROR;

    if (!status && data->mapped_ctx)
        status = ctx->funcs->InitUserModeContext( handle, &ctx_data );
    if (status)
    {
        EnterCriticalSection( &user_ctx_cs );
        list_remove( &ctx->entry );
        free( ctx );
        LeaveCriticalSection( &user_ctx_cs );
        return status;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI nego_SpMakeSignature( LSA_SEC_HANDLE context, ULONG quality_of_protection,
    SecBufferDesc *message, ULONG message_seq_no )
{
    struct user_ctx *ctxt;

    TRACE( "%Ix, %#lx, %p, %lu\n", context, quality_of_protection, message, message_seq_no );

    if (!(ctxt = find_user_ctx( context ))) return SEC_E_INVALID_HANDLE;
    return ctxt->funcs->MakeSignature( ctxt->handle, quality_of_protection, message, message_seq_no );
}

static NTSTATUS NTAPI nego_SpVerifySignature( LSA_SEC_HANDLE context, SecBufferDesc *message,
    ULONG message_seq_no, ULONG *quality_of_protection )
{
    struct user_ctx *ctxt;

    TRACE( "%Ix, %p, %lu, %p\n", context, message, message_seq_no, quality_of_protection );

    if (!(ctxt = find_user_ctx( context ))) return SEC_E_INVALID_HANDLE;
    return ctxt->funcs->VerifySignature( ctxt->handle, message, message_seq_no, quality_of_protection );
}

static NTSTATUS NTAPI nego_SpSealMessage( LSA_SEC_HANDLE context, ULONG quality_of_protection,
    SecBufferDesc *message, ULONG message_seq_no )
{
    struct user_ctx *ctxt;

    TRACE( "%Ix, %#lx, %p, %lu\n", context, quality_of_protection, message, message_seq_no );

    if (!(ctxt = find_user_ctx( context ))) return SEC_E_INVALID_HANDLE;
    return ctxt->funcs->SealMessage( ctxt->handle, quality_of_protection, message, message_seq_no );
}

static NTSTATUS NTAPI nego_SpUnsealMessage( LSA_SEC_HANDLE context, SecBufferDesc *message,
    ULONG message_seq_no, ULONG *quality_of_protection )
{
    struct user_ctx *ctxt;

    TRACE( "%Ix, %p, %lu, %p\n", context, message, message_seq_no, quality_of_protection );

    if (!(ctxt = find_user_ctx( context ))) return SEC_E_INVALID_HANDLE;
    return ctxt->funcs->UnsealMessage( ctxt->handle, message, message_seq_no, quality_of_protection );
}

static NTSTATUS NTAPI nego_SpDeleteUserModeContext( LSA_SEC_HANDLE handle )
{
    struct user_ctx *user_ctx;

    TRACE( "%Ix\n", handle );

    EnterCriticalSection( &user_ctx_cs );
    user_ctx = find_user_ctx( handle );
    if (user_ctx)
    {
        list_remove( &user_ctx->entry );
        free( user_ctx );
    }
    LeaveCriticalSection( &user_ctx_cs );
    return STATUS_SUCCESS;
}

static SECPKG_USER_FUNCTION_TABLE nego_user_table =
{
    nego_SpInstanceInit,
    nego_SpInitUserModeContext,
    nego_SpMakeSignature,
    nego_SpVerifySignature,
    nego_SpSealMessage,
    nego_SpUnsealMessage,
    NULL, /* SpGetContextToken */
    NULL, /* SpQueryContextAttributes */
    NULL, /* SpCompleteAuthToken */
    nego_SpDeleteUserModeContext,
    NULL, /* SpFormatCredentialsFn */
    NULL, /* SpMarshallSupplementalCreds */
    NULL, /* SpExportSecurityContext */
    NULL  /* SpImportSecurityContext */
};

NTSTATUS NTAPI nego_SpUserModeInitialize(ULONG lsa_version, PULONG package_version,
    PSECPKG_USER_FUNCTION_TABLE *table, PULONG table_count)
{
    TRACE("%#lx, %p, %p, %p\n", lsa_version, package_version, table, table_count);

    *package_version = SECPKG_INTERFACE_VERSION;
    *table = &nego_user_table;
    *table_count = 1;
    return STATUS_SUCCESS;
}
