/*
 * Copyright 2016 Nikolay Sivov for CodeWeavers
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "t2embapi.h"
#include "wine/test.h"

static int CALLBACK enum_font_proc(ENUMLOGFONTEXA *enumlf, NEWTEXTMETRICEXA *ntm, DWORD type, LPARAM lParam)
{
    OUTLINETEXTMETRICA otm;
    HDC hdc = GetDC(NULL);
    HFONT hfont, old_font;
    ULONG status;
    LONG ret;

    hfont = CreateFontIndirectA(&enumlf->elfLogFont);
    old_font = SelectObject(hdc, hfont);

    otm.otmSize = sizeof(otm);
    if (GetOutlineTextMetricsA(hdc, otm.otmSize, &otm))
    {
        ULONG expected = 0xffff;
        UINT fsType = otm.otmfsType & 0xf;

        ret = TTGetEmbeddingType(hdc, &status);
        ok(ret == E_NONE || ret == E_NOTATRUETYPEFONT, "Unexpected return value %#x.\n", ret);

        if (ret == E_NONE)
        {
            if (fsType == LICENSE_INSTALLABLE)
                expected = EMBED_INSTALLABLE;
            else if (fsType & LICENSE_EDITABLE)
                expected = EMBED_EDITABLE;
            else if (fsType & LICENSE_PREVIEWPRINT)
                expected = EMBED_PREVIEWPRINT;
            else if (fsType & LICENSE_NOEMBEDDING)
                expected = EMBED_NOEMBEDDING;

            ok(expected == status, "%s: status %d, expected %d, fsType %#x\n", enumlf->elfLogFont.lfFaceName, status,
                    expected, otm.otmfsType);
        }
    }
    else
    {
        status = 0xdeadbeef;
        ret = TTGetEmbeddingType(hdc, &status);
        ok(ret == E_NOTATRUETYPEFONT, "%s: got %d\n", enumlf->elfLogFont.lfFaceName, ret);
        ok(status == 0xdeadbeef, "%s: got status %d\n", enumlf->elfLogFont.lfFaceName, status);
    }

    SelectObject(hdc, old_font);
    DeleteObject(hfont);
    ReleaseDC(NULL, hdc);

    return 1;
}

static void test_TTGetEmbeddingType(void)
{
    HFONT hfont, old_font;
    LOGFONTA logfont;
    ULONG status;
    LONG ret;
    HDC hdc;

    ret = TTGetEmbeddingType(NULL, NULL);
    ok(ret == E_HDCINVALID, "got %d\n", ret);

    status = 0xdeadbeef;
    ret = TTGetEmbeddingType(NULL, &status);
    ok(ret == E_HDCINVALID, "got %#x\n", ret);
    ok(status == 0xdeadbeef, "got %u\n", status);

    status = 0xdeadbeef;
    ret = TTGetEmbeddingType((HDC)0xdeadbeef, &status);
    ok(ret == E_NOTATRUETYPEFONT || broken(ret == E_ERRORACCESSINGFONTDATA) /* xp, vista */, "got %#x\n", ret);
    ok(status == 0xdeadbeef, "got %u\n", status);

    hdc = CreateCompatibleDC(0);

    ret = TTGetEmbeddingType(hdc, NULL);
    ok(ret == E_NOTATRUETYPEFONT, "got %#x\n", ret);

    status = 0xdeadbeef;
    ret = TTGetEmbeddingType(hdc, &status);
    ok(ret == E_NOTATRUETYPEFONT, "got %#x\n", ret);
    ok(status == 0xdeadbeef, "got %u\n", status);

    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWeight = FW_NORMAL;
    strcpy(logfont.lfFaceName, "Tahoma");
    hfont = CreateFontIndirectA(&logfont);
    ok(hfont != NULL, "got %p\n", hfont);

    old_font = SelectObject(hdc, hfont);

    status = 0;
    ret = TTGetEmbeddingType(hdc, &status);
    ok(ret == E_NONE, "got %#x\n", ret);
    ok(status != 0, "got %u\n", status);

    ret = TTGetEmbeddingType(hdc, NULL);
    ok(ret == E_PERMISSIONSINVALID, "got %#x\n", ret);

    SelectObject(hdc, old_font);
    DeleteObject(hfont);

    /* repeat for all system fonts */
    logfont.lfCharSet = DEFAULT_CHARSET;
    logfont.lfFaceName[0] = 0;
    EnumFontFamiliesExA(hdc, &logfont, (FONTENUMPROCA)enum_font_proc, 0, 0);

    DeleteDC(hdc);
}

static void test_TTIsEmbeddingEnabledForFacename(void)
{
    BOOL status;
    LONG ret;

    ret = TTIsEmbeddingEnabledForFacename(NULL, NULL);
    ok(ret == E_FACENAMEINVALID, "got %#x\n", ret);

    status = 123;
    ret = TTIsEmbeddingEnabledForFacename(NULL, &status);
    ok(ret == E_FACENAMEINVALID, "got %#x\n", ret);
    ok(status == 123, "got %d\n", status);

    ret = TTIsEmbeddingEnabledForFacename("Arial", NULL);
    ok(ret == E_PBENABLEDINVALID, "got %#x\n", ret);

    status = 123;
    ret = TTIsEmbeddingEnabledForFacename("Arial", &status);
    ok(ret == E_NONE, "got %#x\n", ret);
    ok(status != 123, "got %d\n", status);
}

static void test_TTIsEmbeddingEnabled(void)
{
    HFONT old_font, hfont;
    LONG ret, status;
    LOGFONTA logfont;
    HDC hdc;

    ret = TTIsEmbeddingEnabled(NULL, NULL);
    ok(ret == E_HDCINVALID, "got %#x\n", ret);

    status = 123;
    ret = TTIsEmbeddingEnabled(NULL, &status);
    ok(ret == E_HDCINVALID, "got %#x\n", ret);
    ok(status == 123, "got %d\n", status);

    hdc = CreateCompatibleDC(0);

    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = 12;
    logfont.lfWeight = FW_NORMAL;
    strcpy(logfont.lfFaceName, "Tahoma");
    hfont = CreateFontIndirectA(&logfont);
    ok(hfont != NULL, "got %p\n", hfont);

    old_font = SelectObject(hdc, hfont);

    ret = TTIsEmbeddingEnabled(hdc, NULL);
    ok(ret == E_PBENABLEDINVALID, "Unexpected return value %#x.\n", ret);

    status = 123;
    ret = TTIsEmbeddingEnabled(hdc, &status);
    ok(ret == E_NONE, "got %#x\n", ret);
    ok(status != 123, "got %u\n", status);

    SelectObject(hdc, old_font);
    DeleteObject(hfont);
    DeleteDC(hdc);
}

START_TEST(t2embed)
{
    test_TTGetEmbeddingType();
    test_TTIsEmbeddingEnabledForFacename();
    test_TTIsEmbeddingEnabled();
}
