/*
 * Unit test suite for GDI objects
 *
 * Copyright 2002 Mike McCormack
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

#include <stdarg.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/test.h"

static void test_logfont(void)
{
    LOGFONTA lf, lfout;
    HFONT hfont;

    memset(&lf, 0, sizeof lf);

    lf.lfCharSet = ANSI_CHARSET;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfWeight = FW_DONTCARE;
    lf.lfHeight = 16;
    lf.lfWidth = 16;
    lf.lfQuality = DEFAULT_QUALITY;
    lstrcpyA(lf.lfFaceName, "Arial");

    hfont = CreateFontIndirectA(&lf);
    ok(hfont != 0, "CreateFontIndirect failed\n");

    ok(GetObjectA(hfont, sizeof(lfout), &lfout) == sizeof(lfout),
       "GetObject returned wrong size\n");

    ok(!memcmp(&lfout, &lf, FIELD_OFFSET(LOGFONTA, lfFaceName)), "fonts don't match\n");
    ok(!lstrcmpA(lfout.lfFaceName, lf.lfFaceName),
       "font names don't match: %s != %s\n", lfout.lfFaceName, lf.lfFaceName);

    DeleteObject(hfont);

    memset(&lf, 'A', sizeof(lf));
    hfont = CreateFontIndirectA(&lf);
    ok(hfont != 0, "CreateFontIndirectA with strange LOGFONT failed\n");

    ok(GetObjectA(hfont, sizeof(lfout), NULL) == sizeof(lfout),
       "GetObjectA with NULL failed\n");

    ok(GetObjectA(hfont, sizeof(lfout), &lfout) == sizeof(lfout),
       "GetObjectA failed\n");
    ok(!memcmp(&lfout, &lf, FIELD_OFFSET(LOGFONTA, lfFaceName)), "fonts don't match\n");
    lf.lfFaceName[LF_FACESIZE - 1] = 0;
    ok(!lstrcmpA(lfout.lfFaceName, lf.lfFaceName),
       "font names don't match: %s != %s\n", lfout.lfFaceName, lf.lfFaceName);

    DeleteObject(hfont);
}

START_TEST(gdiobj)
{
    test_logfont();
}
