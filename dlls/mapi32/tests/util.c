/*
 * Unit test suite for MAPI utility functions
 *
 * Copyright 2004 Jon Griffiths
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "winnt.h"
#include "mapiutil.h"
#include "mapitags.h"

static HMODULE hMapi32 = 0;

static SCODE (WINAPI *pScInitMapiUtil)(ULONG);
static void  (WINAPI *pSwapPword)(PUSHORT,ULONG);
static void  (WINAPI *pSwapPlong)(PULONG,ULONG);

static void test_SwapPword(void)
{
    USHORT shorts[3];

    pSwapPword = (void*)GetProcAddress(hMapi32, "SwapPword@8");
    if (!pSwapPword)
        return;

    shorts[0] = 0xff01;
    shorts[1] = 0x10ff;
    shorts[2] = 0x2001;
    pSwapPword(shorts, 2);
    ok(shorts[0] == 0x01ff && shorts[1] == 0xff10 && shorts[2] == 0x2001,
       "Expected {0x01ff,0xff10,0x2001}, got {0x%04x,0x%04x,0x%04x}\n",
       shorts[0], shorts[1], shorts[2]);
}

static void test_SwapPlong(void)
{
    ULONG longs[3];

    pSwapPlong = (void*)GetProcAddress(hMapi32, "SwapPlong@8");
    if (!pSwapPlong)
        return;

    longs[0] = 0xffff0001;
    longs[1] = 0x1000ffff;
    longs[2] = 0x20000001;
    pSwapPlong(longs, 2);
    ok(longs[0] == 0x0100ffff && longs[1] == 0xffff0010 && longs[2] == 0x20000001,
       "Expected {0x0100ffff,0xffff0010,0x20000001}, got {0x%08lx,0x%08lx,0x%08lx}\n",
       longs[0], longs[1], longs[2]);
}

START_TEST(util)
{
    hMapi32 = LoadLibraryA("mapi32.dll");
    
    pScInitMapiUtil = (void*)GetProcAddress(hMapi32, "ScInitMapiUtil@4");
    if (!pScInitMapiUtil)
        return;
    pScInitMapiUtil(0);

    test_SwapPword();
    test_SwapPlong();
}
