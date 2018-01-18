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
MAKE_FUNCPTR(gss_acquire_cred);
MAKE_FUNCPTR(gss_import_name);
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

    LOAD_FUNCPTR(gss_acquire_cred)
    LOAD_FUNCPTR(gss_import_name)
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

static inline void credhandle_gss_to_sspi( gss_cred_id_t handle, LSA_SEC_HANDLE *cred )
{
    *cred = (LSA_SEC_HANDLE)handle;
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
    NULL, /* FreeCredentialsHandle */
    NULL, /* SaveCredentials */
    NULL, /* GetCredentials */
    NULL, /* DeleteCredentials */
    NULL, /* InitLsaModeContext */
    NULL, /* AcceptLsaModeContext */
    NULL, /* DeleteContext */
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
