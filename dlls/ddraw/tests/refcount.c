/*
 * Some unit tests for ddraw reference counting
 *
 * Copyright (C) 2006 Stefan Dösinger
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

#include <assert.h>
#include "wine/test.h"
#include "ddraw.h"
#include "d3d.h"
#include "unknwn.h"

static HRESULT (WINAPI *pDirectDrawCreateEx)(LPGUID,LPVOID*,REFIID,LPUNKNOWN);

static void init_function_pointers(void)
{
    HMODULE hmod = GetModuleHandleA("ddraw.dll");

    if(hmod)
    {
        pDirectDrawCreateEx = (void*)GetProcAddress(hmod, "DirectDrawCreateEx");
    }
}

unsigned long getRefcount(IUnknown *iface)
{
    IUnknown_AddRef(iface);
    return IUnknown_Release(iface);
}

static void test_ddraw(void)
{
    HRESULT hr;
    unsigned long ref;
    IDirectDraw7 *DDraw;
    IDirectDrawPalette *palette;
    IDirectDrawSurface7 *surface;
    PALETTEENTRY Table[256];
    DDSURFACEDESC2 ddsd;

    hr = pDirectDrawCreateEx(NULL, (void **) &DDraw, &IID_IDirectDraw7, NULL);
    ok(hr == DD_OK || hr==DDERR_NODIRECTDRAWSUPPORT, "DirectDrawCreateEx returned: %lx\n", hr);
    if(!DDraw)
    {
        trace("Couldn't create DDraw interface, skipping tests\n");
        return;
    }

    ref = getRefcount( (IUnknown *) DDraw);
    ok(ref == 1, "Got refcount %ld, expected 1\n", ref);

    /* Fails without a cooplevel */
    hr = IDirectDraw7_CreatePalette(DDraw, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, Table, &palette, NULL);
    ok(hr == DDERR_NOCOOPERATIVELEVELSET, "CreatePalette returned %08lx\n", hr);

    /* This check is before the cooplevel check */
    hr = IDirectDraw7_CreatePalette(DDraw, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, Table, &palette, (void *) 0xdeadbeef);
    ok(hr == CLASS_E_NOAGGREGATION, "CreatePalette returned %08lx\n", hr);

    hr = IDirectDraw7_SetCooperativeLevel(DDraw, 0, DDSCL_NORMAL);
    ok(hr == DD_OK, "SetCooperativeLevel failed with %08lx\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd.dwWidth = 64;
    ddsd.dwHeight = 64;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
    U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 8;

    hr = IDirectDraw7_CreateSurface(DDraw, &ddsd, &surface, NULL);
    ok(hr == DD_OK, "CreateSurface failed with %08lx\n", hr);

    /* DDraw refcount increased by 1 */
    ref = getRefcount( (IUnknown *) DDraw);
    ok(ref == 2, "Got refcount %ld, expected 2\n", ref);

    /* Surface refcount starts with 1 */
    ref = getRefcount( (IUnknown *) surface);
    ok(ref == 1, "Got refcount %ld, expected 1\n", ref);

    hr = IDirectDraw7_CreatePalette(DDraw, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, Table, &palette, NULL);
    ok(hr == DD_OK, "CreatePalette returned %08lx\n", hr);

    /* DDraw refcount increased by 1 */
    ref = getRefcount( (IUnknown *) DDraw);
    ok(ref == 3, "Got refcount %ld, expected 3\n", ref);

    /* Palette starts with 1 */
    ref = getRefcount( (IUnknown *) palette);
    ok(ref == 1, "Got refcount %ld, expected 1\n", ref);

    /* Test attaching a palette to a surface */
    hr = IDirectDrawSurface7_SetPalette(surface, palette);
    ok(hr == DD_OK, "IDirectDrawSurface_SetPalette failed with %08lx\n", hr);

    /* Palette refcount increased, surface stays the same */
    ref = getRefcount( (IUnknown *) palette);
    ok(ref == 2, "Got refcount %ld, expected 2\n", ref);
    ref = getRefcount( (IUnknown *) surface);
    ok(ref == 1, "Got refcount %ld, expected 1\n", ref);

    IDirectDrawSurface7_Release(surface);
    /* Incresed before - decrease now */
    ref = getRefcount( (IUnknown *) DDraw);
    ok(ref == 2, "Got refcount %ld, expected 2\n", ref);

    /* Releasing the surface detaches the palette */
    ref = getRefcount( (IUnknown *) palette);
    ok(ref == 1, "Got refcount %ld, expected 1\n", ref);

    IDirectDrawPalette_Release(palette);

    /* Incresed before - decrease now */
    ref = getRefcount( (IUnknown *) DDraw);
    ok(ref == 1, "Got refcount %ld, expected 1\n", ref);

    IDirectDraw7_Release(DDraw);
}

START_TEST(refcount)
{
    init_function_pointers();
    if(!pDirectDrawCreateEx)
    {
        trace("function DirectDrawCreateEx not available, skipping tests\n");
        return;
    }
    test_ddraw();
}
