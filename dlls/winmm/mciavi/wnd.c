/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * Digital video MCI Wine Driver
 *
 * Copyright 1999, 2000 Eric POUECH
 * Copyright 2003 Dmitry Timoshkov
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

    switch (uMsg) {
    case WM_CREATE:
	SetWindowLongA(hWnd, 0, (LPARAM)((CREATESTRUCTA*)lParam)->lpCreateParams);
	return DefWindowProcA(hWnd, uMsg, wParam, lParam);

    case WM_DESTROY:
        MCIAVI_mciClose(GetWindowLongA(hWnd, 0), MCI_WAIT, NULL);
	SetWindowLongA(hWnd, 0, 0);
	return DefWindowProcA(hWnd, uMsg, wParam, lParam);

    case WM_ERASEBKGND:
	{
	    RECT	rect;
	    GetClientRect(hWnd, &rect);
	    FillRect((HDC)wParam, &rect, GetStockObject(BLACK_BRUSH));
	}
       return 1;

    case WM_PAINT:
        {
            WINE_MCIAVI *wma = (WINE_MCIAVI *)mciGetDriverData(GetWindowLongA(hWnd, 0));

            if (!wma)
                return DefWindowProcA(hWnd, uMsg, wParam, lParam);
            
            EnterCriticalSection(&wma->cs);

            /* the animation isn't playing, don't paint */
	    if (wma->dwStatus == MCI_MODE_NOT_READY)
            {
                LeaveCriticalSection(&wma->cs);
		/* default paint handling */
	    	return DefWindowProcA(hWnd, uMsg, wParam, lParam);
            }

            if (wParam)
                MCIAVI_PaintFrame(wma, (HDC)wParam);
            else
            {
	        PAINTSTRUCT ps;
 	        HDC hDC = BeginPaint(hWnd, &ps);
                MCIAVI_PaintFrame(wma, hDC);
	        EndPaint(hWnd, &ps);
	    }

            LeaveCriticalSection(&wma->cs);
        }
       return 1;

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
    RECT rc;

    /* what should be done ? */
    if (wma->hWnd) return TRUE;

    ZeroMemory(&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_DBLCLKS;
    wndClass.lpfnWndProc   = (WNDPROC)MCIAVI_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(MCIDEVICEID);
    wndClass.hInstance     = MCIAVI_hInstance;
    wndClass.hCursor       = LoadCursorA(0, (LPSTR)IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wndClass.lpszClassName = "MCIAVI";

    if (!RegisterClassA(&wndClass)) return FALSE;

    if (dwFlags & MCI_DGV_OPEN_PARENT)	hParent = lpOpenParms->hWndParent;
    if (dwFlags & MCI_DGV_OPEN_WS)	dwStyle = lpOpenParms->dwStyle;
    if (dwStyle & WS_CHILD)		p = 0;

    rc.left = p;
    rc.top = p;
    rc.right = (wma->hic ? wma->outbih : wma->inbih)->biWidth;
    rc.bottom = (wma->hic ? wma->outbih : wma->inbih)->biHeight;
    AdjustWindowRect(&rc, dwStyle, FALSE);

    wma->hWnd = CreateWindowA("MCIAVI", "Wine MCI-AVI player",
                             dwStyle, rc.left, rc.top,
                             rc.right, rc.bottom,
                             hParent, 0, MCIAVI_hInstance,
                              (LPVOID)wma->wDevID);
    wma->hWndPaint = wma->hWnd;
    return (BOOL)wma->hWnd;
}

/***************************************************************************
 * 				MCIAVI_mciPut			[internal]
 */
DWORD	MCIAVI_mciPut(UINT wDevID, DWORD dwFlags, LPMCI_DGV_PUT_PARMS lpParms)
{
    WINE_MCIAVI*	wma = MCIAVI_mciGetOpenDev(wDevID);
    RECT		rc;

    TRACE("(%04x, %08lX, %p)\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    EnterCriticalSection(&wma->cs);

    if (dwFlags & MCI_DGV_RECT) {
	rc = lpParms->rc;
    } else {
        GetClientRect(wma->hWndPaint, &rc);
    }

    if (dwFlags & MCI_DGV_PUT_CLIENT) {
        FIXME("PUT_CLIENT %s\n", wine_dbgstr_rect(&rc));
        LeaveCriticalSection(&wma->cs);
        return MCIERR_UNRECOGNIZED_COMMAND;
    }
    if (dwFlags & MCI_DGV_PUT_DESTINATION) {
        TRACE("PUT_DESTINATION %s\n", wine_dbgstr_rect(&rc));
        wma->dest = rc;
    }
    if (dwFlags & MCI_DGV_PUT_FRAME) {
        FIXME("PUT_FRAME %s\n", wine_dbgstr_rect(&rc));
        LeaveCriticalSection(&wma->cs);
        return MCIERR_UNRECOGNIZED_COMMAND;
    }
    if (dwFlags & MCI_DGV_PUT_SOURCE) {
        TRACE("PUT_SOURCE %s\n", wine_dbgstr_rect(&rc));
        wma->source = rc;
    }
    if (dwFlags & MCI_DGV_PUT_VIDEO) {
        FIXME("PUT_VIDEO %s\n", wine_dbgstr_rect(&rc));
        LeaveCriticalSection(&wma->cs);
        return MCIERR_UNRECOGNIZED_COMMAND;
    }
    if (dwFlags & MCI_DGV_PUT_WINDOW) {
        FIXME("PUT_WINDOW %s\n", wine_dbgstr_rect(&rc));
        LeaveCriticalSection(&wma->cs);
        return MCIERR_UNRECOGNIZED_COMMAND;
    }
    LeaveCriticalSection(&wma->cs);
    return 0;
}

/******************************************************************************
 * 				MCIAVI_mciWhere			[internal]
 */
DWORD	MCIAVI_mciWhere(UINT wDevID, DWORD dwFlags, LPMCI_DGV_RECT_PARMS lpParms)
{
    WINE_MCIAVI*	wma = MCIAVI_mciGetOpenDev(wDevID);

    TRACE("(%04x, %08lx, %p)\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wma == NULL)		return MCIERR_INVALID_DEVICE_ID;

    if (dwFlags & MCI_DGV_WHERE_MAX)
    {
        FIXME("WHERE_MAX\n");
        return MCIERR_UNRECOGNIZED_COMMAND;
    }

    EnterCriticalSection(&wma->cs);

    if (dwFlags & MCI_DGV_WHERE_DESTINATION) {
        TRACE("WHERE_DESTINATION %s\n", wine_dbgstr_rect(&wma->dest));
        lpParms->rc = wma->dest;
    }
    if (dwFlags & MCI_DGV_WHERE_FRAME) {
       FIXME("MCI_DGV_WHERE_FRAME\n");
        LeaveCriticalSection(&wma->cs);
	return MCIERR_UNRECOGNIZED_COMMAND;
    }
    if (dwFlags & MCI_DGV_WHERE_SOURCE) {
        TRACE("WHERE_SOURCE %s\n", wine_dbgstr_rect(&wma->source));
        lpParms->rc = wma->source;
    }
    if (dwFlags & MCI_DGV_WHERE_VIDEO) {
       FIXME("WHERE_VIDEO\n");
        LeaveCriticalSection(&wma->cs);
	return MCIERR_UNRECOGNIZED_COMMAND;
    }
    if (dwFlags & MCI_DGV_WHERE_WINDOW) {
        GetClientRect(wma->hWndPaint, &lpParms->rc);
        TRACE("WHERE_WINDOW %s\n", wine_dbgstr_rect(&lpParms->rc));
    }
    LeaveCriticalSection(&wma->cs);
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

    EnterCriticalSection(&wma->cs);

    if (dwFlags & MCI_DGV_WINDOW_HWND) {
        if (IsWindow(lpParms->hWnd))
        {
            TRACE("Setting hWnd to %p\n", lpParms->hWnd);
            if (wma->hWnd) ShowWindow(wma->hWnd, SW_HIDE);
            wma->hWndPaint = (lpParms->hWnd == MCI_DGV_WINDOW_DEFAULT) ? wma->hWnd : lpParms->hWnd;
            InvalidateRect(wma->hWndPaint, NULL, FALSE);
        }
    }
    if (dwFlags & MCI_DGV_WINDOW_STATE) {
	TRACE("Setting nCmdShow to %d\n", lpParms->nCmdShow);
       ShowWindow(wma->hWndPaint, lpParms->nCmdShow);
    }
    if (dwFlags & MCI_DGV_WINDOW_TEXT) {
	TRACE("Setting caption to '%s'\n", lpParms->lpstrText);
       SetWindowTextA(wma->hWndPaint, lpParms->lpstrText);
    }

    LeaveCriticalSection(&wma->cs);
    return 0;
}
