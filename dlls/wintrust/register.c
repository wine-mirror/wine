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

#include "guiddef.h"
#include "wintrust.h"
#include "softpub.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wintrust);

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
    static const WCHAR Dll[]      = {'$','D','L','L', 0};
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
    static const WCHAR wszFormat[] = {'{','%','0','8','l','X','-','%','0','4','X','-','%','0','4','X','-',
                                      '%','0','2','X','%','0','2','X','-','%','0','2','X','%','0','2','X','%','0','2','X','%','0','2',
                                      'X','%','0','2','X','%','0','2','X','}', 0};

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
    wsprintfW(GuidString, wszFormat, pgActionID->Data1, pgActionID->Data2, pgActionID->Data3,
        pgActionID->Data4[0], pgActionID->Data4[1], pgActionID->Data4[2], pgActionID->Data4[3],
        pgActionID->Data4[4], pgActionID->Data4[5], pgActionID->Data4[6], pgActionID->Data4[7]);

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
     * an error is the one used to propagate the last error. 
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
    static const WCHAR wszFormat[] = {'{','%','0','8','l','X','-','%','0','4','X','-','%','0','4','X','-',
                                      '%','0','2','X','%','0','2','X','-','%','0','2','X','%','0','2','X','%','0','2','X','%','0','2',
                                      'X','%','0','2','X','%','0','2','X','}', 0};

    WCHAR GuidString[39];

    TRACE("(%s)\n", debugstr_guid(pgActionID));
 
    if (!pgActionID)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return TRUE;
    }

    /* Create this string only once, instead of in the helper function */
    wsprintfW(GuidString, wszFormat, pgActionID->Data1, pgActionID->Data2, pgActionID->Data3,
        pgActionID->Data4[0], pgActionID->Data4[1], pgActionID->Data4[2], pgActionID->Data4[3],
        pgActionID->Data4[4], pgActionID->Data4[5], pgActionID->Data4[6], pgActionID->Data4[7]);

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
 *              DllRegisterServer (WINTRUST.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
     FIXME("stub\n");
     return S_OK;
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
