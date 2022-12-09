/*
 * COMMDLG - Font Dialog
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
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

#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "commdlg.h"
#include "wine/debug.h"
#include "cderr.h"
#include "cdlg16.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

/***********************************************************************
 *                FontFamilyEnumProc                     (COMMDLG.19)
 */
INT16 WINAPI FontFamilyEnumProc16( SEGPTR logfont, SEGPTR metrics,
                                   UINT16 nFontType, LPARAM lParam )
{
    FIXME( "%08lx %08lx %u %Ix\n", logfont, metrics, nFontType, lParam );
    return 0;
}

/***********************************************************************
 *                 FontStyleEnumProc                     (COMMDLG.18)
 */
INT16 WINAPI FontStyleEnumProc16( SEGPTR logfont, SEGPTR metrics,
                                  UINT16 nFontType, LPARAM lParam )
{
    FIXME( "%08lx %08lx %u %Ix\n", logfont, metrics, nFontType, lParam );
    return 0;
}

/***********************************************************************
 *                        ChooseFont   (COMMDLG.15)
 */
BOOL16 WINAPI ChooseFont16(LPCHOOSEFONT16 lpChFont)
{
    CHOOSEFONTA cf32;
    LOGFONTA lf32;
    LOGFONT16 *font16;

    if (!lpChFont) return FALSE;
    font16 = MapSL(lpChFont->lpLogFont);

    cf32.lStructSize = sizeof(CHOOSEFONTW);
    cf32.hwndOwner = HWND_32(lpChFont->hwndOwner);
    cf32.hDC = HDC_32(lpChFont->hDC);
    cf32.iPointSize = lpChFont->iPointSize;
    cf32.Flags = lpChFont->Flags & ~(CF_ENABLETEMPLATEHANDLE | CF_ENABLETEMPLATE);
    cf32.rgbColors = lpChFont->rgbColors;
    cf32.lCustData = lpChFont->lCustData;
    cf32.lpfnHook = NULL;
    cf32.hInstance = GetModuleHandleA("comdlg32.dll");
    cf32.nFontType = lpChFont->nFontType;
    cf32.nSizeMax = lpChFont->nSizeMax;
    cf32.nSizeMin = lpChFont->nSizeMin;
    cf32.lpLogFont = &lf32;

    lf32.lfHeight = font16->lfHeight;
    lf32.lfWidth = font16->lfWidth;
    lf32.lfEscapement = font16->lfEscapement;
    lf32.lfOrientation = font16->lfOrientation;
    lf32.lfWeight = font16->lfWeight;
    lf32.lfItalic = font16->lfItalic;
    lf32.lfUnderline = font16->lfUnderline;
    lf32.lfStrikeOut = font16->lfStrikeOut;
    lf32.lfCharSet = font16->lfCharSet;
    lf32.lfOutPrecision = font16->lfOutPrecision;
    lf32.lfClipPrecision = font16->lfClipPrecision;
    lf32.lfQuality = font16->lfQuality;
    lf32.lfPitchAndFamily = font16->lfPitchAndFamily;
    lstrcpynA( lf32.lfFaceName, font16->lfFaceName, LF_FACESIZE );

    if (lpChFont->Flags & (CF_ENABLETEMPLATEHANDLE | CF_ENABLETEMPLATE))
        FIXME( "custom templates no longer supported, using default\n" );

    if (lpChFont->lpfnHook)
        FIXME( "custom hook %p no longer supported\n", lpChFont->lpfnHook );

    if (!ChooseFontA( &cf32 )) return FALSE;

    lpChFont->iPointSize = cf32.iPointSize;
    lpChFont->Flags = cf32.Flags;
    lpChFont->rgbColors = cf32.rgbColors;
    lpChFont->lCustData = cf32.lCustData;
    lpChFont->nFontType = cf32.nFontType;

    font16->lfHeight = lf32.lfHeight;
    font16->lfWidth = lf32.lfWidth;
    font16->lfEscapement = lf32.lfEscapement;
    font16->lfOrientation = lf32.lfOrientation;
    font16->lfWeight = lf32.lfWeight;
    font16->lfItalic = lf32.lfItalic;
    font16->lfUnderline = lf32.lfUnderline;
    font16->lfStrikeOut = lf32.lfStrikeOut;
    font16->lfCharSet = lf32.lfCharSet;
    font16->lfOutPrecision = lf32.lfOutPrecision;
    font16->lfClipPrecision = lf32.lfClipPrecision;
    font16->lfQuality = lf32.lfQuality;
    font16->lfPitchAndFamily = lf32.lfPitchAndFamily;
    lstrcpynA( font16->lfFaceName, lf32.lfFaceName, LF_FACESIZE );
    return TRUE;
}

/***********************************************************************
 *           FormatCharDlgProc   (COMMDLG.16)
             FIXME: 1. some strings are "hardcoded", but it's better load from sysres
                    2. some CF_.. flags are not supported
                    3. some TType extensions
 */
BOOL16 CALLBACK FormatCharDlgProc16(HWND16 hDlg16, UINT16 message,
				   WPARAM16 wParam, LPARAM lParam)
{
    FIXME( "%04x %04x %04x %08Ix: stub\n", hDlg16, message, wParam, lParam );
    return FALSE;
}
