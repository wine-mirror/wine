/*
 * Unit test suite for AVI Functions
 *
 * Copyright 2008 Detlef Riekenberg
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
 *
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "vfw.h"
#include "wine/test.h"

/* ########################### */

static const CHAR winetest0[] = "winetest0";
static const CHAR winetest1[] = "winetest1";

/* ########################### */

static void test_AVISaveOptions(void)
{
    AVICOMPRESSOPTIONS options[2];
    LPAVICOMPRESSOPTIONS poptions[2];
    PAVISTREAM streams[2] = {NULL, NULL};
    HRESULT hres;
    DWORD   res;
    LONG    lres;

    poptions[0] = &options[0];
    poptions[1] = &options[1];
    ZeroMemory(options, sizeof(options));

    SetLastError(0xdeadbeef);
    hres = CreateEditableStream(&streams[0], NULL);
    ok(hres == AVIERR_OK, "0: got 0x%x and %p (expected AVIERR_OK)\n", hres, streams[0]);

    SetLastError(0xdeadbeef);
    hres = CreateEditableStream(&streams[1], NULL);
    ok(hres == AVIERR_OK, "1: got 0x%x and %p (expected AVIERR_OK)\n", hres, streams[1]);

    SetLastError(0xdeadbeef);
    hres = EditStreamSetNameA(streams[0], winetest0);
    todo_wine ok(hres == AVIERR_OK, "0: got 0x%x (expected AVIERR_OK)\n", hres);

    SetLastError(0xdeadbeef);
    hres = EditStreamSetNameA(streams[1], winetest1);
    todo_wine ok(hres == AVIERR_OK, "1: got 0x%x (expected AVIERR_OK)\n", hres);

    if (winetest_interactive) {
        SetLastError(0xdeadbeef);
        res = AVISaveOptions(0, ICMF_CHOOSE_DATARATE |ICMF_CHOOSE_KEYFRAME | ICMF_CHOOSE_ALLCOMPRESSORS,
                             2, streams, poptions);
        trace("got %u with 0x%x/%u\n", res, GetLastError(), GetLastError());
    }

    SetLastError(0xdeadbeef);
    lres = AVISaveOptionsFree(2, poptions);
    ok(lres == AVIERR_OK, "got 0x%x with 0x%x/%u\n", lres, GetLastError(), GetLastError());

    SetLastError(0xdeadbeef);
    res = AVIStreamRelease(streams[0]);
    ok(res == 0, "0: got refcount %u (expected 0)\n", res);

    SetLastError(0xdeadbeef);
    res = AVIStreamRelease(streams[1]);
    ok(res == 0, "1: got refcount %u (expected 0)\n", res);

}

/* ########################### */

START_TEST(api)
{

    AVIFileInit();
    test_AVISaveOptions();
    AVIFileExit();

}
