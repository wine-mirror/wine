/* Copyright (C) 2004 Juan Lang
 *
 * Implements secur32 functions that forward to (wrap) an SSP's implementation.
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
 */
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "sspi.h"
#include "secur32_priv.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(secur32);

/* Tries to allocate a new SecHandle, into which it stores package (in
 * phSec->dwUpper) and a copy of realHandle (allocated with SECUR32_ALLOC,
 * and stored in phSec->dwLower).  SecHandle is equivalent to both a
 * CredHandle and a CtxtHandle.
 */
static SECURITY_STATUS SECUR32_makeSecHandle(PSecHandle phSec,
 SecurePackage *package, PSecHandle realHandle)
{
    SECURITY_STATUS ret;

    if (phSec && package && realHandle)
    {
        PSecHandle newSec = (PSecHandle)SECUR32_ALLOC(sizeof(SecHandle));

        if (newSec)
        {
            memcpy(newSec, realHandle, sizeof(*realHandle));
            phSec->dwUpper = (ULONG_PTR)package;
            phSec->dwLower = (ULONG_PTR)newSec;
            ret = SEC_E_OK;
        }
        else
            ret = SEC_E_INSUFFICIENT_MEMORY;
    }
    else
        ret = SEC_E_INVALID_HANDLE;
    return ret;
}

SECURITY_STATUS SEC_ENTRY AcquireCredentialsHandleA(
 SEC_CHAR *pszPrincipal, SEC_CHAR *pszPackage, ULONG fCredentialsUse,
 PLUID pvLogonID, PVOID pAuthData, SEC_GET_KEY_FN pGetKeyFn,
 PVOID pvGetKeyArgument, PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;

    TRACE("%s %s %ld %p %p %p %p %p %p\n", debugstr_a(pszPrincipal),
     debugstr_a(pszPackage), fCredentialsUse, pvLogonID, pAuthData, pGetKeyFn,
     pvGetKeyArgument, phCredential, ptsExpiry);
    if (pszPackage)
    {
        SecurePackage *package = SECUR32_findPackageA(pszPackage);

        if (package && package->provider)
        {
            if (package->provider->fnTableA.AcquireCredentialsHandleA)
            {
                CredHandle myCred;

                ret = package->provider->fnTableA.AcquireCredentialsHandleA(
                 pszPrincipal, pszPackage, fCredentialsUse, pvLogonID,
                 pAuthData, pGetKeyFn, pvGetKeyArgument, &myCred,
                 ptsExpiry);
                if (ret == SEC_E_OK)
                {
                    ret = SECUR32_makeSecHandle(phCredential, package, &myCred);
                    if (ret != SEC_E_OK)
                        package->provider->fnTableW.FreeCredentialsHandle(
                         &myCred);
                }
            }
            else
                ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_SECPKG_NOT_FOUND;
    }
    else
        ret = SEC_E_SECPKG_NOT_FOUND;
    return ret;
}

SECURITY_STATUS SEC_ENTRY AcquireCredentialsHandleW(
 SEC_WCHAR *pszPrincipal, SEC_WCHAR *pszPackage, ULONG fCredentialsUse,
 PLUID pvLogonID, PVOID pAuthData, SEC_GET_KEY_FN pGetKeyFn,
 PVOID pvGetKeyArgument, PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;

    TRACE("%s %s %ld %p %p %p %p %p %p\n", debugstr_w(pszPrincipal),
     debugstr_w(pszPackage), fCredentialsUse, pvLogonID, pAuthData, pGetKeyFn,
     pvGetKeyArgument, phCredential, ptsExpiry);
    if (pszPackage)
    {
        SecurePackage *package = SECUR32_findPackageW(pszPackage);

        if (package && package->provider)
        {
            if (package->provider->fnTableW.AcquireCredentialsHandleW)
            {
                CredHandle myCred;

                ret = package->provider->fnTableW.AcquireCredentialsHandleW(
                 pszPrincipal, pszPackage, fCredentialsUse, pvLogonID,
                 pAuthData, pGetKeyFn, pvGetKeyArgument, &myCred,
                 ptsExpiry);
                if (ret == SEC_E_OK)
                {
                    ret = SECUR32_makeSecHandle(phCredential, package, &myCred);
                    if (ret != SEC_E_OK)
                        package->provider->fnTableW.FreeCredentialsHandle(
                         &myCred);
                }
            }
            else
                ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_SECPKG_NOT_FOUND;
    }
    else
        ret = SEC_E_SECPKG_NOT_FOUND;
    return ret;
}

SECURITY_STATUS SEC_ENTRY FreeCredentialsHandle(
 PCredHandle phCredential)
{
    SECURITY_STATUS ret;

    TRACE("%p\n", phCredential);
    if (phCredential)
    {
        SecurePackage *package = (SecurePackage *)phCredential->dwUpper;
        PCredHandle cred = (PCredHandle)phCredential->dwLower;

        if (package && package->provider &&
         package->provider->fnTableW.FreeCredentialsHandle)
            ret = package->provider->fnTableW.FreeCredentialsHandle(cred);
        else
            ret = SEC_E_INVALID_HANDLE;
        SECUR32_FREE(cred);
    }
    else
        ret = SEC_E_INVALID_HANDLE;
    return ret;
}

SECURITY_STATUS SEC_ENTRY QueryCredentialsAttributesA(
 PCredHandle phCredential, unsigned long ulAttribute, void *pBuffer)
{
    SECURITY_STATUS ret;

    TRACE("%p %ld %p\n", phCredential, ulAttribute, pBuffer);
    if (phCredential)
    {
        SecurePackage *package = (SecurePackage *)phCredential->dwUpper;
        PCredHandle cred = (PCredHandle)phCredential->dwLower;

        if (package && package->provider)
        {
            if (package->provider->fnTableA.QueryCredentialsAttributesA)
                ret = package->provider->fnTableA.QueryCredentialsAttributesA(
                 cred, ulAttribute, pBuffer);
            else
                ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INVALID_HANDLE;
    }
    else
        ret = SEC_E_INVALID_HANDLE;
    return ret;
}

SECURITY_STATUS SEC_ENTRY QueryCredentialsAttributesW(
 PCredHandle phCredential, unsigned long ulAttribute, void *pBuffer)
{
    SECURITY_STATUS ret;

    TRACE("%p %ld %p\n", phCredential, ulAttribute, pBuffer);
    if (phCredential)
    {
        SecurePackage *package = (SecurePackage *)phCredential->dwUpper;
        PCredHandle cred = (PCredHandle)phCredential->dwLower;

        if (package && package->provider)
        {
            if (package->provider->fnTableW.QueryCredentialsAttributesW)
                ret = package->provider->fnTableW.QueryCredentialsAttributesW(
                 cred, ulAttribute, pBuffer);
            else
                ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INVALID_HANDLE;
    }
    else
        ret = SEC_E_INVALID_HANDLE;
    return ret;
}

SECURITY_STATUS SEC_ENTRY InitializeSecurityContextA(
 PCredHandle phCredential, PCtxtHandle phContext,
 SEC_CHAR *pszTargetName, unsigned long fContextReq,
 unsigned long Reserved1, unsigned long TargetDataRep, PSecBufferDesc pInput,
 unsigned long Reserved2, PCtxtHandle phNewContext, PSecBufferDesc pOutput,
 unsigned long *pfContextAttr, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;

    TRACE("%p %p %s %ld %ld %ld %p %ld %p %p %p %p\n", phCredential, phContext,
     debugstr_a(pszTargetName), fContextReq, Reserved1, TargetDataRep, pInput,
     Reserved1, phNewContext, pOutput, pfContextAttr, ptsExpiry);
    if (phCredential)
    {
        SecurePackage *package = (SecurePackage *)phCredential->dwUpper;
        PCredHandle cred = (PCredHandle)phCredential->dwLower;

        if (package && package->provider)
        {
            if (package->provider->fnTableA.InitializeSecurityContextA)
            {
                CtxtHandle myCtxt;

                ret = package->provider->fnTableA.InitializeSecurityContextA(
                 cred, phContext ? &myCtxt : NULL, pszTargetName, fContextReq,
                 Reserved1, TargetDataRep, pInput, Reserved2, &myCtxt,
                 pOutput, pfContextAttr, ptsExpiry);
                if (ret == SEC_E_OK)
                {
                    ret = SECUR32_makeSecHandle(phContext, package, &myCtxt);
                    if (ret != SEC_E_OK)
                        package->provider->fnTableW.DeleteSecurityContext(
                         &myCtxt);
                }
            }
            else
                ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INVALID_HANDLE;
    }
    else
        ret = SEC_E_INVALID_HANDLE;
    return ret;
}

SECURITY_STATUS SEC_ENTRY InitializeSecurityContextW(
 PCredHandle phCredential, PCtxtHandle phContext,
 SEC_WCHAR *pszTargetName, unsigned long fContextReq,
 unsigned long Reserved1, unsigned long TargetDataRep, PSecBufferDesc pInput,
 unsigned long Reserved2, PCtxtHandle phNewContext, PSecBufferDesc pOutput,
 unsigned long *pfContextAttr, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;

    TRACE("%p %p %s %ld %ld %ld %p %ld %p %p %p %p\n", phCredential, phContext,
     debugstr_w(pszTargetName), fContextReq, Reserved1, TargetDataRep, pInput,
     Reserved1, phNewContext, pOutput, pfContextAttr, ptsExpiry);
    if (phCredential)
    {
        SecurePackage *package = (SecurePackage *)phCredential->dwUpper;
        PCredHandle cred = (PCredHandle)phCredential->dwLower;

        if (package && package->provider)
        {
            if (package->provider->fnTableW.QueryCredentialsAttributesW)
            {
                CtxtHandle myCtxt;

                ret = package->provider->fnTableW.InitializeSecurityContextW(
                 cred, phContext ? &myCtxt : NULL, pszTargetName, fContextReq,
                 Reserved1, TargetDataRep, pInput, Reserved2, &myCtxt,
                 pOutput, pfContextAttr, ptsExpiry);
                if (ret == SEC_E_OK)
                {
                    ret = SECUR32_makeSecHandle(phContext, package, &myCtxt);
                    if (ret != SEC_E_OK)
                        package->provider->fnTableW.DeleteSecurityContext(
                         &myCtxt);
                }
            }
            else
                ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INVALID_HANDLE;
    }
    else
        ret = SEC_E_INVALID_HANDLE;
    return ret;
}

SECURITY_STATUS SEC_ENTRY AcceptSecurityContext(
 PCredHandle phCredential, PCtxtHandle phContext, PSecBufferDesc pInput,
 unsigned long fContextReq, unsigned long TargetDataRep,
 PCtxtHandle phNewContext, PSecBufferDesc pOutput,
 unsigned long *pfContextAttr, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;

    TRACE("%p %p %p %ld %ld %p %p %p %p\n", phCredential, phContext, pInput,
     fContextReq, TargetDataRep, phNewContext, pOutput, pfContextAttr,
     ptsExpiry);
    if (phCredential)
    {
        SecurePackage *package = (SecurePackage *)phCredential->dwUpper;
        PCredHandle cred = (PCredHandle)phCredential->dwLower;

        if (package && package->provider)
        {
            if (package->provider->fnTableW.AcceptSecurityContext)
            {
                CtxtHandle myCtxt;

                ret = package->provider->fnTableW.AcceptSecurityContext(
                 cred, phContext ? &myCtxt : NULL, pInput, fContextReq,
                 TargetDataRep, &myCtxt, pOutput, pfContextAttr, ptsExpiry);
                if (ret == SEC_E_OK)
                {
                    ret = SECUR32_makeSecHandle(phContext, package, &myCtxt);
                    if (ret != SEC_E_OK)
                        package->provider->fnTableW.DeleteSecurityContext(
                         &myCtxt);
                }
            }
            else
                ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INVALID_HANDLE;
    }
    else
        ret = SEC_E_INVALID_HANDLE;
    return ret;
}

SECURITY_STATUS SEC_ENTRY CompleteAuthToken(PCtxtHandle phContext,
 PSecBufferDesc pToken)
{
    SECURITY_STATUS ret;

    TRACE("%p %p\n", phContext, pToken);
    if (phContext)
    {
        SecurePackage *package = (SecurePackage *)phContext->dwUpper;
        PCtxtHandle ctxt = (PCtxtHandle)phContext->dwLower;

        if (package && package->provider)
        {
            if (package->provider->fnTableW.CompleteAuthToken)
                ret = package->provider->fnTableW.CompleteAuthToken(ctxt,
                 pToken);
            else
                ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INVALID_HANDLE;
    }
    else
        ret = SEC_E_INVALID_HANDLE;
    return ret;
}

SECURITY_STATUS SEC_ENTRY DeleteSecurityContext(PCtxtHandle phContext)
{
    SECURITY_STATUS ret;

    TRACE("%p\n", phContext);
    if (phContext)
    {
        SecurePackage *package = (SecurePackage *)phContext->dwUpper;
        PCtxtHandle ctxt = (PCtxtHandle)phContext->dwLower;

        if (package && package->provider &&
         package->provider->fnTableW.DeleteSecurityContext)
            ret = package->provider->fnTableW.DeleteSecurityContext(ctxt);
        else
            ret = SEC_E_INVALID_HANDLE;
        SECUR32_FREE(ctxt);
    }
    else
        ret = SEC_E_INVALID_HANDLE;
    return ret;
}

SECURITY_STATUS SEC_ENTRY ApplyControlToken(PCtxtHandle phContext,
 PSecBufferDesc pInput)
{
    SECURITY_STATUS ret;

    TRACE("%p %p\n", phContext, pInput);
    if (phContext)
    {
        SecurePackage *package = (SecurePackage *)phContext->dwUpper;
        PCtxtHandle ctxt = (PCtxtHandle)phContext->dwLower;

        if (package && package->provider)
        {
            if (package->provider->fnTableW.ApplyControlToken)
                ret = package->provider->fnTableW.ApplyControlToken(
                 ctxt, pInput);
            else
                ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INVALID_HANDLE;
    }
    else
        ret = SEC_E_INVALID_HANDLE;
    return ret;
}

SECURITY_STATUS SEC_ENTRY QueryContextAttributesA(PCtxtHandle phContext,
 unsigned long ulAttribute, void *pBuffer)
{
    SECURITY_STATUS ret;

    TRACE("%p %ld %p\n", phContext, ulAttribute, pBuffer);
    if (phContext)
    {
        SecurePackage *package = (SecurePackage *)phContext->dwUpper;
        PCtxtHandle ctxt = (PCtxtHandle)phContext->dwLower;

        if (package && package->provider)
        {
            if (package->provider->fnTableA.QueryContextAttributesA)
                ret = package->provider->fnTableA.QueryContextAttributesA(
                 ctxt, ulAttribute, pBuffer);
            else
                ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INVALID_HANDLE;
    }
    else
        ret = SEC_E_INVALID_HANDLE;
    return ret;
}

SECURITY_STATUS SEC_ENTRY QueryContextAttributesW(PCtxtHandle phContext,
 unsigned long ulAttribute, void *pBuffer)
{
    SECURITY_STATUS ret;

    TRACE("%p %ld %p\n", phContext, ulAttribute, pBuffer);
    if (phContext)
    {
        SecurePackage *package = (SecurePackage *)phContext->dwUpper;
        PCtxtHandle ctxt = (PCtxtHandle)phContext->dwLower;

        if (package && package->provider)
        {
            if (package->provider->fnTableW.QueryContextAttributesW)
                ret = package->provider->fnTableW.QueryContextAttributesW(
                 ctxt, ulAttribute, pBuffer);
            else
                ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INVALID_HANDLE;
    }
    else
        ret = SEC_E_INVALID_HANDLE;
    return ret;
}

SECURITY_STATUS SEC_ENTRY ImpersonateSecurityContext(PCtxtHandle phContext)
{
    SECURITY_STATUS ret;

    TRACE("%p\n", phContext);
    if (phContext)
    {
        SecurePackage *package = (SecurePackage *)phContext->dwUpper;
        PCtxtHandle ctxt = (PCtxtHandle)phContext->dwLower;

        if (package && package->provider)
        {
            if (package->provider->fnTableW.ImpersonateSecurityContext)
                ret = package->provider->fnTableW.ImpersonateSecurityContext(
                 ctxt);
            else
                ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INVALID_HANDLE;
    }
    else
        ret = SEC_E_INVALID_HANDLE;
    return ret;
}

SECURITY_STATUS SEC_ENTRY RevertSecurityContext(PCtxtHandle phContext)
{
    SECURITY_STATUS ret;

    TRACE("%p\n", phContext);
    if (phContext)
    {
        SecurePackage *package = (SecurePackage *)phContext->dwUpper;
        PCtxtHandle ctxt = (PCtxtHandle)phContext->dwLower;

        if (package && package->provider)
        {
            if (package->provider->fnTableW.RevertSecurityContext)
                ret = package->provider->fnTableW.RevertSecurityContext(
                 ctxt);
            else
                ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INVALID_HANDLE;
    }
    else
        ret = SEC_E_INVALID_HANDLE;
    return ret;
}

SECURITY_STATUS SEC_ENTRY MakeSignature(PCtxtHandle phContext, ULONG fQOP,
 PSecBufferDesc pMessage, ULONG MessageSeqNo)
{
    SECURITY_STATUS ret;

    TRACE("%p %ld %p %ld\n", phContext, fQOP, pMessage, MessageSeqNo);
    if (phContext)
    {
        SecurePackage *package = (SecurePackage *)phContext->dwUpper;
        PCtxtHandle ctxt = (PCtxtHandle)phContext->dwLower;

        if (package && package->provider)
        {
            if (package->provider->fnTableW.MakeSignature)
                ret = package->provider->fnTableW.MakeSignature(
                 ctxt, fQOP, pMessage, MessageSeqNo);
            else
                ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INVALID_HANDLE;
    }
    else
        ret = SEC_E_INVALID_HANDLE;
    return ret;
}

SECURITY_STATUS SEC_ENTRY VerifySignature(PCtxtHandle phContext,
 PSecBufferDesc pMessage, ULONG MessageSeqNo, PULONG pfQOP)
{
    SECURITY_STATUS ret;

    TRACE("%p %p %ld %p\n", phContext, pMessage, MessageSeqNo, pfQOP);
    if (phContext)
    {
        SecurePackage *package = (SecurePackage *)phContext->dwUpper;
        PCtxtHandle ctxt = (PCtxtHandle)phContext->dwLower;

        if (package && package->provider)
        {
            if (package->provider->fnTableW.VerifySignature)
                ret = package->provider->fnTableW.VerifySignature(
                 ctxt, pMessage, MessageSeqNo, pfQOP);
            else
                ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INVALID_HANDLE;
    }
    else
        ret = SEC_E_INVALID_HANDLE;
    return ret;
}

SECURITY_STATUS SEC_ENTRY QuerySecurityPackageInfoA(SEC_CHAR *pszPackageName,
 PSecPkgInfoA *ppPackageInfo)
{
    SECURITY_STATUS ret;
   
    TRACE("%s %p\n", debugstr_a(pszPackageName), ppPackageInfo);
    if (pszPackageName)
    {
        SecurePackage *package = SECUR32_findPackageA(pszPackageName);

        if (package)
        {
            size_t bytesNeeded = sizeof(SecPkgInfoA);
            int nameLen = 0, commentLen = 0;

            if (package->infoW.Name)
            {
                nameLen = WideCharToMultiByte(CP_ACP, 0,
                 package->infoW.Name, -1, NULL, 0, NULL, NULL);
                bytesNeeded += nameLen;
            }
            if (package->infoW.Comment)
            {
                commentLen = WideCharToMultiByte(CP_ACP, 0,
                 package->infoW.Comment, -1, NULL, 0, NULL, NULL);
                bytesNeeded += commentLen;
            }
            *ppPackageInfo = (PSecPkgInfoA)SECUR32_ALLOC(bytesNeeded);
            if (*ppPackageInfo)
            {
                PSTR nextString = (PSTR)((PBYTE)*ppPackageInfo +
                 sizeof(SecPkgInfoA));

                memcpy(*ppPackageInfo, &package->infoW, sizeof(package->infoW));
                if (package->infoW.Name)
                {
                    (*ppPackageInfo)->Name = nextString;
                    nextString += WideCharToMultiByte(CP_ACP, 0,
                     package->infoW.Name, -1, nextString, nameLen, NULL, NULL);
                }
                else
                    (*ppPackageInfo)->Name = NULL;
                if (package->infoW.Comment)
                {
                    (*ppPackageInfo)->Comment = nextString;
                    nextString += WideCharToMultiByte(CP_ACP, 0,
                     package->infoW.Comment, -1, nextString, commentLen, NULL,
                     NULL);
                }
                else
                    (*ppPackageInfo)->Comment = NULL;
                ret = SEC_E_OK;
            }
            else
                ret = SEC_E_INSUFFICIENT_MEMORY;
        }
        else
            ret = SEC_E_SECPKG_NOT_FOUND;
    }
    else
        ret = SEC_E_SECPKG_NOT_FOUND;
    return ret;
}

SECURITY_STATUS SEC_ENTRY QuerySecurityPackageInfoW(SEC_WCHAR *pszPackageName,
 PSecPkgInfoW *ppPackageInfo)
{
    SECURITY_STATUS ret;
    SecurePackage *package = SECUR32_findPackageW(pszPackageName);

    TRACE("%s %p\n", debugstr_w(pszPackageName), ppPackageInfo);
    if (package)
    {
        size_t bytesNeeded = sizeof(SecPkgInfoW);
        int nameLen = 0, commentLen = 0;

        if (package->infoW.Name)
        {
            nameLen = lstrlenW(package->infoW.Name) + 1;
            bytesNeeded += nameLen * sizeof(WCHAR);
        }
        if (package->infoW.Comment)
        {
            commentLen = lstrlenW(package->infoW.Comment) + 1;
            bytesNeeded += commentLen * sizeof(WCHAR);
        }
        *ppPackageInfo = (PSecPkgInfoW)SECUR32_ALLOC(bytesNeeded);
        if (*ppPackageInfo)
        {
            PWSTR nextString = (PWSTR)((PBYTE)*ppPackageInfo +
             sizeof(SecPkgInfoW));

            memcpy(*ppPackageInfo, &package->infoW, sizeof(package->infoW));
            if (package->infoW.Name)
            {
                (*ppPackageInfo)->Name = nextString;
                lstrcpynW(nextString, package->infoW.Name, nameLen);
                nextString += nameLen;
            }
            else
                (*ppPackageInfo)->Name = NULL;
            if (package->infoW.Comment)
            {
                (*ppPackageInfo)->Comment = nextString;
                lstrcpynW(nextString, package->infoW.Comment, commentLen);
                nextString += commentLen;
            }
            else
                (*ppPackageInfo)->Comment = NULL;
            ret = SEC_E_OK;
        }
        else
            ret = SEC_E_INSUFFICIENT_MEMORY;
    }
    else
        ret = SEC_E_SECPKG_NOT_FOUND;
    return ret;
}

SECURITY_STATUS SEC_ENTRY ExportSecurityContext(PCtxtHandle phContext,
 ULONG fFlags, PSecBuffer pPackedContext, void **pToken)
{
    SECURITY_STATUS ret;

    TRACE("%p %ld %p %p\n", phContext, fFlags, pPackedContext, pToken);
    if (phContext)
    {
        SecurePackage *package = (SecurePackage *)phContext->dwUpper;
        PCtxtHandle ctxt = (PCtxtHandle)phContext->dwLower;

        if (package && package->provider)
        {
            if (package->provider->fnTableW.ExportSecurityContext)
                ret = package->provider->fnTableW.ExportSecurityContext(
                 ctxt, fFlags, pPackedContext, pToken);
            else
                ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INVALID_HANDLE;
    }
    else
        ret = SEC_E_INVALID_HANDLE;
    return ret;
}

SECURITY_STATUS SEC_ENTRY ImportSecurityContextA(SEC_CHAR *pszPackage,
 PSecBuffer pPackedContext, void *Token, PCtxtHandle phContext)
{
    SECURITY_STATUS ret;
    SecurePackage *package = SECUR32_findPackageA(pszPackage);
 
    TRACE("%s %p %p %p\n", debugstr_a(pszPackage), pPackedContext, Token,
     phContext);
    if (package && package->provider)
    {
        if (package->provider->fnTableA.ImportSecurityContextA)
        {
            CtxtHandle myCtxt;

            ret = package->provider->fnTableA.ImportSecurityContextA(
             pszPackage, pPackedContext, Token, &myCtxt);
            if (ret == SEC_E_OK)
            {
                ret = SECUR32_makeSecHandle(phContext, package, &myCtxt);
                if (ret != SEC_E_OK)
                    package->provider->fnTableW.DeleteSecurityContext(&myCtxt);
            }
        }
        else
            ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
        ret = SEC_E_SECPKG_NOT_FOUND;
    return ret;

}

SECURITY_STATUS SEC_ENTRY ImportSecurityContextW(SEC_WCHAR *pszPackage,
 PSecBuffer pPackedContext, void *Token, PCtxtHandle phContext)
{
    SECURITY_STATUS ret;
    SecurePackage *package = SECUR32_findPackageW(pszPackage);

    TRACE("%s %p %p %p\n", debugstr_w(pszPackage), pPackedContext, Token,
     phContext);
    if (package && package->provider)
    {
        if (package->provider->fnTableW.ImportSecurityContextW)
        {
            CtxtHandle myCtxt;

            ret = package->provider->fnTableW.ImportSecurityContextW(
             pszPackage, pPackedContext, Token, &myCtxt);
            if (ret == SEC_E_OK)
            {
                ret = SECUR32_makeSecHandle(phContext, package, &myCtxt);
                if (ret != SEC_E_OK)
                    package->provider->fnTableW.DeleteSecurityContext(&myCtxt);
            }
        }
        else
            ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
        ret = SEC_E_SECPKG_NOT_FOUND;
    return ret;
}

SECURITY_STATUS SEC_ENTRY AddCredentialsA(PCredHandle hCredentials,
 SEC_CHAR *pszPrincipal, SEC_CHAR *pszPackage, unsigned long fCredentialUse,
 void *pAuthData, SEC_GET_KEY_FN pGetKeyFn, void *pvGetKeyArgument,
 PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;

    TRACE("%p %s %s %ld %p %p %p %p\n", hCredentials, debugstr_a(pszPrincipal),
     debugstr_a(pszPackage), fCredentialUse, pAuthData, pGetKeyFn,
     pvGetKeyArgument, ptsExpiry);
    if (hCredentials)
    {
        SecurePackage *package = (SecurePackage *)hCredentials->dwUpper;
        PCredHandle cred = (PCtxtHandle)hCredentials->dwLower;

        if (package && package->provider)
        {
            if (package->provider->fnTableA.AddCredentialsA)
                ret = package->provider->fnTableA.AddCredentialsA(
                 cred, pszPrincipal, pszPackage, fCredentialUse, pAuthData,
                 pGetKeyFn, pvGetKeyArgument, ptsExpiry);
            else
                ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INVALID_HANDLE;
    }
    else
        ret = SEC_E_INVALID_HANDLE;
    return ret;
}

SECURITY_STATUS SEC_ENTRY AddCredentialsW(PCredHandle hCredentials,
 SEC_WCHAR *pszPrincipal, SEC_WCHAR *pszPackage, unsigned long fCredentialUse,
 void *pAuthData, SEC_GET_KEY_FN pGetKeyFn, void *pvGetKeyArgument,
 PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;

    TRACE("%p %s %s %ld %p %p %p %p\n", hCredentials, debugstr_w(pszPrincipal),
     debugstr_w(pszPackage), fCredentialUse, pAuthData, pGetKeyFn,
     pvGetKeyArgument, ptsExpiry);
    if (hCredentials)
    {
        SecurePackage *package = (SecurePackage *)hCredentials->dwUpper;
        PCredHandle cred = (PCtxtHandle)hCredentials->dwLower;

        if (package && package->provider)
        {
            if (package->provider->fnTableW.AddCredentialsW)
                ret = package->provider->fnTableW.AddCredentialsW(
                 cred, pszPrincipal, pszPackage, fCredentialUse, pAuthData,
                 pGetKeyFn, pvGetKeyArgument, ptsExpiry);
            else
                ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INVALID_HANDLE;
    }
    else
        ret = SEC_E_INVALID_HANDLE;
    return ret;
}

SECURITY_STATUS SEC_ENTRY QuerySecurityContextToken(PCtxtHandle phContext,
 HANDLE *phToken)
{
    SECURITY_STATUS ret;

    TRACE("%p %p\n", phContext, phToken);
    if (phContext)
    {
        SecurePackage *package = (SecurePackage *)phContext->dwUpper;
        PCtxtHandle ctxt = (PCtxtHandle)phContext->dwLower;

        if (package && package->provider)
        {
            if (package->provider->fnTableW.QuerySecurityContextToken)
                ret = package->provider->fnTableW.QuerySecurityContextToken(
                 ctxt, phToken);
            else
                ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INVALID_HANDLE;
    }
    else
        ret = SEC_E_INVALID_HANDLE;
    return ret;
}

SECURITY_STATUS SEC_ENTRY EncryptMessage(PCtxtHandle phContext, ULONG fQOP,
 PSecBufferDesc pMessage, ULONG MessageSeqNo)
{
    SECURITY_STATUS ret;

    TRACE("%p %ld %p %ld\n", phContext, fQOP, pMessage, MessageSeqNo);
    if (phContext)
    {
        SecurePackage *package = (SecurePackage *)phContext->dwUpper;
        PCtxtHandle ctxt = (PCtxtHandle)phContext->dwLower;

        if (package && package->provider)
        {
            if (package->provider->fnTableW.EncryptMessage)
                ret = package->provider->fnTableW.EncryptMessage(
                 ctxt, fQOP, pMessage, MessageSeqNo);
            else
                ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INVALID_HANDLE;
    }
    else
        ret = SEC_E_INVALID_HANDLE;
    return ret;
}

SECURITY_STATUS SEC_ENTRY DecryptMessage(PCtxtHandle phContext,
 PSecBufferDesc pMessage, ULONG MessageSeqNo, PULONG pfQOP)
{
    SECURITY_STATUS ret;

    TRACE("%p %p %ld %p\n", phContext, pMessage, MessageSeqNo, pfQOP);
    if (phContext)
    {
        SecurePackage *package = (SecurePackage *)phContext->dwUpper;
        PCtxtHandle ctxt = (PCtxtHandle)phContext->dwLower;

        if (package && package->provider)
        {
            if (package->provider->fnTableW.DecryptMessage)
                ret = package->provider->fnTableW.DecryptMessage(
                 ctxt, pMessage, MessageSeqNo, pfQOP);
            else
                ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INVALID_HANDLE;
    }
    else
        ret = SEC_E_INVALID_HANDLE;
    return ret;
}

SECURITY_STATUS SEC_ENTRY SetContextAttributesA(PCtxtHandle phContext,
 unsigned long ulAttribute, void *pBuffer, unsigned long cbBuffer)
{
    SECURITY_STATUS ret;

    TRACE("%p %ld %p %ld\n", phContext, ulAttribute, pBuffer, cbBuffer);
    if (phContext)
    {
        SecurePackage *package = (SecurePackage *)phContext->dwUpper;
        PCtxtHandle ctxt = (PCtxtHandle)phContext->dwLower;

        if (package && package->provider)
        {
            if (package->provider->fnTableA.SetContextAttributesA)
                ret = package->provider->fnTableA.SetContextAttributesA(
                 ctxt, ulAttribute, pBuffer, cbBuffer);
            else
                ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INVALID_HANDLE;
    }
    else
        ret = SEC_E_INVALID_HANDLE;
    return ret;
}

SECURITY_STATUS SEC_ENTRY SetContextAttributesW(PCtxtHandle phContext,
 unsigned long ulAttribute, void *pBuffer, unsigned long cbBuffer)
{
    SECURITY_STATUS ret;

    TRACE("%p %ld %p %ld\n", phContext, ulAttribute, pBuffer, cbBuffer);
    if (phContext)
    {
        SecurePackage *package = (SecurePackage *)phContext->dwUpper;
        PCtxtHandle ctxt = (PCtxtHandle)phContext->dwLower;

        if (package && package->provider)
        {
            if (package->provider->fnTableW.SetContextAttributesW)
                ret = package->provider->fnTableW.SetContextAttributesW(
                 ctxt, ulAttribute, pBuffer, cbBuffer);
            else
                ret = SEC_E_UNSUPPORTED_FUNCTION;
        }
        else
            ret = SEC_E_INVALID_HANDLE;
    }
    else
        ret = SEC_E_INVALID_HANDLE;
    return ret;
}
