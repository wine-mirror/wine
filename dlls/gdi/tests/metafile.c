/*
 * Unit tests for metafile functions
 *
 * Copyright (c) 2002 Dmitry Timoshkov
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

#include <assert.h>

#include "wine/test.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"

static LOGFONTA orig_lf;
static BOOL emr_processed = FALSE;

static int CALLBACK emf_enum_proc(HDC hdc, HANDLETABLE *handle_table,
    const ENHMETARECORD *emr, int n_objs, LPARAM param)
{
    static int n_record;
    DWORD i;
    INT *dx, *orig_dx = (INT *)param;
    LOGFONTA device_lf;

    trace("hdc %p, emr->iType %ld, emr->nSize %ld, param %p\n",
           hdc, emr->iType, emr->nSize, (void *)param);

    PlayEnhMetaFileRecord(hdc, handle_table, emr, n_objs);

    switch (emr->iType)
    {
    case EMR_HEADER:
        n_record = 0;
        break;

    case EMR_EXTTEXTOUTA:
    {
        EMREXTTEXTOUTA *emr_ExtTextOutA = (EMREXTTEXTOUTA *)emr;
        dx = (INT *)((char *)emr + emr_ExtTextOutA->emrtext.offDx);

        ok(GetObjectA(GetCurrentObject(hdc, OBJ_FONT), sizeof(device_lf), &device_lf) == sizeof(device_lf),
           "GetObjectA error %ld\n", GetLastError());

        /* compare up to lfOutPrecision, other values are not interesting,
         * and in fact sometimes arbitrary adapted by Win9x.
         */
        ok(!memcmp(&orig_lf, &device_lf, FIELD_OFFSET(LOGFONTA, lfOutPrecision)), "fonts don't match\n");
        ok(!lstrcmpA(orig_lf.lfFaceName, device_lf.lfFaceName), "font names don't match\n");

        for(i = 0; i < emr_ExtTextOutA->emrtext.nChars; i++)
        {
            ok(orig_dx[i] == dx[i], "pass %d: dx[%ld] (%d) didn't match %d\n",
                                     n_record, i, dx[i], orig_dx[i]);
        }
        n_record++;
        emr_processed = TRUE;
        break;
    }

    case EMR_EXTTEXTOUTW:
    {
        EMREXTTEXTOUTW *emr_ExtTextOutW = (EMREXTTEXTOUTW *)emr;
        dx = (INT *)((char *)emr + emr_ExtTextOutW->emrtext.offDx);

        ok(GetObjectA(GetCurrentObject(hdc, OBJ_FONT), sizeof(device_lf), &device_lf) == sizeof(device_lf),
           "GetObjectA error %ld\n", GetLastError());

        /* compare up to lfOutPrecision, other values are not interesting,
         * and in fact sometimes arbitrary adapted by Win9x.
         */
        ok(!memcmp(&orig_lf, &device_lf, FIELD_OFFSET(LOGFONTA, lfOutPrecision)), "fonts don't match\n");
        ok(!lstrcmpA(orig_lf.lfFaceName, device_lf.lfFaceName), "font names don't match\n");

        for(i = 0; i < emr_ExtTextOutW->emrtext.nChars; i++)
        {
            ok(orig_dx[i] == dx[i], "pass %d: dx[%ld] (%d) didn't match %d\n",
                                     n_record, i, dx[i], orig_dx[i]);
        }
        n_record++;
        emr_processed = TRUE;
        break;
    }

    default:
        break;
    }

    return 1;
}

static void test_ExtTextOut(void)
{
    HWND hwnd;
    HDC hdcDisplay, hdcMetafile;
    HENHMETAFILE hMetafile;
    HFONT hFont;
    static const char text[] = "Simple text to test ExtTextOut on metafiles";
    INT i, len, dx[256];
    static const RECT rc = { 0, 0, 100, 100 };

    assert(sizeof(dx)/sizeof(dx[0]) >= lstrlenA(text));

    /* Win9x doesn't play EMFs on invisible windows */
    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP | WS_VISIBLE,
                           0, 0, 200, 200, 0, 0, 0, NULL);
    ok(hwnd != 0, "CreateWindowExA error %ld\n", GetLastError());

    hdcDisplay = GetDC(hwnd);
    ok(hdcDisplay != 0, "GetDC error %ld\n", GetLastError());

    trace("hdcDisplay %p\n", hdcDisplay);

    SetMapMode(hdcDisplay, MM_TEXT);

    memset(&orig_lf, 0, sizeof(orig_lf));

    orig_lf.lfCharSet = ANSI_CHARSET;
    orig_lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    orig_lf.lfWeight = FW_DONTCARE;
    orig_lf.lfHeight = 7;
    orig_lf.lfQuality = DEFAULT_QUALITY;
    lstrcpyA(orig_lf.lfFaceName, "Arial");
    hFont = CreateFontIndirectA(&orig_lf);
    ok(hFont != 0, "CreateFontIndirectA error %ld\n", GetLastError());

    hFont = SelectObject(hdcDisplay, hFont);

    len = lstrlenA(text);
    for (i = 0; i < len; i++)
    {
        ok(GetCharWidthA(hdcDisplay, text[i], text[i], &dx[i]),
           "GetCharWidthA error %ld\n", GetLastError());
    }
    hFont = SelectObject(hdcDisplay, hFont);

    hdcMetafile = CreateEnhMetaFileA(hdcDisplay, NULL, NULL, NULL);
    ok(hdcMetafile != 0, "CreateEnhMetaFileA error %ld\n", GetLastError());

    trace("hdcMetafile %p\n", hdcMetafile);

    ok(GetDeviceCaps(hdcMetafile, TECHNOLOGY) == DT_RASDISPLAY,
       "GetDeviceCaps(TECHNOLOGY) has to return DT_RASDISPLAY for a display based EMF\n");

    hFont = SelectObject(hdcMetafile, hFont);

    /* 1. pass NULL lpDx */
    ok(ExtTextOutA(hdcMetafile, 0, 0, 0, &rc, text, lstrlenA(text), NULL),
       "ExtTextOutA error %ld\n", GetLastError());

    /* 2. pass custom lpDx */
    ok(ExtTextOutA(hdcMetafile, 0, 20, 0, &rc, text, lstrlenA(text), dx),
       "ExtTextOutA error %ld\n", GetLastError());

    hFont = SelectObject(hdcMetafile, hFont);
    ok(DeleteObject(hFont), "DeleteObject error %ld\n", GetLastError());

    hMetafile = CloseEnhMetaFile(hdcMetafile);
    ok(hMetafile != 0, "CloseEnhMetaFile error %ld\n", GetLastError());

    ok(!GetObjectType(hdcMetafile), "CloseEnhMetaFile has to destroy metafile hdc\n");

    ok(PlayEnhMetaFile(hdcDisplay, hMetafile, &rc), "PlayEnhMetaFile error %ld\n", GetLastError());

    ok(EnumEnhMetaFile(hdcDisplay, hMetafile, emf_enum_proc, dx, &rc),
       "EnumEnhMetaFile error %ld\n", GetLastError());

    ok(emr_processed, "EnumEnhMetaFile couldn't find EMR_EXTTEXTOUTA or EMR_EXTTEXTOUTW record\n");

    ok(!EnumEnhMetaFile(hdcDisplay, hMetafile, emf_enum_proc, dx, NULL),
       "A valid hdc has to require a valid rc\n");

    ok(DeleteEnhMetaFile(hMetafile), "DeleteEnhMetaFile error %ld\n", GetLastError());
    ok(ReleaseDC(hwnd, hdcDisplay), "ReleaseDC error %ld\n", GetLastError());
}

START_TEST(metafile)
{
    test_ExtTextOut();
}
