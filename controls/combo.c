/*
 * Combo controls
 * 
 * Copyright 1997 Alex Korobka
 * 
 * FIXME: roll up in Netscape 3.01.
 */

#include <stdio.h>
#include <string.h>

#include "windows.h"
#include "sysmetrics.h"
#include "win.h"
#include "spy.h"
#include "user.h"
#include "graphics.h"
#include "heap.h"
#include "combo.h"
#include "drive.h"
#include "debug.h"

  /* bits in the dwKeyData */
#define KEYDATA_ALT             0x2000
#define KEYDATA_PREVSTATE       0x4000

/*
 * Additional combo box definitions
 */

#define CB_GETPTR( wnd )      (*(LPHEADCOMBO*)((wnd)->wExtra))
#define CB_NOTIFY( lphc, code ) \
	(SendMessage32A( (lphc)->owner, WM_COMMAND, \
			 MAKEWPARAM((lphc)->self->wIDmenu, (code)), (lphc)->self->hwndSelf))
#define CB_GETEDITTEXTLENGTH( lphc ) \
	(SendMessage32A( (lphc)->hWndEdit, WM_GETTEXTLENGTH, 0, 0 ))

static HBITMAP16 	hComboBmp = 0;
static UINT16		CBitHeight, CBitWidth;
static UINT16		CBitOffset = 8;

/***********************************************************************
 *           COMBO_Init
 *
 * Load combo button bitmap.
 */
static BOOL32 COMBO_Init()
{
  HDC16		hDC;
  
  if( hComboBmp ) return TRUE;
  if( (hDC = CreateCompatibleDC16(0)) )
  {
    BOOL32	bRet = FALSE;
    if( (hComboBmp = LoadBitmap16(0, MAKEINTRESOURCE(OBM_COMBO))) )
    {
      BITMAP16      bm;
      HBITMAP16     hPrevB;
      RECT16        r;

      GetObject16( hComboBmp, sizeof(bm), &bm );
      CBitHeight = bm.bmHeight;
      CBitWidth  = bm.bmWidth;

      dprintf_info(combo, "combo bitmap [%i,%i]\n", CBitWidth, CBitHeight );

      hPrevB = SelectObject16( hDC, hComboBmp);
      SetRect16( &r, 0, 0, CBitWidth, CBitHeight );
      InvertRect16( hDC, &r );
      SelectObject16( hDC, hPrevB );
      bRet = TRUE;
    }
    DeleteDC16( hDC );
    return bRet;
  }
  return FALSE;
}

/***********************************************************************
 *           COMBO_NCCreate
 */
static LRESULT COMBO_NCCreate(WND* wnd, LPARAM lParam)
{
   LPHEADCOMBO 		lphc;

   if ( wnd && COMBO_Init() &&
      (lphc = HeapAlloc(GetProcessHeap(), 0, sizeof(HEADCOMBO))) )
   {
	LPCREATESTRUCT32A     lpcs = (CREATESTRUCT32A*)lParam;
       
	memset( lphc, 0, sizeof(HEADCOMBO) );
       *(LPHEADCOMBO*)wnd->wExtra = lphc;

       /* some braindead apps do try to use scrollbar/border flags */

	lphc->dwStyle = (lpcs->style & ~(WS_BORDER | WS_HSCROLL | WS_VSCROLL));
	wnd->dwStyle &= ~(WS_BORDER | WS_HSCROLL | WS_VSCROLL);

	if( !(lpcs->style & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) )
              lphc->dwStyle |= CBS_HASSTRINGS;
	if( !(wnd->dwExStyle & WS_EX_NOPARENTNOTIFY) )
	      lphc->wState |= CBF_NOTIFY;

	dprintf_info(combo, "COMBO_NCCreate: [0x%08x], style = %08x\n", 
						(UINT32)lphc, lphc->dwStyle );

	return (LRESULT)(UINT32)wnd->hwndSelf; 
    }
    return (LRESULT)FALSE;
}

/***********************************************************************
 *           COMBO_NCDestroy
 */
static LRESULT COMBO_NCDestroy( LPHEADCOMBO lphc )
{

   if( lphc )
   {
       WND*		wnd = lphc->self;

       dprintf_info(combo,"Combo [%04x]: freeing storage\n", CB_HWND(lphc));

       if( (CB_GETTYPE(lphc) != CBS_SIMPLE) && lphc->hWndLBox ) 
   	   DestroyWindow32( lphc->hWndLBox );

       HeapFree( GetProcessHeap(), 0, lphc );
       wnd->wExtra[0] = 0;
   }
   return 0;
}

/***********************************************************************
 *           CBCalcPlacement
 *
 * Set up component coordinates given valid lphc->RectCombo.
 */
static void CBCalcPlacement( LPHEADCOMBO lphc, 
			     LPRECT16 lprEdit, LPRECT16 lprButton, LPRECT16 lprLB )
{
   RECT16	rect = lphc->RectCombo;
   SIZE16	size;

   /* get combo height and width */

   if( lphc->editHeight )
       size.cy = (INT16)lphc->editHeight;
   else
   {
       HDC16	hDC = GetDC16( lphc->self->hwndSelf );
       HFONT16	hPrevFont = (HFONT16)0;

       if( lphc->hFont ) hPrevFont = SelectObject16( hDC, lphc->hFont );
   
       GetTextExtentPoint16( hDC, "0", 1, &size);

       size.cy += size.cy / 4 + 4 * SYSMETRICS_CYBORDER;

       if( hPrevFont ) SelectObject16( hDC, hPrevFont );
       ReleaseDC16( lphc->self->hwndSelf, hDC );
   }
   size.cx = rect.right - rect.left;

   if( CB_OWNERDRAWN(lphc) )
   {
       UINT16	u = lphc->RectEdit.bottom - lphc->RectEdit.top;

       if( lphc->wState & CBF_MEASUREITEM ) /* first initialization */
       {
	 MEASUREITEMSTRUCT32        mi32;

	 lphc->wState &= ~CBF_MEASUREITEM;
	 mi32.CtlType = ODT_COMBOBOX;
	 mi32.CtlID   = lphc->self->wIDmenu;
	 mi32.itemID  = -1;
	 mi32.itemWidth  = size.cx;
	 mi32.itemHeight = size.cy - 6;	/* ownerdrawn cb is taller */
	 mi32.itemData   = 0;
	 SendMessage32A(lphc->owner, WM_MEASUREITEM, 
				(WPARAM32)mi32.CtlID, (LPARAM)&mi32);
	 u = 6 + (UINT16)mi32.itemHeight;
       }
       size.cy = u;
   }

   /* calculate text and button placement */

   lprEdit->left = lprEdit->top = lprButton->top = 0;
   if( CB_GETTYPE(lphc) == CBS_SIMPLE ) 	/* no button */
       lprButton->left = lprButton->right = lprButton->bottom = 0;
   else
   {
       INT32	i = size.cx - CBitWidth - 10;	/* seems ok */

       lprButton->right = size.cx;
       lprButton->left = (INT16)i;
       lprButton->bottom = lprButton->top + size.cy;

       if( i < 0 ) size.cx = 0;
       else size.cx = (INT16)i;
   }

   if( CB_GETTYPE(lphc) == CBS_DROPDOWN )
   {
       size.cx -= CBitOffset;
       if( size.cx < 0 ) size.cx = 0;
   }

   lprEdit->right = size.cx; lprEdit->bottom = size.cy;

   /* listbox placement */

   lprLB->left = ( CB_GETTYPE(lphc) == CBS_DROPDOWNLIST ) ? 0 : CBitOffset;
   lprLB->top = lprEdit->bottom - SYSMETRICS_CYBORDER;
   lprLB->right = rect.right - rect.left;
   lprLB->bottom = rect.bottom - rect.top;

   if( lphc->droppedWidth > (lprLB->right - lprLB->left) )
       lprLB->right = lprLB->left + (INT16)lphc->droppedWidth;

dprintf_info(combo,"Combo [%04x]: (%i,%i-%i,%i) placement\n\ttext\t= (%i,%i-%i,%i)\
\n\tbutton\t= (%i,%i-%i,%i)\n\tlbox\t= (%i,%i-%i,%i)\n", CB_HWND(lphc),
lphc->RectCombo.left, lphc->RectCombo.top, lphc->RectCombo.right, lphc->RectCombo.bottom,
lprEdit->left, lprEdit->top, lprEdit->right, lprEdit->bottom,
lprButton->left, lprButton->top, lprButton->right, lprButton->bottom, 
lprLB->left, lprLB->top, lprLB->right, lprLB->bottom );

}

/***********************************************************************
 *           CBGetDroppedControlRect32
 */
static void CBGetDroppedControlRect32( LPHEADCOMBO lphc, LPRECT32 lpRect)
{
   lpRect->left = lphc->RectCombo.left +
                      (lphc->wState & CBF_EDIT) ? CBitOffset : 0;
   lpRect->top = lphc->RectCombo.top + lphc->RectEdit.bottom -
                                             SYSMETRICS_CYBORDER;
   lpRect->right = lphc->RectCombo.right;
   lpRect->bottom = lphc->RectCombo.bottom - SYSMETRICS_CYBORDER;
}

/***********************************************************************
 *           COMBO_Create
 */
static LRESULT COMBO_Create( LPHEADCOMBO lphc, WND* wnd, LPARAM lParam)
{
  static char clbName[] = "ComboLBox";
  static char editName[] = "Edit";

  LPCREATESTRUCT32A  lpcs = (CREATESTRUCT32A*)lParam;
  
  if( !CB_GETTYPE(lphc) ) lphc->dwStyle |= CBS_SIMPLE;
  else if( CB_GETTYPE(lphc) != CBS_DROPDOWNLIST ) lphc->wState |= CBF_EDIT;

  lphc->self  = wnd;
  lphc->owner = lpcs->hwndParent;

  /* M$ IE 3.01 actually creates (and rapidly destroys) an ownerless combobox */

  if( lphc->owner || !(lpcs->style & WS_VISIBLE) )
  {
      UINT32	lbeStyle;
      RECT16	editRect, btnRect, lbRect;

      GetWindowRect16( wnd->hwndSelf, &lphc->RectCombo );

      lphc->wState |= CBF_MEASUREITEM;
      CBCalcPlacement( lphc, &editRect, &btnRect, &lbRect );
      lphc->RectButton = btnRect;
      lphc->droppedWidth = lphc->editHeight = 0;

      /* create listbox popup */

      lbeStyle = (LBS_NOTIFY | WS_BORDER | WS_CLIPSIBLINGS) | 
                 (lpcs->style & (WS_VSCROLL | CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE));

      if( lphc->dwStyle & CBS_SORT )
	lbeStyle |= LBS_SORT;
      if( lphc->dwStyle & CBS_HASSTRINGS )
	lbeStyle |= LBS_HASSTRINGS;
      if( lphc->dwStyle & CBS_NOINTEGRALHEIGHT )
	lbeStyle |= LBS_NOINTEGRALHEIGHT;
      if( lphc->dwStyle & CBS_DISABLENOSCROLL )
	lbeStyle |= LBS_DISABLENOSCROLL;
  
      if( CB_GETTYPE(lphc) == CBS_SIMPLE ) 	/* child listbox */
	lbeStyle |= WS_CHILD | WS_VISIBLE;
      else					/* popup listbox */
      {
	lbeStyle |= WS_POPUP;
	OffsetRect16( &lbRect, lphc->RectCombo.left, lphc->RectCombo.top );
      }

     /* Dropdown ComboLBox is not a child window and we cannot pass 
      * ID_CB_LISTBOX directly because it will be treated as a menu handle.
      */

      lphc->hWndLBox = CreateWindowEx32A( 0, clbName, NULL, lbeStyle, 
		        lbRect.left + SYSMETRICS_CXBORDER, 
		        lbRect.top + SYSMETRICS_CYBORDER, 
			lbRect.right - lbRect.left - 2 * SYSMETRICS_CXBORDER, 
			lbRect.bottom - lbRect.top - 2 * SYSMETRICS_CYBORDER, 
			lphc->self->hwndSelf, 
		       (lphc->dwStyle & CBS_DROPDOWN)? (HMENU32)0 : (HMENU32)ID_CB_LISTBOX,
			lphc->self->hInstance, (LPVOID)lphc );
      if( lphc->hWndLBox )
      {
	  BOOL32	bEdit = TRUE;
	  lbeStyle = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NOHIDESEL | ES_LEFT;
	  if( lphc->wState & CBF_EDIT ) 
	  {
	      if( lphc->dwStyle & CBS_OEMCONVERT )
		  lbeStyle |= ES_OEMCONVERT;
	      if( lphc->dwStyle & CBS_AUTOHSCROLL )
		  lbeStyle |= ES_AUTOHSCROLL;
	      if( lphc->dwStyle & CBS_LOWERCASE )
		  lbeStyle |= ES_LOWERCASE;
	      else if( lphc->dwStyle & CBS_UPPERCASE )
		  lbeStyle |= ES_UPPERCASE;
	      lphc->hWndEdit = CreateWindowEx32A( 0, editName, NULL, lbeStyle,
		  	editRect.left, editRect.top, editRect.right - editRect.left,
			editRect.bottom - editRect.top, lphc->self->hwndSelf, 
			(HMENU32)ID_CB_EDIT, lphc->self->hInstance, NULL );
	      if( !lphc->hWndEdit ) bEdit = FALSE;
	  } 

          if( bEdit )
	  {
	      lphc->RectEdit = editRect;
	      if( CB_GETTYPE(lphc) != CBS_SIMPLE )
	      {
		lphc->wState |= CBF_NORESIZE;
		SetWindowPos32( wnd->hwndSelf, 0, 0, 0, 
				lphc->RectCombo.right - lphc->RectCombo.left,
				lphc->RectEdit.bottom - lphc->RectEdit.top,
				SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
		lphc->wState &= ~CBF_NORESIZE;
	      }
	      dprintf_info(combo,"COMBO_Create: init done\n");
	      return wnd->hwndSelf;
	  }
	  dprintf_err(combo, "COMBO_Create: edit control failure.\n");
      } else dprintf_err(combo, "COMBO_Create: listbox failure.\n");
  } else dprintf_err(combo, "COMBO_Create: no owner for visible combo.\n");

  /* CreateWindow() will send WM_NCDESTROY to cleanup */

  return -1;
}

/***********************************************************************
 *           CBPaintButton
 *
 * Paint combo button (normal, pressed, and disabled states).
 */
static void CBPaintButton(LPHEADCOMBO lphc, HDC16 hdc)
{
    RECT32 	r;
    HBRUSH32 	hPrevBrush;
    UINT32 	x, y;
    BOOL32 	bBool;

    if( lphc->wState & CBF_NOREDRAW ) return;

    hPrevBrush = (HBRUSH32)SelectObject32(hdc, GetSysColorBrush32(COLOR_BTNFACE));
    CONV_RECT16TO32( &lphc->RectButton, &r );

    Rectangle32(hdc, r.left, r.top, r.right, r.bottom );
    InflateRect32( &r, -1, -1 );
    if( (bBool = lphc->wState & CBF_BUTTONDOWN) )
    {
	GRAPH_DrawReliefRect(hdc, &r, 1, 0, TRUE);
	OffsetRect32( &r, 1, 1 );
    } else GRAPH_DrawReliefRect(hdc, &r, 1, 2, FALSE);

    x = (r.left + r.right - CBitWidth) >> 1;
    y = (r.top + r.bottom - CBitHeight) >> 1;

    InflateRect32( &r, -3, -3 );
    if( (bBool = CB_DISABLED(lphc)) )
    {
        GRAPH_SelectClipMask(hdc, hComboBmp, x + 1, y + 1 );
        FillRect32(hdc, &r, (HBRUSH32)GetStockObject32(WHITE_BRUSH));
    }

    GRAPH_SelectClipMask(hdc, hComboBmp, x, y );
    FillRect32(hdc, &r, (HBRUSH32)GetStockObject32((bBool) ? GRAY_BRUSH : BLACK_BRUSH));

    GRAPH_SelectClipMask(hdc, (HBITMAP32)0, 0, 0);
    SelectObject32( hdc, hPrevBrush );
}

/***********************************************************************
 *           CBPaintText
 *
 * Paint CBS_DROPDOWNLIST text field / update edit control contents.
 */
static void CBPaintText(LPHEADCOMBO lphc, HDC16 hdc)
{
   INT32	id, size = 0;
   LPSTR	pText = NULL;

   if( lphc->wState & CBF_NOREDRAW ) return;

   /* follow Windows combobox that sends a bunch of text 
    * inquiries to its listbox while processing WM_PAINT. */

   if( (id = SendMessage32A(lphc->hWndLBox, LB_GETCURSEL32, 0, 0) ) != LB_ERR )
   {
        size = SendMessage32A( lphc->hWndLBox, LB_GETTEXTLEN32, id, 0);
        if( (pText = HeapAlloc( GetProcessHeap(), 0, size + 1)) )
	{
	    SendMessage32A( lphc->hWndLBox, LB_GETTEXT32, (WPARAM32)id, (LPARAM)pText );
	    pText[size] = '\0';	/* just in case */
	} else return;
   }

   if( lphc->wState & CBF_EDIT )
   {
	if( CB_HASSTRINGS(lphc) ) SetWindowText32A( lphc->hWndEdit, pText );
	if( lphc->wState & CBF_FOCUSED ) 
	    SendMessage32A( lphc->hWndEdit, EM_SETSEL32, 0, (LPARAM)(-1));
   }
   else /* paint text field ourselves */
   {
        HBRUSH32 hPrevBrush = 0;
	HDC32	 hDC = hdc;

	if( !hDC ) 
	{
	    if ((hDC = GetDC32(lphc->self->hwndSelf)))
            {
                HBRUSH32 hBrush = SendMessage32A( lphc->owner,
                                                  WM_CTLCOLORLISTBOX, 
                                                  hDC, lphc->self->hwndSelf );
                hPrevBrush = SelectObject32( hDC, 
                           (hBrush) ? hBrush : GetStockObject32(WHITE_BRUSH) );
            }
	}
	if( hDC )
	{
	    RECT32	rect;
	    UINT16	itemState;
	    HFONT32	hPrevFont = (lphc->hFont) ? SelectObject32(hDC, lphc->hFont) : 0;

	    PatBlt32( hDC, (rect.left = lphc->RectEdit.left + SYSMETRICS_CXBORDER),
			   (rect.top = lphc->RectEdit.top + SYSMETRICS_CYBORDER),
			   (rect.right = lphc->RectEdit.right - SYSMETRICS_CXBORDER),
			   (rect.bottom = lphc->RectEdit.bottom - SYSMETRICS_CYBORDER) - 1, PATCOPY );
	    InflateRect32( &rect, -1, -1 );

	    if( lphc->wState & CBF_FOCUSED && 
	        !(lphc->wState & CBF_DROPPED) )
	    {
		/* highlight */

		FillRect32( hDC, &rect, GetSysColorBrush32(COLOR_HIGHLIGHT) );
                SetBkColor32( hDC, GetSysColor32( COLOR_HIGHLIGHT ) );
                SetTextColor32( hDC, GetSysColor32( COLOR_HIGHLIGHTTEXT ) );
		itemState = ODS_SELECTED | ODS_FOCUS;
	    } else itemState = 0;

	    if( CB_OWNERDRAWN(lphc) )
	    {
		DRAWITEMSTRUCT32 dis;

		if( lphc->self->dwStyle & WS_DISABLED ) itemState |= ODS_DISABLED;

		dis.CtlType	= ODT_COMBOBOX;
		dis.CtlID	= lphc->self->wIDmenu;
		dis.hwndItem	= lphc->self->hwndSelf;
		dis.itemAction	= ODA_DRAWENTIRE;
		dis.itemID	= id;
		dis.itemState	= itemState;
		dis.hDC		= hDC;
		dis.rcItem	= rect;
		dis.itemData	= SendMessage32A( lphc->hWndLBox, LB_GETITEMDATA32, 
						  		  (WPARAM32)id, 0 );
		SendMessage32A( lphc->owner, WM_DRAWITEM, 
				lphc->self->wIDmenu, (LPARAM)&dis );
	    }
	    else
	    {
		ExtTextOut32A( hDC, rect.left + 1, rect.top + 1,
                               ETO_OPAQUE | ETO_CLIPPED, &rect,
                               (pText) ? pText : "" , size, NULL );
		if(lphc->wState & CBF_FOCUSED && !(lphc->wState & CBF_DROPPED))
		    DrawFocusRect32( hDC, &rect );
	    }

	    if( hPrevFont ) SelectObject32(hDC, hPrevFont );
	    if( !hdc ) 
	    {
		if( hPrevBrush ) SelectObject32( hDC, hPrevBrush );
		ReleaseDC32( lphc->self->hwndSelf, hDC );
	    }
	}
   }
   HeapFree( GetProcessHeap(), 0, pText );
}

/***********************************************************************
 *           COMBO_Paint
 */
static LRESULT COMBO_Paint(LPHEADCOMBO lphc, HDC16 hParamDC)
{
  PAINTSTRUCT16 ps;
  HDC16 	hDC;
  
  hDC = (hParamDC) ? hParamDC
		   : BeginPaint16( lphc->self->hwndSelf, &ps);
  if( hDC && !(lphc->wState & CBF_NOREDRAW) )
  {
      HBRUSH32	hPrevBrush, hBkgBrush;

      hBkgBrush = SendMessage32A( lphc->owner, WM_CTLCOLORLISTBOX,
				  hDC, lphc->self->hwndSelf );
      if( !hBkgBrush ) hBkgBrush = GetStockObject32(WHITE_BRUSH);

      hPrevBrush = SelectObject32( hDC, hBkgBrush );
      if( !IsRectEmpty16(&lphc->RectButton) )
      {
	  /* paint everything to the right of the text field */

	  PatBlt32( hDC, lphc->RectEdit.right, lphc->RectEdit.top,
			 lphc->RectButton.right - lphc->RectEdit.right,
			 lphc->RectEdit.bottom - lphc->RectEdit.top, PATCOPY );
          CBPaintButton( lphc, hDC );
      }

      if( !(lphc->wState & CBF_EDIT) )
      {
	  /* paint text field */

	  GRAPH_DrawRectangle( hDC, lphc->RectEdit.left, lphc->RectEdit.top,
				    lphc->RectEdit.right - lphc->RectEdit.left, 
				    lphc->RectButton.bottom - lphc->RectButton.top,
				    GetSysColorPen32(COLOR_WINDOWFRAME) ); 
	  CBPaintText( lphc, hDC );
      }
      if( hPrevBrush ) SelectObject32( hDC, hPrevBrush );
  }
  if( !hParamDC ) EndPaint16(lphc->self->hwndSelf, &ps);
  return 0;
}

/***********************************************************************
 *           CBUpdateLBox
 *
 * Select listbox entry according to the contents of the edit control.
 */
static INT32 CBUpdateLBox( LPHEADCOMBO lphc )
{
   INT32	length, idx, ret;
   LPSTR	pText = NULL;
   
   idx = ret = LB_ERR;
   length = CB_GETEDITTEXTLENGTH( lphc );
 
   if( length > 0 ) 
       pText = (LPSTR) HeapAlloc( GetProcessHeap(), 0, length + 1);

   dprintf_info(combo,"\tCBUpdateLBox: edit text length %i\n", length );

   if( pText )
   {
       if( length ) GetWindowText32A( lphc->hWndEdit, pText, length + 1);
       else pText[0] = '\0';
       idx = SendMessage32A( lphc->hWndLBox, LB_FINDSTRING32, 
			     (WPARAM32)(-1), (LPARAM)pText );
       if( idx == LB_ERR ) idx = 0;	/* select first item */
       else ret = idx;
       HeapFree( GetProcessHeap(), 0, pText );
   }

   /* select entry */

   SendMessage32A( lphc->hWndLBox, LB_SETCURSEL32, (WPARAM32)idx, 0 );
   
   if( idx >= 0 )
   {
       SendMessage32A( lphc->hWndLBox, LB_SETTOPINDEX32, (WPARAM32)idx, 0 );
       /* probably superfluous but Windows sends this too */
       SendMessage32A( lphc->hWndLBox, LB_SETCARETINDEX32, (WPARAM32)idx, 0 );
   }
   return ret;
}

/***********************************************************************
 *           CBUpdateEdit
 *
 * Copy a listbox entry to the edit control.
 */
static void CBUpdateEdit( LPHEADCOMBO lphc , INT32 index )
{
   INT32	length;
   LPSTR	pText = NULL;

   dprintf_info(combo,"\tCBUpdateEdit: %i\n", index );

   if( index == -1 )
   {
       length = CB_GETEDITTEXTLENGTH( lphc );
       if( length )
       {
           if( (pText = (LPSTR) HeapAlloc( GetProcessHeap(), 0, length + 1)) )
           {
 	   	GetWindowText32A( lphc->hWndEdit, pText, length + 1 );
	   	index = SendMessage32A( lphc->hWndLBox, LB_FINDSTRING32,
				        (WPARAM32)(-1), (LPARAM)pText );
	   	HeapFree( GetProcessHeap(), 0, pText );
           }
       }
   }

   if( index >= 0 ) /* got an entry */
   {
       length = SendMessage32A( lphc->hWndLBox, LB_GETTEXTLEN32, (WPARAM32)index, 0);
       if( length )
       {
	   if( (pText = (LPSTR) HeapAlloc( GetProcessHeap(), 0, length + 1)) )
	   {
		SendMessage32A( lphc->hWndLBox, LB_GETTEXT32, 
				(WPARAM32)index, (LPARAM)pText );
		SendMessage32A( lphc->hWndEdit, WM_SETTEXT, 0, (LPARAM)pText );
		SendMessage32A( lphc->hWndEdit, EM_SETSEL32, 0, (LPARAM)(-1) );
		HeapFree( GetProcessHeap(), 0, pText );
	   }
       }
   }
}

/***********************************************************************
 *           CBDropDown
 * 
 * Show listbox popup.
 */
static void CBDropDown( LPHEADCOMBO lphc )
{
   INT32	index;
   RECT16	rect;
   LPRECT16	pRect = NULL;

   dprintf_info(combo,"Combo [%04x]: drop down\n", CB_HWND(lphc));

   CB_NOTIFY( lphc, CBN_DROPDOWN );

   /* set selection */

   lphc->wState |= CBF_DROPPED;
   if( CB_GETTYPE(lphc) == CBS_DROPDOWN )
   {
       index = CBUpdateLBox( lphc );
       if( !(lphc->wState & CBF_CAPTURE) ) CBUpdateEdit( lphc, index );
   }
   else
   {
       index = SendMessage32A( lphc->hWndLBox, LB_GETCURSEL32, 0, 0 );
       if( index == LB_ERR ) index = 0;
       SendMessage32A( lphc->hWndLBox, LB_SETTOPINDEX32, (WPARAM32)index, 0 );
       SendMessage32A( lphc->hWndLBox, LB_CARETON32, 0, 0 );
       pRect = &lphc->RectEdit;
   }

   /* now set popup position */

   GetWindowRect16( lphc->self->hwndSelf, &rect );
   
   rect.top += lphc->RectEdit.bottom - lphc->RectEdit.top - SYSMETRICS_CYBORDER;
   rect.bottom = rect.top + lphc->RectCombo.bottom - 
			    lphc->RectCombo.top - SYSMETRICS_CYBORDER;
   rect.right = rect.left + lphc->RectCombo.right - lphc->RectCombo.left;
   rect.left += ( CB_GETTYPE(lphc) == CBS_DROPDOWNLIST ) ? 0 : CBitOffset;

   SetWindowPos32( lphc->hWndLBox, HWND_TOP, rect.left, rect.top, 
		 rect.right - rect.left, rect.bottom - rect.top, 
		 SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOREDRAW);

   if( !(lphc->wState & CBF_NOREDRAW) )
       if( pRect )
           RedrawWindow16( lphc->self->hwndSelf, pRect, 0, RDW_INVALIDATE | 
			   RDW_ERASE | RDW_UPDATENOW | RDW_NOCHILDREN );
   ShowWindow32( lphc->hWndLBox, SW_SHOWNA );
}

/***********************************************************************
 *           CBRollUp
 *
 * Hide listbox popup.
 */
static void CBRollUp( LPHEADCOMBO lphc, BOOL32 ok, BOOL32 bButton )
{
   HWND32	hWnd = lphc->self->hwndSelf;

   CB_NOTIFY( lphc, (ok) ? CBN_SELENDOK : CBN_SELENDCANCEL );

   if( IsWindow32( hWnd ) && CB_GETTYPE(lphc) != CBS_SIMPLE )
   {

       dprintf_info(combo,"Combo [%04x]: roll up [%i]\n", CB_HWND(lphc), (INT32)ok );

       /* always send WM_LBUTTONUP? */
       SendMessage32A( lphc->hWndLBox, WM_LBUTTONUP, 0, (LPARAM)(-1) );

       if( lphc->wState & CBF_DROPPED ) 
       {
	   RECT16	rect;

	   lphc->wState &= ~CBF_DROPPED;
	   ShowWindow32( lphc->hWndLBox, SW_HIDE );

	   if( CB_GETTYPE(lphc) == CBS_DROPDOWN )
	   {
	       INT32 index = SendMessage32A( lphc->hWndLBox, LB_GETCURSEL32, 0, 0 );
	       CBUpdateEdit( lphc, index );
	       rect = lphc->RectButton;
	   }
	   else 
           {
	       if( bButton )
	           UnionRect16( &rect, &lphc->RectButton,
				       &lphc->RectEdit );
	       else
		   rect = lphc->RectEdit;
	       bButton = TRUE;
	   }

	   if( bButton && !(lphc->wState & CBF_NOREDRAW) )
	       RedrawWindow16( hWnd, &rect, 0, RDW_INVALIDATE | 
			       RDW_ERASE | RDW_UPDATENOW | RDW_NOCHILDREN );
	   CB_NOTIFY( lphc, CBN_CLOSEUP );
       }
   }
}

/***********************************************************************
 *           COMBO_FlipListbox
 *
 * Used by the ComboLBox to show/hide itself in response to VK_F4, etc...
 */
BOOL32 COMBO_FlipListbox( LPHEADCOMBO lphc, BOOL32 bRedrawButton )
{
   if( lphc->wState & CBF_DROPPED )
   {
       CBRollUp( lphc, TRUE, bRedrawButton );
       return FALSE;
   }

   CBDropDown( lphc );
   return TRUE;
}

/***********************************************************************
 *           COMBO_GetLBWindow
 *
 * Edit control helper.
 */
HWND32 COMBO_GetLBWindow( WND* pWnd )
{
  LPHEADCOMBO       lphc = CB_GETPTR(pWnd);
  if( lphc ) return lphc->hWndLBox;
  return 0;
}


/***********************************************************************
 *           CBRepaintButton
 */
static void CBRepaintButton( LPHEADCOMBO lphc )
{
   HDC32        hDC = GetDC32( lphc->self->hwndSelf );

   if( hDC )
   {
       CBPaintButton( lphc, (HDC16)hDC );
       ReleaseDC32( lphc->self->hwndSelf, hDC );
   }
}

/***********************************************************************
 *           COMBO_SetFocus
 */
static void COMBO_SetFocus( LPHEADCOMBO lphc )
{
   if( !(lphc->wState & CBF_FOCUSED) )
   {
       if( CB_GETTYPE(lphc) == CBS_DROPDOWNLIST )
           SendMessage32A( lphc->hWndLBox, LB_CARETON32, 0, 0 );

       if( lphc->wState & CBF_EDIT )
           SendMessage32A( lphc->hWndEdit, EM_SETSEL32, 0, (LPARAM)(-1) );
       lphc->wState |= CBF_FOCUSED;
       if( !(lphc->wState & CBF_EDIT) ) CBPaintText( lphc, 0 );

       CB_NOTIFY( lphc, CBN_SETFOCUS );
   }
}

/***********************************************************************
 *           COMBO_KillFocus
 */
static void COMBO_KillFocus( LPHEADCOMBO lphc )
{
   HWND32	hWnd = lphc->self->hwndSelf;

   if( lphc->wState & CBF_FOCUSED )
   {
       SendMessage32A( hWnd, WM_LBUTTONUP, 0, (LPARAM)(-1) );

       CBRollUp( lphc, FALSE, TRUE );
       if( IsWindow32( hWnd ) )
       {
           if( CB_GETTYPE(lphc) == CBS_DROPDOWNLIST )
               SendMessage32A( lphc->hWndLBox, LB_CARETOFF32, 0, 0 );

 	   lphc->wState &= ~CBF_FOCUSED;

           /* redraw text */
           if( lphc->wState & CBF_EDIT )
               SendMessage32A( lphc->hWndEdit, EM_SETSEL32, (WPARAM32)(-1), 0 );
           else CBPaintText( lphc, 0 );

           CB_NOTIFY( lphc, CBN_KILLFOCUS );
       }
   }
}

/***********************************************************************
 *           COMBO_Command
 */
static LRESULT COMBO_Command( LPHEADCOMBO lphc, WPARAM32 wParam, HWND32 hWnd )
{
   if( lphc->wState & CBF_EDIT && lphc->hWndEdit == hWnd )
   {
       /* ">> 8" makes gcc generate jump-table instead of cmp ladder */

       switch( HIWORD(wParam) >> 8 )
       {   
	   case (EN_SETFOCUS >> 8):

		dprintf_info(combo,"Combo [%04x]: edit [%04x] got focus\n", 
				     CB_HWND(lphc), (HWND16)lphc->hWndEdit );

		if( !(lphc->wState & CBF_FOCUSED) ) COMBO_SetFocus( lphc );
	        break;

	   case (EN_KILLFOCUS >> 8):

		dprintf_info(combo,"Combo [%04x]: edit [%04x] lost focus\n",
				      CB_HWND(lphc), (HWND16)lphc->hWndEdit );

		/* NOTE: it seems that Windows' edit control sends an
		 * undocumented message WM_USER + 0x1B instead of this
		 * notification (only when it happens to be a part of 
		 * the combo). ?? - AK.
		 */

		COMBO_KillFocus( lphc );
		break;


	   case (EN_CHANGE >> 8):
		CB_NOTIFY( lphc, CBN_EDITCHANGE );
		CBUpdateLBox( lphc );
		break;

	   case (EN_UPDATE >> 8):
		CB_NOTIFY( lphc, CBN_EDITUPDATE );
		break;

	   case (EN_ERRSPACE >> 8):
		CB_NOTIFY( lphc, CBN_ERRSPACE );
       }
   }
   else if( lphc->hWndLBox == hWnd )
   {
       switch( HIWORD(wParam) )
       {
	   case LBN_ERRSPACE:
		CB_NOTIFY( lphc, CBN_ERRSPACE );
		break;

	   case LBN_DBLCLK:
		CB_NOTIFY( lphc, CBN_DBLCLK );
		break;

	   case LBN_SELCHANGE:
	   case LBN_SELCANCEL:

		dprintf_info(combo,"Combo [%04x]: lbox selection change [%04x]\n", 
						      CB_HWND(lphc), lphc->wState );

		/* do not roll up if selection is being tracked 
		 * by arrowkeys in the dropdown listbox */

		if( (lphc->wState & CBF_DROPPED) && !(lphc->wState & CBF_NOROLLUP) )
		     CBRollUp( lphc, (HIWORD(wParam) == LBN_SELCHANGE), TRUE );
		else lphc->wState &= ~CBF_NOROLLUP;

		CB_NOTIFY( lphc, CBN_SELCHANGE );
		CBPaintText( lphc, 0 );
		/* fall through */

	   case LBN_SETFOCUS:
	   case LBN_KILLFOCUS:
		/* nothing to do here since ComboLBox always resets the focus to its
		 * combo/edit counterpart */
		 break;
       }
   }
   return 0;
}

/***********************************************************************
 *           COMBO_ItemOp
 *
 * Fixup an ownerdrawn item operation and pass it up to the combobox owner.
 */
static LRESULT COMBO_ItemOp32( LPHEADCOMBO lphc, UINT32 msg, 
			       WPARAM32 wParam, LPARAM lParam ) 
{
   HWND32	hWnd = lphc->self->hwndSelf;

   dprintf_info(combo,"Combo [%04x]: ownerdraw op %04x\n", 
				       CB_HWND(lphc), (UINT16)msg );

#define lpIS    ((LPDELETEITEMSTRUCT32)lParam)

   /* two first items are the same in all 4 structs */
   lpIS->CtlType = ODT_COMBOBOX;
   lpIS->CtlID   = lphc->self->wIDmenu;

   switch( msg )	/* patch window handle */
   {
	case WM_DELETEITEM: 
	     lpIS->hwndItem = hWnd; 
#undef  lpIS
	     break;
	case WM_DRAWITEM: 
#define lpIS    ((LPDRAWITEMSTRUCT32)lParam)
	     lpIS->hwndItem = hWnd; 
#undef  lpIS
	     break;
	case WM_COMPAREITEM: 
#define lpIS    ((LPCOMPAREITEMSTRUCT32)lParam)
	     lpIS->hwndItem = hWnd; 
#undef  lpIS
	     break;
   }

   return SendMessage32A( lphc->owner, msg, lphc->self->wIDmenu, lParam );
}

/***********************************************************************
 *           COMBO_GetText
 */
static LRESULT COMBO_GetText( LPHEADCOMBO lphc, UINT32 N, LPSTR lpText)
{
   if( lphc->wState & CBF_EDIT )
       return SendMessage32A( lphc->hWndEdit, WM_GETTEXT, 
			     (WPARAM32)N, (LPARAM)lpText );     

   /* get it from the listbox */

   if( lphc->hWndLBox )
   {
       INT32 idx = SendMessage32A( lphc->hWndLBox, LB_GETCURSEL32, 0, 0 );
       if( idx != LB_ERR )
       {
           LPSTR	lpBuffer;
           INT32	length = SendMessage32A( lphc->hWndLBox, LB_GETTEXTLEN32,
					        (WPARAM32)idx, 0 );

           /* 'length' is without the terminating character */
           if( length >= N )
	       lpBuffer = (LPSTR) HeapAlloc( GetProcessHeap(), 0, length + 1 );
           else 
	       lpBuffer = lpText;

           if( lpBuffer )
           {
	       INT32    n = SendMessage32A( lphc->hWndLBox, LB_GETTEXT32, 
					   (WPARAM32)idx, (LPARAM)lpText );

	       /* truncate if buffer is too short */

	       if( length >= N )
	       {
	           if( n != LB_ERR ) memcpy( lpText, lpBuffer, (N>n) ? n+1 : N-1 );
	           lpText[N - 1] = '\0';
	           HeapFree( GetProcessHeap(), 0, lpBuffer );
	       }
	       return (LRESULT)n;
	   }
       }
   }
   return 0;
}


/***********************************************************************
 *           CBResetPos
 *
 * This function sets window positions according to the updated 
 * component placement struct.
 */
static void CBResetPos( LPHEADCOMBO lphc, LPRECT16 lbRect, BOOL32 bRedraw )
{
   BOOL32	bDrop = (CB_GETTYPE(lphc) != CBS_SIMPLE);

   /* NOTE: logs sometimes have WM_LBUTTONUP before a cascade of
    * sizing messages */

   if( lphc->wState & CBF_EDIT )
       SetWindowPos32( lphc->hWndEdit, 0, lphc->RectEdit.left, lphc->RectEdit.top,
                       lphc->RectEdit.right - lphc->RectEdit.left,
                       lphc->RectEdit.bottom - lphc->RectEdit.top,
                       SWP_NOZORDER | SWP_NOACTIVATE | ((bDrop) ? SWP_NOREDRAW : 0) );

   if( bDrop )
       OffsetRect16( lbRect, lphc->RectCombo.left, lphc->RectCombo.top );

   lbRect->right -= lbRect->left;	/* convert to width */
   lbRect->bottom -= lbRect->top;
   SetWindowPos32( lphc->hWndLBox, 0, lbRect->left, lbRect->top,
                   lbRect->right, lbRect->bottom, 
		   SWP_NOACTIVATE | SWP_NOZORDER | ((bDrop) ? SWP_NOREDRAW : 0) );

   if( bDrop )
   {
       if( lphc->wState & CBF_DROPPED )
       {
           lphc->wState &= ~CBF_DROPPED;
           ShowWindow32( lphc->hWndLBox, SW_HIDE );
       }

       lphc->wState |= CBF_NORESIZE;
       SetWindowPos32( lphc->self->hwndSelf, 0, 0, 0,
                       lphc->RectCombo.right - lphc->RectCombo.left,
                       lphc->RectEdit.bottom - lphc->RectEdit.top,
                       SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW );
       lphc->wState &= ~CBF_NORESIZE;

       if( bRedraw && !(lphc->wState & CBF_NOREDRAW) )
           RedrawWindow32( lphc->self->hwndSelf, NULL, 0,
                           RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW );
   }
}


/***********************************************************************
 *           COMBO_Size
 */
static void COMBO_Size( LPHEADCOMBO lphc )
{
  RECT16	rect;
  INT16		w, h;

  GetWindowRect16( lphc->self->hwndSelf, &rect );
  w = rect.right - rect.left; h = rect.bottom - rect.top;

  dprintf_info(combo,"COMBO_Size: w = %i, h = %i\n", w, h );

  /* CreateWindow() may send a bogus WM_SIZE, ignore it */

  if( w == (lphc->RectCombo.right - lphc->RectCombo.left) )
      if( (CB_GETTYPE(lphc) == CBS_SIMPLE) &&
	  (h == (lphc->RectCombo.bottom - lphc->RectCombo.top)) )
          return;
      else if( (lphc->dwStyle & CBS_DROPDOWN) &&
	       (h == (lphc->RectEdit.bottom - lphc->RectEdit.top))  )
	       return;
  
  lphc->RectCombo = rect;
  CBCalcPlacement( lphc, &lphc->RectEdit, &lphc->RectButton, &rect );
  CBResetPos( lphc, &rect, TRUE );
}


/***********************************************************************
 *           COMBO_Font
 */
static void COMBO_Font( LPHEADCOMBO lphc, HFONT16 hFont, BOOL32 bRedraw )
{
  RECT16        rect;

  lphc->hFont = hFont;

  if( lphc->wState & CBF_EDIT )
      SendMessage32A( lphc->hWndEdit, WM_SETFONT, (WPARAM32)hFont, bRedraw );
  SendMessage32A( lphc->hWndLBox, WM_SETFONT, (WPARAM32)hFont, bRedraw );

  GetWindowRect16( lphc->self->hwndSelf, &rect );
  OffsetRect16( &lphc->RectCombo, rect.left - lphc->RectCombo.left,
                                  rect.top - lphc->RectCombo.top );
  CBCalcPlacement( lphc, &lphc->RectEdit,
                         &lphc->RectButton, &rect );
  CBResetPos( lphc, &rect, bRedraw );
}


/***********************************************************************
 *           COMBO_SetItemHeight
 */
static LRESULT COMBO_SetItemHeight( LPHEADCOMBO lphc, INT32 index, INT32 height )
{
   LRESULT	lRet = CB_ERR;

   if( index == -1 ) /* set text field height */
   {
       if( height < 32768 )
       {
           RECT16	rect;

           lphc->editHeight = height;
           GetWindowRect16( lphc->self->hwndSelf, &rect );
           OffsetRect16( &lphc->RectCombo, rect.left - lphc->RectCombo.left,
				           rect.top - lphc->RectCombo.top );
           CBCalcPlacement( lphc, &lphc->RectEdit,
			          &lphc->RectButton, &rect );
           CBResetPos( lphc, &rect, TRUE );
	   lRet = height;
       }
   } 
   else if ( CB_OWNERDRAWN(lphc) )	/* set listbox item height */
	lRet = SendMessage32A( lphc->hWndLBox, LB_SETITEMHEIGHT32, 
			      (WPARAM32)index, (LPARAM)height );
   return lRet;
}

/***********************************************************************
 *           COMBO_SelectString
 */
static LRESULT COMBO_SelectString( LPHEADCOMBO lphc, INT32 start, LPCSTR pText )
{
   INT32 index = SendMessage32A( lphc->hWndLBox, LB_SELECTSTRING32, 
				 (WPARAM32)start, (LPARAM)pText );
   if( index >= 0 )
        if( lphc->wState & CBF_EDIT )
	    CBUpdateEdit( lphc, index );
	else
	    CBPaintText( lphc, 0 );
   return (LRESULT)index;
}

/***********************************************************************
 *           COMBO_LButtonDown
 */
static void COMBO_LButtonDown( LPHEADCOMBO lphc, LPARAM lParam )
{
   BOOL32      bButton = PtInRect16(&lphc->RectButton, MAKEPOINT16(lParam));
   HWND32      hWnd = lphc->self->hwndSelf;

   if( (CB_GETTYPE(lphc) == CBS_DROPDOWNLIST) ||
       (bButton && (CB_GETTYPE(lphc) == CBS_DROPDOWN)) )
   {
       lphc->wState |= CBF_BUTTONDOWN;
       if( lphc->wState & CBF_DROPPED )
       {
	   /* got a click to cancel selection */

           CBRollUp( lphc, TRUE, FALSE );
	   if( !IsWindow32( hWnd ) ) return;

           if( lphc->wState & CBF_CAPTURE )
           {
               lphc->wState &= ~CBF_CAPTURE;
               ReleaseCapture();
           }
           lphc->wState &= ~CBF_BUTTONDOWN;
       }
       else
       {
	   /* drop down the listbox and start tracking */

           lphc->wState |= CBF_CAPTURE;
           CBDropDown( lphc );
           SetCapture32( hWnd );
       }
       if( bButton ) CBRepaintButton( lphc );
   }
}

/***********************************************************************
 *           COMBO_LButtonUp
 *
 * Release capture and stop tracking if needed.
 */
static void COMBO_LButtonUp( LPHEADCOMBO lphc, LPARAM lParam )
{
   if( lphc->wState & CBF_CAPTURE )
   {
       lphc->wState &= ~CBF_CAPTURE;
       if( CB_GETTYPE(lphc) == CBS_DROPDOWN )
       {
	   INT32 index = CBUpdateLBox( lphc );
	   CBUpdateEdit( lphc, index );
       }
       ReleaseCapture();
   }

   if( lphc->wState & CBF_BUTTONDOWN )
   {
       lphc->wState &= ~CBF_BUTTONDOWN;
       CBRepaintButton( lphc );
   }
}

/***********************************************************************
 *           COMBO_MouseMove
 *
 * Two things to do - track combo button and release capture when
 * pointer goes into the listbox.
 */
static void COMBO_MouseMove( LPHEADCOMBO lphc, WPARAM32 wParam, LPARAM lParam )
{
   RECT16	lbRect;

   if( lphc->wState & CBF_BUTTONDOWN )
   {
       BOOL32	bButton = PtInRect16(&lphc->RectButton, MAKEPOINT16(lParam));

       if( !bButton )
       {
	   lphc->wState &= ~CBF_BUTTONDOWN;
	   CBRepaintButton( lphc );
       }
   }

   GetClientRect16( lphc->hWndLBox, &lbRect );
   MapWindowPoints16( lphc->self->hwndSelf, 
		      lphc->hWndLBox, (LPPOINT16)&lParam, 1 );
   if( PtInRect16(&lbRect, MAKEPOINT16(lParam)) )
   {
       lphc->wState &= ~CBF_CAPTURE;
       ReleaseCapture();
       if( CB_GETTYPE(lphc) == CBS_DROPDOWN ) CBUpdateLBox( lphc );

       /* hand over pointer tracking */
       SendMessage32A( lphc->hWndLBox, WM_LBUTTONDOWN, wParam, lParam );
   }
}


/***********************************************************************
 *           ComboWndProc
 *
 * http://www.microsoft.com/msdn/sdk/platforms/doc/sdk/win32/ctrl/src/combobox_15.htm
 */
LRESULT WINAPI ComboWndProc( HWND32 hwnd, UINT32 message,
                             WPARAM32 wParam, LPARAM lParam )
{
    WND*	pWnd = WIN_FindWndPtr(hwnd);
   
    if( pWnd )
    {
      LPHEADCOMBO	lphc = CB_GETPTR(pWnd);

      dprintf_info(combo, "Combo [%04x]: msg %s wp %08x lp %08lx\n",
                     pWnd->hwndSelf, SPY_GetMsgName(message), wParam, lParam );

      if( lphc || message == WM_NCCREATE )
      switch(message) 
      {	

	/* System messages */

     	case WM_NCCREATE: 
		return COMBO_NCCreate(pWnd, lParam);

     	case WM_NCDESTROY: 
		COMBO_NCDestroy(lphc);
		break;

     	case WM_CREATE: 
		return COMBO_Create(lphc, pWnd, lParam);

     	case WM_PAINT:
		/* wParam may contain a valid HDC! */
		return COMBO_Paint(lphc, (HDC16)wParam);

	case WM_ERASEBKGND:
		return TRUE;

     	case WM_GETDLGCODE: 
		return (LRESULT)(DLGC_WANTARROWS | DLGC_WANTCHARS);

	case WM_SIZE:
	        if( lphc->hWndLBox && 
		  !(lphc->wState & CBF_NORESIZE) ) COMBO_Size( lphc );
		return TRUE;

	case WM_SETFONT:
		COMBO_Font( lphc, (HFONT16)wParam, (BOOL32)lParam );
		return TRUE;

	case WM_GETFONT:
		return (LRESULT)lphc->hFont;

	case WM_SETFOCUS:
		if( lphc->wState & CBF_EDIT )
		    SetFocus32( lphc->hWndEdit );
		else
		    COMBO_SetFocus( lphc );
		return TRUE;

	case WM_KILLFOCUS:
#define hwndFocus ((HWND16)wParam)
		if( !hwndFocus ||
		    (hwndFocus != lphc->hWndEdit && hwndFocus != lphc->hWndLBox ))
		    COMBO_KillFocus( lphc );
#undef hwndFocus
		return TRUE;

	case WM_COMMAND:
		return COMBO_Command( lphc, wParam, (HWND32)lParam );

	case WM_GETTEXT:
		return COMBO_GetText( lphc, (UINT32)wParam, (LPSTR)lParam );

	case WM_SETTEXT:
	case WM_GETTEXTLENGTH:
	case WM_CLEAR:
	case WM_CUT:
        case WM_PASTE:
	case WM_COPY:
		if( lphc->wState & CBF_EDIT )
		    return SendMessage32A( lphc->hWndEdit, message, wParam, lParam );
		return CB_ERR;

	case WM_DRAWITEM:
	case WM_DELETEITEM:
	case WM_COMPAREITEM:
	case WM_MEASUREITEM:
		return COMBO_ItemOp32( lphc, message, wParam, lParam );

	case WM_ENABLE:
		if( lphc->wState & CBF_EDIT )
		    EnableWindow32( lphc->hWndEdit, (BOOL32)wParam ); 
		EnableWindow32( lphc->hWndLBox, (BOOL32)wParam );
		return TRUE;

	case WM_SETREDRAW:
		if( wParam )
		    lphc->wState &= ~CBF_NOREDRAW;
		else
		    lphc->wState |= CBF_NOREDRAW;

		if( lphc->wState & CBF_EDIT )
		    SendMessage32A( lphc->hWndEdit, message, wParam, lParam );
		SendMessage32A( lphc->hWndLBox, message, wParam, lParam );
		return 0;
		
	case WM_SYSKEYDOWN:
		if( KEYDATA_ALT & HIWORD(lParam) )
		    if( wParam == VK_UP || wParam == VK_DOWN )
			COMBO_FlipListbox( lphc, TRUE );
		break;

	case WM_CHAR:
	case WM_KEYDOWN:
		if( lphc->wState & CBF_EDIT )
		    return SendMessage32A( lphc->hWndEdit, message, wParam, lParam );
		else
		    return SendMessage32A( lphc->hWndLBox, message, wParam, lParam );

	case WM_LBUTTONDOWN: 
		if( !(lphc->wState & CBF_FOCUSED) ) SetFocus32( lphc->self->hwndSelf );
		if( lphc->wState & CBF_FOCUSED ) COMBO_LButtonDown( lphc, lParam );
		return TRUE;

	case WM_LBUTTONUP:
		COMBO_LButtonUp( lphc, lParam );
		return TRUE;

	case WM_MOUSEMOVE: 
		if( lphc->wState & CBF_CAPTURE ) 
		    COMBO_MouseMove( lphc, wParam, lParam );
		return TRUE;

	/* Combo messages */

	case CB_ADDSTRING16:
		if( CB_HASSTRINGS(lphc) ) lParam = (LPARAM)PTR_SEG_TO_LIN(lParam);
	case CB_ADDSTRING32:
		return SendMessage32A( lphc->hWndLBox, LB_ADDSTRING32, 0, lParam);

	case CB_INSERTSTRING16:
		wParam = (INT32)(INT16)wParam;
		if( CB_HASSTRINGS(lphc) ) lParam = (LPARAM)PTR_SEG_TO_LIN(lParam);
	case CB_INSERTSTRING32:
		return SendMessage32A( lphc->hWndLBox, LB_INSERTSTRING32, wParam, lParam);

	case CB_DELETESTRING16:
	case CB_DELETESTRING32:
		return SendMessage32A( lphc->hWndLBox, LB_DELETESTRING32, wParam, 0);

	case CB_SELECTSTRING16:
		wParam = (INT32)(INT16)wParam;
		if( CB_HASSTRINGS(lphc) ) lParam = (LPARAM)PTR_SEG_TO_LIN(lParam);
	case CB_SELECTSTRING32:
		return COMBO_SelectString( lphc, (INT32)wParam, (LPSTR)lParam );

	case CB_FINDSTRING16:
		wParam = (INT32)(INT16)wParam;
		if( CB_HASSTRINGS(lphc) ) lParam = (LPARAM)PTR_SEG_TO_LIN(lParam);
	case CB_FINDSTRING32:
		return SendMessage32A( lphc->hWndLBox, LB_FINDSTRING32, wParam, lParam);

	case CB_FINDSTRINGEXACT16:
		wParam = (INT32)(INT16)wParam;
		if( CB_HASSTRINGS(lphc) ) lParam = (LPARAM)PTR_SEG_TO_LIN(lParam);
	case CB_FINDSTRINGEXACT32:
		return SendMessage32A( lphc->hWndLBox, LB_FINDSTRINGEXACT32, 
						       wParam, lParam );
	case CB_SETITEMHEIGHT16:
		wParam = (INT32)(INT16)wParam;
	case CB_SETITEMHEIGHT32:
		return COMBO_SetItemHeight( lphc, (INT32)wParam, (INT32)lParam);

	case CB_RESETCONTENT16: 
	case CB_RESETCONTENT32:
		SendMessage32A( lphc->hWndLBox, LB_RESETCONTENT32, 0, 0 );
		CBPaintText( lphc, 0 );
		return TRUE;

	case CB_INITSTORAGE32:
		return SendMessage32A( lphc->hWndLBox, LB_INITSTORAGE32, wParam, lParam);

	case CB_GETHORIZONTALEXTENT32:
		return SendMessage32A( lphc->hWndLBox, LB_GETHORIZONTALEXTENT32, 0, 0);

	case CB_SETHORIZONTALEXTENT32:
		return SendMessage32A( lphc->hWndLBox, LB_SETHORIZONTALEXTENT32, wParam, 0);

	case CB_GETTOPINDEX32:
		return SendMessage32A( lphc->hWndLBox, LB_GETTOPINDEX32, 0, 0);

	case CB_GETLOCALE32:
		return SendMessage32A( lphc->hWndLBox, LB_GETLOCALE32, 0, 0);

	case CB_SETLOCALE32:
		return SendMessage32A( lphc->hWndLBox, LB_SETLOCALE32, wParam, 0);

	case CB_GETDROPPEDWIDTH32:
		if( lphc->droppedWidth )
		    return lphc->droppedWidth;
		return lphc->RectCombo.right - lphc->RectCombo.left - 
		           (lphc->wState & CBF_EDIT) ? CBitOffset : 0;

	case CB_SETDROPPEDWIDTH32:
		if( (CB_GETTYPE(lphc) != CBS_SIMPLE) &&
		    (INT32)wParam < 32768 ) lphc->droppedWidth = (INT32)wParam;
		return CB_ERR;

	case CB_GETDROPPEDCONTROLRECT16:
		lParam = (LPARAM)PTR_SEG_TO_LIN(lParam);
		if( lParam ) 
		{
		    RECT32	r;
		    CBGetDroppedControlRect32( lphc, &r );
		    CONV_RECT32TO16( &r, (LPRECT16)lParam );
		}
		return CB_OKAY;

	case CB_GETDROPPEDCONTROLRECT32:
		if( lParam ) CBGetDroppedControlRect32(lphc, (LPRECT32)lParam );
		return CB_OKAY;

	case CB_GETDROPPEDSTATE16:
	case CB_GETDROPPEDSTATE32:
		return (lphc->wState & CBF_DROPPED) ? TRUE : FALSE;

	case CB_DIR16: 
                lParam = (LPARAM)PTR_SEG_TO_LIN(lParam);
                /* fall through */
	case CB_DIR32:
		return COMBO_Directory( lphc, (UINT32)wParam, 
				       (LPSTR)lParam, (message == CB_DIR32));
	case CB_SHOWDROPDOWN16:
	case CB_SHOWDROPDOWN32:
		if( CB_GETTYPE(lphc) != CBS_SIMPLE )
		    if( wParam )
		    {
			if( !(lphc->wState & CBF_DROPPED) )
			    CBDropDown( lphc );
		    }
		    else 
			if( lphc->wState & CBF_DROPPED ) 
		            CBRollUp( lphc, FALSE, TRUE );
		return TRUE;

	case CB_GETCOUNT16: 
	case CB_GETCOUNT32:
		return SendMessage32A( lphc->hWndLBox, LB_GETCOUNT32, 0, 0);

	case CB_GETCURSEL16: 
	case CB_GETCURSEL32:
		return SendMessage32A( lphc->hWndLBox, LB_GETCURSEL32, 0, 0);

	case CB_SETCURSEL16:
		wParam = (INT32)(INT16)wParam;
	case CB_SETCURSEL32:
		return SendMessage32A( lphc->hWndLBox, LB_SETCURSEL32, wParam, 0);

	case CB_GETLBTEXT16: 
		wParam = (INT32)(INT16)wParam;
		lParam = (LPARAM)PTR_SEG_TO_LIN(lParam);
	case CB_GETLBTEXT32:
		return SendMessage32A( lphc->hWndLBox, LB_GETTEXT32, wParam, lParam);

	case CB_GETLBTEXTLEN16: 
		wParam = (INT32)(INT16)wParam;
	case CB_GETLBTEXTLEN32:
		return SendMessage32A( lphc->hWndLBox, LB_GETTEXTLEN32, wParam, 0);

	case CB_GETITEMDATA16:
		wParam = (INT32)(INT16)wParam;
	case CB_GETITEMDATA32:
		return SendMessage32A( lphc->hWndLBox, LB_GETITEMDATA32, wParam, 0);

	case CB_SETITEMDATA16:
		wParam = (INT32)(INT16)wParam;
	case CB_SETITEMDATA32:
		return SendMessage32A( lphc->hWndLBox, LB_SETITEMDATA32, wParam, lParam);

	case CB_GETEDITSEL16: 
		wParam = lParam = 0;   /* just in case */
	case CB_GETEDITSEL32:
		if( lphc->wState & CBF_EDIT )
		{
		    INT32	a, b;

		    return SendMessage32A( lphc->hWndEdit, EM_GETSEL32,
					   (wParam) ? wParam : (WPARAM32)&a,
					   (lParam) ? lParam : (LPARAM)&b );
		}
		return CB_ERR;

	case CB_SETEDITSEL16: 
	case CB_SETEDITSEL32:
		if( lphc->wState & CBF_EDIT ) 
		    return SendMessage32A( lphc->hWndEdit, EM_SETSEL32, 
			  (INT32)(INT16)LOWORD(lParam), (INT32)(INT16)HIWORD(lParam) );
		return CB_ERR;

	case CB_SETEXTENDEDUI16:
	case CB_SETEXTENDEDUI32:
		if( CB_GETTYPE(lphc) == CBS_SIMPLE ) return CB_ERR;

		if( wParam )
		    lphc->wState |= CBF_EUI;
		else lphc->wState &= ~CBF_EUI;
		return CB_OKAY;

	case CB_GETEXTENDEDUI16:
	case CB_GETEXTENDEDUI32:
		return (lphc->wState & CBF_EUI) ? TRUE : FALSE;

	case (WM_USER + 0x1B):
	        dprintf_warn(combo, "Combo [%04x]: undocumented msg!\n", (HWND16)hwnd );
    }
    return DefWindowProc32A(hwnd, message, wParam, lParam);
  }
  return CB_ERR;
}

