/*
 * Copyright 2013 Jacek Caban for CodeWeavers
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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vfw.h>
#include <aviriff.h>
#include <stdio.h>

#include "wine/test.h"

static void test_encode(void)
{
    DWORD quality;
    ICINFO info;
    HIC hic;
    LRESULT res;

    hic = ICOpen(FCC('V','I','D','C'), FCC('m','r','l','e'), ICMODE_COMPRESS);
    ok(hic != NULL, "ICOpen failed\n");

    res = ICGetInfo(hic, &info, sizeof(info));
    ok(res == sizeof(info), "res = %ld\n", res);
    ok(info.dwSize == sizeof(info), "dwSize = %d\n", info.dwSize);
    ok(info.fccHandler == FCC('M','R','L','E'), "fccHandler = %x\n", info.fccHandler);
    todo_wine ok(info.dwFlags == (VIDCF_QUALITY|VIDCF_CRUNCH|VIDCF_TEMPORAL), "dwFlags = %x\n", info.dwFlags);
    ok(info.dwVersionICM == ICVERSION, "dwVersionICM = %d\n", info.dwVersionICM);

    quality = 0xdeadbeef;
    res = ICSendMessage(hic, ICM_GETDEFAULTQUALITY, (DWORD_PTR)&quality, 0);
    ok(res == ICERR_OK, "ICSendMessage(ICM_GETDEFAULTQUALITY) failed: %ld\n", res);
    ok(quality == 8500, "quality = %d\n", quality);

    quality = 0xdeadbeef;
    res = ICSendMessage(hic, ICM_GETQUALITY, (DWORD_PTR)&quality, 0);
    ok(res == ICERR_UNSUPPORTED, "ICSendMessage(ICM_GETQUALITY) failed: %ld\n", res);
    ok(quality == 0xdeadbeef, "quality = %d\n", quality);

    quality = ICQUALITY_HIGH;
    res = ICSendMessage(hic, ICM_SETQUALITY, (DWORD_PTR)&quality, 0);
    ok(res == ICERR_UNSUPPORTED, "ICSendMessage(ICM_SETQUALITY) failed: %ld\n", res);

    ICClose(hic);
}

START_TEST(msrle)
{
    test_encode();
}
