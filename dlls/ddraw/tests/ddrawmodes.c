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

static LPDIRECTDRAW lpDD;
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
    if (hwnd == NULL)
        assert(0);
    
    ShowWindow(hwnd, SW_HIDE);
    UpdateWindow(hwnd);
    SetFocus(hwnd);
    
    rc = DirectDrawCreate(NULL, &lpDD, NULL);
    ok(rc==DD_OK,"DirectDrawCreate returned: %lx",rc);
}

static void add_mode(LPDDSURFACEDESC lpddsd)
{
    if (!modes) 
	modes = malloc((modes_size = 2) * sizeof(DDSURFACEDESC));
    if (modes_cnt == modes_size) 
	    modes = realloc(modes, (modes_size *= 2) * sizeof(DDSURFACEDESC));
    assert(modes);
    modes[modes_cnt++] = *lpddsd;
}

static void flush_modes()
{
    free(modes);
    modes = 0;
    modes_cnt = modes_size = 0;
}

HRESULT WINAPI enummodes_callback(LPDDSURFACEDESC lpddsd, LPVOID lpContext)
{
    trace("Width = %lx, Height = %lx, Refresh Rate = %lx\r\n",
        lpddsd->dwWidth, lpddsd->dwHeight,
        lpddsd->UNION_MEMBER(2, dwRefreshRate));
    add_mode(lpddsd);

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
        DDEDM_STANDARDVGAMODES, &ddsd, 0, enummodes_callback);
    ok(rc==DD_OK,"EnumDisplayModes returned: %lx",rc);
}

static void setdisplaymode_tests()
{
    HRESULT rc;
    int i;
    
    createdirectdraw();
    enumdisplaymodes();
    for (i = 0; i < modes_cnt; ++i)
    {
        rc = IDirectDraw_SetCooperativeLevel(lpDD,
            hwnd, DDSCL_ALLOWMODEX | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
        ok(rc==DD_OK,"SetCooperativeLevel returned: %lx",rc);
        if (modes[i].dwFlags & DDSD_PIXELFORMAT)
        {
            if (modes[i].ddpfPixelFormat.dwFlags & DDPF_RGB)
            {
                rc = IDirectDraw_SetDisplayMode(lpDD,
                    modes[i].dwWidth, modes[i].dwHeight,
                    modes[i].ddpfPixelFormat.UNION_MEMBER(1, dwRGBBitCount));
                ok(rc==DD_OK,"SetDisplayMode returned: %lx",rc);
                rc = IDirectDraw_RestoreDisplayMode(lpDD);
                ok(rc==DD_OK,"RestoreDisplayMode returned: %lx",rc);
            }
        }
    }
    flush_modes();
}

START_TEST(ddrawmodes)
{
    setdisplaymode_tests();
}
