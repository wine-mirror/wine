/* Copyright (C) 2005 Juan Lang
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
MAKE_FUNCPTR(gnutls_certificate_allocate_credentials);
MAKE_FUNCPTR(gnutls_certificate_free_credentials);
MAKE_FUNCPTR(gnutls_global_deinit);
MAKE_FUNCPTR(gnutls_global_init);
MAKE_FUNCPTR(gnutls_global_set_log_function);
MAKE_FUNCPTR(gnutls_global_set_log_level);
MAKE_FUNCPTR(gnutls_perror);
#undef MAKE_FUNCPTR

#define SCHAN_INVALID_HANDLE ~0UL

enum schan_handle_type
{
    SCHAN_HANDLE_CRED,
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

static struct schan_handle *schan_handle_table;
static struct schan_handle *schan_free_handles;
static SIZE_T schan_handle_table_size;
static SIZE_T schan_handle_count;

static ULONG_PTR schan_alloc_handle(void *object, enum schan_handle_type type)
{
    struct schan_handle *handle;

    if (schan_free_handles)
    {
        /* Use a free handle */
        handle = schan_free_handles;
        if (handle->type != SCHAN_HANDLE_FREE)
        {
            ERR("Handle %d(%p) is in the free list, but has type %#x.\n", (handle-schan_handle_table), handle, handle->type);
            return SCHAN_INVALID_HANDLE;
        }
        schan_free_handles = (struct schan_handle *)handle->object;
        handle->object = object;
        handle->type = type;

        return handle - schan_handle_table;
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
            SecPkgCred_CipherStrengths *r = (SecPkgCred_CipherStrengths*)pBuffer;

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
     (PSCHANNEL_CRED)pAuthData, phCredential, ptsExpiry);
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
     (PSCHANNEL_CRED)pAuthData, phCredential, ptsExpiry);
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

/***********************************************************************
 *              InitializeSecurityContextW
 */
static SECURITY_STATUS SEC_ENTRY schan_InitializeSecurityContextW(
 PCredHandle phCredential, PCtxtHandle phContext, SEC_WCHAR *pszTargetName,
 ULONG fContextReq, ULONG Reserved1, ULONG TargetDataRep,
 PSecBufferDesc pInput, ULONG Reserved2, PCtxtHandle phNewContext,
 PSecBufferDesc pOutput, ULONG *pfContextAttr, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;

    TRACE("%p %p %s %d %d %d %p %d %p %p %p %p\n", phCredential, phContext,
     debugstr_w(pszTargetName), fContextReq, Reserved1, TargetDataRep, pInput,
     Reserved1, phNewContext, pOutput, pfContextAttr, ptsExpiry);
    if (phCredential)
    {
        FIXME("stub\n");
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
    {
        ret = SEC_E_INVALID_HANDLE;
    }
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
        target_name = HeapAlloc(GetProcessHeap(), 0, len * sizeof(*target_name));
        MultiByteToWideChar(CP_ACP, 0, pszTargetName, -1, target_name, len);
    }

    ret = schan_InitializeSecurityContextW(phCredential, phContext, target_name,
            fContextReq, Reserved1, TargetDataRep, pInput, Reserved2,
            phNewContext, pOutput, pfContextAttr, ptsExpiry);

    HeapFree(GetProcessHeap(), 0, target_name);

    return ret;
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
    NULL, /* DeleteSecurityContext */
    NULL, /* ApplyControlToken */
    NULL, /* QueryContextAttributesA */
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
    NULL, /* EncryptMessage */
    NULL, /* DecryptMessage */
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
    NULL, /* DeleteSecurityContext */
    NULL, /* ApplyControlToken */
    NULL, /* QueryContextAttributesW */
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
    NULL, /* EncryptMessage */
    NULL, /* DecryptMessage */
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
    static const long caps =
        SECPKG_FLAG_INTEGRITY |
        SECPKG_FLAG_PRIVACY |
        SECPKG_FLAG_CONNECTION |
        SECPKG_FLAG_MULTI_REQUIRED |
        SECPKG_FLAG_EXTENDED_ERROR |
        SECPKG_FLAG_IMPERSONATION |
        SECPKG_FLAG_ACCEPT_WIN32_NAME |
        SECPKG_FLAG_STREAM;
    static const short version = 1;
    static const long maxToken = 16384;
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

    LOAD_FUNCPTR(gnutls_certificate_allocate_credentials)
    LOAD_FUNCPTR(gnutls_certificate_free_credentials)
    LOAD_FUNCPTR(gnutls_global_deinit)
    LOAD_FUNCPTR(gnutls_global_init)
    LOAD_FUNCPTR(gnutls_global_set_log_function)
    LOAD_FUNCPTR(gnutls_global_set_log_level)
    LOAD_FUNCPTR(gnutls_perror)
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

    schan_handle_table = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 64 * sizeof(*schan_handle_table));
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
    if (!libgnutls_handle) return;

    pgnutls_global_deinit();
    wine_dlclose(libgnutls_handle, NULL, 0);
}

#else /* SONAME_LIBGNUTLS */

void SECUR32_initSchannelSP(void) {}
void SECUR32_deinitSchannelSP(void) {}

#endif /* SONAME_LIBGNUTLS */
