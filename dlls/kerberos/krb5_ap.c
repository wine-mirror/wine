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

#ifdef SONAME_LIBGSSAPI_KRB5

WINE_DECLARE_DEBUG_CHANNEL(winediag);
static void *libgssapi_krb5_handle;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(gss_accept_sec_context);
MAKE_FUNCPTR(gss_acquire_cred);
MAKE_FUNCPTR(gss_delete_sec_context);
MAKE_FUNCPTR(gss_display_status);
MAKE_FUNCPTR(gss_get_mic);
MAKE_FUNCPTR(gss_import_name);
MAKE_FUNCPTR(gss_init_sec_context);
MAKE_FUNCPTR(gss_inquire_context);
MAKE_FUNCPTR(gss_release_buffer);
MAKE_FUNCPTR(gss_release_cred);
MAKE_FUNCPTR(gss_release_iov_buffer);
MAKE_FUNCPTR(gss_release_name);
MAKE_FUNCPTR(gss_unwrap);
MAKE_FUNCPTR(gss_unwrap_iov);
MAKE_FUNCPTR(gss_verify_mic);
MAKE_FUNCPTR(gss_wrap);
MAKE_FUNCPTR(gss_wrap_iov);
#undef MAKE_FUNCPTR

static BOOL load_gssapi_krb5(void)
{
    if (!(libgssapi_krb5_handle = dlopen( SONAME_LIBGSSAPI_KRB5, RTLD_NOW )))
    {
        ERR_(winediag)( "Failed to load libgssapi_krb5, Kerberos SSP support will not be available.\n" );
        return FALSE;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p##f = dlsym( libgssapi_krb5_handle, #f ))) \
    { \
        ERR( "Failed to load %s\n", #f ); \
        goto fail; \
    }

    LOAD_FUNCPTR(gss_accept_sec_context)
    LOAD_FUNCPTR(gss_acquire_cred)
    LOAD_FUNCPTR(gss_delete_sec_context)
    LOAD_FUNCPTR(gss_display_status)
    LOAD_FUNCPTR(gss_get_mic)
    LOAD_FUNCPTR(gss_import_name)
    LOAD_FUNCPTR(gss_init_sec_context)
    LOAD_FUNCPTR(gss_inquire_context)
    LOAD_FUNCPTR(gss_release_buffer)
    LOAD_FUNCPTR(gss_release_cred)
    LOAD_FUNCPTR(gss_release_iov_buffer)
    LOAD_FUNCPTR(gss_release_name)
    LOAD_FUNCPTR(gss_unwrap)
    LOAD_FUNCPTR(gss_unwrap_iov)
    LOAD_FUNCPTR(gss_verify_mic)
    LOAD_FUNCPTR(gss_wrap)
    LOAD_FUNCPTR(gss_wrap_iov)
#undef LOAD_FUNCPTR

    return TRUE;

fail:
    dlclose( libgssapi_krb5_handle );
    libgssapi_krb5_handle = NULL;
    return FALSE;
}

static void unload_gssapi_krb5(void)
{
    dlclose( libgssapi_krb5_handle );
    libgssapi_krb5_handle = NULL;
}

static inline gss_cred_id_t credhandle_sspi_to_gss( LSA_SEC_HANDLE cred )
{
    if (!cred) return GSS_C_NO_CREDENTIAL;
    return (gss_cred_id_t)cred;
}

static inline void credhandle_gss_to_sspi( gss_cred_id_t handle, LSA_SEC_HANDLE *cred )
{
    *cred = (LSA_SEC_HANDLE)handle;
}

static inline gss_ctx_id_t ctxthandle_sspi_to_gss( LSA_SEC_HANDLE ctxt )
{
    if (!ctxt) return GSS_C_NO_CONTEXT;
    return (gss_ctx_id_t)ctxt;
}

static inline void ctxthandle_gss_to_sspi( gss_ctx_id_t handle, LSA_SEC_HANDLE *ctxt )
{
    *ctxt = (LSA_SEC_HANDLE)handle;
}

static NTSTATUS status_gss_to_sspi( OM_uint32 status )
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
    case GSS_S_FAILURE:              return SEC_E_INTERNAL_ERROR;

    default:
        FIXME( "couldn't convert status 0x%08x to NTSTATUS\n", status );
        return SEC_E_INTERNAL_ERROR;
    }
}

static void trace_gss_status_ex( OM_uint32 code, int type )
{
    OM_uint32 ret, minor_status;
    gss_buffer_desc buf;
    OM_uint32 message_context = 0;

    for (;;)
    {
        ret = pgss_display_status( &minor_status, code, type, GSS_C_NULL_OID, &message_context, &buf );
        if (GSS_ERROR(ret))
        {
            TRACE( "gss_display_status(0x%08x,%d) returned %08x minor status %08x\n",
                   code, type, ret, minor_status );
            return;
        }
        TRACE( "GSS-API error: 0x%08x: %s\n", code, debugstr_an(buf.value, buf.length) );
        pgss_release_buffer( &minor_status, &buf );

        if (!message_context) return;
    }
}

static void trace_gss_status( OM_uint32 major_status, OM_uint32 minor_status )
{
    if (TRACE_ON(kerberos))
    {
        trace_gss_status_ex( major_status, GSS_C_GSS_CODE );
        trace_gss_status_ex( minor_status, GSS_C_MECH_CODE );
    }
}

static void expirytime_gss_to_sspi( OM_uint32 expirytime, TimeStamp *timestamp )
{
    FILETIME filetime;
    ULARGE_INTEGER tmp;

    GetSystemTimeAsFileTime( &filetime );
    FileTimeToLocalFileTime( &filetime, &filetime );
    tmp.QuadPart = ((ULONGLONG)filetime.dwLowDateTime | (ULONGLONG)filetime.dwHighDateTime << 32) + expirytime;
    timestamp->LowPart  = tmp.QuadPart;
    timestamp->HighPart = tmp.QuadPart >> 32;
}

static NTSTATUS name_sspi_to_gss( const UNICODE_STRING *name_str, gss_name_t *name )
{
    OM_uint32 ret, minor_status;
    gss_OID type = GSS_C_NO_OID; /* FIXME: detect the appropriate value for this ourselves? */
    gss_buffer_desc buf;

    buf.length = WideCharToMultiByte( CP_UNIXCP, 0, name_str->Buffer, name_str->Length / sizeof(WCHAR), NULL, 0, NULL, NULL );
    if (!(buf.value = heap_alloc( buf.length ))) return SEC_E_INSUFFICIENT_MEMORY;
    WideCharToMultiByte( CP_UNIXCP, 0, name_str->Buffer, name_str->Length / sizeof(WCHAR), buf.value, buf.length, NULL, NULL );

    ret = pgss_import_name( &minor_status, &buf, type, name );
    TRACE( "gss_import_name returned %08x minor status %08x\n", ret, minor_status );
    if (GSS_ERROR(ret)) trace_gss_status( ret, minor_status );

    heap_free( buf.value );
    return status_gss_to_sspi( ret );
}

static ULONG flags_isc_req_to_gss( ULONG flags )
{
    ULONG ret = 0;
    if (flags & ISC_REQ_DELEGATE)        ret |= GSS_C_DELEG_FLAG;
    if (flags & ISC_REQ_MUTUAL_AUTH)     ret |= GSS_C_MUTUAL_FLAG;
    if (flags & ISC_REQ_REPLAY_DETECT)   ret |= GSS_C_REPLAY_FLAG;
    if (flags & ISC_REQ_SEQUENCE_DETECT) ret |= GSS_C_SEQUENCE_FLAG;
    if (flags & ISC_REQ_CONFIDENTIALITY) ret |= GSS_C_CONF_FLAG;
    if (flags & ISC_REQ_INTEGRITY)       ret |= GSS_C_INTEG_FLAG;
    if (flags & ISC_REQ_NULL_SESSION)    ret |= GSS_C_ANON_FLAG;
    if (flags & ISC_REQ_USE_DCE_STYLE)   ret |= GSS_C_DCE_STYLE;
    if (flags & ISC_REQ_IDENTIFY)        ret |= GSS_C_IDENTIFY_FLAG;
    return ret;
}

static ULONG flags_gss_to_isc_ret( ULONG flags )
{
    ULONG ret = 0;
    if (flags & GSS_C_DELEG_FLAG)    ret |= ISC_RET_DELEGATE;
    if (flags & GSS_C_MUTUAL_FLAG)   ret |= ISC_RET_MUTUAL_AUTH;
    if (flags & GSS_C_REPLAY_FLAG)   ret |= ISC_RET_REPLAY_DETECT;
    if (flags & GSS_C_SEQUENCE_FLAG) ret |= ISC_RET_SEQUENCE_DETECT;
    if (flags & GSS_C_CONF_FLAG)     ret |= ISC_RET_CONFIDENTIALITY;
    if (flags & GSS_C_INTEG_FLAG)    ret |= ISC_RET_INTEGRITY;
    if (flags & GSS_C_ANON_FLAG)     ret |= ISC_RET_NULL_SESSION;
    if (flags & GSS_C_DCE_STYLE)     ret |= ISC_RET_USED_DCE_STYLE;
    if (flags & GSS_C_IDENTIFY_FLAG) ret |= ISC_RET_IDENTIFY;
    return ret;
}

static ULONG flags_gss_to_asc_ret( ULONG flags )
{
    ULONG ret = 0;
    if (flags & GSS_C_DELEG_FLAG)    ret |= ASC_RET_DELEGATE;
    if (flags & GSS_C_MUTUAL_FLAG)   ret |= ASC_RET_MUTUAL_AUTH;
    if (flags & GSS_C_REPLAY_FLAG)   ret |= ASC_RET_REPLAY_DETECT;
    if (flags & GSS_C_SEQUENCE_FLAG) ret |= ASC_RET_SEQUENCE_DETECT;
    if (flags & GSS_C_CONF_FLAG)     ret |= ASC_RET_CONFIDENTIALITY;
    if (flags & GSS_C_INTEG_FLAG)    ret |= ASC_RET_INTEGRITY;
    if (flags & GSS_C_ANON_FLAG)     ret |= ASC_RET_NULL_SESSION;
    if (flags & GSS_C_DCE_STYLE)     ret |= ASC_RET_USED_DCE_STYLE;
    if (flags & GSS_C_IDENTIFY_FLAG) ret |= ASC_RET_IDENTIFY;
    return ret;
}

static BOOL is_dce_style_context( gss_ctx_id_t ctxt_handle )
{
    OM_uint32 ret, minor_status, flags;
    ret = pgss_inquire_context( &minor_status, ctxt_handle, NULL, NULL, NULL, NULL, &flags, NULL, NULL );
    return (ret == GSS_S_COMPLETE && (flags & GSS_C_DCE_STYLE));
}

static int get_buffer_index( SecBufferDesc *desc, DWORD type )
{
    UINT i;
    if (!desc) return -1;
    for (i = 0; i < desc->cBuffers; i++)
    {
        if (desc->pBuffers[i].BufferType == type) return i;
    }
    return -1;
}

static char *get_user_at_domain( const WCHAR *user, ULONG user_len, const WCHAR *domain, ULONG domain_len )
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

static char *get_password( const WCHAR *passwd, ULONG passwd_len )
{
    int len;
    char *ret;

    len = WideCharToMultiByte( CP_UNIXCP, WC_NO_BEST_FIT_CHARS, passwd, passwd_len, NULL, 0, NULL, NULL );
    if (!(ret = heap_alloc( len + 1 ))) return NULL;
    WideCharToMultiByte( CP_UNIXCP, 0, passwd, passwd_len, ret, len, NULL, NULL );
    ret[len] = 0;
    return ret;
}

static NTSTATUS init_creds( const SEC_WINNT_AUTH_IDENTITY_W *id )
{
    char *user_at_domain, *password;
    krb5_context ctx;
    krb5_principal principal = NULL;
    krb5_get_init_creds_opt *options = NULL;
    krb5_ccache cache = NULL;
    krb5_creds creds;
    krb5_error_code err;

    if (!id) return STATUS_SUCCESS;
    if (id->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI)
    {
        FIXME( "ANSI identity not supported\n" );
        return SEC_E_UNSUPPORTED_FUNCTION;
    }
    if (!(user_at_domain = get_user_at_domain( id->User, id->UserLength, id->Domain, id->DomainLength )))
    {
        return SEC_E_INSUFFICIENT_MEMORY;
    }
    if (!(password = get_password( id->Password, id->PasswordLength )))
    {
        heap_free( user_at_domain );
        return SEC_E_INSUFFICIENT_MEMORY;
    }

    if ((err = p_krb5_init_context( &ctx )))
    {
        heap_free( password );
        heap_free( user_at_domain );
        return krb5_error_to_status( err );
    }
    if ((err = p_krb5_parse_name_flags( ctx, user_at_domain, 0, &principal ))) goto done;
    if ((err = p_krb5_cc_default( ctx, &cache ))) goto done;
    if ((err = p_krb5_get_init_creds_opt_alloc( ctx, &options ))) goto done;
    if ((err = p_krb5_get_init_creds_opt_set_out_ccache( ctx, options, cache ))) goto done;
    if ((err = p_krb5_get_init_creds_password( ctx, &creds, principal, password, 0, NULL, 0, NULL, 0 ))) goto done;
    if ((err = p_krb5_cc_initialize( ctx, cache, principal ))) goto done;
    if ((err = p_krb5_cc_store_cred( ctx, cache, &creds ))) goto done;

    TRACE( "success\n" );
    p_krb5_free_cred_contents( ctx, &creds );

done:
    if (cache) p_krb5_cc_close( ctx, cache );
    if (principal) p_krb5_free_principal( ctx, principal );
    if (options) p_krb5_get_init_creds_opt_free( ctx, options );
    p_krb5_free_context( ctx );
    heap_free( user_at_domain );
    heap_free( password );

    return krb5_error_to_status( err );
}

static NTSTATUS acquire_credentials_handle( UNICODE_STRING *principal_us, gss_cred_usage_t cred_usage,
    LSA_SEC_HANDLE *credential, TimeStamp *ts_expiry )
{
    OM_uint32 ret, minor_status, expiry_time;
    gss_name_t principal = GSS_C_NO_NAME;
    gss_cred_id_t cred_handle;
    NTSTATUS status;

    if (principal_us && ((status = name_sspi_to_gss( principal_us, &principal )) != SEC_E_OK)) return status;

    ret = pgss_acquire_cred( &minor_status, principal, GSS_C_INDEFINITE, GSS_C_NULL_OID_SET, cred_usage,
                              &cred_handle, NULL, &expiry_time );
    TRACE( "gss_acquire_cred returned %08x minor status %08x\n", ret, minor_status );
    if (GSS_ERROR(ret)) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE)
    {
        credhandle_gss_to_sspi( cred_handle, credential );
        expirytime_gss_to_sspi( expiry_time, ts_expiry );
    }

    if (principal != GSS_C_NO_NAME) pgss_release_name( &minor_status, &principal );

    return status_gss_to_sspi( ret );
}
#endif /* SONAME_LIBGSSAPI_KRB5 */

static NTSTATUS NTAPI kerberos_SpAcquireCredentialsHandle(
    UNICODE_STRING *principal_us, ULONG credential_use, LUID *logon_id, void *auth_data,
    void *get_key_fn, void *get_key_arg, LSA_SEC_HANDLE *credential, TimeStamp *ts_expiry )
{
#ifdef SONAME_LIBGSSAPI_KRB5
    gss_cred_usage_t cred_usage;
    NTSTATUS status;

    TRACE( "(%s 0x%08x %p %p %p %p %p %p)\n", debugstr_us(principal_us), credential_use,
           logon_id, auth_data, get_key_fn, get_key_arg, credential, ts_expiry );

    switch (credential_use)
    {
    case SECPKG_CRED_INBOUND:
        cred_usage = GSS_C_ACCEPT;
        break;

    case SECPKG_CRED_OUTBOUND:
        if ((status = init_creds( auth_data )) != STATUS_SUCCESS) return status;
        cred_usage = GSS_C_INITIATE;
        break;

    case SECPKG_CRED_BOTH:
        cred_usage = GSS_C_BOTH;
        break;

    default:
        return SEC_E_UNKNOWN_CREDENTIALS;
    }

    return acquire_credentials_handle( principal_us, cred_usage, credential, ts_expiry );
#else
    FIXME( "(%s 0x%08x %p %p %p %p %p %p)\n", debugstr_us(principal_us), credential_use,
           logon_id, auth_data, get_key_fn, get_key_arg, credential, ts_expiry );
    FIXME( "Wine was built without Kerberos support.\n" );
    return SEC_E_UNSUPPORTED_FUNCTION;
#endif
}

static NTSTATUS NTAPI kerberos_SpFreeCredentialsHandle( LSA_SEC_HANDLE credential )
{
#ifdef SONAME_LIBGSSAPI_KRB5
    OM_uint32 ret, minor_status;
    gss_cred_id_t cred_handle;

    TRACE( "(%lx)\n", credential );

    if (!credential) return SEC_E_INVALID_HANDLE;
    if (!(cred_handle = credhandle_sspi_to_gss( credential ))) return SEC_E_OK;

    ret = pgss_release_cred( &minor_status, &cred_handle );
    TRACE( "gss_release_cred returned %08x minor status %08x\n", ret, minor_status );
    if (GSS_ERROR(ret)) trace_gss_status( ret, minor_status );

    return status_gss_to_sspi( ret );
#else
    FIXME( "(%lx)\n", credential );
    return SEC_E_UNSUPPORTED_FUNCTION;
#endif
}

static NTSTATUS NTAPI kerberos_SpInitLsaModeContext( LSA_SEC_HANDLE credential, LSA_SEC_HANDLE context,
    UNICODE_STRING *target_name, ULONG context_req, ULONG target_data_rep, SecBufferDesc *input,
    LSA_SEC_HANDLE *new_context, SecBufferDesc *output, ULONG *context_attr, TimeStamp *ts_expiry,
    BOOLEAN *mapped_context, SecBuffer *context_data )
{
#ifdef SONAME_LIBGSSAPI_KRB5
    static const ULONG supported = ISC_REQ_CONFIDENTIALITY | ISC_REQ_INTEGRITY | ISC_REQ_SEQUENCE_DETECT |
                                   ISC_REQ_REPLAY_DETECT | ISC_REQ_MUTUAL_AUTH | ISC_REQ_USE_DCE_STYLE |
                                   ISC_REQ_IDENTIFY | ISC_REQ_CONNECTION;
    OM_uint32 ret, minor_status, ret_flags = 0, expiry_time, req_flags = flags_isc_req_to_gss( context_req );
    gss_cred_id_t cred_handle;
    gss_ctx_id_t ctxt_handle;
    gss_buffer_desc input_token, output_token;
    gss_name_t target = GSS_C_NO_NAME;
    NTSTATUS status;
    int idx;

    TRACE( "(%lx %lx %s 0x%08x %u %p %p %p %p %p %p %p)\n", credential, context, debugstr_us(target_name),
           context_req, target_data_rep, input, new_context, output, context_attr, ts_expiry,
           mapped_context, context_data );
    if (context_req & ~supported)
        FIXME( "flags 0x%08x not supported\n", context_req & ~supported );

    if (!context && !input && !credential) return SEC_E_INVALID_HANDLE;
    cred_handle = credhandle_sspi_to_gss( credential );
    ctxt_handle = ctxthandle_sspi_to_gss( context );

    if ((idx = get_buffer_index( input, SECBUFFER_TOKEN )) == -1) input_token.length = 0;
    else
    {
        input_token.length = input->pBuffers[idx].cbBuffer;
        input_token.value  = input->pBuffers[idx].pvBuffer;
    }

    if ((idx = get_buffer_index( output, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;
    output_token.length = 0;
    output_token.value  = NULL;

    if (target_name && ((status = name_sspi_to_gss( target_name, &target )) != SEC_E_OK)) return status;

    ret = pgss_init_sec_context( &minor_status, cred_handle, &ctxt_handle, target, GSS_C_NO_OID, req_flags, 0,
                                 GSS_C_NO_CHANNEL_BINDINGS, &input_token, NULL, &output_token, &ret_flags,
                                 &expiry_time );
    TRACE( "gss_init_sec_context returned %08x minor status %08x ret_flags %08x\n", ret, minor_status, ret_flags );
    if (GSS_ERROR(ret)) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE || ret == GSS_S_CONTINUE_NEEDED)
    {
        if (output_token.length > output->pBuffers[idx].cbBuffer) /* FIXME: check if larger buffer exists */
        {
            TRACE( "buffer too small %lu > %u\n", (SIZE_T)output_token.length, output->pBuffers[idx].cbBuffer );
            pgss_release_buffer( &minor_status, &output_token );
            pgss_delete_sec_context( &minor_status, &ctxt_handle, GSS_C_NO_BUFFER );
            return SEC_E_INCOMPLETE_MESSAGE;
        }
        output->pBuffers[idx].cbBuffer = output_token.length;
        memcpy( output->pBuffers[idx].pvBuffer, output_token.value, output_token.length );
        pgss_release_buffer( &minor_status, &output_token );

        ctxthandle_gss_to_sspi( ctxt_handle, new_context );
        if (context_attr) *context_attr = flags_gss_to_isc_ret( ret_flags );
        expirytime_gss_to_sspi( expiry_time, ts_expiry );
    }

    if (target != GSS_C_NO_NAME) pgss_release_name( &minor_status, &target );

    /* we do support user mode SSP/AP functions */
    *mapped_context = TRUE;
    /* FIXME: initialize context_data */

    return status_gss_to_sspi( ret );
#else
    FIXME( "(%lx %lx %s 0x%08x %u %p %p %p %p %p %p %p)\n", credential, context, debugstr_us(target_name),
           context_req, target_data_rep, input, new_context, output, context_attr, ts_expiry,
           mapped_context, context_data );
    return SEC_E_UNSUPPORTED_FUNCTION;
#endif
}

static NTSTATUS NTAPI kerberos_SpAcceptLsaModeContext( LSA_SEC_HANDLE credential, LSA_SEC_HANDLE context,
    SecBufferDesc *input, ULONG context_req, ULONG target_data_rep, LSA_SEC_HANDLE *new_context,
    SecBufferDesc *output, ULONG *context_attr, TimeStamp *ts_expiry, BOOLEAN *mapped_context, SecBuffer *context_data )
{
#ifdef SONAME_LIBGSSAPI_KRB5
    OM_uint32 ret, minor_status, ret_flags = 0, expiry_time;
    gss_cred_id_t cred_handle;
    gss_ctx_id_t ctxt_handle;
    gss_buffer_desc input_token, output_token;
    gss_name_t target = GSS_C_NO_NAME;
    int idx;

    TRACE( "(%lx %lx 0x%08x %u %p %p %p %p %p %p %p)\n", credential, context, context_req,
           target_data_rep, input, new_context, output, context_attr, ts_expiry,
           mapped_context, context_data );
    if (context_req) FIXME( "ignoring flags 0x%08x\n", context_req );

    if (!context && !input && !credential) return SEC_E_INVALID_HANDLE;
    cred_handle = credhandle_sspi_to_gss( credential );
    ctxt_handle = ctxthandle_sspi_to_gss( context );

    if (!input) input_token.length = 0;
    else
    {
        if ((idx = get_buffer_index( input, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;
        input_token.length = input->pBuffers[idx].cbBuffer;
        input_token.value  = input->pBuffers[idx].pvBuffer;
    }

    if ((idx = get_buffer_index( output, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;
    output_token.length = 0;
    output_token.value  = NULL;

    ret = pgss_accept_sec_context( &minor_status, &ctxt_handle, cred_handle, &input_token, GSS_C_NO_CHANNEL_BINDINGS,
                                   &target, NULL, &output_token, &ret_flags, &expiry_time, NULL );
    TRACE( "gss_accept_sec_context returned %08x minor status %08x ret_flags %08x\n", ret, minor_status, ret_flags );
    if (GSS_ERROR(ret)) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE || ret == GSS_S_CONTINUE_NEEDED)
    {
        if (output_token.length > output->pBuffers[idx].cbBuffer) /* FIXME: check if larger buffer exists */
        {
            TRACE( "buffer too small %lu > %u\n", (SIZE_T)output_token.length, output->pBuffers[idx].cbBuffer );
            pgss_release_buffer( &minor_status, &output_token );
            pgss_delete_sec_context( &minor_status, &ctxt_handle, GSS_C_NO_BUFFER );
            return SEC_E_BUFFER_TOO_SMALL;
        }
        output->pBuffers[idx].cbBuffer = output_token.length;
        memcpy( output->pBuffers[idx].pvBuffer, output_token.value, output_token.length );
        pgss_release_buffer( &minor_status, &output_token );

        ctxthandle_gss_to_sspi( ctxt_handle, new_context );
        if (context_attr) *context_attr = flags_gss_to_asc_ret( ret_flags );
        expirytime_gss_to_sspi( expiry_time, ts_expiry );
    }

    /* we do support user mode SSP/AP functions */
    *mapped_context = TRUE;
    /* FIXME: initialize context_data */

    return status_gss_to_sspi( ret );
#else
    FIXME( "(%lx %lx 0x%08x %u %p %p %p %p %p %p %p)\n", credential, context, context_req,
           target_data_rep, input, new_context, output, context_attr, ts_expiry,
           mapped_context, context_data );
    return SEC_E_UNSUPPORTED_FUNCTION;
#endif
}

static NTSTATUS NTAPI kerberos_SpDeleteContext( LSA_SEC_HANDLE context )
{
#ifdef SONAME_LIBGSSAPI_KRB5
    OM_uint32 ret, minor_status;
    gss_ctx_id_t ctxt_handle;

    TRACE( "(%lx)\n", context );
    if (!context) return SEC_E_INVALID_HANDLE;
    if (!(ctxt_handle = ctxthandle_sspi_to_gss( context ))) return SEC_E_OK;

    ret = pgss_delete_sec_context( &minor_status, &ctxt_handle, GSS_C_NO_BUFFER );
    TRACE( "gss_delete_sec_context returned %08x minor status %08x\n", ret, minor_status );
    if (GSS_ERROR(ret)) trace_gss_status( ret, minor_status );

    return status_gss_to_sspi( ret );
#else
    FIXME( "(%lx)\n", context );
    return SEC_E_UNSUPPORTED_FUNCTION;
#endif
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
        SecPkgContext_Sizes *sizes = (SecPkgContext_Sizes *)buffer;
        ULONG size_max_signature = 37, size_security_trailer = 49;
#ifdef SONAME_LIBGSSAPI_KRB5
        gss_ctx_id_t ctxt_handle;

        if (!(ctxt_handle = ctxthandle_sspi_to_gss( context ))) return SEC_E_INVALID_HANDLE;
        if (is_dce_style_context( ctxt_handle ))
        {
            size_max_signature = 28;
            size_security_trailer = 76;
        }
#endif
        sizes->cbMaxToken        = KERBEROS_MAX_BUF;
        sizes->cbMaxSignature    = size_max_signature;
        sizes->cbBlockSize       = 1;
        sizes->cbSecurityTrailer = size_security_trailer;
        return SEC_E_OK;
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
#ifdef SONAME_LIBGSSAPI_KRB5
    OM_uint32 ret, minor_status;
    gss_buffer_desc data_buffer, token_buffer;
    gss_ctx_id_t ctxt_handle;
    int data_idx, token_idx;

    TRACE( "(%lx 0x%08x %p %u)\n", context, quality_of_protection, message, message_seq_no );
    if (quality_of_protection) FIXME( "ignoring quality_of_protection 0x%08x\n", quality_of_protection );
    if (message_seq_no) FIXME( "ignoring message_seq_no %u\n", message_seq_no );

    if (!context) return SEC_E_INVALID_HANDLE;
    ctxt_handle = ctxthandle_sspi_to_gss( context );

    /* FIXME: multiple data buffers, read-only buffers */
    if ((data_idx = get_buffer_index( message, SECBUFFER_DATA )) == -1) return SEC_E_INVALID_TOKEN;
    data_buffer.length = message->pBuffers[data_idx].cbBuffer;
    data_buffer.value  = message->pBuffers[data_idx].pvBuffer;

    if ((token_idx = get_buffer_index( message, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;
    token_buffer.length = 0;
    token_buffer.value  = NULL;

    ret = pgss_get_mic( &minor_status, ctxt_handle, GSS_C_QOP_DEFAULT, &data_buffer, &token_buffer );
    TRACE( "gss_get_mic returned %08x minor status %08x\n", ret, minor_status );
    if (GSS_ERROR(ret)) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE)
    {
        memcpy( message->pBuffers[token_idx].pvBuffer, token_buffer.value, token_buffer.length );
        message->pBuffers[token_idx].cbBuffer = token_buffer.length;
        pgss_release_buffer( &minor_status, &token_buffer );
    }

    return status_gss_to_sspi( ret );
#else
    FIXME( "(%lx 0x%08x %p %u)\n", context, quality_of_protection, message, message_seq_no );
    return SEC_E_UNSUPPORTED_FUNCTION;
#endif
}

static NTSTATUS NTAPI kerberos_SpVerifySignature( LSA_SEC_HANDLE context, SecBufferDesc *message,
    ULONG message_seq_no, ULONG *quality_of_protection )
{
#ifdef SONAME_LIBGSSAPI_KRB5
    OM_uint32 ret, minor_status;
    gss_buffer_desc data_buffer, token_buffer;
    gss_ctx_id_t ctxt_handle;
    int data_idx, token_idx;

    TRACE( "(%lx %p %u %p)\n", context, message, message_seq_no, quality_of_protection );
    if (message_seq_no) FIXME( "ignoring message_seq_no %u\n", message_seq_no );

    if (!context) return SEC_E_INVALID_HANDLE;
    ctxt_handle = ctxthandle_sspi_to_gss( context );

    if ((data_idx = get_buffer_index( message, SECBUFFER_DATA )) == -1) return SEC_E_INVALID_TOKEN;
    data_buffer.length = message->pBuffers[data_idx].cbBuffer;
    data_buffer.value  = message->pBuffers[data_idx].pvBuffer;

    if ((token_idx = get_buffer_index( message, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;
    token_buffer.length = message->pBuffers[token_idx].cbBuffer;
    token_buffer.value  = message->pBuffers[token_idx].pvBuffer;

    ret = pgss_verify_mic( &minor_status, ctxt_handle, &data_buffer, &token_buffer, NULL );
    TRACE( "gss_verify_mic returned %08x minor status %08x\n", ret, minor_status );
    if (GSS_ERROR(ret)) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE && quality_of_protection) *quality_of_protection = 0;

    return status_gss_to_sspi( ret );
#else
    FIXME( "(%lx %p %u %p)\n", context, message, message_seq_no, quality_of_protection );
    return SEC_E_UNSUPPORTED_FUNCTION;
#endif
}

#ifdef SONAME_LIBGSSAPI_KRB5
static NTSTATUS seal_message_iov( gss_ctx_id_t ctxt_handle, SecBufferDesc *message, ULONG quality_of_protection )
{
    gss_iov_buffer_desc iov[4];
    OM_uint32 ret, minor_status;
    int token_idx, data_idx, conf_flag, conf_state;

    if (!quality_of_protection)
        conf_flag = 1; /* confidentiality + integrity */
    else if (quality_of_protection == SECQOP_WRAP_NO_ENCRYPT)
        conf_flag = 0; /* only integrity */
    else
    {
        FIXME( "QOP %08x not supported\n", quality_of_protection );
        return SEC_E_UNSUPPORTED_FUNCTION;
    }

    /* FIXME: multiple data buffers, read-only buffers */
    if ((data_idx = get_buffer_index( message, SECBUFFER_DATA )) == -1) return SEC_E_INVALID_TOKEN;
    if ((token_idx = get_buffer_index( message, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;

    iov[0].type          = GSS_IOV_BUFFER_TYPE_SIGN_ONLY | GSS_IOV_BUFFER_FLAG_ALLOCATE;
    iov[0].buffer.length = 0;
    iov[0].buffer.value  = NULL;

    iov[1].type          = GSS_IOV_BUFFER_TYPE_DATA;
    iov[1].buffer.length = message->pBuffers[data_idx].cbBuffer;
    iov[1].buffer.value  = message->pBuffers[data_idx].pvBuffer;

    iov[2].type          = GSS_IOV_BUFFER_TYPE_SIGN_ONLY | GSS_IOV_BUFFER_FLAG_ALLOCATE;
    iov[2].buffer.length = 0;
    iov[2].buffer.value  = NULL;

    iov[3].type          = GSS_IOV_BUFFER_TYPE_HEADER | GSS_IOV_BUFFER_FLAG_ALLOCATE;
    iov[3].buffer.length = 0;
    iov[3].buffer.value  = NULL;

    ret = pgss_wrap_iov( &minor_status, ctxt_handle, conf_flag, GSS_C_QOP_DEFAULT, &conf_state, iov, 4 );
    TRACE( "gss_wrap_iov returned %08x minor status %08x\n", ret, minor_status );
    if (GSS_ERROR(ret)) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE)
    {
        memcpy( message->pBuffers[token_idx].pvBuffer, iov[3].buffer.value, iov[3].buffer.length );
        message->pBuffers[token_idx].cbBuffer = iov[3].buffer.length;
        pgss_release_iov_buffer( &minor_status, iov, 4 );
    }

    return status_gss_to_sspi( ret );
}

static NTSTATUS seal_message( gss_ctx_id_t ctxt_handle, SecBufferDesc *message, ULONG quality_of_protection )
{
    gss_buffer_desc input, output;
    OM_uint32 ret, minor_status;
    int token_idx, data_idx, conf_flag, conf_state;

    if (!quality_of_protection)
        conf_flag = 1; /* confidentiality + integrity */
    else if (quality_of_protection == SECQOP_WRAP_NO_ENCRYPT)
        conf_flag = 0; /* only integrity */
    else
    {
        FIXME( "QOP %08x not supported\n", quality_of_protection );
        return SEC_E_UNSUPPORTED_FUNCTION;
    }

    /* FIXME: multiple data buffers, read-only buffers */
    if ((data_idx = get_buffer_index( message, SECBUFFER_DATA )) == -1) return SEC_E_INVALID_TOKEN;
    if ((token_idx = get_buffer_index( message, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;

    input.length = message->pBuffers[data_idx].cbBuffer;
    input.value  = message->pBuffers[data_idx].pvBuffer;

    ret = pgss_wrap( &minor_status, ctxt_handle, conf_flag, GSS_C_QOP_DEFAULT, &input, &conf_state, &output );
    TRACE( "gss_wrap returned %08x minor status %08x\n", ret, minor_status );
    if (GSS_ERROR(ret)) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE)
    {
        DWORD len_data = message->pBuffers[data_idx].cbBuffer, len_token = message->pBuffers[token_idx].cbBuffer;
        if (len_token < output.length - len_data)
        {
            TRACE( "buffer too small %lu > %u\n", (SIZE_T)output.length - len_data, len_token );
            pgss_release_buffer( &minor_status, &output );
            return SEC_E_BUFFER_TOO_SMALL;
        }
        memcpy( message->pBuffers[data_idx].pvBuffer, output.value, len_data );
        memcpy( message->pBuffers[token_idx].pvBuffer, (char *)output.value + len_data, output.length - len_data );
        message->pBuffers[token_idx].cbBuffer = output.length - len_data;
        pgss_release_buffer( &minor_status, &output );
    }

    return status_gss_to_sspi( ret );
}
#endif

static NTSTATUS NTAPI kerberos_SpSealMessage( LSA_SEC_HANDLE context, ULONG quality_of_protection,
    SecBufferDesc *message, ULONG message_seq_no )
{
#ifdef SONAME_LIBGSSAPI_KRB5
    gss_ctx_id_t ctxt_handle;

    TRACE( "(%lx 0x%08x %p %u)\n", context, quality_of_protection, message, message_seq_no );
    if (message_seq_no) FIXME( "ignoring message_seq_no %u\n", message_seq_no );

    if (!context) return SEC_E_INVALID_HANDLE;
    ctxt_handle = ctxthandle_sspi_to_gss( context );

    if (is_dce_style_context( ctxt_handle )) return seal_message_iov( ctxt_handle, message, quality_of_protection );
    return seal_message( ctxt_handle, message, quality_of_protection );
#else
    FIXME( "(%lx 0x%08x %p %u)\n", context, quality_of_protection, message, message_seq_no );
    return SEC_E_UNSUPPORTED_FUNCTION;
#endif
}

#ifdef SONAME_LIBGSSAPI_KRB5
static NTSTATUS unseal_message_iov( gss_ctx_id_t ctxt_handle, SecBufferDesc *message, ULONG *quality_of_protection )
{
    gss_iov_buffer_desc iov[4];
    OM_uint32 ret, minor_status;
    int token_idx, data_idx, conf_state;

    if ((data_idx = get_buffer_index( message, SECBUFFER_DATA )) == -1) return SEC_E_INVALID_TOKEN;
    if ((token_idx = get_buffer_index( message, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;

    iov[0].type          = GSS_IOV_BUFFER_TYPE_SIGN_ONLY;
    iov[0].buffer.length = 0;
    iov[0].buffer.value  = NULL;

    iov[1].type          = GSS_IOV_BUFFER_TYPE_DATA;
    iov[1].buffer.length = message->pBuffers[data_idx].cbBuffer;
    iov[1].buffer.value  = message->pBuffers[data_idx].pvBuffer;

    iov[2].type          = GSS_IOV_BUFFER_TYPE_SIGN_ONLY;
    iov[2].buffer.length = 0;
    iov[2].buffer.value  = NULL;

    iov[3].type          = GSS_IOV_BUFFER_TYPE_HEADER;
    iov[3].buffer.length = message->pBuffers[token_idx].cbBuffer;
    iov[3].buffer.value  = message->pBuffers[token_idx].pvBuffer;

    ret = pgss_unwrap_iov( &minor_status, ctxt_handle, &conf_state, NULL, iov, 4 );
    TRACE( "gss_unwrap_iov returned %08x minor status %08x\n", ret, minor_status );
    if (GSS_ERROR(ret)) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE && quality_of_protection)
    {
        *quality_of_protection = (conf_state ? 0 : SECQOP_WRAP_NO_ENCRYPT);
    }
    return status_gss_to_sspi( ret );
}

static NTSTATUS unseal_message( gss_ctx_id_t ctxt_handle, SecBufferDesc *message, ULONG *quality_of_protection )
{
    gss_buffer_desc input, output;
    OM_uint32 ret, minor_status;
    int token_idx, data_idx, conf_state;
    DWORD len_data, len_token;

    if ((data_idx = get_buffer_index( message, SECBUFFER_DATA )) == -1) return SEC_E_INVALID_TOKEN;
    if ((token_idx = get_buffer_index( message, SECBUFFER_TOKEN )) == -1) return SEC_E_INVALID_TOKEN;

    len_data = message->pBuffers[data_idx].cbBuffer;
    len_token = message->pBuffers[token_idx].cbBuffer;

    input.length = len_data + len_token;
    if (!(input.value = heap_alloc( input.length ))) return SEC_E_INSUFFICIENT_MEMORY;
    memcpy( input.value, message->pBuffers[data_idx].pvBuffer, len_data );
    memcpy( (char *)input.value + len_data, message->pBuffers[token_idx].pvBuffer, len_token );

    ret = pgss_unwrap( &minor_status, ctxt_handle, &input, &output, &conf_state, NULL );
    heap_free( input.value );
    TRACE( "gss_unwrap returned %08x minor status %08x\n", ret, minor_status );
    if (GSS_ERROR(ret)) trace_gss_status( ret, minor_status );
    if (ret == GSS_S_COMPLETE)
    {
        if (quality_of_protection) *quality_of_protection = (conf_state ? 0 : SECQOP_WRAP_NO_ENCRYPT);
        memcpy( message->pBuffers[data_idx].pvBuffer, output.value, len_data );
        pgss_release_buffer( &minor_status, &output );
    }

    return status_gss_to_sspi( ret );
}
#endif

static NTSTATUS NTAPI kerberos_SpUnsealMessage( LSA_SEC_HANDLE context, SecBufferDesc *message,
    ULONG message_seq_no, ULONG *quality_of_protection )
{
#ifdef SONAME_LIBGSSAPI_KRB5
    gss_ctx_id_t ctxt_handle;

    TRACE( "(%lx %p %u %p)\n", context, message, message_seq_no, quality_of_protection );
    if (message_seq_no) FIXME( "ignoring message_seq_no %u\n", message_seq_no );

    if (!context) return SEC_E_INVALID_HANDLE;
    ctxt_handle = ctxthandle_sspi_to_gss( context );

    if (is_dce_style_context( ctxt_handle )) return unseal_message_iov( ctxt_handle, message, quality_of_protection );
    return unseal_message( ctxt_handle, message, quality_of_protection );
#else
    FIXME( "(%lx %p %u %p)\n", context, message, message_seq_no, quality_of_protection );
    return SEC_E_UNSUPPORTED_FUNCTION;
#endif
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
