/*
 * GDI font definitions
 *
 * Copyright 1994 Alexandre Julliard
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

#ifndef __WINE_FONT_H
#define __WINE_FONT_H

#include "gdi.h"

extern BOOL FONT_Init( UINT16* pTextCaps );
extern LPWSTR FONT_mbtowc(HDC, LPCSTR, INT, INT*, UINT*);

extern INT WineEngAddFontResourceEx(LPCWSTR, DWORD, PVOID);
extern GdiFont WineEngCreateFontInstance(DC*, HFONT);
extern BOOL WineEngDestroyFontInstance(HFONT handle);
extern DWORD WineEngEnumFonts(LPLOGFONTW, DEVICEFONTENUMPROC, LPARAM);
extern BOOL WineEngGetCharWidth(GdiFont, UINT, UINT, LPINT);
extern DWORD WineEngGetFontData(GdiFont, DWORD, DWORD, LPVOID, DWORD);
extern DWORD WineEngGetGlyphIndices(GdiFont font, LPCWSTR lpstr, INT count,
				    LPWORD pgi, DWORD flags);
extern DWORD WineEngGetGlyphOutline(GdiFont, UINT glyph, UINT format,
				    LPGLYPHMETRICS, DWORD buflen, LPVOID buf,
				    const MAT2*);
extern UINT WineEngGetOutlineTextMetrics(GdiFont, UINT, LPOUTLINETEXTMETRICW);
extern BOOL WineEngGetTextExtentPoint(GdiFont, LPCWSTR, INT, LPSIZE);
extern BOOL WineEngGetTextExtentPointI(GdiFont, const WORD *, INT, LPSIZE);
extern INT  WineEngGetTextFace(GdiFont, INT, LPWSTR);
extern BOOL WineEngGetTextMetrics(GdiFont, LPTEXTMETRICW);
extern BOOL WineEngInit(void);
extern BOOL WineEngRemoveFontResourceEx(LPCWSTR, DWORD, PVOID);

#endif /* __WINE_FONT_H */
