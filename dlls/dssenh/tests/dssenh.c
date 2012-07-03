/*
 * Unit tests for dss functions
 *
 * Copyright (c) 2012 Marek Kamil Chmiel
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

#include <string.h>
#include <stdio.h>
#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wincrypt.h"

static void test_acquire_context(void)
{   /* failure tests common between all four CSP providers */

    HCRYPTPROV hProv = 0;
    BOOL result;

    /* cannot acquire provider with 0 as Prov Type and NULL as CSP name */
    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(&hProv, NULL, NULL, 0, 0);
    ok(!result && GetLastError() == NTE_BAD_PROV_TYPE,
        "Expected NTE_BAD_PROV_TYPE, got %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(&hProv, NULL, NULL, 0, CRYPT_VERIFYCONTEXT);
    ok(!result && GetLastError() == NTE_BAD_PROV_TYPE,
        "Expected NTE_BAD_PROV_TYPE, got %08x\n", GetLastError());

    /* flag allows us to delete a keyset, but not of an unknown provider */
    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(&hProv, NULL, NULL, 0, CRYPT_DELETEKEYSET);
    ok(!result && GetLastError() == NTE_BAD_PROV_TYPE,
        "Expected NTE_BAD_PROV_TYPE, got %08x\n", GetLastError());

    /* cannot acquire along with PROV_RSA_SIG, not compatible */
    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(&hProv, NULL, MS_DEF_DSS_PROV_A, PROV_RSA_SIG, 0);
    todo_wine
    ok(!result && GetLastError() == NTE_PROV_TYPE_NO_MATCH,
        "Expected NTE_PROV_TYPE_NO_MATCH, got %08x\n", GetLastError());

    /* cannot acquire along with MS_DEF_RSA_SIG_PROV, not compatible */
    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(&hProv, NULL, MS_DEF_RSA_SIG_PROV, PROV_DSS, 0);
    ok(!result && GetLastError() == NTE_KEYSET_NOT_DEF,
        "Expected NTE_KEYSET_NOT_DEF, got %08x\n", GetLastError());

    /* cannot acquire provider with 0 as Prov Type */
    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(&hProv, NULL, MS_DEF_DSS_PROV_A, 0, 0);
    ok(!result && GetLastError() == NTE_BAD_PROV_TYPE,
        "Expected NTE_BAD_PROV_TYPE, got %08x\n", GetLastError());

    /* test base DSS provider (PROV_DSS) */

    result = CryptAcquireContextA(
        &hProv, NULL, MS_DEF_DSS_PROV_A, PROV_DSS, CRYPT_VERIFYCONTEXT);
    if(!result)
    {
        skip("DSS csp is currently not available, skipping tests.\n");
        return;
    }
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    result = CryptAcquireContextA(
        &hProv, NULL, MS_DEF_DSS_DH_PROV_A, PROV_DSS_DH, CRYPT_NEWKEYSET);
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    result = CryptAcquireContextA(&hProv, NULL, NULL, PROV_DSS, 0);
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    result = CryptAcquireContextA(&hProv, NULL, MS_DEF_DSS_PROV_A, PROV_DSS, 0);
    ok(result, "Expected no errors.\n");

    /* test DSS Diffie Hellman provider (PROV_DSS_DH) */

    result = CryptAcquireContextA(
        &hProv, NULL, MS_DEF_DSS_DH_PROV, PROV_DSS_DH, CRYPT_VERIFYCONTEXT);
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    result = CryptAcquireContextA(&hProv, NULL, NULL, PROV_DSS_DH, 0);
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    result = CryptAcquireContextA(&hProv, NULL, MS_DEF_DSS_DH_PROV, PROV_DSS_DH, 0);
    ok(result, "Expected no errors.\n");

    /* test DSS Enhanced provider (MS_ENH_DSS_DH_PROV) */

    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(
        &hProv, NULL, MS_ENH_DSS_DH_PROV, PROV_DSS_DH, CRYPT_VERIFYCONTEXT);
    if(!result && GetLastError() == NTE_KEYSET_NOT_DEF)
    {
        win_skip("DSSENH and Schannel provider is broken on WinNT4\n");
        return;
    }
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    result = CryptAcquireContextA(&hProv, NULL, MS_ENH_DSS_DH_PROV, PROV_DSS_DH, 0);
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    /* test DSS Schannel provider (PROV_DH_SCHANNEL) */

    result = CryptAcquireContextA(
        &hProv, NULL, MS_DEF_DH_SCHANNEL_PROV, PROV_DH_SCHANNEL, CRYPT_VERIFYCONTEXT);
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    result = CryptAcquireContextA(&hProv, NULL, NULL, PROV_DH_SCHANNEL, 0);
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    result = CryptAcquireContextA(
        &hProv, NULL, MS_DEF_DH_SCHANNEL_PROV, PROV_DH_SCHANNEL, 0);
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    /* failure tests, cannot acquire context because the key container already exists */
    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(
        &hProv, NULL, MS_DEF_DSS_DH_PROV, PROV_DSS_DH, CRYPT_NEWKEYSET);
    ok(!result && GetLastError() == NTE_EXISTS,
        "Expected NTE_EXISTS, got %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(
        &hProv, NULL, MS_ENH_DSS_DH_PROV, PROV_DSS_DH, CRYPT_NEWKEYSET);
    ok(!result && GetLastError() == NTE_EXISTS,
        "Expected NTE_EXISTS, got %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(
        &hProv, NULL, MS_DEF_DH_SCHANNEL_PROV, PROV_DH_SCHANNEL, CRYPT_NEWKEYSET);
    ok(!result && GetLastError() == NTE_EXISTS,
        "Expected NTE_EXISTS, got %08x\n", GetLastError());
}

START_TEST(dssenh)
{
    test_acquire_context();
}
