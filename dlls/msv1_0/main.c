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
    NULL, /* SpAcquireCredentialsHandle */
    NULL, /* SpQueryCredentialsAttributes */
    NULL, /* SpFreeCredentialsHandle */
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
