/* Copyright (C) 2005 Juan Lang
 * Copyright 2008 Henri Verbeet
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
 *
 * This file implements the schannel provider, or, the SSL/TLS implementations.
 * FIXME: It should be rather obvious that this file is empty of any
 * implementation.
 */
#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#ifdef SONAME_LIBGNUTLS
#include <gnutls/gnutls.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "sspi.h"
#include "schannel.h"
#include "secur32_priv.h"
#include "wine/debug.h"
#include "wine/library.h"

WINE_DEFAULT_DEBUG_CHANNEL(secur32);

#ifdef SONAME_LIBGNUTLS

static void *libgnutls_handle;
#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(gnutls_alert_get);
MAKE_FUNCPTR(gnutls_alert_get_name);
MAKE_FUNCPTR(gnutls_certificate_allocate_credentials);
MAKE_FUNCPTR(gnutls_certificate_free_credentials);
MAKE_FUNCPTR(gnutls_certificate_get_peers);
MAKE_FUNCPTR(gnutls_cipher_get);
MAKE_FUNCPTR(gnutls_cipher_get_key_size);
MAKE_FUNCPTR(gnutls_credentials_set);
MAKE_FUNCPTR(gnutls_deinit);
MAKE_FUNCPTR(gnutls_global_deinit);
MAKE_FUNCPTR(gnutls_global_init);
MAKE_FUNCPTR(gnutls_global_set_log_function);
MAKE_FUNCPTR(gnutls_global_set_log_level);
MAKE_FUNCPTR(gnutls_handshake);
MAKE_FUNCPTR(gnutls_init);
MAKE_FUNCPTR(gnutls_kx_get);
MAKE_FUNCPTR(gnutls_mac_get);
MAKE_FUNCPTR(gnutls_mac_get_key_size);
MAKE_FUNCPTR(gnutls_perror);
MAKE_FUNCPTR(gnutls_protocol_get_version);
MAKE_FUNCPTR(gnutls_set_default_priority);
MAKE_FUNCPTR(gnutls_record_recv);
MAKE_FUNCPTR(gnutls_record_send);
MAKE_FUNCPTR(gnutls_transport_set_errno);
MAKE_FUNCPTR(gnutls_transport_set_ptr);
MAKE_FUNCPTR(gnutls_transport_set_pull_function);
MAKE_FUNCPTR(gnutls_transport_set_push_function);
#undef MAKE_FUNCPTR

#define SCHAN_INVALID_HANDLE ~0UL

enum schan_handle_type
{
    SCHAN_HANDLE_CRED,
    SCHAN_HANDLE_CTX,
    SCHAN_HANDLE_FREE
};

struct schan_handle
{
    void *object;
    enum schan_handle_type type;
};

struct schan_credentials
{
    ULONG credential_use;
    gnutls_certificate_credentials credentials;
};

struct schan_context
{
    gnutls_session_t session;
    ULONG req_ctx_attr;
};

struct schan_transport;

struct schan_buffers
{
    SIZE_T offset;
    SIZE_T limit;
    const SecBufferDesc *desc;
    int current_buffer_idx;
    BOOL allow_buffer_resize;
    int (*get_next_buffer)(const struct schan_transport *, struct schan_buffers *);
};

struct schan_transport
{
    struct schan_context *ctx;
    struct schan_buffers in;
    struct schan_buffers out;
};

static struct schan_handle *schan_handle_table;
static struct schan_handle *schan_free_handles;
static SIZE_T schan_handle_table_size;
static SIZE_T schan_handle_count;

static ULONG_PTR schan_alloc_handle(void *object, enum schan_handle_type type)
{
    struct schan_handle *handle;

    if (schan_free_handles)
    {
        DWORD index = schan_free_handles - schan_handle_table;
        /* Use a free handle */
        handle = schan_free_handles;
        if (handle->type != SCHAN_HANDLE_FREE)
        {
            ERR("Handle %d(%p) is in the free list, but has type %#x.\n", index, handle, handle->type);
            return SCHAN_INVALID_HANDLE;
        }
        schan_free_handles = handle->object;
        handle->object = object;
        handle->type = type;

        return index;
    }
    if (!(schan_handle_count < schan_handle_table_size))
    {
        /* Grow the table */
        SIZE_T new_size = schan_handle_table_size + (schan_handle_table_size >> 1);
        struct schan_handle *new_table = HeapReAlloc(GetProcessHeap(), 0, schan_handle_table, new_size * sizeof(*schan_handle_table));
        if (!new_table)
        {
            ERR("Failed to grow the handle table\n");
            return SCHAN_INVALID_HANDLE;
        }
        schan_handle_table = new_table;
        schan_handle_table_size = new_size;
    }

    handle = &schan_handle_table[schan_handle_count++];
    handle->object = object;
    handle->type = type;

    return handle - schan_handle_table;
}

static void *schan_free_handle(ULONG_PTR handle_idx, enum schan_handle_type type)
{
    struct schan_handle *handle;
    void *object;

    if (handle_idx == SCHAN_INVALID_HANDLE) return NULL;
    if (handle_idx >= schan_handle_count) return NULL;
    handle = &schan_handle_table[handle_idx];
    if (handle->type != type)
    {
        ERR("Handle %ld(%p) is not of type %#x\n", handle_idx, handle, type);
        return NULL;
    }

    object = handle->object;
    handle->object = schan_free_handles;
    handle->type = SCHAN_HANDLE_FREE;
    schan_free_handles = handle;

    return object;
}

static void *schan_get_object(ULONG_PTR handle_idx, enum schan_handle_type type)
{
    struct schan_handle *handle;

    if (handle_idx == SCHAN_INVALID_HANDLE) return NULL;
    if (handle_idx >= schan_handle_count) return NULL;
    handle = &schan_handle_table[handle_idx];
    if (handle->type != type)
    {
        ERR("Handle %ld(%p) is not of type %#x\n", handle_idx, handle, type);
        return NULL;
    }

    return handle->object;
}

static SECURITY_STATUS schan_QueryCredentialsAttributes(
 PCredHandle phCredential, ULONG ulAttribute, VOID *pBuffer)
{
    SECURITY_STATUS ret;

    switch (ulAttribute)
    {
    case SECPKG_ATTR_SUPPORTED_ALGS:
        if (pBuffer)
        {
            /* FIXME: get from CryptoAPI */
            FIXME("SECPKG_ATTR_SUPPORTED_ALGS: stub\n");
            ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INTERNAL_ERROR;
        break;
    case SECPKG_ATTR_CIPHER_STRENGTHS:
        if (pBuffer)
        {
            SecPkgCred_CipherStrengths *r = pBuffer;

            /* FIXME: get from CryptoAPI */
            FIXME("SECPKG_ATTR_CIPHER_STRENGTHS: semi-stub\n");
            r->dwMinimumCipherStrength = 40;
            r->dwMaximumCipherStrength = 168;
            ret = SEC_E_OK;
        }
        else
            ret = SEC_E_INTERNAL_ERROR;
        break;
    case SECPKG_ATTR_SUPPORTED_PROTOCOLS:
        if (pBuffer)
        {
            /* FIXME: get from OpenSSL? */
            FIXME("SECPKG_ATTR_SUPPORTED_PROTOCOLS: stub\n");
            ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INTERNAL_ERROR;
        break;
    default:
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    return ret;
}

static SECURITY_STATUS SEC_ENTRY schan_QueryCredentialsAttributesA(
 PCredHandle phCredential, ULONG ulAttribute, PVOID pBuffer)
{
    SECURITY_STATUS ret;

    TRACE("(%p, %d, %p)\n", phCredential, ulAttribute, pBuffer);

    switch (ulAttribute)
    {
    case SECPKG_CRED_ATTR_NAMES:
        FIXME("SECPKG_CRED_ATTR_NAMES: stub\n");
        ret = SEC_E_UNSUPPORTED_FUNCTION;
        break;
    default:
        ret = schan_QueryCredentialsAttributes(phCredential, ulAttribute,
         pBuffer);
    }
    return ret;
}

static SECURITY_STATUS SEC_ENTRY schan_QueryCredentialsAttributesW(
 PCredHandle phCredential, ULONG ulAttribute, PVOID pBuffer)
{
    SECURITY_STATUS ret;

    TRACE("(%p, %d, %p)\n", phCredential, ulAttribute, pBuffer);

    switch (ulAttribute)
    {
    case SECPKG_CRED_ATTR_NAMES:
        FIXME("SECPKG_CRED_ATTR_NAMES: stub\n");
        ret = SEC_E_UNSUPPORTED_FUNCTION;
        break;
    default:
        ret = schan_QueryCredentialsAttributes(phCredential, ulAttribute,
         pBuffer);
    }
    return ret;
}

static SECURITY_STATUS schan_CheckCreds(const SCHANNEL_CRED *schanCred)
{
    SECURITY_STATUS st;
    DWORD i;

    TRACE("dwVersion = %d\n", schanCred->dwVersion);
    TRACE("cCreds = %d\n", schanCred->cCreds);
    TRACE("hRootStore = %p\n", schanCred->hRootStore);
    TRACE("cMappers = %d\n", schanCred->cMappers);
    TRACE("cSupportedAlgs = %d:\n", schanCred->cSupportedAlgs);
    for (i = 0; i < schanCred->cSupportedAlgs; i++)
        TRACE("%08x\n", schanCred->palgSupportedAlgs[i]);
    TRACE("grbitEnabledProtocols = %08x\n", schanCred->grbitEnabledProtocols);
    TRACE("dwMinimumCipherStrength = %d\n", schanCred->dwMinimumCipherStrength);
    TRACE("dwMaximumCipherStrength = %d\n", schanCred->dwMaximumCipherStrength);
    TRACE("dwSessionLifespan = %d\n", schanCred->dwSessionLifespan);
    TRACE("dwFlags = %08x\n", schanCred->dwFlags);
    TRACE("dwCredFormat = %d\n", schanCred->dwCredFormat);

    switch (schanCred->dwVersion)
    {
    case SCH_CRED_V3:
    case SCHANNEL_CRED_VERSION:
        break;
    default:
        return SEC_E_INTERNAL_ERROR;
    }

    if (schanCred->cCreds == 0)
        st = SEC_E_NO_CREDENTIALS;
    else if (schanCred->cCreds > 1)
        st = SEC_E_UNKNOWN_CREDENTIALS;
    else
    {
        DWORD keySpec;
        HCRYPTPROV csp;
        BOOL ret, freeCSP;

        ret = CryptAcquireCertificatePrivateKey(schanCred->paCred[0],
         0, /* FIXME: what flags to use? */ NULL,
         &csp, &keySpec, &freeCSP);
        if (ret)
        {
            st = SEC_E_OK;
            if (freeCSP)
                CryptReleaseContext(csp, 0);
        }
        else
            st = SEC_E_UNKNOWN_CREDENTIALS;
    }
    return st;
}

static SECURITY_STATUS schan_AcquireClientCredentials(const SCHANNEL_CRED *schanCred,
 PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    struct schan_credentials *creds;
    SECURITY_STATUS st = SEC_E_OK;

    TRACE("schanCred %p, phCredential %p, ptsExpiry %p\n", schanCred, phCredential, ptsExpiry);

    if (schanCred)
    {
        st = schan_CheckCreds(schanCred);
        if (st == SEC_E_NO_CREDENTIALS)
            st = SEC_E_OK;
    }

    /* For now, the only thing I'm interested in is the direction of the
     * connection, so just store it.
     */
    if (st == SEC_E_OK)
    {
        ULONG_PTR handle;
        int ret;

        creds = HeapAlloc(GetProcessHeap(), 0, sizeof(*creds));
        if (!creds) return SEC_E_INSUFFICIENT_MEMORY;

        handle = schan_alloc_handle(creds, SCHAN_HANDLE_CRED);
        if (handle == SCHAN_INVALID_HANDLE) goto fail;

        creds->credential_use = SECPKG_CRED_OUTBOUND;
        ret = pgnutls_certificate_allocate_credentials(&creds->credentials);
        if (ret != GNUTLS_E_SUCCESS)
        {
            pgnutls_perror(ret);
            schan_free_handle(handle, SCHAN_HANDLE_CRED);
            goto fail;
        }

        phCredential->dwLower = handle;
        phCredential->dwUpper = 0;

        /* Outbound credentials have no expiry */
        if (ptsExpiry)
        {
            ptsExpiry->LowPart = 0;
            ptsExpiry->HighPart = 0;
        }
    }
    return st;

fail:
    HeapFree(GetProcessHeap(), 0, creds);
    return SEC_E_INTERNAL_ERROR;
}

static SECURITY_STATUS schan_AcquireServerCredentials(const SCHANNEL_CRED *schanCred,
 PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS st;

    TRACE("schanCred %p, phCredential %p, ptsExpiry %p\n", schanCred, phCredential, ptsExpiry);

    if (!schanCred) return SEC_E_NO_CREDENTIALS;

    st = schan_CheckCreds(schanCred);
    if (st == SEC_E_OK)
    {
        ULONG_PTR handle;
        struct schan_credentials *creds;

        creds = HeapAlloc(GetProcessHeap(), 0, sizeof(*creds));
        if (!creds) return SEC_E_INSUFFICIENT_MEMORY;
        creds->credential_use = SECPKG_CRED_INBOUND;

        handle = schan_alloc_handle(creds, SCHAN_HANDLE_CRED);
        if (handle == SCHAN_INVALID_HANDLE)
        {
            HeapFree(GetProcessHeap(), 0, creds);
            return SEC_E_INTERNAL_ERROR;
        }

        phCredential->dwLower = handle;
        phCredential->dwUpper = 0;

        /* FIXME: get expiry from cert */
    }
    return st;
}

static SECURITY_STATUS schan_AcquireCredentialsHandle(ULONG fCredentialUse,
 const SCHANNEL_CRED *schanCred, PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;

    if (fCredentialUse == SECPKG_CRED_OUTBOUND)
        ret = schan_AcquireClientCredentials(schanCred, phCredential,
         ptsExpiry);
    else
        ret = schan_AcquireServerCredentials(schanCred, phCredential,
         ptsExpiry);
    return ret;
}

static SECURITY_STATUS SEC_ENTRY schan_AcquireCredentialsHandleA(
 SEC_CHAR *pszPrincipal, SEC_CHAR *pszPackage, ULONG fCredentialUse,
 PLUID pLogonID, PVOID pAuthData, SEC_GET_KEY_FN pGetKeyFn,
 PVOID pGetKeyArgument, PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    TRACE("(%s, %s, 0x%08x, %p, %p, %p, %p, %p, %p)\n",
     debugstr_a(pszPrincipal), debugstr_a(pszPackage), fCredentialUse,
     pLogonID, pAuthData, pGetKeyFn, pGetKeyArgument, phCredential, ptsExpiry);
    return schan_AcquireCredentialsHandle(fCredentialUse,
     pAuthData, phCredential, ptsExpiry);
}

static SECURITY_STATUS SEC_ENTRY schan_AcquireCredentialsHandleW(
 SEC_WCHAR *pszPrincipal, SEC_WCHAR *pszPackage, ULONG fCredentialUse,
 PLUID pLogonID, PVOID pAuthData, SEC_GET_KEY_FN pGetKeyFn,
 PVOID pGetKeyArgument, PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    TRACE("(%s, %s, 0x%08x, %p, %p, %p, %p, %p, %p)\n",
     debugstr_w(pszPrincipal), debugstr_w(pszPackage), fCredentialUse,
     pLogonID, pAuthData, pGetKeyFn, pGetKeyArgument, phCredential, ptsExpiry);
    return schan_AcquireCredentialsHandle(fCredentialUse,
     pAuthData, phCredential, ptsExpiry);
}

static SECURITY_STATUS SEC_ENTRY schan_FreeCredentialsHandle(
 PCredHandle phCredential)
{
    struct schan_credentials *creds;

    TRACE("phCredential %p\n", phCredential);

    if (!phCredential) return SEC_E_INVALID_HANDLE;

    creds = schan_free_handle(phCredential->dwLower, SCHAN_HANDLE_CRED);
    if (!creds) return SEC_E_INVALID_HANDLE;

    if (creds->credential_use == SECPKG_CRED_OUTBOUND)
        pgnutls_certificate_free_credentials(creds->credentials);
    HeapFree(GetProcessHeap(), 0, creds);

    return SEC_E_OK;
}

static void init_schan_buffers(struct schan_buffers *s, const PSecBufferDesc desc,
        int (*get_next_buffer)(const struct schan_transport *, struct schan_buffers *))
{
    s->offset = 0;
    s->limit = 0;
    s->desc = desc;
    s->current_buffer_idx = -1;
    s->allow_buffer_resize = FALSE;
    s->get_next_buffer = get_next_buffer;
}

static int schan_find_sec_buffer_idx(const SecBufferDesc *desc, unsigned int start_idx, ULONG buffer_type)
{
    unsigned int i;
    PSecBuffer buffer;

    for (i = start_idx; i < desc->cBuffers; ++i)
    {
        buffer = &desc->pBuffers[i];
        if (buffer->BufferType == buffer_type) return i;
    }

    return -1;
}

static void schan_resize_current_buffer(const struct schan_buffers *s, SIZE_T min_size)
{
    SecBuffer *b = &s->desc->pBuffers[s->current_buffer_idx];
    SIZE_T new_size = b->cbBuffer ? b->cbBuffer * 2 : 128;
    void *new_data;

    if (b->cbBuffer >= min_size || !s->allow_buffer_resize || min_size > UINT_MAX / 2) return;

    while (new_size < min_size) new_size *= 2;

    if (b->pvBuffer)
        new_data = HeapReAlloc(GetProcessHeap(), 0, b->pvBuffer, new_size);
    else
        new_data = HeapAlloc(GetProcessHeap(), 0, new_size);

    if (!new_data)
    {
        TRACE("Failed to resize %p from %d to %ld\n", b->pvBuffer, b->cbBuffer, new_size);
        return;
    }

    b->cbBuffer = new_size;
    b->pvBuffer = new_data;
}

static char *schan_get_buffer(const struct schan_transport *t, struct schan_buffers *s, size_t *count)
{
    SIZE_T max_count;
    PSecBuffer buffer;

    if (!s->desc)
    {
        TRACE("No desc\n");
        return NULL;
    }

    if (s->current_buffer_idx == -1)
    {
        /* Initial buffer */
        int buffer_idx = s->get_next_buffer(t, s);
        if (buffer_idx == -1)
        {
            TRACE("No next buffer\n");
            return NULL;
        }
        s->current_buffer_idx = buffer_idx;
    }

    buffer = &s->desc->pBuffers[s->current_buffer_idx];
    TRACE("Using buffer %d: cbBuffer %d, BufferType %#x, pvBuffer %p\n", s->current_buffer_idx, buffer->cbBuffer, buffer->BufferType, buffer->pvBuffer);

    schan_resize_current_buffer(s, s->offset + *count);
    max_count = buffer->cbBuffer - s->offset;
    if (!max_count)
    {
        int buffer_idx;

        s->allow_buffer_resize = FALSE;
        buffer_idx = s->get_next_buffer(t, s);
        if (buffer_idx == -1)
        {
            TRACE("No next buffer\n");
            return NULL;
        }
        s->current_buffer_idx = buffer_idx;
        s->offset = 0;
        return schan_get_buffer(t, s, count);
    }

    if (*count > max_count) *count = max_count;
    return (char *)buffer->pvBuffer + s->offset;
}

static ssize_t schan_pull(gnutls_transport_ptr_t transport, void *buff, size_t buff_len)
{
    struct schan_transport *t = transport;
    char *b;

    TRACE("Pull %zu bytes\n", buff_len);

    b = schan_get_buffer(t, &t->in, &buff_len);
    if (!b)
    {
        pgnutls_transport_set_errno(t->ctx->session, EAGAIN);
        return -1;
    }

    if (t->in.limit != 0 && t->in.offset + buff_len >= t->in.limit)
    {
        buff_len = t->in.limit - t->in.offset;
        if (buff_len == 0)
        {
            pgnutls_transport_set_errno(t->ctx->session, EAGAIN);
            return -1;
        }
    }

    memcpy(buff, b, buff_len);
    t->in.offset += buff_len;

    TRACE("Read %zu bytes\n", buff_len);

    return buff_len;
}

static ssize_t schan_push(gnutls_transport_ptr_t transport, const void *buff, size_t buff_len)
{
    struct schan_transport *t = transport;
    char *b;

    TRACE("Push %zu bytes\n", buff_len);

    b = schan_get_buffer(t, &t->out, &buff_len);
    if (!b)
    {
        pgnutls_transport_set_errno(t->ctx->session, EAGAIN);
        return -1;
    }

    memcpy(b, buff, buff_len);
    t->out.offset += buff_len;

    TRACE("Wrote %zu bytes\n", buff_len);

    return buff_len;
}

static int schan_init_sec_ctx_get_next_buffer(const struct schan_transport *t, struct schan_buffers *s)
{
    if (s->current_buffer_idx == -1)
    {
        int idx = schan_find_sec_buffer_idx(s->desc, 0, SECBUFFER_TOKEN);
        if (t->ctx->req_ctx_attr & ISC_REQ_ALLOCATE_MEMORY)
        {
            if (idx == -1)
            {
                idx = schan_find_sec_buffer_idx(s->desc, 0, SECBUFFER_EMPTY);
                if (idx != -1) s->desc->pBuffers[idx].BufferType = SECBUFFER_TOKEN;
            }
            if (idx != -1 && !s->desc->pBuffers[idx].pvBuffer)
            {
                s->desc->pBuffers[idx].cbBuffer = 0;
                s->allow_buffer_resize = TRUE;
            }
        }
        return idx;
    }

    return -1;
}

static void dump_buffer_desc(SecBufferDesc *desc)
{
    unsigned int i;

    if (!desc) return;
    TRACE("Buffer desc %p:\n", desc);
    for (i = 0; i < desc->cBuffers; ++i)
    {
        SecBuffer *b = &desc->pBuffers[i];
        TRACE("\tbuffer %u: cbBuffer %d, BufferType %#x pvBuffer %p\n", i, b->cbBuffer, b->BufferType, b->pvBuffer);
    }
}

/***********************************************************************
 *              InitializeSecurityContextW
 */
static SECURITY_STATUS SEC_ENTRY schan_InitializeSecurityContextW(
 PCredHandle phCredential, PCtxtHandle phContext, SEC_WCHAR *pszTargetName,
 ULONG fContextReq, ULONG Reserved1, ULONG TargetDataRep,
 PSecBufferDesc pInput, ULONG Reserved2, PCtxtHandle phNewContext,
 PSecBufferDesc pOutput, ULONG *pfContextAttr, PTimeStamp ptsExpiry)
{
    struct schan_context *ctx;
    struct schan_buffers *out_buffers;
    struct schan_credentials *cred;
    struct schan_transport transport;
    int err;

    TRACE("%p %p %s 0x%08x %d %d %p %d %p %p %p %p\n", phCredential, phContext,
     debugstr_w(pszTargetName), fContextReq, Reserved1, TargetDataRep, pInput,
     Reserved1, phNewContext, pOutput, pfContextAttr, ptsExpiry);

    dump_buffer_desc(pInput);
    dump_buffer_desc(pOutput);

    if (!phContext)
    {
        ULONG_PTR handle;

        if (!phCredential) return SEC_E_INVALID_HANDLE;

        cred = schan_get_object(phCredential->dwLower, SCHAN_HANDLE_CRED);
        if (!cred) return SEC_E_INVALID_HANDLE;

        if (!(cred->credential_use & SECPKG_CRED_OUTBOUND))
        {
            WARN("Invalid credential use %#x\n", cred->credential_use);
            return SEC_E_INVALID_HANDLE;
        }

        ctx = HeapAlloc(GetProcessHeap(), 0, sizeof(*ctx));
        if (!ctx) return SEC_E_INSUFFICIENT_MEMORY;

        handle = schan_alloc_handle(ctx, SCHAN_HANDLE_CTX);
        if (handle == SCHAN_INVALID_HANDLE)
        {
            HeapFree(GetProcessHeap(), 0, ctx);
            return SEC_E_INTERNAL_ERROR;
        }

        err = pgnutls_init(&ctx->session, GNUTLS_CLIENT);
        if (err != GNUTLS_E_SUCCESS)
        {
            pgnutls_perror(err);
            schan_free_handle(handle, SCHAN_HANDLE_CTX);
            HeapFree(GetProcessHeap(), 0, ctx);
            return SEC_E_INTERNAL_ERROR;
        }

        /* FIXME: We should be using the information from the credentials here. */
        FIXME("Using hardcoded \"NORMAL\" priority\n");
        err = pgnutls_set_default_priority(ctx->session);
        if (err != GNUTLS_E_SUCCESS)
        {
            pgnutls_perror(err);
            pgnutls_deinit(ctx->session);
            schan_free_handle(handle, SCHAN_HANDLE_CTX);
            HeapFree(GetProcessHeap(), 0, ctx);
        }

        err = pgnutls_credentials_set(ctx->session, GNUTLS_CRD_CERTIFICATE, cred->credentials);
        if (err != GNUTLS_E_SUCCESS)
        {
            pgnutls_perror(err);
            pgnutls_deinit(ctx->session);
            schan_free_handle(handle, SCHAN_HANDLE_CTX);
            HeapFree(GetProcessHeap(), 0, ctx);
        }

        pgnutls_transport_set_pull_function(ctx->session, schan_pull);
        pgnutls_transport_set_push_function(ctx->session, schan_push);

        phNewContext->dwLower = handle;
        phNewContext->dwUpper = 0;
    }
    else
    {
        ctx = schan_get_object(phContext->dwLower, SCHAN_HANDLE_CTX);
    }

    ctx->req_ctx_attr = fContextReq;

    transport.ctx = ctx;
    init_schan_buffers(&transport.in, pInput, schan_init_sec_ctx_get_next_buffer);
    init_schan_buffers(&transport.out, pOutput, schan_init_sec_ctx_get_next_buffer);
    pgnutls_transport_set_ptr(ctx->session, &transport);

    /* Perform the TLS handshake */
    err = pgnutls_handshake(ctx->session);

    if(transport.in.offset && transport.in.offset != pInput->pBuffers[0].cbBuffer) {
        if(pInput->cBuffers<2 || pInput->pBuffers[1].BufferType!=SECBUFFER_EMPTY)
            return SEC_E_INVALID_TOKEN;

        pInput->pBuffers[1].BufferType = SECBUFFER_EXTRA;
        pInput->pBuffers[1].cbBuffer = pInput->pBuffers[0].cbBuffer-transport.in.offset;
    }

    out_buffers = &transport.out;
    if (out_buffers->current_buffer_idx != -1)
    {
        SecBuffer *buffer = &out_buffers->desc->pBuffers[out_buffers->current_buffer_idx];
        buffer->cbBuffer = out_buffers->offset;
    }

    *pfContextAttr = 0;
    if (ctx->req_ctx_attr & ISC_REQ_ALLOCATE_MEMORY)
        *pfContextAttr |= ISC_RET_ALLOCATED_MEMORY;

    switch(err)
    {
        case GNUTLS_E_SUCCESS:
            TRACE("Handshake completed\n");
            return SEC_E_OK;

        case GNUTLS_E_AGAIN:
            TRACE("Continue...\n");
            return SEC_I_CONTINUE_NEEDED;

        case GNUTLS_E_WARNING_ALERT_RECEIVED:
        case GNUTLS_E_FATAL_ALERT_RECEIVED:
        {
            gnutls_alert_description_t alert = pgnutls_alert_get(ctx->session);
            const char *alert_name = pgnutls_alert_get_name(alert);
            WARN("ALERT: %d %s\n", alert, alert_name);
            return SEC_E_INTERNAL_ERROR;
        }

        default:
            pgnutls_perror(err);
            return SEC_E_INTERNAL_ERROR;
    }
}

/***********************************************************************
 *              InitializeSecurityContextA
 */
static SECURITY_STATUS SEC_ENTRY schan_InitializeSecurityContextA(
 PCredHandle phCredential, PCtxtHandle phContext, SEC_CHAR *pszTargetName,
 ULONG fContextReq, ULONG Reserved1, ULONG TargetDataRep,
 PSecBufferDesc pInput, ULONG Reserved2, PCtxtHandle phNewContext,
 PSecBufferDesc pOutput, ULONG *pfContextAttr, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;
    SEC_WCHAR *target_name = NULL;

    TRACE("%p %p %s %d %d %d %p %d %p %p %p %p\n", phCredential, phContext,
     debugstr_a(pszTargetName), fContextReq, Reserved1, TargetDataRep, pInput,
     Reserved1, phNewContext, pOutput, pfContextAttr, ptsExpiry);

    if (pszTargetName)
    {
        INT len = MultiByteToWideChar(CP_ACP, 0, pszTargetName, -1, NULL, 0);
        target_name = HeapAlloc(GetProcessHeap(), 0, len * sizeof(*target_name));
        MultiByteToWideChar(CP_ACP, 0, pszTargetName, -1, target_name, len);
    }

    ret = schan_InitializeSecurityContextW(phCredential, phContext, target_name,
            fContextReq, Reserved1, TargetDataRep, pInput, Reserved2,
            phNewContext, pOutput, pfContextAttr, ptsExpiry);

    HeapFree(GetProcessHeap(), 0, target_name);

    return ret;
}

static unsigned int schannel_get_cipher_block_size(gnutls_cipher_algorithm_t cipher)
{
    const struct
    {
        gnutls_cipher_algorithm_t cipher;
        unsigned int block_size;
    }
    algorithms[] =
    {
        {GNUTLS_CIPHER_3DES_CBC, 8},
        {GNUTLS_CIPHER_AES_128_CBC, 16},
        {GNUTLS_CIPHER_AES_256_CBC, 16},
        {GNUTLS_CIPHER_ARCFOUR_128, 1},
        {GNUTLS_CIPHER_ARCFOUR_40, 1},
        {GNUTLS_CIPHER_DES_CBC, 8},
        {GNUTLS_CIPHER_NULL, 1},
        {GNUTLS_CIPHER_RC2_40_CBC, 8},
    };
    unsigned int i;

    for (i = 0; i < sizeof(algorithms) / sizeof(*algorithms); ++i)
    {
        if (algorithms[i].cipher == cipher)
            return algorithms[i].block_size;
    }

    FIXME("Unknown cipher %#x, returning 1\n", cipher);

    return 1;
}

static DWORD schannel_get_protocol(gnutls_protocol_t proto)
{
    /* FIXME: currently schannel only implements client connections, but
     * there's no reason it couldn't be used for servers as well.  The
     * context doesn't tell us which it is, so assume client for now.
     */
    switch (proto)
    {
    case GNUTLS_SSL3: return SP_PROT_SSL3_CLIENT;
    case GNUTLS_TLS1_0: return SP_PROT_TLS1_CLIENT;
    default:
        FIXME("unknown protocol %d\n", proto);
        return 0;
    }
}

static ALG_ID schannel_get_cipher_algid(gnutls_cipher_algorithm_t cipher)
{
    switch (cipher)
    {
    case GNUTLS_CIPHER_UNKNOWN:
    case GNUTLS_CIPHER_NULL: return 0;
    case GNUTLS_CIPHER_ARCFOUR_40:
    case GNUTLS_CIPHER_ARCFOUR_128: return CALG_RC4;
    case GNUTLS_CIPHER_DES_CBC:
    case GNUTLS_CIPHER_3DES_CBC: return CALG_DES;
    case GNUTLS_CIPHER_AES_128_CBC:
    case GNUTLS_CIPHER_AES_256_CBC: return CALG_AES;
    case GNUTLS_CIPHER_RC2_40_CBC: return CALG_RC2;
    default:
        FIXME("unknown algorithm %d\n", cipher);
        return 0;
    }
}

static ALG_ID schannel_get_mac_algid(gnutls_mac_algorithm_t mac)
{
    switch (mac)
    {
    case GNUTLS_MAC_UNKNOWN:
    case GNUTLS_MAC_NULL: return 0;
    case GNUTLS_MAC_MD5: return CALG_MD5;
    case GNUTLS_MAC_SHA1:
    case GNUTLS_MAC_SHA256:
    case GNUTLS_MAC_SHA384:
    case GNUTLS_MAC_SHA512: return CALG_SHA;
    default:
        FIXME("unknown algorithm %d\n", mac);
        return 0;
    }
}

static ALG_ID schannel_get_kx_algid(gnutls_kx_algorithm_t kx)
{
    switch (kx)
    {
        case GNUTLS_KX_RSA: return CALG_RSA_KEYX;
        case GNUTLS_KX_DHE_DSS:
        case GNUTLS_KX_DHE_RSA: return CALG_DH_EPHEM;
    default:
        FIXME("unknown algorithm %d\n", kx);
        return 0;
    }
}

static SECURITY_STATUS SEC_ENTRY schan_QueryContextAttributesW(
        PCtxtHandle context_handle, ULONG attribute, PVOID buffer)
{
    struct schan_context *ctx;

    TRACE("context_handle %p, attribute %#x, buffer %p\n",
            context_handle, attribute, buffer);

    if (!context_handle) return SEC_E_INVALID_HANDLE;
    ctx = schan_get_object(context_handle->dwLower, SCHAN_HANDLE_CTX);

    switch(attribute)
    {
        case SECPKG_ATTR_STREAM_SIZES:
        {
            SecPkgContext_StreamSizes *stream_sizes = buffer;
            gnutls_mac_algorithm_t mac = pgnutls_mac_get(ctx->session);
            size_t mac_size = pgnutls_mac_get_key_size(mac);
            gnutls_cipher_algorithm_t cipher = pgnutls_cipher_get(ctx->session);
            unsigned int block_size = schannel_get_cipher_block_size(cipher);

            TRACE("Using %zu mac bytes, block size %u\n", mac_size, block_size);

            /* These are defined by the TLS RFC */
            stream_sizes->cbHeader = 5;
            stream_sizes->cbTrailer = mac_size + 256; /* Max 255 bytes padding + 1 for padding size */
            stream_sizes->cbMaximumMessage = 1 << 14;
            stream_sizes->cbBuffers = 4;
            stream_sizes->cbBlockSize = block_size;
            return SEC_E_OK;
        }
        case SECPKG_ATTR_REMOTE_CERT_CONTEXT:
        {
            unsigned int list_size;
            const gnutls_datum_t *datum;

            datum = pgnutls_certificate_get_peers(ctx->session, &list_size);
            if (datum)
            {
                PCCERT_CONTEXT *cert = buffer;

                *cert = CertCreateCertificateContext(X509_ASN_ENCODING,
                        datum->data, datum->size);
                if (!*cert)
                    return GetLastError();
                else
                    return SEC_E_OK;
            }
            else
                return SEC_E_INTERNAL_ERROR;
        }
        case SECPKG_ATTR_CONNECTION_INFO:
        {
            SecPkgContext_ConnectionInfo *info = buffer;
            gnutls_protocol_t proto = pgnutls_protocol_get_version(ctx->session);
            gnutls_cipher_algorithm_t alg = pgnutls_cipher_get(ctx->session);
            gnutls_mac_algorithm_t mac = pgnutls_mac_get(ctx->session);
            gnutls_kx_algorithm_t kx = pgnutls_kx_get(ctx->session);

            info->dwProtocol = schannel_get_protocol(proto);
            info->aiCipher = schannel_get_cipher_algid(alg);
            info->dwCipherStrength = pgnutls_cipher_get_key_size(alg);
            info->aiHash = schannel_get_mac_algid(mac);
            info->dwHashStrength = pgnutls_mac_get_key_size(mac);
            info->aiExch = schannel_get_kx_algid(kx);
            /* FIXME: info->dwExchStrength? */
            info->dwExchStrength = 0;
            return SEC_E_OK;
        }

        default:
            FIXME("Unhandled attribute %#x\n", attribute);
            return SEC_E_UNSUPPORTED_FUNCTION;
    }
}

static SECURITY_STATUS SEC_ENTRY schan_QueryContextAttributesA(
        PCtxtHandle context_handle, ULONG attribute, PVOID buffer)
{
    TRACE("context_handle %p, attribute %#x, buffer %p\n",
            context_handle, attribute, buffer);

    switch(attribute)
    {
        case SECPKG_ATTR_STREAM_SIZES:
            return schan_QueryContextAttributesW(context_handle, attribute, buffer);
        case SECPKG_ATTR_REMOTE_CERT_CONTEXT:
            return schan_QueryContextAttributesW(context_handle, attribute, buffer);
        case SECPKG_ATTR_CONNECTION_INFO:
            return schan_QueryContextAttributesW(context_handle, attribute, buffer);

        default:
            FIXME("Unhandled attribute %#x\n", attribute);
            return SEC_E_UNSUPPORTED_FUNCTION;
    }
}

static int schan_encrypt_message_get_next_buffer(const struct schan_transport *t, struct schan_buffers *s)
{
    SecBuffer *b;

    if (s->current_buffer_idx == -1)
        return schan_find_sec_buffer_idx(s->desc, 0, SECBUFFER_STREAM_HEADER);

    b = &s->desc->pBuffers[s->current_buffer_idx];

    if (b->BufferType == SECBUFFER_STREAM_HEADER)
        return schan_find_sec_buffer_idx(s->desc, 0, SECBUFFER_DATA);

    if (b->BufferType == SECBUFFER_DATA)
        return schan_find_sec_buffer_idx(s->desc, 0, SECBUFFER_STREAM_TRAILER);

    return -1;
}

static int schan_encrypt_message_get_next_buffer_token(const struct schan_transport *t, struct schan_buffers *s)
{
    SecBuffer *b;

    if (s->current_buffer_idx == -1)
        return schan_find_sec_buffer_idx(s->desc, 0, SECBUFFER_TOKEN);

    b = &s->desc->pBuffers[s->current_buffer_idx];

    if (b->BufferType == SECBUFFER_TOKEN)
    {
        int idx = schan_find_sec_buffer_idx(s->desc, 0, SECBUFFER_TOKEN);
        if (idx != s->current_buffer_idx) return -1;
        return schan_find_sec_buffer_idx(s->desc, 0, SECBUFFER_DATA);
    }

    if (b->BufferType == SECBUFFER_DATA)
    {
        int idx = schan_find_sec_buffer_idx(s->desc, 0, SECBUFFER_TOKEN);
        if (idx != -1)
            idx = schan_find_sec_buffer_idx(s->desc, idx + 1, SECBUFFER_TOKEN);
        return idx;
    }

    return -1;
}

static SECURITY_STATUS SEC_ENTRY schan_EncryptMessage(PCtxtHandle context_handle,
        ULONG quality, PSecBufferDesc message, ULONG message_seq_no)
{
    struct schan_transport transport;
    struct schan_context *ctx;
    struct schan_buffers *b;
    SecBuffer *buffer;
    SIZE_T data_size;
    char *data;
    ssize_t sent = 0;
    ssize_t ret;
    int idx;

    TRACE("context_handle %p, quality %d, message %p, message_seq_no %d\n",
            context_handle, quality, message, message_seq_no);

    if (!context_handle) return SEC_E_INVALID_HANDLE;
    ctx = schan_get_object(context_handle->dwLower, SCHAN_HANDLE_CTX);

    dump_buffer_desc(message);

    idx = schan_find_sec_buffer_idx(message, 0, SECBUFFER_DATA);
    if (idx == -1)
    {
        WARN("No data buffer passed\n");
        return SEC_E_INTERNAL_ERROR;
    }
    buffer = &message->pBuffers[idx];

    data_size = buffer->cbBuffer;
    data = HeapAlloc(GetProcessHeap(), 0, data_size);
    memcpy(data, buffer->pvBuffer, data_size);

    transport.ctx = ctx;
    init_schan_buffers(&transport.in, NULL, NULL);
    if (schan_find_sec_buffer_idx(message, 0, SECBUFFER_STREAM_HEADER) != -1)
        init_schan_buffers(&transport.out, message, schan_encrypt_message_get_next_buffer);
    else
        init_schan_buffers(&transport.out, message, schan_encrypt_message_get_next_buffer_token);
    pgnutls_transport_set_ptr(ctx->session, &transport);

    while (sent < data_size)
    {
        ret = pgnutls_record_send(ctx->session, data + sent, data_size - sent);
        if (ret < 0)
        {
            if (ret != GNUTLS_E_AGAIN)
            {
                pgnutls_perror(ret);
                HeapFree(GetProcessHeap(), 0, data);
                ERR("Returning SEC_E_INTERNAL_ERROR\n");
                return SEC_E_INTERNAL_ERROR;
            }
            else break;
        }
        sent += ret;
    }

    TRACE("Sent %zd bytes\n", sent);

    b = &transport.out;
    b->desc->pBuffers[b->current_buffer_idx].cbBuffer = b->offset;
    HeapFree(GetProcessHeap(), 0, data);

    return SEC_E_OK;
}

static int schan_decrypt_message_get_next_buffer(const struct schan_transport *t, struct schan_buffers *s)
{
    if (s->current_buffer_idx == -1)
        return schan_find_sec_buffer_idx(s->desc, 0, SECBUFFER_DATA);

    return -1;
}

static int schan_validate_decrypt_buffer_desc(PSecBufferDesc message)
{
    int data_idx = -1;
    unsigned int empty_count = 0;
    unsigned int i;

    if (message->cBuffers < 4)
    {
        WARN("Less than four buffers passed\n");
        return -1;
    }

    for (i = 0; i < message->cBuffers; ++i)
    {
        SecBuffer *b = &message->pBuffers[i];
        if (b->BufferType == SECBUFFER_DATA)
        {
            if (data_idx != -1)
            {
                WARN("More than one data buffer passed\n");
                return -1;
            }
            data_idx = i;
        }
        else if (b->BufferType == SECBUFFER_EMPTY)
            ++empty_count;
    }

    if (data_idx == -1)
    {
        WARN("No data buffer passed\n");
        return -1;
    }

    if (empty_count < 3)
    {
        WARN("Less than three empty buffers passed\n");
        return -1;
    }

    return data_idx;
}

static void schan_decrypt_fill_buffer(PSecBufferDesc message, ULONG buffer_type, void *data, ULONG size)
{
    int idx;
    SecBuffer *buffer;

    idx = schan_find_sec_buffer_idx(message, 0, SECBUFFER_EMPTY);
    buffer = &message->pBuffers[idx];

    buffer->BufferType = buffer_type;
    buffer->pvBuffer = data;
    buffer->cbBuffer = size;
}

static SECURITY_STATUS SEC_ENTRY schan_DecryptMessage(PCtxtHandle context_handle,
        PSecBufferDesc message, ULONG message_seq_no, PULONG quality)
{
    struct schan_transport transport;
    struct schan_context *ctx;
    SecBuffer *buffer;
    SIZE_T data_size;
    char *data;
    unsigned expected_size;
    ssize_t received = 0;
    ssize_t ret;
    int idx;
    unsigned char *buf_ptr;

    TRACE("context_handle %p, message %p, message_seq_no %d, quality %p\n",
            context_handle, message, message_seq_no, quality);

    if (!context_handle) return SEC_E_INVALID_HANDLE;
    ctx = schan_get_object(context_handle->dwLower, SCHAN_HANDLE_CTX);

    dump_buffer_desc(message);

    idx = schan_validate_decrypt_buffer_desc(message);
    if (idx == -1)
        return SEC_E_INVALID_TOKEN;
    buffer = &message->pBuffers[idx];
    buf_ptr = buffer->pvBuffer;

    expected_size = 5 + ((buf_ptr[3] << 8) | buf_ptr[4]);
    if(buffer->cbBuffer < expected_size)
    {
        TRACE("Expected %u bytes, but buffer only contains %u bytes\n", expected_size, buffer->cbBuffer);
        buffer->BufferType = SECBUFFER_MISSING;
        buffer->cbBuffer = expected_size - buffer->cbBuffer;

        /* This is a bit weird, but windows does it too */
        idx = schan_find_sec_buffer_idx(message, 0, SECBUFFER_EMPTY);
        buffer = &message->pBuffers[idx];
        buffer->BufferType = SECBUFFER_MISSING;
        buffer->cbBuffer = expected_size - buffer->cbBuffer;

        TRACE("Returning SEC_E_INCOMPLETE_MESSAGE\n");
        return SEC_E_INCOMPLETE_MESSAGE;
    }

    data_size = buffer->cbBuffer;
    data = HeapAlloc(GetProcessHeap(), 0, data_size);

    transport.ctx = ctx;
    init_schan_buffers(&transport.in, message, schan_decrypt_message_get_next_buffer);
    transport.in.limit = expected_size;
    init_schan_buffers(&transport.out, NULL, NULL);
    pgnutls_transport_set_ptr(ctx->session, (gnutls_transport_ptr_t)&transport);

    while (received < data_size)
    {
        ret = pgnutls_record_recv(ctx->session, data + received, data_size - received);
        if (ret < 0)
        {
            if (ret == GNUTLS_E_AGAIN)
            {
                if (!received)
                {
                    pgnutls_perror(ret);
                    HeapFree(GetProcessHeap(), 0, data);
                    TRACE("Returning SEC_E_INCOMPLETE_MESSAGE\n");
                    return SEC_E_INCOMPLETE_MESSAGE;
                }
                break;
            }
            else
            {
                pgnutls_perror(ret);
                HeapFree(GetProcessHeap(), 0, data);
                ERR("Returning SEC_E_INTERNAL_ERROR\n");
                return SEC_E_INTERNAL_ERROR;
            }
        }
        else if (!ret)
            break;

        received += ret;
    }

    TRACE("Received %zd bytes\n", received);

    memcpy(buf_ptr + 5, data, received);
    HeapFree(GetProcessHeap(), 0, data);

    schan_decrypt_fill_buffer(message, SECBUFFER_DATA,
        buf_ptr + 5, received);

    schan_decrypt_fill_buffer(message, SECBUFFER_STREAM_TRAILER,
        buf_ptr + 5 + received, buffer->cbBuffer - 5 - received);

    if(buffer->cbBuffer > expected_size)
        schan_decrypt_fill_buffer(message, SECBUFFER_EXTRA,
            buf_ptr + expected_size, buffer->cbBuffer - expected_size);

    buffer->BufferType = SECBUFFER_STREAM_HEADER;
    buffer->cbBuffer = 5;

    return SEC_E_OK;
}

static SECURITY_STATUS SEC_ENTRY schan_DeleteSecurityContext(PCtxtHandle context_handle)
{
    struct schan_context *ctx;

    TRACE("context_handle %p\n", context_handle);

    if (!context_handle) return SEC_E_INVALID_HANDLE;

    ctx = schan_free_handle(context_handle->dwLower, SCHAN_HANDLE_CTX);
    if (!ctx) return SEC_E_INVALID_HANDLE;

    pgnutls_deinit(ctx->session);
    HeapFree(GetProcessHeap(), 0, ctx);

    return SEC_E_OK;
}

static void schan_gnutls_log(int level, const char *msg)
{
    TRACE("<%d> %s", level, msg);
}

static const SecurityFunctionTableA schanTableA = {
    1,
    NULL, /* EnumerateSecurityPackagesA */
    schan_QueryCredentialsAttributesA,
    schan_AcquireCredentialsHandleA,
    schan_FreeCredentialsHandle,
    NULL, /* Reserved2 */
    schan_InitializeSecurityContextA, 
    NULL, /* AcceptSecurityContext */
    NULL, /* CompleteAuthToken */
    schan_DeleteSecurityContext,
    NULL, /* ApplyControlToken */
    schan_QueryContextAttributesA,
    NULL, /* ImpersonateSecurityContext */
    NULL, /* RevertSecurityContext */
    NULL, /* MakeSignature */
    NULL, /* VerifySignature */
    FreeContextBuffer,
    NULL, /* QuerySecurityPackageInfoA */
    NULL, /* Reserved3 */
    NULL, /* Reserved4 */
    NULL, /* ExportSecurityContext */
    NULL, /* ImportSecurityContextA */
    NULL, /* AddCredentialsA */
    NULL, /* Reserved8 */
    NULL, /* QuerySecurityContextToken */
    schan_EncryptMessage,
    schan_DecryptMessage,
    NULL, /* SetContextAttributesA */
};

static const SecurityFunctionTableW schanTableW = {
    1,
    NULL, /* EnumerateSecurityPackagesW */
    schan_QueryCredentialsAttributesW,
    schan_AcquireCredentialsHandleW,
    schan_FreeCredentialsHandle,
    NULL, /* Reserved2 */
    schan_InitializeSecurityContextW, 
    NULL, /* AcceptSecurityContext */
    NULL, /* CompleteAuthToken */
    schan_DeleteSecurityContext,
    NULL, /* ApplyControlToken */
    schan_QueryContextAttributesW,
    NULL, /* ImpersonateSecurityContext */
    NULL, /* RevertSecurityContext */
    NULL, /* MakeSignature */
    NULL, /* VerifySignature */
    FreeContextBuffer,
    NULL, /* QuerySecurityPackageInfoW */
    NULL, /* Reserved3 */
    NULL, /* Reserved4 */
    NULL, /* ExportSecurityContext */
    NULL, /* ImportSecurityContextW */
    NULL, /* AddCredentialsW */
    NULL, /* Reserved8 */
    NULL, /* QuerySecurityContextToken */
    schan_EncryptMessage,
    schan_DecryptMessage,
    NULL, /* SetContextAttributesW */
};

static const WCHAR schannelComment[] = { 'S','c','h','a','n','n','e','l',' ',
 'S','e','c','u','r','i','t','y',' ','P','a','c','k','a','g','e',0 };
static const WCHAR schannelDllName[] = { 's','c','h','a','n','n','e','l','.','d','l','l',0 };

void SECUR32_initSchannelSP(void)
{
    /* This is what Windows reports.  This shouldn't break any applications
     * even though the functions are missing, because the wrapper will
     * return SEC_E_UNSUPPORTED_FUNCTION if our function is NULL.
     */
    static const LONG caps =
        SECPKG_FLAG_INTEGRITY |
        SECPKG_FLAG_PRIVACY |
        SECPKG_FLAG_CONNECTION |
        SECPKG_FLAG_MULTI_REQUIRED |
        SECPKG_FLAG_EXTENDED_ERROR |
        SECPKG_FLAG_IMPERSONATION |
        SECPKG_FLAG_ACCEPT_WIN32_NAME |
        SECPKG_FLAG_STREAM;
    static const short version = 1;
    static const LONG maxToken = 16384;
    SEC_WCHAR *uniSPName = (SEC_WCHAR *)UNISP_NAME_W,
              *schannel = (SEC_WCHAR *)SCHANNEL_NAME_W;
    const SecPkgInfoW info[] = {
        { caps, version, UNISP_RPC_ID, maxToken, uniSPName, uniSPName },
        { caps, version, UNISP_RPC_ID, maxToken, schannel,
            (SEC_WCHAR *)schannelComment },
    };
    SecureProvider *provider;
    int ret;

    libgnutls_handle = wine_dlopen(SONAME_LIBGNUTLS, RTLD_NOW, NULL, 0);
    if (!libgnutls_handle)
    {
        WARN("Failed to load libgnutls.\n");
        return;
    }

#define LOAD_FUNCPTR(f) \
    if (!(p##f = wine_dlsym(libgnutls_handle, #f, NULL, 0))) \
    { \
        ERR("Failed to load %s\n", #f); \
        goto fail; \
    }

    LOAD_FUNCPTR(gnutls_alert_get)
    LOAD_FUNCPTR(gnutls_alert_get_name)
    LOAD_FUNCPTR(gnutls_certificate_allocate_credentials)
    LOAD_FUNCPTR(gnutls_certificate_free_credentials)
    LOAD_FUNCPTR(gnutls_certificate_get_peers)
    LOAD_FUNCPTR(gnutls_cipher_get)
    LOAD_FUNCPTR(gnutls_cipher_get_key_size)
    LOAD_FUNCPTR(gnutls_credentials_set)
    LOAD_FUNCPTR(gnutls_deinit)
    LOAD_FUNCPTR(gnutls_global_deinit)
    LOAD_FUNCPTR(gnutls_global_init)
    LOAD_FUNCPTR(gnutls_global_set_log_function)
    LOAD_FUNCPTR(gnutls_global_set_log_level)
    LOAD_FUNCPTR(gnutls_handshake)
    LOAD_FUNCPTR(gnutls_init)
    LOAD_FUNCPTR(gnutls_kx_get)
    LOAD_FUNCPTR(gnutls_mac_get)
    LOAD_FUNCPTR(gnutls_mac_get_key_size)
    LOAD_FUNCPTR(gnutls_perror)
    LOAD_FUNCPTR(gnutls_protocol_get_version)
    LOAD_FUNCPTR(gnutls_set_default_priority)
    LOAD_FUNCPTR(gnutls_record_recv);
    LOAD_FUNCPTR(gnutls_record_send);
    LOAD_FUNCPTR(gnutls_transport_set_errno)
    LOAD_FUNCPTR(gnutls_transport_set_ptr)
    LOAD_FUNCPTR(gnutls_transport_set_pull_function)
    LOAD_FUNCPTR(gnutls_transport_set_push_function)
#undef LOAD_FUNCPTR

    ret = pgnutls_global_init();
    if (ret != GNUTLS_E_SUCCESS)
    {
        pgnutls_perror(ret);
        goto fail;
    }

    if (TRACE_ON(secur32))
    {
        pgnutls_global_set_log_level(4);
        pgnutls_global_set_log_function(schan_gnutls_log);
    }

    schan_handle_table = HeapAlloc(GetProcessHeap(), 0, 64 * sizeof(*schan_handle_table));
    if (!schan_handle_table)
    {
        ERR("Failed to allocate schannel handle table.\n");
        goto fail;
    }
    schan_handle_table_size = 64;

    provider = SECUR32_addProvider(&schanTableA, &schanTableW, schannelDllName);
    if (!provider)
    {
        ERR("Failed to add schannel provider.\n");
        goto fail;
    }

    SECUR32_addPackages(provider, sizeof(info) / sizeof(info[0]), NULL, info);

    return;

fail:
    HeapFree(GetProcessHeap(), 0, schan_handle_table);
    schan_handle_table = NULL;
    wine_dlclose(libgnutls_handle, NULL, 0);
    libgnutls_handle = NULL;
    return;
}

void SECUR32_deinitSchannelSP(void)
{
    SIZE_T i = schan_handle_count;

    if (!libgnutls_handle) return;

    /* deinitialized sessions first because a pointer to the credentials
     * are stored for the session by calling gnutls_credentials_set. */
    while (i--)
    {
        if (schan_handle_table[i].type == SCHAN_HANDLE_CTX)
        {
            struct schan_context *ctx = schan_free_handle(i, SCHAN_HANDLE_CTX);
            pgnutls_deinit(ctx->session);
            HeapFree(GetProcessHeap(), 0, ctx);
        }
    }
    i = schan_handle_count;
    while (i--)
    {
        if (schan_handle_table[i].type != SCHAN_HANDLE_FREE)
        {
            struct schan_credentials *cred;
            cred = schan_free_handle(i, SCHAN_HANDLE_CRED);
            pgnutls_certificate_free_credentials(cred->credentials);
            HeapFree(GetProcessHeap(), 0, cred);
        }
    }
    HeapFree(GetProcessHeap(), 0, schan_handle_table);
    pgnutls_global_deinit();
    wine_dlclose(libgnutls_handle, NULL, 0);
}

#else /* SONAME_LIBGNUTLS */

void SECUR32_initSchannelSP(void)
{
    ERR("libgnutls not found, SSL connections will fail\n");
}

void SECUR32_deinitSchannelSP(void) {}

#endif /* SONAME_LIBGNUTLS */
