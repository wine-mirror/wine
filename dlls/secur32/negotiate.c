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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * This file implements the negotiate provider.
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

/* Disable for now, see longer comment for SECUR32_initNegotiateSP below */
#if 0
static char nego_name_A[] = "Negotiate";
static WCHAR nego_name_W[] = {'N', 'e', 'g', 'o', 't', 'i', 'a', 't', 'e', 0};
#endif

static SECURITY_STATUS nego_QueryCredentialsAttributes(PCredHandle phCredential,
        ULONG ulAttribute, PVOID pBuffer)
{
    SECURITY_STATUS ret;

    /* FIXME: More attributes to be added here. Need to fix the sspi.h header
     * for that, too.
     */
    switch(ulAttribute)
    {
        default:
            ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    return ret;
}

/***********************************************************************
 *              QueryCredentialsAttributesA
 */
static SECURITY_STATUS SEC_ENTRY nego_QueryCredentialsAttributesA(
        PCredHandle phCredential, ULONG ulAttribute, PVOID pBuffer)
{
    SECURITY_STATUS ret;

    TRACE("(%p, %d, %p)\n", phCredential, ulAttribute, pBuffer);

    switch(ulAttribute)
    {
        case SECPKG_CRED_ATTR_NAMES:
            FIXME("SECPKG_CRED_ATTR_NAMES: stub\n");
            ret = SEC_E_UNSUPPORTED_FUNCTION;
            break;
        default:
            ret = nego_QueryCredentialsAttributes(phCredential, ulAttribute, 
                    pBuffer);
    }
    return ret;
}

/***********************************************************************
 *              QueryCredentialsAttributesW
 */
static SECURITY_STATUS SEC_ENTRY nego_QueryCredentialsAttributesW(
        PCredHandle phCredential, ULONG ulAttribute, PVOID pBuffer)
{
    SECURITY_STATUS ret;

    TRACE("(%p, %d, %p)\n", phCredential, ulAttribute, pBuffer);

    switch(ulAttribute)
    {
        case SECPKG_CRED_ATTR_NAMES:
            FIXME("SECPKG_CRED_ATTR_NAMES: stub\n");
            ret = SEC_E_UNSUPPORTED_FUNCTION;
            break;
        default:
            ret = nego_QueryCredentialsAttributes(phCredential, ulAttribute, 
                    pBuffer);
    }
    return ret;
}


/***********************************************************************
 *              AcquireCredentialsHandleA
 */
static SECURITY_STATUS SEC_ENTRY nego_AcquireCredentialsHandleA(
 SEC_CHAR *pszPrincipal, SEC_CHAR *pszPackage, ULONG fCredentialUse,
 PLUID pLogonID, PVOID pAuthData, SEC_GET_KEY_FN pGetKeyFn,
 PVOID pGetKeyArgument, PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    TRACE("(%s, %s, 0x%08x, %p, %p, %p, %p, %p, %p) stub\n",
     debugstr_a(pszPrincipal), debugstr_a(pszPackage), fCredentialUse,
     pLogonID, pAuthData, pGetKeyFn, pGetKeyArgument, phCredential, ptsExpiry);
    return SEC_E_UNSUPPORTED_FUNCTION;
}

/***********************************************************************
 *              AcquireCredentialsHandleW
 */
static SECURITY_STATUS SEC_ENTRY nego_AcquireCredentialsHandleW(
 SEC_WCHAR *pszPrincipal, SEC_WCHAR *pszPackage, ULONG fCredentialUse,
 PLUID pLogonID, PVOID pAuthData, SEC_GET_KEY_FN pGetKeyFn,
 PVOID pGetKeyArgument, PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    TRACE("(%s, %s, 0x%08x, %p, %p, %p, %p, %p, %p) stub\n",
     debugstr_w(pszPrincipal), debugstr_w(pszPackage), fCredentialUse,
     pLogonID, pAuthData, pGetKeyFn, pGetKeyArgument, phCredential, ptsExpiry);
    return SEC_E_UNSUPPORTED_FUNCTION;
}

/***********************************************************************
 *              InitializeSecurityContextA
 */
static SECURITY_STATUS SEC_ENTRY nego_InitializeSecurityContextA(
 PCredHandle phCredential, PCtxtHandle phContext, SEC_CHAR *pszTargetName, 
 ULONG fContextReq, ULONG Reserved1, ULONG TargetDataRep, 
 PSecBufferDesc pInput, ULONG Reserved2, PCtxtHandle phNewContext, 
 PSecBufferDesc pOutput, ULONG *pfContextAttr, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;

    TRACE("%p %p %s 0x%08x %d %d %p %d %p %p %p %p\n", phCredential, phContext,
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
static SECURITY_STATUS SEC_ENTRY nego_InitializeSecurityContextW(
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
static SECURITY_STATUS SEC_ENTRY nego_AcceptSecurityContext(
 PCredHandle phCredential, PCtxtHandle phContext, PSecBufferDesc pInput,
 ULONG fContextReq, ULONG TargetDataRep, PCtxtHandle phNewContext, 
 PSecBufferDesc pOutput, ULONG *pfContextAttr, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;

    TRACE("%p %p %p %d %d %p %p %p %p\n", phCredential, phContext, pInput,
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
static SECURITY_STATUS SEC_ENTRY nego_CompleteAuthToken(PCtxtHandle phContext,
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
static SECURITY_STATUS SEC_ENTRY nego_DeleteSecurityContext(PCtxtHandle phContext)
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
static SECURITY_STATUS SEC_ENTRY nego_ApplyControlToken(PCtxtHandle phContext,
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
static SECURITY_STATUS SEC_ENTRY nego_QueryContextAttributesW(PCtxtHandle phContext,
 ULONG ulAttribute, void *pBuffer)
{
    SECURITY_STATUS ret;

    /* FIXME: From reading wrapper.h, I think the dwUpper part of a context is
     * the SecurePackage part and the dwLower part is the actual context 
     * handle. It should be easy to extract the context attributes from that.
     */
    TRACE("%p %d %p\n", phContext, ulAttribute, pBuffer);
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
static SECURITY_STATUS SEC_ENTRY nego_QueryContextAttributesA(PCtxtHandle phContext,
 ULONG ulAttribute, void *pBuffer)
{
    return nego_QueryContextAttributesW(phContext, ulAttribute, pBuffer);
}

/***********************************************************************
 *              ImpersonateSecurityContext
 */
static SECURITY_STATUS SEC_ENTRY nego_ImpersonateSecurityContext(PCtxtHandle phContext)
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
static SECURITY_STATUS SEC_ENTRY nego_RevertSecurityContext(PCtxtHandle phContext)
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
static SECURITY_STATUS SEC_ENTRY nego_MakeSignature(PCtxtHandle phContext, ULONG fQOP,
 PSecBufferDesc pMessage, ULONG MessageSeqNo)
{
    SECURITY_STATUS ret;

    TRACE("%p %d %p %d\n", phContext, fQOP, pMessage, MessageSeqNo);
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
static SECURITY_STATUS SEC_ENTRY nego_VerifySignature(PCtxtHandle phContext,
 PSecBufferDesc pMessage, ULONG MessageSeqNo, PULONG pfQOP)
{
    SECURITY_STATUS ret;

    TRACE("%p %p %d %p\n", phContext, pMessage, MessageSeqNo, pfQOP);
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



static const SecurityFunctionTableA negoTableA = {
    1,
    NULL,   /* EnumerateSecurityPackagesA */
    nego_QueryCredentialsAttributesA,   /* QueryCredentialsAttributesA */
    nego_AcquireCredentialsHandleA,     /* AcquireCredentialsHandleA */
    FreeCredentialsHandle,              /* FreeCredentialsHandle */
    NULL,   /* Reserved2 */
    nego_InitializeSecurityContextA,    /* InitializeSecurityContextA */
    nego_AcceptSecurityContext,         /* AcceptSecurityContext */
    nego_CompleteAuthToken,             /* CompleteAuthToken */
    nego_DeleteSecurityContext,         /* DeleteSecurityContext */
    nego_ApplyControlToken,             /* ApplyControlToken */
    nego_QueryContextAttributesA,       /* QueryContextAttributesA */
    nego_ImpersonateSecurityContext,    /* ImpersonateSecurityContext */
    nego_RevertSecurityContext,         /* RevertSecurityContext */
    nego_MakeSignature,                 /* MakeSignature */
    nego_VerifySignature,               /* VerifySignature */
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

static const SecurityFunctionTableW negoTableW = {
    1,
    NULL,   /* EnumerateSecurityPackagesW */
    nego_QueryCredentialsAttributesW,   /* QueryCredentialsAttributesW */
    nego_AcquireCredentialsHandleW,     /* AcquireCredentialsHandleW */
    FreeCredentialsHandle,              /* FreeCredentialsHandle */
    NULL,   /* Reserved2 */
    nego_InitializeSecurityContextW,    /* InitializeSecurityContextW */
    nego_AcceptSecurityContext,         /* AcceptSecurityContext */
    nego_CompleteAuthToken,             /* CompleteAuthToken */
    nego_DeleteSecurityContext,         /* DeleteSecurityContext */
    nego_ApplyControlToken,             /* ApplyControlToken */
    nego_QueryContextAttributesW,       /* QueryContextAttributesW */
    nego_ImpersonateSecurityContext,    /* ImpersonateSecurityContext */
    nego_RevertSecurityContext,         /* RevertSecurityContext */
    nego_MakeSignature,                 /* MakeSignature */
    nego_VerifySignature,               /* VerifySignature */
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

/* Disable for now, see comment below.*/
#if 0
static WCHAR negotiate_comment_W[] = { 'M', 'i', 'c', 'r', 'o', 's', 'o',
    'f', 't', ' ', 'P', 'a', 'c', 'k', 'a', 'g', 'e', ' ', 'N', 'e', 'g', 'o',
    't', 'i', 'a', 't', 'o', 'r', 0};

static CHAR negotiate_comment_A[] = "Microsoft Package Negotiator";
#endif


void SECUR32_initNegotiateSP(void)
{
/* Disable until we really implement a Negotiate provider.
 * For now, the NTLM provider will pretend to be the Negotiate provider as well.
 * Windows seems to be able to deal with it, and it makes several programs
 * happy. */
#if 0
    SecureProvider *provider = SECUR32_addProvider(&negoTableA, &negoTableW,
            NULL);
    /* According to Windows, Negotiate has the following capabilities. 
     */
    
    static const LONG caps = 
        SECPKG_FLAG_INTEGRITY |
	    SECPKG_FLAG_PRIVACY |
	    SECPKG_FLAG_CONNECTION |
        SECPKG_FLAG_MULTI_REQUIRED |
	    SECPKG_FLAG_EXTENDED_ERROR |
	    SECPKG_FLAG_IMPERSONATION |
	    SECPKG_FLAG_ACCEPT_WIN32_NAME |
	    SECPKG_FLAG_READONLY_WITH_CHECKSUM;

    static const USHORT version = 1;
    static const USHORT rpcid = 15;
    static const ULONG  max_token = 12000;
    const SecPkgInfoW infoW = { caps, version, rpcid, max_token, nego_name_W,
        negotiate_comment_W};
    const SecPkgInfoA infoA = { caps, version, rpcid, max_token, nego_name_A,
        negotiate_comment_A};

    SECUR32_addPackages(provider, 1L, &infoA, &infoW);
#endif
}
