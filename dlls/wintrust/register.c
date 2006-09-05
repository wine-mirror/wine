/*
 * Register related wintrust functions
 *
 * Copyright 2006 Paul Vriens
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
#include "winerror.h"
#include "winuser.h"
#include "winreg.h"
#include "winnls.h"

#include "guiddef.h"
#include "wintrust.h"
#include "softpub.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wintrust);

static CRYPT_TRUST_REG_ENTRY SoftpubInitialization;
static CRYPT_TRUST_REG_ENTRY SoftpubMessage;
static CRYPT_TRUST_REG_ENTRY SoftpubSignature;
static CRYPT_TRUST_REG_ENTRY SoftpubCertficate;
static CRYPT_TRUST_REG_ENTRY SoftpubCertCheck;
static CRYPT_TRUST_REG_ENTRY SoftpubFinalPolicy;
static CRYPT_TRUST_REG_ENTRY SoftpubCleanup;

static CRYPT_TRUST_REG_ENTRY SoftpubDefCertInit;

static CRYPT_TRUST_REG_ENTRY SoftpubDumpStructure;

static CRYPT_TRUST_REG_ENTRY HTTPSCertificateTrust;
static CRYPT_TRUST_REG_ENTRY HTTPSFinalProv;

static CRYPT_TRUST_REG_ENTRY OfficeInitializePolicy;
static CRYPT_TRUST_REG_ENTRY OfficeCleanupPolicy;

static const WCHAR Trust[]            = {'S','o','f','t','w','a','r','e','\\',
                                         'M','i','c','r','o','s','o','f','t','\\',
                                         'C','r','y','p','t','o','g','r','a','p','h','y','\\',
                                         'P','r','o','v','i','d','e','r','s','\\',
                                         'T','r','u','s','t','\\', 0 };

static const WCHAR Initialization[]   = {'I','n','i','t','i','a','l','i','z','a','t','i','o','n','\\', 0};
static const WCHAR Message[]          = {'M','e','s','s','a','g','e','\\', 0};
static const WCHAR Signature[]        = {'S','i','g','n','a','t','u','r','e','\\', 0};
static const WCHAR Certificate[]      = {'C','e','r','t','i','f','i','c','a','t','e','\\', 0};
static const WCHAR CertCheck[]        = {'C','e','r','t','C','h','e','c','k','\\', 0};
static const WCHAR FinalPolicy[]      = {'F','i','n','a','l','P','o','l','i','c','y','\\', 0};
static const WCHAR DiagnosticPolicy[] = {'D','i','a','g','n','o','s','t','i','c','P','o','l','i','c','y','\\', 0};
static const WCHAR Cleanup[]          = {'C','l','e','a','n','u','p','\\', 0};

static const WCHAR DefaultId[]        = {'D','e','f','a','u','l','t','I','d', 0};
static const WCHAR Dll[]              = {'$','D','L','L', 0};

/***********************************************************************
 *              WINTRUST_InitRegStructs
 *
 * Helper function to allocate and initialize the members of the
 * CRYPT_TRUST_REG_ENTRY structs.
 */
static void WINTRUST_InitRegStructs(void)
{
#define WINTRUST_INITREGENTRY( action, dllname, functionname ) \
    action.cbStruct = sizeof(CRYPT_TRUST_REG_ENTRY); \
    action.pwszDLLName = HeapAlloc(GetProcessHeap(), 0, sizeof(dllname)); \
    lstrcpyW(action.pwszDLLName, dllname); \
    action.pwszFunctionName = HeapAlloc(GetProcessHeap(), 0, sizeof(functionname)); \
    lstrcpyW(action.pwszFunctionName, functionname);

    WINTRUST_INITREGENTRY(SoftpubInitialization, SP_POLICY_PROVIDER_DLL_NAME, SP_INIT_FUNCTION)
    WINTRUST_INITREGENTRY(SoftpubMessage, SP_POLICY_PROVIDER_DLL_NAME, SP_OBJTRUST_FUNCTION)
    WINTRUST_INITREGENTRY(SoftpubSignature, SP_POLICY_PROVIDER_DLL_NAME, SP_SIGTRUST_FUNCTION)
    WINTRUST_INITREGENTRY(SoftpubCertficate, SP_POLICY_PROVIDER_DLL_NAME, WT_PROVIDER_CERTTRUST_FUNCTION)
    WINTRUST_INITREGENTRY(SoftpubCertCheck, SP_POLICY_PROVIDER_DLL_NAME, SP_CHKCERT_FUNCTION)
    WINTRUST_INITREGENTRY(SoftpubFinalPolicy, SP_POLICY_PROVIDER_DLL_NAME, SP_FINALPOLICY_FUNCTION)
    WINTRUST_INITREGENTRY(SoftpubCleanup, SP_POLICY_PROVIDER_DLL_NAME, SP_CLEANUPPOLICY_FUNCTION)
    WINTRUST_INITREGENTRY(SoftpubDefCertInit, SP_POLICY_PROVIDER_DLL_NAME, SP_GENERIC_CERT_INIT_FUNCTION)
    WINTRUST_INITREGENTRY(SoftpubDumpStructure, SP_POLICY_PROVIDER_DLL_NAME, SP_TESTDUMPPOLICY_FUNCTION_TEST)
    WINTRUST_INITREGENTRY(HTTPSCertificateTrust, SP_POLICY_PROVIDER_DLL_NAME, HTTPS_CERTTRUST_FUNCTION)
    WINTRUST_INITREGENTRY(HTTPSFinalProv, SP_POLICY_PROVIDER_DLL_NAME, HTTPS_FINALPOLICY_FUNCTION)
    WINTRUST_INITREGENTRY(OfficeInitializePolicy, OFFICE_POLICY_PROVIDER_DLL_NAME, OFFICE_INITPROV_FUNCTION)
    WINTRUST_INITREGENTRY(OfficeCleanupPolicy, OFFICE_POLICY_PROVIDER_DLL_NAME, OFFICE_CLEANUPPOLICY_FUNCTION)

#undef WINTRUST_INITREGENTRY
}

/***********************************************************************
 *              WINTRUST_FreeRegStructs
 *
 * Helper function to free 2 members of the CRYPT_TRUST_REG_ENTRY
 * structs.
 */
static void WINTRUST_FreeRegStructs(void)
{
#define WINTRUST_FREEREGENTRY( action ) \
    HeapFree(GetProcessHeap(), 0, action.pwszDLLName); \
    HeapFree(GetProcessHeap(), 0, action.pwszFunctionName);

    WINTRUST_FREEREGENTRY(SoftpubInitialization);
    WINTRUST_FREEREGENTRY(SoftpubMessage);
    WINTRUST_FREEREGENTRY(SoftpubSignature);
    WINTRUST_FREEREGENTRY(SoftpubCertficate);
    WINTRUST_FREEREGENTRY(SoftpubCertCheck);
    WINTRUST_FREEREGENTRY(SoftpubFinalPolicy);
    WINTRUST_FREEREGENTRY(SoftpubCleanup);
    WINTRUST_FREEREGENTRY(SoftpubDefCertInit);
    WINTRUST_FREEREGENTRY(SoftpubDumpStructure);
    WINTRUST_FREEREGENTRY(HTTPSCertificateTrust);
    WINTRUST_FREEREGENTRY(HTTPSFinalProv);
    WINTRUST_FREEREGENTRY(OfficeInitializePolicy);
    WINTRUST_FREEREGENTRY(OfficeCleanupPolicy);

#undef WINTRUST_FREEREGENTRY
}

/***********************************************************************
 *              WINTRUST_guid2wstr
 *
 * Create a wide-string from a GUID
 *
 */
static void WINTRUST_Guid2Wstr(const GUID* pgActionID, WCHAR* GuidString)
{ 
    static const WCHAR wszFormat[] = {'{','%','0','8','l','X','-','%','0','4','X','-','%','0','4','X','-',
                                      '%','0','2','X','%','0','2','X','-','%','0','2','X','%','0','2','X','%','0','2','X','%','0','2',
                                      'X','%','0','2','X','%','0','2','X','}', 0};

    wsprintfW(GuidString, wszFormat, pgActionID->Data1, pgActionID->Data2, pgActionID->Data3,
        pgActionID->Data4[0], pgActionID->Data4[1], pgActionID->Data4[2], pgActionID->Data4[3],
        pgActionID->Data4[4], pgActionID->Data4[5], pgActionID->Data4[6], pgActionID->Data4[7]);
}

/***********************************************************************
 *              WINTRUST_WriteProviderToReg
 *
 * Helper function for WintrustAddActionID
 *
 */
static LONG WINTRUST_WriteProviderToReg(WCHAR* GuidString,
                                        const WCHAR* FunctionType,
                                        CRYPT_TRUST_REG_ENTRY RegEntry)
{
    static const WCHAR Function[] = {'$','F','u','n','c','t','i','o','n', 0};
    WCHAR ProvKey[MAX_PATH];
    HKEY Key;
    LONG Res = ERROR_SUCCESS;

    /* Create the needed key string */
    ProvKey[0]='\0';
    lstrcatW(ProvKey, Trust);
    lstrcatW(ProvKey, FunctionType);
    lstrcatW(ProvKey, GuidString);

    if (!RegEntry.pwszDLLName || !RegEntry.pwszFunctionName)
        return ERROR_INVALID_PARAMETER;

    Res = RegCreateKeyExW(HKEY_LOCAL_MACHINE, ProvKey, 0, NULL, 0, KEY_WRITE, NULL, &Key, NULL);
    if (Res != ERROR_SUCCESS) goto error_close_key;

    /* Create the $DLL entry */
    Res = RegSetValueExW(Key, Dll, 0, REG_SZ, (BYTE*)RegEntry.pwszDLLName,
        (lstrlenW(RegEntry.pwszDLLName) + 1)*sizeof(WCHAR));
    if (Res != ERROR_SUCCESS) goto error_close_key;

    /* Create the $Function entry */
    Res = RegSetValueExW(Key, Function, 0, REG_SZ, (BYTE*)RegEntry.pwszFunctionName,
        (lstrlenW(RegEntry.pwszFunctionName) + 1)*sizeof(WCHAR));

error_close_key:
    RegCloseKey(Key);

    return Res;
}

/***********************************************************************
 *		WintrustAddActionID (WINTRUST.@)
 *
 * Add the definitions of the actions a Trust provider can perform to
 * the registry.
 *
 * PARAMS
 *   pgActionID [I] Pointer to a GUID for the Trust provider.
 *   fdwFlags   [I] Flag to indicate whether registry errors are passed on.
 *   psProvInfo [I] Pointer to a structure with information about DLL
 *                  name and functions.
 *
 * RETURNS
 *   Success: TRUE.
 *   Failure: FALSE. (Use GetLastError() for more information)
 *
 * NOTES
 *   Adding definitions is basically only adding relevant information
 *   to the registry. No verification takes place whether a DLL or it's
 *   entrypoints exist.
 *   Information in the registry will always be overwritten.
 *
 */
BOOL WINAPI WintrustAddActionID( GUID* pgActionID, DWORD fdwFlags,
                                 CRYPT_REGISTER_ACTIONID* psProvInfo)
{
    WCHAR GuidString[39];
    LONG Res;
    LONG WriteActionError = ERROR_SUCCESS;

    TRACE("%p %lx %p\n", debugstr_guid(pgActionID), fdwFlags, psProvInfo);

    /* Some sanity checks.
     * We use the W2K3 last error as it makes more sense (W2K leaves the last error
     * as is).
     */
    if (!pgActionID ||
        !psProvInfo ||
        (psProvInfo->cbStruct != sizeof(CRYPT_REGISTER_ACTIONID)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Create this string only once, instead of in the helper function */
    WINTRUST_Guid2Wstr( pgActionID, GuidString);

    /* Write the information to the registry */
    Res = WINTRUST_WriteProviderToReg(GuidString, Initialization  , psProvInfo->sInitProvider);
    if (Res != ERROR_SUCCESS) WriteActionError = Res;
    Res = WINTRUST_WriteProviderToReg(GuidString, Message         , psProvInfo->sObjectProvider);
    if (Res != ERROR_SUCCESS) WriteActionError = Res;
    Res = WINTRUST_WriteProviderToReg(GuidString, Signature       , psProvInfo->sSignatureProvider);
    if (Res != ERROR_SUCCESS) WriteActionError = Res;
    Res = WINTRUST_WriteProviderToReg(GuidString, Certificate     , psProvInfo->sCertificateProvider);
    if (Res != ERROR_SUCCESS) WriteActionError = Res;
    Res = WINTRUST_WriteProviderToReg(GuidString, CertCheck       , psProvInfo->sCertificatePolicyProvider);
    if (Res != ERROR_SUCCESS) WriteActionError = Res;
    Res = WINTRUST_WriteProviderToReg(GuidString, FinalPolicy     , psProvInfo->sFinalPolicyProvider);
    if (Res != ERROR_SUCCESS) WriteActionError = Res;
    Res = WINTRUST_WriteProviderToReg(GuidString, DiagnosticPolicy, psProvInfo->sTestPolicyProvider);
    if (Res != ERROR_SUCCESS) WriteActionError = Res;
    Res = WINTRUST_WriteProviderToReg(GuidString, Cleanup         , psProvInfo->sCleanupProvider);
    if (Res != ERROR_SUCCESS) WriteActionError = Res;

    /* Testing (by restricting access to the registry for some keys) shows that the last failing function
     * will be used for last error.
     * If the flag WT_ADD_ACTION_ID_RET_RESULT_FLAG is set and there are errors when adding the action
     * we have to return FALSE. Errors includes both invalid entries as well as registry errors.
     * Testing also showed that one error doesn't stop the registry writes. Every action will be dealt with.
     */

    if (WriteActionError != ERROR_SUCCESS)
    {
        SetLastError(WriteActionError);

        if (fdwFlags == WT_ADD_ACTION_ID_RET_RESULT_FLAG)
            return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *              WINTRUST_RemoveProviderFromReg
 *
 * Helper function for WintrustRemoveActionID
 *
 */
static void WINTRUST_RemoveProviderFromReg(WCHAR* GuidString,
                                           const WCHAR* FunctionType)
{
    WCHAR ProvKey[MAX_PATH];

    /* Create the needed key string */
    ProvKey[0]='\0';
    lstrcatW(ProvKey, Trust);
    lstrcatW(ProvKey, FunctionType);
    lstrcatW(ProvKey, GuidString);

    /* We don't care about success or failure */
    RegDeleteKeyW(HKEY_LOCAL_MACHINE, ProvKey);
}

/***********************************************************************
 *              WintrustRemoveActionID (WINTRUST.@)
 *
 * Remove the definitions of the actions a Trust provider can perform
 * from the registry.
 *
 * PARAMS
 *   pgActionID [I] Pointer to a GUID for the Trust provider.
 *
 * RETURNS
 *   Success: TRUE. (Use GetLastError() for more information)
 *   Failure: FALSE. (Use GetLastError() for more information)
 *
 * NOTES
 *   Testing shows that WintrustRemoveActionID always returns TRUE and
 *   that a possible error should be retrieved via GetLastError().
 *   There are no checks if the definitions are in the registry.
 */
BOOL WINAPI WintrustRemoveActionID( GUID* pgActionID )
{
    WCHAR GuidString[39];

    TRACE("(%s)\n", debugstr_guid(pgActionID));
 
    if (!pgActionID)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return TRUE;
    }

    /* Create this string only once, instead of in the helper function */
    WINTRUST_Guid2Wstr( pgActionID, GuidString);

    /* We don't care about success or failure */
    WINTRUST_RemoveProviderFromReg(GuidString, Initialization);
    WINTRUST_RemoveProviderFromReg(GuidString, Message);
    WINTRUST_RemoveProviderFromReg(GuidString, Signature);
    WINTRUST_RemoveProviderFromReg(GuidString, Certificate);
    WINTRUST_RemoveProviderFromReg(GuidString, CertCheck);
    WINTRUST_RemoveProviderFromReg(GuidString, FinalPolicy);
    WINTRUST_RemoveProviderFromReg(GuidString, DiagnosticPolicy);
    WINTRUST_RemoveProviderFromReg(GuidString, Cleanup);

    return TRUE;
}

/***********************************************************************
 *              WINTRUST_WriteSingleUsageEntry
 *
 * Write a single value and its data to:
 *
 * HKLM\Software\Microsoft\Cryptography\Trust\Usages\<OID>
 */
static LONG WINTRUST_WriteSingleUsageEntry(LPCSTR OID,
                                           const WCHAR* Value,
                                           WCHAR* Data)
{
    static const WCHAR Usages[] = {'U','s','a','g','e','s','\\', 0};
    WCHAR* UsageKey;
    HKEY Key;
    LONG Res = ERROR_SUCCESS;
    WCHAR* OIDW;
    DWORD Len;

    /* Turn OID into a wide-character string */
    Len = MultiByteToWideChar( CP_ACP, 0, OID, -1, NULL, 0 );
    OIDW = HeapAlloc( GetProcessHeap(), 0, Len * sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, OID, -1, OIDW, Len );

    /* Allocate the needed space for UsageKey */
    UsageKey = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(Trust) + lstrlenW(Usages) + Len) * sizeof(WCHAR));
    /* Create the key string */
    lstrcpyW(UsageKey, Trust);
    lstrcatW(UsageKey, Usages);
    lstrcatW(UsageKey, OIDW);

    Res = RegCreateKeyExW(HKEY_LOCAL_MACHINE, UsageKey, 0, NULL, 0, KEY_WRITE, NULL, &Key, NULL);
    if (Res == ERROR_SUCCESS)
    {
        /* Create the Value entry */
        Res = RegSetValueExW(Key, Value, 0, REG_SZ, (BYTE*)Data,
                             (lstrlenW(Data) + 1)*sizeof(WCHAR));
    }
    RegCloseKey(Key);

    HeapFree(GetProcessHeap(), 0, OIDW);
    HeapFree(GetProcessHeap(), 0, UsageKey);

    return Res;
}

/***************************************************************************
 *              WINTRUST_RegisterGenVerifyV2
 *
 * Register WINTRUST_ACTION_GENERIC_VERIFY_V2 actions and usages.
 *
 * NOTES
 *   WINTRUST_ACTION_GENERIC_VERIFY_V2 ({00AAC56B-CD44-11D0-8CC2-00C04FC295EE}
 *   is defined in softpub.h
 *   We don't care about failures (see comments in DllRegisterServer)
 */
static void WINTRUST_RegisterGenVerifyV2(void)
{
    static const GUID ProvGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    WCHAR GuidString[39];

    WINTRUST_Guid2Wstr(&ProvGUID , GuidString);

    TRACE("Going to register WINTRUST_ACTION_GENERIC_VERIFY_V2 : %s\n", wine_dbgstr_w(GuidString));

    /* HKLM\Software\Microsoft\Cryptography\Trust\Usages\1.3.6.1.5.5.7.3.3 */
    WINTRUST_WriteSingleUsageEntry(szOID_PKIX_KP_CODE_SIGNING, DefaultId, GuidString);

    /* HKLM\Software\Microsoft\Cryptography\Trust\Provider\*\{00AAC56B-CD44-11D0-8CC2-00C04FC295EE} */
    WINTRUST_WriteProviderToReg(GuidString, Initialization, SoftpubInitialization);
    WINTRUST_WriteProviderToReg(GuidString, Message       , SoftpubMessage);
    WINTRUST_WriteProviderToReg(GuidString, Signature     , SoftpubSignature);
    WINTRUST_WriteProviderToReg(GuidString, Certificate   , SoftpubCertficate);
    WINTRUST_WriteProviderToReg(GuidString, CertCheck     , SoftpubCertCheck);
    WINTRUST_WriteProviderToReg(GuidString, FinalPolicy   , SoftpubFinalPolicy);
    WINTRUST_WriteProviderToReg(GuidString, Cleanup       , SoftpubCleanup);
}

/***************************************************************************
 *              WINTRUST_RegisterPublishedSoftware
 *
 * Register WIN_SPUB_ACTION_PUBLISHED_SOFTWARE actions and usages.
 *
 * NOTES
 *   WIN_SPUB_ACTION_PUBLISHED_SOFTWARE ({64B9D180-8DA2-11CF-8736-00AA00A485EB})
 *   is defined in wintrust.h
 *   We don't care about failures (see comments in DllRegisterServer)
 */
static void WINTRUST_RegisterPublishedSoftware(void)
{
    static const GUID ProvGUID = WIN_SPUB_ACTION_PUBLISHED_SOFTWARE;
    WCHAR GuidString[39];

    WINTRUST_Guid2Wstr(&ProvGUID , GuidString);

    TRACE("Going to register WIN_SPUB_ACTION_PUBLISHED_SOFTWARE : %s\n", wine_dbgstr_w(GuidString));

    /* HKLM\Software\Microsoft\Cryptography\Trust\Provider\*\{64B9D180-8DA2-11CF-8736-00AA00A485EB} */
    WINTRUST_WriteProviderToReg(GuidString, Initialization, SoftpubInitialization);
    WINTRUST_WriteProviderToReg(GuidString, Message       , SoftpubMessage);
    WINTRUST_WriteProviderToReg(GuidString, Signature     , SoftpubSignature);
    WINTRUST_WriteProviderToReg(GuidString, Certificate   , SoftpubCertficate);
    WINTRUST_WriteProviderToReg(GuidString, CertCheck     , SoftpubCertCheck);
    WINTRUST_WriteProviderToReg(GuidString, FinalPolicy   , SoftpubFinalPolicy);
    WINTRUST_WriteProviderToReg(GuidString, Cleanup       , SoftpubCleanup);
}

#define WIN_SPUB_ACTION_PUBLISHED_SOFTWARE_NOBADUI { 0xc6b2e8d0, 0xe005, 0x11cf, { 0xa1,0x34,0x00,0xc0,0x4f,0xd7,0xbf,0x43 }}

/***************************************************************************
 *              WINTRUST_RegisterPublishedSoftwareNoBadUi
 *
 * Register WIN_SPUB_ACTION_PUBLISHED_SOFTWARE_NOBADUI actions and usages.
 *
 * NOTES
 *   WIN_SPUB_ACTION_PUBLISHED_SOFTWARE_NOBADUI ({C6B2E8D0-E005-11CF-A134-00C04FD7BF43})
 *   is not defined in any include file. (FIXME: Find out if the name is correct).
 *   We don't care about failures (see comments in DllRegisterServer)
 */
static void WINTRUST_RegisterPublishedSoftwareNoBadUi(void)
{
    static const GUID ProvGUID = WIN_SPUB_ACTION_PUBLISHED_SOFTWARE_NOBADUI;
    WCHAR GuidString[39];

    WINTRUST_Guid2Wstr(&ProvGUID , GuidString);

    TRACE("Going to register WIN_SPUB_ACTION_PUBLISHED_SOFTWARE_NOBADUI : %s\n", wine_dbgstr_w(GuidString));

    /* HKLM\Software\Microsoft\Cryptography\Trust\Provider\*\{C6B2E8D0-E005-11CF-A134-00C04FD7BF43} */
    WINTRUST_WriteProviderToReg(GuidString, Initialization, SoftpubInitialization);
    WINTRUST_WriteProviderToReg(GuidString, Message       , SoftpubMessage);
    WINTRUST_WriteProviderToReg(GuidString, Signature     , SoftpubSignature);
    WINTRUST_WriteProviderToReg(GuidString, Certificate   , SoftpubCertficate);
    WINTRUST_WriteProviderToReg(GuidString, CertCheck     , SoftpubCertCheck);
    WINTRUST_WriteProviderToReg(GuidString, FinalPolicy   , SoftpubFinalPolicy);
    WINTRUST_WriteProviderToReg(GuidString, Cleanup       , SoftpubCleanup);
}

/***************************************************************************
 *              WINTRUST_RegisterGenCertVerify
 *
 * Register WINTRUST_ACTION_GENERIC_CERT_VERIFY actions and usages.
 *
 * NOTES
 *   WINTRUST_ACTION_GENERIC_CERT_VERIFY ({189A3842-3041-11D1-85E1-00C04FC295EE})
 *   is defined in softpub.h
 *   We don't care about failures (see comments in DllRegisterServer)
 */
static void WINTRUST_RegisterGenCertVerify(void)
{
    static const GUID ProvGUID = WINTRUST_ACTION_GENERIC_CERT_VERIFY;
    WCHAR GuidString[39];

    WINTRUST_Guid2Wstr(&ProvGUID , GuidString);

    TRACE("Going to register WINTRUST_ACTION_GENERIC_CERT_VERIFY : %s\n", wine_dbgstr_w(GuidString));

    /* HKLM\Software\Microsoft\Cryptography\Trust\Provider\*\{189A3842-3041-11D1-85E1-00C04FC295EE} */
    WINTRUST_WriteProviderToReg(GuidString, Initialization, SoftpubDefCertInit);
    WINTRUST_WriteProviderToReg(GuidString, Message       , SoftpubMessage);
    WINTRUST_WriteProviderToReg(GuidString, Signature     , SoftpubSignature);
    WINTRUST_WriteProviderToReg(GuidString, Certificate   , SoftpubCertficate);
    WINTRUST_WriteProviderToReg(GuidString, CertCheck     , SoftpubCertCheck);
    WINTRUST_WriteProviderToReg(GuidString, FinalPolicy   , SoftpubFinalPolicy);
    WINTRUST_WriteProviderToReg(GuidString, Cleanup       , SoftpubCleanup);
}

/***************************************************************************
 *              WINTRUST_RegisterTrustProviderTest
 *
 * Register WINTRUST_ACTION_TRUSTPROVIDER_TEST actions and usages.
 *
 * NOTES
 *   WINTRUST_ACTION_TRUSTPROVIDER_TEST ({573E31F8-DDBA-11D0-8CCB-00C04FC295EE})
 *   is defined in softpub.h
 *   We don't care about failures (see comments in DllRegisterServer)
 */
static void WINTRUST_RegisterTrustProviderTest(void)
{
    static const GUID ProvGUID = WINTRUST_ACTION_TRUSTPROVIDER_TEST;
    WCHAR GuidString[39];

    WINTRUST_Guid2Wstr(&ProvGUID , GuidString);

    TRACE("Going to register WINTRUST_ACTION_TRUSTPROVIDER_TEST : %s\n", wine_dbgstr_w(GuidString));

    /* HKLM\Software\Microsoft\Cryptography\Trust\Provider\*\{573E31F8-DDBA-11D0-8CCB-00C04FC295EE} */
    WINTRUST_WriteProviderToReg(GuidString, Initialization  , SoftpubInitialization);
    WINTRUST_WriteProviderToReg(GuidString, Message         , SoftpubMessage);
    WINTRUST_WriteProviderToReg(GuidString, Signature       , SoftpubSignature);
    WINTRUST_WriteProviderToReg(GuidString, Certificate     , SoftpubCertficate);
    WINTRUST_WriteProviderToReg(GuidString, CertCheck       , SoftpubCertCheck);
    WINTRUST_WriteProviderToReg(GuidString, FinalPolicy     , SoftpubFinalPolicy);
    WINTRUST_WriteProviderToReg(GuidString, DiagnosticPolicy, SoftpubDumpStructure);
    WINTRUST_WriteProviderToReg(GuidString, Cleanup         , SoftpubCleanup);
}

/***************************************************************************
 *              WINTRUST_RegisterHttpsProv
 *
 * Register HTTPSPROV_ACTION actions and usages.
 *
 * NOTES
 *   HTTPSPROV_ACTION ({573E31F8-AABA-11D0-8CCB-00C04FC295EE})
 *   is defined in softpub.h
 *   We don't care about failures (see comments in DllRegisterServer)
 */
static void WINTRUST_RegisterHttpsProv(void)
{
    static WCHAR SoftpubLoadUsage[] = {'S','o','f','t','p','u','b','L','o','a','d','D','e','f','U','s','a','g','e','C','a','l','l','D','a','t','a', 0};
    static WCHAR SoftpubFreeUsage[] = {'S','o','f','t','p','u','b','F','r','e','e','D','e','f','U','s','a','g','e','C','a','l','l','D','a','t','a', 0};
    static const WCHAR CBAlloc[]    = {'C','a','l','l','b','a','c','k','A','l','l','o','c','F','u','n','c','t','i','o','n', 0};
    static const WCHAR CBFree[]     = {'C','a','l','l','b','a','c','k','F','r','e','e','F','u','n','c','t','i','o','n', 0};
    static const GUID ProvGUID = HTTPSPROV_ACTION;
    WCHAR GuidString[39];
    WCHAR ProvDllName[sizeof(SP_POLICY_PROVIDER_DLL_NAME)];

    WINTRUST_Guid2Wstr(&ProvGUID , GuidString);

    TRACE("Going to register HTTPSPROV_ACTION : %s\n", wine_dbgstr_w(GuidString));

    lstrcpyW(ProvDllName, SP_POLICY_PROVIDER_DLL_NAME); \

    /* HKLM\Software\Microsoft\Cryptography\Trust\Usages\1.3.6.1.5.5.7.3.1 */
    WINTRUST_WriteSingleUsageEntry(szOID_PKIX_KP_SERVER_AUTH, Dll, ProvDllName);
    WINTRUST_WriteSingleUsageEntry(szOID_PKIX_KP_SERVER_AUTH, CBAlloc, SoftpubLoadUsage );
    WINTRUST_WriteSingleUsageEntry(szOID_PKIX_KP_SERVER_AUTH, CBFree, SoftpubFreeUsage );
    WINTRUST_WriteSingleUsageEntry(szOID_PKIX_KP_SERVER_AUTH, DefaultId, GuidString );
    /* HKLM\Software\Microsoft\Cryptography\Trust\Usages\1.3.6.1.5.5.7.3.2 */
    WINTRUST_WriteSingleUsageEntry(szOID_PKIX_KP_CLIENT_AUTH, Dll, ProvDllName);
    WINTRUST_WriteSingleUsageEntry(szOID_PKIX_KP_CLIENT_AUTH, CBAlloc, SoftpubLoadUsage );
    WINTRUST_WriteSingleUsageEntry(szOID_PKIX_KP_CLIENT_AUTH, CBFree, SoftpubFreeUsage );
    WINTRUST_WriteSingleUsageEntry(szOID_PKIX_KP_CLIENT_AUTH, DefaultId, GuidString );
    /* HKLM\Software\Microsoft\Cryptography\Trust\Usages\1.3.6.1.4.1.311.10.3.3 */
    WINTRUST_WriteSingleUsageEntry(szOID_SERVER_GATED_CRYPTO, Dll, ProvDllName);
    WINTRUST_WriteSingleUsageEntry(szOID_SERVER_GATED_CRYPTO, CBAlloc, SoftpubLoadUsage );
    WINTRUST_WriteSingleUsageEntry(szOID_SERVER_GATED_CRYPTO, CBFree, SoftpubFreeUsage );
    WINTRUST_WriteSingleUsageEntry(szOID_SERVER_GATED_CRYPTO, DefaultId, GuidString );
    /* HKLM\Software\Microsoft\Cryptography\Trust\Usages\2.16.840.1.113730.4.1 */
    WINTRUST_WriteSingleUsageEntry(szOID_SGC_NETSCAPE, Dll, ProvDllName);
    WINTRUST_WriteSingleUsageEntry(szOID_SGC_NETSCAPE, CBAlloc, SoftpubLoadUsage );
    WINTRUST_WriteSingleUsageEntry(szOID_SGC_NETSCAPE, CBFree, SoftpubFreeUsage );
    WINTRUST_WriteSingleUsageEntry(szOID_SGC_NETSCAPE, DefaultId, GuidString );

    /* HKLM\Software\Microsoft\Cryptography\Trust\Provider\*\{573E31F8-AABA-11D0-8CCB-00C04FC295EE} */
    WINTRUST_WriteProviderToReg(GuidString, Initialization, SoftpubInitialization);
    WINTRUST_WriteProviderToReg(GuidString, Message       , SoftpubMessage);
    WINTRUST_WriteProviderToReg(GuidString, Signature     , SoftpubSignature);
    WINTRUST_WriteProviderToReg(GuidString, Certificate   , HTTPSCertificateTrust);
    WINTRUST_WriteProviderToReg(GuidString, CertCheck     , SoftpubCertCheck);
    WINTRUST_WriteProviderToReg(GuidString, FinalPolicy   , HTTPSFinalProv);
    WINTRUST_WriteProviderToReg(GuidString, Cleanup       , SoftpubCleanup);
}

/***************************************************************************
 *              WINTRUST_RegisterOfficeSignVerify
 *
 * Register OFFICESIGN_ACTION_VERIFY actions and usages.
 *
 * NOTES
 *   OFFICESIGN_ACTION_VERIFY ({5555C2CD-17FB-11D1-85C4-00C04FC295EE})
 *   is defined in softpub.h
 *   We don't care about failures (see comments in DllRegisterServer)
 */
static void WINTRUST_RegisterOfficeSignVerify(void)
{
    static const GUID ProvGUID = OFFICESIGN_ACTION_VERIFY;
    WCHAR GuidString[39];

    WINTRUST_Guid2Wstr(&ProvGUID , GuidString);

    TRACE("Going to register OFFICESIGN_ACTION_VERIFY : %s\n", wine_dbgstr_w(GuidString));

    /* HKLM\Software\Microsoft\Cryptography\Trust\Provider\*\{5555C2CD-17FB-11D1-85C4-00C04FC295EE} */
    WINTRUST_WriteProviderToReg(GuidString, Initialization, OfficeInitializePolicy);
    WINTRUST_WriteProviderToReg(GuidString, Message       , SoftpubMessage);
    WINTRUST_WriteProviderToReg(GuidString, Signature     , SoftpubSignature);
    WINTRUST_WriteProviderToReg(GuidString, Certificate   , SoftpubCertficate);
    WINTRUST_WriteProviderToReg(GuidString, CertCheck     , SoftpubCertCheck);
    WINTRUST_WriteProviderToReg(GuidString, FinalPolicy   , SoftpubFinalPolicy);
    WINTRUST_WriteProviderToReg(GuidString, Cleanup       , OfficeCleanupPolicy);
}

/***********************************************************************
 *              DllRegisterServer (WINTRUST.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    HRESULT Res = S_OK;
    HKEY Key;

    TRACE("\n");

    /* Create the necessary action registry structures */
    WINTRUST_InitRegStructs();

    /* FIXME:
     * 
     * A short list of stuff that should be done here:
     *
     * - Several calls to CryptRegisterOIDFunction
     * - Several action registrations (NOT through WintrustAddActionID)
     * - Several calls to CryptSIPAddProvider
     * - One call to CryptSIPRemoveProvider (do we need that?)
     */

    /* Testing on W2K3 shows:
     * If we cannot open HKLM\Software\Microsoft\Cryptography\Providers\Trust
     * for writing, DllRegisterServer returns S_FALSE. If the key can be opened 
     * there is no check whether the actions can be written in the registry.
     * As the previous list shows, there are several calls after these registrations.
     * If they fail they will overwrite the returnvalue of DllRegisterServer.
     */

    /* Check if we can open/create HKLM\Software\Microsoft\Cryptography\Providers\Trust */
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, Trust, 0, NULL, 0, KEY_WRITE, NULL, &Key, NULL) != ERROR_SUCCESS)
        Res = S_FALSE;
    RegCloseKey(Key);

    /* Register several Trust Provider actions */
    WINTRUST_RegisterGenVerifyV2();
    WINTRUST_RegisterPublishedSoftware();
    WINTRUST_RegisterPublishedSoftwareNoBadUi();
    WINTRUST_RegisterGenCertVerify();
    WINTRUST_RegisterTrustProviderTest();
    WINTRUST_RegisterHttpsProv();
    WINTRUST_RegisterOfficeSignVerify();

    /* Free the registry structures */
    WINTRUST_FreeRegStructs();

    return Res;
}

/***********************************************************************
 *              DllUnregisterServer (WINTRUST.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
     FIXME("stub\n");
     return S_OK;
}

/***********************************************************************
 *              SoftpubDllRegisterServer (WINTRUST.@)
 */
HRESULT WINAPI SoftpubDllRegisterServer(void)
{
     FIXME("stub\n");
     return S_OK;
}

/***********************************************************************
 *              SoftpubDllUnregisterServer (WINTRUST.@)
 */
HRESULT WINAPI SoftpubDllUnregisterServer(void)
{
     FIXME("stub\n");
     return S_OK;
}

/***********************************************************************
 *              mscat32DllRegisterServer (WINTRUST.@)
 */
HRESULT WINAPI mscat32DllRegisterServer(void)
{
     FIXME("stub\n");
     return S_OK;
}

/***********************************************************************
 *              mscat32DllUnregisterServer (WINTRUST.@)
 */
HRESULT WINAPI mscat32DllUnregisterServer(void)
{
     FIXME("stub\n");
     return S_OK;
}

/***********************************************************************
 *              mssip32DllRegisterServer (WINTRUST.@)
 */
HRESULT WINAPI mssip32DllRegisterServer(void)
{
     FIXME("stub\n");
     return S_OK;
}

/***********************************************************************
 *              mssip32DllUnregisterServer (WINTRUST.@)
 */
HRESULT WINAPI mssip32DllUnregisterServer(void)
{
     FIXME("stub\n");
     return S_OK;
}
