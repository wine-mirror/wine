/*
 * ComboBoxEx control
 *
 * Copyright 1998, 1999 Eric Kohl
 *
 * NOTES
 *   This is just a dummy control. An author is needed! Any volunteers?
 *   I will only improve this control once in a while.
 *     Eric <ekohl@abo.rhein-zeitung.de>
 *
 * TODO:
 *   - All messages.
 *   - All notifications.
 *
 * FIXME:
 *   - should include "combo.h" 

 * Changes  Guy Albertelli <galberte@neo.lrun.com>
 *   1. Implemented message CB_SETITEMHEIGHT 
 *   2. Implemented message WM_WINDOWPOSCHANGING
 *   3. Implemented message WM_MEASUREITEM
 *   4. Add code to WM_CREATE processing to set font of COMBOBOX and
 *      issue the CB_SETITEMHEIGHT to start the correct sizing process.
 * The above 4 changes allow the window rect for the comboboxex
 * to be set properly, which in turn allows the height of the
 * rebar control it *may* be imbeded in to be correct.
 *   5. Rewrite CBEM_INSERTITEMA to save the information.
 *   6. Implemented message WM_DRAWITEM. The code will handle images
 *      but not "overlays" yet.
 *   7. Fixed code in CBEM_SETIMAGELIST to resize control.
 *   8. Add debugging code.
 *
 * Test vehicals were the ControlSpy modules (rebar.exe and comboboxex.exe)
 *
 */

#include "winbase.h"
#include "wine/winestring.h"
#include "commctrl.h"
#include "debugtools.h"
#include "wine/unicode.h"

DEFAULT_DEBUG_CHANNEL(comboex);
DECLARE_DEBUG_CHANNEL(message);

/* Item structure */
typedef struct
{
    VOID *next;
    UINT mask;
    LPWSTR pszText;
    int cchTextMax;
    int iImage;
    int iSelectedImage;
    int iOverlay;
    int iIndent;
    LPARAM lParam;
} CBE_ITEMDATA;

/* ComboBoxEx structure */
typedef struct
{
    HIMAGELIST   himl;
    HWND         hwndCombo;
    DWORD        dwExtStyle;
    HFONT        font;
    INT          nb_items;         /* Number of items */
    CBE_ITEMDATA *items;           /* Array of items */
} COMBOEX_INFO;

#define ID_CB_EDIT    1001

/* Height in pixels of control over the amount of the selected font */
#define CBE_EXTRA     3

/* Indent amount per MS documentation */
#define CBE_INDENT    10

/* Offset in pixels from left side for start of image or text */
#define CBE_STARTOFFSET   6

/* Offset between image and text */
#define CBE_SEP   4

#define COMBOEX_GetInfoPtr(wndPtr) ((COMBOEX_INFO *)GetWindowLongA (hwnd, 0))


static void
COMBOEX_DumpItem (CBE_ITEMDATA *item)
{
    if (TRACE_ON(comboex)){
      TRACE("item %p - mask=%08x, pszText=%p, cchTM=%d, iImage=%d\n",
	    item, item->mask, item->pszText, item->cchTextMax,
	    item->iImage);
      TRACE("item %p - iSelectedImage=%d, iOverlay=%d, iIndent=%d, lParam=%08lx\n",
	    item, item->iSelectedImage, item->iOverlay, item->iIndent, item->lParam);
    }
}


inline static LRESULT
COMBOEX_Forward (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);

    FIXME("(0x%x 0x%x 0x%lx): stub\n", uMsg, wParam, lParam);

    if (infoPtr->hwndCombo)    
	return SendMessageA (infoPtr->hwndCombo, uMsg, wParam, lParam);

    return 0;
}


static void
COMBOEX_ReSize (HWND hwnd, COMBOEX_INFO *infoPtr)
{
    HFONT nfont, ofont;
    HDC mydc;
    SIZE mysize;
    UINT cy;
    IMAGEINFO iinfo;

    mydc = GetDC (0); /* why the entire screen???? */
    nfont = SendMessageA (infoPtr->hwndCombo, WM_GETFONT, 0, 0);
    ofont = (HFONT) SelectObject (mydc, nfont);
    GetTextExtentPointA (mydc, "A", 1, &mysize);
    SelectObject (mydc, ofont);
    ReleaseDC (0, mydc);
    cy = mysize.cy + CBE_EXTRA;
    if (infoPtr->himl) {
	ImageList_GetImageInfo(infoPtr->himl, 0, &iinfo);
	cy = max (iinfo.rcImage.bottom - iinfo.rcImage.top, cy);
    }
    TRACE("selected font hwnd=%08x, height=%d\n", nfont, cy);
    SendMessageA (hwnd, CB_SETITEMHEIGHT, (WPARAM) -1, (LPARAM) cy);
    if (infoPtr->hwndCombo)
        SendMessageA (infoPtr->hwndCombo, CB_SETITEMHEIGHT,
		      (WPARAM) 0, (LPARAM) cy);
}


/* ***  CBEM_xxx message support  *** */


/* << COMBOEX_DeleteItem >> */


inline static LRESULT
COMBOEX_GetComboControl (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);

    TRACE("\n");

    return (LRESULT)infoPtr->hwndCombo;
}


inline static LRESULT
COMBOEX_GetEditControl (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);

    if ((GetWindowLongA (hwnd, GWL_STYLE) & CBS_DROPDOWNLIST) != CBS_DROPDOWN)
	return 0;

    TRACE("-- 0x%x\n", GetDlgItem (infoPtr->hwndCombo, ID_CB_EDIT));

    return (LRESULT)GetDlgItem (infoPtr->hwndCombo, ID_CB_EDIT);
}


inline static LRESULT
COMBOEX_GetExtendedStyle (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);

    return (LRESULT)infoPtr->dwExtStyle;
}


inline static LRESULT
COMBOEX_GetImageList (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);

    TRACE("(0x%08x 0x%08lx)\n", wParam, lParam);

    return (LRESULT)infoPtr->himl;
}


/* << COMBOEX_GetItemA >> */

/* << COMBOEX_GetItemW >> */

/* << COMBOEX_GetUniCodeFormat >> */

/* << COMBOEX_HasEditChanged >> */


static LRESULT
COMBOEX_InsertItemA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);
    COMBOBOXEXITEMA *cit = (COMBOBOXEXITEMA *) lParam;
    INT index;
    CBE_ITEMDATA *item;


    /* get real index of item to insert */
    index = cit->iItem;
    if (index == -1) index = infoPtr->nb_items;
    if (index > infoPtr->nb_items) index = infoPtr->nb_items;

    /* get space and chain it in */
    item = (CBE_ITEMDATA *)COMCTL32_Alloc (sizeof (CBE_ITEMDATA));
    item->next = NULL;
    item->pszText = NULL;

    /* locate position to insert new item in */
    if (index == infoPtr->nb_items) {
        /* fast path for iItem = -1 */
        item->next = infoPtr->items;
	infoPtr->items = item;
    }
    else {
        int i = infoPtr->nb_items-1;
	CBE_ITEMDATA *moving = infoPtr->items;

	while (i > index && moving) {
	    moving = (CBE_ITEMDATA *)moving->next;
	}
	if (!moving) {
	    FIXME("COMBOBOXEX item structures broken. Please report!\n");
	    COMCTL32_Free(item);
	    return -1;
	}
	item->next = moving->next;
	moving->next = item;
    }

    /* fill in our hidden item structure */
    item->mask           = cit->mask;
    if (item->mask & CBEIF_TEXT) {
        LPSTR str;
	INT len;

        str = cit->pszText;
        if (!str) str="";
	len = MultiByteToWideChar (CP_ACP, 0, str, -1, NULL, 0);
	if (len > 0) {
	    item->pszText = (LPWSTR)COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    MultiByteToWideChar (CP_ACP, 0, str, -1, item->pszText, len);
	}
        item->cchTextMax   = cit->cchTextMax;
    }
    if (item->mask & CBEIF_IMAGE)
      item->iImage         = cit->iImage;
    if (item->mask & CBEIF_SELECTEDIMAGE)
      item->iSelectedImage = cit->iSelectedImage;
    if (item->mask & CBEIF_OVERLAY)
      item->iOverlay       = cit->iOverlay;
    if (item->mask & CBEIF_INDENT)
      item->iIndent        = cit->iIndent;
    if (item->mask & CBEIF_LPARAM)
      item->lParam         = cit->lParam;
    infoPtr->nb_items++;

    COMBOEX_DumpItem (item);

    SendMessageA (infoPtr->hwndCombo, CB_INSERTSTRING, 
		  (WPARAM)cit->iItem, (LPARAM)item);

    return index;

}


static LRESULT
COMBOEX_InsertItemW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);
    COMBOBOXEXITEMW *cit = (COMBOBOXEXITEMW *) lParam;
    INT index;
    CBE_ITEMDATA *item;

    /* get real index of item to insert */
    index = cit->iItem;
    if (index == -1) index = infoPtr->nb_items;
    if (index > infoPtr->nb_items) index = infoPtr->nb_items;

    /* get space and chain it in */
    item = (CBE_ITEMDATA *)COMCTL32_Alloc (sizeof (CBE_ITEMDATA));
    item->next = NULL;
    item->pszText = NULL;

    /* locate position to insert new item in */
    if (index == infoPtr->nb_items) {
        /* fast path for iItem = -1 */
        item->next = infoPtr->items;
	infoPtr->items = item;
    }
    else {
        INT i = infoPtr->nb_items-1;
	CBE_ITEMDATA *moving = infoPtr->items;

	while ((i > index) && moving) {
	    moving = (CBE_ITEMDATA *)moving->next;
	    i--;
	}
	if (!moving) {
	    FIXME("COMBOBOXEX item structures broken. Please report!\n");
	    COMCTL32_Free(item);
	    return -1;
	}
	item->next = moving->next;
	moving->next = item;
    }

    /* fill in our hidden item structure */
    item->mask           = cit->mask;
    if (item->mask & CBEIF_TEXT) {
        LPWSTR str;
	INT len;

        str = cit->pszText;
        if (!str) str = (LPWSTR) L"";
	len = strlenW (str);
	if (len > 0) {
	    item->pszText = (LPWSTR)COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    strcpyW (item->pszText, str);
	}
        item->cchTextMax   = cit->cchTextMax;
    }
    if (item->mask & CBEIF_IMAGE)
      item->iImage         = cit->iImage;
    if (item->mask & CBEIF_SELECTEDIMAGE)
      item->iSelectedImage = cit->iSelectedImage;
    if (item->mask & CBEIF_OVERLAY)
      item->iOverlay       = cit->iOverlay;
    if (item->mask & CBEIF_INDENT)
      item->iIndent        = cit->iIndent;
    if (item->mask & CBEIF_LPARAM)
      item->lParam         = cit->lParam;
    infoPtr->nb_items++;

    COMBOEX_DumpItem (item);

    SendMessageA (infoPtr->hwndCombo, CB_INSERTSTRING, 
		  (WPARAM)cit->iItem, (LPARAM)item);

    return index;

}


static LRESULT
COMBOEX_SetExtendedStyle (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);
    DWORD dwTemp;

    TRACE("(0x%08x 0x%08lx)\n", wParam, lParam);

    dwTemp = infoPtr->dwExtStyle;

    if ((DWORD)wParam) {
	infoPtr->dwExtStyle = (infoPtr->dwExtStyle & ~(DWORD)wParam) | (DWORD)lParam;
    }
    else
	infoPtr->dwExtStyle = (DWORD)lParam;

    /* FIXME: repaint?? */

    return (LRESULT)dwTemp;
}


inline static LRESULT
COMBOEX_SetImageList (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);
    HIMAGELIST himlTemp;

    TRACE("(0x%08x 0x%08lx)\n", wParam, lParam);

    himlTemp = infoPtr->himl;
    infoPtr->himl = (HIMAGELIST)lParam;

    COMBOEX_ReSize (hwnd, infoPtr);
    InvalidateRect (hwnd, NULL, TRUE);

    return (LRESULT)himlTemp;
}


static LRESULT
COMBOEX_SetItemA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);
    COMBOBOXEXITEMA *cit = (COMBOBOXEXITEMA *) lParam;
    INT index;
    INT i;
    CBE_ITEMDATA *item;

    /* get real index of item to insert */
    index = cit->iItem;
    if (index == -1) {
        FIXME("NYI setting data for item in edit control\n");
	return 0;
    }

    /* if item number requested does not exist then return failure */
    if ((index > infoPtr->nb_items) || (index < 0)) return 0;

    /* find the item in the list */
    item = infoPtr->items;
    i = infoPtr->nb_items - 1;
    while (item && (i > index)) {
        item = (CBE_ITEMDATA *)item->next;
	i--;
    }
    if (!item || (i != index)) {
	FIXME("COMBOBOXEX item structures broken. Please report!\n");
	return 0;
    }

    /* add/change stuff to the internal item structure */ 
    item->mask |= cit->mask;
    if (cit->mask & CBEIF_TEXT) {
        LPSTR str;
	INT len;

        str = cit->pszText;
        if (!str) str="";
	len = MultiByteToWideChar (CP_ACP, 0, str, -1, NULL, 0);
	if (len > 0) {
	    item->pszText = (LPWSTR)COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    MultiByteToWideChar (CP_ACP, 0, str, -1, item->pszText, len);
	}
        item->cchTextMax   = cit->cchTextMax;
    }
    if (cit->mask & CBEIF_IMAGE)
      item->iImage         = cit->iImage;
    if (cit->mask & CBEIF_SELECTEDIMAGE)
      item->iSelectedImage = cit->iSelectedImage;
    if (cit->mask & CBEIF_OVERLAY)
      item->iOverlay       = cit->iOverlay;
    if (cit->mask & CBEIF_INDENT)
      item->iIndent        = cit->iIndent;
    if (cit->mask & CBEIF_LPARAM)
      cit->lParam         = cit->lParam;

    COMBOEX_DumpItem (item);

    return TRUE;
}


/* << COMBOEX_SetItemW >> */

/* << COMBOEX_SetUniCodeFormat >> */


/* ***  CB_xxx message support  *** */


static LRESULT
COMBOEX_SetItemHeight (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);
    RECT cb_wrect, cbx_wrect, cbx_crect;
    LRESULT ret = 0;
    UINT height;

    /* First, lets forward the message to the normal combo control
       just like Windows.     */
    if (infoPtr->hwndCombo)    
       SendMessageA (infoPtr->hwndCombo, CB_SETITEMHEIGHT, wParam, lParam);

    /* *** new *** */
    GetWindowRect (infoPtr->hwndCombo, &cb_wrect);
    GetWindowRect (hwnd, &cbx_wrect);
    GetClientRect (hwnd, &cbx_crect);
    /* the height of comboex as height of the combo + comboex border */ 
    height = cb_wrect.bottom-cb_wrect.top
             + cbx_wrect.bottom-cbx_wrect.top
             - (cbx_crect.bottom-cbx_crect.top);
    TRACE("EX window=(%d,%d)-(%d,%d), client=(%d,%d)-(%d,%d)\n",
	  cbx_wrect.left, cbx_wrect.top, cbx_wrect.right, cbx_wrect.bottom,
	  cbx_crect.left, cbx_crect.top, cbx_crect.right, cbx_crect.bottom);
    TRACE("CB window=(%d,%d)-(%d,%d), EX setting=(0,0)-(%d,%d)\n",
	  cb_wrect.left, cb_wrect.top, cb_wrect.right, cb_wrect.bottom,
	  cbx_wrect.right-cbx_wrect.left, height);
    SetWindowPos (hwnd, HWND_TOP, 0, 0,
		  cbx_wrect.right-cbx_wrect.left,
		  height,
		  SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
    /* *** end new *** */

    return ret;
}


/* ***  WM_xxx message support  *** */


static LRESULT
COMBOEX_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    LPCREATESTRUCTA cs = (LPCREATESTRUCTA) lParam;
    COMBOEX_INFO *infoPtr;
    DWORD dwComboStyle;
    LOGFONTA mylogfont;

    /* allocate memory for info structure */
    infoPtr = (COMBOEX_INFO *)COMCTL32_Alloc (sizeof(COMBOEX_INFO));
    if (infoPtr == NULL) {
	ERR("could not allocate info memory!\n");
	return 0;
    }
    infoPtr->items    = NULL;
    infoPtr->nb_items = 0;

    SetWindowLongA (hwnd, 0, (DWORD)infoPtr);


    /* initialize info structure */


    /* create combo box */
    dwComboStyle = GetWindowLongA (hwnd, GWL_STYLE) &
			(CBS_SIMPLE|CBS_DROPDOWN|CBS_DROPDOWNLIST|WS_CHILD);

    infoPtr->hwndCombo = CreateWindowA ("ComboBox", "",
			 /* following line added to match native */
                         WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL | CBS_NOINTEGRALHEIGHT | 
			 /* was base and is necessary */
			 WS_CHILD | WS_VISIBLE | CBS_OWNERDRAWFIXED | dwComboStyle,
			cs->y, cs->x, cs->cx, cs->cy, hwnd, (HMENU)0,
			GetWindowLongA (hwnd, GWL_HINSTANCE), NULL);

    /* *** new *** */
    SystemParametersInfoA (SPI_GETICONTITLELOGFONT, sizeof(mylogfont), &mylogfont, 0);
    infoPtr->font = CreateFontIndirectA (&mylogfont);
    SendMessageA (infoPtr->hwndCombo, WM_SETFONT, (WPARAM)infoPtr->font, 0);
    COMBOEX_ReSize (hwnd, infoPtr);
    /* *** end new *** */

    return 0;
}


inline static LRESULT
COMBOEX_DrawItem (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);
    DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lParam;
    CBE_ITEMDATA *item;
    SIZE txtsize;
    COLORREF nbkc, ntxc;
    RECT rect;
    int drawimage;
    UINT x, xbase, y;
    UINT xioff = 0;               /* size and spacer of image if any */
    IMAGEINFO iinfo;
    INT len;

    if (!IsWindowEnabled(infoPtr->hwndCombo)) return 0;

    /* MSDN says:                                                       */
    /*     "itemID - Specifies the menu item identifier for a menu      */
    /*      item or the index of the item in a list box or combo box.   */
    /*      For an empty list box or combo box, this member can be -1.  */
    /*      This allows the application to draw only the focus          */
    /*      rectangle at the coordinates specified by the rcItem        */
    /*      member even though there are no items in the control.       */
    /*      This indicates to the user whether the list box or combo    */
    /*      box has the focus. How the bits are set in the itemAction   */
    /*      member determines whether the rectangle is to be drawn as   */
    /*      though the list box or combo box has the focus.             */
    if (dis->itemID == 0xffffffff) {
      if ( ( (dis->itemAction & ODA_FOCUS) && (dis->itemState & ODS_SELECTED)) ||
	   ( (dis->itemAction & (ODA_SELECT | ODA_DRAWENTIRE)) && (dis->itemState & ODS_FOCUS) ) ) { 
        TRACE("drawing item -1 special focus, rect=(%d,%d)-(%d,%d)\n",
	      dis->rcItem.left, dis->rcItem.top,
	      dis->rcItem.right, dis->rcItem.bottom);
	DrawFocusRect(dis->hDC, &dis->rcItem);
	return 0;
      }
      else {
	TRACE("NOT drawing item  -1 special focus, rect=(%d,%d)-(%d,%d), action=%08x, state=%08x\n",
	      dis->rcItem.left, dis->rcItem.top,
	      dis->rcItem.right, dis->rcItem.bottom,
	      dis->itemAction, dis->itemState);
	return 0;
      }
    }

    item = (CBE_ITEMDATA *)SendMessageA (infoPtr->hwndCombo, CB_GETITEMDATA, 
					 (WPARAM)dis->itemID, 0);
    if (item == (CBE_ITEMDATA *)CB_ERR)
    {
        FIXME("invalid item for id %d \n",dis->itemID);
        return 0;
    }
    if (!TRACE_ON(message)) {
	TRACE("DRAWITEMSTRUCT: CtlType=0x%08x CtlID=0x%08x\n", 
	      dis->CtlType, dis->CtlID);
	TRACE("itemID=0x%08x itemAction=0x%08x itemState=0x%08x\n", 
	      dis->itemID, dis->itemAction, dis->itemState);
	TRACE("hWnd=0x%04x hDC=0x%04x (%d,%d)-(%d,%d) itemData=0x%08lx\n",
	      dis->hwndItem, dis->hDC, dis->rcItem.left, 
	      dis->rcItem.top, dis->rcItem.right, dis->rcItem.bottom, 
	      dis->itemData);
    }
    COMBOEX_DumpItem (item);

    xbase = CBE_STARTOFFSET;
    if (item->mask & CBEIF_INDENT)
        xbase += (item->iIndent * CBE_INDENT);
    if (item->mask & CBEIF_IMAGE) {
	ImageList_GetImageInfo(infoPtr->himl, item->iImage, &iinfo);
	xioff = (iinfo.rcImage.right - iinfo.rcImage.left + CBE_SEP);
    }

    switch (dis->itemAction) {
    case ODA_FOCUS:
        if (dis->itemState & ODS_SELECTED /*1*/) {
	    if ((item->mask & CBEIF_TEXT) && item->pszText) {
	        len = strlenW (item->pszText);
		GetTextExtentPointW (dis->hDC, item->pszText, len, &txtsize);
		rect.left = xbase + xioff - 1;
	        rect.top = dis->rcItem.top - 1 +
		  (dis->rcItem.bottom - dis->rcItem.top - txtsize.cy) / 2;
		rect.right = rect.left + txtsize.cx + 2;
		rect.bottom = rect.top + txtsize.cy + 2;
		TRACE("drawing item %d focus, rect=(%d,%d)-(%d,%d)\n",
		      dis->itemID, rect.left, rect.top,
		      rect.right, rect.bottom);
		DrawFocusRect(dis->hDC, &rect);
	    }
	}
        break;
    case ODA_SELECT:
    case ODA_DRAWENTIRE:
        drawimage = -1;
	if (item->mask & CBEIF_IMAGE) drawimage = item->iImage;
	if ((dis->itemState & ODS_SELECTED) && 
	    (item->mask & CBEIF_SELECTEDIMAGE))
	        drawimage = item->iSelectedImage;
	if (drawimage != -1) {
	    ImageList_Draw (infoPtr->himl, drawimage, dis->hDC, 
			    xbase, dis->rcItem.top, 
			    (dis->itemState & ODS_SELECTED) ? 
			    ILD_SELECTED : ILD_NORMAL);
	}
	if ((item->mask & CBEIF_TEXT) && item->pszText) {
	    len = strlenW (item->pszText);
	    GetTextExtentPointW (dis->hDC, item->pszText, len, &txtsize);
	    nbkc = GetSysColor ((dis->itemState & ODS_SELECTED) ?
			        COLOR_HIGHLIGHT : COLOR_WINDOW);
	    SetBkColor (dis->hDC, nbkc);
	    ntxc = GetSysColor ((dis->itemState & ODS_SELECTED) ?
			        COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT);
	    SetTextColor (dis->hDC, ntxc);
	    x = xbase + xioff;
	    y = dis->rcItem.top +
	        (dis->rcItem.bottom - dis->rcItem.top - txtsize.cy) / 2;
	    rect.left = x;
	    rect.right = x + txtsize.cx;
	    rect.top = y;
	    rect.bottom = y + txtsize.cy;
	    TRACE("drawing item %d text, rect=(%d,%d)-(%d,%d)\n",
		  dis->itemID, rect.left, rect.top, rect.right, rect.bottom);
	    ExtTextOutW (dis->hDC, x, y, ETO_OPAQUE | ETO_CLIPPED,
			 &rect, item->pszText, len, 0);
	    if (dis->itemState & ODS_FOCUS) {
	        rect.top -= 1;
		rect.bottom += 1;
		rect.left -= 1;
		rect.right += 2;
		TRACE("drawing item %d focus, rect=(%d,%d)-(%d,%d)\n",
		      dis->itemID, rect.left, rect.top, rect.right, rect.bottom);
		DrawFocusRect (dis->hDC, &rect);
	    }
	}
	break;
    default:
        FIXME("unknown action hwnd=%08x, wparam=%08x, lparam=%08lx, action=%d\n", 
	      hwnd, wParam, lParam, dis->itemAction);
    }

    return 0;
}


static LRESULT
COMBOEX_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);

    if (infoPtr->hwndCombo)
	DestroyWindow (infoPtr->hwndCombo);

    if (infoPtr->items) {
        CBE_ITEMDATA *this, *next;

	this = infoPtr->items;
	while (this) {
	    next = (CBE_ITEMDATA *)this->next;
	    if ((this->mask & CBEIF_TEXT) && this->pszText)
	        COMCTL32_Free (this->pszText);
	    COMCTL32_Free (this);
	    this = next;
	}
    }

    DeleteObject (infoPtr->font);

    /* free comboex info data */
    COMCTL32_Free (infoPtr);
    SetWindowLongA (hwnd, 0, 0);
    return 0;
}


static LRESULT
COMBOEX_MeasureItem (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    /*COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);*/
    MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *) lParam;
    HDC hdc;
    SIZE mysize;

    hdc = GetDC (0);
    GetTextExtentPointA (hdc, "W", 1, &mysize);
    ReleaseDC (0, hdc);
    mis->itemHeight = mysize.cy + CBE_EXTRA;

    TRACE("adjusted height hwnd=%08x, height=%d\n",
	  hwnd, mis->itemHeight);

    return 0;
}


static LRESULT
COMBOEX_Size (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);
    RECT rect;

    GetClientRect (hwnd, &rect);

    MoveWindow (infoPtr->hwndCombo, 0, 0, rect.right -rect.left,
		  rect.bottom - rect.top, TRUE);

    return 0;
}


static LRESULT
COMBOEX_WindowPosChanging (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COMBOEX_INFO *infoPtr = COMBOEX_GetInfoPtr (hwnd);
    LRESULT ret;
    RECT cbx_wrect, cbx_crect, cb_wrect;
    UINT width;
    WINDOWPOS *wp = (WINDOWPOS *)lParam;

    ret = DefWindowProcA (hwnd, WM_WINDOWPOSCHANGING, wParam, lParam);
    GetWindowRect (hwnd, &cbx_wrect);
    GetClientRect (hwnd, &cbx_crect);
    GetWindowRect (infoPtr->hwndCombo, &cb_wrect);

    /* width is winpos value + border width of comboex */
    width = wp->cx
            + cbx_wrect.right-cbx_wrect.left 
            - (cbx_crect.right - cbx_crect.left); 

    TRACE("EX window=(%d,%d)-(%d,%d), client=(%d,%d)-(%d,%d)\n",
	  cbx_wrect.left, cbx_wrect.top, cbx_wrect.right, cbx_wrect.bottom,
	  cbx_crect.left, cbx_crect.top, cbx_crect.right, cbx_crect.bottom);
    TRACE("CB window=(%d,%d)-(%d,%d), EX setting=(0,0)-(%d,%d)\n",
	  cb_wrect.left, cb_wrect.top, cb_wrect.right, cb_wrect.bottom,
	  width, cb_wrect.bottom-cb_wrect.top);

    SetWindowPos (infoPtr->hwndCombo, HWND_TOP, 0, 0,
		  width,
		  cb_wrect.bottom-cb_wrect.top,
		  SWP_NOACTIVATE);

    return 0;
}


static LRESULT WINAPI
COMBOEX_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TRACE("hwnd=%x msg=%x wparam=%x lParam=%lx\n", hwnd, uMsg, wParam, lParam);
    if (!COMBOEX_GetInfoPtr (hwnd) && (uMsg != WM_CREATE))
        return DefWindowProcA (hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
/*	case CBEM_DELETEITEM: */

	case CBEM_GETCOMBOCONTROL:
	    return COMBOEX_GetComboControl (hwnd, wParam, lParam);

	case CBEM_GETEDITCONTROL:
	    return COMBOEX_GetEditControl (hwnd, wParam, lParam);

	case CBEM_GETEXTENDEDSTYLE:
	    return COMBOEX_GetExtendedStyle (hwnd, wParam, lParam);

	case CBEM_GETIMAGELIST:
	    return COMBOEX_GetImageList (hwnd, wParam, lParam);

/*	case CBEM_GETITEMA:
	case CBEM_GETITEMW:
	case CBEM_GETUNICODEFORMAT:
	case CBEM_HASEDITCHANGED:
*/

	case CBEM_INSERTITEMA:
	    return COMBOEX_InsertItemA (hwnd, wParam, lParam);

	case CBEM_INSERTITEMW:
	    return COMBOEX_InsertItemW (hwnd, wParam, lParam);

	case CBEM_SETEXSTYLE:	/* FIXME: obsoleted, should be the same as: */
	case CBEM_SETEXTENDEDSTYLE:
	    return COMBOEX_SetExtendedStyle (hwnd, wParam, lParam);

	case CBEM_SETIMAGELIST:
	    return COMBOEX_SetImageList (hwnd, wParam, lParam);

	case CBEM_SETITEMA:
	    return COMBOEX_SetItemA (hwnd, wParam, lParam);

/*	case CBEM_SETITEMW:
	case CBEM_SETUNICODEFORMAT:
*/

	case CB_DELETESTRING:
	case CB_FINDSTRINGEXACT:
	case CB_GETCOUNT:
	case CB_GETCURSEL:
	case CB_GETDROPPEDCONTROLRECT:
	case CB_GETDROPPEDSTATE:
	case CB_GETITEMDATA:
	case CB_GETITEMHEIGHT:
	case CB_GETLBTEXT:
	case CB_GETLBTEXTLEN:
	case CB_GETEXTENDEDUI:
	case CB_LIMITTEXT:
	case CB_RESETCONTENT:
	case CB_SELECTSTRING:
	case CB_SETCURSEL:
	case CB_SETDROPPEDWIDTH:
	case CB_SETEXTENDEDUI:
	case CB_SETITEMDATA:
	case CB_SHOWDROPDOWN:
	case WM_SETTEXT:
	case WM_GETTEXT:
	    return COMBOEX_Forward (hwnd, uMsg, wParam, lParam);

	case CB_SETITEMHEIGHT:
	    return COMBOEX_SetItemHeight (hwnd, wParam, lParam);


	case WM_CREATE:
	    return COMBOEX_Create (hwnd, wParam, lParam);

        case WM_DRAWITEM:
            return COMBOEX_DrawItem (hwnd, wParam, lParam);

	case WM_DESTROY:
	    return COMBOEX_Destroy (hwnd, wParam, lParam);

        case WM_MEASUREITEM:
            return COMBOEX_MeasureItem (hwnd, wParam, lParam);

	case WM_SIZE:
	    return COMBOEX_Size (hwnd, wParam, lParam);

        case WM_WINDOWPOSCHANGING:
	    return COMBOEX_WindowPosChanging (hwnd, wParam, lParam);

	default:
	    if (uMsg >= WM_USER)
		ERR("unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
COMBOEX_Register (void)
{
    WNDCLASSA wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = (WNDPROC)COMBOEX_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(COMBOEX_INFO *);
    wndClass.hCursor       = LoadCursorA (0, IDC_ARROWA);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = WC_COMBOBOXEXA;
 
    RegisterClassA (&wndClass);
}


VOID
COMBOEX_Unregister (void)
{
    UnregisterClassA (WC_COMBOBOXEXA, (HINSTANCE)NULL);
}

