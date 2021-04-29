/*
 * Copyright 2005, 2006 Kai Blin
 * Copyright 2021 Hans Leidekker for CodeWeavers
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
#include "winternl.h"
#include "sspi.h"
#include "ntsecapi.h"
#include "ntsecpkg.h"
#include "rpc.h"

#include "wine/debug.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

static HINSTANCE instance;
static ULONG ntlm_package_id;
static LSA_DISPATCH_TABLE lsa_dispatch;

const struct ntlm_funcs *ntlm_funcs = NULL;

#define NTLM_CAPS \
    ( SECPKG_FLAG_INTEGRITY \
    | SECPKG_FLAG_PRIVACY \
    | SECPKG_FLAG_TOKEN_ONLY \
    | SECPKG_FLAG_CONNECTION \
    | SECPKG_FLAG_MULTI_REQUIRED  \
    | SECPKG_FLAG_IMPERSONATION \
    | SECPKG_FLAG_ACCEPT_WIN32_NAME \
    | SECPKG_FLAG_NEGOTIABLE \
    | SECPKG_FLAG_LOGON  \
    | SECPKG_FLAG_RESTRICTED_TOKENS )

#define NTLM_MAX_BUF 1904

static const SecPkgInfoW ntlm_package_info =
{
    NTLM_CAPS,
    1,
    RPC_C_AUTHN_WINNT,
    NTLM_MAX_BUF,
    (SEC_WCHAR *)L"NTLM",
    (SEC_WCHAR *)L"NTLM Security Package"
};

static inline const char *debugstr_as( const STRING *str )
{
    if (!str) return "<null>";
    return debugstr_an( str->Buffer, str->Length );
}

static inline const char *debugstr_us( const UNICODE_STRING *str )
{
    if (!str) return "<null>";
    return debugstr_wn( str->Buffer, str->Length / sizeof(WCHAR) );
}

static NTSTATUS NTAPI ntlm_LsaApInitializePackage( ULONG package_id, LSA_DISPATCH_TABLE *dispatch,
                                                   LSA_STRING *database, LSA_STRING *confidentiality,
                                                   LSA_STRING **package_name )
{
    LSA_STRING *str;
    char *ptr;

    TRACE( "%08x, %p, %s, %s, %p\n", package_id, dispatch, debugstr_as(database), debugstr_as(confidentiality),
           package_name );

    if (!ntlm_funcs && __wine_init_unix_lib( instance, DLL_PROCESS_ATTACH, NULL, &ntlm_funcs ))
    {
        ERR( "no NTLM support, expect problems\n" );
        return STATUS_UNSUCCESSFUL;
    }

    if (!(str = dispatch->AllocateLsaHeap( sizeof(*str) + sizeof("NTLM" )))) return STATUS_NO_MEMORY;
    ptr = (char *)(str + 1);
    memcpy( ptr, "NTLM", sizeof("NTLM") );
    RtlInitString( str, ptr );

    ntlm_package_id = package_id;
    lsa_dispatch = *dispatch;

    *package_name = str;
    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI ntlm_SpInitialize( ULONG_PTR package_id, SECPKG_PARAMETERS *params,
                                         LSA_SECPKG_FUNCTION_TABLE *lsa_function_table )
{
    TRACE( "%lu, %p, %p\n", package_id, params, lsa_function_table );

    if (!ntlm_funcs && __wine_init_unix_lib( instance, DLL_PROCESS_ATTACH, NULL, &ntlm_funcs ))
    {
        ERR( "no NTLM support, expect problems\n" );
        return STATUS_UNSUCCESSFUL;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI ntlm_SpGetInfo( SecPkgInfoW *info )
{
    TRACE( "%p\n", info );
    *info = ntlm_package_info;
    return STATUS_SUCCESS;
}

static char *get_username_arg( const WCHAR *user, int user_len )
{
    static const char arg[] = "--username=";
    int len = sizeof(arg);
    char *ret;

    len += WideCharToMultiByte( CP_UNIXCP, WC_NO_BEST_FIT_CHARS, user, user_len, NULL, 0, NULL, NULL );
    if (!(ret = malloc( len ))) return NULL;
    memcpy( ret, arg, sizeof(arg) - 1 );
    WideCharToMultiByte( CP_UNIXCP, WC_NO_BEST_FIT_CHARS, user, user_len, ret + sizeof(arg) - 1,
                         len - sizeof(arg) + 1, NULL, NULL );
    ret[len - 1] = 0;
    return ret;
}

static char *get_domain_arg( const WCHAR *domain, int domain_len )
{
    static const char arg[] = "--domain=";
    int len = sizeof(arg);
    char *ret;

    len += WideCharToMultiByte( CP_UNIXCP, WC_NO_BEST_FIT_CHARS, domain, domain_len, NULL, 0, NULL, NULL );
    if (!(ret = malloc( len ))) return NULL;
    memcpy( ret, arg, sizeof(arg) - 1 );
    WideCharToMultiByte( CP_UNIXCP, WC_NO_BEST_FIT_CHARS, domain, domain_len, ret + sizeof(arg) - 1,
                         len - sizeof(arg) + 1, NULL, NULL );
    ret[len - 1] = 0;
    return ret;
}

static NTSTATUS NTAPI ntlm_SpAcquireCredentialsHandle( UNICODE_STRING *principal, ULONG cred_use, LUID *logon_id,
                                                       void *auth_data, void *get_key_fn, void *get_key_arg,
                                                       LSA_SEC_HANDLE *handle, TimeStamp *expiry )
{
    SECURITY_STATUS status = SEC_E_INSUFFICIENT_MEMORY;
    struct ntlm_cred *cred = NULL;
    WCHAR *domain = NULL, *user = NULL, *password = NULL;
    SEC_WINNT_AUTH_IDENTITY_W *id = NULL;

    TRACE( "%s, 0x%08x, %p, %p, %p, %p, %p, %p\n", debugstr_us(principal), cred_use, logon_id, auth_data,
           get_key_fn, get_key_arg, cred, expiry );

    switch (cred_use)
    {
    case SECPKG_CRED_INBOUND:
        if (!(cred = malloc( sizeof(*cred) ))) return SEC_E_INSUFFICIENT_MEMORY;
        cred->mode         = MODE_SERVER;
        cred->username_arg = NULL;
        cred->domain_arg   = NULL;
        cred->password     = NULL;
        cred->password_len = 0;
        cred->no_cached_credentials = 0;

        *handle = (LSA_SEC_HANDLE)cred;
        status = SEC_E_OK;
        break;

    case SECPKG_CRED_OUTBOUND:
        if (!(cred = malloc( sizeof(*cred) ))) return SEC_E_INSUFFICIENT_MEMORY;

        cred->mode         = MODE_CLIENT;
        cred->username_arg = NULL;
        cred->domain_arg   = NULL;
        cred->password     = NULL;
        cred->password_len = 0;
        cred->no_cached_credentials = 0;

        if ((id = auth_data))
        {
            int domain_len = 0, user_len = 0, password_len = 0;
            if (id->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI)
            {
                if (id->DomainLength)
                {
                    domain_len = MultiByteToWideChar( CP_ACP, 0, (char *)id->Domain, id->DomainLength, NULL, 0 );
                    if (!(domain = malloc( sizeof(WCHAR) * domain_len ))) goto done;
                    MultiByteToWideChar( CP_ACP, 0, (char *)id->Domain, id->DomainLength, domain, domain_len );
                }
                if (id->UserLength)
               {
                    user_len = MultiByteToWideChar( CP_ACP, 0, (char *)id->User, id->UserLength, NULL, 0 );
                    if (!(user = malloc( sizeof(WCHAR) * user_len ))) goto done;
                    MultiByteToWideChar( CP_ACP, 0, (char *)id->User, id->UserLength, user, user_len );
                }
                if (id->PasswordLength)
                {
                    password_len = MultiByteToWideChar( CP_ACP, 0,(char *)id->Password, id->PasswordLength, NULL, 0 );
                    if (!(password = malloc( sizeof(WCHAR) * password_len ))) goto done;
                    MultiByteToWideChar( CP_ACP, 0, (char *)id->Password, id->PasswordLength, password, password_len );
                }
            }
            else
            {
                domain = id->Domain;
                domain_len = id->DomainLength;
                user = id->User;
                user_len = id->UserLength;
                password = id->Password;
                password_len = id->PasswordLength;
            }

            TRACE( "username is %s\n", debugstr_wn(user, user_len) );
            TRACE( "domain name is %s\n", debugstr_wn(domain, domain_len) );

            cred->username_arg = get_username_arg( user, user_len );
            cred->domain_arg   = get_domain_arg( domain, domain_len );
            if (password_len)
            {
                cred->password_len = WideCharToMultiByte( CP_UNIXCP, WC_NO_BEST_FIT_CHARS, password, password_len,
                                                          NULL, 0, NULL, NULL );
                if (!(cred->password = malloc( cred->password_len ))) goto done;
                WideCharToMultiByte( CP_UNIXCP, WC_NO_BEST_FIT_CHARS, password, password_len, cred->password,
                                     cred->password_len, NULL, NULL );
            }
        }

        *handle = (LSA_SEC_HANDLE)cred;
        status = SEC_E_OK;
        break;

    case SECPKG_CRED_BOTH:
        FIXME( "SECPKG_CRED_BOTH not supported\n" );
        status = SEC_E_UNSUPPORTED_FUNCTION;
        break;

    default:
        status = SEC_E_UNKNOWN_CREDENTIALS;
        break;
    }

done:
    if (id && (id->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI))
    {
        free( domain );
        free( user );
        free( password );
    }
    if (status != SEC_E_OK) free( cred );
    return status;
}

static NTSTATUS NTAPI ntlm_SpFreeCredentialsHandle( LSA_SEC_HANDLE handle )
{
    struct ntlm_cred *cred = (struct ntlm_cred *)handle;

    TRACE( "%lx\n", handle );

    if (!cred) return SEC_E_OK;

    cred->mode = MODE_INVALID;
    if (cred->password) memset( cred->password, 0, cred->password_len );
    free( cred->password );
    free( cred->username_arg );
    free( cred->domain_arg );
    free( cred );
    return SEC_E_OK;
}

static SECPKG_FUNCTION_TABLE ntlm_table =
{
    ntlm_LsaApInitializePackage,
    NULL, /* LsaLogonUser */
    NULL, /* CallPackage */
    NULL, /* LogonTerminated */
    NULL, /* CallPackageUntrusted */
    NULL, /* CallPackagePassthrough */
    NULL, /* LogonUserEx */
    NULL, /* LogonUserEx2 */
    ntlm_SpInitialize,
    NULL, /* SpShutdown */
    ntlm_SpGetInfo,
    NULL, /* AcceptCredentials */
    ntlm_SpAcquireCredentialsHandle,
    NULL, /* SpQueryCredentialsAttributes */
    ntlm_SpFreeCredentialsHandle,
    NULL, /* SaveCredentials */
    NULL, /* GetCredentials */
    NULL, /* DeleteCredentials */
    NULL, /* SpInitLsaModeContext */
    NULL, /* SpAcceptLsaModeContext */
    NULL, /* SpDeleteContext */
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

NTSTATUS NTAPI SpLsaModeInitialize( ULONG lsa_version, ULONG *package_version, SECPKG_FUNCTION_TABLE **table,
                                    ULONG *table_count )
{
    TRACE( "%08x, %p, %p, %p\n", lsa_version, package_version, table, table_count );

    *package_version = SECPKG_INTERFACE_VERSION;
    *table = &ntlm_table;
    *table_count = 1;
    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI ntlm_SpInstanceInit( ULONG version, SECPKG_DLL_FUNCTIONS *dll_functions, void **user_functions )
{
    TRACE( "%08x, %p, %p\n", version, dll_functions, user_functions );
    return STATUS_SUCCESS;
}

static SECPKG_USER_FUNCTION_TABLE ntlm_user_table =
{
    ntlm_SpInstanceInit,
    NULL, /* SpInitUserModeContext */
    NULL, /* SpMakeSignature */
    NULL, /* SpVerifySignature */
    NULL, /* SpSealMessage */
    NULL, /* SpUnsealMessage */
    NULL, /* SpGetContextToken */
    NULL, /* SpQueryContextAttributes */
    NULL, /* SpCompleteAuthToken */
    NULL, /* SpDeleteContext */
    NULL, /* SpFormatCredentialsFn */
    NULL, /* SpMarshallSupplementalCreds */
    NULL, /* SpExportSecurityContext */
    NULL  /* SpImportSecurityContext */
};

NTSTATUS NTAPI SpUserModeInitialize( ULONG lsa_version, ULONG *package_version, SECPKG_USER_FUNCTION_TABLE **table,
                                     ULONG *table_count )
{
    TRACE( "%08x, %p, %p, %p\n", lsa_version, package_version, table, table_count );

    *package_version = SECPKG_INTERFACE_VERSION;
    *table = &ntlm_user_table;
    *table_count = 1;
    return STATUS_SUCCESS;
}

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, void *reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        instance = hinst;
        DisableThreadLibraryCalls( hinst );
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
