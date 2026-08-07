/*
 * Copyright 2016 Hans Leidekker for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "rpc.h"
#include "sspi.h"
#include "wincred.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(sspicli);

struct auth_identity_marshalled
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
};

/***********************************************************************
 *		SspiEncodeStringsAsAuthIdentity (SECUR32.0)
 */
SECURITY_STATUS SEC_ENTRY SspiEncodeStringsAsAuthIdentity(
    const WCHAR *username, const WCHAR *domainname, const WCHAR *creds,
    PSEC_WINNT_AUTH_IDENTITY_OPAQUE *opaque_id )
{
    SEC_WINNT_AUTH_IDENTITY_EXW *id;
    DWORD len_username = 0, len_domainname = 0, len_password = 0, size;
    WCHAR *ptr;

    TRACE( "%s %s %s %p\n", debugstr_w(username), debugstr_w(domainname),
           debugstr_w(creds), opaque_id );

    if (!username && !domainname && !creds) return SEC_E_INVALID_TOKEN;

    if (username) len_username = lstrlenW( username );
    if (domainname) len_domainname = lstrlenW( domainname );
    if (creds) len_password = lstrlenW( creds );

    size = sizeof(*id) + (len_username + len_domainname + len_password) * sizeof(WCHAR) + sizeof(DWORD);
    if (!(id = calloc( 1, size ))) return ERROR_OUTOFMEMORY;
    ptr = (WCHAR *)(id + 1);

    id->Version = SEC_WINNT_AUTH_IDENTITY_VERSION;
    id->Length = sizeof(*id);
    id->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE | SEC_WINNT_AUTH_IDENTITY_MARSHALLED;
    if (username)
    {
        memcpy( ptr, username, len_username * sizeof(WCHAR) );
        id->User       = ptr;
        id->UserLength = len_username;
        ptr += len_username;
    }
    if (domainname)
    {
        memcpy( ptr, domainname, len_domainname * sizeof(WCHAR) );
        id->Domain       = ptr;
        id->DomainLength = len_domainname;
        ptr += len_domainname;
    }
    if (creds)
    {
        memcpy( ptr, creds, len_password * sizeof(WCHAR) );
        id->Password       = ptr;
        id->PasswordLength = len_password;
    }

    *opaque_id = id;
    return SEC_E_OK;
}

/***********************************************************************
 *		SspiZeroAuthIdentity (SECUR32.0)
 */
void SEC_ENTRY SspiZeroAuthIdentity( PSEC_WINNT_AUTH_IDENTITY_OPAQUE opaque_id )
{
    SEC_WINNT_AUTH_IDENTITY_EXW *idex = (SEC_WINNT_AUTH_IDENTITY_EXW *)opaque_id;
    ULONG char_size;

    TRACE( "%p\n", opaque_id );

    if (!idex) return;

    if (idex->Version >= 0x10000)
    {
        SEC_WINNT_AUTH_IDENTITY_W *id = (SEC_WINNT_AUTH_IDENTITY_W *)opaque_id;

        char_size = id->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI ? sizeof(char) : sizeof(WCHAR);
        memset( id->Password, 0, id->PasswordLength * char_size );
    }
    else if (idex->Version == SEC_WINNT_AUTH_IDENTITY_VERSION)
    {
        char_size = idex->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI ? sizeof(char) : sizeof(WCHAR);
        memset( idex->Password, 0, idex->PasswordLength * char_size );
    }
    else if (idex->Version == SEC_WINNT_AUTH_IDENTITY_VERSION_2)
    {
        SEC_WINNT_AUTH_IDENTITY_EX2 *id = (SEC_WINNT_AUTH_IDENTITY_EX2 *)opaque_id;

        memset( (char *)id + id->PackedCredentialsOffset, 0, id->PackedCredentialsLength );
    }
    else
    {
        FIXME( "auth identity format not handled: %lu\n", idex->Version );
    }
}

static WCHAR *dup_auth_str( void *data, ULONG len, ULONG flags )
{
    ULONG ret_len;
    WCHAR *ret;

    if (!len) return NULL;

    if (flags & SEC_WINNT_AUTH_IDENTITY_ANSI)
    {
        ret_len = MultiByteToWideChar( CP_ACP, 0, data, len, NULL, 0 );
        ret = LocalAlloc( LMEM_FIXED, (ret_len + 1) * sizeof(WCHAR) );
        if (!ret) return ret;
        MultiByteToWideChar( CP_ACP, 0, data, len, ret, ret_len );
        ret[ret_len] = 0;
        return ret;
    }

    ret = LocalAlloc( LMEM_FIXED, (len + 1) * sizeof(WCHAR) );
    if (!ret) return ret;
    memcpy( ret, data, len * sizeof(WCHAR) );
    ret[len] = 0;
    return ret;
}

/***********************************************************************
 *		SspiEncodeAuthIdentityAsStrings (SECUR32.0)
 */
SECURITY_STATUS SEC_ENTRY SspiEncodeAuthIdentityAsStrings(
    PSEC_WINNT_AUTH_IDENTITY_OPAQUE opaque_id, PCWSTR *username,
    PCWSTR *domainname, PCWSTR *creds )
{
    SEC_WINNT_AUTH_IDENTITY_EXW *idex = (SEC_WINNT_AUTH_IDENTITY_EXW *)opaque_id;

    TRACE("%p %p %p %p\n", opaque_id, username, domainname, creds);

    if (idex->Version >= 0x10000)
    {
        SEC_WINNT_AUTH_IDENTITY_W *id = (SEC_WINNT_AUTH_IDENTITY_W *)opaque_id;

        *username = dup_auth_str( id->User, id->UserLength, id->Flags );
        *domainname = dup_auth_str( id->Domain, id->DomainLength, id->Flags );
        *creds = dup_auth_str( id->Password, id->PasswordLength, id->Flags );

    }
    else if (idex->Version == SEC_WINNT_AUTH_IDENTITY_VERSION)
    {
        *username = dup_auth_str( idex->User, idex->UserLength, idex->Flags );
        *domainname = dup_auth_str( idex->Domain, idex->DomainLength, idex->Flags );
        *creds = dup_auth_str( idex->Password, idex->PasswordLength, idex->Flags );
    }
    else
    {
        FIXME( "auth identity format not handled: %lu\n", idex->Version );
        return SEC_E_INTERNAL_ERROR;
    }

    return SEC_E_OK;
}

/***********************************************************************
 *		SspiFreeAuthIdentity (SECUR32.0)
 */
void SEC_ENTRY SspiFreeAuthIdentity( PSEC_WINNT_AUTH_IDENTITY_OPAQUE opaque_id )
{
    SEC_WINNT_AUTH_IDENTITY_EXW *idex = (SEC_WINNT_AUTH_IDENTITY_EXW *)opaque_id;

    TRACE( "%p\n", opaque_id );

    SspiZeroAuthIdentity( opaque_id );

    if (idex->Version >= 0x10000)
    {
        SEC_WINNT_AUTH_IDENTITY_W *id = (SEC_WINNT_AUTH_IDENTITY_W *)opaque_id;

        if (!(id->Flags & SEC_WINNT_AUTH_IDENTITY_MARSHALLED))
        {
            SspiLocalFree( id->User );
            SspiLocalFree( id->Domain );
            SspiLocalFree( id->Password );
        }
    }
    else if (idex->Version == SEC_WINNT_AUTH_IDENTITY_VERSION)
    {
        if (!(idex->Flags & SEC_WINNT_AUTH_IDENTITY_MARSHALLED))
        {
            SspiLocalFree( idex->User );
            SspiLocalFree( idex->Domain );
            SspiLocalFree( idex->Password );
        }
    }
    else if (idex->Version != SEC_WINNT_AUTH_IDENTITY_VERSION_2)
    {
        FIXME( "auth identity format not handled: %lu\n", idex->Version );
    }
    SspiLocalFree( opaque_id );
}

/***********************************************************************
 *		SspiLocalFree (SECUR32.0)
 */
void SEC_ENTRY SspiLocalFree( void *ptr )
{
    TRACE( "%p\n", ptr );
    LocalFree( ptr );
}

/***********************************************************************
 *		SspiPrepareForCredWrite (SECUR32.0)
 */
SECURITY_STATUS SEC_ENTRY SspiPrepareForCredWrite( PSEC_WINNT_AUTH_IDENTITY_OPAQUE opaque_id,
    PCWSTR target, PULONG type, PCWSTR *targetname, PCWSTR *username, PUCHAR *blob, PULONG size )
{
    const WCHAR *user, *domain, *password;
    SECURITY_STATUS status;
    WCHAR *str, *str2;
    ULONG len;

    FIXME( "%p %s %p %p %p %p %p\n", opaque_id, debugstr_w(target), type, targetname, username,
           blob, size );

    status = SspiEncodeAuthIdentityAsStrings( opaque_id, &user, &domain, &password );
    if (status) return status;

    if (domain)
    {
        len = (wcslen( user ) + wcslen( domain ) + 2) * sizeof(WCHAR);
        if (!(str = LocalAlloc( LMEM_FIXED, len ))) goto err;
        wcscpy( str, domain );
        wcscat( str, L"\\" );
        wcscat( str, user );
    }
    else
    {
        len = (wcslen( user ) + 1) * sizeof(WCHAR);
        if (!(str = LocalAlloc( LMEM_FIXED, len ))) goto err;
        wcscpy( str, user );
    }

    str2 = LocalAlloc( LMEM_FIXED, target ? (wcslen( target ) + 1) * sizeof(WCHAR) : len );
    str2 = target ? wcsdup( target ) : wcsdup( str );
    if (!str2) goto err;
    wcscpy( str2, target ? target : str );

    SspiLocalFree( (void *)user );
    SspiLocalFree( (void *)domain );

    *type = CRED_TYPE_DOMAIN_PASSWORD;
    *username = str;
    *targetname = str2;
    *blob = (UCHAR *)password;
    *size = wcslen( password ) * sizeof(WCHAR);
    return SEC_E_OK;

err:
    SspiLocalFree( (void *)user );
    SspiLocalFree( (void *)domain );
    if (password) SecureZeroMemory( (void *)password, wcslen(password) * sizeof(WCHAR) );
    SspiLocalFree( (void *)password );
    SspiLocalFree( (void *)str );
    return SEC_E_INSUFFICIENT_MEMORY;
}

/***********************************************************************
 *		SspiMarshalAuthIdentity
 */
SECURITY_STATUS SEC_ENTRY SspiMarshalAuthIdentity(
        PSEC_WINNT_AUTH_IDENTITY_OPAQUE opaque_id, ULONG *len, char **byte_array )
{
    SEC_WINNT_AUTH_IDENTITY_EXW *idex = opaque_id;
    struct auth_identity_marshalled *marshalled;
    ULONG size, char_size;
    BYTE *data;

    TRACE( "%p %p %p\n", opaque_id, len, byte_array );

    if (idex->Version >= 0x10000)
    {
        SEC_WINNT_AUTH_IDENTITY_W *id = (SEC_WINNT_AUTH_IDENTITY_W *)opaque_id;

        char_size = id->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI ? sizeof(char) : sizeof(WCHAR);
        size = id->UserLength + id->DomainLength + id->PasswordLength;
        size = sizeof(*marshalled) + size * char_size + sizeof(DWORD);

        marshalled = LocalAlloc( LMEM_FIXED, size );
        if (!marshalled) return SEC_E_INSUFFICIENT_MEMORY;
        memset( marshalled, 0, sizeof(*marshalled) );
        data = (BYTE *)(marshalled + 1);

        marshalled->version = SEC_WINNT_AUTH_IDENTITY_VERSION;
        marshalled->length = sizeof(*marshalled);
        if (id->User)
        {
            marshalled->user_off = data - (BYTE *)marshalled;
            memcpy( data, id->User, id->UserLength * char_size );
            data += id->UserLength * char_size;
        }
        marshalled->user_len = id->UserLength;
        if (id->Domain)
        {
            marshalled->domain_off = data - (BYTE *)marshalled;
            memcpy( data, id->Domain, id->DomainLength * char_size );
            data += id->DomainLength * char_size;
        }
        marshalled->domain_len = id->DomainLength;
        if (id->Password)
        {
            marshalled->password_off = data - (BYTE *)marshalled;
            memcpy( data, id->Password, id->PasswordLength * char_size );
            data += id->PasswordLength * char_size;
        }
        marshalled->password_len = id->PasswordLength;
        marshalled->flags = id->Flags;

        *len = size;
        *byte_array = (char *)marshalled;
    }
    else if (idex->Version == SEC_WINNT_AUTH_IDENTITY_VERSION)
    {
        char_size = idex->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI ? sizeof(char) : sizeof(WCHAR);
        size = idex->UserLength + idex->DomainLength + idex->PasswordLength + idex->PackageListLength;
        size = sizeof(*marshalled) + size * char_size + sizeof(DWORD);

        marshalled = LocalAlloc( LMEM_FIXED, size );
        if (!marshalled) return SEC_E_INSUFFICIENT_MEMORY;
        memset( marshalled, 0, sizeof(*marshalled) );
        data = (BYTE *)(marshalled + 1);

        marshalled->version = SEC_WINNT_AUTH_IDENTITY_VERSION;
        marshalled->length = sizeof(*marshalled);
        if (idex->User)
        {
            marshalled->user_off = data - (BYTE *)marshalled;
            memcpy( data, idex->User, idex->UserLength * char_size );
            data += idex->UserLength * char_size;
        }
        marshalled->user_len = idex->UserLength;
        if (idex->Domain)
        {
            marshalled->domain_off = data - (BYTE *)marshalled;
            memcpy( data, idex->Domain, idex->DomainLength * char_size );
            data += idex->DomainLength * char_size;
        }
        marshalled->domain_len = idex->DomainLength;
        if (idex->Password)
        {
            marshalled->password_off = data - (BYTE *)marshalled;
            memcpy( data, idex->Password, idex->PasswordLength * char_size );
            data += idex->PasswordLength * char_size;
        }
        marshalled->password_len = idex->PasswordLength;
        marshalled->flags = idex->Flags;
        if (idex->PackageList)
        {
            marshalled->package_list_off = data - (BYTE *)marshalled;
            memcpy( data, idex->PackageList, idex->PackageListLength * char_size );
            data += idex->PackageListLength * char_size;
        }
        marshalled->package_list_len = idex->PackageListLength;

        *len = size;
        *byte_array = (char *)marshalled;
    }
    else if (idex->Version == SEC_WINNT_AUTH_IDENTITY_VERSION_2)
    {
        SEC_WINNT_AUTH_IDENTITY_EX2 *id = (SEC_WINNT_AUTH_IDENTITY_EX2 *)opaque_id;

        *byte_array = LocalAlloc( LMEM_FIXED, id->cbStructureLength );
        if (!*byte_array) return SEC_E_INSUFFICIENT_MEMORY;
        memcpy( *byte_array, id, id->cbStructureLength );

        *len = id->cbStructureLength;
    }
    else
    {
        FIXME( "auth identity format not handled: %lu\n", idex->Version );
        return SEC_E_INTERNAL_ERROR;
    }

    return SEC_E_OK;
}

/***********************************************************************
 *		SspiUnmarshalAuthIdentity
 */
SECURITY_STATUS SEC_ENTRY SspiUnmarshalAuthIdentity(
        ULONG len, char *byte_array, PSEC_WINNT_AUTH_IDENTITY_OPAQUE *opaque_id )
{
    struct auth_identity_marshalled *marshalled = (struct auth_identity_marshalled *)byte_array;

    TRACE( "%lu %p %p\n", len, byte_array, opaque_id );

    if (len < sizeof(*marshalled)) return SEC_E_INVALID_TOKEN;

    if (marshalled->version == SEC_WINNT_AUTH_IDENTITY_VERSION)
    {
        SEC_WINNT_AUTH_IDENTITY_EXW *ret;
        ULONG size, char_size;
        BYTE *data;

        if (marshalled->length != sizeof(*marshalled)) return SEC_E_INVALID_TOKEN;

        char_size = marshalled->flags & SEC_WINNT_AUTH_IDENTITY_ANSI ? sizeof(char) : sizeof(WCHAR);
        size = marshalled->user_len + marshalled->domain_len +
            marshalled->password_len + marshalled->package_list_len;
        size = sizeof(*ret) + size * char_size;
        ret = LocalAlloc( LMEM_FIXED, size );
        if (!ret) return SEC_E_INSUFFICIENT_MEMORY;
        memset( ret, 0, size );

        data = (BYTE *)(ret + 1);
        ret->Version = SEC_WINNT_AUTH_IDENTITY_VERSION;
        ret->Length = sizeof(*ret);
        if (marshalled->user_off)
        {
            ret->User = (WCHAR *)data;
            memcpy( data, (BYTE *)marshalled + marshalled->user_off, marshalled->user_len * char_size );
            data += marshalled->user_len * char_size;
        }
        ret->UserLength = marshalled->user_len;
        if (marshalled->domain_off)
        {
            ret->Domain = (WCHAR *)data;
            memcpy( data, (BYTE *)marshalled + marshalled->domain_off, marshalled->domain_len * char_size );
            data += marshalled->domain_len * char_size;
        }
        ret->DomainLength = marshalled->domain_len;
        if (marshalled->password_off)
        {
            ret->Password = (WCHAR *)data;
            memcpy( data, (BYTE *)marshalled + marshalled->password_off, marshalled->password_len * char_size );
            data += marshalled->password_len * char_size;
        }
        ret->PasswordLength = marshalled->password_len;
        ret->Flags = marshalled->flags | SEC_WINNT_AUTH_IDENTITY_MARSHALLED;
        if (marshalled->package_list_off)
        {
            ret->PackageList = (WCHAR *)data;
            memcpy( data, (BYTE *)marshalled + marshalled->package_list_off,
                    marshalled->package_list_len * char_size );
            data += marshalled->package_list_len * char_size;
        }
        ret->PackageListLength = marshalled->package_list_len;

        *opaque_id = ret;
        return SEC_E_OK;
    }
    else if (marshalled->version == SEC_WINNT_AUTH_IDENTITY_VERSION_2)
    {
        SEC_WINNT_AUTH_IDENTITY_EX2 *id, *ret;

        id = (SEC_WINNT_AUTH_IDENTITY_EX2 *)marshalled;
        if (id->cbStructureLength > len) return SEC_E_INVALID_TOKEN;
        ret = LocalAlloc( LMEM_FIXED, sizeof(*ret) );
        if (!ret) return SEC_E_INSUFFICIENT_MEMORY;
        memcpy( ret, id, id->cbStructureLength );

        *opaque_id = ret;
        return SEC_E_OK;
    }

    return SEC_E_INVALID_TOKEN;
}
