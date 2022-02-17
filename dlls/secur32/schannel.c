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

#include "wine/unixlib.h"
#include "wine/debug.h"
#include "secur32_priv.h"

WINE_DEFAULT_DEBUG_CHANNEL(secur32);

static unixlib_handle_t gnutls_handle;

#define GNUTLS_CALL( func, params ) __wine_unix_call( gnutls_handle, unix_ ## func, params )

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
            ERR("Handle %ld(%p) is in the free list, but has type %#x.\n", index, handle, handle->type);
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
        ERR("Handle %Id(%p) is not of type %#x\n", handle_idx, handle, type);
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
        ERR("Handle %Id(%p) is not of type %#x\n", handle_idx, handle, type);
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

    config_enabled_protocols = enabled & GNUTLS_CALL( get_enabled_protocols, NULL );
    config_default_disabled_protocols = default_disabled;
    config_read = TRUE;

    TRACE("enabled %lx, disabled by default %lx\n", config_enabled_protocols, config_default_disabled_protocols);
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

    TRACE("(%p, %ld, %p)\n", phCredential, ulAttribute, pBuffer);

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

    TRACE("(%p, %ld, %p)\n", phCredential, ulAttribute, pBuffer);

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

    TRACE("dwVersion = %lu\n", cred->dwVersion);
    TRACE("cCreds = %lu\n", cred->cCreds);
    TRACE("paCred = %p\n", cred->paCred);
    TRACE("hRootStore = %p\n", cred->hRootStore);
    TRACE("cMappers = %lu\n", cred->cMappers);
    TRACE("cSupportedAlgs = %lu:\n", cred->cSupportedAlgs);
    for (i = 0; i < cred->cSupportedAlgs; i++) TRACE("%08x\n", cred->palgSupportedAlgs[i]);
    TRACE("grbitEnabledProtocols = %08lx\n", cred->grbitEnabledProtocols);
    TRACE("dwMinimumCipherStrength = %lu\n", cred->dwMinimumCipherStrength);
    TRACE("dwMaximumCipherStrength = %lu\n", cred->dwMaximumCipherStrength);
    TRACE("dwSessionLifespan = %lu\n", cred->dwSessionLifespan);
    TRACE("dwFlags = %08lx\n", cred->dwFlags);
    TRACE("dwCredFormat = %lu\n", cred->dwCredFormat);

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
    struct allocate_certificate_credentials_params params;

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
    params.c = creds;
    params.ctx = cert;
    params.key_blob = &key_blob;
    if (GNUTLS_CALL( allocate_certificate_credentials, &params )) goto fail;
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
    TRACE("(%s, %s, 0x%08lx, %p, %p, %p, %p, %p, %p)\n",
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
    TRACE("(%s, %s, 0x%08lx, %p, %p, %p, %p, %p, %p)\n",
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
    {
        struct free_certificate_credentials_params params = { creds };
        GNUTLS_CALL( free_certificate_credentials, &params );
    }
    free(creds);
    return SEC_E_OK;
}

static int schan_find_sec_buffer_idx(const SecBufferDesc *desc, unsigned int start_idx, ULONG buffer_type)
{
    unsigned int i;
    PSecBuffer buffer;

    for (i = start_idx; i < desc->cBuffers; ++i)
    {
        buffer = &desc->pBuffers[i];
        if ((buffer->BufferType | SECBUFFER_ATTRMASK) == (buffer_type | SECBUFFER_ATTRMASK))
            return i;
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
        TRACE("\tbuffer %u: cbBuffer %ld, BufferType %#lx pvBuffer %p\n", i, b->cbBuffer, b->BufferType, b->pvBuffer);
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
    return ctx->header_size == HEADER_SIZE_DTLS;
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
    const ULONG extra_size = 0x10000;
    struct schan_context *ctx;
    struct schan_buffers *out_buffers;
    struct schan_credentials *cred;
    SIZE_T expected_size = 0;
    SECURITY_STATUS ret;
    SecBuffer *buffer;
    SecBuffer alloc_buffer = { 0 };
    struct handshake_params params;
    int idx, i;

    TRACE("%p %p %s 0x%08lx %ld %ld %p %ld %p %p %p %p\n", phCredential, phContext,
     debugstr_w(pszTargetName), fContextReq, Reserved1, TargetDataRep, pInput,
     Reserved1, phNewContext, pOutput, pfContextAttr, ptsExpiry);

    dump_buffer_desc(pInput);
    dump_buffer_desc(pOutput);

    if (ptsExpiry)
    {
        ptsExpiry->LowPart = 0;
        ptsExpiry->HighPart = 0;
    }

    if (!pOutput || !pOutput->cBuffers) return SEC_E_INVALID_TOKEN;
    for (i = 0; i < pOutput->cBuffers; i++)
    {
        ULONG type = pOutput->pBuffers[i].BufferType;

        if (type != SECBUFFER_TOKEN && type != SECBUFFER_ALERT) continue;
        if (!pOutput->pBuffers[i].cbBuffer && !(fContextReq & ISC_REQ_ALLOCATE_MEMORY))
            return SEC_E_INSUFFICIENT_MEMORY;
    }

    if (!phContext)
    {
        ULONG_PTR handle;
        struct create_session_params create_params;

        if (!phCredential) return SEC_E_INVALID_HANDLE;

        cred = schan_get_object(phCredential->dwLower, SCHAN_HANDLE_CRED);
        if (!cred) return SEC_E_INVALID_HANDLE;

        if (!(cred->credential_use & SECPKG_CRED_OUTBOUND))
        {
            WARN("Invalid credential use %#lx\n", cred->credential_use);
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

        create_params.transport = &ctx->transport;
        create_params.cred = cred;
        if (GNUTLS_CALL( create_session, &create_params ))
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

        if (pszTargetName && *pszTargetName)
        {
            UINT len = WideCharToMultiByte( CP_UNIXCP, 0, pszTargetName, -1, NULL, 0, NULL, NULL );
            char *target = malloc( len );

            if (target)
            {
                struct set_session_target_params params = { ctx->transport.session, target };
                WideCharToMultiByte( CP_UNIXCP, 0, pszTargetName, -1, target, len, NULL, NULL );
                GNUTLS_CALL( set_session_target, &params );
                free( target );
            }
        }

        if (pInput && (idx = schan_find_sec_buffer_idx(pInput, 0, SECBUFFER_APPLICATION_PROTOCOLS)) != -1)
        {
            struct set_application_protocols_params params = { ctx->transport.session,
                pInput->pBuffers[idx].pvBuffer, pInput->pBuffers[idx].cbBuffer };
            GNUTLS_CALL( set_application_protocols, &params );
        }

        if (pInput && (idx = schan_find_sec_buffer_idx(pInput, 0, SECBUFFER_DTLS_MTU)) != -1)
        {
            buffer = &pInput->pBuffers[idx];
            if (buffer->cbBuffer >= sizeof(WORD))
            {
                struct set_dtls_mtu_params params = { ctx->transport.session, *(WORD *)buffer->pvBuffer };
                GNUTLS_CALL( set_dtls_mtu, &params );
            }
            else WARN("invalid buffer size %lu\n", buffer->cbBuffer);
        }

        if (is_dtls_context(ctx))
        {
            struct set_dtls_timeouts_params params = { ctx->transport.session, 0, 60000 };
            GNUTLS_CALL( set_dtls_timeouts, &params );
        }

        phNewContext->dwLower = handle;
        phNewContext->dwUpper = 0;
    }
    else
    {
        SIZE_T record_size = 0;
        unsigned char *ptr;

        if (!(ctx = schan_get_object(phContext->dwLower, SCHAN_HANDLE_CTX))) return SEC_E_INVALID_HANDLE;
        if (!pInput && !is_dtls_context(ctx)) return SEC_E_INCOMPLETE_MESSAGE;

        if (pInput)
        {
            if ((idx = schan_find_sec_buffer_idx(pInput, 0, SECBUFFER_TOKEN)) == -1) return SEC_E_INCOMPLETE_MESSAGE;

            buffer = &pInput->pBuffers[idx];
            ptr = buffer->pvBuffer;

            while (buffer->cbBuffer > expected_size + ctx->header_size)
            {
                record_size = ctx->header_size + read_record_size(ptr, ctx->header_size);

                if (buffer->cbBuffer < expected_size + record_size) break;
                expected_size += record_size;
                ptr += record_size;
            }

            if (!expected_size)
            {
                TRACE("Expected at least %Iu bytes, but buffer only contains %lu bytes.\n",
                      max(ctx->header_size + 1, record_size), buffer->cbBuffer);
                return SEC_E_INCOMPLETE_MESSAGE;
            }

            TRACE("Using expected_size %Iu.\n", expected_size);
        }

        if (phNewContext) *phNewContext = *phContext;
    }

    ctx->req_ctx_attr = fContextReq;

    /* Perform the TLS handshake */
    if (fContextReq & ISC_REQ_ALLOCATE_MEMORY)
    {
        alloc_buffer.cbBuffer = extra_size;
        alloc_buffer.BufferType = SECBUFFER_TOKEN;
        alloc_buffer.pvBuffer = RtlAllocateHeap( GetProcessHeap(), 0, extra_size );
    }
    params.session = ctx->transport.session;
    params.input = pInput;
    params.input_size = expected_size;
    params.output = pOutput;
    params.alloc_buffer = &alloc_buffer;
    ret = GNUTLS_CALL( handshake, &params );

    out_buffers = &ctx->transport.out;
    if (out_buffers->current_buffer_idx != -1)
    {
        SecBuffer *buffer = &out_buffers->desc->pBuffers[out_buffers->current_buffer_idx];
        buffer->cbBuffer = out_buffers->offset;
        if (buffer->pvBuffer == alloc_buffer.pvBuffer)
        {
            RtlReAllocateHeap( GetProcessHeap(), HEAP_REALLOC_IN_PLACE_ONLY,
                               buffer->pvBuffer, buffer->cbBuffer );
            alloc_buffer.pvBuffer = NULL;
        }
    }
    else if (out_buffers->desc && out_buffers->desc->cBuffers > 0)
    {
        SecBuffer *buffer = &out_buffers->desc->pBuffers[0];
        buffer->cbBuffer = 0;
    }
    RtlFreeHeap( GetProcessHeap(), 0, alloc_buffer.pvBuffer );

    if(ctx->transport.in.offset && ctx->transport.in.offset != pInput->pBuffers[0].cbBuffer) {
        if(pInput->cBuffers<2 || pInput->pBuffers[1].BufferType!=SECBUFFER_EMPTY)
            return SEC_E_INVALID_TOKEN;

        pInput->pBuffers[1].BufferType = SECBUFFER_EXTRA;
        pInput->pBuffers[1].cbBuffer = pInput->pBuffers[0].cbBuffer-ctx->transport.in.offset;
    }

    for (i = 0; i < pOutput->cBuffers; i++)
    {
        SecBuffer *buffer = &pOutput->pBuffers[i];
        if (buffer->BufferType == SECBUFFER_ALERT) buffer->cbBuffer = 0;
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

    TRACE("%p %p %s %ld %ld %ld %p %ld %p %p %p %p\n", phCredential, phContext,
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
    CERT_BLOB *certs;
    ULONG count, size = 0;
    struct get_session_peer_certificate_params params = { ctx->transport.session, NULL, &size, &count };

    if (ctx->cert) return SEC_E_OK;
    if (!(store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0, CERT_STORE_CREATE_NEW_FLAG, NULL)))
        return GetLastError();

    status = GNUTLS_CALL( get_session_peer_certificate, &params );
    if (status != SEC_E_BUFFER_TOO_SMALL) goto done;
    if (!(certs = malloc( size )))
    {
        status = SEC_E_INSUFFICIENT_MEMORY;
        goto done;
    }
    params.certs = certs;
    status = GNUTLS_CALL( get_session_peer_certificate, &params );
    if (status == SEC_E_OK)
    {
        unsigned int i;
        for (i = 0; i < count; i++)
        {
            if (!CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING, certs[i].pbData,
                                                  certs[i].cbData, CERT_STORE_ADD_REPLACE_EXISTING,
                                                  i ? NULL : &cert))
            {
                if (i) CertFreeCertificateContext(cert);
                return GetLastError();
            }
        }
    }
    free(certs);
done:
    ctx->cert = cert;
    CertCloseStore(store, 0);
    return status;
}

static SECURITY_STATUS SEC_ENTRY schan_QueryContextAttributesW(
        PCtxtHandle context_handle, ULONG attribute, PVOID buffer)
{
    struct schan_context *ctx;
    SECURITY_STATUS status;

    TRACE("context_handle %p, attribute %#lx, buffer %p\n",
            context_handle, attribute, buffer);

    if (!context_handle) return SEC_E_INVALID_HANDLE;
    ctx = schan_get_object(context_handle->dwLower, SCHAN_HANDLE_CTX);

    switch(attribute)
    {
        case SECPKG_ATTR_STREAM_SIZES:
        {
            SecPkgContext_ConnectionInfo info;
            struct get_connection_info_params params = { ctx->transport.session, &info };
            status = GNUTLS_CALL( get_connection_info, &params );
            if (status == SEC_E_OK)
            {
                struct session_params params = { ctx->transport.session };
                SecPkgContext_StreamSizes *stream_sizes = buffer;
                SIZE_T mac_size = info.dwHashStrength;
                unsigned int block_size = GNUTLS_CALL( get_session_cipher_block_size, &params );
                unsigned int message_size = GNUTLS_CALL( get_max_message_size, &params );

                TRACE("Using header size %Iu mac bytes %Iu, message size %u, block size %u\n",
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
            struct get_connection_info_params params = { ctx->transport.session, &conn_info };
            status = GNUTLS_CALL( get_connection_info, &params );
            if (status == SEC_E_OK)
            {
                struct session_params params = { ctx->transport.session };
                SecPkgContext_KeyInfoW *info = buffer;
                info->KeySize = conn_info.dwCipherStrength;
                info->SignatureAlgorithm = GNUTLS_CALL( get_key_signature_algorithm, &params );
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
            struct get_connection_info_params params = { ctx->transport.session, info };
            return GNUTLS_CALL( get_connection_info, &params );
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
            static const char prefix[] = "tls-unique:";
            SecPkgContext_Bindings *bindings = buffer;
            ULONG size;
            char *p;
            struct get_unique_channel_binding_params params = { ctx->transport.session, NULL, &size };

            if (GNUTLS_CALL( get_unique_channel_binding, &params ) != SEC_E_BUFFER_TOO_SMALL)
                return SEC_E_INTERNAL_ERROR;

            bindings->BindingsLength = sizeof(*bindings->Bindings) + sizeof(prefix)-1 + size;
            /* freed with FreeContextBuffer */
            bindings->Bindings = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, bindings->BindingsLength);
            if(!bindings->Bindings)
                return SEC_E_INSUFFICIENT_MEMORY;

            bindings->Bindings->cbApplicationDataLength = sizeof(prefix)-1 + size;
            bindings->Bindings->dwApplicationDataOffset = sizeof(*bindings->Bindings);

            p = (char*)(bindings->Bindings+1);
            memcpy(p, prefix, sizeof(prefix)-1);
            p += sizeof(prefix)-1;
            params.buffer = p;
            return GNUTLS_CALL( get_unique_channel_binding, &params );
        }
        case SECPKG_ATTR_APPLICATION_PROTOCOL:
        {
            SecPkgContext_ApplicationProtocol *protocol = buffer;
            struct get_application_protocol_params params = { ctx->transport.session, protocol };
            return GNUTLS_CALL( get_application_protocol, &params );
        }

        default:
            FIXME("Unhandled attribute %#lx\n", attribute);
            return SEC_E_UNSUPPORTED_FUNCTION;
    }
}

static SECURITY_STATUS SEC_ENTRY schan_QueryContextAttributesA(
        PCtxtHandle context_handle, ULONG attribute, PVOID buffer)
{
    TRACE("context_handle %p, attribute %#lx, buffer %p\n",
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
            FIXME("Unhandled attribute %#lx\n", attribute);
            return SEC_E_UNSUPPORTED_FUNCTION;
    }
}

static SECURITY_STATUS SEC_ENTRY schan_EncryptMessage(PCtxtHandle context_handle,
        ULONG quality, PSecBufferDesc message, ULONG message_seq_no)
{
    struct schan_context *ctx;
    struct send_params params;
    SECURITY_STATUS status;
    SecBuffer *buffer;
    SIZE_T data_size;
    SIZE_T length;
    char *data;
    int idx;

    TRACE("context_handle %p, quality %ld, message %p, message_seq_no %ld\n",
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

    length = data_size;
    params.session = ctx->transport.session;
    params.output = message;
    params.buffer = data;
    params.length = &length;
    status = GNUTLS_CALL( send, &params );

    TRACE("Sent %Id bytes.\n", length);

    if (length != data_size)
        status = SEC_E_INTERNAL_ERROR;

    free(data);

    TRACE("Returning %#lx.\n", status);

    return status;
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
    struct recv_params params;
    SecBuffer *buffer;
    SIZE_T data_size;
    char *data;
    unsigned expected_size;
    SIZE_T received = 0;
    int idx;
    unsigned char *buf_ptr;

    TRACE("context_handle %p, message %p, message_seq_no %ld, quality %p\n",
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
        TRACE("Expected %u bytes, but buffer only contains %lu bytes\n", expected_size, buffer->cbBuffer);
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

    received = data_size;

    params.session = ctx->transport.session;
    params.input = message;
    params.input_size = expected_size;
    params.buffer = data;
    params.length = &received;
    status = GNUTLS_CALL( recv, &params );

    if (status != SEC_E_OK && status != SEC_I_RENEGOTIATE)
    {
        free(data);
        ERR("Returning %lx\n", status);
        return status;
    }

    TRACE("Received %Id bytes\n", received);

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
    struct session_params params;

    TRACE("context_handle %p\n", context_handle);

    if (!context_handle) return SEC_E_INVALID_HANDLE;

    ctx = schan_free_handle(context_handle->dwLower, SCHAN_HANDLE_CTX);
    if (!ctx) return SEC_E_INVALID_HANDLE;

    if (ctx->cert) CertFreeCertificateContext(ctx->cert);
    params.session = ctx->transport.session;
    GNUTLS_CALL( dispose_session, &params );
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

    if (!gnutls_handle)
    {
        if (NtQueryVirtualMemory( GetCurrentProcess(), hsecur32, MemoryWineUnixFuncs,
                                  &gnutls_handle, sizeof(gnutls_handle), NULL ) ||
            GNUTLS_CALL( process_attach, NULL ))
        {
            ERR( "no schannel support, expect problems\n" );
            return;
        }
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
            struct session_params params = { ctx->transport.session };
            GNUTLS_CALL( dispose_session, &params );
            free(ctx);
        }
    }
    i = schan_handle_count;
    while (i--)
    {
        if (schan_handle_table[i].type != SCHAN_HANDLE_FREE)
        {
            struct schan_credentials *cred = schan_free_handle(i, SCHAN_HANDLE_CRED);
            struct free_certificate_credentials_params params = { cred };
            GNUTLS_CALL( free_certificate_credentials, &params );
            free(cred);
        }
    }
    free(schan_handle_table);
    GNUTLS_CALL( process_detach, NULL );
    gnutls_handle = 0;
}
