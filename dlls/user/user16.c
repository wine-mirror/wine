/*
 * Misc 16-bit USER functions
 *
 * Copyright 2002 Patrik Stridvall
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

#include "wine/winuser16.h"
#include "winbase.h"
#include "wownt32.h"
#include "user.h"
#include "win.h"

/* handle to handle 16 conversions */
#define HANDLE_16(h32)		(LOWORD(h32))

/* handle16 to handle conversions */
#define HANDLE_32(h16)		((HANDLE)(ULONG_PTR)(h16))
#define HINSTANCE_32(h16)	((HINSTANCE)(ULONG_PTR)(h16))

WORD WINAPI DestroyIcon32(HGLOBAL16, UINT16);


/***********************************************************************
 *		SetCursor (USER.69)
 */
HCURSOR16 WINAPI SetCursor16(HCURSOR16 hCursor)
{
  return HCURSOR_16(SetCursor(HCURSOR_32(hCursor)));
}

/***********************************************************************
 *		ShowCursor (USER.71)
 */
INT16 WINAPI ShowCursor16(BOOL16 bShow)
{
  return ShowCursor(bShow);
}

/***********************************************************************
 *		DrawIcon (USER.84)
 */
BOOL16 WINAPI DrawIcon16(HDC16 hdc, INT16 x, INT16 y, HICON16 hIcon)
{
  return DrawIcon(HDC_32(hdc), x, y, HICON_32(hIcon));
}

/***********************************************************************
 *		IconSize (USER.86)
 *
 * See "Undocumented Windows". Used by W2.0 paint.exe.
 */
DWORD WINAPI IconSize16(void)
{
  return MAKELONG(GetSystemMetrics(SM_CYICON), GetSystemMetrics(SM_CXICON));
}

/***********************************************************************
 *		LoadCursor (USER.173)
 */
HCURSOR16 WINAPI LoadCursor16(HINSTANCE16 hInstance, LPCSTR name)
{
  return HCURSOR_16(LoadCursorA(HINSTANCE_32(hInstance), name));
}


/***********************************************************************
 *		LoadIcon (USER.174)
 */
HICON16 WINAPI LoadIcon16(HINSTANCE16 hInstance, LPCSTR name)
{
  return HICON_16(LoadIconA(HINSTANCE_32(hInstance), name));
}

/**********************************************************************
 *		LoadBitmap (USER.175)
 */
HBITMAP16 WINAPI LoadBitmap16(HINSTANCE16 hInstance, LPCSTR name)
{
  return HBITMAP_16(LoadBitmapA(HINSTANCE_32(hInstance), name));
}

/*************************************************************************
 *		ScrollDC (USER.221)
 */
BOOL16 WINAPI ScrollDC16( HDC16 hdc, INT16 dx, INT16 dy, const RECT16 *rect,
                          const RECT16 *cliprc, HRGN16 hrgnUpdate,
                          LPRECT16 rcUpdate )
{
    RECT rect32, clipRect32, rcUpdate32;
    BOOL16 ret;

    if (rect) CONV_RECT16TO32( rect, &rect32 );
    if (cliprc) CONV_RECT16TO32( cliprc, &clipRect32 );
    ret = ScrollDC( HDC_32(hdc), dx, dy, rect ? &rect32 : NULL,
                    cliprc ? &clipRect32 : NULL, HRGN_32(hrgnUpdate),
                    &rcUpdate32 );
    if (rcUpdate) CONV_RECT32TO16( &rcUpdate32, rcUpdate );
    return ret;
}

/***********************************************************************
 *		GetCursor (USER.247)
 */
HCURSOR16 WINAPI GetCursor16(void)
{
  return HCURSOR_16(GetCursor());
}

/***********************************************************************
 *		GlobalAddAtom (USER.268)
 */
ATOM WINAPI GlobalAddAtom16(LPCSTR lpString)
{
  return GlobalAddAtomA(lpString);
}

/***********************************************************************
 *		GlobalDeleteAtom (USER.269)
 */
ATOM WINAPI GlobalDeleteAtom16(ATOM nAtom)
{
  return GlobalDeleteAtom(nAtom);
}

/***********************************************************************
 *		GlobalFindAtom (USER.270)
 */
ATOM WINAPI GlobalFindAtom16(LPCSTR lpString)
{
  return GlobalFindAtomA(lpString);
}

/***********************************************************************
 *		GlobalGetAtomName (USER.271)
 */
UINT16 WINAPI GlobalGetAtomName16(ATOM nAtom, LPSTR lpBuffer, INT16 nSize)
{
  return GlobalGetAtomNameA(nAtom, lpBuffer, nSize);
}

/***********************************************************************
 *		SelectPalette (USER.282)
 */
HPALETTE16 WINAPI SelectPalette16( HDC16 hdc, HPALETTE16 hpal, BOOL16 bForceBackground )
{
    return HPALETTE_16( SelectPalette( HDC_32(hdc), HPALETTE_32(hpal), bForceBackground ));
}

/***********************************************************************
 *		RealizePalette (USER.283)
 */
UINT16 WINAPI RealizePalette16( HDC16 hdc )
{
    return UserRealizePalette( HDC_32(hdc) );
}

/***********************************************************************
 *		LoadImage (USER.389)
 *
 */
HANDLE16 WINAPI LoadImage16(HINSTANCE16 hinst, LPCSTR name, UINT16 type,
			    INT16 desiredx, INT16 desiredy, UINT16 loadflags)
{
  return HANDLE_16(LoadImageA(HINSTANCE_32(hinst), name, type, desiredx,
			      desiredy, loadflags));
}

/******************************************************************************
 *		CopyImage (USER.390) Creates new image and copies attributes to it
 *
 */
HICON16 WINAPI CopyImage16(HANDLE16 hnd, UINT16 type, INT16 desiredx,
			   INT16 desiredy, UINT16 flags)
{
  return HICON_16(CopyImage(HANDLE_32(hnd), (UINT)type, (INT)desiredx,
			    (INT)desiredy, (UINT)flags));
}

/**********************************************************************
 *		DrawIconEx (USER.394)
 */
BOOL16 WINAPI DrawIconEx16(HDC16 hdc, INT16 xLeft, INT16 yTop, HICON16 hIcon,
			   INT16 cxWidth, INT16 cyWidth, UINT16 istep,
			   HBRUSH16 hbr, UINT16 flags)
{
  return DrawIconEx(HDC_32(hdc), xLeft, yTop, HICON_32(hIcon), cxWidth, cyWidth,
		    istep, HBRUSH_32(hbr), flags);
}

/**********************************************************************
 *		GetIconInfo (USER.395)
 */
BOOL16 WINAPI GetIconInfo16(HICON16 hIcon, LPICONINFO16 iconinfo)
{
  ICONINFO ii32;
  BOOL16 ret = GetIconInfo(HICON_32(hIcon), &ii32);

  iconinfo->fIcon = ii32.fIcon;
  iconinfo->xHotspot = ii32.xHotspot;
  iconinfo->yHotspot = ii32.yHotspot;
  iconinfo->hbmMask  = HBITMAP_16(ii32.hbmMask);
  iconinfo->hbmColor = HBITMAP_16(ii32.hbmColor);
  return ret;
}

/***********************************************************************
 *		CreateCursor (USER.406)
 */
HCURSOR16 WINAPI CreateCursor16(HINSTANCE16 hInstance,
				INT16 xHotSpot, INT16 yHotSpot,
				INT16 nWidth, INT16 nHeight,
				LPCVOID lpANDbits, LPCVOID lpXORbits)
{
  CURSORICONINFO info;

  info.ptHotSpot.x = xHotSpot;
  info.ptHotSpot.y = yHotSpot;
  info.nWidth = nWidth;
  info.nHeight = nHeight;
  info.nWidthBytes = 0;
  info.bPlanes = 1;
  info.bBitsPerPixel = 1;

  return CreateCursorIconIndirect16(hInstance, &info, lpANDbits, lpXORbits);
}

/**********************************************************************
 *		CreateIconFromResourceEx (USER.450)
 *
 * FIXME: not sure about exact parameter types
 */
HICON16 WINAPI CreateIconFromResourceEx16(LPBYTE bits, UINT16 cbSize,
					  BOOL16 bIcon, DWORD dwVersion,
					  INT16 width, INT16 height,
					  UINT16 cFlag)
{
  return HICON_16(CreateIconFromResourceEx(bits, cbSize, bIcon, dwVersion,
					   width, height, cFlag));
}

/***********************************************************************
 *		DestroyIcon (USER.457)
 */
BOOL16 WINAPI DestroyIcon16(HICON16 hIcon)
{
  return DestroyIcon32(hIcon, 0);
}

/***********************************************************************
 *		DestroyCursor (USER.458)
 */
BOOL16 WINAPI DestroyCursor16(HCURSOR16 hCursor)
{
  return DestroyIcon32(hCursor, 0);
}

/*******************************************************************
 *			DRAG_QueryUpdate16
 *
 * Recursively find a child that contains spDragInfo->pt point
 * and send WM_QUERYDROPOBJECT. Helper for DragObject16.
 */
static BOOL DRAG_QueryUpdate16( HWND hQueryWnd, SEGPTR spDragInfo )
{
    BOOL bResult = 0;
    WPARAM wParam;
    POINT pt, old_pt;
    LPDRAGINFO16 ptrDragInfo = MapSL(spDragInfo);
    RECT tempRect;
    HWND child;

    if (!IsWindowEnabled(hQueryWnd)) return FALSE;

    old_pt.x = ptrDragInfo->pt.x;
    old_pt.y = ptrDragInfo->pt.y;
    pt = old_pt;
    ScreenToClient( hQueryWnd, &pt );
    child = ChildWindowFromPointEx( hQueryWnd, pt, CWP_SKIPINVISIBLE );
    if (!child) return FALSE;

    if (child != hQueryWnd)
    {
        wParam = 0;
        if (DRAG_QueryUpdate16( child, spDragInfo )) return TRUE;
    }
    else
    {
        GetClientRect( hQueryWnd, &tempRect );
        wParam = !PtInRect( &tempRect, pt );
    }

    ptrDragInfo->pt.x = pt.x;
    ptrDragInfo->pt.y = pt.y;
    ptrDragInfo->hScope = HWND_16(hQueryWnd);

    bResult = SendMessage16( HWND_16(hQueryWnd), WM_QUERYDROPOBJECT, wParam, spDragInfo );

    if (!bResult)
    {
        ptrDragInfo->pt.x = old_pt.x;
        ptrDragInfo->pt.y = old_pt.y;
    }
    return bResult;
}


/******************************************************************************
 *		DragObject (USER.464)
 */
DWORD WINAPI DragObject16( HWND16 hwndScope, HWND16 hWnd, UINT16 wObj,
                           HANDLE16 hOfStruct, WORD szList, HCURSOR16 hCursor )
{
    MSG	msg;
    LPDRAGINFO16 lpDragInfo;
    SEGPTR	spDragInfo;
    HCURSOR 	hOldCursor=0, hBummer=0;
    HGLOBAL16	hDragInfo  = GlobalAlloc16( GMEM_SHARE | GMEM_ZEROINIT, 2*sizeof(DRAGINFO16));
    HCURSOR	hCurrentCursor = 0;
    HWND16	hCurrentWnd = 0;

    lpDragInfo = (LPDRAGINFO16) GlobalLock16(hDragInfo);
    spDragInfo = K32WOWGlobalLock16(hDragInfo);

    if( !lpDragInfo || !spDragInfo ) return 0L;

    if (!(hBummer = LoadCursorA(0, MAKEINTRESOURCEA(OCR_NO))))
    {
        GlobalFree16(hDragInfo);
        return 0L;
    }

    if(hCursor) hOldCursor = SetCursor(HCURSOR_32(hCursor));

    lpDragInfo->hWnd   = hWnd;
    lpDragInfo->hScope = 0;
    lpDragInfo->wFlags = wObj;
    lpDragInfo->hList  = szList; /* near pointer! */
    lpDragInfo->hOfStruct = hOfStruct;
    lpDragInfo->l = 0L;

    SetCapture( HWND_32(hWnd) );
    ShowCursor( TRUE );

    do
    {
        GetMessageW( &msg, 0, WM_MOUSEFIRST, WM_MOUSELAST );

       *(lpDragInfo+1) = *lpDragInfo;

	lpDragInfo->pt.x = msg.pt.x;
	lpDragInfo->pt.y = msg.pt.y;

	/* update DRAGINFO struct */
	if( DRAG_QueryUpdate16(WIN_Handle32(hwndScope), spDragInfo) > 0 )
	    hCurrentCursor = HCURSOR_32(hCursor);
	else
        {
            hCurrentCursor = hBummer;
            lpDragInfo->hScope = 0;
	}
	if( hCurrentCursor )
	    SetCursor(hCurrentCursor);

	/* send WM_DRAGLOOP */
	SendMessage16( hWnd, WM_DRAGLOOP, (WPARAM16)(hCurrentCursor != hBummer),
	                                  (LPARAM) spDragInfo );
	/* send WM_DRAGSELECT or WM_DRAGMOVE */
	if( hCurrentWnd != lpDragInfo->hScope )
	{
	    if( hCurrentWnd )
	        SendMessage16( hCurrentWnd, WM_DRAGSELECT, 0,
		       (LPARAM)MAKELONG(LOWORD(spDragInfo)+sizeof(DRAGINFO16),
				        HIWORD(spDragInfo)) );
	    hCurrentWnd = lpDragInfo->hScope;
	    if( hCurrentWnd )
                SendMessage16( hCurrentWnd, WM_DRAGSELECT, 1, (LPARAM)spDragInfo);
	}
	else
	    if( hCurrentWnd )
	        SendMessage16( hCurrentWnd, WM_DRAGMOVE, 0, (LPARAM)spDragInfo);

    } while( msg.message != WM_LBUTTONUP && msg.message != WM_NCLBUTTONUP );

    ReleaseCapture();
    ShowCursor( FALSE );

    if( hCursor ) SetCursor(hOldCursor);

    if( hCurrentCursor != hBummer )
	msg.lParam = SendMessage16( lpDragInfo->hScope, WM_DROPOBJECT,
				   (WPARAM16)hWnd, (LPARAM)spDragInfo );
    else
        msg.lParam = 0;
    GlobalFree16(hDragInfo);

    return (DWORD)(msg.lParam);
}


/**********************************************************************
 *          DrawFrameControl  (USER.656)
 */
BOOL16 WINAPI DrawFrameControl16( HDC16 hdc, LPRECT16 rc, UINT16 uType, UINT16 uState )
{
    RECT rect32;
    BOOL ret;

    CONV_RECT16TO32( rc, &rect32 );
    ret = DrawFrameControl( HDC_32(hdc), &rect32, uType, uState );
    CONV_RECT32TO16( &rect32, rc );
    return ret;
}

/**********************************************************************
 *          DrawEdge   (USER.659)
 */
BOOL16 WINAPI DrawEdge16( HDC16 hdc, LPRECT16 rc, UINT16 edge, UINT16 flags )
{
    RECT rect32;
    BOOL ret;

    CONV_RECT16TO32( rc, &rect32 );
    ret = DrawEdge( HDC_32(hdc), &rect32, edge, flags );
    CONV_RECT32TO16( &rect32, rc );
    return ret;
}

/**********************************************************************
 *		WinHelp (USER.171)
 */
BOOL16 WINAPI WinHelp16( HWND16 hWnd, LPCSTR lpHelpFile, UINT16 wCommand,
                         DWORD dwData )
{
    BOOL ret;
    DWORD mutex_count;

    /* We might call WinExec() */
    ReleaseThunkLock(&mutex_count);

    ret = WinHelpA(WIN_Handle32(hWnd), lpHelpFile, wCommand, (DWORD)MapSL(dwData));

    RestoreThunkLock(mutex_count);
    return ret;
}
