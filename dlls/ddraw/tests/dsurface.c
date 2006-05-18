/*
 * Unit tests for (a few) ddraw surface functions
 *
 * Copyright (C) 2005 Antoine Chavasse (a.chavasse@gmail.com)
 * Copyright (C) 2005 Christian Costa
 * Copyright 2005 Ivan Leo Puoti
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

#include <assert.h>
#include "wine/test.h"
#include "ddraw.h"

static LPDIRECTDRAW lpDD = NULL;

static void CreateDirectDraw(void)
{
    HRESULT rc;

    rc = DirectDrawCreate(NULL, &lpDD, NULL);
    ok(rc==DD_OK,"DirectDrawCreate returned: %lx\n",rc);

    rc = IDirectDraw_SetCooperativeLevel(lpDD, NULL, DDSCL_NORMAL);
    ok(rc==DD_OK,"SetCooperativeLevel returned: %lx\n",rc);
}


static void ReleaseDirectDraw(void)
{
    if( lpDD != NULL )
    {
        IDirectDraw_Release(lpDD);
        lpDD = NULL;
    }
}

static void MipMapCreationTest(void)
{
    LPDIRECTDRAWSURFACE lpDDSMipMapTest;
    DDSURFACEDESC ddsd;
    HRESULT rc;

    /* First mipmap creation test: create a surface with DDSCAPS_COMPLEX,
       DDSCAPS_MIPMAP, and DDSD_MIPMAPCOUNT. This create the number of
        requested mipmap levels. */
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_MIPMAPCOUNT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    U2(ddsd).dwMipMapCount = 3;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 32;
    rc = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpDDSMipMapTest, NULL);
    ok(rc==DD_OK,"CreateSurface returned: %lx\n",rc);

    /* Check the number of created mipmaps */
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(ddsd);
    rc = IDirectDrawSurface_GetSurfaceDesc(lpDDSMipMapTest, &ddsd);
    ok(rc==DD_OK,"GetSurfaceDesc returned: %lx\n",rc);
    ok(ddsd.dwFlags & DDSD_MIPMAPCOUNT,
        "GetSurfaceDesc returned no mipmapcount.\n");
    ok(U2(ddsd).dwMipMapCount == 3, "Incorrect mipmap count: %ld.\n",
        U2(ddsd).dwMipMapCount);

    /* Destroy the surface. */
    IDirectDrawSurface_Release(lpDDSMipMapTest);


    /* Second mipmap creation test: create a surface without a mipmap
       count, with DDSCAPS_MIPMAP and without DDSCAPS_COMPLEX.
       This creates a single mipmap level. */
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 32;
    rc = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpDDSMipMapTest, NULL);
    ok(rc==DD_OK,"CreateSurface returned: %lx\n",rc);

    /* Check the number of created mipmaps */
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(ddsd);
    rc = IDirectDrawSurface_GetSurfaceDesc(lpDDSMipMapTest, &ddsd);
    ok(rc==DD_OK,"GetSurfaceDesc returned: %lx\n",rc);
    ok(ddsd.dwFlags & DDSD_MIPMAPCOUNT,
        "GetSurfaceDesc returned no mipmapcount.\n");
    ok(U2(ddsd).dwMipMapCount == 1, "Incorrect mipmap count: %ld.\n",
        U2(ddsd).dwMipMapCount);


    /* Third mipmap creation test: create a surface with DDSCAPS_MIPMAP,
        DDSCAPS_COMPLEX and without DDSD_MIPMAPCOUNT.
       It's an undocumented features where a chain of mipmaps, starting from
       he specified size and down to the smallest size, is automatically
       created.
       Anarchy Online needs this feature to work. */
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 32;
    rc = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpDDSMipMapTest, NULL);
    ok(rc==DD_OK,"CreateSurface returned: %lx\n",rc);

    /* Check the number of created mipmaps */
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(ddsd);
    rc = IDirectDrawSurface_GetSurfaceDesc(lpDDSMipMapTest, &ddsd);
    ok(rc==DD_OK,"GetSurfaceDesc returned: %lx\n",rc);
    ok(ddsd.dwFlags & DDSD_MIPMAPCOUNT,
        "GetSurfaceDesc returned no mipmapcount.\n");
    ok(U2(ddsd).dwMipMapCount == 6, "Incorrect mipmap count: %ld.\n",
        U2(ddsd).dwMipMapCount);


    /* Fourth mipmap creation test: same as above with a different texture
       size.
       The purpose is to verify that the number of generated mipmaps is
       dependent on the smallest dimension. */
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    ddsd.dwWidth = 32;
    ddsd.dwHeight = 64;
    rc = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpDDSMipMapTest, NULL);
    ok(rc==DD_OK,"CreateSurface returned: %lx\n",rc);

    /* Check the number of created mipmaps */
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(ddsd);
    rc = IDirectDrawSurface_GetSurfaceDesc(lpDDSMipMapTest, &ddsd);
    ok(rc==DD_OK,"GetSurfaceDesc returned: %lx\n",rc);
    ok(ddsd.dwFlags & DDSD_MIPMAPCOUNT,
        "GetSurfaceDesc returned no mipmapcount.\n");
    ok(U2(ddsd).dwMipMapCount == 6, "Incorrect mipmap count: %ld.\n",
        U2(ddsd).dwMipMapCount);

    /* Destroy the surface. */
    IDirectDrawSurface_Release(lpDDSMipMapTest);
}

static void SrcColorKey32BlitTest(void)
{
    LPDIRECTDRAWSURFACE lpSrc;
    LPDIRECTDRAWSURFACE lpDst;
    DDSURFACEDESC ddsd;
    DDSURFACEDESC ddsd2;
    LPDWORD lpData;
    HRESULT rc;

    ddsd2.dwSize = sizeof(ddsd2);
    ddsd2.ddpfPixelFormat.dwSize = sizeof(ddsd2.ddpfPixelFormat);

    ddsd.dwSize = sizeof(ddsd);
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = 800;
    ddsd.dwHeight = 600;
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(ddsd.ddpfPixelFormat).dwRGBBitCount = 32;
    U2(ddsd.ddpfPixelFormat).dwRBitMask = 0xFF0000;
    U3(ddsd.ddpfPixelFormat).dwGBitMask = 0x00FF00;
    U4(ddsd.ddpfPixelFormat).dwBBitMask = 0x0000FF;
    rc = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpDst, NULL);
    ok(rc==DD_OK,"CreateSurface returned: %lx\n",rc);

    ddsd.dwFlags |= DDSD_CKSRCBLT;
    ddsd.ddckCKSrcBlt.dwColorSpaceLowValue = 0xFF00FF;
    ddsd.ddckCKSrcBlt.dwColorSpaceHighValue = 0xFF00FF;
    rc = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpSrc, NULL);
    ok(rc==DD_OK,"CreateSurface returned: %lx\n",rc);
    
    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %lx\n",rc);
    lpData = (LPDWORD)ddsd2.lpSurface;
    lpData[0] = 0xCCCCCCCC;
    lpData[1] = 0xCCCCCCCC;
    lpData[2] = 0xCCCCCCCC;
    lpData[3] = 0xCCCCCCCC;
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %lx\n",rc);

    rc = IDirectDrawSurface_Lock(lpSrc, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %lx\n",rc);
    lpData = (LPDWORD)ddsd2.lpSurface;
    lpData[0] = 0x77010203;
    lpData[1] = 0x00010203;
    lpData[2] = 0x77FF00FF;
    lpData[3] = 0x00FF00FF;
    rc = IDirectDrawSurface_Unlock(lpSrc, NULL);
    ok(rc==DD_OK,"Unlock returned: %lx\n",rc);

    IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYSRC, NULL);

    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %lx\n",rc);
    lpData = (LPDWORD)ddsd2.lpSurface;
    ok((lpData[0]==0x77010203)&&(lpData[1]==0x00010203)&&(lpData[2]==0xCCCCCCCC)&&(lpData[3]==0xCCCCCCCC),
       "Destination data after blitting is not correct\n");
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %lx\n",rc);

    IDirectDrawSurface_Release(lpSrc);
    IDirectDrawSurface_Release(lpDst);
}

static void QueryInterface(void)
{
    LPDIRECTDRAWSURFACE dsurface;
    DDSURFACEDESC surface;
    LPVOID object;
    HRESULT ret;

    /* Create a surface */
    ZeroMemory(&surface, sizeof(surface));
    surface.dwSize = sizeof(surface);
    surface.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
    surface.dwHeight = 10;
    surface.dwWidth = 10;
    ret = IDirectDraw_CreateSurface(lpDD, &surface, &dsurface, NULL);
    if(ret != DD_OK)
    {
        ok(FALSE, "IDirectDraw::CreateSurface failed with error %lx\n", ret);
        return;
    }

    /* Call IUnknown::QueryInterface */
    ret = IDirectDrawSurface_QueryInterface(dsurface, 0, &object);
    ok(ret == DDERR_INVALIDPARAMS, "IDirectDrawSurface::QueryInterface returned %lx\n", ret);

    IDirectDrawSurface_Release(dsurface);
}

START_TEST(dsurface)
{
    CreateDirectDraw();
    MipMapCreationTest();
    SrcColorKey32BlitTest();
    QueryInterface();
    ReleaseDirectDraw();
}
