/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * Digital video MCI Wine Driver
 *
 * Copyright 1999, 2000 Eric POUECH
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

#include <string.h>
#include "private_mciavi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mciavi);

static LRESULT WINAPI MCIAVI_WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TRACE("hwnd=%p msg=%x wparam=%x lparam=%lx\n", hWnd, uMsg, wParam, lParam);

    if (!(WINE_MCIAVI*)GetWindowLongA(hWnd, 0) && uMsg != WM_CREATE)
	return DefWindowProcA(hWnd, uMsg, wParam, lParam);

    switch (uMsg) {
    case WM_CREATE:
	SetWindowLongA(hWnd, 0, (LPARAM)((CREATESTRUCTA*)lParam)->lpCreateParams);
	return DefWindowProcA(hWnd, uMsg, wParam, lParam);

    case WM_DESTROY:
	SetWindowLongA(hWnd, 0, 0);
	return DefWindowProcA(hWnd, uMsg, wParam, lParam);

    case WM_ERASEBKGND:
	{
	    RECT	rect;
	    GetClientRect(hWnd, &rect);
	    FillRect((HDC)wParam, &rect, GetStockObject(BLACK_BRUSH));
	}
	break;
    case WM_PAINT:
        {
            WINE_MCIAVI* wma = (WINE_MCIAVI*)GetWindowLongA(hWnd, 0);

            /* the animation isn't playing, don't paint */
	    if (wma->dwStatus == MCI_MODE_NOT_READY)
		/* default paint handling */
	    	return DefWindowProcA(hWnd, uMsg, wParam, lParam);

            if (wParam) {
                EnterCriticalSection(&wma->cs);
                MCIAVI_PaintFrame(wma, (HDC)wParam);
                LeaveCriticalSection(&wma->cs);
            } else {
	        PAINTSTRUCT ps;
 	        HDC hDC = BeginPaint(hWnd, &ps);

                EnterCriticalSection(&wma->cs);
                MCIAVI_PaintFrame(wma, hDC);
                LeaveCriticalSection(&wma->cs);

	        EndPaint(hWnd, &ps);
	    }
        }
	break;

    default:
	return DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

BOOL    MCIAVI_CreateWindow(WINE_MCIAVI* wma, DWORD dwFlags, LPMCI_DGV_OPEN_PARMSA lpOpenParms)
{
    WNDCLASSA 	wndClass;
    HWND	hParent = 0;
    DWORD	dwStyle = WS_OVERLAPPEDWINDOW;
    int		p = CW_USEDEFAULT;

    /* what should be done ? */
    if (wma->hWnd) return TRUE;

    ZeroMemory(&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = (WNDPROC)MCIAVI_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(WINE_MCIAVI*);
    wndClass.hCursor       = LoadCursorA(0, IDC_ARROWA);
    wndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wndClass.lpszClassName = "MCIAVI";

    RegisterClassA(&wndClass);

    if (dwFlags & MCI_DGV_OPEN_PARENT)	hParent = lpOpenParms->hWndParent;
    if (dwFlags & MCI_DGV_OPEN_WS)	dwStyle = lpOpenParms->dwStyle;
    if (dwStyle & WS_CHILD)		p = 0;

    wma->hWnd = CreateWindowA("MCIAVI", "Wine MCI-AVI player",
			      dwStyle, p, p,
			      (wma->hic ? wma->outbih : wma->inbih)->biWidth,
			      (wma->hic ? wma->outbih : wma->inbih)->biHeight,
			      hParent, 0, MCIAVI_hInstance, wma);
    return (BOOL)wma->hWnd;
}

/***************************************************************************
 * 				MCIAVI_mciPut			[internal]
 */
DWORD	MCIAVI_mciPut(UINT wDevID, DWORD dwFlags, LPMCI_DGV_PUT_PARMS lpParms)
{
    WINE_MCIAVI*	wma = MCIAVI_mciGetOpenDev(wDevID);
    RECT		rc;
    char		buffer[256];

    FIXME("(%04x, %08lX, %p) : stub\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    if (dwFlags & MCI_DGV_RECT) {
	rc = lpParms->rc;
    } else {
	SetRectEmpty(&rc);
    }

    *buffer = 0;
    if (dwFlags & MCI_DGV_PUT_CLIENT) {
	strncat(buffer, "PUT_CLIENT", sizeof(buffer));
    }
    if (dwFlags & MCI_DGV_PUT_DESTINATION) {
	strncat(buffer, "PUT_DESTINATION", sizeof(buffer));
    }
    if (dwFlags & MCI_DGV_PUT_FRAME) {
	strncat(buffer, "PUT_FRAME", sizeof(buffer));
    }
    if (dwFlags & MCI_DGV_PUT_SOURCE) {
	strncat(buffer, "PUT_SOURCE", sizeof(buffer));
    }
    if (dwFlags & MCI_DGV_PUT_VIDEO) {
	strncat(buffer, "PUT_VIDEO", sizeof(buffer));
    }
    if (dwFlags & MCI_DGV_PUT_WINDOW) {
	strncat(buffer, "PUT_WINDOW", sizeof(buffer));
    }
    TRACE("%s (%d,%d,%d,%d)\n", buffer, rc.left, rc.top, rc.right, rc.bottom);

    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciWhere			[internal]
 */
DWORD	MCIAVI_mciWhere(UINT wDevID, DWORD dwFlags, LPMCI_DGV_RECT_PARMS lpParms)
{
    WINE_MCIAVI*	wma = MCIAVI_mciGetOpenDev(wDevID);
    LPSTR		x = "";

    TRACE("(%04x, %08lx, %p)\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    if (dwFlags & MCI_DGV_WHERE_MAX) FIXME("Max NIY\n");

    if (dwFlags & MCI_DGV_WHERE_DESTINATION) {
	x = "Dest";
	GetClientRect(wma->hWnd, &lpParms->rc);
    }
    if (dwFlags & MCI_DGV_WHERE_FRAME) {
	FIXME(x = "Frame\n");
	return MCIERR_UNRECOGNIZED_COMMAND;
    }
    if (dwFlags & MCI_DGV_WHERE_SOURCE) {
	x = "Source";
	lpParms->rc.left = lpParms->rc.top = 0;
	lpParms->rc.right = wma->mah.dwWidth;
	lpParms->rc.bottom = wma->mah.dwHeight;
    }
    if (dwFlags & MCI_DGV_WHERE_VIDEO) {
	FIXME(x = "Video\n");
	return MCIERR_UNRECOGNIZED_COMMAND;
    }
    if (dwFlags & MCI_DGV_WHERE_WINDOW) {
	x = "Window";
	GetClientRect(wma->hWnd, &lpParms->rc);
    }
    TRACE("%s -> (%d,%d,%d,%d)\n",
	  x, lpParms->rc.left, lpParms->rc.top, lpParms->rc.right, lpParms->rc.bottom);

    return 0;
}

/***************************************************************************
 * 				MCIAVI_mciWindow			[internal]
 */
DWORD	MCIAVI_mciWindow(UINT wDevID, DWORD dwFlags, LPMCI_DGV_WINDOW_PARMSA lpParms)
{
    WINE_MCIAVI*	wma = MCIAVI_mciGetOpenDev(wDevID);

    TRACE("(%04x, %08lX, %p)\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    if (dwFlags & MCI_DGV_WINDOW_HWND) {
	FIXME("Setting hWnd to %08lx\n", (DWORD)lpParms->hWnd);
#if 0
	if (wma->hWnd) DestroyWindow(wma->hWnd);
	/* is the window to be subclassed ? */
	wma->hWnd = lpParms->hWnd;
#endif
    }
    if (dwFlags & MCI_DGV_WINDOW_STATE) {
	TRACE("Setting nCmdShow to %d\n", lpParms->nCmdShow);
	ShowWindow(wma->hWnd, lpParms->nCmdShow);
    }
    if (dwFlags & MCI_DGV_WINDOW_TEXT) {
	TRACE("Setting caption to '%s'\n", lpParms->lpstrText);
	SetWindowTextA(wma->hWnd, lpParms->lpstrText);
    }

    return 0;
}

