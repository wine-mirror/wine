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


static void check_font(const char* test, const LOGFONTA* lf, HFONT hfont)
{
    LOGFONTA getobj_lf;
    int ret, minlen = 0;

    if (!hfont)
        return;

    ret = GetObject(hfont, sizeof(getobj_lf), &getobj_lf);
    /* NT4 tries to be clever and only returns the minimum length */
    while (lf->lfFaceName[minlen] && minlen < LF_FACESIZE-1)
        minlen++;
    minlen += FIELD_OFFSET(LOGFONTA, lfFaceName) + 1;
    ok(ret == sizeof(LOGFONTA) || ret == minlen,
       "%s: GetObject returned %d expected %d or %d\n", test, ret, sizeof(LOGFONTA), minlen);
    ok(!memcmp(&lf, &lf, FIELD_OFFSET(LOGFONTA, lfFaceName)), "%s: fonts don't match\n", test);
    ok(!lstrcmpA(lf->lfFaceName, getobj_lf.lfFaceName),
       "%s: font names don't match: %s != %s\n", test, lf->lfFaceName, getobj_lf.lfFaceName);
}

static HFONT create_font(const char* test, const LOGFONTA* lf)
{
    HFONT hfont = CreateFontIndirectA(lf);
    ok(hfont != 0, "%s: CreateFontIndirect failed\n", test);
    if (hfont)
        check_font(test, lf, hfont);
    return hfont;
}

static void test_logfont(void)
{
    LOGFONTA lf;
    HFONT hfont;

    memset(&lf, 0, sizeof lf);

    lf.lfCharSet = ANSI_CHARSET;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfWeight = FW_DONTCARE;
    lf.lfHeight = 16;
    lf.lfWidth = 16;
    lf.lfQuality = DEFAULT_QUALITY;

    lstrcpyA(lf.lfFaceName, "Arial");
    hfont = create_font("Arial", &lf);
    DeleteObject(hfont);

    memset(&lf, 'A', sizeof(lf));
    hfont = CreateFontIndirectA(&lf);
    ok(hfont != 0, "CreateFontIndirectA with strange LOGFONT failed\n");
    
    lf.lfFaceName[LF_FACESIZE - 1] = 0;
    check_font("AAA...", &lf, hfont);
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

    if (!hfont)
        return;

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
    LOGFONTA bitmap_lf;
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
    hfont = create_font("bitmap", &bitmap_lf);

    old_hfont = SelectObject(hdc, hfont);
    ok(GetTextMetricsA(hdc, &tm_orig), "GetTextMetricsA failed\n");
    ok(GetTextExtentPoint32A(hdc, test_str, sizeof(test_str), &size_orig), "GetTextExtentPoint32A failed\n");
    ok(GetCharWidthA(hdc, 'A', 'A', &width_orig), "GetCharWidthA failed\n");
    SelectObject(hdc, old_hfont);
    DeleteObject(hfont);

    /* test fractional scaling */
    for (i = 1; i < height_orig; i++)
    {
	hfont = create_font("fractional", &bitmap_lf);
	test_font_metrics(hdc, hfont, test_str, sizeof(test_str), &tm_orig, &size_orig, width_orig, 1, 1);
	DeleteObject(hfont);
    }

    /* test integer scaling 3x2 */
    bitmap_lf.lfHeight = height_orig * 2;
    bitmap_lf.lfWidth *= 3;
    hfont = create_font("3x2", &bitmap_lf);
todo_wine
{
    test_font_metrics(hdc, hfont, test_str, sizeof(test_str), &tm_orig, &size_orig, width_orig, 3, 2);
}
    DeleteObject(hfont);

    /* test integer scaling 3x3 */
    bitmap_lf.lfHeight = height_orig * 3;
    bitmap_lf.lfWidth = 0;
    hfont = create_font("3x3", &bitmap_lf);

todo_wine
{
    test_font_metrics(hdc, hfont, test_str, sizeof(test_str), &tm_orig, &size_orig, width_orig, 3, 3);
}
    DeleteObject(hfont);

    ReleaseDC(0, hdc);
}

static INT CALLBACK find_font_proc(const LOGFONT *elf, const TEXTMETRIC *ntm, DWORD type, LPARAM lParam)
{
    LOGFONT *lf = (LOGFONT *)lParam;

    trace("found font %s, height %ld\n", elf->lfFaceName, elf->lfHeight);

    if (elf->lfHeight == lf->lfHeight && !strcmp(elf->lfFaceName, lf->lfFaceName))
    {
        *lf = *elf;
        return 0; /* stop enumeration */
    }
    return 1; /* continue enumeration */
}

static void test_bitmap_font_metrics(void)
{
    static const struct font_data
    {
        const char face_name[LF_FACESIZE];
        int weight, height, ascent, descent, int_leading, ext_leading;
        int ave_char_width, max_char_width;
    } fd[] =
    {
        { "MS Sans Serif", FW_NORMAL, 13, 11, 2, 2, 0, 5, 11 },
        { "MS Sans Serif", FW_NORMAL, 16, 13, 3, 3, 0, 7, 14 },
        { "MS Sans Serif", FW_NORMAL, 20, 16, 4, 4, 0, 8, 16 },
        { "MS Sans Serif", FW_NORMAL, 24, 19, 5, 6, 0, 9, 20 },
        { "MS Sans Serif", FW_NORMAL, 29, 23, 6, 5, 0, 12, 25 },
        { "MS Sans Serif", FW_NORMAL, 37, 29, 8, 5, 0, 16, 32 },
        { "MS Serif", FW_NORMAL, 10, 8, 2, 2, 0, 5, 8 },
        { "MS Serif", FW_NORMAL, 11, 9, 2, 2, 0, 5, 9 },
        { "MS Serif", FW_NORMAL, 13, 11, 2, 2, 0, 5, 12 },
        { "MS Serif", FW_NORMAL, 16, 13, 3, 3, 0, 6, 16 },
        { "MS Serif", FW_NORMAL, 19, 15, 4, 3, 0, 8, 19 },
        { "MS Serif", FW_NORMAL, 21, 16, 5, 3, 0, 9, 23 },
        { "MS Serif", FW_NORMAL, 27, 21, 6, 3, 0, 12, 27 },
        { "MS Serif", FW_NORMAL, 35, 27, 8, 3, 0, 16, 34 },
#if 0 /* FIXME: enable once the bug in sfnt2fnt is fixed */
        { "Courier", FW_NORMAL, 13, 11, 2, 0, 0, 8, 8 },
#endif
        { "Courier", FW_NORMAL, 16, 13, 3, 0, 0, 9, 9 },
        { "Courier", FW_NORMAL, 20, 16, 4, 0, 0, 12, 12 },
        { "System", FW_BOLD, 16, 13, 3, 3, 0, 7, 15 }
        /* FIXME: add "Fixedsys", "Terminal", "Small Fonts" */
    };
    HDC hdc;
    LOGFONT lf;
    HFONT hfont, old_hfont;
    TEXTMETRIC tm;
    INT ret, i;

    hdc = CreateCompatibleDC(0);
    assert(hdc);

    for (i = 0; i < sizeof(fd)/sizeof(fd[0]); i++)
    {
        memset(&lf, 0, sizeof(lf));

        lf.lfHeight = fd[i].height;
        strcpy(lf.lfFaceName, fd[i].face_name);
        ret = EnumFontFamilies(hdc, fd[i].face_name, find_font_proc, (LPARAM)&lf);
        if (ret)
        {
            trace("font %s height %d not found\n", fd[i].face_name, fd[i].height);
            continue;
        }

        trace("found font %s, height %ld\n", lf.lfFaceName, lf.lfHeight);

        hfont = create_font(lf.lfFaceName, &lf);
        old_hfont = SelectObject(hdc, hfont);
        ok(GetTextMetrics(hdc, &tm), "GetTextMetrics error %ld\n", GetLastError());

        ok(tm.tmWeight == fd[i].weight, "%s(%d): tm.tmWeight %ld != %d\n", fd[i].face_name, fd[i].height, tm.tmWeight, fd[i].weight);
        ok(tm.tmHeight == fd[i].height, "%s(%d): tm.tmHeight %ld != %d\n", fd[i].face_name, fd[i].height, tm.tmHeight, fd[i].height);
        ok(tm.tmAscent == fd[i].ascent, "%s(%d): tm.tmAscent %ld != %d\n", fd[i].face_name, fd[i].height, tm.tmAscent, fd[i].ascent);
        ok(tm.tmDescent == fd[i].descent, "%s(%d): tm.tmDescent %ld != %d\n", fd[i].face_name, fd[i].height, tm.tmDescent, fd[i].descent);
        ok(tm.tmInternalLeading == fd[i].int_leading, "%s(%d): tm.tmInternalLeading %ld != %d\n", fd[i].face_name, fd[i].height, tm.tmInternalLeading, fd[i].int_leading);
        ok(tm.tmExternalLeading == fd[i].ext_leading, "%s(%d): tm.tmExternalLeading %ld != %d\n", fd[i].face_name, fd[i].height, tm.tmExternalLeading, fd[i].ext_leading);
        ok(tm.tmAveCharWidth == fd[i].ave_char_width, "%s(%d): tm.tmAveCharWidth %ld != %d\n", fd[i].face_name, fd[i].height, tm.tmAveCharWidth, fd[i].ave_char_width);
        ok(tm.tmMaxCharWidth == fd[i].max_char_width, "%s(%d): tm.tmMaxCharWidth %ld != %d\n", fd[i].face_name, fd[i].height, tm.tmMaxCharWidth, fd[i].max_char_width);

        SelectObject(hdc, old_hfont);
        DeleteObject(hfont);
    }

    DeleteDC(hdc);
}

static void test_gdi_objects(void)
{
    BYTE buff[256];
    HDC hdc = GetDC(NULL);
    HPEN hp;
    int i;
    BOOL ret;

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
    ret = DeleteObject(NULL);
    ok( !ret && !GetLastError(),
       "DeleteObject(NULL obj), expected 0, NO_ERROR, got %d, 0x%08lx\n",
       ret, GetLastError());

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

static void test_GdiGetCharDimensions(void)
{
    HDC hdc;
    TEXTMETRICW tm;
    LONG ret;
    SIZE size;
    LONG avgwidth, height;
    static const char szAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    typedef LONG (WINAPI *fnGdiGetCharDimensions)(HDC hdc, LPTEXTMETRICW lptm, LONG *height);
    fnGdiGetCharDimensions GdiGetCharDimensions = (fnGdiGetCharDimensions)GetProcAddress(LoadLibrary("gdi32"), "GdiGetCharDimensions");
    if (!GdiGetCharDimensions) return;

    hdc = CreateCompatibleDC(NULL);

    GetTextExtentPoint(hdc, szAlphabet, strlen(szAlphabet), &size);
    avgwidth = ((size.cx / 26) + 1) / 2;

    ret = GdiGetCharDimensions(hdc, &tm, &height);
    ok(ret == avgwidth, "GdiGetCharDimensions should have returned width of %ld instead of %ld\n", avgwidth, ret);
    ok(height == tm.tmHeight, "GdiGetCharDimensions should have set height to %ld instead of %ld\n", tm.tmHeight, height);

    ret = GdiGetCharDimensions(hdc, &tm, NULL);
    ok(ret == avgwidth, "GdiGetCharDimensions should have returned width of %ld instead of %ld\n", avgwidth, ret);

    ret = GdiGetCharDimensions(hdc, NULL, NULL);
    ok(ret == avgwidth, "GdiGetCharDimensions should have returned width of %ld instead of %ld\n", avgwidth, ret);

    height = 0;
    ret = GdiGetCharDimensions(hdc, NULL, &height);
    ok(ret == avgwidth, "GdiGetCharDimensions should have returned width of %ld instead of %ld\n", avgwidth, ret);
    ok(height == size.cy, "GdiGetCharDimensions should have set height to %ld instead of %ld\n", size.cy, height);

    DeleteDC(hdc);
}

static void test_text_extents(void)
{
    LOGFONTA lf;
    TEXTMETRICA tm;
    HDC hdc;
    HFONT hfont;
    SIZE sz;

    memset(&lf, 0, sizeof(lf));
    strcpy(lf.lfFaceName, "Arial");
    lf.lfHeight = 20;

    hfont = CreateFontIndirectA(&lf);
    hdc = GetDC(0);
    hfont = SelectObject(hdc, hfont);
    GetTextMetricsA(hdc, &tm);
    GetTextExtentPointA(hdc, "o", 1, &sz);
    ok(sz.cy == tm.tmHeight, "cy %ld tmHeight %ld\n", sz.cy, tm.tmHeight);

    SelectObject(hdc, hfont);
    DeleteObject(hfont);
    ReleaseDC(NULL, hdc);
}

struct hgdiobj_event
{
    HDC hdc;
    HGDIOBJ hgdiobj1;
    HGDIOBJ hgdiobj2;
    HANDLE stop_event;
    HANDLE ready_event;
};

static DWORD WINAPI thread_proc(void *param)
{
    LOGPEN lp;
    struct hgdiobj_event *hgdiobj_event = (struct hgdiobj_event *)param;

    hgdiobj_event->hdc = CreateDC("display", NULL, NULL, NULL);
    ok(hgdiobj_event->hdc != NULL, "CreateDC error %ld\n", GetLastError());

    hgdiobj_event->hgdiobj1 = CreatePen(PS_DASHDOTDOT, 17, RGB(1, 2, 3));
    ok(hgdiobj_event->hgdiobj1 != 0, "Failed to create pen\n");

    hgdiobj_event->hgdiobj2 = CreateRectRgn(0, 1, 12, 17);
    ok(hgdiobj_event->hgdiobj2 != 0, "Failed to create pen\n");

    SetEvent(hgdiobj_event->ready_event);
    ok(WaitForSingleObject(hgdiobj_event->stop_event, INFINITE) == WAIT_OBJECT_0,
       "WaitForSingleObject error %ld\n", GetLastError());

    ok(!GetObject(hgdiobj_event->hgdiobj1, sizeof(lp), &lp), "GetObject should fail\n");

    ok(!GetDeviceCaps(hgdiobj_event->hdc, TECHNOLOGY), "GetDeviceCaps(TECHNOLOGY) should fail\n");

    return 0;
}

static void test_thread_objects(void)
{
    LOGPEN lp;
    DWORD tid, type;
    HANDLE hthread;
    struct hgdiobj_event hgdiobj_event;
    INT ret;

    hgdiobj_event.stop_event = CreateEvent(NULL, 0, 0, NULL);
    ok(hgdiobj_event.stop_event != NULL, "CreateEvent error %ld\n", GetLastError());
    hgdiobj_event.ready_event = CreateEvent(NULL, 0, 0, NULL);
    ok(hgdiobj_event.ready_event != NULL, "CreateEvent error %ld\n", GetLastError());

    hthread = CreateThread(NULL, 0, thread_proc, &hgdiobj_event, 0, &tid);
    ok(hthread != NULL, "CreateThread error %ld\n", GetLastError());

    ok(WaitForSingleObject(hgdiobj_event.ready_event, INFINITE) == WAIT_OBJECT_0,
       "WaitForSingleObject error %ld\n", GetLastError());

    ok(GetObject(hgdiobj_event.hgdiobj1, sizeof(lp), &lp) == sizeof(lp),
       "GetObject error %ld\n", GetLastError());
    ok(lp.lopnStyle == PS_DASHDOTDOT, "wrong pen style %d\n", lp.lopnStyle);
    ok(lp.lopnWidth.x == 17, "wrong pen width.y %ld\n", lp.lopnWidth.x);
    ok(lp.lopnWidth.y == 0, "wrong pen width.y %ld\n", lp.lopnWidth.y);
    ok(lp.lopnColor == RGB(1, 2, 3), "wrong pen width.y %08lx\n", lp.lopnColor);

    ret = GetDeviceCaps(hgdiobj_event.hdc, TECHNOLOGY);
    ok(ret == DT_RASDISPLAY, "GetDeviceCaps(TECHNOLOGY) should return DT_RASDISPLAY not %d\n", ret);

    ok(DeleteObject(hgdiobj_event.hgdiobj1), "DeleteObject error %ld\n", GetLastError());
    ok(DeleteDC(hgdiobj_event.hdc), "DeleteDC error %ld\n", GetLastError());

    type = GetObjectType(hgdiobj_event.hgdiobj2);
    ok(type == OBJ_REGION, "GetObjectType returned %lu\n", type);

    SetEvent(hgdiobj_event.stop_event);
    ok(WaitForSingleObject(hthread, INFINITE) == WAIT_OBJECT_0,
       "WaitForSingleObject error %ld\n", GetLastError());
    CloseHandle(hthread);

    type = GetObjectType(hgdiobj_event.hgdiobj2);
    ok(type == OBJ_REGION, "GetObjectType returned %lu\n", type);
    ok(DeleteObject(hgdiobj_event.hgdiobj2), "DeleteObject error %ld\n", GetLastError());

    CloseHandle(hgdiobj_event.stop_event);
    CloseHandle(hgdiobj_event.ready_event);
}

static void test_GetCurrentObject(void)
{
    DWORD type;
    HPEN hpen;
    HBRUSH hbrush;
    HPALETTE hpal;
    HFONT hfont;
    HBITMAP hbmp;
    HRGN hrgn;
    HDC hdc;
    HCOLORSPACE hcs;
    HGDIOBJ hobj;
    LOGBRUSH lb;
    LOGCOLORSPACEA lcs;

    hdc = CreateCompatibleDC(0);
    assert(hdc != 0);

    type = GetObjectType(hdc);
    ok(type == OBJ_MEMDC, "GetObjectType returned %lu\n", type);

    hpen = CreatePen(PS_SOLID, 10, RGB(10, 20, 30));
    assert(hpen != 0);
    SelectObject(hdc, hpen);
    hobj = GetCurrentObject(hdc, OBJ_PEN);
    ok(hobj == hpen, "OBJ_PEN is wrong: %p\n", hobj);
    hobj = GetCurrentObject(hdc, OBJ_EXTPEN);
    ok(hobj == hpen, "OBJ_EXTPEN is wrong: %p\n", hobj);

    hbrush = CreateSolidBrush(RGB(10, 20, 30));
    assert(hbrush != 0);
    SelectObject(hdc, hbrush);
    hobj = GetCurrentObject(hdc, OBJ_BRUSH);
    ok(hobj == hbrush, "OBJ_BRUSH is wrong: %p\n", hobj);

    hpal = CreateHalftonePalette(hdc);
    assert(hpal != 0);
    SelectPalette(hdc, hpal, FALSE);
    hobj = GetCurrentObject(hdc, OBJ_PAL);
    ok(hobj == hpal, "OBJ_PAL is wrong: %p\n", hobj);

    hfont = CreateFontA(10, 5, 0, 0, FW_DONTCARE, 0, 0, 0, ANSI_CHARSET,
                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                        DEFAULT_PITCH, "MS Sans Serif");
    assert(hfont != 0);
    SelectObject(hdc, hfont);
    hobj = GetCurrentObject(hdc, OBJ_FONT);
    ok(hobj == hfont, "OBJ_FONT is wrong: %p\n", hobj);

    hbmp = CreateBitmap(100, 100, 1, 1, NULL);
    assert(hbmp != 0);
    SelectObject(hdc, hbmp);
    hobj = GetCurrentObject(hdc, OBJ_BITMAP);
    ok(hobj == hbmp, "OBJ_BITMAP is wrong: %p\n", hobj);

    assert(GetObject(hbrush, sizeof(lb), &lb) == sizeof(lb));
    hpen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_SQUARE | PS_JOIN_BEVEL,
                        10, &lb, 0, NULL);
    assert(hpen != 0);
    SelectObject(hdc, hpen);
    hobj = GetCurrentObject(hdc, OBJ_PEN);
    ok(hobj == hpen, "OBJ_PEN is wrong: %p\n", hobj);
    hobj = GetCurrentObject(hdc, OBJ_EXTPEN);
    ok(hobj == hpen, "OBJ_EXTPEN is wrong: %p\n", hobj);

    hcs = GetColorSpace(hdc);
    if (hcs)
    {
        trace("current color space is not NULL\n");
        ok(GetLogColorSpaceA(hcs, &lcs, sizeof(lcs)), "GetLogColorSpace failed\n");
        hcs = CreateColorSpaceA(&lcs);
        ok(hcs != 0, "CreateColorSpace failed\n");
        SelectObject(hdc, hcs);
        hobj = GetCurrentObject(hdc, OBJ_COLORSPACE);
        ok(hobj == hcs, "OBJ_COLORSPACE is wrong: %p\n", hobj);
    }

    hrgn = CreateRectRgn(1, 1, 100, 100);
    assert(hrgn != 0);
    SelectObject(hdc, hrgn);
    hobj = GetCurrentObject(hdc, OBJ_REGION);
    ok(!hobj, "OBJ_REGION is wrong: %p\n", hobj);

    DeleteDC(hdc);
}

static void test_logpen(void)
{
    static const struct
    {
        UINT style;
        INT width;
        COLORREF color;
        UINT ret_style;
        INT ret_width;
        COLORREF ret_color;
    } pen[] = {
        { PS_SOLID, -123, RGB(0x12,0x34,0x56), PS_SOLID, 123, RGB(0x12,0x34,0x56) },
        { PS_SOLID, 0, RGB(0x12,0x34,0x56), PS_SOLID, 0, RGB(0x12,0x34,0x56) },
        { PS_SOLID, 123, RGB(0x12,0x34,0x56), PS_SOLID, 123, RGB(0x12,0x34,0x56) },
        { PS_DASH, 123, RGB(0x12,0x34,0x56), PS_DASH, 123, RGB(0x12,0x34,0x56) },
        { PS_DOT, 123, RGB(0x12,0x34,0x56), PS_DOT, 123, RGB(0x12,0x34,0x56) },
        { PS_DASHDOT, 123, RGB(0x12,0x34,0x56), PS_DASHDOT, 123, RGB(0x12,0x34,0x56) },
        { PS_DASHDOTDOT, 123, RGB(0x12,0x34,0x56), PS_DASHDOTDOT, 123, RGB(0x12,0x34,0x56) },
        { PS_NULL, -123, RGB(0x12,0x34,0x56), PS_NULL, 1, 0 },
        { PS_NULL, 123, RGB(0x12,0x34,0x56), PS_NULL, 1, 0 },
        { PS_INSIDEFRAME, 123, RGB(0x12,0x34,0x56), PS_INSIDEFRAME, 123, RGB(0x12,0x34,0x56) },
        { PS_USERSTYLE, 123, RGB(0x12,0x34,0x56), PS_SOLID, 123, RGB(0x12,0x34,0x56) },
        { PS_ALTERNATE, 123, RGB(0x12,0x34,0x56), PS_SOLID, 123, RGB(0x12,0x34,0x56) }
    };
    INT i, size;
    HPEN hpen;
    LOGPEN lp;
    EXTLOGPEN elp;
    LOGBRUSH lb;
    DWORD obj_type, user_style[2] = { 0xabc, 0xdef };
    struct
    {
        EXTLOGPEN elp;
        DWORD style_data[10];
    } ext_pen;

    for (i = 0; i < sizeof(pen)/sizeof(pen[0]); i++)
    {
        trace("testing style %u\n", pen[i].style);

        /********************** cosmetic pens **********************/
        /* CreatePenIndirect behaviour */
        lp.lopnStyle = pen[i].style,
        lp.lopnWidth.x = pen[i].width;
        lp.lopnWidth.y = 11; /* just in case */
        lp.lopnColor = pen[i].color;
        SetLastError(0xdeadbeef);
        hpen = CreatePenIndirect(&lp);
        ok(hpen != 0, "CreatePen error %ld\n", GetLastError());

        obj_type = GetObjectType(hpen);
        ok(obj_type == OBJ_PEN, "wrong object type %lu\n", obj_type);

        memset(&lp, 0xb0, sizeof(lp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(lp), &lp);
        ok(size == sizeof(lp), "GetObject returned %d, error %ld\n", size, GetLastError());

        ok(lp.lopnStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, lp.lopnStyle);
        ok(lp.lopnWidth.x == pen[i].ret_width, "expected %u, got %ld\n", pen[i].ret_width, lp.lopnWidth.x);
        ok(lp.lopnWidth.y == 0, "expected 0, got %ld\n", lp.lopnWidth.y);
        ok(lp.lopnColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, lp.lopnColor);

        DeleteObject(hpen);

        /* CreatePen behaviour */
        SetLastError(0xdeadbeef);
        hpen = CreatePen(pen[i].style, pen[i].width, pen[i].color);
        ok(hpen != 0, "CreatePen error %ld\n", GetLastError());

        obj_type = GetObjectType(hpen);
        ok(obj_type == OBJ_PEN, "wrong object type %lu\n", obj_type);

        /* check what's the real size of the object */
        size = GetObject(hpen, 0, NULL);
        ok(size == sizeof(lp), "GetObject returned %d, error %ld\n", size, GetLastError());

        /* ask for truncated data */
        memset(&lp, 0xb0, sizeof(lp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(lp.lopnStyle), &lp);
        ok(!size, "GetObject should fail: size %d, error %ld\n", size, GetLastError());

        /* see how larger buffer sizes are handled */
        memset(&lp, 0xb0, sizeof(lp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(lp) * 2, &lp);
        ok(size == sizeof(lp), "GetObject returned %d, error %ld\n", size, GetLastError());

        /* see how larger buffer sizes are handled */
        memset(&elp, 0xb0, sizeof(elp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(elp) * 2, &elp);
        ok(size == sizeof(lp), "GetObject returned %d, error %ld\n", size, GetLastError());

        memset(&lp, 0xb0, sizeof(lp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(lp), &lp);
        ok(size == sizeof(lp), "GetObject returned %d, error %ld\n", size, GetLastError());

        ok(lp.lopnStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, lp.lopnStyle);
        ok(lp.lopnWidth.x == pen[i].ret_width, "expected %u, got %ld\n", pen[i].ret_width, lp.lopnWidth.x);
        ok(lp.lopnWidth.y == 0, "expected 0, got %ld\n", lp.lopnWidth.y);
        ok(lp.lopnColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, lp.lopnColor);

        memset(&elp, 0xb0, sizeof(elp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(elp), &elp);

        /* for some reason XP differentiates PS_NULL here */
        if (pen[i].style == PS_NULL)
        {
            ok(size == sizeof(EXTLOGPEN), "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(elp.elpPenStyle == pen[i].ret_style, "expected %u, got %lu\n", pen[i].ret_style, elp.elpPenStyle);
            ok(elp.elpWidth == 0, "expected 0, got %lu\n", elp.elpWidth);
            ok(elp.elpColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, elp.elpColor);
            ok(elp.elpBrushStyle == BS_SOLID, "expected BS_SOLID, got %u\n", elp.elpBrushStyle);
            ok(elp.elpHatch == 0, "expected 0, got %p\n", (void *)elp.elpHatch);
            ok(elp.elpNumEntries == 0, "expected 0, got %lx\n", elp.elpNumEntries);
        }
        else
        {
            ok(size == sizeof(LOGPEN), "GetObject returned %d, error %ld\n", size, GetLastError());
            memcpy(&lp, &elp, sizeof(lp));
            ok(lp.lopnStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, lp.lopnStyle);
            ok(lp.lopnWidth.x == pen[i].ret_width, "expected %u, got %ld\n", pen[i].ret_width, lp.lopnWidth.x);
            ok(lp.lopnWidth.y == 0, "expected 0, got %ld\n", lp.lopnWidth.y);
            ok(lp.lopnColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, lp.lopnColor);
        }

        DeleteObject(hpen);

        /********** cosmetic pens created by ExtCreatePen ***********/
        lb.lbStyle = BS_SOLID;
        lb.lbColor = pen[i].color;
        lb.lbHatch = HS_CROSS; /* just in case */
        SetLastError(0xdeadbeef);
        hpen = ExtCreatePen(pen[i].style, pen[i].width, &lb, 2, user_style);
        if (pen[i].style != PS_USERSTYLE)
        {
            ok(hpen == 0, "ExtCreatePen should fail\n");
            ok(GetLastError() == ERROR_INVALID_PARAMETER,
               "wrong last error value %ld\n", GetLastError());
            SetLastError(0xdeadbeef);
            hpen = ExtCreatePen(pen[i].style, pen[i].width, &lb, 0, NULL);
            if (pen[i].style != PS_NULL)
            {
                ok(hpen == 0, "ExtCreatePen with width != 1 should fail\n");
                ok(GetLastError() == ERROR_INVALID_PARAMETER,
                   "wrong last error value %ld\n", GetLastError());

                SetLastError(0xdeadbeef);
                hpen = ExtCreatePen(pen[i].style, 1, &lb, 0, NULL);
            }
        }
        else
        {
            ok(hpen == 0, "ExtCreatePen with width != 1 should fail\n");
            ok(GetLastError() == ERROR_INVALID_PARAMETER,
               "wrong last error value %ld\n", GetLastError());
            SetLastError(0xdeadbeef);
            hpen = ExtCreatePen(pen[i].style, 1, &lb, 2, user_style);
        }
        if (pen[i].style == PS_INSIDEFRAME)
        {
            /* This style is applicable only for gemetric pens */
            ok(hpen == 0, "ExtCreatePen should fail\n");
            goto test_geometric_pens;
        }
        ok(hpen != 0, "ExtCreatePen error %ld\n", GetLastError());

        obj_type = GetObjectType(hpen);
        /* for some reason XP differentiates PS_NULL here */
        if (pen[i].style == PS_NULL)
            ok(obj_type == OBJ_PEN, "wrong object type %lu\n", obj_type);
        else
            ok(obj_type == OBJ_EXTPEN, "wrong object type %lu\n", obj_type);

        /* check what's the real size of the object */
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, 0, NULL);
        switch (pen[i].style)
        {
        case PS_NULL:
            ok(size == sizeof(LOGPEN),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            break;

        case PS_USERSTYLE:
            ok(size == sizeof(EXTLOGPEN) - sizeof(elp.elpStyleEntry) + sizeof(user_style),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            break;

        default:
            ok(size == sizeof(EXTLOGPEN) - sizeof(elp.elpStyleEntry),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            break;
        }

        /* ask for truncated data */
        memset(&elp, 0xb0, sizeof(elp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(elp.elpPenStyle), &elp);
        ok(!size, "GetObject should fail: size %d, error %ld\n", size, GetLastError());

        /* see how larger buffer sizes are handled */
        memset(&ext_pen, 0xb0, sizeof(ext_pen));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(ext_pen), &ext_pen.elp);
        switch (pen[i].style)
        {
        case PS_NULL:
            ok(size == sizeof(LOGPEN),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            memcpy(&lp, &ext_pen.elp, sizeof(lp));
            ok(lp.lopnStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, lp.lopnStyle);
            ok(lp.lopnWidth.x == pen[i].ret_width, "expected %u, got %ld\n", pen[i].ret_width, lp.lopnWidth.x);
            ok(lp.lopnWidth.y == 0, "expected 0, got %ld\n", lp.lopnWidth.y);
            ok(lp.lopnColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, lp.lopnColor);

            /* for PS_NULL it also works this way */
            memset(&elp, 0xb0, sizeof(elp));
            SetLastError(0xdeadbeef);
            size = GetObject(hpen, sizeof(elp), &elp);
            ok(size == sizeof(EXTLOGPEN),
                "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(ext_pen.elp.elpHatch == 0xb0b0b0b0, "expected 0xb0b0b0b0, got %p\n", (void *)ext_pen.elp.elpHatch);
            ok(ext_pen.elp.elpNumEntries == 0xb0b0b0b0, "expected 0xb0b0b0b0, got %lx\n", ext_pen.elp.elpNumEntries);
            break;

        case PS_USERSTYLE:
            ok(size == sizeof(EXTLOGPEN) - sizeof(elp.elpStyleEntry) + sizeof(user_style),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(ext_pen.elp.elpHatch == HS_CROSS, "expected HS_CROSS, got %p\n", (void *)ext_pen.elp.elpHatch);
            ok(ext_pen.elp.elpNumEntries == 2, "expected 0, got %lx\n", ext_pen.elp.elpNumEntries);
            ok(ext_pen.elp.elpStyleEntry[0] == 0xabc, "expected 0xabc, got %lx\n", ext_pen.elp.elpStyleEntry[0]);
            ok(ext_pen.elp.elpStyleEntry[1] == 0xdef, "expected 0xabc, got %lx\n", ext_pen.elp.elpStyleEntry[1]);
            break;

        default:
            ok(size == sizeof(EXTLOGPEN) - sizeof(elp.elpStyleEntry),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(ext_pen.elp.elpHatch == HS_CROSS, "expected HS_CROSS, got %p\n", (void *)ext_pen.elp.elpHatch);
            ok(ext_pen.elp.elpNumEntries == 0, "expected 0, got %lx\n", ext_pen.elp.elpNumEntries);
            break;
        }

if (pen[i].style == PS_USERSTYLE)
{
    todo_wine
        ok(ext_pen.elp.elpPenStyle == pen[i].style, "expected %x, got %lx\n", pen[i].style, ext_pen.elp.elpPenStyle);
}
else
        ok(ext_pen.elp.elpPenStyle == pen[i].style, "expected %x, got %lx\n", pen[i].style, ext_pen.elp.elpPenStyle);
        ok(ext_pen.elp.elpWidth == 1, "expected 1, got %lx\n", ext_pen.elp.elpWidth);
        ok(ext_pen.elp.elpColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, ext_pen.elp.elpColor);
        ok(ext_pen.elp.elpBrushStyle == BS_SOLID, "expected BS_SOLID, got %x\n", ext_pen.elp.elpBrushStyle);

        DeleteObject(hpen);

test_geometric_pens:
        /********************** geometric pens **********************/
        lb.lbStyle = BS_SOLID;
        lb.lbColor = pen[i].color;
        lb.lbHatch = HS_CROSS; /* just in case */
        SetLastError(0xdeadbeef);
        hpen = ExtCreatePen(PS_GEOMETRIC | pen[i].style, pen[i].width, &lb, 2, user_style);
        if (pen[i].style != PS_USERSTYLE)
        {
            ok(hpen == 0, "ExtCreatePen should fail\n");
            SetLastError(0xdeadbeef);
            hpen = ExtCreatePen(PS_GEOMETRIC | pen[i].style, pen[i].width, &lb, 0, NULL);
        }
        if (pen[i].style == PS_ALTERNATE)
        {
            /* This style is applicable only for cosmetic pens */
            ok(hpen == 0, "ExtCreatePen should fail\n");
            continue;
        }
        ok(hpen != 0, "ExtCreatePen error %ld\n", GetLastError());

        obj_type = GetObjectType(hpen);
        /* for some reason XP differentiates PS_NULL here */
        if (pen[i].style == PS_NULL)
            ok(obj_type == OBJ_PEN, "wrong object type %lu\n", obj_type);
        else
            ok(obj_type == OBJ_EXTPEN, "wrong object type %lu\n", obj_type);

        /* check what's the real size of the object */
        size = GetObject(hpen, 0, NULL);
        switch (pen[i].style)
        {
        case PS_NULL:
            ok(size == sizeof(LOGPEN),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            break;

        case PS_USERSTYLE:
            ok(size == sizeof(EXTLOGPEN) - sizeof(elp.elpStyleEntry) + sizeof(user_style),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            break;

        default:
            ok(size == sizeof(EXTLOGPEN) - sizeof(elp.elpStyleEntry),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            break;
        }

        /* ask for truncated data */
        memset(&lp, 0xb0, sizeof(lp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(lp.lopnStyle), &lp);
        ok(!size, "GetObject should fail: size %d, error %ld\n", size, GetLastError());

        memset(&lp, 0xb0, sizeof(lp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(lp), &lp);
        /* for some reason XP differenciates PS_NULL here */
        if (pen[i].style == PS_NULL)
        {
            ok(size == sizeof(LOGPEN), "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(lp.lopnStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, lp.lopnStyle);
            ok(lp.lopnWidth.x == pen[i].ret_width, "expected %u, got %ld\n", pen[i].ret_width, lp.lopnWidth.x);
            ok(lp.lopnWidth.y == 0, "expected 0, got %ld\n", lp.lopnWidth.y);
            ok(lp.lopnColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, lp.lopnColor);
        }
        else
            /* XP doesn't set last error here */
            ok(!size /*&& GetLastError() == ERROR_INVALID_PARAMETER*/,
               "GetObject should fail: size %d, error %ld\n", size, GetLastError());

        memset(&ext_pen, 0xb0, sizeof(ext_pen));
        SetLastError(0xdeadbeef);
        /* buffer is too small for user styles */
        size = GetObject(hpen, sizeof(elp), &ext_pen.elp);
        switch (pen[i].style)
        {
        case PS_NULL:
            ok(size == sizeof(EXTLOGPEN),
                "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(ext_pen.elp.elpHatch == 0, "expected 0, got %p\n", (void *)ext_pen.elp.elpHatch);
            ok(ext_pen.elp.elpNumEntries == 0, "expected 0, got %lx\n", ext_pen.elp.elpNumEntries);

            /* for PS_NULL it also works this way */
            SetLastError(0xdeadbeef);
            size = GetObject(hpen, sizeof(ext_pen), &lp);
            ok(size == sizeof(LOGPEN),
                "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(lp.lopnStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, lp.lopnStyle);
            ok(lp.lopnWidth.x == pen[i].ret_width, "expected %u, got %ld\n", pen[i].ret_width, lp.lopnWidth.x);
            ok(lp.lopnWidth.y == 0, "expected 0, got %ld\n", lp.lopnWidth.y);
            ok(lp.lopnColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, lp.lopnColor);
            break;

        case PS_USERSTYLE:
            ok(!size /*&& GetLastError() == ERROR_INVALID_PARAMETER*/,
               "GetObject should fail: size %d, error %ld\n", size, GetLastError());
            size = GetObject(hpen, sizeof(ext_pen), &ext_pen.elp);
            ok(size == sizeof(EXTLOGPEN) - sizeof(elp.elpStyleEntry) + sizeof(user_style),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(ext_pen.elp.elpHatch == HS_CROSS, "expected HS_CROSS, got %p\n", (void *)ext_pen.elp.elpHatch);
            ok(ext_pen.elp.elpNumEntries == 2, "expected 0, got %lx\n", ext_pen.elp.elpNumEntries);
            ok(ext_pen.elp.elpStyleEntry[0] == 0xabc, "expected 0xabc, got %lx\n", ext_pen.elp.elpStyleEntry[0]);
            ok(ext_pen.elp.elpStyleEntry[1] == 0xdef, "expected 0xabc, got %lx\n", ext_pen.elp.elpStyleEntry[1]);
            break;

        default:
            ok(size == sizeof(EXTLOGPEN) - sizeof(elp.elpStyleEntry),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(ext_pen.elp.elpHatch == HS_CROSS, "expected HS_CROSS, got %p\n", (void *)ext_pen.elp.elpHatch);
            ok(ext_pen.elp.elpNumEntries == 0, "expected 0, got %lx\n", ext_pen.elp.elpNumEntries);
            break;
        }

        /* for some reason XP differenciates PS_NULL here */
        if (pen[i].style == PS_NULL)
            ok(ext_pen.elp.elpPenStyle == pen[i].ret_style, "expected %x, got %lx\n", pen[i].ret_style, ext_pen.elp.elpPenStyle);
        else
        {
if (pen[i].style == PS_USERSTYLE)
{
    todo_wine
            ok(ext_pen.elp.elpPenStyle == (PS_GEOMETRIC | pen[i].style), "expected %x, got %lx\n", PS_GEOMETRIC | pen[i].style, ext_pen.elp.elpPenStyle);
}
else
            ok(ext_pen.elp.elpPenStyle == (PS_GEOMETRIC | pen[i].style), "expected %x, got %lx\n", PS_GEOMETRIC | pen[i].style, ext_pen.elp.elpPenStyle);
        }

        if (pen[i].style == PS_NULL)
            ok(ext_pen.elp.elpWidth == 0, "expected 0, got %lx\n", ext_pen.elp.elpWidth);
        else
            ok(ext_pen.elp.elpWidth == pen[i].ret_width, "expected %u, got %lx\n", pen[i].ret_width, ext_pen.elp.elpWidth);
        ok(ext_pen.elp.elpColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, ext_pen.elp.elpColor);
        ok(ext_pen.elp.elpBrushStyle == BS_SOLID, "expected BS_SOLID, got %x\n", ext_pen.elp.elpBrushStyle);

        DeleteObject(hpen);
    }
}

static void test_bitmap(void)
{
    char buf[256], buf_cmp[256];
    HBITMAP hbmp, hbmp_old;
    HDC hdc;
    BITMAP bm;
    INT ret;

    hdc = CreateCompatibleDC(0);
    assert(hdc != 0);

    hbmp = CreateBitmap(15, 15, 1, 1, NULL);
    assert(hbmp != NULL);

    ret = GetObject(hbmp, sizeof(bm), &bm);
    ok(ret == sizeof(bm), "%d != %d\n", ret, sizeof(bm));

    ok(bm.bmType == 0, "wrong bm.bmType %d\n", bm.bmType);
    ok(bm.bmWidth == 15, "wrong bm.bmWidth %d\n", bm.bmWidth);
    ok(bm.bmHeight == 15, "wrong bm.bmHeight %d\n", bm.bmHeight);
    ok(bm.bmWidthBytes == 2, "wrong bm.bmWidthBytes %d\n", bm.bmWidthBytes);
    ok(bm.bmPlanes == 1, "wrong bm.bmPlanes %d\n", bm.bmPlanes);
    ok(bm.bmBitsPixel == 1, "wrong bm.bmBitsPixel %d\n", bm.bmBitsPixel);
    ok(bm.bmBits == NULL, "wrong bm.bmBits %p\n", bm.bmBits);

    assert(sizeof(buf) >= bm.bmWidthBytes * bm.bmHeight);
    assert(sizeof(buf) == sizeof(buf_cmp));

    memset(buf_cmp, 0xAA, sizeof(buf_cmp));
    memset(buf_cmp, 0, bm.bmWidthBytes * bm.bmHeight);

    memset(buf, 0xAA, sizeof(buf));
    ret = GetBitmapBits(hbmp, sizeof(buf), buf);
    ok(ret == bm.bmWidthBytes * bm.bmHeight, "%d != %d\n", ret, bm.bmWidthBytes * bm.bmHeight);
    ok(!memcmp(buf, buf_cmp, sizeof(buf)), "buffers do not match\n");

    hbmp_old = SelectObject(hdc, hbmp);

    ret = GetObject(hbmp, sizeof(bm), &bm);
    ok(ret == sizeof(bm), "%d != %d\n", ret, sizeof(bm));

    ok(bm.bmType == 0, "wrong bm.bmType %d\n", bm.bmType);
    ok(bm.bmWidth == 15, "wrong bm.bmWidth %d\n", bm.bmWidth);
    ok(bm.bmHeight == 15, "wrong bm.bmHeight %d\n", bm.bmHeight);
    ok(bm.bmWidthBytes == 2, "wrong bm.bmWidthBytes %d\n", bm.bmWidthBytes);
    ok(bm.bmPlanes == 1, "wrong bm.bmPlanes %d\n", bm.bmPlanes);
    ok(bm.bmBitsPixel == 1, "wrong bm.bmBitsPixel %d\n", bm.bmBitsPixel);
    ok(bm.bmBits == NULL, "wrong bm.bmBits %p\n", bm.bmBits);

    memset(buf, 0xAA, sizeof(buf));
    ret = GetBitmapBits(hbmp, sizeof(buf), buf);
    ok(ret == bm.bmWidthBytes * bm.bmHeight, "%d != %d\n", ret, bm.bmWidthBytes * bm.bmHeight);
    ok(!memcmp(buf, buf_cmp, sizeof(buf)), "buffers do not match\n");

    hbmp_old = SelectObject(hdc, hbmp_old);
    ok(hbmp_old == hbmp, "wrong old bitmap %p\n", hbmp_old);

    /* test various buffer sizes for GetObject */
    ret = GetObject(hbmp, sizeof(bm) * 2, &bm);
    ok(ret == sizeof(bm), "%d != %d\n", ret, sizeof(bm));

    ret = GetObject(hbmp, sizeof(bm) / 2, &bm);
todo_wine {
    ok(ret == 0, "%d != 0\n", ret);
}

    ret = GetObject(hbmp, 0, &bm);
    ok(ret == 0, "%d != 0\n", ret);

    ret = GetObject(hbmp, 1, &bm);
todo_wine {
    ok(ret == 0, "%d != 0\n", ret);
}

    DeleteObject(hbmp);
    DeleteDC(hdc);
}

START_TEST(gdiobj)
{
    test_logfont();
    test_logpen();
    test_bitmap();
    test_bitmap_font();
    test_bitmap_font_metrics();
    test_gdi_objects();
    test_GdiGetCharDimensions();
    test_text_extents();
    test_thread_objects();
    test_GetCurrentObject();
}
