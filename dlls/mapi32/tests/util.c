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
static void  (WINAPI *pHexFromBin)(LPBYTE,int,LPWSTR);
static void  (WINAPI *pFBinFromHex)(LPWSTR,LPBYTE);

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

static void test_HexFromBin(void)
{
    static const char res[] = { "000102030405060708090A0B0C0D0E0F101112131415161"
      "718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F303132333435363738393A3B"
      "3C3D3E3F404142434445464748494A4B4C4D4E4F505152535455565758595A5B5C5D5E5F6"
      "06162636465666768696A6B6C6D6E6F707172737475767778797A7B7C7D7E7F8081828384"
      "85868788898A8B8C8D8E8F909192939495969798999A9B9C9D9E9FA0A1A2A3A4A5A6A7A8A"
      "9AAABACADAEAFB0B1B2B3B4B5B6B7B8B9BABBBCBDBEBFC0C1C2C3C4C5C6C7C8C9CACBCCCD"
      "CECFD0D1D2D3D4D5D6D7D8D9DADBDCDDDEDFE0E1E2E3E4E5E6E7E8E9EAEBECEDEEEFF0F1F"
      "2F3F4F5F6F7F8F9FAFBFCFDFE\0X" };
    BYTE data[255];
    WCHAR strw[256];
    BOOL bOk;
    int i;

    pHexFromBin = (void*)GetProcAddress(hMapi32, "HexFromBin@12");
    pFBinFromHex = (void*)GetProcAddress(hMapi32, "FBinFromHex@8");
    if (!pHexFromBin || !pFBinFromHex)
        return;

    for (i = 0; i < 255; i++)
        data[i] = i;
    memset(strw, 'X', sizeof(strw));
    pHexFromBin(data, sizeof(data), strw);

    ok(memcmp(strw, res, sizeof(res) - 1) == 0, "HexFromBin: Result differs\n");

    memset(data, 0, sizeof(data));
    pFBinFromHex((LPWSTR)res, data);
    bOk = TRUE;
    for (i = 0; i < 255; i++)
        if (data[i] != i)
            bOk = FALSE;
    ok(bOk == TRUE, "FBinFromHex: Result differs\n");
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
    test_HexFromBin();
}
