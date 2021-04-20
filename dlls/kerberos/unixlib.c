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

static inline gss_cred_id_t credhandle_sspi_to_gss( LSA_SEC_HANDLE handle )
{
    return (gss_cred_id_t)handle;
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

static NTSTATUS CDECL free_credentials_handle( LSA_SEC_HANDLE handle )
{
    OM_uint32 ret, minor_status;
    gss_cred_id_t cred = credhandle_sspi_to_gss( handle );

    ret = pgss_release_cred( &minor_status, &cred );
    TRACE( "gss_release_cred returned %08x minor status %08x\n", ret, minor_status );
    if (GSS_ERROR( ret )) trace_gss_status( ret, minor_status );
    return status_gss_to_sspi( ret );
}

static const struct krb5_funcs funcs =
{
    acquire_credentials_handle,
    free_credentials_handle,
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
