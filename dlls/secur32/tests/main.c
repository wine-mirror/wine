/*
 * Miscellaneous secur32 tests
 *
 * Copyright 2005, 2006 Kai Blin
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

#define SECURITY_WIN32

#include <stdarg.h>
#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include <sspi.h>
#include <ntsecapi.h>

#include "wine/test.h"

static HMODULE secdll;
static PSecurityFunctionTableA (SEC_ENTRY * pInitSecurityInterfaceA)(void);
static SECURITY_STATUS (SEC_ENTRY * pEnumerateSecurityPackagesA)(PULONG, PSecPkgInfoA*);
static SECURITY_STATUS (SEC_ENTRY * pFreeContextBuffer)(PVOID pv);
static SECURITY_STATUS (SEC_ENTRY * pQuerySecurityPackageInfoA)(SEC_CHAR*, PSecPkgInfoA*);
static SECURITY_STATUS (SEC_ENTRY * pAcquireCredentialsHandleA)(SEC_CHAR*, SEC_CHAR*,
                            ULONG, PLUID, PVOID, SEC_GET_KEY_FN, PVOID, PCredHandle, PTimeStamp);
static SECURITY_STATUS (SEC_ENTRY * pInitializeSecurityContextA)(PCredHandle, PCtxtHandle,
                            SEC_CHAR*, ULONG, ULONG, ULONG, PSecBufferDesc, ULONG, 
                            PCtxtHandle, PSecBufferDesc, PULONG, PTimeStamp);
static SECURITY_STATUS (SEC_ENTRY * pCompleteAuthToken)(PCtxtHandle, PSecBufferDesc);
static SECURITY_STATUS (SEC_ENTRY * pAcceptSecurityContext)(PCredHandle, PCtxtHandle,
                            PSecBufferDesc, ULONG, ULONG, PCtxtHandle, PSecBufferDesc,
                            PULONG, PTimeStamp);
static SECURITY_STATUS (SEC_ENTRY * pFreeCredentialsHandle)(PCredHandle);
static SECURITY_STATUS (SEC_ENTRY * pDeleteSecurityContext)(PCtxtHandle);
static SECURITY_STATUS (SEC_ENTRY * pQueryContextAttributesA)(PCtxtHandle, ULONG, PVOID);

static void InitFunctionPtrs(void)
{
    secdll = LoadLibraryA("secur32.dll");
    if(!secdll)
        secdll = LoadLibraryA("security.dll");
    if(secdll)
    {
        pInitSecurityInterfaceA = (PVOID)GetProcAddress(secdll, "InitSecurityInterfaceA");
        pEnumerateSecurityPackagesA = (PVOID)GetProcAddress(secdll, "EnumerateSecurityPackagesA");
        pFreeContextBuffer = (PVOID)GetProcAddress(secdll, "FreeContextBuffer");
        pQuerySecurityPackageInfoA = (PVOID)GetProcAddress(secdll, "QuerySecurityPackageInfoA");
        pAcquireCredentialsHandleA = (PVOID)GetProcAddress(secdll, "AcquireCredentialsHandleA");
        pInitializeSecurityContextA = (PVOID)GetProcAddress(secdll, "InitializeSecurityContextA");
        pCompleteAuthToken = (PVOID)GetProcAddress(secdll, "CompleteAuthToken");
        pAcceptSecurityContext = (PVOID)GetProcAddress(secdll, "AcceptSecurityContext");
        pFreeCredentialsHandle = (PVOID)GetProcAddress(secdll, "FreeCredentialsHandle");
        pDeleteSecurityContext = (PVOID)GetProcAddress(secdll, "DeleteSecurityContext");
        pQueryContextAttributesA = (PVOID)GetProcAddress(secdll, "QueryContextAttributesA");
    }
}

/*---------------------------------------------------------*/
/* General helper functions */

static const char* getSecError(SECURITY_STATUS status)
{
    static char buf[20];

#define _SEC_ERR(x) case (x): return #x;
    switch(status)
    {
        _SEC_ERR(SEC_E_OK);
        _SEC_ERR(SEC_E_INSUFFICIENT_MEMORY);
        _SEC_ERR(SEC_E_INVALID_HANDLE);
        _SEC_ERR(SEC_E_UNSUPPORTED_FUNCTION);
        _SEC_ERR(SEC_E_TARGET_UNKNOWN);
        _SEC_ERR(SEC_E_INTERNAL_ERROR);
        _SEC_ERR(SEC_E_SECPKG_NOT_FOUND);
        _SEC_ERR(SEC_E_NOT_OWNER);
        _SEC_ERR(SEC_E_CANNOT_INSTALL);
        _SEC_ERR(SEC_E_INVALID_TOKEN);
        _SEC_ERR(SEC_E_CANNOT_PACK);
        _SEC_ERR(SEC_E_QOP_NOT_SUPPORTED);
        _SEC_ERR(SEC_E_NO_IMPERSONATION);
        _SEC_ERR(SEC_I_CONTINUE_NEEDED);
        _SEC_ERR(SEC_E_BUFFER_TOO_SMALL);
        _SEC_ERR(SEC_E_ILLEGAL_MESSAGE);
        _SEC_ERR(SEC_E_LOGON_DENIED);
        _SEC_ERR(SEC_E_NO_CREDENTIALS);
        _SEC_ERR(SEC_E_OUT_OF_SEQUENCE);
        default:
            sprintf(buf, "%08lx\n", status);
            return buf;
    }
#undef _SEC_ERR
}

/*---------------------------------------------------------*/
/* Helper for testQuerySecurityPagageInfo */

static SECURITY_STATUS setupPackageA(SEC_CHAR *p_package_name, 
        PSecPkgInfoA *p_pkg_info)
{
    SECURITY_STATUS ret;
    
    ret = pQuerySecurityPackageInfoA( p_package_name, p_pkg_info);
    return ret;
}

/*--------------------------------------------------------- */
/* The test functions */

static void testInitSecurityInterface(void)
{
    PSecurityFunctionTableA sec_fun_table = NULL;

    sec_fun_table = pInitSecurityInterfaceA();
    ok(sec_fun_table != NULL, "InitSecurityInterface() returned NULL.\n");

}

static void testEnumerateSecurityPackages(void)
{

    SECURITY_STATUS sec_status;
    ULONG           num_packages, i;
    PSecPkgInfoA    pkg_info = NULL;

    trace("Running testEnumerateSecurityPackages\n");
    
    sec_status = pEnumerateSecurityPackagesA(&num_packages, &pkg_info);

    ok(sec_status == SEC_E_OK, 
            "EnumerateSecurityPackages() should return %ld, not %08lx\n",
            SEC_E_OK, sec_status);

    if (num_packages == 0)
    {
        todo_wine
        ok(num_packages > 0, "Number of sec packages should be > 0 ,but is %ld\n",
                num_packages);
        skip("no sec packages to check\n");
        return;
    }
    else
        ok(num_packages > 0, "Number of sec packages should be > 0 ,but is %ld\n",
                num_packages);

    ok(pkg_info != NULL, 
            "pkg_info should not be NULL after EnumerateSecurityPackages\n");
    
    trace("Number of packages: %ld\n", num_packages);
    for(i = 0; i < num_packages; ++i){
        trace("%ld: Package \"%s\"\n", i, pkg_info[i].Name);
        trace("Supported flags:\n");
#define X(flag) \
        if(pkg_info[i].fCapabilities & flag) \
            trace("\t" #flag "\n")

        X(SECPKG_FLAG_INTEGRITY);
        X(SECPKG_FLAG_PRIVACY);
        X(SECPKG_FLAG_TOKEN_ONLY);
        X(SECPKG_FLAG_DATAGRAM);
        X(SECPKG_FLAG_CONNECTION);
        X(SECPKG_FLAG_MULTI_REQUIRED);
        X(SECPKG_FLAG_CLIENT_ONLY);
        X(SECPKG_FLAG_EXTENDED_ERROR);
        X(SECPKG_FLAG_IMPERSONATION);
        X(SECPKG_FLAG_ACCEPT_WIN32_NAME);
        X(SECPKG_FLAG_STREAM);
        X(SECPKG_FLAG_NEGOTIABLE);
        X(SECPKG_FLAG_GSS_COMPATIBLE);
        X(SECPKG_FLAG_LOGON);
        X(SECPKG_FLAG_ASCII_BUFFERS);
        X(SECPKG_FLAG_FRAGMENT);
        X(SECPKG_FLAG_MUTUAL_AUTH);
        X(SECPKG_FLAG_DELEGATION);
        X(SECPKG_FLAG_READONLY_WITH_CHECKSUM);
        X(SECPKG_FLAG_RESTRICTED_TOKENS);
        X(SECPKG_FLAG_NEGO_EXTENDER);
        X(SECPKG_FLAG_NEGOTIABLE2);
        X(SECPKG_FLAG_APPCONTAINER_PASSTHROUGH);
        X(SECPKG_FLAG_APPCONTAINER_CHECKS);
#undef X
        trace("Comment: %s\n", pkg_info[i].Comment);
        trace("\n");
    }

    pFreeContextBuffer(pkg_info);
}


static void testQuerySecurityPackageInfo(void)
{
    SECURITY_STATUS     sec_status;
    PSecPkgInfoA        pkg_info;
    static SEC_CHAR     ntlm[]     = "NTLM",
                        winetest[] = "Winetest";

    trace("Running testQuerySecurityPackageInfo\n");

    /* Test with an existing package. Test should pass */

    pkg_info = (void *)0xdeadbeef;
    sec_status = setupPackageA(ntlm, &pkg_info);

    ok((sec_status == SEC_E_OK) || (sec_status == SEC_E_SECPKG_NOT_FOUND) ||
       broken(sec_status == SEC_E_UNSUPPORTED_FUNCTION), /* win95 */
       "Return value of QuerySecurityPackageInfo() shouldn't be %s\n",
       getSecError(sec_status) );

    if (sec_status == SEC_E_OK)
    {
        ok(pkg_info != (void *)0xdeadbeef, "wrong pkg_info address %p\n", pkg_info);
        ok(pkg_info->wVersion == 1, "wVersion always should be 1, but is %d\n", pkg_info->wVersion);
        /* there is no point in testing pkg_info->cbMaxToken since it varies
         * between implementations.
         */

        sec_status = pFreeContextBuffer(pkg_info);
        ok( sec_status == SEC_E_OK,
            "Return value of FreeContextBuffer() shouldn't be %s\n",
            getSecError(sec_status) );
    }

    /* Test with a nonexistent package, test should fail */

    pkg_info = (void *)0xdeadbeef;
    sec_status = pQuerySecurityPackageInfoA(winetest, &pkg_info);

    ok( sec_status == SEC_E_SECPKG_NOT_FOUND,
        "Return value of QuerySecurityPackageInfo() should be %s for a nonexistent package\n",
        getSecError(SEC_E_SECPKG_NOT_FOUND));

    ok(pkg_info == (void *)0xdeadbeef, "wrong pkg_info address %p\n", pkg_info);
}

static void test_get_logon_session_data(void)
{
    SECURITY_LOGON_SESSION_DATA *data = NULL;
    TOKEN_STATISTICS ts;
    HANDLE thread_hdl;
    NTSTATUS status;
    DWORD ret_len;
    BOOL ret;

    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &thread_hdl);
    ok(ret, "got %ld\n", GetLastError());

    if (!ret) return;

    ret_len = sizeof(TOKEN_STATISTICS);
    ret = GetTokenInformation(thread_hdl, TokenStatistics, &ts, sizeof(TOKEN_STATISTICS), &ret_len);
    ok(ret, "got %ld\n", GetLastError());

    if (!ret) goto cleanup;

    status = LsaGetLogonSessionData(&ts.AuthenticationId, &data);
    ok(!status, "got %08lx\n", status);

    if (status) goto cleanup;

    ok(data->Size >= sizeof(SECURITY_LOGON_SESSION_DATA), "Size == %ld\n", data->Size);
    ok(!memcmp(&data->LogonId, &ts.AuthenticationId, sizeof(LUID)), "LogonId mismatch\n");

    /* We can't easily verify the content of the various logon parameters, so just ensure data is present */
    ok(data->AuthenticationPackage.Length > 0, "AuthenticationPackage missing\n");

cleanup:
    CloseHandle(thread_hdl);
    if (data != NULL) LsaFreeReturnBuffer(data);
}

START_TEST(main)
{
    InitFunctionPtrs();
    if(pInitSecurityInterfaceA)
        testInitSecurityInterface();
    if(pFreeContextBuffer)
    {
        if(pEnumerateSecurityPackagesA)
            testEnumerateSecurityPackages();
        if(pQuerySecurityPackageInfoA)
        {
            testQuerySecurityPackageInfo();
        }
    }
    if(secdll)
        FreeLibrary(secdll);

    test_get_logon_session_data();
}
