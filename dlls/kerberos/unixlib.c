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
    KERB_TICKET_CACHE_INFO_EX *tickets;
};

static void utf8_to_wstr( UNICODE_STRING *strW, const char *src )
{
    ULONG dstlen, srclen = strlen( src ) + 1;

    strW->Buffer = malloc( srclen * sizeof(WCHAR) );
    RtlUTF8ToUnicodeN( strW->Buffer, srclen * sizeof(WCHAR), &dstlen, src, srclen );
    strW->MaximumLength = dstlen;
    strW->Length = dstlen - sizeof(WCHAR);
}

static void principal_to_name_and_realm(char *name_with_realm, char **name, char **realm)
{
    char* separator;

    separator = strchr( name_with_realm, '@' );
    if (!separator)
    {
        ERR( "got a name without a @\n" );
        *name = name_with_realm;
        *realm = *name + strlen(*name); /* point it to a null byte */
        return;
    }

    *separator = '\0';
    *name = name_with_realm;
    *realm = separator + 1; /* character after a @ */
    TRACE( "name: %s, realm: %s\n", debugstr_a(*name), debugstr_a(*realm) );
}

static NTSTATUS copy_tickets_from_cache( krb5_context ctx, krb5_ccache cache, struct ticket_list *list )
{
    NTSTATUS status;
    krb5_cc_cursor cursor;
    krb5_error_code err;
    krb5_creds creds;
    krb5_ticket *ticket;
    char *server_name_with_realm, *server_name, *server_realm;
    char *client_name_with_realm, *client_name, *client_realm;

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
            KERB_TICKET_CACHE_INFO_EX *new_tickets = realloc( list->tickets, sizeof(*new_tickets) * new_allocated );
            if (!new_tickets)
            {
                p_krb5_free_cred_contents( ctx, &creds );
                status = STATUS_NO_MEMORY;
                break;
            }
            list->tickets = new_tickets;
            list->allocated = new_allocated;
        }

        if ((err = p_krb5_unparse_name_flags( ctx, creds.server, 0, &server_name_with_realm )))
        {
            p_krb5_free_cred_contents( ctx, &creds );
            status = krb5_error_to_status( err );
            break;
        }
        TRACE( "server_name_with_realm: %s\n", debugstr_a(server_name_with_realm) );

        principal_to_name_and_realm( server_name_with_realm, &server_name, &server_realm );

        utf8_to_wstr( &list->tickets[list->count].ServerName, server_name );
        utf8_to_wstr( &list->tickets[list->count].ServerRealm, server_realm );

        if ((err = p_krb5_unparse_name_flags( ctx, creds.client, 0, &client_name_with_realm )))
        {
            p_krb5_free_cred_contents( ctx, &creds );
            status = krb5_error_to_status( err );
            break;
        }
        TRACE( "client_name_with_realm: %s\n", debugstr_a(client_name_with_realm) );

        principal_to_name_and_realm( client_name_with_realm, &client_name, &client_realm );

        utf8_to_wstr( &list->tickets[list->count].ClientName, client_name );
        utf8_to_wstr( &list->tickets[list->count].ClientRealm, client_realm );

        if (!creds.times.starttime) creds.times.starttime = creds.times.authtime;

        /* TODO: if krb5_is_config_principal = true */

        /* note: store times as seconds, they will be converted to NT timestamps on the PE side */
        list->tickets[list->count].StartTime.QuadPart = creds.times.starttime;
        list->tickets[list->count].EndTime.QuadPart   = creds.times.endtime;
        list->tickets[list->count].RenewTime.QuadPart = creds.times.renew_till;
        list->tickets[list->count].TicketFlags        = creds.ticket_flags;

        err = p_krb5_decode_ticket( &creds.ticket, &ticket );
        p_krb5_free_unparsed_name( ctx, server_name_with_realm );
        p_krb5_free_unparsed_name( ctx, client_name_with_realm );
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

static NTSTATUS copy_tickets_to_client( struct ticket_list *list, KERB_QUERY_TKT_CACHE_EX_RESPONSE *resp,
                                        ULONG *out_size )
{
    char *client_str;
    ULONG i, size = offsetof( KERB_QUERY_TKT_CACHE_EX_RESPONSE, Tickets[list->count] );

    for (i = 0; i < list->count; i++)
    {
        size += list->tickets[i].ClientRealm.MaximumLength;
        size += list->tickets[i].ClientName.MaximumLength;
        size += list->tickets[i].ServerRealm.MaximumLength;
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
        resp->Tickets[i].ClientRealm.Buffer = (WCHAR *)client_str;
        memcpy( client_str, list->tickets[i].ClientRealm.Buffer, list->tickets[i].ClientRealm.MaximumLength );
        client_str += list->tickets[i].ClientRealm.MaximumLength;
        resp->Tickets[i].ClientName.Buffer = (WCHAR *)client_str;
        memcpy( client_str, list->tickets[i].ClientName.Buffer, list->tickets[i].ClientName.MaximumLength );
        client_str += list->tickets[i].ClientName.MaximumLength;
        resp->Tickets[i].ServerRealm.Buffer = (WCHAR *)client_str;
        memcpy( client_str, list->tickets[i].ServerRealm.Buffer, list->tickets[i].ServerRealm.MaximumLength );
        client_str += list->tickets[i].ServerRealm.MaximumLength;
        resp->Tickets[i].ServerName.Buffer = (WCHAR *)client_str;
        memcpy( client_str, list->tickets[i].ServerName.Buffer, list->tickets[i].ServerName.MaximumLength );
        client_str += list->tickets[i].ServerName.MaximumLength;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS kerberos_fill_ticket_list( struct ticket_list *list )
{
    NTSTATUS status = STATUS_SUCCESS;
    krb5_error_code err;
    krb5_context ctx;
    krb5_cccol_cursor cursor = NULL;
    krb5_ccache cache;

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

        status = copy_tickets_from_cache( ctx, cache, list );
        p_krb5_cc_close( ctx, cache );
        if (status != STATUS_SUCCESS) goto done;
    }

done:
    if (cursor) p_krb5_cccol_cursor_free( ctx, &cursor );
    if (ctx) p_krb5_free_context( ctx );

    return status;
}

static void free_tickets_in_list( struct ticket_list *list )
{
    ULONG i;

    for (i = 0; i < list->count; i++)
    {
        free( list->tickets[i].ClientRealm.Buffer );
        free( list->tickets[i].ClientName.Buffer );
        free( list->tickets[i].ServerRealm.Buffer );
        free( list->tickets[i].ServerName.Buffer );
    }

    free( list->tickets );
}

static NTSTATUS query_ticket_cache( void *args )
{
    struct query_ticket_cache_params *params = args;
    struct ticket_list list = { 0 };
    NTSTATUS status;

    status = kerberos_fill_ticket_list( &list );

    if (status == STATUS_SUCCESS) status = copy_tickets_to_client( &list, params->resp, params->out_size );

    free_tickets_in_list( &list );
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
MAKE_FUNCPTR( gss_inquire_sec_context_by_oid );
MAKE_FUNCPTR( gss_release_buffer );
MAKE_FUNCPTR( gss_release_buffer_set );
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

    LOAD_FUNCPTR( gss_accept_sec_context )
    LOAD_FUNCPTR( gss_acquire_cred )
    LOAD_FUNCPTR( gss_delete_sec_context )
    LOAD_FUNCPTR( gss_display_status )
    LOAD_FUNCPTR( gss_get_mic )
    LOAD_FUNCPTR( gss_import_name )
    LOAD_FUNCPTR( gss_init_sec_context )
    LOAD_FUNCPTR( gss_inquire_context )
    LOAD_FUNCPTR( gss_inquire_sec_context_by_oid )
    LOAD_FUNCPTR( gss_release_buffer )
    LOAD_FUNCPTR( gss_release_buffer_set )
    LOAD_FUNCPTR( gss_release_cred )
    LOAD_FUNCPTR( gss_release_iov_buffer )
    LOAD_FUNCPTR( gss_release_name )
    LOAD_FUNCPTR( gss_unwrap )
    LOAD_FUNCPTR( gss_unwrap_iov )
    LOAD_FUNCPTR( gss_verify_mic )
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

static inline gss_ctx_id_t ctxhandle_sspi_to_gss( UINT64 handle )
{
    return (gss_ctx_id_t)(ULONG_PTR)handle;
}

static inline gss_cred_id_t credhandle_sspi_to_gss( UINT64 handle )
{
    return (gss_cred_id_t)(ULONG_PTR)handle;
}

static inline void ctxhandle_gss_to_sspi( gss_ctx_id_t handle, UINT64 *ctx )
{
    *ctx = (ULONG_PTR)handle;
}

static inline void credhandle_gss_to_sspi( gss_cred_id_t handle, UINT64 *cred )
{
    *cred = (ULONG_PTR)handle;
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

    input_token.length  = params->input_token_length;
    input_token.value   = params->input_token;
    output_token.length = 0;
    output_token.value  = NULL;

    ret = pgss_accept_sec_context( &minor_status, &ctx_handle, cred_handle, &input_token, GSS_C_NO_CHANNEL_BINDINGS,
                                   NULL, NULL, &output_token, &ret_flags, &expiry_time, NULL );
    TRACE( "gss_accept_sec_context returned %#x minor status %#x ret_flags %#x\n", ret, minor_status, ret_flags );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE || ret == GSS_S_CONTINUE_NEEDED)
    {
        if (output_token.length > *params->output_token_length) /* FIXME: check if larger buffer exists */
        {
            TRACE( "buffer too small %lu > %u\n",
                   (SIZE_T)output_token.length, (unsigned int)*params->output_token_length );
            pgss_release_buffer( &minor_status, &output_token );
            pgss_delete_sec_context( &minor_status, &ctx_handle, GSS_C_NO_BUFFER );
            return SEC_E_BUFFER_TOO_SMALL;
        }
        *params->output_token_length = output_token.length;
        memcpy( params->output_token, output_token.value, output_token.length );
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

    case SECPKG_CRED_BOTH:
    case SECPKG_CRED_OUTBOUND:
        if ((status = init_creds( params->username, params->password )) != STATUS_SUCCESS) return status;
        cred_usage = params->credential_use == SECPKG_CRED_OUTBOUND ? GSS_C_INITIATE : GSS_C_BOTH;
        break;

    default:
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
    const struct delete_context_params *params = args;
    OM_uint32 ret, minor_status;
    gss_ctx_id_t ctx_handle = ctxhandle_sspi_to_gss( params->context );

    ret = pgss_delete_sec_context( &minor_status, &ctx_handle, GSS_C_NO_BUFFER );
    TRACE( "gss_delete_sec_context returned %#x minor status %#x\n", ret, minor_status );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    return status_gss_to_sspi( ret );
}

static NTSTATUS free_credentials_handle( void *args )
{
    const struct free_credentials_handle_params *params = args;
    OM_uint32 ret, minor_status;
    gss_cred_id_t cred = credhandle_sspi_to_gss( params->credential );

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

    input_token.length = params->input_token_length;
    input_token.value  = params->input_token;
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
        if (output_token.length > *params->output_token_length) /* FIXME: check if larger buffer exists */
        {
            TRACE( "buffer too small %lu > %u\n", (SIZE_T)output_token.length, (unsigned int)*params->output_token_length);
            pgss_release_buffer( &minor_status, &output_token );
            pgss_delete_sec_context( &minor_status, &ctx_handle, GSS_C_NO_BUFFER );
            if (target != GSS_C_NO_NAME) pgss_release_name( &minor_status, &target );
            return SEC_E_INCOMPLETE_MESSAGE;
        }
        *params->output_token_length = output_token.length;
        memcpy( params->output_token, output_token.value, output_token.length );
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
    OM_uint32 ret, minor_status;
    gss_buffer_desc data_buffer, token_buffer;
    gss_ctx_id_t ctx_handle = ctxhandle_sspi_to_gss( params->context );

    data_buffer.length  = params->data_length;
    data_buffer.value   = params->data;
    token_buffer.length = 0;
    token_buffer.value  = NULL;

    ret = pgss_get_mic( &minor_status, ctx_handle, GSS_C_QOP_DEFAULT, &data_buffer, &token_buffer );
    TRACE( "gss_get_mic returned %#x minor status %#x\n", ret, minor_status );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE)
    {
        memcpy( params->token, token_buffer.value, token_buffer.length );
        *params->token_length = token_buffer.length;
        pgss_release_buffer( &minor_status, &token_buffer );
    }

    return status_gss_to_sspi( ret );
}

#define KERBEROS_MAX_SIGNATURE        64
#define KERBEROS_SECURITY_TRAILER     64
#define KERBEROS_MAX_SIGNATURE_DCE    28
#define KERBEROS_SECURITY_TRAILER_DCE 76

static NTSTATUS get_session_key( gss_ctx_id_t ctx, SecPkgContext_SessionKey *key )
{
    gss_OID_desc GSS_C_INQ_SSPI_SESSION_KEY =
        { 11, (void *)"\x2a\x86\x48\x86\xf7\x12\x01\x02\x02\x05\x05" }; /* 1.2.840.113554.1.2.2.5.5 */
    OM_uint32 ret, minor_status;
    gss_buffer_set_t buffer_set = GSS_C_NO_BUFFER_SET;

    ret = pgss_inquire_sec_context_by_oid( &minor_status, ctx, &GSS_C_INQ_SSPI_SESSION_KEY, &buffer_set );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    if (ret != GSS_S_COMPLETE) return STATUS_INTERNAL_ERROR;

    if (buffer_set == GSS_C_NO_BUFFER_SET || buffer_set->count != 2)
    {
        pgss_release_buffer_set( &minor_status, &buffer_set );
        return STATUS_INTERNAL_ERROR;
    }

    if (key->SessionKeyLength < buffer_set->elements[0].length )
    {
        key->SessionKeyLength = buffer_set->elements[0].length;
        pgss_release_buffer_set( &minor_status, &buffer_set );
        return STATUS_BUFFER_TOO_SMALL;
    }

    memcpy( key->SessionKey, buffer_set->elements[0].value, buffer_set->elements[0].length );
    key->SessionKeyLength = buffer_set->elements[0].length;

    pgss_release_buffer_set( &minor_status, &buffer_set );
    return STATUS_SUCCESS;
}

static NTSTATUS query_context_attributes( void *args )
{
    struct query_context_attributes_params *params = args;
    switch (params->attr)
    {
    case SECPKG_ATTR_SESSION_KEY:
    {
        SecPkgContext_SessionKey *key = (SecPkgContext_SessionKey *)params->buf;
        gss_ctx_id_t ctx  = ctxhandle_sspi_to_gss( params->context );

        return get_session_key( ctx, key );
    }
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
        sizes->cbBlockSize       = 8;
        sizes->cbSecurityTrailer = size_security_trailer;
        return SEC_E_OK;
    }
    default:
        FIXME( "unhandled attribute %u\n", params->attr );
        break;
    }

    return SEC_E_UNSUPPORTED_FUNCTION;
}

static NTSTATUS seal_message_vector( gss_ctx_id_t ctx, const struct seal_message_params *params )
{
    gss_iov_buffer_desc iov[4];
    OM_uint32 ret, minor_status;
    int conf_flag, conf_state;

    if (!params->qop)
        conf_flag = 1; /* confidentiality + integrity */
    else if (params->qop == SECQOP_WRAP_NO_ENCRYPT)
        conf_flag = 0; /* only integrity */
    else
    {
        FIXME( "QOP %#x not supported\n", params->qop );
        return SEC_E_UNSUPPORTED_FUNCTION;
    }

    iov[0].type          = GSS_IOV_BUFFER_TYPE_SIGN_ONLY | GSS_IOV_BUFFER_FLAG_ALLOCATE;
    iov[0].buffer.length = 0;
    iov[0].buffer.value  = NULL;

    iov[1].type          = GSS_IOV_BUFFER_TYPE_DATA;
    iov[1].buffer.length = params->data_length;
    iov[1].buffer.value  = params->data;

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
        memcpy( params->token, iov[3].buffer.value, iov[3].buffer.length );
        *params->token_length = iov[3].buffer.length;
        pgss_release_iov_buffer( &minor_status, iov, 4 );
    }

    return status_gss_to_sspi( ret );
}

static NTSTATUS seal_message_no_vector( gss_ctx_id_t ctx, const struct seal_message_params *params )
{
    gss_buffer_desc input, output;
    OM_uint32 ret, minor_status;
    int conf_flag, conf_state;

    if (!params->qop)
        conf_flag = 1; /* confidentiality + integrity */
    else if (params->qop == SECQOP_WRAP_NO_ENCRYPT)
        conf_flag = 0; /* only integrity */
    else
    {
        FIXME( "QOP %#x not supported\n", params->qop );
        return SEC_E_UNSUPPORTED_FUNCTION;
    }

    input.length = params->data_length;
    input.value  = params->data;

    ret = pgss_wrap( &minor_status, ctx, conf_flag, GSS_C_QOP_DEFAULT, &input, &conf_state, &output );
    TRACE( "gss_wrap returned %#x minor status %#x\n", ret, minor_status );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE)
    {
        unsigned len_data = params->data_length, len_token = *params->token_length;
        if (len_token < output.length - len_data)
        {
            TRACE( "buffer too small %lu > %u\n", (SIZE_T)output.length - len_data, len_token );
            pgss_release_buffer( &minor_status, &output );
            return SEC_E_BUFFER_TOO_SMALL;
        }
        memcpy( params->data, output.value, len_data );
        memcpy( params->token, (char *)output.value + len_data, output.length - len_data );
        *params->token_length = output.length - len_data;
        pgss_release_buffer( &minor_status, &output );
    }

    return status_gss_to_sspi( ret );
}

static NTSTATUS seal_message( void *args )
{
    struct seal_message_params *params = args;
    gss_ctx_id_t ctx = ctxhandle_sspi_to_gss( params->context );

    if (is_dce_style_context( ctx )) return seal_message_vector( ctx, params );
    return seal_message_no_vector( ctx, params );
}

static NTSTATUS unseal_message_vector( gss_ctx_id_t ctx, const struct unseal_message_params *params )
{
    gss_iov_buffer_desc iov[4];
    OM_uint32 ret, minor_status;
    int conf_state;

    if (params->stream_length)
    {
        iov[0].type          = GSS_IOV_BUFFER_TYPE_STREAM;
        iov[0].buffer.length = params->stream_length;
        iov[0].buffer.value  = malloc( params->stream_length );
        if (!iov[0].buffer.value) return STATUS_NO_MEMORY;
        memcpy( iov[0].buffer.value, params->stream, params->stream_length );

        iov[1].type          = GSS_IOV_BUFFER_TYPE_DATA;
        iov[1].buffer.length = 0;
        iov[1].buffer.value  = NULL;

        ret = pgss_unwrap_iov( &minor_status, ctx, &conf_state, NULL, iov, 2 );
        TRACE( "gss_unwrap_iov returned %#x minor status %#x\n", ret, minor_status );
        if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
        if (ret == GSS_S_COMPLETE)
        {
            if (params->data_length < iov[1].buffer.length)
            {
                free( iov[0].buffer.value );
                return SEC_E_BUFFER_TOO_SMALL;
            }

            memcpy( params->data, iov[1].buffer.value, iov[1].buffer.length );
            if (params->qop)
                *params->qop = conf_state ? 0 : SECQOP_WRAP_NO_ENCRYPT;
        }
        free( iov[0].buffer.value );
        return status_gss_to_sspi( ret );
    }

    iov[0].type          = GSS_IOV_BUFFER_TYPE_SIGN_ONLY;
    iov[0].buffer.length = 0;
    iov[0].buffer.value  = NULL;

    iov[1].type          = GSS_IOV_BUFFER_TYPE_DATA;
    iov[1].buffer.length = params->data_length;
    iov[1].buffer.value  = params->data;

    iov[2].type          = GSS_IOV_BUFFER_TYPE_SIGN_ONLY;
    iov[2].buffer.length = 0;
    iov[2].buffer.value  = NULL;

    iov[3].type          = GSS_IOV_BUFFER_TYPE_HEADER;
    iov[3].buffer.length = params->token_length;
    iov[3].buffer.value  = params->token;

    ret = pgss_unwrap_iov( &minor_status, ctx, &conf_state, NULL, iov, 4 );
    TRACE( "gss_unwrap_iov returned %#x minor status %#x\n", ret, minor_status );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE && params->qop)
    {
        *params->qop = (conf_state ? 0 : SECQOP_WRAP_NO_ENCRYPT);
    }
    return status_gss_to_sspi( ret );
}

static NTSTATUS unseal_message_no_vector( gss_ctx_id_t ctx, const struct unseal_message_params *params )
{
    gss_buffer_desc input, output;
    OM_uint32 ret, minor_status;
    int conf_state;

    if (params->stream_length)
    {
        input.length = params->stream_length;
        input.value = params->stream;
    }
    else
    {
        DWORD len_data, len_token;

        len_data = params->data_length;
        len_token = params->token_length;
        input.length = len_data + len_token;
        if (!(input.value = malloc( input.length ))) return STATUS_NO_MEMORY;
        memcpy( input.value, params->data, len_data );
        memcpy( (char *)input.value + len_data, params->token, len_token );
    }

    ret = pgss_unwrap( &minor_status, ctx, &input, &output, &conf_state, NULL );
    if (input.value != params->stream) free( input.value );
    TRACE( "gss_unwrap returned %#x minor status %#x\n", ret, minor_status );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE)
    {
        if (params->qop) *params->qop = (conf_state ? 0 : SECQOP_WRAP_NO_ENCRYPT);
        memcpy( params->data, output.value, output.length );
        pgss_release_buffer( &minor_status, &output );
    }

    return status_gss_to_sspi( ret );
}

static NTSTATUS unseal_message( void *args )
{
    struct unseal_message_params *params = args;
    gss_ctx_id_t ctx = ctxhandle_sspi_to_gss( params->context );

    if (is_dce_style_context( ctx )) return unseal_message_vector( ctx, params );
    return unseal_message_no_vector( ctx, params );
}

static NTSTATUS verify_signature( void *args )
{
    struct verify_signature_params *params = args;
    OM_uint32 ret, minor_status;
    gss_buffer_desc data_buffer, token_buffer;
    gss_ctx_id_t ctx_handle = ctxhandle_sspi_to_gss( params->context );

    data_buffer.length  = params->data_length;
    data_buffer.value   = params->data;
    token_buffer.length = params->token_length;
    token_buffer.value  = params->token;

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

const unixlib_entry_t __wine_unix_call_funcs[] =
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

C_ASSERT( ARRAYSIZE(__wine_unix_call_funcs) == unix_funcs_count );

#ifdef _WIN64

typedef ULONG PTR32;

static NTSTATUS wow64_accept_context( void *args )
{
    struct
    {
        UINT64 credential;
        UINT64 context;
        PTR32 input_token;
        ULONG input_token_length;
        PTR32 new_context;
        PTR32 output_token;
        PTR32 output_token_length;
        PTR32 context_attr;
        PTR32 expiry;
    } const *params32 = args;
    struct accept_context_params params =
    {
        params32->credential,
        params32->context,
        ULongToPtr(params32->input_token),
        params32->input_token_length,
        ULongToPtr(params32->new_context),
        ULongToPtr(params32->output_token),
        ULongToPtr(params32->output_token_length),
        ULongToPtr(params32->context_attr),
        ULongToPtr(params32->expiry),
    };
    return accept_context( &params );
}

static NTSTATUS wow64_acquire_credentials_handle( void *args )
{
    struct
    {
        PTR32 principal;
        ULONG credential_use;
        PTR32 username;
        PTR32 password;
        PTR32 credential;
        PTR32 expiry;
    } const *params32 = args;
    struct acquire_credentials_handle_params params =
    {
        ULongToPtr(params32->principal),
        params32->credential_use,
        ULongToPtr(params32->username),
        ULongToPtr(params32->password),
        ULongToPtr(params32->credential),
        ULongToPtr(params32->expiry),
    };
    return acquire_credentials_handle( &params );
}

static NTSTATUS wow64_delete_context( void *args )
{
    struct
    {
        UINT64 context;
    } const *params32 = args;
    struct delete_context_params params =
    {
        params32->context,
    };
    return delete_context( &params );
}

static NTSTATUS wow64_free_credentials_handle( void *args )
{
    struct
    {
        UINT64 credential;
    } const *params32 = args;
    struct free_credentials_handle_params params =
    {
        params32->credential,
    };
    return free_credentials_handle( &params );
}

static NTSTATUS wow64_initialize_context( void *args )
{
    struct
    {
        UINT64 credential;
        UINT64 context;
        PTR32 target_name;
        ULONG context_req;
        PTR32 input_token;
        ULONG input_token_length;
        PTR32 output_token;
        PTR32 output_token_length;
        PTR32 new_context;
        PTR32 context_attr;
        PTR32 expiry;
    } const *params32 = args;
    struct initialize_context_params params =
    {
        params32->credential,
        params32->context,
        ULongToPtr(params32->target_name),
        params32->context_req,
        ULongToPtr(params32->input_token),
        params32->input_token_length,
        ULongToPtr(params32->output_token),
        ULongToPtr(params32->output_token_length),
        ULongToPtr(params32->new_context),
        ULongToPtr(params32->context_attr),
        ULongToPtr(params32->expiry),
    };
    return initialize_context( &params );
}

static NTSTATUS wow64_make_signature( void *args )
{
    struct
    {
        UINT64 context;
        PTR32 data;
        ULONG data_length;
        PTR32 token;
        PTR32 token_length;
    } const *params32 = args;
    struct make_signature_params params =
    {
        params32->context,
        ULongToPtr(params32->data),
        params32->data_length,
        ULongToPtr(params32->token),
        ULongToPtr(params32->token_length),
    };
    return make_signature( &params );
}

static NTSTATUS wow64_query_context_attributes( void *args )
{
    struct
    {
        UINT64 context;
        ULONG attr;
        PTR32 buf;
    } const *params32 = args;
    struct query_context_attributes_params params =
    {
        params32->context,
        params32->attr,
        ULongToPtr(params32->buf),
    };
    return query_context_attributes( &params );
}

struct KERB_TICKET_CACHE_INFO_EX32
{
    UNICODE_STRING32 ClientName;
    UNICODE_STRING32 ClientRealm;
    UNICODE_STRING32 ServerName;
    UNICODE_STRING32 ServerRealm;
    LARGE_INTEGER StartTime;
    LARGE_INTEGER EndTime;
    LARGE_INTEGER RenewTime;
    LONG EncryptionType;
    ULONG TicketFlags;
};

struct KERB_QUERY_TKT_CACHE_EX_RESPONSE32
{
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    ULONG CountOfTickets;
    struct KERB_TICKET_CACHE_INFO_EX32 Tickets[ANYSIZE_ARRAY];
};

static void copy_ticket_ustr_64to32( const UNICODE_STRING *str, UNICODE_STRING32 *str32, ULONG *client_str )
{
    str32->Length = str->Length;
    str32->MaximumLength = str->MaximumLength;
    str32->Buffer = *client_str;
    memcpy( ULongToPtr(str32->Buffer), str->Buffer, str->MaximumLength );
    *client_str += str->MaximumLength;
}

static NTSTATUS copy_tickets_to_client32( struct ticket_list *list, struct KERB_QUERY_TKT_CACHE_EX_RESPONSE32 *resp,
        ULONG *out_size )
{
    ULONG i, size, size_fixed;
    ULONG client_str;

    size = size_fixed = offsetof( struct KERB_QUERY_TKT_CACHE_EX_RESPONSE32, Tickets[list->count] );
    for (i = 0; i < list->count; i++)
    {
        size += list->tickets[i].ServerRealm.MaximumLength;
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
    client_str = PtrToUlong(resp) + size_fixed;

    for (i = 0; i < list->count; i++)
    {
        copy_ticket_ustr_64to32( &list->tickets[i].ServerName, &resp->Tickets[i].ServerName, &client_str );
        copy_ticket_ustr_64to32( &list->tickets[i].ServerRealm, &resp->Tickets[i].ServerRealm, &client_str );
        resp->Tickets[i].StartTime = list->tickets[i].StartTime;
        resp->Tickets[i].EndTime = list->tickets[i].EndTime;
        resp->Tickets[i].RenewTime = list->tickets[i].RenewTime;
        resp->Tickets[i].EncryptionType = list->tickets[i].EncryptionType;
        resp->Tickets[i].TicketFlags = list->tickets[i].TicketFlags;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS wow64_query_ticket_cache( void *args )
{
    struct
    {
        PTR32 resp;
        PTR32 out_size;
    } const *params32 = args;
    struct ticket_list list = { 0 };
    NTSTATUS status;

    status = kerberos_fill_ticket_list( &list );
    if (status == STATUS_SUCCESS)
        status = copy_tickets_to_client32( &list, ULongToPtr(params32->resp), ULongToPtr(params32->out_size) );

    free_tickets_in_list( &list );
    return status;

}

static NTSTATUS wow64_seal_message( void *args )
{
    struct
    {
        UINT64 context;
        PTR32 data;
        ULONG data_length;
        PTR32 token;
        PTR32 token_length;
    } const *params32 = args;
    struct seal_message_params params =
    {
        params32->context,
        ULongToPtr(params32->data),
        params32->data_length,
        ULongToPtr(params32->token),
        ULongToPtr(params32->token_length),
    };
    return seal_message( &params );
}

static NTSTATUS wow64_unseal_message( void *args )
{
    struct
    {
        UINT64 context;
        PTR32 stream;
        ULONG stream_length;
        PTR32 data;
        ULONG data_length;
        PTR32 token;
        ULONG token_length;
        PTR32 qop;
    } const *params32 = args;
    struct unseal_message_params params =
    {
        params32->context,
        ULongToPtr(params32->stream),
        params32->stream_length,
        ULongToPtr(params32->data),
        params32->data_length,
        ULongToPtr(params32->token),
        params32->token_length,
        ULongToPtr(params32->qop),
    };
    return unseal_message( &params );
}

static NTSTATUS wow64_verify_signature( void *args )
{
    struct
    {
        UINT64 context;
        PTR32 data;
        ULONG data_length;
        PTR32 token;
        ULONG token_length;
        PTR32 qop;
    } const *params32 = args;
    struct verify_signature_params params =
    {
        params32->context,
        ULongToPtr(params32->data),
        params32->data_length,
        ULongToPtr(params32->token),
        params32->token_length,
        ULongToPtr(params32->qop),
    };
    return verify_signature( &params );
}

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    process_attach,
    wow64_accept_context,
    wow64_acquire_credentials_handle,
    wow64_delete_context,
    wow64_free_credentials_handle,
    wow64_initialize_context,
    wow64_make_signature,
    wow64_query_context_attributes,
    wow64_query_ticket_cache,
    wow64_seal_message,
    wow64_unseal_message,
    wow64_verify_signature,
};

C_ASSERT( ARRAYSIZE(__wine_unix_call_wow64_funcs) == unix_funcs_count );

#endif /* _WIN64 */

#endif /* defined(SONAME_LIBKRB5) && defined(SONAME_LIBGSSAPI_KRB5) */
