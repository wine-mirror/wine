/*
 * Combo controls
 * 
 * Copyright 1997 Alex Korobka
 * 
 * FIXME: roll up in Netscape 3.01.
 */

#include <string.h>

#include "winbase.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/winuser16.h"
#include "win.h"
#include "spy.h"
#include "user.h"
#include "heap.h"
#include "combo.h"
#include "drive.h"
#include "debugtools.h"
#include "tweak.h"

DEFAULT_DEBUG_CHANNEL(combo)

  /* bits in the dwKeyData */
#define KEYDATA_ALT             0x2000
#define KEYDATA_PREVSTATE       0x4000

/*
 * Additional combo box definitions
 */

#define CB_GETPTR( wnd )      (*(LPHEADCOMBO*)((wnd)->wExtra))
#define CB_NOTIFY( lphc, code ) \
	(SendMessageA( (lphc)->owner, WM_COMMAND, \
			 MAKEWPARAM((lphc)->self->wIDmenu, (code)), (lphc)->self->hwndSelf))
#define CB_GETEDITTEXTLENGTH( lphc ) \
	(SendMessageA( (lphc)->hWndEdit, WM_GETTEXTLENGTH, 0, 0 ))

#define ISWIN31 (LOWORD(GetVersion()) == 0x0a03)

/*
 * Drawing globals
 */
static HBITMAP 	hComboBmp = 0;
static UINT	CBitHeight, CBitWidth;

/*
 * Look and feel dependant "constants"
 */

#define COMBO_YBORDERGAP         5
#define COMBO_XBORDERSIZE()      ( (TWEAK_WineLook == WIN31_LOOK) ? 0 : 2 )
#define COMBO_YBORDERSIZE()      ( (TWEAK_WineLook == WIN31_LOOK) ? 0 : 2 )
#define COMBO_EDITBUTTONSPACE()  ( (TWEAK_WineLook == WIN31_LOOK) ? 8 : 0 )
#define EDIT_CONTROL_PADDING()   ( (TWEAK_WineLook == WIN31_LOOK) ? 0 : 1 )

/***********************************************************************
 *           COMBO_Init
 *
 * Load combo button bitmap.
 */
static BOOL COMBO_Init()
{
  HDC		hDC;
  
  if( hComboBmp ) return TRUE;
  if( (hDC = CreateCompatibleDC(0)) )
  {
    BOOL	bRet = FALSE;
    if( (hComboBmp = LoadBitmapA(0, MAKEINTRESOURCEA(OBM_COMBO))) )
    {
      BITMAP      bm;
      HBITMAP     hPrevB;
      RECT        r;

      GetObjectA( hComboBmp, sizeof(bm), &bm );
      CBitHeight = bm.bmHeight;
      CBitWidth  = bm.bmWidth;

      TRACE("combo bitmap [%i,%i]\n", CBitWidth, CBitHeight );

      hPrevB = SelectObject16( hDC, hComboBmp);
      SetRect( &r, 0, 0, CBitWidth, CBitHeight );
      InvertRect( hDC, &r );
      SelectObject( hDC, hPrevB );
      bRet = TRUE;
    }
    DeleteDC( hDC );
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
	LPCREATESTRUCTA     lpcs = (CREATESTRUCTA*)lParam;
       
	memset( lphc, 0, sizeof(HEADCOMBO) );
       *(LPHEADCOMBO*)wnd->wExtra = lphc;

       /* some braindead apps do try to use scrollbar/border flags */

	lphc->dwStyle = (lpcs->style & ~(WS_BORDER | WS_HSCROLL | WS_VSCROLL));
	wnd->dwStyle &= ~(WS_BORDER | WS_HSCROLL | WS_VSCROLL);

	/*
	 * We also have to remove the client edge style to make sure
	 * we don't end-up with a non client area.
	 */
	wnd->dwExStyle &= ~(WS_EX_CLIENTEDGE);

	if( !(lpcs->style & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) )
              lphc->dwStyle |= CBS_HASSTRINGS;
	if( !(wnd->dwExStyle & WS_EX_NOPARENTNOTIFY) )
	      lphc->wState |= CBF_NOTIFY;

	TRACE("[0x%08x], style = %08x\n", 
		     (UINT)lphc, lphc->dwStyle );

	return (LRESULT)(UINT)wnd->hwndSelf; 
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

       TRACE("[%04x]: freeing storage\n", CB_HWND(lphc));

       if( (CB_GETTYPE(lphc) != CBS_SIMPLE) && lphc->hWndLBox ) 
   	   DestroyWindow( lphc->hWndLBox );

       HeapFree( GetProcessHeap(), 0, lphc );
       wnd->wExtra[0] = 0;
   }
   return 0;
}

/***********************************************************************
 *           CBGetTextAreaHeight
 *
 * This method will calculate the height of the text area of the 
 * combobox.
 * The height of the text area is set in two ways. 
 * It can be set explicitely through a combobox message of through a
 * WM_MEASUREITEM callback.
 * If this is not the case, the height is set to 13 dialog units.
 * This height was determined through experimentation.
 */
static INT CBGetTextAreaHeight(
  HWND        hwnd,
  LPHEADCOMBO lphc)
{
  INT iTextItemHeight;

  if( lphc->editHeight ) /* explicitly set height */
  {
    iTextItemHeight = lphc->editHeight;
  }
  else
  {
    TEXTMETRICA tm;
    HDC         hDC       = GetDC(hwnd);
    HFONT       hPrevFont = 0;
    INT         baseUnitY;
    
    if (lphc->hFont)
      hPrevFont = SelectObject( hDC, lphc->hFont );
    
    GetTextMetricsA(hDC, &tm);
    
    baseUnitY = tm.tmHeight;
    
    if( hPrevFont )
      SelectObject( hDC, hPrevFont );
    
    ReleaseDC(hwnd, hDC);
    
    iTextItemHeight = ((13 * baseUnitY) / 8);

    /*
     * This "formula" calculates the height of the complete control.
     * To calculate the height of the text area, we have to remove the
     * borders.
     */
    iTextItemHeight -= 2*COMBO_YBORDERSIZE();
  }
  
  /*
   * Check the ownerdraw case if we haven't asked the parent the size
   * of the item yet.
   */
  if ( CB_OWNERDRAWN(lphc) &&
       (lphc->wState & CBF_MEASUREITEM) )
  {
    MEASUREITEMSTRUCT measureItem;
    RECT              clientRect;
    INT               originalItemHeight = iTextItemHeight;
    
    /*
     * We use the client rect for the width of the item.
     */
    GetClientRect(hwnd, &clientRect);
    
    lphc->wState &= ~CBF_MEASUREITEM;
    
    /*
     * Send a first one to measure the size of the text area
     */
    measureItem.CtlType    = ODT_COMBOBOX;
    measureItem.CtlID      = lphc->self->wIDmenu;
    measureItem.itemID     = -1;
    measureItem.itemWidth  = clientRect.right;
    measureItem.itemHeight = iTextItemHeight - 6; /* ownerdrawn cb is taller */
    measureItem.itemData   = 0;
    SendMessageA(lphc->owner, WM_MEASUREITEM, 
		 (WPARAM)measureItem.CtlID, (LPARAM)&measureItem);
    iTextItemHeight = 6 + measureItem.itemHeight;

    /*
     * Send a second one in the case of a fixed ownerdraw list to calculate the
     * size of the list items. (we basically do this on behalf of the listbox)
     */
    if (lphc->dwStyle & CBS_OWNERDRAWFIXED)
    {
      measureItem.CtlType    = ODT_COMBOBOX;
      measureItem.CtlID      = lphc->self->wIDmenu;
      measureItem.itemID     = 0;
      measureItem.itemWidth  = clientRect.right;
      measureItem.itemHeight = originalItemHeight;
      measureItem.itemData   = 0;
      SendMessageA(lphc->owner, WM_MEASUREITEM, 
		   (WPARAM)measureItem.CtlID, (LPARAM)&measureItem);
      lphc->fixedOwnerDrawHeight = measureItem.itemHeight;
    }
    
    /*
     * Keep the size for the next time 
     */
    lphc->editHeight = iTextItemHeight;
  }

  return iTextItemHeight;
}

/***********************************************************************
 *           CBForceDummyResize
 *
 * The dummy resize is used for listboxes that have a popup to trigger
 * a re-arranging of the contents of the combobox and the recalculation
 * of the size of the "real" control window.
 */
static void CBForceDummyResize(
  LPHEADCOMBO lphc)
{
  RECT windowRect;
  int newComboHeight;

  newComboHeight = CBGetTextAreaHeight(CB_HWND(lphc),lphc) + 2*COMBO_YBORDERSIZE();
	
  GetWindowRect(CB_HWND(lphc), &windowRect);

  /*
   * We have to be careful, resizing a combobox also has the meaning that the
   * dropped rect will be resized. In this case, we want to trigger a resize
   * to recalculate layout but we don't want to change the dropped rectangle
   * So, we pass the height of text area of control as the height.
   * this will cancel-out in the processing of the WM_WINDOWPOSCHANGING
   * message.
   */
  SetWindowPos( CB_HWND(lphc),
		(HWND)NULL,
		0, 0,
		windowRect.right  - windowRect.left,
		newComboHeight,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
}

/***********************************************************************
 *           CBCalcPlacement
 *
 * Set up component coordinates given valid lphc->RectCombo.
 */
static void CBCalcPlacement(
  HWND        hwnd,
  LPHEADCOMBO lphc,
  LPRECT      lprEdit, 
  LPRECT      lprButton, 
  LPRECT      lprLB)
{
  /*
   * Again, start with the client rectangle.
   */
  GetClientRect(hwnd, lprEdit);

  /*
   * Remove the borders
   */
  InflateRect(lprEdit, -COMBO_XBORDERSIZE(), -COMBO_YBORDERSIZE());

  /*
   * Chop off the bottom part to fit with the height of the text area.
   */
  lprEdit->bottom = lprEdit->top + CBGetTextAreaHeight(hwnd, lphc);

  /*
   * The button starts the same vertical position as the text area.
   */
  CopyRect(lprButton, lprEdit);

  /*
   * If the combobox is "simple" there is no button.
   */
  if( CB_GETTYPE(lphc) == CBS_SIMPLE ) 
    lprButton->left = lprButton->right = lprButton->bottom = 0;
  else
  {
    /*
     * Let's assume the combobox button is the same width as the
     * scrollbar button.
     * size the button horizontally and cut-off the text area.
     */
    lprButton->left = lprButton->right - GetSystemMetrics(SM_CXVSCROLL);
    lprEdit->right  = lprButton->left;
  }
  
  /*
   * In the case of a dropdown, there is an additional spacing between the
   * text area and the button.
   */
  if( CB_GETTYPE(lphc) == CBS_DROPDOWN )
  {
    lprEdit->right -= COMBO_EDITBUTTONSPACE();
  }

  /*
   * If we have an edit control, we space it away from the borders slightly.
   */
  if (CB_GETTYPE(lphc) != CBS_DROPDOWNLIST)
  {
    InflateRect(lprEdit, -EDIT_CONTROL_PADDING(), -EDIT_CONTROL_PADDING());
  }
  
  /*
   * Adjust the size of the listbox popup.
   */
  if( CB_GETTYPE(lphc) == CBS_SIMPLE )
  {
    /*
     * Use the client rectangle to initialize the listbox rectangle
     */
    GetClientRect(hwnd, lprLB);

    /*
     * Then, chop-off the top part.
     */
    lprLB->top = lprEdit->bottom + COMBO_YBORDERSIZE();
  }
  else
  {
    /*
     * Make sure the dropped width is as large as the combobox itself.
     */
    if (lphc->droppedWidth < (lprButton->right + COMBO_XBORDERSIZE()))
    {
      lprLB->right  = lprLB->left + (lprButton->right + COMBO_XBORDERSIZE());

      /*
       * In the case of a dropdown, the popup listbox is offset to the right.
       * so, we want to make sure it's flush with the right side of the 
       * combobox
       */
      if( CB_GETTYPE(lphc) == CBS_DROPDOWN )
	lprLB->right -= COMBO_EDITBUTTONSPACE();
    }
    else
       lprLB->right = lprLB->left + lphc->droppedWidth;
  }

  TRACE("\ttext\t= (%i,%i-%i,%i)\n",
	lprEdit->left, lprEdit->top, lprEdit->right, lprEdit->bottom);
  
  TRACE("\tbutton\t= (%i,%i-%i,%i)\n",
	lprButton->left, lprButton->top, lprButton->right, lprButton->bottom);
  
  TRACE("\tlbox\t= (%i,%i-%i,%i)\n", 
	lprLB->left, lprLB->top, lprLB->right, lprLB->bottom );
}

/***********************************************************************
 *           CBGetDroppedControlRect
 */
static void CBGetDroppedControlRect( LPHEADCOMBO lphc, LPRECT lpRect)
{
    /* In windows, CB_GETDROPPEDCONTROLRECT returns the upper left corner
     of the combo box and the lower right corner of the listbox */
    
    GetWindowRect(lphc->self->hwndSelf, lpRect);

    lpRect->right =  lpRect->left + lphc->droppedRect.right - lphc->droppedRect.left;
    lpRect->bottom = lpRect->top + lphc->droppedRect.bottom - lphc->droppedRect.top;

}

/***********************************************************************
 *           COMBO_WindowPosChanging         
 */
static LRESULT COMBO_WindowPosChanging(
  HWND        hwnd,
  LPHEADCOMBO lphc,
  WINDOWPOS*  posChanging)
{
  /*
   * We need to override the WM_WINDOWPOSCHANGING method to handle all
   * the non-simple comboboxes. The problem is that those controls are
   * always the same height. We have to make sure they are not resized
   * to another value.
   */
  if ( ( CB_GETTYPE(lphc) != CBS_SIMPLE ) && 
       ((posChanging->flags & SWP_NOSIZE) == 0) )
  {
    int newComboHeight;

    newComboHeight = CBGetTextAreaHeight(hwnd,lphc) +
                      2*COMBO_YBORDERSIZE();

    /*
     * Resizing a combobox has another side effect, it resizes the dropped
     * rectangle as well. However, it does it only if the new height for the
     * combobox is different than the height it should have. In other words,
     * if the application resizing the combobox only had the intention to resize
     * the actual control, for example, to do the layout of a dialog that is
     * resized, the height of the dropdown is not changed.
     */
    if (posChanging->cy != newComboHeight)
    {
      lphc->droppedRect.bottom = lphc->droppedRect.top + posChanging->cy - newComboHeight;

      posChanging->cy = newComboHeight;
    }
  }

  return 0;
}

/***********************************************************************
 *           COMBO_Create
 */
static LRESULT COMBO_Create( LPHEADCOMBO lphc, WND* wnd, LPARAM lParam)
{
  static char clbName[] = "ComboLBox";
  static char editName[] = "Edit";

  LPCREATESTRUCTA  lpcs = (CREATESTRUCTA*)lParam;
  
  if( !CB_GETTYPE(lphc) ) lphc->dwStyle |= CBS_SIMPLE;
  if( CB_GETTYPE(lphc) != CBS_DROPDOWNLIST ) lphc->wState |= CBF_EDIT;
  
  lphc->self  = wnd;
  lphc->owner = lpcs->hwndParent;

  /*
   * The item height and dropped width are not set when the control
   * is created.
   */
  lphc->droppedWidth = lphc->editHeight = 0;

  /*
   * The first time we go through, we want to measure the ownerdraw item
   */
  lphc->wState |= CBF_MEASUREITEM;

  /* M$ IE 3.01 actually creates (and rapidly destroys) an ownerless combobox */

  if( lphc->owner || !(lpcs->style & WS_VISIBLE) )
  {
      UINT lbeStyle   = 0;
      UINT lbeExStyle = 0;

      /*
       * Initialize the dropped rect to the size of the client area of the
       * control and then, force all the areas of the combobox to be
       * recalculated.
       */
      GetClientRect( wnd->hwndSelf, &lphc->droppedRect );

      CBCalcPlacement(wnd->hwndSelf, 
		      lphc, 
		      &lphc->textRect, 
		      &lphc->buttonRect, 
		      &lphc->droppedRect );

      /*
       * Adjust the position of the popup listbox if it's necessary
       */
      if ( CB_GETTYPE(lphc) != CBS_SIMPLE )
      {
	lphc->droppedRect.top   = lphc->textRect.bottom + COMBO_YBORDERSIZE();

	/*
	 * If it's a dropdown, the listbox is offset
	 */
	if( CB_GETTYPE(lphc) == CBS_DROPDOWN )
	  lphc->droppedRect.left += COMBO_EDITBUTTONSPACE();

	ClientToScreen(wnd->hwndSelf, (LPPOINT)&lphc->droppedRect);
	ClientToScreen(wnd->hwndSelf, (LPPOINT)&lphc->droppedRect.right);
      }

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
      {
	lbeStyle |= WS_CHILD | WS_VISIBLE;

	/*
	 * In win 95 look n feel, the listbox in the simple combobox has
	 * the WS_EXCLIENTEDGE style instead of the WS_BORDER style.
	 */
	if (TWEAK_WineLook > WIN31_LOOK)
	{
	  lbeStyle   &= ~WS_BORDER;
	  lbeExStyle |= WS_EX_CLIENTEDGE;
	}
      }
      else					/* popup listbox */
	lbeStyle |= WS_POPUP;

     /* Dropdown ComboLBox is not a child window and we cannot pass 
      * ID_CB_LISTBOX directly because it will be treated as a menu handle.
      */
      lphc->hWndLBox = CreateWindowExA(lbeExStyle,
				       clbName,
				       NULL, 
				       lbeStyle, 
				       lphc->droppedRect.left, 
				       lphc->droppedRect.top, 
				       lphc->droppedRect.right - lphc->droppedRect.left, 
				       lphc->droppedRect.bottom - lphc->droppedRect.top, 
				       lphc->self->hwndSelf, 
		       (lphc->dwStyle & CBS_DROPDOWN)? (HMENU)0 : (HMENU)ID_CB_LISTBOX,
				       lphc->self->hInstance,
				       (LPVOID)lphc );

      /*
       * The ComboLBox is a strange little beast (when it's not a CBS_SIMPLE)...
       * It's a popup window but, when you get the window style, you get WS_CHILD.
       * When created, it's parent is the combobox but, when you ask for it's parent
       * after that, you're supposed to get the desktop. (see MFC code function
       * AfxCancelModes)
       * To achieve this in Wine, we have to create it as a popup and change 
       * it's style to child after the creation. 
       */
      if ( (lphc->hWndLBox!= 0) &&
	   (CB_GETTYPE(lphc) != CBS_SIMPLE) )
      {
	SetWindowLongA(lphc->hWndLBox, 
		       GWL_STYLE, 
		       (GetWindowLongA(lphc->hWndLBox, GWL_STYLE) | WS_CHILD) & ~WS_POPUP);
      }

      if( lphc->hWndLBox )
      {
	  BOOL	bEdit = TRUE;
	  lbeStyle = WS_CHILD | WS_VISIBLE | ES_NOHIDESEL | ES_LEFT;

	  /*
	   * In Win95 look, the border fo the edit control is 
	   * provided by the combobox
	   */
	  if (TWEAK_WineLook == WIN31_LOOK)
	    lbeStyle |= WS_BORDER;
	    
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

	      lphc->hWndEdit = CreateWindowExA(0,
					       editName, 
					       NULL, 
					       lbeStyle,
					       lphc->textRect.left, lphc->textRect.top, 
					       lphc->textRect.right - lphc->textRect.left,
					       lphc->textRect.bottom - lphc->textRect.top, 
					       lphc->self->hwndSelf, 
					       (HMENU)ID_CB_EDIT, 
					       lphc->self->hInstance, 
					       NULL );

	      if( !lphc->hWndEdit )
		bEdit = FALSE;
	  } 

          if( bEdit )
	  {
	    /* 
	     * If the combo is a dropdown, we must resize the control to fit only
	     * the text area and button. To do this, we send a dummy resize and the
	     * WM_WINDOWPOSCHANGING message will take care of setting the height for
	     * us.
	     */
	    if( CB_GETTYPE(lphc) != CBS_SIMPLE )
	    {
	      CBForceDummyResize(lphc);
	    }
	    
	    TRACE("init done\n");
	    return wnd->hwndSelf;
	  }
	  ERR("edit control failure.\n");
      } else ERR("listbox failure.\n");
  } else ERR("no owner for visible combo.\n");

  /* CreateWindow() will send WM_NCDESTROY to cleanup */

  return -1;
}

/***********************************************************************
 *           CBPaintButton
 *
 * Paint combo button (normal, pressed, and disabled states).
 */
static void CBPaintButton(
  LPHEADCOMBO lphc, 
  HDC         hdc,
  RECT        rectButton)
{
    if( lphc->wState & CBF_NOREDRAW ) 
      return;

    if (TWEAK_WineLook == WIN31_LOOK)
    {
        UINT 	  x, y;
	BOOL 	  bBool;
	HDC       hMemDC;
	HBRUSH    hPrevBrush;
	COLORREF  oldTextColor, oldBkColor;
	

	hPrevBrush = SelectObject(hdc, GetSysColorBrush(COLOR_BTNFACE));

	/*
	 * Draw the button background
	 */
	PatBlt( hdc,
		rectButton.left,
		rectButton.top,
		rectButton.right-rectButton.left,
		rectButton.bottom-rectButton.top,
		PATCOPY );
	
	if( (bBool = lphc->wState & CBF_BUTTONDOWN) )
	{
	    DrawEdge( hdc, &rectButton, EDGE_SUNKEN, BF_RECT );
	} 
	else 
	{
	    DrawEdge( hdc, &rectButton, EDGE_RAISED, BF_RECT );
	}
	
	/*
	 * Remove the edge of the button from the rectangle
	 * and calculate the position of the bitmap.
	 */
	InflateRect( &rectButton, -2, -2);	
	
	x = (rectButton.left + rectButton.right - CBitWidth) >> 1;
	y = (rectButton.top + rectButton.bottom - CBitHeight) >> 1;
	
	
	hMemDC = CreateCompatibleDC( hdc );
	SelectObject( hMemDC, hComboBmp );
	oldTextColor = SetTextColor( hdc, GetSysColor(COLOR_BTNFACE) );
	oldBkColor = SetBkColor( hdc, CB_DISABLED(lphc) ? RGB(128,128,128) :
				 RGB(0,0,0) );
	BitBlt( hdc, x, y, CBitWidth, CBitHeight, hMemDC, 0, 0, SRCCOPY );
	SetBkColor( hdc, oldBkColor );
	SetTextColor( hdc, oldTextColor );
	DeleteDC( hMemDC );
	SelectObject( hdc, hPrevBrush );
    }
    else
    {
        UINT buttonState = DFCS_SCROLLCOMBOBOX;

	if (lphc->wState & CBF_BUTTONDOWN)
	{
	    buttonState |= DFCS_PUSHED;
	}

	if (CB_DISABLED(lphc))
	{
	  buttonState |= DFCS_INACTIVE;
	}

	DrawFrameControl(hdc,
			 &rectButton,
			 DFC_SCROLL,
			 buttonState);			  
    }
}

/***********************************************************************
 *           CBPaintText
 *
 * Paint CBS_DROPDOWNLIST text field / update edit control contents.
 */
static void CBPaintText(
  LPHEADCOMBO lphc, 
  HDC         hdc,
  RECT        rectEdit)
{
   INT	id, size = 0;
   LPSTR	pText = NULL;

   if( lphc->wState & CBF_NOREDRAW ) return;

   /* follow Windows combobox that sends a bunch of text 
    * inquiries to its listbox while processing WM_PAINT. */

   if( (id = SendMessageA(lphc->hWndLBox, LB_GETCURSEL, 0, 0) ) != LB_ERR )
   {
        size = SendMessageA( lphc->hWndLBox, LB_GETTEXTLEN, id, 0);
        if( (pText = HeapAlloc( GetProcessHeap(), 0, size + 1)) )
	{
	    SendMessageA( lphc->hWndLBox, LB_GETTEXT, (WPARAM)id, (LPARAM)pText );
	    pText[size] = '\0';	/* just in case */
	} else return;
   }

   if( lphc->wState & CBF_EDIT )
   {
	if( CB_HASSTRINGS(lphc) ) SetWindowTextA( lphc->hWndEdit, pText ? pText : "" );
	if( lphc->wState & CBF_FOCUSED ) 
	    SendMessageA( lphc->hWndEdit, EM_SETSEL, 0, (LPARAM)(-1));
   }
   else /* paint text field ourselves */
   {
     UINT	itemState;
     HFONT	hPrevFont = (lphc->hFont) ? SelectObject(hdc, lphc->hFont) : 0;

     /*
      * Give ourselves some space.
      */
     InflateRect( &rectEdit, -1, -1 );
     
     if ( (lphc->wState & CBF_FOCUSED) && 
	  !(lphc->wState & CBF_DROPPED) )
     {
       /* highlight */
       
       FillRect( hdc, &rectEdit, GetSysColorBrush(COLOR_HIGHLIGHT) );
       SetBkColor( hdc, GetSysColor( COLOR_HIGHLIGHT ) );
       SetTextColor( hdc, GetSysColor( COLOR_HIGHLIGHTTEXT ) );
       itemState = ODS_SELECTED | ODS_FOCUS;
     } 
     else 
       itemState = 0;
     
     if( CB_OWNERDRAWN(lphc) )
     {
       DRAWITEMSTRUCT dis;
       HRGN           clipRegion;
       
       /*
	* Save the current clip region.
	* To retrieve the clip region, we need to create one "dummy"
	* clip region.
	*/
       clipRegion = CreateRectRgnIndirect(&rectEdit);
       
       if (GetClipRgn(hdc, clipRegion)!=1)
       {
	 DeleteObject(clipRegion);
	 clipRegion=(HRGN)NULL;
       }
       
       if ( lphc->self->dwStyle & WS_DISABLED )
	 itemState |= ODS_DISABLED;
       
       dis.CtlType	= ODT_COMBOBOX;
       dis.CtlID	= lphc->self->wIDmenu;
       dis.hwndItem	= lphc->self->hwndSelf;
       dis.itemAction	= ODA_DRAWENTIRE;
       dis.itemID	= id;
       dis.itemState	= itemState;
       dis.hDC		= hdc;
       dis.rcItem	= rectEdit;
       dis.itemData	= SendMessageA( lphc->hWndLBox, LB_GETITEMDATA, 
					(WPARAM)id, 0 );
       
       /*
	* Clip the DC and have the parent draw the item.
	*/
       IntersectClipRect(hdc,
			 rectEdit.left,  rectEdit.top,
			 rectEdit.right, rectEdit.bottom);
       
       SendMessageA(lphc->owner, WM_DRAWITEM, 
		    lphc->self->wIDmenu, (LPARAM)&dis );
       
       /*
	* Reset the clipping region.
	*/
       SelectClipRgn(hdc, clipRegion);		
     }
     else
     {
       ExtTextOutA( hdc, 
		    rectEdit.left + 1, 
		    rectEdit.top + 1,
		    ETO_OPAQUE | ETO_CLIPPED, 
		    &rectEdit,
		    pText ? pText : "" , size, NULL );
       
       if(lphc->wState & CBF_FOCUSED && !(lphc->wState & CBF_DROPPED))
	 DrawFocusRect( hdc, &rectEdit );
     }
     
     if( hPrevFont ) 
       SelectObject(hdc, hPrevFont );
   }
   if (pText)
	HeapFree( GetProcessHeap(), 0, pText );
}

/***********************************************************************
 *           CBPaintBorder
 */
static void CBPaintBorder(
  HWND        hwnd,
  LPHEADCOMBO lphc,   
  HDC         hdc)
{
  RECT clientRect;

  if (CB_GETTYPE(lphc) != CBS_SIMPLE)
  {
    GetClientRect(hwnd, &clientRect);
  }
  else
  {
    CopyRect(&clientRect, &lphc->textRect);

    InflateRect(&clientRect, EDIT_CONTROL_PADDING(), EDIT_CONTROL_PADDING());
    InflateRect(&clientRect, COMBO_XBORDERSIZE(), COMBO_YBORDERSIZE());
  }

  DrawEdge(hdc, &clientRect, EDGE_SUNKEN, BF_RECT);
}

/***********************************************************************
 *           COMBO_PrepareColors
 *
 * This method will sent the appropriate WM_CTLCOLOR message to
 * prepare and setup the colors for the combo's DC.
 *
 * It also returns the brush to use for the background.
 */
static HBRUSH COMBO_PrepareColors(
  HWND        hwnd, 
  LPHEADCOMBO lphc,
  HDC         hDC)
{
  HBRUSH  hBkgBrush;

  /*
   * Get the background brush for this control.
   */
  if (CB_DISABLED(lphc))
  {
    hBkgBrush = SendMessageA( lphc->owner, WM_CTLCOLORSTATIC,
			      hDC, lphc->self->hwndSelf );
    
    /*
     * We have to change the text color since WM_CTLCOLORSTATIC will
     * set it to the "enabled" color. This is the same behavior as the
     * edit control
     */
    SetTextColor(hDC, GetSysColor(COLOR_GRAYTEXT));
  }
  else
  {
    if (lphc->wState & CBF_EDIT)
    {
      hBkgBrush = SendMessageA( lphc->owner, WM_CTLCOLOREDIT,
				hDC, lphc->self->hwndSelf );
    }
    else
    {
      hBkgBrush = SendMessageA( lphc->owner, WM_CTLCOLORLISTBOX,
				hDC, lphc->self->hwndSelf );
    }
  }

  /*
   * Catch errors.
   */
  if( !hBkgBrush ) 
    hBkgBrush = GetSysColorBrush(COLOR_WINDOW);

  return hBkgBrush;
}

/***********************************************************************
 *           COMBO_EraseBackground
 */
static LRESULT COMBO_EraseBackground(
  HWND        hwnd, 
  LPHEADCOMBO lphc,
  HDC         hParamDC)
{
  HBRUSH  hBkgBrush;
  RECT    clientRect;
  HDC 	  hDC;
  
  hDC = (hParamDC) ? hParamDC
		   : GetDC(hwnd);

  /*
   * Calculate the area that we want to erase.
   */
  if (CB_GETTYPE(lphc) != CBS_SIMPLE)
  {
    GetClientRect(hwnd, &clientRect);
  }
  else
  {
    CopyRect(&clientRect, &lphc->textRect);

    InflateRect(&clientRect, COMBO_XBORDERSIZE(), COMBO_YBORDERSIZE());
  }

  /*
   * Retrieve the background brush
   */
  hBkgBrush = COMBO_PrepareColors(hwnd, lphc, hDC);
  
  FillRect(hDC, &clientRect, hBkgBrush);

  if (!hParamDC)
    ReleaseDC(hwnd, hDC);

  return TRUE;
}

/***********************************************************************
 *           COMBO_Paint
 */
static LRESULT COMBO_Paint(LPHEADCOMBO lphc, HDC hParamDC)
{
  PAINTSTRUCT ps;
  HDC 	hDC;
  
  hDC = (hParamDC) ? hParamDC
		   : BeginPaint( lphc->self->hwndSelf, &ps);


  if( hDC && !(lphc->wState & CBF_NOREDRAW) )
  {
      HBRUSH	hPrevBrush, hBkgBrush;

      /*
       * Retrieve the background brush and select it in the
       * DC.
       */
      hBkgBrush = COMBO_PrepareColors(lphc->self->hwndSelf, lphc, hDC);

      hPrevBrush = SelectObject( hDC, hBkgBrush );

      /*
       * In non 3.1 look, there is a sunken border on the combobox
       */
      if (TWEAK_WineLook != WIN31_LOOK)
      {
	CBPaintBorder(CB_HWND(lphc), lphc, hDC);
      }

      if( !IsRectEmpty(&lphc->buttonRect) )
      {
	CBPaintButton(lphc, hDC, lphc->buttonRect);
      }

      if( !(lphc->wState & CBF_EDIT) )
      {
	/*
	 * The text area has a border only in Win 3.1 look.
	 */
	if (TWEAK_WineLook == WIN31_LOOK)
	{
	  HPEN hPrevPen = SelectObject( hDC, GetSysColorPen(COLOR_WINDOWFRAME) );
	  
	  Rectangle( hDC, 
		     lphc->textRect.left, lphc->textRect.top,
		     lphc->textRect.right - 1, lphc->textRect.bottom - 1);

	  SelectObject( hDC, hPrevPen );
	}

	CBPaintText( lphc, hDC, lphc->textRect);
      }

      if( hPrevBrush )
	SelectObject( hDC, hPrevBrush );
  }

  if( !hParamDC ) 
    EndPaint(lphc->self->hwndSelf, &ps);

  return 0;
}

/***********************************************************************
 *           CBUpdateLBox
 *
 * Select listbox entry according to the contents of the edit control.
 */
static INT CBUpdateLBox( LPHEADCOMBO lphc )
{
   INT	length, idx;
   LPSTR	pText = NULL;
   
   idx = LB_ERR;
   length = CB_GETEDITTEXTLENGTH( lphc );
 
   if( length > 0 ) 
       pText = (LPSTR) HeapAlloc( GetProcessHeap(), 0, length + 1);

   TRACE("\t edit text length %i\n", length );

   if( pText )
   {
       if( length ) GetWindowTextA( lphc->hWndEdit, pText, length + 1);
       else pText[0] = '\0';
       idx = SendMessageA( lphc->hWndLBox, LB_FINDSTRING, 
			     (WPARAM)(-1), (LPARAM)pText );
       HeapFree( GetProcessHeap(), 0, pText );
   }

   if( idx >= 0 )
   {
       SendMessageA( lphc->hWndLBox, LB_SETTOPINDEX, (WPARAM)idx, 0 );
       /* probably superfluous but Windows sends this too */
       SendMessageA( lphc->hWndLBox, LB_SETCARETINDEX, (WPARAM)idx, 0 );
   }
   return idx;
}

/***********************************************************************
 *           CBUpdateEdit
 *
 * Copy a listbox entry to the edit control.
 */
static void CBUpdateEdit( LPHEADCOMBO lphc , INT index )
{
   INT	length;
   LPSTR	pText = NULL;

   TRACE("\t %i\n", index );

   if( index >= 0 ) /* got an entry */
   {
       length = SendMessageA( lphc->hWndLBox, LB_GETTEXTLEN, (WPARAM)index, 0);
       if( length )
       {
	   if( (pText = (LPSTR) HeapAlloc( GetProcessHeap(), 0, length + 1)) )
	   {
		SendMessageA( lphc->hWndLBox, LB_GETTEXT, 
				(WPARAM)index, (LPARAM)pText );
	   }
       }
   }

   lphc->wState |= CBF_NOEDITNOTIFY;
   SendMessageA( lphc->hWndEdit, WM_SETTEXT, 0, pText ? (LPARAM)pText : (LPARAM)"" );
   lphc->wState &= ~CBF_NOEDITNOTIFY;

   if( lphc->wState & CBF_FOCUSED )
      SendMessageA( lphc->hWndEdit, EM_SETSEL, 0, (LPARAM)(-1) );

   if( pText )
       HeapFree( GetProcessHeap(), 0, pText );
}

/***********************************************************************
 *           CBDropDown
 * 
 * Show listbox popup.
 */
static void CBDropDown( LPHEADCOMBO lphc )
{
   RECT	rect;
   int nItems = 0;
   int i;
   int nHeight;
   int nDroppedHeight, nTempDroppedHeight;

   TRACE("[%04x]: drop down\n", CB_HWND(lphc));

   CB_NOTIFY( lphc, CBN_DROPDOWN );

   /* set selection */

   lphc->wState |= CBF_DROPPED;
   if( CB_GETTYPE(lphc) == CBS_DROPDOWN )
   {
       lphc->droppedIndex = CBUpdateLBox( lphc );

       if( !(lphc->wState & CBF_CAPTURE) ) 
	 CBUpdateEdit( lphc, lphc->droppedIndex );
   }
   else
   {
       lphc->droppedIndex = SendMessageA( lphc->hWndLBox, LB_GETCURSEL, 0, 0 );

       if( lphc->droppedIndex == LB_ERR ) 
	 lphc->droppedIndex = 0;

       SendMessageA( lphc->hWndLBox, LB_SETTOPINDEX, (WPARAM)lphc->droppedIndex, 0 );
       SendMessageA( lphc->hWndLBox, LB_CARETON, 0, 0 );
   }

   /* now set popup position */
   GetWindowRect( lphc->self->hwndSelf, &rect );
   
   /*
    * If it's a dropdown, the listbox is offset
    */
   if( CB_GETTYPE(lphc) == CBS_DROPDOWN )
     rect.left += COMBO_EDITBUTTONSPACE();

  /* if the dropped height is greater than the total height of the dropped
     items list, then force the drop down list height to be the total height
     of the items in the dropped list */

  /* And Remove any extra space (Best Fit) */
   nDroppedHeight = lphc->droppedRect.bottom - lphc->droppedRect.top;
   nItems = (int)SendMessageA (lphc->hWndLBox, LB_GETCOUNT, 0, 0);
   nHeight = COMBO_YBORDERSIZE();
   nTempDroppedHeight = 0;
   for (i = 0; i < nItems; i++)
   {
     nHeight += (int)SendMessageA (lphc->hWndLBox, LB_GETITEMHEIGHT, i, 0);

     /* Did we pass the limit of what can be displayed */
     if (nHeight > nDroppedHeight)
     {
       break;
   }
     nTempDroppedHeight = nHeight;
   }

   nDroppedHeight = nTempDroppedHeight;

   SetWindowPos( lphc->hWndLBox, HWND_TOP, rect.left, rect.bottom, 
		 lphc->droppedRect.right - lphc->droppedRect.left,
      nDroppedHeight,
		 SWP_NOACTIVATE | SWP_NOREDRAW);

   if( !(lphc->wState & CBF_NOREDRAW) )
     RedrawWindow( lphc->self->hwndSelf, NULL, 0, RDW_INVALIDATE | 
			   RDW_ERASE | RDW_UPDATENOW | RDW_NOCHILDREN );

   EnableWindow( lphc->hWndLBox, TRUE );
   ShowWindow( lphc->hWndLBox, SW_SHOW);
}

/***********************************************************************
 *           CBRollUp
 *
 * Hide listbox popup.
 */
static void CBRollUp( LPHEADCOMBO lphc, BOOL ok, BOOL bButton )
{
   HWND	hWnd = lphc->self->hwndSelf;

   CB_NOTIFY( lphc, (ok) ? CBN_SELENDOK : CBN_SELENDCANCEL );

   if( IsWindow( hWnd ) && CB_GETTYPE(lphc) != CBS_SIMPLE )
   {

       TRACE("[%04x]: roll up [%i]\n", CB_HWND(lphc), (INT)ok );

       if( lphc->wState & CBF_DROPPED ) 
       {
	   RECT	rect;

	   lphc->wState &= ~CBF_DROPPED;
	   ShowWindow( lphc->hWndLBox, SW_HIDE );
           if(GetCapture() == lphc->hWndLBox)
           {
               ReleaseCapture();
           }

	   if( CB_GETTYPE(lphc) == CBS_DROPDOWN )
	   {
	       INT index = SendMessageA( lphc->hWndLBox, LB_GETCURSEL, 0, 0 );
	       CBUpdateEdit( lphc, index );
	       rect = lphc->buttonRect;
	   }
	   else 
           {
	       if( bButton )
	       {
		 UnionRect( &rect,
			    &lphc->buttonRect,
			    &lphc->textRect);
	       }
	       else
		 rect = lphc->textRect;

	       bButton = TRUE;
	   }

	   if( bButton && !(lphc->wState & CBF_NOREDRAW) )
	       RedrawWindow( hWnd, &rect, 0, RDW_INVALIDATE | 
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
BOOL COMBO_FlipListbox( LPHEADCOMBO lphc, BOOL bRedrawButton )
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
HWND COMBO_GetLBWindow( WND* pWnd )
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
  InvalidateRect(CB_HWND(lphc), &lphc->buttonRect, TRUE);
  UpdateWindow(CB_HWND(lphc));
}

/***********************************************************************
 *           COMBO_SetFocus
 */
static void COMBO_SetFocus( LPHEADCOMBO lphc )
{
   if( !(lphc->wState & CBF_FOCUSED) )
   {
       if( CB_GETTYPE(lphc) == CBS_DROPDOWNLIST )
           SendMessageA( lphc->hWndLBox, LB_CARETON, 0, 0 );

       lphc->wState |= CBF_FOCUSED;

       if( !(lphc->wState & CBF_EDIT) )
	 InvalidateRect(CB_HWND(lphc), &lphc->textRect, TRUE);

       CB_NOTIFY( lphc, CBN_SETFOCUS );
   }
}

/***********************************************************************
 *           COMBO_KillFocus
 */
static void COMBO_KillFocus( LPHEADCOMBO lphc )
{
   HWND	hWnd = lphc->self->hwndSelf;

   if( lphc->wState & CBF_FOCUSED )
   {
       CBRollUp( lphc, FALSE, TRUE );
       if( IsWindow( hWnd ) )
       {
           if( CB_GETTYPE(lphc) == CBS_DROPDOWNLIST )
               SendMessageA( lphc->hWndLBox, LB_CARETOFF, 0, 0 );

 	   lphc->wState &= ~CBF_FOCUSED;

           /* redraw text */
	   if( !(lphc->wState & CBF_EDIT) )
	     InvalidateRect(CB_HWND(lphc), &lphc->textRect, TRUE);

           CB_NOTIFY( lphc, CBN_KILLFOCUS );
       }
   }
}

/***********************************************************************
 *           COMBO_Command
 */
static LRESULT COMBO_Command( LPHEADCOMBO lphc, WPARAM wParam, HWND hWnd )
{
   if ( lphc->wState & CBF_EDIT && lphc->hWndEdit == hWnd )
   {
       /* ">> 8" makes gcc generate jump-table instead of cmp ladder */

       switch( HIWORD(wParam) >> 8 )
       {   
	   case (EN_SETFOCUS >> 8):

		TRACE("[%04x]: edit [%04x] got focus\n", 
			     CB_HWND(lphc), lphc->hWndEdit );

		COMBO_SetFocus( lphc );
	        break;

	   case (EN_KILLFOCUS >> 8):

		TRACE("[%04x]: edit [%04x] lost focus\n",
			     CB_HWND(lphc), lphc->hWndEdit );

		/* NOTE: it seems that Windows' edit control sends an
		 * undocumented message WM_USER + 0x1B instead of this
		 * notification (only when it happens to be a part of 
		 * the combo). ?? - AK.
		 */

		COMBO_KillFocus( lphc );
		break;


	   case (EN_CHANGE >> 8):
	       /*
	        * In some circumstances (when the selection of the combobox
		* is changed for example) we don't wans the EN_CHANGE notification
		* to be forwarded to the parent of the combobox. This code
		* checks a flag that is set in these occasions and ignores the 
		* notification.
	        */
	        if (!(lphc->wState & CBF_NOEDITNOTIFY))
		  CB_NOTIFY( lphc, CBN_EDITCHANGE );

		if (lphc->wState & CBF_NOLBSELECT)
		{
		  lphc->wState &= ~CBF_NOLBSELECT;
		}
		else
		{
		  CBUpdateLBox( lphc );
		}
		break;

	   case (EN_UPDATE >> 8):
	        if (!(lphc->wState & CBF_NOEDITNOTIFY))
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

		TRACE("[%04x]: lbox selection change [%04x]\n", 
			     CB_HWND(lphc), lphc->wState );

		/* do not roll up if selection is being tracked 
		 * by arrowkeys in the dropdown listbox */

                if( (lphc->dwStyle & CBS_SIMPLE) ||
                    ((lphc->wState & CBF_DROPPED) && !(lphc->wState & CBF_NOROLLUP)) )
                {
                   CBRollUp( lphc, (HIWORD(wParam) == LBN_SELCHANGE), TRUE );
                }
		else lphc->wState &= ~CBF_NOROLLUP;

		if( lphc->wState & CBF_EDIT )
		{
		    INT index = SendMessageA(lphc->hWndLBox, LB_GETCURSEL, 0, 0);
                    lphc->wState |= CBF_NOLBSELECT;
		    CBUpdateEdit( lphc, index );
		    /* select text in edit, as Windows does */
		    SendMessageA( lphc->hWndEdit, EM_SETSEL, 0, (LPARAM)(-1) );
		}
		else
		    InvalidateRect(CB_HWND(lphc), &lphc->textRect, TRUE);

		CB_NOTIFY( lphc, CBN_SELCHANGE );
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
static LRESULT COMBO_ItemOp( LPHEADCOMBO lphc, UINT msg, 
			       WPARAM wParam, LPARAM lParam ) 
{
   HWND	hWnd = lphc->self->hwndSelf;

   TRACE("[%04x]: ownerdraw op %04x\n", CB_HWND(lphc), msg );

#define lpIS    ((LPDELETEITEMSTRUCT)lParam)

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
#define lpIS    ((LPDRAWITEMSTRUCT)lParam)
	     lpIS->hwndItem = hWnd; 
#undef  lpIS
	     break;
	case WM_COMPAREITEM: 
#define lpIS    ((LPCOMPAREITEMSTRUCT)lParam)
	     lpIS->hwndItem = hWnd; 
#undef  lpIS
	     break;
   }

   return SendMessageA( lphc->owner, msg, lphc->self->wIDmenu, lParam );
}

/***********************************************************************
 *           COMBO_GetText
 */
static LRESULT COMBO_GetText( LPHEADCOMBO lphc, UINT N, LPSTR lpText)
{
   if( lphc->wState & CBF_EDIT )
       return SendMessageA( lphc->hWndEdit, WM_GETTEXT, 
			     (WPARAM)N, (LPARAM)lpText );     

   /* get it from the listbox */

   if( lphc->hWndLBox )
   {
       INT idx = SendMessageA( lphc->hWndLBox, LB_GETCURSEL, 0, 0 );
       if( idx != LB_ERR )
       {
           LPSTR	lpBuffer;
           INT	length = SendMessageA( lphc->hWndLBox, LB_GETTEXTLEN,
					        (WPARAM)idx, 0 );

           /* 'length' is without the terminating character */
           if( length >= N )
	       lpBuffer = (LPSTR) HeapAlloc( GetProcessHeap(), 0, length + 1 );
           else 
	       lpBuffer = lpText;

           if( lpBuffer )
           {
	       INT    n = SendMessageA( lphc->hWndLBox, LB_GETTEXT, 
					   (WPARAM)idx, (LPARAM)lpBuffer );

	       /* truncate if buffer is too short */

	       if( length >= N )
	       {
	       	   if (N && lpText) {
	           if( n != LB_ERR ) memcpy( lpText, lpBuffer, (N>n) ? n+1 : N-1 );
	           lpText[N - 1] = '\0';
                   }
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
static void CBResetPos(
  LPHEADCOMBO lphc, 
  LPRECT      rectEdit,
  LPRECT      rectLB,
  BOOL        bRedraw)
{
   BOOL	bDrop = (CB_GETTYPE(lphc) != CBS_SIMPLE);

   /* NOTE: logs sometimes have WM_LBUTTONUP before a cascade of
    * sizing messages */

   if( lphc->wState & CBF_EDIT )
     SetWindowPos( lphc->hWndEdit, 0, 
		   rectEdit->left, rectEdit->top,
		   rectEdit->right - rectEdit->left,
		   rectEdit->bottom - rectEdit->top,
                       SWP_NOZORDER | SWP_NOACTIVATE | ((bDrop) ? SWP_NOREDRAW : 0) );

   SetWindowPos( lphc->hWndLBox, 0,
		 rectLB->left, rectLB->top,
                 rectLB->right - rectLB->left, 
		 rectLB->bottom - rectLB->top, 
		   SWP_NOACTIVATE | SWP_NOZORDER | ((bDrop) ? SWP_NOREDRAW : 0) );

   if( bDrop )
   {
       if( lphc->wState & CBF_DROPPED )
       {
           lphc->wState &= ~CBF_DROPPED;
           ShowWindow( lphc->hWndLBox, SW_HIDE );
       }

       if( bRedraw && !(lphc->wState & CBF_NOREDRAW) )
           RedrawWindow( lphc->self->hwndSelf, NULL, 0,
                           RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW );
   }
}


/***********************************************************************
 *           COMBO_Size
 */
static void COMBO_Size( LPHEADCOMBO lphc )
  {
  CBCalcPlacement(lphc->self->hwndSelf,
		  lphc, 
		  &lphc->textRect, 
		  &lphc->buttonRect, 
		  &lphc->droppedRect);

  CBResetPos( lphc, &lphc->textRect, &lphc->droppedRect, TRUE );
}


/***********************************************************************
 *           COMBO_Font
 */
static void COMBO_Font( LPHEADCOMBO lphc, HFONT hFont, BOOL bRedraw )
{
  /*
   * Set the font
   */
  lphc->hFont = hFont;

  /*
   * Propagate to owned windows.
   */
  if( lphc->wState & CBF_EDIT )
      SendMessageA( lphc->hWndEdit, WM_SETFONT, (WPARAM)hFont, bRedraw );
  SendMessageA( lphc->hWndLBox, WM_SETFONT, (WPARAM)hFont, bRedraw );

  /*
   * Redo the layout of the control.
   */
  if ( CB_GETTYPE(lphc) == CBS_SIMPLE)
  {
    CBCalcPlacement(lphc->self->hwndSelf,
		    lphc, 
		    &lphc->textRect, 
		    &lphc->buttonRect, 
		    &lphc->droppedRect);
    
    CBResetPos( lphc, &lphc->textRect, &lphc->droppedRect, TRUE );
  }
  else
  {
    CBForceDummyResize(lphc);
  }
}


/***********************************************************************
 *           COMBO_SetItemHeight
 */
static LRESULT COMBO_SetItemHeight( LPHEADCOMBO lphc, INT index, INT height )
{
   LRESULT	lRet = CB_ERR;

   if( index == -1 ) /* set text field height */
   {
       if( height < 32768 )
       {
           lphc->editHeight = height;

	 /*
	  * Redo the layout of the control.
	  */
	 if ( CB_GETTYPE(lphc) == CBS_SIMPLE)
	 {
	   CBCalcPlacement(lphc->self->hwndSelf,
			   lphc, 
			   &lphc->textRect, 
			   &lphc->buttonRect, 
			   &lphc->droppedRect);
	   
	   CBResetPos( lphc, &lphc->textRect, &lphc->droppedRect, TRUE );
	 }
	 else
	 {
	   CBForceDummyResize(lphc);
	 }
	 	 
	   lRet = height;
       }
   } 
   else if ( CB_OWNERDRAWN(lphc) )	/* set listbox item height */
	lRet = SendMessageA( lphc->hWndLBox, LB_SETITEMHEIGHT, 
			      (WPARAM)index, (LPARAM)height );
   return lRet;
}

/***********************************************************************
 *           COMBO_SelectString
 */
static LRESULT COMBO_SelectString( LPHEADCOMBO lphc, INT start, LPCSTR pText )
{
   INT index = SendMessageA( lphc->hWndLBox, LB_SELECTSTRING, 
				 (WPARAM)start, (LPARAM)pText );
   if( index >= 0 )
   {
     if( lphc->wState & CBF_EDIT )
       CBUpdateEdit( lphc, index );
     else
     {
       InvalidateRect(CB_HWND(lphc), &lphc->textRect, TRUE);
     }
   }
   return (LRESULT)index;
}

/***********************************************************************
 *           COMBO_LButtonDown
 */
static void COMBO_LButtonDown( LPHEADCOMBO lphc, LPARAM lParam )
{
   POINT     pt;
   BOOL      bButton;
   HWND      hWnd = lphc->self->hwndSelf;

   pt.x = LOWORD(lParam);
   pt.y = HIWORD(lParam);
   bButton = PtInRect(&lphc->buttonRect, pt);

   if( (CB_GETTYPE(lphc) == CBS_DROPDOWNLIST) ||
       (bButton && (CB_GETTYPE(lphc) == CBS_DROPDOWN)) )
   {
       lphc->wState |= CBF_BUTTONDOWN;
       if( lphc->wState & CBF_DROPPED )
       {
	   /* got a click to cancel selection */

           lphc->wState &= ~CBF_BUTTONDOWN;
           CBRollUp( lphc, TRUE, FALSE );
	   if( !IsWindow( hWnd ) ) return;

           if( lphc->wState & CBF_CAPTURE )
           {
               lphc->wState &= ~CBF_CAPTURE;
               ReleaseCapture();
           }
       }
       else
       {
	   /* drop down the listbox and start tracking */

           lphc->wState |= CBF_CAPTURE;
           CBDropDown( lphc );
           SetCapture( hWnd );
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
	   INT index = CBUpdateLBox( lphc );
	   CBUpdateEdit( lphc, index );
       }
       ReleaseCapture();
       SetCapture(lphc->hWndLBox);

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
static void COMBO_MouseMove( LPHEADCOMBO lphc, WPARAM wParam, LPARAM lParam )
{
   POINT  pt;
   RECT   lbRect;
   
   pt.x = LOWORD(lParam);
   pt.y = HIWORD(lParam);
   
   if( lphc->wState & CBF_BUTTONDOWN )
   {
     BOOL bButton;

     bButton = PtInRect(&lphc->buttonRect, pt);

     if( !bButton )
     {
       lphc->wState &= ~CBF_BUTTONDOWN;
       CBRepaintButton( lphc );
     }
   }

   GetClientRect( lphc->hWndLBox, &lbRect );
   MapWindowPoints( lphc->self->hwndSelf, lphc->hWndLBox, &pt, 1 );
   if( PtInRect(&lbRect, pt) )
   {
       lphc->wState &= ~CBF_CAPTURE;
       ReleaseCapture();
       if( CB_GETTYPE(lphc) == CBS_DROPDOWN ) CBUpdateLBox( lphc );

       /* hand over pointer tracking */
       SendMessageA( lphc->hWndLBox, WM_LBUTTONDOWN, wParam, lParam );
   }
}


/***********************************************************************
 *           ComboWndProc_locked
 *
 * http://www.microsoft.com/msdn/sdk/platforms/doc/sdk/win32/ctrl/src/combobox_15.htm
 */
static inline LRESULT WINAPI ComboWndProc_locked( WND* pWnd, UINT message,
                             WPARAM wParam, LPARAM lParam )
{
    if( pWnd ) {
      LPHEADCOMBO	lphc = CB_GETPTR(pWnd);
      HWND		hwnd = pWnd->hwndSelf;

      TRACE("[%04x]: msg %s wp %08x lp %08lx\n",
		   pWnd->hwndSelf, SPY_GetMsgName(message), wParam, lParam );

      if( lphc || message == WM_NCCREATE )
      switch(message) 
      {	

	/* System messages */

     	case WM_NCCREATE: 
                return COMBO_NCCreate(pWnd, lParam);
     	case WM_NCDESTROY: 
		COMBO_NCDestroy(lphc);
		break;/* -> DefWindowProc */

     	case WM_CREATE: 
                return COMBO_Create(lphc, pWnd, lParam);

        case WM_PRINTCLIENT:
	        if (lParam & PRF_ERASEBKGND)
		  COMBO_EraseBackground(hwnd, lphc, wParam);

		/* Fallthrough */
     	case WM_PAINT:
		/* wParam may contain a valid HDC! */
		return  COMBO_Paint(lphc, wParam);
	case WM_ERASEBKGND:
		return  COMBO_EraseBackground(hwnd, lphc, wParam);
     	case WM_GETDLGCODE: 
		return  (LRESULT)(DLGC_WANTARROWS | DLGC_WANTCHARS);
        case WM_WINDOWPOSCHANGING:
	        return  COMBO_WindowPosChanging(hwnd, lphc, (LPWINDOWPOS)lParam);
	case WM_SIZE:
	        if( lphc->hWndLBox && 
		  !(lphc->wState & CBF_NORESIZE) ) COMBO_Size( lphc );
		return  TRUE;
	case WM_SETFONT:
		COMBO_Font( lphc, (HFONT16)wParam, (BOOL)lParam );
		return  TRUE;
	case WM_GETFONT:
		return  (LRESULT)lphc->hFont;
	case WM_SETFOCUS:
		if( lphc->wState & CBF_EDIT )
		    SetFocus( lphc->hWndEdit );
		else
		    COMBO_SetFocus( lphc );
		return  TRUE;
	case WM_KILLFOCUS:
#define hwndFocus ((HWND16)wParam)
		if( !hwndFocus ||
		    (hwndFocus != lphc->hWndEdit && hwndFocus != lphc->hWndLBox ))
		    COMBO_KillFocus( lphc );
#undef hwndFocus
		return  TRUE;
	case WM_COMMAND:
		return  COMBO_Command( lphc, wParam, (HWND)lParam );
	case WM_GETTEXT:
		return  COMBO_GetText( lphc, (UINT)wParam, (LPSTR)lParam );
	case WM_SETTEXT:
	case WM_GETTEXTLENGTH:
	case WM_CLEAR:
	case WM_CUT:
        case WM_PASTE:
	case WM_COPY:
                if ((message == WM_GETTEXTLENGTH) && !ISWIN31 && !(lphc->wState & CBF_EDIT))
                {
                int j = SendMessageA( lphc->hWndLBox, LB_GETCURSEL, 0, 0 );
                if (j == -1) return 0;
                return SendMessageA( lphc->hWndLBox, LB_GETTEXTLEN, j, 0);
                }
		else if( lphc->wState & CBF_EDIT ) 
		{
		    LRESULT ret;
		    lphc->wState |= CBF_NOEDITNOTIFY;
		    ret = SendMessageA( lphc->hWndEdit, message, wParam, lParam );
		    lphc->wState &= ~CBF_NOEDITNOTIFY;
		    return ret;
		}
		else return  CB_ERR;

	case WM_DRAWITEM:
	case WM_DELETEITEM:
	case WM_COMPAREITEM:
	case WM_MEASUREITEM:
		return  COMBO_ItemOp( lphc, message, wParam, lParam );
	case WM_ENABLE:
		if( lphc->wState & CBF_EDIT )
		    EnableWindow( lphc->hWndEdit, (BOOL)wParam ); 
		EnableWindow( lphc->hWndLBox, (BOOL)wParam );

		/* Force the control to repaint when the enabled state changes. */
		InvalidateRect(CB_HWND(lphc), NULL, TRUE);
		return  TRUE;
	case WM_SETREDRAW:
		if( wParam )
		    lphc->wState &= ~CBF_NOREDRAW;
		else
		    lphc->wState |= CBF_NOREDRAW;

		if( lphc->wState & CBF_EDIT )
		    SendMessageA( lphc->hWndEdit, message, wParam, lParam );
		SendMessageA( lphc->hWndLBox, message, wParam, lParam );
		return  0;
	case WM_SYSKEYDOWN:
		if( KEYDATA_ALT & HIWORD(lParam) )
		    if( wParam == VK_UP || wParam == VK_DOWN )
			COMBO_FlipListbox( lphc, TRUE );
		break;/* -> DefWindowProc */

	case WM_CHAR:
	case WM_KEYDOWN:
		if( lphc->wState & CBF_EDIT )
		    return  SendMessageA( lphc->hWndEdit, message, wParam, lParam );
		else
		    return  SendMessageA( lphc->hWndLBox, message, wParam, lParam );
	case WM_LBUTTONDOWN: 
		if( !(lphc->wState & CBF_FOCUSED) ) SetFocus( lphc->self->hwndSelf );
		if( lphc->wState & CBF_FOCUSED ) COMBO_LButtonDown( lphc, lParam );
		return  TRUE;
	case WM_LBUTTONUP:
		COMBO_LButtonUp( lphc, lParam );
		return  TRUE;
	case WM_MOUSEMOVE: 
		if( lphc->wState & CBF_CAPTURE ) 
		    COMBO_MouseMove( lphc, wParam, lParam );
		return  TRUE;

        case WM_MOUSEWHEEL:
                if (wParam & (MK_SHIFT | MK_CONTROL))
                    return DefWindowProcA( hwnd, message, wParam, lParam );
                if ((short) HIWORD(wParam) > 0) return SendMessageA(hwnd,WM_KEYDOWN,VK_UP,0);
                if ((short) HIWORD(wParam) < 0) return SendMessageA(hwnd,WM_KEYDOWN,VK_DOWN,0);
                return TRUE;

	/* Combo messages */

	case CB_ADDSTRING16:
		if( CB_HASSTRINGS(lphc) ) lParam = (LPARAM)PTR_SEG_TO_LIN(lParam);
	case CB_ADDSTRING:
		return  SendMessageA( lphc->hWndLBox, LB_ADDSTRING, 0, lParam);
	case CB_INSERTSTRING16:
		wParam = (INT)(INT16)wParam;
		if( CB_HASSTRINGS(lphc) ) lParam = (LPARAM)PTR_SEG_TO_LIN(lParam);
	case CB_INSERTSTRING:
		return  SendMessageA( lphc->hWndLBox, LB_INSERTSTRING, wParam, lParam);
	case CB_DELETESTRING16:
	case CB_DELETESTRING:
		return  SendMessageA( lphc->hWndLBox, LB_DELETESTRING, wParam, 0);
	case CB_SELECTSTRING16:
		wParam = (INT)(INT16)wParam;
		if( CB_HASSTRINGS(lphc) ) lParam = (LPARAM)PTR_SEG_TO_LIN(lParam);
	case CB_SELECTSTRING:
		return  COMBO_SelectString( lphc, (INT)wParam, (LPSTR)lParam );
	case CB_FINDSTRING16:
		wParam = (INT)(INT16)wParam;
		if( CB_HASSTRINGS(lphc) ) lParam = (LPARAM)PTR_SEG_TO_LIN(lParam);
	case CB_FINDSTRING:
		return  SendMessageA( lphc->hWndLBox, LB_FINDSTRING, wParam, lParam);
	case CB_FINDSTRINGEXACT16:
		wParam = (INT)(INT16)wParam;
		if( CB_HASSTRINGS(lphc) ) lParam = (LPARAM)PTR_SEG_TO_LIN(lParam);
	case CB_FINDSTRINGEXACT:
		return  SendMessageA( lphc->hWndLBox, LB_FINDSTRINGEXACT, 
						       wParam, lParam );
	case CB_SETITEMHEIGHT16:
		wParam = (INT)(INT16)wParam;	/* signed integer */
	case CB_SETITEMHEIGHT:
		return  COMBO_SetItemHeight( lphc, (INT)wParam, (INT)lParam);
	case CB_GETITEMHEIGHT16:
		wParam = (INT)(INT16)wParam;
	case CB_GETITEMHEIGHT:
		if( (INT)wParam >= 0 )	/* listbox item */
                    return  SendMessageA( lphc->hWndLBox, LB_GETITEMHEIGHT, wParam, 0);
                return  CBGetTextAreaHeight(hwnd, lphc);
	case CB_RESETCONTENT16: 
	case CB_RESETCONTENT:
		SendMessageA( lphc->hWndLBox, LB_RESETCONTENT, 0, 0 );
		InvalidateRect(CB_HWND(lphc), NULL, TRUE);
		return  TRUE;
	case CB_INITSTORAGE:
		return  SendMessageA( lphc->hWndLBox, LB_INITSTORAGE, wParam, lParam);
	case CB_GETHORIZONTALEXTENT:
		return  SendMessageA( lphc->hWndLBox, LB_GETHORIZONTALEXTENT, 0, 0);
	case CB_SETHORIZONTALEXTENT:
		return  SendMessageA( lphc->hWndLBox, LB_SETHORIZONTALEXTENT, wParam, 0);
	case CB_GETTOPINDEX:
		return  SendMessageA( lphc->hWndLBox, LB_GETTOPINDEX, 0, 0);
	case CB_GETLOCALE:
		return  SendMessageA( lphc->hWndLBox, LB_GETLOCALE, 0, 0);
	case CB_SETLOCALE:
		return  SendMessageA( lphc->hWndLBox, LB_SETLOCALE, wParam, 0);
	case CB_GETDROPPEDWIDTH:
		if( lphc->droppedWidth )
                    return  lphc->droppedWidth;
		return  lphc->droppedRect.right - lphc->droppedRect.left;
	case CB_SETDROPPEDWIDTH:
		if( (CB_GETTYPE(lphc) != CBS_SIMPLE) &&
		    (INT)wParam < 32768 ) lphc->droppedWidth = (INT)wParam;
		return  CB_ERR;
	case CB_GETDROPPEDCONTROLRECT16:
		lParam = (LPARAM)PTR_SEG_TO_LIN(lParam);
		if( lParam ) 
		{
		    RECT	r;
		    CBGetDroppedControlRect( lphc, &r );
		    CONV_RECT32TO16( &r, (LPRECT16)lParam );
		}
		return  CB_OKAY;
	case CB_GETDROPPEDCONTROLRECT:
		if( lParam ) CBGetDroppedControlRect(lphc, (LPRECT)lParam );
		return  CB_OKAY;
	case CB_GETDROPPEDSTATE16:
	case CB_GETDROPPEDSTATE:
		return  (lphc->wState & CBF_DROPPED) ? TRUE : FALSE;
	case CB_DIR16: 
                lParam = (LPARAM)PTR_SEG_TO_LIN(lParam);
                /* fall through */
	case CB_DIR:
		return  COMBO_Directory( lphc, (UINT)wParam, 
				       (LPSTR)lParam, (message == CB_DIR));
	case CB_SHOWDROPDOWN16:
	case CB_SHOWDROPDOWN:
		if( CB_GETTYPE(lphc) != CBS_SIMPLE )
		{
		    if( wParam )
		    {
			if( !(lphc->wState & CBF_DROPPED) )
			    CBDropDown( lphc );
		    }
		    else 
			if( lphc->wState & CBF_DROPPED ) 
		            CBRollUp( lphc, FALSE, TRUE );
		}
		return  TRUE;
	case CB_GETCOUNT16: 
	case CB_GETCOUNT:
		return  SendMessageA( lphc->hWndLBox, LB_GETCOUNT, 0, 0);
	case CB_GETCURSEL16: 
	case CB_GETCURSEL:
		return  SendMessageA( lphc->hWndLBox, LB_GETCURSEL, 0, 0);
	case CB_SETCURSEL16:
		wParam = (INT)(INT16)wParam;
	case CB_SETCURSEL:
		lParam = SendMessageA( lphc->hWndLBox, LB_SETCURSEL, wParam, 0);
	        if( lParam >= 0 )
	            SendMessageA( lphc->hWndLBox, LB_SETTOPINDEX, wParam, 0);
   	        if( lphc->wState & CBF_SELCHANGE )
		{
		    /* no LBN_SELCHANGE in this case, update manually */
		    if( lphc->wState & CBF_EDIT )
			CBUpdateEdit( lphc, (INT)wParam );
		    else
			InvalidateRect(CB_HWND(lphc), &lphc->textRect, TRUE);
		    lphc->wState &= ~CBF_SELCHANGE;
		}
	        return  lParam;
	case CB_GETLBTEXT16: 
		wParam = (INT)(INT16)wParam;
		lParam = (LPARAM)PTR_SEG_TO_LIN(lParam);
	case CB_GETLBTEXT:
		return  SendMessageA( lphc->hWndLBox, LB_GETTEXT, wParam, lParam);
	case CB_GETLBTEXTLEN16: 
		wParam = (INT)(INT16)wParam;
	case CB_GETLBTEXTLEN:
		return  SendMessageA( lphc->hWndLBox, LB_GETTEXTLEN, wParam, 0);
	case CB_GETITEMDATA16:
		wParam = (INT)(INT16)wParam;
	case CB_GETITEMDATA:
		return  SendMessageA( lphc->hWndLBox, LB_GETITEMDATA, wParam, 0);
	case CB_SETITEMDATA16:
		wParam = (INT)(INT16)wParam;
	case CB_SETITEMDATA:
		return  SendMessageA( lphc->hWndLBox, LB_SETITEMDATA, wParam, lParam);
	case CB_GETEDITSEL16: 
		wParam = lParam = 0;   /* just in case */
	case CB_GETEDITSEL:
		if( lphc->wState & CBF_EDIT )
		{
		    INT	a, b;

		    return  SendMessageA( lphc->hWndEdit, EM_GETSEL,
					   (wParam) ? wParam : (WPARAM)&a,
					   (lParam) ? lParam : (LPARAM)&b );
		}
		return  CB_ERR;
	case CB_SETEDITSEL16: 
	case CB_SETEDITSEL:
		if( lphc->wState & CBF_EDIT ) 
                    return  SendMessageA( lphc->hWndEdit, EM_SETSEL,
			  (INT)(INT16)LOWORD(lParam), (INT)(INT16)HIWORD(lParam) );
		return  CB_ERR;
	case CB_SETEXTENDEDUI16:
	case CB_SETEXTENDEDUI:
                if( CB_GETTYPE(lphc) == CBS_SIMPLE )
                    return  CB_ERR;
		if( wParam )
		    lphc->wState |= CBF_EUI;
		else lphc->wState &= ~CBF_EUI;
		return  CB_OKAY;
	case CB_GETEXTENDEDUI16:
	case CB_GETEXTENDEDUI:
		return  (lphc->wState & CBF_EUI) ? TRUE : FALSE;

	default:
		if (message >= WM_USER)
		    WARN("unknown msg WM_USER+%04x wp=%04x lp=%08lx\n",
			message - WM_USER, wParam, lParam );
		break;
    }
    return DefWindowProcA(hwnd, message, wParam, lParam);
  }
  return CB_ERR;
}

/***********************************************************************
 *           ComboWndProc
 *
 * This is just a wrapper for the real ComboWndProc which locks/unlocks
 * window structs.
 */
LRESULT WINAPI ComboWndProc( HWND hwnd, UINT message,
                             WPARAM wParam, LPARAM lParam )
{
    WND*	pWnd = WIN_FindWndPtr(hwnd);
    LRESULT retvalue = ComboWndProc_locked(pWnd,message,wParam,lParam);

    
    WIN_ReleaseWndPtr(pWnd);
    return retvalue;
}

