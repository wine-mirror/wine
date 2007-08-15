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

#include "objbase.h"

#include "gdiplus.h"
#include "gdiplus_private.h"

GpStatus WINGDIPAPI GdipCreateFontFromLogfontW(HDC hdc,
    GDIPCONST LOGFONTW *logfont, GpFont **font)
{
    if(!logfont || !font)
        return InvalidParameter;

    *font = GdipAlloc(sizeof(GpFont));
    if(!*font)  return OutOfMemory;

    memcpy(&(*font)->lfw, logfont, sizeof(LOGFONTW));

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateFontFromLogfontA(HDC hdc,
    GDIPCONST LOGFONTA *lfa, GpFont **font)
{
    LOGFONTW lfw;

    if(!lfa || !font)
        return InvalidParameter;

    memcpy(&lfw, lfa, sizeof(LOGFONTA));

    if(!MultiByteToWideChar(CP_ACP, 0, lfa->lfFaceName, -1, lfw.lfFaceName, LF_FACESIZE))
        return GenericError;

    GdipCreateFontFromLogfontW(hdc, &lfw, font);

    return Ok;
}

/* FIXME: use graphics */
GpStatus WINGDIPAPI GdipGetLogFontW(GpFont *font, GpGraphics *graphics,
    LOGFONTW *lfw)
{
    if(!font || !graphics || !lfw)
        return InvalidParameter;

    memcpy(lfw, &font->lfw, sizeof(LOGFONTW));

    return Ok;
}
