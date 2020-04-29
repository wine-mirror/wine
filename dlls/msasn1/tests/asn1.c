/*
 * Copyright 2020 Vijay Kiran Kamuju
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
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "msasn1.h"
#include "wine/test.h"

static void test_CreateModule(void)
{
    const ASN1GenericFun_t encfn[] = { NULL };
    const ASN1GenericFun_t decfn[] = { NULL };
    const ASN1FreeFun_t freefn[] = { NULL };
    const ASN1uint32_t size[] = { 0 };
    ASN1magic_t name = 0x61736e31;
    ASN1module_t mod;

    mod = ASN1_CreateModule(0, 0, 0, 0, NULL, NULL, NULL, NULL, 0);
    ok(!mod, "Expected Failure.\n");

    mod = ASN1_CreateModule(0, 0, 0, 0, encfn, NULL, NULL, NULL, 0);
    ok(!mod, "Expected Failure.\n");

    mod = ASN1_CreateModule(0, 0, 0, 0, encfn, decfn, NULL, NULL, 0);
    ok(!mod, "Expected Failure.\n");

    mod = ASN1_CreateModule(0, 0, 0, 0, encfn, decfn, freefn, NULL, 0);
    ok(!mod, "Expected Failure.\n");

    mod = ASN1_CreateModule(0, 0, 0, 0, encfn, decfn, freefn, size, 0);
    todo_wine ok(!!mod, "Failed to create module.\n");

    mod = ASN1_CreateModule(ASN1_THIS_VERSION, ASN1_BER_RULE_DER, ASN1FLAGS_NOASSERT, 1, encfn, decfn, freefn, size, name);
    todo_wine ok(!!mod, "Failed to create module.\n");
}

START_TEST(asn1)
{
    test_CreateModule();
}
