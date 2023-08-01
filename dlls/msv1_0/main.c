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
#define WIN32_NO_STATUS
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

#include "wine/debug.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

static ULONG ntlm_package_id;
static LSA_DISPATCH_TABLE lsa_dispatch;

static NTSTATUS ntlm_check_version(void)
{
    return WINE_UNIX_CALL( unix_check_version, NULL );
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

    if (ntlm_check_version())
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
    TRACE( "%#Ix, %p, %p\n", package_id, params, lsa_function_table );

    if (ntlm_check_version())
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

#define WINE_NO_CACHED_CREDENTIALS 0x10000000
static NTSTATUS NTAPI ntlm_SpAcquireCredentialsHandle( UNICODE_STRING *principal, ULONG cred_use, LUID *logon_id,
                                                       void *auth_data, void *get_key_fn, void *get_key_arg,
                                                       LSA_SEC_HANDLE *handle, TimeStamp *expiry )
{
    SECURITY_STATUS status = SEC_E_INSUFFICIENT_MEMORY;
    struct ntlm_cred *cred = NULL;
    WCHAR *domain = NULL, *user = NULL, *password = NULL;
    SEC_WINNT_AUTH_IDENTITY_W *id = NULL;

    TRACE( "%s, %#lx, %p, %p, %p, %p, %p, %p\n", debugstr_us(principal), cred_use, logon_id, auth_data,
           get_key_fn, get_key_arg, cred, expiry );

    switch (cred_use & ~SECPKG_CRED_RESERVED)
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
        cred->no_cached_credentials = (cred_use & WINE_NO_CACHED_CREDENTIALS);

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

    TRACE( "%#Ix\n", handle );

    if (!cred) return SEC_E_OK;

    cred->mode = MODE_INVALID;
    if (cred->password) memset( cred->password, 0, cred->password_len );
    free( cred->password );
    free( cred->username_arg );
    free( cred->domain_arg );
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

struct md5_ctx
{
    unsigned int i[2];
    unsigned int buf[4];
    unsigned char in[64];
    unsigned char digest[16];
};

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

static void create_ntlm2_subkeys( struct ntlm_ctx *ctx )
{
    if (ctx->mode == MODE_CLIENT)
    {
        calc_ntlm2_subkey( ctx->session_key, client_to_server_sign_constant, ctx->crypt.ntlm2.send_sign_key );
        calc_ntlm2_subkey( ctx->session_key, client_to_server_seal_constant, ctx->crypt.ntlm2.send_seal_key );
        calc_ntlm2_subkey( ctx->session_key, server_to_client_sign_constant, ctx->crypt.ntlm2.recv_sign_key );
        calc_ntlm2_subkey( ctx->session_key, server_to_client_seal_constant, ctx->crypt.ntlm2.recv_seal_key );
    }
    else
    {
        calc_ntlm2_subkey( ctx->session_key, server_to_client_sign_constant, ctx->crypt.ntlm2.send_sign_key );
        calc_ntlm2_subkey( ctx->session_key, server_to_client_seal_constant, ctx->crypt.ntlm2.send_seal_key );
        calc_ntlm2_subkey( ctx->session_key, client_to_server_sign_constant, ctx->crypt.ntlm2.recv_sign_key );
        calc_ntlm2_subkey( ctx->session_key, client_to_server_seal_constant, ctx->crypt.ntlm2.recv_seal_key );
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

    if (!(buf = malloc( NTLM_MAX_BUF ))) return SEC_E_INSUFFICIENT_MEMORY;
    if (!(bin = malloc( NTLM_MAX_BUF ))) goto done;

    if (!ctx_handle && !input)
    {
        char *argv[5];
        int password_len = 0;
        struct ntlm_cred *cred = (struct ntlm_cred *)cred_handle;

        if (!cred || cred->mode != MODE_CLIENT)
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
        if (ctx_req & ISC_REQ_CONFIDENTIALITY) strcat( want_flags, " NTLMSSP_FEATURE_SEAL" );
        if ((ctx_req & ISC_REQ_INTEGRITY) || (ctx_req & ISC_REQ_REPLAY_DETECT) ||
            (ctx_req & ISC_REQ_SEQUENCE_DETECT)) strcat( want_flags, " NTLMSSP_FEATURE_SIGN" );

        if (ctx_req & ISC_REQ_CONNECTION) ctx->attrs |= ISC_RET_CONNECTION;
        if (ctx_req & ISC_REQ_EXTENDED_ERROR) ctx->attrs |= ISC_RET_EXTENDED_ERROR;
        if (ctx_req & ISC_REQ_MUTUAL_AUTH) ctx->attrs |= ISC_RET_MUTUAL_AUTH;
        if (ctx_req & ISC_REQ_USE_DCE_STYLE) ctx->attrs |= ISC_RET_USED_DCE_STYLE;
        if (ctx_req & ISC_REQ_DELEGATE) ctx->attrs |= ISC_RET_DELEGATE;
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

        TRACE( "sending to ntlm_auth: %s\n", debugstr_a(buf) );
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

        *new_ctx_handle = (LSA_SEC_HANDLE)ctx;
        status = SEC_I_CONTINUE_NEEDED;
    }
    else /* !ctx_handle && !input */
    {
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

        if (!input->pBuffers[idx].pvBuffer || input->pBuffers[idx].cbBuffer > NTLM_MAX_BUF)
        {
            status = SEC_E_INVALID_TOKEN;
            goto done;
        }
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
        if (!(output->pBuffers[idx].pvBuffer = RtlAllocateHeap( GetProcessHeap(), 0, bin_len )))
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

        arc4_init( &ctx->crypt.ntlm.arc4info, ctx->session_key, 16 );
        ctx->crypt.ntlm.seq_no = 0;
        create_ntlm2_subkeys( ctx );
        arc4_init( &ctx->crypt.ntlm2.send_arc4info, ctx->crypt.ntlm2.send_seal_key, 16 );
        arc4_init( &ctx->crypt.ntlm2.recv_arc4info, ctx->crypt.ntlm2.recv_seal_key, 16 );
        ctx->crypt.ntlm2.send_seq_no = 0;
        ctx->crypt.ntlm2.recv_seq_no = 0;
    }

done:
    if (status != SEC_E_OK && status != SEC_I_CONTINUE_NEEDED && !ctx_handle && !input)
    {
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

    if (!(buf = malloc( NTLM_MAX_BUF ))) return SEC_E_INSUFFICIENT_MEMORY;
    if (!(bin = malloc( NTLM_MAX_BUF ))) goto done;

    if (!ctx_handle)
    {
        struct ntlm_cred *cred = (struct ntlm_cred *)cred_handle;
        char *argv[3];

        if (!cred || cred->mode != MODE_SERVER)
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
        if (ctx_req & ASC_REQ_CONNECTION)
        {
            strcat( want_flags, " NTLMSSP_FEATURE_SESSION_KEY" );
            ctx->attrs |= ASC_RET_CONNECTION;
        }
        if (ctx_req & ASC_REQ_INTEGRITY) strcat( want_flags, " NTLMSSP_FEATURE_SIGN" );
        if (ctx_req & ASC_REQ_ALLOCATE_MEMORY) FIXME( "ASC_REQ_ALLOCATE_MEMORY\n" );
        if (ctx_req & ASC_REQ_EXTENDED_ERROR) FIXME( "ASC_REQ_EXTENDED_ERROR\n" );
        if (ctx_req & ASC_REQ_MUTUAL_AUTH) FIXME( "ASC_REQ_MUTUAL_AUTH\n" );
        if (ctx_req & ASC_REQ_REPLAY_DETECT) FIXME( "ASC_REQ_REPLAY_DETECT\n" );
        if (ctx_req & ASC_REQ_SEQUENCE_DETECT) FIXME( "ASC_REQ_SEQUENCE_DETECT\n" );
        if (ctx_req & ASC_REQ_STREAM) FIXME( "ASC_REQ_STREAM\n" );

        if (strlen( want_flags ) > 3)
        {
            TRACE( "want flags are %s\n", debugstr_a(want_flags) );
            strcpy( buf, want_flags );
            if ((status = ntlm_chat( ctx, buf, NTLM_MAX_BUF, &len )) != SEC_E_OK) goto done;
            if (!strncmp( buf, "BH", 2 )) ERR( "ntlm_auth doesn't understand new command set\n" );
        }

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

        if (!output || output->cBuffers < 1)
        {
            status = SEC_E_INSUFFICIENT_MEMORY;
            goto done;
        }
        output->pBuffers[0].cbBuffer = bin_len;
        output->pBuffers[0].BufferType = SECBUFFER_DATA;
        memcpy( output->pBuffers[0].pvBuffer, bin, bin_len );

        *new_ctx_handle = (LSA_SEC_HANDLE)ctx;
        status = SEC_I_CONTINUE_NEEDED;
    }
    else
    {
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

        if (input->pBuffers[0].cbBuffer > NTLM_MAX_BUF)
        {
            status = SEC_E_INVALID_TOKEN;
            goto done;
        }
        else bin_len = input->pBuffers[0].cbBuffer;
        memcpy( bin, input->pBuffers[0].pvBuffer, bin_len );

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
        arc4_init( &ctx->crypt.ntlm.arc4info, ctx->session_key, 16 );
        ctx->crypt.ntlm.seq_no = 0;

        *new_ctx_handle = (LSA_SEC_HANDLE)ctx;
        status = SEC_E_OK;
    }

done:
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
    ntlm_cleanup( ctx );
    free( ctx );
    return SEC_E_OK;
}

static SecPkgInfoW *build_package_info( const SecPkgInfoW *info )
{
    SecPkgInfoW *ret;
    DWORD size_name = (wcslen(info->Name) + 1) * sizeof(WCHAR);
    DWORD size_comment = (wcslen(info->Comment) + 1) * sizeof(WCHAR);

    if (!(ret = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*ret) + size_name + size_comment ))) return NULL;
    ret->fCapabilities = info->fCapabilities;
    ret->wVersion      = info->wVersion;
    ret->wRPCID        = info->wRPCID;
    ret->cbMaxToken    = info->cbMaxToken;
    ret->Name          = (SEC_WCHAR *)(ret + 1);
    memcpy( ret->Name, info->Name, size_name );
    ret->Comment       = (SEC_WCHAR *)((char *)ret->Name + size_name);
    memcpy( ret->Comment, info->Comment, size_comment );
    return ret;
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
        SecPkgContext_Flags *flags = (SecPkgContext_Flags *)buf;
        struct ntlm_ctx *ctx = (struct ntlm_ctx *)handle;

        flags->Flags = 0;
        if (ctx->flags & FLAG_NEGOTIATE_SIGN) flags->Flags |= ISC_RET_INTEGRITY;
        if (ctx->flags & FLAG_NEGOTIATE_SEAL) flags->Flags |= ISC_RET_CONFIDENTIALITY;
        return SEC_E_OK;
    }
    case SECPKG_ATTR_SIZES:
    {
        SecPkgContext_Sizes *sizes = (SecPkgContext_Sizes *)buf;
        sizes->cbMaxToken        = NTLM_MAX_BUF;
        sizes->cbMaxSignature    = 16;
        sizes->cbBlockSize       = 0;
        sizes->cbSecurityTrailer = 16;
        return SEC_E_OK;
    }
    case SECPKG_ATTR_NEGOTIATION_INFO:
    {
        SecPkgContext_NegotiationInfoW *info = (SecPkgContext_NegotiationInfoW *)buf;
        if (!(info->PackageInfo = build_package_info( &ntlm_package_info ))) return SEC_E_INSUFFICIENT_MEMORY;
        info->NegotiationState = SECPKG_NEGOTIATION_COMPLETE;
        return SEC_E_OK;
    }
    case SECPKG_ATTR_SESSION_KEY:
    {
        struct ntlm_ctx *ctx = (struct ntlm_ctx *)handle;
        SecPkgContext_SessionKey *key = (SecPkgContext_SessionKey *)buf;
        unsigned char *session_key;

        if (!(session_key = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(ctx->session_key) )))
            return SEC_E_INSUFFICIENT_MEMORY;
        memcpy( session_key, ctx->session_key, sizeof(ctx->session_key) );
        key->SessionKey = session_key;
        key->SessionKeyLength = sizeof(ctx->session_key);
        return SEC_E_OK;
    }
    case SECPKG_ATTR_KEY_INFO:
    {
        struct ntlm_ctx *ctx = (struct ntlm_ctx *)handle;
        SecPkgContext_KeyInfoW *info = (SecPkgContext_KeyInfoW *)buf;
        SEC_WCHAR *signature_alg;
        ULONG signature_size, signature_algid;

        if (ctx->flags & FLAG_NEGOTIATE_KEY_EXCHANGE)
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

        if (!(info->sSignatureAlgorithmName = RtlAllocateHeap( GetProcessHeap(), 0, signature_size )))
            return SEC_E_INSUFFICIENT_MEMORY;
        wcscpy( info->sSignatureAlgorithmName, signature_alg );

        if (!(info->sEncryptAlgorithmName = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(L"RSADSI RC4") )))
        {
            RtlFreeHeap( GetProcessHeap(), 0, info->sSignatureAlgorithmName );
            return SEC_E_INSUFFICIENT_MEMORY;
        }
        wcscpy( info->sEncryptAlgorithmName, L"RSADSI RC4" );

        info->KeySize = sizeof(ctx->session_key) * 8;
        info->SignatureAlgorithm = signature_algid;
        info->EncryptAlgorithm = CALG_RC4;
        return SEC_E_OK;
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
    NULL, /* SpQueryCredentialsAttributes */
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

struct hmac_md5_ctx
{
    struct md5_ctx ctx;
    char outer_padding[64];
};

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

static SECURITY_STATUS create_signature( struct ntlm_ctx *ctx, unsigned int flags, SecBufferDesc *msg, int idx,
                                         enum sign_direction dir, BOOL encrypt )
{
    unsigned int i, sign_version = 1;
    char *sig = msg->pBuffers[idx].pvBuffer;

    if (flags & FLAG_NEGOTIATE_NTLM2 && flags & FLAG_NEGOTIATE_SIGN)
    {
        char digest[16], seq_no[4];
        struct hmac_md5_ctx hmac_md5;

        if (dir == SIGN_SEND)
        {
            seq_no[0] = (ctx->crypt.ntlm2.send_seq_no >>  0) & 0xff;
            seq_no[1] = (ctx->crypt.ntlm2.send_seq_no >>  8) & 0xff;
            seq_no[2] = (ctx->crypt.ntlm2.send_seq_no >> 16) & 0xff;
            seq_no[3] = (ctx->crypt.ntlm2.send_seq_no >> 24) & 0xff;
            ctx->crypt.ntlm2.send_seq_no++;

            hmac_md5_init( &hmac_md5, ctx->crypt.ntlm2.send_sign_key, 16 );
        }
        else
        {
            seq_no[0] = (ctx->crypt.ntlm2.recv_seq_no >>  0) & 0xff;
            seq_no[1] = (ctx->crypt.ntlm2.recv_seq_no >>  8) & 0xff;
            seq_no[2] = (ctx->crypt.ntlm2.recv_seq_no >> 16) & 0xff;
            seq_no[3] = (ctx->crypt.ntlm2.recv_seq_no >> 24) & 0xff;
            ctx->crypt.ntlm2.recv_seq_no++;

            hmac_md5_init( &hmac_md5, ctx->crypt.ntlm2.recv_sign_key, 16 );
        }

        hmac_md5_update( &hmac_md5, seq_no, 4 );
        for (i = 0; i < msg->cBuffers; ++i)
        {
            if (msg->pBuffers[i].BufferType & SECBUFFER_DATA)
                hmac_md5_update( &hmac_md5, msg->pBuffers[i].pvBuffer, msg->pBuffers[i].cbBuffer );
        }
        hmac_md5_final( &hmac_md5, digest );

        if (encrypt && flags & FLAG_NEGOTIATE_KEY_EXCHANGE)
        {
            if (dir == SIGN_SEND)
                arc4_process( &ctx->crypt.ntlm2.send_arc4info, digest, 8 );
            else
                arc4_process( &ctx->crypt.ntlm2.recv_arc4info, digest, 8 );
        }

        sig[0] = (sign_version >>  0) & 0xff;
        sig[1] = (sign_version >>  8) & 0xff;
        sig[2] = (sign_version >> 16) & 0xff;
        sig[3] = (sign_version >> 24) & 0xff;
        memcpy( sig + 4, digest, 8 );
        memcpy( sig + 12, seq_no, 4 );

        msg->pBuffers[idx].cbBuffer = 16;
        return SEC_E_OK;
    }

    if (flags & FLAG_NEGOTIATE_SIGN)
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
        sig[12] = (ctx->crypt.ntlm.seq_no >>  0) & 0xff;
        sig[13] = (ctx->crypt.ntlm.seq_no >>  8) & 0xff;
        sig[14] = (ctx->crypt.ntlm.seq_no >> 16) & 0xff;
        sig[15] = (ctx->crypt.ntlm.seq_no >> 24) & 0xff;
        ctx->crypt.ntlm.seq_no++;

        if (encrypt) arc4_process( &ctx->crypt.ntlm.arc4info, sig + 4, 12 );
        return SEC_E_OK;
    }

    if (flags & FLAG_NEGOTIATE_ALWAYS_SIGN || !flags)
    {
        /* create dummy signature */
        memset( msg->pBuffers[idx].pvBuffer, 0, 16 );
        memset( msg->pBuffers[idx].pvBuffer, 1, 1 );
        msg->pBuffers[idx].cbBuffer = 16;
        return SEC_E_OK;
    }

    return SEC_E_UNSUPPORTED_FUNCTION;
}

static NTSTATUS NTAPI ntlm_SpMakeSignature( LSA_SEC_HANDLE handle, ULONG qop, SecBufferDesc *msg, ULONG msg_seq_no )
{
    struct ntlm_ctx *ctx = (struct ntlm_ctx *)handle;
    int idx;

    TRACE( "%#Ix, %#lx, %p, %lu\n", handle, qop, msg, msg_seq_no );
    if (qop) FIXME( "ignoring quality of protection %#lx\n", qop );
    if (msg_seq_no) FIXME( "ignoring message sequence number %lu\n", msg_seq_no );

    if (!handle) return SEC_E_INVALID_HANDLE;
    if (!msg || !msg->pBuffers || msg->cBuffers < 2 || (idx = get_buffer_index( msg, SECBUFFER_TOKEN )) == -1)
        return SEC_E_INVALID_TOKEN;
    if (msg->pBuffers[idx].cbBuffer < 16) return SEC_E_BUFFER_TOO_SMALL;

    return create_signature( ctx, ctx->flags, msg, idx, SIGN_SEND, TRUE );
}

static NTSTATUS verify_signature( struct ntlm_ctx *ctx, unsigned int flags, SecBufferDesc *msg, int idx )
{
    NTSTATUS status;
    unsigned int i;
    SecBufferDesc desc;
    SecBuffer *buf;
    char sig[16];

    if (!(buf = malloc( msg->cBuffers * sizeof(*buf) ))) return SEC_E_INSUFFICIENT_MEMORY;
    desc.ulVersion = SECBUFFER_VERSION;
    desc.cBuffers  = msg->cBuffers;
    desc.pBuffers  = buf;

    for (i = 0; i < msg->cBuffers; i++)
    {
        if (msg->pBuffers[i].BufferType == SECBUFFER_TOKEN)
        {
            buf[i].BufferType = SECBUFFER_TOKEN;
            buf[i].cbBuffer   = 16;
            buf[i].pvBuffer   = sig;
        }
        else
        {
            buf[i].BufferType = msg->pBuffers[i].BufferType;
            buf[i].cbBuffer   = msg->pBuffers[i].cbBuffer;
            buf[i].pvBuffer   = msg->pBuffers[i].pvBuffer;
        }
    }

    if ((status = create_signature( ctx, flags, &desc, idx, SIGN_RECV, TRUE )) == SEC_E_OK)
    {
        if (memcmp( (char *)buf[idx].pvBuffer + 8, (char *)msg->pBuffers[idx].pvBuffer + 8, 8 ))
            status = SEC_E_MESSAGE_ALTERED;
    }

    free( buf );
    return status;
}

static NTSTATUS NTAPI ntlm_SpVerifySignature( LSA_SEC_HANDLE handle, SecBufferDesc *msg, ULONG msg_seq_no, ULONG *qop )
{
    struct ntlm_ctx *ctx = (struct ntlm_ctx *)handle;
    int idx;

    TRACE( "%#Ix, %p, %lu, %p\n", handle, msg, msg_seq_no, qop );
    if (msg_seq_no) FIXME( "ignoring message sequence number %lu\n", msg_seq_no );

    if (!handle) return SEC_E_INVALID_HANDLE;
    if (!msg || !msg->pBuffers || msg->cBuffers < 2 || (idx = get_buffer_index( msg, SECBUFFER_TOKEN )) == -1)
        return SEC_E_INVALID_TOKEN;
    if (msg->pBuffers[idx].cbBuffer < 16) return SEC_E_BUFFER_TOO_SMALL;

    return verify_signature( ctx, ctx->flags, msg, idx );
}

static NTSTATUS NTAPI ntlm_SpSealMessage( LSA_SEC_HANDLE handle, ULONG qop, SecBufferDesc *msg, ULONG msg_seq_no )
{
    int token_idx, data_idx;
    struct ntlm_ctx *ctx;

    TRACE( "%#Ix, %#lx, %p %lu\n", handle, qop, msg, msg_seq_no );
    if (qop) FIXME( "ignoring quality of protection %#lx\n", qop );
    if (msg_seq_no) FIXME( "ignoring message sequence number %lu\n", msg_seq_no );

    if (!handle) return SEC_E_INVALID_HANDLE;

    if (!msg || !msg->pBuffers || msg->cBuffers < 2 ||
        (token_idx = get_buffer_index( msg, SECBUFFER_TOKEN )) == -1 ||
        (data_idx = get_buffer_index( msg, SECBUFFER_DATA )) == -1) return SEC_E_INVALID_TOKEN;

    if (msg->pBuffers[token_idx].cbBuffer < 16) return SEC_E_BUFFER_TOO_SMALL;

    ctx = (struct ntlm_ctx *)handle;
    if (ctx->flags & FLAG_NEGOTIATE_NTLM2 && ctx->flags & FLAG_NEGOTIATE_SEAL)
    {
        create_signature( ctx, ctx->flags, msg, token_idx, SIGN_SEND, FALSE );

        arc4_process( &ctx->crypt.ntlm2.send_arc4info, msg->pBuffers[data_idx].pvBuffer,
                      msg->pBuffers[data_idx].cbBuffer );
        if (ctx->flags & FLAG_NEGOTIATE_KEY_EXCHANGE)
            arc4_process( &ctx->crypt.ntlm2.send_arc4info, (char *)msg->pBuffers[token_idx].pvBuffer + 4, 8 );
    }
    else
    {
        char *sig = msg->pBuffers[token_idx].pvBuffer;

        create_signature( ctx, ctx->flags | FLAG_NEGOTIATE_SIGN, msg, token_idx, SIGN_SEND, FALSE );

        arc4_process( &ctx->crypt.ntlm.arc4info, msg->pBuffers[data_idx].pvBuffer, msg->pBuffers[data_idx].cbBuffer );
        arc4_process( &ctx->crypt.ntlm.arc4info, sig + 4, 12 );

        if (ctx->flags & FLAG_NEGOTIATE_ALWAYS_SIGN || !ctx->flags) memset( sig + 4, 0, 4 );
    }

    return SEC_E_OK;
}

static NTSTATUS NTAPI ntlm_SpUnsealMessage( LSA_SEC_HANDLE handle, SecBufferDesc *msg, ULONG msg_seq_no, ULONG *qop )
{
    int token_idx, data_idx;
    struct ntlm_ctx *ctx;

    TRACE( "%#Ix, %p, %lu, %p\n", handle, msg, msg_seq_no, qop );
    if (msg_seq_no) FIXME( "ignoring message sequence number %lu\n", msg_seq_no );

    if (!handle) return SEC_E_INVALID_HANDLE;

    if (!msg || !msg->pBuffers || msg->cBuffers < 2 ||
        (token_idx = get_buffer_index( msg, SECBUFFER_TOKEN )) == -1 ||
        (data_idx = get_buffer_index( msg, SECBUFFER_DATA )) == -1) return SEC_E_INVALID_TOKEN;

    if (msg->pBuffers[token_idx].cbBuffer < 16) return SEC_E_BUFFER_TOO_SMALL;

    ctx = (struct ntlm_ctx *)handle;
    if (ctx->flags & FLAG_NEGOTIATE_NTLM2 && ctx->flags & FLAG_NEGOTIATE_SEAL)
        arc4_process( &ctx->crypt.ntlm2.recv_arc4info, msg->pBuffers[data_idx].pvBuffer,
                      msg->pBuffers[data_idx].cbBuffer );
    else
        arc4_process( &ctx->crypt.ntlm.arc4info, msg->pBuffers[data_idx].pvBuffer,
                      msg->pBuffers[data_idx].cbBuffer);

    /* make sure we use a session key for the signature check, SealMessage always does that,
       even in the dummy case */
    return verify_signature( ctx, ctx->flags | FLAG_NEGOTIATE_SIGN, msg, token_idx );
}

static SECPKG_USER_FUNCTION_TABLE ntlm_user_table =
{
    ntlm_SpInstanceInit,
    NULL, /* SpInitUserModeContext */
    ntlm_SpMakeSignature,
    ntlm_SpVerifySignature,
    ntlm_SpSealMessage,
    ntlm_SpUnsealMessage,
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
    }
    return TRUE;
}
