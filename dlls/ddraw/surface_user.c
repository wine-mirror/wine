/*	User-based primary surface driver
 *
 * Copyright 2000-2001 TransGaming Technologies Inc.
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "winerror.h"
#include "wine/debug.h"
#include "ddraw_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

/* if you use OWN_WINDOW, don't use SYNC_UPDATE, or you may get trouble */
/* #define SYNC_UPDATE */
/*
 * FIXME: This does not work any more because the created window has its own
 *        thread queue that cannot be manipulated by application threads.
 * #define OWN_WINDOW
 */

#ifdef OWN_WINDOW
static void User_create_own_window(IDirectDrawSurfaceImpl* This);
static void User_destroy_own_window(IDirectDrawSurfaceImpl* This);
#endif
#ifndef SYNC_UPDATE
static DWORD CALLBACK User_update_thread(LPVOID);
#endif
static void User_copy_to_screen(IDirectDrawSurfaceImpl* This, LPCRECT rc);

static HWND get_display_window(IDirectDrawSurfaceImpl* This, LPPOINT pt);

static const IDirectDrawSurface7Vtbl User_IDirectDrawSurface7_VTable;

HRESULT
User_DirectDrawSurface_Construct(IDirectDrawSurfaceImpl* This,
				 IDirectDrawImpl* pDD,
				 const DDSURFACEDESC2* pDDSD)
{
    USER_PRIV_VAR(priv, This);
    HRESULT hr;

    TRACE("(%p,%p,%p)\n",This,pDD,pDDSD);
    hr = DIB_DirectDrawSurface_Construct(This, pDD, pDDSD);
    if (FAILED(hr)) return hr;

    ICOM_INIT_INTERFACE(This, IDirectDrawSurface7,
			User_IDirectDrawSurface7_VTable);

    This->final_release = User_DirectDrawSurface_final_release;
    This->duplicate_surface = User_DirectDrawSurface_duplicate_surface;

    This->lock_update   = User_DirectDrawSurface_lock_update;
    This->unlock_update = User_DirectDrawSurface_unlock_update;

    This->flip_data   = User_DirectDrawSurface_flip_data;
    This->flip_update = User_DirectDrawSurface_flip_update;

    This->get_dc     = User_DirectDrawSurface_get_dc;
    This->release_dc = User_DirectDrawSurface_release_dc;

    This->set_palette    = User_DirectDrawSurface_set_palette;
    This->update_palette = User_DirectDrawSurface_update_palette;

    This->get_gamma_ramp = User_DirectDrawSurface_get_gamma_ramp;
    This->set_gamma_ramp = User_DirectDrawSurface_set_gamma_ramp;

    This->get_display_window = User_DirectDrawSurface_get_display_window;

    if (This->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
#ifdef OWN_WINDOW
	DirectDrawSurface_RegisterClass();
#endif
#ifndef SYNC_UPDATE
	InitializeCriticalSection(&priv->user.crit);
	priv->user.refresh_event = CreateEventW(NULL, TRUE, FALSE, NULL);
	priv->user.update_event = CreateEventW(NULL, FALSE, FALSE, NULL);
	priv->user.update_thread = CreateThread(NULL, 0, User_update_thread, This, 0, NULL);
#ifdef OWN_WINDOW
	if (This->ddraw_owner->cooperative_level & DDSCL_FULLSCREEN) {
	    /* wait for window creation (or update thread destruction) */
	    while (WaitForMultipleObjects(1, &priv->user.update_thread, FALSE, 100) == WAIT_TIMEOUT)
		if (This->more.lpDDRAWReserved) break;
	    if (!This->more.lpDDRAWReserved) {
		ERR("window creation failed\n");
	    }
	}
#endif
#else
#ifdef OWN_WINDOW
	User_create_own_window(This);
#endif
#endif
	if (!This->more.lpDDRAWReserved)
	    This->more.lpDDRAWReserved = (LPVOID)pDD->window;
    }

    return DIB_DirectDrawSurface_alloc_dc(This, &priv->user.cached_dc);
}

HRESULT
User_DirectDrawSurface_Create(IDirectDrawImpl *pDD,
			      const DDSURFACEDESC2 *pDDSD,
			      LPDIRECTDRAWSURFACE7 *ppSurf,
			      IUnknown *pUnkOuter)
{
    IDirectDrawSurfaceImpl* This;
    HRESULT hr;
    assert(pUnkOuter == NULL);

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		     sizeof(*This) + sizeof(User_DirectDrawSurfaceImpl));
    if (This == NULL) return E_OUTOFMEMORY;

    This->private = (User_DirectDrawSurfaceImpl*)(This+1);

    hr = User_DirectDrawSurface_Construct(This, pDD, pDDSD);
    if (FAILED(hr))
	HeapFree(GetProcessHeap(), 0, This);
    else
	*ppSurf = ICOM_INTERFACE(This, IDirectDrawSurface7);

    return hr;
}

void User_DirectDrawSurface_final_release(IDirectDrawSurfaceImpl* This)
{
    USER_PRIV_VAR(priv, This);

    if (This->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
#ifndef SYNC_UPDATE
	HANDLE event = priv->user.update_event;
	priv->user.update_event = 0;
	SetEvent(event);
	TRACE("waiting for update thread to terminate...\n");
#ifdef OWN_WINDOW
	/* dispatch any waiting sendmessages */
	{
	    MSG msg;
	    PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE);
	}
	/* to avoid deadlocks, allow SendMessages from update thread
	 * through while we wait for it */
	while (MsgWaitForMultipleObjects(1, &priv->user.update_thread, FALSE,
					 INFINITE, QS_SENDMESSAGE) == WAIT_OBJECT_0+1)
	{
	    MSG msg;
	    PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE);
	}
#else
	WaitForSingleObject(priv->user.update_thread,INFINITE);
#endif
	TRACE("update thread terminated\n");
	CloseHandle(event);
	CloseHandle(priv->user.update_thread);
	CloseHandle(priv->user.refresh_event);
	DeleteCriticalSection(&priv->user.crit);
#else
#ifdef OWN_WINDOW
	User_destroy_own_window(This);
#endif
#endif
	This->more.lpDDRAWReserved = 0;
#ifdef OWN_WINDOW
	DirectDrawSurface_UnregisterClass();
#endif
    }
    DIB_DirectDrawSurface_free_dc(This, priv->user.cached_dc);
    DIB_DirectDrawSurface_final_release(This);
}

static int User_DirectDrawSurface_init_wait(IDirectDrawSurfaceImpl* This)
{
    USER_PRIV_VAR(priv, This);
    int need_wait;
    EnterCriticalSection(&priv->user.crit);
    priv->user.wait_count++;
    need_wait = priv->user.in_refresh;
    LeaveCriticalSection(&priv->user.crit);
    return need_wait;
}

static void User_DirectDrawSurface_end_wait(IDirectDrawSurfaceImpl* This)
{
    USER_PRIV_VAR(priv, This);
    EnterCriticalSection(&priv->user.crit);
    if (!--priv->user.wait_count)
	ResetEvent(priv->user.refresh_event);
    LeaveCriticalSection(&priv->user.crit);
}

static void User_DirectDrawSurface_wait_update(IDirectDrawSurfaceImpl* This)
{
    USER_PRIV_VAR(priv, This);
    if (priv->user.in_refresh) {
	if (User_DirectDrawSurface_init_wait(This))
	    WaitForSingleObject(priv->user.refresh_event, 2);
	User_DirectDrawSurface_end_wait(This);
    }
}

void User_DirectDrawSurface_lock_update(IDirectDrawSurfaceImpl* This,
					LPCRECT pRect, DWORD dwFlags)
{
#if 0
    if (!(dwFlags & DDLOCK_WRITEONLY))
	User_copy_from_screen(This, pRect);
#endif
    if (dwFlags & DDLOCK_WAIT) User_DirectDrawSurface_wait_update(This);

    if (pRect) {
	This->lastlockrect = *pRect;
    } else {
	This->lastlockrect.left = This->lastlockrect.right = 0;
    }
}

void User_DirectDrawSurface_unlock_update(IDirectDrawSurfaceImpl* This,
					  LPCRECT pRect)
{
    if (This->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
#ifdef SYNC_UPDATE
	User_copy_to_screen(This, pRect);
#else
	USER_PRIV_VAR(priv, This);
	SetEvent(priv->user.update_event);
#endif
    }
}

void User_DirectDrawSurface_set_palette(IDirectDrawSurfaceImpl* This,
					IDirectDrawPaletteImpl* pal)
{
    USER_PRIV_VAR(priv, This);

    if (!pal) {
	FIXME("selecting null palette\n");
	/* I don't think selecting GDI object 0 will work, we should select
	 * the original palette, returned by the first SelectPalette */
	SelectPalette(priv->user.cached_dc, 0, FALSE);
	return;
    }

    SelectPalette(priv->user.cached_dc, pal->hpal, FALSE);

    DIB_DirectDrawSurface_set_palette(This, pal);
}

void User_DirectDrawSurface_update_palette(IDirectDrawSurfaceImpl* This,
					   IDirectDrawPaletteImpl* pal,
					   DWORD dwStart, DWORD dwCount,
					   LPPALETTEENTRY palent)
{
#ifndef SYNC_UPDATE
    USER_PRIV_VAR(priv, This);
#endif

    DIB_DirectDrawSurface_update_palette(This, pal, dwStart, dwCount, palent);
    /* FIXME: realize palette on display window */

#ifndef SYNC_UPDATE
    /* with async updates, it's probably cool to force an update */
    SetEvent(priv->user.update_event);
#endif
}

HRESULT User_DirectDrawSurface_duplicate_surface(IDirectDrawSurfaceImpl* This,
						 LPDIRECTDRAWSURFACE7* ppDup)
{
    return User_DirectDrawSurface_Create(This->ddraw_owner,
					 &This->surface_desc, ppDup, NULL);
}

BOOL User_DirectDrawSurface_flip_data(IDirectDrawSurfaceImpl* front,
				      IDirectDrawSurfaceImpl* back,
				      DWORD dwFlags)
{
    USER_PRIV_VAR(front_priv, front);
    USER_PRIV_VAR(back_priv, back);

    {
	HDC tmp;
	tmp = front_priv->user.cached_dc;
	front_priv->user.cached_dc = back_priv->user.cached_dc;
	back_priv->user.cached_dc = tmp;
    }

    return DIB_DirectDrawSurface_flip_data(front, back, dwFlags);
}

void User_DirectDrawSurface_flip_update(IDirectDrawSurfaceImpl* This, DWORD dwFlags)
{
#ifdef SYNC_UPDATE
    This->lastlockrect.left = This->lastlockrect.right = 0;
    assert(This->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE);
    User_copy_to_screen(This,NULL);
#else
    USER_PRIV_VAR(priv, This);
    assert(This->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE);
    if (dwFlags & DDFLIP_WAIT) User_DirectDrawSurface_wait_update(This);
    This->lastlockrect.left = This->lastlockrect.right = 0;
    SetEvent(priv->user.update_event);
#endif
}

HRESULT User_DirectDrawSurface_get_dc(IDirectDrawSurfaceImpl* This, HDC* phDC)
{
    USER_PRIV_VAR(priv, This);

    *phDC = priv->user.cached_dc;
    return S_OK;
}

HRESULT User_DirectDrawSurface_release_dc(IDirectDrawSurfaceImpl* This,
					  HDC hDC)
{
    return S_OK;
}

HRESULT User_DirectDrawSurface_get_gamma_ramp(IDirectDrawSurfaceImpl* This,
					      DWORD dwFlags,
					      LPDDGAMMARAMP lpGammaRamp)
{
    if (This->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
	POINT offset;
	HWND hDisplayWnd;
	HDC hDisplayDC;
	HRESULT hr;
	hDisplayWnd = get_display_window(This, &offset);
	hDisplayDC = GetDCEx(hDisplayWnd, 0, DCX_CLIPSIBLINGS|DCX_CACHE);
	hr = GetDeviceGammaRamp(hDisplayDC, lpGammaRamp) ? DD_OK : DDERR_UNSUPPORTED;
	ReleaseDC(hDisplayWnd, hDisplayDC);
	return hr;
    }
    return Main_DirectDrawSurface_get_gamma_ramp(This, dwFlags, lpGammaRamp);
}

HRESULT User_DirectDrawSurface_set_gamma_ramp(IDirectDrawSurfaceImpl* This,
					      DWORD dwFlags,
					      LPDDGAMMARAMP lpGammaRamp)
{
    if (This->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
	POINT offset;
	HWND hDisplayWnd;
	HDC hDisplayDC;
	HRESULT hr;
	hDisplayWnd = get_display_window(This, &offset);
	hDisplayDC = GetDCEx(hDisplayWnd, 0, DCX_CLIPSIBLINGS|DCX_CACHE);
	hr = SetDeviceGammaRamp(hDisplayDC, lpGammaRamp) ? DD_OK : DDERR_UNSUPPORTED;
	ReleaseDC(hDisplayWnd, hDisplayDC);
	return hr;
    }
    return Main_DirectDrawSurface_set_gamma_ramp(This, dwFlags, lpGammaRamp);
}

/* Returns the window that hosts the display.
 * *pt is set to the upper left position of the window relative to the
 * upper left corner of the surface. */
static HWND get_display_window(IDirectDrawSurfaceImpl* This, LPPOINT pt)
{
    memset(pt, 0, sizeof(*pt));

    if (This->ddraw_owner->cooperative_level & DDSCL_FULLSCREEN)
    {
#ifdef OWN_WINDOW
	USER_PRIV_VAR(priv, This);
#if 1
	SetWindowPos(priv->user.window, HWND_TOP, 0, 0, 0, 0,
		     SWP_DEFERERASE|SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_NOMOVE|
		     SWP_NOREDRAW|SWP_NOSENDCHANGING|SWP_NOSIZE);
#endif
	return priv->user.window;
#else
	return This->ddraw_owner->window;
#endif
    }
    else
    {
	if (This->clipper != NULL)
	{
	    /* looks silly, but since we don't have the clipper locked */
	    HWND hWnd = This->clipper->hWnd;

	    if (hWnd != 0)
	    {
		ClientToScreen(hWnd, pt);
		return hWnd;
	    }
	    else
	    {
		static BOOL warn = 0;
		if (!warn++) FIXME("clipper clip lists not supported\n");

		return GetDesktopWindow();
	    }
	}
	else
	{
	    static BOOL warn = 0;
	    if (!warn++) WARN("hosting on root\n");

	    return GetDesktopWindow();
	}
    }
}

HWND User_DirectDrawSurface_get_display_window(IDirectDrawSurfaceImpl* This)
{
    POINT offset;
    return get_display_window(This, &offset);
}

#ifdef OWN_WINDOW
static void User_create_own_window(IDirectDrawSurfaceImpl* This)
{
    USER_PRIV_VAR(priv, This);

    if (This->ddraw_owner->cooperative_level & DDSCL_FULLSCREEN)
    {
	priv->user.window = CreateWindowExA(WS_EX_TOPMOST |
					    WS_EX_LAYERED |
					    WS_EX_TRANSPARENT,
					    "WINE_DDRAW", "DirectDraw",
					    WS_POPUP,
					    0, 0,
					    This->surface_desc.dwWidth,
					    This->surface_desc.dwHeight,
					    GetDesktopWindow(),
					    0, 0, This);
	This->more.lpDDRAWReserved = (LPVOID)priv->user.window;
	SetWindowPos(priv->user.window, HWND_TOP, 0, 0, 0, 0,
		     SWP_DEFERERASE|SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_NOMOVE|
		     SWP_NOREDRAW|SWP_NOSENDCHANGING|SWP_NOSIZE|SWP_SHOWWINDOW);
	UpdateWindow(priv->user.window);
    }
}

static void User_destroy_own_window(IDirectDrawSurfaceImpl* This)
{
    USER_PRIV_VAR(priv, This);

    if (priv->user.window)
    {
	SetWindowPos(priv->user.window, 0, 0, 0, 0, 0,
		     SWP_DEFERERASE|SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_NOMOVE|SWP_NOZORDER|
		     SWP_NOREDRAW|SWP_NOSENDCHANGING|SWP_NOSIZE|SWP_HIDEWINDOW);
	This->more.lpDDRAWReserved = NULL;
	DestroyWindow(priv->user.window);
	priv->user.window = 0;
    }
}
#endif

#ifndef SYNC_UPDATE
static DWORD CALLBACK User_update_thread(LPVOID arg)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)arg;
    USER_PRIV_VAR(priv, This);
    volatile HANDLE *pActive = (volatile HANDLE *)&priv->user.update_event;
    HANDLE event = *pActive;

#ifdef OWN_WINDOW
    User_create_own_window(This);
#endif

    /* the point of this is that many games lock the primary surface
     * multiple times per frame; this thread will then simply copy as
     * often as it can without keeping the main thread waiting for
     * each unlock, thus keeping the frame rate high */
    do {
#ifdef OWN_WINDOW
	DWORD ret = MsgWaitForMultipleObjects(1, &event, FALSE, INFINITE, QS_ALLINPUT);
	MSG msg;

	while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
	{
	    switch (msg.message) {
	    case WM_PAINT:
		DispatchMessageA(&msg);
		break;
	    default:
		/* since we risk getting keyboard messages posted to us,
		 * repost posted messages to cooperative window */
		PostMessageA(This->ddraw_owner->window, msg.message, msg.wParam, msg.lParam);
	    }
	}
#else
	DWORD ret = WaitForSingleObject(event, INFINITE);
#endif
	if (ret == WAIT_OBJECT_0)
	{
	    if (*pActive) {
		priv->user.in_refresh = TRUE;
		User_copy_to_screen(This, NULL);
		EnterCriticalSection(&priv->user.crit);
		priv->user.in_refresh = FALSE;
		if (priv->user.wait_count)
		    SetEvent(priv->user.refresh_event);
		LeaveCriticalSection(&priv->user.crit);
	    } else
		break;
	}
	else if (ret != WAIT_OBJECT_0+1) break;
    } while (TRUE);

    SetEvent(priv->user.refresh_event);
#ifdef OWN_WINDOW
    User_destroy_own_window(This);
#endif

    return 0;
}
#endif

static void User_copy_to_screen(IDirectDrawSurfaceImpl* This, LPCRECT rc)
{
    if (This->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
	POINT offset;
	HWND hDisplayWnd;
	HDC hDisplayDC;
	HDC hSurfaceDC;
	RECT drawrect;

	if (FAILED(This->get_dc(This, &hSurfaceDC)))
	    return;

	hDisplayWnd = get_display_window(This, &offset);
	hDisplayDC = GetDCEx(hDisplayWnd, 0, DCX_CLIPSIBLINGS|DCX_CACHE);
#if 0
	/* FIXME: this doesn't work... if users really want to run
	 * X in 8bpp, then we need to call directly into display.drv
	 * (or Wine's equivalent), and force a private colormap
	 * without default entries. */
	if (This->palette) {
	    SelectPalette(hDisplayDC, This->palette->hpal, FALSE);
	    RealizePalette(hDisplayDC); /* sends messages => deadlocks */
	}
#endif
	drawrect.left	= 0;
	drawrect.right	= This->surface_desc.dwWidth;
	drawrect.top	= 0;
	drawrect.bottom	= This->surface_desc.dwHeight;

	if (This->clipper) {
	    RECT xrc;
	    HWND hwnd = This->clipper->hWnd;
	    if (hwnd && GetClientRect(hwnd,&xrc)) {
		OffsetRect(&xrc,offset.x,offset.y);
		IntersectRect(&drawrect,&drawrect,&xrc);
	    }
	}
	if (rc)
	    IntersectRect(&drawrect,&drawrect,rc);
	else {
	    /* Only use this if the caller did not pass a rectangle, since
	     * due to double locking this could be the wrong one ... */
	    if (This->lastlockrect.left != This->lastlockrect.right)
		IntersectRect(&drawrect,&drawrect,&This->lastlockrect);
	}
	BitBlt(hDisplayDC,
		drawrect.left-offset.x, drawrect.top-offset.y,
		drawrect.right-drawrect.left, drawrect.bottom-drawrect.top,
		hSurfaceDC,
		drawrect.left, drawrect.top,
		SRCCOPY
	);
	ReleaseDC(hDisplayWnd, hDisplayDC);
    }
}

#if 0
static void User_copy_from_screen(IDirectDrawSurfaceImpl* This, LPCRECT rc)
{
    if (This->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
	POINT offset;
	HWND hDisplayWnd = get_display_window(This, &offset);
	HDC hDisplayDC = GetDC(hDisplayWnd);
	RECT drawrect;

	drawrect.left	= 0;
	drawrect.right	= This->surface_desc.dwWidth;
	drawrect.top	= 0;
	drawrect.bottom	= This->surface_desc.dwHeight;
	if (rc)
	    IntersectRect(&drawrect,&drawrect,rc);

	BitBlt(This->hDC,
	    drawrect.left, drawrect.top,
	    drawrect.right-drawrect.left, drawrect.bottom-drawrect.top,
	    hDisplayDC,
	    drawrect.left+offset.x, drawrect.top+offset.y,
	    SRCCOPY
	);
	ReleaseDC(hDisplayWnd, hDisplayDC);
    }
}
#endif

static const IDirectDrawSurface7Vtbl User_IDirectDrawSurface7_VTable =
{
    Main_DirectDrawSurface_QueryInterface,
    Main_DirectDrawSurface_AddRef,
    Main_DirectDrawSurface_Release,
    Main_DirectDrawSurface_AddAttachedSurface,
    Main_DirectDrawSurface_AddOverlayDirtyRect,
    DIB_DirectDrawSurface_Blt,
    Main_DirectDrawSurface_BltBatch,
    DIB_DirectDrawSurface_BltFast,
    Main_DirectDrawSurface_DeleteAttachedSurface,
    Main_DirectDrawSurface_EnumAttachedSurfaces,
    Main_DirectDrawSurface_EnumOverlayZOrders,
    Main_DirectDrawSurface_Flip,
    Main_DirectDrawSurface_GetAttachedSurface,
    Main_DirectDrawSurface_GetBltStatus,
    Main_DirectDrawSurface_GetCaps,
    Main_DirectDrawSurface_GetClipper,
    Main_DirectDrawSurface_GetColorKey,
    Main_DirectDrawSurface_GetDC,
    Main_DirectDrawSurface_GetFlipStatus,
    Main_DirectDrawSurface_GetOverlayPosition,
    Main_DirectDrawSurface_GetPalette,
    Main_DirectDrawSurface_GetPixelFormat,
    Main_DirectDrawSurface_GetSurfaceDesc,
    Main_DirectDrawSurface_Initialize,
    Main_DirectDrawSurface_IsLost,
    Main_DirectDrawSurface_Lock,
    Main_DirectDrawSurface_ReleaseDC,
    DIB_DirectDrawSurface_Restore,
    Main_DirectDrawSurface_SetClipper,
    Main_DirectDrawSurface_SetColorKey,
    Main_DirectDrawSurface_SetOverlayPosition,
    Main_DirectDrawSurface_SetPalette,
    Main_DirectDrawSurface_Unlock,
    Main_DirectDrawSurface_UpdateOverlay,
    Main_DirectDrawSurface_UpdateOverlayDisplay,
    Main_DirectDrawSurface_UpdateOverlayZOrder,
    Main_DirectDrawSurface_GetDDInterface,
    Main_DirectDrawSurface_PageLock,
    Main_DirectDrawSurface_PageUnlock,
    DIB_DirectDrawSurface_SetSurfaceDesc,
    Main_DirectDrawSurface_SetPrivateData,
    Main_DirectDrawSurface_GetPrivateData,
    Main_DirectDrawSurface_FreePrivateData,
    Main_DirectDrawSurface_GetUniquenessValue,
    Main_DirectDrawSurface_ChangeUniquenessValue,
    Main_DirectDrawSurface_SetPriority,
    Main_DirectDrawSurface_GetPriority,
    Main_DirectDrawSurface_SetLOD,
    Main_DirectDrawSurface_GetLOD
};
