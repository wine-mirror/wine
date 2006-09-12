/* Unit test suite for wintrust API functions
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
 *
 */

#include <stdarg.h>
#include <stdio.h>

#include "windows.h"
#include "softpub.h"
#include "wintrust.h"
#include "winreg.h"

#include "wine/test.h"

static BOOL (WINAPI * pWintrustAddActionID)(GUID*, DWORD, CRYPT_REGISTER_ACTIONID*);
static BOOL (WINAPI * pWintrustAddDefaultForUsage)(const CHAR*,CRYPT_PROVIDER_REGDEFUSAGE*);
static BOOL (WINAPI * pWintrustRemoveActionID)(GUID*);

static HMODULE hWintrust = 0;

#define WINTRUST_GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hWintrust, #func); \
    if(!p ## func) { \
      trace("GetProcAddress(%s) failed\n", #func); \
      FreeLibrary(hWintrust); \
      return FALSE; \
    }

static BOOL InitFunctionPtrs(void)
{
    hWintrust = LoadLibraryA("wintrust.dll");

    if(!hWintrust)
    {
        trace("Could not load wintrust.dll\n");
        return FALSE;
    }

    WINTRUST_GET_PROC(WintrustAddActionID)
    WINTRUST_GET_PROC(WintrustAddDefaultForUsage)
    WINTRUST_GET_PROC(WintrustRemoveActionID)

    return TRUE;
}

static void test_AddRem_ActionID(void)
{
    static WCHAR DummyDllW[]      = {'d','e','a','d','b','e','e','f','.','d','l','l',0 };
    static WCHAR DummyFunctionW[] = {'d','u','m','m','y','f','u','n','c','t','i','o','n',0 };
    GUID ActionID = { 0xdeadbeef, 0xdead, 0xbeef, { 0xde,0xad,0xbe,0xef,0xde,0xad,0xbe,0xef }};
    CRYPT_REGISTER_ACTIONID ActionIDFunctions;
    CRYPT_TRUST_REG_ENTRY EmptyProvider = { 0, NULL, NULL };
    CRYPT_TRUST_REG_ENTRY DummyProvider = { sizeof(CRYPT_TRUST_REG_ENTRY), DummyDllW, DummyFunctionW };
    BOOL ret;

    /* All NULL */
    SetLastError(0xdeadbeef);
    ret = pWintrustAddActionID(NULL, 0, NULL);
    ok (!ret, "Expected WintrustAddActionID to fail.\n");
    ok (GetLastError() == ERROR_INVALID_PARAMETER /* XP/W2K3 */ ||
        GetLastError() == 0xdeadbeef              /* Win98/NT4/W2K */,
        "Expected ERROR_INVALID_PARAMETER(W2K3) or 0xdeadbeef(Win98/NT4/W2K), got %ld.\n", GetLastError());

    /* NULL functions */
    SetLastError(0xdeadbeef);
    ret = pWintrustAddActionID(&ActionID, 0, NULL);
    ok (!ret, "Expected WintrustAddActionID to fail.\n");
    ok (GetLastError() == ERROR_INVALID_PARAMETER /* XP/W2K3 */ ||
        GetLastError() == 0xdeadbeef              /* Win98/NT4/W2K */,
        "Expected ERROR_INVALID_PARAMETER(W2K3) or 0xdeadbeef(Win98/NT4/W2K), got %ld.\n", GetLastError());

    /* All OK (although no functions defined), except cbStruct is not set in ActionIDFunctions */
    SetLastError(0xdeadbeef);
    memset(&ActionIDFunctions, 0, sizeof(CRYPT_REGISTER_ACTIONID));
    ret = pWintrustAddActionID(&ActionID, 0, &ActionIDFunctions);
    ok (!ret, "Expected WintrustAddActionID to fail.\n");
    ok (GetLastError() == ERROR_INVALID_PARAMETER /* XP/W2K3 */ ||
        GetLastError() == 0xdeadbeef              /* Win98/NT4/W2K */,
        "Expected ERROR_INVALID_PARAMETER(W2K3) or 0xdeadbeef(Win98/NT4/W2K), got %ld.\n", GetLastError());

    /* All OK (although no functions defined) and cbStruct is set now */
    SetLastError(0xdeadbeef);
    memset(&ActionIDFunctions, 0, sizeof(CRYPT_REGISTER_ACTIONID));
    ActionIDFunctions.cbStruct = sizeof(CRYPT_REGISTER_ACTIONID);
    ret = pWintrustAddActionID(&ActionID, 0, &ActionIDFunctions);
    ok (ret, "Expected WintrustAddActionID to succeed.\n");
    ok (GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %ld.\n", GetLastError());

    /* All OK and all (but 1) functions are correctly defined. The DLL and entrypoints
     * are not present.
     */
    memset(&ActionIDFunctions, 0, sizeof(CRYPT_REGISTER_ACTIONID));
    ActionIDFunctions.cbStruct = sizeof(CRYPT_REGISTER_ACTIONID);
    ActionIDFunctions.sInitProvider = DummyProvider;
    ActionIDFunctions.sObjectProvider = DummyProvider;
    ActionIDFunctions.sSignatureProvider = EmptyProvider;
    ActionIDFunctions.sCertificateProvider = DummyProvider;
    ActionIDFunctions.sCertificatePolicyProvider = DummyProvider;
    ActionIDFunctions.sFinalPolicyProvider = DummyProvider;
    ActionIDFunctions.sTestPolicyProvider = DummyProvider;
    ActionIDFunctions.sCleanupProvider = DummyProvider;
    SetLastError(0xdeadbeef);
    ret = pWintrustAddActionID(&ActionID, 0, &ActionIDFunctions);
    ok (ret, "Expected WintrustAddActionID to succeed.\n");
    ok (GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %ld.\n", GetLastError());

    /* All OK and all functions are correctly defined. The DLL and entrypoints
     * are not present.
     */
    memset(&ActionIDFunctions, 0, sizeof(CRYPT_REGISTER_ACTIONID));
    ActionIDFunctions.cbStruct = sizeof(CRYPT_REGISTER_ACTIONID);
    ActionIDFunctions.sInitProvider = DummyProvider;
    ActionIDFunctions.sObjectProvider = DummyProvider;
    ActionIDFunctions.sSignatureProvider = DummyProvider;
    ActionIDFunctions.sCertificateProvider = DummyProvider;
    ActionIDFunctions.sCertificatePolicyProvider = DummyProvider;
    ActionIDFunctions.sFinalPolicyProvider = DummyProvider;
    ActionIDFunctions.sTestPolicyProvider = DummyProvider;
    ActionIDFunctions.sCleanupProvider = DummyProvider;
    SetLastError(0xdeadbeef);
    ret = pWintrustAddActionID(&ActionID, 0, &ActionIDFunctions);
    ok (ret, "Expected WintrustAddActionID to succeed.\n");
    ok (GetLastError() == 0xdeadbeef,
        "Expected 0xdeadbeef, got %ld.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pWintrustRemoveActionID(&ActionID);
    ok ( ret, "WintrustRemoveActionID failed : 0x%08lx\n", GetLastError());
    ok ( GetLastError() == 0xdeadbeef, "Last error should not have been changed: 0x%08lx\n", GetLastError());

    /* NULL input */
    SetLastError(0xdeadbeef);
    ret = pWintrustRemoveActionID(NULL);
    ok (ret, "Expected WintrustRemoveActionID to succeed.\n");
    ok (GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %ld.\n", GetLastError());

    /* The passed GUID is removed by a previous call, so it's basically a test with a nonexistent Trust provider */ 
    SetLastError(0xdeadbeef);
    ret = pWintrustRemoveActionID(&ActionID);
    ok (ret, "Expected WintrustRemoveActionID to succeed.\n");
    ok (GetLastError() == 0xdeadbeef,
        "Expected 0xdeadbeef, got %ld.\n", GetLastError());
}

static void test_AddDefaultForUsage(void)
{
    BOOL ret;
    LONG res;
    static GUID ActionID        = { 0xdeadbeef, 0xdead, 0xbeef, { 0xde,0xad,0xbe,0xef,0xde,0xad,0xbe,0xef }};
    static WCHAR DummyDllW[]    = {'d','e','a','d','b','e','e','f','.','d','l','l',0 };
    static CHAR DummyFunction[] = "dummyfunction";
    static const CHAR oid[]     = "1.2.3.4.5.6.7.8.9.10";
    static const CHAR Usages[]  = "SOFTWARE\\Microsoft\\Cryptography\\Providers\\Trust\\Usages\\1.2.3.4.5.6.7.8.9.10";
    static CRYPT_PROVIDER_REGDEFUSAGE DefUsage;

    /* All NULL */
    SetLastError(0xdeadbeef);
    ret = pWintrustAddDefaultForUsage(NULL, NULL);
    ok (!ret, "Expected WintrustAddDefaultForUsage to fail.\n");
    ok (GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %ld.\n", GetLastError());

    /* NULL defusage */
    SetLastError(0xdeadbeef);
    ret = pWintrustAddDefaultForUsage(oid, NULL);
    ok (!ret, "Expected WintrustAddDefaultForUsage to fail.\n");
    ok (GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %ld.\n", GetLastError());

    /* NULL oid and proper defusage */
    memset(&DefUsage, 0 , sizeof(CRYPT_PROVIDER_REGDEFUSAGE));
    DefUsage.cbStruct = sizeof(CRYPT_PROVIDER_REGDEFUSAGE);
    DefUsage.pgActionID = &ActionID;
    DefUsage.pwszDllName = DummyDllW;
    DefUsage.pwszLoadCallbackDataFunctionName = DummyFunction;
    DefUsage.pwszFreeCallbackDataFunctionName = DummyFunction;
    SetLastError(0xdeadbeef);
    ret = pWintrustAddDefaultForUsage(NULL, &DefUsage);
    ok (!ret, "Expected WintrustAddDefaultForUsage to fail.\n");
    ok (GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %ld.\n", GetLastError());

    /* Just the ActionID */
    memset(&DefUsage, 0 , sizeof(CRYPT_PROVIDER_REGDEFUSAGE));
    DefUsage.cbStruct = sizeof(CRYPT_PROVIDER_REGDEFUSAGE);
    DefUsage.pgActionID = &ActionID;
    SetLastError(0xdeadbeef);
    ret = pWintrustAddDefaultForUsage(oid, &DefUsage);
    ok ( ret, "Expected WintrustAddDefaultForUsage to succeed\n");
    ok (GetLastError() == 0xdeadbeef,
        "Last error should not have been changed: 0x%08lx\n", GetLastError());
   
    /* No ActionID */
    memset(&DefUsage, 0 , sizeof(CRYPT_PROVIDER_REGDEFUSAGE));
    DefUsage.cbStruct = sizeof(CRYPT_PROVIDER_REGDEFUSAGE);
    DefUsage.pwszDllName = DummyDllW;
    DefUsage.pwszLoadCallbackDataFunctionName = DummyFunction;
    DefUsage.pwszFreeCallbackDataFunctionName = DummyFunction;
    ret = pWintrustAddDefaultForUsage(oid, &DefUsage);
    ok (!ret, "Expected WintrustAddDefaultForUsage to fail.\n");
    ok (GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %ld.\n", GetLastError());

    /* cbStruct set to 0 */
    memset(&DefUsage, 0 , sizeof(CRYPT_PROVIDER_REGDEFUSAGE));
    DefUsage.cbStruct = 0;
    DefUsage.pgActionID = &ActionID;
    DefUsage.pwszDllName = DummyDllW;
    DefUsage.pwszLoadCallbackDataFunctionName = DummyFunction;
    DefUsage.pwszFreeCallbackDataFunctionName = DummyFunction;
    SetLastError(0xdeadbeef);
    ret = pWintrustAddDefaultForUsage(oid, &DefUsage);
    ok (!ret, "Expected WintrustAddDefaultForUsage to fail.\n");
    ok (GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %ld.\n", GetLastError());

    /* All OK */
    memset(&DefUsage, 0 , sizeof(CRYPT_PROVIDER_REGDEFUSAGE));
    DefUsage.cbStruct = sizeof(CRYPT_PROVIDER_REGDEFUSAGE);
    DefUsage.pgActionID = &ActionID;
    DefUsage.pwszDllName = DummyDllW;
    DefUsage.pwszLoadCallbackDataFunctionName = DummyFunction;
    DefUsage.pwszFreeCallbackDataFunctionName = DummyFunction;
    SetLastError(0xdeadbeef);
    ret = pWintrustAddDefaultForUsage(oid, &DefUsage);
    ok ( ret, "Expected WintrustAddDefaultForUsage to succeed\n");
    ok (GetLastError() == 0xdeadbeef,
        "Last error should not have been changed: 0x%08lx\n", GetLastError());

    /* There is no corresponding remove for WintrustAddDefaultForUsage
     * so we delete the registry key manually.
     */
    if (ret)
    {
        res = RegDeleteKeyA(HKEY_LOCAL_MACHINE, Usages);
        ok (res == ERROR_SUCCESS, "Key delete failed : 0x%08lx\n", res);
    }
}

START_TEST(register)
{
    if(!InitFunctionPtrs())
        return;

    test_AddRem_ActionID();
    test_AddDefaultForUsage();

    FreeLibrary(hWintrust);
}
