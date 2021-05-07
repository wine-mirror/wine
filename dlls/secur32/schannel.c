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
 */

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winreg.h"
#include "winnls.h"
#include "lmcons.h"
#include "sspi.h"
#include "schannel.h"

#include "wine/debug.h"
#include "secur32_priv.h"

WINE_DEFAULT_DEBUG_CHANNEL(secur32);

const struct schan_funcs *schan_funcs = NULL;

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

struct schan_context
{
    schan_session session;
    struct schan_transport transport;
    ULONG req_ctx_attr;
    const CERT_CONTEXT *cert;
    SIZE_T header_size;
};

static struct schan_handle *schan_handle_table;
static struct schan_handle *schan_free_handles;
static SIZE_T schan_handle_table_size;
static SIZE_T schan_handle_count;

/* Protocols enabled, only those may be used for the connection. */
static DWORD config_enabled_protocols;

/* Protocols disabled by default. They are enabled for using, but disabled when caller asks for default settings. */
static DWORD config_default_disabled_protocols;

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
        struct schan_handle *new_table = realloc(schan_handle_table, new_size * sizeof(*schan_handle_table));
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

static void read_config(void)
{
    DWORD enabled = 0, default_disabled = 0;
    HKEY protocols_key, key;
    WCHAR subkey_name[64];
    unsigned i;
    DWORD res;

    static BOOL config_read = FALSE;
    static const struct {
        WCHAR key_name[20];
        DWORD prot_client_flag;
        BOOL enabled; /* If no config is present, enable the protocol */
        BOOL disabled_by_default; /* Disable if caller asks for default protocol set */
    } protocol_config_keys[] = {
        { L"SSL 2.0", SP_PROT_SSL2_CLIENT, FALSE, TRUE }, /* NOTE: TRUE, TRUE on Windows */
        { L"SSL 3.0", SP_PROT_SSL3_CLIENT, TRUE, FALSE },
        { L"TLS 1.0", SP_PROT_TLS1_0_CLIENT, TRUE, FALSE },
        { L"TLS 1.1", SP_PROT_TLS1_1_CLIENT, TRUE, FALSE /* NOTE: not enabled by default on Windows */ },
        { L"TLS 1.2", SP_PROT_TLS1_2_CLIENT, TRUE, FALSE /* NOTE: not enabled by default on Windows */ },
        { L"DTLS 1.0", SP_PROT_DTLS1_0_CLIENT, TRUE, TRUE },
        { L"DTLS 1.2", SP_PROT_DTLS1_2_CLIENT, TRUE, TRUE },
    };

    /* No need for thread safety */
    if(config_read)
        return;

    res = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"SYSTEM\\CurrentControlSet\\Control\\SecurityProviders\\SCHANNEL\\Protocols", 0, KEY_READ,
                        &protocols_key);
    if(res == ERROR_SUCCESS) {
        DWORD type, size, value;

        for(i = 0; i < ARRAY_SIZE(protocol_config_keys); i++) {
            wcscpy(subkey_name, protocol_config_keys[i].key_name);
            wcscat(subkey_name, L"\\Client");
            res = RegOpenKeyExW(protocols_key, subkey_name, 0, KEY_READ, &key);
            if(res != ERROR_SUCCESS) {
                if(protocol_config_keys[i].enabled)
                    enabled |= protocol_config_keys[i].prot_client_flag;
                if(protocol_config_keys[i].disabled_by_default)
                    default_disabled |= protocol_config_keys[i].prot_client_flag;
                continue;
            }

            size = sizeof(value);
            res = RegQueryValueExW(key, L"enabled", NULL, &type, (BYTE *)&value, &size);
            if(res == ERROR_SUCCESS) {
                if(type == REG_DWORD && value)
                    enabled |= protocol_config_keys[i].prot_client_flag;
            }else if(protocol_config_keys[i].enabled) {
                enabled |= protocol_config_keys[i].prot_client_flag;
            }

            size = sizeof(value);
            res = RegQueryValueExW(key, L"DisabledByDefault", NULL, &type, (BYTE *)&value, &size);
            if(res == ERROR_SUCCESS) {
                if(type != REG_DWORD || value)
                    default_disabled |= protocol_config_keys[i].prot_client_flag;
            }else if(protocol_config_keys[i].disabled_by_default) {
                default_disabled |= protocol_config_keys[i].prot_client_flag;
            }

            RegCloseKey(key);
        }
    }else {
        /* No config, enable all known protocols. */
        for(i = 0; i < ARRAY_SIZE(protocol_config_keys); i++) {
            if(protocol_config_keys[i].enabled)
                enabled |= protocol_config_keys[i].prot_client_flag;
            if(protocol_config_keys[i].disabled_by_default)
                default_disabled |= protocol_config_keys[i].prot_client_flag;
        }
    }

    RegCloseKey(protocols_key);

    config_enabled_protocols = enabled & schan_funcs->get_enabled_protocols();
    config_default_disabled_protocols = default_disabled;
    config_read = TRUE;

    TRACE("enabled %x, disabled by default %x\n", config_enabled_protocols, config_default_disabled_protocols);
}

static SECURITY_STATUS schan_QueryCredentialsAttributes(
 PCredHandle phCredential, ULONG ulAttribute, VOID *pBuffer)
{
    struct schan_credentials *cred;
    SECURITY_STATUS ret;

    cred = schan_get_object(phCredential->dwLower, SCHAN_HANDLE_CRED);
    if(!cred)
        return SEC_E_INVALID_HANDLE;

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
        if(pBuffer) {
            /* Regardless of MSDN documentation, tests show that this attribute takes into account
             * what protocols are enabled for given credential. */
            ((SecPkgCred_SupportedProtocols*)pBuffer)->grbitProtocol = cred->enabled_protocols;
            ret = SEC_E_OK;
        }else {
            ret = SEC_E_INTERNAL_ERROR;
        }
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

static SECURITY_STATUS get_cert(const SCHANNEL_CRED *cred, CERT_CONTEXT const **cert)
{
    SECURITY_STATUS status;
    DWORD i;

    TRACE("dwVersion = %u\n", cred->dwVersion);
    TRACE("cCreds = %u\n", cred->cCreds);
    TRACE("paCred = %p\n", cred->paCred);
    TRACE("hRootStore = %p\n", cred->hRootStore);
    TRACE("cMappers = %u\n", cred->cMappers);
    TRACE("cSupportedAlgs = %u:\n", cred->cSupportedAlgs);
    for (i = 0; i < cred->cSupportedAlgs; i++) TRACE("%08x\n", cred->palgSupportedAlgs[i]);
    TRACE("grbitEnabledProtocols = %08x\n", cred->grbitEnabledProtocols);
    TRACE("dwMinimumCipherStrength = %u\n", cred->dwMinimumCipherStrength);
    TRACE("dwMaximumCipherStrength = %u\n", cred->dwMaximumCipherStrength);
    TRACE("dwSessionLifespan = %u\n", cred->dwSessionLifespan);
    TRACE("dwFlags = %08x\n", cred->dwFlags);
    TRACE("dwCredFormat = %u\n", cred->dwCredFormat);

    switch (cred->dwVersion)
    {
    case SCH_CRED_V3:
    case SCHANNEL_CRED_VERSION:
        break;
    default:
        return SEC_E_INTERNAL_ERROR;
    }

    if (!cred->cCreds) status = SEC_E_NO_CREDENTIALS;
    else if (cred->cCreds > 1) status = SEC_E_UNKNOWN_CREDENTIALS;
    else
    {
        DWORD spec;
        HCRYPTPROV prov;
        BOOL free;

        if (CryptAcquireCertificatePrivateKey(cred->paCred[0], CRYPT_ACQUIRE_CACHE_FLAG, NULL, &prov, &spec, &free))
        {
            if (free) CryptReleaseContext(prov, 0);
            *cert = cred->paCred[0];
            status = SEC_E_OK;
        }
        else status = SEC_E_UNKNOWN_CREDENTIALS;
    }

    return status;
}

static WCHAR *get_key_container_path(const CERT_CONTEXT *ctx)
{
    CERT_KEY_CONTEXT keyctx;
    DWORD size = sizeof(keyctx), prov_size = 0;
    CRYPT_KEY_PROV_INFO *prov;
    WCHAR username[UNLEN + 1], *ret = NULL;
    DWORD len = ARRAY_SIZE(username);

    if (CertGetCertificateContextProperty(ctx, CERT_KEY_CONTEXT_PROP_ID, &keyctx, &size))
    {
        char *str;
        if (!CryptGetProvParam(keyctx.hCryptProv, PP_CONTAINER, NULL, &size, 0)) return NULL;
        if (!(str = RtlAllocateHeap(GetProcessHeap(), 0, size))) return NULL;
        if (!CryptGetProvParam(keyctx.hCryptProv, PP_CONTAINER, (BYTE *)str, &size, 0)) return NULL;

        len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
        if (!(ret = RtlAllocateHeap(GetProcessHeap(), 0, sizeof(L"Software\\Wine\\Crypto\\RSA\\") + len * sizeof(WCHAR))))
        {
            RtlFreeHeap(GetProcessHeap(), 0, str);
            return NULL;
        }
        wcscpy(ret, L"Software\\Wine\\Crypto\\RSA\\");
        MultiByteToWideChar(CP_ACP, 0, str, -1, ret + wcslen(ret), len);
        RtlFreeHeap(GetProcessHeap(), 0, str);
    }
    else if (CertGetCertificateContextProperty(ctx, CERT_KEY_PROV_INFO_PROP_ID, NULL, &prov_size))
    {
        if (!(prov = RtlAllocateHeap(GetProcessHeap(), 0, prov_size))) return NULL;
        if (!CertGetCertificateContextProperty(ctx, CERT_KEY_PROV_INFO_PROP_ID, prov, &prov_size))
        {
            RtlFreeHeap(GetProcessHeap(), 0, prov);
            return NULL;
        }
        if (!(ret = RtlAllocateHeap(GetProcessHeap(), 0,
                                    sizeof(L"Software\\Wine\\Crypto\\RSA\\") + wcslen(prov->pwszContainerName) * sizeof(WCHAR))))
        {
            RtlFreeHeap(GetProcessHeap(), 0, prov);
            return NULL;
        }
        wcscpy(ret, L"Software\\Wine\\Crypto\\RSA\\");
        wcscat(ret, prov->pwszContainerName);
        RtlFreeHeap(GetProcessHeap(), 0, prov);
    }

    if (!ret && GetUserNameW(username, &len) &&
        (ret = RtlAllocateHeap(GetProcessHeap(), 0, sizeof(L"Software\\Wine\\Crypto\\RSA\\") + len * sizeof(WCHAR))))
    {
        wcscpy(ret, L"Software\\Wine\\Crypto\\RSA\\");
        wcscat(ret, username);
    }

    return ret;
}

#define MAX_LEAD_BYTES 8
static BYTE *get_key_blob(const CERT_CONTEXT *ctx, DWORD *size)
{
    BYTE *buf, *ret = NULL;
    DATA_BLOB blob_in, blob_out;
    DWORD spec = 0, type, len;
    WCHAR *path;
    HKEY hkey;

    if (!(path = get_key_container_path(ctx))) return NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, path, 0, KEY_READ, &hkey))
    {
        RtlFreeHeap(GetProcessHeap(), 0, path);
        return NULL;
    }
    RtlFreeHeap(GetProcessHeap(), 0, path);

    if (!RegQueryValueExW(hkey, L"KeyExchangeKeyPair", 0, &type, NULL, &len)) spec = AT_KEYEXCHANGE;
    else if (!RegQueryValueExW(hkey, L"SignatureKeyPair", 0, &type, NULL, &len)) spec = AT_SIGNATURE;
    else
    {
        RegCloseKey(hkey);
        return NULL;
    }

    if (!(buf = RtlAllocateHeap(GetProcessHeap(), 0, len + MAX_LEAD_BYTES)))
    {
        RegCloseKey(hkey);
        return NULL;
    }

    if (!RegQueryValueExW(hkey, (spec == AT_KEYEXCHANGE) ? L"KeyExchangeKeyPair" : L"SignatureKeyPair", 0,
                          &type, buf, &len))
    {
        blob_in.pbData = buf;
        blob_in.cbData = len;
        if (CryptUnprotectData(&blob_in, NULL, NULL, NULL, NULL, 0, &blob_out))
        {
            assert(blob_in.cbData >= blob_out.cbData);
            memcpy(buf, blob_out.pbData, blob_out.cbData);
            LocalFree(blob_out.pbData);
            *size = blob_out.cbData + MAX_LEAD_BYTES;
            ret = buf;
        }
    }
    else RtlFreeHeap(GetProcessHeap(), 0, buf);

    RegCloseKey(hkey);
    return ret;
}

static SECURITY_STATUS schan_AcquireClientCredentials(const SCHANNEL_CRED *schanCred,
 PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    struct schan_credentials *creds;
    unsigned enabled_protocols;
    ULONG_PTR handle;
    SECURITY_STATUS status = SEC_E_OK;
    const CERT_CONTEXT *cert = NULL;
    DATA_BLOB key_blob = {0};

    TRACE("schanCred %p, phCredential %p, ptsExpiry %p\n", schanCred, phCredential, ptsExpiry);

    if (schanCred)
    {
        const unsigned dtls_protocols = SP_PROT_DTLS_CLIENT | SP_PROT_DTLS1_2_CLIENT;
        const unsigned tls_protocols = SP_PROT_TLS1_CLIENT | SP_PROT_TLS1_0_CLIENT | SP_PROT_TLS1_1_CLIENT |
                                       SP_PROT_TLS1_2_CLIENT | SP_PROT_TLS1_3_CLIENT;

        status = get_cert(schanCred, &cert);
        if (status != SEC_E_OK && status != SEC_E_NO_CREDENTIALS)
            return status;

        if ((schanCred->grbitEnabledProtocols & tls_protocols) &&
            (schanCred->grbitEnabledProtocols & dtls_protocols)) return SEC_E_ALGORITHM_MISMATCH;

        status = SEC_E_OK;
    }

    read_config();
    if(schanCred && schanCred->grbitEnabledProtocols)
        enabled_protocols = schanCred->grbitEnabledProtocols & config_enabled_protocols;
    else
        enabled_protocols = config_enabled_protocols & ~config_default_disabled_protocols;
    if(!enabled_protocols) {
        ERR("Could not find matching protocol\n");
        return SEC_E_NO_AUTHENTICATING_AUTHORITY;
    }

    if (!(creds = malloc(sizeof(*creds)))) return SEC_E_INSUFFICIENT_MEMORY;
    creds->credential_use = SECPKG_CRED_OUTBOUND;
    creds->enabled_protocols = enabled_protocols;

    if (cert && !(key_blob.pbData = get_key_blob(cert, &key_blob.cbData))) goto fail;
    if (!schan_funcs->allocate_certificate_credentials(creds, cert, &key_blob)) goto fail;
    RtlFreeHeap(GetProcessHeap(), 0, key_blob.pbData);

    handle = schan_alloc_handle(creds, SCHAN_HANDLE_CRED);
    if (handle == SCHAN_INVALID_HANDLE) goto fail;

    phCredential->dwLower = handle;
    phCredential->dwUpper = 0;

    /* Outbound credentials have no expiry */
    if (ptsExpiry)
    {
        ptsExpiry->LowPart = 0;
        ptsExpiry->HighPart = 0;
    }

    return status;

fail:
    free(creds);
    RtlFreeHeap(GetProcessHeap(), 0, key_blob.pbData);
    return SEC_E_INTERNAL_ERROR;
}

static SECURITY_STATUS schan_AcquireServerCredentials(const SCHANNEL_CRED *schanCred,
 PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS status;
    const CERT_CONTEXT *cert = NULL;

    TRACE("schanCred %p, phCredential %p, ptsExpiry %p\n", schanCred, phCredential, ptsExpiry);

    if (!schanCred) return SEC_E_NO_CREDENTIALS;

    status = get_cert(schanCred, &cert);
    if (status == SEC_E_OK)
    {
        ULONG_PTR handle;
        struct schan_credentials *creds;

        if (!(creds = calloc(1, sizeof(*creds)))) return SEC_E_INSUFFICIENT_MEMORY;
        creds->credential_use = SECPKG_CRED_INBOUND;

        handle = schan_alloc_handle(creds, SCHAN_HANDLE_CRED);
        if (handle == SCHAN_INVALID_HANDLE)
        {
            free(creds);
            return SEC_E_INTERNAL_ERROR;
        }

        phCredential->dwLower = handle;
        phCredential->dwUpper = 0;

        /* FIXME: get expiry from cert */
    }
    return status;
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

    if (creds->credential_use == SECPKG_CRED_OUTBOUND) schan_funcs->free_certificate_credentials(creds);
    free(creds);
    return SEC_E_OK;
}

static void init_schan_buffers(struct schan_buffers *s, const PSecBufferDesc desc,
        int (*get_next_buffer)(const struct schan_transport *, struct schan_buffers *))
{
    s->offset = 0;
    s->limit = ~0UL;
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

    if (b->pvBuffer) /* freed with FreeContextBuffer */
        new_data = RtlReAllocateHeap(GetProcessHeap(), 0, b->pvBuffer, new_size);
    else
        new_data = RtlAllocateHeap(GetProcessHeap(), 0, new_size);

    if (!new_data)
    {
        TRACE("Failed to resize %p from %d to %ld\n", b->pvBuffer, b->cbBuffer, new_size);
        return;
    }

    b->cbBuffer = new_size;
    b->pvBuffer = new_data;
}

static char * CDECL schan_get_buffer(const struct schan_transport *t, struct schan_buffers *s, SIZE_T *count)
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
    if (s->limit != ~0UL && s->limit < max_count)
        max_count = s->limit;
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

    if (*count > max_count)
        *count = max_count;
    if (s->limit != ~0UL)
        s->limit -= *count;

    return (char *)buffer->pvBuffer + s->offset;
}

/* schan_pull
 *      Read data from the transport input buffer.
 *
 * t - The session transport object.
 * buff - The buffer into which to store the read data.  Must be at least
 *        *buff_len bytes in length.
 * buff_len - On input, *buff_len is the desired length to read.  On successful
 *            return, *buff_len is the number of bytes actually read.
 *
 * Returns:
 *  0 on success, in which case:
 *      *buff_len == 0 indicates end of file.
 *      *buff_len > 0 indicates that some data was read.  May be less than
 *          what was requested, in which case the caller should call again if/
 *          when they want more.
 *  EAGAIN when no data could be read without blocking
 *  another errno-style error value on failure
 *
 */
static int CDECL schan_pull(struct schan_transport *t, void *buff, size_t *buff_len)
{
    char *b;
    SIZE_T local_len = *buff_len;

    TRACE("Pull %lu bytes\n", local_len);

    *buff_len = 0;

    b = schan_get_buffer(t, &t->in, &local_len);
    if (!b)
        return EAGAIN;

    memcpy(buff, b, local_len);
    t->in.offset += local_len;

    TRACE("Read %lu bytes\n", local_len);

    *buff_len = local_len;
    return 0;
}

/* schan_push
 *      Write data to the transport output buffer.
 *
 * t - The session transport object.
 * buff - The buffer of data to write.  Must be at least *buff_len bytes in length.
 * buff_len - On input, *buff_len is the desired length to write.  On successful
 *            return, *buff_len is the number of bytes actually written.
 *
 * Returns:
 *  0 on success
 *      *buff_len will be > 0 indicating how much data was written.  May be less
 *          than what was requested, in which case the caller should call again
            if/when they want to write more.
 *  EAGAIN when no data could be written without blocking
 *  another errno-style error value on failure
 *
 */
static int CDECL schan_push(struct schan_transport *t, const void *buff, size_t *buff_len)
{
    char *b;
    SIZE_T local_len = *buff_len;

    TRACE("Push %lu bytes\n", local_len);

    *buff_len = 0;

    b = schan_get_buffer(t, &t->out, &local_len);
    if (!b)
        return EAGAIN;

    memcpy(b, buff, local_len);
    t->out.offset += local_len;

    TRACE("Wrote %lu bytes\n", local_len);

    *buff_len = local_len;
    return 0;
}

static schan_session CDECL schan_get_session_for_transport(struct schan_transport* t)
{
    return t->ctx->session;
}

static int schan_init_sec_ctx_get_next_input_buffer(const struct schan_transport *t, struct schan_buffers *s)
{
    if (s->current_buffer_idx != -1)
        return -1;
    return schan_find_sec_buffer_idx(s->desc, 0, SECBUFFER_TOKEN);
}

static int schan_init_sec_ctx_get_next_output_buffer(const struct schan_transport *t, struct schan_buffers *s)
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

#define HEADER_SIZE_TLS  5
#define HEADER_SIZE_DTLS 13

static inline SIZE_T read_record_size(const BYTE *buf, SIZE_T header_size)
{
    return (buf[header_size - 2] << 8) | buf[header_size - 1];
}

static inline BOOL is_dtls_context(const struct schan_context *ctx)
{
    return (ctx->header_size == HEADER_SIZE_DTLS);
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
    SIZE_T expected_size = ~0UL;
    SECURITY_STATUS ret;
    SecBuffer *buffer;
    int idx;

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

        if (!(ctx = malloc(sizeof(*ctx)))) return SEC_E_INSUFFICIENT_MEMORY;

        ctx->cert = NULL;
        handle = schan_alloc_handle(ctx, SCHAN_HANDLE_CTX);
        if (handle == SCHAN_INVALID_HANDLE)
        {
            free(ctx);
            return SEC_E_INTERNAL_ERROR;
        }

        if (!schan_funcs->create_session(&ctx->session, cred))
        {
            schan_free_handle(handle, SCHAN_HANDLE_CTX);
            free(ctx);
            return SEC_E_INTERNAL_ERROR;
        }

        if (cred->enabled_protocols & (SP_PROT_DTLS1_0_CLIENT | SP_PROT_DTLS1_2_CLIENT))
            ctx->header_size = HEADER_SIZE_DTLS;
        else
            ctx->header_size = HEADER_SIZE_TLS;

        ctx->transport.ctx = ctx;
        schan_funcs->set_session_transport(ctx->session, &ctx->transport);

        if (pszTargetName && *pszTargetName)
        {
            UINT len = WideCharToMultiByte( CP_UNIXCP, 0, pszTargetName, -1, NULL, 0, NULL, NULL );
            char *target = malloc( len );

            if (target)
            {
                WideCharToMultiByte( CP_UNIXCP, 0, pszTargetName, -1, target, len, NULL, NULL );
                schan_funcs->set_session_target( ctx->session, target );
                free( target );
            }
        }

        if (pInput && (idx = schan_find_sec_buffer_idx(pInput, 0, SECBUFFER_APPLICATION_PROTOCOLS)) != -1)
        {
            buffer = &pInput->pBuffers[idx];
            schan_funcs->set_application_protocols(ctx->session, buffer->pvBuffer, buffer->cbBuffer);
        }

        if (pInput && (idx = schan_find_sec_buffer_idx(pInput, 0, SECBUFFER_DTLS_MTU)) != -1)
        {
            buffer = &pInput->pBuffers[idx];
            if (buffer->cbBuffer >= sizeof(WORD)) schan_funcs->set_dtls_mtu(ctx->session, *(WORD *)buffer->pvBuffer);
            else WARN("invalid buffer size %u\n", buffer->cbBuffer);
        }

        phNewContext->dwLower = handle;
        phNewContext->dwUpper = 0;
    }
    else
    {
        SIZE_T record_size = 0;
        unsigned char *ptr;

        ctx = schan_get_object(phContext->dwLower, SCHAN_HANDLE_CTX);
        if (pInput)
        {
            idx = schan_find_sec_buffer_idx(pInput, 0, SECBUFFER_TOKEN);
            if (idx == -1)
                return SEC_E_INCOMPLETE_MESSAGE;

            buffer = &pInput->pBuffers[idx];
            ptr = buffer->pvBuffer;
            expected_size = 0;

            while (buffer->cbBuffer > expected_size + ctx->header_size)
            {
                record_size = ctx->header_size + read_record_size(ptr, ctx->header_size);

                if (buffer->cbBuffer < expected_size + record_size)
                    break;

                expected_size += record_size;
                ptr += record_size;
            }

            if (!expected_size)
            {
                TRACE("Expected at least %lu bytes, but buffer only contains %u bytes.\n",
                      max(6, record_size), buffer->cbBuffer);
                return SEC_E_INCOMPLETE_MESSAGE;
            }
        }
        else if (!is_dtls_context(ctx)) return SEC_E_INCOMPLETE_MESSAGE;

        TRACE("Using expected_size %lu.\n", expected_size);
    }

    ctx->req_ctx_attr = fContextReq;

    init_schan_buffers(&ctx->transport.in, pInput, schan_init_sec_ctx_get_next_input_buffer);
    ctx->transport.in.limit = expected_size;
    init_schan_buffers(&ctx->transport.out, pOutput, schan_init_sec_ctx_get_next_output_buffer);

    /* Perform the TLS handshake */
    ret = schan_funcs->handshake(ctx->session);

    out_buffers = &ctx->transport.out;
    if (out_buffers->current_buffer_idx != -1)
    {
        SecBuffer *buffer = &out_buffers->desc->pBuffers[out_buffers->current_buffer_idx];
        buffer->cbBuffer = out_buffers->offset;
    }
    else if (out_buffers->desc && out_buffers->desc->cBuffers > 0)
    {
        SecBuffer *buffer = &out_buffers->desc->pBuffers[0];
        buffer->cbBuffer = 0;
    }

    if(ctx->transport.in.offset && ctx->transport.in.offset != pInput->pBuffers[0].cbBuffer) {
        if(pInput->cBuffers<2 || pInput->pBuffers[1].BufferType!=SECBUFFER_EMPTY)
            return SEC_E_INVALID_TOKEN;

        pInput->pBuffers[1].BufferType = SECBUFFER_EXTRA;
        pInput->pBuffers[1].cbBuffer = pInput->pBuffers[0].cbBuffer-ctx->transport.in.offset;
    }

    *pfContextAttr = ISC_RET_REPLAY_DETECT | ISC_RET_SEQUENCE_DETECT | ISC_RET_CONFIDENTIALITY | ISC_RET_STREAM;
    if (ctx->req_ctx_attr & ISC_REQ_EXTENDED_ERROR) *pfContextAttr |= ISC_RET_EXTENDED_ERROR;
    if (ctx->req_ctx_attr & ISC_REQ_DATAGRAM) *pfContextAttr |= ISC_RET_DATAGRAM;
    if (ctx->req_ctx_attr & ISC_REQ_ALLOCATE_MEMORY) *pfContextAttr |= ISC_RET_ALLOCATED_MEMORY;
    if (ctx->req_ctx_attr & ISC_REQ_USE_SUPPLIED_CREDS) *pfContextAttr |= ISC_RET_USED_SUPPLIED_CREDS;
    if (ctx->req_ctx_attr & ISC_REQ_MANUAL_CRED_VALIDATION) *pfContextAttr |= ISC_RET_MANUAL_CRED_VALIDATION;

    return ret;
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
        if (!(target_name = malloc(len * sizeof(*target_name)))) return SEC_E_INSUFFICIENT_MEMORY;
        MultiByteToWideChar(CP_ACP, 0, pszTargetName, -1, target_name, len);
    }

    ret = schan_InitializeSecurityContextW(phCredential, phContext, target_name,
            fContextReq, Reserved1, TargetDataRep, pInput, Reserved2,
            phNewContext, pOutput, pfContextAttr, ptsExpiry);

    free(target_name);
    return ret;
}

static void *get_alg_name(ALG_ID id, BOOL wide)
{
    static const struct {
        ALG_ID alg_id;
        const char* name;
        const WCHAR nameW[8];
    } alg_name_map[] = {
        { CALG_ECDSA,      "ECDSA", L"ECDSA" },
        { CALG_RSA_SIGN,   "RSA",   L"RSA" },
        { CALG_DES,        "DES",   L"DES" },
        { CALG_RC2,        "RC2",   L"RC2" },
        { CALG_3DES,       "3DES",  L"3DES" },
        { CALG_AES_128,    "AES",   L"AES" },
        { CALG_AES_192,    "AES",   L"AES" },
        { CALG_AES_256,    "AES",   L"AES" },
        { CALG_RC4,        "RC4",   L"RC4" },
    };
    unsigned i;

    for (i = 0; i < ARRAY_SIZE(alg_name_map); i++)
        if (alg_name_map[i].alg_id == id)
            return wide ? (void*)alg_name_map[i].nameW : (void*)alg_name_map[i].name;

    FIXME("Unknown ALG_ID %04x\n", id);
    return NULL;
}

static SECURITY_STATUS ensure_remote_cert(struct schan_context *ctx)
{
    HCERTSTORE store;
    PCCERT_CONTEXT cert = NULL;
    SECURITY_STATUS status;
    struct schan_cert_list list;

    if (ctx->cert) return SEC_E_OK;
    if (!(store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0, CERT_STORE_CREATE_NEW_FLAG, NULL)))
        return GetLastError();

    if ((status = schan_funcs->get_session_peer_certificate(ctx->session, &list)) == SEC_E_OK)
    {
        unsigned int i;
        for (i = 0; i < list.count; i++)
        {
            if (!CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING, list.certs[i].pbData,
                                                  list.certs[i].cbData, CERT_STORE_ADD_REPLACE_EXISTING,
                                                  i ? NULL : &cert))
            {
                if (i) CertFreeCertificateContext(cert);
                return GetLastError();
            }
        }
        RtlFreeHeap(GetProcessHeap(), 0, list.certs);
    }

    ctx->cert = cert;
    CertCloseStore(store, 0);
    return status;
}

static SECURITY_STATUS SEC_ENTRY schan_QueryContextAttributesW(
        PCtxtHandle context_handle, ULONG attribute, PVOID buffer)
{
    struct schan_context *ctx;
    SECURITY_STATUS status;

    TRACE("context_handle %p, attribute %#x, buffer %p\n",
            context_handle, attribute, buffer);

    if (!context_handle) return SEC_E_INVALID_HANDLE;
    ctx = schan_get_object(context_handle->dwLower, SCHAN_HANDLE_CTX);

    switch(attribute)
    {
        case SECPKG_ATTR_STREAM_SIZES:
        {
            SecPkgContext_ConnectionInfo info;
            status = schan_funcs->get_connection_info(ctx->session, &info);
            if (status == SEC_E_OK)
            {
                SecPkgContext_StreamSizes *stream_sizes = buffer;
                SIZE_T mac_size = info.dwHashStrength;
                unsigned int block_size = schan_funcs->get_session_cipher_block_size(ctx->session);
                unsigned int message_size = schan_funcs->get_max_message_size(ctx->session);

                TRACE("Using header size %lu mac bytes %lu, message size %u, block size %u\n",
                      ctx->header_size, mac_size, message_size, block_size);

                /* These are defined by the TLS RFC */
                stream_sizes->cbHeader = ctx->header_size;
                stream_sizes->cbTrailer = mac_size + 256; /* Max 255 bytes padding + 1 for padding size */
                stream_sizes->cbMaximumMessage = message_size;
                stream_sizes->cbBuffers = 4;
                stream_sizes->cbBlockSize = block_size;
            }

            return status;
        }
        case SECPKG_ATTR_KEY_INFO:
        {
            SecPkgContext_ConnectionInfo conn_info;
            status = schan_funcs->get_connection_info(ctx->session, &conn_info);
            if (status == SEC_E_OK)
            {
                SecPkgContext_KeyInfoW *info = buffer;
                info->KeySize = conn_info.dwCipherStrength;
                info->SignatureAlgorithm = schan_funcs->get_key_signature_algorithm(ctx->session);
                info->EncryptAlgorithm = conn_info.aiCipher;
                info->sSignatureAlgorithmName = get_alg_name(info->SignatureAlgorithm, TRUE);
                info->sEncryptAlgorithmName = get_alg_name(info->EncryptAlgorithm, TRUE);
            }
            return status;
        }
        case SECPKG_ATTR_REMOTE_CERT_CONTEXT:
        {
            PCCERT_CONTEXT *cert = buffer;

            status = ensure_remote_cert(ctx);
            if(status != SEC_E_OK)
                return status;

            *cert = CertDuplicateCertificateContext(ctx->cert);
            return SEC_E_OK;
        }
        case SECPKG_ATTR_CONNECTION_INFO:
        {
            SecPkgContext_ConnectionInfo *info = buffer;
            return schan_funcs->get_connection_info(ctx->session, info);
        }
        case SECPKG_ATTR_ENDPOINT_BINDINGS:
        {
            SecPkgContext_Bindings *bindings = buffer;
            CCRYPT_OID_INFO *info;
            ALG_ID hash_alg = CALG_SHA_256;
            BYTE hash[1024];
            DWORD hash_size;
            char *p;
            BOOL r;

            static const char prefix[] = "tls-server-end-point:";

            status = ensure_remote_cert(ctx);
            if(status != SEC_E_OK)
                return status;

            /* RFC 5929 */
            info = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY, ctx->cert->pCertInfo->SignatureAlgorithm.pszObjId, 0);
            if(info && info->u.Algid != CALG_SHA1 && info->u.Algid != CALG_MD5)
                hash_alg = info->u.Algid;

            hash_size = sizeof(hash);
            r = CryptHashCertificate(0, hash_alg, 0, ctx->cert->pbCertEncoded, ctx->cert->cbCertEncoded, hash, &hash_size);
            if(!r)
                return GetLastError();

            bindings->BindingsLength = sizeof(*bindings->Bindings) + sizeof(prefix)-1 + hash_size;
            /* freed with FreeContextBuffer */
            bindings->Bindings = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, bindings->BindingsLength);
            if(!bindings->Bindings)
                return SEC_E_INSUFFICIENT_MEMORY;

            bindings->Bindings->cbApplicationDataLength = sizeof(prefix)-1 + hash_size;
            bindings->Bindings->dwApplicationDataOffset = sizeof(*bindings->Bindings);

            p = (char*)(bindings->Bindings+1);
            memcpy(p, prefix, sizeof(prefix)-1);
            p += sizeof(prefix)-1;
            memcpy(p, hash, hash_size);
            return SEC_E_OK;
        }
        case SECPKG_ATTR_UNIQUE_BINDINGS:
        {
            SecPkgContext_Bindings *bindings = buffer;
            return schan_funcs->get_unique_channel_binding(ctx->session, bindings);
        }
        case SECPKG_ATTR_APPLICATION_PROTOCOL:
        {
            SecPkgContext_ApplicationProtocol *protocol = buffer;
            return schan_funcs->get_application_protocol(ctx->session, protocol);
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
        case SECPKG_ATTR_KEY_INFO:
        {
            SECURITY_STATUS status = schan_QueryContextAttributesW(context_handle, attribute, buffer);
            if (status == SEC_E_OK)
            {
                SecPkgContext_KeyInfoA *info = buffer;
                info->sSignatureAlgorithmName = get_alg_name(info->SignatureAlgorithm, FALSE);
                info->sEncryptAlgorithmName = get_alg_name(info->EncryptAlgorithm, FALSE);
            }
            return status;
        }
        case SECPKG_ATTR_REMOTE_CERT_CONTEXT:
            return schan_QueryContextAttributesW(context_handle, attribute, buffer);
        case SECPKG_ATTR_CONNECTION_INFO:
            return schan_QueryContextAttributesW(context_handle, attribute, buffer);
        case SECPKG_ATTR_ENDPOINT_BINDINGS:
            return schan_QueryContextAttributesW(context_handle, attribute, buffer);
        case SECPKG_ATTR_UNIQUE_BINDINGS:
            return schan_QueryContextAttributesW(context_handle, attribute, buffer);
        case SECPKG_ATTR_APPLICATION_PROTOCOL:
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
    struct schan_context *ctx;
    struct schan_buffers *b;
    SECURITY_STATUS status;
    SecBuffer *buffer;
    SIZE_T data_size;
    SIZE_T length;
    char *data;
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
    data = malloc(data_size);
    memcpy(data, buffer->pvBuffer, data_size);

    if (schan_find_sec_buffer_idx(message, 0, SECBUFFER_STREAM_HEADER) != -1)
        init_schan_buffers(&ctx->transport.out, message, schan_encrypt_message_get_next_buffer);
    else
        init_schan_buffers(&ctx->transport.out, message, schan_encrypt_message_get_next_buffer_token);

    length = data_size;
    status = schan_funcs->send(ctx->session, data, &length);

    TRACE("Sent %ld bytes.\n", length);

    if (length != data_size)
        status = SEC_E_INTERNAL_ERROR;

    b = &ctx->transport.out;
    b->desc->pBuffers[b->current_buffer_idx].cbBuffer = b->offset;
    free(data);

    TRACE("Returning %#x.\n", status);

    return status;
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
    SECURITY_STATUS status = SEC_E_OK;
    struct schan_context *ctx;
    SecBuffer *buffer;
    SIZE_T data_size;
    char *data;
    unsigned expected_size;
    SSIZE_T received = 0;
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

    expected_size = ctx->header_size + read_record_size(buf_ptr, ctx->header_size);
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

    data_size = expected_size - ctx->header_size;
    data = malloc(data_size);

    init_schan_buffers(&ctx->transport.in, message, schan_decrypt_message_get_next_buffer);
    ctx->transport.in.limit = expected_size;

    while (received < data_size)
    {
        SIZE_T length = data_size - received;
        status = schan_funcs->recv(ctx->session, data + received, &length);

        if (status == SEC_I_RENEGOTIATE)
            break;

        if (status == SEC_I_CONTINUE_NEEDED)
        {
            status = SEC_E_OK;
            break;
        }

        if (status != SEC_E_OK)
        {
            free(data);
            ERR("Returning %x\n", status);
            return status;
        }

        if (!length)
            break;

        received += length;
    }

    TRACE("Received %ld bytes\n", received);

    memcpy(buf_ptr + ctx->header_size, data, received);
    free(data);

    schan_decrypt_fill_buffer(message, SECBUFFER_DATA,
        buf_ptr + ctx->header_size, received);

    schan_decrypt_fill_buffer(message, SECBUFFER_STREAM_TRAILER,
        buf_ptr + ctx->header_size + received, buffer->cbBuffer - ctx->header_size - received);

    if(buffer->cbBuffer > expected_size)
        schan_decrypt_fill_buffer(message, SECBUFFER_EXTRA,
            buf_ptr + expected_size, buffer->cbBuffer - expected_size);

    buffer->BufferType = SECBUFFER_STREAM_HEADER;
    buffer->cbBuffer = ctx->header_size;

    return status;
}

static SECURITY_STATUS SEC_ENTRY schan_DeleteSecurityContext(PCtxtHandle context_handle)
{
    struct schan_context *ctx;

    TRACE("context_handle %p\n", context_handle);

    if (!context_handle) return SEC_E_INVALID_HANDLE;

    ctx = schan_free_handle(context_handle->dwLower, SCHAN_HANDLE_CTX);
    if (!ctx) return SEC_E_INVALID_HANDLE;

    if (ctx->cert) CertFreeCertificateContext(ctx->cert);
    schan_funcs->dispose_session(ctx->session);
    free(ctx);
    return SEC_E_OK;
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

const struct schan_callbacks schan_callbacks =
{
    schan_get_buffer,
    schan_get_session_for_transport,
    schan_pull,
    schan_push,
};

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
        { caps, version, UNISP_RPC_ID, maxToken, schannel, (SEC_WCHAR *)L"Schannel Security Package" },
    };
    SecureProvider *provider;

    if (!schan_funcs && __wine_init_unix_lib(hsecur32, DLL_PROCESS_ATTACH, &schan_callbacks, &schan_funcs))
    {
        ERR( "no schannel support, expect problems\n" );
        return;
    }

    schan_handle_table = malloc(64 * sizeof(*schan_handle_table));
    if (!schan_handle_table)
    {
        ERR("Failed to allocate schannel handle table.\n");
        goto fail;
    }
    schan_handle_table_size = 64;

    provider = SECUR32_addProvider(&schanTableA, &schanTableW, L"schannel.dll");
    if (!provider)
    {
        ERR("Failed to add schannel provider.\n");
        goto fail;
    }

    SECUR32_addPackages(provider, ARRAY_SIZE(info), NULL, info);
    return;

fail:
    free(schan_handle_table);
    schan_handle_table = NULL;
    return;
}

void SECUR32_deinitSchannelSP(void)
{
    SIZE_T i = schan_handle_count;

    if (!schan_handle_table) return;

    /* deinitialized sessions first because a pointer to the credentials
     * may be stored for the session. */
    while (i--)
    {
        if (schan_handle_table[i].type == SCHAN_HANDLE_CTX)
        {
            struct schan_context *ctx = schan_free_handle(i, SCHAN_HANDLE_CTX);
            schan_funcs->dispose_session(ctx->session);
            free(ctx);
        }
    }
    i = schan_handle_count;
    while (i--)
    {
        if (schan_handle_table[i].type != SCHAN_HANDLE_FREE)
        {
            struct schan_credentials *cred;
            cred = schan_free_handle(i, SCHAN_HANDLE_CRED);
            schan_funcs->free_certificate_credentials(cred);
            free(cred);
        }
    }
    free(schan_handle_table);

    __wine_init_unix_lib(hsecur32, DLL_PROCESS_DETACH, NULL, NULL);
    schan_funcs = NULL;
}
