/*
 * Unit tests for DirectDraw overlay functions
 *
 * Copyright (C) 2008,2011 Stefan Dösinger for CodeWeavers
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
#define COBJMACROS

#include "wine/test.h"
#include "ddraw.h"
#include "unknwn.h"

static HRESULT (WINAPI *pDirectDrawCreateEx)(GUID *driver_guid,
        void **ddraw, REFIID interface_iid, IUnknown *outer);

static IDirectDraw7 *ddraw = NULL;
static IDirectDrawSurface7 *primary = NULL;

static IDirectDrawSurface7 *create_overlay(DWORD width, DWORD height, DWORD format) {
    DDSURFACEDESC2 ddsd;
    HRESULT hr;
    IDirectDrawSurface7 *ret;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.dwWidth = width;
    ddsd.dwHeight = height;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OVERLAY;
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_FOURCC;
    U4(ddsd).ddpfPixelFormat.dwFourCC = format;
    hr = IDirectDraw7_CreateSurface(ddraw, &ddsd, &ret, NULL);
    if(FAILED(hr)) return NULL;
    else return ret;
}

static BOOL CreateDirectDraw(void)
{
    HRESULT hr;
    DDSURFACEDESC2 ddsd;
    IDirectDrawSurface7 *overlay = NULL;
    HMODULE hmod = GetModuleHandleA("ddraw.dll");

    pDirectDrawCreateEx = (void*)GetProcAddress(hmod, "DirectDrawCreateEx");
    if (!pDirectDrawCreateEx) {
        win_skip("DirectDrawCreateEx is not available\n");
        return FALSE;
    }

    hr = pDirectDrawCreateEx(NULL, (void**)&ddraw, &IID_IDirectDraw7, NULL);
    ok(hr == DD_OK || hr == DDERR_NODIRECTDRAWSUPPORT, "DirectDrawCreateEx returned: %x\n", hr);
    if (!ddraw) {
        trace("DirectDrawCreateEx() failed with an error %x\n", hr);
        return FALSE;
    }

    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "SetCooperativeLevel returned: %x\n", hr );

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw7_CreateSurface(ddraw, &ddsd, &primary, NULL);
    if (FAILED(hr)) {
        IDirectDraw7_Release(ddraw);
        trace("IDirectDraw7_CreateSurface() failed with an error %x\n", hr);
        return FALSE;
    }

    overlay = create_overlay(64, 64, MAKEFOURCC('U','Y','V','Y'));
    if (!overlay) {
        IDirectDrawSurface7_Release(primary);
        IDirectDraw7_Release(ddraw);
        skip("Failed to create an overlay - assuming not supported\n");
        return FALSE;
    }
    IDirectDraw7_Release(overlay);

    return TRUE;
}

static void rectangle_settings(void) {
    IDirectDrawSurface7 *overlay = create_overlay(64, 64, MAKEFOURCC('U','Y','V','Y'));
    HRESULT hr, hr2;
    RECT rect = {0, 0, 64, 64};
    LONG posx, posy;

    /* The dx sdk sort of implies that rect must be set when DDOVER_SHOW is used. This is not true
     * in Windows Vista and earlier, but changed in Win7 */
    hr = IDirectDrawSurface7_UpdateOverlay(overlay, NULL, primary, &rect, DDOVER_SHOW, NULL);
    ok(hr == DD_OK, "IDirectDrawSurface7_UpdateOverlay failed with hr=0x%08x\n", hr);
    hr = IDirectDrawSurface7_UpdateOverlay(overlay, NULL, primary, NULL, DDOVER_HIDE, NULL);
    ok(hr == DD_OK, "IDirectDrawSurface7_UpdateOverlay failed with hr=0x%08x\n", hr);
    hr = IDirectDrawSurface7_UpdateOverlay(overlay, NULL, primary, NULL, DDOVER_SHOW, NULL);
    ok(hr == DD_OK || hr == DDERR_INVALIDPARAMS, "IDirectDrawSurface7_UpdateOverlay failed with hr=0x%08x\n", hr);

    /* Show that the overlay position is the (top, left) coordinate of the dest rectangle */
    rect.top += 16;
    rect.left += 32;
    rect.bottom += 16;
    rect.right += 32;
    hr = IDirectDrawSurface7_UpdateOverlay(overlay, NULL, primary, &rect, DDOVER_SHOW, NULL);
    ok(hr == DD_OK, "IDirectDrawSurface7_UpdateOverlay failed with hr=0x%08x\n", hr);
    posx = -1; posy = -1;
    hr = IDirectDrawSurface7_GetOverlayPosition(overlay, &posx, &posy);
    ok(hr == DD_OK, "IDirectDrawSurface7_GetOverlayPosition failed with hr=0x%08x\n", hr);
    ok(posx == rect.left && posy == rect.top, "Overlay position is (%d, %d), expected (%d, %d)\n",
       posx, posy, rect.left, rect.top);

    /* Passing a NULL dest rect sets the position to 0/0 . Visually it can be seen that the overlay overlays the whole primary(==screen)*/
    hr2 = IDirectDrawSurface7_UpdateOverlay(overlay, NULL, primary, NULL, 0, NULL);
    ok(hr2 == DD_OK || hr2 == DDERR_INVALIDPARAMS
            || hr2 == DDERR_OUTOFCAPS, "IDirectDrawSurface7_UpdateOverlay failed with hr=0x%08x\n", hr2);
    hr = IDirectDrawSurface7_GetOverlayPosition(overlay, &posx, &posy);
    ok(hr == DD_OK, "IDirectDrawSurface7_GetOverlayPosition failed with hr=0x%08x\n", hr);
    if (SUCCEEDED(hr2))
    {
        ok(posx == 0 && posy == 0, "Overlay position is (%d, %d), expected (%d, %d)\n",
                posx, posy, 0, 0);
    }
    else
    {
        /* Otherwise the position remains untouched */
        ok(posx == 32 && posy == 16, "Overlay position is (%d, %d), expected (%d, %d)\n",
                posx, posy, 32, 16);
    }
    /* The position cannot be retrieved when the overlay is not shown */
    hr = IDirectDrawSurface7_UpdateOverlay(overlay, NULL, primary, &rect, DDOVER_HIDE, NULL);
    ok(hr == DD_OK, "IDirectDrawSurface7_UpdateOverlay failed with hr=0x%08x\n", hr);
    posx = -1; posy = -1;
    hr = IDirectDrawSurface7_GetOverlayPosition(overlay, &posx, &posy);
    ok(hr == DDERR_OVERLAYNOTVISIBLE, "IDirectDrawSurface7_GetOverlayPosition failed with hr=0x%08x\n", hr);
    ok(posx == 0 && posy == 0, "Overlay position is (%d, %d), expected (%d, %d)\n",
       posx, posy, 0, 0);

    IDirectDrawSurface7_Release(overlay);
}

START_TEST(overlay)
{
    if(CreateDirectDraw() == FALSE) {
        skip("Failed to initialize ddraw\n");
        return;
    }

    rectangle_settings();

    if(primary) IDirectDrawSurface7_Release(primary);
    if(ddraw) IDirectDraw7_Release(ddraw);
}
