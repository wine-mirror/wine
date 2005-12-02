/*
 * Unit test suite for crypt32.dll's OID support functions.
 *
 * Copyright 2005 Juan Lang
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
#include <stdio.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#include <wincrypt.h>

#include "wine/test.h"

static void test_oidFunctionSet(void)
{
    HCRYPTOIDFUNCSET set1, set2;
    BOOL ret;
    LPWSTR buf = NULL;
    DWORD size;

    /* This crashes
    set = CryptInitOIDFunctionSet(NULL, 0);
     */

    /* The name doesn't mean much */
    set1 = CryptInitOIDFunctionSet("funky", 0);
    ok(set1 != 0, "CryptInitOIDFunctionSet failed: %08lx\n", GetLastError());
    if (set1)
    {
        /* These crash
        ret = CryptGetDefaultOIDDllList(NULL, 0, NULL, NULL);
        ret = CryptGetDefaultOIDDllList(NULL, 0, NULL, &size);
         */
        size = 0;
        ret = CryptGetDefaultOIDDllList(set1, 0, NULL, &size);
        ok(ret, "CryptGetDefaultOIDDllList failed: %08lx\n", GetLastError());
        if (ret)
        {
            buf = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
            if (buf)
            {
                ret = CryptGetDefaultOIDDllList(set1, 0, buf, &size);
                ok(ret, "CryptGetDefaultOIDDllList failed: %08lx\n",
                 GetLastError());
                ok(!*buf, "Expected empty DLL list\n");
                HeapFree(GetProcessHeap(), 0, buf);
            }
        }
    }

    /* MSDN says flags must be 0, but it's not checked */
    set1 = CryptInitOIDFunctionSet("", 1);
    ok(set1 != 0, "CryptInitOIDFunctionSet failed: %08lx\n", GetLastError());
    set2 = CryptInitOIDFunctionSet("", 0);
    ok(set2 != 0, "CryptInitOIDFunctionSet failed: %08lx\n", GetLastError());
    /* There isn't a free function, so there must be only one set per name to
     * limit leaks.  (I guess the sets are freed when crypt32 is unloaded.)
     */
    ok(set1 == set2, "Expected identical sets\n");
    if (set1)
    {
        /* The empty name function set used here seems to correspond to
         * DEFAULT.
         */
    }

    /* There's no installed function for a built-in encoding. */
    set1 = CryptInitOIDFunctionSet("CryptDllEncodeObject", 0);
    ok(set1 != 0, "CryptInitOIDFunctionSet failed: %08lx\n", GetLastError());
    if (set1)
    {
        void *funcAddr;
        HCRYPTOIDFUNCADDR hFuncAddr;

        ret = CryptGetOIDFunctionAddress(set1, X509_ASN_ENCODING, X509_CERT, 0,
         &funcAddr, &hFuncAddr);
        ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
         "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    }
}

typedef int (*funcY)(int);

static int funky(int x)
{
    return x;
}

static void test_installOIDFunctionAddress(void)
{
    BOOL ret;
    CRYPT_OID_FUNC_ENTRY entry = { CRYPT_DEFAULT_OID, funky };
    HCRYPTOIDFUNCSET set;

    /* This crashes
    ret = CryptInstallOIDFunctionAddress(NULL, 0, NULL, 0, NULL, 0);
     */

    /* Installing zero functions should work */
    SetLastError(0xdeadbeef);
    ret = CryptInstallOIDFunctionAddress(NULL, 0, "CryptDllEncodeObject", 0,
     NULL, 0);
    ok(ret && GetLastError() == 0xdeadbeef, "Expected success, got %08lx\n",
     GetLastError());

    /* The function name doesn't much matter */
    SetLastError(0xdeadbeef);
    ret = CryptInstallOIDFunctionAddress(NULL, 0, "OhSoFunky", 0, NULL, 0);
    ok(ret && GetLastError() == 0xdeadbeef, "Expected success, got %08lx\n",
     GetLastError());
    SetLastError(0xdeadbeef);
    entry.pszOID = X509_CERT;
    ret = CryptInstallOIDFunctionAddress(NULL, 0, "OhSoFunky", 1, &entry, 0);
    ok(ret && GetLastError() == 0xdeadbeef, "Expected success, got %08lx\n",
     GetLastError());
    set = CryptInitOIDFunctionSet("OhSoFunky", 0);
    ok(set != 0, "CryptInitOIDFunctionSet failed: %08lx\n", GetLastError());
    if (set)
    {
        funcY funcAddr = NULL;
        HCRYPTOIDFUNCADDR hFuncAddr = NULL;

        /* This crashes
        ret = CryptGetOIDFunctionAddress(set, X509_ASN_ENCODING, 0, 0, NULL,
         NULL);
         */
        ret = CryptGetOIDFunctionAddress(set, X509_ASN_ENCODING, 0, 0,
         (void **)&funcAddr, &hFuncAddr);
        ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
         "Expected ERROR_FILE_NOT_FOUND, got %ld\n", GetLastError());
        ret = CryptGetOIDFunctionAddress(set, X509_ASN_ENCODING, X509_CERT, 0,
         (void **)&funcAddr, &hFuncAddr);
        ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
         "Expected ERROR_FILE_NOT_FOUND, got %ld\n", GetLastError());
        ret = CryptGetOIDFunctionAddress(set, 0, X509_CERT, 0,
         (void **)&funcAddr, &hFuncAddr);
        ok(ret, "CryptGetOIDFunctionAddress failed: %ld\n", GetLastError());
        if (funcAddr)
        {
            int y = funcAddr(0xabadc0da);

            ok(y == 0xabadc0da, "Unexpected return (%d) from function\n", y);
            CryptFreeOIDFunctionAddress(hFuncAddr, 0);
        }
    }
}

static void test_registerOIDFunction(void)
{
    static const WCHAR bogusDll[] = { 'b','o','g','u','s','.','d','l','l',0 };
    BOOL ret;

    /* oddly, this succeeds under WinXP; the function name key is merely
     * omitted.  This may be a side effect of the registry code, I don't know.
     * I don't check it because I doubt anyone would depend on it.
    ret = CryptRegisterOIDFunction(X509_ASN_ENCODING, NULL,
     "1.2.3.4.5.6.7.8.9.10", bogusDll, NULL);
     */
    /* On windows XP, GetLastError is incorrectly being set with an HRESULT,
     * HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER)
     */
    ret = CryptRegisterOIDFunction(X509_ASN_ENCODING, "foo", NULL, bogusDll,
     NULL);
    ok(!ret && GetLastError() == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER),
     "Expected ERROR_INVALID_PARAMETER: %ld\n", GetLastError());
    /* This has no effect, but "succeeds" on XP */
    ret = CryptRegisterOIDFunction(X509_ASN_ENCODING, "foo",
     "1.2.3.4.5.6.7.8.9.10", NULL, NULL);
    ok(ret, "Expected pseudo-success, got %ld\n", GetLastError());
    ret = CryptRegisterOIDFunction(X509_ASN_ENCODING, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10", bogusDll, NULL);
    ok(ret, "CryptRegisterOIDFunction failed: %ld\n", GetLastError());
    ret = CryptUnregisterOIDFunction(X509_ASN_ENCODING, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10");
    ok(ret, "CryptUnregisterOIDFunction failed: %ld\n", GetLastError());
    ret = CryptRegisterOIDFunction(X509_ASN_ENCODING, "bogus",
     "1.2.3.4.5.6.7.8.9.10", bogusDll, NULL);
    ok(ret, "CryptRegisterOIDFunction failed: %ld\n", GetLastError());
    ret = CryptUnregisterOIDFunction(X509_ASN_ENCODING, "bogus",
     "1.2.3.4.5.6.7.8.9.10");
    ok(ret, "CryptUnregisterOIDFunction failed: %ld\n", GetLastError());
    /* This has no effect */
    ret = CryptRegisterOIDFunction(PKCS_7_ASN_ENCODING, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10", bogusDll, NULL);
    ok(ret, "CryptRegisterOIDFunction failed: %ld\n", GetLastError());
    /* Check with bogus encoding type: */
    ret = CryptRegisterOIDFunction(0, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10", bogusDll, NULL);
    ok(ret, "CryptRegisterOIDFunction failed: %ld\n", GetLastError());
    /* This is written with value 3 verbatim.  Thus, the encoding type isn't
     * (for now) treated as a mask.
     */
    ret = CryptRegisterOIDFunction(3, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10", bogusDll, NULL);
    ok(ret, "CryptRegisterOIDFunction failed: %ld\n", GetLastError());
    ret = CryptUnregisterOIDFunction(3, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10");
    ok(ret, "CryptUnregisterOIDFunction failed: %ld\n", GetLastError());
}

START_TEST(oid)
{
    test_oidFunctionSet();
    test_installOIDFunctionAddress();
    test_registerOIDFunction();
}
