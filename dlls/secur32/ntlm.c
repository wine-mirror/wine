/*
 * Copyright 2005 Kai Blin
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This file implements the NTLM security provider.
 * FIXME: So far, this beast doesn't do anything.
 */
#include <assert.h>
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "sspi.h"
#include "secur32_priv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(secur32);

static char ntlm_name_A[] = "NTLM";
static WCHAR ntlm_name_W[] = {'N', 'T', 'L', 'M', 0};


/***********************************************************************
 *              QueryCredentialsAttributesA
 */
static SECURITY_STATUS SEC_ENTRY ntlm_QueryCredentialsAttributesA(
        PCredHandle phCredential, ULONG ulAttribute, PVOID pBuffer)
{
    SECURITY_STATUS ret;

    TRACE("(%p, %ld, %p)\n", phCredential, ulAttribute, pBuffer);

    if(ulAttribute == SECPKG_ATTR_NAMES)
    {
        FIXME("SECPKG_CRED_ATTR_NAMES: stub\n");
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    
    return ret;
}

/***********************************************************************
 *              QueryCredentialsAttributesW
 */
static SECURITY_STATUS SEC_ENTRY ntlm_QueryCredentialsAttributesW(
        PCredHandle phCredential, ULONG ulAttribute, PVOID pBuffer)
{
    SECURITY_STATUS ret;

    TRACE("(%p, %ld, %p)\n", phCredential, ulAttribute, pBuffer);

    if(ulAttribute == SECPKG_ATTR_NAMES)
    {
        FIXME("SECPKG_CRED_ATTR_NAMES: stub\n");
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    
    return ret;
}

static SECURITY_STATUS ntlm_AcquireCredentialsHandle(ULONG fCredentialsUse,
        PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;

    if(fCredentialsUse == SECPKG_CRED_BOTH)
    {
        ret = SEC_E_NO_CREDENTIALS;
    }
    else
    {
        /* Ok, just store the direction like schannel does for now.
         * FIXME: This should probably do something useful later on
         */
        phCredential->dwUpper = fCredentialsUse;
        phCredential->dwLower = 0;
        /* Same here, shamelessly stolen from schannel.c */
        if (ptsExpiry)
            ptsExpiry->QuadPart = 0;
        ret = SEC_E_OK;
    }
    return ret;
}

/***********************************************************************
 *              AcquireCredentialsHandleA
 */
static SECURITY_STATUS SEC_ENTRY ntlm_AcquireCredentialsHandleA(
 SEC_CHAR *pszPrincipal, SEC_CHAR *pszPackage, ULONG fCredentialUse,
 PLUID pLogonID, PVOID pAuthData, SEC_GET_KEY_FN pGetKeyFn,
 PVOID pGetKeyArgument, PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    TRACE("(%s, %s, 0x%08lx, %p, %p, %p, %p, %p, %p)\n",
     debugstr_a(pszPrincipal), debugstr_a(pszPackage), fCredentialUse,
     pLogonID, pAuthData, pGetKeyFn, pGetKeyArgument, phCredential, ptsExpiry);
    return ntlm_AcquireCredentialsHandle(fCredentialUse, phCredential,
            ptsExpiry);
}

/***********************************************************************
 *              AcquireCredentialsHandleW
 */
static SECURITY_STATUS SEC_ENTRY ntlm_AcquireCredentialsHandleW(
 SEC_WCHAR *pszPrincipal, SEC_WCHAR *pszPackage, ULONG fCredentialUse,
 PLUID pLogonID, PVOID pAuthData, SEC_GET_KEY_FN pGetKeyFn,
 PVOID pGetKeyArgument, PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    TRACE("(%s, %s, 0x%08lx, %p, %p, %p, %p, %p, %p)\n",
     debugstr_w(pszPrincipal), debugstr_w(pszPackage), fCredentialUse,
     pLogonID, pAuthData, pGetKeyFn, pGetKeyArgument, phCredential, ptsExpiry);
    return ntlm_AcquireCredentialsHandle(fCredentialUse, phCredential,
            ptsExpiry);
}

/***********************************************************************
 *              InitializeSecurityContextA
 */
static SECURITY_STATUS SEC_ENTRY ntlm_InitializeSecurityContextA(
 PCredHandle phCredential, PCtxtHandle phContext, SEC_CHAR *pszTargetName, 
 ULONG fContextReq, ULONG Reserved1, ULONG TargetDataRep, 
 PSecBufferDesc pInput, ULONG Reserved2, PCtxtHandle phNewContext, 
 PSecBufferDesc pOutput, ULONG *pfContextAttr, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;

    TRACE("%p %p %s %ld %ld %ld %p %ld %p %p %p %p\n", phCredential, phContext,
     debugstr_a(pszTargetName), fContextReq, Reserved1, TargetDataRep, pInput,
     Reserved1, phNewContext, pOutput, pfContextAttr, ptsExpiry);
    if(phCredential){
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
static SECURITY_STATUS SEC_ENTRY ntlm_InitializeSecurityContextW(
 PCredHandle phCredential, PCtxtHandle phContext, SEC_WCHAR *pszTargetName,
 ULONG fContextReq, ULONG Reserved1, ULONG TargetDataRep, 
 PSecBufferDesc pInput,ULONG Reserved2, PCtxtHandle phNewContext, 
 PSecBufferDesc pOutput, ULONG *pfContextAttr, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;

    TRACE("%p %p %s %ld %ld %ld %p %ld %p %p %p %p\n", phCredential, phContext,
     debugstr_w(pszTargetName), fContextReq, Reserved1, TargetDataRep, pInput,
     Reserved1, phNewContext, pOutput, pfContextAttr, ptsExpiry);
    if (phCredential)
    {
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
    {
        ret = SEC_E_INVALID_HANDLE;
    }
    return ret;
}

/***********************************************************************
 *              AcceptSecurityContext
 */
static SECURITY_STATUS SEC_ENTRY ntlm_AcceptSecurityContext(
 PCredHandle phCredential, PCtxtHandle phContext, PSecBufferDesc pInput,
 ULONG fContextReq, ULONG TargetDataRep, PCtxtHandle phNewContext, 
 PSecBufferDesc pOutput, ULONG *pfContextAttr, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;

    TRACE("%p %p %p %ld %ld %p %p %p %p\n", phCredential, phContext, pInput,
     fContextReq, TargetDataRep, phNewContext, pOutput, pfContextAttr,
     ptsExpiry);
    if (phCredential)
    {
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
    {
        ret = SEC_E_INVALID_HANDLE;
    }
    return ret;
}

/***********************************************************************
 *              CompleteAuthToken
 */
static SECURITY_STATUS SEC_ENTRY ntlm_CompleteAuthToken(PCtxtHandle phContext,
 PSecBufferDesc pToken)
{
    SECURITY_STATUS ret;

    TRACE("%p %p\n", phContext, pToken);
    if (phContext)
    {
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
    {
        ret = SEC_E_INVALID_HANDLE;
    }
    return ret;
}

/***********************************************************************
 *              DeleteSecurityContext
 */
static SECURITY_STATUS SEC_ENTRY ntlm_DeleteSecurityContext(PCtxtHandle phContext)
{
    SECURITY_STATUS ret;

    TRACE("%p\n", phContext);
    if (phContext)
    {
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
    {
        ret = SEC_E_INVALID_HANDLE;
    }
    return ret;
}

/***********************************************************************
 *              ApplyControlToken
 */
static SECURITY_STATUS SEC_ENTRY ntlm_ApplyControlToken(PCtxtHandle phContext,
 PSecBufferDesc pInput)
{
    SECURITY_STATUS ret;

    TRACE("%p %p\n", phContext, pInput);
    if (phContext)
    {
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
    {
        ret = SEC_E_INVALID_HANDLE;
    }
    return ret;
}

/***********************************************************************
 *              QueryContextAttributesW
 */
static SECURITY_STATUS SEC_ENTRY ntlm_QueryContextAttributesW(PCtxtHandle phContext,
 unsigned long ulAttribute, void *pBuffer)
{
    SECURITY_STATUS ret;

    /* FIXME: From reading wrapper.h, I think the dwUpper part of a context is
     * the SecurePackage part and the dwLower part is the actual context 
     * handle. It should be easy to extract the context attributes from that.
     */
    TRACE("%p %ld %p\n", phContext, ulAttribute, pBuffer);
    if (phContext)
    {
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
    {
        ret = SEC_E_INVALID_HANDLE;
    }
    return ret;
}

/***********************************************************************
 *              QueryContextAttributesA
 */
static SECURITY_STATUS SEC_ENTRY ntlm_QueryContextAttributesA(PCtxtHandle phContext,
 unsigned long ulAttribute, void *pBuffer)
{
    return ntlm_QueryContextAttributesW(phContext, ulAttribute, pBuffer);
}

/***********************************************************************
 *              ImpersonateSecurityContext
 */
static SECURITY_STATUS SEC_ENTRY ntlm_ImpersonateSecurityContext(PCtxtHandle phContext)
{
    SECURITY_STATUS ret;

    TRACE("%p\n", phContext);
    if (phContext)
    {
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
    {
        ret = SEC_E_INVALID_HANDLE;
    }
    return ret;
}

/***********************************************************************
 *              RevertSecurityContext
 */
static SECURITY_STATUS SEC_ENTRY ntlm_RevertSecurityContext(PCtxtHandle phContext)
{
    SECURITY_STATUS ret;

    TRACE("%p\n", phContext);
    if (phContext)
    {
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
    {
        ret = SEC_E_INVALID_HANDLE;
    }
    return ret;
}

/***********************************************************************
 *              MakeSignature
 */
static SECURITY_STATUS SEC_ENTRY ntlm_MakeSignature(PCtxtHandle phContext, ULONG fQOP,
 PSecBufferDesc pMessage, ULONG MessageSeqNo)
{
    SECURITY_STATUS ret;

    TRACE("%p %ld %p %ld\n", phContext, fQOP, pMessage, MessageSeqNo);
    if (phContext)
    {
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
    {
        ret = SEC_E_INVALID_HANDLE;
    }
    return ret;
}

/***********************************************************************
 *              VerifySignature
 */
static SECURITY_STATUS SEC_ENTRY ntlm_VerifySignature(PCtxtHandle phContext,
 PSecBufferDesc pMessage, ULONG MessageSeqNo, PULONG pfQOP)
{
    SECURITY_STATUS ret;

    TRACE("%p %p %ld %p\n", phContext, pMessage, MessageSeqNo, pfQOP);
    if (phContext)
    {
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
    {
        ret = SEC_E_INVALID_HANDLE;
    }
    return ret;
}



static SecurityFunctionTableA negoTableA = {
    1,
    NULL,   /* EnumerateSecurityPackagesA */
    ntlm_QueryCredentialsAttributesA,   /* QueryCredentialsAttributesA */
    ntlm_AcquireCredentialsHandleA,     /* AcquireCredentialsHandleA */
    FreeCredentialsHandle,              /* FreeCredentialsHandle */
    NULL,   /* Reserved2 */
    ntlm_InitializeSecurityContextA,    /* InitializeSecurityContextA */
    ntlm_AcceptSecurityContext,         /* AcceptSecurityContext */
    ntlm_CompleteAuthToken,             /* CompleteAuthToken */
    ntlm_DeleteSecurityContext,         /* DeleteSecurityContext */
    ntlm_ApplyControlToken,             /* ApplyControlToken */
    ntlm_QueryContextAttributesA,       /* QueryContextAttributesA */
    ntlm_ImpersonateSecurityContext,    /* ImpersonateSecurityContext */
    ntlm_RevertSecurityContext,         /* RevertSecurityContext */
    ntlm_MakeSignature,                 /* MakeSignature */
    ntlm_VerifySignature,               /* VerifySignature */
    FreeContextBuffer,                  /* FreeContextBuffer */
    NULL,   /* QuerySecurityPackageInfoA */
    NULL,   /* Reserved3 */
    NULL,   /* Reserved4 */
    NULL,   /* ExportSecurityContext */
    NULL,   /* ImportSecurityContextA */
    NULL,   /* AddCredentialsA */
    NULL,   /* Reserved8 */
    NULL,   /* QuerySecurityContextToken */
    NULL,   /* EncryptMessage */
    NULL,   /* DecryptMessage */
    NULL,   /* SetContextAttributesA */
};

static SecurityFunctionTableW negoTableW = {
    1,
    NULL,   /* EnumerateSecurityPackagesW */
    ntlm_QueryCredentialsAttributesW,   /* QueryCredentialsAttributesW */
    ntlm_AcquireCredentialsHandleW,     /* AcquireCredentialsHandleW */
    FreeCredentialsHandle,              /* FreeCredentialsHandle */
    NULL,   /* Reserved2 */
    ntlm_InitializeSecurityContextW,    /* InitializeSecurityContextW */
    ntlm_AcceptSecurityContext,         /* AcceptSecurityContext */
    ntlm_CompleteAuthToken,             /* CompleteAuthToken */
    ntlm_DeleteSecurityContext,         /* DeleteSecurityContext */
    ntlm_ApplyControlToken,             /* ApplyControlToken */
    ntlm_QueryContextAttributesW,       /* QueryContextAttributesW */
    ntlm_ImpersonateSecurityContext,    /* ImpersonateSecurityContext */
    ntlm_RevertSecurityContext,         /* RevertSecurityContext */
    ntlm_MakeSignature,                 /* MakeSignature */
    ntlm_VerifySignature,               /* VerifySignature */
    FreeContextBuffer,                  /* FreeContextBuffer */
    NULL,   /* QuerySecurityPackageInfoW */
    NULL,   /* Reserved3 */
    NULL,   /* Reserved4 */
    NULL,   /* ExportSecurityContext */
    NULL,   /* ImportSecurityContextW */
    NULL,   /* AddCredentialsW */
    NULL,   /* Reserved8 */
    NULL,   /* QuerySecurityContextToken */
    NULL,   /* EncryptMessage */
    NULL,   /* DecryptMessage */
    NULL,   /* SetContextAttributesW */
};

static WCHAR ntlm_comment_W[] = { 'N', 'T', 'L', 'M', ' ', 'S', 'e',
    'c', 'u', 'r', 'i', 't', 'y', ' ', 'P', 'a', 'c', 'k', 'a', 'g', 'e',0};

static CHAR ntlm_comment_A[] = "NTLM Security Package";

void SECUR32_initNTLMSP(void)
{
    SecureProvider *provider = SECUR32_addProvider(&negoTableA, &negoTableW,
            NULL);
    /* According to Windows, NTLM has the following capabilities. 
     */
    
    static const LONG caps =
        SECPKG_FLAG_INTEGRITY |
        SECPKG_FLAG_PRIVACY |
        SECPKG_FLAG_TOKEN_ONLY |
        SECPKG_FLAG_CONNECTION |
        SECPKG_FLAG_MULTI_REQUIRED |
        SECPKG_FLAG_IMPERSONATION |
        SECPKG_FLAG_ACCEPT_WIN32_NAME |
        SECPKG_FLAG_READONLY_WITH_CHECKSUM;

    static const USHORT version = 1;
    static const USHORT rpcid = 10;
    static const ULONG  max_token = 12000;
    const SecPkgInfoW infoW = { caps, version, rpcid, max_token, ntlm_name_W, 
        ntlm_comment_W};
    const SecPkgInfoA infoA = { caps, version, rpcid, max_token, ntlm_name_A,
        ntlm_comment_A};

    SECUR32_addPackages(provider, 1L, &infoA, &infoW);        
    
}
