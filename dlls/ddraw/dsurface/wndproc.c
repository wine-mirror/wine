/*	User-based primary surface driver
 *
 * Copyright 2000 TransGaming Technologies Inc.
 */

#include "config.h"
#include "winerror.h"

#include <assert.h>
#include <stdlib.h>

#include "debugtools.h"
#include "ddraw_private.h"

DEFAULT_DEBUG_CHANNEL(ddraw);

static LRESULT WINAPI DirectDrawSurface_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void DirectDrawSurface_RegisterClass(void)
{
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = DirectDrawSurface_WndProc;
    wc.cbWndExtra  = sizeof(IDirectDrawSurfaceImpl*);
    wc.hCursor     = (HCURSOR)IDC_ARROWA;
    wc.lpszClassName = "WINE_DDRAW";
    RegisterClassA(&wc);
}

void DirectDrawSurface_UnregisterClass(void)
{
    UnregisterClassA("WINE_DDRAW", 0);
}

static LRESULT WINAPI DirectDrawSurface_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    IDirectDrawSurfaceImpl *This;
    LRESULT ret;

    This = (IDirectDrawSurfaceImpl *)GetWindowLongA(hwnd, 0);
    if (This) {
	HWND window = This->ddraw_owner->window;

	switch (msg) {
	case WM_DESTROY:
	case WM_NCDESTROY:
	case WM_SHOWWINDOW:
	case WM_WINDOWPOSCHANGING:
	case WM_WINDOWPOSCHANGED:
	case WM_SIZE:
	case WM_MOVE:
	case WM_ERASEBKGND:
	    /* since we're pretending fullscreen,
	     * let's not pass these on to the app */
	    ret = DefWindowProcA(hwnd, msg, wParam, lParam);
	    break;
	case WM_NCHITTEST:
	    ret = HTCLIENT;
	    break;
	case WM_MOUSEACTIVATE:
	    ret = MA_NOACTIVATE;
	    break;
	case WM_PAINT:
	    {
		PAINTSTRUCT ps;
		HDC dc;
		dc = BeginPaint(hwnd, &ps);
		/* call User_copy_to_screen? */
		EndPaint(hwnd, &ps);
		ret = 0;
	    }
	    break;
	default:
	    ret = window ? SendMessageA(window, msg, wParam, lParam)
			 : DefWindowProcA(hwnd, msg, wParam, lParam);
	}
    } else {
	if (msg == WM_CREATE) {
	    CREATESTRUCTA *cs = (CREATESTRUCTA *)lParam;
	    This = (IDirectDrawSurfaceImpl *)cs->lpCreateParams;
	    SetWindowLongA(hwnd, 0, (LONG)This);
	}
	ret = DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    return ret;
}
