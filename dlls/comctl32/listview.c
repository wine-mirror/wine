/*
 * Listview control
 *
 * Copyright 1998, 1999 Eric Kohl
 * Copyright 1999 Luc Tourangeau
 * Copyright 2000 Jason Mawdsley
 * Copyright 2001 Codeweavers Inc.
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
 *
 * NOTES
 * Listview control implementation.
 *
 * TODO:
 *   -- Drawing optimizations.
 *   -- Hot item handling.
 *
 * Notifications:
 *   LISTVIEW_Notify : most notifications from children (editbox and header)
 *
 * Data structure:
 *   LISTVIEW_SetItemCount : not completed for non OWNERDATA
 *
 * Advanced functionality:
 *   LISTVIEW_GetNumberOfWorkAreas : not implemented
 *   LISTVIEW_GetISearchString : not implemented
 *   LISTVIEW_GetBkImage : not implemented
 *   LISTVIEW_SetBkImage : not implemented
 *   LISTVIEW_GetColumnOrderArray : simple hack only
 *   LISTVIEW_SetColumnOrderArray : simple hack only
 *   LISTVIEW_Arrange : empty stub
 *   LISTVIEW_ApproximateViewRect : incomplete
 *   LISTVIEW_Update : not completed
 *
 * Known differences in message stream from native control (not known if
 * these differences cause problems):
 *   LVM_INSERTITEM issues LVM_SETITEMSTATE and LVM_SETITEM in certain cases.
 *   LVM_SETITEM does not always issue LVN_ITEMCHANGING/LVN_ITEMCHANGED.
 *   WM_PAINT does LVN_GETDISPINFO in item order 0->n, native does n->0.
 *   WM_SETREDRAW(True) native does LVN_GETDISPINFO for all items and
 *     does *not* invoke DefWindowProc
 *   WM_CREATE does not issue WM_QUERYUISTATE and associated registry
 *     processing for "USEDOUBLECLICKTIME".
 */


#include "config.h"
#include "wine/port.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "winbase.h"
#include "winnt.h"
#include "heap.h"
#include "commctrl.h"
#include "comctl32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(listview);

/* Some definitions for inline edit control */

typedef struct tagCOLUMNCACHE
{
  RECT rc;
  UINT align;
} COLUMNCACHE, *LPCOLUMNCACHE;

typedef struct tagITEMHDR
{
  LPWSTR pszText;
  INT iImage;
} ITEMHDR, *LPITEMHDR;

typedef struct tagLISTVIEW_SUBITEM
{
  ITEMHDR hdr;
  INT iSubItem;
} LISTVIEW_SUBITEM;

typedef struct tagLISTVIEW_ITEM
{
  ITEMHDR hdr;
  UINT state;
  LPARAM lParam;
  INT iIndent;
  POINT ptPosition;
  BOOL valid;
  RECT rcLastDraw;
} LISTVIEW_ITEM;

typedef struct tagRANGE
{
  INT lower;
  INT upper;
} RANGE;

typedef struct tagLISTVIEW_INFO
{
  HWND hwndSelf;
  HBRUSH hBkBrush;
  COLORREF clrBk;
  COLORREF clrText;
  COLORREF clrTextBk;
  HIMAGELIST himlNormal;
  HIMAGELIST himlSmall;
  HIMAGELIST himlState;
  BOOL bLButtonDown;
  BOOL bRButtonDown;
  INT nItemHeight;
  INT nItemWidth;
  HDPA hdpaSelectionRanges;
  INT nSelectionMark;
  INT nHotItem;
  SHORT notifyFormat;
  RECT rcList;                 /* This rectangle is really the window
				* client rectangle possibly reduced by the 
				* horizontal scroll bar and/or header - see 
				* LISTVIEW_UpdateSize. This rectangle offset
				* by the LISTVIEW_GetOrigin value is within
				* the rcView rectangle  */
  RECT rcView;                 /* This rectangle contains all items - 
				* contructed in LISTVIEW_AlignTop and
				* LISTVIEW_AlignLeft   */
  SIZE iconSize;
  SIZE iconSpacing;
  SIZE iconStateSize;
  UINT uCallbackMask;
  HWND hwndHeader;
  HFONT hDefaultFont;
  HCURSOR hHotCursor;
  HFONT hFont;
  INT ntmHeight;		/*  from GetTextMetrics from above font */
  INT ntmAveCharWidth;		/*  from GetTextMetrics from above font */
  BOOL bFocus;
  INT nFocusedItem;
  RECT rcFocus;
  DWORD dwStyle;		/* the cached window GWL_STYLE */
  DWORD dwLvExStyle;		/* extended listview style */
  HDPA hdpaItems;
  PFNLVCOMPARE pfnCompare;
  LPARAM lParamSort;
  HWND hwndEdit;
  BOOL bEditing;
  WNDPROC EditWndProc;
  INT nEditLabelItem;
  DWORD dwHoverTime;

  DWORD lastKeyPressTimestamp;
  WPARAM charCode;
  INT nSearchParamLength;
  WCHAR szSearchParam[ MAX_PATH ];
  BOOL bIsDrawing;
} LISTVIEW_INFO;

DEFINE_COMMON_NOTIFICATIONS(LISTVIEW_INFO, hwndSelf);

/*
 * constants
 */
/* How many we debug buffer to allocate */
#define DEBUG_BUFFERS 20
/* The size of a single debug bbuffer */
#define DEBUG_BUFFER_SIZE 256

/* Internal interface to LISTVIEW_HScroll and LISTVIEW_VScroll */
#define SB_INTERNAL      -1

/* maximum size of a label */
#define DISP_TEXT_SIZE 512

/* padding for items in list and small icon display modes */
#define WIDTH_PADDING 12

/* padding for items in list, report and small icon display modes */
#define HEIGHT_PADDING 1

/* offset of items in report display mode */
#define REPORT_MARGINX 2

/* padding for icon in large icon display mode
 *   ICON_TOP_PADDING_NOTHITABLE - space between top of box and area
 *                                 that HITTEST will see.
 *   ICON_TOP_PADDING_HITABLE - spacing between above and icon.
 *   ICON_TOP_PADDING - sum of the two above.
 *   ICON_BOTTOM_PADDING - between bottom of icon and top of text
 *   LABEL_VERT_PADDING - between bottom of text and end of box
 *
 *   ICON_LR_PADDING - additional width above icon size.
 *   ICON_LR_HALF - half of the above value
 */
#define ICON_TOP_PADDING_NOTHITABLE  2
#define ICON_TOP_PADDING_HITABLE     2
#define ICON_TOP_PADDING (ICON_TOP_PADDING_NOTHITABLE + ICON_TOP_PADDING_HITABLE)
#define ICON_BOTTOM_PADDING          4
#define LABEL_VERT_PADDING           7
#define ICON_LR_PADDING              16
#define ICON_LR_HALF                 (ICON_LR_PADDING/2)

/* default label width for items in list and small icon display modes */
#define DEFAULT_LABEL_WIDTH 40

/* default column width for items in list display mode */
#define DEFAULT_COLUMN_WIDTH 128

/* Size of "line" scroll for V & H scrolls */
#define LISTVIEW_SCROLL_ICON_LINE_SIZE 37

/* Padding betwen image and label */
#define IMAGE_PADDING  2

/* Padding behind the label */
#define TRAILING_PADDING  5

/* Border for the icon caption */
#define CAPTION_BORDER  2

/* Standard DrawText flags for LISTVIEW_UpdateLargeItemLabelRect and LISTVIEW_DrawLargeItem */
#define LISTVIEW_DTFLAGS  DT_TOP | DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL

/*
 * macros
 */
/* retrieve the number of items in the listview */
#define GETITEMCOUNT(infoPtr) ((infoPtr)->hdpaItems->nItemCount)

#define LISTVIEW_DUMP(iP) do { \
  TRACE("hwndSelf=%08x, clrBk=0x%06lx, clrText=0x%06lx, clrTextBk=0x%06lx, ItemHeight=%d, ItemWidth=%d, Style=0x%08lx\n", \
        iP->hwndSelf, iP->clrBk, iP->clrText, iP->clrTextBk, \
        iP->nItemHeight, iP->nItemWidth, infoPtr->dwStyle); \
  TRACE("hwndSelf=%08x, himlNor=%p, himlSml=%p, himlState=%p, Focused=%d, Hot=%d, exStyle=0x%08lx, Focus=%s\n", \
        iP->hwndSelf, iP->himlNormal, iP->himlSmall, iP->himlState, \
        iP->nFocusedItem, iP->nHotItem, iP->dwLvExStyle, \
        (iP->bFocus) ? "true" : "false"); \
  TRACE("hwndSelf=%08x, ntmH=%d, icSz.cx=%ld, icSz.cy=%ld, icSp.cx=%ld, icSp.cy=%ld, notifyFmt=%d\n", \
        iP->hwndSelf, iP->ntmHeight, iP->iconSize.cx, iP->iconSize.cy, \
        iP->iconSpacing.cx, iP->iconSpacing.cy, iP->notifyFormat); \
  TRACE("hwndSelf=%08x, rcList=(%d,%d)-(%d,%d), rcView=(%d,%d)-(%d,%d)\n", \
        iP->hwndSelf, \
	iP->rcList.left, iP->rcList.top, iP->rcList.right, iP->rcList.bottom, \
	iP->rcView.left, iP->rcView.top, iP->rcView.right, iP->rcView.bottom); \
} while(0)


/*
 * forward declarations
 */
static BOOL LISTVIEW_GetItemT(LISTVIEW_INFO *, LPLVITEMW, BOOL, BOOL);
static INT LISTVIEW_SuperHitTestItem(LISTVIEW_INFO *, LPLVHITTESTINFO, BOOL, BOOL);
static void LISTVIEW_AlignLeft(LISTVIEW_INFO *);
static void LISTVIEW_AlignTop(LISTVIEW_INFO *);
static void LISTVIEW_AddGroupSelection(LISTVIEW_INFO *, INT);
static INT LISTVIEW_GetItemHeight(LISTVIEW_INFO *);
static BOOL LISTVIEW_GetItemPosition(LISTVIEW_INFO *, INT, LPPOINT);
static BOOL LISTVIEW_GetItemRect(LISTVIEW_INFO *, INT, LPRECT);
static INT LISTVIEW_GetItemWidth(LISTVIEW_INFO *);
static INT LISTVIEW_GetLabelWidth(LISTVIEW_INFO *, INT);
static LRESULT LISTVIEW_GetColumnWidth(LISTVIEW_INFO *, INT);
static BOOL LISTVIEW_GetOrigin(LISTVIEW_INFO *, LPPOINT);
static BOOL LISTVIEW_GetViewRect(LISTVIEW_INFO *, LPRECT);
static void LISTVIEW_SetGroupSelection(LISTVIEW_INFO *, INT);
static BOOL LISTVIEW_SetItemT(LISTVIEW_INFO *, LPLVITEMW, BOOL);
static BOOL LISTVIEW_SetItemPosition(LISTVIEW_INFO *, INT, LONG, LONG);
static void LISTVIEW_UpdateScroll(LISTVIEW_INFO *);
static void LISTVIEW_SetSelection(LISTVIEW_INFO *, INT);
static BOOL LISTVIEW_UpdateSize(LISTVIEW_INFO *);
static void LISTVIEW_UnsupportedStyles(LONG);
static HWND LISTVIEW_EditLabelT(LISTVIEW_INFO *, INT, BOOL);
static LRESULT LISTVIEW_Command(LISTVIEW_INFO *, WPARAM, LPARAM);
static LRESULT LISTVIEW_SortItems(LISTVIEW_INFO *, PFNLVCOMPARE, LPARAM);
static LRESULT LISTVIEW_GetStringWidthT(LISTVIEW_INFO *, LPCWSTR, BOOL);
static INT LISTVIEW_ProcessLetterKeys(LISTVIEW_INFO *, WPARAM, LPARAM);
static BOOL LISTVIEW_KeySelection(LISTVIEW_INFO *, INT);
static LRESULT LISTVIEW_GetItemState(LISTVIEW_INFO *, INT, UINT);
static LRESULT LISTVIEW_SetItemState(LISTVIEW_INFO *, INT, LPLVITEMW);
static BOOL LISTVIEW_UpdateLargeItemLabelRect (LISTVIEW_INFO *, int, RECT*);
static LRESULT LISTVIEW_GetColumnT(LISTVIEW_INFO *, INT, LPLVCOLUMNW, BOOL);
static LRESULT LISTVIEW_VScroll(LISTVIEW_INFO *, INT, INT, HWND);
static LRESULT LISTVIEW_HScroll(LISTVIEW_INFO *, INT, INT, HWND);
static INT LISTVIEW_GetTopIndex(LISTVIEW_INFO *);
static BOOL LISTVIEW_EnsureVisible(LISTVIEW_INFO *, INT, BOOL);
static HWND CreateEditLabelT(LISTVIEW_INFO *, LPCWSTR, DWORD, INT, INT, INT, INT, BOOL);

/******** Defines that LISTVIEW_ProcessLetterKeys uses ****************/
#define KEY_DELAY       450

#define COUNTOF(array) (sizeof(array)/sizeof(array[0]))


/******** Text handling functions *************************************/

/* A text pointer is either NULL, LPSTR_TEXTCALLBACK, or points to a
 * text string. The string may be ANSI or Unicode, in which case
 * the boolean isW tells us the type of the string.
 *
 * The name of the function tell what type of strings it expects:
 *   W: Unicode, T: ANSI/Unicode - function of isW
 */

static inline BOOL is_textW(LPCWSTR text)
{
    return text != NULL && text != LPSTR_TEXTCALLBACKW;
}

static inline BOOL is_textT(LPCWSTR text, BOOL isW)
{
    /* we can ignore isW since LPSTR_TEXTCALLBACKW == LPSTR_TEXTCALLBACKA */
    return is_textW(text);
}

static inline int textlenT(LPCWSTR text, BOOL isW)
{
    return !is_textT(text, isW) ? 0 :
	   isW ? lstrlenW(text) : lstrlenA((LPCSTR)text);
}

static inline void textcpynT(LPWSTR dest, BOOL isDestW, LPCWSTR src, BOOL isSrcW, INT max)
{
    if (isDestW)
	if (isSrcW) lstrcpynW(dest, src, max);
	else MultiByteToWideChar(CP_ACP, 0, (LPCSTR)src, -1, dest, max);
    else
	if (isSrcW) WideCharToMultiByte(CP_ACP, 0, src, -1, (LPSTR)dest, max, NULL, NULL);
	else lstrcpynA((LPSTR)dest, (LPCSTR)src, max);
}

static inline LPWSTR textdupTtoW(LPCWSTR text, BOOL isW)
{
    LPWSTR wstr = (LPWSTR)text;

    if (!isW && is_textT(text, isW))
    {
	INT len = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)text, -1, NULL, 0);
	wstr = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
	if (wstr) MultiByteToWideChar(CP_ACP, 0, (LPCSTR)text, -1, wstr, len);
    }
    TRACE("   wstr=%s\n", text == LPSTR_TEXTCALLBACKW ?  "(callback)" : debugstr_w(wstr));
    return wstr;
}

static inline void textfreeT(LPWSTR wstr, BOOL isW)
{
    if (!isW && is_textT(wstr, isW)) HeapFree(GetProcessHeap(), 0, wstr);
}

/*
 * dest is a pointer to a Unicode string
 * src is a pointer to a string (Unicode if isW, ANSI if !isW)
 */
static BOOL textsetptrT(LPWSTR *dest, LPWSTR src, BOOL isW)
{
    BOOL bResult = TRUE;
    
    if (src == LPSTR_TEXTCALLBACKW)
    {
	if (is_textW(*dest)) COMCTL32_Free(*dest);
	*dest = LPSTR_TEXTCALLBACKW;
    }
    else
    {
	LPWSTR pszText = textdupTtoW(src, isW);
	if (*dest == LPSTR_TEXTCALLBACKW) *dest = NULL;
	bResult = Str_SetPtrW(dest, pszText);
	textfreeT(pszText, isW);
    }
    return bResult;
}

/*
 * compares a Unicode to a Unicode/ANSI text string
 */
static inline int textcmpWT(LPWSTR aw, LPWSTR bt, BOOL isW)
{
    if (!aw) return bt ? -1 : 0;
    if (!bt) return aw ? 1 : 0;
    if (aw == LPSTR_TEXTCALLBACKW)
	return bt == LPSTR_TEXTCALLBACKW ? 0 : -1;
    if (bt != LPSTR_TEXTCALLBACKW)
    {
	LPWSTR bw = textdupTtoW(bt, isW);
	int r = bw ? lstrcmpW(aw, bw) : 1;
	textfreeT(bw, isW);
	return r;
    }	    
	    
    return 1;
}
    
static inline int lstrncmpiW(LPCWSTR s1, LPCWSTR s2, int n)
{
    int res;

    n = min(min(n, strlenW(s1)), strlenW(s2));
    res = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, s1, n, s2, n);
    return res ? res - sizeof(WCHAR) : res;
}

/******** Debugging functions *****************************************/

static inline LPCSTR debugtext_t(LPCWSTR text, BOOL isW)
{
    if (text == LPSTR_TEXTCALLBACKW) return "(callback)";
    return isW ? debugstr_w(text) : debugstr_a((LPCSTR)text);
}

static inline LPCSTR debugtext_tn(LPCWSTR text, BOOL isW, INT n)
{
    if (text == LPSTR_TEXTCALLBACKW) return "(callback)";
    n = min(textlenT(text, isW), n);
    return isW ? debugstr_wn(text, n) : debugstr_an((LPCSTR)text, n);
}

static char* debug_getbuf()
{
    static int index = 0;
    static char buffers[DEBUG_BUFFERS][DEBUG_BUFFER_SIZE];
    return buffers[index++ % DEBUG_BUFFERS];
}

static inline char* debugrect(const RECT* rect)
{
    if (rect) {
	char* buf = debug_getbuf();
	snprintf(buf, DEBUG_BUFFER_SIZE, "[(%d, %d);(%d, %d)]", 
		 rect->left, rect->top, rect->right, rect->bottom);
    	return buf;
    } else return "(null)";
}

static char* debuglvitem_t(LPLVITEMW lpLVItem, BOOL isW)
{
    char* buf = debug_getbuf(), *text = buf;
    int len, size = DEBUG_BUFFER_SIZE;
    
    if (lpLVItem == NULL) return "(null)";
    len = snprintf(buf, size, "{iItem=%d, iSubItem=%d, ", lpLVItem->iItem, lpLVItem->iSubItem);
    if (len == -1) goto end; buf += len; size -= len;
    if (lpLVItem->mask & LVIF_STATE)
	len = snprintf(buf, size, "state=%x, stateMask=%x, ", lpLVItem->state, lpLVItem->stateMask);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    if (lpLVItem->mask & LVIF_TEXT)
	len = snprintf(buf, size, "pszText=%s, cchTextMax=%d, ", debugtext_tn(lpLVItem->pszText, isW, 80), lpLVItem->cchTextMax);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    if (lpLVItem->mask & LVIF_IMAGE)
	len = snprintf(buf, size, "iImage=%d, ", lpLVItem->iImage);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    if (lpLVItem->mask & LVIF_PARAM)
	len = snprintf(buf, size, "lParam=%lx, ", lpLVItem->lParam);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    if (lpLVItem->mask & LVIF_INDENT)
	len = snprintf(buf, size, "iIndent=%d, ", lpLVItem->iIndent);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    goto undo;
end:
    buf = text + strlen(text);
undo:
    if (buf - text > 2) buf[-2] = '}'; buf[-1] = 0;
    return text;
}

static char* debuglvcolumn_t(LPLVCOLUMNW lpColumn, BOOL isW)
{
    char* buf = debug_getbuf(), *text = buf;
    int len, size = DEBUG_BUFFER_SIZE;
    
    if (lpColumn == NULL) return "(null)";
    len = snprintf(buf, size, "{");
    if (len == -1) goto end; buf += len; size -= len;
    if (lpColumn->mask & LVCF_SUBITEM)
	len = snprintf(buf, size, "iSubItem=%d, ",  lpColumn->iSubItem);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    if (lpColumn->mask & LVCF_FMT)
	len = snprintf(buf, size, "fmt=%x, ", lpColumn->fmt);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    if (lpColumn->mask & LVCF_WIDTH)
	len = snprintf(buf, size, "cx=%d, ", lpColumn->cx);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    if (lpColumn->mask & LVCF_TEXT)
	len = snprintf(buf, size, "pszText=%s, cchTextMax=%d, ", debugtext_tn(lpColumn->pszText, isW, 80), lpColumn->cchTextMax);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    if (lpColumn->mask & LVCF_IMAGE)
	len = snprintf(buf, size, "iImage=%d, ", lpColumn->iImage);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    if (lpColumn->mask & LVCF_ORDER)
	len = snprintf(buf, size, "iOrder=%d, ", lpColumn->iOrder);
    else len = 0;
    if (len == -1) goto end; buf += len; size -= len;
    goto undo;
end:
    buf = text + strlen(text);
undo:
    if (buf - text > 2) buf[-2] = '}'; buf[-1] = 0;
    return text;
}


/******** Notification functions i************************************/

static inline BOOL notify(LISTVIEW_INFO *infoPtr, INT code, LPNMHDR pnmh)
{
    pnmh->hwndFrom = infoPtr->hwndSelf;
    pnmh->idFrom = GetWindowLongW(infoPtr->hwndSelf, GWL_ID);
    pnmh->code = code;
    return (BOOL)SendMessageW(GetParent(infoPtr->hwndSelf), WM_NOTIFY,
			      (WPARAM)pnmh->idFrom, (LPARAM)pnmh);
}

static inline void notify_itemactivate(LISTVIEW_INFO *infoPtr)
{
    NMHDR nmh;
    notify(infoPtr, LVN_ITEMACTIVATE, &nmh);
}

static inline BOOL notify_listview(LISTVIEW_INFO *infoPtr, INT code, LPNMLISTVIEW plvnm)
{
    return notify(infoPtr, code, (LPNMHDR)plvnm);
}

static int get_ansi_notification(INT unicodeNotificationCode)
{
    switch (unicodeNotificationCode)
    {
    case LVN_BEGINLABELEDITW: return LVN_BEGINLABELEDITA;
    case LVN_ENDLABELEDITW: return LVN_ENDLABELEDITA;
    case LVN_GETDISPINFOW: return LVN_GETDISPINFOA;
    case LVN_SETDISPINFOW: return LVN_SETDISPINFOA;
    case LVN_ODFINDITEMW: return LVN_ODFINDITEMA;
    case LVN_GETINFOTIPW: return LVN_GETINFOTIPA;
    }
    ERR("unknown notification %x\n", unicodeNotificationCode);
    return unicodeNotificationCode;
}

/*
  Send notification. depends on dispinfoW having same
  structure as dispinfoA.
  infoPtr : listview struct
  notificationCode : *Unicode* notification code
  pdi : dispinfo structure (can be unicode or ansi)
  isW : TRUE if dispinfo is Unicode
*/
static BOOL notify_dispinfoT(LISTVIEW_INFO *infoPtr, INT notificationCode, LPNMLVDISPINFOW pdi, BOOL isW)
{
  BOOL bResult = FALSE;
  BOOL convertToAnsi = FALSE, convertToUnicode = FALSE;
  INT realNotifCode;
  INT cchTempBufMax = 0, savCchTextMax = 0;
  LPWSTR pszTempBuf = NULL, savPszText = NULL;

  TRACE("(code=%x, pdi=%p, isW=%d)\n", notificationCode, pdi, isW);
  TRACE("   notifyFormat=%s\n",
	infoPtr->notifyFormat == NFR_UNICODE ? "NFR_UNICODE" :
	infoPtr->notifyFormat == NFR_ANSI ? "NFR_ANSI" : "(not set)");
  if (infoPtr->notifyFormat == NFR_ANSI)
    realNotifCode = get_ansi_notification(notificationCode);
  else
    realNotifCode = notificationCode;

  if ((pdi->item.mask & LVIF_TEXT) && is_textT(pdi->item.pszText, isW))
  {
    if (isW && infoPtr->notifyFormat == NFR_ANSI)
        convertToAnsi = TRUE;
    if (!isW && infoPtr->notifyFormat == NFR_UNICODE)
        convertToUnicode = TRUE;
  }

  if (convertToAnsi || convertToUnicode)
  {
    TRACE("   we have to convert the text to the correct format\n");
    if (notificationCode != LVN_GETDISPINFOW)
    { /* length of existing text */
      cchTempBufMax = convertToUnicode ?
      MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pdi->item.pszText, -1, NULL, 0):
      WideCharToMultiByte(CP_ACP, 0, pdi->item.pszText, -1, NULL, 0, NULL, NULL);
    }
    else
      cchTempBufMax = pdi->item.cchTextMax;

    pszTempBuf = HeapAlloc(GetProcessHeap(), 0,
        (convertToUnicode ? sizeof(WCHAR) : sizeof(CHAR)) * cchTempBufMax);
    if (!pszTempBuf) return FALSE;
    if (convertToUnicode)
      MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pdi->item.pszText, -1,
                          pszTempBuf, cchTempBufMax);
    else
      WideCharToMultiByte(CP_ACP, 0, pdi->item.pszText, -1, (LPSTR) pszTempBuf,
                          cchTempBufMax, NULL, NULL);
    TRACE("   text=%s\n", debugtext_t(pszTempBuf, convertToUnicode));
    savCchTextMax = pdi->item.cchTextMax;
    savPszText = pdi->item.pszText;
    pdi->item.pszText = pszTempBuf;
    pdi->item.cchTextMax = cchTempBufMax;
  }

  bResult = notify(infoPtr, realNotifCode, (LPNMHDR)pdi);

  if (convertToUnicode || convertToAnsi)
  { /* convert back result */
    TRACE("   returned text=%s\n", debugtext_t(pdi->item.pszText, convertToUnicode));
    if (convertToUnicode) /* note : pointer can be changed by app ! */
      WideCharToMultiByte(CP_ACP, 0, pdi->item.pszText, -1, (LPSTR) savPszText,
                          savCchTextMax, NULL, NULL);
    else
      MultiByteToWideChar(CP_ACP, 0, (LPSTR) pdi->item.pszText, -1,
                          savPszText, savCchTextMax);
    pdi->item.pszText = savPszText; /* restores our buffer */
    pdi->item.cchTextMax = savCchTextMax;
    HeapFree(GetProcessHeap(), 0, pszTempBuf);
  }
  return bResult;
}

static inline void notify_odcachehint(LISTVIEW_INFO *infoPtr, INT iFrom, INT iTo)
{
    NMLVCACHEHINT nmlv;

    nmlv.iFrom = iFrom;
    nmlv.iTo   = iTo;
    notify(infoPtr, LVN_ODCACHEHINT, &nmlv.hdr);
}

static BOOL notify_customdraw (LISTVIEW_INFO *infoPtr, DWORD dwDrawStage, HDC hdc, RECT rc)
{
    NMLVCUSTOMDRAW nmlvcd;

    TRACE("(dwDrawStage=%lx, hdc=%x, rc=?)\n", dwDrawStage, hdc);

    nmlvcd.nmcd.dwDrawStage = dwDrawStage;
    nmlvcd.nmcd.hdc         = hdc;
    nmlvcd.nmcd.rc          = rc;
    nmlvcd.nmcd.dwItemSpec  = 0;
    nmlvcd.nmcd.uItemState  = 0;
    nmlvcd.nmcd.lItemlParam = 0;
    nmlvcd.clrText          = infoPtr->clrText;
    nmlvcd.clrTextBk        = infoPtr->clrBk;

    return (BOOL)notify(infoPtr, NM_CUSTOMDRAW, &nmlvcd.nmcd.hdr);
}

/* FIXME: we should inline this where it's called somehow
 * I think we need to pass in the structure
 */
static BOOL notify_customdrawitem (LISTVIEW_INFO *infoPtr, HDC hdc, UINT iItem, UINT iSubItem, UINT uItemDrawState)
{
    NMLVCUSTOMDRAW nmlvcd;
    UINT uItemState;
    RECT itemRect;
    LVITEMW item;
    BOOL bReturn;

    item.iItem = iItem;
    item.iSubItem = 0;
    item.mask = LVIF_PARAM;
    if (!LISTVIEW_GetItemT(infoPtr, &item, TRUE, TRUE)) return FALSE;

    uItemState = 0;

    if (LISTVIEW_GetItemState(infoPtr, iItem, LVIS_SELECTED)) uItemState |= CDIS_SELECTED;
    if (LISTVIEW_GetItemState(infoPtr, iItem, LVIS_FOCUSED)) uItemState |= CDIS_FOCUS;
    if (iItem == infoPtr->nHotItem)       uItemState |= CDIS_HOT;

    itemRect.left = LVIR_BOUNDS;
    LISTVIEW_GetItemRect(infoPtr, iItem, &itemRect);

    nmlvcd.nmcd.dwDrawStage = CDDS_ITEM | uItemDrawState;
    nmlvcd.nmcd.hdc         = hdc;
    nmlvcd.nmcd.rc          = itemRect;
    nmlvcd.nmcd.dwItemSpec  = iItem;
    nmlvcd.nmcd.uItemState  = uItemState;
    nmlvcd.nmcd.lItemlParam = item.lParam;
    nmlvcd.clrText          = infoPtr->clrText;
    nmlvcd.clrTextBk        = infoPtr->clrBk;
    nmlvcd.iSubItem         = iSubItem;

    TRACE("drawstage=%lx hdc=%x item=%lx, itemstate=%x, lItemlParam=%lx\n",
          nmlvcd.nmcd.dwDrawStage, nmlvcd.nmcd.hdc, nmlvcd.nmcd.dwItemSpec,
          nmlvcd.nmcd.uItemState, nmlvcd.nmcd.lItemlParam);

    bReturn = notify(infoPtr, NM_CUSTOMDRAW, &nmlvcd.nmcd.hdr);

    infoPtr->clrText = nmlvcd.clrText;
    infoPtr->clrBk   = nmlvcd.clrTextBk;
    
    return bReturn;
}

/******** Misc helper functions ************************************/

static inline LRESULT CallWindowProcT(WNDPROC proc, HWND hwnd, UINT uMsg,
		                      WPARAM wParam, LPARAM lParam, BOOL isW)
{
    if (isW) return CallWindowProcW(proc, hwnd, uMsg, wParam, lParam);
    else return CallWindowProcA(proc, hwnd, uMsg, wParam, lParam);
}

/******** Internal API functions ************************************/

/* The Invalidate* are macros, so we preserve the caller location */
#define LISTVIEW_InvalidateRect(infoPtr, rect) do { \
    TRACE(" invalidating rect=%s\n", debugrect(rect)); \
    InvalidateRect(infoPtr->hwndSelf, rect, TRUE); \
} while (0)

#define LISTVIEW_InvalidateItem(infoPtr, nItem) do { \
    RECT rcItem; \
    rcItem.left = LVIR_BOUNDS; \
    if(LISTVIEW_GetItemRect(infoPtr, nItem, &rcItem)) \
	LISTVIEW_InvalidateRect(infoPtr, &rcItem); \
} while (0)

#define LISTVIEW_InvalidateList(infoPtr)\
    LISTVIEW_InvalidateRect(infoPtr, &infoPtr->rcList)

static inline BOOL LISTVIEW_GetItemW(LISTVIEW_INFO *infoPtr, LPLVITEMW lpLVItem, BOOL internal)
{
    return LISTVIEW_GetItemT(infoPtr, lpLVItem, internal, TRUE);
}

static inline int LISTVIEW_GetType(LISTVIEW_INFO *infoPtr)
{
    return infoPtr->dwStyle & LVS_TYPEMASK;
}

/***
 * DESCRIPTION:
 * Retrieves the number of items that can fit vertically in the client area.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 * Number of items per row.
 */
static inline INT LISTVIEW_GetCountPerRow(LISTVIEW_INFO *infoPtr)
{
    INT nListWidth = infoPtr->rcList.right - infoPtr->rcList.left;

    return max(nListWidth/infoPtr->nItemWidth, 1);
}

/***
 * DESCRIPTION:
 * Retrieves the number of items that can fit horizontally in the client
 * area.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 * Number of items per column.
 */
static inline INT LISTVIEW_GetCountPerColumn(LISTVIEW_INFO *infoPtr)
{
    INT nListHeight = infoPtr->rcList.bottom - infoPtr->rcList.top;

    return max(nListHeight / infoPtr->nItemHeight, 1);
}

/***
 * DESCRIPTION:
 * Retrieves the range of visible items. Note that the upper limit
 * may be a bit larger than the actual last visible item.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 * maximum range of visible items
 */
static RANGE LISTVIEW_GetVisibleRange(LISTVIEW_INFO *infoPtr)
{
    UINT uView = LISTVIEW_GetType(infoPtr);
    INT nPerCol, nPerRow;
    RANGE visrange;
    
    visrange.lower = LISTVIEW_GetTopIndex(infoPtr);
    
    if (uView == LVS_REPORT)
    {
	nPerCol = LISTVIEW_GetCountPerColumn(infoPtr) + 1;
	nPerRow = 1;
    }
    else if (uView == LVS_LIST)
    {
	nPerCol = LISTVIEW_GetCountPerColumn(infoPtr) + 1;
	nPerRow = LISTVIEW_GetCountPerRow(infoPtr);
    }
    else
    {
	/* FIXME: this is correct only in autoarrange mode */
	nPerCol = LISTVIEW_GetCountPerColumn(infoPtr) + 1;
	nPerRow = LISTVIEW_GetCountPerRow(infoPtr) + 1;
    }

    visrange.upper = visrange.lower + nPerCol * nPerRow;
    if (visrange.upper > GETITEMCOUNT(infoPtr)) 
	visrange.upper = GETITEMCOUNT(infoPtr);
   
    TRACE("range=(%d, %d)\n", visrange.lower, visrange.upper);

    return visrange;
}


/*************************************************************************
 *		LISTVIEW_ProcessLetterKeys
 *
 *  Processes keyboard messages generated by pressing the letter keys
 *  on the keyboard.
 *  What this does is perform a case insensitive search from the
 *  current position with the following quirks:
 *  - If two chars or more are pressed in quick succession we search
 *    for the corresponding string (e.g. 'abc').
 *  - If there is a delay we wipe away the current search string and
 *    restart with just that char.
 *  - If the user keeps pressing the same character, whether slowly or
 *    fast, so that the search string is entirely composed of this
 *    character ('aaaaa' for instance), then we search for first item
 *    that starting with that character.
 *  - If the user types the above character in quick succession, then
 *    we must also search for the corresponding string ('aaaaa'), and
 *    go to that string if there is a match.
 *
 * PARAMETERS
 *   [I] hwnd : handle to the window
 *   [I] charCode : the character code, the actual character
 *   [I] keyData : key data
 *
 * RETURNS
 *
 *  Zero.
 *
 * BUGS
 *
 *  - The current implementation has a list of characters it will
 *    accept and it ignores averything else. In particular it will
 *    ignore accentuated characters which seems to match what
 *    Windows does. But I'm not sure it makes sense to follow
 *    Windows there.
 *  - We don't sound a beep when the search fails.
 *
 * SEE ALSO
 *
 *  TREEVIEW_ProcessLetterKeys
 */
static INT LISTVIEW_ProcessLetterKeys(LISTVIEW_INFO *infoPtr, WPARAM charCode, LPARAM keyData)
{
    INT nItem;
    INT nSize;
    INT endidx,idx;
    LVITEMW item;
    WCHAR buffer[MAX_PATH];
    DWORD timestamp,elapsed;

    /* simple parameter checking */
    if (!charCode || !keyData)
        return 0;

    if (!infoPtr)
        return 0;

    /* only allow the valid WM_CHARs through */
    if (!isalnum(charCode) &&
        charCode != '.' && charCode != '`' && charCode != '!' &&
        charCode != '@' && charCode != '#' && charCode != '$' &&
        charCode != '%' && charCode != '^' && charCode != '&' &&
        charCode != '*' && charCode != '(' && charCode != ')' &&
        charCode != '-' && charCode != '_' && charCode != '+' &&
        charCode != '=' && charCode != '\\'&& charCode != ']' &&
        charCode != '}' && charCode != '[' && charCode != '{' &&
        charCode != '/' && charCode != '?' && charCode != '>' &&
        charCode != '<' && charCode != ',' && charCode != '~')
        return 0;

    nSize=GETITEMCOUNT(infoPtr);
    /* if there's one item or less, there is no where to go */
    if (nSize <= 1)
        return 0;

    /* compute how much time elapsed since last keypress */
    timestamp=GetTickCount();
    if (timestamp > infoPtr->lastKeyPressTimestamp) {
        elapsed=timestamp-infoPtr->lastKeyPressTimestamp;
    } else {
        elapsed=infoPtr->lastKeyPressTimestamp-timestamp;
    }

    /* update the search parameters */
    infoPtr->lastKeyPressTimestamp=timestamp;
    if (elapsed < KEY_DELAY) {
        if (infoPtr->nSearchParamLength < COUNTOF(infoPtr->szSearchParam)) {
            infoPtr->szSearchParam[infoPtr->nSearchParamLength++]=charCode;
        }
        if (infoPtr->charCode != charCode) {
            infoPtr->charCode=charCode=0;
        }
    } else {
        infoPtr->charCode=charCode;
        infoPtr->szSearchParam[0]=charCode;
        infoPtr->nSearchParamLength=1;
        /* Redundant with the 1 char string */
        charCode=0;
    }

    /* and search from the current position */
    nItem=-1;
    if (infoPtr->nFocusedItem >= 0) {
        endidx=infoPtr->nFocusedItem;
        idx=endidx;
        /* if looking for single character match,
         * then we must always move forward
         */
        if (infoPtr->nSearchParamLength == 1)
            idx++;
    } else {
        endidx=nSize;
        idx=0;
    }
    do {
        if (idx == nSize) {
            if (endidx == nSize || endidx == 0)
                break;
            idx=0;
        }

        /* get item */
        item.mask = LVIF_TEXT;
        item.iItem = idx;
        item.iSubItem = 0;
        item.pszText = buffer;
        item.cchTextMax = COUNTOF(buffer);
        if (!LISTVIEW_GetItemW(infoPtr, &item, TRUE)) return 0;

        /* check for a match */
        if (lstrncmpiW(item.pszText,infoPtr->szSearchParam,infoPtr->nSearchParamLength) == 0) {
            nItem=idx;
            break;
        } else if ( (charCode != 0) && (nItem == -1) && (nItem != infoPtr->nFocusedItem) &&
                    (lstrncmpiW(item.pszText,infoPtr->szSearchParam,1) == 0) ) {
            /* This would work but we must keep looking for a longer match */
            nItem=idx;
        }
        idx++;
    } while (idx != endidx);

    if (nItem != -1)
        LISTVIEW_KeySelection(infoPtr, nItem);

    return 0;
}

/*************************************************************************
 * LISTVIEW_UpdateHeaderSize [Internal]
 *
 * Function to resize the header control
 *
 * PARAMS
 *     hwnd             [I] handle to a window
 *     nNewScrollPos    [I] Scroll Pos to Set
 *
 * RETURNS
 *     nothing
 *
 * NOTES
 */
static void LISTVIEW_UpdateHeaderSize(LISTVIEW_INFO *infoPtr, INT nNewScrollPos)
{
    RECT winRect;
    POINT point[2];

    TRACE("nNewScrollPos=%d", nNewScrollPos);

    GetWindowRect(infoPtr->hwndHeader, &winRect);
    point[0].x = winRect.left;
    point[0].y = winRect.top;
    point[1].x = winRect.right;
    point[1].y = winRect.bottom;

    MapWindowPoints(HWND_DESKTOP, infoPtr->hwndSelf, point, 2);
    point[0].x = -nNewScrollPos;
    point[1].x += nNewScrollPos;

    SetWindowPos(infoPtr->hwndHeader,0,
        point[0].x,point[0].y,point[1].x,point[1].y,
        SWP_NOZORDER | SWP_NOACTIVATE);
}

/***
 * DESCRIPTION:
 * Update the scrollbars. This functions should be called whenever
 * the content, size or view changes.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 * None
 */
static void LISTVIEW_UpdateScroll(LISTVIEW_INFO *infoPtr)
{
  LONG lStyle = infoPtr->dwStyle;
  UINT uView =  lStyle & LVS_TYPEMASK;
  INT nListHeight = infoPtr->rcList.bottom - infoPtr->rcList.top;
  INT nListWidth = infoPtr->rcList.right - infoPtr->rcList.left;
  SCROLLINFO scrollInfo;

  if (lStyle & LVS_NOSCROLL) return;

  scrollInfo.cbSize = sizeof(SCROLLINFO);

  if (uView == LVS_LIST)
  {
    /* update horizontal scrollbar */
    INT nCountPerColumn = LISTVIEW_GetCountPerColumn(infoPtr);
    INT nCountPerRow = LISTVIEW_GetCountPerRow(infoPtr);
    INT nNumOfItems = GETITEMCOUNT(infoPtr);

    TRACE("items=%d, perColumn=%d, perRow=%d\n",
	  nNumOfItems, nCountPerColumn, nCountPerRow);
   
    scrollInfo.nMin = 0; 
    scrollInfo.nMax = nNumOfItems / nCountPerColumn;
    if((nNumOfItems % nCountPerColumn) == 0)
        scrollInfo.nMax--;
    if (scrollInfo.nMax < 0) scrollInfo.nMax = 0;
    scrollInfo.nPos = ListView_GetTopIndex(infoPtr->hwndSelf) / nCountPerColumn;
    scrollInfo.nPage = nCountPerRow;
    scrollInfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
    SetScrollInfo(infoPtr->hwndSelf, SB_HORZ, &scrollInfo, TRUE);
    ShowScrollBar(infoPtr->hwndSelf, SB_VERT, FALSE);
  }
  else if (uView == LVS_REPORT)
  {
    BOOL test;

    /* update vertical scrollbar */
    scrollInfo.nMin = 0;
    scrollInfo.nMax = GETITEMCOUNT(infoPtr) - 1;
    scrollInfo.nPos = ListView_GetTopIndex(infoPtr->hwndSelf);
    scrollInfo.nPage = LISTVIEW_GetCountPerColumn(infoPtr);
    scrollInfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
    test = (scrollInfo.nMin >= scrollInfo.nMax - max((INT)scrollInfo.nPage - 1, 0));
    TRACE("LVS_REPORT Vert. nMax=%d, nPage=%d, test=%d\n",
	  scrollInfo.nMax, scrollInfo.nPage, test);
    SetScrollInfo(infoPtr->hwndSelf, SB_VERT, &scrollInfo, TRUE);
    ShowScrollBar(infoPtr->hwndSelf, SB_VERT, (test) ? FALSE : TRUE);

    /* update horizontal scrollbar */
    nListWidth = infoPtr->rcList.right - infoPtr->rcList.left;
    scrollInfo.fMask = SIF_POS;
    if (!GetScrollInfo(infoPtr->hwndSelf, SB_HORZ, &scrollInfo)
       || GETITEMCOUNT(infoPtr) == 0)
    {
      scrollInfo.nPos = 0;
    }
    scrollInfo.nMin = 0;
    scrollInfo.nMax = max(infoPtr->nItemWidth, 0)-1;
    scrollInfo.nPage = nListWidth;
    scrollInfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE  ;
    test = (scrollInfo.nMin >= scrollInfo.nMax - max((INT)scrollInfo.nPage - 1, 0));
    TRACE("LVS_REPORT Horz. nMax=%d, nPage=%d, test=%d\n",
	  scrollInfo.nMax, scrollInfo.nPage, test);
    SetScrollInfo(infoPtr->hwndSelf, SB_HORZ, &scrollInfo, TRUE);
    ShowScrollBar(infoPtr->hwndSelf, SB_HORZ, (test) ? FALSE : TRUE);

    /* Update the Header Control */
    scrollInfo.fMask = SIF_POS;
    GetScrollInfo(infoPtr->hwndSelf, SB_HORZ, &scrollInfo);
    LISTVIEW_UpdateHeaderSize(infoPtr, scrollInfo.nPos);

  }
  else
  {
    RECT rcView;

    if (LISTVIEW_GetViewRect(infoPtr, &rcView))
    {
      INT nViewWidth = rcView.right - rcView.left;
      INT nViewHeight = rcView.bottom - rcView.top;

      /* Update Horizontal Scrollbar */
      scrollInfo.fMask = SIF_POS;
      if (!GetScrollInfo(infoPtr->hwndSelf, SB_HORZ, &scrollInfo)
        || GETITEMCOUNT(infoPtr) == 0)
      {
        scrollInfo.nPos = 0;
      }
      scrollInfo.nMin = 0;
      scrollInfo.nMax = max(nViewWidth, 0)-1;
      scrollInfo.nPage = nListWidth;
      scrollInfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
      TRACE("LVS_ICON/SMALLICON Horz.\n");
      SetScrollInfo(infoPtr->hwndSelf, SB_HORZ, &scrollInfo, TRUE);

      /* Update Vertical Scrollbar */
      nListHeight = infoPtr->rcList.bottom - infoPtr->rcList.top;
      scrollInfo.fMask = SIF_POS;
      if (!GetScrollInfo(infoPtr->hwndSelf, SB_VERT, &scrollInfo)
        || GETITEMCOUNT(infoPtr) == 0)
      {
        scrollInfo.nPos = 0;
      }
      scrollInfo.nMin = 0;
      scrollInfo.nMax = max(nViewHeight,0)-1;
      scrollInfo.nPage = nListHeight;
      scrollInfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
      TRACE("LVS_ICON/SMALLICON Vert.\n");
      SetScrollInfo(infoPtr->hwndSelf, SB_VERT, &scrollInfo, TRUE);
    }
  }
}


/***
 * Toggles (draws/erase) the focus rectangle. 
 */
static inline void LISTVIEW_ToggleFocusRect(LISTVIEW_INFO *infoPtr)
{
    TRACE("rcFocus=%s\n", debugrect(&infoPtr->rcFocus));

    /* if we have a focus rectagle, draw it */
    if (!IsRectEmpty(&infoPtr->rcFocus))
    {
	HDC hdc = GetDC(infoPtr->hwndSelf);
	DrawFocusRect(hdc, &infoPtr->rcFocus);
	ReleaseDC(infoPtr->hwndSelf, hdc);
    }
}

/***
 * Invalidates all visible selected items.
 */
static void LISTVIEW_InvalidateSelectedItems(LISTVIEW_INFO *infoPtr)
{
    RANGE visrange;
    INT i;

    visrange = LISTVIEW_GetVisibleRange(infoPtr);
    for (i = visrange.lower; i <= visrange.upper; i++)
    {
	if (LISTVIEW_GetItemState(infoPtr, i, LVIS_SELECTED))
	    LISTVIEW_InvalidateItem(infoPtr, i);
    }
}

	    
/***
 * DESCRIPTION:
 * Prints a message for unsupported window styles.
 * A kind of TODO list for window styles.
 *
 * PARAMETER(S):
 * [I] LONG : window style
 *
 * RETURN:
 * None
 */
static void LISTVIEW_UnsupportedStyles(LONG lStyle)
{
  if ((LVS_TYPESTYLEMASK & lStyle) == LVS_NOSCROLL)
    FIXME("  LVS_NOSCROLL\n");

  if (lStyle & LVS_NOLABELWRAP)
    FIXME("  LVS_NOLABELWRAP\n");

  if (lStyle & LVS_SORTASCENDING)
    FIXME("  LVS_SORTASCENDING\n");

  if (lStyle & LVS_SORTDESCENDING)
    FIXME("  LVS_SORTDESCENDING\n");
}

/***
 * DESCRIPTION:            [INTERNAL]
 * Compute sizes and rectangles of an item.  This is to localize all
 * the computations in one place. If you are not interested in some
 * of these values, simply pass in a NULL.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem  : item number
 * [O] lpptPosition : ptr to Position point
 *                the Position point is relative to infoPtr->rcList
 *                (has *NOT* been adjusted by the origin)
 * [O] lprcBoundary : ptr to Boundary rectangle
 *                the Boundary rectangle is relative to infoPtr->rcList
 *                (has *NOT* been adjusted by the origin)
 * [O] lprcIcon : ptr to Icon rectangle
 *                the Icon rectangle is relative to infoPtr->rcView
 *                (has already been adjusted by the origin)
 * [O] lprcLabel : ptr to Label rectangle
 *              - the Label rectangle is relative to infoPtr->rcView
 *                (has already been adjusted by the origin)
 *              - the Label rectangle encloses the label text only
 * [O] lprcText : ptr to FullText rectangle
 *              - the FullText rectangle is relative to infoPtr->rcView
 *                (has already been adjusted by the origin)
 *              - the FullText rectangle contains the Label rectangle
 *                but may be bigger. Used for filling the background.
 *
 * RETURN:
 *   TRUE if computations OK
 *   FALSE otherwise
 */
static BOOL LISTVIEW_GetItemMeasures(LISTVIEW_INFO *infoPtr, INT nItem,
				    LPPOINT lpptPosition,
				    LPRECT lprcBoundary, LPRECT lprcIcon,
				    LPRECT lprcLabel, LPRECT lprcText)
{
    LONG lStyle = infoPtr->dwStyle;
    UINT uView = lStyle & LVS_TYPEMASK;
    POINT Origin, Position, TopLeft;
    RECT Icon, Boundary, Label;
    INT nIndent = 0;

    if (!LISTVIEW_GetOrigin(infoPtr, &Origin)) return FALSE;

    if (uView == LVS_REPORT)
    {
	LVITEMW lvItem;

	lvItem.mask = LVIF_INDENT;
	lvItem.iItem = nItem;
	lvItem.iSubItem = 0;
	if (!LISTVIEW_GetItemW(infoPtr, &lvItem, TRUE)) return FALSE;

	/* do indent */
	nIndent = infoPtr->iconSize.cx * lvItem.iIndent;
    }

    /************************************************************/
    /* compute the top, left corner of the absolute boundry box */
    /************************************************************/
    if ((uView == LVS_SMALLICON) || (uView == LVS_ICON))
    {
	/* FIXME: what about virtual listview? */
	HDPA hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, nItem);
	LISTVIEW_ITEM * lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0);
	if (!lpItem) return FALSE;
	TopLeft = lpItem->ptPosition;
    }
    else if (uView == LVS_LIST)
    {
	INT nCountPerColumn;
	INT nRow, adjItem;

        adjItem = nItem - LISTVIEW_GetTopIndex(infoPtr);
        nCountPerColumn = LISTVIEW_GetCountPerColumn(infoPtr);
        if (adjItem < 0)
        {
	    nRow = adjItem % nCountPerColumn;
	    if (nRow == 0)
	    {
	        TopLeft.x = adjItem / nCountPerColumn * infoPtr->nItemWidth;
	        TopLeft.y = 0; /*FIXME: how so? */
	    }
	    else
	    {
	        TopLeft.x = (adjItem / nCountPerColumn -1) * infoPtr->nItemWidth;
	        TopLeft.y = (nRow + nCountPerColumn) * infoPtr->nItemHeight;
	    }
        }
        else
        {
	    TopLeft.x = adjItem / nCountPerColumn * infoPtr->nItemWidth;
	    TopLeft.y = adjItem % nCountPerColumn * infoPtr->nItemHeight;
        }
    }
    else  /* LVS_REPORT */
    {
        TopLeft.x = REPORT_MARGINX;
        TopLeft.y = ((nItem - LISTVIEW_GetTopIndex(infoPtr)) *
                        infoPtr->nItemHeight) + infoPtr->rcList.top;

        if (!(lStyle & LVS_NOSCROLL))
        {  /*FIXME: why not use Origin? */
	    SCROLLINFO scrollInfo;
	  
	    scrollInfo.cbSize = sizeof(SCROLLINFO);
	    scrollInfo.fMask = SIF_POS;
		  
	    if (GetScrollInfo(infoPtr->hwndSelf, SB_HORZ, &scrollInfo))
	        TopLeft.x -= scrollInfo.nPos;
        }
    }

    /************************************************************/
    /* compute position point (ala LVM_GETITEMPOSITION)         */
    /************************************************************/
    Position.x = TopLeft.x;
    Position.y = TopLeft.y;
    if (uView == LVS_ICON)
    {
        Position.y += ICON_TOP_PADDING;
        Position.x += (infoPtr->iconSpacing.cx - infoPtr->iconSize.cx) / 2;
    }
    if (lpptPosition) *lpptPosition = Position;
    TRACE("hwnd=%x, item=%d, position=(%ld,%ld)\n",
	  infoPtr->hwndSelf, nItem, Position.x, Position.y);

  /************************************************************/
  /* compute ICON bounding box (ala LVM_GETITEMRECT)          */
  /************************************************************/
  if (uView == LVS_ICON)
  {
      if (infoPtr->himlNormal != NULL)
      {
	  Icon.left   = Position.x + Origin.x - ICON_LR_HALF;
	  Icon.top    = Position.y + Origin.y - ICON_TOP_PADDING;
	  Icon.right  = Icon.left + infoPtr->iconSize.cx + ICON_LR_PADDING;
	  Icon.bottom = Icon.top + infoPtr->iconSize.cy + ICON_TOP_PADDING;
      }
      else return FALSE;
  }
  else if (uView == LVS_SMALLICON)
  {
      Icon.left   = Position.x + Origin.x;
      Icon.top    = Position.y + Origin.y;
      Icon.bottom = Icon.top + infoPtr->nItemHeight;

      if (infoPtr->himlState != NULL)
	  Icon.left += infoPtr->iconStateSize.cx;

      Icon.right = Icon.left;
      if (infoPtr->himlSmall != NULL)
	  Icon.right += infoPtr->iconSize.cx;
  }
  else /* LVS_LIST or LVS_REPORT */
  {
	/* FIXME: why is the one above relative to origin??? */
        Icon.left = Position.x;
        Icon.top = Position.y;
        Icon.bottom = Icon.top + infoPtr->nItemHeight;

        if (uView & LVS_REPORT)
	    Icon.left += nIndent;

        if (infoPtr->himlState != NULL)
	    Icon.left += infoPtr->iconStateSize.cx;

        Icon.right = Icon.left;
        if (infoPtr->himlSmall != NULL)
	    Icon.right += infoPtr->iconSize.cx;

  }
  if(lprcIcon) *lprcIcon = Icon;
  TRACE("hwnd=%x, item=%d, icon=%s\n", infoPtr->hwndSelf, nItem, debugrect(&Icon));

  /************************************************************/
  /* compute LABEL bounding box (ala LVM_GETITEMRECT)         */
  /************************************************************/
  if (uView == LVS_ICON)
  {
      if (infoPtr->himlNormal != NULL)
      {
	  INT nLabelWidth;
	  RECT FullText;

	  Label.left = TopLeft.x + Origin.x;
	  Label.top = TopLeft.y + Origin.y + ICON_TOP_PADDING_HITABLE +
	              infoPtr->iconSize.cy + ICON_BOTTOM_PADDING;

	  nLabelWidth = LISTVIEW_GetLabelWidth(infoPtr, nItem);
	  if (infoPtr->iconSpacing.cx - nLabelWidth > 1)
	  {
	      Label.left += (infoPtr->iconSpacing.cx - nLabelWidth) / 2;
	      Label.right = Label.left + nLabelWidth;
	      Label.bottom = Label.top + infoPtr->ntmHeight + 1;
	      Label.bottom += HEIGHT_PADDING;
	  }
	  else
	  {
	      Label.right = Label.left + infoPtr->nItemWidth;
	      Label.bottom = Label.top + infoPtr->nItemHeight + HEIGHT_PADDING;
	      LISTVIEW_UpdateLargeItemLabelRect (infoPtr, nItem, &Label);
	  }
	  FullText = Label;
	  InflateRect(&FullText, 2, 0);
	  if (lprcLabel) *lprcLabel = Label;
	  if (lprcText) *lprcText = FullText;
	  TRACE("hwnd=%x, item=%d, label=(%d,%d)-(%d,%d), fulltext=(%d,%d)-(%d,%d)\n",
		infoPtr->hwndSelf, nItem,
		Label.left, Label.top, Label.right, Label.bottom,
		FullText.left, FullText.top, FullText.right, FullText.bottom);
      }
      else return FALSE;
  }
  else if (uView == LVS_SMALLICON)
  {
      INT nLeftPos, nLabelWidth;

      nLeftPos = Label.left = Position.x + Origin.x;
      Label.top = Position.y + Origin.y;
      Label.bottom = Label.top + infoPtr->nItemHeight;

      if (infoPtr->himlState != NULL)
	  Label.left += infoPtr->iconStateSize.cx;

      if (infoPtr->himlSmall != NULL)
	  Label.left += infoPtr->iconSize.cx;

      nLabelWidth = LISTVIEW_GetLabelWidth(infoPtr, nItem);
      nLabelWidth += TRAILING_PADDING;
      if (Label.left + nLabelWidth < nLeftPos + infoPtr->nItemWidth)
	  Label.right = Label.left + nLabelWidth;
      else
	  Label.right = nLeftPos + infoPtr->nItemWidth;
      if (lprcLabel) *lprcLabel = Label;
      if (lprcText) *lprcText = Label;
      TRACE("hwnd=%x, item=%d, label=(%d,%d)-(%d,%d)\n",
	    infoPtr->hwndSelf, nItem,
	    Label.left, Label.top, Label.right, Label.bottom);
  }
  else /* LVS_LIST or LVS_REPORT */
  {
	INT nLabelWidth;

        if (uView != LVS_REPORT)
        {
	    nLabelWidth = LISTVIEW_GetLabelWidth(infoPtr, nItem);
            nLabelWidth += TRAILING_PADDING;
            if (infoPtr->himlSmall) nLabelWidth += IMAGE_PADDING;
        }
        else
            nLabelWidth = LISTVIEW_GetColumnWidth(infoPtr, 0) - Icon.left;
	
	Label.left = Icon.right;
        Label.top = Position.y;
	Label.right = Label.left + nLabelWidth;
        Label.bottom = Label.top + infoPtr->nItemHeight;

	if (Label.right - Position.x > infoPtr->nItemWidth)
	    Label.right = Position.x + infoPtr->nItemWidth;
        if (lprcLabel) *lprcLabel = Label;
        if (lprcText) *lprcText = Label;
  }
  
  TRACE("hwnd=%x, item=%d, label=%s\n", infoPtr->hwndSelf, nItem, debugrect(&Label));
  
    /***********************************************************/
    /* compute boundary box for the item (ala LVM_GETITEMRECT) */
    /***********************************************************/

    if (uView == LVS_REPORT)
    {
	Boundary.left = TopLeft.x;
	Boundary.top = TopLeft.y;
	Boundary.right = Boundary.left + infoPtr->nItemWidth;
	Boundary.bottom = Boundary.top + infoPtr->nItemHeight;
	if (!(infoPtr->dwLvExStyle & LVS_EX_FULLROWSELECT))
	    Boundary.left += nIndent;
	Boundary.right -= REPORT_MARGINX;
	if (Boundary.right < Boundary.left) Boundary.right = Boundary.left;
    }
    else
    {
	UnionRect(&Boundary, &Icon, &Label);
    }

    TRACE("hwnd=%x, item=%d, boundary=%s\n", infoPtr->hwndSelf, nItem, debugrect(&Boundary));
    if (lprcBoundary) *lprcBoundary = Boundary;

    return TRUE;
}

/***
 * DESCRIPTION:
 * Aligns the items with the top edge of the window.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 * None
 */
static void LISTVIEW_AlignTop(LISTVIEW_INFO *infoPtr)
{
  UINT uView = LISTVIEW_GetType(infoPtr);
  INT nListWidth = infoPtr->rcList.right - infoPtr->rcList.left;
  POINT ptItem;
  RECT rcView;
  INT i, off_x=0, off_y=0;

  if ((uView == LVS_SMALLICON) || (uView == LVS_ICON))
  {
    /* Since SetItemPosition uses upper-left of icon, and for
       style=LVS_ICON the icon is not left adjusted, get the offset */
    if (uView == LVS_ICON)
    {
      off_y = ICON_TOP_PADDING;
      off_x = (infoPtr->iconSpacing.cx - infoPtr->iconSize.cx) / 2;
    }
    ptItem.x = off_x;
    ptItem.y = off_y;
    ZeroMemory(&rcView, sizeof(RECT));
    TRACE("Icon  off.x=%d, off.y=%d, left=%d, right=%d\n",
	  off_x, off_y,
	  infoPtr->rcList.left, infoPtr->rcList.right);

    if (nListWidth > infoPtr->nItemWidth)
    {
      for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
      {
        if ((ptItem.x-off_x) + infoPtr->nItemWidth > nListWidth)
        {
          ptItem.x = off_x;
          ptItem.y += infoPtr->nItemHeight;
        }

        LISTVIEW_SetItemPosition(infoPtr, i, ptItem.x, ptItem.y);
        ptItem.x += infoPtr->nItemWidth;
        rcView.right = max(rcView.right, ptItem.x);
      }

      rcView.right -= off_x;
      rcView.bottom = (ptItem.y-off_y) + infoPtr->nItemHeight;
    }
    else
    {
      for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
      {
        LISTVIEW_SetItemPosition(infoPtr, i, ptItem.x, ptItem.y);
        ptItem.y += infoPtr->nItemHeight;
      }

      rcView.right = infoPtr->nItemWidth;
      rcView.bottom = ptItem.y-off_y;
    }

    infoPtr->rcView = rcView;
  }
}

/***
 * DESCRIPTION:
 * Aligns the items with the left edge of the window.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 * None
 */
static void LISTVIEW_AlignLeft(LISTVIEW_INFO *infoPtr)
{
  UINT uView = LISTVIEW_GetType(infoPtr);
  INT nListHeight = infoPtr->rcList.bottom - infoPtr->rcList.top;
  POINT ptItem;
  RECT rcView;
  INT i, off_x=0, off_y=0;

  if ((uView == LVS_SMALLICON) || (uView == LVS_ICON))
  {
    /* Since SetItemPosition uses upper-left of icon, and for
       style=LVS_ICON the icon is not left adjusted, get the offset */
    if (uView == LVS_ICON)
    {
      off_y = ICON_TOP_PADDING;
      off_x = (infoPtr->iconSpacing.cx - infoPtr->iconSize.cx) / 2;
    }
    ptItem.x = off_x;
    ptItem.y = off_y;
    ZeroMemory(&rcView, sizeof(RECT));
    TRACE("Icon  off.x=%d, off.y=%d\n", off_x, off_y);

    if (nListHeight > infoPtr->nItemHeight)
    {
      for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
      {
        if (ptItem.y + infoPtr->nItemHeight > nListHeight)
        {
          ptItem.y = off_y;
          ptItem.x += infoPtr->nItemWidth;
        }

        LISTVIEW_SetItemPosition(infoPtr, i, ptItem.x, ptItem.y);
        ptItem.y += infoPtr->nItemHeight;
        rcView.bottom = max(rcView.bottom, ptItem.y);
      }

      rcView.right = ptItem.x + infoPtr->nItemWidth;
    }
    else
    {
      for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
      {
        LISTVIEW_SetItemPosition(infoPtr, i, ptItem.x, ptItem.y);
        ptItem.x += infoPtr->nItemWidth;
      }

      rcView.bottom = infoPtr->nItemHeight;
      rcView.right = ptItem.x;
    }

    infoPtr->rcView = rcView;
  }
}


/***
 * DESCRIPTION:
 * Retrieves the bounding rectangle of all the items.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [O] lprcView : bounding rectangle
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_GetViewRect(LISTVIEW_INFO *infoPtr, LPRECT lprcView)
{
    POINT ptOrigin;

    TRACE("(lprcView=%p)\n", lprcView);

    if (!lprcView) return FALSE;
  
    if (!LISTVIEW_GetOrigin(infoPtr, &ptOrigin)) return FALSE;
   
    *lprcView = infoPtr->rcView;
    OffsetRect(lprcView, ptOrigin.x, ptOrigin.y); 

    TRACE("lprcView=%s\n", debugrect(lprcView));

    return TRUE;
}

/***
 * DESCRIPTION:
 * Retrieves the subitem pointer associated with the subitem index.
 *
 * PARAMETER(S):
 * [I] HDPA : DPA handle for a specific item
 * [I] INT : index of subitem
 *
 * RETURN:
 *   SUCCESS : subitem pointer
 *   FAILURE : NULL
 */
static LISTVIEW_SUBITEM* LISTVIEW_GetSubItemPtr(HDPA hdpaSubItems,
                                                INT nSubItem)
{
    LISTVIEW_SUBITEM *lpSubItem;
    INT i;

    /* we should binary search here if need be */
    for (i = 1; i < hdpaSubItems->nItemCount; i++)
    {
	lpSubItem = (LISTVIEW_SUBITEM *) DPA_GetPtr(hdpaSubItems, i);
	if (lpSubItem && (lpSubItem->iSubItem == nSubItem))
	    return lpSubItem;
    }

    return NULL;
}


/***
 * DESCRIPTION:
 * Calculates the width of a specific item.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem : item to calculate width, or -1 for max of all
 *
 * RETURN:
 * Returns the width of an item width an item.
 */
static INT LISTVIEW_CalculateWidth(LISTVIEW_INFO *infoPtr, INT nItem)
{
    UINT uView = LISTVIEW_GetType(infoPtr);
    INT nItemWidth = 0, i;

    if (uView == LVS_ICON) 
	nItemWidth = infoPtr->iconSpacing.cx;
    else if (uView == LVS_REPORT)
    {
	INT nHeaderItemCount;
	RECT rcHeaderItem;
	
	/* calculate width of header */
	nHeaderItemCount = Header_GetItemCount(infoPtr->hwndHeader);
	for (i = 0; i < nHeaderItemCount; i++)
	    if (Header_GetItemRect(infoPtr->hwndHeader, i, &rcHeaderItem))
		nItemWidth += (rcHeaderItem.right - rcHeaderItem.left);
    }
    else
    {
	INT nLabelWidth;
	
	if (GETITEMCOUNT(infoPtr) == 0) return DEFAULT_COLUMN_WIDTH;
    
        /* get width of string */
	if (nItem == -1) 
	{
	    for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
	    {
		nLabelWidth = LISTVIEW_GetLabelWidth(infoPtr, i);
		nItemWidth = max(nItemWidth, nLabelWidth);
	    }
	}
	else
            nItemWidth = LISTVIEW_GetLabelWidth(infoPtr, nItem);
        if (!nItemWidth)  return DEFAULT_COLUMN_WIDTH;
        nItemWidth += WIDTH_PADDING;
        if (infoPtr->himlSmall) nItemWidth += infoPtr->iconSize.cx; 
        if (infoPtr->himlState) nItemWidth += infoPtr->iconStateSize.cx;
	if (nItem == -1) nItemWidth = max(DEFAULT_COLUMN_WIDTH, nItemWidth);
    }

    return max(nItemWidth, 1);
}

/***
 * DESCRIPTION:
 * Calculates the max width of any item in the list.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] LONG : window style
 *
 * RETURN:
 * Returns item width.
 */
static inline INT LISTVIEW_GetItemWidth(LISTVIEW_INFO *infoPtr)
{
    return LISTVIEW_CalculateWidth(infoPtr, -1);
}

/***
 * DESCRIPTION:
 * Retrieves and saves important text metrics info for the current
 * Listview font.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 */
static void LISTVIEW_SaveTextMetrics(LISTVIEW_INFO *infoPtr)
{
  TEXTMETRICW tm;
  HDC hdc = GetDC(infoPtr->hwndSelf);
  HFONT hOldFont = SelectObject(hdc, infoPtr->hFont);
  INT oldHeight, oldACW;

  GetTextMetricsW(hdc, &tm);

  oldHeight = infoPtr->ntmHeight;
  oldACW = infoPtr->ntmAveCharWidth;
  infoPtr->ntmHeight = tm.tmHeight;
  infoPtr->ntmAveCharWidth = tm.tmAveCharWidth;

  SelectObject(hdc, hOldFont);
  ReleaseDC(infoPtr->hwndSelf, hdc);
  TRACE("tmHeight old=%d,new=%d; tmAveCharWidth old=%d,new=%d\n",
        oldHeight, infoPtr->ntmHeight, oldACW, infoPtr->ntmAveCharWidth);
}


/***
 * DESCRIPTION:
 * Calculates the height of an item.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 * Returns item height.
 */
static INT LISTVIEW_GetItemHeight(LISTVIEW_INFO *infoPtr)
{
    INT nItemHeight;

    if (LISTVIEW_GetType(infoPtr) == LVS_ICON)
	nItemHeight = infoPtr->iconSpacing.cy;
    else
    {
	nItemHeight = infoPtr->ntmHeight; 
	if (infoPtr->himlState)
	    nItemHeight = max(nItemHeight, infoPtr->iconStateSize.cy);
	if (infoPtr->himlSmall)
	    nItemHeight = max(nItemHeight, infoPtr->iconSize.cy);
	if (infoPtr->himlState || infoPtr->himlSmall)
	    nItemHeight += HEIGHT_PADDING;
    }
    return nItemHeight;
}

#if 0
static void LISTVIEW_PrintSelectionRanges(LISTVIEW_INFO *infoPtr)
{
  RANGE *selection;
  INT topSelection = infoPtr->hdpaSelectionRanges->nItemCount;
  INT i;

  TRACE("Selections are:\n");
  for (i = 0; i < topSelection; i++)
  {
    selection = DPA_GetPtr(infoPtr->hdpaSelectionRanges,i);
    TRACE("     %lu - %lu\n",selection->lower,selection->upper);
  }
}
#endif

/***
 * DESCRIPTION:
 * A compare function for selection ranges
 *
 *PARAMETER(S)
 * [I] range1 : pointer to selection range 1;
 * [I] range2 : pointer to selection range 2;
 * [I] flags : flags
 *
 *RETURNS:
 * >0 : if Item 1 > Item 2
 * <0 : if Item 2 > Item 1
 * 0 : if Item 1 == Item 2
 */
static INT CALLBACK LISTVIEW_CompareSelectionRanges(LPVOID range1, LPVOID range2, LPARAM flags)
{
    if (((RANGE*)range1)->upper < ((RANGE*)range2)->lower) 
	return -1;
    if (((RANGE*)range2)->upper < ((RANGE*)range1)->lower) 
	return 1;
    return 0;
}

/***
 * Helper function for LISTVIEW_AddSelectionRange, and LISTVIEW_SetItem.
 */
static BOOL add_selection_range(LISTVIEW_INFO *infoPtr, INT lower, INT upper, BOOL adj_sel_only)
{
    RANGE selection;
    LVITEMW lvItem;
    INT index, i;

    TRACE("range (%i - %i)\n", lower, upper);

    /* try find overlapping selections first */
    selection.lower = lower - 1;
    selection.upper = upper + 1;
    index = DPA_Search(infoPtr->hdpaSelectionRanges, &selection, 0,
		       LISTVIEW_CompareSelectionRanges, 0, 0);
   
    if (index == -1)
    {
	RANGE *newsel;

	/* create the brand new selection to insert */	
        newsel = (RANGE *)COMCTL32_Alloc(sizeof(RANGE));
	if(!newsel) return FALSE;
	newsel->lower = lower;
	newsel->upper = upper;
	
	/* figure out where to insert it */
	index = DPA_Search(infoPtr->hdpaSelectionRanges, newsel, 0,
			   LISTVIEW_CompareSelectionRanges, 0, DPAS_INSERTAFTER);
	if (index == -1) index = 0;
	
	/* and get it over with */
	DPA_InsertPtr(infoPtr->hdpaSelectionRanges, index, newsel);
    }
    else
    {
	RANGE *chksel, *mrgsel;
	INT fromindex, mergeindex;

	chksel = DPA_GetPtr(infoPtr->hdpaSelectionRanges, index);
	if (!chksel) return FALSE;
	TRACE("Merge with index %i (%d - %d)\n",
	      index, chksel->lower, chksel->upper);

	chksel->lower = min(lower, chksel->lower);
	chksel->upper = max(upper, chksel->upper);
	
	TRACE("New range %i (%d - %d)\n",
	      index, chksel->lower, chksel->upper);

        /* merge now common selection ranges */
	fromindex = 0;
	selection.lower = chksel->lower - 1;
	selection.upper = chksel->upper + 1;
	    
	do
	{
	    mergeindex = DPA_Search(infoPtr->hdpaSelectionRanges, &selection, fromindex,
				    LISTVIEW_CompareSelectionRanges, 0, 0);
	    if (mergeindex == -1) break;
	    if (mergeindex == index) 
	    {
		fromindex = index + 1;
		continue;
	    }
	  
	    TRACE("Merge with index %i\n", mergeindex);
	    
	    mrgsel = DPA_GetPtr(infoPtr->hdpaSelectionRanges, mergeindex);
	    if (!mrgsel) return FALSE;
	    
	    chksel->lower = min(chksel->lower, mrgsel->lower);
	    chksel->upper = max(chksel->upper, mrgsel->upper);
	    COMCTL32_Free(mrgsel);
	    DPA_DeletePtr(infoPtr->hdpaSelectionRanges, mergeindex);
	    if (mergeindex < index) index --;
	} while(1);
    }

    /*DPA_Sort(infoPtr->hdpaSelectionRanges, LISTVIEW_CompareSelectionRanges, 0);*/
   
    if (adj_sel_only) return TRUE;
   
    /* set the selection on items */
    lvItem.state = LVIS_SELECTED;
    lvItem.stateMask = LVIS_SELECTED;
    for(i = lower; i <= upper; i++)
	LISTVIEW_SetItemState(infoPtr, i, &lvItem);

    return TRUE;
}
   
/***
 * Helper function for LISTVIEW_RemoveSelectionRange, and LISTVIEW_SetItem.
 */
static BOOL remove_selection_range(LISTVIEW_INFO *infoPtr, INT lower, INT upper, BOOL adj_sel_only)
{
    RANGE remsel, tmpsel, *chksel;
    BOOL done = FALSE;
    LVITEMW lvItem;
    INT index, i;

    lvItem.state = 0;
    lvItem.stateMask = LVIS_SELECTED;
    
    remsel.lower = lower;
    remsel.upper = upper;

    TRACE("range: (%d - %d)\n", remsel.lower, remsel.upper);

    do 
    {
	index = DPA_Search(infoPtr->hdpaSelectionRanges, &remsel, 0,
			   LISTVIEW_CompareSelectionRanges, 0, 0);
	if (index == -1) return TRUE;

	chksel = DPA_GetPtr(infoPtr->hdpaSelectionRanges, index);
	if (!chksel) return FALSE;
	
        TRACE("Matches range index %i (%d - %d)\n", 
	      index, chksel->lower, chksel->upper);

	/* case 1: Same range */
	if ( (chksel->upper == remsel.upper) &&
	     (chksel->lower == remsel.lower) )
	{
	    DPA_DeletePtr(infoPtr->hdpaSelectionRanges, index);
	    done = TRUE;
	}
	/* case 2: engulf */
	else if ( (chksel->upper <= remsel.upper) &&
		  (chksel->lower >= remsel.lower) ) 
	{
	    DPA_DeletePtr(infoPtr->hdpaSelectionRanges, index);
	}
	/* case 3: overlap upper */
	else if ( (chksel->upper < remsel.upper) &&
		  (chksel->lower < remsel.lower) )
	{
	    chksel->upper = remsel.lower - 1;
	}
	/* case 4: overlap lower */
	else if ( (chksel->upper > remsel.upper) &&
		  (chksel->lower > remsel.lower) )
	{
	    chksel->lower = remsel.upper + 1;
	}
	/* case 5: fully internal */
	else
	{
	    RANGE *newsel = 
		(RANGE *)COMCTL32_Alloc(sizeof(RANGE));
	    if (!newsel) return FALSE;
	    tmpsel = *chksel;
	    newsel->lower = chksel->lower;
	    newsel->upper = remsel.lower - 1;
	    chksel->lower = remsel.upper + 1;
	    DPA_InsertPtr(infoPtr->hdpaSelectionRanges, index, newsel);
	    /*DPA_Sort(infoPtr->hdpaSelectionRanges, LISTVIEW_CompareSelectionRanges, 0);*/
	    chksel = &tmpsel;
	}

	if (adj_sel_only) continue;
	
	/* here, chksel holds the selection to delete */
	for (i = chksel->lower; i <= chksel->upper; i++)
	    LISTVIEW_SetItemState(infoPtr, i, &lvItem);
    }
    while(!done);

    return TRUE;
}

/**
* DESCRIPTION:
* Adds a selection range.
*
* PARAMETER(S):
* [I] infoPtr : valid pointer to the listview structure
* [I] lower : lower item index
* [I] upper : upper item index
*
* RETURN:
*   Success: TRUE
*   Failure: FALSE
*/
static inline BOOL LISTVIEW_AddSelectionRange(LISTVIEW_INFO *infoPtr, INT lower, INT upper)
{
    return add_selection_range(infoPtr, lower, upper, FALSE);
}

/***
* DESCRIPTION:
* Removes a range selections.
*
* PARAMETER(S):
* [I] infoPtr : valid pointer to the listview structure
* [I] lower : lower item index
* [I] upper : upper item index
*
* RETURN:
*   Success: TRUE
*   Failure: FALSE
*/
static inline BOOL LISTVIEW_RemoveSelectionRange(LISTVIEW_INFO *infoPtr, INT lower, INT upper)
{
    return remove_selection_range(infoPtr, lower, upper, FALSE);
}

/***
* DESCRIPTION:
* Removes all selection ranges
*
* Parameters(s):
* [I] infoPtr : valid pointer to the listview structure
*
* RETURNS:
*   SUCCESS : TRUE
*   FAILURE : TRUE
*/
static LRESULT LISTVIEW_RemoveAllSelections(LISTVIEW_INFO *infoPtr)
{
    RANGE *sel;
    static BOOL removing_all_selections = FALSE;

    if (removing_all_selections) return TRUE;

    removing_all_selections = TRUE;

    TRACE("()\n");

    do
    {
	sel = DPA_GetPtr(infoPtr->hdpaSelectionRanges, 0);
	if (sel) LISTVIEW_RemoveSelectionRange(infoPtr, sel->lower, sel->upper);
    }
    while (infoPtr->hdpaSelectionRanges->nItemCount > 0);

    removing_all_selections = FALSE;

    return TRUE;
}

/***
 * DESCRIPTION:
 * Manages the item focus.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT : item index
 *
 * RETURN:
 *   TRUE : focused item changed
 *   FALSE : focused item has NOT changed
 */
static inline BOOL LISTVIEW_SetItemFocus(LISTVIEW_INFO *infoPtr, INT nItem)
{
    INT oldFocus = infoPtr->nFocusedItem;
    LVITEMW lvItem;

    lvItem.state =  LVIS_FOCUSED;
    lvItem.stateMask = LVIS_FOCUSED;
    LISTVIEW_SetItemState(infoPtr, nItem, &lvItem);

    return oldFocus != infoPtr->nFocusedItem;
}

/**
* DESCRIPTION:
* Updates the various indices after an item has been inserted or deleted.
*
* PARAMETER(S):
* [I] infoPtr : valid pointer to the listview structure
* [I] nItem : item index
* [I] direction : Direction of shift, +1 or -1.
*
* RETURN:
* None
*/
static void LISTVIEW_ShiftIndices(LISTVIEW_INFO *infoPtr, INT nItem, INT direction)
{
  RANGE selection,*checkselection;
  INT index;

  TRACE("Shifting %iu, %i steps\n",nItem,direction);

  selection.upper = nItem;
  selection.lower = nItem;

  index = DPA_Search(infoPtr->hdpaSelectionRanges, &selection, 0,
                     LISTVIEW_CompareSelectionRanges,
                     0,DPAS_SORTED|DPAS_INSERTAFTER);

  while ((index < infoPtr->hdpaSelectionRanges->nItemCount)&&(index != -1))
  {
    checkselection = DPA_GetPtr(infoPtr->hdpaSelectionRanges,index);
    if ((checkselection->lower >= nItem)&&
       ((int)(checkselection->lower + direction) >= 0))
        checkselection->lower += direction;
    if ((checkselection->upper >= nItem)&&
       ((int)(checkselection->upper + direction) >= 0))
        checkselection->upper += direction;
    index ++;
  }

  /* Note that the following will fail if direction != +1 and -1 */
  if (infoPtr->nSelectionMark > nItem)
      infoPtr->nSelectionMark += direction;
  else if (infoPtr->nSelectionMark == nItem)
  {
    if (direction > 0)
      infoPtr->nSelectionMark += direction;
    else if (infoPtr->nSelectionMark >= GETITEMCOUNT(infoPtr))
      infoPtr->nSelectionMark = GETITEMCOUNT(infoPtr) - 1;
  }

  if (infoPtr->nFocusedItem > nItem)
    infoPtr->nFocusedItem += direction;
  else if (infoPtr->nFocusedItem == nItem)
  {
    if (direction > 0)
      infoPtr->nFocusedItem += direction;
    else
    {
      if (infoPtr->nFocusedItem >= GETITEMCOUNT(infoPtr))
        infoPtr->nFocusedItem = GETITEMCOUNT(infoPtr) - 1;
      if (infoPtr->nFocusedItem >= 0)
        LISTVIEW_SetItemFocus(infoPtr, infoPtr->nFocusedItem);
    }
  }
  /* But we are not supposed to modify nHotItem! */
}


/**
 * DESCRIPTION:
 * Adds a block of selections.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT : item index
 *
 * RETURN:
 * None
 */
static void LISTVIEW_AddGroupSelection(LISTVIEW_INFO *infoPtr, INT nItem)
{
  INT nFirst = min(infoPtr->nSelectionMark, nItem);
  INT nLast = max(infoPtr->nSelectionMark, nItem);
  INT i;
  LVITEMW item;

  if (nFirst == -1)
    nFirst = nItem;

  item.state = LVIS_SELECTED;
  item.stateMask = LVIS_SELECTED;

  /* FIXME: this is not correct LVS_OWNERDATA
   * See docu for LVN_ITEMCHANGED. Is there something similar for
   * RemoveGroupSelection (is there such a thing?)?
   */
  for (i = nFirst; i <= nLast; i++)
    LISTVIEW_SetItemState(infoPtr,i,&item);

  LISTVIEW_SetItemFocus(infoPtr, nItem);
  infoPtr->nSelectionMark = nItem;
}


/***
 * DESCRIPTION:
 * Sets a single group selection.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT : item index
 *
 * RETURN:
 * None
 */
static void LISTVIEW_SetGroupSelection(LISTVIEW_INFO *infoPtr, INT nItem)
{
    UINT uView = LISTVIEW_GetType(infoPtr);
    INT i, nFirst, nLast;
    LVITEMW item;
    POINT ptItem;
    RECT rcSel;

    if ((uView == LVS_LIST) || (uView == LVS_REPORT))
    {
	if (infoPtr->nSelectionMark == -1)
	    infoPtr->nSelectionMark = nFirst = nLast = nItem;
	else
	{
	    nFirst = min(infoPtr->nSelectionMark, nItem);
	    nLast = max(infoPtr->nSelectionMark, nItem);
	}
    }
    else
    {
	RECT rcItem, rcSelMark;
	
	rcItem.left = LVIR_BOUNDS;
	if (!LISTVIEW_GetItemRect(infoPtr, nItem, &rcItem)) return;
	rcSelMark.left = LVIR_BOUNDS;
	if (!LISTVIEW_GetItemRect(infoPtr, infoPtr->nSelectionMark, &rcSelMark)) return;
	UnionRect(&rcSel, &rcItem, &rcSelMark);
	nFirst = nLast = -1;
    }

    item.stateMask = LVIS_SELECTED;

    for (i = 0; i <= GETITEMCOUNT(infoPtr); i++)
    {
	if (nFirst > -1) 
	    item.state = (i < nFirst) || (i > nLast) ? 0 : LVIS_SELECTED;
	else
	{
	    LISTVIEW_GetItemPosition(infoPtr, i, &ptItem);
	    item.state = PtInRect(&rcSel, ptItem) ? LVIS_SELECTED : 0;
	}
	LISTVIEW_SetItemState(infoPtr, i, &item);
    }
    LISTVIEW_SetItemFocus(infoPtr, nItem);
}

/***
 * DESCRIPTION:
 * Sets a single selection.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT : item index
 *
 * RETURN:
 * None
 */
static void LISTVIEW_SetSelection(LISTVIEW_INFO *infoPtr, INT nItem)
{
    LVITEMW lvItem;

    LISTVIEW_RemoveAllSelections(infoPtr);

    lvItem.state = LVIS_FOCUSED | LVIS_SELECTED;
    lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
    LISTVIEW_SetItemState(infoPtr, nItem, &lvItem);

    infoPtr->nSelectionMark = nItem;
}

/***
 * DESCRIPTION:
 * Set selection(s) with keyboard.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT : item index
 *
 * RETURN:
 *   SUCCESS : TRUE (needs to be repainted)
 *   FAILURE : FALSE (nothing has changed)
 */
static BOOL LISTVIEW_KeySelection(LISTVIEW_INFO *infoPtr, INT nItem)
{
  /* FIXME: pass in the state */
  LONG lStyle = infoPtr->dwStyle;
  WORD wShift = HIWORD(GetKeyState(VK_SHIFT));
  WORD wCtrl = HIWORD(GetKeyState(VK_CONTROL));
  BOOL bResult = FALSE;

  if ((nItem >= 0) && (nItem < GETITEMCOUNT(infoPtr)))
  {
    if (lStyle & LVS_SINGLESEL)
    {
      bResult = TRUE;
      LISTVIEW_SetSelection(infoPtr, nItem);
      ListView_EnsureVisible(infoPtr->hwndSelf, nItem, FALSE);
    }
    else
    {
      if (wShift)
      {
        bResult = TRUE;
        LISTVIEW_SetGroupSelection(infoPtr, nItem);
      }
      else if (wCtrl)
      {
        bResult = LISTVIEW_SetItemFocus(infoPtr, nItem);
      }
      else
      {
        bResult = TRUE;
        LISTVIEW_SetSelection(infoPtr, nItem);
        ListView_EnsureVisible(infoPtr->hwndSelf, nItem, FALSE);
      }
    }
  }

  UpdateWindow(infoPtr->hwndSelf); /* update client area */
  return bResult;
}

/***
 * DESCRIPTION:
 * Selects an item based on coordinates.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] pt : mouse click ccordinates
 *
 * RETURN:
 *   SUCCESS : item index
 *   FAILURE : -1
 */
static INT LISTVIEW_GetItemAtPt(LISTVIEW_INFO *infoPtr, POINT pt)
{
    RANGE visrange;
    RECT rcItem;
    INT i;

    visrange = LISTVIEW_GetVisibleRange(infoPtr);
    for (i = visrange.lower; i <= visrange.upper; i++)
    {
	rcItem.left = LVIR_SELECTBOUNDS;
	if (LISTVIEW_GetItemRect(infoPtr, i, &rcItem))
	{
	    TRACE("i=%d, rcItem=%s\n", i, debugrect(&rcItem));
	    if (PtInRect(&rcItem, pt)) return i;
	}
    }
    return -1;
}

/***
 * DESCRIPTION:
 * Called when the mouse is being actively tracked and has hovered for a specified
 * amount of time
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] fwKeys : key indicator
 * [I] pts : mouse position
 *
 * RETURN:
 *   0 if the message was processed, non-zero if there was an error
 *
 * INFO:
 * LVS_EX_TRACKSELECT: An item is automatically selected when the cursor remains
 * over the item for a certain period of time.
 *
 */
static LRESULT LISTVIEW_MouseHover(LISTVIEW_INFO *infoPtr, WORD fwKyes, POINTS pts)
{
    if(infoPtr->dwLvExStyle & LVS_EX_TRACKSELECT)
	/* FIXME: select the item!!! */
	/*LISTVIEW_GetItemAtPt(infoPtr, pt)*/;

    return 0;
}

/***
 * DESCRIPTION:
 * Called whenever WM_MOUSEMOVE is received.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] fwKeys : key indicator
 * [I] pts : mouse position
 *
 * RETURN:
 *   0 if the message is processed, non-zero if there was an error
 */
static LRESULT LISTVIEW_MouseMove(LISTVIEW_INFO *infoPtr, WORD fwKeys, POINTS pts)
{
  TRACKMOUSEEVENT trackinfo;

  /* see if we are supposed to be tracking mouse hovering */
  if(infoPtr->dwLvExStyle & LVS_EX_TRACKSELECT) {
     /* fill in the trackinfo struct */
     trackinfo.cbSize = sizeof(TRACKMOUSEEVENT);
     trackinfo.dwFlags = TME_QUERY;
     trackinfo.hwndTrack = infoPtr->hwndSelf;
     trackinfo.dwHoverTime = infoPtr->dwHoverTime;

     /* see if we are already tracking this hwnd */
     _TrackMouseEvent(&trackinfo);

     if(!(trackinfo.dwFlags & TME_HOVER)) {
       trackinfo.dwFlags = TME_HOVER;

       /* call TRACKMOUSEEVENT so we receive WM_MOUSEHOVER messages */
       _TrackMouseEvent(&trackinfo);
    }
  }

  return 0;
}


 /***
  * DESCRIPTION:          [INTERNAL]
  * Sets rectangle that the item was last drawn at.
  *
  * PARAMETER(S):
  * [I] HWND : window handle
  * [I] INT : item index
  * [I] LPRECT : coordinate information
  *
  * RETURN:
  *   SUCCESS : TRUE
  *   FAILURE : FALSE
  */
static BOOL LISTVIEW_SetItemDrawRect(LISTVIEW_INFO *infoPtr, INT nItem, LPRECT lpRect)
{
  BOOL bResult = FALSE;
  HDPA hdpaSubItems;
  LISTVIEW_ITEM *lpItem;

  TRACE("(hwnd=%x,nItem=%d,rect=(%d,%d)-(%d,%d))\n",
	infoPtr->hwndSelf, nItem,
	lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);

  if ((nItem >= 0) && (nItem < GETITEMCOUNT(infoPtr)) && (lpRect != NULL))
  {
    if ((hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, nItem)))
    {
      if ((lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0)))
      {
        bResult = TRUE;
        lpItem->rcLastDraw = *lpRect;
      }
    }
  }
  return bResult;
}

 /***
  * DESCRIPTION:          [INTERNAL]
  * Gets rectangle that the item was last drawn at.
  *
  * PARAMETER(S):
  * [I] HWND : window handle
  * [I] INT : item index
  * [O] LPRECT : coordinate information
  *
  * RETURN:
  *   SUCCESS : TRUE
  *   FAILURE : FALSE
  */
static BOOL LISTVIEW_GetItemDrawRect(LISTVIEW_INFO *infoPtr, INT nItem, LPRECT lpRect)
{
  BOOL bResult = FALSE;
  HDPA hdpaSubItems;
  LISTVIEW_ITEM *lpItem;

  if ((nItem >= 0) && (nItem < GETITEMCOUNT(infoPtr)) && (lpRect != NULL))
  {
    if ((hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, nItem)))
    {
      if ((lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0)))
      {
        bResult = TRUE;
        if (lpRect) *lpRect = lpItem->rcLastDraw;
	TRACE("(hwnd=%x,nItem=%d,rect=(%d,%d)-(%d,%d))\n",
	      infoPtr->hwndSelf, nItem,
	      lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
      }
    }
  }
  return bResult;
}


/***
 * Tests wheather the item is assignable to a list with style lStyle 
 */
static inline BOOL is_assignable_item(LPLVITEMW lpLVItem, LONG lStyle)
{
    if ( (lpLVItem->mask & LVIF_TEXT) && 
	(lpLVItem->pszText == LPSTR_TEXTCALLBACKW) &&
	(lStyle & (LVS_SORTASCENDING | LVS_SORTDESCENDING)) ) return FALSE;
    
    return TRUE;
}

/***
 * DESCRIPTION:
 * Helper for LISTVIEW_SetItemT *only*: sets item attributes.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] lpLVItem : valid pointer to new item atttributes
 * [I] isW : TRUE if lpLVItem is Unicode, FALSE if it's ANSI
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL set_owner_item(LISTVIEW_INFO *infoPtr, LPLVITEMW lpLVItem, BOOL isW)
{
    LONG lStyle = infoPtr->dwStyle;
    NMLISTVIEW nmlv;
    INT oldState;

    /* a virtual livst view stores only state for the main item */
    if (lpLVItem->iSubItem || !(lpLVItem->mask & LVIF_STATE)) return FALSE;

    oldState = LISTVIEW_GetItemState(infoPtr, lpLVItem->iItem, LVIS_FOCUSED | LVIS_SELECTED);

    /* we're done if we don't need to change anything we handle */
    if ( (oldState ^ lpLVItem->state) & lpLVItem->stateMask &
         ~infoPtr->uCallbackMask & (LVIS_FOCUSED | LVIS_SELECTED)) return FALSE;

    /*
     * As per MSDN LVN_ITEMCHANGING notifications are _NOT_ sent for
     * by LVS_OWERNDATA list controls
     */

    /* if we handle the focus, and we're asked to change it, do it now */
    if ( lpLVItem->stateMask & LVIS_FOCUSED )
    {
        if (lpLVItem->state & LVIS_FOCUSED)
	    infoPtr->nFocusedItem = lpLVItem->iItem;
        else if (infoPtr->nFocusedItem == lpLVItem->iItem)
	    infoPtr->nFocusedItem = -1;
    }
    
    /* and the selection is the only other state a virtual list may hold */
    if (lpLVItem->stateMask & LVIS_SELECTED)
    {
        if (lpLVItem->state & LVIS_SELECTED)
        {
	    if (lStyle & LVS_SINGLESEL) LISTVIEW_RemoveAllSelections(infoPtr);
	    add_selection_range(infoPtr, lpLVItem->iItem, lpLVItem->iItem, TRUE);
        }
        else
	remove_selection_range(infoPtr, lpLVItem->iItem, lpLVItem->iItem, TRUE);
    }

    /* notify the parent now that things have changed */
    ZeroMemory(&nmlv, sizeof(nmlv));
    nmlv.iItem = lpLVItem->iItem;
    nmlv.uNewState = lpLVItem->state;
    nmlv.uOldState = oldState;
    nmlv.uChanged = LVIF_STATE;
    notify_listview(infoPtr, LVN_ITEMCHANGED, &nmlv);

    return TRUE;
}

/***
 * DESCRIPTION:
 * Helper for LISTVIEW_SetItemT *only*: sets item attributes.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] lpLVItem : valid pointer to new item atttributes
 * [I] isW : TRUE if lpLVItem is Unicode, FALSE if it's ANSI
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL set_main_item(LISTVIEW_INFO *infoPtr, LPLVITEMW lpLVItem, BOOL isW)
{
    LONG lStyle = infoPtr->dwStyle;
    UINT uView = lStyle & LVS_TYPEMASK;
    HDPA hdpaSubItems;
    LISTVIEW_ITEM *lpItem;
    NMLISTVIEW nmlv;
    UINT uChanged = 0;

    hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, lpLVItem->iItem);
    if (!hdpaSubItems && hdpaSubItems != (HDPA)-1) return FALSE;
    
    lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0);
    if (!lpItem) return FALSE;

    /* determine what fields will change */    
    if ((lpLVItem->mask & LVIF_STATE) &&
	((lpItem->state ^ lpLVItem->state) & lpLVItem->stateMask))
	uChanged |= LVIF_STATE;

    if ((lpLVItem->mask & LVIF_IMAGE) && (lpItem->hdr.iImage != lpLVItem->iImage))
	uChanged |= LVIF_IMAGE;

    if ((lpLVItem->mask & LVIF_PARAM) && (lpItem->lParam != lpLVItem->lParam))
	uChanged |= LVIF_PARAM;

    if ((lpLVItem->mask & LVIF_INDENT) && (lpItem->iIndent != lpLVItem->iIndent))
	uChanged |= LVIF_INDENT;

    if ((lpLVItem->mask & LVIF_TEXT) && textcmpWT(lpItem->hdr.pszText, lpLVItem->pszText, isW))
	uChanged |= LVIF_TEXT;
    
    if (!uChanged) return TRUE;
    
    ZeroMemory(&nmlv, sizeof(NMLISTVIEW));
    nmlv.iItem = lpLVItem->iItem;
    nmlv.uNewState = lpLVItem->state & lpLVItem->stateMask;
    nmlv.uOldState = lpItem->state & lpLVItem->stateMask;
    nmlv.uChanged = uChanged;
    nmlv.lParam = lpItem->lParam;
    
    /* send LVN_ITEMCHANGING notification, if the item is not being inserted */
    if(lpItem->valid && notify_listview(infoPtr, LVN_ITEMCHANGING, &nmlv)) 
	return FALSE;

    /* copy information */
    if (lpLVItem->mask & LVIF_TEXT)
        textsetptrT(&lpItem->hdr.pszText, lpLVItem->pszText, isW);

    if (lpLVItem->mask & LVIF_IMAGE)
	lpItem->hdr.iImage = lpLVItem->iImage;

    if (lpLVItem->mask & LVIF_PARAM)
	lpItem->lParam = lpLVItem->lParam;

    if (lpLVItem->mask & LVIF_INDENT)
	lpItem->iIndent = lpLVItem->iIndent;

    if (uChanged & LVIF_STATE)
    {
	lpItem->state &= ~lpLVItem->stateMask;
	lpItem->state |= (lpLVItem->state & lpLVItem->stateMask);
	if (nmlv.uNewState & LVIS_SELECTED)
	{
	    if (lStyle & LVS_SINGLESEL) LISTVIEW_RemoveAllSelections(infoPtr);
	    add_selection_range(infoPtr, lpLVItem->iItem, lpLVItem->iItem, TRUE);
	}
	else if (lpLVItem->stateMask & LVIS_SELECTED)
	    remove_selection_range(infoPtr, lpLVItem->iItem, lpLVItem->iItem, TRUE);
	
	/* if we are asked to change focus, and we manage it, do it */
	if (nmlv.uNewState & ~infoPtr->uCallbackMask & LVIS_FOCUSED)
	{
	    if (lpLVItem->state & LVIS_FOCUSED)
	    {
		infoPtr->nFocusedItem = lpLVItem->iItem;
    	        LISTVIEW_EnsureVisible(infoPtr, lpLVItem->iItem, FALSE);
	    }
	    else if (infoPtr->nFocusedItem == lpLVItem->iItem)
		infoPtr->nFocusedItem = -1;
	}
    }

    /* if LVS_LIST or LVS_SMALLICON, update the width of the items */
    if((uChanged & LVIF_TEXT) && ((uView == LVS_LIST) || (uView == LVS_SMALLICON)))
    {
	int item_width = LISTVIEW_CalculateWidth(infoPtr, lpLVItem->iItem);
	if(item_width > infoPtr->nItemWidth) infoPtr->nItemWidth = item_width;
    }

    /* if we're inserting the item, we're done */
    if (!lpItem->valid) return TRUE;
    
    /* send LVN_ITEMCHANGED notification */
    nmlv.lParam = lpItem->lParam;
    notify_listview(infoPtr, LVN_ITEMCHANGED, &nmlv);

    return TRUE;
}

/***
 * DESCRIPTION:
 * Helper for LISTVIEW_SetItemT *only*: sets subitem attributes.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] lpLVItem : valid pointer to new subitem atttributes
 * [I] isW : TRUE if lpLVItem is Unicode, FALSE if it's ANSI
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL set_sub_item(LISTVIEW_INFO *infoPtr, LPLVITEMW lpLVItem, BOOL isW)
{
    HDPA hdpaSubItems;
    LISTVIEW_SUBITEM *lpSubItem;

    /* set subitem only if column is present */
    if (Header_GetItemCount(infoPtr->hwndHeader) <= lpLVItem->iSubItem) 
	return FALSE;
   
    /* First do some sanity checks */
    if (lpLVItem->mask & ~(LVIF_TEXT | LVIF_IMAGE)) return FALSE;
   
    /* get the subitem structure, and create it if not there */
    hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, lpLVItem->iItem);
    if (!hdpaSubItems) return FALSE;
    
    lpSubItem = LISTVIEW_GetSubItemPtr(hdpaSubItems, lpLVItem->iSubItem);
    if (!lpSubItem)
    {
	LISTVIEW_SUBITEM *tmpSubItem;
	INT i;

	lpSubItem = (LISTVIEW_SUBITEM *)COMCTL32_Alloc(sizeof(LISTVIEW_SUBITEM));
	if (!lpSubItem) return FALSE;
	/* we could binary search here, if need be...*/
  	for (i = 1; i < hdpaSubItems->nItemCount; i++)
  	{
	    tmpSubItem = (LISTVIEW_SUBITEM *)DPA_GetPtr(hdpaSubItems, i);
	    if (tmpSubItem && tmpSubItem->iSubItem > lpLVItem->iSubItem) break;
  	}
	if (DPA_InsertPtr(hdpaSubItems, i, lpSubItem) == -1)
	{
	    COMCTL32_Free(lpSubItem);
	    return FALSE;
	}
        lpSubItem->iSubItem = lpLVItem->iSubItem;
    }
    
    if (lpLVItem->mask & LVIF_IMAGE)
	lpSubItem->hdr.iImage = lpLVItem->iImage;

    if (lpLVItem->mask & LVIF_TEXT)
	textsetptrT(&lpSubItem->hdr.pszText, lpLVItem->pszText, isW);
	  
    return TRUE;
}

/***
 * DESCRIPTION:
 * Sets item attributes.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] LPLVITEM : new item atttributes
 * [I] isW : TRUE if lpLVItem is Unicode, FALSE if it's ANSI
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetItemT(LISTVIEW_INFO *infoPtr, LPLVITEMW lpLVItem, BOOL isW)
{
    INT oldFocus = infoPtr->nFocusedItem;
    LPWSTR pszText = NULL;
    BOOL bResult;
    
    TRACE("(lpLVItem=%s, isW=%d)\n", debuglvitem_t(lpLVItem, isW), isW);

    if (!lpLVItem || lpLVItem->iItem < 0 ||
	lpLVItem->iItem>=GETITEMCOUNT(infoPtr))
	return FALSE;
   
    /* For efficiency, we transform the lpLVItem->pszText to Unicode here */
    if ((lpLVItem->mask & LVIF_TEXT) && is_textW(lpLVItem->pszText))
    {
	pszText = lpLVItem->pszText;
	lpLVItem->pszText = textdupTtoW(lpLVItem->pszText, isW);
    }
    
    /* actually set the fields */
    if (infoPtr->dwStyle & LVS_OWNERDATA)
	bResult = set_owner_item(infoPtr, lpLVItem, TRUE);
    else
    {
	/* sanity checks first */
	if (!is_assignable_item(lpLVItem, infoPtr->dwStyle)) return FALSE;
    
	if (lpLVItem->iSubItem)
	    bResult = set_sub_item(infoPtr, lpLVItem, TRUE);
	else
	    bResult = set_main_item(infoPtr, lpLVItem, TRUE);
    }

    /* redraw item, if necessary */
    if (bResult && !infoPtr->bIsDrawing)
    {
	RECT rcOldItem={0,0,0,0};

	if (oldFocus != infoPtr->nFocusedItem && infoPtr->bFocus)
	    LISTVIEW_ToggleFocusRect(infoPtr);

	/* Note that ->rcLastDraw is normally all zero, so
	 * no second InvalidateRect is issued.
	 *
	 * However, when a large icon style is drawn (LVS_ICON),
	 * the rectangle drawn is saved in rcLastDraw. That way
	 * the InvalidateRect will invalidate the entire area drawn
	 *
	 * FIXME: this is not right. We have already a ton of rects,
	 *        we have two functions to get to them (GetItemRect,
	 *        and GetItemMeasurements), and we introduce yet
	 *        another rectangle with setter/getter functions!!!
	 *        This is too much. Besides, this does not work
	 *        correctly for owner drawn control...
	 */
	if ((oldFocus >= 0) && (oldFocus < GETITEMCOUNT(infoPtr)))
	{
	    LISTVIEW_GetItemDrawRect(infoPtr, oldFocus, &rcOldItem);
	    if(!IsRectEmpty(&rcOldItem))
		LISTVIEW_InvalidateRect(infoPtr, &rcOldItem);
	}
	LISTVIEW_InvalidateItem(infoPtr, lpLVItem->iItem);
    }
    /* restore text */
    if (pszText)
    {
	textfreeT(lpLVItem->pszText, isW);
	lpLVItem->pszText = pszText;
    }

    return bResult;
}

/***
 * DESCRIPTION:
 * Retrieves the index of the item at coordinate (0, 0) of the client area.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 * item index
 */
static INT LISTVIEW_GetTopIndex(LISTVIEW_INFO *infoPtr)
{
    LONG lStyle = infoPtr->dwStyle;
    UINT uView = lStyle & LVS_TYPEMASK;
    INT nItem = 0;
    SCROLLINFO scrollInfo;

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_POS;

    if (uView == LVS_LIST)
    {
	if ((lStyle & WS_HSCROLL) && GetScrollInfo(infoPtr->hwndSelf, SB_HORZ, &scrollInfo))
	    nItem = scrollInfo.nPos * LISTVIEW_GetCountPerColumn(infoPtr);
    }
    else if (uView == LVS_REPORT)
    {
	if ((lStyle & WS_VSCROLL) && GetScrollInfo(infoPtr->hwndSelf, SB_VERT, &scrollInfo))
	    nItem = scrollInfo.nPos;
    } 
    else
    {
	if ((lStyle & WS_VSCROLL) && GetScrollInfo(infoPtr->hwndSelf, SB_VERT, &scrollInfo))
	    nItem = LISTVIEW_GetCountPerRow(infoPtr) * (scrollInfo.nPos / infoPtr->nItemHeight);
    }

    TRACE("nItem=%d\n", nItem);
    
    return nItem;
}

/* used by the drawing code */
typedef struct tagTEXTATTR
{
    int      bkMode;
    COLORREF bkColor;
    COLORREF fgColor;
} TEXTATTR;

/* helper function for the drawing code */
static inline void set_text_attr(HDC hdc, TEXTATTR *ta)
{
    ta->bkMode = SetBkMode(hdc, ta->bkMode);
    ta->bkColor = SetBkColor(hdc, ta->bkColor);
    ta->fgColor = SetTextColor(hdc, ta->fgColor);
}

/* helper function for the drawing code */
static void select_text_attr(LISTVIEW_INFO *infoPtr, HDC hdc, BOOL isSelected, TEXTATTR *ta)
{
    ta->bkMode = OPAQUE;

    if (isSelected && infoPtr->bFocus)
    {
	ta->bkColor = comctl32_color.clrHighlight;
	ta->fgColor = comctl32_color.clrHighlightText;
    }
    else if (isSelected && (infoPtr->dwStyle & LVS_SHOWSELALWAYS))
    {
	ta->bkColor = comctl32_color.clr3dFace;
	ta->fgColor = comctl32_color.clrBtnText;
    }
    else if ( (infoPtr->clrTextBk != CLR_DEFAULT) && (infoPtr->clrTextBk != CLR_NONE) )
    {
	ta->bkColor = infoPtr->clrTextBk;
	ta->fgColor = infoPtr->clrText;
    }
    else
    {
	ta->bkMode  = TRANSPARENT;
	ta->bkColor = GetBkColor(hdc);
	ta->fgColor = infoPtr->clrText;
    }

    set_text_attr(hdc, ta);
}

/***
 * DESCRIPTION:
 * Erases the background of the given rectangle
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] hdc : device context handle
 * [I] lprcBox : clipping rectangle
 *
 * RETURN:
 *   Success: TRUE
 *   Failure: FALSE
 */
static inline BOOL LISTVIEW_FillBkgnd(LISTVIEW_INFO *infoPtr, HDC hdc, const RECT* lprcBox)
{
    if (!infoPtr->hBkBrush) return FALSE;

    TRACE("(hdc=%x, lprcBox=%s, hBkBrush=%x)\n", hdc, debugrect(lprcBox), infoPtr->hBkBrush);

    return FillRect(hdc, lprcBox, infoPtr->hBkBrush);
}

/***
 * DESCRIPTION:
 * Draws a subitem.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] HDC : device context handle
 * [I] INT : item index
 * [I] INT : subitem index
 * [I] RECT * : clipping rectangle
 *
 * RETURN:
 *   Success: TRUE
 *   Failure: FALSE
 */
static BOOL LISTVIEW_DrawSubItem(LISTVIEW_INFO *infoPtr, HDC hdc, INT nItem, 
		                 INT nSubItem, RECT rcItem, UINT align)
{
    WCHAR szDispText[DISP_TEXT_SIZE];
    LVITEMW lvItem;

    TRACE("(hdc=%x, nItem=%d, nSubItem=%d, rcItem=%s)\n", 
	   hdc, nItem, nSubItem, debugrect(&rcItem));

    /* get information needed for drawing the item */
    lvItem.mask = LVIF_TEXT | LVIF_IMAGE;
    lvItem.iItem = nItem;
    lvItem.iSubItem = nSubItem;
    lvItem.cchTextMax = COUNTOF(szDispText);
    lvItem.pszText = szDispText;
    *lvItem.pszText = '\0';
    if (!LISTVIEW_GetItemW(infoPtr, &lvItem, TRUE)) return FALSE;
    
    TRACE("   lvItem=%s\n", debuglvitem_t(&lvItem, TRUE));

    if (lvItem.iImage) FIXME("Draw the image for the subitem\n");
    
    DrawTextW(hdc, lvItem.pszText, -1, &rcItem, 
	      DT_SINGLELINE | DT_VCENTER | DT_WORD_ELLIPSIS | align);

    return TRUE;
}


/***
 * DESCRIPTION:
 * Draws an item.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] hdc : device context handle
 * [I] nItem : item index
 * [I] rcItem : item rectangle
 *
 * RETURN:
 *   TRUE: if item is focused
 *   FALSE: otherwise
 */
static BOOL LISTVIEW_DrawItem(LISTVIEW_INFO *infoPtr, HDC hdc, INT nItem, RECT rcItem)
{
    WCHAR szDispText[DISP_TEXT_SIZE];
    INT nLabelWidth, imagePadding = 0;
    RECT* lprcFocus, rcOrig = rcItem;
    LVITEMW lvItem;
    TEXTATTR ta;

    TRACE("(hdc=%x, nItem=%d)\n", hdc, nItem);

    /* get information needed for drawing the item */
    lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_INDENT;
    lvItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED | LVIS_STATEIMAGEMASK;
    lvItem.iItem = nItem;
    lvItem.iSubItem = 0;
    lvItem.cchTextMax = DISP_TEXT_SIZE;
    lvItem.pszText = szDispText;
    *lvItem.pszText = '\0';
    if (!LISTVIEW_GetItemW(infoPtr, &lvItem, TRUE)) return FALSE;
    TRACE("   lvItem=%s\n", debuglvitem_t(&lvItem, TRUE));

    /* now check if we need to update the focus rectangle */
    lprcFocus = infoPtr->bFocus && (lvItem.state & LVIS_FOCUSED) ? &infoPtr->rcFocus : 0;
    if (lprcFocus) SetRectEmpty(lprcFocus);

    /* do indent */  
    rcItem.left += infoPtr->iconSize.cx * lvItem.iIndent;

    /* state icons */
    if (infoPtr->himlState)
    {
        UINT uStateImage = (lvItem.state & LVIS_STATEIMAGEMASK) >> 12;
        if (uStateImage)
	{
	     ImageList_Draw(infoPtr->himlState, uStateImage - 1, hdc, 
			    rcItem.left, rcItem.top, ILD_NORMAL);
	}
        rcItem.left += infoPtr->iconStateSize.cx;
	imagePadding = IMAGE_PADDING;
    }

    /* small icons */
    if (infoPtr->himlSmall)
    {
	if (lvItem.iImage >= 0)
	{
	    UINT mode = (lvItem.state & LVIS_SELECTED) && (infoPtr->bFocus) ?
		        ILD_SELECTED : ILD_NORMAL;
	    ImageList_SetBkColor(infoPtr->himlSmall, CLR_NONE);
	    ImageList_Draw(infoPtr->himlSmall, lvItem.iImage, hdc, 
			   rcItem.left, rcItem.top, mode);
	}
        rcItem.left += infoPtr->iconSize.cx;
	imagePadding = IMAGE_PADDING;
    }

    /* Don't bother painting item being edited */
    if (infoPtr->bEditing && lprcFocus) 
    	return FALSE;

    select_text_attr(infoPtr, hdc, lvItem.state & LVIS_SELECTED, &ta);

    nLabelWidth = LISTVIEW_GetStringWidthT(infoPtr, lvItem.pszText, TRUE);
    rcItem.left += imagePadding;
    rcItem.right = rcItem.left + nLabelWidth + TRAILING_PADDING;
    if (rcItem.right > rcOrig.right) rcItem.right = rcOrig.right;
    
    if (lvItem.pszText)
    {
    	TRACE("drawing text=%s, in rect=%s\n", debugstr_w(lvItem.pszText), debugrect(&rcItem));
	if(lprcFocus) *lprcFocus = rcItem;
	if (lvItem.state & LVIS_SELECTED)
	    ExtTextOutW(hdc, rcItem.left, rcItem.top, ETO_OPAQUE, &rcItem, 0, 0, 0);
        DrawTextW(hdc, lvItem.pszText, -1, &rcItem, 
	          DT_SINGLELINE | DT_VCENTER | DT_WORD_ELLIPSIS | DT_CENTER);
    }

    set_text_attr(hdc, &ta);
    return lprcFocus != NULL;
}

/***
 * DESCRIPTION:
 * Draws an item when in large icon display mode.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] hdc : device context handle
 * [I] nItem : item index
 * [I] rcItem : clipping rectangle
 *
 * RETURN:
 *   TRUE: if item is focused
 *   FALSE: otherwise
 */
static BOOL LISTVIEW_DrawLargeItem(LISTVIEW_INFO *infoPtr, HDC hdc, INT nItem, RECT rcItem)
{
  WCHAR szDispText[DISP_TEXT_SIZE] = { '\0' };
  LVITEMW lvItem;
  UINT uFormat = LISTVIEW_DTFLAGS;
  RECT rcIcon, rcFocus, rcFullText, rcLabel, *lprcFocus;
  POINT ptOrg;

  TRACE("(hdc=%x, nItem=%d, rcItem=%s)\n", hdc, nItem, debugrect(&rcItem));

  /* get information needed for drawing the item */
  lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
  lvItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED | LVIS_STATEIMAGEMASK;
  lvItem.iItem = nItem;
  lvItem.iSubItem = 0;
  lvItem.cchTextMax = DISP_TEXT_SIZE;
  lvItem.pszText = szDispText;
  *lvItem.pszText = '\0';
  if (!LISTVIEW_GetItemW(infoPtr, &lvItem, FALSE)) return FALSE;
  TRACE("   lvItem=%s\n", debuglvitem_t(&lvItem, TRUE));

  /* now check if we need to update the focus rectangle */
  lprcFocus = infoPtr->bFocus && (lvItem.state & LVIS_FOCUSED) ? &infoPtr->rcFocus : 0;
  
  LISTVIEW_GetItemMeasures(infoPtr, nItem, &ptOrg, NULL, &rcIcon, &rcLabel, &rcFullText);

  /* Set the item to the boundary box for now */
  TRACE("iconSize.cx=%ld, nItemWidth=%d\n", infoPtr->iconSize.cx, infoPtr->nItemWidth);
  TRACE("rcList=%s, rcView=%s\n", debugrect(&infoPtr->rcList), debugrect(&infoPtr->rcView));

  /* Figure out text colours etc. depending on state
   * At least the following states exist; there may be more.
   * Many items may be selected
   * At most one item may have the focus
   * The application may not actually be active currently
   * 1. The item is not selected in any way
   * 2. The cursor is flying over the icon or text and the text is being
   *    expanded because it is not fully displayed currently.
   * 3. The item is selected and is focussed, i.e. the user has not clicked
   *    in the blank area of the window, and the window (or application?)
   *    still has the focus.
   * 4. As 3 except that a different window has the focus
   * 5. The item is the selected item of all the items, but the user has
   *    clicked somewhere else on the window.
   * Only a few of these are handled currently. In particular 2 is not yet
   * handled since we do not support the functionality currently (or at least
   * we didn't when I wrote this)
   */

  if (lvItem.state & LVIS_SELECTED)
  {
    /* set item colors */
    SetBkColor(hdc, comctl32_color.clrHighlight);
    SetTextColor(hdc, comctl32_color.clrHighlightText);
    SetBkMode (hdc, OPAQUE);
    /* set raster mode */
    SetROP2(hdc, R2_XORPEN);
    /* When exactly is it in XOR? while being dragged? */
  }
  else
  {
    /* set item colors */
    if ( (infoPtr->clrTextBk == CLR_DEFAULT) || (infoPtr->clrTextBk == CLR_NONE) )
    {
       SetBkMode(hdc, TRANSPARENT);
    }
    else
    {
      SetBkMode(hdc, OPAQUE);
      SetBkColor(hdc, infoPtr->clrTextBk);
    }

    SetTextColor(hdc, infoPtr->clrText);
    /* set raster mode */
    SetROP2(hdc, R2_COPYPEN);
  }

  /* In cases 2,3 and 5 (see above) the full text is displayed, with word
   * wrapping and long words split.
   * In cases 1 and 4 only a portion of the text is displayed with word
   * wrapping and both word and end ellipsis.  (I don't yet know about path
   * ellipsis)
   */
  uFormat |= lprcFocus ?  DT_NOCLIP : DT_WORD_ELLIPSIS | DT_END_ELLIPSIS;

  /* state icons */
  if (infoPtr->himlState != NULL)
  {
     UINT uStateImage = (lvItem.state & LVIS_STATEIMAGEMASK) >> 12;
     INT x, y;

     x = rcIcon.left - infoPtr->iconStateSize.cx + 10;
     y = rcIcon.top + infoPtr->iconSize.cy - infoPtr->iconStateSize.cy + 4;
     if (uStateImage > 0)
     {
       ImageList_Draw(infoPtr->himlState, uStateImage - 1, hdc, x,
                      y, ILD_NORMAL);
     }
  }

  /* draw the icon */
  if (infoPtr->himlNormal != NULL)
  {
    if (lvItem.iImage >= 0)
    {
      ImageList_Draw (infoPtr->himlNormal, lvItem.iImage, hdc,
		      rcIcon.left+ICON_LR_HALF,
		      rcIcon.top+ICON_TOP_PADDING_HITABLE,
                      (lvItem.state & LVIS_SELECTED) ? ILD_SELECTED : ILD_NORMAL);
      TRACE("icon %d at (%d,%d)\n",
	    lvItem.iImage, rcIcon.left+ICON_LR_HALF,
	    rcIcon.top+ICON_TOP_PADDING_HITABLE);
    }
  }

  /* Draw the text below the icon */

  /* Don't bother painting item being edited */
  if ((infoPtr->bEditing && lprcFocus) || !lvItem.pszText || !lstrlenW(lvItem.pszText))
  {
    if(lprcFocus) SetRectEmpty(lprcFocus);
    return FALSE;
  }

  /* draw label */

  /* I am sure of most of the uFormat values.  However I am not sure about
   * whether we need or do not need the following:
   * DT_EXTERNALLEADING, DT_INTERNAL, DT_CALCRECT, DT_NOFULLWIDTHCHARBREAK,
   * DT_PATH_ELLIPSIS, DT_RTLREADING,
   * We certainly do not need
   * DT_BOTTOM, DT_VCENTER, DT_MODIFYSTRING, DT_LEFT, DT_RIGHT, DT_PREFIXONLY,
   * DT_SINGLELINE, DT_TABSTOP, DT_EXPANDTABS
   */

  /* If the text is being drawn without clipping (i.e. the full text) then we
   * need to jump through a few hoops to ensure that it all gets displayed and
   * that the background is complete
   */
  rcFocus = rcLabel;  /* save for focus */
  if ((uFormat & DT_NOCLIP) || (lvItem.state & LVIS_SELECTED))
  {
      /* FIXME: why do we need this??? */
      HBRUSH hBrush = CreateSolidBrush(GetBkColor (hdc));

      FillRect(hdc, &rcFullText, hBrush);
      rcFocus = rcFullText;
      DeleteObject(hBrush);

      /* Save size of item drawing for next InvalidateRect */
      LISTVIEW_SetItemDrawRect(infoPtr, nItem, &rcFullText);
      TRACE("focused/selected, rcFocus=%s\n", debugrect(&rcFocus));
  }
  /* else ? What if we are losing the focus? will we not get a complete
   * background?
   */

  DrawTextW (hdc, lvItem.pszText, -1, &rcLabel, uFormat);
  TRACE("text at rcLabel=%s is %s\n", debugrect(&rcLabel), debugstr_w(lvItem.pszText));

  if(lprcFocus) CopyRect(lprcFocus, &rcFocus);

  return lprcFocus != NULL;
}

/***
 * DESCRIPTION:
 * Draws listview items when in report display mode.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] HDC : device context handle
 *
 * RETURN:
 * None
 */
static void LISTVIEW_RefreshReport(LISTVIEW_INFO *infoPtr, HDC hdc, DWORD cdmode)
{
    INT rgntype, nDrawPosY, j;
    INT nTop, nItem, nLast, nUpdateHeight, nUpdateWidth;
    INT nColumnCount, nFirstCol, nLastCol;
    RECT rcItem, rcClip, rcFullSelect;
    BOOL bFullSelected, isFocused;
    DWORD cditemmode = CDRF_DODEFAULT;
    LONG lStyle = infoPtr->dwStyle;
    UINT uID = GetWindowLongW(infoPtr->hwndSelf, GWL_ID);
    TEXTATTR tmpTa, oldTa;
    COLUMNCACHE *lpCols;
    LVCOLUMNW lvColumn;
    LVITEMW lvItem;
    POINT ptOrig;

    TRACE("()\n");

    /* nothing to draw */
    if(GETITEMCOUNT(infoPtr) == 0) return;

    /* figure out what to draw */
    rgntype = GetClipBox(hdc, &rcClip);
    if (rgntype == NULLREGION) return;
    nUpdateHeight = rcClip.bottom - rcClip.top + 1;
    nUpdateWidth = rcClip.right - rcClip.left;
    nTop = LISTVIEW_GetTopIndex(infoPtr);
    nItem = nTop + (rcClip.top - infoPtr->rcList.top) / infoPtr->nItemHeight;
    if (nItem < nTop)
        nItem = nTop;
    nLast = nItem + nUpdateHeight / infoPtr->nItemHeight;
    if (nUpdateHeight % infoPtr->nItemHeight) nLast++;
    if (nLast > GETITEMCOUNT(infoPtr))
        nLast = GETITEMCOUNT(infoPtr);

    /* send cache hint notification */
    if (lStyle & LVS_OWNERDATA) 
	notify_odcachehint(infoPtr, nItem, nLast);

    /* cache column info */
    nColumnCount = Header_GetItemCount(infoPtr->hwndHeader);
    lpCols = COMCTL32_Alloc(nColumnCount * sizeof(COLUMNCACHE));
    if (!lpCols) return;
    for (j = 0; j < nColumnCount; j++) 
    {
    	Header_GetItemRect(infoPtr->hwndHeader, j, &lpCols[j].rc);
	TRACE("lpCols[%d].rc=%s\n", j, debugrect(&lpCols[j].rc));
    }
    
    /* Get scroll info once before loop */
    LISTVIEW_GetOrigin(infoPtr, &ptOrig);
    
    /* we now narrow the columns as well */
    nLastCol = nColumnCount - 1;
    for(nFirstCol = 0; nFirstCol < nColumnCount; nFirstCol++)
	if (lpCols[nFirstCol].rc.right + ptOrig.x >= rcClip.left) break;
    for(nLastCol = nColumnCount - 1; nLastCol >= 0; nLastCol--)
	if (lpCols[nLastCol].rc.left + ptOrig.x < rcClip.right) break;

    /* cache the per-column information before we start drawing */
    for (j = nFirstCol; j <= nLastCol; j++)
    {
	lvColumn.mask = LVCF_FMT;
	LISTVIEW_GetColumnT(infoPtr, j, &lvColumn, TRUE);
	TRACE("lvColumn=%s\n", debuglvcolumn_t(&lvColumn, TRUE));
	lpCols[j].align = DT_LEFT;
	if (lvColumn.fmt & LVCFMT_RIGHT)
	    lpCols[j].align = DT_RIGHT;
	else if (lvColumn.fmt & LVCFMT_CENTER)
	    lpCols[j].align = DT_CENTER;
    }
	
    /* a last few bits before we start drawing */
    TRACE("nTop=%d, nItem=%d, nLast=%d, nFirstCol=%d, nLastCol=%d\n",
	  nTop, nItem, nLast, nFirstCol, nLastCol);
    bFullSelected = infoPtr->dwLvExStyle & LVS_EX_FULLROWSELECT;
    nDrawPosY = infoPtr->rcList.top + (nItem - nTop) * infoPtr->nItemHeight;

    /* save dc values we're gonna trash while drawing */
    oldTa.bkMode = GetBkMode(hdc);
    oldTa.bkColor = GetBkColor(hdc);
    oldTa.fgColor = GetTextColor(hdc);
   
    /* iterate through the invalidated rows */ 
    for (; nItem < nLast; nItem++, nDrawPosY += infoPtr->nItemHeight)
    {
	/* if owner wants to take a first stab at it, have it his way... */
	if (lStyle & LVS_OWNERDRAWFIXED)
	{
            DRAWITEMSTRUCT dis;
            LVITEMW item;

            TRACE("Owner Drawn\n");

            item.iItem = nItem;
	    item.iSubItem = 0;
            item.mask = LVIF_PARAM | LVIF_STATE;
	    item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	    if (!LISTVIEW_GetItemW(infoPtr, &item, TRUE)) continue;
	    
            dis.hwndItem = infoPtr->hwndSelf;
            dis.hDC = hdc;
            dis.CtlType = ODT_LISTVIEW;
            dis.CtlID = uID;
            dis.itemID = nItem;
            dis.itemAction = ODA_DRAWENTIRE;
            dis.itemData = item.lParam;
            dis.itemState = 0;
            if (item.state & LVIS_SELECTED) dis.itemState |= ODS_SELECTED;
            if (item.state & LVIS_FOCUSED) dis.itemState |= ODS_FOCUS;
            dis.rcItem.left = lpCols[0].rc.left;
            dis.rcItem.right = lpCols[nColumnCount - 1].rc.right;
            dis.rcItem.top = nDrawPosY;
            dis.rcItem.bottom = dis.rcItem.top + infoPtr->nItemHeight;
            OffsetRect(&dis.rcItem, ptOrig.x, 0);

            if (SendMessageW(GetParent(infoPtr->hwndSelf), WM_DRAWITEM, uID, (LPARAM)&dis))
		continue;
        }
	
	/* compute the full select rectangle, if needed */
	if (bFullSelected)
	{
	    lvItem.mask = LVIF_IMAGE | LVIF_STATE | LVIF_INDENT;
	    lvItem.stateMask = LVIS_SELECTED;
	    lvItem.iItem = nItem;
	    lvItem.iSubItem = 0;
  	    if (!LISTVIEW_GetItemW(infoPtr, &lvItem, TRUE)) continue;
	     
	    rcFullSelect.left = lpCols[0].rc.left + REPORT_MARGINX +
	    			infoPtr->iconSize.cx * lvItem.iIndent +
				(infoPtr->himlSmall ? infoPtr->iconSize.cx : 0);
	    rcFullSelect.right = max(rcFullSelect.left, lpCols[nColumnCount - 1].rc.right - REPORT_MARGINX);
	    rcFullSelect.top = nDrawPosY;
	    rcFullSelect.bottom = rcFullSelect.top + infoPtr->nItemHeight;
	    OffsetRect(&rcFullSelect, ptOrig.x, 0);
	}

	/* draw the background of the selection rectangle, if need be */
	select_text_attr(infoPtr, hdc, bFullSelected && (lvItem.state & LVIS_SELECTED), &tmpTa);
	if (bFullSelected && (lvItem.state & LVIS_SELECTED))
	    ExtTextOutW(hdc, rcFullSelect.left, rcFullSelect.top, ETO_OPAQUE, &rcFullSelect, 0, 0, 0);
	    
	/* iterate through the invalidated columns */
    	isFocused = FALSE;	    
	for (j = nFirstCol; j <= nLastCol; j++)
	{
	    if (cdmode & CDRF_NOTIFYITEMDRAW)
		cditemmode = notify_customdrawitem (infoPtr, hdc, nItem, j, CDDS_ITEMPREPAINT);
	    if (cditemmode & CDRF_SKIPDEFAULT) continue;

	    rcItem = lpCols[j].rc;
	    rcItem.left += REPORT_MARGINX;
	    rcItem.right = max(rcItem.left, rcItem.right - REPORT_MARGINX);
	    rcItem.top = nDrawPosY;
	    rcItem.bottom = rcItem.top + infoPtr->nItemHeight;

	    /* Offset the Scroll Bar Pos */
	    OffsetRect(&rcItem, ptOrig.x, 0);

	    if (j == 0)
		isFocused = LISTVIEW_DrawItem(infoPtr, hdc, nItem, rcItem);
	    else
		LISTVIEW_DrawSubItem(infoPtr, hdc, nItem, j, rcItem, lpCols[j].align);

	    if (cditemmode & CDRF_NOTIFYPOSTPAINT)
		notify_customdrawitem(infoPtr, hdc, nItem, 0, CDDS_ITEMPOSTPAINT);
	}

	/* Adjust focus if we have it, and we are in full select */
	if (bFullSelected && isFocused) infoPtr->rcFocus = rcFullSelect;
    }

    /* cleanup the mess */
    set_text_attr(hdc, &oldTa);
    COMCTL32_Free(lpCols);
}

/***
 * DESCRIPTION:
 * Draws listview items when in list display mode.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] HDC : device context handle
 *
 * RETURN:
 * None
 */
static void LISTVIEW_RefreshList(LISTVIEW_INFO *infoPtr, HDC hdc, DWORD cdmode)
{
  RECT rcItem;
  INT i, j;
  INT nItem;
  INT nColumnCount;
  INT nCountPerColumn;
  INT nItemWidth = infoPtr->nItemWidth;
  INT nItemHeight = infoPtr->nItemHeight;
  INT nListWidth = infoPtr->rcList.right - infoPtr->rcList.left;
  DWORD cditemmode = CDRF_DODEFAULT;

  /* get number of fully visible columns */
  nColumnCount = nListWidth / nItemWidth;
  if (nListWidth % nItemWidth) nColumnCount++;
  nCountPerColumn = LISTVIEW_GetCountPerColumn(infoPtr);
  nItem = ListView_GetTopIndex(infoPtr->hwndSelf);
  TRACE("nColumnCount=%d, nCountPerColumn=%d, start item=%d\n",
	nColumnCount, nCountPerColumn, nItem);

  /* nothing to draw, return here */
  if(GETITEMCOUNT(infoPtr) == 0)
    return;

  for (i = 0; i < nColumnCount; i++)
  {
    for (j = 0; j < nCountPerColumn; j++, nItem++)
    {
      if (nItem >= GETITEMCOUNT(infoPtr))
        return;

      if (cdmode & CDRF_NOTIFYITEMDRAW)
        cditemmode = notify_customdrawitem (infoPtr, hdc, nItem, 0, CDDS_ITEMPREPAINT);
      if (cditemmode & CDRF_SKIPDEFAULT)
        continue;

      rcItem.top = j * nItemHeight;
      rcItem.left = i * nItemWidth;
      rcItem.bottom = rcItem.top + nItemHeight;
      rcItem.right = rcItem.left + nItemWidth;

      LISTVIEW_DrawItem(infoPtr, hdc, nItem, rcItem);

      if (cditemmode & CDRF_NOTIFYPOSTPAINT)
        notify_customdrawitem(infoPtr, hdc, nItem, 0, CDDS_ITEMPOSTPAINT);

    }
  }
}

/***
 * DESCRIPTION:
 * Draws listview items when in icon or small icon display mode.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] HDC : device context handle
 *
 * RETURN:
 * None
 */
static void LISTVIEW_RefreshIcon(LISTVIEW_INFO *infoPtr, HDC hdc, BOOL bSmall, DWORD cdmode)
{
  POINT ptPosition;
  POINT ptOrigin;
  RECT rcItem, rcClip, rcTemp;
  INT i;
  DWORD cditemmode = CDRF_DODEFAULT;

  TRACE("\n");

  /* nothing to draw, return here */
  if(GETITEMCOUNT(infoPtr) == 0)
    return;

  LISTVIEW_GetOrigin(infoPtr, &ptOrigin);

  GetClipBox(hdc, &rcClip);

  /* Draw the visible non-selected items */
  for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
  {
    if (LISTVIEW_GetItemState(infoPtr,i,LVIS_SELECTED))
	continue;

    rcItem.left = LVIR_BOUNDS;
    LISTVIEW_GetItemRect(infoPtr, i, &rcItem);
    if (!IntersectRect(&rcTemp, &rcItem, &rcClip))
	continue;

    if (cdmode & CDRF_NOTIFYITEMDRAW)
      cditemmode = notify_customdrawitem (infoPtr, hdc, i, 0, CDDS_ITEMPREPAINT);
    if (cditemmode & CDRF_SKIPDEFAULT)
        continue;

    LISTVIEW_GetItemPosition(infoPtr, i, &ptPosition);
    ptPosition.x += ptOrigin.x;
    ptPosition.y += ptOrigin.y;

    if (ptPosition.y + infoPtr->nItemHeight > infoPtr->rcList.top)
    {
      if (ptPosition.x + infoPtr->nItemWidth > infoPtr->rcList.left)
      {
        if (ptPosition.y < infoPtr->rcList.bottom)
        {
          if (ptPosition.x < infoPtr->rcList.right)
          {
            rcItem.top = ptPosition.y;
            rcItem.left = ptPosition.x;
            rcItem.bottom = rcItem.top + infoPtr->nItemHeight;
            rcItem.right = rcItem.left + infoPtr->nItemWidth;
	    
            if (bSmall)
              LISTVIEW_DrawItem(infoPtr, hdc, i, rcItem);
            else
              LISTVIEW_DrawLargeItem(infoPtr, hdc, i, rcItem);
          }
        }
      }
    }
    if (cditemmode & CDRF_NOTIFYPOSTPAINT)
        notify_customdrawitem(infoPtr, hdc, i, 0, CDDS_ITEMPOSTPAINT);
  }

  /* Draw the visible selected items */
  for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
  {
    if (!LISTVIEW_GetItemState(infoPtr,i,LVIS_SELECTED))
	continue;

    rcItem.left = LVIR_BOUNDS;
    LISTVIEW_GetItemRect(infoPtr, i, &rcItem);
    if (!IntersectRect(&rcTemp, &rcItem, &rcClip))
	continue;

    if (cdmode & CDRF_NOTIFYITEMDRAW)
      cditemmode = notify_customdrawitem (infoPtr, hdc, i, 0, CDDS_ITEMPREPAINT);
    if (cditemmode & CDRF_SKIPDEFAULT)
        continue;

    LISTVIEW_GetItemPosition(infoPtr, i, &ptPosition);
    ptPosition.x += ptOrigin.x;
    ptPosition.y += ptOrigin.y;

    if (ptPosition.y + infoPtr->nItemHeight > infoPtr->rcList.top)
    {
      if (ptPosition.x + infoPtr->nItemWidth > infoPtr->rcList.left)
      {
        if (ptPosition.y < infoPtr->rcList.bottom)
        {
          if (ptPosition.x < infoPtr->rcList.right)
          {
            rcItem.top = ptPosition.y;
            rcItem.left = ptPosition.x;
            rcItem.bottom = rcItem.top + infoPtr->nItemHeight;
            rcItem.right = rcItem.left + infoPtr->nItemWidth;
	    
            if (bSmall)
              LISTVIEW_DrawItem(infoPtr, hdc, i, rcItem);
            else
              LISTVIEW_DrawLargeItem(infoPtr, hdc, i, rcItem);
          }
        }
      }
    }
    if (cditemmode & CDRF_NOTIFYPOSTPAINT)
        notify_customdrawitem(infoPtr, hdc, i, 0, CDDS_ITEMPOSTPAINT);
  }
}

/***
 * DESCRIPTION:
 * Draws listview items.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] HDC : device context handle
 *
 * RETURN:
 * NoneX
 */
static void LISTVIEW_Refresh(LISTVIEW_INFO *infoPtr, HDC hdc)
{
    UINT uView = LISTVIEW_GetType(infoPtr);
    HFONT hOldFont;
    DWORD cdmode;
    RECT rcClient;

    LISTVIEW_DUMP(infoPtr);
  
    GetClientRect(infoPtr->hwndSelf, &rcClient);
  
    cdmode = notify_customdraw(infoPtr, CDDS_PREPAINT, hdc, rcClient);
    if (cdmode == CDRF_SKIPDEFAULT) return;

    infoPtr->bIsDrawing = TRUE;

    /* select font */
    hOldFont = SelectObject(hdc, infoPtr->hFont);

    if (uView == LVS_LIST)
	LISTVIEW_RefreshList(infoPtr, hdc, cdmode);
    else if (uView == LVS_REPORT)
	LISTVIEW_RefreshReport(infoPtr, hdc, cdmode);
    else
	LISTVIEW_RefreshIcon(infoPtr, hdc, uView == LVS_SMALLICON, cdmode);

    /* if we have a focus rect, draw it */
    if (infoPtr->bFocus && !IsRectEmpty(&infoPtr->rcFocus))
	DrawFocusRect(hdc, &infoPtr->rcFocus);

    /* unselect objects */
    SelectObject(hdc, hOldFont);

    if (cdmode & CDRF_NOTIFYPOSTPAINT)
	notify_customdraw(infoPtr, CDDS_POSTPAINT, hdc, rcClient);

    infoPtr->bIsDrawing = FALSE;
}


/***
 * DESCRIPTION:
 * Calculates the approximate width and height of a given number of items.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT : number of items
 * [I] INT : width
 * [I] INT : height
 *
 * RETURN:
 * Returns a DWORD. The width in the low word and the height in high word.
 */
static LRESULT LISTVIEW_ApproximateViewRect(LISTVIEW_INFO *infoPtr, INT nItemCount,
                                            WORD wWidth, WORD wHeight)
{
  UINT uView = LISTVIEW_GetType(infoPtr);
  INT nItemCountPerColumn = 1;
  INT nColumnCount = 0;
  DWORD dwViewRect = 0;

  if (nItemCount == -1)
    nItemCount = GETITEMCOUNT(infoPtr);

  if (uView == LVS_LIST)
  {
    if (wHeight == 0xFFFF)
    {
      /* use current height */
      wHeight = infoPtr->rcList.bottom - infoPtr->rcList.top;
    }

    if (wHeight < infoPtr->nItemHeight)
      wHeight = infoPtr->nItemHeight;

    if (nItemCount > 0)
    {
      if (infoPtr->nItemHeight > 0)
      {
        nItemCountPerColumn = wHeight / infoPtr->nItemHeight;
        if (nItemCountPerColumn == 0)
          nItemCountPerColumn = 1;

        if (nItemCount % nItemCountPerColumn != 0)
          nColumnCount = nItemCount / nItemCountPerColumn;
        else
          nColumnCount = nItemCount / nItemCountPerColumn + 1;
      }
    }

    /* Microsoft padding magic */
    wHeight = nItemCountPerColumn * infoPtr->nItemHeight + 2;
    wWidth = nColumnCount * infoPtr->nItemWidth + 2;

    dwViewRect = MAKELONG(wWidth, wHeight);
  }
  else if (uView == LVS_REPORT)
    FIXME("uView == LVS_REPORT: not implemented\n");
  else if (uView == LVS_SMALLICON)
    FIXME("uView == LVS_SMALLICON: not implemented\n");
  else if (uView == LVS_ICON)
    FIXME("uView == LVS_ICON: not implemented\n");

  return dwViewRect;
}

/***
 * DESCRIPTION:
 * Arranges listview items in icon display mode.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT : alignment code
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_Arrange(LISTVIEW_INFO *infoPtr, INT nAlignCode)
{
  UINT uView = LISTVIEW_GetType(infoPtr);
  BOOL bResult = FALSE;

  if ((uView == LVS_ICON) || (uView == LVS_SMALLICON))
  {
    switch (nAlignCode)
    {
    case LVA_ALIGNLEFT:
      FIXME("nAlignCode=LVA_ALIGNLEFT: not implemented\n");
      break;
    case LVA_ALIGNTOP:
      FIXME("nAlignCode=LVA_ALIGNTOP: not implemented\n");
      break;
    case LVA_DEFAULT:
      FIXME("nAlignCode=LVA_DEFAULT: not implemented\n");
      break;
    case LVA_SNAPTOGRID:
      FIXME("nAlignCode=LVA_SNAPTOGRID: not implemented\n");
      break;
    }
  }

  return bResult;
}

/* << LISTVIEW_CreateDragImage >> */


/***
 * DESCRIPTION:
 * Removes all listview items and subitems.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_DeleteAllItems(LISTVIEW_INFO *infoPtr)
{
  LONG lStyle = infoPtr->dwStyle;
  UINT uView = lStyle & LVS_TYPEMASK;
  LISTVIEW_ITEM *lpItem;
  LISTVIEW_SUBITEM *lpSubItem;
  NMLISTVIEW nmlv;
  BOOL bSuppress;
  BOOL bResult = FALSE;
  HDPA hdpaSubItems;

  TRACE("()\n");

  LISTVIEW_RemoveAllSelections(infoPtr);
  infoPtr->nSelectionMark=-1;
  infoPtr->nFocusedItem=-1;
  /* But we are supposed to leave nHotItem as is! */

  if (lStyle & LVS_OWNERDATA)
  {
    infoPtr->hdpaItems->nItemCount = 0;
    LISTVIEW_InvalidateList(infoPtr);
    return TRUE;
  }

  if (GETITEMCOUNT(infoPtr) > 0)
  {
    INT i, j;

    /* send LVN_DELETEALLITEMS notification */
    /* verify if subsequent LVN_DELETEITEM notifications should be
       suppressed */
    ZeroMemory(&nmlv, sizeof(NMLISTVIEW));
    nmlv.iItem = -1;
    bSuppress = notify_listview(infoPtr, LVN_DELETEALLITEMS, &nmlv);

    for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
    {
      hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, i);
      if (hdpaSubItems != NULL)
      {
        for (j = 1; j < hdpaSubItems->nItemCount; j++)
        {
          lpSubItem = (LISTVIEW_SUBITEM *)DPA_GetPtr(hdpaSubItems, j);
          if (lpSubItem != NULL)
          {
            /* free subitem string */
            if (is_textW(lpSubItem->hdr.pszText))
              COMCTL32_Free(lpSubItem->hdr.pszText);

            /* free subitem */
            COMCTL32_Free(lpSubItem);
          }
        }

        lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0);
        if (lpItem != NULL)
        {
          if (!bSuppress)
          {
            /* send LVN_DELETEITEM notification */
            nmlv.iItem = i;
            nmlv.lParam = lpItem->lParam;
            notify_listview(infoPtr, LVN_DELETEITEM, &nmlv);
          }

          /* free item string */
          if (is_textW(lpItem->hdr.pszText))
            COMCTL32_Free(lpItem->hdr.pszText);

          /* free item */
          COMCTL32_Free(lpItem);
        }

        DPA_Destroy(hdpaSubItems);
      }
    }

    /* reinitialize listview memory */
    bResult = DPA_DeleteAllPtrs(infoPtr->hdpaItems);

    /* align items (set position of each item) */
    if ((uView == LVS_ICON) || (uView == LVS_SMALLICON))
    {
      if (lStyle & LVS_ALIGNLEFT)
      {
        LISTVIEW_AlignLeft(infoPtr);
      }
      else
      {
        LISTVIEW_AlignTop(infoPtr);
      }
    }

    LISTVIEW_UpdateScroll(infoPtr);

    /* invalidate client area (optimization needed) */
    LISTVIEW_InvalidateList(infoPtr);
  }

  return bResult;
}

/***
 * DESCRIPTION:
 * Removes a column from the listview control.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT : column index
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_DeleteColumn(LISTVIEW_INFO *infoPtr, INT nColumn)
{
    UINT uView = infoPtr->dwStyle & LVS_TYPEMASK;
    RECT rcCol, rcOld;
    
    TRACE("nColumn=%d\n", nColumn);

    if (nColumn <= 0) return FALSE;

    if (uView == LVS_REPORT)
    {
        if (!Header_GetItemRect(infoPtr->hwndHeader, nColumn, &rcCol))
	    return FALSE;
    
    	if (!Header_DeleteItem(infoPtr->hwndHeader, nColumn))
	    return FALSE;
    }
  
    if (!(infoPtr->dwStyle & LVS_OWNERDATA))
    {
	LISTVIEW_SUBITEM *lpSubItem, *lpDelItem;
	HDPA hdpaSubItems;
	INT nItem, nSubItem, i;
	
	for (nItem = 0; nItem < infoPtr->hdpaItems->nItemCount; nItem++)
	{
	    hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, nItem);
	    if (!hdpaSubItems) continue;
	    nSubItem = 0;
	    lpDelItem = 0;
	    for (i = 1; i < hdpaSubItems->nItemCount; i++)
	    {
		lpSubItem = (LISTVIEW_SUBITEM *)DPA_GetPtr(hdpaSubItems, i);
		if (!lpSubItem) break;
		if (lpSubItem->iSubItem == nColumn)
		{
		    nSubItem = i;
		    lpDelItem = lpSubItem;
		}
		else if (lpSubItem->iSubItem > nColumn) 
		{
		    lpSubItem->iSubItem--;
		}
	    }

	    /* if we found our subitem, zapp it */	
	    if (nSubItem > 0)
	    {
		/* free string */
		if (is_textW(lpDelItem->hdr.pszText))
		    COMCTL32_Free(lpDelItem->hdr.pszText);

		/* free item */
		COMCTL32_Free(lpDelItem);

		/* free dpa memory */
		DPA_DeletePtr(hdpaSubItems, nSubItem);
    	    }
	}
    }

    /* we need to worry about display issues in report mode only */
    if (uView != LVS_REPORT) return TRUE;

    /* Need to reset the item width when deleting a column */
    infoPtr->nItemWidth = LISTVIEW_GetItemWidth(infoPtr);

    /* update scrollbar(s) */
    LISTVIEW_UpdateScroll(infoPtr);

    /* scroll to cover the deleted column, and invalidate for redraw */
    rcOld = infoPtr->rcList;
    rcOld.left = rcCol.left;
    ScrollWindowEx(infoPtr->hwndSelf, -(rcCol.right - rcCol.left), 0,
		   &rcOld, &rcOld, 0, 0, SW_ERASE | SW_INVALIDATE);

    return TRUE;
}

/***
 * DESCRIPTION:
 * Removes an item from the listview control.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT : item index
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_DeleteItem(LISTVIEW_INFO *infoPtr, INT nItem)
{
  LONG lStyle = infoPtr->dwStyle;
  UINT uView = lStyle & LVS_TYPEMASK;
  LONG lCtrlId = GetWindowLongW(infoPtr->hwndSelf, GWL_ID);
  NMLISTVIEW nmlv;
  BOOL bResult = FALSE;
  HDPA hdpaSubItems;
  LISTVIEW_ITEM *lpItem;
  LISTVIEW_SUBITEM *lpSubItem;
  INT i;
  LVITEMW item;

  TRACE("(nItem=%d)\n", nItem);


  /* First, send LVN_DELETEITEM notification. */
  memset(&nmlv, 0, sizeof (NMLISTVIEW));
  nmlv.hdr.hwndFrom = infoPtr->hwndSelf;
  nmlv.hdr.idFrom = lCtrlId;
  nmlv.hdr.code = LVN_DELETEITEM;
  nmlv.iItem = nItem;
  SendMessageW((infoPtr->hwndSelf), WM_NOTIFY, (WPARAM)lCtrlId,
               (LPARAM)&nmlv);


  /* remove it from the selection range */
  item.state = LVIS_SELECTED;
  item.stateMask = LVIS_SELECTED;
  LISTVIEW_SetItemState(infoPtr,nItem,&item);

  if (lStyle & LVS_OWNERDATA)
  {
    infoPtr->hdpaItems->nItemCount --;
    LISTVIEW_InvalidateList(infoPtr);
    return TRUE;
  }

  if ((nItem >= 0) && (nItem < GETITEMCOUNT(infoPtr)))
  {
    /* initialize memory */
    ZeroMemory(&nmlv, sizeof(NMLISTVIEW));

    hdpaSubItems = (HDPA)DPA_DeletePtr(infoPtr->hdpaItems, nItem);
    if (hdpaSubItems != NULL)
    {
      for (i = 1; i < hdpaSubItems->nItemCount; i++)
      {
        lpSubItem = (LISTVIEW_SUBITEM *)DPA_GetPtr(hdpaSubItems, i);
        if (lpSubItem != NULL)
        {
          /* free item string */
          if (is_textW(lpSubItem->hdr.pszText))
            COMCTL32_Free(lpSubItem->hdr.pszText);

          /* free item */
          COMCTL32_Free(lpSubItem);
        }
      }

      lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0);
      if (lpItem != NULL)
      {
        /* free item string */
        if (is_textW(lpItem->hdr.pszText))
          COMCTL32_Free(lpItem->hdr.pszText);

        /* free item */
        COMCTL32_Free(lpItem);
      }

      bResult = DPA_Destroy(hdpaSubItems);
    }

    LISTVIEW_ShiftIndices(infoPtr,nItem,-1);

    /* align items (set position of each item) */
    if ((uView == LVS_SMALLICON) || (uView == LVS_ICON))
    {
      if (lStyle & LVS_ALIGNLEFT)
        LISTVIEW_AlignLeft(infoPtr);
      else
        LISTVIEW_AlignTop(infoPtr);
    }

    LISTVIEW_UpdateScroll(infoPtr);

    /* FIXME: optimizartion refresh client area */
    LISTVIEW_InvalidateList(infoPtr);
  }

  return bResult;
}


/***
 * DESCRIPTION:
 * Callback implementation for editlabel control
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] pszText : modified text
 * [I] isW : TRUE if psxText is Unicode, FALSE if it's ANSI
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_EndEditLabelT(LISTVIEW_INFO *infoPtr, LPWSTR pszText, BOOL isW)
{
  LONG lStyle = infoPtr->dwStyle;
  NMLVDISPINFOW dispInfo;
  LISTVIEW_ITEM *lpItem;
  HDPA hdpaSubItems;
  LISTVIEW_ITEM lvItemRef;
  LVITEMW item;
  BOOL bResult = TRUE;
  INT nItem = infoPtr->nEditLabelItem;

  TRACE("(pszText=%s, nItem=%d, isW=%d)\n", debugtext_t(pszText, isW), nItem, isW);

  infoPtr->bEditing = FALSE;
  if (!(lStyle & LVS_OWNERDATA))
  {
    if (!(hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, nItem)))
	  return FALSE;

    if (!(lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0)))
  	  return FALSE;
  }
  else
  {
    ZeroMemory(&lvItemRef,sizeof(LISTVIEW_ITEM));
    item.iItem = nItem;
    item.iSubItem = 0;
    item.mask = LVIF_PARAM | LVIF_STATE;
    item.stateMask = ~0;
    if (!LISTVIEW_GetItemW(infoPtr, &item, TRUE)) return FALSE;
    lvItemRef.hdr.iImage = item.iImage;
    lvItemRef.state = item.state;
    lvItemRef.lParam = item.lParam;
    lpItem = &lvItemRef;
  }

  ZeroMemory(&dispInfo, sizeof(dispInfo));
  dispInfo.item.mask = 0;
  dispInfo.item.iItem = nItem;
  dispInfo.item.state = lpItem->state;
  dispInfo.item.stateMask = 0; /* FIXME: why not copy the state mask in here? */
  dispInfo.item.pszText = pszText;
  dispInfo.item.cchTextMax = textlenT(pszText, isW);
  dispInfo.item.iImage = lpItem->hdr.iImage;
  dispInfo.item.lParam = lpItem->lParam;

  /* Do we need to update the Item Text */
  if(notify_dispinfoT(infoPtr, LVN_ENDLABELEDITW, &dispInfo, isW))
    if (lpItem->hdr.pszText != LPSTR_TEXTCALLBACKW && !(lStyle & LVS_OWNERDATA))
      bResult = textsetptrT(&lpItem->hdr.pszText, pszText, isW);

  return bResult;
}

/***
 * DESCRIPTION:
 * Begin in place editing of specified list view item
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT : item index
 * [I] isW : TRUE if it's a Unicode req, FALSE if ASCII
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static HWND LISTVIEW_EditLabelT(LISTVIEW_INFO *infoPtr, INT nItem, BOOL isW)
{
  NMLVDISPINFOW dispInfo;
  RECT rect;
  LISTVIEW_ITEM *lpItem;
  HWND hedit;
  HDPA hdpaSubItems;
  WCHAR szDispText[DISP_TEXT_SIZE];
  LVITEMW lvItem;
  LISTVIEW_ITEM lvItemRef;
  LONG lStyle = infoPtr->dwStyle;

  if (~infoPtr->dwStyle & LVS_EDITLABELS)
      return FALSE;

  infoPtr->nEditLabelItem = nItem;

  TRACE("(nItem=%d, isW=%d)\n", nItem, isW);

  /* Is the EditBox still there, if so remove it */
  if(infoPtr->hwndEdit != 0)
  {
      SetFocus(infoPtr->hwndSelf);
      infoPtr->hwndEdit = 0;
  }

  LISTVIEW_SetSelection(infoPtr, nItem);
  LISTVIEW_SetItemFocus(infoPtr, nItem);

  if (!(lStyle & LVS_OWNERDATA))
  {
    if (NULL == (hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, nItem)))
  	  return 0;

    if (NULL == (lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0)))
  	  return 0;
  }
  else
  {
    LVITEMW item;
    item.iItem = nItem;
    item.iSubItem = 0;
    item.mask = LVIF_PARAM | LVIF_STATE;
    item.stateMask = ~0;
    if (!LISTVIEW_GetItemW(infoPtr, &item, TRUE)) return FALSE;
    lvItemRef.hdr.iImage = item.iImage;
    lvItemRef.state = item.state;
    lvItemRef.lParam = item.lParam;
    lpItem = &lvItemRef;
  }

  /* get information needed for drawing the item */
  lvItem.mask = LVIF_TEXT;
  lvItem.iItem = nItem;
  lvItem.iSubItem = 0;
  lvItem.cchTextMax = DISP_TEXT_SIZE;
  lvItem.pszText = szDispText;
  *lvItem.pszText = '\0';
  if (!LISTVIEW_GetItemT(infoPtr, &lvItem, FALSE, isW)) return FALSE;

  ZeroMemory(&dispInfo, sizeof(dispInfo));
  dispInfo.item.mask = 0;
  dispInfo.item.iItem = nItem;
  dispInfo.item.state = lpItem->state;
  dispInfo.item.stateMask = 0; /* FIXME: why not copy the state mask in here? */
  dispInfo.item.pszText = lvItem.pszText;
  dispInfo.item.cchTextMax = lstrlenW(lvItem.pszText);
  dispInfo.item.iImage = lpItem->hdr.iImage;
  dispInfo.item.lParam = lpItem->lParam;

  if (notify_dispinfoT(infoPtr, LVN_BEGINLABELEDITW, &dispInfo, isW))
	  return 0;

  rect.left = LVIR_LABEL;
  if (!LISTVIEW_GetItemRect(infoPtr, nItem, &rect))
	  return 0;

  if (!(hedit = CreateEditLabelT(infoPtr, szDispText, WS_VISIBLE,
		 rect.left-2, rect.top-1, 0, rect.bottom - rect.top+2, isW)))
	 return 0;

  infoPtr->hwndEdit = hedit;

  ShowWindow(infoPtr->hwndEdit,SW_NORMAL);
  infoPtr->bEditing = TRUE;
  SetFocus(infoPtr->hwndEdit);
  SendMessageW(infoPtr->hwndEdit, EM_SETSEL, 0, -1);
  return infoPtr->hwndEdit;
}


/***
 * DESCRIPTION:
 * Ensures the specified item is visible, scrolling into view if necessary.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem : item index
 * [I] bPartial : partially or entirely visible
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_EnsureVisible(LISTVIEW_INFO *infoPtr, INT nItem, BOOL bPartial)
{
    UINT uView = LISTVIEW_GetType(infoPtr);
    INT nScrollPosHeight = 0;
    INT nScrollPosWidth = 0;
    INT nPartialAdjust = 0;
    INT nHorzDiff = 0;
    INT nVertDiff = 0;
    RECT rcItem;

    /* FIXME: ALWAYS bPartial == FALSE, FOR NOW! */

    rcItem.left = LVIR_BOUNDS;
    if (!LISTVIEW_GetItemRect(infoPtr, nItem, &rcItem)) return FALSE;
    
    if (rcItem.left < infoPtr->rcList.left || rcItem.right > infoPtr->rcList.right)
    {
        /* scroll left/right, but in LVS_REPORT mode */
        if (uView == LVS_LIST)
            nScrollPosWidth = infoPtr->nItemWidth;
        else if ((uView == LVS_SMALLICON) || (uView == LVS_ICON))
            nScrollPosWidth = 1;

	if (rcItem.left < infoPtr->rcList.left)
	{
	    nPartialAdjust = -1;
	    if (uView != LVS_REPORT) nHorzDiff = rcItem.left + infoPtr->rcList.left;
	}
	else
	{
	    nPartialAdjust = 1;
	    if (uView != LVS_REPORT) nHorzDiff = rcItem.right - infoPtr->rcList.right;
	}
    }

    if (rcItem.top < infoPtr->rcList.top || rcItem.bottom > infoPtr->rcList.bottom)
    {
	/* scroll up/down, but not in LVS_LIST mode */
        if (uView == LVS_REPORT)
            nScrollPosHeight = infoPtr->nItemHeight;
        else if ((uView == LVS_ICON) || (uView == LVS_SMALLICON))
            nScrollPosHeight = 1;

	if (rcItem.top < infoPtr->rcList.top)
	{
	    nPartialAdjust = -1;
	    if (uView != LVS_LIST) nVertDiff = rcItem.top + infoPtr->rcList.top;
	}
	else
	{
	    nPartialAdjust = 1;
	    if (uView != LVS_LIST) nVertDiff = rcItem.bottom - infoPtr->rcList.bottom;
	}
    }

    if (!nScrollPosWidth && !nScrollPosHeight) return TRUE;

    if (nScrollPosWidth)
    {
	INT diff = nHorzDiff / nScrollPosWidth;
	if (rcItem.left % nScrollPosWidth) diff += nPartialAdjust;
	LISTVIEW_HScroll(infoPtr, SB_INTERNAL, diff, 0);
    }

    if (nScrollPosHeight)
    {
	INT diff = nVertDiff / nScrollPosHeight;
	if (rcItem.top % nScrollPosHeight) diff += nPartialAdjust;
	LISTVIEW_VScroll(infoPtr, SB_INTERNAL, diff, 0);
    }

    return TRUE;
}

/***
 * DESCRIPTION:
 * Retrieves the nearest item, given a position and a direction.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] POINT : start position
 * [I] UINT : direction
 *
 * RETURN:
 * Item index if successdful, -1 otherwise.
 */
static INT LISTVIEW_GetNearestItem(LISTVIEW_INFO *infoPtr, POINT pt, UINT vkDirection)
{
    LVHITTESTINFO ht;
    RECT rcView;

    TRACE("point %ld,%ld, direction %s\n", pt.x, pt.y,
          (vkDirection == VK_DOWN) ? "VK_DOWN" :
          ((vkDirection == VK_UP) ? "VK_UP" :
          ((vkDirection == VK_LEFT) ? "VK_LEFT" : "VK_RIGHT")));

    if (!LISTVIEW_GetViewRect(infoPtr, &rcView)) return -1;
    
    if (!LISTVIEW_GetOrigin(infoPtr, &ht.pt)) return -1;
    
    ht.pt.x += pt.x;
    ht.pt.y += pt.y;

    if (vkDirection == VK_DOWN)       ht.pt.y += infoPtr->nItemHeight;
    else if (vkDirection == VK_UP)    ht.pt.y -= infoPtr->nItemHeight;
    else if (vkDirection == VK_LEFT)  ht.pt.x -= infoPtr->nItemWidth;
    else if (vkDirection == VK_RIGHT) ht.pt.x += infoPtr->nItemWidth;

    if (!PtInRect(&rcView, ht.pt)) return -1;
    
    return LISTVIEW_SuperHitTestItem(infoPtr, &ht, TRUE, TRUE);
}

/***
 * DESCRIPTION:
 * Searches for an item with specific characteristics.
 *
 * PARAMETER(S):
 * [I] hwnd : window handle
 * [I] nStart : base item index
 * [I] lpFindInfo : item information to look for
 *
 * RETURN:
 *   SUCCESS : index of item
 *   FAILURE : -1
 */
static LRESULT LISTVIEW_FindItemW(LISTVIEW_INFO *infoPtr, INT nStart,
                                  LPLVFINDINFOW lpFindInfo)
{
  POINT ptItem;
  WCHAR szDispText[DISP_TEXT_SIZE] = { '\0' };
  LVITEMW lvItem;
  BOOL bWrap = FALSE;
  INT nItem = nStart;
  INT nLast = GETITEMCOUNT(infoPtr);

  if ((nItem >= -1) && (lpFindInfo != NULL))
  {
    lvItem.mask = 0;
    if (lpFindInfo->flags & LVFI_PARAM)
    {
      lvItem.mask |= LVIF_PARAM;
    }

    if (lpFindInfo->flags & (LVFI_STRING | LVFI_PARTIAL))
    {
      lvItem.mask |= LVIF_TEXT;
      lvItem.pszText = szDispText;
      lvItem.cchTextMax = DISP_TEXT_SIZE;
    }

    if (lpFindInfo->flags & LVFI_WRAP)
      bWrap = TRUE;

    if (lpFindInfo->flags & LVFI_NEARESTXY)
    {
      ptItem.x = lpFindInfo->pt.x;
      ptItem.y = lpFindInfo->pt.y;
    }

    while (1)
    {
      while (nItem < nLast)
      {
        if (lpFindInfo->flags & LVFI_NEARESTXY)
        {
          nItem = LISTVIEW_GetNearestItem(infoPtr, ptItem,
                                          lpFindInfo->vkDirection);
          if (nItem != -1)
          {
            /* get position of the new item index */
            if (!ListView_GetItemPosition(infoPtr->hwndSelf, nItem, &ptItem))
              return -1;
          }
          else
            return -1;
        }
        else
        {
          nItem++;
        }

        lvItem.iItem = nItem;
        lvItem.iSubItem = 0;
        if (LISTVIEW_GetItemW(infoPtr, &lvItem, TRUE))
        {
          if (lvItem.mask & LVIF_TEXT)
          {
            if (lpFindInfo->flags & LVFI_PARTIAL)
            {
              if (strstrW(lvItem.pszText, lpFindInfo->psz) == NULL)
                continue;
            }
            else
            {
              if (lstrcmpW(lvItem.pszText, lpFindInfo->psz) != 0)
                continue;
            }
          }

          if (lvItem.mask & LVIF_PARAM)
          {
            if (lpFindInfo->lParam != lvItem.lParam)
              continue;
          }

          return nItem;
        }
      }

      if (bWrap)
      {
        nItem = -1;
        nLast = nStart + 1;
        bWrap = FALSE;
      }
      else
      {
        return -1;
      }
    }
  }

 return -1;
}

/***
 * DESCRIPTION:
 * Searches for an item with specific characteristics.
 *
 * PARAMETER(S):
 * [I] hwnd : window handle
 * [I] nStart : base item index
 * [I] lpFindInfo : item information to look for
 *
 * RETURN:
 *   SUCCESS : index of item
 *   FAILURE : -1
 */
static LRESULT LISTVIEW_FindItemA(LISTVIEW_INFO *infoPtr, INT nStart,
                                  LPLVFINDINFOA lpFindInfo)
{
  BOOL hasText = lpFindInfo->flags & (LVFI_STRING | LVFI_PARTIAL);
  LVFINDINFOW fiw;
  LRESULT res;

  memcpy(&fiw, lpFindInfo, sizeof(fiw));
  if (hasText) fiw.psz = textdupTtoW((LPCWSTR)lpFindInfo->psz, FALSE);
  res = LISTVIEW_FindItemW(infoPtr, nStart, &fiw);
  if (hasText) textfreeT((LPWSTR)fiw.psz, FALSE);
  return res;
}

/***
 * DESCRIPTION:
 * Retrieves the background image of the listview control.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [O] LPLVMKBIMAGE : background image attributes
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
/* static LRESULT LISTVIEW_GetBkImage(LISTVIEW_INFO *infoPtr, LPLVBKIMAGE lpBkImage)   */
/* {   */
/*   FIXME (listview, "empty stub!\n"); */
/*   return FALSE;   */
/* }   */

/***
 * DESCRIPTION:
 * Retrieves column attributes.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT :  column index
 * [IO] LPLVCOLUMNW : column information
 * [I] isW : if TRUE, then lpColumn is a LPLVCOLUMNW
 *           otherwise it is in fact a LPLVCOLUMNA
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_GetColumnT(LISTVIEW_INFO *infoPtr, INT nItem, LPLVCOLUMNW lpColumn, BOOL isW)
{
  HDITEMW hdi;
  BOOL bResult = FALSE;

  if (lpColumn != NULL)
  {

    /* initialize memory */
    ZeroMemory(&hdi, sizeof(hdi));

    if (lpColumn->mask & LVCF_FMT)
      hdi.mask |= HDI_FORMAT;

    if (lpColumn->mask & LVCF_WIDTH)
      hdi.mask |= HDI_WIDTH;

    if (lpColumn->mask & LVCF_TEXT)
    {
      hdi.mask |= HDI_TEXT;
      hdi.cchTextMax = lpColumn->cchTextMax;
      hdi.pszText    = lpColumn->pszText;
    }

    if (lpColumn->mask & LVCF_IMAGE)
      hdi.mask |= HDI_IMAGE;

    if (lpColumn->mask & LVCF_ORDER)
      hdi.mask |= HDI_ORDER;

    if (isW)
      bResult = Header_GetItemW(infoPtr->hwndHeader, nItem, &hdi);
    else
      bResult = Header_GetItemA(infoPtr->hwndHeader, nItem, &hdi);

    if (bResult)
    {
      if (lpColumn->mask & LVCF_FMT)
      {
        lpColumn->fmt = 0;

        if (hdi.fmt & HDF_LEFT)
          lpColumn->fmt |= LVCFMT_LEFT;
        else if (hdi.fmt & HDF_RIGHT)
          lpColumn->fmt |= LVCFMT_RIGHT;
        else if (hdi.fmt & HDF_CENTER)
          lpColumn->fmt |= LVCFMT_CENTER;

        if (hdi.fmt & HDF_IMAGE)
          lpColumn->fmt |= LVCFMT_COL_HAS_IMAGES;

	if (hdi.fmt & HDF_BITMAP_ON_RIGHT)
	  lpColumn->fmt |= LVCFMT_BITMAP_ON_RIGHT;
      }

      if (lpColumn->mask & LVCF_WIDTH)
        lpColumn->cx = hdi.cxy;

      if (lpColumn->mask & LVCF_IMAGE)
        lpColumn->iImage = hdi.iImage;

      if (lpColumn->mask & LVCF_ORDER)
        lpColumn->iOrder = hdi.iOrder;

      TRACE("(col=%d, lpColumn=%s, isW=%d)\n",
	    nItem, debuglvcolumn_t(lpColumn, isW), isW);

    }
  }

  return bResult;
}


static LRESULT LISTVIEW_GetColumnOrderArray(LISTVIEW_INFO *infoPtr, INT iCount, LPINT lpiArray)
{
    INT i;

    if (!lpiArray)
	return FALSE;

    /* FIXME: little hack */
    for (i = 0; i < iCount; i++)
	lpiArray[i] = i;

    return TRUE;
}

/***
 * DESCRIPTION:
 * Retrieves the column width.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] int : column index
 *
 * RETURN:
 *   SUCCESS : column width
 *   FAILURE : zero
 */
static LRESULT LISTVIEW_GetColumnWidth(LISTVIEW_INFO *infoPtr, INT nColumn)
{
    INT nColumnWidth = 0;
    HDITEMW hdi;

    TRACE("nColumn=%d\n", nColumn);

    switch(LISTVIEW_GetType(infoPtr))
    {
    case LVS_LIST:
	nColumnWidth = infoPtr->nItemWidth;
	break;
    case LVS_REPORT:
	hdi.mask = HDI_WIDTH;
	if (Header_GetItemW(infoPtr->hwndHeader, nColumn, &hdi))
	    nColumnWidth = hdi.cxy;
	break;
    default:
	/* we don't have a 'column' in [SMALL]ICON mode */
    }

    TRACE("nColumnWidth=%d\n", nColumnWidth);
    return nColumnWidth;
}

/***
 * DESCRIPTION:
 * In list or report display mode, retrieves the number of items that can fit
 * vertically in the visible area. In icon or small icon display mode,
 * retrieves the total number of visible items.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 * Number of fully visible items.
 */
static LRESULT LISTVIEW_GetCountPerPage(LISTVIEW_INFO *infoPtr)
{
  UINT uView = LISTVIEW_GetType(infoPtr);
  INT nItemCount = 0;

  if (uView == LVS_LIST)
  {
    if (infoPtr->rcList.right > infoPtr->nItemWidth)
    {
      nItemCount = LISTVIEW_GetCountPerRow(infoPtr) *
                   LISTVIEW_GetCountPerColumn(infoPtr);
    }
  }
  else if (uView == LVS_REPORT)
  {
    nItemCount = LISTVIEW_GetCountPerColumn(infoPtr);
  }
  else
  {
    nItemCount = GETITEMCOUNT(infoPtr);
  }

  return nItemCount;
}


/***
 * DESCRIPTION:
 * Retrieves an image list handle.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nImageList : image list identifier
 *
 * RETURN:
 *   SUCCESS : image list handle
 *   FAILURE : NULL
 */
static LRESULT LISTVIEW_GetImageList(LISTVIEW_INFO *infoPtr, INT nImageList)
{
  HIMAGELIST himl = NULL;

  switch (nImageList)
  {
  case LVSIL_NORMAL:
    himl = infoPtr->himlNormal;
    break;
  case LVSIL_SMALL:
    himl = infoPtr->himlSmall;
    break;
  case LVSIL_STATE:
    himl = infoPtr->himlState;
    break;
  }

  return (LRESULT)himl;
}

/* LISTVIEW_GetISearchString */

/***
 * Helper function for LISTVIEW_GetItemT *only*. Tests if an item is selected.
 * It is important that no other functions call this because of callbacks.
 */
static inline BOOL is_item_selected(LISTVIEW_INFO *infoPtr, INT nItem)
{
  RANGE selection = { nItem, nItem };

  return DPA_Search(infoPtr->hdpaSelectionRanges, &selection, 0,
                    LISTVIEW_CompareSelectionRanges, 0, DPAS_SORTED) != -1;
}

/***
 * DESCRIPTION:
 * Retrieves item attributes.
 *
 * PARAMETER(S):
 * [I] hwnd : window handle
 * [IO] lpLVItem : item info
 * [I] internal : if true then we will use tricks that avoid copies
 *               but are not compatible with the regular interface
 * [I] isW : if TRUE, then lpLVItem is a LPLVITEMW,
 *           if FALSE, the lpLVItem is a LPLVITEMA.
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_GetItemT(LISTVIEW_INFO *infoPtr, LPLVITEMW lpLVItem, BOOL internal, BOOL isW)
{
    NMLVDISPINFOW dispInfo;
    LISTVIEW_ITEM *lpItem;
    ITEMHDR* pItemHdr;
    HDPA hdpaSubItems;

    if (internal && !isW)
    {
        ERR("We can't have internal non-Unicode GetItem!\n");
        return FALSE;
    }

    /* In the following:
     * lpLVItem describes the information requested by the user
     * lpItem is what we have
     * dispInfo is a structure we use to request the missing
     *     information from the application
     */

    TRACE("(lpLVItem=%s, internal=%d, isW=%d)\n", debuglvitem_t(lpLVItem, isW), internal, isW);

    if (!lpLVItem || (lpLVItem->iItem < 0) ||
        (lpLVItem->iItem >= GETITEMCOUNT(infoPtr)))
	return FALSE;

    /* a quick optimization if all we're asked is the focus state
     * these queries are worth optimising since they are common,
     * and can be answered in constant time, without the heavy accesses */
    if ( (lpLVItem->mask == LVIF_STATE) && (lpLVItem->stateMask == LVIF_STATE) &&
	 !(infoPtr->uCallbackMask & LVIS_FOCUSED) )
    {
	lpLVItem->state = 0;
	if (infoPtr->nFocusedItem == lpLVItem->iItem)
	    lpLVItem->state |= LVIS_FOCUSED;
	return TRUE;
    }

    ZeroMemory(&dispInfo, sizeof(dispInfo));

    /* if the app stores all the data, handle it separately */
    if (infoPtr->dwStyle & LVS_OWNERDATA)
    {
	dispInfo.item.state = 0;

	/* if we need to callback, do it now */
	if ((lpLVItem->mask & ~LVIF_STATE) || infoPtr->uCallbackMask)
	{
	    /* NOTE: copy only fields which we _know_ are initialized, some apps
	     *       depend on the uninitialized fields being 0 */
	    dispInfo.item.mask = lpLVItem->mask;
	    dispInfo.item.iItem = lpLVItem->iItem;
	    dispInfo.item.iSubItem = lpLVItem->iSubItem;
	    if (lpLVItem->mask & LVIF_TEXT)
	    {
		dispInfo.item.pszText = lpLVItem->pszText;
		dispInfo.item.cchTextMax = lpLVItem->cchTextMax;		
	    }
	    if (lpLVItem->mask & LVIF_STATE)
	        dispInfo.item.stateMask = lpLVItem->stateMask & infoPtr->uCallbackMask;
	    notify_dispinfoT(infoPtr, LVN_GETDISPINFOW, &dispInfo, isW);
	    *lpLVItem = dispInfo.item;
	    TRACE("   getdispinfo(1):lpLVItem=%s\n", debuglvitem_t(lpLVItem, isW));
	}

	/* we store only a little state, so if we're not asked, we're done */
	if (!(lpLVItem->mask & LVIF_STATE) || lpLVItem->iSubItem) return FALSE;

	/* if focus is handled by us, report it */
	if ( !(infoPtr->uCallbackMask & LVIS_FOCUSED) ) 
	{
	    lpLVItem->state &= ~LVIS_FOCUSED;
	    if (infoPtr->nFocusedItem == lpLVItem->iItem)
	        lpLVItem->state |= LVIS_FOCUSED;
        }

	/* and do the same for selection, if we handle it */
	if ( !(infoPtr->uCallbackMask & LVIS_SELECTED) ) 
	{
	    lpLVItem->state &= ~LVIS_SELECTED;
	    if ((lpLVItem->stateMask & LVIS_SELECTED) &&
		is_item_selected(infoPtr, lpLVItem->iItem))
		lpLVItem->state |= LVIS_SELECTED;
	}
	
	return TRUE;
    }

    /* find the item and subitem structures before we proceed */
    hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, lpLVItem->iItem);
    if (hdpaSubItems == NULL) return FALSE;

    if ( !(lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0)) )
        return FALSE;

    if (lpLVItem->iSubItem)
    {
	LISTVIEW_SUBITEM *lpSubItem = LISTVIEW_GetSubItemPtr(hdpaSubItems, lpLVItem->iSubItem);
        if(!lpSubItem) return FALSE;
	pItemHdr = &lpSubItem->hdr;
    }
    else
	pItemHdr = &lpItem->hdr;

    /* Do we need to query the state from the app? */
    if ((lpLVItem->mask & LVIF_STATE) && infoPtr->uCallbackMask && lpLVItem->iSubItem == 0)
    {
	dispInfo.item.mask |= LVIF_STATE;
	dispInfo.item.stateMask = infoPtr->uCallbackMask;
    }
  
    /* Do we need to enquire about the image? */
    if ((lpLVItem->mask & LVIF_IMAGE) && (pItemHdr->iImage==I_IMAGECALLBACK))
	dispInfo.item.mask |= LVIF_IMAGE;

    /* Do we need to enquire about the text? */
    if ((lpLVItem->mask & LVIF_TEXT) && !is_textW(pItemHdr->pszText))
    {
	dispInfo.item.mask |= LVIF_TEXT;
	dispInfo.item.pszText = lpLVItem->pszText;
	dispInfo.item.cchTextMax = lpLVItem->cchTextMax;
	if (dispInfo.item.pszText && dispInfo.item.cchTextMax > 0)
	    *dispInfo.item.pszText = '\0';
    }

    /* If we don't have all the requested info, query the application */
    if (dispInfo.item.mask != 0)
    {
	dispInfo.item.iItem = lpLVItem->iItem;
	dispInfo.item.iSubItem = lpLVItem->iSubItem;
	dispInfo.item.lParam = lpItem->lParam;
	notify_dispinfoT(infoPtr, LVN_GETDISPINFOW, &dispInfo, isW);
	TRACE("   getdispinfo(2):item=%s\n", debuglvitem_t(&dispInfo.item, isW));
    }

    /* Now, handle the iImage field */
    if (dispInfo.item.mask & LVIF_IMAGE)
    {
	lpLVItem->iImage = dispInfo.item.iImage;
	if ((dispInfo.item.mask & LVIF_DI_SETITEM) && (pItemHdr->iImage==I_IMAGECALLBACK))
	    pItemHdr->iImage = dispInfo.item.iImage;
    }
    else if (lpLVItem->mask & LVIF_IMAGE)
	lpLVItem->iImage = pItemHdr->iImage;

    /* The pszText field */
    if (dispInfo.item.mask & LVIF_TEXT)
    {
	if ((dispInfo.item.mask & LVIF_DI_SETITEM) && pItemHdr->pszText)
	    textsetptrT(&pItemHdr->pszText, dispInfo.item.pszText, isW);

	/* If lpLVItem->pszText==dispInfo.item.pszText a copy is unnecessary, but */
	/* some apps give a new pointer in ListView_Notify so we can't be sure.  */
	if (lpLVItem->pszText != dispInfo.item.pszText)
	    textcpynT(lpLVItem->pszText, isW, dispInfo.item.pszText, isW, lpLVItem->cchTextMax);
    }
    else if (lpLVItem->mask & LVIF_TEXT)
    {
	if (internal) lpLVItem->pszText = pItemHdr->pszText;
	else textcpynT(lpLVItem->pszText, isW, pItemHdr->pszText, TRUE, lpLVItem->cchTextMax);
    }

    /* if this is a subitem, we're done*/
    if (lpLVItem->iSubItem) return TRUE;
  
    /* Next is the lParam field */
    if (dispInfo.item.mask & LVIF_PARAM)
    {
	lpLVItem->lParam = dispInfo.item.lParam;
	if ((dispInfo.item.mask & LVIF_DI_SETITEM))
	    lpItem->lParam = dispInfo.item.lParam;
    }
    else if (lpLVItem->mask & LVIF_PARAM)
	lpLVItem->lParam = lpItem->lParam;

    /* ... the state field (this one is different due to uCallbackmask) */
    if (lpLVItem->mask & LVIF_STATE) 
    {
	lpLVItem->state = lpItem->state;
	if (dispInfo.item.mask & LVIF_STATE)
	{
	    lpLVItem->state &= ~dispInfo.item.stateMask;
	    lpLVItem->state |= (dispInfo.item.state & dispInfo.item.stateMask);
	}
	if ( !(infoPtr->uCallbackMask & LVIS_FOCUSED) ) 
	{
	    lpLVItem->state &= ~LVIS_FOCUSED;
	    if (infoPtr->nFocusedItem == lpLVItem->iItem)
	        lpLVItem->state |= LVIS_FOCUSED;
        }
	if ( !(infoPtr->uCallbackMask & LVIS_SELECTED) ) 
	{
	    lpLVItem->state &= ~LVIS_SELECTED;
	    if ((lpLVItem->stateMask & LVIS_SELECTED) &&
	        is_item_selected(infoPtr, lpLVItem->iItem))
		lpLVItem->state |= LVIS_SELECTED;
	}	    
    }

    /* and last, but not least, the indent field */
    if (lpLVItem->mask & LVIF_INDENT)
	lpLVItem->iIndent = lpItem->iIndent;

    return TRUE;
}


/***
 * DESCRIPTION:
 * Retrieves the position (upper-left) of the listview control item.
 * Note that for LVS_ICON style, the upper-left is that of the icon
 * and not the bounding box.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT : item index
 * [O] LPPOINT : coordinate information
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_GetItemPosition(LISTVIEW_INFO *infoPtr, INT nItem, LPPOINT lpptPosition)
{
    TRACE("(nItem=%d, lpptPosition=%p)\n", nItem, lpptPosition);

    if ((nItem < 0) || (nItem >= GETITEMCOUNT(infoPtr)) || !lpptPosition) return FALSE;

    return LISTVIEW_GetItemMeasures(infoPtr, nItem, lpptPosition, NULL, NULL, NULL, NULL);
}

/***
 *  Adjust a text rectangle to an integral number of text lines.
 */
static void LISTVIEW_GetIntegralLines(
	const LISTVIEW_INFO *infoPtr,
	RECT *rcText)
{
    INT i, j, k, l;

    /*
     * We need to have the bottom to be an intergal number of
     * text lines (ntmHeight) below text top that is less than
     * or equal to the nItemHeight.
	     */
    i = infoPtr->nItemHeight - infoPtr->iconSize.cy -
	ICON_TOP_PADDING - ICON_BOTTOM_PADDING;
    j = i / infoPtr->ntmHeight;
    k = j * infoPtr->ntmHeight;
    l = rcText->top + k;
    rcText->bottom = min(rcText->bottom, l);
    rcText->bottom += 1;

    TRACE("integral lines, nitemH=%d, ntmH=%d, icon.cy=%ld, i=%d, j=%d, k=%d, rect=(%d,%d)-(%d,%d)\n",
	  infoPtr->nItemHeight, infoPtr->ntmHeight, infoPtr->iconSize.cy,
	  i, j, k,
	  rcText->left, rcText->top, rcText->right, rcText->bottom);
}


/***
 * DESCRIPTION:          [INTERNAL]
 * Update the bounding rectangle around the text under a large icon.
 * This depends on whether it has the focus or not.
 * On entry the rectangle's top, left and right should be set.
 * On return the bottom will also be set and the width may have been
 * modified.
 *
 * PARAMETER
 * [I] infoPtr : pointer to the listview structure
 * [I] nItem : the item for which we are calculating this
 * [I/O] rect : the rectangle to be updated
 *
 * This appears to be weird, even in the Microsoft implementation.
 */
static BOOL LISTVIEW_UpdateLargeItemLabelRect (LISTVIEW_INFO *infoPtr, int nItem, RECT *rect)
{
    HDC hdc = GetDC (infoPtr->hwndSelf);
    HFONT hOldFont = SelectObject (hdc, infoPtr->hFont);
    UINT uFormat = LISTVIEW_DTFLAGS | DT_CALCRECT;
    RECT rcText = *rect;
    RECT rcBack = *rect;
    BOOL focused, selected;
    int dx, dy, old_wid, new_wid;
    LVITEMW lvItem;

    TRACE("%s, focus item=%d, cur item=%d\n",
	  (infoPtr->bFocus) ? "Window has focus" : "Window not focused",
	  infoPtr->nFocusedItem, nItem);


    focused = infoPtr->bFocus && LISTVIEW_GetItemState(infoPtr, nItem, LVIS_FOCUSED); 
    selected = LISTVIEW_GetItemState(infoPtr, nItem, LVIS_SELECTED);

    uFormat |= (focused) ? DT_NOCLIP : DT_WORD_ELLIPSIS | DT_END_ELLIPSIS;

    /* We (aim to) display the full text.  In Windows 95 it appears to
     * calculate the size assuming the specified font and then it draws
     * the text in that region with the specified font except scaled to
     * 10 point (or the height of the system font or ...).  Thus if the
     * window has 24 point Helvetica the highlit rectangle will be
     * taller than the text and if it is 7 point Helvetica then the text
     * will be clipped.
     * For now we will simply say that it is the correct size to display
     * the text in the specified font.
     */
    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = nItem;
    lvItem.iSubItem = 0;
    /* We will specify INTERNAL and so will receive back a const
     * pointer to the text, rather than specifying a buffer to which
     * to copy it. FIXME: what about OWNERDRAW???
     */
    if (!LISTVIEW_GetItemW(infoPtr, &lvItem, TRUE)) return FALSE;

    InflateRect(&rcText, -2, 0);
    DrawTextW (hdc, lvItem.pszText, -1, &rcText, uFormat);
    /* Microsoft, in their great wisdom, have decided that the rectangle
     * returned by DrawText on DT_CALCRECT will only guarantee the dimension,
     * not the location.  So we have to do the centring ourselves (and take
     * responsibility for agreeing off-by-one consistency with them).
     */

    old_wid = rcText.right - rcText.left;
    new_wid = rcBack.right - rcBack.left;
    dx = rcBack.left - rcText.left + (new_wid-old_wid)/2;
    dy = rcBack.top - rcText.top;
    OffsetRect (&rcText, dx, dy);

    if (focused)
    {
	rcText.bottom += 2;
    }
    else /* not focused, may or may not be selected */
    {
	LISTVIEW_GetIntegralLines(infoPtr, &rcText);
    }
    *rect = rcText;

    TRACE("%s and %s, bounding rect=(%d,%d)-(%d,%d)\n",
	  (focused) ? "focused(full text)" : "not focused",
	  (selected) ? "selected" : "not selected",
	  rect->left, rect->top, rect->right, rect->bottom);

    SelectObject (hdc, hOldFont);
    ReleaseDC (infoPtr->hwndSelf, hdc);

    return TRUE;
}

/***
 * DESCRIPTION:
 * Retrieves the bounding rectangle for a listview control item.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem : item index
 * [IO] lprc : bounding rectangle coordinates
 *     lprc->left specifies the portion of the item for which the bounding
 *     rectangle will be retrieved.
 *
 *     LVIR_BOUNDS Returns the bounding rectangle of the entire item,
 *        including the icon and label.
 *         *
 *         * For LVS_ICON
 *         * Experiment shows that native control returns:
 *         *  width = min (48, length of text line)
 *         *    .left = position.x - (width - iconsize.cx)/2
 *         *    .right = .left + width
 *         *  height = #lines of text * ntmHeight + icon height + 8
 *         *    .top = position.y - 2
 *         *    .bottom = .top + height
 *         *  separation between items .y = itemSpacing.cy - height
 *         *                           .x = itemSpacing.cx - width
 *     LVIR_ICON Returns the bounding rectangle of the icon or small icon.
 *         *
 *         * For LVS_ICON
 *         * Experiment shows that native control returns:
 *         *  width = iconSize.cx + 16
 *         *    .left = position.x - (width - iconsize.cx)/2
 *         *    .right = .left + width
 *         *  height = iconSize.cy + 4
 *         *    .top = position.y - 2
 *         *    .bottom = .top + height
 *         *  separation between items .y = itemSpacing.cy - height
 *         *                           .x = itemSpacing.cx - width
 *     LVIR_LABEL Returns the bounding rectangle of the item text.
 *         *
 *         * For LVS_ICON
 *         * Experiment shows that native control returns:
 *         *  width = text length
 *         *    .left = position.x - width/2
 *         *    .right = .left + width
 *         *  height = ntmH * linecount + 2
 *         *    .top = position.y + iconSize.cy + 6
 *         *    .bottom = .top + height
 *         *  separation between items .y = itemSpacing.cy - height
 *         *                           .x = itemSpacing.cx - width
 *     LVIR_SELECTBOUNDS Returns the union of the LVIR_ICON and LVIR_LABEL
 *	rectangles, but excludes columns in report view.
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 *
 * NOTES
 *   Note that the bounding rectangle of the label in the LVS_ICON view depends
 *   upon whether the window has the focus currently and on whether the item
 *   is the one with the focus.  Ensure that the control's record of which
 *   item has the focus agrees with the items' records.
 */
static BOOL LISTVIEW_GetItemRect(LISTVIEW_INFO *infoPtr, INT nItem, LPRECT lprc)
{
    RECT label_rect, icon_rect;

    TRACE("(hwnd=%x, nItem=%d, lprc=%p)\n", infoPtr->hwndSelf, nItem, lprc);

    if ((nItem < 0) || (nItem >= GETITEMCOUNT(infoPtr)) || !lprc) return FALSE;

    switch(lprc->left)
    {
    case LVIR_ICON:
	if (!LISTVIEW_GetItemMeasures(infoPtr, nItem, NULL, NULL, lprc, NULL, NULL)) return FALSE;
        break;

    case LVIR_LABEL:
	if (!LISTVIEW_GetItemMeasures(infoPtr, nItem, NULL, NULL, NULL, lprc, NULL)) return FALSE;
        break;

    case LVIR_BOUNDS:
	if (!LISTVIEW_GetItemMeasures(infoPtr, nItem, NULL, lprc, NULL, NULL, NULL)) return FALSE;
        break;

    case LVIR_SELECTBOUNDS:
	if (!LISTVIEW_GetItemMeasures(infoPtr, nItem, NULL, NULL, &icon_rect, &label_rect, NULL)) return FALSE;
	UnionRect (lprc, &icon_rect, &label_rect);
        break;

    default:
	WARN("Unknown value: %d\n", lprc->left);
	return FALSE;
    }

    TRACE(" rect=%s\n", debugrect(lprc));

    return TRUE;
}


static BOOL LISTVIEW_GetSubItemRect(LISTVIEW_INFO *infoPtr, INT nItem, INT nSubItem, INT flags, LPRECT lprc)
{
    UINT uView = LISTVIEW_GetType(infoPtr);
    INT  count;

    TRACE("(nItem=%d, nSubItem=%d lprc=%p)\n", nItem, nSubItem,
            lprc);

    if (!(uView & LVS_REPORT))
        return FALSE;

    if (flags & LVIR_ICON)
    {
        FIXME("Unimplemented LVIR_ICON\n");
        return FALSE;
    }
    else
    {
        int top = min(Header_GetItemCount(infoPtr->hwndHeader), nSubItem - 1);

        LISTVIEW_GetItemRect(infoPtr,nItem,lprc);
        for (count = 0; count < top; count++)
            lprc->left += LISTVIEW_GetColumnWidth(infoPtr,count);

        lprc->right = LISTVIEW_GetColumnWidth(infoPtr,(nSubItem-1)) +
                            lprc->left;
    }
    return TRUE;
}


/***
 * DESCRIPTION:
 * Retrieves the width of a label.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 *   SUCCESS : string width (in pixels)
 *   FAILURE : zero
 */
static INT LISTVIEW_GetLabelWidth(LISTVIEW_INFO *infoPtr, INT nItem)
{
    WCHAR szDispText[DISP_TEXT_SIZE] = { '\0' };
    LVITEMW lvItem;

    TRACE("(nItem=%d)\n", nItem);

    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = nItem;
    lvItem.iSubItem = 0;
    lvItem.cchTextMax = DISP_TEXT_SIZE;
    lvItem.pszText = szDispText;
    if (!LISTVIEW_GetItemW(infoPtr, &lvItem, TRUE)) return 0;
  
    /* FIXME: is this right? What if the label is very long? */ 
    return LISTVIEW_GetStringWidthT(infoPtr, lvItem.pszText, TRUE);
}

/***
 * DESCRIPTION:
 * Retrieves the spacing between listview control items.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] BOOL : flag for small or large icon
 *
 * RETURN:
 * Horizontal + vertical spacing
 */
static LRESULT LISTVIEW_GetItemSpacing(LISTVIEW_INFO *infoPtr, BOOL bSmall)
{
  LONG lResult;

  if (!bSmall)
  {
    lResult = MAKELONG(infoPtr->iconSpacing.cx, infoPtr->iconSpacing.cy);
  }
  else
  {
    if (LISTVIEW_GetType(infoPtr) == LVS_ICON)
      lResult = MAKELONG(DEFAULT_COLUMN_WIDTH, GetSystemMetrics(SM_CXSMICON)+HEIGHT_PADDING);
    else
      lResult = MAKELONG(infoPtr->nItemWidth, infoPtr->nItemHeight);
  }
  return lResult;
}

/***
 * DESCRIPTION:
 * Retrieves the state of a listview control item.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nItem : item index
 * [I] uMask : state mask
 *
 * RETURN:
 * State specified by the mask.
 */
static LRESULT LISTVIEW_GetItemState(LISTVIEW_INFO *infoPtr, INT nItem, UINT uMask)
{
    LVITEMW lvItem;

    if ((nItem < 0) || (nItem >= GETITEMCOUNT(infoPtr))) return 0;

    lvItem.iItem = nItem;
    lvItem.iSubItem = 0;
    lvItem.mask = LVIF_STATE;
    lvItem.stateMask = uMask;
    if (!LISTVIEW_GetItemW(infoPtr, &lvItem, TRUE)) return 0;

    return lvItem.state & uMask;
}

/***
 * DESCRIPTION:
 * Retrieves the text of a listview control item or subitem.
 *
 * PARAMETER(S):
 * [I] hwnd : window handle
 * [I] nItem : item index
 * [IO] lpLVItem : item information
 * [I] isW :  TRUE if lpLVItem is Unicode
 *
 * RETURN:
 *   SUCCESS : string length
 *   FAILURE : 0
 */
static LRESULT LISTVIEW_GetItemTextT(LISTVIEW_INFO *infoPtr, INT nItem, LPLVITEMW lpLVItem, BOOL isW)
{
    if (!lpLVItem || (nItem < 0) || (nItem >= GETITEMCOUNT(infoPtr))) return 0;

    lpLVItem->mask = LVIF_TEXT;
    lpLVItem->iItem = nItem;
    if (!LISTVIEW_GetItemT(infoPtr, lpLVItem, FALSE, isW)) return 0;

    return textlenT(lpLVItem->pszText, isW);
}

/***
 * DESCRIPTION:
 * Searches for an item based on properties + relationships.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT : item index
 * [I] INT : relationship flag
 *
 * RETURN:
 *   SUCCESS : item index
 *   FAILURE : -1
 */
static LRESULT LISTVIEW_GetNextItem(LISTVIEW_INFO *infoPtr, INT nItem, UINT uFlags)
{
  UINT uView = LISTVIEW_GetType(infoPtr);
  UINT uMask = 0;
  LVFINDINFOW lvFindInfo;
  INT nCountPerColumn;
  INT i;

  if ((nItem >= -1) && (nItem < GETITEMCOUNT(infoPtr)))
  {
    ZeroMemory(&lvFindInfo, sizeof(lvFindInfo));

    if (uFlags & LVNI_CUT)
      uMask |= LVIS_CUT;

    if (uFlags & LVNI_DROPHILITED)
      uMask |= LVIS_DROPHILITED;

    if (uFlags & LVNI_FOCUSED)
      uMask |= LVIS_FOCUSED;

    if (uFlags & LVNI_SELECTED)
      uMask |= LVIS_SELECTED;

    if (uFlags & LVNI_ABOVE)
    {
      if ((uView == LVS_LIST) || (uView == LVS_REPORT))
      {
        while (nItem >= 0)
        {
          nItem--;
          if ((ListView_GetItemState(infoPtr->hwndSelf, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
      else
      {
        lvFindInfo.flags = LVFI_NEARESTXY;
        lvFindInfo.vkDirection = VK_UP;
        ListView_GetItemPosition(infoPtr->hwndSelf, nItem, &lvFindInfo.pt);
        while ((nItem = ListView_FindItemW(infoPtr->hwndSelf, nItem, &lvFindInfo)) != -1)
        {
          if ((ListView_GetItemState(infoPtr->hwndSelf, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
    }
    else if (uFlags & LVNI_BELOW)
    {
      if ((uView == LVS_LIST) || (uView == LVS_REPORT))
      {
        while (nItem < GETITEMCOUNT(infoPtr))
        {
          nItem++;
          if ((LISTVIEW_GetItemState(infoPtr, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
      else
      {
        lvFindInfo.flags = LVFI_NEARESTXY;
        lvFindInfo.vkDirection = VK_DOWN;
        ListView_GetItemPosition(infoPtr->hwndSelf, nItem, &lvFindInfo.pt);
        while ((nItem = ListView_FindItemW(infoPtr->hwndSelf, nItem, &lvFindInfo)) != -1)
        {
          if ((LISTVIEW_GetItemState(infoPtr, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
    }
    else if (uFlags & LVNI_TOLEFT)
    {
      if (uView == LVS_LIST)
      {
        nCountPerColumn = LISTVIEW_GetCountPerColumn(infoPtr);
        while (nItem - nCountPerColumn >= 0)
        {
          nItem -= nCountPerColumn;
          if ((LISTVIEW_GetItemState(infoPtr, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
      else if ((uView == LVS_SMALLICON) || (uView == LVS_ICON))
      {
        lvFindInfo.flags = LVFI_NEARESTXY;
        lvFindInfo.vkDirection = VK_LEFT;
        ListView_GetItemPosition(infoPtr->hwndSelf, nItem, &lvFindInfo.pt);
        while ((nItem = ListView_FindItemW(infoPtr->hwndSelf, nItem, &lvFindInfo)) != -1)
        {
          if ((ListView_GetItemState(infoPtr->hwndSelf, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
    }
    else if (uFlags & LVNI_TORIGHT)
    {
      if (uView == LVS_LIST)
      {
        nCountPerColumn = LISTVIEW_GetCountPerColumn(infoPtr);
        while (nItem + nCountPerColumn < GETITEMCOUNT(infoPtr))
        {
          nItem += nCountPerColumn;
          if ((ListView_GetItemState(infoPtr->hwndSelf, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
      else if ((uView == LVS_SMALLICON) || (uView == LVS_ICON))
      {
        lvFindInfo.flags = LVFI_NEARESTXY;
        lvFindInfo.vkDirection = VK_RIGHT;
        ListView_GetItemPosition(infoPtr->hwndSelf, nItem, &lvFindInfo.pt);
        while ((nItem = ListView_FindItemW(infoPtr->hwndSelf, nItem, &lvFindInfo)) != -1)
        {
          if ((LISTVIEW_GetItemState(infoPtr, nItem, uMask) & uMask) == uMask)
            return nItem;
        }
      }
    }
    else
    {
      nItem++;

      /* search by index */
      for (i = nItem; i < GETITEMCOUNT(infoPtr); i++)
      {
        if ((LISTVIEW_GetItemState(infoPtr, i, uMask) & uMask) == uMask)
          return i;
      }
    }
  }

  return -1;
}

/* LISTVIEW_GetNumberOfWorkAreas */

/***
 * DESCRIPTION:
 * Retrieves the origin coordinates when in icon or small icon display mode.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [O] lpptOrigin : coordinate information
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_GetOrigin(LISTVIEW_INFO *infoPtr, LPPOINT lpptOrigin)
{
    DWORD lStyle = infoPtr->dwStyle;
    UINT uView = lStyle & LVS_TYPEMASK;
    INT nHorzPos = 0, nVertPos = 0;
    SCROLLINFO scrollInfo;

    if (!lpptOrigin) return FALSE;

    scrollInfo.cbSize = sizeof(SCROLLINFO);    
    scrollInfo.fMask = SIF_POS;
    
    if ((lStyle & WS_HSCROLL) && GetScrollInfo(infoPtr->hwndSelf, SB_HORZ, &scrollInfo))
	nHorzPos = scrollInfo.nPos;
    if ((lStyle & WS_VSCROLL) && GetScrollInfo(infoPtr->hwndSelf, SB_VERT, &scrollInfo))
	nVertPos = scrollInfo.nPos;

    TRACE("nHorzPos=%d, nVertPos=%d\n", nHorzPos, nVertPos);

    lpptOrigin->x = infoPtr->rcList.left;
    lpptOrigin->y = infoPtr->rcList.top;
    if (uView == LVS_LIST)
    {
	nHorzPos *= LISTVIEW_GetCountPerColumn(infoPtr);
	nVertPos = 0;
    }
    else if (uView == LVS_REPORT)
    {
	nVertPos *= infoPtr->nItemHeight;
    }
    
    lpptOrigin->x -= nHorzPos;
    lpptOrigin->y -= nVertPos;

    TRACE("(pt=(%ld,%ld))\n", lpptOrigin->x, lpptOrigin->y);

    return TRUE;
}

/***
 * DESCRIPTION:
 * Retrieves the number of items that are marked as selected.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 * Number of items selected.
 */
static LRESULT LISTVIEW_GetSelectedCount(LISTVIEW_INFO *infoPtr)
{
/* REDO THIS */
  INT nSelectedCount = 0;
  INT i;

  for (i = 0; i < GETITEMCOUNT(infoPtr); i++)
  {
    if (ListView_GetItemState(infoPtr->hwndSelf, i, LVIS_SELECTED) & LVIS_SELECTED)
      nSelectedCount++;
  }

  return nSelectedCount;
}

/***
 * DESCRIPTION:
 * Retrieves the width of a string.
 *
 * PARAMETER(S):
 * [I] hwnd : window handle
 * [I] lpszText : text string to process
 * [I] isW : TRUE if lpszText is Unicode, FALSE otherwise
 *
 * RETURN:
 *   SUCCESS : string width (in pixels)
 *   FAILURE : zero
 */
static LRESULT LISTVIEW_GetStringWidthT(LISTVIEW_INFO *infoPtr, LPCWSTR lpszText, BOOL isW)
{
    SIZE stringSize;
    
    stringSize.cx = 0;    
    if (is_textT(lpszText, isW))
    {
    	HFONT hFont = infoPtr->hFont ? infoPtr->hFont : infoPtr->hDefaultFont;
    	HDC hdc = GetDC(infoPtr->hwndSelf);
    	HFONT hOldFont = SelectObject(hdc, hFont);

    	if (isW)
  	    GetTextExtentPointW(hdc, lpszText, lstrlenW(lpszText), &stringSize);
    	else
  	    GetTextExtentPointA(hdc, (LPCSTR)lpszText, lstrlenA((LPCSTR)lpszText), &stringSize);
    	SelectObject(hdc, hOldFont);
    	ReleaseDC(infoPtr->hwndSelf, hdc);
    }
    return stringSize.cx;
}


/***
 * DESCRIPTION:
 * Determines item if a hit or closest if not
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [IO] lpht : hit test information
 * [I] subitem : fill out iSubItem.
 * [I] bNearItem : return the nearest item
 *
 * RETURN:
 *   SUCCESS : item index of hit
 *   FAILURE : -1
 */
static INT LISTVIEW_SuperHitTestItem(LISTVIEW_INFO *infoPtr, LPLVHITTESTINFO lpht, BOOL subitem, BOOL bNearItem)
{
  LONG lStyle = infoPtr->dwStyle;
  UINT uView = lStyle & LVS_TYPEMASK;
  INT i,j,topindex,bottomindex,nearestItem;
  RECT rcItem,rcSubItem;
  DWORD xterm, yterm, dist, mindist;

  TRACE("(x=%ld, y=%ld)\n", lpht->pt.x, lpht->pt.y);

  nearestItem = -1;
  mindist = -1;
  
  /* FIXME: get the visible range */
  topindex = LISTVIEW_GetTopIndex(infoPtr);
  if (uView == LVS_REPORT)
  {
    bottomindex = topindex + LISTVIEW_GetCountPerColumn(infoPtr) + 1;
    bottomindex = min(bottomindex,GETITEMCOUNT(infoPtr));
  }
  else
  {
    bottomindex = GETITEMCOUNT(infoPtr);
  }

  for (i = topindex; i < bottomindex; i++)
  {
    rcItem.left = LVIR_BOUNDS;
    if (LISTVIEW_GetItemRect(infoPtr, i, &rcItem))
    {
      if (PtInRect(&rcItem, lpht->pt))
      {
        rcSubItem = rcItem;
        rcItem.left = LVIR_ICON;
        if (LISTVIEW_GetItemRect(infoPtr, i, &rcItem))
        {
          if (PtInRect(&rcItem, lpht->pt))
          {
            lpht->flags = LVHT_ONITEMICON;
            lpht->iItem = i;
            goto set_subitem;
          }
        }

        rcItem.left = LVIR_LABEL;
        if (LISTVIEW_GetItemRect(infoPtr, i, &rcItem))
        {
          if (PtInRect(&rcItem, lpht->pt))
          {
            lpht->flags = LVHT_ONITEMLABEL;
            lpht->iItem = i;
            goto set_subitem;
          }
        }

        lpht->flags = LVHT_ONITEMSTATEICON;
        lpht->iItem = i;
       set_subitem:
        if (subitem)
        {
	  INT nColumnCount = Header_GetItemCount(infoPtr->hwndHeader);
          lpht->iSubItem = 0;
          rcSubItem.right = rcSubItem.left;
          for (j = 0; j < nColumnCount; j++)
          {
            rcSubItem.left = rcSubItem.right;
            rcSubItem.right += LISTVIEW_GetColumnWidth(infoPtr, j);
            if (PtInRect(&rcSubItem, lpht->pt))
            {
              lpht->iSubItem = j;
              break;
            }
          }
        }
	TRACE("hit on item %d\n", i);
        return i;
      }
      else if (bNearItem)
      {
        /*
         * Now compute distance from point to center of boundary
         * box. Since we are only interested in the relative
         * distance, we can skip the nasty square root operation
         */
        xterm = rcItem.left + (rcItem.right - rcItem.left)/2 - lpht->pt.x;
        yterm = rcItem.top + (rcItem.bottom - rcItem.top)/2 - lpht->pt.y;
        dist = xterm * xterm + yterm * yterm;
	if (mindist < 0 || dist < mindist)
	{
	    mindist = dist;
	    nearestItem = i;
	}
      }
    }
  }

  lpht->flags = LVHT_NOWHERE;

  return bNearItem ? nearestItem : -1;
}


/***
 * DESCRIPTION:
 * Determines which listview item is located at the specified position.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [IO] lpht : hit test information
 * [I] subitem : fill out iSubItem.
 *
 * RETURN:
 *   SUCCESS : item index
 *   FAILURE : -1
 */
static LRESULT LISTVIEW_HitTest(LISTVIEW_INFO *infoPtr, LPLVHITTESTINFO lpht, BOOL subitem)
{
    TRACE("(x=%ld, y=%ld)\n", lpht->pt.x, lpht->pt.y);
    
    lpht->flags = 0;

    if (infoPtr->rcList.left > lpht->pt.x)
	lpht->flags |= LVHT_TOLEFT;
    else if (infoPtr->rcList.right < lpht->pt.x)
	lpht->flags |= LVHT_TORIGHT;
    
    if (infoPtr->rcList.top > lpht->pt.y)
	lpht->flags |= LVHT_ABOVE;
    else if (infoPtr->rcList.bottom < lpht->pt.y)
	lpht->flags |= LVHT_BELOW;

    if (lpht->flags) return -1;
    
    /* NOTE (mm 20001022): We must not allow iSubItem to be touched, for
     * an app might pass only a structure with space up to iItem!
     * (MS Office 97 does that for instance in the file open dialog)
     */
    return LISTVIEW_SuperHitTestItem(infoPtr, lpht, subitem, FALSE);
}


/***
 * DESCRIPTION:
 * Inserts a new column.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT : column index
 * [I] LPLVCOLUMNW : column information
 *
 * RETURN:
 *   SUCCESS : new column index
 *   FAILURE : -1
 */
static LRESULT LISTVIEW_InsertColumnT(LISTVIEW_INFO *infoPtr, INT nColumn,
                                      LPLVCOLUMNW lpColumn, BOOL isW)
{
    RECT rcOld, rcCol;
    INT nNewColumn;
    HDITEMW hdi;

    TRACE("(nColumn=%d, lpColumn=%s, isW=%d)\n", nColumn, debuglvcolumn_t(lpColumn, isW), isW);

    if (!lpColumn) return -1;

    hdi.mask = hdi.fmt = 0;
    if (lpColumn->mask & LVCF_FMT)
    {
	/* format member is valid */
	hdi.mask |= HDI_FORMAT;

	/* set text alignment (leftmost column must be left-aligned) */
        if (nColumn == 0 || lpColumn->fmt & LVCFMT_LEFT)
            hdi.fmt |= HDF_LEFT;
        else if (lpColumn->fmt & LVCFMT_RIGHT)
            hdi.fmt |= HDF_RIGHT;
        else if (lpColumn->fmt & LVCFMT_CENTER)
            hdi.fmt |= HDF_CENTER;

        if (lpColumn->fmt & LVCFMT_BITMAP_ON_RIGHT)
            hdi.fmt |= HDF_BITMAP_ON_RIGHT;

        if (lpColumn->fmt & LVCFMT_COL_HAS_IMAGES)
        {
            hdi.fmt |= HDF_IMAGE;
            hdi.iImage = I_IMAGECALLBACK;
        }

        if (lpColumn->fmt & LVCFMT_IMAGE)
	   ; /* FIXME: enable images for *(sub)items* this column */
    }

    if (lpColumn->mask & LVCF_WIDTH)
    {
        hdi.mask |= HDI_WIDTH;
        if(lpColumn->cx == LVSCW_AUTOSIZE_USEHEADER)
        {
            /* make it fill the remainder of the controls width */
            HDITEMW hdit;
            RECT rcHeader;
            INT item_index;

            /* get the width of every item except the current one */
            hdit.mask = HDI_WIDTH;
            hdi.cxy = 0;

            for(item_index = 0; item_index < (nColumn - 1); item_index++)
            	if (Header_GetItemW(infoPtr->hwndHeader, item_index, (LPARAM)(&hdit)))
		    hdi.cxy += hdit.cxy;

            /* retrieve the layout of the header */
            GetClientRect(infoPtr->hwndSelf, &rcHeader);
            TRACE("start cxy=%d rcHeader=%s\n", hdi.cxy, debugrect(&rcHeader));

            hdi.cxy = (rcHeader.right - rcHeader.left) - hdi.cxy;
        }
        else
            hdi.cxy = lpColumn->cx;
    }

    if (lpColumn->mask & LVCF_TEXT)
    {
        hdi.mask |= HDI_TEXT | HDI_FORMAT;
        hdi.fmt |= HDF_STRING;
        hdi.pszText = lpColumn->pszText;
        hdi.cchTextMax = textlenT(lpColumn->pszText, isW);
    }

    if (lpColumn->mask & LVCF_IMAGE)
    {
        hdi.mask |= HDI_IMAGE;
        hdi.iImage = lpColumn->iImage;
    }

    if (lpColumn->mask & LVCF_ORDER)
    {
	hdi.mask |= HDI_ORDER;
	hdi.iOrder = lpColumn->iOrder;
    }

    /* insert item in header control */
    nNewColumn = SendMessageW(infoPtr->hwndHeader, 
		              isW ? HDM_INSERTITEMW : HDM_INSERTITEMA,
                              (WPARAM)nColumn, (LPARAM)&hdi);
    if (nNewColumn == -1) return -1;
    if (!Header_GetItemRect(infoPtr->hwndHeader, nNewColumn, &rcCol)) return -1;
   
    /* now we have to actually adjust the data */
    if (!(infoPtr->dwStyle & LVS_OWNERDATA) && infoPtr->hdpaItems->nItemCount > 0)
    {
	LISTVIEW_SUBITEM *lpSubItem, *lpMainItem, **lpNewItems = 0;
	HDPA hdpaSubItems;
	INT nItem, i;
	
	/* preallocate memory, so we can fail gracefully */
	if (nNewColumn == 0)
	{
	    lpNewItems = COMCTL32_Alloc(sizeof(LISTVIEW_SUBITEM *) * infoPtr->hdpaItems->nItemCount);
	    if (!lpNewItems) return -1;
	    for (i = 0; i < infoPtr->hdpaItems->nItemCount; i++)
		if (!(lpNewItems[i] = COMCTL32_Alloc(sizeof(LISTVIEW_SUBITEM)))) break;
	    if (i != infoPtr->hdpaItems->nItemCount)
	    {
		for(; i >=0; i--) COMCTL32_Free(lpNewItems[i]);
		COMCTL32_Free(lpNewItems);
		return -1;
	    }
	}
	
	for (nItem = 0; nItem < infoPtr->hdpaItems->nItemCount; nItem++)
	{
	    hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, nItem);
	    if (!hdpaSubItems) continue;
	    for (i = 1; i < hdpaSubItems->nItemCount; i++)
	    {
		lpSubItem = (LISTVIEW_SUBITEM *)DPA_GetPtr(hdpaSubItems, i);
		if (!lpSubItem) break;
		if (lpSubItem->iSubItem >= nNewColumn)
		    lpSubItem->iSubItem++;
	    }

	    /* if we found our subitem, zapp it */	
	    if (nNewColumn == 0)
	    {
		lpMainItem = (LISTVIEW_SUBITEM *)DPA_GetPtr(hdpaSubItems, 0);
		lpSubItem = lpNewItems[nItem];
		lpSubItem->hdr = lpMainItem->hdr;
		lpSubItem->iSubItem = 1;
		ZeroMemory(&lpMainItem->hdr, sizeof(lpMainItem->hdr));
		lpMainItem->iSubItem = 0;
		DPA_InsertPtr(hdpaSubItems, 1, lpSubItem);
    	    }
	}

	COMCTL32_Free(lpNewItems);
    }

    /* we don't have to worry abiut display issues in non-report mode */
    if ((infoPtr->dwStyle & LVS_TYPEMASK) != LVS_REPORT) return nNewColumn;
    
    /* Need to reset the item width when inserting a new column */
    infoPtr->nItemWidth = LISTVIEW_GetItemWidth(infoPtr);

    LISTVIEW_UpdateScroll(infoPtr);

    /* scroll to cover the deleted column, and invalidate for redraw */
    rcOld = infoPtr->rcList;
    rcOld.left = rcCol.left;
    ScrollWindowEx(infoPtr->hwndSelf, rcCol.right - rcCol.left, 0,
		   &rcOld, &rcOld, 0, 0, SW_ERASE | SW_INVALIDATE);
    
    return nNewColumn;
}

/* LISTVIEW_InsertCompare:  callback routine for comparing pszText members of the LV_ITEMS
   in a LISTVIEW on insert.  Passed to DPA_Sort in LISTVIEW_InsertItem.
   This function should only be used for inserting items into a sorted list (LVM_INSERTITEM)
   and not during the processing of a LVM_SORTITEMS message. Applications should provide
   their own sort proc. when sending LVM_SORTITEMS.
*/
/* Platform SDK:
    (remarks on LVITEM: LVM_INSERTITEM will insert the new item in the proper sort postion...
        if:
          LVS_SORTXXX must be specified,
          LVS_OWNERDRAW is not set,
          <item>.pszText is not LPSTR_TEXTCALLBACK.

    (LVS_SORT* flags): "For the LVS_SORTASCENDING... styles, item indices
    are sorted based on item text..."
*/
static INT WINAPI LISTVIEW_InsertCompare(  LPVOID first, LPVOID second,  LPARAM lParam)
{
    LISTVIEW_ITEM* lv_first = (LISTVIEW_ITEM*) DPA_GetPtr( (HDPA)first, 0 );
    LISTVIEW_ITEM* lv_second = (LISTVIEW_ITEM*) DPA_GetPtr( (HDPA)second, 0 );
    INT cmpv = textcmpWT(lv_first->hdr.pszText, lv_second->hdr.pszText, TRUE); 

    /* if we're sorting descending, negate the return value */
    return (((LISTVIEW_INFO *)lParam)->dwStyle & LVS_SORTDESCENDING) ? -cmpv : cmpv;
}

/***
 * nESCRIPTION:
 * Inserts a new item in the listview control.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] lpLVItem : item information
 * [I] isW : TRUE if lpLVItem is Unicode, FALSE if it's ANSI
 *
 * RETURN:
 *   SUCCESS : new item index
 *   FAILURE : -1
 */
static LRESULT LISTVIEW_InsertItemT(LISTVIEW_INFO *infoPtr, LPLVITEMW lpLVItem, BOOL isW)
{
    LONG lStyle = infoPtr->dwStyle;
    UINT uView = lStyle & LVS_TYPEMASK;
    INT nItem = -1;
    HDPA hdpaSubItems;
    NMLISTVIEW nmlv;
    LISTVIEW_ITEM *lpItem;
    BOOL is_sorted;

    TRACE("(lpLVItem=%s, isW=%d)\n", debuglvitem_t(lpLVItem, isW), isW);

    if (lStyle & LVS_OWNERDATA)
    {
	nItem = infoPtr->hdpaItems->nItemCount;
	infoPtr->hdpaItems->nItemCount++;
	return nItem;
    }

    /* make sure it's an item, and not a subitem; cannot insert a subitem */
    if (!lpLVItem || lpLVItem->iSubItem) return -1;

    if (!is_assignable_item(lpLVItem, lStyle)) return -1;

    if ( !(lpItem = (LISTVIEW_ITEM *)COMCTL32_Alloc(sizeof(LISTVIEW_ITEM))) )
	return -1;
    
    /* insert item in listview control data structure */
    if ( (hdpaSubItems = DPA_Create(8)) )
	nItem = DPA_InsertPtr(hdpaSubItems, 0, lpItem);
    if (nItem == -1) goto fail;

    is_sorted = (lStyle & (LVS_SORTASCENDING | LVS_SORTDESCENDING)) &&
	        !(lStyle & LVS_OWNERDRAWFIXED) && (LPSTR_TEXTCALLBACKW != lpLVItem->pszText);

    nItem = DPA_InsertPtr( infoPtr->hdpaItems, 
		           is_sorted ? GETITEMCOUNT( infoPtr ) + 1 : lpLVItem->iItem, 
			   hdpaSubItems );
    if (nItem == -1) goto fail;
   
    if (!LISTVIEW_SetItemT(infoPtr, lpLVItem, isW))
    {
	DPA_DeletePtr(infoPtr->hdpaItems, nItem);
	goto fail;
    }

    /* if we're sorted, sort the list, and update the index */
    if (is_sorted)
    {
	DPA_Sort( infoPtr->hdpaItems, LISTVIEW_InsertCompare, (LPARAM)infoPtr );
	nItem = DPA_GetPtrIndex( infoPtr->hdpaItems, hdpaSubItems );
	if (nItem == -1)
	{
	    ERR("We can't find the item we just inserted, possible memory corruption.");
	    /* we can't remove it from the list if we can't find it, so just fail */
	    /* we don't deallocate memory here, as it will probably cause more problems */
	    return -1;
	}
    }

    /* Add the subitem list to the items array. Do this last in case we go to
     * fail during the above.
     */
    LISTVIEW_ShiftIndices(infoPtr, nItem, 1);
    
    lpItem->valid = TRUE;

    /* send LVN_INSERTITEM notification */
    ZeroMemory(&nmlv, sizeof(NMLISTVIEW));
    nmlv.iItem = nItem;
    nmlv.lParam = lpItem->lParam;
    notify_listview(infoPtr, LVN_INSERTITEM, &nmlv);

    /* align items (set position of each item) */
    if ((uView == LVS_SMALLICON) || (uView == LVS_ICON))
    {
	if (lStyle & LVS_ALIGNLEFT) LISTVIEW_AlignLeft(infoPtr);
        else LISTVIEW_AlignTop(infoPtr);
    }

    LISTVIEW_UpdateScroll(infoPtr);
    
    /* FIXME: refresh client area */
    LISTVIEW_InvalidateList(infoPtr);

    TRACE("    <- %d\n", nItem);
    return nItem;

fail:
    DPA_DeletePtr(hdpaSubItems, 0);
    DPA_Destroy (hdpaSubItems);
    COMCTL32_Free (lpItem);
    return -1;
}

/***
 * DESCRIPTION:
 * Redraws a range of items.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT : first item
 * [I] INT : last item
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_RedrawItems(LISTVIEW_INFO *infoPtr, INT nFirst, INT nLast)
{
    INT i;
 
    if (nLast < nFirst || min(nFirst, nLast) < 0 || 
	max(nFirst, nLast) >= GETITEMCOUNT(infoPtr))
	return FALSE;
    
    for (i = nFirst; i <= nLast; i++)
	LISTVIEW_InvalidateItem(infoPtr, i);

    return TRUE;
}

/***
 * DESCRIPTION:
 * Scroll the content of a listview.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT : horizontal scroll amount in pixels
 * [I] INT : vertical scroll amount in pixels
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 *
 * COMMENTS:
 *  If the control is in report mode (LVS_REPORT) the control can
 *  be scrolled only in line increments. "dy" will be rounded to the
 *  nearest number of pixels that are a whole line. Ex: if line height
 *  is 16 and an 8 is passed, the list will be scrolled by 16. If a 7
 *  is passed the the scroll will be 0.  (per MSDN 7/2002)
 *
 *  For:  (per experimentaion with native control and CSpy ListView)
 *     LVS_ICON       dy=1 = 1 pixel  (vertical only)
 *                    dx ignored
 *     LVS_SMALLICON  dy=1 = 1 pixel  (vertical only)
 *                    dx ignored
 *     LVS_LIST       dx=1 = 1 column (horizontal only)
 *                           but will only scroll 1 column per message
 *                           no matter what the value.
 *                    dy must be 0 or FALSE returned.
 *     LVS_REPORT     dx=1 = 1 pixel
 *                    dy=  see above
 *
 */
static LRESULT LISTVIEW_Scroll(LISTVIEW_INFO *infoPtr, INT dx, INT dy)
{
    switch(LISTVIEW_GetType(infoPtr)) {
    case LVS_REPORT:
	dy += (dy < 0 ? -1 : 1) * infoPtr->nItemHeight/2;
        dy /= infoPtr->nItemHeight;
	break;
    case LVS_LIST:
    	if (dy != 0) return FALSE;
	break;
    default: /* icon */
	dx = 0;
	break;
    }	

    if (dx != 0) LISTVIEW_HScroll(infoPtr, SB_INTERNAL, dx, 0);
    if (dy != 0) LISTVIEW_VScroll(infoPtr, SB_INTERNAL, dy, 0);
  
    return TRUE;
}

/***
 * DESCRIPTION:
 * Sets the background color.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] COLORREF : background color
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_SetBkColor(LISTVIEW_INFO *infoPtr, COLORREF clrBk)
{
    TRACE("(clrBk=%lx)\n", clrBk);

    if(infoPtr->clrBk != clrBk) {
	if (infoPtr->clrBk != CLR_NONE) DeleteObject(infoPtr->hBkBrush);
	infoPtr->clrBk = clrBk;
	if (clrBk == CLR_NONE)
	    infoPtr->hBkBrush = GetClassLongW(infoPtr->hwndSelf, GCL_HBRBACKGROUND);
	else
	    infoPtr->hBkBrush = CreateSolidBrush(clrBk);
	LISTVIEW_InvalidateList(infoPtr);
    }

   return TRUE;
}

/* LISTVIEW_SetBkImage */

/***
 * DESCRIPTION:
 * Sets the attributes of a header item.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT : column index
 * [I] LPLVCOLUMNW : column attributes
 * [I] isW: if TRUE, the lpColumn is a LPLVCOLUMNW,
 *          otherwise it is in fact a LPLVCOLUMNA
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_SetColumnT(LISTVIEW_INFO *infoPtr, INT nColumn,
                                   LPLVCOLUMNW lpColumn, BOOL isW)
{
  BOOL bResult = FALSE;
  HDITEMW hdi, hdiget;

  if ((lpColumn != NULL) && (nColumn >= 0) &&
      (nColumn < Header_GetItemCount(infoPtr->hwndHeader)))
  {
    /* initialize memory */
    ZeroMemory(&hdi, sizeof(hdi));

    if (lpColumn->mask & LVCF_FMT)
    {
      /* format member is valid */
      hdi.mask |= HDI_FORMAT;

      /* get current format first */
      hdiget.mask = HDI_FORMAT;
      if (Header_GetItemW(infoPtr->hwndHeader, nColumn, &hdiget))
	      /* preserve HDF_STRING if present */
	      hdi.fmt = hdiget.fmt & HDF_STRING;

      /* set text alignment (leftmost column must be left-aligned) */
      if (nColumn == 0)
      {
        hdi.fmt |= HDF_LEFT;
      }
      else
      {
        if (lpColumn->fmt & LVCFMT_LEFT)
          hdi.fmt |= HDF_LEFT;
        else if (lpColumn->fmt & LVCFMT_RIGHT)
          hdi.fmt |= HDF_RIGHT;
        else if (lpColumn->fmt & LVCFMT_CENTER)
          hdi.fmt |= HDF_CENTER;
      }

      if (lpColumn->fmt & LVCFMT_BITMAP_ON_RIGHT)
        hdi.fmt |= HDF_BITMAP_ON_RIGHT;

      if (lpColumn->fmt & LVCFMT_COL_HAS_IMAGES)
        hdi.fmt |= HDF_IMAGE;

      if (lpColumn->fmt & LVCFMT_IMAGE)
      {
        hdi.fmt |= HDF_IMAGE;
        hdi.iImage = I_IMAGECALLBACK;
      }
    }

    if (lpColumn->mask & LVCF_WIDTH)
    {
      hdi.mask |= HDI_WIDTH;
      hdi.cxy = lpColumn->cx;
    }

    if (lpColumn->mask & LVCF_TEXT)
    {
      hdi.mask |= HDI_TEXT | HDI_FORMAT;
      hdi.pszText = lpColumn->pszText;
      hdi.cchTextMax = textlenT(lpColumn->pszText, isW);
      hdi.fmt |= HDF_STRING;
    }

    if (lpColumn->mask & LVCF_IMAGE)
    {
      hdi.mask |= HDI_IMAGE;
      hdi.iImage = lpColumn->iImage;
    }

    if (lpColumn->mask & LVCF_ORDER)
    {
      hdi.mask |= HDI_ORDER;
      hdi.iOrder = lpColumn->iOrder;
    }

    /* set header item attributes */
    if (isW)
      bResult = Header_SetItemW(infoPtr->hwndHeader, nColumn, &hdi);
    else
      bResult = Header_SetItemA(infoPtr->hwndHeader, nColumn, &hdi);
  }

  return bResult;
}

/***
 * DESCRIPTION:
 * Sets the column order array
 *
 * PARAMETERS:
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT : number of elements in column order array
 * [I] INT : pointer to column order array
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_SetColumnOrderArray(LISTVIEW_INFO *infoPtr, INT iCount, LPINT lpiArray)
{
  FIXME("iCount %d lpiArray %p\n", iCount, lpiArray);

  if (!lpiArray)
    return FALSE;

  return TRUE;
}


/***
 * DESCRIPTION:
 * Sets the width of a column
 *
 * PARAMETERS:
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT : column index
 * [I] INT : column width
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_SetColumnWidth(LISTVIEW_INFO *infoPtr, INT iCol, INT cx)
{
    HDITEMW hdi;
    LRESULT lret;
    LONG lStyle = infoPtr->dwStyle;
    UINT uView = lStyle & LVS_TYPEMASK;
    HDC hdc;
    HFONT header_font;
    HFONT old_font;
    SIZE size;
    WCHAR text_buffer[DISP_TEXT_SIZE];
    INT header_item_count;
    INT item_index;
    INT nLabelWidth;
    RECT rcHeader;
    LVITEMW lvItem;
    WCHAR szDispText[DISP_TEXT_SIZE];

    if (!infoPtr->hwndHeader) /* make sure we have a header */
      return (FALSE);

    /* set column width only if in report or list mode */
    if ((uView != LVS_REPORT) && (uView != LVS_LIST))
      return (FALSE);

    TRACE("(iCol=%d, cx=%d\n", iCol, cx);

    /* take care of invalid cx values */
    if((uView == LVS_REPORT) && (cx < -2))
      cx = LVSCW_AUTOSIZE;
    else if (uView == LVS_LIST && (cx < 1))
      return FALSE;

    /* resize all columns if in LVS_LIST mode */
    if(uView == LVS_LIST) {
      infoPtr->nItemWidth = cx;
      LISTVIEW_InvalidateList(infoPtr);
      return TRUE;
    }

    /* autosize based on listview items width */
    if(cx == LVSCW_AUTOSIZE)
    {
      /* set the width of the column to the width of the widest item */
      if (iCol == 0 || uView == LVS_LIST)
      {
        cx = 0;
        for(item_index = 0; item_index < GETITEMCOUNT(infoPtr); item_index++)
        {
          nLabelWidth = LISTVIEW_GetLabelWidth(infoPtr, item_index);
          cx = (nLabelWidth>cx)?nLabelWidth:cx;
        }
        if (infoPtr->himlSmall)
          cx += infoPtr->iconSize.cx + IMAGE_PADDING;
      }
      else
      {
        lvItem.iSubItem = iCol;
        lvItem.mask = LVIF_TEXT;
        lvItem.cchTextMax = DISP_TEXT_SIZE;
        lvItem.pszText = szDispText;
        *lvItem.pszText = '\0';
        cx = 0;
        for(item_index = 0; item_index < GETITEMCOUNT(infoPtr); item_index++)
        {
          lvItem.iItem = item_index;
          if (!LISTVIEW_GetItemT(infoPtr, &lvItem, FALSE, TRUE)) continue;
          nLabelWidth = LISTVIEW_GetStringWidthT(infoPtr, lvItem.pszText, TRUE);
          cx = (nLabelWidth>cx)?nLabelWidth:cx;
        }
      }
      cx += TRAILING_PADDING;
    } /* autosize based on listview header width */
    else if(cx == LVSCW_AUTOSIZE_USEHEADER)
    {
      header_item_count = Header_GetItemCount(infoPtr->hwndHeader);

      /* if iCol is the last column make it fill the remainder of the controls width */
      if(iCol == (header_item_count - 1)) {
        /* get the width of every item except the current one */
        hdi.mask = HDI_WIDTH;
        cx = 0;

        for(item_index = 0; item_index < (header_item_count - 1); item_index++) {
          Header_GetItemW(infoPtr->hwndHeader, item_index, (LPARAM)(&hdi));
          cx+=hdi.cxy;
        }

        /* retrieve the layout of the header */
        GetWindowRect(infoPtr->hwndHeader, &rcHeader);

        cx = (rcHeader.right - rcHeader.left) - cx;
      }
      else
      {
        /* Despite what the MS docs say, if this is not the last
           column, then MS resizes the column to the width of the
           largest text string in the column, including headers
           and items. This is different from LVSCW_AUTOSIZE in that
           LVSCW_AUTOSIZE ignores the header string length.
           */

        /* retrieve header font */
        header_font = SendMessageW(infoPtr->hwndHeader, WM_GETFONT, 0L, 0L);

        /* retrieve header text */
        hdi.mask = HDI_TEXT;
        hdi.cchTextMax = sizeof(text_buffer)/sizeof(text_buffer[0]);
        hdi.pszText = text_buffer;

        Header_GetItemW(infoPtr->hwndHeader, iCol, (LPARAM)(&hdi));

        /* determine the width of the text in the header */
        hdc = GetDC(infoPtr->hwndSelf);
        old_font = SelectObject(hdc, header_font); /* select the font into hdc */

        GetTextExtentPoint32W(hdc, text_buffer, lstrlenW(text_buffer), &size);

        SelectObject(hdc, old_font); /* restore the old font */
        ReleaseDC(infoPtr->hwndSelf, hdc);

        lvItem.iSubItem = iCol;
        lvItem.mask = LVIF_TEXT;
        lvItem.cchTextMax = DISP_TEXT_SIZE;
        lvItem.pszText = szDispText;
        *lvItem.pszText = '\0';
        cx = size.cx;
        for(item_index = 0; item_index < GETITEMCOUNT(infoPtr); item_index++)
        {
          lvItem.iItem = item_index;
          if (!LISTVIEW_GetItemT(infoPtr, &lvItem, FALSE, TRUE)) continue;
          nLabelWidth = LISTVIEW_GetStringWidthT(infoPtr, lvItem.pszText, TRUE);
          nLabelWidth += TRAILING_PADDING;
          /* While it is possible for subitems to have icons, even MS messes
             up the positioning, so I suspect no applications actually use
             them. */
          if (item_index == 0 && infoPtr->himlSmall)
            nLabelWidth += infoPtr->iconSize.cx + IMAGE_PADDING;
          cx = (nLabelWidth>cx)?nLabelWidth:cx;
        }
      }
  }

  /* call header to update the column change */
  hdi.mask = HDI_WIDTH;

  hdi.cxy = cx;
  lret = Header_SetItemW(infoPtr->hwndHeader, (WPARAM)iCol, (LPARAM)&hdi);

  LISTVIEW_InvalidateList(infoPtr);

  return lret;
}

/***
 * DESCRIPTION:
 * Sets the extended listview style.
 *
 * PARAMETERS:
 * [I] infoPtr : valid pointer to the listview structure
 * [I] DWORD : mask
 * [I] DWORD : style
 *
 * RETURN:
 *   SUCCESS : previous style
 *   FAILURE : 0
 */
static LRESULT LISTVIEW_SetExtendedListViewStyle(LISTVIEW_INFO *infoPtr, DWORD dwMask, DWORD dwStyle)
{
  DWORD dwOldStyle = infoPtr->dwLvExStyle;

  /* set new style */
  if (dwMask)
    infoPtr->dwLvExStyle = (dwOldStyle & ~dwMask) | (dwStyle & dwMask);
  else
    infoPtr->dwLvExStyle = dwStyle;

  return dwOldStyle;
}

/***
 * DESCRIPTION:
 * Sets the new hot cursor used during hot tracking and hover selection.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I} hCurosr : the new hot cursor handle
 *
 * RETURN:
 * Returns the previous hot cursor
 */
static HCURSOR LISTVIEW_SetHotCursor(LISTVIEW_INFO *infoPtr, HCURSOR hCursor)
{
    HCURSOR oldCursor = infoPtr->hHotCursor;
    infoPtr->hHotCursor = hCursor;
    return oldCursor;
}


/***
 * DESCRIPTION:
 * Sets the hot item index.
 *
 * PARAMETERS:
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT   : index
 *
 * RETURN:
 *   SUCCESS : previous hot item index
 *   FAILURE : -1 (no hot item)
 */
static LRESULT LISTVIEW_SetHotItem(LISTVIEW_INFO *infoPtr, INT iIndex)
{
    INT iOldIndex = infoPtr->nHotItem;
    infoPtr->nHotItem = iIndex;
    return iOldIndex;
}


/***
 * DESCRIPTION:
 * Sets the amount of time the cursor must hover over an item before it is selected.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] DWORD : dwHoverTime, if -1 the hover time is set to the default
 *
 * RETURN:
 * Returns the previous hover time
 */
static LRESULT LISTVIEW_SetHoverTime(LISTVIEW_INFO *infoPtr, DWORD dwHoverTime)
{
    DWORD oldHoverTime = infoPtr->dwHoverTime;
    infoPtr->dwHoverTime = dwHoverTime;
    return oldHoverTime;
}

/***
 * DESCRIPTION:
 * Sets spacing for icons of LVS_ICON style.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] DWORD : MAKELONG(cx, cy)
 *
 * RETURN:
 *   MAKELONG(oldcx, oldcy)
 */
static LRESULT LISTVIEW_SetIconSpacing(LISTVIEW_INFO *infoPtr, DWORD spacing)
{
    INT cy = HIWORD(spacing), cx = LOWORD(spacing);
    DWORD oldspacing = MAKELONG(infoPtr->iconSpacing.cx, infoPtr->iconSpacing.cy);
    LONG lStyle = infoPtr->dwStyle;
    UINT uView = lStyle & LVS_TYPEMASK;

    TRACE("requested=(%d,%d)\n", cx, cy);
    
    /* this is supported only for LVS_ICON style */
    if (uView != LVS_ICON) return oldspacing;
  
    /* set to defaults, if instructed to */
    if (cx == -1) cx = GetSystemMetrics(SM_CXICONSPACING);
    if (cy == -1) cy = GetSystemMetrics(SM_CYICONSPACING);

    /* if 0 then compute width
     * FIXME: Should scan each item and determine max width of
     *        icon or label, then make that the width */
    if (cx == 0)
	cx = infoPtr->iconSpacing.cx;

    /* if 0 then compute height */
    if (cy == 0) 
	cy = infoPtr->iconSize.cy + 2 * infoPtr->ntmHeight +
	     ICON_BOTTOM_PADDING + ICON_TOP_PADDING + LABEL_VERT_PADDING;
    

    infoPtr->iconSpacing.cx = cx;
    infoPtr->iconSpacing.cy = cy;

    TRACE("old=(%d,%d), new=(%d,%d), iconSize=(%ld,%ld), ntmH=%d\n",
	  LOWORD(oldspacing), HIWORD(oldspacing), cx, cy, 
	  infoPtr->iconSize.cx, infoPtr->iconSize.cy,
	  infoPtr->ntmHeight);

    /* these depend on the iconSpacing */
    infoPtr->nItemWidth = LISTVIEW_GetItemWidth(infoPtr);
    infoPtr->nItemHeight = LISTVIEW_GetItemHeight(infoPtr);

    return oldspacing;
}

inline void update_icon_size(HIMAGELIST himl, SIZE *size)
{
    INT cx, cy;
    
    if (himl && ImageList_GetIconSize(himl, &cx, &cy))
    {
	size->cx = cx;
	size->cy = cy;
    }
    else
	size->cx = size->cy = 0;
}

/***
 * DESCRIPTION:
 * Sets image lists.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT : image list type
 * [I] HIMAGELIST : image list handle
 *
 * RETURN:
 *   SUCCESS : old image list
 *   FAILURE : NULL
 */
static HIMAGELIST LISTVIEW_SetImageList(LISTVIEW_INFO *infoPtr, INT nType, HIMAGELIST himl)
{
    UINT uView = LISTVIEW_GetType(infoPtr);
    INT oldHeight = infoPtr->nItemHeight;
    HIMAGELIST himlOld = 0;

    switch (nType)
    {
    case LVSIL_NORMAL:
        himlOld = infoPtr->himlNormal;
        infoPtr->himlNormal = himl;
        if (uView == LVS_ICON) update_icon_size(himl, &infoPtr->iconSize);
        LISTVIEW_SetIconSpacing(infoPtr, 0);
    break;

    case LVSIL_SMALL:
        himlOld = infoPtr->himlSmall;
        infoPtr->himlSmall = himl;
         if (uView != LVS_ICON) update_icon_size(himl, &infoPtr->iconSize);
    break;

    case LVSIL_STATE:
        himlOld = infoPtr->himlState;
        infoPtr->himlState = himl;
        update_icon_size(himl, &infoPtr->iconStateSize);
        ImageList_SetBkColor(infoPtr->himlState, CLR_NONE);
    break;

    default:
        ERR("Unknown icon type=%d\n", nType);
	return NULL;
    }

    infoPtr->nItemHeight = LISTVIEW_GetItemHeight(infoPtr);
    if (infoPtr->nItemHeight != oldHeight)
        LISTVIEW_UpdateScroll(infoPtr);

    return himlOld;
}

/***
 * DESCRIPTION:
 * Preallocates memory (does *not* set the actual count of items !)
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT   : item count (projected number of items to allocate)
 * [I] DWORD : update flags
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetItemCount(LISTVIEW_INFO *infoPtr, INT nItems, DWORD dwFlags)
{
  TRACE("(nItems=%d, dwFlags=%lx)\n", nItems, dwFlags);

  if (infoPtr->dwStyle & LVS_OWNERDATA)
  {
      int precount,topvisible;

      TRACE("LVS_OWNERDATA is set!\n");
      if (dwFlags & (LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL))
        FIXME("flags %s %s not implemented\n",
              (dwFlags & LVSICF_NOINVALIDATEALL) ? "LVSICF_NOINVALIDATEALL"
              : "",
              (dwFlags & LVSICF_NOSCROLL) ? "LVSICF_NOSCROLL" : "");

      /*
       * Internally remove all the selections.
       */
      do
      {
        RANGE *selection;
        selection = DPA_GetPtr(infoPtr->hdpaSelectionRanges,0);
        if (selection)
            LISTVIEW_RemoveSelectionRange(infoPtr,selection->lower,
                                          selection->upper);
      }
      while (infoPtr->hdpaSelectionRanges->nItemCount>0);

      precount = infoPtr->hdpaItems->nItemCount;
      topvisible = LISTVIEW_GetTopIndex(infoPtr) +
                   LISTVIEW_GetCountPerColumn(infoPtr) + 1;

      /* Grow the hdpaItems array if necessary */
      if (nItems > infoPtr->hdpaItems->nMaxCount)
          if (!DPA_SetPtr(infoPtr->hdpaItems, nItems - 1, NULL))
              return FALSE;

      infoPtr->nItemWidth = max(LISTVIEW_GetItemWidth(infoPtr),
                                DEFAULT_COLUMN_WIDTH);

      LISTVIEW_UpdateSize(infoPtr);
      LISTVIEW_UpdateScroll(infoPtr);

      if (min(precount,infoPtr->hdpaItems->nItemCount)<topvisible)
        LISTVIEW_InvalidateList(infoPtr);
  }
  else
  {
    /* According to MSDN for non-LVS_OWNERDATA this is just
     * a performance issue. The control allocates its internal
     * data structures for the number of items specified. It
     * cuts down on the number of memory allocations. Therefore
     * we will just issue a WARN here
     */
     WARN("for non-ownerdata performance option not implemented.\n");
  }

  return TRUE;
}

/***
 * DESCRIPTION:
 * Sets the position of an item.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT : item index
 * [I] LONG : x coordinate
 * [I] LONG : y coordinate
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetItemPosition(LISTVIEW_INFO *infoPtr, INT nItem,
                                     LONG nPosX, LONG nPosY)
{
  UINT lStyle = infoPtr->dwStyle;
  UINT uView = lStyle & LVS_TYPEMASK;
  LISTVIEW_ITEM *lpItem;
  HDPA hdpaSubItems;
  BOOL bResult = FALSE;

  TRACE("(nItem=%d, X=%ld, Y=%ld)\n", nItem, nPosX, nPosY);

  if (lStyle & LVS_OWNERDATA)
    return FALSE;

  if ((nItem >= 0) || (nItem < GETITEMCOUNT(infoPtr)))
  {
    if ((uView == LVS_ICON) || (uView == LVS_SMALLICON))
    {
      if ( (hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, nItem)) )
      {
        if ( (lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0)) )
        {
	  POINT orig;
          bResult = TRUE;
	  orig = lpItem->ptPosition;
          if ((nPosX == -1) && (nPosY == -1))
          {
            /* This point value seems to be an undocumented feature. The
             * best guess is that it means either at the origin, or at
             * the true beginning of the list. I will assume the origin.
             */
            POINT pt1;
            if (!LISTVIEW_GetOrigin(infoPtr, &pt1))
            {
              pt1.x = 0;
              pt1.y = 0;
            }
            nPosX = pt1.x;
            nPosY = pt1.y;
            if (uView == LVS_ICON)
            {
              nPosX += (infoPtr->iconSpacing.cx - infoPtr->iconSize.cx) / 2;
              nPosY += ICON_TOP_PADDING;
            }
            TRACE("requested special (-1,-1), set to origin (%ld,%ld)\n",
                  nPosX, nPosY);
          }

          lpItem->ptPosition.x = nPosX;
          lpItem->ptPosition.y = nPosY;
	  if (uView == LVS_ICON)
	  {
	    lpItem->ptPosition.y -= ICON_TOP_PADDING;
              lpItem->ptPosition.x -= (infoPtr->iconSpacing.cx - infoPtr->iconSize.cx) / 2;
              if ((lpItem->ptPosition.y < 0) || (lpItem->ptPosition.x < 0))
              {
                  FIXME("failed orig (%ld,%ld), intent (%ld,%ld), is (%ld, %ld), setting neg to 0\n",
                        orig.x, orig.y, nPosX, nPosY, lpItem->ptPosition.x, lpItem->ptPosition.y);

                  /*
                  if (lpItem->ptPosition.x < 0) lpItem->ptPosition.x = 0;
                  if (lpItem->ptPosition.y < 0) lpItem->ptPosition.y = 0;
                  */
              }
              else
              {
                  TRACE("orig (%ld,%ld), intent (%ld,%ld), is (%ld,%ld)\n",
                        orig.x, orig.y, nPosX, nPosY, lpItem->ptPosition.x, lpItem->ptPosition.y);
              }
	  }
        }
      }
    }
  }

  return bResult;
}

/***
 * DESCRIPTION:
 * Sets the state of one or many items.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I]INT : item index
 * [I] LPLVITEM : item or subitem info
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_SetItemState(LISTVIEW_INFO *infoPtr, INT nItem, LPLVITEMW lpLVItem)
{
  BOOL bResult = TRUE;
  LVITEMW lvItem;

  TRACE("(nItem=%d, state=%x, stateMask=%x)\n", nItem, lpLVItem->state,
        lpLVItem->stateMask);

  lvItem.iItem = nItem;
  lvItem.iSubItem = 0;
  lvItem.mask = LVIF_STATE;
  lvItem.state = lpLVItem->state;
  lvItem.stateMask = lpLVItem->stateMask;

  if (nItem == -1)
  {
    /* apply to all items */
    for (lvItem.iItem = 0; lvItem.iItem < GETITEMCOUNT(infoPtr); lvItem.iItem++)
      if (!LISTVIEW_SetItemT(infoPtr, &lvItem, TRUE)) bResult = FALSE;
  }
  else
    bResult = LISTVIEW_SetItemT(infoPtr, &lvItem, TRUE);

  return bResult;
}

/***
 * DESCRIPTION:
 * Sets the text of an item or subitem.
 *
 * PARAMETER(S):
 * [I] hwnd : window handle
 * [I] nItem : item index
 * [I] lpLVItem : item or subitem info
 * [I] isW : TRUE if input is Unicode
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_SetItemTextT(LISTVIEW_INFO *infoPtr, INT nItem, LPLVITEMW lpLVItem, BOOL isW)
{
    LVITEMW lvItem;

    TRACE("(nItem=%d, lpLVItem=%s, isW=%d)\n", nItem, debuglvitem_t(lpLVItem, isW), isW);

    if ((nItem < 0) && (nItem >= GETITEMCOUNT(infoPtr))) return FALSE;
    
    lvItem.iItem = nItem;
    lvItem.iSubItem = lpLVItem->iSubItem;
    lvItem.mask = LVIF_TEXT;
    lvItem.pszText = lpLVItem->pszText;
    
    return LISTVIEW_SetItemT(infoPtr, &lvItem, isW); 
}

/***
 * DESCRIPTION:
 * Set item index that marks the start of a multiple selection.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT  : index
 *
 * RETURN:
 * Index number or -1 if there is no selection mark.
 */
static LRESULT LISTVIEW_SetSelectionMark(LISTVIEW_INFO *infoPtr, INT nIndex)
{
  INT nOldIndex = infoPtr->nSelectionMark;

  TRACE("(nIndex=%d)\n", nIndex);

  infoPtr->nSelectionMark = nIndex;

  return nOldIndex;
}

/***
 * DESCRIPTION:
 * Sets the text background color.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] COLORREF : text background color
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_SetTextBkColor(LISTVIEW_INFO *infoPtr, COLORREF clrTextBk)
{
    TRACE("(clrTextBk=%lx)\n", clrTextBk);

    if (infoPtr->clrTextBk != clrTextBk)
    {
	infoPtr->clrTextBk = clrTextBk;
	LISTVIEW_InvalidateList(infoPtr);
    }
    
  return TRUE;
}

/***
 * DESCRIPTION:
 * Sets the text foreground color.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] COLORREF : text color
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_SetTextColor (LISTVIEW_INFO *infoPtr, COLORREF clrText)
{
    TRACE("(clrText=%lx)\n", clrText);

    if (infoPtr->clrText != clrText)
    {
	infoPtr->clrText = clrText;
	LISTVIEW_InvalidateList(infoPtr);
    }

    return TRUE;
}

/* LISTVIEW_SetToolTips */
/* LISTVIEW_SetUnicodeFormat */
/* LISTVIEW_SetWorkAreas */

/***
 * DESCRIPTION:
 * Callback internally used by LISTVIEW_SortItems()
 *
 * PARAMETER(S):
 * [I] LPVOID : first LISTVIEW_ITEM to compare
 * [I] LPVOID : second LISTVIEW_ITEM to compare
 * [I] LPARAM : HWND of control
 *
 * RETURN:
 *   if first comes before second : negative
 *   if first comes after second : positive
 *   if first and second are equivalent : zero
 */
static INT WINAPI LISTVIEW_CallBackCompare(LPVOID first, LPVOID second, LPARAM lParam)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongW((HWND)lParam, 0);
  LISTVIEW_ITEM* lv_first = (LISTVIEW_ITEM*) DPA_GetPtr( (HDPA)first, 0 );
  LISTVIEW_ITEM* lv_second = (LISTVIEW_ITEM*) DPA_GetPtr( (HDPA)second, 0 );

  /* Forward the call to the client defined callback */
  return (infoPtr->pfnCompare)( lv_first->lParam , lv_second->lParam, infoPtr->lParamSort );
}

/***
 * DESCRIPTION:
 * Sorts the listview items.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] WPARAM : application-defined value
 * [I] LPARAM : pointer to comparision callback
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static LRESULT LISTVIEW_SortItems(LISTVIEW_INFO *infoPtr, PFNLVCOMPARE pfnCompare, LPARAM lParamSort)
{
    UINT lStyle = infoPtr->dwStyle;
    HDPA hdpaSubItems;
    LISTVIEW_ITEM *lpItem;
    LPVOID selectionMarkItem;
    int i;

    TRACE("(pfnCompare=%p, lParamSort=%lx)\n", pfnCompare, lParamSort);

    if (lStyle & LVS_OWNERDATA) return FALSE;

    if (!infoPtr->hdpaItems) return FALSE;

    /* if there are 0 or 1 items, there is no need to sort */
    if (GETITEMCOUNT(infoPtr) < 2) return TRUE;

    if (infoPtr->nFocusedItem >= 0)
    {
	hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, infoPtr->nFocusedItem);
	lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0);
	if (lpItem) lpItem->state |= LVIS_FOCUSED;
    }
    
    infoPtr->pfnCompare = pfnCompare;
    infoPtr->lParamSort = lParamSort;
    DPA_Sort(infoPtr->hdpaItems, LISTVIEW_CallBackCompare, (LPARAM)infoPtr->hwndSelf);

    /* Adjust selections and indices so that they are the way they should
     * be after the sort (otherwise, the list items move around, but
     * whatever is at the item's previous original position will be
     * selected instead)
     */
    selectionMarkItem=(infoPtr->nSelectionMark>=0)?DPA_GetPtr(infoPtr->hdpaItems, infoPtr->nSelectionMark):NULL;
    for (i=0; i < GETITEMCOUNT(infoPtr); i++)
    {
	hdpaSubItems = (HDPA)DPA_GetPtr(infoPtr->hdpaItems, i);
	lpItem = (LISTVIEW_ITEM *)DPA_GetPtr(hdpaSubItems, 0);

	if (lpItem->state & LVIS_SELECTED)
	    LISTVIEW_AddSelectionRange(infoPtr, i, i);
	else
	    LISTVIEW_RemoveSelectionRange(infoPtr, i, i);
	if (lpItem->state & LVIS_FOCUSED)
	{
            infoPtr->nFocusedItem = i;
	    lpItem->state &= ~LVIS_FOCUSED;
	}
    }
    if (selectionMarkItem != NULL)
	infoPtr->nSelectionMark = DPA_GetPtrIndex(infoPtr->hdpaItems, selectionMarkItem);
    /* I believe nHotItem should be left alone, see LISTVIEW_ShiftIndices */

    /* align the items */
    LISTVIEW_AlignTop(infoPtr);

    /* refresh the display */
    LISTVIEW_InvalidateList(infoPtr);

    return TRUE;
}

/***
 * DESCRIPTION:
 * Updates an items or rearranges the listview control.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT : item index
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static BOOL LISTVIEW_Update(LISTVIEW_INFO *infoPtr, INT nItem)
{
    LONG lStyle = infoPtr->dwStyle;
    UINT uView = lStyle & LVS_TYPEMASK;

    TRACE("(nItem=%d)\n", nItem);

    if ((nItem < 0) && (nItem >= GETITEMCOUNT(infoPtr))) return FALSE;

    /* rearrange with default alignment style */
    if ((lStyle & LVS_AUTOARRANGE) && ((uView == LVS_ICON) ||(uView == LVS_SMALLICON)))
	LISTVIEW_Arrange(infoPtr, 0);
    else
	LISTVIEW_InvalidateItem(infoPtr, nItem);

    return TRUE;
}

	
/***
 * DESCRIPTION:
 * Creates the listview control.
 *
 * PARAMETER(S):
 * [I] hwnd : window handle
 * [I] lpcs : the create parameters
 *
 * RETURN:
 *   Success: 0
 *   Failure: -1
 */
static LRESULT LISTVIEW_Create(HWND hwnd, LPCREATESTRUCTW lpcs)
{
  LISTVIEW_INFO *infoPtr;
  UINT uView = lpcs->style & LVS_TYPEMASK;
  LOGFONTW logFont;

  TRACE("(lpcs=%p)\n", lpcs);

  /* initialize info pointer */
  infoPtr = (LISTVIEW_INFO *)COMCTL32_Alloc(sizeof(LISTVIEW_INFO));
  if (!infoPtr) return -1;

  SetWindowLongW(hwnd, 0, (LONG)infoPtr);

  infoPtr->hwndSelf = hwnd;
  infoPtr->dwStyle = lpcs->style;
  /* determine the type of structures to use */
  infoPtr->notifyFormat = SendMessageW(GetParent(infoPtr->hwndSelf), WM_NOTIFYFORMAT,
                                       (WPARAM)infoPtr->hwndSelf, (LPARAM)NF_QUERY);

  /* initialize color information  */
  infoPtr->clrBk = CLR_NONE;
  infoPtr->clrText = comctl32_color.clrWindowText;
  infoPtr->clrTextBk = CLR_DEFAULT;
  LISTVIEW_SetBkColor(infoPtr, comctl32_color.clrWindow);

  /* set default values */
  infoPtr->nFocusedItem = -1;
  infoPtr->nSelectionMark = -1;
  infoPtr->nHotItem = -1;
  infoPtr->iconSpacing.cx = GetSystemMetrics(SM_CXICONSPACING);
  infoPtr->iconSpacing.cy = GetSystemMetrics(SM_CYICONSPACING);
  infoPtr->nEditLabelItem = -1;

  /* get default font (icon title) */
  SystemParametersInfoW(SPI_GETICONTITLELOGFONT, 0, &logFont, 0);
  infoPtr->hDefaultFont = CreateFontIndirectW(&logFont);
  infoPtr->hFont = infoPtr->hDefaultFont;
  LISTVIEW_SaveTextMetrics(infoPtr);

  /* create header */
  infoPtr->hwndHeader =	CreateWindowW(WC_HEADERW, (LPCWSTR)NULL,
    WS_CHILD | HDS_HORZ | (DWORD)((LVS_NOSORTHEADER & lpcs->style)?0:HDS_BUTTONS),
    0, 0, 0, 0, hwnd, (HMENU)0,
    lpcs->hInstance, NULL);

  /* set header unicode format */
  SendMessageW(infoPtr->hwndHeader, HDM_SETUNICODEFORMAT,(WPARAM)TRUE,(LPARAM)NULL);

  /* set header font */
  SendMessageW(infoPtr->hwndHeader, WM_SETFONT, (WPARAM)infoPtr->hFont,
               (LPARAM)TRUE);

  if (uView == LVS_ICON)
  {
    infoPtr->iconSize.cx = GetSystemMetrics(SM_CXICON);
    infoPtr->iconSize.cy = GetSystemMetrics(SM_CYICON);
  }
  else if (uView == LVS_REPORT)
  {
    if (!(LVS_NOCOLUMNHEADER & lpcs->style))
    {
      ShowWindow(infoPtr->hwndHeader, SW_SHOWNORMAL);
    }
    else
    {
      /* set HDS_HIDDEN flag to hide the header bar */
      SetWindowLongW(infoPtr->hwndHeader, GWL_STYLE,
                    GetWindowLongW(infoPtr->hwndHeader, GWL_STYLE) | HDS_HIDDEN);
    }


    infoPtr->iconSize.cx = GetSystemMetrics(SM_CXSMICON);
    infoPtr->iconSize.cy = GetSystemMetrics(SM_CYSMICON);
  }
  else
  {
    infoPtr->iconSize.cx = GetSystemMetrics(SM_CXSMICON);
    infoPtr->iconSize.cy = GetSystemMetrics(SM_CYSMICON);
  }

  infoPtr->iconStateSize.cx = GetSystemMetrics(SM_CXSMICON);
  infoPtr->iconStateSize.cy = GetSystemMetrics(SM_CYSMICON);

  /* display unsupported listview window styles */
  LISTVIEW_UnsupportedStyles(lpcs->style);

  /* allocate memory for the data structure */
  infoPtr->hdpaItems = DPA_Create(10);

  /* allocate memory for the selection ranges */
  infoPtr->hdpaSelectionRanges = DPA_Create(10);

  /* initialize size of items */
  infoPtr->nItemWidth = LISTVIEW_GetItemWidth(infoPtr);
  infoPtr->nItemHeight = LISTVIEW_GetItemHeight(infoPtr);

  /* initialize the hover time to -1(indicating the default system hover time) */
  infoPtr->dwHoverTime = -1;

  return 0;
}

/***
 * DESCRIPTION:
 * Erases the background of the listview control.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] hdc : device context handle
 *
 * RETURN:
 *   SUCCESS : TRUE
 *   FAILURE : FALSE
 */
static inline BOOL LISTVIEW_EraseBkgnd(LISTVIEW_INFO *infoPtr, HDC hdc)
{
    RECT rc;

    TRACE("(hdc=%x)\n", hdc);

    if (!GetClipBox(hdc, &rc)) return FALSE;

    return LISTVIEW_FillBkgnd(infoPtr, hdc, &rc);
}
	

/***
 * DESCRIPTION:
 * Helper function for LISTVIEW_[HV]Scroll *only*.
 * Performs vertical/horizontal scrolling by a give amount.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] dx : amount of horizontal scroll
 * [I] dy : amount of vertical scroll
 */
static void scroll_list(LISTVIEW_INFO *infoPtr, INT dx, INT dy)
{
    /* now we can scroll the list */
    ScrollWindowEx(infoPtr->hwndSelf, dx, dy, &infoPtr->rcList, 
		   &infoPtr->rcList, 0, 0, SW_ERASE | SW_INVALIDATE);
    /* if we have focus, adjust rect */
    if (infoPtr->bFocus && !IsRectEmpty(&infoPtr->rcFocus))
	OffsetRect(&infoPtr->rcFocus, dx, dy);
    UpdateWindow(infoPtr->hwndSelf);
}

/***
 * DESCRIPTION:
 * Performs vertical scrolling.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nScrollCode : scroll code
 * [I] nScrollDiff : units to scroll in SB_INTERNAL mode, 0 otherwise
 * [I] hScrollWnd  : scrollbar control window handle
 *
 * RETURN:
 * Zero
 *
 * NOTES:
 *   SB_LINEUP/SB_LINEDOWN:
 *        for LVS_ICON, LVS_SMALLICON is 37 by experiment
 *        for LVS_REPORT is 1 line
 *        for LVS_LIST cannot occur
 *
 */
static LRESULT LISTVIEW_VScroll(LISTVIEW_INFO *infoPtr, INT nScrollCode, 
				INT nScrollDiff, HWND hScrollWnd)
{
    UINT uView = LISTVIEW_GetType(infoPtr);
    INT nOldScrollPos, nNewScrollPos;
    SCROLLINFO scrollInfo;
    BOOL is_an_icon;

    TRACE("(nScrollCode=%d, nScrollDiff=%d)\n", nScrollCode, nScrollDiff);

    SendMessageW(infoPtr->hwndEdit, WM_KILLFOCUS, 0, 0);

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;

    is_an_icon = ((uView == LVS_ICON) || (uView == LVS_SMALLICON));

    if (!GetScrollInfo(infoPtr->hwndSelf, SB_VERT, &scrollInfo)) return 1;

    nOldScrollPos = scrollInfo.nPos;
    switch (nScrollCode)
    {
    case SB_INTERNAL:
        break;

    case SB_LINEUP:
	nScrollDiff = (is_an_icon) ? -LISTVIEW_SCROLL_ICON_LINE_SIZE : -1;
        break;

    case SB_LINEDOWN:
	nScrollDiff = (is_an_icon) ? LISTVIEW_SCROLL_ICON_LINE_SIZE : 1;
        break;

    case SB_PAGEUP:
	nScrollDiff = -scrollInfo.nPage;
        break;

    case SB_PAGEDOWN:
	nScrollDiff = scrollInfo.nPage;
        break;

    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
	nScrollDiff = scrollInfo.nTrackPos - scrollInfo.nPos;
        break;

    default:
	nScrollDiff = 0;
    }

    /* quit right away if pos isn't changing */
    if (nScrollDiff == 0) return 0;
    
    /* calculate new position, and handle overflows */
    nNewScrollPos = scrollInfo.nPos + nScrollDiff;
    if (nScrollDiff > 0) {
	if (nNewScrollPos < nOldScrollPos ||
	    nNewScrollPos > scrollInfo.nMax)
	    nNewScrollPos = scrollInfo.nMax;
    } else {
	if (nNewScrollPos > nOldScrollPos ||
	    nNewScrollPos < scrollInfo.nMin)
	    nNewScrollPos = scrollInfo.nMin;
    }

    /* set the new position, and reread in case it changed */
    scrollInfo.fMask = SIF_POS;
    scrollInfo.nPos = nNewScrollPos;
    nNewScrollPos = SetScrollInfo(infoPtr->hwndSelf, SB_VERT, &scrollInfo, TRUE);
    
    /* carry on only if it really changed */
    if (nNewScrollPos == nOldScrollPos) return 0;
    
    /* now adjust to client coordinates */
    nScrollDiff = nOldScrollPos - nNewScrollPos;
    if (uView == LVS_REPORT) nScrollDiff *= infoPtr->nItemHeight;
   
    /* and scroll the window */ 
    scroll_list(infoPtr, 0, nScrollDiff);

    return 0;
}

/***
 * DESCRIPTION:
 * Performs horizontal scrolling.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] nScrollCode : scroll code
 * [I] nScrollDiff : units to scroll in SB_INTERNAL mode, 0 otherwise
 * [I] hScrollWnd  : scrollbar control window handle
 *
 * RETURN:
 * Zero
 *
 * NOTES:
 *   SB_LINELEFT/SB_LINERIGHT:
 *        for LVS_ICON, LVS_SMALLICON  1 pixel
 *        for LVS_REPORT is 1 pixel
 *        for LVS_LIST  is 1 column --> which is a 1 because the
 *                                      scroll is based on columns not pixels
 *
 */
static LRESULT LISTVIEW_HScroll(LISTVIEW_INFO *infoPtr, INT nScrollCode,
                                INT nScrollDiff, HWND hScrollWnd)
{
    UINT uView = LISTVIEW_GetType(infoPtr);
    INT nOldScrollPos, nNewScrollPos;
    SCROLLINFO scrollInfo;

    TRACE("(nScrollCode=%d, nScrollDiff=%d)\n", nScrollCode, nScrollDiff);

    SendMessageW(infoPtr->hwndEdit, WM_KILLFOCUS, 0, 0);

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;

    if (!GetScrollInfo(infoPtr->hwndSelf, SB_HORZ, &scrollInfo)) return 1;

    nOldScrollPos = scrollInfo.nPos;
   
    switch (nScrollCode)
    {
    case SB_INTERNAL:
        break;

    case SB_LINELEFT:
	nScrollDiff = -1;
        break;

    case SB_LINERIGHT:
	nScrollDiff = 1;
        break;

    case SB_PAGELEFT:
	nScrollDiff = -scrollInfo.nPage;
        break;

    case SB_PAGERIGHT:
	nScrollDiff = scrollInfo.nPage;
        break;

    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
	nScrollDiff = scrollInfo.nTrackPos - scrollInfo.nPos;
	break;

    default:
	nScrollDiff = 0;
    }

    /* quit right away if pos isn't changing */
    if (nScrollDiff == 0) return 0;
    
    /* calculate new position, and handle overflows */
    nNewScrollPos = scrollInfo.nPos + nScrollDiff;
    if (nScrollDiff > 0) {
	if (nNewScrollPos < nOldScrollPos ||
	    nNewScrollPos > scrollInfo.nMax)
	    nNewScrollPos = scrollInfo.nMax;
    } else {
	if (nNewScrollPos > nOldScrollPos ||
	    nNewScrollPos < scrollInfo.nMin)
	    nNewScrollPos = scrollInfo.nMin;
    }

    /* set the new position, and reread in case it changed */
    scrollInfo.fMask = SIF_POS;
    scrollInfo.nPos = nNewScrollPos;
    nNewScrollPos = SetScrollInfo(infoPtr->hwndSelf, SB_HORZ, &scrollInfo, TRUE);
    
    /* carry on only if it really changed */
    if (nNewScrollPos == nOldScrollPos) return 0;
    
    if(uView == LVS_REPORT)
        LISTVIEW_UpdateHeaderSize(infoPtr, nNewScrollPos);
      
    /* now adjust to client coordinates */
    nScrollDiff = nOldScrollPos - nNewScrollPos;
    if (uView == LVS_LIST) nScrollDiff *= infoPtr->nItemWidth;
   
    /* and scroll the window */
    scroll_list(infoPtr, nScrollDiff, 0);

  return 0;
}

static LRESULT LISTVIEW_MouseWheel(LISTVIEW_INFO *infoPtr, INT wheelDelta)
{
    UINT uView = LISTVIEW_GetType(infoPtr);
    INT gcWheelDelta = 0;
    UINT pulScrollLines = 3;
    SCROLLINFO scrollInfo;

    TRACE("(wheelDelta=%d)\n", wheelDelta);

    SystemParametersInfoW(SPI_GETWHEELSCROLLLINES,0, &pulScrollLines, 0);
    gcWheelDelta -= wheelDelta;

    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_POS;

    switch(uView)
    {
    case LVS_ICON:
    case LVS_SMALLICON:
       /*
        *  listview should be scrolled by a multiple of 37 dependently on its dimension or its visible item number
        *  should be fixed in the future.
        */
        if (GetScrollInfo(infoPtr->hwndSelf, SB_VERT, &scrollInfo))
            LISTVIEW_VScroll(infoPtr, SB_THUMBPOSITION,
			     scrollInfo.nPos + (gcWheelDelta < 0) ?
			     LISTVIEW_SCROLL_ICON_LINE_SIZE :
			     -LISTVIEW_SCROLL_ICON_LINE_SIZE, 0);
        break;

    case LVS_REPORT:
        if (abs(gcWheelDelta) >= WHEEL_DELTA && pulScrollLines)
        {
            if (GetScrollInfo(infoPtr->hwndSelf, SB_VERT, &scrollInfo))
            {
                int cLineScroll = min(LISTVIEW_GetCountPerColumn(infoPtr), pulScrollLines);
                cLineScroll *= (gcWheelDelta / WHEEL_DELTA);
                LISTVIEW_VScroll(infoPtr, SB_THUMBPOSITION, scrollInfo.nPos + cLineScroll, 0);
            }
        }
        break;

    case LVS_LIST:
        LISTVIEW_HScroll(infoPtr, (gcWheelDelta < 0) ? SB_LINELEFT : SB_LINERIGHT, 0, 0);
        break;
    }
    return 0;
}

/***
 * DESCRIPTION:
 * ???
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT : virtual key
 * [I] LONG : key data
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_KeyDown(LISTVIEW_INFO *infoPtr, INT nVirtualKey, LONG lKeyData)
{
  UINT uView =  LISTVIEW_GetType(infoPtr);
  INT nItem = -1;
  NMLVKEYDOWN nmKeyDown;

  TRACE("(nVirtualKey=%d, lKeyData=%ld)\n", nVirtualKey, lKeyData);

  /* send LVN_KEYDOWN notification */
  nmKeyDown.wVKey = nVirtualKey;
  nmKeyDown.flags = 0;
  notify(infoPtr, LVN_KEYDOWN, &nmKeyDown.hdr);

  switch (nVirtualKey)
  {
  case VK_RETURN:
    if ((GETITEMCOUNT(infoPtr) > 0) && (infoPtr->nFocusedItem != -1))
    {
      notify_return(infoPtr);
      notify_itemactivate(infoPtr);
    }
    break;

  case VK_HOME:
    if (GETITEMCOUNT(infoPtr) > 0)
      nItem = 0;
    break;

  case VK_END:
    if (GETITEMCOUNT(infoPtr) > 0)
      nItem = GETITEMCOUNT(infoPtr) - 1;
    break;

  case VK_LEFT:
    nItem = ListView_GetNextItem(infoPtr->hwndSelf, infoPtr->nFocusedItem, LVNI_TOLEFT);
    break;

  case VK_UP:
    nItem = ListView_GetNextItem(infoPtr->hwndSelf, infoPtr->nFocusedItem, LVNI_ABOVE);
    break;

  case VK_RIGHT:
    nItem = ListView_GetNextItem(infoPtr->hwndSelf, infoPtr->nFocusedItem, LVNI_TORIGHT);
    break;

  case VK_DOWN:
    nItem = ListView_GetNextItem(infoPtr->hwndSelf, infoPtr->nFocusedItem, LVNI_BELOW);
    break;

  case VK_PRIOR:
    if (uView == LVS_REPORT)
      nItem = infoPtr->nFocusedItem - LISTVIEW_GetCountPerColumn(infoPtr);
    else
      nItem = infoPtr->nFocusedItem - LISTVIEW_GetCountPerColumn(infoPtr)
                                    * LISTVIEW_GetCountPerRow(infoPtr);
    if(nItem < 0) nItem = 0;
    break;

  case VK_NEXT:
    if (uView == LVS_REPORT)
      nItem = infoPtr->nFocusedItem + LISTVIEW_GetCountPerColumn(infoPtr);
    else
      nItem = infoPtr->nFocusedItem + LISTVIEW_GetCountPerColumn(infoPtr)
                                    * LISTVIEW_GetCountPerRow(infoPtr);
    if(nItem >= GETITEMCOUNT(infoPtr)) nItem = GETITEMCOUNT(infoPtr) - 1;
    break;
  }

  if ((nItem != -1) && (nItem != infoPtr->nFocusedItem))
      LISTVIEW_KeySelection(infoPtr, nItem);

  return 0;
}

/***
 * DESCRIPTION:
 * Kills the focus.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_KillFocus(LISTVIEW_INFO *infoPtr)
{
    TRACE("()\n");

    /* if we did not have the focus, there's nothing to do */
    if (!infoPtr->bFocus) return 0;
   
    /* send NM_KILLFOCUS notification */
    notify_killfocus(infoPtr);

    /* if we have a focus rectagle, get rid of it */
    LISTVIEW_ToggleFocusRect(infoPtr);
    
    /* invalidate the selected items before reseting focus flag */
    LISTVIEW_InvalidateSelectedItems(infoPtr);
    
    /* set window focus flag */
    infoPtr->bFocus = FALSE;

    return 0;
}

/***
 * DESCRIPTION:
 * Processes double click messages (left mouse button).
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] wKey : key flag
 * [I] pts : mouse coordinate
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_LButtonDblClk(LISTVIEW_INFO *infoPtr, WORD wKey, POINTS pts)
{
    LVHITTESTINFO htInfo;
    NMLISTVIEW nmlv;

    TRACE("(key=%hu, X=%hu, Y=%hu)\n", wKey, pts.x, pts.y);

    htInfo.pt.x = pts.x;
    htInfo.pt.y = pts.y;

    /* send NM_DBLCLK notification */
    ZeroMemory(&nmlv, sizeof(NMLISTVIEW));
    LISTVIEW_HitTest(infoPtr, &htInfo, TRUE);
    nmlv.iItem = htInfo.iItem;
    nmlv.iSubItem = htInfo.iSubItem;
    nmlv.ptAction = htInfo.pt;
    notify_listview(infoPtr, NM_DBLCLK, &nmlv);

    /* To send the LVN_ITEMACTIVATE, it must be on an Item */
    if(nmlv.iItem != -1)
        notify_itemactivate(infoPtr);

    return 0;
}

/***
 * DESCRIPTION:
 * Processes mouse down messages (left mouse button).
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] wKey : key flag
 * [I] pts : mouse coordinate
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_LButtonDown(LISTVIEW_INFO *infoPtr, WORD wKey, POINTS pts)
{
  LONG lStyle = infoPtr->dwStyle;
  static BOOL bGroupSelect = TRUE;
  POINT pt = { pts.x, pts.y };
  INT nItem;

  TRACE("(key=%hu, X=%hu, Y=%hu)\n", wKey, pts.x, pts.y);

  /* FIXME: NM_CLICK */
  
  /* send NM_RELEASEDCAPTURE notification */
  notify_releasedcapture(infoPtr);

  if (!infoPtr->bFocus) SetFocus(infoPtr->hwndSelf);

  /* set left button down flag */
  infoPtr->bLButtonDown = TRUE;

  nItem = LISTVIEW_GetItemAtPt(infoPtr, pt);
  if ((nItem >= 0) && (nItem < GETITEMCOUNT(infoPtr)))
  {
    if (lStyle & LVS_SINGLESEL)
    {
      if ((LISTVIEW_GetItemState(infoPtr, nItem, LVIS_SELECTED) & LVIS_SELECTED)
          && infoPtr->nEditLabelItem == -1)
          infoPtr->nEditLabelItem = nItem;
      else
        LISTVIEW_SetSelection(infoPtr, nItem);
    }
    else
    {
      if ((wKey & MK_CONTROL) && (wKey & MK_SHIFT))
      {
        if (bGroupSelect)
          LISTVIEW_AddGroupSelection(infoPtr, nItem);
        else
	{
          LVITEMW item;

	  item.state = LVIS_SELECTED;
	  item.stateMask = LVIS_SELECTED;

	  LISTVIEW_SetItemState(infoPtr,nItem,&item);

	  LISTVIEW_SetItemFocus(infoPtr, nItem);
	  infoPtr->nSelectionMark = nItem;
	}
      }
      else if (wKey & MK_CONTROL)
      {
        LVITEMW item;

	bGroupSelect = (LISTVIEW_GetItemState(infoPtr, nItem, LVIS_SELECTED) == 0);
	
	item.state = bGroupSelect ? LVIS_SELECTED : 0;
        item.stateMask = LVIS_SELECTED;
	LISTVIEW_SetItemState(infoPtr, nItem, &item);

        LISTVIEW_SetItemFocus(infoPtr, nItem);
        infoPtr->nSelectionMark = nItem;
      }
      else  if (wKey & MK_SHIFT)
      {
        LISTVIEW_SetGroupSelection(infoPtr, nItem);
      }
      else
      {
	BOOL was_selected =
	    (LISTVIEW_GetItemState(infoPtr, nItem, LVIS_SELECTED) & LVIS_SELECTED);

	/* set selection (clears other pre-existing selections) */
        LISTVIEW_SetSelection(infoPtr, nItem);

        if (was_selected && infoPtr->nEditLabelItem == -1)
          infoPtr->nEditLabelItem = nItem;
      }
    }
  }
  else
  {
    /* remove all selections */
    LISTVIEW_RemoveAllSelections(infoPtr);
  }

  return 0;
}

/***
 * DESCRIPTION:
 * Processes mouse up messages (left mouse button).
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] wKey : key flag
 * [I] pts : mouse coordinate
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_LButtonUp(LISTVIEW_INFO *infoPtr, WORD wKey, POINTS pts)
{
  TRACE("(key=%hu, X=%hu, Y=%hu)\n", wKey, pts.x, pts.y);

  if (infoPtr->bLButtonDown)
  {
    LVHITTESTINFO lvHitTestInfo;
    NMLISTVIEW nmlv;

    lvHitTestInfo.pt.x = pts.x;
    lvHitTestInfo.pt.y = pts.y;

  /* send NM_CLICK notification */
    ZeroMemory(&nmlv, sizeof(NMLISTVIEW));
    if (LISTVIEW_HitTest(infoPtr, &lvHitTestInfo, TRUE) != -1)
    {
        nmlv.iItem = lvHitTestInfo.iItem;
        nmlv.iSubItem = lvHitTestInfo.iSubItem;
    }
    else
    {
        nmlv.iItem = -1;
        nmlv.iSubItem = 0;
    }
    nmlv.ptAction.x = pts.x;
    nmlv.ptAction.y = pts.y;
    notify_listview(infoPtr, NM_CLICK, &nmlv);

    /* set left button flag */
    infoPtr->bLButtonDown = FALSE;

    if(infoPtr->nEditLabelItem != -1)
    {
      if(lvHitTestInfo.iItem == infoPtr->nEditLabelItem && lvHitTestInfo.flags & LVHT_ONITEMLABEL) {
        LISTVIEW_EditLabelT(infoPtr, lvHitTestInfo.iItem, TRUE);
      }
      infoPtr->nEditLabelItem = -1;
    }
  }

  return 0;
}

/***
 * DESCRIPTION:
 * Destroys the listview control (called after WM_DESTROY).
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_NCDestroy(LISTVIEW_INFO *infoPtr)
{
  LONG lStyle = infoPtr->dwStyle;

  TRACE("()\n");

  /* delete all items */
  LISTVIEW_DeleteAllItems(infoPtr);

  /* destroy data structure */
  DPA_Destroy(infoPtr->hdpaItems);
  DPA_Destroy(infoPtr->hdpaSelectionRanges);

  /* destroy image lists */
  if (!(lStyle & LVS_SHAREIMAGELISTS))
  {
      /* FIXME: If the caller does a ImageList_Destroy and then we
       *        do this code the area will be freed twice. Currently
       *        this generates an "err:heap:HEAP_ValidateInUseArena
       *        Heap xxxxxxxx: in-use arena yyyyyyyy next block
       *        has PREV_FREE flag" sometimes.
       *
       *        We will leak the memory till we figure out how to fix
       */
      if (infoPtr->himlNormal)
	  ImageList_Destroy(infoPtr->himlNormal);
      if (infoPtr->himlSmall)
	  ImageList_Destroy(infoPtr->himlSmall);
      if (infoPtr->himlState)
	  ImageList_Destroy(infoPtr->himlState);
  }

  /* destroy font, bkgnd brush */
  infoPtr->hFont = 0;
  if (infoPtr->hDefaultFont) DeleteObject(infoPtr->hDefaultFont);
  if (infoPtr->clrBk != CLR_NONE) DeleteObject(infoPtr->hBkBrush);

  /* free listview info pointer*/
  COMCTL32_Free(infoPtr);

  SetWindowLongW(infoPtr->hwndSelf, 0, 0);
  return 0;
}

/***
 * DESCRIPTION:
 * Handles notifications from children.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] INT : control identifier
 * [I] LPNMHDR : notification information
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_Notify(LISTVIEW_INFO *infoPtr, INT nCtrlId, LPNMHDR lpnmh)
{
  TRACE("(nCtrlId=%d, lpnmh=%p)\n", nCtrlId, lpnmh);

  if (lpnmh->hwndFrom == infoPtr->hwndHeader)
  {
    /* handle notification from header control */
    if (lpnmh->code == HDN_ENDTRACKW)
    {
      infoPtr->nItemWidth = LISTVIEW_GetItemWidth(infoPtr);
      LISTVIEW_InvalidateList(infoPtr);
    }
    else if(lpnmh->code ==  HDN_ITEMCLICKW || lpnmh->code ==  HDN_ITEMCLICKA)
    {
        /* Handle sorting by Header Column */
        NMLISTVIEW nmlv;

        ZeroMemory(&nmlv, sizeof(NMLISTVIEW));
        nmlv.iItem = -1;
        nmlv.iSubItem = ((LPNMHEADERW)lpnmh)->iItem;
        notify_listview(infoPtr, LVN_COLUMNCLICK, &nmlv);
    }
    else if(lpnmh->code == NM_RELEASEDCAPTURE)
    {
      /* Idealy this should be done in HDN_ENDTRACKA
       * but since SetItemBounds in Header.c is called after
       * the notification is sent, it is neccessary to handle the
       * update of the scroll bar here (Header.c works fine as it is,
       * no need to disturb it)
       */
      infoPtr->nItemWidth = LISTVIEW_GetItemWidth(infoPtr);
      LISTVIEW_UpdateScroll(infoPtr);
      LISTVIEW_InvalidateList(infoPtr);
    }

  }

  return 0;
}

/***
 * DESCRIPTION:
 * Determines the type of structure to use.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structureof the sender
 * [I] HWND : listview window handle
 * [I] INT : command specifying the nature of the WM_NOTIFYFORMAT
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_NotifyFormat(LISTVIEW_INFO *infoPtr, HWND hwndFrom, INT nCommand)
{
  TRACE("(hwndFrom=%x, nCommand=%d)\n", hwndFrom, nCommand);

  if (nCommand == NF_REQUERY)
    infoPtr->notifyFormat = SendMessageW(hwndFrom, WM_NOTIFYFORMAT,
                                         (WPARAM)infoPtr->hwndSelf, (LPARAM)NF_QUERY);
  return 0;
}

/***
 * DESCRIPTION:
 * Paints/Repaints the listview control.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] HDC : device context handle
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_Paint(LISTVIEW_INFO *infoPtr, HDC hdc)
{
    TRACE("(hdc=%x)\n", hdc);

    if (hdc) 
	LISTVIEW_Refresh(infoPtr, hdc);
    else
    {
	PAINTSTRUCT ps;

	hdc = BeginPaint(infoPtr->hwndSelf, &ps);
	if (!hdc) return 1;
	if (ps.fErase) LISTVIEW_FillBkgnd(infoPtr, hdc, &ps.rcPaint);
	LISTVIEW_Refresh(infoPtr, hdc);
	EndPaint(infoPtr->hwndSelf, &ps);
    }

    return 0;
}

/***
 * DESCRIPTION:
 * Processes double click messages (right mouse button).
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] wKey : key flag
 * [I] pts : mouse coordinate
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_RButtonDblClk(LISTVIEW_INFO *infoPtr, WORD wKey, POINTS pts)
{
    TRACE("(key=%hu,X=%hu,Y=%hu)\n", wKey, pts.x, pts.y);

    /* send NM_RELEASEDCAPTURE notification */
    notify_releasedcapture(infoPtr);

    /* send NM_RDBLCLK notification */
    notify_rdblclk(infoPtr);

    return 0;
}

/***
 * DESCRIPTION:
 * Processes mouse down messages (right mouse button).
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] wKey : key flag
 * [I] pts : mouse coordinate
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_RButtonDown(LISTVIEW_INFO *infoPtr, WORD wKey, POINTS pts)
{
    LVHITTESTINFO lvHitTestInfo;
    NMLISTVIEW nmlv;
    INT nItem;

    TRACE("(key=%hu,X=%hu,Y=%hu)\n", wKey, pts.x, pts.y);

    /* FIXME: NM_CLICK */
  
    /* send NM_RELEASEDCAPTURE notification */
    notify_releasedcapture(infoPtr);

    /* make sure the listview control window has the focus */
    if (!infoPtr->bFocus) SetFocus(infoPtr->hwndSelf);

    /* set right button down flag */
    infoPtr->bRButtonDown = TRUE;

    /* determine the index of the selected item */
    lvHitTestInfo.pt.x = pts.x;
    lvHitTestInfo.pt.y = pts.y;
    nItem = LISTVIEW_GetItemAtPt(infoPtr, lvHitTestInfo.pt);
  
    if ((nItem >= 0) && (nItem < GETITEMCOUNT(infoPtr)))
    {
	LISTVIEW_SetItemFocus(infoPtr,nItem);
	if (!((wKey & MK_SHIFT) || (wKey & MK_CONTROL)) &&
            !LISTVIEW_GetItemState(infoPtr, nItem, LVIS_SELECTED))
	    LISTVIEW_SetSelection(infoPtr, nItem);
    }
    else
    {
	LISTVIEW_RemoveAllSelections(infoPtr);
    }


    /* Send NM_RClICK notification */
    ZeroMemory(&nmlv, sizeof(nmlv));
    LISTVIEW_HitTest(infoPtr, &lvHitTestInfo, TRUE);
    nmlv.iItem = lvHitTestInfo.iItem;
    nmlv.iSubItem = lvHitTestInfo.iSubItem;
    nmlv.ptAction = lvHitTestInfo.pt;
    notify_listview(infoPtr, NM_RCLICK, &nmlv);

    return 0;
}

/***
 * DESCRIPTION:
 * Processes mouse up messages (right mouse button).
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] wKey : key flag
 * [I] pts : mouse coordinate
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_RButtonUp(LISTVIEW_INFO *infoPtr, WORD wKey, POINTS pts)
{
    POINT pt = { pts.x, pts.y };

    TRACE("(key=%hu,X=%hu,Y=%hu)\n", wKey, pts.x, pts.y);

    if (!infoPtr->bRButtonDown) return 0;
 
    /* set button flag */
    infoPtr->bRButtonDown = FALSE;

    /* Change to screen coordinate for WM_CONTEXTMENU */
    ClientToScreen(infoPtr->hwndSelf, &pt);

    /* Send a WM_CONTEXTMENU message in response to the RBUTTONUP */
    SendMessageW(infoPtr->hwndSelf, WM_CONTEXTMENU,
		 (WPARAM)infoPtr->hwndSelf, MAKELPARAM(pt.x, pt.y));

  return 0;
}


/***
 * DESCRIPTION:
 * Sets the cursor.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] hwnd : window handle of window containing the cursor
 * [I] nHittest : hit-test code
 * [I] wMouseMsg : ideintifier of the mouse message
 *
 * RETURN:
 * TRUE if cursor is set
 * FALSE otherwise
 */
static BOOL LISTVIEW_SetCursor(LISTVIEW_INFO *infoPtr, HWND hwnd, UINT nHittest, UINT wMouseMsg)
{
    POINT pt;

    if(!(infoPtr->dwLvExStyle & LVS_EX_TRACKSELECT)) return FALSE;

    if(!infoPtr->hHotCursor)  return FALSE;

    GetCursorPos(&pt);
    if (LISTVIEW_GetItemAtPt(infoPtr, pt) < 0) return FALSE;

    SetCursor(infoPtr->hHotCursor);

    return TRUE;
}

/***
 * DESCRIPTION:
 * Sets the focus.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] infoPtr : handle of previously focused window
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_SetFocus(LISTVIEW_INFO *infoPtr, HWND hwndLoseFocus)
{
    TRACE("(hwndLoseFocus=%x)\n", hwndLoseFocus);

    /* if we have the focus already, there's nothing to do */
    if (infoPtr->bFocus) return 0;
   
    /* send NM_SETFOCUS notification */
    notify_setfocus(infoPtr);

    /* put the focus rect back on */
    LISTVIEW_ToggleFocusRect(infoPtr);

    /* set window focus flag */
    infoPtr->bFocus = TRUE;

    /* redraw all visible selected items */
    LISTVIEW_InvalidateSelectedItems(infoPtr);

    return 0;
}

/***
 * DESCRIPTION:
 * Sets the font.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] HFONT : font handle
 * [I] WORD : redraw flag
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_SetFont(LISTVIEW_INFO *infoPtr, HFONT hFont, WORD fRedraw)
{
    TRACE("(hfont=%x,redraw=%hu)\n", hFont, fRedraw);

    infoPtr->hFont = hFont ? hFont : infoPtr->hDefaultFont;
    LISTVIEW_SaveTextMetrics(infoPtr);

    if (LISTVIEW_GetType(infoPtr) == LVS_REPORT)
	SendMessageW(infoPtr->hwndHeader, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(fRedraw, 0));

    if (fRedraw) LISTVIEW_InvalidateList(infoPtr);

    return 0;
}

/***
 * DESCRIPTION:
 * Message handling for WM_SETREDRAW.
 * For the Listview, it invalidates the entire window (the doc specifies otherwise)
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] bRedraw: state of redraw flag
 *
 * RETURN:
 * DefWinProc return value
 */
static LRESULT LISTVIEW_SetRedraw(LISTVIEW_INFO *infoPtr, BOOL bRedraw)
{
    /* FIXME: this is bogus */
    LRESULT lResult = DefWindowProcW(infoPtr->hwndSelf, WM_SETREDRAW, bRedraw, 0);
    if(bRedraw)
        RedrawWindow(infoPtr->hwndSelf, NULL, 0,
            RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_ALLCHILDREN | RDW_ERASENOW);
    return lResult;
}

/***
 * DESCRIPTION:
 * Resizes the listview control. This function processes WM_SIZE
 * messages.  At this time, the width and height are not used.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] WORD : new width
 * [I] WORD : new height
 *
 * RETURN:
 * Zero
 */
static LRESULT LISTVIEW_Size(LISTVIEW_INFO *infoPtr, int Width, int Height)
{
  LONG lStyle = infoPtr->dwStyle;
  UINT uView = lStyle & LVS_TYPEMASK;

  TRACE("(width=%d, height=%d)\n", Width, Height);

  if (LISTVIEW_UpdateSize(infoPtr))
  {
    if ((uView == LVS_SMALLICON) || (uView == LVS_ICON))
    {
        if (lStyle & LVS_ALIGNLEFT)
            LISTVIEW_AlignLeft(infoPtr);
        else
            LISTVIEW_AlignTop(infoPtr);
    }

    LISTVIEW_UpdateScroll(infoPtr);

    /* FIXME: be smarter here */
    LISTVIEW_InvalidateList(infoPtr);
  }

  return 0;
}

/***
 * DESCRIPTION:
 * Sets the size information.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 *
 * RETURN:
 * Zero if no size change
 * 1 of size changed
 */
static BOOL LISTVIEW_UpdateSize(LISTVIEW_INFO *infoPtr)
{
  LONG lStyle = infoPtr->dwStyle;
  UINT uView = lStyle & LVS_TYPEMASK;
  RECT rcList;
  RECT rcOld;

  GetClientRect(infoPtr->hwndSelf, &rcList);
  CopyRect(&rcOld,&(infoPtr->rcList));
  infoPtr->rcList.left = 0;
  infoPtr->rcList.right = max(rcList.right - rcList.left, 1);
  infoPtr->rcList.top = 0;
  infoPtr->rcList.bottom = max(rcList.bottom - rcList.top, 1);

  if (uView == LVS_LIST)
  {
    /* Apparently the "LIST" style is supposed to have the same
     * number of items in a column even if there is no scroll bar.
     * Since if a scroll bar already exists then the bottom is already
     * reduced, only reduce if the scroll bar does not currently exist.
     * The "2" is there to mimic the native control. I think it may be
     * related to either padding or edges.  (GLA 7/2002)
     */
    if (!(lStyle & WS_HSCROLL))
    {
      INT nHScrollHeight = GetSystemMetrics(SM_CYHSCROLL);
      if (infoPtr->rcList.bottom > nHScrollHeight)
        infoPtr->rcList.bottom -= (nHScrollHeight + 2);
    }
    else
    {
      if (infoPtr->rcList.bottom > 2)
        infoPtr->rcList.bottom -= 2;
    }
  }
  else if (uView == LVS_REPORT)
  {
    HDLAYOUT hl;
    WINDOWPOS wp;

    hl.prc = &rcList;
    hl.pwpos = &wp;
    Header_Layout(infoPtr->hwndHeader, &hl);

    SetWindowPos(wp.hwnd, wp.hwndInsertAfter, wp.x, wp.y, wp.cx, wp.cy, wp.flags);

    if (!(LVS_NOCOLUMNHEADER & lStyle))
      infoPtr->rcList.top = max(wp.cy, 0);
  }
  return (EqualRect(&rcOld,&(infoPtr->rcList)));
}

/***
 * DESCRIPTION:
 * Processes WM_STYLECHANGED messages.
 *
 * PARAMETER(S):
 * [I] infoPtr : valid pointer to the listview structure
 * [I] WPARAM : window style type (normal or extended)
 * [I] LPSTYLESTRUCT : window style information
 *
 * RETURN:
 * Zero
 */
static INT LISTVIEW_StyleChanged(LISTVIEW_INFO *infoPtr, WPARAM wStyleType,
                                 LPSTYLESTRUCT lpss)
{
  UINT uNewView = lpss->styleNew & LVS_TYPEMASK;
  UINT uOldView = lpss->styleOld & LVS_TYPEMASK;
  RECT rcList = infoPtr->rcList;

  TRACE("(styletype=%x, styleOld=0x%08lx, styleNew=0x%08lx)\n",
        wStyleType, lpss->styleOld, lpss->styleNew);

  /* FIXME: if LVS_NOSORTHEADER changed, update header */

  if (wStyleType == GWL_STYLE)
  {
    infoPtr->dwStyle = lpss->styleNew;

    if (uOldView == LVS_REPORT)
      ShowWindow(infoPtr->hwndHeader, SW_HIDE);

    if (((lpss->styleOld & WS_HSCROLL) != 0)&&
        ((lpss->styleNew & WS_HSCROLL) == 0))
       ShowScrollBar(infoPtr->hwndSelf, SB_HORZ, FALSE);

    if (((lpss->styleOld & WS_VSCROLL) != 0)&&
        ((lpss->styleNew & WS_VSCROLL) == 0))
       ShowScrollBar(infoPtr->hwndSelf, SB_VERT, FALSE);

    /* If switching modes, then start with no scroll bars and then
     * decide.
     */
    if (uNewView != uOldView)
	ShowScrollBar(infoPtr->hwndSelf, SB_BOTH, FALSE);

    if (uNewView == LVS_ICON)
    {
      INT oldcx, oldcy;

      /* First readjust the iconSize and if necessary the iconSpacing */
      oldcx = infoPtr->iconSize.cx;
      oldcy = infoPtr->iconSize.cy;
      infoPtr->iconSize.cx = GetSystemMetrics(SM_CXICON);
      infoPtr->iconSize.cy = GetSystemMetrics(SM_CYICON);
      if (infoPtr->himlNormal != NULL)
      {
	  INT cx, cy;
	  ImageList_GetIconSize(infoPtr->himlNormal, &cx, &cy);
	  infoPtr->iconSize.cx = cx;
	  infoPtr->iconSize.cy = cy;
      }
      if ((infoPtr->iconSize.cx != oldcx) || (infoPtr->iconSize.cy != oldcy))
      {
	  TRACE("icon old size=(%d,%d), new size=(%ld,%ld)\n",
		oldcx, oldcy, infoPtr->iconSize.cx, infoPtr->iconSize.cy);
	  LISTVIEW_SetIconSpacing(infoPtr,0);
      }

      /* Now update the full item width and height */
      infoPtr->nItemWidth = LISTVIEW_GetItemWidth(infoPtr);
      infoPtr->nItemHeight = LISTVIEW_GetItemHeight(infoPtr);
      if (lpss->styleNew & LVS_ALIGNLEFT)
        LISTVIEW_AlignLeft(infoPtr);
      else
        LISTVIEW_AlignTop(infoPtr);
    }
    else if (uNewView == LVS_REPORT)
    {
      HDLAYOUT hl;
      WINDOWPOS wp;

      hl.prc = &rcList;
      hl.pwpos = &wp;
      Header_Layout(infoPtr->hwndHeader, &hl);
      SetWindowPos(infoPtr->hwndHeader, infoPtr->hwndSelf, wp.x, wp.y, wp.cx, wp.cy,
                   wp.flags);
      if (!(LVS_NOCOLUMNHEADER & lpss->styleNew))
        ShowWindow(infoPtr->hwndHeader, SW_SHOWNORMAL);

      infoPtr->iconSize.cx = GetSystemMetrics(SM_CXSMICON);
      infoPtr->iconSize.cy = GetSystemMetrics(SM_CYSMICON);
      infoPtr->nItemWidth = LISTVIEW_GetItemWidth(infoPtr);
      infoPtr->nItemHeight = LISTVIEW_GetItemHeight(infoPtr);
    }
    else if (uNewView == LVS_LIST)
    {
      infoPtr->iconSize.cx = GetSystemMetrics(SM_CXSMICON);
      infoPtr->iconSize.cy = GetSystemMetrics(SM_CYSMICON);
      infoPtr->nItemWidth = LISTVIEW_GetItemWidth(infoPtr);
      infoPtr->nItemHeight = LISTVIEW_GetItemHeight(infoPtr);
    }
    else
    {
      infoPtr->iconSize.cx = GetSystemMetrics(SM_CXSMICON);
      infoPtr->iconSize.cy = GetSystemMetrics(SM_CYSMICON);
      infoPtr->nItemWidth = LISTVIEW_GetItemWidth(infoPtr);
      infoPtr->nItemHeight = LISTVIEW_GetItemHeight(infoPtr);
      if (lpss->styleNew & LVS_ALIGNLEFT)
        LISTVIEW_AlignLeft(infoPtr);
      else
        LISTVIEW_AlignTop(infoPtr);
    }

    /* update the size of the client area */
    LISTVIEW_UpdateSize(infoPtr);

    /* add scrollbars if needed */
    LISTVIEW_UpdateScroll(infoPtr);

    /* invalidate client area + erase background */
    LISTVIEW_InvalidateList(infoPtr);

    /* print the list of unsupported window styles */
    LISTVIEW_UnsupportedStyles(lpss->styleNew);
  }

  /* If they change the view and we have an active edit control
     we will need to kill the control since the redraw will
     misplace the edit control.
   */
  if (infoPtr->bEditing &&
        ((uNewView & (LVS_ICON|LVS_LIST|LVS_SMALLICON)) !=
        ((LVS_ICON|LVS_LIST|LVS_SMALLICON) & uOldView)))
  {
     SendMessageW(infoPtr->hwndEdit, WM_KILLFOCUS, 0, 0);
  }

  return 0;
}

/***
 * DESCRIPTION:
 * Window procedure of the listview control.
 *
 */
static LRESULT WINAPI
LISTVIEW_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongW(hwnd, 0);

  TRACE("(uMsg=%x wParam=%x lParam=%lx)\n", uMsg, wParam, lParam);

  if (!infoPtr && (uMsg != WM_CREATE))
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);

  if (infoPtr)
  {
    infoPtr->dwStyle = GetWindowLongW(hwnd, GWL_STYLE);
  }

  switch (uMsg)
  {
  case LVM_APPROXIMATEVIEWRECT:
    return LISTVIEW_ApproximateViewRect(infoPtr, (INT)wParam,
                                        LOWORD(lParam), HIWORD(lParam));
  case LVM_ARRANGE:
    return LISTVIEW_Arrange(infoPtr, (INT)wParam);

/* case LVN_CANCELEDITLABEL */

/* case LVM_CREATEDRAGIMAGE: */

  case LVM_DELETEALLITEMS:
    return LISTVIEW_DeleteAllItems(infoPtr);

  case LVM_DELETECOLUMN:
    return LISTVIEW_DeleteColumn(infoPtr, (INT)wParam);

  case LVM_DELETEITEM:
    return LISTVIEW_DeleteItem(infoPtr, (INT)wParam);

  case LVM_EDITLABELW:
    return (LRESULT)LISTVIEW_EditLabelT(infoPtr, (INT)wParam, TRUE);

  case LVM_EDITLABELA:
    return (LRESULT)LISTVIEW_EditLabelT(infoPtr, (INT)wParam, FALSE);

  /* case LVN_ENABLEGROUPVIEW: */

  case LVM_ENSUREVISIBLE:
    return LISTVIEW_EnsureVisible(infoPtr, (INT)wParam, (BOOL)lParam);

  case LVM_FINDITEMW:
    return LISTVIEW_FindItemW(infoPtr, (INT)wParam, (LPLVFINDINFOW)lParam);

  case LVM_FINDITEMA:
    return LISTVIEW_FindItemA(infoPtr, (INT)wParam, (LPLVFINDINFOA)lParam);

  case LVM_GETBKCOLOR:
    return infoPtr->clrBk;

  /* case LVM_GETBKIMAGE: */

  case LVM_GETCALLBACKMASK:
    return infoPtr->uCallbackMask;

  case LVM_GETCOLUMNA:
    return LISTVIEW_GetColumnT(infoPtr, (INT)wParam, (LPLVCOLUMNW)lParam, FALSE);

  case LVM_GETCOLUMNW:
    return LISTVIEW_GetColumnT(infoPtr, (INT)wParam, (LPLVCOLUMNW)lParam, TRUE);

  case LVM_GETCOLUMNORDERARRAY:
    return LISTVIEW_GetColumnOrderArray(infoPtr, (INT)wParam, (LPINT)lParam);

  case LVM_GETCOLUMNWIDTH:
    return LISTVIEW_GetColumnWidth(infoPtr, (INT)wParam);

  case LVM_GETCOUNTPERPAGE:
    return LISTVIEW_GetCountPerPage(infoPtr);

  case LVM_GETEDITCONTROL:
    return (LRESULT)infoPtr->hwndEdit;

  case LVM_GETEXTENDEDLISTVIEWSTYLE:
    return infoPtr->dwLvExStyle;

  case LVM_GETHEADER:
    return (LRESULT)infoPtr->hwndHeader;

  case LVM_GETHOTCURSOR:
    return infoPtr->hHotCursor;

  case LVM_GETHOTITEM:
    return infoPtr->nHotItem;

  case LVM_GETHOVERTIME:
    return infoPtr->dwHoverTime;

  case LVM_GETIMAGELIST:
    return LISTVIEW_GetImageList(infoPtr, (INT)wParam);

  /* case LVN_GETINSERTMARK: */

  /* case LVN_GETINSERTMARKCOLOR: */

  /* case LVN_GETINSERTMARKRECT: */

  case LVM_GETISEARCHSTRINGA:
  case LVM_GETISEARCHSTRINGW:
    FIXME("LVM_GETISEARCHSTRING: unimplemented\n");
    return FALSE;

  case LVM_GETITEMA:
    return LISTVIEW_GetItemT(infoPtr, (LPLVITEMW)lParam, FALSE, FALSE);

  case LVM_GETITEMW:
    return LISTVIEW_GetItemT(infoPtr, (LPLVITEMW)lParam, FALSE, TRUE);

  case LVM_GETITEMCOUNT:
    return GETITEMCOUNT(infoPtr);

  case LVM_GETITEMPOSITION:
    return LISTVIEW_GetItemPosition(infoPtr, (INT)wParam, (LPPOINT)lParam);

  case LVM_GETITEMRECT:
    return LISTVIEW_GetItemRect(infoPtr, (INT)wParam, (LPRECT)lParam);

  case LVM_GETITEMSPACING:
    return LISTVIEW_GetItemSpacing(infoPtr, (BOOL)wParam);

  case LVM_GETITEMSTATE:
    return LISTVIEW_GetItemState(infoPtr, (INT)wParam, (UINT)lParam);

  case LVM_GETITEMTEXTA:
    return LISTVIEW_GetItemTextT(infoPtr, (INT)wParam, (LPLVITEMW)lParam, FALSE);

  case LVM_GETITEMTEXTW:
    return LISTVIEW_GetItemTextT(infoPtr, (INT)wParam, (LPLVITEMW)lParam, TRUE);

  case LVM_GETNEXTITEM:
    return LISTVIEW_GetNextItem(infoPtr, (INT)wParam, LOWORD(lParam));

  case LVM_GETNUMBEROFWORKAREAS:
    FIXME("LVM_GETNUMBEROFWORKAREAS: unimplemented\n");
    return 1;

  case LVM_GETORIGIN:
    return LISTVIEW_GetOrigin(infoPtr, (LPPOINT)lParam);

  /* case LVN_GETOUTLINECOLOR: */

  /* case LVM_GETSELECTEDCOLUMN: */

  case LVM_GETSELECTEDCOUNT:
    return LISTVIEW_GetSelectedCount(infoPtr);

  case LVM_GETSELECTIONMARK:
    return infoPtr->nSelectionMark;

  case LVM_GETSTRINGWIDTHA:
    return LISTVIEW_GetStringWidthT(infoPtr, (LPCWSTR)lParam, FALSE);

  case LVM_GETSTRINGWIDTHW:
    return LISTVIEW_GetStringWidthT(infoPtr, (LPCWSTR)lParam, TRUE);

  case LVM_GETSUBITEMRECT:
    return LISTVIEW_GetSubItemRect(infoPtr, (UINT)wParam, ((LPRECT)lParam)->top,
                                   ((LPRECT)lParam)->left, (LPRECT)lParam);

  case LVM_GETTEXTBKCOLOR:
    return infoPtr->clrTextBk;

  case LVM_GETTEXTCOLOR:
    return infoPtr->clrText;

  /* case LVN_GETTILEINFO: */

  /* case LVN_GETTILEVIEWINFO: */

  case LVM_GETTOOLTIPS:
    FIXME("LVM_GETTOOLTIPS: unimplemented\n");
    return FALSE;

  case LVM_GETTOPINDEX:
    return LISTVIEW_GetTopIndex(infoPtr);

  /*case LVM_GETUNICODEFORMAT:
    FIXME("LVM_GETUNICODEFORMAT: unimplemented\n");
    return FALSE;*/

  case LVM_GETVIEWRECT:
    return LISTVIEW_GetViewRect(infoPtr, (LPRECT)lParam);

  case LVM_GETWORKAREAS:
    FIXME("LVM_GETWORKAREAS: unimplemented\n");
    return FALSE;

  /* case LVN_HASGROUP: */

  case LVM_HITTEST:
    return LISTVIEW_HitTest(infoPtr, (LPLVHITTESTINFO)lParam, FALSE);

  case LVM_INSERTCOLUMNA:
    return LISTVIEW_InsertColumnT(infoPtr, (INT)wParam, (LPLVCOLUMNW)lParam, FALSE);

  case LVM_INSERTCOLUMNW:
    return LISTVIEW_InsertColumnT(infoPtr, (INT)wParam, (LPLVCOLUMNW)lParam, TRUE);

  /* case LVN_INSERTGROUP: */

  /* case LVN_INSERTGROUPSORTED: */

  case LVM_INSERTITEMA:
    return LISTVIEW_InsertItemT(infoPtr, (LPLVITEMW)lParam, FALSE);

  case LVM_INSERTITEMW:
    return LISTVIEW_InsertItemT(infoPtr, (LPLVITEMW)lParam, TRUE);

  /* case LVN_INSERTMARKHITTEST: */

  /* case LVN_ISGROUPVIEWENABLED: */

  /* case LVN_MAPIDTOINDEX: */

  /* case LVN_INEDXTOID: */

  /* case LVN_MOVEGROUP: */

  /* case LVN_MOVEITEMTOGROUP: */

  case LVM_REDRAWITEMS:
    return LISTVIEW_RedrawItems(infoPtr, (INT)wParam, (INT)lParam);

  /* case LVN_REMOVEALLGROUPS: */

  /* case LVN_REMOVEGROUP: */

  case LVM_SCROLL:
    return LISTVIEW_Scroll(infoPtr, (INT)wParam, (INT)lParam);

  case LVM_SETBKCOLOR:
    return LISTVIEW_SetBkColor(infoPtr, (COLORREF)lParam);

  /* case LVM_SETBKIMAGE: */

  case LVM_SETCALLBACKMASK:
    infoPtr->uCallbackMask = (UINT)wParam;
    return TRUE;

  case LVM_SETCOLUMNA:
    return LISTVIEW_SetColumnT(infoPtr, (INT)wParam, (LPLVCOLUMNW)lParam, FALSE);

  case LVM_SETCOLUMNW:
    return LISTVIEW_SetColumnT(infoPtr, (INT)wParam, (LPLVCOLUMNW)lParam, TRUE);

  case LVM_SETCOLUMNORDERARRAY:
    return LISTVIEW_SetColumnOrderArray(infoPtr, (INT)wParam, (LPINT)lParam);

  case LVM_SETCOLUMNWIDTH:
    return LISTVIEW_SetColumnWidth(infoPtr, (INT)wParam, SLOWORD(lParam));

  case LVM_SETEXTENDEDLISTVIEWSTYLE:
    return LISTVIEW_SetExtendedListViewStyle(infoPtr, (DWORD)wParam, (DWORD)lParam);

  /* case LVN_SETGROUPINFO: */

  /* case LVN_SETGROUPMETRICS: */

  case LVM_SETHOTCURSOR:
    return LISTVIEW_SetHotCursor(infoPtr, (HCURSOR)lParam);

  case LVM_SETHOTITEM:
    return LISTVIEW_SetHotItem(infoPtr, (INT)wParam);

  case LVM_SETHOVERTIME:
    return LISTVIEW_SetHoverTime(infoPtr, (DWORD)wParam);

  case LVM_SETICONSPACING:
    return LISTVIEW_SetIconSpacing(infoPtr, (DWORD)lParam);

  case LVM_SETIMAGELIST:
    return (LRESULT)LISTVIEW_SetImageList(infoPtr, (INT)wParam, (HIMAGELIST)lParam);

  /* case LVN_SETINFOTIP: */

  /* case LVN_SETINSERTMARK: */

  /* case LVN_SETINSERTMARKCOLOR: */

  case LVM_SETITEMA:
    return LISTVIEW_SetItemT(infoPtr, (LPLVITEMW)lParam, FALSE);

  case LVM_SETITEMW:
    return LISTVIEW_SetItemT(infoPtr, (LPLVITEMW)lParam, TRUE);

  case LVM_SETITEMCOUNT:
    return LISTVIEW_SetItemCount(infoPtr, (INT)wParam, (DWORD)lParam);

  case LVM_SETITEMPOSITION:
    return LISTVIEW_SetItemPosition(infoPtr, (INT)wParam, (INT)LOWORD(lParam),
                                    (INT)HIWORD(lParam));

  case LVM_SETITEMPOSITION32:
    return LISTVIEW_SetItemPosition(infoPtr, (INT)wParam, ((POINT*)lParam)->x,
				    ((POINT*)lParam)->y);

  case LVM_SETITEMSTATE:
    return LISTVIEW_SetItemState(infoPtr, (INT)wParam, (LPLVITEMW)lParam);

  case LVM_SETITEMTEXTA:
    return LISTVIEW_SetItemTextT(infoPtr, (INT)wParam, (LPLVITEMW)lParam, FALSE);

  case LVM_SETITEMTEXTW:
    return LISTVIEW_SetItemTextT(infoPtr, (INT)wParam, (LPLVITEMW)lParam, TRUE);

  /* case LVN_SETOUTLINECOLOR: */

  /* case LVN_SETSELECTEDCOLUMN: */

  case LVM_SETSELECTIONMARK:
    return LISTVIEW_SetSelectionMark(infoPtr, (INT)lParam);

  case LVM_SETTEXTBKCOLOR:
    return LISTVIEW_SetTextBkColor(infoPtr, (COLORREF)lParam);

  case LVM_SETTEXTCOLOR:
    return LISTVIEW_SetTextColor(infoPtr, (COLORREF)lParam);

  /* case LVN_SETTILEINFO: */

  /* case LVN_SETTILEVIEWINFO: */

  /* case LVN_SETTILEWIDTH: */

  /* case LVM_SETTOOLTIPS: */

  /* case LVM_SETUNICODEFORMAT: */

  /* case LVN_SETVIEW: */

  /* case LVM_SETWORKAREAS: */

  /* case LVN_SORTGROUPS: */

  case LVM_SORTITEMS:
    return LISTVIEW_SortItems(infoPtr, (PFNLVCOMPARE)lParam, (LPARAM)wParam);

  case LVM_SUBITEMHITTEST:
    return LISTVIEW_HitTest(infoPtr, (LPLVHITTESTINFO)lParam, TRUE);

  case LVM_UPDATE:
    return LISTVIEW_Update(infoPtr, (INT)wParam);

  case WM_CHAR:
    return LISTVIEW_ProcessLetterKeys( infoPtr, wParam, lParam );

  case WM_COMMAND:
    return LISTVIEW_Command(infoPtr, wParam, lParam);

  case WM_CREATE:
    return LISTVIEW_Create(hwnd, (LPCREATESTRUCTW)lParam);

  case WM_ERASEBKGND:
    return LISTVIEW_EraseBkgnd(infoPtr, (HDC)wParam);

  case WM_GETDLGCODE:
    return DLGC_WANTCHARS | DLGC_WANTARROWS;

  case WM_GETFONT:
    return infoPtr->hFont;

  case WM_HSCROLL:
    return LISTVIEW_HScroll(infoPtr, (INT)LOWORD(wParam), 0, (HWND)lParam);

  case WM_KEYDOWN:
    return LISTVIEW_KeyDown(infoPtr, (INT)wParam, (LONG)lParam);

  case WM_KILLFOCUS:
    return LISTVIEW_KillFocus(infoPtr);

  case WM_LBUTTONDBLCLK:
    return LISTVIEW_LButtonDblClk(infoPtr, (WORD)wParam, MAKEPOINTS(lParam));

  case WM_LBUTTONDOWN:
    return LISTVIEW_LButtonDown(infoPtr, (WORD)wParam, MAKEPOINTS(lParam));

  case WM_LBUTTONUP:
    return LISTVIEW_LButtonUp(infoPtr, (WORD)wParam, MAKEPOINTS(lParam));

  case WM_MOUSEMOVE:
    return LISTVIEW_MouseMove (infoPtr, (WORD)wParam, MAKEPOINTS(lParam));

  case WM_MOUSEHOVER:
    return LISTVIEW_MouseHover(infoPtr, (WORD)wParam, MAKEPOINTS(lParam));

  case WM_NCDESTROY:
    return LISTVIEW_NCDestroy(infoPtr);

  case WM_NOTIFY:
    return LISTVIEW_Notify(infoPtr, (INT)wParam, (LPNMHDR)lParam);

  case WM_NOTIFYFORMAT:
    return LISTVIEW_NotifyFormat(infoPtr, (HWND)wParam, (INT)lParam);

  case WM_PAINT:
    return LISTVIEW_Paint(infoPtr, (HDC)wParam);

  case WM_RBUTTONDBLCLK:
    return LISTVIEW_RButtonDblClk(infoPtr, (WORD)wParam, MAKEPOINTS(lParam));

  case WM_RBUTTONDOWN:
    return LISTVIEW_RButtonDown(infoPtr, (WORD)wParam, MAKEPOINTS(lParam));

  case WM_RBUTTONUP:
    return LISTVIEW_RButtonUp(infoPtr, (WORD)wParam, MAKEPOINTS(lParam));

  case WM_SETCURSOR:
    if(LISTVIEW_SetCursor(infoPtr, (HWND)wParam, LOWORD(lParam), HIWORD(lParam)))
      return TRUE;
    goto fwd_msg;

  case WM_SETFOCUS:
    return LISTVIEW_SetFocus(infoPtr, (HWND)wParam);

  case WM_SETFONT:
    return LISTVIEW_SetFont(infoPtr, (HFONT)wParam, (WORD)lParam);

  case WM_SETREDRAW:
    return LISTVIEW_SetRedraw(infoPtr, (BOOL)wParam);

  case WM_SIZE:
    return LISTVIEW_Size(infoPtr, (int)SLOWORD(lParam), (int)SHIWORD(lParam));

  case WM_STYLECHANGED:
    return LISTVIEW_StyleChanged(infoPtr, wParam, (LPSTYLESTRUCT)lParam);

  case WM_SYSCOLORCHANGE:
    COMCTL32_RefreshSysColors();
    return 0;

/*	case WM_TIMER: */

  case WM_VSCROLL:
    return LISTVIEW_VScroll(infoPtr, (INT)LOWORD(wParam), 0, (HWND)lParam);

  case WM_MOUSEWHEEL:
      if (wParam & (MK_SHIFT | MK_CONTROL))
          return DefWindowProcW(hwnd, uMsg, wParam, lParam);
      return LISTVIEW_MouseWheel(infoPtr, (short int)HIWORD(wParam));

  case WM_WINDOWPOSCHANGED:
      if (!(((WINDOWPOS *)lParam)->flags & SWP_NOSIZE)) {
	  SetWindowPos(infoPtr->hwndSelf, 0, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOACTIVATE |
		       SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE);
	  LISTVIEW_UpdateSize(infoPtr);
	  LISTVIEW_UpdateScroll(infoPtr);
      }
      return DefWindowProcW(hwnd, uMsg, wParam, lParam);

/*	case WM_WININICHANGE: */

  default:
    if ((uMsg >= WM_USER) && (uMsg < WM_APP))
      ERR("unknown msg %04x wp=%08x lp=%08lx\n", uMsg, wParam, lParam);

  fwd_msg:
    /* call default window procedure */
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
  }

  return 0;
}

/***
 * DESCRIPTION:
 * Registers the window class.
 *
 * PARAMETER(S):
 * None
 *
 * RETURN:
 * None
 */
void LISTVIEW_Register(void)
{
    WNDCLASSW wndClass;

    ZeroMemory(&wndClass, sizeof(WNDCLASSW));
    wndClass.style = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc = (WNDPROC)LISTVIEW_WindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = sizeof(LISTVIEW_INFO *);
    wndClass.hCursor = LoadCursorW(0, IDC_ARROWW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = WC_LISTVIEWW;
    RegisterClassW(&wndClass);
}

/***
 * DESCRIPTION:
 * Unregisters the window class.
 *
 * PARAMETER(S):
 * None
 *
 * RETURN:
 * None
 */
void LISTVIEW_Unregister(void)
{
    UnregisterClassW(WC_LISTVIEWW, (HINSTANCE)NULL);
}

/***
 * DESCRIPTION:
 * Handle any WM_COMMAND messages
 *
 * PARAMETER(S):
 *
 * RETURN:
 */
static LRESULT LISTVIEW_Command(LISTVIEW_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    switch (HIWORD(wParam))
    {
	case EN_UPDATE:
	{
	    /*
	     * Adjust the edit window size
	     */
	    WCHAR buffer[1024];
	    HDC           hdc      = GetDC(infoPtr->hwndEdit);
            HFONT         hFont, hOldFont = 0;
	    RECT	  rect;
	    SIZE	  sz;
	    int		  len;

	    len = GetWindowTextW(infoPtr->hwndEdit, buffer, sizeof(buffer)/sizeof(buffer[0]));
	    GetWindowRect(infoPtr->hwndEdit, &rect);

            /* Select font to get the right dimension of the string */
            hFont = SendMessageW(infoPtr->hwndEdit, WM_GETFONT, 0, 0);
            if(hFont != 0)
            {
                hOldFont = SelectObject(hdc, hFont);
            }

	    if (GetTextExtentPoint32W(hdc, buffer, lstrlenW(buffer), &sz))
	    {
                TEXTMETRICW textMetric;

                /* Add Extra spacing for the next character */
                GetTextMetricsW(hdc, &textMetric);
                sz.cx += (textMetric.tmMaxCharWidth * 2);

		SetWindowPos (
		    infoPtr->hwndEdit,
		    HWND_TOP,
		    0,
		    0,
		    sz.cx,
		    rect.bottom - rect.top,
		    SWP_DRAWFRAME|SWP_NOMOVE);
	    }
            if(hFont != 0)
                SelectObject(hdc, hOldFont);

	    ReleaseDC(infoPtr->hwndSelf, hdc);

	    break;
	}

	default:
	  return SendMessageW (GetParent (infoPtr->hwndSelf), WM_COMMAND, wParam, lParam);
    }

    return 0;
}


/***
 * DESCRIPTION:
 * Subclassed edit control windproc function
 *
 * PARAMETER(S):
 *
 * RETURN:
 */
static LRESULT EditLblWndProcT(HWND hwnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam, BOOL isW)
{
    LISTVIEW_INFO *infoPtr = (LISTVIEW_INFO *)GetWindowLongW(GetParent(hwnd), 0);
    static BOOL bIgnoreKillFocus = FALSE;
    BOOL cancel = FALSE;

    TRACE("(hwnd=%x, uMsg=%x, wParam=%x, lParam=%lx, isW=%d)\n",
	  hwnd, uMsg, wParam, lParam, isW);

    switch (uMsg)
    {
	case WM_GETDLGCODE:
	  return DLGC_WANTARROWS | DLGC_WANTALLKEYS;

	case WM_KILLFOCUS:
            if(bIgnoreKillFocus) return TRUE;
	    break;

	case WM_DESTROY:
	{
	    WNDPROC editProc = infoPtr->EditWndProc;
	    infoPtr->EditWndProc = 0;
	    SetWindowLongW(hwnd, GWL_WNDPROC, (LONG)editProc);
	    return CallWindowProcT(editProc, hwnd, uMsg, wParam, lParam, isW);
	}

	case WM_KEYDOWN:
	    if (VK_ESCAPE == (INT)wParam)
	    {
		cancel = TRUE;
                break;
	    }
	    else if (VK_RETURN == (INT)wParam)
		break;

	default:
	    return CallWindowProcT(infoPtr->EditWndProc, hwnd, uMsg, wParam, lParam, isW);
    }

    if (infoPtr->bEditing)
    {
	LPWSTR buffer = NULL;

	if (!cancel)
	{
	    DWORD len = isW ? GetWindowTextLengthW(hwnd) : GetWindowTextLengthA(hwnd);

	    if (len)
	    {
		if ( (buffer = COMCTL32_Alloc((len+1) * (isW ? sizeof(WCHAR) : sizeof(CHAR)))) )
		{
		    if (isW) GetWindowTextW(hwnd, buffer, len+1);
		    else GetWindowTextA(hwnd, (CHAR*)buffer, len+1);
		}
	    }
	}
        /* Processing LVN_ENDLABELEDIT message could kill the focus       */
        /* eg. Using a messagebox                                         */
        bIgnoreKillFocus = TRUE;
	LISTVIEW_EndEditLabelT(infoPtr, buffer, isW);

	if (buffer) COMCTL32_Free(buffer);

        bIgnoreKillFocus = FALSE;
    }

    SendMessageW(hwnd, WM_CLOSE, 0, 0);
    return TRUE;
}

/***
 * DESCRIPTION:
 * Subclassed edit control windproc function
 *
 * PARAMETER(S):
 *
 * RETURN:
 */
LRESULT CALLBACK EditLblWndProcW(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return EditLblWndProcT(hwnd, uMsg, wParam, lParam, TRUE);
}

/***
 * DESCRIPTION:
 * Subclassed edit control windproc function
 *
 * PARAMETER(S):
 *
 * RETURN:
 */
LRESULT CALLBACK EditLblWndProcA(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return EditLblWndProcT(hwnd, uMsg, wParam, lParam, FALSE);
}

/***
 * DESCRIPTION:
 * Creates a subclassed edit cotrol
 *
 * PARAMETER(S):
 *
 * RETURN:
 */
static HWND CreateEditLabelT(LISTVIEW_INFO *infoPtr, LPCWSTR text, DWORD style,
	INT x, INT y, INT width, INT height, BOOL isW)
{
    WCHAR editName[5] = { 'E', 'd', 'i', 't', '\0' };
    HWND hedit;
    SIZE sz;
    HDC hdc;
    HDC hOldFont=0;
    TEXTMETRICW textMetric;
    HINSTANCE hinst = GetWindowLongW(infoPtr->hwndSelf, GWL_HINSTANCE);

    TRACE("(text=%s, ..., isW=%d)\n", debugtext_t(text, isW), isW);

    style |= WS_CHILDWINDOW|WS_CLIPSIBLINGS|ES_LEFT|WS_BORDER;
    hdc = GetDC(infoPtr->hwndSelf);

    /* Select the font to get appropriate metric dimensions */
    if(infoPtr->hFont != 0)
        hOldFont = SelectObject(hdc, infoPtr->hFont);

    /*Get String Lenght in pixels */
    GetTextExtentPoint32W(hdc, text, lstrlenW(text), &sz);

    /*Add Extra spacing for the next character */
    GetTextMetricsW(hdc, &textMetric);
    sz.cx += (textMetric.tmMaxCharWidth * 2);

    if(infoPtr->hFont != 0)
        SelectObject(hdc, hOldFont);

    ReleaseDC(infoPtr->hwndSelf, hdc);
    if (isW)
	hedit = CreateWindowW(editName, text, style, x, y, sz.cx, height, infoPtr->hwndSelf, 0, hinst, 0);
    else
	hedit = CreateWindowA("Edit", (LPCSTR)text, style, x, y, sz.cx, height, infoPtr->hwndSelf, 0, hinst, 0);

    if (!hedit) return 0;

    infoPtr->EditWndProc = (WNDPROC)
	(isW ? SetWindowLongW(hedit, GWL_WNDPROC, (LONG)EditLblWndProcW) :
               SetWindowLongA(hedit, GWL_WNDPROC, (LONG)EditLblWndProcA) );

    SendMessageW(hedit, WM_SETFONT, infoPtr->hFont, FALSE);

    return hedit;
}
