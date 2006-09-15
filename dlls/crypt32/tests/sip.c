/*
 * Subject Interface Package tests
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

#include <stdio.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#include <wincrypt.h>
#include <mssip.h>

#include "wine/test.h"

static void test_AddRemoveProvider(void)
{
    BOOL ret;
    SIP_ADD_NEWPROVIDER newprov;
    GUID actionid = { 0xdeadbe, 0xefde, 0xadbe, { 0xef,0xde,0xad,0xbe,0xef,0xde,0xad,0xbe }};
    static WCHAR dummydll[]      = {'d','e','a','d','b','e','e','f','.','d','l','l',0 };
    static WCHAR dummyfunction[] = {'d','u','m','m','y','f','u','n','c','t','i','o','n',0 };

    /* NULL check */
    SetLastError(0xdeadbeef);
    ret = CryptSIPRemoveProvider(NULL);
    ok (!ret, "Expected CryptSIPRemoveProvider to fail.\n");
    ok (GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %ld.\n", GetLastError());

    /* nonexistent provider should result in a registry error */
    SetLastError(0xdeadbeef);
    ret = CryptSIPRemoveProvider(&actionid);
    ok (!ret, "Expected CryptSIPRemoveProvider to fail.\n");
    ok (GetLastError() == ERROR_FILE_NOT_FOUND,
        "Expected ERROR_FILE_NOT_FOUND, got %ld.\n", GetLastError());

    /* Everything OK, pwszIsFunctionName and pwszIsFunctionNameFmt2 are left NULL
     * as allowed */

    memset(&newprov, 0, sizeof(SIP_ADD_NEWPROVIDER));
    newprov.cbStruct = sizeof(SIP_ADD_NEWPROVIDER);
    newprov.pgSubject = &actionid;
    newprov.pwszDLLFileName = dummydll;
    newprov.pwszGetFuncName = dummyfunction;
    newprov.pwszPutFuncName = dummyfunction;
    newprov.pwszCreateFuncName = dummyfunction;
    newprov.pwszVerifyFuncName = dummyfunction;
    newprov.pwszRemoveFuncName = dummyfunction;
    SetLastError(0xdeadbeef);
    ret = CryptSIPAddProvider(&newprov);
    ok ( ret, "CryptSIPAddProvider should have succeeded\n");
    ok ( GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n",
     GetLastError());

    /* Dummy provider will be deleted, but the function still fails because
     * pwszIsFunctionName and pwszIsFunctionNameFmt2 are not present in the
     * registry.
     */
    SetLastError(0xdeadbeef);
    ret = CryptSIPRemoveProvider(&actionid);
    ok (!ret, "Expected CryptSIPRemoveProvider to fail.\n");
    ok (GetLastError() == ERROR_FILE_NOT_FOUND,
        "Expected ERROR_FILE_NOT_FOUND, got %ld.\n", GetLastError());

    /* Everything OK */
    memset(&newprov, 0, sizeof(SIP_ADD_NEWPROVIDER));
    newprov.cbStruct = sizeof(SIP_ADD_NEWPROVIDER);
    newprov.pgSubject = &actionid;
    newprov.pwszDLLFileName = dummydll;
    newprov.pwszGetFuncName = dummyfunction;
    newprov.pwszPutFuncName = dummyfunction;
    newprov.pwszCreateFuncName = dummyfunction;
    newprov.pwszVerifyFuncName = dummyfunction;
    newprov.pwszRemoveFuncName = dummyfunction;
    newprov.pwszIsFunctionNameFmt2 = dummyfunction;
    newprov.pwszIsFunctionName = dummyfunction;
    SetLastError(0xdeadbeef);
    ret = CryptSIPAddProvider(&newprov);
    ok ( ret, "CryptSIPAddProvider should have succeeded\n");
    ok ( GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n",
     GetLastError());

    /* Dummy provider should be deleted */
    SetLastError(0xdeadbeef);
    ret = CryptSIPRemoveProvider(&actionid);
    ok ( ret, "CryptSIPRemoveProvider should have succeeded\n");
    ok ( GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n",
     GetLastError());
}

static void test_SIPLoad(void)
{
    BOOL ret;
    GUID subject;
    static GUID dummySubject = { 0xdeadbeef, 0xdead, 0xbeef, { 0xde,0xad,0xbe,0xef,0xde,0xad,0xbe,0xef }};
    static GUID unknown      = { 0xC689AABA, 0x8E78, 0x11D0, { 0x8C,0x47,0x00,0xC0,0x4F,0xC2,0x95,0xEE }};
    SIP_DISPATCH_INFO sdi;

    /* All NULL */
    SetLastError(0xdeadbeef);
    ret = CryptSIPLoad(NULL, 0, NULL);
    ok ( !ret, "Expected CryptSIPLoad to fail\n");
    todo_wine
        ok ( GetLastError() == ERROR_INVALID_PARAMETER,
            "Expected ERROR_INVALID_PARAMETER, got 0x%08lx\n", GetLastError());

    /* Only pSipDispatch NULL */
    SetLastError(0xdeadbeef);
    ret = CryptSIPLoad(&subject, 0, NULL);
    ok ( !ret, "Expected CryptSIPLoad to fail\n");
    todo_wine
        ok ( GetLastError() == ERROR_INVALID_PARAMETER,
            "Expected ERROR_INVALID_PARAMETER, got 0x%08lx\n", GetLastError());

    /* No NULLs, but nonexistent pgSubject */
    SetLastError(0xdeadbeef);
    memset(&sdi, 0, sizeof(SIP_DISPATCH_INFO));
    sdi.cbSize = sizeof(SIP_DISPATCH_INFO);
    ret = CryptSIPLoad(&dummySubject, 0, &sdi);
    ok ( !ret, "Expected CryptSIPLoad to fail\n");
    todo_wine
        ok ( GetLastError() == TRUST_E_SUBJECT_FORM_UNKNOWN,
            "Expected TRUST_E_SUBJECT_FORM_UNKNOWN, got 0x%08lx\n", GetLastError());

    /* cbSize not initialized */
    SetLastError(0xdeadbeef);
    memset(&sdi, 0, sizeof(SIP_DISPATCH_INFO));
    ret = CryptSIPLoad(&dummySubject, 0, &sdi);
    ok ( !ret, "Expected CryptSIPLoad to fail\n");
    todo_wine
        ok ( GetLastError() == TRUST_E_SUBJECT_FORM_UNKNOWN,
            "Expected TRUST_E_SUBJECT_FORM_UNKNOWN, got 0x%08lx\n", GetLastError());

    /* cbSize not initialized, but valid subject (named unknown but registered by wintrust) */
    SetLastError(0xdeadbeef);
    memset(&sdi, 0, sizeof(SIP_DISPATCH_INFO));
    ret = CryptSIPLoad(&unknown, 0, &sdi);
    todo_wine
    {
        ok ( ret, "Expected CryptSIPLoad to succeed\n");
        ok ( GetLastError() == ERROR_PROC_NOT_FOUND,
            "Expected ERROR_PROC_NOT_FOUND, got 0x%08lx\n", GetLastError());
    }

    /* All OK */
    SetLastError(0xdeadbeef);
    memset(&sdi, 0, sizeof(SIP_DISPATCH_INFO));
    sdi.cbSize = sizeof(SIP_DISPATCH_INFO);
    ret = CryptSIPLoad(&unknown, 0, &sdi);
    todo_wine
        ok ( ret, "Expected CryptSIPLoad to succeed\n");
    ok ( GetLastError() == 0xdeadbeef,
        "Expected 0xdeadbeef, got 0x%08lx\n", GetLastError());

    /* Reserved parameter not 0 */
    SetLastError(0xdeadbeef);
    memset(&sdi, 0, sizeof(SIP_DISPATCH_INFO));
    sdi.cbSize = sizeof(SIP_DISPATCH_INFO);
    ret = CryptSIPLoad(&unknown, 1, &sdi);
    ok ( !ret, "Expected CryptSIPLoad to fail\n");
    todo_wine
        ok ( GetLastError() == ERROR_INVALID_PARAMETER,
            "Expected ERROR_INVALID_PARAMETER, got 0x%08lx\n", GetLastError());
}

START_TEST(sip)
{
    test_AddRemoveProvider();
    test_SIPLoad();
}
