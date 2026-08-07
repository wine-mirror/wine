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
    TRACE( "%p\n", opaque_id );
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
