/*
 * Copyright 2017 Dmitry Timoshkov
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#ifdef HAVE_KRB5_KRB5_H
#include <krb5/krb5.h>
#endif
#ifdef SONAME_LIBGSSAPI_KRB5
# include <gssapi/gssapi.h>
# include <gssapi/gssapi_ext.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "rpc.h"
#include "sspi.h"
#include "ntsecapi.h"
#include "ntsecpkg.h"
#include "winternl.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(kerberos);

#define KERBEROS_MAX_BUF 12000

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

static ULONG kerberos_package_id;
static LSA_DISPATCH_TABLE lsa_dispatch;

#ifdef SONAME_LIBKRB5

static void *libkrb5_handle;

#define MAKE_FUNCPTR(f) static typeof(f) * p_##f
MAKE_FUNCPTR(krb5_init_context);
#undef MAKE_FUNCPTR

static void load_krb5(void)
{
    if (!(libkrb5_handle = wine_dlopen(SONAME_LIBKRB5, RTLD_NOW, NULL, 0)))
    {
        WARN("Failed to load %s, Kerberos support will be disabled\n", SONAME_LIBKRB5);
        return;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p_##f = wine_dlsym(libkrb5_handle, #f, NULL, 0))) \
    { \
        ERR("Failed to load %s\n", #f); \
        goto fail; \
    }

    LOAD_FUNCPTR(krb5_init_context)
#undef LOAD_FUNCPTR

    return;

fail:
    wine_dlclose(libkrb5_handle, NULL, 0);
    libkrb5_handle = NULL;
}

#else /* SONAME_LIBKRB5 */

static void load_krb5(void)
{
    WARN("Kerberos support was not provided at compile time\n");
}

#endif /* SONAME_LIBKRB5 */

static const char *debugstr_us( const UNICODE_STRING *us )
{
    if (!us) return "<null>";
    return debugstr_wn( us->Buffer, us->Length / sizeof(WCHAR) );
}

static NTSTATUS NTAPI kerberos_LsaApInitializePackage(ULONG package_id, PLSA_DISPATCH_TABLE dispatch,
    PLSA_STRING database, PLSA_STRING confidentiality, PLSA_STRING *package_name)
{
    char *kerberos_name;

    load_krb5();

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

static NTSTATUS NTAPI kerberos_LsaApCallPackageUntrusted(PLSA_CLIENT_REQUEST request,
    PVOID in_buffer, PVOID client_buffer_base, ULONG in_buffer_length,
    PVOID *out_buffer, PULONG out_buffer_length, PNTSTATUS status)
{
    FIXME("%p,%p,%p,%u,%p,%p,%p: stub\n", request, in_buffer, client_buffer_base,
        in_buffer_length, out_buffer, out_buffer_length, status);

    *status = STATUS_NOT_IMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS NTAPI kerberos_SpGetInfo(SecPkgInfoW *info)
{
    static WCHAR kerberos_name_W[] = {'K','e','r','b','e','r','o','s',0};
    static WCHAR kerberos_comment_W[] = {'M','i','c','r','o','s','o','f','t',' ','K','e','r','b','e','r','o','s',' ','V','1','.','0',0};
    static const SecPkgInfoW infoW =
    {
        KERBEROS_CAPS,
        1,
        RPC_C_AUTHN_GSS_KERBEROS,
        KERBEROS_MAX_BUF,
        kerberos_name_W,
        kerberos_comment_W
    };

    TRACE("%p\n", info);

    /* LSA will make a copy before forwarding the structure, so
     * it's safe to put pointers to dynamic or constant data there.
     */
    *info = infoW;

    return STATUS_SUCCESS;
}

#ifdef SONAME_LIBGSSAPI_KRB5

WINE_DECLARE_DEBUG_CHANNEL(winediag);
static void *libgssapi_krb5_handle;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(gss_accept_sec_context);
MAKE_FUNCPTR(gss_acquire_cred);
MAKE_FUNCPTR(gss_delete_sec_context);
MAKE_FUNCPTR(gss_import_name);
MAKE_FUNCPTR(gss_init_sec_context);
MAKE_FUNCPTR(gss_release_buffer);
MAKE_FUNCPTR(gss_release_cred);
MAKE_FUNCPTR(gss_release_name);
#undef MAKE_FUNCPTR

static BOOL load_gssapi_krb5(void)
{
    if (!(libgssapi_krb5_handle = wine_dlopen( SONAME_LIBGSSAPI_KRB5, RTLD_NOW, NULL, 0 )))
    {
        ERR_(winediag)( "Failed to load libgssapi_krb5, Kerberos SSP support will not be available.\n" );
        return FALSE;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p##f = wine_dlsym( libgssapi_krb5_handle, #f, NULL, 0 ))) \
    { \
        ERR( "Failed to load %s\n", #f ); \
        goto fail; \
    }

    LOAD_FUNCPTR(gss_accept_sec_context)
    LOAD_FUNCPTR(gss_acquire_cred)
    LOAD_FUNCPTR(gss_delete_sec_context)
    LOAD_FUNCPTR(gss_import_name)
    LOAD_FUNCPTR(gss_init_sec_context)
    LOAD_FUNCPTR(gss_release_buffer)
    LOAD_FUNCPTR(gss_release_cred)
    LOAD_FUNCPTR(gss_release_name)
#undef LOAD_FUNCPTR

    return TRUE;

fail:
    wine_dlclose( libgssapi_krb5_handle, NULL, 0 );
    libgssapi_krb5_handle = NULL;
    return FALSE;
}

static void unload_gssapi_krb5(void)
{
    wine_dlclose( libgssapi_krb5_handle, NULL, 0 );
    libgssapi_krb5_handle = NULL;
}

static inline gss_cred_id_t credhandle_sspi_to_gss( LSA_SEC_HANDLE cred )
{
    if (!cred) return GSS_C_NO_CREDENTIAL;
    return (gss_cred_id_t)cred;
}

static inline void credhandle_gss_to_sspi( gss_cred_id_t handle, LSA_SEC_HANDLE *cred )
{
    *cred = (LSA_SEC_HANDLE)handle;
}

static inline gss_ctx_id_t ctxthandle_sspi_to_gss( LSA_SEC_HANDLE ctxt )
{
    if (!ctxt) return GSS_C_NO_CONTEXT;
    return (gss_ctx_id_t)ctxt;
}

static inline void ctxthandle_gss_to_sspi( gss_ctx_id_t handle, LSA_SEC_HANDLE *ctxt )
{
    *ctxt = (LSA_SEC_HANDLE)handle;
}

static SECURITY_STATUS status_gss_to_sspi( OM_uint32 status )
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

    default:
        FIXME( "couldn't convert status 0x%08x to SECURITY_STATUS\n", status );
        return SEC_E_INTERNAL_ERROR;
    }
}

static void expirytime_gss_to_sspi( OM_uint32 expirytime, TimeStamp *timestamp )
{
    SYSTEMTIME time;
    FILETIME filetime;
    ULARGE_INTEGER tmp;

    GetLocalTime( &time );
    SystemTimeToFileTime( &time, &filetime );
    tmp.QuadPart = ((ULONGLONG)filetime.dwLowDateTime | (ULONGLONG)filetime.dwHighDateTime << 32) + expirytime;
    timestamp->LowPart  = tmp.QuadPart;
    timestamp->HighPart = tmp.QuadPart >> 32;
}

static SECURITY_STATUS name_sspi_to_gss( const UNICODE_STRING *name_str, gss_name_t *name )
{
    OM_uint32 ret, minor_status;
    gss_OID type = GSS_C_NO_OID; /* FIXME: detect the appropriate value for this ourselves? */
    gss_buffer_desc buf;

    buf.length = WideCharToMultiByte( CP_UNIXCP, 0, name_str->Buffer, name_str->Length / sizeof(WCHAR), NULL, 0, NULL, NULL ) + 1;
    if (!(buf.value = HeapAlloc( GetProcessHeap(), 0, buf.length ))) return SEC_E_INSUFFICIENT_MEMORY;
    WideCharToMultiByte( CP_UNIXCP, 0, name_str->Buffer, name_str->Length / sizeof(WCHAR), buf.value, buf.length, NULL, NULL );
    buf.length--;

    ret = pgss_import_name( &minor_status, &buf, type, name );
    TRACE( "gss_import_name returned %08x minor status %08x\n", ret, minor_status );

    HeapFree( GetProcessHeap(), 0, buf.value );
    return status_gss_to_sspi( ret );
}

static ULONG flags_isc_req_to_gss( ULONG flags )
{
    ULONG ret = GSS_C_DCE_STYLE;
    if (flags & ISC_REQ_DELEGATE)        ret |= GSS_C_DELEG_FLAG;
    if (flags & ISC_REQ_MUTUAL_AUTH)     ret |= GSS_C_MUTUAL_FLAG;
    if (flags & ISC_REQ_REPLAY_DETECT)   ret |= GSS_C_REPLAY_FLAG;
    if (flags & ISC_REQ_SEQUENCE_DETECT) ret |= GSS_C_SEQUENCE_FLAG;
    if (flags & ISC_REQ_CONFIDENTIALITY) ret |= GSS_C_CONF_FLAG;
    if (flags & ISC_REQ_INTEGRITY)       ret |= GSS_C_INTEG_FLAG;
    if (flags & ISC_REQ_NULL_SESSION)    ret |= GSS_C_ANON_FLAG;
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
    return ret;
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
    return ret;
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
#endif /* SONAME_LIBGSSAPI_KRB5 */

static NTSTATUS NTAPI kerberos_SpAcquireCredentialsHandle(
    UNICODE_STRING *principal_us, ULONG credential_use, LUID *logon_id, void *auth_data,
    void *get_key_fn, void *get_key_arg, LSA_SEC_HANDLE *credential, TimeStamp *ts_expiry )
{
#ifdef SONAME_LIBGSSAPI_KRB5
    OM_uint32 ret, minor_status, expiry_time;
    gss_name_t principal = GSS_C_NO_NAME;
    gss_cred_usage_t cred_usage;
    gss_cred_id_t cred_handle;

    TRACE( "(%s 0x%08x %p %p %p %p %p %p)\n", debugstr_us(principal_us), credential_use,
           logon_id, auth_data, get_key_fn, get_key_arg, credential, ts_expiry );

    if (auth_data)
    {
        FIXME( "specific credentials not supported\n" );
        return SEC_E_UNKNOWN_CREDENTIALS;
    }

    switch (credential_use)
    {
        case SECPKG_CRED_INBOUND:
            cred_usage = GSS_C_ACCEPT;
            break;
        case SECPKG_CRED_OUTBOUND:
            cred_usage = GSS_C_INITIATE;
            break;
        case SECPKG_CRED_BOTH:
            cred_usage = GSS_C_BOTH;
            break;
        default:
            return SEC_E_UNKNOWN_CREDENTIALS;
    }

    if (principal_us && ((ret = name_sspi_to_gss( principal_us, &principal )) != SEC_E_OK)) return ret;

    ret = pgss_acquire_cred( &minor_status, principal, GSS_C_INDEFINITE, GSS_C_NULL_OID_SET, cred_usage,
                              &cred_handle, NULL, &expiry_time );
    TRACE( "gss_acquire_cred returned %08x minor status %08x\n", ret, minor_status );
    if (ret == GSS_S_COMPLETE || ret == GSS_S_CONTINUE_NEEDED)
    {
        credhandle_gss_to_sspi( cred_handle, credential );
        expirytime_gss_to_sspi( expiry_time, ts_expiry );
    }

    if (principal != GSS_C_NO_NAME) pgss_release_name( &minor_status, &principal );

    return status_gss_to_sspi( ret );
#else
    FIXME( "(%s 0x%08x %p %p %p %p %p %p)\n", debugstr_us(principal_us), credential_use,
           logon_id, auth_data, get_key_fn, get_key_arg, credential, ts_expiry );
    FIXME( "Wine was built without Kerberos support.\n" );
    return SEC_E_UNSUPPORTED_FUNCTION;
#endif
}

static NTSTATUS NTAPI kerberos_SpFreeCredentialsHandle( LSA_SEC_HANDLE credential )
{
#ifdef SONAME_LIBGSSAPI_KRB5
    OM_uint32 ret, minor_status;
    gss_cred_id_t cred_handle;

    TRACE( "(%lx)\n", credential );

    if (!credential) return SEC_E_INVALID_HANDLE;
    if (!(cred_handle = credhandle_sspi_to_gss( credential ))) return SEC_E_OK;

    ret = pgss_release_cred( &minor_status, &cred_handle );
    TRACE( "gss_release_cred returned %08x minor status %08x\n", ret, minor_status );

    return status_gss_to_sspi( ret );
#else
    FIXME( "(%lx)\n", credential );
    return SEC_E_UNSUPPORTED_FUNCTION;
#endif
}

static NTSTATUS NTAPI kerberos_SpInitLsaModeContext( LSA_SEC_HANDLE credential, LSA_SEC_HANDLE context,
    UNICODE_STRING *target_name, ULONG context_req, ULONG target_data_rep, SecBufferDesc *input,
    LSA_SEC_HANDLE *new_context, SecBufferDesc *output, ULONG *context_attr, TimeStamp *ts_expiry,
    BOOLEAN *mapped_context, SecBuffer *context_data )
{
#ifdef SONAME_LIBGSSAPI_KRB5
    static const ULONG supported = ISC_REQ_CONFIDENTIALITY | ISC_REQ_INTEGRITY | ISC_REQ_SEQUENCE_DETECT |
                                   ISC_REQ_REPLAY_DETECT | ISC_REQ_MUTUAL_AUTH;
    OM_uint32 ret, minor_status, ret_flags, expiry_time, req_flags = flags_isc_req_to_gss( context_req );
    gss_cred_id_t cred_handle;
    gss_ctx_id_t ctxt_handle;
    gss_buffer_desc input_token, output_token;
    gss_name_t target = GSS_C_NO_NAME;
    int idx;

    TRACE( "(%lx %lx %s 0x%08x %u %p %p %p %p %p %p %p)\n", credential, context, debugstr_us(target_name),
           context_req, target_data_rep, input, new_context, output, context_attr, ts_expiry,
           mapped_context, context_data );
    if (context_req & ~supported)
    {
        FIXME( "flags 0x%08x not supported\n", context_req & ~supported );
        return SEC_E_UNSUPPORTED_FUNCTION;
    }

    if (!context && !input && !credential) return SEC_E_INVALID_HANDLE;
    cred_handle = credhandle_sspi_to_gss( credential );
    ctxt_handle = ctxthandle_sspi_to_gss( context );

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

    if (target_name && ((ret = name_sspi_to_gss( target_name, &target )) != SEC_E_OK)) return ret;

    ret = pgss_init_sec_context( &minor_status, cred_handle, &ctxt_handle, target, GSS_C_NO_OID, req_flags, 0,
                                 GSS_C_NO_CHANNEL_BINDINGS, &input_token, NULL, &output_token, &ret_flags,
                                 &expiry_time );
    TRACE( "gss_init_sec_context returned %08x minor status %08x ret_flags %08x\n", ret, minor_status, ret_flags );
    if (ret == GSS_S_COMPLETE || ret == GSS_S_CONTINUE_NEEDED)
    {
        if (output_token.length > output->pBuffers[idx].cbBuffer) /* FIXME: check if larger buffer exists */
        {
            TRACE( "buffer too small %lu > %u\n", (SIZE_T)output_token.length, output->pBuffers[idx].cbBuffer );
            pgss_release_buffer( &minor_status, &output_token );
            pgss_delete_sec_context( &minor_status, &ctxt_handle, GSS_C_NO_BUFFER );
            return SEC_E_INCOMPLETE_MESSAGE;
        }
        output->pBuffers[idx].cbBuffer = output_token.length;
        memcpy( output->pBuffers[idx].pvBuffer, output_token.value, output_token.length );
        pgss_release_buffer( &minor_status, &output_token );

        ctxthandle_gss_to_sspi( ctxt_handle, new_context );
        if (context_attr) *context_attr = flags_gss_to_isc_ret( ret_flags );
        expirytime_gss_to_sspi( expiry_time, ts_expiry );
    }

    if (target != GSS_C_NO_NAME) pgss_release_name( &minor_status, &target );

    /* we do support user mode SSP/AP functions */
    *mapped_context = TRUE;
    /* FIXME: initialize context_data */

    return status_gss_to_sspi( ret );
#else
    FIXME( "(%lx %lx %s 0x%08x %u %p %p %p %p %p %p %p)\n", credential, context, debugstr_us(target_name),
           context_req, target_data_rep, input, new_context, output, context_attr, ts_expiry,
           mapped_context, context_data );
    return SEC_E_UNSUPPORTED_FUNCTION;
#endif
}

static NTSTATUS NTAPI kerberos_SpAcceptLsaModeContext( LSA_SEC_HANDLE credential, LSA_SEC_HANDLE context,
    SecBufferDesc *input, ULONG context_req, ULONG target_data_rep, LSA_SEC_HANDLE *new_context,
    SecBufferDesc *output, ULONG *context_attr, TimeStamp *ts_expiry, BOOLEAN *mapped_context, SecBuffer *context_data )
{
#ifdef SONAME_LIBGSSAPI_KRB5
    OM_uint32 ret, minor_status, ret_flags, expiry_time;
    gss_cred_id_t cred_handle;
    gss_ctx_id_t ctxt_handle;
    gss_buffer_desc input_token, output_token;
    gss_name_t target = GSS_C_NO_NAME;
    int idx;

    TRACE( "(%lx %lx 0x%08x %u %p %p %p %p %p %p %p)\n", credential, context, context_req,
           target_data_rep, input, new_context, output, context_attr, ts_expiry,
           mapped_context, context_data );
    if (context_req) FIXME( "ignoring flags 0x%08x\n", context_req );

    if (!context && !input && !credential) return SEC_E_INVALID_HANDLE;
    cred_handle = credhandle_sspi_to_gss( credential );
    ctxt_handle = ctxthandle_sspi_to_gss( context );

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

    ret = pgss_accept_sec_context( &minor_status, &ctxt_handle, cred_handle, &input_token, GSS_C_NO_CHANNEL_BINDINGS,
                                   &target, NULL, &output_token, &ret_flags, &expiry_time, NULL );
    TRACE( "gss_accept_sec_context returned %08x minor status %08x ctxt_handle %p\n", ret, minor_status, ctxt_handle );
    if (ret == GSS_S_COMPLETE || ret == GSS_S_CONTINUE_NEEDED)
    {
        if (output_token.length > output->pBuffers[idx].cbBuffer) /* FIXME: check if larger buffer exists */
        {
            TRACE( "buffer too small %lu > %u\n", (SIZE_T)output_token.length, output->pBuffers[idx].cbBuffer );
            pgss_release_buffer( &minor_status, &output_token );
            pgss_delete_sec_context( &minor_status, &ctxt_handle, GSS_C_NO_BUFFER );
            return SEC_E_BUFFER_TOO_SMALL;
        }
        output->pBuffers[idx].cbBuffer = output_token.length;
        memcpy( output->pBuffers[idx].pvBuffer, output_token.value, output_token.length );
        pgss_release_buffer( &minor_status, &output_token );

        ctxthandle_gss_to_sspi( ctxt_handle, new_context );
        if (context_attr) *context_attr = flags_gss_to_asc_ret( ret_flags );
        expirytime_gss_to_sspi( expiry_time, ts_expiry );
    }

    /* we do support user mode SSP/AP functions */
    *mapped_context = TRUE;
    /* FIXME: initialize context_data */

    return status_gss_to_sspi( ret );
#else
    FIXME( "(%lx %lx 0x%08x %u %p %p %p %p %p %p %p)\n", credential, context, context_req,
           target_data_rep, input, new_context, output, context_attr, ts_expiry,
           mapped_context, context_data );
    return SEC_E_UNSUPPORTED_FUNCTION;
#endif
}

static NTSTATUS NTAPI kerberos_SpDeleteContext( LSA_SEC_HANDLE context )
{
#ifdef SONAME_LIBGSSAPI_KRB5
    OM_uint32 ret, minor_status;
    gss_ctx_id_t ctxt_handle;

    TRACE( "(%lx)\n", context );
    if (!context) return SEC_E_INVALID_HANDLE;
    if (!(ctxt_handle = ctxthandle_sspi_to_gss( context ))) return SEC_E_OK;

    ret = pgss_delete_sec_context( &minor_status, &ctxt_handle, GSS_C_NO_BUFFER );
    TRACE( "gss_delete_sec_context returned %08x minor status %08x\n", ret, minor_status );

    return status_gss_to_sspi( ret );
#else
    FIXME( "(%lx)\n", context );
    return SEC_E_UNSUPPORTED_FUNCTION;
#endif
}

static NTSTATUS NTAPI kerberos_SpInitialize(ULONG_PTR package_id, SECPKG_PARAMETERS *params,
    LSA_SECPKG_FUNCTION_TABLE *lsa_function_table)
{
    TRACE("%lu,%p,%p\n", package_id, params, lsa_function_table);

#ifdef SONAME_LIBGSSAPI_KRB5
    if (load_gssapi_krb5()) return STATUS_SUCCESS;
#endif

    return STATUS_UNSUCCESSFUL;
}

static NTSTATUS NTAPI kerberos_SpShutdown(void)
{
    TRACE("\n");

#ifdef SONAME_LIBGSSAPI_KRB5
    unload_gssapi_krb5();
#endif

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
    NULL, /* SpQueryContextAttributes */
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
    TRACE("%#x,%p,%p,%p\n", lsa_version, package_version, table, table_count);

    *package_version = SECPKG_INTERFACE_VERSION;
    *table = &kerberos_table;
    *table_count = 1;

    return STATUS_SUCCESS;
}
