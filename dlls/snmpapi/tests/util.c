/*
 * Copyright 2007 Hans Leidekker
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

#include <wine/test.h>

#include <windef.h>
#include <snmp.h>

static void test_SnmpUtilOidToA(void)
{
    LPSTR ret;
    static UINT ids1[] = { 1,3,6,1,4,1,311 };
    static UINT ids2[] = {
          1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
          1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
          1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
          1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
          1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
          1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
          1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
          1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
          1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };
    static UINT ids3[] = { 0xffffffff };
    static AsnObjectIdentifier oid0 = { 0, ids1 };
    static AsnObjectIdentifier oid1 = { 7, ids1 };
    static AsnObjectIdentifier oid2 = { 256, ids2 };
    static AsnObjectIdentifier oid3 = { 257, ids2 };
    static AsnObjectIdentifier oid4 = { 258, ids2 };
    static AsnObjectIdentifier oid5 = { 1, ids3 };
    static const char expect0[] = "<null oid>";
    static const char expect1[] = "1.3.6.1.4.1.311";
    static const char expect2[] =
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1";
    static const char expect3[] =
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1."
        "1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1.1";
    static const char expect4[] = "-1";

    ret = SnmpUtilOidToA(NULL);
    ok(ret != NULL, "SnmpUtilOidToA failed\n");
    ok(!strcmp(ret, expect0), "SnmpUtilOidToA failed got \n%s\n expected \n%s\n",
       ret, expect1);

    ret = SnmpUtilOidToA(&oid0);
    ok(ret != NULL, "SnmpUtilOidToA failed\n");
    ok(!strcmp(ret, expect0), "SnmpUtilOidToA failed got \n%s\n expected \n%s\n",
       ret, expect0);

    ret = SnmpUtilOidToA(&oid1);
    ok(ret != NULL, "SnmpUtilOidToA failed\n");
    ok(!strcmp(ret, expect1), "SnmpUtilOidToA failed got \n%s\n expected \n%s\n",
       ret, expect1);

    ret = SnmpUtilOidToA(&oid2);
    ok(ret != NULL, "SnmpUtilOidToA failed\n");
    ok(!strcmp(ret, expect2), "SnmpUtilOidToA failed got \n%s\n expected \n%s\n %d\n",
       ret, expect2, strlen(ret));

    ret = SnmpUtilOidToA(&oid3);
    ok(ret != NULL, "SnmpUtilOidToA failed\n");
    ok(!strcmp(ret, expect3), "SnmpUtilOidToA failed got \n%s\n expected \n%s\n %d\n",
       ret, expect3, strlen(ret));

    ret = SnmpUtilOidToA(&oid4);
    ok(ret != NULL, "SnmpUtilOidToA failed\n");
    ok(!strcmp(ret, expect3), "SnmpUtilOidToA failed got \n%s\n expected \n%s\n %d\n",
       ret, expect3, strlen(ret));

    ret = SnmpUtilOidToA(&oid5);
    ok(ret != NULL, "SnmpUtilOidToA failed\n");
    ok(!strcmp(ret, expect4), "SnmpUtilOidToA failed got \n%s\n expected \n%s\n %d\n",
       ret, expect4, strlen(ret));
}

static void test_SnmpUtilAsnAnyCpyFree(void)
{
    INT ret;
    static AsnAny dst, src = { ASN_INTEGER, { 1 } };

    if (0) { /* these crash on XP */
    ret = SnmpUtilAsnAnyCpy(NULL, NULL);
    ok(!ret, "SnmpUtilAsnAnyCpy succeeded\n");

    ret = SnmpUtilAsnAnyCpy(&dst, NULL);
    ok(!ret, "SnmpUtilAsnAnyCpy succeeded\n");

    ret = SnmpUtilAsnAnyCpy(NULL, &src);
    ok(!ret, "SnmpUtilAsnAnyCpy succeeded\n");
    }

    ret = SnmpUtilAsnAnyCpy(&dst, &src);
    ok(ret, "SnmpUtilAsnAnyCpy failed\n");
    ok(!memcmp(&src, &dst, sizeof(AsnAny)), "SnmpUtilAsnAnyCpy failed\n");

    if (0) { /* crashes on XP */
    SnmpUtilAsnAnyFree(NULL);
    }
    SnmpUtilAsnAnyFree(&dst);
    ok(dst.asnType == ASN_NULL, "SnmpUtilAsnAnyFree failed\n");
    ok(dst.asnValue.number == 1, "SnmpUtilAsnAnyFree failed\n");
}

static void test_SnmpUtilOctetsCpyFree(void)
{
    INT ret;
    static BYTE stream[] = { '1', '2', '3', '4' };
    static AsnOctetString dst, src = { stream, 4, TRUE };

    ret = SnmpUtilOctetsCpy(NULL, NULL);
    ok(!ret, "SnmpUtilOctetsCpy succeeded\n");

    memset(&dst, 1, sizeof(AsnOctetString));
    ret = SnmpUtilOctetsCpy(&dst, NULL);
    ok(ret, "SnmpUtilOctetsCpy failed\n");
    ok(dst.length == 0, "SnmpUtilOctetsCpy failed\n");
    ok(dst.stream == NULL, "SnmpUtilOctetsCpy failed\n");
    ok(dst.dynamic == FALSE, "SnmpUtilOctetsCpy failed\n");

    ret = SnmpUtilOctetsCpy(NULL, &src);
    ok(!ret, "SnmpUtilOctetsCpy succeeded\n");

    memset(&dst, 0, sizeof(AsnOctetString));
    ret = SnmpUtilOctetsCpy(&dst, &src);
    ok(ret, "SnmpUtilOctetsCpy failed\n");
    ok(src.length == dst.length, "SnmpUtilOctetsCpy failed\n");
    ok(!memcmp(src.stream, dst.stream, dst.length), "SnmpUtilOctetsCpy failed\n");
    ok(dst.dynamic == TRUE, "SnmpUtilOctetsCpy failed\n");

    SnmpUtilOctetsFree(NULL);
    SnmpUtilOctetsFree(&dst);
    ok(dst.stream == NULL, "SnmpUtilOctetsFree failed\n");
    ok(dst.length == 0, "SnmpUtilOctetsFree failed\n");
    ok(dst.dynamic == FALSE, "SnmpUtilOctetsFree failed\n");
}

static void test_SnmpUtilOidCpyFree(void)
{
    INT ret;
    static UINT ids[] = { 1, 3, 6, 1, 4, 1, 311 };
    static AsnObjectIdentifier dst, src = { sizeof(ids) / sizeof(ids[0]), ids };

    ret = SnmpUtilOidCpy(NULL, NULL);
    ok(!ret, "SnmpUtilOidCpy succeeded\n");

    memset(&dst, 1, sizeof(AsnObjectIdentifier));
    ret = SnmpUtilOidCpy(&dst, NULL);
    ok(ret, "SnmpUtilOidCpy failed\n");
    ok(dst.idLength == 0, "SnmpUtilOidCpy failed\n");
    ok(dst.ids == NULL, "SnmpUtilOidCpy failed\n");

    ret = SnmpUtilOidCpy(NULL, &src);
    ok(!ret, "SnmpUtilOidCpy succeeded\n");

    memset(&dst, 0, sizeof(AsnObjectIdentifier));
    ret = SnmpUtilOidCpy(&dst, &src);
    ok(ret, "SnmpUtilOidCpy failed\n");
    ok(src.idLength == dst.idLength, "SnmpUtilOidCpy failed\n");
    ok(!memcmp(src.ids, dst.ids, dst.idLength * sizeof(UINT)), "SnmpUtilOidCpy failed\n");

    SnmpUtilOidFree(NULL);
    SnmpUtilOidFree(&dst);
    ok(dst.idLength == 0, "SnmpUtilOidFree failed\n");
    ok(dst.ids == NULL, "SnmpUtilOidFree failed\n");
}

static void test_SnmpUtilOctetsNCmp(void)
{
    INT ret;
    static BYTE stream1[] = { '1', '2', '3', '4' };
    static BYTE stream2[] = { '5', '6', '7', '8' };
    static AsnOctetString octets1 = { stream1, 4, FALSE };
    static AsnOctetString octets2 = { stream2, 4, FALSE };

    ret = SnmpUtilOctetsNCmp(NULL, NULL, 0);
    ok(!ret, "SnmpUtilOctetsNCmp succeeded\n");

    ret = SnmpUtilOctetsNCmp(NULL, NULL, 1);
    ok(!ret, "SnmpUtilOctetsNCmp succeeded\n");

    ret = SnmpUtilOctetsNCmp(&octets1, NULL, 0);
    ok(!ret, "SnmpUtilOctetsNCmp succeeded\n");

    ret = SnmpUtilOctetsNCmp(&octets1, NULL, 1);
    ok(!ret, "SnmpUtilOctetsNCmp succeeded\n");

    ret = SnmpUtilOctetsNCmp(NULL, &octets2, 0);
    ok(!ret, "SnmpUtilOctetsNCmp succeeded\n");

    ret = SnmpUtilOctetsNCmp(NULL, &octets2, 1);
    ok(!ret, "SnmpUtilOctetsNCmp succeeded\n");

    ret = SnmpUtilOctetsNCmp(&octets1, &octets1, 0);
    ok(!ret, "SnmpUtilOctetsNCmp failed\n");

    ret = SnmpUtilOctetsNCmp(&octets1, &octets1, 4);
    ok(!ret, "SnmpUtilOctetsNCmp failed\n");

    ret = SnmpUtilOctetsNCmp(&octets1, &octets2, 4);
    ok(ret == -4, "SnmpUtilOctetsNCmp failed\n");

    ret = SnmpUtilOctetsNCmp(&octets2, &octets1, 4);
    ok(ret == 4, "SnmpUtilOctetsNCmp failed\n");
}

static void test_SnmpUtilOctetsCmp(void)
{
    INT ret;
    static BYTE stream1[] = { '1', '2', '3' };
    static BYTE stream2[] = { '1', '2', '3', '4' };
    static AsnOctetString octets1 = { stream1, 3, FALSE };
    static AsnOctetString octets2 = { stream2, 4, FALSE };

    if (0) { /* these crash on XP */
    ret = SnmpUtilOctetsCmp(NULL, NULL);
    ok(!ret, "SnmpUtilOctetsCmp succeeded\n");

    ret = SnmpUtilOctetsCmp(&octets1, NULL);
    ok(!ret, "SnmpUtilOctetsCmp succeeded\n");

    ret = SnmpUtilOctetsCmp(NULL, &octets2);
    ok(!ret, "SnmpUtilOctetsCmp succeeded\n");
    }

    ret = SnmpUtilOctetsCmp(&octets2, &octets1);
    ok(ret == 1, "SnmpUtilOctetsCmp failed\n");

    ret = SnmpUtilOctetsCmp(&octets1, &octets2);
    ok(ret == -1, "SnmpUtilOctetsCmp failed\n");
}

static void test_SnmpUtilOidNCmp(void)
{
    INT ret;
    static UINT ids1[] = { 1, 2, 3, 4 };
    static UINT ids2[] = { 5, 6, 7, 8 };
    static AsnObjectIdentifier oid1 = { 4, ids1 };
    static AsnObjectIdentifier oid2 = { 4, ids2 };

    ret = SnmpUtilOidNCmp(NULL, NULL, 0);
    ok(!ret, "SnmpUtilOidNCmp succeeded\n");

    ret = SnmpUtilOidNCmp(NULL, NULL, 1);
    ok(!ret, "SnmpUtilOidNCmp succeeded\n");

    ret = SnmpUtilOidNCmp(&oid1, NULL, 0);
    ok(!ret, "SnmpUtilOidNCmp succeeded\n");

    ret = SnmpUtilOidNCmp(&oid1, NULL, 1);
    ok(!ret, "SnmpUtilOidNCmp succeeded\n");

    ret = SnmpUtilOidNCmp(NULL, &oid2, 0);
    ok(!ret, "SnmpUtilOidNCmp succeeded\n");

    ret = SnmpUtilOidNCmp(NULL, &oid2, 1);
    ok(!ret, "SnmpUtilOidNCmp succeeded\n");

    ret = SnmpUtilOidNCmp(&oid1, &oid1, 0);
    ok(!ret, "SnmpUtilOidNCmp failed\n");

    ret = SnmpUtilOidNCmp(&oid1, &oid1, 4);
    ok(!ret, "SnmpUtilOidNCmp failed\n");

    ret = SnmpUtilOidNCmp(&oid1, &oid2, 4);
    ok(ret == -1, "SnmpUtilOidNCmp failed: %d\n", ret);

    ret = SnmpUtilOidNCmp(&oid2, &oid1, 4);
    ok(ret == 1, "SnmpUtilOidNCmp failed: %d\n", ret);
}

static void test_SnmpUtilOidCmp(void)
{
    INT ret;
    static UINT ids1[] = { 1, 2, 3 };
    static UINT ids2[] = { 1, 2, 3, 4 };
    static AsnObjectIdentifier oid1 = { 3, ids1 };
    static AsnObjectIdentifier oid2 = { 4, ids2 };

    if (0) { /* these crash on XP */
    ret = SnmpUtilOidCmp(NULL, NULL);
    ok(!ret, "SnmpUtilOidCmp succeeded\n");

    ret = SnmpUtilOidCmp(&oid1, NULL);
    ok(!ret, "SnmpUtilOidCmp succeeded\n");

    ret = SnmpUtilOidCmp(NULL, &oid2);
    ok(!ret, "SnmpUtilOidCmp succeeded\n");
    }

    ret = SnmpUtilOidCmp(&oid2, &oid1);
    ok(ret == 1, "SnmpUtilOidCmp failed\n");

    ret = SnmpUtilOidCmp(&oid1, &oid2);
    ok(ret == -1, "SnmpUtilOidCmp failed\n");
}

static void test_SnmpUtilOidAppend(void)
{
    INT ret;
    static UINT ids1[] = { 1, 2, 3 };
    static UINT ids2[] = { 4, 5, 6 };
    static AsnObjectIdentifier oid1 = { 3, ids1 };
    static AsnObjectIdentifier oid2 = { 3, ids2 };

    ret = SnmpUtilOidAppend(NULL, NULL);
    ok(!ret, "SnmpUtilOidAppend succeeded\n");

    ret = SnmpUtilOidAppend(&oid1, NULL);
    ok(ret, "SnmpUtilOidAppend failed\n");

    ret = SnmpUtilOidAppend(NULL, &oid2);
    ok(!ret, "SnmpUtilOidAppend succeeded\n");

    ret = SnmpUtilOidAppend(&oid1, &oid2);
    ok(ret, "SnmpUtilOidAppend failed\n");
    ok(oid1.idLength == 6, "SnmpUtilOidAppend failed\n");
    ok(!memcmp(&oid1.ids[3], ids2, 3 * sizeof(UINT)),
       "SnmpUtilOidAppend failed\n");
}

START_TEST(util)
{
    test_SnmpUtilOidToA();
    test_SnmpUtilAsnAnyCpyFree();
    test_SnmpUtilOctetsCpyFree();
    test_SnmpUtilOidCpyFree();
    test_SnmpUtilOctetsNCmp();
    test_SnmpUtilOctetsCmp();
    test_SnmpUtilOidCmp();
    test_SnmpUtilOidNCmp();
    test_SnmpUtilOidAppend();
}
