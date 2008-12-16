/*
 * Unit tests for video playback
 *
 * Copyright 2008 Jörg Höhle
 * Copyright 2008 Austin English
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

#include "wine/test.h"

static void test_OpenCase(void)
{
    HIC h;
    /* ICOpen(fcc: vidc , handler: cvid) */
    /* ICTYPE_VIDEO is vidc: lowercase */
    /* Officially registered fcc for Cinepak is CVID: uppercase */
    h = ICOpen(mmioFOURCC('v','i','d','c'),mmioFOURCC('c','v','i','d'),ICMODE_DECOMPRESS);
    ok(0!=h,"ICOpen(vidc.cvid) failed\n");
    if (h) {
        ok(ICClose(h)==ICERR_OK,"ICClose failed\n");
    }
    h = ICOpen(mmioFOURCC('v','i','d','c'),mmioFOURCC('C','V','I','D'),ICMODE_DECOMPRESS);
    ok(0!=h,"ICOpen(vidc.CVID) failed\n");
    if (h) {
        ok(ICClose(h)==ICERR_OK,"ICClose failed\n");
    }
    h = ICOpen(mmioFOURCC('V','I','D','C'),mmioFOURCC('c','v','i','d'),ICMODE_DECOMPRESS);
    todo_wine ok(0!=h,"ICOpen(VIDC.cvid) failed\n");
    if (h) {
        ok(ICClose(h)==ICERR_OK,"ICClose failed\n");
    }
    h = ICOpen(mmioFOURCC('V','I','D','C'),mmioFOURCC('C','V','I','D'),ICMODE_DECOMPRESS);
    todo_wine ok(0!=h,"ICOpen(VIDC.CVID) failed\n");
    if (h) {
        ok(ICClose(h)==ICERR_OK,"ICClose failed\n");
    }
}

START_TEST(msvfw)
{
    test_OpenCase();
}
