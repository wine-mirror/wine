/* Copyright (C) 2004 Juan Lang
 *
 * This file implements thunks between wide char and multibyte functions for
 * SSPs that only provide one or the other.
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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"
#include "sspi.h"

#include "wine/debug.h"
#include "secur32_priv.h"
#include "thunks.h"

WINE_DEFAULT_DEBUG_CHANNEL(secur32);

SECURITY_STATUS SEC_ENTRY thunk_AcquireCredentialsHandleA(
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
        UNICODE_STRING principal, package;

        RtlCreateUnicodeStringFromAsciiz(&principal, pszPrincipal);
        RtlCreateUnicodeStringFromAsciiz(&package, pszPackage);
        ret = AcquireCredentialsHandleW(principal.Buffer, package.Buffer,
         fCredentialsUse, pvLogonID, pAuthData, pGetKeyFn, pvGetKeyArgument,
         phCredential, ptsExpiry);
        RtlFreeUnicodeString(&principal);
        RtlFreeUnicodeString(&package);
    }
    else
        ret = SEC_E_SECPKG_NOT_FOUND;
    return ret;
}

static char *strdupWA( const WCHAR *str )
{
    char *ret = NULL;
    if (str)
    {
        int len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
        if ((ret = RtlAllocateHeap( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    }
    return ret;
}

SECURITY_STATUS SEC_ENTRY thunk_AcquireCredentialsHandleW(
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
        char *principal, *package;

        principal = strdupWA(pszPrincipal);
        package = strdupWA(pszPackage);
        ret = AcquireCredentialsHandleA(principal, package, fCredentialsUse,
         pvLogonID, pAuthData, pGetKeyFn, pvGetKeyArgument, phCredential, ptsExpiry);
        RtlFreeHeap(GetProcessHeap(), 0, principal);
        RtlFreeHeap(GetProcessHeap(), 0, package);
    }
    else
        ret = SEC_E_SECPKG_NOT_FOUND;
    return ret;
}

/* thunking is pretty dicey for these--the output type depends on ulAttribute,
 * so we have to know about every type the caller does
 */
SECURITY_STATUS SEC_ENTRY thunk_QueryCredentialsAttributesA(
 PCredHandle phCredential, ULONG ulAttribute, void *pBuffer)
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
            {
                ret = package->provider->fnTableW.QueryCredentialsAttributesW( cred, ulAttribute, pBuffer);
                if (ret == SEC_E_OK)
                {
                    switch (ulAttribute)
                    {
                        case SECPKG_CRED_ATTR_NAMES:
                        {
                            PSecPkgCredentials_NamesW names = (PSecPkgCredentials_NamesW)pBuffer;
                            SEC_WCHAR *oldUser = names->sUserName;

                            if (oldUser)
                            {
                                names->sUserName = (WCHAR *)strdupWA(oldUser);
                                package->provider->fnTableW.FreeContextBuffer(oldUser);
                            }
                            break;
                        }
                        default:
                            WARN("attribute type %ld unknown\n", ulAttribute);
                            ret = SEC_E_INTERNAL_ERROR;
                    }
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

static WCHAR *strdupAW( const char *str )
{
    WCHAR *ret = NULL;
    if (str)
    {
        int len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if ((ret = RtlAllocateHeap( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    }
    return ret;
}

SECURITY_STATUS SEC_ENTRY thunk_QueryCredentialsAttributesW(
 PCredHandle phCredential, ULONG ulAttribute, void *pBuffer)
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
            {
                ret = package->provider->fnTableA.QueryCredentialsAttributesA(cred, ulAttribute, pBuffer);
                if (ret == SEC_E_OK)
                {
                    switch (ulAttribute)
                    {
                        case SECPKG_CRED_ATTR_NAMES:
                        {
                            PSecPkgCredentials_NamesA names = (PSecPkgCredentials_NamesA)pBuffer;
                            SEC_CHAR *oldUser = names->sUserName;

                            if (oldUser)
                            {
                                names->sUserName = (char *)strdupAW(oldUser);
                                package->provider->fnTableA.FreeContextBuffer(oldUser);
                            }
                            break;
                        }
                        default:
                            WARN("attribute type %ld unknown\n", ulAttribute);
                            ret = SEC_E_INTERNAL_ERROR;
                    }
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

SECURITY_STATUS SEC_ENTRY thunk_InitializeSecurityContextA(
 PCredHandle phCredential, PCtxtHandle phContext,
 SEC_CHAR *pszTargetName, ULONG fContextReq,
 ULONG Reserved1, ULONG TargetDataRep, PSecBufferDesc pInput,
 ULONG Reserved2, PCtxtHandle phNewContext, PSecBufferDesc pOutput,
 ULONG *pfContextAttr, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;

    TRACE("%p %p %s %ld %ld %ld %p %ld %p %p %p %p\n", phCredential, phContext,
     debugstr_a(pszTargetName), fContextReq, Reserved1, TargetDataRep, pInput,
     Reserved1, phNewContext, pOutput, pfContextAttr, ptsExpiry);
    if (phCredential)
    {
        SecurePackage *package = (SecurePackage *)phCredential->dwUpper;

        if (package && package->provider)
        {
            if (package->provider->fnTableW.InitializeSecurityContextW)
            {
                UNICODE_STRING target;

                RtlCreateUnicodeStringFromAsciiz(&target, pszTargetName);
                ret = package->provider->fnTableW.InitializeSecurityContextW(
                 phCredential, phContext, target.Buffer, fContextReq, Reserved1,
                 TargetDataRep, pInput, Reserved2, phNewContext, pOutput,
                 pfContextAttr, ptsExpiry);
                RtlFreeUnicodeString(&target);
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

SECURITY_STATUS SEC_ENTRY thunk_InitializeSecurityContextW(
 PCredHandle phCredential, PCtxtHandle phContext,
 SEC_WCHAR *pszTargetName, ULONG fContextReq,
 ULONG Reserved1, ULONG TargetDataRep, PSecBufferDesc pInput,
 ULONG Reserved2, PCtxtHandle phNewContext, PSecBufferDesc pOutput,
 ULONG *pfContextAttr, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;

    TRACE("%p %p %s %ld %ld %ld %p %ld %p %p %p %p\n", phCredential, phContext,
     debugstr_w(pszTargetName), fContextReq, Reserved1, TargetDataRep, pInput,
     Reserved1, phNewContext, pOutput, pfContextAttr, ptsExpiry);
    if (phCredential)
    {
        SecurePackage *package = (SecurePackage *)phCredential->dwUpper;

        if (package && package->provider)
        {
            if (package->provider->fnTableA.InitializeSecurityContextA)
            {
                char *target = strdupWA(pszTargetName);

                ret = package->provider->fnTableA.InitializeSecurityContextA(
                 phCredential, phContext, target, fContextReq, Reserved1,
                 TargetDataRep, pInput, Reserved2, phNewContext, pOutput,
                 pfContextAttr, ptsExpiry);
                RtlFreeHeap(GetProcessHeap(), 0, target);
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

SECURITY_STATUS SEC_ENTRY thunk_AddCredentialsA(PCredHandle hCredentials,
 SEC_CHAR *pszPrincipal, SEC_CHAR *pszPackage, ULONG fCredentialUse,
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
            if (package->provider->fnTableW.AddCredentialsW)
            {
                UNICODE_STRING szPrincipal, szPackage;

                RtlCreateUnicodeStringFromAsciiz(&szPrincipal, pszPrincipal);
                RtlCreateUnicodeStringFromAsciiz(&szPackage, pszPackage);
                ret = package->provider->fnTableW.AddCredentialsW(
                 cred, szPrincipal.Buffer, szPackage.Buffer, fCredentialUse,
                 pAuthData, pGetKeyFn, pvGetKeyArgument, ptsExpiry);
                RtlFreeUnicodeString(&szPrincipal);
                RtlFreeUnicodeString(&szPackage);
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

SECURITY_STATUS SEC_ENTRY thunk_AddCredentialsW(PCredHandle hCredentials,
 SEC_WCHAR *pszPrincipal, SEC_WCHAR *pszPackage, ULONG fCredentialUse,
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
            if (package->provider->fnTableA.AddCredentialsA)
            {
                char *szPrincipal = strdupWA(pszPrincipal);
                char *szPackage = strdupWA(pszPackage);

                ret = package->provider->fnTableA.AddCredentialsA(
                 cred, szPrincipal, szPackage, fCredentialUse, pAuthData,
                 pGetKeyFn, pvGetKeyArgument, ptsExpiry);
                RtlFreeHeap(GetProcessHeap(), 0, szPrincipal);
                RtlFreeHeap(GetProcessHeap(), 0, szPackage);
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

static PSecPkgInfoA _copyPackageInfoFlatWToA(const SecPkgInfoW *infoW)
{
    PSecPkgInfoA ret;

    if (infoW)
    {
        size_t bytesNeeded = sizeof(SecPkgInfoA);
        int nameLen = 0, commentLen = 0;

        if (infoW->Name)
        {
            nameLen = WideCharToMultiByte(CP_ACP, 0, infoW->Name, -1,
             NULL, 0, NULL, NULL);
            bytesNeeded += nameLen;
        }
        if (infoW->Comment)
        {
            commentLen = WideCharToMultiByte(CP_ACP, 0, infoW->Comment, -1,
             NULL, 0, NULL, NULL);
            bytesNeeded += commentLen;
        }
        if ((ret = RtlAllocateHeap(GetProcessHeap(), 0, bytesNeeded)))
        {
            char *nextString = (char *)ret + sizeof(SecPkgInfoA);

            memcpy(ret, infoW, sizeof(SecPkgInfoA));
            if (infoW->Name)
            {
                ret->Name = nextString;
                WideCharToMultiByte(CP_ACP, 0, infoW->Name, -1, nextString, nameLen, NULL, NULL);
                nextString += nameLen;
            }
            else
                ret->Name = NULL;
            if (infoW->Comment)
            {
                ret->Comment = nextString;
                WideCharToMultiByte(CP_ACP, 0, infoW->Comment, -1, nextString, nameLen, NULL, NULL);
            }
            else
                ret->Comment = NULL;
        }
    }
    else
        ret = NULL;
    return ret;
}

static SECURITY_STATUS thunk_ContextAttributesWToA(SecurePackage *package,
 ULONG ulAttribute, void *pBuffer)
{
    SECURITY_STATUS ret = SEC_E_OK;

    if (package && pBuffer)
    {
        switch (ulAttribute)
        {
            case SECPKG_ATTR_NAMES:
            {
                PSecPkgContext_NamesW names = (PSecPkgContext_NamesW)pBuffer;
                SEC_WCHAR *oldUser = names->sUserName;

                if (oldUser)
                {
                    names->sUserName = (WCHAR *)strdupWA(oldUser);
                    package->provider->fnTableW.FreeContextBuffer(oldUser);
                }
                break;
            }
            case SECPKG_ATTR_AUTHORITY:
            {
                PSecPkgContext_AuthorityW names = (PSecPkgContext_AuthorityW)pBuffer;
                SEC_WCHAR *oldAuth = names->sAuthorityName;

                if (oldAuth)
                {
                    names->sAuthorityName = (WCHAR *)strdupWA(oldAuth);
                    package->provider->fnTableW.FreeContextBuffer(oldAuth);
                }
                break;
            }
            case SECPKG_ATTR_KEY_INFO:
            {
                PSecPkgContext_KeyInfoW info = (PSecPkgContext_KeyInfoW)pBuffer;
                SEC_WCHAR *oldSigAlgName = info->sSignatureAlgorithmName;
                SEC_WCHAR *oldEncAlgName = info->sEncryptAlgorithmName;

                if (oldSigAlgName)
                {
                    info->sSignatureAlgorithmName = (WCHAR *)strdupWA(oldSigAlgName);
                    package->provider->fnTableW.FreeContextBuffer(oldSigAlgName);
                }
                if (oldEncAlgName)
                {
                    info->sEncryptAlgorithmName = (WCHAR *)strdupWA(oldEncAlgName);
                    package->provider->fnTableW.FreeContextBuffer(oldEncAlgName);
                }
                break;
            }
            case SECPKG_ATTR_PACKAGE_INFO:
            {
                PSecPkgContext_PackageInfoW info = (PSecPkgContext_PackageInfoW)pBuffer;
                PSecPkgInfoW oldPkgInfo = info->PackageInfo;

                if (oldPkgInfo)
                {
                    info->PackageInfo = (PSecPkgInfoW)
                     _copyPackageInfoFlatWToA(oldPkgInfo);
                    package->provider->fnTableW.FreeContextBuffer(oldPkgInfo);
                }
                break;
            }
            case SECPKG_ATTR_NEGOTIATION_INFO:
            {
                PSecPkgContext_NegotiationInfoW info = (PSecPkgContext_NegotiationInfoW)pBuffer;
                PSecPkgInfoW oldPkgInfo = info->PackageInfo;

                if (oldPkgInfo)
                {
                    info->PackageInfo = (PSecPkgInfoW)
                     _copyPackageInfoFlatWToA(oldPkgInfo);
                    package->provider->fnTableW.FreeContextBuffer(oldPkgInfo);
                }
                break;
            }
            case SECPKG_ATTR_NATIVE_NAMES:
            {
                PSecPkgContext_NativeNamesW names = (PSecPkgContext_NativeNamesW)pBuffer;
                WCHAR *oldClient = names->sClientName;
                WCHAR *oldServer = names->sServerName;

                if (oldClient)
                {
                    names->sClientName = (WCHAR *)strdupWA(oldClient);
                    package->provider->fnTableW.FreeContextBuffer(oldClient);
                }
                if (oldServer)
                {
                    names->sServerName = (WCHAR *)strdupWA(oldServer);
                    package->provider->fnTableW.FreeContextBuffer(oldServer);
                }
                break;
            }
            case SECPKG_ATTR_CREDENTIAL_NAME:
            {
                PSecPkgContext_CredentialNameW name = (PSecPkgContext_CredentialNameW)pBuffer;
                WCHAR *oldCred = name->sCredentialName;

                if (oldCred)
                {
                    name->sCredentialName = (WCHAR *)strdupWA(oldCred);
                    package->provider->fnTableW.FreeContextBuffer(oldCred);
                }
                break;
            }
            /* no thunking needed: */
            case SECPKG_ATTR_ACCESS_TOKEN:
            case SECPKG_ATTR_DCE_INFO:
            case SECPKG_ATTR_FLAGS:
            case SECPKG_ATTR_LIFESPAN:
            case SECPKG_ATTR_PASSWORD_EXPIRY:
            case SECPKG_ATTR_SESSION_KEY:
            case SECPKG_ATTR_SIZES:
            case SECPKG_ATTR_STREAM_SIZES:
            case SECPKG_ATTR_TARGET_INFORMATION:
                break;
            default:
                WARN("attribute type %ld unknown\n", ulAttribute);
                ret = SEC_E_INTERNAL_ERROR;
        }
    }
    else
        ret = SEC_E_INVALID_TOKEN;
    return ret;
}

SECURITY_STATUS SEC_ENTRY thunk_QueryContextAttributesA(PCtxtHandle phContext,
 ULONG ulAttribute, void *pBuffer)
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
            {
                ret = package->provider->fnTableW.QueryContextAttributesW( ctxt, ulAttribute, pBuffer);
                if (ret == SEC_E_OK)
                    ret = thunk_ContextAttributesWToA(package, ulAttribute, pBuffer);
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

static PSecPkgInfoW _copyPackageInfoFlatAToW(const SecPkgInfoA *infoA)
{
    PSecPkgInfoW ret;

    if (infoA)
    {
        size_t bytesNeeded = sizeof(SecPkgInfoW);
        int nameLen = 0, commentLen = 0;

        if (infoA->Name)
        {
            nameLen = MultiByteToWideChar(CP_ACP, 0, infoA->Name, -1, NULL, 0);
            bytesNeeded += nameLen * sizeof(WCHAR);
        }
        if (infoA->Comment)
        {
            commentLen = MultiByteToWideChar(CP_ACP, 0, infoA->Comment, -1, NULL, 0);
            bytesNeeded += commentLen * sizeof(WCHAR);
        }
        if ((ret = RtlAllocateHeap(GetProcessHeap(), 0, bytesNeeded)))
        {
            WCHAR *nextString = (WCHAR *)(char *)ret + sizeof(SecPkgInfoW);

            memcpy(ret, infoA, sizeof(SecPkgInfoA));
            if (infoA->Name)
            {
                ret->Name = nextString;
                MultiByteToWideChar(CP_ACP, 0, infoA->Name, -1, nextString, nameLen);
                nextString += nameLen;
            }
            else
                ret->Name = NULL;
            if (infoA->Comment)
            {
                ret->Comment = nextString;
                MultiByteToWideChar(CP_ACP, 0, infoA->Comment, -1, nextString, commentLen);
            }
            else
                ret->Comment = NULL;
        }
    }
    else
        ret = NULL;
    return ret;
}

static SECURITY_STATUS thunk_ContextAttributesAToW(SecurePackage *package,
 ULONG ulAttribute, void *pBuffer)
{
    SECURITY_STATUS ret = SEC_E_OK;

    if (package && pBuffer)
    {
        switch (ulAttribute)
        {
            case SECPKG_ATTR_NAMES:
            {
                PSecPkgContext_NamesA names = (PSecPkgContext_NamesA)pBuffer;
                SEC_CHAR *oldUser = names->sUserName;

                if (oldUser)
                {
                    names->sUserName = (char *)strdupAW(oldUser);
                    package->provider->fnTableW.FreeContextBuffer(oldUser);
                }
                break;
            }
            case SECPKG_ATTR_AUTHORITY:
            {
                PSecPkgContext_AuthorityA names = (PSecPkgContext_AuthorityA)pBuffer;
                SEC_CHAR *oldAuth = names->sAuthorityName;

                if (oldAuth)
                {
                    names->sAuthorityName = (char *)strdupAW(oldAuth);
                    package->provider->fnTableW.FreeContextBuffer(oldAuth);
                }
                break;
            }
            case SECPKG_ATTR_KEY_INFO:
            {
                PSecPkgContext_KeyInfoA info = (PSecPkgContext_KeyInfoA)pBuffer;
                SEC_CHAR *oldSigAlgName = info->sSignatureAlgorithmName;
                SEC_CHAR *oldEncAlgName = info->sEncryptAlgorithmName;

                if (oldSigAlgName)
                {
                    info->sSignatureAlgorithmName = (char *)strdupAW(oldSigAlgName);
                    package->provider->fnTableW.FreeContextBuffer(oldSigAlgName);
                }
                if (oldEncAlgName)
                {
                    info->sEncryptAlgorithmName = (char *)strdupAW(oldEncAlgName);
                    package->provider->fnTableW.FreeContextBuffer(oldEncAlgName);
                }
                break;
            }
            case SECPKG_ATTR_PACKAGE_INFO:
            {
                PSecPkgContext_PackageInfoA info = (PSecPkgContext_PackageInfoA)pBuffer;
                PSecPkgInfoA oldPkgInfo = info->PackageInfo;

                if (oldPkgInfo)
                {
                    info->PackageInfo = (PSecPkgInfoA)
                     _copyPackageInfoFlatAToW(oldPkgInfo);
                    package->provider->fnTableW.FreeContextBuffer(oldPkgInfo);
                }
                break;
            }
            case SECPKG_ATTR_NEGOTIATION_INFO:
            {
                PSecPkgContext_NegotiationInfoA info = (PSecPkgContext_NegotiationInfoA)pBuffer;
                PSecPkgInfoA oldPkgInfo = info->PackageInfo;

                if (oldPkgInfo)
                {
                    info->PackageInfo = (PSecPkgInfoA)
                     _copyPackageInfoFlatAToW(oldPkgInfo);
                    package->provider->fnTableW.FreeContextBuffer(oldPkgInfo);
                }
                break;
            }
            case SECPKG_ATTR_NATIVE_NAMES:
            {
                PSecPkgContext_NativeNamesA names = (PSecPkgContext_NativeNamesA)pBuffer;
                char *oldClient = names->sClientName;
                char *oldServer = names->sServerName;

                if (oldClient)
                {
                    names->sClientName = (char *)strdupAW(oldClient);
                    package->provider->fnTableW.FreeContextBuffer(oldClient);
                }
                if (oldServer)
                {
                    names->sServerName = (char *)strdupAW(oldServer);
                    package->provider->fnTableW.FreeContextBuffer(oldServer);
                }
                break;
            }
            case SECPKG_ATTR_CREDENTIAL_NAME:
            {
                PSecPkgContext_CredentialNameA name = (PSecPkgContext_CredentialNameA)pBuffer;
                char *oldCred = name->sCredentialName;

                if (oldCred)
                {
                    name->sCredentialName = (char *)strdupAW(oldCred);
                    package->provider->fnTableW.FreeContextBuffer(oldCred);
                }
                break;
            }
            /* no thunking needed: */
            case SECPKG_ATTR_ACCESS_TOKEN:
            case SECPKG_ATTR_DCE_INFO:
            case SECPKG_ATTR_FLAGS:
            case SECPKG_ATTR_LIFESPAN:
            case SECPKG_ATTR_PASSWORD_EXPIRY:
            case SECPKG_ATTR_SESSION_KEY:
            case SECPKG_ATTR_SIZES:
            case SECPKG_ATTR_STREAM_SIZES:
            case SECPKG_ATTR_TARGET_INFORMATION:
                break;
            default:
                WARN("attribute type %ld unknown\n", ulAttribute);
                ret = SEC_E_INTERNAL_ERROR;
        }
    }
    else
        ret = SEC_E_INVALID_TOKEN;
    return ret;
}

SECURITY_STATUS SEC_ENTRY thunk_QueryContextAttributesW(PCtxtHandle phContext,
 ULONG ulAttribute, void *pBuffer)
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
            {
                ret = package->provider->fnTableA.QueryContextAttributesA(ctxt, ulAttribute, pBuffer);
                if (ret == SEC_E_OK)
                    ret = thunk_ContextAttributesAToW(package, ulAttribute, pBuffer);
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

SECURITY_STATUS SEC_ENTRY thunk_SetContextAttributesA(PCtxtHandle phContext,
 ULONG ulAttribute, void *pBuffer, ULONG cbBuffer)
{
    SECURITY_STATUS ret;

    TRACE("%p %ld %p %ld\n", phContext, ulAttribute, pBuffer, cbBuffer);
    if (phContext)
    {
        SecurePackage *package = (SecurePackage *)phContext->dwUpper;
        PCtxtHandle ctxt = (PCtxtHandle)phContext->dwLower;

        if (package && package->provider && pBuffer && cbBuffer)
        {
            if (package->provider->fnTableW.SetContextAttributesW)
            {
                /* TODO: gotta validate size too! */
                ret = thunk_ContextAttributesWToA(package, ulAttribute, pBuffer);
                if (ret == SEC_E_OK)
                    ret = package->provider->fnTableW.SetContextAttributesW(ctxt, ulAttribute, pBuffer, cbBuffer);
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

SECURITY_STATUS SEC_ENTRY thunk_SetContextAttributesW(PCtxtHandle phContext,
 ULONG ulAttribute, void *pBuffer, ULONG cbBuffer)
{
    SECURITY_STATUS ret;

    TRACE("%p %ld %p %ld\n", phContext, ulAttribute, pBuffer, cbBuffer);
    if (phContext)
    {
        SecurePackage *package = (SecurePackage *)phContext->dwUpper;
        PCtxtHandle ctxt = (PCtxtHandle)phContext->dwLower;

        if (package && package->provider && pBuffer && cbBuffer)
        {
            if (package->provider->fnTableA.SetContextAttributesA)
            {
                /* TODO: gotta validate size too! */
                ret = thunk_ContextAttributesAToW(package, ulAttribute, pBuffer);
                if (ret == SEC_E_OK)
                    ret = package->provider->fnTableA.SetContextAttributesA(ctxt, ulAttribute, pBuffer, cbBuffer);
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

SECURITY_STATUS SEC_ENTRY thunk_ImportSecurityContextA(
 SEC_CHAR *pszPackage, PSecBuffer pPackedContext, void *Token,
 PCtxtHandle phContext)
{
    SECURITY_STATUS ret;
    UNICODE_STRING package;

    TRACE("%s %p %p %p\n", debugstr_a(pszPackage), pPackedContext, Token, phContext);
    RtlCreateUnicodeStringFromAsciiz(&package, pszPackage);
    ret = ImportSecurityContextW(package.Buffer, pPackedContext, Token, phContext);
    RtlFreeUnicodeString(&package);
    return ret;
}

SECURITY_STATUS SEC_ENTRY thunk_ImportSecurityContextW(
 SEC_WCHAR *pszPackage, PSecBuffer pPackedContext, void *Token,
 PCtxtHandle phContext)
{
    SECURITY_STATUS ret;
    char *package = strdupWA(pszPackage);

    TRACE("%s %p %p %p\n", debugstr_w(pszPackage), pPackedContext, Token, phContext);
    ret = ImportSecurityContextA(package, pPackedContext, Token, phContext);
    RtlFreeHeap(GetProcessHeap(), 0, package);
    return ret;
}
