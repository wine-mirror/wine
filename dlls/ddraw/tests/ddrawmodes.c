/*
 * Unit tests for ddraw functions
 *
 * Copyright (C) 2003 Sami Aario
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

#include <assert.h>
#include "wine/test.h"
#include "ddraw.h"

#ifdef NONAMELESSUNION
#define UNION_MEMBER(x, y) DUMMYUNIONNAME##x.y
#else
#define UNION_MEMBER(x, y) y
#endif

static LPDIRECTDRAW lpDD = NULL;
static LPDIRECTDRAWSURFACE lpDDSPrimary = NULL;
static LPDIRECTDRAWSURFACE lpDDSBack = NULL;
static WNDCLASS wc;
static HWND hwnd;
static int modes_cnt;
static int modes_size;
static LPDDSURFACEDESC modes;

static void createdirectdraw()
{
    HRESULT rc;
    
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DefWindowProcA;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandleA(0);
    wc.hIcon = LoadIconA(wc.hInstance, IDI_APPLICATION);
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "TestWindowClass";
    if(!RegisterClassA(&wc))
        assert(0);
    
    hwnd = CreateWindowExA(0, "TestWindowClass", "TestWindowClass",
        WS_POPUP, 0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, GetModuleHandleA(0), NULL);
    assert(hwnd != NULL);
    
    ShowWindow(hwnd, SW_HIDE);
    UpdateWindow(hwnd);
    SetFocus(hwnd);
    
    rc = DirectDrawCreate(NULL, &lpDD, NULL);
    ok(rc==DD_OK,"DirectDrawCreate returned: %lx\n",rc);
}


static void releasedirectdraw()
{
	if( lpDD != NULL )
	{
		IDirectDraw_Release(lpDD);
		lpDD = NULL;
	}
}

static void adddisplaymode(LPDDSURFACEDESC lpddsd)
{
    if (!modes) 
	modes = malloc((modes_size = 2) * sizeof(DDSURFACEDESC));
    if (modes_cnt == modes_size) 
	    modes = realloc(modes, (modes_size *= 2) * sizeof(DDSURFACEDESC));
    assert(modes);
    modes[modes_cnt++] = *lpddsd;
}

static void flushdisplaymodes()
{
    free(modes);
    modes = 0;
    modes_cnt = modes_size = 0;
}

HRESULT WINAPI enummodescallback(LPDDSURFACEDESC lpddsd, LPVOID lpContext)
{
    trace("Width = %li, Height = %li, Refresh Rate = %li\r\n",
        lpddsd->dwWidth, lpddsd->dwHeight,
        lpddsd->UNION_MEMBER(2, dwRefreshRate));
    adddisplaymode(lpddsd);

    return DDENUMRET_OK;
}

void enumdisplaymodes()
{
    DDSURFACEDESC ddsd;
    HRESULT rc;

    ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    rc = IDirectDraw_EnumDisplayModes(lpDD,
        DDEDM_STANDARDVGAMODES, &ddsd, 0, enummodescallback);
    ok(rc==DD_OK,"EnumDisplayModes returned: %lx\n",rc);
}

static void setdisplaymode(int i)
{
    HRESULT rc;

    rc = IDirectDraw_SetCooperativeLevel(lpDD,
        hwnd, DDSCL_ALLOWMODEX | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(rc==DD_OK,"SetCooperativeLevel returned: %lx\n",rc);
    if (modes[i].dwFlags & DDSD_PIXELFORMAT)
    {
        if (modes[i].ddpfPixelFormat.dwFlags & DDPF_RGB)
        {
            rc = IDirectDraw_SetDisplayMode(lpDD,
                modes[i].dwWidth, modes[i].dwHeight,
                modes[i].ddpfPixelFormat.UNION_MEMBER(1, dwRGBBitCount));
            ok(rc==DD_OK,"SetDisplayMode returned: %lx\n",rc);
            rc = IDirectDraw_RestoreDisplayMode(lpDD);
            ok(rc==DD_OK,"RestoreDisplayMode returned: %lx\n",rc);
            
        }
    }
}

static void createsurface()
{
    DDSURFACEDESC ddsd;
    DDSCAPS ddscaps;
    HRESULT rc;
    
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE |
        DDSCAPS_FLIP |
        DDSCAPS_COMPLEX;
    ddsd.dwBackBufferCount = 1;
    rc = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpDDSPrimary, NULL );
    ok(rc==DD_OK,"CreateSurface returned: %lx\n",rc);
    ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
    rc = IDirectDrawSurface_GetAttachedSurface(lpDDSPrimary, &ddscaps, &lpDDSBack);
    ok(rc==DD_OK,"GetAttachedSurface returned: %lx\n",rc);
}

static void destroysurface()
{
    if( lpDDSPrimary != NULL )
    {
        IDirectDrawSurface_Release(lpDDSPrimary);
        lpDDSPrimary = NULL;
    }
}

static void testsurface()
{
    const char* testMsg = "ddraw device context test";
    HDC hdc;
    HRESULT rc;
    
    rc = IDirectDrawSurface_GetDC(lpDDSBack, &hdc);
    ok(rc==DD_OK, "IDirectDrawSurface_GetDC returned: %lx\n",rc);
    SetBkColor(hdc, RGB(0, 0, 255));
    SetTextColor(hdc, RGB(255, 255, 0));
    TextOut(hdc, 0, 0, testMsg, lstrlen(testMsg));
    IDirectDrawSurface_ReleaseDC(lpDDSBack, hdc);
    ok(rc==DD_OK, "IDirectDrawSurface_ReleaseDC returned: %lx\n",rc);
    
    while (1)
    {
        rc = IDirectDrawSurface_Flip(lpDDSPrimary, NULL, DDFLIP_WAIT);
        ok(rc==DD_OK || rc==DDERR_SURFACELOST, "IDirectDrawSurface_BltFast returned: %lx\n",rc);

        if (rc == DD_OK)
        {
            break;
        }
        else if (rc == DDERR_SURFACELOST)
        {
            rc = IDirectDrawSurface_Restore(lpDDSPrimary);
            ok(rc==DD_OK, "IDirectDrawSurface_Restore returned: %lx\n",rc);
        }
    }
}

static void testdisplaymodes()
{
    int i;

    for (i = 0; i < modes_cnt; ++i)
    {
        setdisplaymode(i);
        createsurface();
        testsurface();
        destroysurface();
    }
}

START_TEST(ddrawmodes)
{
    createdirectdraw();
    enumdisplaymodes();
    testdisplaymodes();
    flushdisplaymodes();
    releasedirectdraw();
}
