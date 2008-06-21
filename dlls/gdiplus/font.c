/*
 * Copyright (C) 2007 Google (Evan Stade)
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
#include "winnls.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL (gdiplus);

#include "objbase.h"

#include "gdiplus.h"
#include "gdiplus_private.h"

GpStatus WINGDIPAPI GdipCreateFontFromLogfontW(HDC hdc,
    GDIPCONST LOGFONTW *logfont, GpFont **font)
{
    HFONT hfont, oldfont;
    TEXTMETRICW textmet;

    if(!logfont || !font)
        return InvalidParameter;

    *font = GdipAlloc(sizeof(GpFont));
    if(!*font)  return OutOfMemory;

    memcpy(&(*font)->lfw.lfFaceName, logfont->lfFaceName, LF_FACESIZE *
           sizeof(WCHAR));
    (*font)->lfw.lfHeight = logfont->lfHeight;
    (*font)->lfw.lfItalic = logfont->lfItalic;
    (*font)->lfw.lfUnderline = logfont->lfUnderline;
    (*font)->lfw.lfStrikeOut = logfont->lfStrikeOut;

    hfont = CreateFontIndirectW(&(*font)->lfw);
    oldfont = SelectObject(hdc, hfont);
    GetTextMetricsW(hdc, &textmet);

    (*font)->lfw.lfHeight = -textmet.tmHeight;
    (*font)->lfw.lfWeight = textmet.tmWeight;

    SelectObject(hdc, oldfont);
    DeleteObject(hfont);

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateFontFromLogfontA(HDC hdc,
    GDIPCONST LOGFONTA *lfa, GpFont **font)
{
    LOGFONTW lfw;

    if(!lfa || !font)
        return InvalidParameter;

    memcpy(&lfw, lfa, FIELD_OFFSET(LOGFONTA,lfFaceName) );

    if(!MultiByteToWideChar(CP_ACP, 0, lfa->lfFaceName, -1, lfw.lfFaceName, LF_FACESIZE))
        return GenericError;

    GdipCreateFontFromLogfontW(hdc, &lfw, font);

    return Ok;
}

GpStatus WINGDIPAPI GdipDeleteFont(GpFont* font)
{
    if(!font)
        return InvalidParameter;

    GdipFree(font);

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateFontFromDC(HDC hdc, GpFont **font)
{
    HFONT hfont;
    LOGFONTW lfw;

    if(!font)
        return InvalidParameter;

    hfont = (HFONT)GetCurrentObject(hdc, OBJ_FONT);
    if(!hfont)
        return GenericError;

    if(!GetObjectW(hfont, sizeof(LOGFONTW), &lfw))
        return GenericError;

    return GdipCreateFontFromLogfontW(hdc, &lfw, font);
}

/* FIXME: use graphics */
GpStatus WINGDIPAPI GdipGetLogFontW(GpFont *font, GpGraphics *graphics,
    LOGFONTW *lfw)
{
    if(!font || !graphics || !lfw)
        return InvalidParameter;

    *lfw = font->lfw;

    return Ok;
}

GpStatus WINGDIPAPI GdipCloneFont(GpFont *font, GpFont **cloneFont)
{
    if(!font || !cloneFont)
        return InvalidParameter;

    *cloneFont = GdipAlloc(sizeof(GpFont));
    if(!*cloneFont)    return OutOfMemory;

    **cloneFont = *font;

    return Ok;
}

/* Borrowed from GDI32 */
static INT CALLBACK is_font_installed_proc(const LOGFONTW *elf,
                            const TEXTMETRICW *ntm, DWORD type, LPARAM lParam)
{
    return 0;
}

static BOOL is_font_installed(const WCHAR *name)
{
    HDC hdc = GetDC(0);
    BOOL ret = FALSE;

    if(!EnumFontFamiliesW(hdc, name, is_font_installed_proc, 0))
        ret = TRUE;

    ReleaseDC(0, hdc);
    return ret;
}

/*******************************************************************************
 * GdipCreateFontFamilyFromName [GDIPLUS.@]
 *
 * Creates a font family object based on a supplied name
 *
 * PARAMS
 *  name               [I] Name of the font
 *  fontCollection     [I] What font collection (if any) the font belongs to (may be NULL)
 *  FontFamily         [O] Pointer to the resulting FontFamily object
 *
 * RETURNS
 *  SUCCESS: Ok
 *  FAILURE: FamilyNotFound if the requested FontFamily does not exist on the system
 *  FAILURE: Invalid parameter if FontFamily or name is NULL
 *
 * NOTES
 *   If fontCollection is NULL then the object is not part of any collection
 *
 */

GpStatus WINGDIPAPI GdipCreateFontFamilyFromName(GDIPCONST WCHAR *name,
                                        GpFontCollection *fontCollection,
                                        GpFontFamily **FontFamily)
{
    GpFontFamily* ffamily;
    HDC hdc;
    HFONT hFont;
    LOGFONTW lfw;

    TRACE("%s, %p %p\n", debugstr_w(name), fontCollection, FontFamily);

    if (!(name && FontFamily))
        return InvalidParameter;
    if (fontCollection)
        FIXME("No support for FontCollections yet!\n");
    if (!is_font_installed(name))
        return FontFamilyNotFound;

    ffamily = GdipAlloc(sizeof (GpFontFamily));
    if (!ffamily) return OutOfMemory;
    ffamily->tmw = GdipAlloc(sizeof (TEXTMETRICW));
    if (!ffamily->tmw) {GdipFree (ffamily); return OutOfMemory;}

    hdc = GetDC(0);
    lstrcpynW(lfw.lfFaceName, name, sizeof(WCHAR) * LF_FACESIZE);
    hFont = CreateFontIndirectW (&lfw);
    SelectObject(hdc, hFont);

    GetTextMetricsW(hdc, ffamily->tmw);

    ffamily->FamilyName = GdipAlloc(LF_FACESIZE * sizeof (WCHAR));
    if (!ffamily->FamilyName)
    {
        GdipFree(ffamily);
        return OutOfMemory;
    }

    lstrcpynW(ffamily->FamilyName, name, sizeof(WCHAR) * LF_FACESIZE);

    *FontFamily = ffamily;
    ReleaseDC(0, hdc);

    return Ok;
}
