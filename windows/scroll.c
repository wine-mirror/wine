/*
 * Scroll windows and DCs
 *
 * Copyright  David W. Metcalfe, 1993
 *
 */

#include <stdlib.h>
#include "windows.h"
#include "gdi.h"
#include "scroll.h"
#include "stddebug.h"
/* #define DEBUG_SCROLL */
#include "debug.h"


static int RgnType;


/*************************************************************************
 *             ScrollWindow         (USER.61)
 */

void ScrollWindow(HWND hwnd, short dx, short dy, LPRECT rect, LPRECT clipRect)
{
    HDC hdc;
    HRGN hrgnUpdate;
    RECT rc, cliprc;

    dprintf_scroll(stddeb,"ScrollWindow: dx=%d, dy=%d, rect=%d,%d,%d,%d\n", 
	   dx, dy, rect->left, rect->top, rect->right, rect->bottom);

    hdc = GetDC(hwnd);

    if (rect == NULL)
	GetClientRect(hwnd, &rc);
    else
	CopyRect(&rc, rect);
    if (clipRect == NULL)
	GetClientRect(hwnd, &cliprc);
    else
	CopyRect(&cliprc, clipRect);

    hrgnUpdate = CreateRectRgn(0, 0, 0, 0);
    ScrollDC(hdc, dx, dy, &rc, &cliprc, hrgnUpdate, NULL);
    InvalidateRgn(hwnd, hrgnUpdate, TRUE);
    ReleaseDC(hwnd, hdc);
}


/*************************************************************************
 *             ScrollDC         (USER.221)
 */

BOOL ScrollDC(HDC hdc, short dx, short dy, LPRECT rc, LPRECT cliprc,
	      HRGN hrgnUpdate, LPRECT rcUpdate)
{
    HRGN hrgnClip, hrgn1, hrgn2;
    POINT src, dest;
    short width, height;
    DC *dc = (DC *)GDI_GetObjPtr(hdc, DC_MAGIC);

    dprintf_scroll(stddeb, "ScrollDC: dx=%d, dy=%d, rc=%d,%d,%d,%d\n", dx, dy,
	   rc->left, rc->top, rc->right, rc->bottom);

    if (rc == NULL)
	return FALSE;

    if (cliprc)
    {
	hrgnClip = CreateRectRgnIndirect(cliprc);
	SelectClipRgn(hdc, hrgnClip);
    }

    if (dx > 0)
    {
	src.x = XDPTOLP(dc, rc->left);
	dest.x = XDPTOLP(dc, rc->left + abs(dx));
    }
    else
    {
	src.x = XDPTOLP(dc, rc->left + abs(dx));
	dest.x = XDPTOLP(dc, rc->left);
    }
    if (dy > 0)
    {
	src.y = YDPTOLP(dc, rc->top);
	dest.y = YDPTOLP(dc, rc->top + abs(dy));
    }
    else
    {
	src.y = YDPTOLP(dc, rc->top + abs(dy));
	dest.y = YDPTOLP(dc, rc->top);
    }

    width = rc->right - rc->left - abs(dx);
    height = rc->bottom - rc->top - abs(dy);

    if (!BitBlt(hdc, dest.x, dest.y, width, height, hdc, src.x, src.y, 
		SRCCOPY))
	return FALSE;

    if (hrgnUpdate)
    {
	if (dx > 0)
	    hrgn1 = CreateRectRgn(rc->left, rc->top, rc->left+dx, rc->bottom);
	else if (dx < 0)
	    hrgn1 = CreateRectRgn(rc->right+dx, rc->top, rc->right, 
				  rc->bottom);
	else
	    hrgn1 = CreateRectRgn(0, 0, 0, 0);

	if (dy > 0)
	    hrgn2 = CreateRectRgn(rc->left, rc->top, rc->right, rc->top+dy);
	else if (dy < 0)
	    hrgn2 = CreateRectRgn(rc->left, rc->bottom+dy, rc->right, 
				  rc->bottom);
	else
	    hrgn2 = CreateRectRgn(0, 0, 0, 0);

	RgnType = CombineRgn(hrgnUpdate, hrgn1, hrgn2, RGN_OR);
    }

    if (rcUpdate) GetRgnBox( hrgnUpdate, rcUpdate );
    return TRUE;
}


/*************************************************************************
 *             ScrollWindowEx       (USER.319)
 */

int ScrollWindowEx(HWND hwnd, short dx, short dy, LPRECT rect, LPRECT clipRect,
		   HRGN hrgnUpdate, LPRECT rcUpdate, WORD flags)
{
    HDC hdc;
    RECT rc, cliprc;

    dprintf_scroll(stddeb,"ScrollWindowEx: dx=%d, dy=%d, rect=%d,%d,%d,%d\n", 
	   dx, dy, rect->left, rect->top, rect->right, rect->bottom);

    hdc = GetDC(hwnd);

    if (rect == NULL)
	GetClientRect(hwnd, &rc);
    else
	CopyRect(&rc, rect);
    if (clipRect == NULL)
	GetClientRect(hwnd, &cliprc);
    else
	CopyRect(&cliprc, clipRect);

    ScrollDC(hdc, dx, dy, &rc, &cliprc, hrgnUpdate, rcUpdate);

    if (flags | SW_INVALIDATE)
    {
	RedrawWindow(hwnd, NULL, hrgnUpdate,
		     RDW_INVALIDATE | ((flags & SW_ERASE) ? RDW_ERASENOW : 0));
    }

    ReleaseDC(hwnd, hdc);
    return RgnType;
}
