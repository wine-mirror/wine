/*
 * Copyright 2017 Dmitry Timoshkov
 * Copyright 2017 George Popoff
 * Copyright 2008 Robert Shearman for CodeWeavers
 * Copyright 2017,2021 Hans Leidekker for CodeWeavers
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

#include "wine/unixlib.h"

#define KERBEROS_MAX_BUF 12000

struct accept_context_params
{
    LSA_SEC_HANDLE credential;
    LSA_SEC_HANDLE context;
    SecBufferDesc *input;
    LSA_SEC_HANDLE *new_context;
    SecBufferDesc *output;
    ULONG *context_attr;
    ULONG *expiry;
};

struct acquire_credentials_handle_params
{
    const char *principal;
    ULONG credential_use;
    const char *username;
    const char *password;
    LSA_SEC_HANDLE *credential;
    ULONG *expiry;
};

struct initialize_context_params
{
    LSA_SEC_HANDLE credential;
    LSA_SEC_HANDLE context;
    const char *target_name;
    ULONG context_req;
    SecBufferDesc *input;
    LSA_SEC_HANDLE *new_context;
    SecBufferDesc *output;
    ULONG *context_attr;
    ULONG *expiry;
};

struct make_signature_params
{
    LSA_SEC_HANDLE context;
    SecBufferDesc *msg;
};

struct query_context_attributes_params
{
    LSA_SEC_HANDLE context;
    unsigned attr;
    void *buf;
};

struct query_ticket_cache_params
{
    KERB_QUERY_TKT_CACHE_RESPONSE *resp;
    ULONG *out_size;
};

struct seal_message_params
{
    LSA_SEC_HANDLE context;
    SecBufferDesc *msg;
    unsigned qop;
};

struct unseal_message_params
{
    LSA_SEC_HANDLE context;
    SecBufferDesc *msg;
    ULONG *qop;
};

struct verify_signature_params
{
    LSA_SEC_HANDLE context;
    SecBufferDesc *msg;
    ULONG *qop;
};

enum unix_funcs
{
    unix_process_attach,
    unix_accept_context,
    unix_acquire_credentials_handle,
    unix_delete_context,
    unix_free_credentials_handle,
    unix_initialize_context,
    unix_make_signature,
    unix_query_context_attributes,
    unix_query_ticket_cache,
    unix_seal_message,
    unix_unseal_message,
    unix_verify_signature,
};

extern unixlib_handle_t krb5_handle DECLSPEC_HIDDEN;

#define KRB5_CALL( func, params ) __wine_unix_call( krb5_handle, unix_ ## func, params )
