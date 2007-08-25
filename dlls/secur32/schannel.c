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
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "sspi.h"
#include "schannel.h"
#include "secur32_priv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(secur32);

static SECURITY_STATUS schan_QueryCredentialsAttributes(
 PCredHandle phCredential, ULONG ulAttribute, const VOID *pBuffer)
{
    SECURITY_STATUS ret;

    switch (ulAttribute)
    {
    case SECPKG_ATTR_SUPPORTED_ALGS:
        if (pBuffer)
        {
            /* FIXME: get from CryptoAPI */
            FIXME("%d: stub\n", ulAttribute);
            ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INTERNAL_ERROR;
        break;
    case SECPKG_ATTR_CIPHER_STRENGTHS:
        if (pBuffer)
        {
            /* FIXME: get from CryptoAPI */
            FIXME("%d: stub\n", ulAttribute);
            ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INTERNAL_ERROR;
        break;
    case SECPKG_ATTR_SUPPORTED_PROTOCOLS:
        if (pBuffer)
        {
            /* FIXME: get from OpenSSL? */
            FIXME("%d: stub\n", ulAttribute);
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
    SECURITY_STATUS st = SEC_E_OK;

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
        phCredential->dwUpper = SECPKG_CRED_OUTBOUND;
        /* Outbound credentials have no expiry */
        if (ptsExpiry)
        {
            ptsExpiry->LowPart = 0;
            ptsExpiry->HighPart = 0;
        }
    }
    return st;
}

static SECURITY_STATUS schan_AcquireServerCredentials(const SCHANNEL_CRED *schanCred,
 PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS st;

    if (!schanCred) return SEC_E_NO_CREDENTIALS;

    st = schan_CheckCreds(schanCred);
    if (st == SEC_E_OK)
    {
        phCredential->dwUpper = SECPKG_CRED_INBOUND;
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
    FIXME("(%p): stub\n", phCredential);
    return SEC_E_OK;
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

    TRACE("%p %p %s %d %d %d %p %d %p %p %p %p\n", phCredential, phContext,
     debugstr_a(pszTargetName), fContextReq, Reserved1, TargetDataRep, pInput,
     Reserved1, phNewContext, pOutput, pfContextAttr, ptsExpiry);
    if(phCredential)
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
 *              InitializeSecurityContextW
 */
static SECURITY_STATUS SEC_ENTRY schan_InitializeSecurityContextW(
 PCredHandle phCredential, PCtxtHandle phContext, SEC_WCHAR *pszTargetName,
 ULONG fContextReq, ULONG Reserved1, ULONG TargetDataRep,
 PSecBufferDesc pInput,ULONG Reserved2, PCtxtHandle phNewContext,
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

void SECUR32_initSchannelSP(void)
{
    SecureProvider *provider = SECUR32_addProvider(&schanTableA, &schanTableW,
     NULL);

    if (provider)
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

        SECUR32_addPackages(provider, sizeof(info) / sizeof(info[0]), NULL,
         info);
    }
}
