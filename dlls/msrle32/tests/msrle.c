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

#include "wine/test.h"

static void test_encode(void)
{
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

    ICClose(hic);
}

START_TEST(msrle)
{
    test_encode();
}
