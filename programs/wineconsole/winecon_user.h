/*
 * an application for displaying Win32 console
 * USER32 backend
 *
 * Copyright 2001 Eric Pouech
 */

#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include "winecon_private.h"

struct inner_data_user {
    /* the following fields are only user by the USER backend (should be hidden in user) */
    HWND		hWnd;		/* handle to windows for rendering */
    HFONT		hFont;		/* font used for rendering, usually fixed */
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
extern COLORREF	WCUSER_ColorMap[16];
extern BOOL WCUSER_GetProperties(struct inner_data*, BOOL);
extern BOOL WCUSER_SetFont(struct inner_data* data, const LOGFONT* font);
extern BOOL WCUSER_ValidateFont(const struct inner_data* data, const LOGFONT* lf);
extern BOOL WCUSER_ValidateFontMetric(const struct inner_data* data, const TEXTMETRIC* tm);
extern BOOL WCUSER_AreFontsEqual(const struct config_data* config, const LOGFONT* lf);
extern void WCUSER_CopyFont(struct config_data* config, const LOGFONT* lf);

