/*
 * Copyright 2017 Dmitry Timoshkov
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "sspi.h"
#include "ntsecapi.h"
#include "ntsecpkg.h"
#include "winternl.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(kerberos);

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
    NULL, /* Initialize */
    NULL, /* Shutdown */
    NULL, /* SpGetInfoUnified */
    NULL, /* AcceptCredentials */
    NULL, /* SpAcquireCredentialsHandle */
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
