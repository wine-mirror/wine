/*
 * oleacc tests
 *
 * Copyright 2008 Nikolay Sivov
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

#include <oleacc.h>
#include "wine/test.h"

static void test_getroletext(void)
{
    INT ret, role;
    CHAR buf[2], *buff, buff2[100];
    WCHAR bufW[2], *buffW, buff2W[100];

    /* wrong role number */
    ret = GetRoleTextA(-1, NULL, 0);
    ok(ret == 0, "GetRoleTextA doesn't return zero on wrong role number, got %d\n", ret);
    buf[0] = '*';
    ret = GetRoleTextA(-1, buf, 2);
    ok(ret == 0, "GetRoleTextA doesn't return zero on wrong role number, got %d\n", ret);
    ok(buf[0] == '*', "GetRoleTextA modified buffer on wrong role number\n");
    buf[0] = '*';
    ret = GetRoleTextA(-1, buf, 0);
    ok(ret == 0, "GetRoleTextA doesn't return zero on wrong role number, got %d\n", ret);
    ok(buf[0] == '*', "GetRoleTextA modified buffer on wrong role number\n");

    ret = GetRoleTextW(-1, NULL, 0);
    ok(ret == 0, "GetRoleTextW doesn't return zero on wrong role number, got %d\n", ret);
    bufW[0] = '*';
    ret = GetRoleTextW(-1, bufW, 2);
    ok(ret == 0, "GetRoleTextW doesn't return zero on wrong role number, got %d\n", ret);
    ok(bufW[0] == '\0', "GetRoleTextW doesn't return NULL char on wrong role number\n");
    bufW[0] = '*';
    ret = GetRoleTextW(-1, bufW, 0);
    ok(ret == 0, "GetRoleTextW doesn't return zero on wrong role number, got %d\n", ret);
    /* don't know why this char */
    todo_wine ok(bufW[0] == 0x1e90, "GetRoleTextW returned wrong char, got %u\n", bufW[0]);

    /* zero role number - not documented */
    ret = GetRoleTextA(0, NULL, 0);
    ok(ret > 0, "GetRoleTextA doesn't return (>0) for zero role number, got %d\n", ret);
    ret = GetRoleTextW(0, NULL, 0);
    ok(ret > 0, "GetRoleTextW doesn't return (>0) for zero role number, got %d\n", ret);

    /* NULL buffer, return length */
    ret = GetRoleTextA(ROLE_SYSTEM_TITLEBAR, NULL, 0);
    ok(ret > 0, "GetRoleTextA doesn't return length on NULL buffer, got %d\n", ret);
    ret = GetRoleTextA(ROLE_SYSTEM_TITLEBAR, NULL, 1);
    ok(ret > 0, "GetRoleTextA doesn't return length on NULL buffer, got %d\n", ret);
    ret = GetRoleTextW(ROLE_SYSTEM_TITLEBAR, NULL, 0);
    ok(ret > 0, "GetRoleTextW doesn't return length on NULL buffer, got %d\n", ret);
    ret = GetRoleTextW(ROLE_SYSTEM_TITLEBAR, NULL, 1);
    ok(ret > 0, "GetRoleTextW doesn't return length on NULL buffer, got %d\n", ret);

    /* use a smaller buffer */
    buf[0] = '*';
    ret = GetRoleTextA(ROLE_SYSTEM_TITLEBAR, buf, 1);
    ok(ret == 0, "GetRoleTextA returned wrong length\n");
    ok(buf[0] == '\0', "GetRoleTextA returned not zero-length buffer\n");
    buf[1] = '*';
    ret = GetRoleTextA(ROLE_SYSTEM_TITLEBAR, buf, 2);
    ok(ret == 1, "GetRoleTextA returned wrong length, got %d, expected 1\n", ret);
    ok(buf[1] == '\0', "GetRoleTextA returned not zero-length buffer\n");

    bufW[0] = '*';
    ret = GetRoleTextW(ROLE_SYSTEM_TITLEBAR, bufW, 1);
    ok(ret == 0, "GetRoleTextW returned wrong length, got %d, expected 1\n", ret);
    ok(bufW[0] == '\0', "GetRoleTextW returned not zero-length buffer\n");
    bufW[1] = '*';
    ret = GetRoleTextW(ROLE_SYSTEM_TITLEBAR, bufW, 2);
    ok(ret == 1, "GetRoleTextW returned wrong length, got %d, expected 1\n", ret);
    ok(bufW[1] == '\0', "GetRoleTextW returned not zero-length buffer\n");

    /* use bigger buffer */
    ret = GetRoleTextA(ROLE_SYSTEM_TITLEBAR, NULL, 0);
    buff = HeapAlloc(GetProcessHeap(), 0, 2*ret);
    buff[2*ret-1] = '*';
    ret = GetRoleTextA(ROLE_SYSTEM_TITLEBAR, buff, 2*ret);
    ok(buff[2*ret-1] == '*', "GetRoleTextA shouldn't modify this part of buffer\n");
    HeapFree(GetProcessHeap(), 0, buff);

    ret = GetRoleTextW(ROLE_SYSTEM_TITLEBAR, NULL, 0);
    buffW = HeapAlloc(GetProcessHeap(), 0, 2*ret*sizeof(WCHAR));
    buffW[2*ret-1] = '*';
    ret = GetRoleTextW(ROLE_SYSTEM_TITLEBAR, buffW, 2*ret);
    ok(buffW[2*ret-1] == '*', "GetRoleTextW shouldn't modify this part of buffer\n");
    HeapFree(GetProcessHeap(), 0, buffW);

    /* check returned length for all roles */
    for(role = 0; role <= ROLE_SYSTEM_OUTLINEBUTTON; role++){
        ret = GetRoleTextA(role, NULL, 0);
        GetRoleTextA(role, buff2, sizeof(buff2));
        ok(ret == lstrlenA(buff2),
           "GetRoleTextA: returned length doesn't match returned buffer for role %d\n", role);

        ret = GetRoleTextW(role, NULL, 0);
        GetRoleTextW(role, buff2W, sizeof(buff2W)/sizeof(WCHAR));
        ok(ret == lstrlenW(buff2W),
           "GetRoleTextW: returned length doesn't match returned buffer for role %d\n", role);
    }
}

START_TEST(main)
{
    test_getroletext();
}
