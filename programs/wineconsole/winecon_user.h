/*
 * an application for displaying Win32 console
 * USER32 backend
 *
 * Copyright 2001 Eric Pouech
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
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include "winecon_private.h"

struct inner_data_user {
    /* the following fields are only user by the USER backend (should be hidden in user) */
    HFONT		hFont;		/* font used for rendering, usually fixed */
    LONG                ext_leading;    /* external leading for hFont */
    HDC			hMemDC;		/* memory DC holding the bitmap below */
    HBITMAP		hBitmap;	/* bitmap of display window content */
    HMENU               hPopMenu;       /* popup menu triggered by right mouse click */

    HBITMAP		cursor_bitmap;  /* bitmap used for the caret */
    BOOL                has_selection;  /* an area is being selected (selectPt[12] are edges of the rect) */
    COORD		selectPt1;	/* start (and end) point of a mouse selection */
    COORD		selectPt2;
};

#define PRIVATE(data)   ((struct inner_data_user*)((data)->private))

/* from user.c */
extern const COLORREF WCUSER_ColorMap[16];
extern BOOL WCUSER_GetProperties(struct inner_data*, BOOL);
extern BOOL WCUSER_ValidateFont(const struct inner_data* data, const LOGFONT* lf);
extern BOOL WCUSER_ValidateFontMetric(const struct inner_data* data,
                                      const TEXTMETRIC* tm, DWORD fontType);
extern BOOL WCUSER_AreFontsEqual(const struct config_data* config,
                                 const LOGFONT* lf);
extern HFONT WCUSER_CopyFont(struct config_data* config, HWND hWnd, const LOGFONT* lf,
                             LONG* el);
extern void WCUSER_FillLogFont(LOGFONT* lf, const WCHAR* name,
                               UINT height, UINT weight);

extern void WCUSER_DumpLogFont(const char* pfx, const LOGFONT* lf, DWORD ft);
extern void WCUSER_DumpTextMetric(const TEXTMETRIC* tm, DWORD ft);
