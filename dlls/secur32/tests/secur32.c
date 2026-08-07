/*
 * tests
 *
 * Copyright 2006 Robert Reif
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
#include <stdio.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <rpc.h>
#include <rpcdce.h>
#define SECURITY_WIN32
#include <security.h>
#include <schannel.h>
#include <wincred.h>
#include <winsock2.h>
#include <ntsecapi.h>
#include <winternl.h>

#include "wine/test.h"

static HMODULE secdll;

static SECURITY_STATUS (SEC_ENTRY *pSspiEncodeAuthIdentityAsStrings)
    (PSEC_WINNT_AUTH_IDENTITY_OPAQUE, PCWSTR *, PCWSTR *, PCWSTR *);
static SECURITY_STATUS (SEC_ENTRY *pSspiEncodeStringsAsAuthIdentity)
    (PCWSTR, PCWSTR, PCWSTR, PSEC_WINNT_AUTH_IDENTITY_OPAQUE *);
static void (SEC_ENTRY *pSspiFreeAuthIdentity)
    (PSEC_WINNT_AUTH_IDENTITY_OPAQUE);
static void (SEC_ENTRY *pSspiLocalFree)
    (void *);
static SECURITY_STATUS (SEC_ENTRY *pSspiMarshalAuthIdentity)
    (PSEC_WINNT_AUTH_IDENTITY_OPAQUE, ULONG *, char **);
static SECURITY_STATUS (SEC_ENTRY *pSspiPrepareForCredWrite)
    (PSEC_WINNT_AUTH_IDENTITY_OPAQUE, PCWSTR, PULONG, PCWSTR *, PCWSTR *, PUCHAR *, PULONG);
static SECURITY_STATUS (SEC_ENTRY *pSspiUnmarshalAuthIdentity)
    (ULONG, char *, PSEC_WINNT_AUTH_IDENTITY_OPAQUE *);
static void (SEC_ENTRY *pSspiZeroAuthIdentity)
    (PSEC_WINNT_AUTH_IDENTITY_OPAQUE);

static BOOLEAN (WINAPI * pGetComputerObjectNameA)(EXTENDED_NAME_FORMAT NameFormat, LPSTR lpNameBuffer, PULONG lpnSize);
static BOOLEAN (WINAPI * pGetComputerObjectNameW)(EXTENDED_NAME_FORMAT NameFormat, LPWSTR lpNameBuffer, PULONG lpnSize);
static BOOLEAN (WINAPI * pGetUserNameExA)(EXTENDED_NAME_FORMAT NameFormat, LPSTR lpNameBuffer, PULONG lpnSize);
static BOOLEAN (WINAPI * pGetUserNameExW)(EXTENDED_NAME_FORMAT NameFormat, LPWSTR lpNameBuffer, PULONG lpnSize);
static PSecurityFunctionTableA (SEC_ENTRY * pInitSecurityInterfaceA)(void);
static PSecurityFunctionTableW (SEC_ENTRY * pInitSecurityInterfaceW)(void);

static EXTENDED_NAME_FORMAT formats[] = {
    NameUnknown, NameFullyQualifiedDN, NameSamCompatible, NameDisplay,
    NameUniqueId, NameCanonical, NameUserPrincipal, NameCanonicalEx,
    NameServicePrincipal, NameDnsDomain
};

static void testGetComputerObjectNameA(void)
{
    char name[256];
    ULONG size;
    BOOLEAN rc;
    UINT i;

    for (i = 0; i < ARRAY_SIZE(formats); i++) {
        size = 0;
        SetLastError(0xdeadbeef);
        rc = pGetComputerObjectNameA(formats[i], NULL, &size);
        ok(!rc, "GetComputerObjectName(%u) should fail\n", formats[i]);
        switch (formats[i])
        {
        case NameUnknown:
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "%u: got %lu\n", formats[i], GetLastError());
            break;
        default:
            ok(GetLastError() == ERROR_NONE_MAPPED ||
               GetLastError() == ERROR_NO_SUCH_USER ||
               GetLastError() == ERROR_CANT_ACCESS_DOMAIN_INFO ||
               GetLastError() == ERROR_INSUFFICIENT_BUFFER,
               "%u: got %lu\n", formats[i], GetLastError());
            break;
        }

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) continue;

        size = sizeof(name);
        SetLastError(0xdeadbeef);
        rc = pGetComputerObjectNameA(formats[i], name, &size);
        switch (formats[i])
        {
        case NameUnknown:
            ok(!rc, "GetComputerObjectName(%u) should fail\n", formats[i]);
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "%u: got %lu\n", formats[i], GetLastError());
            break;
        default:
            ok(rc, "GetComputerObjectName(%u) error %lu\n", formats[i], GetLastError());
            trace("GetComputerObjectName(%u) returned %s\n", formats[i], name);
            break;
        }
    }
}

static void testGetComputerObjectNameW(void)
{
    WCHAR nameW[256];
    ULONG size;
    BOOLEAN rc;
    UINT i;

    for (i = 0; i < ARRAY_SIZE(formats); i++) {
        size = 0;
        SetLastError(0xdeadbeef);
        rc = pGetComputerObjectNameW(formats[i], NULL, &size);
        ok(!rc || broken(rc) /* win10 */, "GetComputerObjectName(%u) should fail\n", formats[i]);
        switch (formats[i])
        {
        case NameUnknown:
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "%u: got %lu\n", formats[i], GetLastError());
            break;
        default:
            ok(GetLastError() == ERROR_NONE_MAPPED ||
               GetLastError() == ERROR_NO_SUCH_USER ||
               GetLastError() == ERROR_CANT_ACCESS_DOMAIN_INFO ||
               GetLastError() == WSAHOST_NOT_FOUND ||
               GetLastError() == ERROR_INSUFFICIENT_BUFFER,
               "%u: got %lu\n", formats[i], GetLastError());
            break;
        }

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) continue;

        size = ARRAY_SIZE(nameW);
        SetLastError(0xdeadbeef);
        rc = pGetComputerObjectNameW(formats[i], nameW, &size);
        switch (formats[i])
        {
        case NameUnknown:
            ok(!rc, "GetComputerObjectName(%u) should fail\n", formats[i]);
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "%u: got %lu\n", formats[i], GetLastError());
            break;
        default:
            ok(rc, "GetComputerObjectName(%u) error %lu\n", formats[i], GetLastError());
            trace("GetComputerObjectName(%u) returned %s\n", formats[i], wine_dbgstr_w(nameW));
            break;
        }
    }
}

static void testGetUserNameExA(void)
{
    char name[256];
    ULONG size;
    BOOLEAN rc;
    UINT i;

    for (i = 0; i < ARRAY_SIZE(formats); i++) {
        size = sizeof(name);
        ZeroMemory(name, sizeof(name));
        rc = pGetUserNameExA(formats[i], name, &size);
        ok(rc ||
           (formats[i] == NameUnknown &&
            GetLastError() == ERROR_NO_SUCH_USER) ||
           GetLastError() == ERROR_NONE_MAPPED ||
           broken(formats[i] == NameDnsDomain &&
                  GetLastError() == ERROR_INVALID_PARAMETER),
           "GetUserNameExW(%d) failed: %ld\n",
           formats[i], GetLastError());
    }

    if (0) /* Crashes on Windows */
        pGetUserNameExA(NameSamCompatible, NULL, NULL);

    size = 0;
    rc = pGetUserNameExA(NameSamCompatible, NULL, &size);
    ok(! rc && GetLastError() == ERROR_MORE_DATA, "Expected fail with ERROR_MORE_DATA, got %d with %lu\n", rc, GetLastError());
    ok(size != 0, "Expected size to be set to required size\n");

    if (0) /* Crashes on Windows with big enough size */
    {
        /* Returned size is already big enough */
        pGetUserNameExA(NameSamCompatible, NULL, &size);
    }

    size = 0;
    rc = pGetUserNameExA(NameSamCompatible, name, &size);
    ok(! rc && GetLastError() == ERROR_MORE_DATA, "Expected fail with ERROR_MORE_DATA, got %d with %lu\n", rc, GetLastError());
    ok(size != 0, "Expected size to be set to required size\n");
    size = 1;
    name[0] = 0xff;
    rc = pGetUserNameExA(NameSamCompatible, name, &size);
    ok(! rc && GetLastError() == ERROR_MORE_DATA, "Expected fail with ERROR_MORE_DATA, got %d with %lu\n", rc, GetLastError());
    ok(1 < size, "Expected size to be set to required size\n");
    ok(name[0] == (char) 0xff, "Expected unchanged buffer\n");
}

static void testGetUserNameExW(void)
{
    WCHAR nameW[256];
    ULONG size;
    BOOLEAN rc;
    UINT i;

    for (i = 0; i < ARRAY_SIZE(formats); i++) {
        size = sizeof(nameW);
        ZeroMemory(nameW, sizeof(nameW));
        rc = pGetUserNameExW(formats[i], nameW, &size);
        ok(rc ||
           (formats[i] == NameUnknown &&
            GetLastError() == ERROR_NO_SUCH_USER) ||
           GetLastError() == ERROR_NONE_MAPPED ||
           broken(formats[i] == NameDnsDomain &&
                  GetLastError() == ERROR_INVALID_PARAMETER),
           "GetUserNameExW(%d) failed: %ld\n",
           formats[i], GetLastError());
    }

    if (0) /* Crashes on Windows */
        pGetUserNameExW(NameSamCompatible, NULL, NULL);

    size = 0;
    rc = pGetUserNameExW(NameSamCompatible, NULL, &size);
    ok(! rc && GetLastError() == ERROR_MORE_DATA, "Expected fail with ERROR_MORE_DATA, got %d with %lu\n", rc, GetLastError());
    ok(size != 0, "Expected size to be set to required size\n");

    if (0) /* Crashes on Windows with big enough size */
    {
        /* Returned size is already big enough */
        pGetUserNameExW(NameSamCompatible, NULL, &size);
    }

    size = 0;
    rc = pGetUserNameExW(NameSamCompatible, nameW, &size);
    ok(! rc && GetLastError() == ERROR_MORE_DATA, "Expected fail with ERROR_MORE_DATA, got %d with %lu\n", rc, GetLastError());
    ok(size != 0, "Expected size to be set to required size\n");
    size = 1;
    nameW[0] = 0xff;
    rc = pGetUserNameExW(NameSamCompatible, nameW, &size);
    ok(! rc && GetLastError() == ERROR_MORE_DATA, "Expected fail with ERROR_MORE_DATA, got %d with %lu\n", rc, GetLastError());
    ok(1 < size, "Expected size to be set to required size\n");
    ok(nameW[0] == (WCHAR) 0xff, "Expected unchanged buffer\n");
}

static void test_InitSecurityInterface(void)
{
    PSecurityFunctionTableA sftA;
    PSecurityFunctionTableW sftW;

    sftA = pInitSecurityInterfaceA();
    ok(sftA != NULL, "pInitSecurityInterfaceA failed\n");
    ok(sftA->dwVersion == SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION, "wrong dwVersion %ld in security function table\n", sftA->dwVersion);
    ok(!sftA->Reserved2,
       "Reserved2 should be NULL instead of %p in security function table\n",
       sftA->Reserved2);
    ok(sftA->Reserved3 == sftA->EncryptMessage,
       "Reserved3 should be equal to EncryptMessage in the security function table\n");
    ok(sftA->Reserved4 == sftA->DecryptMessage,
       "Reserved4 should be equal to DecryptMessage in the security function table\n");

    if (!pInitSecurityInterfaceW)
    {
        win_skip("InitSecurityInterfaceW not exported by secur32.dll\n");
        return;
    }

    sftW = pInitSecurityInterfaceW();
    ok(sftW != NULL, "pInitSecurityInterfaceW failed\n");
    ok(sftW->dwVersion == SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION, "wrong dwVersion %ld in security function table\n", sftW->dwVersion);
    ok(!sftW->Reserved2, "Reserved2 should be NULL instead of %p in security function table\n", sftW->Reserved2);
    ok(sftW->Reserved3 == sftW->EncryptMessage, "Reserved3 should be equal to EncryptMessage in the security function table\n");
    ok(sftW->Reserved4 == sftW->DecryptMessage, "Reserved4 should be equal to DecryptMessage in the security function table\n");
}

static void test_SspiEncodeStringsAsAuthIdentity(void)
{
    static const WCHAR username[] = {'u','s','e','r','n','a','m','e',0};
    static const WCHAR domainname[] = {'d','o','m','a','i','n','n','a','m','e',0};
    static const WCHAR password[] = {'p','a','s','s','w','o','r','d',0};
    const WCHAR *username_ptr, *domainname_ptr, *password_ptr;
    PSEC_WINNT_AUTH_IDENTITY_OPAQUE id;
    SEC_WINNT_AUTH_IDENTITY_EXW *idex;
    SECURITY_STATUS status;

    if (!pSspiEncodeStringsAsAuthIdentity)
    {
        win_skip( "SspiEncodeAuthIdentityAsStrings not exported by secur32.dll\n" );
        return;
    }

    status = pSspiEncodeStringsAsAuthIdentity( NULL, NULL, NULL, NULL );
    ok( status == SEC_E_INVALID_TOKEN, "got %08lx\n", status );

    id = (PSEC_WINNT_AUTH_IDENTITY_OPAQUE)0xdeadbeef;
    status = pSspiEncodeStringsAsAuthIdentity( NULL, NULL, NULL, &id );
    ok( status == SEC_E_INVALID_TOKEN, "got %08lx\n", status );
    ok( id == (PSEC_WINNT_AUTH_IDENTITY_OPAQUE)0xdeadbeef, "id set\n" );

    id = NULL;
    status = pSspiEncodeStringsAsAuthIdentity( NULL, NULL, password, &id );
    ok( status == SEC_E_OK, "got %08lx\n", status );
    ok( id != NULL, "id not set\n" );

    idex = id;
    ok( idex->Version == SEC_WINNT_AUTH_IDENTITY_VERSION, "Version = %lx\n", idex->Version );
    ok( idex->Length == sizeof(*idex), "Length = %lu\n", idex->Length );
    ok( !idex->User, "User = %p\n", idex->User );
    ok( !idex->UserLength, "UserLength = %lu\n", idex->UserLength );
    ok( !idex->Domain, "Domain = %p\n", idex->Domain );
    ok( !idex->DomainLength, "DomainLength = %lu\n", idex->DomainLength );
    ok( idex->Password == (USHORT *)(idex + 1), "Password = %p, idex = %p\n", idex->Password, idex );
    ok( idex->PasswordLength == wcslen(password), "PasswordLength = %lu\n", idex->PasswordLength );
    ok( idex->Flags == (SEC_WINNT_AUTH_IDENTITY_UNICODE | SEC_WINNT_AUTH_IDENTITY_MARSHALLED),
            "Flags = %lx\n", idex->Flags );
    ok( !idex->PackageList, "PackageList = %p\n", idex->PackageList );
    ok( !idex->PackageListLength, "PackageListLength = %lu\n", idex->PackageListLength );
    pSspiFreeAuthIdentity( id );

    id = NULL;
    status = pSspiEncodeStringsAsAuthIdentity( NULL, domainname, password, &id );
    ok( status == SEC_E_OK, "got %08lx\n", status );
    ok( id != NULL, "id not set\n" );
    pSspiFreeAuthIdentity( id );

    id = NULL;
    status = pSspiEncodeStringsAsAuthIdentity( username, NULL, password, &id );
    ok( status == SEC_E_OK, "got %08lx\n", status );
    ok( id != NULL, "id not set\n" );
    pSspiFreeAuthIdentity( id );

    id = NULL;
    status = pSspiEncodeStringsAsAuthIdentity( username, NULL, NULL, &id );
    ok( status == SEC_E_OK, "got %08lx\n", status );
    ok( id != NULL, "id not set\n" );
    pSspiFreeAuthIdentity( id );

    id = NULL;
    status = pSspiEncodeStringsAsAuthIdentity( username, domainname, password, &id );
    ok( status == SEC_E_OK, "got %08lx\n", status );
    ok( id != NULL, "id not set\n" );

    username_ptr = domainname_ptr = password_ptr = NULL;
    status = pSspiEncodeAuthIdentityAsStrings( id, &username_ptr, &domainname_ptr, &password_ptr );
    ok( status == SEC_E_OK, "got %08lx\n", status );
    ok( !lstrcmpW( username, username_ptr ), "wrong username\n" );
    ok( !lstrcmpW( domainname, domainname_ptr ), "wrong domainname\n" );
    ok( !lstrcmpW( password, password_ptr ), "wrong password\n" );
    pSspiLocalFree( (void *)username_ptr );
    pSspiLocalFree( (void *)domainname_ptr );
    pSspiLocalFree( (void *)password_ptr );

    pSspiZeroAuthIdentity( id );
    status = pSspiEncodeAuthIdentityAsStrings( id, &username_ptr, &domainname_ptr, &password_ptr );
    ok( status == SEC_E_OK, "got %08lx\n", status );
    ok( !lstrcmpW( username, username_ptr ), "wrong username\n" );
    ok( !lstrcmpW( domainname, domainname_ptr ), "wrong domainname\n" );
    ok( password_ptr != NULL, "password_ptr = NULL\n" );
    ok( !lstrcmpW( L"", password_ptr ), "wrong password\n" );
    pSspiLocalFree( (void *)username_ptr );
    pSspiLocalFree( (void *)domainname_ptr );
    pSspiLocalFree( (void *)password_ptr );

    pSspiFreeAuthIdentity( id );

    id = NULL;
    status = pSspiEncodeStringsAsAuthIdentity( username, NULL, password, &id );
    ok( status == SEC_E_OK, "got %08lx\n", status );
    ok( id != NULL, "id not set\n" );

    username_ptr = password_ptr = NULL;
    domainname_ptr = (const WCHAR *)0xdeadbeef;
    status = pSspiEncodeAuthIdentityAsStrings( id, &username_ptr, &domainname_ptr, &password_ptr );
    ok( status == SEC_E_OK, "got %08lx\n", status );
    ok( !lstrcmpW( username, username_ptr ), "wrong username\n" );
    ok( domainname_ptr == NULL, "domainname_ptr not cleared\n" );
    ok( !lstrcmpW( password, password_ptr ), "wrong password\n" );

    pSspiLocalFree( (void *)username_ptr );
    pSspiLocalFree( (void *)password_ptr );
    pSspiFreeAuthIdentity( id );
}

static void test_SspiPrepareForCredWrite(void)
{
    static const WCHAR usernameW[] =
        {'u','s','e','r','n','a','m','e',0};
    static const WCHAR domainnameW[] =
        {'d','o','m','a','i','n','n','a','m','e',0};
    static const WCHAR passwordW[] =
        {'p','a','s','s','w','o','r','d',0};
    static const WCHAR targetW[] =
        {'d','o','m','a','i','n','n','a','m','e','\\','u','s','e','r','n','a','m','e',0};
    static const WCHAR target2W[] =
        {'d','o','m','a','i','n','n','a','m','e','2','\\','u','s','e','r','n','a','m','e','2',0};
    const WCHAR *target, *username;
    PSEC_WINNT_AUTH_IDENTITY_OPAQUE id;
    SECURITY_STATUS status;
    ULONG type, size;
    UCHAR *blob;

    if (!pSspiPrepareForCredWrite)
    {
        win_skip( "SspiPrepareForCredWrite not exported by secur32.dll\n" );
        return;
    }

    status = pSspiEncodeStringsAsAuthIdentity( usernameW, domainnameW, passwordW, &id );
    ok( status == SEC_E_OK, "got %08lx\n", status );

    type = size = 0;
    status = pSspiPrepareForCredWrite( id, NULL, &type, &target, &username, &blob, &size );
    ok( status == SEC_E_OK, "got %08lx\n", status );
    ok( type == CRED_TYPE_DOMAIN_PASSWORD, "got %lu\n", type );
    ok( !lstrcmpW( target, targetW ), "got %s\n", wine_dbgstr_w(target) );
    ok( !lstrcmpW( username, targetW ), "got %s\n", wine_dbgstr_w(username) );
    ok( !memcmp( blob, passwordW, sizeof(passwordW) - sizeof(WCHAR) ), "wrong data\n" );
    ok( size == sizeof(passwordW) - sizeof(WCHAR), "got %lu\n", size );
    pSspiLocalFree( (void *)target );
    pSspiLocalFree( (void *)username );
    pSspiLocalFree( blob );

    type = size = 0;
    status = pSspiPrepareForCredWrite( id, target2W, &type, &target, &username, &blob, &size );
    ok( status == SEC_E_OK, "got %08lx\n", status );
    ok( type == CRED_TYPE_DOMAIN_PASSWORD, "got %lu\n", type );
    ok( !lstrcmpW( target, target2W ), "got %s\n", wine_dbgstr_w(target) );
    ok( !lstrcmpW( username, targetW ), "got %s\n", wine_dbgstr_w(username) );
    ok( !memcmp( blob, passwordW, sizeof(passwordW) - sizeof(WCHAR) ), "wrong data\n" );
    ok( size == sizeof(passwordW) - sizeof(WCHAR), "got %lu\n", size );
    pSspiLocalFree( (void *)target );
    pSspiLocalFree( (void *)username );
    pSspiLocalFree( blob );

    pSspiFreeAuthIdentity( id );
}

static void test_SspiMarshalAuthIdentity(void)
{
    struct
    {
        ULONG version;
        ULONG length;
        ULONG user_off;
        ULONG user_len;
        ULONG domain_off;
        ULONG domain_len;
        ULONG password_off;
        ULONG password_len;
        ULONG flags;
        ULONG package_list_off;
        ULONG package_list_len;
    } *data;

    PSEC_WINNT_AUTH_IDENTITY_OPAQUE opaque_id;
    SEC_WINNT_AUTH_IDENTITY_EX2 idex2;
    SEC_WINNT_AUTH_IDENTITY_EXW idex, *idexw;
    SEC_WINNT_AUTH_IDENTITY_EXA *idexa;
    SEC_WINNT_AUTH_IDENTITY_A id;
    SECURITY_STATUS status;
    ULONG len;

    if (!pSspiMarshalAuthIdentity)
    {
        win_skip( "SspiMarshalAuthIdentity not available\n" );
        return;
    }

    status = pSspiEncodeStringsAsAuthIdentity( L"user", L"domain", L"password", &opaque_id );
    ok( status == SEC_E_OK, "got %08lx\n", status );
    status = pSspiMarshalAuthIdentity( opaque_id, &len, (char **)&data );
    ok( status == SEC_E_OK, "got %08lx\n", status );
    ok( len == sizeof(*data) + sizeof(L"userdomainpassword") - sizeof(WCHAR) + sizeof(DWORD), "len = %lu\n", len );
    ok( data->version == SEC_WINNT_AUTH_IDENTITY_VERSION, "data->version = %lu\n", data->version );
    ok( data->length == sizeof(*data), "data->length = %lu\n", data->length );
    ok( data->user_off == sizeof(*data), "data->user_off = %lu\n", data->user_off );
    ok( data->user_len == wcslen(L"user"), "data->user_len = %lu\n", data->user_len );
    ok( data->domain_off == data->user_off + data->user_len * sizeof(WCHAR),
            "data->domain_off = %lu\n", data->domain_off );
    ok( data->domain_len == wcslen(L"domain"), "data->domain_len = %lu\n", data->domain_len );
    ok( data->password_off == data->domain_off + data->domain_len * sizeof(WCHAR),
            "data->password_off = %lu\n", data->password_off );
    ok( data->password_len == wcslen(L"password"), "data->password_len = %lu\n", data->password_len );
    ok( data->flags == (SEC_WINNT_AUTH_IDENTITY_UNICODE | SEC_WINNT_AUTH_IDENTITY_MARSHALLED),
            "data->flags = %lx\n", data->flags );
    ok( data->package_list_off == 0, "data->package_list_off = %lu\n", data->package_list_off );
    ok( data->package_list_len == 0, "data->package_list_len = %lu\n", data->package_list_len );
    ok( !memcmp((BYTE *)data + data->user_off, L"user", data->user_len * sizeof(WCHAR)),
            "user = %s\n", wine_dbgstr_wn((WCHAR *)((BYTE *)data + data->user_off), data->user_len) );
    ok( !memcmp((BYTE *)data + data->domain_off, L"domain", data->domain_len * sizeof(WCHAR)),
            "domain = %s\n", wine_dbgstr_wn((WCHAR *)((BYTE *)data + data->domain_off), data->domain_len) );
    ok( !memcmp((BYTE *)data + data->password_off, L"password", data->password_len * sizeof(WCHAR)),
            "password = %s\n", wine_dbgstr_wn((WCHAR *)((BYTE *)data + data->password_off), data->password_len) );
    pSspiFreeAuthIdentity( opaque_id );

    status = pSspiUnmarshalAuthIdentity( len, (char *)data, &opaque_id );
    ok( status == SEC_E_OK, "got %08lx\n", status );
    pSspiLocalFree( data );
    idexw = opaque_id;
    ok( idexw->Version == SEC_WINNT_AUTH_IDENTITY_VERSION, "Version = %lx\n", idexw->Version );
    ok( idexw->Length == sizeof(*idexw), "Length = %lu\n", idexw->Length );
    ok( !memcmp(idexw->User, L"user", idexw->UserLength * sizeof(WCHAR) ),
            "User = %s\n", wine_dbgstr_wn(idexw->User, idexw->UserLength) );
    ok( (BYTE *)idexw->User == (BYTE *)idexw + sizeof(*idexw),
            "User = %p, opaque_id = %p\n", idexw->User, idexw);
    ok( !memcmp(idexw->Domain, L"domain", idexw->DomainLength * sizeof(WCHAR) ),
            "Domain = %s\n", wine_dbgstr_wn(idexw->Domain, idexw->DomainLength) );
    ok( !memcmp(idexw->Password, L"password", idexw->PasswordLength * sizeof(WCHAR) ),
            "Password = %s\n", wine_dbgstr_wn(idexw->Password, idexw->PasswordLength) );
    ok( idexw->Flags == (SEC_WINNT_AUTH_IDENTITY_UNICODE | SEC_WINNT_AUTH_IDENTITY_MARSHALLED),
            "Flags = %lx\n", idexw->Flags );
    ok( !idexw->PackageList, "PackageList = %p\n", idexw->PackageList );
    ok( !idexw->PackageListLength, "PackageListLength = %lu\n", idexw->PackageListLength );
    pSspiFreeAuthIdentity( opaque_id );

    /* use SEC_WINNT_AUTH_IDENTITY_A as opaque auth identity */
    memset( &id, 0, sizeof(id) );
    id.User = (unsigned char *)"user";
    id.UserLength = strlen( (char *)id.User );
    id.Domain = (unsigned char *)"domain";
    id.DomainLength = strlen( (char *)id.Domain );
    id.Password = (unsigned char*)"password";
    id.PasswordLength = strlen( (char *)id.Password );
    id.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
    status = pSspiMarshalAuthIdentity( &id, &len, (char **)&data );
    ok( status == SEC_E_OK, "got %08lx\n", status );
    ok( len == sizeof(*data) + sizeof("userdomainpassword") - sizeof(char) + sizeof(DWORD), "len = %lu\n", len );
    ok( data->version == SEC_WINNT_AUTH_IDENTITY_VERSION, "data->version = %lu\n", data->version );
    ok( data->length == sizeof(*data), "data->length = %lu\n", data->length );
    ok( data->user_off == sizeof(*data), "data->user_off = %lu\n", data->user_off );
    ok( data->user_len == wcslen(L"user"), "data->user_len = %lu\n", data->user_len );
    ok( data->domain_off == data->user_off + data->user_len,
            "data->domain_off = %lu\n", data->domain_off );
    ok( data->domain_len == wcslen(L"domain"), "data->domain_len = %lu\n", data->domain_len );
    ok( data->password_off == data->domain_off + data->domain_len,
            "data->password_off = %lu\n", data->password_off );
    ok( data->password_len == wcslen(L"password"), "data->password_len = %lu\n", data->password_len );
    ok( data->flags == SEC_WINNT_AUTH_IDENTITY_ANSI, "data->flags = %lx\n", data->flags );
    ok( data->package_list_off == 0, "data->package_list_off = %lu\n", data->package_list_off );
    ok( data->package_list_len == 0, "data->package_list_len = %lu\n", data->package_list_len );
    ok( !memcmp((BYTE *)data + data->user_off, "user", data->user_len),
            "user = %s\n", wine_dbgstr_an((char *)data + data->user_off, data->user_len) );
    ok( !memcmp((BYTE *)data + data->domain_off, "domain", data->domain_len),
            "domain = %s\n", wine_dbgstr_an((char *)data + data->domain_off, data->domain_len) );
    ok( !memcmp((BYTE *)data + data->password_off, "password", data->password_len),
            "password = %s\n", wine_dbgstr_an((char *)data + data->password_off, data->password_len) );

    status = pSspiUnmarshalAuthIdentity( len, (char *)data, &opaque_id );
    ok( status == SEC_E_OK, "got %08lx\n", status );
    pSspiLocalFree( data );
    idexa = opaque_id;
    ok( idexa->Version == SEC_WINNT_AUTH_IDENTITY_VERSION, "Version = %lx\n", idexa->Version );
    ok( idexa->Length == sizeof(*idexa), "Length = %lu\n", idexa->Length );
    ok( !memcmp(idexa->User, "user", idexa->UserLength ),
            "User = %s\n", wine_dbgstr_an((char *)idexa->User, idexa->UserLength) );
    ok( idexa->User == (UCHAR *)idexa + sizeof(*idexa),
            "User = %p, opaque_id = %p\n", idexa->User, idexa);
    ok( !memcmp(idexa->Domain, "domain", idexa->DomainLength ),
            "Domain = %s\n", wine_dbgstr_an((char *)idexa->Domain, idexa->DomainLength) );
    ok( !memcmp(idexa->Password, "password", idexa->PasswordLength ),
            "Password = %s\n", wine_dbgstr_an((char *)idexa->Password, idexa->PasswordLength) );
    ok( idexa->Flags == (SEC_WINNT_AUTH_IDENTITY_ANSI | SEC_WINNT_AUTH_IDENTITY_MARSHALLED),
            "Flags = %lx\n", idexa->Flags );
    ok( !idexa->PackageList, "PackageList = %p\n", idexa->PackageList );
    ok( !idexa->PackageListLength, "PackageListLength = %lu\n", idexa->PackageListLength );
    pSspiFreeAuthIdentity( opaque_id );

    /* use SEC_WINNT_AUTH_IDENTITY_EXW as opaque auth identity */
    memset( &idex, 0, sizeof(idex) );
    idex.Version = SEC_WINNT_AUTH_IDENTITY_VERSION;
    idex.Length = sizeof(idex);
    idex.User = (WCHAR *)L"user";
    idex.UserLength = wcslen( L"user" );
    idex.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
    status = pSspiMarshalAuthIdentity( &idex, &len, (char **)&data );
    ok( status == SEC_E_OK, "got %08lx\n", status );
    ok( len == sizeof(*data) + sizeof(L"user") - sizeof(WCHAR) + sizeof(DWORD), "len = %lu\n", len );
    ok( data->version == SEC_WINNT_AUTH_IDENTITY_VERSION, "data->version = %lu\n", data->version );
    ok( data->length == sizeof(*data), "data->length = %lu\n", data->length );
    ok( data->user_off == sizeof(*data), "data->user_off = %lu\n", data->user_off );
    ok( data->user_len == wcslen(L"user"), "data->user_len = %lu\n", data->user_len );
    ok( data->domain_off == 0, "data->domain_off = %lu\n", data->domain_off );
    ok( data->domain_len == 0, "data->domain_len = %lu\n", data->domain_len );
    ok( data->password_off == 0, "data->password_off = %lu\n", data->password_off );
    ok( data->password_len == 0, "data->password_len = %lu\n", data->password_len );
    ok( data->flags == SEC_WINNT_AUTH_IDENTITY_UNICODE, "data->flags = %lx\n", data->flags );
    ok( data->package_list_off == 0, "data->package_list_off = %lu\n", data->package_list_off );
    ok( data->package_list_len == 0, "data->package_list_len = %lu\n", data->package_list_len );
    ok( !memcmp((BYTE *)data + data->user_off, L"user", data->user_len * sizeof(WCHAR)),
            "user = %s\n", wine_dbgstr_wn((WCHAR *)((BYTE *)data + data->user_off), data->user_len) );

    status = pSspiUnmarshalAuthIdentity( len, (char *)data, &opaque_id );
    ok( status == SEC_E_OK, "got %08lx\n", status );
    pSspiLocalFree( data );
    idexw = opaque_id;
    ok( idexw->Version == SEC_WINNT_AUTH_IDENTITY_VERSION, "Version = %lx\n", idexw->Version );
    ok( idexw->Length == sizeof(*idexw), "Length = %lu\n", idexw->Length );
    ok( !memcmp(idexw->User, L"user", idexw->UserLength * sizeof(WCHAR) ),
            "User = %s\n", wine_dbgstr_wn(idexw->User, idexw->UserLength) );
    ok( (BYTE *)idexw->User == (BYTE *)idexw + sizeof(*idexw),
            "User = %p, opaque_id = %p\n", idexw->User, idexw);
    ok( !idexw->Domain, "Domain = %p\n", idexw->Domain );
    ok( !idexw->DomainLength, "DomainLength = %lu\n", idexw->DomainLength );
    ok( !idexw->Password, "Password = %p\n", idexw->Password );
    ok( !idexw->PasswordLength, "PasswordLength = %lu\n", idexw->PasswordLength );
    ok( idexw->Flags == (SEC_WINNT_AUTH_IDENTITY_UNICODE | SEC_WINNT_AUTH_IDENTITY_MARSHALLED),
            "Flags = %lx\n", idexw->Flags );
    ok( !idexw->PackageList, "PackageList = %p\n", idexw->PackageList );
    ok( !idexw->PackageListLength, "PackageListLength = %lu\n", idexw->PackageListLength );
    pSspiFreeAuthIdentity( opaque_id );

    /* use SEC_WINNT_AUTH_IDENTITY_EX2 as opaque auth identity */
    memset( &idex2, 0, sizeof(idex2) );
    idex2.Version = SEC_WINNT_AUTH_IDENTITY_VERSION_2;
    idex2.cbHeaderLength = sizeof(idex2);
    idex2.cbStructureLength = sizeof(idex2);
    status = pSspiMarshalAuthIdentity( &idex2, &len, (char **)&data );
    ok( status == SEC_E_OK, "got %08lx\n", status );
    todo_wine ok( len == sizeof(idex2) + 2 * sizeof(DWORD), "len = %lu\n", len );
    ok( !memcmp(&idex2, data, sizeof(idex2)), "data differs\n" );

    status = pSspiUnmarshalAuthIdentity( len, (char *)data, &opaque_id );
    ok( status == SEC_E_OK, "got %08lx\n", status );
    pSspiLocalFree( data );
    ok( !memcmp(opaque_id, &idex2, sizeof(idex2)), "data differs\n" );
    pSspiFreeAuthIdentity( opaque_id );
}

static void test_kerberos(void)
{
    SecPkgInfoA *info;
    CredHandle cred;
    SECURITY_STATUS status;

    SEC_CHAR provider[] = {'K','e','r','b','e','r','o','s',0};

    static const ULONG expected_flags =
          SECPKG_FLAG_INTEGRITY
        | SECPKG_FLAG_PRIVACY
        | SECPKG_FLAG_TOKEN_ONLY
        | SECPKG_FLAG_DATAGRAM
        | SECPKG_FLAG_CONNECTION
        | SECPKG_FLAG_MULTI_REQUIRED
        | SECPKG_FLAG_EXTENDED_ERROR
        | SECPKG_FLAG_IMPERSONATION
        | SECPKG_FLAG_ACCEPT_WIN32_NAME
        | SECPKG_FLAG_NEGOTIABLE
        | SECPKG_FLAG_GSS_COMPATIBLE
        | SECPKG_FLAG_LOGON
        | SECPKG_FLAG_MUTUAL_AUTH
        | SECPKG_FLAG_DELEGATION
        | SECPKG_FLAG_READONLY_WITH_CHECKSUM;
    static const ULONG optional_mask =
          SECPKG_FLAG_RESTRICTED_TOKENS
        | SECPKG_FLAG_APPCONTAINER_CHECKS
        | SECPKG_FLAG_APPLY_LOOPBACK;

    status = QuerySecurityPackageInfoA(provider, &info);
    ok(status == SEC_E_OK, "Kerberos package not installed (%08lx), skipping test\n", status);
    if(status != SEC_E_OK)
        return;

    ok( (info->fCapabilities & ~optional_mask) == expected_flags, "got %08lx, expected %08lx\n", info->fCapabilities, expected_flags );
    ok( info->wVersion == 1, "got %u\n", info->wVersion );
    ok( info->wRPCID == RPC_C_AUTHN_GSS_KERBEROS, "got %u\n", info->wRPCID );
    ok( info->cbMaxToken == 48000 || broken(info->cbMaxToken == 12000) /* Win7 */, "got %lu\n", info->cbMaxToken );
    ok( !lstrcmpA( info->Name, "Kerberos" ), "got %s\n", info->Name );
    ok( !lstrcmpA( info->Comment, "Microsoft Kerberos V1.0" ), "got %s\n", info->Comment );
    FreeContextBuffer( info );

    status = AcquireCredentialsHandleA( NULL, provider, SECPKG_CRED_OUTBOUND, NULL,
                                        NULL, NULL, NULL, &cred, NULL );
    todo_wine ok( status == SEC_E_OK, "AcquireCredentialsHandleA returned %08lx\n", status );
    if(status == SEC_E_OK)
        FreeCredentialHandle( &cred );
}

static void test_ticket_cache(void)
{
    KERB_QUERY_TKT_CACHE_REQUEST req = { KerbQueryTicketCacheMessage };
    KERB_QUERY_TKT_CACHE_RESPONSE *resp;
    NTSTATUS status;
    HANDLE lsa;
    ULONG package, len, i;
    LSA_STRING name;

    status = LsaConnectUntrusted( &lsa );
    ok( !status, "got %08lx\n", status );

    RtlInitAnsiString( &name, MICROSOFT_KERBEROS_NAME_A );
    status = LsaLookupAuthenticationPackage( lsa, &name, &package );
    ok(status == SEC_E_OK, "Kerberos package not installed (%08lx), skipping test\n", status);
    if(status != SEC_E_OK)
    {
      LsaDeregisterLogonProcess( lsa );
      return;
    }

    status = LsaCallAuthenticationPackage( lsa, package, &req, sizeof(req), (void **)&resp, &len, &status );
    ok( !status, "got %08lx\n", status );
    ok( resp->MessageType == KerbQueryTicketCacheMessage, "got %u\n", resp->MessageType );

    for (i = 0; i < resp->CountOfTickets; i++)
    {
        KERB_TICKET_CACHE_INFO *info = &resp->Tickets[i];
        trace( "ServerName %s\n", wine_dbgstr_wn(info->ServerName.Buffer, info->ServerName.Length/sizeof(WCHAR)) );
        trace( "RealmName %s\n", wine_dbgstr_wn(info->RealmName.Buffer, info->RealmName.Length/sizeof(WCHAR)) );
        trace( "EncryptionType %08lx\n", info->EncryptionType );
        trace( "TicketFlags %08lx\n", info->TicketFlags );
    }
    LsaFreeReturnBuffer( resp );
    LsaDeregisterLogonProcess( lsa );
}

START_TEST(secur32)
{
    secdll = LoadLibraryA("secur32.dll");

    if (!secdll)
        secdll = LoadLibraryA("security.dll");

    if (secdll)
    {
        pSspiEncodeAuthIdentityAsStrings = (void *)GetProcAddress(secdll, "SspiEncodeAuthIdentityAsStrings");
        pSspiEncodeStringsAsAuthIdentity = (void *)GetProcAddress(secdll, "SspiEncodeStringsAsAuthIdentity");
        pSspiFreeAuthIdentity = (void *)GetProcAddress(secdll, "SspiFreeAuthIdentity");
        pSspiLocalFree = (void *)GetProcAddress(secdll, "SspiLocalFree");
        pSspiMarshalAuthIdentity = (void *)GetProcAddress(secdll, "SspiMarshalAuthIdentity");
        pSspiPrepareForCredWrite = (void *)GetProcAddress(secdll, "SspiPrepareForCredWrite");
        pSspiUnmarshalAuthIdentity = (void *)GetProcAddress(secdll, "SspiUnmarshalAuthIdentity");
        pSspiZeroAuthIdentity = (void *)GetProcAddress(secdll, "SspiZeroAuthIdentity");
        pGetComputerObjectNameA = (PVOID)GetProcAddress(secdll, "GetComputerObjectNameA");
        pGetComputerObjectNameW = (PVOID)GetProcAddress(secdll, "GetComputerObjectNameW");
        pGetUserNameExA = (PVOID)GetProcAddress(secdll, "GetUserNameExA");
        pGetUserNameExW = (PVOID)GetProcAddress(secdll, "GetUserNameExW");
        pInitSecurityInterfaceA = (PVOID)GetProcAddress(secdll, "InitSecurityInterfaceA");
        pInitSecurityInterfaceW = (PVOID)GetProcAddress(secdll, "InitSecurityInterfaceW");

        if (pGetComputerObjectNameA)
            testGetComputerObjectNameA();
        else
            win_skip("GetComputerObjectNameA not exported by secur32.dll\n");

        if (pGetComputerObjectNameW)
            testGetComputerObjectNameW();
        else
            win_skip("GetComputerObjectNameW not exported by secur32.dll\n");

        if (pGetUserNameExA)
            testGetUserNameExA();
        else
            win_skip("GetUserNameExA not exported by secur32.dll\n");

        if (pGetUserNameExW)
            testGetUserNameExW();
        else
            win_skip("GetUserNameExW not exported by secur32.dll\n");

        test_InitSecurityInterface();
        test_SspiEncodeStringsAsAuthIdentity();
        test_SspiPrepareForCredWrite();
        test_SspiMarshalAuthIdentity();

        FreeLibrary(secdll);
    }

    test_kerberos();
    test_ticket_cache();
}
