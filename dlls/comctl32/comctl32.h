/******************************************************************************
 *
 * Common definitions (resource ids and global variables)
 *
 * Copyright 1999 Thuy Nguyen
 * Copyright 1999 Eric Kohl
 * Copyright 2002 Dimitrie O. Paun
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

#ifndef __WINE_COMCTL32_H
#define __WINE_COMCTL32_H

#include "commctrl.h"

extern HMODULE COMCTL32_hModule;
extern HBRUSH  COMCTL32_hPattern55AABrush;

/* Property sheet / Wizard */
#define IDD_PROPSHEET 1006
#define IDD_WIZARD    1020

#define IDC_TABCONTROL   12320
#define IDC_APPLY_BUTTON 12321
#define IDC_BACK_BUTTON  12323
#define IDC_NEXT_BUTTON  12324
#define IDC_FINISH_BUTTON 12325
#define IDC_SUNKEN_LINE   12326

#define IDS_CLOSE	  4160

/* Toolbar customization dialog */
#define IDD_TBCUSTOMIZE     200

#define IDC_AVAILBTN_LBOX   201
#define IDC_RESET_BTN       202
#define IDC_TOOLBARBTN_LBOX 203
#define IDC_REMOVE_BTN      204
#define IDC_HELP_BTN        205
#define IDC_MOVEUP_BTN      206
#define IDC_MOVEDN_BTN      207

#define IDS_SEPARATOR      1024

/* Toolbar imagelist bitmaps */
#define IDB_STD_SMALL       120
#define IDB_STD_LARGE       121
#define IDB_VIEW_SMALL      124
#define IDB_VIEW_LARGE      125
#define IDB_HIST_SMALL      130
#define IDB_HIST_LARGE      131


/* Month calendar month menu popup */
#define IDD_MCMONTHMENU     300

#define IDM_JAN				301
#define IDM_FEB				302
#define IDM_MAR				303
#define IDM_APR				304
#define IDM_MAY				305
#define IDM_JUN				306
#define IDM_JUL				307
#define IDM_AUG				308
#define IDM_SEP				309
#define IDM_OCT				310
#define IDM_NOV				311
#define IDM_DEC				312

#define IDM_TODAY                      4163
#define IDM_GOTODAY                    4164

/* Treeview Checkboxes */

#define IDT_CHECK        401


/* Header cursors */
#define IDC_DIVIDER                     106
#define IDC_DIVIDEROPEN                 107


/* DragList icon */
#define IDI_DRAGARROW                   150


/* HOTKEY internal strings */
#define HKY_NONE                        2048

typedef struct
{
    COLORREF clrBtnHighlight;       /* COLOR_BTNHIGHLIGHT                  */
    COLORREF clrBtnShadow;          /* COLOR_BTNSHADOW                     */
    COLORREF clrBtnText;            /* COLOR_BTNTEXT                       */
    COLORREF clrBtnFace;            /* COLOR_BTNFACE                       */
    COLORREF clrHighlight;          /* COLOR_HIGHLIGHT                     */
    COLORREF clrHighlightText;      /* COLOR_HIGHLIGHTTEXT                 */
    COLORREF clr3dHilight;          /* COLOR_3DHILIGHT                     */
    COLORREF clr3dShadow;           /* COLOR_3DSHADOW                      */
    COLORREF clr3dDkShadow;         /* COLOR_3DDKSHADOW                    */
    COLORREF clr3dFace;             /* COLOR_3DFACE                        */
    COLORREF clrWindow;             /* COLOR_WINDOW                        */
    COLORREF clrWindowText;         /* COLOR_WINDOWTEXT                    */
    COLORREF clrGrayText;           /* COLOR_GREYTEXT                      */
    COLORREF clrActiveCaption;      /* COLOR_ACTIVECAPTION                 */
    COLORREF clrInfoBk;             /* COLOR_INFOBK                        */
    COLORREF clrInfoText;           /* COLOR_INFOTEXT                      */
} COMCTL32_SysColor;

extern COMCTL32_SysColor  comctl32_color;

/* Internal function */
HWND COMCTL32_CreateToolTip (HWND);
VOID COMCTL32_RefreshSysColors(void);
INT  Str_GetPtrWtoA (LPCWSTR lpSrc, LPSTR lpDest, INT nMaxLen);
BOOL Str_SetPtrAtoW (LPWSTR *lppDest, LPCSTR lpSrc);

#define COMCTL32_VERSION_MINOR 0
#define WINE_FILEVERSION 5, COMCTL32_VERSION_MINOR, 0, 0
#define WINE_FILEVERSIONSTR "5.00"

/* Notification support */

inline static LRESULT send_notify(HWND hwnd, UINT code, NMHDR *hdr)
{
    hdr->hwndFrom = hwnd;
    hdr->idFrom = GetWindowLongW (hwnd, GWL_ID);
    hdr->code = code;

    return SendMessageW (GetParent(hwnd), WM_NOTIFY, hdr->idFrom, (LPARAM)hdr);
}


inline static LRESULT hwnd_notify(HWND hwnd, UINT code)
{
    NMHDR hdr;

    return send_notify(hwnd, code, &hdr);
}

inline static BOOL hwnd_notify_char(HWND hwnd, UINT ch, DWORD prev, DWORD next)
{
    NMCHAR nmch;

    nmch.ch = ch;
    nmch.dwItemPrev = prev;
    nmch.dwItemNext = next;
    return (BOOL)send_notify(hwnd, NM_CHAR, &nmch.hdr);
}

inline static BOOL hwnd_notify_keydown(HWND hwnd, UINT nVKey, UINT uFlags)
{
    NMKEY nmk;

    nmk.nVKey = nVKey;
    nmk.uFlags = uFlags;
    return (BOOL)send_notify(hwnd, NM_KEYDOWN, &nmk.hdr);
}

inline static DWORD hwnd_notify_mouse(HWND hwnd, UINT code, DWORD spec, DWORD data, POINT *pt, LPARAM dwHitInfo)
{
    NMMOUSE nmm;

    nmm.dwItemSpec = spec;
    nmm.dwItemData = data;
    nmm.pt.x = pt->x;
    nmm.pt.y = pt->y;
    nmm.dwHitInfo = dwHitInfo;
    return send_notify(hwnd, code, &nmm.hdr);
}

#define DEFINE_CHAR_NOTIFICATION(CTRLINFO, hwndSelf) \
    inline static BOOL notify_char(CTRLINFO *infoPtr, UINT ch, DWORD prev, DWORD next) \
	{ return hwnd_notify_char(infoPtr->hwndSelf, ch, prev, next); }

#define DEFINE_CLICK_NOTIFICATION(CTRLINFO, hwndSelf) \
    inline static void notify_click(CTRLINFO *infoPtr) \
	{ hwnd_notify(infoPtr->hwndSelf, NM_CLICK); }

#define DEFINE_DBLCLK_NOTIFICATION(CTRLINFO, hwndSelf) \
    inline static void notify_dblclk(CTRLINFO *infoPtr) \
	{ hwnd_notify(infoPtr->hwndSelf, NM_DBLCLK); }

#define DEFINE_HOVER_NOTIFICATION(CTRLINFO, hwndSelf) \
    inline static BOOL notify_hover(CTRLINFO *infoPtr) \
	{ return hwnd_notify(infoPtr->hwndSelf, NM_HOVER); }

#define DEFINE_KEYDOWN_NOTIFICATION(CTRLINFO, hwndSelf) \
    inline static BOOL notify_keydown(CTRLINFO *infoPtr, UINT nVKey, UINT uFlags) \
	{ return hwnd_notify_keydown(infoPtr->hwndSelf, nVKey, uFlags); }

#define DEFINE_KILLFOCUS_NOTIFICATION(CTRLINFO, hwndSelf) \
    inline static void notify_killfocus(CTRLINFO *infoPtr) \
	{ hwnd_notify(infoPtr->hwndSelf, NM_KILLFOCUS); }

#define DEFINE_NCHITTEST_NOTIFICATION(CTRLINFO, hwndSelf) \
    inline static DWORD notify_nchittest(CTRLINFO *infoPtr, DWORD spec, DWORD data, POINT *pt, LPARAM dwHitInfo) \
	{ return hwnd_notify_mouse(infoPtr->hwndSelf, NM_NCHITTEST, spec, data, pt, dwHitInfo); }

#define DEFINE_OUTOFMEMORY_NOTIFICATION(CTRLINFO, hwndSelf) \
    inline static void notify_outofmemory(CTRLINFO *infoPtr) \
	{ hwnd_notify(infoPtr->hwndSelf, NM_OUTOFMEMORY); }

#define DEFINE_RCLICK_NOTIFICATION(CTRLINFO, hwndSelf) \
    inline static BOOL notify_rclick(CTRLINFO *infoPtr) \
	{ return hwnd_notify(infoPtr->hwndSelf, NM_RCLICK); }

#define DEFINE_RDBLCLK_NOTIFICATION(CTRLINFO, hwndSelf) \
    inline static void notify_rdblclk(CTRLINFO *infoPtr) \
	{ hwnd_notify(infoPtr->hwndSelf, NM_RDBLCLK); }

#define DEFINE_RELEASEDCAPTURE_NOTIFICATION(CTRLINFO, hwndSelf) \
    inline static void notify_releasedcapture(CTRLINFO *infoPtr) \
	{ hwnd_notify(infoPtr->hwndSelf, NM_RELEASEDCAPTURE); }

#define DEFINE_RETURN_NOTIFICATION(CTRLINFO, hwndSelf) \
    inline static void notify_return(CTRLINFO *infoPtr) \
	{ hwnd_notify(infoPtr->hwndSelf, NM_RETURN); }

#define DEFINE_SETCURSOR_NOTIFICATION(CTRLINFO, hwndSelf) \
    inline static BOOL notify_setcursor(CTRLINFO *infoPtr, DWORD spec, DWORD data, POINT *pt, LPARAM dwHitInfo) \
	{ return hwnd_notify_mouse(infoPtr->hwndSelf, NM_SETCURSOR, spec, data, pt, dwHitInfo); }

#define DEFINE_SETFOCUS_NOTIFICATION(CTRLINFO, hwndSelf) \
    inline static void notify_setfocus(CTRLINFO *infoPtr) \
	{ hwnd_notify(infoPtr->hwndSelf, NM_SETFOCUS); }

#define DEFINE_TOOLTIPSCREATED_NOTIFICATION(CTRLINFO, hwndSelf) \
    inline static void notify_tooltipscreated(CTRLINFO *infoPtr) \
	{ hwnd_notify(infoPtr->hwndSelf, NM_TOOLTIPSCREATED); }

#define DEFINE_COMMON_NOTIFICATIONS(CTRLINFO, hwndSelf) \
    DEFINE_CHAR_NOTIFICATION(CTRLINFO, hwndSelf) \
    DEFINE_CLICK_NOTIFICATION(CTRLINFO, hwndSelf) \
    DEFINE_DBLCLK_NOTIFICATION(CTRLINFO, hwndSelf) \
    DEFINE_HOVER_NOTIFICATION(CTRLINFO, hwndSelf) \
    DEFINE_KEYDOWN_NOTIFICATION(CTRLINFO, hwndSelf) \
    DEFINE_KILLFOCUS_NOTIFICATION(CTRLINFO, hwndSelf) \
    DEFINE_NCHITTEST_NOTIFICATION(CTRLINFO, hwndSelf) \
    DEFINE_OUTOFMEMORY_NOTIFICATION(CTRLINFO, hwndSelf) \
    DEFINE_RCLICK_NOTIFICATION(CTRLINFO, hwndSelf) \
    DEFINE_RDBLCLK_NOTIFICATION(CTRLINFO, hwndSelf) \
    DEFINE_RELEASEDCAPTURE_NOTIFICATION(CTRLINFO, hwndSelf) \
    DEFINE_RETURN_NOTIFICATION(CTRLINFO, hwndSelf) \
    DEFINE_SETCURSOR_NOTIFICATION(CTRLINFO, hwndSelf) \
    DEFINE_SETFOCUS_NOTIFICATION(CTRLINFO, hwndSelf) \
    DEFINE_TOOLTIPSCREATED_NOTIFICATION(CTRLINFO, hwndSelf) \
    struct __forward_dummy_struc_dec_to_catch_missing_semicolon

/* Our internal stack structure of the window procedures to subclass */
typedef struct
{
   struct {
      SUBCLASSPROC subproc;
      UINT_PTR id;
      DWORD_PTR ref;
   } SubclassProcs[31];
   int stackpos;
   int stacknum;
   int stacknew;
   WNDPROC origproc;
} SUBCLASS_INFO, *LPSUBCLASS_INFO;

#endif  /* __WINE_COMCTL32_H */
