/*
 * Unit test suite for GDI objects
 *
 * Copyright 2002 Mike McCormack
 * Copyright 2004 Dmitry Timoshkov
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

static INT CALLBACK font_enum_proc(const LOGFONT *elf, const TEXTMETRIC *ntm, DWORD type, LPARAM lParam)
{
    if (type & RASTER_FONTTYPE)
    {
	LOGFONT *lf = (LOGFONT *)lParam;
	*lf = *elf;
	return 0; /* stop enumeration */
    }

    return 1; /* continue enumeration */
}

static void test_font_metrics(HDC hdc, HFONT hfont, const char *test_str,
			      INT test_str_len, const TEXTMETRICA *tm_orig,
			      const SIZE *size_orig, INT width_orig,
			      INT scale_x, INT scale_y)
{
    HFONT old_hfont;
    TEXTMETRICA tm;
    SIZE size;
    INT width;

    old_hfont = SelectObject(hdc, hfont);

    GetTextMetricsA(hdc, &tm);

    ok(tm.tmHeight == tm_orig->tmHeight * scale_y, "%ld != %ld\n", tm.tmHeight, tm_orig->tmHeight * scale_y);
    ok(tm.tmAscent == tm_orig->tmAscent * scale_y, "%ld != %ld\n", tm.tmAscent, tm_orig->tmAscent * scale_y);
    ok(tm.tmDescent == tm_orig->tmDescent * scale_y, "%ld != %ld\n", tm.tmDescent, tm_orig->tmDescent * scale_y);
    ok(tm.tmAveCharWidth == tm_orig->tmAveCharWidth * scale_x, "%ld != %ld\n", tm.tmAveCharWidth, tm_orig->tmAveCharWidth * scale_x);

    GetTextExtentPoint32A(hdc, test_str, test_str_len, &size);

    ok(size.cx == size_orig->cx * scale_x, "%ld != %ld\n", size.cx, size_orig->cx * scale_x);
    ok(size.cy == size_orig->cy * scale_y, "%ld != %ld\n", size.cy, size_orig->cy * scale_y);

    GetCharWidthA(hdc, 'A', 'A', &width);

    ok(width == width_orig * scale_x, "%d != %d\n", width, width_orig * scale_x);

    SelectObject(hdc, old_hfont);
}

/* see whether GDI scales bitmap font metrics */
static void test_bitmap_font(void)
{
    static const char test_str[11] = "Test String";
    HDC hdc;
    LOGFONTA bitmap_lf, lf;
    HFONT hfont, old_hfont;
    TEXTMETRICA tm_orig;
    SIZE size_orig;
    INT ret, i, width_orig, height_orig;

    hdc = GetDC(0);

    /* "System" has only 1 pixel size defined, otherwise the test breaks */
    ret = EnumFontFamiliesA(hdc, "System", font_enum_proc, (LPARAM)&bitmap_lf);
    if (ret)
    {
	ReleaseDC(0, hdc);
	trace("no bitmap fonts were found, skipping the test\n");
	return;
    }

    trace("found bitmap font %s, height %ld\n", bitmap_lf.lfFaceName, bitmap_lf.lfHeight);

    height_orig = bitmap_lf.lfHeight;
    hfont = CreateFontIndirectA(&bitmap_lf);
    assert(hfont);

    old_hfont = SelectObject(hdc, hfont);
    ok(GetTextMetricsA(hdc, &tm_orig), "GetTextMetricsA failed\n");
    ok(GetTextExtentPoint32A(hdc, test_str, sizeof(test_str), &size_orig), "GetTextExtentPoint32A failed\n");
    ok(GetCharWidthA(hdc, 'A', 'A', &width_orig), "GetCharWidthA failed\n");
    SelectObject(hdc, old_hfont);
    DeleteObject(hfont);

    /* test fractional scaling */
    for (i = 1; i < height_orig; i++)
    {
	bitmap_lf.lfHeight = i;
	hfont = CreateFontIndirectA(&bitmap_lf);
	assert(hfont);

	ret = GetObject(hfont, sizeof(lf), &lf);
	ok(ret == sizeof(lf), "GetObject failed: %d\n", ret);
	ok(!memcmp(&bitmap_lf, &lf, FIELD_OFFSET(LOGFONTA, lfFaceName)), "fonts don't match\n");
	ok(!lstrcmpA(bitmap_lf.lfFaceName, lf.lfFaceName),
	   "font names don't match: %s != %s\n", bitmap_lf.lfFaceName, lf.lfFaceName);

	test_font_metrics(hdc, hfont, test_str, sizeof(test_str), &tm_orig, &size_orig, width_orig, 1, 1);
	DeleteObject(hfont);
    }

    /* test integer scaling 3x2 */
    bitmap_lf.lfHeight = height_orig * 2;
    bitmap_lf.lfWidth *= 3;
    hfont = CreateFontIndirectA(&bitmap_lf);
    assert(hfont);

    ret = GetObject(hfont, sizeof(lf), &lf);
    ok(ret == sizeof(lf), "GetObject failed: %d\n", ret);
    ok(!memcmp(&bitmap_lf, &lf, FIELD_OFFSET(LOGFONTA, lfFaceName)), "fonts don't match\n");
    ok(!lstrcmpA(bitmap_lf.lfFaceName, lf.lfFaceName),
       "font names don't match: %s != %s\n", bitmap_lf.lfFaceName, lf.lfFaceName);
todo_wine
{
    test_font_metrics(hdc, hfont, test_str, sizeof(test_str), &tm_orig, &size_orig, width_orig, 3, 2);
}
    DeleteObject(hfont);

    /* test integer scaling 3x3 */
    bitmap_lf.lfHeight = height_orig * 3;
    bitmap_lf.lfWidth = 0;
    hfont = CreateFontIndirectA(&bitmap_lf);
    assert(hfont);

    ret = GetObject(hfont, sizeof(lf), &lf);
    ok(ret == sizeof(lf), "GetObject failed: %d\n", ret);
    ok(!memcmp(&bitmap_lf, &lf, FIELD_OFFSET(LOGFONTA, lfFaceName)), "fonts don't match\n");
    ok(!lstrcmpA(bitmap_lf.lfFaceName, lf.lfFaceName),
       "font names don't match: %s != %s\n", bitmap_lf.lfFaceName, lf.lfFaceName);
todo_wine
{
    test_font_metrics(hdc, hfont, test_str, sizeof(test_str), &tm_orig, &size_orig, width_orig, 3, 3);
}
    DeleteObject(hfont);

    ReleaseDC(0, hdc);
}

static void test_gdi_objects(void)
{
    BYTE buff[256];
    HDC hdc = GetDC(NULL);
    HPEN hp;
    int i;

    /* SelectObject() with a NULL DC returns 0 and sets ERROR_INVALID_HANDLE.
     * Note: Under XP at least invalid ptrs can also be passed, not just NULL;
     *       Don't test that here in case it crashes earlier win versions.
     */
    SetLastError(0);
    hp = SelectObject(NULL, GetStockObject(BLACK_PEN));
    ok(!hp && GetLastError() == ERROR_INVALID_HANDLE,
       "SelectObject(NULL DC) expected 0, ERROR_INVALID_HANDLE, got %p, 0x%08lx\n",
       hp, GetLastError());

    /* With a valid DC and a NULL object, the call returns 0 but does not SetLastError() */
    SetLastError(0);
    hp = SelectObject(hdc, NULL);
    ok(!hp && !GetLastError(),
       "SelectObject(NULL obj) expected 0, NO_ERROR, got %p, 0x%08lx\n",
       hp, GetLastError());

    /* The DC is unaffected by the NULL SelectObject */
    SetLastError(0);
    hp = SelectObject(hdc, GetStockObject(BLACK_PEN));
    ok(hp && !GetLastError(),
       "SelectObject(post NULL) expected non-null, NO_ERROR, got %p, 0x%08lx\n",
       hp, GetLastError());

    /* GetCurrentObject does not SetLastError() on a null object */
    SetLastError(0);
    hp = GetCurrentObject(NULL, OBJ_PEN);
    ok(!hp && !GetLastError(),
       "GetCurrentObject(NULL DC) expected 0, NO_ERROR, got %p, 0x%08lx\n",
       hp, GetLastError());

    /* DeleteObject does not SetLastError() on a null object */
    ok(!DeleteObject(NULL) && !GetLastError(),
       "DeleteObject(NULL obj), expected 0, NO_ERROR, got %d, 0x%08lx\n",
       DeleteObject(NULL), GetLastError());

    /* GetObject does not SetLastError() on a null object */
    SetLastError(0);
    i = GetObjectA(NULL, sizeof(buff), buff);
    ok (!i && !GetLastError(),
        "GetObject(NULL obj), expected 0, NO_ERROR, got %d, 0x%08lx\n",
	i, GetLastError());

    /* GetObjectType does SetLastError() on a null object */
    SetLastError(0);
    i = GetObjectType(NULL);
    ok (!i && GetLastError() == ERROR_INVALID_HANDLE,
        "GetObjectType(NULL obj), expected 0, ERROR_INVALID_HANDLE, got %d, 0x%08lx\n",
        i, GetLastError());

    /* UnrealizeObject does not SetLastError() on a null object */
    SetLastError(0);
    i = UnrealizeObject(NULL);
    ok (!i && !GetLastError(),
        "UnrealizeObject(NULL obj), expected 0, NO_ERROR, got %d, 0x%08lx\n",
        i, GetLastError());

    ReleaseDC(NULL, hdc);
}

START_TEST(gdiobj)
{
    test_logfont();
    test_bitmap_font();
    test_gdi_objects();
}
