/*
 * Unix interface for libkrb5/libgssapi_krb5
 *
 * Copyright 2017 Dmitry Timoshkov
 * Copyright 2017 George Popoff
 * Copyright 2008 Robert Shearman for CodeWeavers
 * Copyright 2017,2021 Hans Leidekker for CodeWeavers
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#if defined(SONAME_LIBKRB5) && defined(SONAME_LIBGSSAPI_KRB5)

#include <stdarg.h>
#include <sys/types.h>
#include <dlfcn.h>

#ifdef HAVE_KRB5_KRB5_H
# include <krb5/krb5.h>
#endif
#ifdef HAVE_GSSAPI_GSSAPI_H
# include <gssapi/gssapi.h>
#endif
#ifdef HAVE_GSSAPI_GSSAPI_EXT_H
# include <gssapi/gssapi_ext.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winbase.h"
#include "rpc.h"
#include "sspi.h"
#include "ntsecapi.h"
#include "ntsecpkg.h"

#include "wine/debug.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(kerberos);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

static void *libkrb5_handle;

#define MAKE_FUNCPTR(f) static typeof(f) * p_##f
MAKE_FUNCPTR( krb5_cc_close );
MAKE_FUNCPTR( krb5_cc_default );
MAKE_FUNCPTR( krb5_cc_end_seq_get );
MAKE_FUNCPTR( krb5_cc_initialize );
MAKE_FUNCPTR( krb5_cc_next_cred );
MAKE_FUNCPTR( krb5_cc_start_seq_get );
MAKE_FUNCPTR( krb5_cc_store_cred );
MAKE_FUNCPTR( krb5_cccol_cursor_free );
MAKE_FUNCPTR( krb5_cccol_cursor_new );
MAKE_FUNCPTR( krb5_cccol_cursor_next );
MAKE_FUNCPTR( krb5_decode_ticket );
MAKE_FUNCPTR( krb5_free_context );
MAKE_FUNCPTR( krb5_free_cred_contents );
MAKE_FUNCPTR( krb5_free_principal );
MAKE_FUNCPTR( krb5_free_ticket );
MAKE_FUNCPTR( krb5_free_unparsed_name );
MAKE_FUNCPTR( krb5_get_init_creds_opt_alloc );
MAKE_FUNCPTR( krb5_get_init_creds_opt_free );
MAKE_FUNCPTR( krb5_get_init_creds_opt_set_out_ccache );
MAKE_FUNCPTR( krb5_get_init_creds_password );
MAKE_FUNCPTR( krb5_init_context );
MAKE_FUNCPTR( krb5_is_config_principal );
MAKE_FUNCPTR( krb5_parse_name_flags );
MAKE_FUNCPTR( krb5_unparse_name_flags );
#undef MAKE_FUNCPTR

static BOOL load_krb5(void)
{
    if (!(libkrb5_handle = dlopen( SONAME_LIBKRB5, RTLD_NOW )))
    {
        WARN_(winediag)( "failed to load %s, Kerberos support will be disabled\n", SONAME_LIBKRB5 );
        return FALSE;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p_##f = dlsym( libkrb5_handle, #f ))) \
    { \
        ERR( "failed to load %s\n", #f ); \
        goto fail; \
    }

    LOAD_FUNCPTR( krb5_cc_close )
    LOAD_FUNCPTR( krb5_cc_default )
    LOAD_FUNCPTR( krb5_cc_end_seq_get )
    LOAD_FUNCPTR( krb5_cc_initialize )
    LOAD_FUNCPTR( krb5_cc_next_cred )
    LOAD_FUNCPTR( krb5_cc_start_seq_get )
    LOAD_FUNCPTR( krb5_cc_store_cred )
    LOAD_FUNCPTR( krb5_cccol_cursor_free )
    LOAD_FUNCPTR( krb5_cccol_cursor_new )
    LOAD_FUNCPTR( krb5_cccol_cursor_next )
    LOAD_FUNCPTR( krb5_decode_ticket )
    LOAD_FUNCPTR( krb5_free_context )
    LOAD_FUNCPTR( krb5_free_cred_contents )
    LOAD_FUNCPTR( krb5_free_principal )
    LOAD_FUNCPTR( krb5_free_ticket )
    LOAD_FUNCPTR( krb5_free_unparsed_name )
    LOAD_FUNCPTR( krb5_get_init_creds_opt_alloc )
    LOAD_FUNCPTR( krb5_get_init_creds_opt_free )
    LOAD_FUNCPTR( krb5_get_init_creds_opt_set_out_ccache )
    LOAD_FUNCPTR( krb5_get_init_creds_password )
    LOAD_FUNCPTR( krb5_init_context )
    LOAD_FUNCPTR( krb5_is_config_principal )
    LOAD_FUNCPTR( krb5_parse_name_flags )
    LOAD_FUNCPTR( krb5_unparse_name_flags )
#undef LOAD_FUNCPTR
    return TRUE;

fail:
    dlclose( libkrb5_handle );
    libkrb5_handle = NULL;
    return FALSE;
}

static void unload_krb5(void)
{
    dlclose( libkrb5_handle );
    libkrb5_handle = NULL;
}

static NTSTATUS krb5_error_to_status( krb5_error_code err )
{
    switch (err)
    {
    case 0:  return STATUS_SUCCESS;
    default: return STATUS_UNSUCCESSFUL; /* FIXME */
    }
}

struct ticket_list
{
    ULONG count;
    ULONG allocated;
    KERB_TICKET_CACHE_INFO *tickets;
};

static void utf8_to_wstr( UNICODE_STRING *strW, const char *src )
{
    ULONG dstlen, srclen = strlen( src ) + 1;

    strW->Buffer = malloc( srclen * sizeof(WCHAR) );
    RtlUTF8ToUnicodeN( strW->Buffer, srclen * sizeof(WCHAR), &dstlen, src, srclen );
    strW->MaximumLength = dstlen;
    strW->Length = dstlen - sizeof(WCHAR);
}

static NTSTATUS copy_tickets_from_cache( krb5_context ctx, krb5_ccache cache, struct ticket_list *list )
{
    NTSTATUS status;
    krb5_cc_cursor cursor;
    krb5_error_code err;
    krb5_creds creds;
    krb5_ticket *ticket;
    char *name_with_realm, *name_without_realm, *realm_name;

    if ((err = p_krb5_cc_start_seq_get( ctx, cache, &cursor ))) return krb5_error_to_status( err );
    for (;;)
    {
        if ((err = p_krb5_cc_next_cred( ctx, cache, &cursor, &creds )))
        {
            if (err == KRB5_CC_END)
                status = STATUS_SUCCESS;
            else
                status = krb5_error_to_status( err );
            break;
        }

        if (p_krb5_is_config_principal( ctx, creds.server ))
        {
            p_krb5_free_cred_contents( ctx, &creds );
            continue;
        }

        if (list->count == list->allocated)
        {
            ULONG new_allocated = max( 16, list->allocated * 2 );
            KERB_TICKET_CACHE_INFO *new_tickets = realloc( list->tickets, sizeof(*new_tickets) * new_allocated );
            if (!new_tickets)
            {
                p_krb5_free_cred_contents( ctx, &creds );
                status = STATUS_NO_MEMORY;
                break;
            }
            list->tickets = new_tickets;
            list->allocated = new_allocated;
        }

        if ((err = p_krb5_unparse_name_flags( ctx, creds.server, 0, &name_with_realm )))
        {
            p_krb5_free_cred_contents( ctx, &creds );
            status = krb5_error_to_status( err );
            break;
        }
        TRACE( "name_with_realm: %s\n", debugstr_a(name_with_realm) );

        if ((err = p_krb5_unparse_name_flags( ctx, creds.server, KRB5_PRINCIPAL_UNPARSE_NO_REALM,
                                              &name_without_realm )))
        {
            p_krb5_free_unparsed_name( ctx, name_with_realm );
            p_krb5_free_cred_contents( ctx, &creds );
            status = krb5_error_to_status( err );
            break;
        }
        TRACE( "name_without_realm: %s\n", debugstr_a(name_without_realm) );

        utf8_to_wstr( &list->tickets[list->count].ServerName, name_without_realm );

        if (!(realm_name = strchr( name_with_realm, '@' )))
        {
            ERR( "wrong name with realm %s\n", debugstr_a(name_with_realm) );
            realm_name = name_with_realm;
        }
        else realm_name++;

        /* realm_name - now contains only realm! */
        utf8_to_wstr( &list->tickets[list->count].RealmName, realm_name );

        if (!creds.times.starttime) creds.times.starttime = creds.times.authtime;

        /* TODO: if krb5_is_config_principal = true */

        /* note: store times as seconds, they will be converted to NT timestamps on the PE side */
        list->tickets[list->count].StartTime.QuadPart = creds.times.starttime;
        list->tickets[list->count].EndTime.QuadPart   = creds.times.endtime;
        list->tickets[list->count].RenewTime.QuadPart = creds.times.renew_till;
        list->tickets[list->count].TicketFlags        = creds.ticket_flags;

        err = p_krb5_decode_ticket( &creds.ticket, &ticket );
        p_krb5_free_unparsed_name( ctx, name_with_realm );
        p_krb5_free_unparsed_name( ctx, name_without_realm );
        p_krb5_free_cred_contents( ctx, &creds );
        if (err)
        {
            status = krb5_error_to_status( err );
            break;
        }

        list->tickets[list->count].EncryptionType = ticket->enc_part.enctype;
        p_krb5_free_ticket( ctx, ticket );
        list->count++;
    }

    p_krb5_cc_end_seq_get( ctx, cache, &cursor );
    return status;
}

static NTSTATUS copy_tickets_to_client( struct ticket_list *list, KERB_QUERY_TKT_CACHE_RESPONSE *resp,
                                        ULONG *out_size )
{
    char *client_str;
    ULONG i, size = offsetof( KERB_QUERY_TKT_CACHE_RESPONSE, Tickets[list->count] );

    for (i = 0; i < list->count; i++)
    {
        size += list->tickets[i].RealmName.MaximumLength;
        size += list->tickets[i].ServerName.MaximumLength;
    }
    if (!resp || size > *out_size)
    {
        *out_size = size;
        return STATUS_BUFFER_TOO_SMALL;
    }
    *out_size = size;

    resp->MessageType = KerbQueryTicketCacheMessage;
    resp->CountOfTickets = list->count;
    memcpy( resp->Tickets, list->tickets, list->count * sizeof(list->tickets[0]) );
    client_str = (char *)&resp->Tickets[list->count];

    for (i = 0; i < list->count; i++)
    {
        resp->Tickets[i].RealmName.Buffer = (WCHAR *)client_str;
        memcpy( client_str, list->tickets[i].RealmName.Buffer, list->tickets[i].RealmName.MaximumLength );
        client_str += list->tickets[i].RealmName.MaximumLength;
        resp->Tickets[i].ServerName.Buffer = (WCHAR *)client_str;
        memcpy( client_str, list->tickets[i].ServerName.Buffer, list->tickets[i].ServerName.MaximumLength );
        client_str += list->tickets[i].ServerName.MaximumLength;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS query_ticket_cache( void *args )
{
    struct query_ticket_cache_params *params = args;
    NTSTATUS status;
    krb5_error_code err;
    krb5_context ctx;
    krb5_cccol_cursor cursor = NULL;
    krb5_ccache cache;
    ULONG i;
    struct ticket_list list = { 0 };

    if ((err = p_krb5_init_context( &ctx ))) return krb5_error_to_status( err );
    if ((err = p_krb5_cccol_cursor_new( ctx, &cursor )))
    {
        status = krb5_error_to_status( err );
        goto done;
    }

    for (;;)
    {
        if ((err = p_krb5_cccol_cursor_next( ctx, cursor, &cache )))
        {
            status = krb5_error_to_status( err );
            goto done;
        }
        if (!cache) break;

        status = copy_tickets_from_cache( ctx, cache, &list );
        p_krb5_cc_close( ctx, cache );
        if (status != STATUS_SUCCESS) goto done;
    }

done:
    if (cursor) p_krb5_cccol_cursor_free( ctx, &cursor );
    if (ctx) p_krb5_free_context( ctx );

    if (status == STATUS_SUCCESS) status = copy_tickets_to_client( &list, params->resp, params->out_size );

    for (i = 0; i < list.count; i++)
    {
        free( list.tickets[i].RealmName.Buffer );
        free( list.tickets[i].ServerName.Buffer );
    }
    return status;
}

static void *libgssapi_krb5_handle;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR( gss_accept_sec_context );
MAKE_FUNCPTR( gss_acquire_cred );
MAKE_FUNCPTR( gss_delete_sec_context );
MAKE_FUNCPTR( gss_display_status );
MAKE_FUNCPTR( gss_get_mic );
MAKE_FUNCPTR( gss_import_name );
MAKE_FUNCPTR( gss_init_sec_context );
MAKE_FUNCPTR( gss_inquire_context );
MAKE_FUNCPTR( gss_release_buffer );
MAKE_FUNCPTR( gss_release_cred );
MAKE_FUNCPTR( gss_release_iov_buffer );
MAKE_FUNCPTR( gss_release_name );
MAKE_FUNCPTR( gss_unwrap );
MAKE_FUNCPTR( gss_unwrap_iov );
MAKE_FUNCPTR( gss_verify_mic );
MAKE_FUNCPTR( gss_wrap );
MAKE_FUNCPTR( gss_wrap_iov );
#undef MAKE_FUNCPTR

static BOOL load_gssapi_krb5(void)
{
    if (!(libgssapi_krb5_handle = dlopen( SONAME_LIBGSSAPI_KRB5, RTLD_NOW )))
    {
        WARN_(winediag)( "failed to load %s, Kerberos support will be disabled\n", SONAME_LIBGSSAPI_KRB5 );
        return FALSE;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p##f = dlsym( libgssapi_krb5_handle, #f ))) \
    { \
        ERR( "failed to load %s\n", #f ); \
        goto fail; \
    }

    LOAD_FUNCPTR( gss_accept_sec_context)
    LOAD_FUNCPTR( gss_acquire_cred)
    LOAD_FUNCPTR( gss_delete_sec_context)
    LOAD_FUNCPTR( gss_display_status)
    LOAD_FUNCPTR( gss_get_mic)
    LOAD_FUNCPTR( gss_import_name)
    LOAD_FUNCPTR( gss_init_sec_context)
    LOAD_FUNCPTR( gss_inquire_context)
    LOAD_FUNCPTR( gss_release_buffer)
    LOAD_FUNCPTR( gss_release_cred)
    LOAD_FUNCPTR( gss_release_iov_buffer)
    LOAD_FUNCPTR( gss_release_name)
    LOAD_FUNCPTR( gss_unwrap)
    LOAD_FUNCPTR( gss_unwrap_iov)
    LOAD_FUNCPTR( gss_verify_mic)
    LOAD_FUNCPTR( gss_wrap )
    LOAD_FUNCPTR( gss_wrap_iov )
#undef LOAD_FUNCPTR
    return TRUE;

fail:
    dlclose( libgssapi_krb5_handle );
    libgssapi_krb5_handle = NULL;
    return FALSE;
}

static BOOL is_dce_style_context( gss_ctx_id_t ctx )
{
    OM_uint32 ret, minor_status, flags;
    ret = pgss_inquire_context( &minor_status, ctx, NULL, NULL, NULL, NULL, &flags, NULL, NULL );
    return (ret == GSS_S_COMPLETE && (flags & GSS_C_DCE_STYLE));
}

static int get_buffer_index( SecBufferDesc *desc, DWORD type )
{
    UINT i;
    if (!desc) return -1;
    for (i = 0; i < desc->cBuffers; i++)
    {
        if (desc->pBuffers[i].BufferType == type) return i;
    }
    return -1;
}

static NTSTATUS status_gss_to_sspi( OM_uint32 status )
{
    switch (status)
    {
    case GSS_S_COMPLETE:             return SEC_E_OK;
    case GSS_S_BAD_MECH:             return SEC_E_SECPKG_NOT_FOUND;
    case GSS_S_BAD_SIG:              return SEC_E_MESSAGE_ALTERED;
    case GSS_S_NO_CRED:              return SEC_E_NO_CREDENTIALS;
    case GSS_S_NO_CONTEXT:           return SEC_E_INVALID_HANDLE;
    case GSS_S_DEFECTIVE_TOKEN:      return SEC_E_INVALID_TOKEN;
    case GSS_S_DEFECTIVE_CREDENTIAL: return SEC_E_NO_CREDENTIALS;
    case GSS_S_CREDENTIALS_EXPIRED:  return SEC_E_CONTEXT_EXPIRED;
    case GSS_S_CONTEXT_EXPIRED:      return SEC_E_CONTEXT_EXPIRED;
    case GSS_S_BAD_QOP:              return SEC_E_QOP_NOT_SUPPORTED;
    case GSS_S_CONTINUE_NEEDED:      return SEC_I_CONTINUE_NEEDED;
    case GSS_S_DUPLICATE_TOKEN:      return SEC_E_INVALID_TOKEN;
    case GSS_S_OLD_TOKEN:            return SEC_E_INVALID_TOKEN;
    case GSS_S_UNSEQ_TOKEN:          return SEC_E_OUT_OF_SEQUENCE;
    case GSS_S_GAP_TOKEN:            return SEC_E_OUT_OF_SEQUENCE;
    case GSS_S_FAILURE:              return SEC_E_INTERNAL_ERROR;

    default:
        FIXME( "couldn't convert status %#x to NTSTATUS\n", status );
        return SEC_E_INTERNAL_ERROR;
    }
}

static void trace_gss_status_ex( OM_uint32 code, int type )
{
    OM_uint32 ret, minor_status;
    gss_buffer_desc buf;
    OM_uint32 msg_ctx = 0;

    for (;;)
    {
        ret = pgss_display_status( &minor_status, code, type, GSS_C_NULL_OID, &msg_ctx, &buf );
        if (GSS_ERROR( ret ))
        {
            TRACE( "gss_display_status(%#x, %d) returned %#x minor status %#x\n", code, type, ret, minor_status );
            return;
        }
        TRACE( "GSS-API error: %#x: %s\n", code, debugstr_an(buf.value, buf.length) );
        pgss_release_buffer( &minor_status, &buf );
        if (!msg_ctx) return;
    }
}

static void trace_gss_status( OM_uint32 major_status, OM_uint32 minor_status )
{
    if (TRACE_ON(kerberos))
    {
        trace_gss_status_ex( major_status, GSS_C_GSS_CODE );
        trace_gss_status_ex( minor_status, GSS_C_MECH_CODE );
    }
}

static inline gss_ctx_id_t ctxhandle_sspi_to_gss( LSA_SEC_HANDLE handle )
{
    return (gss_ctx_id_t)handle;
}

static inline gss_cred_id_t credhandle_sspi_to_gss( LSA_SEC_HANDLE handle )
{
    return (gss_cred_id_t)handle;
}

static inline void ctxhandle_gss_to_sspi( gss_ctx_id_t handle, LSA_SEC_HANDLE *ctx )
{
    *ctx = (LSA_SEC_HANDLE)handle;
}

static inline void credhandle_gss_to_sspi( gss_cred_id_t handle, LSA_SEC_HANDLE *cred )
{
    *cred = (LSA_SEC_HANDLE)handle;
}

static ULONG flags_gss_to_asc_ret( ULONG flags )
{
    ULONG ret = 0;
    if (flags & GSS_C_DELEG_FLAG)    ret |= ASC_RET_DELEGATE;
    if (flags & GSS_C_MUTUAL_FLAG)   ret |= ASC_RET_MUTUAL_AUTH;
    if (flags & GSS_C_REPLAY_FLAG)   ret |= ASC_RET_REPLAY_DETECT;
    if (flags & GSS_C_SEQUENCE_FLAG) ret |= ASC_RET_SEQUENCE_DETECT;
    if (flags & GSS_C_CONF_FLAG)     ret |= ASC_RET_CONFIDENTIALITY;
    if (flags & GSS_C_INTEG_FLAG)    ret |= ASC_RET_INTEGRITY;
    if (flags & GSS_C_ANON_FLAG)     ret |= ASC_RET_NULL_SESSION;
    if (flags & GSS_C_DCE_STYLE)     ret |= ASC_RET_USED_DCE_STYLE;
    if (flags & GSS_C_IDENTIFY_FLAG) ret |= ASC_RET_IDENTIFY;
    return ret;
}

static NTSTATUS accept_context( void *args )
{
    struct accept_context_params *params = args;
    OM_uint32 ret, minor_status, ret_flags = 0, expiry_time;
    gss_cred_id_t cred_handle = credhandle_sspi_to_gss( params->credential );
    gss_ctx_id_t ctx_handle = ctxhandle_sspi_to_gss( params->context );
    gss_buffer_desc input_token, output_token;
    int idx;

    if (!params->input) input_token.length = 0;
    else
    {
        if ((idx = get_buffer_index( params->input, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;
        input_token.length = params->input->pBuffers[idx].cbBuffer;
        input_token.value  = params->input->pBuffers[idx].pvBuffer;
    }

    if ((idx = get_buffer_index( params->output, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;
    output_token.length = 0;
    output_token.value  = NULL;

    ret = pgss_accept_sec_context( &minor_status, &ctx_handle, cred_handle, &input_token, GSS_C_NO_CHANNEL_BINDINGS,
                                   NULL, NULL, &output_token, &ret_flags, &expiry_time, NULL );
    TRACE( "gss_accept_sec_context returned %#x minor status %#x ret_flags %#x\n", ret, minor_status, ret_flags );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE || ret == GSS_S_CONTINUE_NEEDED)
    {
        if (output_token.length > params->output->pBuffers[idx].cbBuffer) /* FIXME: check if larger buffer exists */
        {
            TRACE( "buffer too small %lu > %u\n",
                   (SIZE_T)output_token.length, (unsigned int)params->output->pBuffers[idx].cbBuffer );
            pgss_release_buffer( &minor_status, &output_token );
            pgss_delete_sec_context( &minor_status, &ctx_handle, GSS_C_NO_BUFFER );
            return SEC_E_BUFFER_TOO_SMALL;
        }
        params->output->pBuffers[idx].cbBuffer = output_token.length;
        memcpy( params->output->pBuffers[idx].pvBuffer, output_token.value, output_token.length );
        pgss_release_buffer( &minor_status, &output_token );

        ctxhandle_gss_to_sspi( ctx_handle, params->new_context );
        if (params->context_attr) *params->context_attr = flags_gss_to_asc_ret( ret_flags );
        *params->expiry = expiry_time;
    }

    return status_gss_to_sspi( ret );
}

static NTSTATUS init_creds( const char *user_at_domain, const char *password )
{
    krb5_context ctx;
    krb5_principal principal = NULL;
    krb5_get_init_creds_opt *options = NULL;
    krb5_ccache cache = NULL;
    krb5_creds creds;
    krb5_error_code err;

    if (!user_at_domain) return STATUS_SUCCESS;
    if ((err = p_krb5_init_context( &ctx ))) return krb5_error_to_status( err );
    if ((err = p_krb5_parse_name_flags( ctx, user_at_domain, 0, &principal ))) goto done;
    if ((err = p_krb5_cc_default( ctx, &cache ))) goto done;
    if ((err = p_krb5_get_init_creds_opt_alloc( ctx, &options ))) goto done;
    if ((err = p_krb5_get_init_creds_opt_set_out_ccache( ctx, options, cache ))) goto done;
    if ((err = p_krb5_get_init_creds_password( ctx, &creds, principal, password, 0, NULL, 0, NULL, 0 ))) goto done;
    if ((err = p_krb5_cc_initialize( ctx, cache, principal ))) goto done;
    if ((err = p_krb5_cc_store_cred( ctx, cache, &creds ))) goto done;

    TRACE( "success\n" );
    p_krb5_free_cred_contents( ctx, &creds );

done:
    if (cache) p_krb5_cc_close( ctx, cache );
    if (principal) p_krb5_free_principal( ctx, principal );
    if (options) p_krb5_get_init_creds_opt_free( ctx, options );
    p_krb5_free_context( ctx );
    return krb5_error_to_status( err );
}

static NTSTATUS import_name( const char *src, gss_name_t *dst )
{
    OM_uint32 ret, minor_status;
    gss_buffer_desc buf;

    buf.length = strlen( src );
    buf.value  = (void *)src;
    ret = pgss_import_name( &minor_status, &buf, GSS_C_NO_OID, dst );
    TRACE( "gss_import_name returned %#x minor status %#x\n", ret, minor_status );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    return status_gss_to_sspi( ret );
}

static NTSTATUS acquire_credentials_handle( void *args )
{
    struct acquire_credentials_handle_params *params = args;
    OM_uint32 ret, minor_status, expiry_time;
    gss_name_t name = GSS_C_NO_NAME;
    gss_cred_usage_t cred_usage;
    gss_cred_id_t cred_handle;
    NTSTATUS status;

    switch (params->credential_use)
    {
    case SECPKG_CRED_INBOUND:
        cred_usage = GSS_C_ACCEPT;
        break;

    case SECPKG_CRED_OUTBOUND:
        if ((status = init_creds( params->username, params->password )) != STATUS_SUCCESS) return status;
        cred_usage = GSS_C_INITIATE;
        break;

    default:
        FIXME( "SECPKG_CRED_BOTH not supported\n" );
        return SEC_E_UNKNOWN_CREDENTIALS;
    }

    if (params->principal && (status = import_name( params->principal, &name ))) return status;

    ret = pgss_acquire_cred( &minor_status, name, GSS_C_INDEFINITE, GSS_C_NULL_OID_SET, cred_usage, &cred_handle,
                             NULL, &expiry_time );
    TRACE( "gss_acquire_cred returned %#x minor status %#x\n", ret, minor_status );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE)
    {
        credhandle_gss_to_sspi( cred_handle, params->credential );
        *params->expiry = expiry_time;
    }

    if (name != GSS_C_NO_NAME) pgss_release_name( &minor_status, &name );
    return status_gss_to_sspi( ret );
}

static NTSTATUS delete_context( void *args )
{
    OM_uint32 ret, minor_status;
    gss_ctx_id_t ctx_handle = ctxhandle_sspi_to_gss( (LSA_SEC_HANDLE)args );

    ret = pgss_delete_sec_context( &minor_status, &ctx_handle, GSS_C_NO_BUFFER );
    TRACE( "gss_delete_sec_context returned %#x minor status %#x\n", ret, minor_status );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    return status_gss_to_sspi( ret );
}

static NTSTATUS free_credentials_handle( void *args )
{
    OM_uint32 ret, minor_status;
    gss_cred_id_t cred = credhandle_sspi_to_gss( (LSA_SEC_HANDLE)args );

    ret = pgss_release_cred( &minor_status, &cred );
    TRACE( "gss_release_cred returned %#x minor status %#x\n", ret, minor_status );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    return status_gss_to_sspi( ret );
}

static ULONG flags_isc_req_to_gss( ULONG flags )
{
    ULONG ret = 0;
    if (flags & ISC_REQ_DELEGATE)        ret |= GSS_C_DELEG_FLAG;
    if (flags & ISC_REQ_MUTUAL_AUTH)     ret |= GSS_C_MUTUAL_FLAG;
    if (flags & ISC_REQ_REPLAY_DETECT)   ret |= GSS_C_REPLAY_FLAG;
    if (flags & ISC_REQ_SEQUENCE_DETECT) ret |= GSS_C_SEQUENCE_FLAG;
    if (flags & ISC_REQ_CONFIDENTIALITY) ret |= GSS_C_CONF_FLAG;
    if (flags & ISC_REQ_INTEGRITY)       ret |= GSS_C_INTEG_FLAG;
    if (flags & ISC_REQ_NULL_SESSION)    ret |= GSS_C_ANON_FLAG;
    if (flags & ISC_REQ_USE_DCE_STYLE)   ret |= GSS_C_DCE_STYLE;
    if (flags & ISC_REQ_IDENTIFY)        ret |= GSS_C_IDENTIFY_FLAG;
    return ret;
}

static ULONG flags_gss_to_isc_ret( ULONG flags )
{
    ULONG ret = 0;
    if (flags & GSS_C_DELEG_FLAG)    ret |= ISC_RET_DELEGATE;
    if (flags & GSS_C_MUTUAL_FLAG)   ret |= ISC_RET_MUTUAL_AUTH;
    if (flags & GSS_C_REPLAY_FLAG)   ret |= ISC_RET_REPLAY_DETECT;
    if (flags & GSS_C_SEQUENCE_FLAG) ret |= ISC_RET_SEQUENCE_DETECT;
    if (flags & GSS_C_CONF_FLAG)     ret |= ISC_RET_CONFIDENTIALITY;
    if (flags & GSS_C_INTEG_FLAG)    ret |= ISC_RET_INTEGRITY;
    if (flags & GSS_C_ANON_FLAG)     ret |= ISC_RET_NULL_SESSION;
    if (flags & GSS_C_DCE_STYLE)     ret |= ISC_RET_USED_DCE_STYLE;
    if (flags & GSS_C_IDENTIFY_FLAG) ret |= ISC_RET_IDENTIFY;
    return ret;
}

static NTSTATUS initialize_context( void *args )
{
    struct initialize_context_params *params = args;
    OM_uint32 ret, minor_status, ret_flags = 0, expiry_time, req_flags = flags_isc_req_to_gss( params->context_req );
    gss_cred_id_t cred_handle = credhandle_sspi_to_gss( params->credential );
    gss_ctx_id_t ctx_handle = ctxhandle_sspi_to_gss( params->context );
    gss_buffer_desc input_token, output_token;
    gss_name_t target = GSS_C_NO_NAME;
    NTSTATUS status;
    int idx;

    if ((idx = get_buffer_index( params->input, SECBUFFER_TOKEN )) == -1) input_token.length = 0;
    else
    {
        input_token.length = params->input->pBuffers[idx].cbBuffer;
        input_token.value  = params->input->pBuffers[idx].pvBuffer;
    }

    if ((idx = get_buffer_index( params->output, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;
    output_token.length = 0;
    output_token.value  = NULL;

    if (params->target_name && (status = import_name( params->target_name, &target ))) return status;

    ret = pgss_init_sec_context( &minor_status, cred_handle, &ctx_handle, target, GSS_C_NO_OID, req_flags, 0,
                                 GSS_C_NO_CHANNEL_BINDINGS, &input_token, NULL, &output_token, &ret_flags,
                                 &expiry_time );
    TRACE( "gss_init_sec_context returned %#x minor status %#x ret_flags %#x\n", ret, minor_status, ret_flags );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE || ret == GSS_S_CONTINUE_NEEDED)
    {
        if (output_token.length > params->output->pBuffers[idx].cbBuffer) /* FIXME: check if larger buffer exists */
        {
            TRACE( "buffer too small %lu > %u\n",
                   (SIZE_T)output_token.length, (unsigned int)params->output->pBuffers[idx].cbBuffer );
            pgss_release_buffer( &minor_status, &output_token );
            pgss_delete_sec_context( &minor_status, &ctx_handle, GSS_C_NO_BUFFER );
            return SEC_E_INCOMPLETE_MESSAGE;
        }
        params->output->pBuffers[idx].cbBuffer = output_token.length;
        memcpy( params->output->pBuffers[idx].pvBuffer, output_token.value, output_token.length );
        pgss_release_buffer( &minor_status, &output_token );

        ctxhandle_gss_to_sspi( ctx_handle, params->new_context );
        if (params->context_attr) *params->context_attr = flags_gss_to_isc_ret( ret_flags );
        *params->expiry = expiry_time;
    }

    if (target != GSS_C_NO_NAME) pgss_release_name( &minor_status, &target );
    return status_gss_to_sspi( ret );
}

static NTSTATUS make_signature( void *args )
{
    struct make_signature_params *params = args;
    SecBufferDesc *msg = params->msg;
    OM_uint32 ret, minor_status;
    gss_buffer_desc data_buffer, token_buffer;
    gss_ctx_id_t ctx_handle = ctxhandle_sspi_to_gss( params->context );
    int data_idx, token_idx;

    /* FIXME: multiple data buffers, read-only buffers */
    if ((data_idx = get_buffer_index( msg, SECBUFFER_DATA )) == -1) return SEC_E_INVALID_TOKEN;
    data_buffer.length = msg->pBuffers[data_idx].cbBuffer;
    data_buffer.value  = msg->pBuffers[data_idx].pvBuffer;

    if ((token_idx = get_buffer_index( msg, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;
    token_buffer.length = 0;
    token_buffer.value  = NULL;

    ret = pgss_get_mic( &minor_status, ctx_handle, GSS_C_QOP_DEFAULT, &data_buffer, &token_buffer );
    TRACE( "gss_get_mic returned %#x minor status %#x\n", ret, minor_status );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE)
    {
        memcpy( msg->pBuffers[token_idx].pvBuffer, token_buffer.value, token_buffer.length );
        msg->pBuffers[token_idx].cbBuffer = token_buffer.length;
        pgss_release_buffer( &minor_status, &token_buffer );
    }

    return status_gss_to_sspi( ret );
}

#define KERBEROS_MAX_SIGNATURE        37
#define KERBEROS_SECURITY_TRAILER     49
#define KERBEROS_MAX_SIGNATURE_DCE    28
#define KERBEROS_SECURITY_TRAILER_DCE 76

static NTSTATUS query_context_attributes( void *args )
{
    struct query_context_attributes_params *params = args;
    switch (params->attr)
    {
    case SECPKG_ATTR_SIZES:
    {
        SecPkgContext_Sizes *sizes = (SecPkgContext_Sizes *)params->buf;
        ULONG size_max_signature, size_security_trailer;
        gss_ctx_id_t ctx  = ctxhandle_sspi_to_gss( params->context );

        if (is_dce_style_context( ctx ))
        {
            size_max_signature = KERBEROS_MAX_SIGNATURE_DCE;
            size_security_trailer = KERBEROS_SECURITY_TRAILER_DCE;
        }
        else
        {
            size_max_signature = KERBEROS_MAX_SIGNATURE;
            size_security_trailer = KERBEROS_SECURITY_TRAILER;
        }
        sizes->cbMaxToken        = KERBEROS_MAX_BUF;
        sizes->cbMaxSignature    = size_max_signature;
        sizes->cbBlockSize       = 1;
        sizes->cbSecurityTrailer = size_security_trailer;
        return SEC_E_OK;
    }
    default:
        FIXME( "unhandled attribute %u\n", params->attr );
        break;
    }

    return SEC_E_UNSUPPORTED_FUNCTION;
}

static NTSTATUS seal_message_vector( gss_ctx_id_t ctx, SecBufferDesc *msg, unsigned qop )
{
    gss_iov_buffer_desc iov[4];
    OM_uint32 ret, minor_status;
    int token_idx, data_idx, conf_flag, conf_state;

    if (!qop)
        conf_flag = 1; /* confidentiality + integrity */
    else if (qop == SECQOP_WRAP_NO_ENCRYPT)
        conf_flag = 0; /* only integrity */
    else
    {
        FIXME( "QOP %#x not supported\n", qop );
        return SEC_E_UNSUPPORTED_FUNCTION;
    }

    /* FIXME: multiple data buffers, read-only buffers */
    if ((data_idx = get_buffer_index( msg, SECBUFFER_DATA )) == -1) return SEC_E_INVALID_TOKEN;
    if ((token_idx = get_buffer_index( msg, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;

    iov[0].type          = GSS_IOV_BUFFER_TYPE_SIGN_ONLY | GSS_IOV_BUFFER_FLAG_ALLOCATE;
    iov[0].buffer.length = 0;
    iov[0].buffer.value  = NULL;

    iov[1].type          = GSS_IOV_BUFFER_TYPE_DATA;
    iov[1].buffer.length = msg->pBuffers[data_idx].cbBuffer;
    iov[1].buffer.value  = msg->pBuffers[data_idx].pvBuffer;

    iov[2].type          = GSS_IOV_BUFFER_TYPE_SIGN_ONLY | GSS_IOV_BUFFER_FLAG_ALLOCATE;
    iov[2].buffer.length = 0;
    iov[2].buffer.value  = NULL;

    iov[3].type          = GSS_IOV_BUFFER_TYPE_HEADER | GSS_IOV_BUFFER_FLAG_ALLOCATE;
    iov[3].buffer.length = 0;
    iov[3].buffer.value  = NULL;

    ret = pgss_wrap_iov( &minor_status, ctx, conf_flag, GSS_C_QOP_DEFAULT, &conf_state, iov, 4 );
    TRACE( "gss_wrap_iov returned %#x minor status %#x\n", ret, minor_status );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE)
    {
        memcpy( msg->pBuffers[token_idx].pvBuffer, iov[3].buffer.value, iov[3].buffer.length );
        msg->pBuffers[token_idx].cbBuffer = iov[3].buffer.length;
        pgss_release_iov_buffer( &minor_status, iov, 4 );
    }

    return status_gss_to_sspi( ret );
}

static NTSTATUS seal_message_no_vector( gss_ctx_id_t ctx, SecBufferDesc *msg, unsigned qop )
{
    gss_buffer_desc input, output;
    OM_uint32 ret, minor_status;
    int token_idx, data_idx, conf_flag, conf_state;

    if (!qop)
        conf_flag = 1; /* confidentiality + integrity */
    else if (qop == SECQOP_WRAP_NO_ENCRYPT)
        conf_flag = 0; /* only integrity */
    else
    {
        FIXME( "QOP %#x not supported\n", qop );
        return SEC_E_UNSUPPORTED_FUNCTION;
    }

    /* FIXME: multiple data buffers, read-only buffers */
    if ((data_idx = get_buffer_index( msg, SECBUFFER_DATA )) == -1) return SEC_E_INVALID_TOKEN;
    if ((token_idx = get_buffer_index( msg, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;

    input.length = msg->pBuffers[data_idx].cbBuffer;
    input.value  = msg->pBuffers[data_idx].pvBuffer;

    ret = pgss_wrap( &minor_status, ctx, conf_flag, GSS_C_QOP_DEFAULT, &input, &conf_state, &output );
    TRACE( "gss_wrap returned %#x minor status %#x\n", ret, minor_status );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE)
    {
        unsigned len_data = msg->pBuffers[data_idx].cbBuffer, len_token = msg->pBuffers[token_idx].cbBuffer;
        if (len_token < output.length - len_data)
        {
            TRACE( "buffer too small %lu > %u\n", (SIZE_T)output.length - len_data, len_token );
            pgss_release_buffer( &minor_status, &output );
            return SEC_E_BUFFER_TOO_SMALL;
        }
        memcpy( msg->pBuffers[data_idx].pvBuffer, output.value, len_data );
        memcpy( msg->pBuffers[token_idx].pvBuffer, (char *)output.value + len_data, output.length - len_data );
        msg->pBuffers[token_idx].cbBuffer = output.length - len_data;
        pgss_release_buffer( &minor_status, &output );
    }

    return status_gss_to_sspi( ret );
}

static NTSTATUS seal_message( void *args )
{
    struct seal_message_params *params = args;
    gss_ctx_id_t ctx = ctxhandle_sspi_to_gss( params->context );

    if (is_dce_style_context( ctx )) return seal_message_vector( ctx, params->msg, params->qop );
    return seal_message_no_vector( ctx, params->msg, params->qop );
}

static NTSTATUS unseal_message_vector( gss_ctx_id_t ctx, SecBufferDesc *msg, ULONG *qop )
{
    gss_iov_buffer_desc iov[4];
    OM_uint32 ret, minor_status;
    int token_idx, data_idx, conf_state;

    if ((data_idx = get_buffer_index( msg, SECBUFFER_DATA )) == -1) return SEC_E_INVALID_TOKEN;
    if ((token_idx = get_buffer_index( msg, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;

    iov[0].type          = GSS_IOV_BUFFER_TYPE_SIGN_ONLY;
    iov[0].buffer.length = 0;
    iov[0].buffer.value  = NULL;

    iov[1].type          = GSS_IOV_BUFFER_TYPE_DATA;
    iov[1].buffer.length = msg->pBuffers[data_idx].cbBuffer;
    iov[1].buffer.value  = msg->pBuffers[data_idx].pvBuffer;

    iov[2].type          = GSS_IOV_BUFFER_TYPE_SIGN_ONLY;
    iov[2].buffer.length = 0;
    iov[2].buffer.value  = NULL;

    iov[3].type          = GSS_IOV_BUFFER_TYPE_HEADER;
    iov[3].buffer.length = msg->pBuffers[token_idx].cbBuffer;
    iov[3].buffer.value  = msg->pBuffers[token_idx].pvBuffer;

    ret = pgss_unwrap_iov( &minor_status, ctx, &conf_state, NULL, iov, 4 );
    TRACE( "gss_unwrap_iov returned %#x minor status %#x\n", ret, minor_status );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE && qop)
    {
        *qop = (conf_state ? 0 : SECQOP_WRAP_NO_ENCRYPT);
    }
    return status_gss_to_sspi( ret );
}

static NTSTATUS unseal_message_no_vector( gss_ctx_id_t ctx, SecBufferDesc *msg, ULONG *qop )
{
    gss_buffer_desc input, output;
    OM_uint32 ret, minor_status;
    int token_idx, data_idx, conf_state;
    DWORD len_data, len_token;

    if ((data_idx = get_buffer_index( msg, SECBUFFER_DATA )) == -1) return SEC_E_INVALID_TOKEN;
    if ((token_idx = get_buffer_index( msg, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;

    len_data = msg->pBuffers[data_idx].cbBuffer;
    len_token = msg->pBuffers[token_idx].cbBuffer;

    input.length = len_data + len_token;
    if (!(input.value = malloc( input.length ))) return SEC_E_INSUFFICIENT_MEMORY;
    memcpy( input.value, msg->pBuffers[data_idx].pvBuffer, len_data );
    memcpy( (char *)input.value + len_data, msg->pBuffers[token_idx].pvBuffer, len_token );

    ret = pgss_unwrap( &minor_status, ctx, &input, &output, &conf_state, NULL );
    free( input.value );
    TRACE( "gss_unwrap returned %#x minor status %#x\n", ret, minor_status );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE)
    {
        if (qop) *qop = (conf_state ? 0 : SECQOP_WRAP_NO_ENCRYPT);
        memcpy( msg->pBuffers[data_idx].pvBuffer, output.value, len_data );
        pgss_release_buffer( &minor_status, &output );
    }

    return status_gss_to_sspi( ret );
}

static NTSTATUS unseal_message( void *args )
{
    struct unseal_message_params *params = args;
    gss_ctx_id_t ctx = ctxhandle_sspi_to_gss( params->context );

    if (is_dce_style_context( ctx )) return unseal_message_vector( ctx, params->msg, params->qop );
    return unseal_message_no_vector( ctx, params->msg, params->qop );
}

static NTSTATUS verify_signature( void *args )
{
    struct verify_signature_params *params = args;
    SecBufferDesc *msg = params->msg;
    OM_uint32 ret, minor_status;
    gss_buffer_desc data_buffer, token_buffer;
    gss_ctx_id_t ctx_handle = ctxhandle_sspi_to_gss( params->context );
    int data_idx, token_idx;

    if ((data_idx = get_buffer_index( msg, SECBUFFER_DATA )) == -1) return SEC_E_INVALID_TOKEN;
    data_buffer.length = msg->pBuffers[data_idx].cbBuffer;
    data_buffer.value  = msg->pBuffers[data_idx].pvBuffer;

    if ((token_idx = get_buffer_index( msg, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;
    token_buffer.length = msg->pBuffers[token_idx].cbBuffer;
    token_buffer.value  = msg->pBuffers[token_idx].pvBuffer;

    ret = pgss_verify_mic( &minor_status, ctx_handle, &data_buffer, &token_buffer, NULL );
    TRACE( "gss_verify_mic returned %#x minor status %#x\n", ret, minor_status );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE && params->qop) *params->qop = 0;

    return status_gss_to_sspi( ret );
}

static NTSTATUS process_attach( void *args )
{
    if (load_krb5() && load_gssapi_krb5()) return STATUS_SUCCESS;
    if (libkrb5_handle) unload_krb5();
    return STATUS_DLL_NOT_FOUND;
}

unixlib_entry_t __wine_unix_call_funcs[] =
{
    process_attach,
    accept_context,
    acquire_credentials_handle,
    delete_context,
    free_credentials_handle,
    initialize_context,
    make_signature,
    query_context_attributes,
    query_ticket_cache,
    seal_message,
    unseal_message,
    verify_signature,
};

#endif /* defined(SONAME_LIBKRB5) && defined(SONAME_LIBGSSAPI_KRB5) */
