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
#include <wchar.h>
#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "sspi.h"
#include "ntsecapi.h"
#include "ntsecpkg.h"
#include "rpc.h"
#include "wincred.h"
#include "wincrypt.h"
#include "lmwksta.h"
#include "lmapibuf.h"
#include "lmerr.h"

#include "wine/list.h"
#include "wine/debug.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

static const LSA_SECPKG_FUNCTION_TABLE *lsa_secpkg_table;

enum message_type
{
    NTLM_NEGOTIATE      = 1,
    NTLM_CHALLENGE      = 2,
    NTLM_AUTHENTICATE   = 3
};

enum negotiate_flags
{
    NTLMSSP_NEGOTIATE_UNICODE                   = 0x00000001,
    NTLM_NEGOTIATE_OEM                          = 0x00000002,
    NTLMSSP_REQUEST_TARGET                      = 0x00000004,
    NTLMSSP_NEGOTIATE_SIGN                      = 0x00000010,
    NTLMSSP_NEGOTIATE_SEAL                      = 0x00000020,
    NTLMSSP_NEGOTIATE_DATAGRAM                  = 0x00000040,
    NTLMSSP_NEGOTIATE_LM_KEY                    = 0x00000080,
    NTLMSSP_NEGOTIATE_NETWARE                   = 0x00000100,
    NTLMSSP_NEGOTIATE_NTLM                      = 0x00000200,
    NTLMSSP_NEGOTIATE_ANONYMOUS                 = 0x00000800,
    NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED       = 0x00001000,
    NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED  = 0x00002000,
    NTLMSSP_NEGOTIATE_LOCAL_CALL                = 0x00004000,
    NTLMSSP_NEGOTIATE_ALWAYS_SIGN               = 0x00008000,
    NTLMSSP_TARGET_TYPE_DOMAIN                  = 0x00010000,
    NTLMSSP_TARGET_TYPE_SERVER                  = 0x00020000,
    NTLMSSP_TARGET_TYPE_SHARE                   = 0x00040000,
    NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY  = 0x00080000,
    NTLMSSP_NEGOTIATE_IDENTIFY                  = 0x00100000,
    NTLMSSP_REQUEST_NON_NT_SESSION_KEY          = 0x00400000,
    NTLMSSP_NEGOTIATE_TARGET_INFO               = 0x00800000,
    NTLMSSP_NEGOTIATE_VERSION                   = 0x02000000,
    NTLMSSP_NEGOTIATE_128                       = 0x20000000,
    NTLMSSP_NEGOTIATE_KEY_EXCH                  = 0x40000000,
    NTLMSSP_NEGOTIATE_56                        = 0x80000000,
};

struct ntlm_negotiate
{
    char signature[8];
    int message_type;
    enum negotiate_flags negotiate_flags;
    unsigned short target_name_len;
    unsigned short target_name_max_len;
    unsigned int target_name_off;
    unsigned short workstation_name_len;
    unsigned short workstation_name_max_len;
    unsigned int workstation_name_off;
    BYTE version[8];
};

struct ntlm_challenge
{
    char signature[8];
    int message_type;
    unsigned short target_name_len;
    unsigned short target_name_max_len;
    unsigned int target_name_off;
    enum negotiate_flags negotiate_flags;
    BYTE challenge[8];
    BYTE reserved[8];
    unsigned short target_info_len;
    unsigned short target_info_max_len;
    unsigned int target_info_off;
    BYTE version[8];
};

struct ntlm_authenticate
{
    char signature[8];
    int message_type;
    unsigned short lm_response_len;
    unsigned short lm_response_max_len;
    unsigned int lm_response_off;
    unsigned short nt_response_len;
    unsigned short nt_response_max_len;
    unsigned int nt_response_off;
    unsigned short domain_len;
    unsigned short domain_max_len;
    unsigned int domain_off;
    unsigned short username_len;
    unsigned short username_max_len;
    unsigned int username_off;
    unsigned short workstation_len;
    unsigned short workstation_max_len;
    unsigned int workstation_off;
    unsigned short random_session_key_len;
    unsigned short random_session_key_max_len;
    unsigned int random_session_key_off;
    unsigned int negotiate_flags;
    BYTE version[8];
    BYTE mic[16];
};

struct local_auth
{
    struct list entry;
    BYTE challenge[8];
    char session_key[16];
    BOOL got_token;
    HANDLE token;
};

static struct list local_auths_list = LIST_INIT(local_auths_list);

static CRITICAL_SECTION local_auth_cs;
static CRITICAL_SECTION_DEBUG local_auth_debug =
{
    0, 0, &local_auth_cs,
    { &local_auth_debug.ProcessLocksList, &local_auth_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": local_auth_cs") }
};
static CRITICAL_SECTION local_auth_cs = { &local_auth_debug, -1, 0, 0, 0, 0 };

struct user_context_data
{
    enum mode mode;
    unsigned int flags;
    char session_key[16];
};

struct user_ctx
{
    struct list entry;
    LSA_SEC_HANDLE handle;

    enum mode mode;
    unsigned int flags;
    struct
    {
        unsigned int seq_no;
        struct arc4_info arc4info;
    } ntlm;
    struct
    {
        char send_sign_key[16];
        char send_seal_key[16];
        char recv_sign_key[16];
        char recv_seal_key[16];
        unsigned int send_seq_no;
        unsigned int recv_seq_no;
        struct arc4_info send_arc4info;
        struct arc4_info recv_arc4info;
    } ntlm2;
};

static struct list user_ctx_list = LIST_INIT(user_ctx_list);
static CRITICAL_SECTION user_ctx_cs;
static CRITICAL_SECTION_DEBUG user_ctx_debug =
{
    0, 0, &user_ctx_cs,
    { &user_ctx_debug.ProcessLocksList, &user_ctx_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": user_ctx_cs") }
};
static CRITICAL_SECTION user_ctx_cs = { &user_ctx_debug, -1, 0, 0, 0, 0 };

static const char *debugstr_challenge( const BYTE challenge[8] )
{
    return wine_dbg_sprintf( "%02x%02x%02x%02x%02x%02x%02x%02x",
            challenge[0], challenge[1], challenge[2], challenge[3],
            challenge[4], challenge[5], challenge[6], challenge[7] );
}

static NTSTATUS local_auth_init( const BYTE challenge[8], char *session_key )
{
    struct local_auth *auth;
    NTSTATUS status = SEC_E_OK;

    TRACE( "local authentication challenge: %s\n", debugstr_challenge(challenge) );

    EnterCriticalSection( &local_auth_cs );
    LIST_FOR_EACH_ENTRY( auth, &local_auths_list, struct local_auth, entry )
    {
        if (!memcmp( challenge, auth->challenge, sizeof(auth->challenge) ))
        {
            TRACE( "server challenge conflict detected\n" );
            status = STATUS_RETRY;
            break;
        }
    }

    if (!status && !(auth = calloc( 1, sizeof(*auth) )))
        status = SEC_E_INSUFFICIENT_MEMORY;

    if (!status && !RtlGenRandom( auth->session_key, sizeof(auth->session_key) ))
    {
        free( auth );
        status = SEC_E_INTERNAL_ERROR;
    }

    if (!status)
    {
        memcpy( auth->challenge, challenge, sizeof(auth->challenge) );
        memcpy( session_key, auth->session_key, sizeof(auth->session_key) );
        list_add_head( &local_auths_list, &auth->entry );
    }

    LeaveCriticalSection( &local_auth_cs );
    return status;
}

static NTSTATUS local_auth_authenticate( const BYTE challenge[8], HANDLE token, char *session_key )
{
    NTSTATUS status = STATUS_NOT_FOUND;
    struct local_auth *auth;

    EnterCriticalSection( &local_auth_cs );
    LIST_FOR_EACH_ENTRY( auth, &local_auths_list, struct local_auth, entry )
    {
        if (auth->got_token) continue;
        if (!memcmp( challenge, auth->challenge, sizeof(auth->challenge) ))
        {
            if (!DuplicateToken( token, SecurityImpersonation, &auth->token ))
                status = SEC_E_INVALID_TOKEN;
            else
            {
                memcpy( session_key, auth->session_key, sizeof(auth->session_key) );
                status = SEC_E_OK;
            }

            auth->got_token = TRUE;
            break;
        }
    }
    LeaveCriticalSection( &local_auth_cs );
    return status;
}

static NTSTATUS local_auth_finalize( const BYTE challenge[8], HANDLE *token )
{
    NTSTATUS status = SEC_E_LOGON_DENIED;
    struct local_auth *auth;

    EnterCriticalSection( &local_auth_cs );
    LIST_FOR_EACH_ENTRY( auth, &local_auths_list, struct local_auth, entry )
    {
        if (!memcmp( challenge, auth->challenge, sizeof(auth->challenge) ))
        {
            if (auth->token)
            {
                if (token) *token = auth->token;
                else CloseHandle( auth->token );
                status = SEC_E_OK;
            }

            list_remove( &auth->entry );
            free( auth );
            break;
        }
    }
    LeaveCriticalSection( &local_auth_cs );
    return status;
}

static void local_auth_cleanup( void )
{
    struct local_auth *auth, *tmp;

    LIST_FOR_EACH_ENTRY_SAFE( auth, tmp, &local_auths_list, struct local_auth, entry )
    {
        list_remove( &auth->entry );
        CloseHandle( auth->token );
        free( auth );
    }
}

static void ntlm_cleanup( struct ntlm_ctx *ctx )
{
    WINE_UNIX_CALL( unix_cleanup, ctx );
}

static NTSTATUS ntlm_chat( struct ntlm_ctx *ctx, char *buf, unsigned int buflen, unsigned int *retlen )
{
    struct chat_params params = { ctx, buf, buflen, retlen };

    return WINE_UNIX_CALL( unix_chat, &params );
}

static NTSTATUS ntlm_fork( struct ntlm_ctx *ctx, char **argv )
{
    struct fork_params params = { ctx, argv };

    return WINE_UNIX_CALL( unix_fork, &params );
}

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

    TRACE( "%#lx, %p, %s, %s, %p\n", package_id, dispatch, debugstr_as(database), debugstr_as(confidentiality),
           package_name );

    if (!(str = dispatch->AllocateLsaHeap( sizeof(*str) + sizeof("NTLM" )))) return STATUS_NO_MEMORY;
    ptr = (char *)(str + 1);
    memcpy( ptr, "NTLM", sizeof("NTLM") );
    RtlInitString( str, ptr );

    *package_name = str;
    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI ntlm_SpInitialize( ULONG_PTR package_id, SECPKG_PARAMETERS *params,
                                         LSA_SECPKG_FUNCTION_TABLE *lsa_function_table )
{
    TRACE( "%#Ix, %p, %p\n", package_id, params, lsa_function_table );
    lsa_secpkg_table = lsa_function_table;
    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI ntlm_SpGetInfo( SecPkgInfoW *info )
{
    TRACE( "%p\n", info );
    *info = ntlm_package_info;
    return STATUS_SUCCESS;
}

static WCHAR *strndupW( const WCHAR *str, size_t len )
{
    WCHAR *ret = malloc( (len + 1) * sizeof(WCHAR) );
    if (ret)
    {
        memcpy( ret, str, len * sizeof(WCHAR) );
        ret[len] = 0;
    }
    return ret;
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


static NTSTATUS map_auth_data( void *auth_data, SEC_WINNT_AUTH_IDENTITY_W **ret )
{
    SEC_WINNT_AUTH_IDENTITY_A id;
    SECPKG_CALL_INFO info;
    ULONG size, char_size;
    NTSTATUS status;
    BYTE *p;

    if (!auth_data)
    {
        *ret = NULL;
        return SEC_E_OK;
    }

    lsa_secpkg_table->GetCallInfo( &info );
    if (info.Attributes & SECPKG_CALL_WOWCLIENT)
    {
        struct {
            ULONG User;
            ULONG UserLength;
            ULONG Domain;
            ULONG DomainLength;
            ULONG Password;
            ULONG PasswordLength;
            ULONG Flags;
        } id32;

        status = lsa_secpkg_table->CopyFromClientBuffer( NULL, sizeof(id32), &id32, auth_data );
        if (status) return status;

        id.User = (void *)(ULONG_PTR)id32.User;
        id.UserLength = id32.UserLength;
        id.Domain = (void *)(ULONG_PTR)id32.Domain;
        id.DomainLength = id32.DomainLength;
        id.Password = (void *)(ULONG_PTR)id32.Password;
        id.PasswordLength = id32.PasswordLength;
        id.Flags = id32.Flags;
    }
    else
    {
        status = lsa_secpkg_table->CopyFromClientBuffer( NULL, sizeof(id), &id, auth_data );
        if (status) return status;
    }

    if (*((ULONG *)&id) == SEC_WINNT_AUTH_IDENTITY_VERSION)
    {
        if (info.Attributes & SECPKG_CALL_WOWCLIENT)
        {
            struct {
                ULONG Version;
                ULONG Length;
                ULONG User;
                ULONG UserLength;
                ULONG Domain;
                ULONG DomainLength;
                ULONG Password;
                ULONG PasswordLength;
                ULONG Flags;
                ULONG PackageList;
                ULONG PackageListLength;
            } idex32;

            status = lsa_secpkg_table->CopyFromClientBuffer( NULL, sizeof(idex32), &idex32, auth_data );
            if (status) return status;
            if (idex32.PackageList) FIXME( "ignoring package list\n" );
            id.User = (void *)(ULONG_PTR)idex32.User;
            id.UserLength = idex32.UserLength;
            id.Domain = (void *)(ULONG_PTR)idex32.Domain;
            id.DomainLength = idex32.DomainLength;
            id.Password = (void *)(ULONG_PTR)idex32.Password;
            id.PasswordLength = idex32.PasswordLength;
            id.Flags = idex32.Flags;
        }
        else
        {
            SEC_WINNT_AUTH_IDENTITY_EXA idex;

            status = lsa_secpkg_table->CopyFromClientBuffer( NULL, sizeof(idex), &idex, auth_data );
            if (status) return status;
            if (idex.PackageList) FIXME( "ignoring package list\n" );
            id.User = idex.User;
            id.UserLength = idex.UserLength;
            id.Domain = idex.Domain;
            id.DomainLength = idex.DomainLength;
            id.Password = idex.Password;
            id.PasswordLength = idex.PasswordLength;
            id.Flags = idex.Flags;
        }
    }

    char_size = (id.Flags & SEC_WINNT_AUTH_IDENTITY_ANSI ? 1 : 2);
    size = sizeof(id) + (id.UserLength + id.DomainLength + id.PasswordLength) * char_size;
    *ret = malloc( size );
    if (!*ret) return SEC_E_INSUFFICIENT_MEMORY;

    p = (BYTE *)(*ret + 1);
    if (id.UserLength)
    {
        status = lsa_secpkg_table->CopyFromClientBuffer( NULL, id.UserLength * char_size, p, id.User );
        id.User = p;
        p += id.UserLength * char_size;
    }
    if (!status && id.DomainLength)
    {
        status = lsa_secpkg_table->CopyFromClientBuffer( NULL, id.DomainLength * char_size, p, id.Domain );
        id.Domain = p;
        p += id.DomainLength * char_size;
    }
    if (!status && id.PasswordLength)
    {
        status = lsa_secpkg_table->CopyFromClientBuffer( NULL, id.PasswordLength * char_size, p, id.Password );
        id.Password = p;
        p += id.PasswordLength * char_size;
    }
    memcpy( *ret, &id, sizeof(id) );

    if (status) free( *ret );
    return status;
}

#define WINE_NO_CACHED_CREDENTIALS 0x10000000
static NTSTATUS NTAPI ntlm_SpAcquireCredentialsHandle( UNICODE_STRING *principal, ULONG cred_use, LUID *logon_id,
                                                       void *auth_data, void *get_key_fn, void *get_key_arg,
                                                       LSA_SEC_HANDLE *handle, TimeStamp *expiry )
{
    SECURITY_STATUS status;
    struct ntlm_cred *cred = NULL;
    WCHAR *domain = NULL, *user = NULL, *password = NULL;
    SEC_WINNT_AUTH_IDENTITY_W *id;

    TRACE( "%s, %#lx, %p, %p, %p, %p, %p, %p\n", debugstr_us(principal), cred_use, logon_id, auth_data,
           get_key_fn, get_key_arg, cred, expiry );

    if ((status = map_auth_data( auth_data, &id ))) return status;
    if (!(cred = calloc( 1, sizeof(*cred) )))
    {
        free( id );
        return SEC_E_INSUFFICIENT_MEMORY;
    }

    cred_use &= ~SECPKG_CRED_RESERVED;
    switch (cred_use)
    {
    case SECPKG_CRED_INBOUND:
        cred->mode = MODE_SERVER;

        *handle = (LSA_SEC_HANDLE)cred;
        status = SEC_E_OK;
        break;

    case SECPKG_CRED_BOTH:
        /* fall through */
    case SECPKG_CRED_OUTBOUND:
        cred->mode = cred_use == SECPKG_CRED_OUTBOUND ? MODE_CLIENT : MODE_BOTH;
        cred->no_cached_credentials = (cred_use & WINE_NO_CACHED_CREDENTIALS);

        if (id)
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

            cred->usernameW = strndupW(user, user_len);
            cred->domainW = strndupW(domain, domain_len);
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
        else
        {
            SECPKG_CALL_INFO info;
            HANDLE h;

            lsa_secpkg_table->GetCallInfo( &info );
            h = OpenThread( THREAD_QUERY_INFORMATION, FALSE, info.ThreadId );
            if (!h || !OpenThreadToken( h, TOKEN_QUERY | TOKEN_DUPLICATE, TRUE, &cred->token ))
            {
                CloseHandle( h );
                h = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, info.ProcessId );
                if (!h || !OpenProcessToken( h, TOKEN_QUERY | TOKEN_DUPLICATE, &cred->token ))
                    WARN("failed to get user token (%ld)\n", GetLastError());
            }
            CloseHandle( h );
        }

        *handle = (LSA_SEC_HANDLE)cred;
        status = SEC_E_OK;
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
    if (status != SEC_E_OK)
    {
        free( id );
        free( cred );
    }
    return status;
}

static NTSTATUS NTAPI ntlm_SpQueryCredentialsAttributes( LSA_SEC_HANDLE handle, ULONG attr, void *buf)
{
    WCHAR domain[DNLEN + 2], username_buf[UNLEN + 1], *username;
    struct ntlm_cred *cred = (struct ntlm_cred *)handle;
    SecPkgCredentials_NamesW names;
    SECPKG_CALL_INFO info;
    DWORD domain_len;
    NTSTATUS status;
    size_t len;

    TRACE( "%#Ix, %lu, %p\n", handle, attr, buf );

    if (attr != SECPKG_CRED_ATTR_NAMES) return STATUS_INVALID_PARAMETER;
    if (cred->mode == MODE_SERVER)
    {
        FIXME("MODE_SERVER not handled\n");
        return STATUS_NOT_IMPLEMENTED;
    }
    if (!cred->usernameW && !cred->domainW && !cred->token)
    {
        FIXME("no username/domain/token\n");
        return STATUS_NOT_IMPLEMENTED;
    }


    if (cred->token)
    {
        DWORD username_len = sizeof(username_buf);
        char tmp[256];
        TOKEN_USER *token_user = (TOKEN_USER *)tmp;
        DWORD size = sizeof(tmp);
        SID_NAME_USE use;
        BOOL r;

        if (!GetTokenInformation( cred->token, TokenUser, token_user, size, &size ))
        {
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                return SEC_E_INTERNAL_ERROR;

            token_user = malloc( size );
            if (!token_user) return SEC_E_INSUFFICIENT_MEMORY;

            if (!GetTokenInformation( cred->token, TokenUser, token_user, size, &size ))
            {
                free( token_user );
                return SEC_E_INTERNAL_ERROR;
            }
        }
        domain_len = sizeof(domain) - sizeof(WCHAR);
        r = LookupAccountSidW( NULL, token_user->User.Sid, username_buf, &username_len,
                domain, &domain_len, &use);
        if (token_user != (TOKEN_USER *)tmp) free( token_user );
        if (!r) return SEC_E_INTERNAL_ERROR;

        username = username_buf;
    }
    else
    {
        username = cred->usernameW;
        domain_len = cred->domainW ? wcslen(cred->domainW) : 0;
        memcpy( domain, cred->domainW, domain_len * sizeof(WCHAR) );
    }

    if (domain_len)
    {
        domain[domain_len++] = '\\';
        domain[domain_len] = 0;
    }

    len = domain_len + 1;
    if (username) len += wcslen( username );
    status = lsa_secpkg_table->AllocateClientBuffer( NULL, len * sizeof(WCHAR), (void **)&names.sUserName );
    if (status) return status;
    if (domain_len)
    {
        lsa_secpkg_table->CopyToClientBuffer( NULL, (domain_len + 1) * sizeof(WCHAR),
                names.sUserName, domain );
    }
    if (username)
    {
        lsa_secpkg_table->CopyToClientBuffer( NULL, (wcslen(username) + 1) * sizeof(WCHAR),
                names.sUserName + domain_len, username );
    }

    lsa_secpkg_table->GetCallInfo( &info );
    if (info.Attributes & SECPKG_CALL_WOWCLIENT)
    {
        struct
        {
            ULONG sUserName;
        } names32 =
        {
            (ULONG_PTR)names.sUserName
        };

        return lsa_secpkg_table->CopyToClientBuffer( NULL, sizeof(names32), buf, &names32 );
    }
    return lsa_secpkg_table->CopyToClientBuffer( NULL, sizeof(names), buf, &names );
}

static NTSTATUS NTAPI ntlm_SpFreeCredentialsHandle( LSA_SEC_HANDLE handle )
{
    struct ntlm_cred *cred = (struct ntlm_cred *)handle;

    TRACE( "%#Ix\n", handle );

    if (!cred) return SEC_E_OK;

    cred->mode = MODE_INVALID;
    if (cred->password) memset( cred->password, 0, cred->password_len );
    free( cred->usernameW );
    free( cred->domainW );
    free( cred->password );
    free( cred->username_arg );
    free( cred->domain_arg );
    CloseHandle( cred->token );
    free( cred );
    return SEC_E_OK;
}

static BOOL get_cached_credential( const UNICODE_STRING *target, CREDENTIALW **cred )
{
    const WCHAR *ptr, *host;
    WCHAR *hostonly;
    size_t len;
    BOOL ret;

    if (!target) return FALSE;

    len = target->Length / sizeof(WCHAR);
    if ((host = wmemchr( target->Buffer, '/', len )))
    {
        host++;
        len -= host - target->Buffer;
        if (!(ptr = wmemchr( host, ':', len ))) ptr = wmemchr( host, '/', len );
        if (!ptr) ptr = host + len;
    }
    else
    {
        host = target->Buffer;
        ptr = host + len;
    }

    if (!(hostonly = malloc( (ptr - host + 1) * sizeof(WCHAR) ))) return FALSE;
    memcpy( hostonly, host, (ptr - host) * sizeof(WCHAR) );
    hostonly[ptr - host] = 0;

    ret = CredReadW( hostonly, CRED_TYPE_DOMAIN_PASSWORD, 0, cred );
    free( hostonly );
    return ret;
}

static UINT encode_base64( const char *bin, unsigned int len, char *base64 )
{
    static const char base64enc[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    UINT n = 0, x;

    while (len > 0)
    {
        /* first 6 bits, all from bin[0] */
        base64[n++] = base64enc[(bin[0] & 0xfc) >> 2];
        x = (bin[0] & 3) << 4;

        /* next 6 bits, 2 from bin[0] and 4 from bin[1] */
        if (len == 1)
        {
            base64[n++] = base64enc[x];
            base64[n++] = '=';
            base64[n++] = '=';
            break;
        }
        base64[n++] = base64enc[x | ((bin[1] & 0xf0) >> 4)];
        x = (bin[1] & 0x0f) << 2;

        /* next 6 bits 4 from bin[1] and 2 from bin[2] */
        if (len == 2)
        {
            base64[n++] = base64enc[x];
            base64[n++] = '=';
            break;
        }
        base64[n++] = base64enc[x | ((bin[2] & 0xc0) >> 6)];

        /* last 6 bits, all from bin[2] */
        base64[n++] = base64enc[bin[2] & 0x3f];
        bin += 3;
        len -= 3;
    }
    base64[n] = 0;
    return n;
}

static inline char decode_char( char c )
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return 64;
}

static unsigned int decode_base64( const char *base64, unsigned int len, char *buf )
{
    unsigned int i = 0;
    char c0, c1, c2, c3;
    const char *p = base64;

    while (len > 4)
    {
        if ((c0 = decode_char( p[0] )) > 63) return 0;
        if ((c1 = decode_char( p[1] )) > 63) return 0;
        if ((c2 = decode_char( p[2] )) > 63) return 0;
        if ((c3 = decode_char( p[3] )) > 63) return 0;
        if (buf)
        {
            buf[i + 0] = (c0 << 2) | (c1 >> 4);
            buf[i + 1] = (c1 << 4) | (c2 >> 2);
            buf[i + 2] = (c2 << 6) |  c3;
        }
        len -= 4;
        i += 3;
        p += 4;
    }
    if (p[2] == '=')
    {
        if ((c0 = decode_char( p[0] )) > 63) return 0;
        if ((c1 = decode_char( p[1] )) > 63) return 0;
        if (buf) buf[i] = (c0 << 2) | (c1 >> 4);
        i++;
    }
    else if (p[3] == '=')
    {
        if ((c0 = decode_char( p[0] )) > 63) return 0;
        if ((c1 = decode_char( p[1] )) > 63) return 0;
        if ((c2 = decode_char( p[2] )) > 63) return 0;
        if (buf)
        {
            buf[i + 0] = (c0 << 2) | (c1 >> 4);
            buf[i + 1] = (c1 << 4) | (c2 >> 2);
        }
        i += 2;
    }
    else
    {
        if ((c0 = decode_char( p[0] )) > 63) return 0;
        if ((c1 = decode_char( p[1] )) > 63) return 0;
        if ((c2 = decode_char( p[2] )) > 63) return 0;
        if ((c3 = decode_char( p[3] )) > 63) return 0;
        if (buf)
        {
            buf[i + 0] = (c0 << 2) | (c1 >> 4);
            buf[i + 1] = (c1 << 4) | (c2 >> 2);
            buf[i + 2] = (c2 << 6) |  c3;
        }
        i += 3;
    }
    return i;
}

struct md4_ctx
{
    unsigned int buf[4];
    unsigned int i[2];
    unsigned char in[64];
    unsigned char digest[16];
};

void WINAPI MD4Init( struct md4_ctx * );
void WINAPI MD4Update( struct md4_ctx *, const char *, unsigned int );
void WINAPI MD4Final( struct md4_ctx * );

static void create_ntlm1_session_key( const char *secret, unsigned int len, char *session_key )
{
    struct md4_ctx ctx;
    char hash[16];

    MD4Init( &ctx );
    MD4Update( &ctx, secret, len );
    MD4Final( &ctx );
    memcpy( hash, ctx.digest, 16 );

    MD4Init( &ctx );
    MD4Update( &ctx, hash, 16 );
    MD4Final( &ctx );
    memcpy( session_key, ctx.digest, 16 );
}

void WINAPI MD5Init( struct md5_ctx * );
void WINAPI MD5Update( struct md5_ctx *, const char *, unsigned int );
void WINAPI MD5Final( struct md5_ctx * );

static void calc_ntlm2_subkey( const char *session_key, const char *magic, char *subkey )
{
    struct md5_ctx ctx;

    MD5Init( &ctx );
    MD5Update( &ctx, session_key, 16 );
    MD5Update( &ctx, magic, strlen(magic) + 1 );
    MD5Final( &ctx );
    memcpy( subkey, ctx.digest, 16 );
}

static const char client_to_server_sign_constant[] = "session key to client-to-server signing key magic constant";
static const char client_to_server_seal_constant[] = "session key to client-to-server sealing key magic constant";
static const char server_to_client_sign_constant[] = "session key to server-to-client signing key magic constant";
static const char server_to_client_seal_constant[] = "session key to server-to-client sealing key magic constant";

static void create_ntlm2_subkeys( struct user_ctx *ctx, const char *session_key )
{
    if (ctx->mode == MODE_CLIENT)
    {
        calc_ntlm2_subkey( session_key, client_to_server_sign_constant, ctx->ntlm2.send_sign_key );
        calc_ntlm2_subkey( session_key, client_to_server_seal_constant, ctx->ntlm2.send_seal_key );
        calc_ntlm2_subkey( session_key, server_to_client_sign_constant, ctx->ntlm2.recv_sign_key );
        calc_ntlm2_subkey( session_key, server_to_client_seal_constant, ctx->ntlm2.recv_seal_key );
    }
    else
    {
        calc_ntlm2_subkey( session_key, server_to_client_sign_constant, ctx->ntlm2.send_sign_key );
        calc_ntlm2_subkey( session_key, server_to_client_seal_constant, ctx->ntlm2.send_seal_key );
        calc_ntlm2_subkey( session_key, client_to_server_sign_constant, ctx->ntlm2.recv_sign_key );
        calc_ntlm2_subkey( session_key, client_to_server_seal_constant, ctx->ntlm2.recv_seal_key );
    }
}

/*
 * The arc4 code is based on dlls/advapi32/crypt_arc4.c by Mike McCormack,
 * which in turn is based on public domain code by Wei Dai
 */
static void arc4_init( struct arc4_info *info, const char *key, unsigned int len )
{
    unsigned int key_idx = 0, state_idx = 0, i, a;

    info->x = info->y = 0;
    for (i = 0; i < 256; i++) info->state[i] = i;

    for (i = 0; i < 256; i++)
    {
        a = info->state[i];
        state_idx += key[key_idx] + a;
        state_idx &= 0xff;
        info->state[i] = info->state[state_idx];
        info->state[state_idx] = a;
        if (++key_idx >= len) key_idx = 0;
    }
}

static void arc4_process( struct arc4_info *info, char *buf, unsigned int len )
{
    char *state = info->state;
    unsigned int x = info->x, y = info->y, a, b;

    while (len--)
    {
        x = (x + 1) & 0xff;
        a = state[x];
        y = (y + a) & 0xff;
        b = state[y];
        state[x] = b;
        state[y] = a;
        *buf++ ^= state[(a + b) & 0xff];
    }

    info->x = x;
    info->y = y;
}

static int get_buffer_index( SecBufferDesc *desc, ULONG type )
{
    int idx;
    for (idx = 0; idx < desc->cBuffers; idx++)
    {
        if (desc->pBuffers[idx].BufferType == type) return idx;
    }
    return -1;
}

static void hmac_md5_init( struct hmac_md5_ctx *ctx, const char *key, unsigned int key_len )
{
    char inner_padding[64], tmp_key[16];
    unsigned int i;

    if (key_len > 64)
    {
        struct md5_ctx tmp_ctx;

        MD5Init( &tmp_ctx );
        MD5Update( &tmp_ctx, key, key_len );
        MD5Final( &tmp_ctx );
        memcpy( tmp_key, tmp_ctx.digest, 16 );

        key = tmp_key;
        key_len = 16;
    }

    memset( inner_padding, 0, 64 );
    memset( ctx->outer_padding, 0, 64 );
    memcpy( inner_padding, key, key_len );
    memcpy( ctx->outer_padding, key, key_len );

    for (i = 0; i < 64; i++)
    {
        inner_padding[i] ^= 0x36;
        ctx->outer_padding[i] ^= 0x5c;
    }

    MD5Init( &ctx->ctx );
    MD5Update( &ctx->ctx, inner_padding, 64 );
}

static void hmac_md5_update( struct hmac_md5_ctx *ctx, const char *buf, unsigned int len )
{
    MD5Update( &ctx->ctx, buf, len );
}

static void hmac_md5_final( struct hmac_md5_ctx *ctx, char *digest )
{
    struct md5_ctx outer_ctx;
    char inner_digest[16];

    MD5Final( &ctx->ctx );
    memcpy( inner_digest, ctx->ctx.digest, 16 );

    MD5Init( &outer_ctx );
    MD5Update( &outer_ctx, ctx->outer_padding, 64 );
    MD5Update( &outer_ctx, inner_digest, 16 );
    MD5Final( &outer_ctx );

    memcpy( digest, outer_ctx.digest, 16 );
}

static NTSTATUS NTAPI ntlm_SpInitLsaModeContext( LSA_SEC_HANDLE cred_handle, LSA_SEC_HANDLE ctx_handle,
                                                 UNICODE_STRING *target, ULONG ctx_req, ULONG data_rep,
                                                 SecBufferDesc *input, LSA_SEC_HANDLE *new_ctx_handle,
                                                 SecBufferDesc *output, ULONG *ctx_attr, TimeStamp *expiry,
                                                 BOOLEAN *mapped_ctx, SecBuffer *ctx_data )
{
    NTSTATUS status = SEC_E_INSUFFICIENT_MEMORY;
    struct ntlm_ctx *ctx = NULL;
    char *buf, *bin, *want_flags = NULL, *username = NULL, *domain = NULL, *password = NULL;
    unsigned int len, bin_len;
    int idx;

    TRACE( "%#Ix, %#Ix, %s, %#lx, %lu, %p, %p, %p, %p, %p, %p, %p\n", cred_handle, ctx_handle, debugstr_us(target),
           ctx_req, data_rep, input, new_ctx_handle, output, ctx_attr, expiry, mapped_ctx, ctx_data );

    /* when communicating with the client there can be the following reply packets:
     * YR <base64 blob>         should be sent to the server
     * PW                       should be sent back to helper with base64 encoded password
     * AF <base64 blob>         client is done, blob should be sent to server with KK prefixed
     * GF <string list>         a string list of negotiated flags
     * GK <base64 blob>         base64 encoded session key
     * BH <char reason>         something broke
     *
     * The squid cache size is 2010 chars and that's what ntlm_auth uses */

    if (!(buf = malloc( NTLM_MAX_BUF * 3 + 64 ))) return SEC_E_INSUFFICIENT_MEMORY;
    if (!(bin = malloc( NTLM_MAX_BUF ))) goto done;

    if (!ctx_handle && !input)
    {
        char *argv[5];
        int password_len = 0;
        struct ntlm_cred *cred = (struct ntlm_cred *)cred_handle;

        if (!cred || !(cred->mode & MODE_CLIENT))
        {
            status = SEC_E_INVALID_HANDLE;
            goto done;
        }

        argv[0] = (char *)"ntlm_auth";
        argv[1] = (char *)"--helper-protocol=ntlmssp-client-1";
        if (!cred->username_arg && !cred->domain_arg)
        {
            WKSTA_USER_INFO_1 *ui = NULL;
            NET_API_STATUS ret;
            CREDENTIALW *cached;

            if (get_cached_credential( target, &cached ))
            {
                WCHAR *p;
                if ((p = wcschr( cached->UserName, '\\' )))
                {
                    if (!(domain = get_domain_arg( cached->UserName, p - cached->UserName ))) goto done;
                    p++;
                }
                else
                {
                    if (!(domain = get_domain_arg( NULL, 0 ))) goto done;
                    p = cached->UserName;
                }
                if (!(username = get_username_arg( p, -1 ))) goto done;

                if (cached->CredentialBlobSize)
                {
                    password_len = WideCharToMultiByte( CP_UNIXCP, WC_NO_BEST_FIT_CHARS, (WCHAR *)cached->CredentialBlob,
                                                        cached->CredentialBlobSize / sizeof(WCHAR), NULL, 0, NULL, NULL );
                    if (!(password = malloc( password_len ))) goto done;
                    WideCharToMultiByte( CP_UNIXCP, WC_NO_BEST_FIT_CHARS, (WCHAR *)cached->CredentialBlob,
                                         cached->CredentialBlobSize / sizeof(WCHAR), password, password_len, NULL, NULL );
                }
                CredFree( cached );

                argv[2] = username;
                argv[3] = domain;
                argv[4] = NULL;
            }
            else
            {
                ret = NetWkstaUserGetInfo( NULL, 1, (BYTE **)&ui );
                if (ret != NERR_Success || !ui || cred->no_cached_credentials)
                {
                    status = SEC_E_NO_CREDENTIALS;
                    goto done;
                }
                if (!(username = get_username_arg( ui->wkui1_username, -1 ))) goto done;
                NetApiBufferFree( ui );
                TRACE("using cached credentials\n");

                argv[2] = username;
                argv[3] = (char *)"--use-cached-creds";
                argv[4] = NULL;
            }
        }
        else
        {
            argv[2] = cred->username_arg;
            argv[3] = cred->domain_arg;
            argv[4] = NULL;
        }

        if (!(ctx = calloc( 1, sizeof(*ctx) ))) goto done;

        if (cred->token)
        {
            DuplicateTokenEx(cred->token, TOKEN_DUPLICATE | TOKEN_QUERY, NULL,
                    SecurityImpersonation, TokenImpersonation, &ctx->token);
        }

        if ((status = ntlm_fork( ctx, argv )) != SEC_E_OK) goto done;
        status = SEC_E_INSUFFICIENT_MEMORY;

        ctx->mode = MODE_CLIENT;
        memset( ctx->session_key, 0, sizeof(ctx->session_key) );

        /* generate the dummy session key = MD4(MD4(password))*/
        if (password || cred->password)
        {
            WCHAR *passwordW;
            len = MultiByteToWideChar( CP_ACP, 0, password ? password : cred->password,
                                       password ? password_len : cred->password_len, NULL, 0 );
            if (!(passwordW = malloc( len * sizeof(WCHAR) ))) goto done;
            MultiByteToWideChar( CP_ACP, 0, password ? password : cred->password,
                                 password ? password_len : cred->password_len, passwordW, len );

            create_ntlm1_session_key( (const char *)passwordW, len * sizeof(WCHAR), ctx->session_key );
            free( passwordW );
        }

        /* allocate space for a maximum string of
         * "SF NTLMSSP_FEATURE_SIGN NTLMSSP_FEATURE_SEAL NTLMSSP_FEATURE_SESSION_KEY"
         */
        if (!(want_flags = malloc( 73 ))) goto done;
        strcpy( want_flags, "SF" );
        if (ctx_req & ISC_REQ_CONFIDENTIALITY) ctx_req |= ISC_REQ_INTEGRITY;
        if (ctx_req & ISC_REQ_CONFIDENTIALITY) strcat( want_flags, " NTLMSSP_FEATURE_SEAL" );
        if ((ctx_req & ISC_REQ_INTEGRITY) || (ctx_req & ISC_REQ_REPLAY_DETECT) ||
            (ctx_req & ISC_REQ_SEQUENCE_DETECT)) strcat( want_flags, " NTLMSSP_FEATURE_SIGN" );

        ctx->req_attrs = ctx_req;
        *ctx_attr = 0;
        if (ctx_req & ISC_REQ_REPLAY_DETECT) *ctx_attr |= ISC_RET_REPLAY_DETECT;
        if (ctx_req & ISC_REQ_SEQUENCE_DETECT) *ctx_attr |= ISC_RET_SEQUENCE_DETECT;
        if (ctx_req & ISC_REQ_CONFIDENTIALITY) *ctx_attr |= ISC_RET_CONFIDENTIALITY;
        if (ctx_req & ISC_REQ_CONNECTION) *ctx_attr |= ISC_RET_CONNECTION;
        if (ctx_req & ISC_REQ_INTEGRITY) *ctx_attr |= ISC_RET_INTEGRITY;
        if (ctx_req & ISC_REQ_STREAM) FIXME( "ISC_REQ_STREAM\n" );

        /* use cached credentials if no password was given, fall back to an empty password on failure */
        if (!password && !cred->password)
        {
            strcpy( buf, "OK" );
            if ((status = ntlm_chat( ctx, buf, NTLM_MAX_BUF, &len )) != SEC_E_OK) goto done;

            /* if the helper replied with "PW" using cached credentials failed */
            if (!strncmp( buf, "PW", 2 ))
            {
                TRACE( "using cached credentials failed\n" );
                strcpy( buf, "PW AA==" );
            }
            else strcpy( buf, "OK" ); /* just do a noop on the next run */
        }
        else
        {
            strcpy( buf, "PW " );
            encode_base64( password ? password : cred->password, password ? password_len : cred->password_len, buf + 3 );
        }

        TRACE( "sending to ntlm_auth: %s\n", strncmp(buf, "PW ", 3) ? debugstr_a(buf) : "PW <password>" );
        if ((status = ntlm_chat( ctx, buf, NTLM_MAX_BUF, &len )) != SEC_E_OK) goto done;
        TRACE( "ntlm_auth returned %s\n", debugstr_a(buf) );

        if (strlen( want_flags ) > 2)
        {
            TRACE( "want flags are %s\n", debugstr_a(want_flags) );
            strcpy( buf, want_flags );
            if ((status = ntlm_chat( ctx, buf, NTLM_MAX_BUF, &len )) != SEC_E_OK) goto done;
            if (!strncmp( buf, "BH", 2 )) ERR( "ntlm_auth doesn't understand new command set\n" );
        }

        strcpy( buf, "YR" );
        if ((status = ntlm_chat( ctx, buf, NTLM_MAX_BUF, &len )) != SEC_E_OK) goto done;
        TRACE( "ntlm_auth returned %s\n", buf );
        if (strncmp( buf, "YR ", 3 ))
        {
            status = SEC_E_INTERNAL_ERROR;
            goto done;
        }
        bin_len = decode_base64( buf + 3, len - 3, bin );

        ctx->negotiate = malloc( bin_len );
        if (ctx->negotiate)
        {
            ctx->negotiate_len = bin_len;
            memcpy( ctx->negotiate, bin, bin_len );
        }

        *new_ctx_handle = (LSA_SEC_HANDLE)ctx;
        status = SEC_I_CONTINUE_NEEDED;
    }
    else /* !ctx_handle && !input */
    {
        struct ntlm_challenge *challenge;

        if (!input || ((idx = get_buffer_index( input, SECBUFFER_TOKEN )) == -1))
        {
            status = SEC_E_INVALID_TOKEN;
            goto done;
        }

        ctx = (struct ntlm_ctx *)ctx_handle;
        if (!ctx || ctx->mode != MODE_CLIENT)
        {
            status = SEC_E_INVALID_HANDLE;
            goto done;
        }

        if (!input->pBuffers[idx].pvBuffer || input->pBuffers[idx].cbBuffer > NTLM_MAX_BUF ||
                input->pBuffers[idx].cbBuffer < offsetof(struct ntlm_challenge, target_info_len))
        {
            status = SEC_E_INVALID_TOKEN;
            goto done;
        }

        status = lsa_secpkg_table->MapBuffer( input->pBuffers + idx, input->pBuffers + idx );
        if (status) goto done;
        challenge = input->pBuffers[idx].pvBuffer;
        ctx->req_attrs |= ctx_req;
        *ctx_attr = 0;
        if (ctx->req_attrs & ISC_REQ_MUTUAL_AUTH) FIXME( "ASC_REQ_MUTUAL_AUTH\n" );
        if (ctx->req_attrs & (ISC_REQ_INTEGRITY | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT) &&
                challenge->negotiate_flags & NTLMSSP_NEGOTIATE_SIGN)
            *ctx_attr |= ISC_RET_INTEGRITY | ISC_RET_SEQUENCE_DETECT | ISC_RET_REPLAY_DETECT;
        if (ctx->req_attrs & ISC_REQ_CONFIDENTIALITY && challenge->negotiate_flags & NTLMSSP_NEGOTIATE_SEAL)
            *ctx_attr |= ISC_RET_CONFIDENTIALITY;

        if (ctx->token && !local_auth_authenticate( challenge->challenge, ctx->token, ctx->session_key ))
        {
            struct ntlm_authenticate *authenticate = (struct ntlm_authenticate *)bin;

            TRACE( "using local authentication\n" );

            memset( authenticate, 0, sizeof(*authenticate) );
            memcpy( authenticate->signature, "NTLMSSP", sizeof(authenticate->signature) );
            authenticate->message_type = NTLM_AUTHENTICATE;
            authenticate->lm_response_off = sizeof(*authenticate);
            authenticate->nt_response_off = sizeof(*authenticate);
            authenticate->domain_off = sizeof(*authenticate);
            authenticate->username_off = sizeof(*authenticate);
            authenticate->workstation_off = sizeof(*authenticate);
            authenticate->random_session_key_off = sizeof(*authenticate);
            authenticate->negotiate_flags = challenge->negotiate_flags;

            hmac_md5_init( &ctx->mic, ctx->session_key, sizeof(ctx->session_key) );
            hmac_md5_update( &ctx->mic, ctx->negotiate, ctx->negotiate_len );
            hmac_md5_update( &ctx->mic, (char *)challenge, input->pBuffers[idx].cbBuffer );
            hmac_md5_update( &ctx->mic, (char *)authenticate, sizeof(*authenticate) );
            hmac_md5_final( &ctx->mic, (char *)authenticate->mic );

            ctx->flags = challenge->negotiate_flags;
            bin_len = sizeof(*authenticate);
        }
        else
        {
            bin_len = input->pBuffers[idx].cbBuffer;
            memcpy( bin, input->pBuffers[idx].pvBuffer, bin_len );

            strcpy( buf, "TT " );
            encode_base64( bin, bin_len, buf + 3 );
            TRACE( "server sent: %s\n", debugstr_a(buf) );

            if ((status = ntlm_chat( ctx, buf, NTLM_MAX_BUF, &len ))) goto done;
            TRACE( "ntlm_auth returned: %s\n", debugstr_a(buf) );

            if (strncmp( buf, "KK ", 3 ) && strncmp( buf, "AF ", 3 ))
            {
                status = SEC_E_INVALID_TOKEN;
                goto done;
            }
            bin_len = decode_base64( buf + 3, len - 3, bin );

            strcpy( buf, "GF" );
            if ((status = ntlm_chat( ctx, buf, NTLM_MAX_BUF, &len )) != SEC_E_OK) goto done;
            if (len < 3) ctx->flags = 0;
            else sscanf( buf + 3, "%x", &ctx->flags );

            strcpy( buf, "GK" );
            if ((status = ntlm_chat( ctx, buf, NTLM_MAX_BUF, &len )) != SEC_E_OK) goto done;

            if (!strncmp( buf, "BH", 2 )) TRACE( "no key negotiated\n" );
            else if (!strncmp( buf, "GK ", 3 ))
            {
                TRACE( "session key is %s\n", debugstr_a(buf + 3) );
                decode_base64( buf + 3, len - 3, ctx->session_key );
            }
        }

        *new_ctx_handle = (LSA_SEC_HANDLE)ctx;
        status = SEC_E_OK;
    }

    if (!output || ((idx = get_buffer_index( output, SECBUFFER_TOKEN )) == -1))
    {
        if (!ctx_handle && !input) *new_ctx_handle = 0;
        status = SEC_E_BUFFER_TOO_SMALL;
        goto done;
    }

    if (ctx_req & ISC_REQ_ALLOCATE_MEMORY)
    {
        /* freed with secur32.FreeContextBuffer */
        if (!(output->pBuffers[idx].pvBuffer = lsa_secpkg_table->AllocateLsaHeap( bin_len )))
        {
            status = SEC_E_INSUFFICIENT_MEMORY;
            goto done;
        }
        output->pBuffers[idx].cbBuffer = bin_len;
    }
    else if (output->pBuffers[idx].cbBuffer < bin_len)
    {
        if (!ctx_handle && !input) *new_ctx_handle = 0;
        status = SEC_E_BUFFER_TOO_SMALL;
        goto done;
    }
    else
    {
        NTSTATUS ret = lsa_secpkg_table->MapBuffer( output->pBuffers + idx, output->pBuffers + idx );
        if (ret)
        {
            status = ret;
            goto done;
        }
    }

    if (!output->pBuffers[idx].pvBuffer)
    {
        if (!ctx_handle && !input) *new_ctx_handle = 0;
        status = SEC_E_INTERNAL_ERROR;
        goto done;
    }

    output->pBuffers[idx].cbBuffer = bin_len;
    memcpy( output->pBuffers[idx].pvBuffer, bin, bin_len );

    if (status == SEC_E_OK)
    {
        struct user_context_data *data = lsa_secpkg_table->AllocateLsaHeap( sizeof( *data ));

        if (!data)
        {
            if (!ctx_handle && !input) *new_ctx_handle = 0;
            status = SEC_E_INSUFFICIENT_MEMORY;
            goto done;
        }
        data->mode = ctx->mode;
        data->flags = ctx->flags;
        memcpy( data->session_key, ctx->session_key, sizeof(data->session_key) );

        *mapped_ctx = TRUE;
        ctx_data->cbBuffer = sizeof( *data );
        ctx_data->pvBuffer = data;
    }
done:
    if (status != SEC_E_OK && status != SEC_I_CONTINUE_NEEDED && !ctx_handle && !input)
    {
        CloseHandle( ctx->token );
        ntlm_cleanup( ctx );
        free( ctx );
    }
    free( username );
    free( domain );
    free( password );
    free( buf );
    free( bin );
    free( want_flags );

    TRACE( "returning %#lx\n", status );
    return status;
}

static NTSTATUS NTAPI ntlm_SpAcceptLsaModeContext( LSA_SEC_HANDLE cred_handle, LSA_SEC_HANDLE ctx_handle,
                                                   SecBufferDesc *input, ULONG ctx_req, ULONG data_rep,
                                                   LSA_SEC_HANDLE *new_ctx_handle, SecBufferDesc *output,
                                                   ULONG *ctx_attr, TimeStamp *expiry, BOOLEAN *mapped_ctx,
                                                   SecBuffer *ctx_data )
{
    NTSTATUS status = SEC_E_INSUFFICIENT_MEMORY;
    struct ntlm_ctx *ctx = NULL;
    char *buf, *bin, *want_flags = NULL;
    unsigned int len, bin_len;

    TRACE( "%#Ix, %#Ix, %#lx, %lu, %p, %p, %p, %p, %p, %p, %p\n", cred_handle, ctx_handle, ctx_req, data_rep, input,
           new_ctx_handle, output, ctx_attr, expiry, mapped_ctx, ctx_data );
    if (ctx_req) FIXME( "ignoring flags %#lx\n", ctx_req );

    if (!(buf = malloc( NTLM_MAX_BUF * 3 + 64 ))) return SEC_E_INSUFFICIENT_MEMORY;
    if (!(bin = malloc( NTLM_MAX_BUF ))) goto done;

    if (!ctx_handle)
    {
        struct ntlm_cred *cred = (struct ntlm_cred *)cred_handle;
        struct ntlm_negotiate *negotiate;
        char *argv[3];

        if (!cred || !(cred->mode & MODE_SERVER))
        {
            status = SEC_E_INVALID_HANDLE;
            goto done;
        }

        if (!input || input->cBuffers < 1)
        {
            status = SEC_E_INCOMPLETE_MESSAGE;
            goto done;
        }

        if (input->pBuffers[0].cbBuffer > NTLM_MAX_BUF)
        {
            status = SEC_E_INVALID_TOKEN;
            goto done;
        }
        else bin_len = input->pBuffers[0].cbBuffer;

        if (bin_len < offsetof(struct ntlm_negotiate, target_name_len))
        {
            status = SEC_E_INVALID_TOKEN;
            goto done;
        }
        status = lsa_secpkg_table->MapBuffer( input->pBuffers, input->pBuffers );
        if (status) goto done;
        negotiate = input->pBuffers[0].pvBuffer;

        if (!(ctx = calloc( 1, sizeof(*ctx) ))) goto done;

        argv[0] = (char *)"ntlm_auth";
        argv[1] = (char *)"--helper-protocol=squid-2.5-ntlmssp";
        argv[2] = NULL;
        if ((status = ntlm_fork( ctx, argv )) != SEC_E_OK) goto done;
        ctx->mode = MODE_SERVER;

        if (!(want_flags = malloc( 73 )))
        {
            status = SEC_E_INSUFFICIENT_MEMORY;
            goto done;
        }
        strcpy( want_flags, "SF" );
        if (ctx_req & ASC_REQ_CONFIDENTIALITY) strcat( want_flags, " NTLMSSP_FEATURE_SEAL" );
        if (ctx_req & ASC_REQ_CONNECTION) strcat( want_flags, " NTLMSSP_FEATURE_SESSION_KEY" );
        if (ctx_req & ASC_REQ_INTEGRITY) strcat( want_flags, " NTLMSSP_FEATURE_SIGN" );
        if (ctx_req & ASC_REQ_ALLOCATE_MEMORY) FIXME( "ASC_REQ_ALLOCATE_MEMORY\n" );
        if (ctx_req & ASC_REQ_EXTENDED_ERROR) FIXME( "ASC_REQ_EXTENDED_ERROR\n" );
        if (ctx_req & ASC_REQ_MUTUAL_AUTH) FIXME( "ASC_REQ_MUTUAL_AUTH\n" );
        if (ctx_req & ASC_REQ_REPLAY_DETECT) FIXME( "ASC_REQ_REPLAY_DETECT\n" );
        if (ctx_req & ASC_REQ_SEQUENCE_DETECT) FIXME( "ASC_REQ_SEQUENCE_DETECT\n" );
        if (ctx_req & ASC_REQ_STREAM) FIXME( "ASC_REQ_STREAM\n" );

        ctx->req_attrs = ctx_req;
        *ctx_attr = 0;
        if (ctx_req & ASC_REQ_CONFIDENTIALITY || negotiate->negotiate_flags & NTLMSSP_NEGOTIATE_SEAL)
            *ctx_attr |= ASC_RET_CONFIDENTIALITY;
        if (ctx_req & ASC_REQ_CONNECTION) *ctx_attr |= ASC_RET_CONNECTION;
        if (ctx_req & ASC_REQ_INTEGRITY) *ctx_attr |= ASC_RET_INTEGRITY;
        if (ctx_req & ISC_REQ_REPLAY_DETECT || negotiate->negotiate_flags & NTLMSSP_NEGOTIATE_SIGN)
            *ctx_attr |= ASC_RET_REPLAY_DETECT;
        if (ctx_req & ISC_REQ_SEQUENCE_DETECT || negotiate->negotiate_flags & NTLMSSP_NEGOTIATE_SIGN)
            *ctx_attr |= ASC_RET_SEQUENCE_DETECT;

        if (strlen( want_flags ) > 3)
        {
            TRACE( "want flags are %s\n", debugstr_a(want_flags) );
            strcpy( buf, want_flags );
            if ((status = ntlm_chat( ctx, buf, NTLM_MAX_BUF, &len )) != SEC_E_OK) goto done;
            if (!strncmp( buf, "BH", 2 )) ERR( "ntlm_auth doesn't understand new command set\n" );
        }

        do
        {
            memcpy( bin, input->pBuffers[0].pvBuffer, bin_len );
            strcpy( buf, "YR " );
            encode_base64( bin, bin_len, buf + 3 );

            if ((status = ntlm_chat( ctx, buf, NTLM_MAX_BUF, &len )) != SEC_E_OK) goto done;
            TRACE( "ntlm_auth returned %s\n", buf );
            if (strncmp( buf, "TT ", 3))
            {
                status = SEC_E_INTERNAL_ERROR;
                goto done;
            }
            bin_len = decode_base64( buf + 3, len - 3, bin );

            if (bin_len >= sizeof(struct ntlm_challenge))
            {
                struct ntlm_challenge *challenge = (struct ntlm_challenge *)bin;

                ctx->flags = challenge->negotiate_flags;
                status = local_auth_init( challenge->challenge, ctx->session_key );
                if (status == STATUS_RETRY) continue;
                if (!status)
                {
                    memcpy( ctx->server_challenge, challenge->challenge, sizeof(ctx->server_challenge) );
                    hmac_md5_init( &ctx->mic, ctx->session_key, sizeof(ctx->session_key) );
                    hmac_md5_update( &ctx->mic, (const char *)negotiate, input->pBuffers[0].cbBuffer );
                    hmac_md5_update( &ctx->mic, bin, bin_len );
                }
            }
        } while(0);

        if (!output || output->cBuffers < 1)
        {
            status = SEC_E_INSUFFICIENT_MEMORY;
            goto done;
        }
        output->pBuffers[0].cbBuffer = bin_len;
        output->pBuffers[0].BufferType = SECBUFFER_TOKEN;
        status = lsa_secpkg_table->MapBuffer( output->pBuffers, output->pBuffers );
        if (status) goto done;
        memcpy( output->pBuffers[0].pvBuffer, bin, bin_len );

        *new_ctx_handle = (LSA_SEC_HANDLE)ctx;
        status = SEC_I_CONTINUE_NEEDED;
    }
    else
    {
        struct ntlm_authenticate *authenticate;

        if (!input || input->cBuffers < 1)
        {
            status = SEC_E_INCOMPLETE_MESSAGE;
            goto done;
        }

        ctx = (struct ntlm_ctx *)ctx_handle;
        if (!ctx || ctx->mode != MODE_SERVER)
        {
            status = SEC_E_INVALID_HANDLE;
            goto done;
        }

        if (input->pBuffers[0].cbBuffer > NTLM_MAX_BUF ||
                input->pBuffers[0].cbBuffer < offsetof(struct ntlm_authenticate, version))
        {
            status = SEC_E_INVALID_TOKEN;
            goto done;
        }
        else bin_len = input->pBuffers[0].cbBuffer;
        status = lsa_secpkg_table->MapBuffer( input->pBuffers, input->pBuffers );
        if (status) goto done;
        memcpy( bin, input->pBuffers[0].pvBuffer, bin_len );
        authenticate = input->pBuffers[0].pvBuffer;

        *ctx_attr = 0;
        if (authenticate->negotiate_flags & NTLMSSP_NEGOTIATE_SEAL)
            *ctx_attr |= ASC_RET_CONFIDENTIALITY;
        if (authenticate->negotiate_flags & NTLMSSP_NEGOTIATE_SIGN)
            *ctx_attr |= ASC_RET_INTEGRITY | ASC_RET_REPLAY_DETECT | ASC_RET_SEQUENCE_DETECT;


        if (bin_len >= sizeof(*authenticate) && ctx->flags == authenticate->negotiate_flags &&
                !authenticate->lm_response_len && !authenticate->nt_response_len &&
                !authenticate->username_len && !authenticate->domain_len)
        {
            BYTE mic_client[16], mic_server[16];
            HANDLE token;

            TRACE( "using local authentication\n" );

            status = local_auth_finalize( ctx->server_challenge, &token );
            memset( ctx->server_challenge, 0, sizeof(ctx->server_challenge) );
            if (status)
            {
                memset( ctx->session_key, 0, sizeof(ctx->session_key) );
                goto done;
            }
            /* FIXME: Use the token in QuerySecurityContextToken */
            CloseHandle( token );

            memcpy( mic_client, authenticate->mic, sizeof(authenticate->mic) );
            memset( authenticate->mic, 0, sizeof(authenticate->mic) );
            hmac_md5_update( &ctx->mic, (const char *)authenticate, bin_len );
            hmac_md5_final( &ctx->mic, (char *)mic_server );
            if (memcmp(mic_client, mic_server, sizeof(mic_client)))
                status = SEC_E_MESSAGE_ALTERED;
            else
                status = SEC_E_OK;
            goto done;
        }
        else
        {
            local_auth_finalize( ctx->server_challenge, NULL );
            memset( ctx->server_challenge, 0, sizeof(ctx->server_challenge) );
            memset( ctx->session_key, 0, sizeof(ctx->session_key) );
        }

        strcpy( buf, "KK " );
        encode_base64( bin, bin_len, buf + 3 );

        TRACE( "client sent %s\n", debugstr_a(buf) );
        if ((status = ntlm_chat( ctx, buf, NTLM_MAX_BUF, &len )) != SEC_E_OK) goto done;
        TRACE( "ntlm_auth returned %s\n", debugstr_a(buf) );

        /* At this point, we get a NA if the user didn't authenticate, but a BH if ntlm_auth could not
         * connect to winbindd. Apart from running Wine as root, there is no way to fix this for now,
         * so just handle this as a failed login. */
        if (strncmp( buf, "AF ", 3 ))
        {
            if (!strncmp( buf, "NA ", 3 ))
            {
                status = SEC_E_LOGON_DENIED;
                goto done;
            }
            else
            {
                const char err_v3[] = "BH NT_STATUS_ACCESS_DENIED";
                const char err_v4[] = "BH NT_STATUS_UNSUCCESSFUL";

                if ((len >= strlen(err_v3) && !strncmp( buf, err_v3, strlen(err_v3) )) ||
                    (len >= strlen(err_v4) && !strncmp( buf, err_v4, strlen(err_v4) )))
                {
                    TRACE( "connection to winbindd failed\n" );
                    status = SEC_E_LOGON_DENIED;
                }
                else status = SEC_E_INTERNAL_ERROR;
                goto done;
            }
        }
        if (output && output->cBuffers > 0)
            output->pBuffers[0].cbBuffer = 0;

        strcpy( buf, "GF" );
        if ((status = ntlm_chat( ctx, buf, NTLM_MAX_BUF, &len )) != SEC_E_OK) goto done;
        if (len < 3) ctx->flags = 0;
        else sscanf( buf + 3, "%x", &ctx->flags );

        strcpy( buf, "GK" );
        if ((status = ntlm_chat( ctx, buf, NTLM_MAX_BUF, &len )) != SEC_E_OK) goto done;

        if (!strncmp( buf, "BH", 2 )) TRACE( "no key negotiated\n" );
        else if (!strncmp( buf, "GK ", 3 ))
        {
            bin_len = decode_base64( buf + 3, len - 3, bin );
            TRACE( "session key is %s\n", debugstr_a(buf + 3) );
            memcpy( ctx->session_key, bin, bin_len );
        }

        if (len < 3) memset( ctx->session_key, 0 , 16 );
        else
        {
            if (!strncmp( buf, "BH ", 3 ))
            {
                TRACE( "helper sent %s\n", debugstr_a(buf + 3) );
                /*FIXME: generate dummy session key = MD4(MD4(password))*/
                memset( ctx->session_key, 0 , 16 );
            }
            else if (!strncmp( buf, "GK ", 3 ))
            {
                bin_len = decode_base64( buf + 3, len - 3, bin );
                TRACE( "session key is %s\n", debugstr_a(buf + 3) );
                memcpy( ctx->session_key, bin, 16 );
            }
        }
    }

done:
    if (status == SEC_E_OK)
    {
        struct user_context_data *data = lsa_secpkg_table->AllocateLsaHeap( sizeof( *data ));

        if (!data)
        {
            status = SEC_E_INSUFFICIENT_MEMORY;
        }
        else
        {
            data->mode = ctx->mode;
            data->flags = ctx->flags;
            memcpy( data->session_key, ctx->session_key, sizeof(data->session_key) );

            *mapped_ctx = TRUE;
            ctx_data->cbBuffer = sizeof( *data );
            ctx_data->pvBuffer = data;

            *new_ctx_handle = (LSA_SEC_HANDLE)ctx;
        }
    }

    if (status != SEC_E_OK && status != SEC_I_CONTINUE_NEEDED && !ctx_handle)
    {
        ntlm_cleanup( ctx );
        free( ctx );
    }
    free( buf );
    free( bin );
    free( want_flags );

    TRACE( "returning %#lx\n", status );
    return status;
}

static NTSTATUS NTAPI ntlm_SpDeleteContext( LSA_SEC_HANDLE handle )
{
    struct ntlm_ctx *ctx = (struct ntlm_ctx *)handle;

    TRACE( "%#Ix\n", handle );

    if (!ctx) return SEC_E_INVALID_HANDLE;
    free( ctx->negotiate );
    CloseHandle( ctx->token );
    local_auth_finalize( ctx->server_challenge, NULL );
    ntlm_cleanup( ctx );
    free( ctx );
    return SEC_E_OK;
}

static NTSTATUS build_package_info( const SecPkgInfoW *info, SecPkgInfoW **ret,
        const SECPKG_CALL_INFO *call_info )
{
    DWORD size_name = (wcslen(info->Name) + 1) * sizeof(WCHAR);
    DWORD size_comment = (wcslen(info->Comment) + 1) * sizeof(WCHAR);
    SecPkgInfoW pkg_info;
    NTSTATUS status;

    pkg_info = *info;
    status = lsa_secpkg_table->AllocateClientBuffer( NULL,
            sizeof(pkg_info) + size_name + size_comment, (void **)ret );
    if (status) return status;

    pkg_info.Name = (SEC_WCHAR *)((*ret) + 1);
    pkg_info.Comment = (SEC_WCHAR *)((char *)pkg_info.Name + size_name);
    lsa_secpkg_table->CopyToClientBuffer( NULL, size_name, pkg_info.Name, info->Name );
    lsa_secpkg_table->CopyToClientBuffer( NULL, size_comment, pkg_info.Comment, info->Comment );

    if (call_info->Attributes & SECPKG_CALL_WOWCLIENT)
    {
        struct
        {
            ULONG fCapabilities;
            USHORT wVersion;
            USHORT wRPCID;
            ULONG cbMaxToken;
            ULONG Name;
            ULONG Comment;
        } pkg_info32 =
        {
            pkg_info.fCapabilities,
            pkg_info.wVersion,
            pkg_info.wRPCID,
            pkg_info.cbMaxToken,
            (ULONG_PTR)pkg_info.Name,
            (ULONG_PTR)pkg_info.Comment
        };

        return lsa_secpkg_table->CopyToClientBuffer( NULL, sizeof(pkg_info32), *ret, &pkg_info32 );
    }
    return lsa_secpkg_table->CopyToClientBuffer( NULL, sizeof(pkg_info), *ret, &pkg_info );
}

static NTSTATUS NTAPI ntlm_SpQueryContextAttributes( LSA_SEC_HANDLE handle, ULONG attr, void *buf )
{
    TRACE( "%#Ix, %lu, %p\n", handle, attr, buf );

    if (!handle) return SEC_E_INVALID_HANDLE;

    switch (attr)
    {
#define X(x) case (x) : FIXME(#x" stub\n"); break
    X(SECPKG_ATTR_ACCESS_TOKEN);
    X(SECPKG_ATTR_AUTHORITY);
    X(SECPKG_ATTR_DCE_INFO);
    X(SECPKG_ATTR_LIFESPAN);
    X(SECPKG_ATTR_NAMES);
    X(SECPKG_ATTR_NATIVE_NAMES);
    X(SECPKG_ATTR_PACKAGE_INFO);
    X(SECPKG_ATTR_PASSWORD_EXPIRY);
    X(SECPKG_ATTR_STREAM_SIZES);
    X(SECPKG_ATTR_TARGET_INFORMATION);
    case SECPKG_ATTR_FLAGS:
    {
        SecPkgContext_Flags flags;
        struct ntlm_ctx *ctx = (struct ntlm_ctx *)handle;

        flags.Flags = 0;
        if (ctx->flags & NTLMSSP_NEGOTIATE_SIGN) flags.Flags |= ISC_RET_INTEGRITY;
        if (ctx->flags & NTLMSSP_NEGOTIATE_SEAL) flags.Flags |= ISC_RET_CONFIDENTIALITY;
        return lsa_secpkg_table->CopyToClientBuffer( NULL, sizeof(flags), buf, &flags );
    }
    case SECPKG_ATTR_SIZES:
    {
        SecPkgContext_Sizes sizes;

        sizes.cbMaxToken        = NTLM_MAX_BUF;
        sizes.cbMaxSignature    = 16;
        sizes.cbBlockSize       = 0;
        sizes.cbSecurityTrailer = 16;
        return lsa_secpkg_table->CopyToClientBuffer( NULL, sizeof(sizes), buf, &sizes );
    }
    case SECPKG_ATTR_NEGOTIATION_INFO:
    {
        SecPkgContext_NegotiationInfoW info;
        SECPKG_CALL_INFO call_info;
        NTSTATUS status;

        lsa_secpkg_table->GetCallInfo( &call_info );
        status = build_package_info( &ntlm_package_info, &info.PackageInfo, &call_info );
        if (status) return status;
        info.NegotiationState = SECPKG_NEGOTIATION_COMPLETE;

        if (call_info.Attributes & SECPKG_CALL_WOWCLIENT)
        {
            struct
            {
                ULONG PackageInfo;
                ULONG NegotiationState;
            } info32 =
            {
                (ULONG_PTR)info.PackageInfo,
                info.NegotiationState
            };

            return lsa_secpkg_table->CopyToClientBuffer( NULL, sizeof(info32), buf, &info32 );
        }
        return lsa_secpkg_table->CopyToClientBuffer( NULL, sizeof(info), buf, &info );
    }
    case SECPKG_ATTR_SESSION_KEY:
    {
        struct ntlm_ctx *ctx = (struct ntlm_ctx *)handle;
        SecPkgContext_SessionKey key;
        SECPKG_CALL_INFO info;
        NTSTATUS status;

        key.SessionKeyLength = sizeof(ctx->session_key);
        status = lsa_secpkg_table->AllocateClientBuffer( NULL, key.SessionKeyLength, (void **)&key.SessionKey );
        if (status) return status;
        lsa_secpkg_table->CopyToClientBuffer( NULL, key.SessionKeyLength, key.SessionKey, ctx->session_key );

        lsa_secpkg_table->GetCallInfo( &info );
        if (info.Attributes & SECPKG_CALL_WOWCLIENT)
        {
            struct
            {
                ULONG SessionKeyLength;
                ULONG SessionKey;
            } key32 =
            {
                key.SessionKeyLength,
                (ULONG_PTR)key.SessionKey
            };

            return lsa_secpkg_table->CopyToClientBuffer( NULL, sizeof(key32), buf, &key32 );
        }
        return lsa_secpkg_table->CopyToClientBuffer( NULL, sizeof(key), buf, &key );
    }
    case SECPKG_ATTR_KEY_INFO:
    {
        struct ntlm_ctx *ctx = (struct ntlm_ctx *)handle;
        SecPkgContext_KeyInfoW info;
        SEC_WCHAR *signature_alg;
        ULONG signature_size, signature_algid;
        SECPKG_CALL_INFO call_info;
        NTSTATUS status;

        if (ctx->flags & NTLMSSP_NEGOTIATE_KEY_EXCH)
        {
            signature_alg = (SEC_WCHAR *)L"HMAC-MD5";
            signature_size = sizeof(L"HMAC-MD5");
            signature_algid = 0xffffff76;
        }
        else
        {
            signature_alg = (SEC_WCHAR *)L"RSADSI RC4-CRC32";
            signature_size = sizeof(L"RSADSI RC4-CRC32");
            signature_algid = 0xffffff7c;
        }

        status = lsa_secpkg_table->AllocateClientBuffer( NULL, signature_size,
                (void **)&info.sSignatureAlgorithmName );
        if (status) return status;
        lsa_secpkg_table->CopyToClientBuffer( NULL, signature_size,
                info.sSignatureAlgorithmName, signature_alg );

        status = lsa_secpkg_table->AllocateClientBuffer( NULL, sizeof(L"RSADSI RC4"),
                (void **)&info.sEncryptAlgorithmName );
        if (status)
        {
            lsa_secpkg_table->FreeClientBuffer( NULL, info.sSignatureAlgorithmName );
            return status;
        }
        lsa_secpkg_table->CopyToClientBuffer( NULL, sizeof(L"RSADSI RC4"),
                info.sEncryptAlgorithmName, (void *)L"RSADSI RC4" );

        info.KeySize = sizeof(ctx->session_key) * 8;
        info.SignatureAlgorithm = signature_algid;
        info.EncryptAlgorithm = CALG_RC4;

        lsa_secpkg_table->GetCallInfo( &call_info );
        if (call_info.Attributes & SECPKG_CALL_WOWCLIENT)
        {
            struct
            {
                ULONG sSignatureAlgorithmName;
                ULONG sEncryptAlgorithmName;
                ULONG KeySize;
                ULONG SignatureAlgorithm;
                ULONG EncryptAlgorithm;
            } info32 =
            {
                (ULONG_PTR)info.sSignatureAlgorithmName,
                (ULONG_PTR)info.sEncryptAlgorithmName,
                info.KeySize,
                info.SignatureAlgorithm,
                info.EncryptAlgorithm
            };

            return lsa_secpkg_table->CopyToClientBuffer( NULL, sizeof(info32), buf, &info32 );
        }
        return lsa_secpkg_table->CopyToClientBuffer( NULL, sizeof(info), buf, &info );
    }
#undef X
    default:
        FIXME( "unknown attribute %lu\n", attr );
        break;
    }

    return SEC_E_UNSUPPORTED_FUNCTION;
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
    ntlm_SpQueryCredentialsAttributes,
    ntlm_SpFreeCredentialsHandle,
    NULL, /* SaveCredentials */
    NULL, /* GetCredentials */
    NULL, /* DeleteCredentials */
    ntlm_SpInitLsaModeContext,
    ntlm_SpAcceptLsaModeContext,
    ntlm_SpDeleteContext,
    NULL, /* ApplyControlToken */
    NULL, /* GetUserInfo */
    NULL, /* GetExtendedInformation */
    ntlm_SpQueryContextAttributes,
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
    TRACE( "%#lx, %p, %p, %p\n", lsa_version, package_version, table, table_count );

    *package_version = SECPKG_INTERFACE_VERSION;
    *table = &ntlm_table;
    *table_count = 1;
    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI ntlm_SpInstanceInit( ULONG version, SECPKG_DLL_FUNCTIONS *dll_functions, void **user_functions )
{
    TRACE( "%#lx, %p, %p\n", version, dll_functions, user_functions );
    return STATUS_SUCCESS;
}

static struct user_ctx* find_user_ctx( LSA_SEC_HANDLE handle )
{
    struct user_ctx *ret;

    EnterCriticalSection( &user_ctx_cs );
    LIST_FOR_EACH_ENTRY( ret, &user_ctx_list, struct user_ctx, entry )
    {
        if (ret->handle == handle)
        {
            LeaveCriticalSection( &user_ctx_cs );
            return ret;
        }
    }
    LeaveCriticalSection( &user_ctx_cs );
    return NULL;
}

static NTSTATUS NTAPI ntlm_SpInitUserModeContext( LSA_SEC_HANDLE handle, SecBuffer *buf )
{
    struct user_context_data *data = buf->pvBuffer;
    struct user_ctx *ctx;

    TRACE( "%Ix, %p\n", handle, buf);

    if (buf->cbBuffer != sizeof( *data ))
        return SEC_E_INTERNAL_ERROR;

    EnterCriticalSection( &user_ctx_cs );
    ctx = find_user_ctx( handle );
    if (!ctx)
    {
        ctx = malloc( sizeof(*ctx) );
        if (!ctx)
        {
            LeaveCriticalSection( &user_ctx_cs );
            return SEC_E_INSUFFICIENT_MEMORY;
        }
        list_add_head( &user_ctx_list, &ctx->entry );
    }

    ctx->handle = handle;
    ctx->mode = data->mode;
    ctx->flags = data->flags;

    arc4_init( &ctx->ntlm.arc4info, data->session_key, 16 );
    ctx->ntlm.seq_no = 0;
    create_ntlm2_subkeys( ctx, data->session_key );
    arc4_init( &ctx->ntlm2.send_arc4info, ctx->ntlm2.send_seal_key, 16 );
    arc4_init( &ctx->ntlm2.recv_arc4info, ctx->ntlm2.recv_seal_key, 16 );
    ctx->ntlm2.send_seq_no = 0;
    ctx->ntlm2.recv_seq_no = 0;
    LeaveCriticalSection( &user_ctx_cs );
    return STATUS_SUCCESS;
}

static SECURITY_STATUS create_signature( struct user_ctx *ctx, unsigned int flags, SecBufferDesc *msg,
                                         SecBuffer *sig_buf, enum sign_direction dir, BOOL encrypt )
{
    unsigned int i, sign_version = 1;
    char *sig = sig_buf->pvBuffer;

    if (flags & NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY && flags & NTLMSSP_NEGOTIATE_SIGN)
    {
        char digest[16], seq_no[4];
        struct hmac_md5_ctx hmac_md5;

        if (dir == SIGN_SEND)
        {
            seq_no[0] = (ctx->ntlm2.send_seq_no >>  0) & 0xff;
            seq_no[1] = (ctx->ntlm2.send_seq_no >>  8) & 0xff;
            seq_no[2] = (ctx->ntlm2.send_seq_no >> 16) & 0xff;
            seq_no[3] = (ctx->ntlm2.send_seq_no >> 24) & 0xff;
            ctx->ntlm2.send_seq_no++;

            hmac_md5_init( &hmac_md5, ctx->ntlm2.send_sign_key, 16 );
        }
        else
        {
            seq_no[0] = (ctx->ntlm2.recv_seq_no >>  0) & 0xff;
            seq_no[1] = (ctx->ntlm2.recv_seq_no >>  8) & 0xff;
            seq_no[2] = (ctx->ntlm2.recv_seq_no >> 16) & 0xff;
            seq_no[3] = (ctx->ntlm2.recv_seq_no >> 24) & 0xff;
            ctx->ntlm2.recv_seq_no++;

            hmac_md5_init( &hmac_md5, ctx->ntlm2.recv_sign_key, 16 );
        }

        hmac_md5_update( &hmac_md5, seq_no, 4 );
        for (i = 0; i < msg->cBuffers; ++i)
        {
            if (msg->pBuffers[i].BufferType & SECBUFFER_DATA)
                hmac_md5_update( &hmac_md5, msg->pBuffers[i].pvBuffer, msg->pBuffers[i].cbBuffer );
        }
        hmac_md5_final( &hmac_md5, digest );

        if (encrypt && flags & NTLMSSP_NEGOTIATE_KEY_EXCH)
        {
            if (dir == SIGN_SEND)
                arc4_process( &ctx->ntlm2.send_arc4info, digest, 8 );
            else
                arc4_process( &ctx->ntlm2.recv_arc4info, digest, 8 );
        }

        sig[0] = (sign_version >>  0) & 0xff;
        sig[1] = (sign_version >>  8) & 0xff;
        sig[2] = (sign_version >> 16) & 0xff;
        sig[3] = (sign_version >> 24) & 0xff;
        memcpy( sig + 4, digest, 8 );
        memcpy( sig + 12, seq_no, 4 );

        sig_buf->cbBuffer = 16;
        return SEC_E_OK;
    }

    if (flags & NTLMSSP_NEGOTIATE_SIGN)
    {
        unsigned int crc = 0;

        for (i = 0; i < msg->cBuffers; ++i)
        {
            if (msg->pBuffers[i].BufferType & SECBUFFER_DATA)
                crc = RtlComputeCrc32( crc, msg->pBuffers[i].pvBuffer, msg->pBuffers[i].cbBuffer );
        }

        sig[0] = (sign_version >>  0) & 0xff;
        sig[1] = (sign_version >>  8) & 0xff;
        sig[2] = (sign_version >> 16) & 0xff;
        sig[3] = (sign_version >> 24) & 0xff;
        memset( sig + 4, 0, 4 );
        sig[8] = (crc >>  0) & 0xff;
        sig[9] = (crc >>  8) & 0xff;
        sig[10] = (crc >> 16) & 0xff;
        sig[11] = (crc >> 24) & 0xff;
        sig[12] = (ctx->ntlm.seq_no >>  0) & 0xff;
        sig[13] = (ctx->ntlm.seq_no >>  8) & 0xff;
        sig[14] = (ctx->ntlm.seq_no >> 16) & 0xff;
        sig[15] = (ctx->ntlm.seq_no >> 24) & 0xff;
        ctx->ntlm.seq_no++;

        if (encrypt) arc4_process( &ctx->ntlm.arc4info, sig + 4, 12 );
        return SEC_E_OK;
    }

    if (flags & NTLMSSP_NEGOTIATE_ALWAYS_SIGN || !flags)
    {
        /* create dummy signature */
        memset( sig_buf->pvBuffer, 0, 16 );
        memset( sig_buf->pvBuffer, 1, 1 );
        sig_buf->cbBuffer = 16;
        return SEC_E_OK;
    }

    return SEC_E_UNSUPPORTED_FUNCTION;
}

static NTSTATUS NTAPI ntlm_SpMakeSignature( LSA_SEC_HANDLE handle, ULONG qop, SecBufferDesc *msg, ULONG msg_seq_no )
{
    struct user_ctx *ctx;
    int idx;

    TRACE( "%#Ix, %#lx, %p, %lu\n", handle, qop, msg, msg_seq_no );
    if (qop) FIXME( "ignoring quality of protection %#lx\n", qop );
    if (msg_seq_no) FIXME( "ignoring message sequence number %lu\n", msg_seq_no );

    if (!handle) return SEC_E_INVALID_HANDLE;
    if (!msg || !msg->pBuffers || msg->cBuffers < 2 || (idx = get_buffer_index( msg, SECBUFFER_TOKEN )) == -1)
        return SEC_E_INVALID_TOKEN;
    if (msg->pBuffers[idx].cbBuffer < 16) return SEC_E_BUFFER_TOO_SMALL;
    if (!(ctx = find_user_ctx( handle ))) return SEC_E_INVALID_HANDLE;

    return create_signature( ctx, ctx->flags, msg, &msg->pBuffers[idx], SIGN_SEND, TRUE );
}

static NTSTATUS verify_signature( struct user_ctx *ctx, unsigned int flags, SecBufferDesc *msg, SecBuffer *sig_buf )
{
    NTSTATUS status;
    unsigned int i, sig_idx = 0;
    SecBufferDesc desc;
    SecBuffer *buf;
    char sig[16];

    if (!(buf = malloc( msg->cBuffers * sizeof(*buf) ))) return SEC_E_INSUFFICIENT_MEMORY;
    desc.ulVersion = SECBUFFER_VERSION;
    desc.cBuffers  = msg->cBuffers;
    desc.pBuffers  = buf;

    for (i = 0; i < msg->cBuffers; i++)
    {
        if (msg->pBuffers[i].BufferType == SECBUFFER_TOKEN || msg->pBuffers[i].BufferType == SECBUFFER_STREAM)
        {
            buf[i].BufferType = SECBUFFER_TOKEN;
            buf[i].cbBuffer   = 16;
            buf[i].pvBuffer   = sig;
            sig_idx = i;
        }
        else
        {
            buf[i].BufferType = msg->pBuffers[i].BufferType;
            buf[i].cbBuffer   = msg->pBuffers[i].cbBuffer;
            buf[i].pvBuffer   = msg->pBuffers[i].pvBuffer;
        }
    }

    if ((status = create_signature( ctx, flags, &desc, &buf[sig_idx], SIGN_RECV, TRUE )) == SEC_E_OK)
    {
        if (memcmp( (char *)buf[sig_idx].pvBuffer + 8, (char *)sig_buf->pvBuffer + 8, 8 ))
            status = SEC_E_MESSAGE_ALTERED;
    }

    free( buf );
    if (status != SEC_E_OK) TRACE( "signature verification failed %lx\n", status );
    return status;
}

static NTSTATUS NTAPI ntlm_SpVerifySignature( LSA_SEC_HANDLE handle, SecBufferDesc *msg, ULONG msg_seq_no, ULONG *qop )
{
    struct user_ctx *ctx;
    int idx;

    TRACE( "%#Ix, %p, %lu, %p\n", handle, msg, msg_seq_no, qop );
    if (msg_seq_no) FIXME( "ignoring message sequence number %lu\n", msg_seq_no );

    if (!handle) return SEC_E_INVALID_HANDLE;
    if (!msg || !msg->pBuffers || msg->cBuffers < 2 || (idx = get_buffer_index( msg, SECBUFFER_TOKEN )) == -1)
        return SEC_E_INVALID_TOKEN;
    if (msg->pBuffers[idx].cbBuffer < 16) return SEC_E_BUFFER_TOO_SMALL;
    if (!(ctx = find_user_ctx( handle ))) return SEC_E_INVALID_HANDLE;

    return verify_signature( ctx, ctx->flags, msg, &msg->pBuffers[idx] );
}

static NTSTATUS NTAPI ntlm_SpSealMessage( LSA_SEC_HANDLE handle, ULONG qop, SecBufferDesc *msg, ULONG msg_seq_no )
{
    int token_idx, data_idx;
    struct user_ctx *ctx;

    TRACE( "%#Ix, %#lx, %p %lu\n", handle, qop, msg, msg_seq_no );
    if (qop) FIXME( "ignoring quality of protection %#lx\n", qop );
    if (msg_seq_no) FIXME( "ignoring message sequence number %lu\n", msg_seq_no );

    if (!handle) return SEC_E_INVALID_HANDLE;
    if (!(ctx = find_user_ctx( handle ))) return SEC_E_INVALID_HANDLE;

    if (!msg || !msg->pBuffers || msg->cBuffers < 2 ||
        (token_idx = get_buffer_index( msg, SECBUFFER_TOKEN )) == -1 ||
        (data_idx = get_buffer_index( msg, SECBUFFER_DATA )) == -1) return SEC_E_INVALID_TOKEN;

    if (msg->pBuffers[token_idx].cbBuffer < 16) return SEC_E_BUFFER_TOO_SMALL;

    if (ctx->flags & NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY && ctx->flags & NTLMSSP_NEGOTIATE_SEAL)
    {
        create_signature( ctx, ctx->flags, msg, &msg->pBuffers[token_idx], SIGN_SEND, FALSE );

        arc4_process( &ctx->ntlm2.send_arc4info, msg->pBuffers[data_idx].pvBuffer,
                      msg->pBuffers[data_idx].cbBuffer );
        if (ctx->flags & NTLMSSP_NEGOTIATE_KEY_EXCH)
            arc4_process( &ctx->ntlm2.send_arc4info, (char *)msg->pBuffers[token_idx].pvBuffer + 4, 8 );
    }
    else
    {
        char *sig = msg->pBuffers[token_idx].pvBuffer;

        create_signature( ctx, ctx->flags | NTLMSSP_NEGOTIATE_SIGN, msg, &msg->pBuffers[token_idx], SIGN_SEND, FALSE );

        arc4_process( &ctx->ntlm.arc4info, msg->pBuffers[data_idx].pvBuffer, msg->pBuffers[data_idx].cbBuffer );
        arc4_process( &ctx->ntlm.arc4info, sig + 4, 12 );

        if (ctx->flags & NTLMSSP_NEGOTIATE_ALWAYS_SIGN || !ctx->flags) memset( sig + 4, 0, 4 );
    }

    return SEC_E_OK;
}

static NTSTATUS NTAPI ntlm_SpUnsealMessage( LSA_SEC_HANDLE handle, SecBufferDesc *msg, ULONG msg_seq_no, ULONG *qop )
{
    int i, data_idx, stream_idx, token_idx;
    SecBuffer token_buf;
    struct user_ctx *ctx;

    TRACE( "%#Ix, %p, %lu, %p\n", handle, msg, msg_seq_no, qop );
    if (msg_seq_no) FIXME( "ignoring message sequence number %lu\n", msg_seq_no );

    if (!handle) return SEC_E_INVALID_HANDLE;
    if (!(ctx = find_user_ctx( handle ))) return SEC_E_INVALID_HANDLE;

    if (!msg || !msg->pBuffers || msg->cBuffers < 2) return SEC_E_INVALID_TOKEN;

    data_idx = get_buffer_index( msg, SECBUFFER_DATA );
    stream_idx = get_buffer_index( msg, SECBUFFER_STREAM );
    token_idx = get_buffer_index( msg, SECBUFFER_TOKEN );

    if ((token_idx == -1 && stream_idx == -1) || (stream_idx != -1 && token_idx != -1) || data_idx == -1)
        return SEC_E_INVALID_TOKEN;

    if (stream_idx != -1)
    {
        if (msg->pBuffers[stream_idx].cbBuffer < 16) return SEC_E_BUFFER_TOO_SMALL;
        token_buf.BufferType = SECBUFFER_TOKEN;
        token_buf.cbBuffer = 16;
        token_buf.pvBuffer = msg->pBuffers[stream_idx].pvBuffer;

        /* native decrypts in-place even when an appropriately sized data buffer is supplied */
        msg->pBuffers[data_idx].pvBuffer = (char *)msg->pBuffers[stream_idx].pvBuffer + 16;
        msg->pBuffers[data_idx].cbBuffer = msg->pBuffers[stream_idx].cbBuffer - 16;
    }
    else
    {
        if (msg->pBuffers[token_idx].cbBuffer < 16) return SEC_E_BUFFER_TOO_SMALL;
        token_buf = msg->pBuffers[token_idx];
    }

    if (ctx->flags & NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY && ctx->flags & NTLMSSP_NEGOTIATE_SEAL)
    {
        for (i = 0; i < msg->cBuffers; i++)
        {
            if (msg->pBuffers[i].BufferType != SECBUFFER_DATA) continue;
            arc4_process( &ctx->ntlm2.recv_arc4info, msg->pBuffers[i].pvBuffer,
                          msg->pBuffers[i].cbBuffer );
        }
    }
    else
    {
        for (i = 0; i < msg->cBuffers; i++)
        {
            if (msg->pBuffers[i].BufferType != SECBUFFER_DATA) continue;
            arc4_process( &ctx->ntlm.arc4info, msg->pBuffers[i].pvBuffer,
                          msg->pBuffers[i].cbBuffer);
        }
    }

    /* make sure we use a session key for the signature check, SealMessage always does that,
       even in the dummy case */
    return verify_signature( ctx, ctx->flags | NTLMSSP_NEGOTIATE_SIGN, msg, &token_buf );
}

static NTSTATUS NTAPI ntlm_SpDeleteUserModeContext( LSA_SEC_HANDLE handle )
{
    struct user_ctx *user_ctx;
    TRACE( "%Ix\n", handle );

    EnterCriticalSection( &user_ctx_cs );
    user_ctx = find_user_ctx( handle );
    if (user_ctx)
    {
        list_remove( &user_ctx->entry );
        free( user_ctx );
    }
    LeaveCriticalSection( &user_ctx_cs );
    return STATUS_SUCCESS;
}

static SECPKG_USER_FUNCTION_TABLE ntlm_user_table =
{
    ntlm_SpInstanceInit,
    ntlm_SpInitUserModeContext,
    ntlm_SpMakeSignature,
    ntlm_SpVerifySignature,
    ntlm_SpSealMessage,
    ntlm_SpUnsealMessage,
    NULL, /* SpGetContextToken */
    NULL, /* SpQueryContextAttributes */
    NULL, /* SpCompleteAuthToken */
    ntlm_SpDeleteUserModeContext,
    NULL, /* SpFormatCredentialsFn */
    NULL, /* SpMarshallSupplementalCreds */
    NULL, /* SpExportSecurityContext */
    NULL  /* SpImportSecurityContext */
};

NTSTATUS NTAPI SpUserModeInitialize( ULONG lsa_version, ULONG *package_version, SECPKG_USER_FUNCTION_TABLE **table,
                                     ULONG *table_count )
{
    TRACE( "%#lx, %p, %p, %p\n", lsa_version, package_version, table, table_count );

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
        if (__wine_init_unix_call())
            return FALSE;
        DisableThreadLibraryCalls( hinst );
        break;
    case DLL_PROCESS_DETACH:
        if (reserved) break;
        local_auth_cleanup();
        break;
    }
    return TRUE;
}
