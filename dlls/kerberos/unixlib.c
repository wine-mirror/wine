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
#include "wine/port.h"

#include <stdarg.h>
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

static WCHAR *utf8_to_wstr( const char *src )
{
    ULONG dstlen, srclen = strlen( src ) + 1;
    WCHAR *dst;

    RtlUTF8ToUnicodeN( NULL, 0, &dstlen, src, srclen );
    if ((dst = RtlAllocateHeap( GetProcessHeap(), 0, dstlen )))
        RtlUTF8ToUnicodeN( dst, dstlen, &dstlen, src, srclen );
    return dst;
}

static NTSTATUS copy_tickets_from_cache( krb5_context ctx, krb5_ccache cache, struct ticket_list *list )
{
    NTSTATUS status;
    krb5_cc_cursor cursor;
    krb5_error_code err;
    krb5_creds creds;
    krb5_ticket *ticket;
    char *name_with_realm, *name_without_realm, *realm_name;
    WCHAR *realm_nameW, *name_without_realmW;

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
            KERB_TICKET_CACHE_INFO *new_tickets;
            ULONG new_allocated;

            if (list->allocated)
            {
                new_allocated = list->allocated * 2;
                new_tickets = RtlReAllocateHeap( GetProcessHeap(), 0, list->tickets, sizeof(*new_tickets) * new_allocated );
            }
            else
            {
                new_allocated = 16;
                new_tickets = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*new_tickets) * new_allocated );
            }
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

        name_without_realmW = utf8_to_wstr( name_without_realm );
        RtlInitUnicodeString( &list->tickets[list->count].ServerName, name_without_realmW );

        if (!(realm_name = strchr( name_with_realm, '@' )))
        {
            ERR( "wrong name with realm %s\n", debugstr_a(name_with_realm) );
            realm_name = name_with_realm;
        }
        else realm_name++;

        /* realm_name - now contains only realm! */
        realm_nameW = utf8_to_wstr( realm_name );
        RtlInitUnicodeString( &list->tickets[list->count].RealmName, realm_nameW );

        if (!creds.times.starttime) creds.times.starttime = creds.times.authtime;

        /* TODO: if krb5_is_config_principal = true */
        RtlSecondsSince1970ToTime( creds.times.starttime, &list->tickets[list->count].StartTime );
        RtlSecondsSince1970ToTime( creds.times.endtime, &list->tickets[list->count].EndTime );
        RtlSecondsSince1970ToTime( creds.times.renew_till, &list->tickets[list->count].RenewTime );

        list->tickets[list->count].TicketFlags = creds.ticket_flags;

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

static NTSTATUS CDECL query_ticket_cache( struct ticket_list *list )
{
    NTSTATUS status;
    krb5_error_code err;
    krb5_context ctx;
    krb5_cccol_cursor cursor = NULL;
    krb5_ccache cache;

    list->count     = 0;
    list->allocated = 0;
    list->tickets   = NULL;

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
        FIXME( "couldn't convert status 0x%08x to NTSTATUS\n", status );
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
            TRACE( "gss_display_status(0x%08x,%d) returned %08x minor status %08x\n", code, type, ret, minor_status );
            return;
        }
        TRACE( "GSS-API error: 0x%08x: %s\n", code, debugstr_an(buf.value, buf.length) );
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

static void expirytime_gss_to_sspi( OM_uint32 expirytime, TimeStamp *timestamp )
{
    LARGE_INTEGER time;

    NtQuerySystemTime( &time );
    RtlSystemTimeToLocalTime( &time, &time );
    timestamp->LowPart  = time.QuadPart;
    timestamp->HighPart = time.QuadPart >> 32;
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

static NTSTATUS CDECL accept_context( LSA_SEC_HANDLE credential, LSA_SEC_HANDLE context, SecBufferDesc *input,
                               LSA_SEC_HANDLE *new_context, SecBufferDesc *output, ULONG *context_attr,
                               TimeStamp *expiry )
{
    OM_uint32 ret, minor_status, ret_flags = 0, expiry_time;
    gss_cred_id_t cred_handle = credhandle_sspi_to_gss( credential );
    gss_ctx_id_t ctx_handle = ctxhandle_sspi_to_gss( context );
    gss_buffer_desc input_token, output_token;
    int idx;

    if (!input) input_token.length = 0;
    else
    {
        if ((idx = get_buffer_index( input, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;
        input_token.length = input->pBuffers[idx].cbBuffer;
        input_token.value  = input->pBuffers[idx].pvBuffer;
    }

    if ((idx = get_buffer_index( output, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;
    output_token.length = 0;
    output_token.value  = NULL;

    ret = pgss_accept_sec_context( &minor_status, &ctx_handle, cred_handle, &input_token, GSS_C_NO_CHANNEL_BINDINGS,
                                   NULL, NULL, &output_token, &ret_flags, &expiry_time, NULL );
    TRACE( "gss_accept_sec_context returned %08x minor status %08x ret_flags %08x\n", ret, minor_status, ret_flags );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE || ret == GSS_S_CONTINUE_NEEDED)
    {
        if (output_token.length > output->pBuffers[idx].cbBuffer) /* FIXME: check if larger buffer exists */
        {
            TRACE( "buffer too small %lu > %u\n", (SIZE_T)output_token.length, output->pBuffers[idx].cbBuffer );
            pgss_release_buffer( &minor_status, &output_token );
            pgss_delete_sec_context( &minor_status, &ctx_handle, GSS_C_NO_BUFFER );
            return SEC_E_BUFFER_TOO_SMALL;
        }
        output->pBuffers[idx].cbBuffer = output_token.length;
        memcpy( output->pBuffers[idx].pvBuffer, output_token.value, output_token.length );
        pgss_release_buffer( &minor_status, &output_token );

        ctxhandle_gss_to_sspi( ctx_handle, new_context );
        if (context_attr) *context_attr = flags_gss_to_asc_ret( ret_flags );
        expirytime_gss_to_sspi( expiry_time, expiry );
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
    TRACE( "gss_import_name returned %08x minor status %08x\n", ret, minor_status );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    return status_gss_to_sspi( ret );
}

static NTSTATUS CDECL acquire_credentials_handle( const char *principal, ULONG credential_use, const char *username,
                                                  const char *password, LSA_SEC_HANDLE *credential, TimeStamp *expiry )
{
    OM_uint32 ret, minor_status, expiry_time;
    gss_name_t name = GSS_C_NO_NAME;
    gss_cred_usage_t cred_usage;
    gss_cred_id_t cred_handle;
    NTSTATUS status;

    switch (credential_use)
    {
    case SECPKG_CRED_INBOUND:
        cred_usage = GSS_C_ACCEPT;
        break;

    case SECPKG_CRED_OUTBOUND:
        if ((status = init_creds( username, password )) != STATUS_SUCCESS) return status;
        cred_usage = GSS_C_INITIATE;
        break;

    default:
        FIXME( "SECPKG_CRED_BOTH not supported\n" );
        return SEC_E_UNKNOWN_CREDENTIALS;
    }

    if (principal && (status = import_name( principal, &name ))) return status;

    ret = pgss_acquire_cred( &minor_status, name, GSS_C_INDEFINITE, GSS_C_NULL_OID_SET, cred_usage, &cred_handle,
                             NULL, &expiry_time );
    TRACE( "gss_acquire_cred returned %08x minor status %08x\n", ret, minor_status );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE)
    {
        credhandle_gss_to_sspi( cred_handle, credential );
        expirytime_gss_to_sspi( expiry_time, expiry );
    }

    if (name != GSS_C_NO_NAME) pgss_release_name( &minor_status, &name );
    return status_gss_to_sspi( ret );
}

static NTSTATUS CDECL delete_context( LSA_SEC_HANDLE context )
{
    OM_uint32 ret, minor_status;
    gss_ctx_id_t ctx_handle = ctxhandle_sspi_to_gss( context );

    ret = pgss_delete_sec_context( &minor_status, &ctx_handle, GSS_C_NO_BUFFER );
    TRACE( "gss_delete_sec_context returned %08x minor status %08x\n", ret, minor_status );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    return status_gss_to_sspi( ret );
}

static NTSTATUS CDECL free_credentials_handle( LSA_SEC_HANDLE handle )
{
    OM_uint32 ret, minor_status;
    gss_cred_id_t cred = credhandle_sspi_to_gss( handle );

    ret = pgss_release_cred( &minor_status, &cred );
    TRACE( "gss_release_cred returned %08x minor status %08x\n", ret, minor_status );
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

static NTSTATUS CDECL initialize_context( LSA_SEC_HANDLE credential, LSA_SEC_HANDLE context, const char *target_name,
                                          ULONG context_req, SecBufferDesc *input, LSA_SEC_HANDLE *new_context,
                                          SecBufferDesc *output, ULONG *context_attr, TimeStamp *expiry )
{
    OM_uint32 ret, minor_status, ret_flags = 0, expiry_time, req_flags = flags_isc_req_to_gss( context_req );
    gss_cred_id_t cred_handle = credhandle_sspi_to_gss( credential );
    gss_ctx_id_t ctx_handle = ctxhandle_sspi_to_gss( context );
    gss_buffer_desc input_token, output_token;
    gss_name_t target = GSS_C_NO_NAME;
    NTSTATUS status;
    int idx;

    if ((idx = get_buffer_index( input, SECBUFFER_TOKEN )) == -1) input_token.length = 0;
    else
    {
        input_token.length = input->pBuffers[idx].cbBuffer;
        input_token.value  = input->pBuffers[idx].pvBuffer;
    }

    if ((idx = get_buffer_index( output, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;
    output_token.length = 0;
    output_token.value  = NULL;

    if (target_name && (status = import_name( target_name, &target ))) return status;

    ret = pgss_init_sec_context( &minor_status, cred_handle, &ctx_handle, target, GSS_C_NO_OID, req_flags, 0,
                                 GSS_C_NO_CHANNEL_BINDINGS, &input_token, NULL, &output_token, &ret_flags,
                                 &expiry_time );
    TRACE( "gss_init_sec_context returned %08x minor status %08x ret_flags %08x\n", ret, minor_status, ret_flags );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE || ret == GSS_S_CONTINUE_NEEDED)
    {
        if (output_token.length > output->pBuffers[idx].cbBuffer) /* FIXME: check if larger buffer exists */
        {
            TRACE( "buffer too small %lu > %u\n", (SIZE_T)output_token.length, output->pBuffers[idx].cbBuffer );
            pgss_release_buffer( &minor_status, &output_token );
            pgss_delete_sec_context( &minor_status, &ctx_handle, GSS_C_NO_BUFFER );
            return SEC_E_INCOMPLETE_MESSAGE;
        }
        output->pBuffers[idx].cbBuffer = output_token.length;
        memcpy( output->pBuffers[idx].pvBuffer, output_token.value, output_token.length );
        pgss_release_buffer( &minor_status, &output_token );

        ctxhandle_gss_to_sspi( ctx_handle, new_context );
        if (context_attr) *context_attr = flags_gss_to_isc_ret( ret_flags );
        expirytime_gss_to_sspi( expiry_time, expiry );
    }

    if (target != GSS_C_NO_NAME) pgss_release_name( &minor_status, &target );
    return status_gss_to_sspi( ret );
}

static NTSTATUS CDECL make_signature( LSA_SEC_HANDLE context, SecBufferDesc *msg )
{
    OM_uint32 ret, minor_status;
    gss_buffer_desc data_buffer, token_buffer;
    gss_ctx_id_t ctx_handle = ctxhandle_sspi_to_gss( context );
    int data_idx, token_idx;

    /* FIXME: multiple data buffers, read-only buffers */
    if ((data_idx = get_buffer_index( msg, SECBUFFER_DATA )) == -1) return SEC_E_INVALID_TOKEN;
    data_buffer.length = msg->pBuffers[data_idx].cbBuffer;
    data_buffer.value  = msg->pBuffers[data_idx].pvBuffer;

    if ((token_idx = get_buffer_index( msg, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;
    token_buffer.length = 0;
    token_buffer.value  = NULL;

    ret = pgss_get_mic( &minor_status, ctx_handle, GSS_C_QOP_DEFAULT, &data_buffer, &token_buffer );
    TRACE( "gss_get_mic returned %08x minor status %08x\n", ret, minor_status );
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

static NTSTATUS CDECL query_context_attributes( LSA_SEC_HANDLE context, ULONG attr, void *buf )
{
    switch (attr)
    {
    case SECPKG_ATTR_SIZES:
    {
        SecPkgContext_Sizes *sizes = (SecPkgContext_Sizes *)buf;
        ULONG size_max_signature, size_security_trailer;
        gss_ctx_id_t ctx  = ctxhandle_sspi_to_gss( context );

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
        FIXME( "unhandled attribute %u\n", attr );
        break;
    }

    return SEC_E_UNSUPPORTED_FUNCTION;
}

static NTSTATUS seal_message_vector( gss_ctx_id_t ctx, SecBufferDesc *msg, ULONG qop )
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
        FIXME( "QOP %08x not supported\n", qop );
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
    TRACE( "gss_wrap_iov returned %08x minor status %08x\n", ret, minor_status );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE)
    {
        memcpy( msg->pBuffers[token_idx].pvBuffer, iov[3].buffer.value, iov[3].buffer.length );
        msg->pBuffers[token_idx].cbBuffer = iov[3].buffer.length;
        pgss_release_iov_buffer( &minor_status, iov, 4 );
    }

    return status_gss_to_sspi( ret );
}

static NTSTATUS seal_message_no_vector( gss_ctx_id_t ctx, SecBufferDesc *msg, ULONG qop )
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
        FIXME( "QOP %08x not supported\n", qop );
        return SEC_E_UNSUPPORTED_FUNCTION;
    }

    /* FIXME: multiple data buffers, read-only buffers */
    if ((data_idx = get_buffer_index( msg, SECBUFFER_DATA )) == -1) return SEC_E_INVALID_TOKEN;
    if ((token_idx = get_buffer_index( msg, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;

    input.length = msg->pBuffers[data_idx].cbBuffer;
    input.value  = msg->pBuffers[data_idx].pvBuffer;

    ret = pgss_wrap( &minor_status, ctx, conf_flag, GSS_C_QOP_DEFAULT, &input, &conf_state, &output );
    TRACE( "gss_wrap returned %08x minor status %08x\n", ret, minor_status );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE)
    {
        DWORD len_data = msg->pBuffers[data_idx].cbBuffer, len_token = msg->pBuffers[token_idx].cbBuffer;
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

static NTSTATUS CDECL seal_message( LSA_SEC_HANDLE context, SecBufferDesc *msg, ULONG qop )
{
    gss_ctx_id_t ctx = ctxhandle_sspi_to_gss( context );

    if (is_dce_style_context( ctx )) return seal_message_vector( ctx, msg, qop );
    return seal_message_no_vector( ctx, msg, qop );
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
    TRACE( "gss_unwrap_iov returned %08x minor status %08x\n", ret, minor_status );
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
    if (!(input.value = RtlAllocateHeap( GetProcessHeap(), 0, input.length ))) return SEC_E_INSUFFICIENT_MEMORY;
    memcpy( input.value, msg->pBuffers[data_idx].pvBuffer, len_data );
    memcpy( (char *)input.value + len_data, msg->pBuffers[token_idx].pvBuffer, len_token );

    ret = pgss_unwrap( &minor_status, ctx, &input, &output, &conf_state, NULL );
    RtlFreeHeap( GetProcessHeap(), 0, input.value );
    TRACE( "gss_unwrap returned %08x minor status %08x\n", ret, minor_status );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE)
    {
        if (qop) *qop = (conf_state ? 0 : SECQOP_WRAP_NO_ENCRYPT);
        memcpy( msg->pBuffers[data_idx].pvBuffer, output.value, len_data );
        pgss_release_buffer( &minor_status, &output );
    }

    return status_gss_to_sspi( ret );
}

static NTSTATUS CDECL unseal_message( LSA_SEC_HANDLE context, SecBufferDesc *msg, ULONG *qop )
{
    gss_ctx_id_t ctx = ctxhandle_sspi_to_gss( context );

    if (is_dce_style_context( ctx )) return unseal_message_vector( ctx, msg, qop );
    return unseal_message_no_vector( ctx, msg, qop );
}

static NTSTATUS CDECL verify_signature( LSA_SEC_HANDLE context, SecBufferDesc *msg, ULONG *qop )
{
    OM_uint32 ret, minor_status;
    gss_buffer_desc data_buffer, token_buffer;
    gss_ctx_id_t ctx_handle = ctxhandle_sspi_to_gss( context );
    int data_idx, token_idx;

    if ((data_idx = get_buffer_index( msg, SECBUFFER_DATA )) == -1) return SEC_E_INVALID_TOKEN;
    data_buffer.length = msg->pBuffers[data_idx].cbBuffer;
    data_buffer.value  = msg->pBuffers[data_idx].pvBuffer;

    if ((token_idx = get_buffer_index( msg, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;
    token_buffer.length = msg->pBuffers[token_idx].cbBuffer;
    token_buffer.value  = msg->pBuffers[token_idx].pvBuffer;

    ret = pgss_verify_mic( &minor_status, ctx_handle, &data_buffer, &token_buffer, NULL );
    TRACE( "gss_verify_mic returned %08x minor status %08x\n", ret, minor_status );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE && qop) *qop = 0;

    return status_gss_to_sspi( ret );
}

static const struct krb5_funcs funcs =
{
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

NTSTATUS CDECL __wine_init_unix_lib( HMODULE module, DWORD reason, const void *ptr_in, void *ptr_out )
{
    if (reason != DLL_PROCESS_ATTACH) return STATUS_SUCCESS;
    if (load_krb5() && load_gssapi_krb5())
    {
        *(const struct krb5_funcs **)ptr_out = &funcs;
        return STATUS_SUCCESS;
    }
    if (libkrb5_handle) unload_krb5();
    return STATUS_DLL_NOT_FOUND;
}
#endif /* defined(SONAME_LIBKRB5) && defined(SONAME_LIBGSSAPI_KRB5) */
