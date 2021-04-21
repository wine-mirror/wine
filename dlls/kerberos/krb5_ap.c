/*
 * Copyright 2017 Dmitry Timoshkov
 * Copyright 2017 George Popoff
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
#include "winnls.h"
#include "rpc.h"
#include "sspi.h"
#include "ntsecapi.h"
#include "ntsecpkg.h"
#include "winternl.h"
#include "wine/heap.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(kerberos);

static HINSTANCE instance;

const struct krb5_funcs *krb5_funcs = NULL;

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

static ULONG kerberos_package_id;
static LSA_DISPATCH_TABLE lsa_dispatch;

#ifdef SONAME_LIBKRB5

static void *libkrb5_handle;

#define MAKE_FUNCPTR(f) static typeof(f) * p_##f
MAKE_FUNCPTR(krb5_cc_close);
MAKE_FUNCPTR(krb5_cc_default);
MAKE_FUNCPTR(krb5_cc_end_seq_get);
MAKE_FUNCPTR(krb5_cc_initialize);
MAKE_FUNCPTR(krb5_cc_next_cred);
MAKE_FUNCPTR(krb5_cc_start_seq_get);
MAKE_FUNCPTR(krb5_cc_store_cred);
MAKE_FUNCPTR(krb5_cccol_cursor_free);
MAKE_FUNCPTR(krb5_cccol_cursor_new);
MAKE_FUNCPTR(krb5_cccol_cursor_next);
MAKE_FUNCPTR(krb5_decode_ticket);
MAKE_FUNCPTR(krb5_free_context);
MAKE_FUNCPTR(krb5_free_cred_contents);
MAKE_FUNCPTR(krb5_free_principal);
MAKE_FUNCPTR(krb5_free_ticket);
MAKE_FUNCPTR(krb5_free_unparsed_name);
MAKE_FUNCPTR(krb5_get_init_creds_opt_alloc);
MAKE_FUNCPTR(krb5_get_init_creds_opt_free);
MAKE_FUNCPTR(krb5_get_init_creds_opt_set_out_ccache);
MAKE_FUNCPTR(krb5_get_init_creds_password);
MAKE_FUNCPTR(krb5_init_context);
MAKE_FUNCPTR(krb5_is_config_principal);
MAKE_FUNCPTR(krb5_parse_name_flags);
MAKE_FUNCPTR(krb5_unparse_name_flags);
#undef MAKE_FUNCPTR

static void load_krb5(void)
{
    if (!(libkrb5_handle = dlopen(SONAME_LIBKRB5, RTLD_NOW)))
    {
        WARN("Failed to load %s, Kerberos support will be disabled\n", SONAME_LIBKRB5);
        return;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p_##f = dlsym(libkrb5_handle, #f))) \
    { \
        ERR("Failed to load %s\n", #f); \
        goto fail; \
    }

    LOAD_FUNCPTR(krb5_cc_close)
    LOAD_FUNCPTR(krb5_cc_default)
    LOAD_FUNCPTR(krb5_cc_end_seq_get)
    LOAD_FUNCPTR(krb5_cc_initialize)
    LOAD_FUNCPTR(krb5_cc_next_cred)
    LOAD_FUNCPTR(krb5_cc_start_seq_get)
    LOAD_FUNCPTR(krb5_cc_store_cred)
    LOAD_FUNCPTR(krb5_cccol_cursor_free)
    LOAD_FUNCPTR(krb5_cccol_cursor_new)
    LOAD_FUNCPTR(krb5_cccol_cursor_next)
    LOAD_FUNCPTR(krb5_decode_ticket)
    LOAD_FUNCPTR(krb5_free_context)
    LOAD_FUNCPTR(krb5_free_cred_contents)
    LOAD_FUNCPTR(krb5_free_principal)
    LOAD_FUNCPTR(krb5_free_ticket)
    LOAD_FUNCPTR(krb5_free_unparsed_name)
    LOAD_FUNCPTR(krb5_get_init_creds_opt_alloc)
    LOAD_FUNCPTR(krb5_get_init_creds_opt_free)
    LOAD_FUNCPTR(krb5_get_init_creds_opt_set_out_ccache)
    LOAD_FUNCPTR(krb5_get_init_creds_password)
    LOAD_FUNCPTR(krb5_init_context)
    LOAD_FUNCPTR(krb5_is_config_principal)
    LOAD_FUNCPTR(krb5_parse_name_flags)
    LOAD_FUNCPTR(krb5_unparse_name_flags)
#undef LOAD_FUNCPTR

    return;

fail:
    dlclose(libkrb5_handle);
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

#ifdef SONAME_LIBKRB5

struct ticket_info
{
    ULONG count, allocated;
    KERB_TICKET_CACHE_INFO *info;
};

static NTSTATUS krb5_error_to_status(krb5_error_code error)
{
    switch (error)
    {
    case 0: return STATUS_SUCCESS;
    default:
        /* FIXME */
        return STATUS_UNSUCCESSFUL;
    }
}

static void free_ticket_info(struct ticket_info *info)
{
    ULONG i;

    for (i = 0; i < info->count; i++)
    {
        heap_free(info->info[i].RealmName.Buffer);
        heap_free(info->info[i].ServerName.Buffer);
    }

    heap_free(info->info);
}

static WCHAR *utf8_to_wstr(const char *utf8)
{
    int len;
    WCHAR *wstr;

    len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    wstr = heap_alloc(len * sizeof(WCHAR));
    if (wstr)
        MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr, len);

    return wstr;
}

static NTSTATUS copy_tickets_from_cache(krb5_context context, krb5_ccache cache, struct ticket_info *info)
{
    NTSTATUS status;
    krb5_cc_cursor cursor;
    krb5_error_code error;
    krb5_creds credentials;
    krb5_ticket *ticket;
    char *name_with_realm, *name_without_realm, *realm_name;
    WCHAR *realm_nameW, *name_without_realmW;

    error = p_krb5_cc_start_seq_get(context, cache, &cursor);
    if (error) return krb5_error_to_status(error);

    for (;;)
    {
        error = p_krb5_cc_next_cred(context, cache, &cursor, &credentials);
        if (error)
        {
            if (error == KRB5_CC_END)
                status = STATUS_SUCCESS;
            else
                status = krb5_error_to_status(error);
            break;
        }

        if (p_krb5_is_config_principal(context, credentials.server))
        {
            p_krb5_free_cred_contents(context, &credentials);
            continue;
        }

        if (info->count == info->allocated)
        {
            KERB_TICKET_CACHE_INFO *new_info;
            ULONG new_allocated;

            if (info->allocated)
            {
                new_allocated = info->allocated * 2;
                new_info = heap_realloc(info->info, sizeof(*new_info) * new_allocated);
            }
            else
            {
                new_allocated = 16;
                new_info = heap_alloc(sizeof(*new_info) * new_allocated);
            }
            if (!new_info)
            {
                p_krb5_free_cred_contents(context, &credentials);
                status = STATUS_NO_MEMORY;
                break;
            }

            info->info = new_info;
            info->allocated = new_allocated;
        }

        error = p_krb5_unparse_name_flags(context, credentials.server, 0, &name_with_realm);
        if (error)
        {
            p_krb5_free_cred_contents(context, &credentials);
            status = krb5_error_to_status(error);
            break;
        }

        TRACE("name_with_realm: %s\n", debugstr_a(name_with_realm));

        error = p_krb5_unparse_name_flags(context, credentials.server,
            KRB5_PRINCIPAL_UNPARSE_NO_REALM, &name_without_realm);
        if (error)
        {
            p_krb5_free_unparsed_name(context, name_with_realm);
            p_krb5_free_cred_contents(context, &credentials);
            status = krb5_error_to_status(error);
            break;
        }

        TRACE("name_without_realm: %s\n", debugstr_a(name_without_realm));

        name_without_realmW = utf8_to_wstr(name_without_realm);
        RtlInitUnicodeString(&info->info[info->count].ServerName, name_without_realmW);

        realm_name = strchr(name_with_realm, '@');
        if (!realm_name)
        {
            ERR("wrong name with realm %s\n", debugstr_a(name_with_realm));
            realm_name = name_with_realm;
        }
        else
            realm_name++;

        /* realm_name - now contains only realm! */

        realm_nameW = utf8_to_wstr(realm_name);
        RtlInitUnicodeString(&info->info[info->count].RealmName, realm_nameW);

        if (!credentials.times.starttime)
            credentials.times.starttime = credentials.times.authtime;

        /* TODO: if krb5_is_config_principal = true */
        RtlSecondsSince1970ToTime(credentials.times.starttime, &info->info[info->count].StartTime);
        RtlSecondsSince1970ToTime(credentials.times.endtime, &info->info[info->count].EndTime);
        RtlSecondsSince1970ToTime(credentials.times.renew_till, &info->info[info->count].RenewTime);

        info->info[info->count].TicketFlags = credentials.ticket_flags;

        error = p_krb5_decode_ticket(&credentials.ticket, &ticket);

        p_krb5_free_unparsed_name(context, name_with_realm);
        p_krb5_free_unparsed_name(context, name_without_realm);
        p_krb5_free_cred_contents(context, &credentials);

        if (error)
        {
            status = krb5_error_to_status(error);
            break;
        }

        info->info[info->count].EncryptionType = ticket->enc_part.enctype;

        p_krb5_free_ticket(context, ticket);

        info->count++;
    }

    p_krb5_cc_end_seq_get(context, cache, &cursor);

    return status;
}

static inline void init_client_us(UNICODE_STRING *dst, void *client_ws, const UNICODE_STRING *src)
{
    dst->Buffer = client_ws;
    dst->Length = src->Length;
    dst->MaximumLength = src->MaximumLength;
}

static NTSTATUS copy_to_client(PLSA_CLIENT_REQUEST lsa_req, struct ticket_info *info, void **out, ULONG *out_size)
{
    NTSTATUS status;
    ULONG i;
    SIZE_T size, client_str_off;
    char *client_resp, *client_ticket, *client_str;
    KERB_QUERY_TKT_CACHE_RESPONSE resp;

    size = sizeof(KERB_QUERY_TKT_CACHE_RESPONSE);
    if (info->count != 0)
        size += (info->count - 1) * sizeof(KERB_TICKET_CACHE_INFO);

    client_str_off = size;

    for (i = 0; i < info->count; i++)
    {
        size += info->info[i].RealmName.MaximumLength;
        size += info->info[i].ServerName.MaximumLength;
    }

    status = lsa_dispatch.AllocateClientBuffer(lsa_req, size, (void **)&client_resp);
    if (status != STATUS_SUCCESS) return status;

    resp.MessageType = KerbQueryTicketCacheMessage;
    resp.CountOfTickets = info->count;
    size = FIELD_OFFSET(KERB_QUERY_TKT_CACHE_RESPONSE, Tickets);
    status = lsa_dispatch.CopyToClientBuffer(lsa_req, size, client_resp, &resp);
    if (status != STATUS_SUCCESS) goto fail;

    if (!info->count)
    {
        *out = client_resp;
        *out_size = sizeof(resp);
        return STATUS_SUCCESS;
    }

    *out_size = size;

    client_ticket = client_resp + size;
    client_str = client_resp + client_str_off;

    for (i = 0; i < info->count; i++)
    {
        KERB_TICKET_CACHE_INFO ticket;

        ticket = info->info[i];

        init_client_us(&ticket.RealmName, client_str, &info->info[i].RealmName);

        size = info->info[i].RealmName.MaximumLength;
        status = lsa_dispatch.CopyToClientBuffer(lsa_req, size, client_str, info->info[i].RealmName.Buffer);
        if (status != STATUS_SUCCESS) goto fail;
        client_str += size;
        *out_size += size;

        init_client_us(&ticket.ServerName, client_str, &info->info[i].ServerName);

        size = info->info[i].ServerName.MaximumLength;
        status = lsa_dispatch.CopyToClientBuffer(lsa_req, size, client_str, info->info[i].ServerName.Buffer);
        if (status != STATUS_SUCCESS) goto fail;
        client_str += size;
        *out_size += size;

        status = lsa_dispatch.CopyToClientBuffer(lsa_req, sizeof(ticket), client_ticket, &ticket);
        if (status != STATUS_SUCCESS) goto fail;

        client_ticket += sizeof(ticket);
        *out_size += sizeof(ticket);
    }

    *out = client_resp;
    return STATUS_SUCCESS;

fail:
    lsa_dispatch.FreeClientBuffer(lsa_req, client_resp);
    return status;
}

static NTSTATUS query_ticket_cache(PLSA_CLIENT_REQUEST lsa_req, void *in, ULONG in_len, void **out, ULONG *out_len)
{
    NTSTATUS status;
    KERB_QUERY_TKT_CACHE_REQUEST *query;
    struct ticket_info info;
    krb5_error_code error;
    krb5_context context = NULL;
    krb5_cccol_cursor cursor = NULL;
    krb5_ccache cache;

    if (!in || in_len != sizeof(KERB_QUERY_TKT_CACHE_REQUEST) || !out || !out_len)
        return STATUS_INVALID_PARAMETER;

    query = (KERB_QUERY_TKT_CACHE_REQUEST *)in;

    if (query->LogonId.HighPart != 0 || query->LogonId.LowPart != 0)
        return STATUS_ACCESS_DENIED;

    info.count = 0;
    info.allocated = 0;
    info.info = NULL;

    error = p_krb5_init_context(&context);
    if (error)
    {
        status = krb5_error_to_status(error);
        goto done;
    }

    error = p_krb5_cccol_cursor_new(context, &cursor);
    if (error)
    {
        status = krb5_error_to_status(error);
        goto done;
    }

    for (;;)
    {
        error = p_krb5_cccol_cursor_next(context, cursor, &cache);
        if (error)
        {
            status = krb5_error_to_status(error);
            goto done;
        }
        if (!cache) break;

        status = copy_tickets_from_cache(context, cache, &info);

        p_krb5_cc_close(context, cache);

        if (status != STATUS_SUCCESS)
            goto done;
    }

    status = copy_to_client(lsa_req, &info, out, out_len);

done:
    if (cursor)
        p_krb5_cccol_cursor_free(context, &cursor);

    if (context)
        p_krb5_free_context(context);

    free_ticket_info(&info);

    return status;
}

#else /* SONAME_LIBKRB5 */

static NTSTATUS query_ticket_cache(PLSA_CLIENT_REQUEST lsa_req, void *in, ULONG in_len, void **out, ULONG *out_len)
{
    FIXME("%p,%p,%u,%p,%p: stub\n", lsa_req, in, in_len, out, out_len);
    return STATUS_NOT_IMPLEMENTED;
}

#endif /* SONAME_LIBKRB5 */

static NTSTATUS NTAPI kerberos_LsaApCallPackageUntrusted(PLSA_CLIENT_REQUEST request,
    PVOID in_buffer, PVOID client_buffer_base, ULONG in_buffer_length,
    PVOID *out_buffer, PULONG out_buffer_length, PNTSTATUS status)
{
    KERB_PROTOCOL_MESSAGE_TYPE msg;

    TRACE("%p,%p,%p,%u,%p,%p,%p\n", request, in_buffer, client_buffer_base,
        in_buffer_length, out_buffer, out_buffer_length, status);

    if (!in_buffer || in_buffer_length < sizeof(msg))
        return STATUS_INVALID_PARAMETER;

    msg = *(KERB_PROTOCOL_MESSAGE_TYPE *)in_buffer;

    switch (msg)
    {
    case KerbQueryTicketCacheMessage:
        *status = query_ticket_cache(request, in_buffer, in_buffer_length, out_buffer, out_buffer_length);
        break;

    case KerbRetrieveTicketMessage:
        FIXME("KerbRetrieveTicketMessage stub\n");
        *status = STATUS_NOT_IMPLEMENTED;
        break;

    case KerbPurgeTicketCacheMessage:
        FIXME("KerbPurgeTicketCacheMessage stub\n");
        *status = STATUS_NOT_IMPLEMENTED;
        break;

    default: /* All other requests should call LsaApCallPackage */
        WARN("%u => access denied\n", msg);
        *status = STATUS_ACCESS_DENIED;
        break;
    }

    return *status;
}

static NTSTATUS NTAPI kerberos_SpGetInfo(SecPkgInfoW *info)
{
    TRACE("%p\n", info);

    /* LSA will make a copy before forwarding the structure, so
     * it's safe to put pointers to dynamic or constant data there.
     */
    *info = infoW;

    return STATUS_SUCCESS;
}

static char *get_str_unixcp( const UNICODE_STRING *str )
{
    char *ret;
    int len = WideCharToMultiByte( CP_UNIXCP, 0, str->Buffer, str->Length / sizeof(WCHAR), NULL, 0, NULL, NULL );
    if (!(ret = heap_alloc( len + 1 ))) return NULL;
    WideCharToMultiByte( CP_UNIXCP, 0, str->Buffer, str->Length / sizeof(WCHAR), ret, len, NULL, NULL );
    ret[len] = 0;
    return ret;
}

static char *get_username_unixcp( const WCHAR *user, ULONG user_len, const WCHAR *domain, ULONG domain_len )
{
    int len_user, len_domain;
    char *ret;

    len_user = WideCharToMultiByte( CP_UNIXCP, 0, user, user_len, NULL, 0, NULL, NULL );
    len_domain = WideCharToMultiByte( CP_UNIXCP, 0, domain, domain_len, NULL, 0, NULL, NULL );
    if (!(ret = heap_alloc( len_user + len_domain + 2 ))) return NULL;

    WideCharToMultiByte( CP_UNIXCP, 0, user, user_len, ret, len_user, NULL, NULL );
    ret[len_user] = '@';
    WideCharToMultiByte( CP_UNIXCP, 0, domain, domain_len, ret + len_user + 1, len_domain, NULL, NULL );
    ret[len_user + len_domain + 1] = 0;
    return ret;
}

static char *get_password_unixcp( const WCHAR *passwd, ULONG passwd_len )
{
    int len;
    char *ret;

    len = WideCharToMultiByte( CP_UNIXCP, WC_NO_BEST_FIT_CHARS, passwd, passwd_len, NULL, 0, NULL, NULL );
    if (!(ret = heap_alloc( len + 1 ))) return NULL;
    WideCharToMultiByte( CP_UNIXCP, 0, passwd, passwd_len, ret, len, NULL, NULL );
    ret[len] = 0;
    return ret;
}

static NTSTATUS NTAPI kerberos_SpAcquireCredentialsHandle(
    UNICODE_STRING *principal_us, ULONG credential_use, LUID *logon_id, void *auth_data,
    void *get_key_fn, void *get_key_arg, LSA_SEC_HANDLE *credential, TimeStamp *expiry )
{
    char *principal = NULL, *username = NULL,  *password = NULL;
    SEC_WINNT_AUTH_IDENTITY_W *id = auth_data;
    NTSTATUS status = SEC_E_INSUFFICIENT_MEMORY;

    TRACE( "(%s 0x%08x %p %p %p %p %p %p)\n", debugstr_us(principal_us), credential_use,
           logon_id, auth_data, get_key_fn, get_key_arg, credential, expiry );

    if (principal_us && !(principal = get_str_unixcp( principal_us ))) return SEC_E_INSUFFICIENT_MEMORY;
    if (id)
    {
        if (id->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI)
        {
            FIXME( "ANSI identity not supported\n" );
            status = SEC_E_UNSUPPORTED_FUNCTION;
            goto done;
        }
        if (!(username = get_username_unixcp( id->User, id->UserLength, id->Domain, id->DomainLength ))) goto done;
        if (!(password = get_password_unixcp( id->Password, id->PasswordLength ))) goto done;
    }

    status = krb5_funcs->acquire_credentials_handle( principal, credential_use, username, password, credential,
                                                     expiry );
done:
    heap_free( principal );
    heap_free( username );
    heap_free( password );
    return status;
}

static NTSTATUS NTAPI kerberos_SpFreeCredentialsHandle( LSA_SEC_HANDLE credential )
{
    TRACE( "(%lx)\n", credential );
    if (!credential) return SEC_E_INVALID_HANDLE;
    return krb5_funcs->free_credentials_handle( credential );
}

static NTSTATUS NTAPI kerberos_SpInitLsaModeContext( LSA_SEC_HANDLE credential, LSA_SEC_HANDLE context,
    UNICODE_STRING *target_name, ULONG context_req, ULONG target_data_rep, SecBufferDesc *input,
    LSA_SEC_HANDLE *new_context, SecBufferDesc *output, ULONG *context_attr, TimeStamp *expiry,
    BOOLEAN *mapped_context, SecBuffer *context_data )
{
    static const ULONG supported = ISC_REQ_CONFIDENTIALITY | ISC_REQ_INTEGRITY | ISC_REQ_SEQUENCE_DETECT |
                                   ISC_REQ_REPLAY_DETECT | ISC_REQ_MUTUAL_AUTH | ISC_REQ_USE_DCE_STYLE |
                                   ISC_REQ_IDENTIFY | ISC_REQ_CONNECTION;
    char *target = NULL;
    NTSTATUS status;

    TRACE( "(%lx %lx %s 0x%08x %u %p %p %p %p %p %p %p)\n", credential, context, debugstr_us(target_name),
           context_req, target_data_rep, input, new_context, output, context_attr, expiry,
           mapped_context, context_data );
    if (context_req & ~supported) FIXME( "flags 0x%08x not supported\n", context_req & ~supported );

    if (!context && !input && !credential) return SEC_E_INVALID_HANDLE;
    if (target_name && !(target = get_str_unixcp( target_name ))) return SEC_E_INSUFFICIENT_MEMORY;

    status = krb5_funcs->initialize_context( credential, context, target, context_req, input, new_context, output,
                                             context_attr, expiry );
    if (!status) *mapped_context = TRUE;
    /* FIXME: initialize context_data */
    heap_free( target );
    return status;
}

static NTSTATUS NTAPI kerberos_SpAcceptLsaModeContext( LSA_SEC_HANDLE credential, LSA_SEC_HANDLE context,
    SecBufferDesc *input, ULONG context_req, ULONG target_data_rep, LSA_SEC_HANDLE *new_context,
    SecBufferDesc *output, ULONG *context_attr, TimeStamp *expiry, BOOLEAN *mapped_context, SecBuffer *context_data )
{
    NTSTATUS status;

    TRACE( "(%lx %lx 0x%08x %u %p %p %p %p %p %p %p)\n", credential, context, context_req, target_data_rep, input,
           new_context, output, context_attr, expiry, mapped_context, context_data );
    if (context_req) FIXME( "ignoring flags 0x%08x\n", context_req );

    if (!context && !input && !credential) return SEC_E_INVALID_HANDLE;

    status = krb5_funcs->accept_context( credential, context, input, new_context, output, context_attr, expiry );
    if (!status) *mapped_context = TRUE;
    /* FIXME: initialize context_data */
    return status;
}

static NTSTATUS NTAPI kerberos_SpDeleteContext( LSA_SEC_HANDLE context )
{
    TRACE( "(%lx)\n", context );
    if (!context) return SEC_E_INVALID_HANDLE;
    return krb5_funcs->delete_context( context );
}

static SecPkgInfoW *build_package_info( const SecPkgInfoW *info )
{
    SecPkgInfoW *ret;
    DWORD size_name = (strlenW(info->Name) + 1) * sizeof(WCHAR);
    DWORD size_comment = (strlenW(info->Comment) + 1) * sizeof(WCHAR);

    if (!(ret = heap_alloc( sizeof(*ret) + size_name + size_comment ))) return NULL;
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

static NTSTATUS NTAPI kerberos_SpQueryContextAttributes( LSA_SEC_HANDLE context, ULONG attribute, void *buffer )
{
    TRACE( "(%lx %u %p)\n", context, attribute, buffer );

    if (!context) return SEC_E_INVALID_HANDLE;

    switch (attribute)
    {
#define X(x) case (x) : FIXME(#x" stub\n"); break
    X(SECPKG_ATTR_ACCESS_TOKEN);
    X(SECPKG_ATTR_AUTHORITY);
    X(SECPKG_ATTR_DCE_INFO);
    X(SECPKG_ATTR_KEY_INFO);
    X(SECPKG_ATTR_LIFESPAN);
    X(SECPKG_ATTR_NAMES);
    X(SECPKG_ATTR_NATIVE_NAMES);
    X(SECPKG_ATTR_PACKAGE_INFO);
    X(SECPKG_ATTR_PASSWORD_EXPIRY);
    X(SECPKG_ATTR_SESSION_KEY);
    X(SECPKG_ATTR_STREAM_SIZES);
    X(SECPKG_ATTR_TARGET_INFORMATION);
    case SECPKG_ATTR_SIZES:
    {
        return krb5_funcs->query_context_attributes( context, attribute, buffer );
    }
    case SECPKG_ATTR_NEGOTIATION_INFO:
    {
        SecPkgContext_NegotiationInfoW *info = (SecPkgContext_NegotiationInfoW *)buffer;
        if (!(info->PackageInfo = build_package_info( &infoW ))) return SEC_E_INSUFFICIENT_MEMORY;
        info->NegotiationState = SECPKG_NEGOTIATION_COMPLETE;
        return SEC_E_OK;
    }
#undef X
    default:
        FIXME( "unknown attribute %u\n", attribute );
        break;
    }

    return SEC_E_UNSUPPORTED_FUNCTION;
}

static NTSTATUS NTAPI kerberos_SpInitialize(ULONG_PTR package_id, SECPKG_PARAMETERS *params,
    LSA_SECPKG_FUNCTION_TABLE *lsa_function_table)
{
    TRACE("%lu,%p,%p\n", package_id, params, lsa_function_table);

    if (!krb5_funcs && __wine_init_unix_lib( instance, DLL_PROCESS_ATTACH, NULL, &krb5_funcs ))
    {
        WARN( "no Kerberos support\n" );
        return STATUS_UNSUCCESSFUL;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI kerberos_SpShutdown(void)
{
    TRACE("\n");
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
    kerberos_SpQueryContextAttributes,
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

static NTSTATUS NTAPI kerberos_SpInstanceInit(ULONG version, SECPKG_DLL_FUNCTIONS *dll_function_table, void **user_functions)
{
    TRACE("%#x,%p,%p\n", version, dll_function_table, user_functions);
    return STATUS_SUCCESS;
}

static NTSTATUS SEC_ENTRY kerberos_SpMakeSignature( LSA_SEC_HANDLE context, ULONG quality_of_protection,
    SecBufferDesc *message, ULONG message_seq_no )
{
    TRACE( "(%lx 0x%08x %p %u)\n", context, quality_of_protection, message, message_seq_no );
    if (quality_of_protection) FIXME( "ignoring quality_of_protection 0x%08x\n", quality_of_protection );
    if (message_seq_no) FIXME( "ignoring message_seq_no %u\n", message_seq_no );

    if (!context) return SEC_E_INVALID_HANDLE;
    return krb5_funcs->make_signature( context, message );
}

static NTSTATUS NTAPI kerberos_SpVerifySignature( LSA_SEC_HANDLE context, SecBufferDesc *message,
    ULONG message_seq_no, ULONG *quality_of_protection )
{
    TRACE( "(%lx %p %u %p)\n", context, message, message_seq_no, quality_of_protection );
    if (message_seq_no) FIXME( "ignoring message_seq_no %u\n", message_seq_no );

    if (!context) return SEC_E_INVALID_HANDLE;
    return krb5_funcs->verify_signature( context, message, quality_of_protection );
}

static NTSTATUS NTAPI kerberos_SpSealMessage( LSA_SEC_HANDLE context, ULONG quality_of_protection,
    SecBufferDesc *message, ULONG message_seq_no )
{
    TRACE( "(%lx 0x%08x %p %u)\n", context, quality_of_protection, message, message_seq_no );
    if (message_seq_no) FIXME( "ignoring message_seq_no %u\n", message_seq_no );

    if (!context) return SEC_E_INVALID_HANDLE;
    return krb5_funcs->seal_message( context, message, quality_of_protection );
}

static NTSTATUS NTAPI kerberos_SpUnsealMessage( LSA_SEC_HANDLE context, SecBufferDesc *message,
    ULONG message_seq_no, ULONG *quality_of_protection )
{
    TRACE( "(%lx %p %u %p)\n", context, message, message_seq_no, quality_of_protection );
    if (message_seq_no) FIXME( "ignoring message_seq_no %u\n", message_seq_no );

    if (!context) return SEC_E_INVALID_HANDLE;
    return krb5_funcs->unseal_message( context, message, quality_of_protection );
}

static SECPKG_USER_FUNCTION_TABLE kerberos_user_table =
{
    kerberos_SpInstanceInit,
    NULL, /* SpInitUserModeContext */
    kerberos_SpMakeSignature,
    kerberos_SpVerifySignature,
    kerberos_SpSealMessage,
    kerberos_SpUnsealMessage,
    NULL, /* SpGetContextToken */
    NULL, /* SpQueryContextAttributes */
    NULL, /* SpCompleteAuthToken */
    NULL, /* SpDeleteContext */
    NULL, /* SpFormatCredentialsFn */
    NULL, /* SpMarshallSupplementalCreds */
    NULL, /* SpExportSecurityContext */
    NULL  /* SpImportSecurityContext */
};

NTSTATUS NTAPI SpUserModeInitialize(ULONG lsa_version, PULONG package_version,
    PSECPKG_USER_FUNCTION_TABLE *table, PULONG table_count)
{
    TRACE("%#x,%p,%p,%p\n", lsa_version, package_version, table, table_count);

    *package_version = SECPKG_INTERFACE_VERSION;
    *table = &kerberos_user_table;
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
