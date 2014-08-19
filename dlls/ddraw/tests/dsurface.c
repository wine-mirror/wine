/*
 * Unit tests for (a few) ddraw surface functions
 *
 * Copyright (C) 2005 Antoine Chavasse (a.chavasse@gmail.com)
 * Copyright (C) 2005 Christian Costa
 * Copyright 2005 Ivan Leo Puoti
 * Copyright (C) 2007-2009, 2011 Stefan Dösinger for CodeWeavers
 * Copyright (C) 2008 Alexander Dorofeyev
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
#include "wine/exception.h"
#include "ddraw.h"
#include "d3d.h"
#include "unknwn.h"

static HRESULT (WINAPI *pDirectDrawCreateEx)(GUID *, void **, REFIID, IUnknown *);

static IDirectDraw *lpDD;
static DDCAPS ddcaps;

static BOOL CreateDirectDraw(void)
{
    HRESULT rc;

    rc = DirectDrawCreate(NULL, &lpDD, NULL);
    ok(rc==DD_OK || rc==DDERR_NODIRECTDRAWSUPPORT, "DirectDrawCreateEx returned: %x\n", rc);
    if (!lpDD) {
        trace("DirectDrawCreateEx() failed with an error %x\n", rc);
        return FALSE;
    }

    rc = IDirectDraw_SetCooperativeLevel(lpDD, NULL, DDSCL_NORMAL);
    ok(rc==DD_OK,"SetCooperativeLevel returned: %x\n",rc);

    return TRUE;
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
    IDirectDrawSurface *lpDDSMipMapTest;
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
    ok(rc==DD_OK,"CreateSurface returned: %x\n",rc);
    if (FAILED(rc))
    {
        skip("failed to create surface\n");
        return;
    }

    /* Check the number of created mipmaps */
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(ddsd);
    rc = IDirectDrawSurface_GetSurfaceDesc(lpDDSMipMapTest, &ddsd);
    ok(rc==DD_OK,"GetSurfaceDesc returned: %x\n",rc);
    ok(ddsd.dwFlags & DDSD_MIPMAPCOUNT,
        "GetSurfaceDesc returned no mipmapcount.\n");
    ok(U2(ddsd).dwMipMapCount == 3, "Incorrect mipmap count: %d.\n",
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
    ok(rc==DD_OK,"CreateSurface returned: %x\n",rc);
    if (FAILED(rc))
    {
        skip("failed to create surface\n");
        return;
    }
    /* Check the number of created mipmaps */
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(ddsd);
    rc = IDirectDrawSurface_GetSurfaceDesc(lpDDSMipMapTest, &ddsd);
    ok(rc==DD_OK,"GetSurfaceDesc returned: %x\n",rc);
    ok(ddsd.dwFlags & DDSD_MIPMAPCOUNT,
        "GetSurfaceDesc returned no mipmapcount.\n");
    ok(U2(ddsd).dwMipMapCount == 1, "Incorrect mipmap count: %d.\n",
        U2(ddsd).dwMipMapCount);

    /* Destroy the surface. */
    IDirectDrawSurface_Release(lpDDSMipMapTest);


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
    ok(rc==DD_OK,"CreateSurface returned: %x\n",rc);
    if (FAILED(rc))
    {
        skip("failed to create surface\n");
        return;
    }

    /* Check the number of created mipmaps */
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(ddsd);
    rc = IDirectDrawSurface_GetSurfaceDesc(lpDDSMipMapTest, &ddsd);
    ok(rc==DD_OK,"GetSurfaceDesc returned: %x\n",rc);
    ok(ddsd.dwFlags & DDSD_MIPMAPCOUNT,
        "GetSurfaceDesc returned no mipmapcount.\n");
    ok(U2(ddsd).dwMipMapCount == 6, "Incorrect mipmap count: %d.\n",
        U2(ddsd).dwMipMapCount);

    /* Destroy the surface. */
    IDirectDrawSurface_Release(lpDDSMipMapTest);


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
    ok(rc==DD_OK,"CreateSurface returned: %x\n",rc);
    if (FAILED(rc))
    {
        skip("failed to create surface\n");
        return;
    }

    /* Check the number of created mipmaps */
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(ddsd);
    rc = IDirectDrawSurface_GetSurfaceDesc(lpDDSMipMapTest, &ddsd);
    ok(rc==DD_OK,"GetSurfaceDesc returned: %x\n",rc);
    ok(ddsd.dwFlags & DDSD_MIPMAPCOUNT,
        "GetSurfaceDesc returned no mipmapcount.\n");
    ok(U2(ddsd).dwMipMapCount == 6, "Incorrect mipmap count: %d.\n",
        U2(ddsd).dwMipMapCount);

    /* Destroy the surface. */
    IDirectDrawSurface_Release(lpDDSMipMapTest);


    /* Fifth mipmap creation test: try to create a surface with
       DDSCAPS_COMPLEX, DDSCAPS_MIPMAP, DDSD_MIPMAPCOUNT,
       where dwMipMapCount = 0. This should fail. */

    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_MIPMAPCOUNT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    U2(ddsd).dwMipMapCount = 0;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 32;
    rc = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpDDSMipMapTest, NULL);
    ok(rc==DDERR_INVALIDPARAMS,"CreateSurface returned: %x\n",rc);

    /* Destroy the surface. */
    if( rc == DD_OK )
        IDirectDrawSurface_Release(lpDDSMipMapTest);

}

static void SrcColorKey32BlitTest(void)
{
    IDirectDrawSurface *lpSrc;
    IDirectDrawSurface *lpDst;
    DDSURFACEDESC ddsd, ddsd2;
    DDCOLORKEY DDColorKey;
    LPDWORD lpData;
    HRESULT rc;
    DDBLTFX fx;

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
    ok(rc==DD_OK,"CreateSurface returned: %x\n",rc);
    if (FAILED(rc))
    {
        skip("failed to create surface\n");
        return;
    }

    ddsd.dwFlags |= DDSD_CKSRCBLT;
    ddsd.ddckCKSrcBlt.dwColorSpaceLowValue = 0xFF00FF;
    ddsd.ddckCKSrcBlt.dwColorSpaceHighValue = 0xFF00FF;
    rc = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpSrc, NULL);
    ok(rc==DD_OK,"CreateSurface returned: %x\n",rc);
    if (FAILED(rc))
    {
        skip("failed to create surface\n");
        return;
    }

    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    lpData = ddsd2.lpSurface;
    lpData[0] = 0xCCCCCCCC;
    lpData[1] = 0xCCCCCCCC;
    lpData[2] = 0xCCCCCCCC;
    lpData[3] = 0xCCCCCCCC;

    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    rc = IDirectDrawSurface_Lock(lpSrc, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    ok((ddsd2.dwFlags & DDSD_LPSURFACE) == 0, "Surface desc has LPSURFACE Flags set\n");
    lpData = ddsd2.lpSurface;
    lpData[0] = 0x77010203;
    lpData[1] = 0x00010203;
    lpData[2] = 0x77FF00FF;
    lpData[3] = 0x00FF00FF;
    rc = IDirectDrawSurface_Unlock(lpSrc, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYSRC, NULL);

    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    ok((ddsd2.dwFlags & DDSD_LPSURFACE) == 0, "Surface desc has LPSURFACE Flags set\n");
    lpData = ddsd2.lpSurface;
    /* Different behavior on some drivers / windows versions. Some versions ignore the X channel when
     * color keying, but copy it to the destination surface. Others apply it for color keying, but
     * do not copy it into the destination surface.
     */
    if(lpData[0]==0x00010203) {
        trace("X channel was not copied into the destination surface\n");
        ok((lpData[0]==0x00010203)&&(lpData[1]==0x00010203)&&(lpData[2]==0x00FF00FF)&&(lpData[3]==0xCCCCCCCC),
           "Destination data after blitting is not correct\n");
    } else {
        ok((lpData[0]==0x77010203)&&(lpData[1]==0x00010203)&&(lpData[2]==0xCCCCCCCC)&&(lpData[3]==0xCCCCCCCC),
           "Destination data after blitting is not correct\n");
    }
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    /* Below we repeat the same test as above but now using BltFast instead of Blt. Before
     * we can carry out the test we need to restore the color of the destination surface.
     */
    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    lpData = ddsd2.lpSurface;
    lpData[0] = 0xCCCCCCCC;
    lpData[1] = 0xCCCCCCCC;
    lpData[2] = 0xCCCCCCCC;
    lpData[3] = 0xCCCCCCCC;
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    IDirectDrawSurface_BltFast(lpDst, 0, 0, lpSrc, NULL, DDBLTFAST_SRCCOLORKEY);

    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    ok((ddsd2.dwFlags & DDSD_LPSURFACE) == 0, "Surface desc has LPSURFACE Flags set\n");
    lpData = ddsd2.lpSurface;
    /* Different behavior on some drivers / windows versions. Some versions ignore the X channel when
     * color keying, but copy it to the destination surface. Others apply it for color keying, but
     * do not copy it into the destination surface.
     */
    if(lpData[0]==0x00010203) {
        trace("X channel was not copied into the destination surface\n");
        ok((lpData[0]==0x00010203)&&(lpData[1]==0x00010203)&&(lpData[2]==0x00FF00FF)&&(lpData[3]==0xCCCCCCCC),
           "Destination data after blitting is not correct\n");
    } else {
        ok((lpData[0]==0x77010203)&&(lpData[1]==0x00010203)&&(lpData[2]==0xCCCCCCCC)&&(lpData[3]==0xCCCCCCCC),
           "Destination data after blitting is not correct\n");
    }
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    /* Also test SetColorKey */
    IDirectDrawSurface_GetColorKey(lpSrc, DDCKEY_SRCBLT, &DDColorKey);
    ok(DDColorKey.dwColorSpaceLowValue == 0xFF00FF && DDColorKey.dwColorSpaceHighValue == 0xFF00FF,
       "GetColorKey does not return the colorkey used at surface creation\n");

    DDColorKey.dwColorSpaceLowValue = 0x00FF00;
    DDColorKey.dwColorSpaceHighValue = 0x00FF00;
    IDirectDrawSurface_SetColorKey(lpSrc, DDCKEY_SRCBLT, &DDColorKey);

    DDColorKey.dwColorSpaceLowValue = 0;
    DDColorKey.dwColorSpaceHighValue = 0;
    IDirectDrawSurface_GetColorKey(lpSrc, DDCKEY_SRCBLT, &DDColorKey);
    ok(DDColorKey.dwColorSpaceLowValue == 0x00FF00 && DDColorKey.dwColorSpaceHighValue == 0x00FF00,
       "GetColorKey does not return the colorkey set with SetColorKey\n");

    ddsd.ddckCKSrcBlt.dwColorSpaceLowValue = 0;
    ddsd.ddckCKSrcBlt.dwColorSpaceHighValue = 0;
    IDirectDrawSurface_GetSurfaceDesc(lpSrc, &ddsd);
    ok(ddsd.ddckCKSrcBlt.dwColorSpaceLowValue == 0x00FF00 && ddsd.ddckCKSrcBlt.dwColorSpaceHighValue == 0x00FF00,
       "GetSurfaceDesc does not return the colorkey set with SetColorKey\n");

    /* Test SetColorKey with dwColorSpaceHighValue < dwColorSpaceLowValue */
    DDColorKey.dwColorSpaceLowValue = 0x0000FF;
    DDColorKey.dwColorSpaceHighValue = 0x000000;
    IDirectDrawSurface_SetColorKey(lpSrc, DDCKEY_SRCBLT, &DDColorKey);

    DDColorKey.dwColorSpaceLowValue = 0;
    DDColorKey.dwColorSpaceHighValue = 0;
    IDirectDrawSurface_GetColorKey(lpSrc, DDCKEY_SRCBLT, &DDColorKey);
    ok(DDColorKey.dwColorSpaceLowValue == 0x0000FF && DDColorKey.dwColorSpaceHighValue == 0x0000FF,
       "GetColorKey does not return the colorkey set with SetColorKey (%x %x)\n", DDColorKey.dwColorSpaceLowValue, DDColorKey.dwColorSpaceHighValue);

    DDColorKey.dwColorSpaceLowValue = 0x0000FF;
    DDColorKey.dwColorSpaceHighValue = 0x000001;
    IDirectDrawSurface_SetColorKey(lpSrc, DDCKEY_SRCBLT, &DDColorKey);

    DDColorKey.dwColorSpaceLowValue = 0;
    DDColorKey.dwColorSpaceHighValue = 0;
    IDirectDrawSurface_GetColorKey(lpSrc, DDCKEY_SRCBLT, &DDColorKey);
    ok(DDColorKey.dwColorSpaceLowValue == 0x0000FF && DDColorKey.dwColorSpaceHighValue == 0x0000FF,
       "GetColorKey does not return the colorkey set with SetColorKey (%x %x)\n", DDColorKey.dwColorSpaceLowValue, DDColorKey.dwColorSpaceHighValue);

    DDColorKey.dwColorSpaceLowValue = 0x0000FF;
    DDColorKey.dwColorSpaceHighValue = 0x0000FE;
    IDirectDrawSurface_SetColorKey(lpSrc, DDCKEY_SRCBLT, &DDColorKey);

    DDColorKey.dwColorSpaceLowValue = 0;
    DDColorKey.dwColorSpaceHighValue = 0;
    IDirectDrawSurface_GetColorKey(lpSrc, DDCKEY_SRCBLT, &DDColorKey);
    ok(DDColorKey.dwColorSpaceLowValue == 0x0000FF && DDColorKey.dwColorSpaceHighValue == 0x0000FF,
       "GetColorKey does not return the colorkey set with SetColorKey (%x %x)\n", DDColorKey.dwColorSpaceLowValue, DDColorKey.dwColorSpaceHighValue);

    IDirectDrawSurface_Release(lpSrc);
    IDirectDrawSurface_Release(lpDst);

    /* start with a new set of surfaces to test the color keying parameters to blit */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CKSRCBLT | DDSD_CKDESTBLT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = 800;
    ddsd.dwHeight = 600;
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(ddsd.ddpfPixelFormat).dwRGBBitCount = 32;
    U2(ddsd.ddpfPixelFormat).dwRBitMask = 0xFF0000;
    U3(ddsd.ddpfPixelFormat).dwGBitMask = 0x00FF00;
    U4(ddsd.ddpfPixelFormat).dwBBitMask = 0x0000FF;
    ddsd.ddckCKDestBlt.dwColorSpaceLowValue = 0xFF0000;
    ddsd.ddckCKDestBlt.dwColorSpaceHighValue = 0xFF0000;
    ddsd.ddckCKSrcBlt.dwColorSpaceLowValue = 0x00FF00;
    ddsd.ddckCKSrcBlt.dwColorSpaceHighValue = 0x00FF00;
    rc = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpDst, NULL);
    ok(rc==DD_OK || rc == DDERR_NOCOLORKEYHW,"CreateSurface returned: %x\n",rc);
    if(FAILED(rc))
    {
        skip("Failed to create surface\n");
        return;
    }

    /* start with a new set of surfaces to test the color keying parameters to blit */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CKSRCBLT | DDSD_CKDESTBLT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = 800;
    ddsd.dwHeight = 600;
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(ddsd.ddpfPixelFormat).dwRGBBitCount = 32;
    U2(ddsd.ddpfPixelFormat).dwRBitMask = 0xFF0000;
    U3(ddsd.ddpfPixelFormat).dwGBitMask = 0x00FF00;
    U4(ddsd.ddpfPixelFormat).dwBBitMask = 0x0000FF;
    ddsd.ddckCKSrcBlt.dwColorSpaceLowValue = 0x0000FF;
    ddsd.ddckCKSrcBlt.dwColorSpaceHighValue = 0x0000FF;
    ddsd.ddckCKDestBlt.dwColorSpaceLowValue = 0x000000;
    ddsd.ddckCKDestBlt.dwColorSpaceHighValue = 0x000000;
    rc = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpSrc, NULL);
    ok(rc==DD_OK || rc == DDERR_NOCOLORKEYHW,"CreateSurface returned: %x\n",rc);
    if(FAILED(rc))
    {
        skip("Failed to create surface\n");
        IDirectDrawSurface_Release(lpDst);
        return;
    }

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    fx.ddckSrcColorkey.dwColorSpaceHighValue = 0x110000;
    fx.ddckSrcColorkey.dwColorSpaceLowValue = 0x110000;
    fx.ddckDestColorkey.dwColorSpaceHighValue = 0x001100;
    fx.ddckDestColorkey.dwColorSpaceLowValue = 0x001100;

    rc = IDirectDrawSurface_Lock(lpSrc, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    ok((ddsd2.dwFlags & DDSD_LPSURFACE) == 0, "Surface desc has LPSURFACE Flags set\n");
    lpData = ddsd2.lpSurface;
    lpData[0] = 0x000000FF; /* Applies to src blt key in src surface */
    lpData[1] = 0x00000000; /* Applies to dst blt key in src surface */
    lpData[2] = 0x00FF0000; /* Dst color key in dst surface */
    lpData[3] = 0x0000FF00; /* Src color key in dst surface */
    lpData[4] = 0x00001100; /* Src color key in ddbltfx */
    lpData[5] = 0x00110000; /* Dst color key in ddbltfx */
    rc = IDirectDrawSurface_Unlock(lpSrc, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    ok((ddsd2.dwFlags & DDSD_LPSURFACE) == 0, "Surface desc has LPSURFACE Flags set\n");
    lpData = ddsd2.lpSurface;
    lpData[0] = 0x55555555;
    lpData[1] = 0x55555555;
    lpData[2] = 0x55555555;
    lpData[3] = 0x55555555;
    lpData[4] = 0x55555555;
    lpData[5] = 0x55555555;
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    /* Test a blit without keying */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, 0, &fx);
    ok(rc == DD_OK, "IDirectDrawSurface_Blt returned %08x\n", rc);

    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    ok((ddsd2.dwFlags & DDSD_LPSURFACE) == 0, "Surface desc has LPSURFACE Flags set\n");
    lpData = ddsd2.lpSurface;
    /* Should have copied src data unmodified to dst */
    ok(lpData[0] == 0x000000FF &&
       lpData[1] == 0x00000000 &&
       lpData[2] == 0x00FF0000 &&
       lpData[3] == 0x0000FF00 &&
       lpData[4] == 0x00001100 &&
       lpData[5] == 0x00110000, "Surface data after unkeyed blit does not match\n");

    lpData[0] = 0x55555555;
    lpData[1] = 0x55555555;
    lpData[2] = 0x55555555;
    lpData[3] = 0x55555555;
    lpData[4] = 0x55555555;
    lpData[5] = 0x55555555;
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    /* Src key */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYSRC, &fx);
    ok(rc == DD_OK, "IDirectDrawSurface_Blt returned %08x\n", rc);

    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    ok((ddsd2.dwFlags & DDSD_LPSURFACE) == 0, "Surface desc has LPSURFACE Flags set\n");
    lpData = ddsd2.lpSurface;

    ok(lpData[0] == 0x55555555 && /* Here the src key applied */
       lpData[1] == 0x00000000 &&
       lpData[2] == 0x00FF0000 &&
       lpData[3] == 0x0000FF00 &&
       lpData[4] == 0x00001100 &&
       lpData[5] == 0x00110000, "Surface data after srckey blit does not match\n");

    lpData[0] = 0x55555555;
    lpData[1] = 0x55555555;
    lpData[2] = 0x55555555;
    lpData[3] = 0x55555555;
    lpData[4] = 0x55555555;
    lpData[5] = 0x55555555;
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    /* Src override */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYSRCOVERRIDE, &fx);
    ok(rc == DD_OK, "IDirectDrawSurface_Blt returned %08x\n", rc);

    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    ok((ddsd2.dwFlags & DDSD_LPSURFACE) == 0, "Surface desc has LPSURFACE Flags set\n");
    lpData = ddsd2.lpSurface;

    ok(lpData[0] == 0x000000FF &&
       lpData[1] == 0x00000000 &&
       lpData[2] == 0x00FF0000 &&
       lpData[3] == 0x0000FF00 &&
       lpData[4] == 0x00001100 &&
       lpData[5] == 0x55555555, /* Override key applies here */
       "Surface data after src override key blit does not match\n");

    lpData[0] = 0x55555555;
    lpData[1] = 0x55555555;
    lpData[2] = 0x55555555;
    lpData[3] = 0x55555555;
    lpData[4] = 0x55555555;
    lpData[5] = 0x55555555;
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    /* Src override AND src key. That is not supposed to work */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYSRC | DDBLT_KEYSRCOVERRIDE, &fx);
    ok(rc == DDERR_INVALIDPARAMS, "IDirectDrawSurface_Blt returned %08x\n", rc);

    /* Verify that the destination is unchanged */
    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    ok((ddsd2.dwFlags & DDSD_LPSURFACE) == 0, "Surface desc has LPSURFACE Flags set\n");
    lpData = ddsd2.lpSurface;

    ok(lpData[0] == 0x55555555 &&
       lpData[1] == 0x55555555 &&
       lpData[2] == 0x55555555 &&
       lpData[3] == 0x55555555 &&
       lpData[4] == 0x55555555 &&
       lpData[5] == 0x55555555, /* Override key applies here */
       "Surface data after src key blit with override does not match\n");

    lpData[0] = 0x00FF0000; /* Dest key in dst surface */
    lpData[1] = 0x00FF0000; /* Dest key in dst surface */
    lpData[2] = 0x00001100; /* Dest key in override */
    lpData[3] = 0x00001100; /* Dest key in override */
    lpData[4] = 0x00000000; /* Dest key in src surface */
    lpData[5] = 0x00000000; /* Dest key in src surface */
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    /* Dest key blit */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYDEST, &fx);
    ok(rc == DD_OK, "IDirectDrawSurface_Blt returned %08x\n", rc);

    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    ok((ddsd2.dwFlags & DDSD_LPSURFACE) == 0, "Surface desc has LPSURFACE Flags set\n");
    lpData = ddsd2.lpSurface;

    /* DirectDraw uses the dest blit key from the SOURCE surface ! */
    ok(lpData[0] == 0x00ff0000 &&
       lpData[1] == 0x00ff0000 &&
       lpData[2] == 0x00001100 &&
       lpData[3] == 0x00001100 &&
       lpData[4] == 0x00001100 && /* Key applies here */
       lpData[5] == 0x00110000,   /* Key applies here */
       "Surface data after dest key blit does not match\n");

    lpData[0] = 0x00FF0000; /* Dest key in dst surface */
    lpData[1] = 0x00FF0000; /* Dest key in dst surface */
    lpData[2] = 0x00001100; /* Dest key in override */
    lpData[3] = 0x00001100; /* Dest key in override */
    lpData[4] = 0x00000000; /* Dest key in src surface */
    lpData[5] = 0x00000000; /* Dest key in src surface */
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    /* Dest override key blit */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYDESTOVERRIDE, &fx);
    ok(rc == DD_OK, "IDirectDrawSurface_Blt returned %08x\n", rc);

    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    ok((ddsd2.dwFlags & DDSD_LPSURFACE) == 0, "Surface desc has LPSURFACE Flags set\n");
    lpData = ddsd2.lpSurface;

    ok(lpData[0] == 0x00FF0000 &&
       lpData[1] == 0x00FF0000 &&
       lpData[2] == 0x00FF0000 && /* Key applies here */
       lpData[3] == 0x0000FF00 && /* Key applies here */
       lpData[4] == 0x00000000 &&
       lpData[5] == 0x00000000,
       "Surface data after dest key override blit does not match\n");

    lpData[0] = 0x00FF0000; /* Dest key in dst surface */
    lpData[1] = 0x00FF0000; /* Dest key in dst surface */
    lpData[2] = 0x00001100; /* Dest key in override */
    lpData[3] = 0x00001100; /* Dest key in override */
    lpData[4] = 0x00000000; /* Dest key in src surface */
    lpData[5] = 0x00000000; /* Dest key in src surface */
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    /* Dest override key blit. Supposed to fail too */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYDEST | DDBLT_KEYDESTOVERRIDE, &fx);
    ok(rc == DDERR_INVALIDPARAMS, "IDirectDrawSurface_Blt returned %08x\n", rc);

    /* Check for unchanged data */
    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    ok((ddsd2.dwFlags & DDSD_LPSURFACE) == 0, "Surface desc has LPSURFACE Flags set\n");
    lpData = ddsd2.lpSurface;

    ok(lpData[0] == 0x00FF0000 &&
       lpData[1] == 0x00FF0000 &&
       lpData[2] == 0x00001100 && /* Key applies here */
       lpData[3] == 0x00001100 && /* Key applies here */
       lpData[4] == 0x00000000 &&
       lpData[5] == 0x00000000,
       "Surface data with dest key and dest override does not match\n");

    lpData[0] = 0x00FF0000; /* Dest key in dst surface */
    lpData[1] = 0x00FF0000; /* Dest key in dst surface */
    lpData[2] = 0x00001100; /* Dest key in override */
    lpData[3] = 0x00001100; /* Dest key in override */
    lpData[4] = 0x00000000; /* Dest key in src surface */
    lpData[5] = 0x00000000; /* Dest key in src surface */
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    /* Modify the source data a bit to give some more conclusive results */
    rc = IDirectDrawSurface_Lock(lpSrc, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    ok((ddsd2.dwFlags & DDSD_LPSURFACE) == 0, "Surface desc has LPSURFACE Flags set\n");
    lpData = ddsd2.lpSurface;
    lpData[5] = 0x000000FF; /* Applies to src blt key in src surface */
    rc = IDirectDrawSurface_Unlock(lpSrc, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    /* Source and destination key */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYDEST | DDBLT_KEYSRC, &fx);
    ok(rc == DD_OK, "IDirectDrawSurface_Blt returned %08x\n", rc);

    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    ok((ddsd2.dwFlags & DDSD_LPSURFACE) == 0, "Surface desc has LPSURFACE Flags set\n");
    lpData = ddsd2.lpSurface;

    ok(lpData[0] == 0x00FF0000 && /* Masked by Destination key */
       lpData[1] == 0x00FF0000 && /* Masked by Destination key */
       lpData[2] == 0x00001100 && /* Masked by Destination key */
       lpData[3] == 0x00001100 && /* Masked by Destination key */
       lpData[4] == 0x00001100 && /* Allowed by destination key, not masked by source key */
       lpData[5] == 0x00000000,   /* Allowed by dst key, but masked by source key */
       "Surface data with src key and dest key blit does not match\n");

    lpData[0] = 0x00FF0000; /* Dest key in dst surface */
    lpData[1] = 0x00FF0000; /* Dest key in dst surface */
    lpData[2] = 0x00001100; /* Dest key in override */
    lpData[3] = 0x00001100; /* Dest key in override */
    lpData[4] = 0x00000000; /* Dest key in src surface */
    lpData[5] = 0x00000000; /* Dest key in src surface */
    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    /* Override keys without ddbltfx parameter fail */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYDESTOVERRIDE, NULL);
    ok(rc == DDERR_INVALIDPARAMS, "IDirectDrawSurface_Blt returned %08x\n", rc);
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYSRCOVERRIDE, NULL);
    ok(rc == DDERR_INVALIDPARAMS, "IDirectDrawSurface_Blt returned %08x\n", rc);

    /* Try blitting without keys in the source surface*/
    rc = IDirectDrawSurface_SetColorKey(lpSrc, DDCKEY_SRCBLT, NULL);
    ok(rc == DD_OK, "SetColorKey returned %x\n", rc);
    rc = IDirectDrawSurface_SetColorKey(lpSrc, DDCKEY_DESTBLT, NULL);
    ok(rc == DD_OK, "SetColorKey returned %x\n", rc);

    /* That fails now. Do not bother to check that the data is unmodified */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYSRC, &fx);
    ok(rc == DDERR_INVALIDPARAMS, "IDirectDrawSurface_Blt returned %08x\n", rc);

    /* Dest key blit still works. Which key is used this time??? */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYDEST, &fx);
    ok(rc == DD_OK, "IDirectDrawSurface_Blt returned %08x\n", rc);

    /* With correctly passed override keys no key in the surface is needed.
     * Again, the result was checked before, no need to do that again
     */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYDESTOVERRIDE, &fx);
    ok(rc == DD_OK, "IDirectDrawSurface_Blt returned %08x\n", rc);
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYSRCOVERRIDE, &fx);
    ok(rc == DD_OK, "IDirectDrawSurface_Blt returned %08x\n", rc);

    IDirectDrawSurface_Release(lpSrc);
    IDirectDrawSurface_Release(lpDst);
}

static void QueryInterface(void)
{
    IDirectDrawSurface *dsurface;
    DDSURFACEDESC surface;
    void *object;
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
        ok(FALSE, "IDirectDraw::CreateSurface failed with error %x\n", ret);
        return;
    }

    /* Call IUnknown::QueryInterface */
    ret = IDirectDrawSurface_QueryInterface(dsurface, 0, &object);
    ok(ret == DDERR_INVALIDPARAMS, "IDirectDrawSurface::QueryInterface returned %x\n", ret);

    IDirectDrawSurface_Release(dsurface);
}

/* The following tests test which interface is returned by IDirectDrawSurfaceX::GetDDInterface.
 * It uses refcounts to test that and compares the interface addresses. Partially fits here, and
 * partially in the refcount test
 */

static ULONG getref(IUnknown *iface)
{
    IUnknown_AddRef(iface);
    return IUnknown_Release(iface);
}

static void GetDDInterface_1(void)
{
    IDirectDrawSurface2 *dsurface2;
    IDirectDrawSurface *dsurface;
    DDSURFACEDESC surface;
    HRESULT ret;
    IDirectDraw2 *dd2;
    IDirectDraw4 *dd4;
    IDirectDraw7 *dd7;
    ULONG ref1, ref2, ref4, ref7;
    void *dd;

    /* Create a surface */
    ZeroMemory(&surface, sizeof(surface));
    surface.dwSize = sizeof(surface);
    surface.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
    surface.dwHeight = 10;
    surface.dwWidth = 10;
    ret = IDirectDraw_CreateSurface(lpDD, &surface, &dsurface, NULL);
    if(ret != DD_OK)
    {
        ok(FALSE, "IDirectDraw::CreateSurface failed with error %x\n", ret);
        return;
    }
    ret = IDirectDrawSurface_QueryInterface(dsurface, &IID_IDirectDrawSurface2, (void **) &dsurface2);
    ok(ret == DD_OK, "IDirectDrawSurface_QueryInterface returned %08x\n", ret);
    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw2, (void **) &dd2);
    ok(ret == DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", ret);
    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw4, (void **) &dd4);
    ok(ret == DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", ret);
    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    ok(ret == DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", ret);

    ref1 = getref((IUnknown *) lpDD);
    ok(ref1 == 1, "IDirectDraw refcount is %d\n", ref1);
    ref2 = getref((IUnknown *) dd2);
    ok(ref2 == 1, "IDirectDraw2 refcount is %d\n", ref2);
    ref4 = getref((IUnknown *) dd4);
    ok(ref4 == 1, "IDirectDraw4 refcount is %d\n", ref4);
    ref7 = getref((IUnknown *) dd7);
    ok(ref7 == 1, "IDirectDraw7 refcount is %d\n", ref7);


    ret = IDirectDrawSurface2_GetDDInterface(dsurface2, &dd);
    ok(ret == DD_OK, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);
    ok(getref((IUnknown *) lpDD) == ref1 + 1, "IDirectDraw refcount was increased by %d\n", getref((IUnknown *) lpDD) - ref1);
    ok(getref((IUnknown *) dd2) == ref2 + 0, "IDirectDraw2 refcount was increased by %d\n", getref((IUnknown *) dd2) - ref2);
    ok(getref((IUnknown *) dd4) == ref4 + 0, "IDirectDraw4 refcount was increased by %d\n", getref((IUnknown *) dd4) - ref4);
    ok(getref((IUnknown *) dd7) == ref7 + 0, "IDirectDraw7 refcount was increased by %d\n", getref((IUnknown *) dd7) - ref7);

    ok(dd == lpDD, "Returned interface pointer is not equal to the creation interface\n");
    IUnknown_Release((IUnknown *) dd);

    /* try a NULL pointer */
    ret = IDirectDrawSurface2_GetDDInterface(dsurface2, NULL);
    ok(ret == DDERR_INVALIDPARAMS, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);

    IDirectDraw_Release(dd2);
    IDirectDraw_Release(dd4);
    IDirectDraw_Release(dd7);
    IDirectDrawSurface2_Release(dsurface2);
    IDirectDrawSurface_Release(dsurface);
}

static void GetDDInterface_2(void)
{
    IDirectDrawSurface2 *dsurface2;
    IDirectDrawSurface *dsurface;
    DDSURFACEDESC surface;
    HRESULT ret;
    IDirectDraw2 *dd2;
    IDirectDraw4 *dd4;
    IDirectDraw7 *dd7;
    ULONG ref1, ref2, ref4, ref7;
    void *dd;

    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw2, (void **) &dd2);
    ok(ret == DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", ret);
    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw4, (void **) &dd4);
    ok(ret == DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", ret);
    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    ok(ret == DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", ret);

    /* Create a surface */
    ZeroMemory(&surface, sizeof(surface));
    surface.dwSize = sizeof(surface);
    surface.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
    surface.dwHeight = 10;
    surface.dwWidth = 10;
    ret = IDirectDraw2_CreateSurface(dd2, &surface, &dsurface, NULL);
    if(ret != DD_OK)
    {
        ok(FALSE, "IDirectDraw::CreateSurface failed with error %x\n", ret);
        return;
    }
    ret = IDirectDrawSurface_QueryInterface(dsurface, &IID_IDirectDrawSurface2, (void **) &dsurface2);
    ok(ret == DD_OK, "IDirectDrawSurface_QueryInterface returned %08x\n", ret);

    ref1 = getref((IUnknown *) lpDD);
    ok(ref1 == 1, "IDirectDraw refcount is %d\n", ref1);
    ref2 = getref((IUnknown *) dd2);
    ok(ref2 == 1, "IDirectDraw2 refcount is %d\n", ref2);
    ref4 = getref((IUnknown *) dd4);
    ok(ref4 == 1, "IDirectDraw4 refcount is %d\n", ref4);
    ref7 = getref((IUnknown *) dd7);
    ok(ref7 == 1, "IDirectDraw7 refcount is %d\n", ref7);


    ret = IDirectDrawSurface2_GetDDInterface(dsurface2, &dd);
    ok(ret == DD_OK, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);
    ok(getref((IUnknown *) lpDD) == ref1 + 0, "IDirectDraw refcount was increased by %d\n", getref((IUnknown *) lpDD) - ref1);
    ok(getref((IUnknown *) dd2) == ref2 + 1, "IDirectDraw2 refcount was increased by %d\n", getref((IUnknown *) dd2) - ref2);
    ok(getref((IUnknown *) dd4) == ref4 + 0, "IDirectDraw4 refcount was increased by %d\n", getref((IUnknown *) dd4) - ref4);
    ok(getref((IUnknown *) dd7) == ref7 + 0, "IDirectDraw7 refcount was increased by %d\n", getref((IUnknown *) dd7) - ref7);

    ok(dd == dd2, "Returned interface pointer is not equal to the creation interface\n");
    IUnknown_Release((IUnknown *) dd);

    IDirectDraw_Release(dd2);
    IDirectDraw_Release(dd4);
    IDirectDraw_Release(dd7);
    IDirectDrawSurface2_Release(dsurface2);
    IDirectDrawSurface_Release(dsurface);
}

static void GetDDInterface_4(void)
{
    IDirectDrawSurface4 *dsurface4;
    IDirectDrawSurface2 *dsurface2;
    DDSURFACEDESC2 surface;
    HRESULT ret;
    IDirectDraw2 *dd2;
    IDirectDraw4 *dd4;
    IDirectDraw7 *dd7;
    ULONG ref1, ref2, ref4, ref7;
    void *dd;

    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw2, (void **) &dd2);
    ok(ret == DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", ret);
    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw4, (void **) &dd4);
    ok(ret == DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", ret);
    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    ok(ret == DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", ret);

    /* Create a surface */
    ZeroMemory(&surface, sizeof(surface));
    surface.dwSize = sizeof(surface);
    surface.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
    surface.dwHeight = 10;
    surface.dwWidth = 10;
    ret = IDirectDraw4_CreateSurface(dd4, &surface, &dsurface4, NULL);
    if(ret != DD_OK)
    {
        ok(FALSE, "IDirectDraw::CreateSurface failed with error %x\n", ret);
        return;
    }
    ret = IDirectDrawSurface4_QueryInterface(dsurface4, &IID_IDirectDrawSurface2, (void **) &dsurface2);
    ok(ret == DD_OK, "IDirectDrawSurface_QueryInterface returned %08x\n", ret);

    ref1 = getref((IUnknown *) lpDD);
    ok(ref1 == 1, "IDirectDraw refcount is %d\n", ref1);
    ref2 = getref((IUnknown *) dd2);
    ok(ref2 == 1, "IDirectDraw2 refcount is %d\n", ref2);
    ref4 = getref((IUnknown *) dd4);
    ok(ref4 == 2, "IDirectDraw4 refcount is %d\n", ref4);
    ref7 = getref((IUnknown *) dd7);
    ok(ref7 == 1, "IDirectDraw7 refcount is %d\n", ref7);

    ret = IDirectDrawSurface4_GetDDInterface(dsurface4, &dd);
    ok(ret == DD_OK, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);
    ok(getref((IUnknown *) lpDD) == ref1 + 0, "IDirectDraw refcount was increased by %d\n", getref((IUnknown *) lpDD) - ref1);
    ok(getref((IUnknown *) dd2) == ref2 + 0, "IDirectDraw2 refcount was increased by %d\n", getref((IUnknown *) dd2) - ref2);
    ok(getref((IUnknown *) dd4) == ref4 + 1, "IDirectDraw4 refcount was increased by %d\n", getref((IUnknown *) dd4) - ref4);
    ok(getref((IUnknown *) dd7) == ref7 + 0, "IDirectDraw7 refcount was increased by %d\n", getref((IUnknown *) dd7) - ref7);

    ok(dd == dd4, "Returned interface pointer is not equal to the creation interface\n");
    IUnknown_Release((IUnknown *) dd);

    /* Now test what happens if we QI the surface for some other version - It should still return the creation interface */
    ret = IDirectDrawSurface2_GetDDInterface(dsurface2, &dd);
    ok(ret == DD_OK, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);
    ok(getref((IUnknown *) lpDD) == ref1 + 0, "IDirectDraw refcount was increased by %d\n", getref((IUnknown *) lpDD) - ref1);
    ok(getref((IUnknown *) dd2) == ref2 + 0, "IDirectDraw2 refcount was increased by %d\n", getref((IUnknown *) dd2) - ref2);
    ok(getref((IUnknown *) dd4) == ref4 + 1, "IDirectDraw4 refcount was increased by %d\n", getref((IUnknown *) dd4) - ref4);
    ok(getref((IUnknown *) dd7) == ref7 + 0, "IDirectDraw7 refcount was increased by %d\n", getref((IUnknown *) dd7) - ref7);

    ok(dd == dd4, "Returned interface pointer is not equal to the creation interface\n");
    IUnknown_Release((IUnknown *) dd);

    IDirectDraw_Release(dd2);
    IDirectDraw_Release(dd4);
    IDirectDraw_Release(dd7);
    IDirectDrawSurface4_Release(dsurface4);
    IDirectDrawSurface2_Release(dsurface2);
}

static void GetDDInterface_7(void)
{
    IDirectDrawSurface7 *dsurface7;
    IDirectDrawSurface4 *dsurface4;
    DDSURFACEDESC2 surface;
    HRESULT ret;
    IDirectDraw2 *dd2;
    IDirectDraw4 *dd4;
    IDirectDraw7 *dd7;
    ULONG ref1, ref2, ref4, ref7;
    void *dd;

    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw2, (void **) &dd2);
    ok(ret == DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", ret);
    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw4, (void **) &dd4);
    ok(ret == DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", ret);
    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    ok(ret == DD_OK, "IDirectDraw7_QueryInterface returned %08x\n", ret);

    /* Create a surface */
    ZeroMemory(&surface, sizeof(surface));
    surface.dwSize = sizeof(surface);
    surface.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
    surface.dwHeight = 10;
    surface.dwWidth = 10;
    ret = IDirectDraw7_CreateSurface(dd7, &surface, &dsurface7, NULL);
    if(ret != DD_OK)
    {
        ok(FALSE, "IDirectDraw::CreateSurface failed with error %x\n", ret);
        return;
    }
    ret = IDirectDrawSurface7_QueryInterface(dsurface7, &IID_IDirectDrawSurface4, (void **) &dsurface4);
    ok(ret == DD_OK, "IDirectDrawSurface_QueryInterface returned %08x\n", ret);

    ref1 = getref((IUnknown *) lpDD);
    ok(ref1 == 1, "IDirectDraw refcount is %d\n", ref1);
    ref2 = getref((IUnknown *) dd2);
    ok(ref2 == 1, "IDirectDraw2 refcount is %d\n", ref2);
    ref4 = getref((IUnknown *) dd4);
    ok(ref4 == 1, "IDirectDraw4 refcount is %d\n", ref4);
    ref7 = getref((IUnknown *) dd7);
    ok(ref7 == 2, "IDirectDraw7 refcount is %d\n", ref7);

    ret = IDirectDrawSurface7_GetDDInterface(dsurface7, &dd);
    ok(ret == DD_OK, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);
    ok(getref((IUnknown *) lpDD) == ref1 + 0, "IDirectDraw refcount was increased by %d\n", getref((IUnknown *) lpDD) - ref1);
    ok(getref((IUnknown *) dd2) == ref2 + 0, "IDirectDraw2 refcount was increased by %d\n", getref((IUnknown *) dd2) - ref2);
    ok(getref((IUnknown *) dd4) == ref4 + 0, "IDirectDraw4 refcount was increased by %d\n", getref((IUnknown *) dd4) - ref4);
    ok(getref((IUnknown *) dd7) == ref7 + 1, "IDirectDraw7 refcount was increased by %d\n", getref((IUnknown *) dd7) - ref7);

    ok(dd == dd7, "Returned interface pointer is not equal to the creation interface\n");
    IUnknown_Release((IUnknown *) dd);

    /* Now test what happens if we QI the surface for some other version - It should still return the creation interface */
    ret = IDirectDrawSurface4_GetDDInterface(dsurface4, &dd);
    ok(ret == DD_OK, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);
    ok(getref((IUnknown *) lpDD) == ref1 + 0, "IDirectDraw refcount was increased by %d\n", getref((IUnknown *) lpDD) - ref1);
    ok(getref((IUnknown *) dd2) == ref2 + 0, "IDirectDraw2 refcount was increased by %d\n", getref((IUnknown *) dd2) - ref2);
    ok(getref((IUnknown *) dd4) == ref4 + 0, "IDirectDraw4 refcount was increased by %d\n", getref((IUnknown *) dd4) - ref4);
    ok(getref((IUnknown *) dd7) == ref7 + 1, "IDirectDraw7 refcount was increased by %d\n", getref((IUnknown *) dd7) - ref7);

    ok(dd == dd7, "Returned interface pointer is not equal to the creation interface\n");
    IUnknown_Release((IUnknown *) dd);

    IDirectDraw_Release(dd2);
    IDirectDraw_Release(dd4);
    IDirectDraw_Release(dd7);
    IDirectDrawSurface4_Release(dsurface4);
    IDirectDrawSurface7_Release(dsurface7);
}

static ULONG getRefcount(IUnknown *iface)
{
    IUnknown_AddRef(iface);
    return IUnknown_Release(iface);
}

#define MAXEXPECTED 8  /* Can match up to 8 expected surfaces */
struct enumstruct
{
    IDirectDrawSurface *expected[MAXEXPECTED];
    UINT count;
};

static HRESULT WINAPI enumCB(IDirectDrawSurface *surf, DDSURFACEDESC *desc, void *ctx)
{
    int i;
    BOOL found = FALSE;

    for(i = 0; i < MAXEXPECTED; i++)
    {
        if(((struct enumstruct *)ctx)->expected[i] == surf) found = TRUE;
    }

    ok(found, "Unexpected surface %p enumerated\n", surf);
    ((struct enumstruct *)ctx)->count++;
    IDirectDrawSurface_Release(surf);
    return DDENUMRET_OK;
}

static void EnumTest(void)
{
    HRESULT rc;
    DDSURFACEDESC ddsd;
    IDirectDrawSurface *surface;
    struct enumstruct ctx;

    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_MIPMAPCOUNT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    U2(ddsd).dwMipMapCount = 3;
    ddsd.dwWidth = 32;
    ddsd.dwHeight = 32;
    rc = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface, NULL);
    ok(rc==DD_OK,"CreateSurface returned: %x\n",rc);

    memset(&ctx, 0, sizeof(ctx));
    ctx.expected[0] = surface;
    rc = IDirectDrawSurface_GetAttachedSurface(ctx.expected[0], &ddsd.ddsCaps, &ctx.expected[1]);
    ok(rc == DD_OK, "GetAttachedSurface returned %08x\n", rc);
    rc = IDirectDrawSurface_GetAttachedSurface(ctx.expected[1], &ddsd.ddsCaps, &ctx.expected[2]);
    ok(rc == DD_OK, "GetAttachedSurface returned %08x\n", rc);
    rc = IDirectDrawSurface_GetAttachedSurface(ctx.expected[2], &ddsd.ddsCaps, &ctx.expected[3]);
    ok(rc == DDERR_NOTFOUND, "GetAttachedSurface returned %08x\n", rc);
    ok(!ctx.expected[3], "expected NULL pointer\n");
    ctx.count = 0;

    rc = IDirectDraw_EnumSurfaces(lpDD, DDENUMSURFACES_DOESEXIST | DDENUMSURFACES_ALL, &ddsd, &ctx, enumCB);
    ok(rc == DD_OK, "IDirectDraw_EnumSurfaces returned %08x\n", rc);
    ok(ctx.count == 3, "%d surfaces enumerated, expected 3\n", ctx.count);

    ctx.count = 0;
    rc = IDirectDraw_EnumSurfaces(lpDD, DDENUMSURFACES_DOESEXIST | DDENUMSURFACES_ALL, NULL, &ctx, enumCB);
    ok(rc == DD_OK, "IDirectDraw_EnumSurfaces returned %08x\n", rc);
    ok(ctx.count == 3, "%d surfaces enumerated, expected 3\n", ctx.count);

    IDirectDrawSurface_Release(ctx.expected[2]);
    IDirectDrawSurface_Release(ctx.expected[1]);
    IDirectDrawSurface_Release(surface);
}

struct compare
{
    DWORD width, height;
    DWORD caps, caps2;
    UINT mips;
};

static HRESULT WINAPI CubeTestPaletteEnum(IDirectDrawSurface7 *surface, DDSURFACEDESC2 *desc, void *context)
{
    HRESULT hr;

    hr = IDirectDrawSurface7_SetPalette(surface, context);
    if (desc->dwWidth == 64) /* This is for first mimpmap */
        ok(hr == DDERR_NOTONMIPMAPSUBLEVEL, "SetPalette returned: %x\n",hr);
    else
        ok(hr == DD_OK, "SetPalette returned: %x\n",hr);

    IDirectDrawSurface7_Release(surface);

    return DDENUMRET_OK;
}

static HRESULT WINAPI CubeTestLvl2Enum(IDirectDrawSurface7 *surface, DDSURFACEDESC2 *desc, void *context)
{
    UINT *mips = context;

    (*mips)++;
    IDirectDrawSurface7_EnumAttachedSurfaces(surface,
                                             context,
                                             CubeTestLvl2Enum);

    return DDENUMRET_OK;
}

static HRESULT WINAPI CubeTestLvl1Enum(IDirectDrawSurface7 *surface, DDSURFACEDESC2 *desc, void *context)
{
    UINT mips = 0;
    UINT *num = context;
    static const struct compare expected[] =
    {
        {
            128, 128,
            DDSCAPS_MIPMAP | DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_COMPLEX,
            DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ,
            7
        },
        {
            128, 128,
            DDSCAPS_MIPMAP | DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_COMPLEX,
            DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ,
            7
        },
        {
            128, 128,
            DDSCAPS_MIPMAP | DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_COMPLEX,
            DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY,
            7
        },
        {
            128, 128,
            DDSCAPS_MIPMAP | DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_COMPLEX,
            DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY,
            7
        },
        {
            128, 128,
            DDSCAPS_MIPMAP | DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_COMPLEX,
            DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX,
            7
        },
        {
            64, 64, /* This is the first mipmap */
            DDSCAPS_MIPMAP | DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_COMPLEX,
            DDSCAPS2_MIPMAPSUBLEVEL | DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX,
            6
        },
    };

    mips = 0;
    IDirectDrawSurface7_EnumAttachedSurfaces(surface,
                                             &mips,
                                             CubeTestLvl2Enum);

    ok(desc->dwWidth == expected[*num].width, "Surface width is %d expected %d\n", desc->dwWidth, expected[*num].width);
    ok(desc->dwHeight == expected[*num].height, "Surface height is %d expected %d\n", desc->dwHeight, expected[*num].height);
    ok(desc->ddsCaps.dwCaps == expected[*num].caps, "Surface caps are %08x expected %08x\n", desc->ddsCaps.dwCaps, expected[*num].caps);
    ok(desc->ddsCaps.dwCaps2 == expected[*num].caps2, "Surface caps2 are %08x expected %08x\n", desc->ddsCaps.dwCaps2, expected[*num].caps2);
    ok(mips == expected[*num].mips, "Surface has %d mipmaps, expected %d\n", mips, expected[*num].mips);

    (*num)++;

    IDirectDrawSurface7_Release(surface);

    return DDENUMRET_OK;
}

static void CubeMapTest(void)
{
    IDirectDraw7 *dd7 = NULL;
    IDirectDrawSurface7 *cubemap = NULL;
    IDirectDrawPalette *palette = NULL;
    DDSURFACEDESC2 ddsd;
    HRESULT hr;
    PALETTEENTRY Table[256];
    int i;
    UINT num = 0;
    UINT ref;
    struct enumstruct ctx;

    for(i=0; i<256; i++)
    {
        Table[i].peRed   = 0xff;
        Table[i].peGreen = 0;
        Table[i].peBlue  = 0;
        Table[i].peFlags = 0;
    }

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    ok(hr == DD_OK, "IDirectDraw::QueryInterface returned %08x\n", hr);
    if (FAILED(hr)) goto err;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP | DDSCAPS_SYSTEMMEMORY;
    ddsd.ddsCaps.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALLFACES;

    /* D3DFMT_R5G6B5 */
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 16;
    U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0xF800;
    U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x07E0;
    U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x001F;

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &cubemap, NULL);
    if (FAILED(hr))
    {
        skip("Can't create cubemap surface\n");
        goto err;
    }

    hr = IDirectDrawSurface7_GetSurfaceDesc(cubemap, &ddsd);
    ok(hr == DD_OK, "IDirectDrawSurface7_GetSurfaceDesc returned %08x\n", hr);
    ok(ddsd.ddsCaps.dwCaps == (DDSCAPS_MIPMAP | DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_COMPLEX),
       "Root Caps are %08x\n", ddsd.ddsCaps.dwCaps);
    ok(ddsd.ddsCaps.dwCaps2 == (DDSCAPS2_CUBEMAP_POSITIVEX | DDSCAPS2_CUBEMAP),
       "Root Caps2 are %08x\n", ddsd.ddsCaps.dwCaps2);

    IDirectDrawSurface7_EnumAttachedSurfaces(cubemap,
                                             &num,
                                             CubeTestLvl1Enum);
    ok(num == 6, "Surface has %d attachments\n", num);
    IDirectDrawSurface7_Release(cubemap);

    /* What happens if I do not specify any faces? */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP | DDSCAPS_SYSTEMMEMORY;
    ddsd.ddsCaps.dwCaps2 = DDSCAPS2_CUBEMAP;

    /* D3DFMT_R5G6B5 */
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 16;
    U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0xF800;
    U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x07E0;
    U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x001F;

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &cubemap, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw7::CreateSurface asking for a cube map without faces returned %08x\n", hr);

    /* Cube map faces without a cube map? */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP | DDSCAPS_SYSTEMMEMORY;
    ddsd.ddsCaps.dwCaps2 = DDSCAPS2_CUBEMAP_ALLFACES;

    /* D3DFMT_R5G6B5 */
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 16;
    U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0xF800;
    U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x07E0;
    U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x001F;

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &cubemap, NULL);
    ok(hr == DDERR_INVALIDCAPS, "IDirectDraw7::CreateSurface returned %08x\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP | DDSCAPS_SYSTEMMEMORY;
    ddsd.ddsCaps.dwCaps2 = DDSCAPS2_CUBEMAP_POSITIVEX;

    /* D3DFMT_R5G6B5 */
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 16;
    U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0xF800;
    U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x07E0;
    U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x001F;

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &cubemap, NULL);
    ok(hr == DDERR_INVALIDCAPS, "IDirectDraw7::CreateSurface returned %08x\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP | DDSCAPS_SYSTEMMEMORY;
    ddsd.ddsCaps.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALLFACES;

    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
    U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 8;

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &cubemap, NULL);
    if (FAILED(hr))
    {
        skip("Can't create palletized cubemap surface\n");
        goto err;
    }

    hr = IDirectDraw7_CreatePalette(dd7, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, Table, &palette, NULL);
    ok(hr == DD_OK, "CreatePalette failed with %08x\n", hr);

    hr = IDirectDrawSurface7_EnumAttachedSurfaces(cubemap, palette, CubeTestPaletteEnum);
    ok(hr == DD_OK, "EnumAttachedSurfaces failed\n");

    ref = getRefcount((IUnknown *) palette);
    ok(ref == 6, "Refcount is %u, expected 1\n", ref);

    IDirectDrawSurface7_Release(cubemap);

    ref = getRefcount((IUnknown *) palette);
    ok(ref == 1, "Refcount is %u, expected 1\n", ref);

    IDirectDrawPalette_Release(palette);

    /* Make sure everything is cleaned up properly. Use the enumSurfaces test infrastructure */
    memset(&ctx, 0, sizeof(ctx));
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    hr = IDirectDraw_EnumSurfaces(lpDD, DDENUMSURFACES_DOESEXIST | DDENUMSURFACES_ALL, (DDSURFACEDESC *) &ddsd, (void *) &ctx, enumCB);
    ok(hr == DD_OK, "IDirectDraw_EnumSurfaces returned %08x\n", hr);
    ok(ctx.count == 0, "%d surfaces enumerated, expected 0\n", ctx.count);

    err:
    if (dd7) IDirectDraw7_Release(dd7);
}

static void test_lockrect_invalid(void)
{
    unsigned int i, j;

    RECT valid[] = {
        {60, 60, 68, 68},
        {60, 60, 60, 68},
        {60, 60, 68, 60},
        {120, 60, 128, 68},
        {60, 120, 68, 128},
    };

    RECT invalid[] = {
        {68, 60, 60, 68},       /* left > right */
        {60, 68, 68, 60},       /* top > bottom */
        {-8, 60,  0, 68},       /* left < surface */
        {60, -8, 68,  0},       /* top < surface */
        {-16, 60, -8, 68},      /* right < surface */
        {60, -16, 68, -8},      /* bottom < surface */
        {60, 60, 136, 68},      /* right > surface */
        {60, 60, 68, 136},      /* bottom > surface */
        {136, 60, 144, 68},     /* left > surface */
        {60, 136, 68, 144},     /* top > surface */
    };

    const DWORD dds_caps[] = {
        DDSCAPS_OFFSCREENPLAIN
    };

    for (j = 0; j < (sizeof(dds_caps) / sizeof(*dds_caps)); ++j)
    {
        IDirectDrawSurface *surface = 0;
        DDSURFACEDESC surface_desc = {0};
        DDSURFACEDESC locked_desc = {0};
        HRESULT hr;

        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.ddpfPixelFormat.dwSize = sizeof(surface_desc.ddpfPixelFormat);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
        surface_desc.ddsCaps.dwCaps = dds_caps[j];
        surface_desc.dwWidth = 128;
        surface_desc.dwHeight = 128;
        surface_desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
        U1(surface_desc.ddpfPixelFormat).dwRGBBitCount = 32;
        U2(surface_desc.ddpfPixelFormat).dwRBitMask = 0xFF0000;
        U3(surface_desc.ddpfPixelFormat).dwGBitMask = 0x00FF00;
        U4(surface_desc.ddpfPixelFormat).dwBBitMask = 0x0000FF;

        hr = IDirectDraw_CreateSurface(lpDD, &surface_desc, &surface, NULL);
        ok(SUCCEEDED(hr), "CreateSurface failed (0x%08x)\n", hr);
        if (FAILED(hr))
        {
            skip("failed to create surface\n");
            continue;
        }

        hr = IDirectDrawSurface_Lock(surface, NULL, NULL, DDLOCK_WAIT, NULL);
        ok(hr == DDERR_INVALIDPARAMS, "Lock returned 0x%08x for NULL DDSURFACEDESC,"
                " expected DDERR_INVALIDPARAMS (0x%08x)\n", hr, DDERR_INVALIDPARAMS);

        for (i = 0; i < (sizeof(valid) / sizeof(*valid)); ++i)
        {
            RECT *rect = &valid[i];

            memset(&locked_desc, 0, sizeof(locked_desc));
            locked_desc.dwSize = sizeof(locked_desc);

            hr = IDirectDrawSurface_Lock(surface, rect, &locked_desc, DDLOCK_WAIT, NULL);
            ok(SUCCEEDED(hr), "Lock failed (0x%08x) for rect [%d, %d]->[%d, %d]\n",
                    hr, rect->left, rect->top, rect->right, rect->bottom);

            hr = IDirectDrawSurface_Unlock(surface, NULL);
            ok(SUCCEEDED(hr), "Unlock failed (0x%08x)\n", hr);
        }

        for (i = 0; i < (sizeof(invalid) / sizeof(*invalid)); ++i)
        {
            RECT *rect = &invalid[i];

            memset(&locked_desc, 1, sizeof(locked_desc));
            locked_desc.dwSize = sizeof(locked_desc);

            hr = IDirectDrawSurface_Lock(surface, rect, &locked_desc, DDLOCK_WAIT, NULL);
            ok(hr == DDERR_INVALIDPARAMS, "Lock returned 0x%08x for rect [%d, %d]->[%d, %d]"
                    ", expected DDERR_INVALIDPARAMS (0x%08x)\n", hr, rect->left, rect->top,
                    rect->right, rect->bottom, DDERR_INVALIDPARAMS);
            ok(!locked_desc.lpSurface, "IDirectDrawSurface_Lock did not set lpSurface in the surface desc to zero.\n");
        }

        hr = IDirectDrawSurface_Lock(surface, NULL, &locked_desc, DDLOCK_WAIT, NULL);
        ok(hr == DD_OK, "IDirectDrawSurface_Lock(rect = NULL) failed (0x%08x)\n", hr);
        hr = IDirectDrawSurface_Lock(surface, NULL, &locked_desc, DDLOCK_WAIT, NULL);
        ok(hr == DDERR_SURFACEBUSY, "Double lock(rect = NULL) returned 0x%08x\n", hr);
        if(SUCCEEDED(hr)) {
            hr = IDirectDrawSurface_Unlock(surface, NULL);
            ok(SUCCEEDED(hr), "Unlock failed (0x%08x)\n", hr);
        }
        hr = IDirectDrawSurface_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "Unlock failed (0x%08x)\n", hr);

        memset(&locked_desc, 0, sizeof(locked_desc));
        locked_desc.dwSize = sizeof(locked_desc);
        hr = IDirectDrawSurface_Lock(surface, &valid[0], &locked_desc, DDLOCK_WAIT, NULL);
        ok(hr == DD_OK, "IDirectDrawSurface_Lock(rect = [%d, %d]->[%d, %d]) failed (0x%08x)\n",
           valid[0].left, valid[0].top, valid[0].right, valid[0].bottom, hr);
        hr = IDirectDrawSurface_Lock(surface, &valid[0], &locked_desc, DDLOCK_WAIT, NULL);
        ok(hr == DDERR_SURFACEBUSY, "Double lock(rect = [%d, %d]->[%d, %d]) failed (0x%08x)\n",
           valid[0].left, valid[0].top, valid[0].right, valid[0].bottom, hr);

        /* Locking a different rectangle returns DD_OK, but it seems to break the surface.
         * Afterwards unlocking the surface fails(NULL rectangle, and both locked rectangles
         */

        hr = IDirectDrawSurface_Unlock(surface, NULL);
        ok(hr == DD_OK, "Unlock returned (0x%08x)\n", hr);

        IDirectDrawSurface_Release(surface);
    }
}

static void CompressedTest(void)
{
    HRESULT hr;
    IDirectDrawSurface7 *surface;
    DDSURFACEDESC2 ddsd, ddsd2;
    IDirectDraw7 *dd7 = NULL;
    RECT r = { 0, 0, 128, 128 };
    RECT r2 = { 32, 32, 64, 64 };

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    ok(hr == DD_OK, "IDirectDraw::QueryInterface returned %08x\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_FOURCC;
    U4(ddsd).ddpfPixelFormat.dwFourCC = MAKEFOURCC('D','X','T','1');

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface, NULL);
    ok(hr == DD_OK, "CreateSurface returned %08x\n", hr);
    if (FAILED(hr))
    {
        skip("failed to create surface\n");
        return;
    }

    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);
    hr = IDirectDrawSurface7_GetSurfaceDesc(surface, &ddsd2);
    ok(hr == DD_OK, "GetSurfaceDesc returned %08x\n", hr);

    ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
       "Surface desc flags: %08x\n", ddsd2.dwFlags);
    ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
    ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
    ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY),
       "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
    ok(U1(ddsd2).dwLinearSize == 8192, "Linear size is %d\n", U1(ddsd2).dwLinearSize);
    ok(ddsd2.ddsCaps.dwCaps2 == 0, "Caps2: %08x\n", ddsd2.ddsCaps.dwCaps2);
    IDirectDrawSurface7_Release(surface);

    U4(ddsd).ddpfPixelFormat.dwFourCC = MAKEFOURCC('D','X','T','3');
    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface, NULL);
    ok(hr == DD_OK, "CreateSurface returned %08x\n", hr);
    if (FAILED(hr))
    {
        skip("failed to create surface\n");
        return;
    }

    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);
    hr = IDirectDrawSurface7_GetSurfaceDesc(surface, &ddsd2);
    ok(hr == DD_OK, "GetSurfaceDesc returned %08x\n", hr);

    ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
       "Surface desc flags: %08x\n", ddsd2.dwFlags);
    ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
    ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
    ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY),
       "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
    ok(U1(ddsd2).dwLinearSize == 16384, "Linear size is %d\n", U1(ddsd2).dwLinearSize);
    IDirectDrawSurface7_Release(surface);

    U4(ddsd).ddpfPixelFormat.dwFourCC = MAKEFOURCC('D','X','T','5');
    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface, NULL);
    ok(hr == DD_OK, "CreateSurface returned %08x\n", hr);
    if (FAILED(hr))
    {
        skip("failed to create surface\n");
        return;
    }

    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);
    hr = IDirectDrawSurface7_GetSurfaceDesc(surface, &ddsd2);
    ok(hr == DD_OK, "GetSurfaceDesc returned %08x\n", hr);

    ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
       "Surface desc flags: %08x\n", ddsd2.dwFlags);
    ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
    ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
    ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY),
       "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
    ok(U1(ddsd2).dwLinearSize == 16384, "Linear size is %d\n", U1(ddsd2).dwLinearSize);
    ok(ddsd2.lpSurface == 0, "Surface memory is at %p, expected NULL\n", ddsd2.lpSurface);

    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);

    /* Show that the description is not changed when locking the surface. What is really interesting
     * about this is that DDSD_LPSURFACE isn't set.
     */
    hr = IDirectDrawSurface7_Lock(surface, NULL, &ddsd2, DDLOCK_READONLY, 0);
    ok(hr == DD_OK, "Lock returned %08x\n", hr);

    ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
       "Surface desc flags: %08x\n", ddsd2.dwFlags);
    ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
    ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
    ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY),
       "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
    ok(U1(ddsd2).dwLinearSize == 16384, "Linear size is %d\n", U1(ddsd2).dwLinearSize);
    ok(ddsd2.lpSurface != 0, "Surface memory is at NULL\n");

    hr = IDirectDrawSurface7_Unlock(surface, NULL);
    ok(hr == DD_OK, "Unlock returned %08x\n", hr);

    /* Now what about a locking rect?  */
    hr = IDirectDrawSurface7_Lock(surface, &r, &ddsd2, DDLOCK_READONLY, 0);
    ok(hr == DD_OK, "Lock returned %08x\n", hr);

    ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
       "Surface desc flags: %08x\n", ddsd2.dwFlags);
    ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
    ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
    ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY),
       "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
    ok(U1(ddsd2).dwLinearSize == 16384, "Linear size is %d\n", U1(ddsd2).dwLinearSize);
    ok(ddsd2.lpSurface != 0, "Surface memory is at NULL\n");

    hr = IDirectDrawSurface7_Unlock(surface, &r);
    ok(hr == DD_OK, "Unlock returned %08x\n", hr);

    /* Now what about a different locking offset? */
    hr = IDirectDrawSurface7_Lock(surface, &r2, &ddsd2, DDLOCK_READONLY, 0);
    ok(hr == DD_OK, "Lock returned %08x\n", hr);

    ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
       "Surface desc flags: %08x\n", ddsd2.dwFlags);
    ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
    ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
    ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY),
       "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
    ok(U1(ddsd2).dwLinearSize == 16384, "Linear size is %d\n", U1(ddsd2).dwLinearSize);
    ok(ddsd2.lpSurface != 0, "Surface memory is at NULL\n");

    hr = IDirectDrawSurface7_Unlock(surface, &r2);
    ok(hr == DD_OK, "Unlock returned %08x\n", hr);
    IDirectDrawSurface7_Release(surface);

    /* Try this with video memory. A kind of surprise. It still has the LINEARSIZE flag set,
     * but seems to have a pitch instead.
     */
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY;
    U4(ddsd).ddpfPixelFormat.dwFourCC = MAKEFOURCC('D','X','T','1');

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface, NULL);
    ok(hr == DD_OK || hr == DDERR_NOTEXTUREHW || hr == DDERR_INVALIDPARAMS ||
       broken(hr == DDERR_NODIRECTDRAWHW), "CreateSurface returned %08x\n", hr);

    /* Not supported everywhere */
    if(SUCCEEDED(hr))
    {
        memset(&ddsd2, 0, sizeof(ddsd2));
        ddsd2.dwSize = sizeof(ddsd2);
        U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);
        hr = IDirectDrawSurface7_GetSurfaceDesc(surface, &ddsd2);
        ok(hr == DD_OK, "GetSurfaceDesc returned %08x\n", hr);

        ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
        "Surface desc flags: %08x\n", ddsd2.dwFlags);
        ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
        ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
        ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM),
        "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
        /* ATI drivers report a broken linear size, thus no need to clone the exact behaviour. nvidia reports the correct size */
        ok(ddsd2.ddsCaps.dwCaps2 == 0, "Caps2: %08x\n", ddsd2.ddsCaps.dwCaps2);
        IDirectDrawSurface7_Release(surface);

        U4(ddsd).ddpfPixelFormat.dwFourCC = MAKEFOURCC('D','X','T','3');
        hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface, NULL);
        ok(hr == DD_OK, "CreateSurface returned %08x\n", hr);

        memset(&ddsd2, 0, sizeof(ddsd2));
        ddsd2.dwSize = sizeof(ddsd2);
        U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);
        hr = IDirectDrawSurface7_GetSurfaceDesc(surface, &ddsd2);
        ok(hr == DD_OK, "GetSurfaceDesc returned %08x\n", hr);

        ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
        "Surface desc flags: %08x\n", ddsd2.dwFlags);
        ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
        ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
        ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM),
        "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
        /* ATI drivers report a broken linear size, thus no need to clone the exact behaviour. nvidia reports the correct size */
        IDirectDrawSurface7_Release(surface);

        U4(ddsd).ddpfPixelFormat.dwFourCC = MAKEFOURCC('D','X','T','5');
        hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface, NULL);
        ok(hr == DD_OK, "CreateSurface returned %08x\n", hr);

        memset(&ddsd2, 0, sizeof(ddsd2));
        ddsd2.dwSize = sizeof(ddsd2);
        U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);
        hr = IDirectDrawSurface7_GetSurfaceDesc(surface, &ddsd2);
        ok(hr == DD_OK, "GetSurfaceDesc returned %08x\n", hr);

        ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
        "Surface desc flags: %08x\n", ddsd2.dwFlags);
        ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
        ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
        ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM),
        "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
        /* ATI drivers report a broken linear size, thus no need to clone the exact behaviour. nvidia reports the correct size */
        ok(ddsd2.lpSurface == 0, "Surface memory is at %p, expected NULL\n", ddsd2.lpSurface);

        memset(&ddsd2, 0, sizeof(ddsd2));
        ddsd2.dwSize = sizeof(ddsd2);
        U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);

        /* Show that the description is not changed when locking the surface. What is really interesting
        * about this is that DDSD_LPSURFACE isn't set.
        */
        hr = IDirectDrawSurface7_Lock(surface, NULL, &ddsd2, DDLOCK_READONLY, 0);
        ok(hr == DD_OK, "Lock returned %08x\n", hr);

        ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
        "Surface desc flags: %08x\n", ddsd2.dwFlags);
        ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
        ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
        ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM),
        "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
        /* ATI drivers report a broken linear size, thus no need to clone the exact behaviour. nvidia reports the correct size */
        ok(ddsd2.lpSurface != 0, "Surface memory is at NULL\n");

        hr = IDirectDrawSurface7_Unlock(surface, NULL);
        ok(hr == DD_OK, "Unlock returned %08x\n", hr);

        /* Now what about a locking rect?  */
        hr = IDirectDrawSurface7_Lock(surface, &r, &ddsd2, DDLOCK_READONLY, 0);
        ok(hr == DD_OK, "Lock returned %08x\n", hr);

        ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
        "Surface desc flags: %08x\n", ddsd2.dwFlags);
        ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
        ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
        ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM),
        "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
        /* ATI drivers report a broken linear size, thus no need to clone the exact behaviour. nvidia reports the correct size */
        ok(ddsd2.lpSurface != 0, "Surface memory is at NULL\n");

        hr = IDirectDrawSurface7_Unlock(surface, &r);
        ok(hr == DD_OK, "Unlock returned %08x\n", hr);

        /* Now what about a different locking offset? */
        hr = IDirectDrawSurface7_Lock(surface, &r2, &ddsd2, DDLOCK_READONLY, 0);
        ok(hr == DD_OK, "Lock returned %08x\n", hr);

        ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
        "Surface desc flags: %08x\n", ddsd2.dwFlags);
        ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
        ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
        ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM),
        "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
        /* ATI drivers report a broken linear size, thus no need to clone the exact behaviour. nvidia reports the correct size */
        ok(ddsd2.lpSurface != 0, "Surface memory is at NULL\n");

        hr = IDirectDrawSurface7_Unlock(surface, &r2);
        ok(hr == DD_OK, "Unlock returned %08x\n", hr);

        IDirectDrawSurface7_Release(surface);
    }
    else
    {
        skip("Hardware DXTN textures not supported\n");
    }

    /* What happens to managed textures? Interestingly, Windows reports them as being in system
     * memory. The linear size fits again.
     */
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
    U4(ddsd).ddpfPixelFormat.dwFourCC = MAKEFOURCC('D','X','T','1');

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface, NULL);
    ok(hr == DD_OK || hr == DDERR_NOTEXTUREHW, "CreateSurface returned %08x\n", hr);

    /* Not supported everywhere */
    if(SUCCEEDED(hr))
    {
        memset(&ddsd2, 0, sizeof(ddsd2));
        ddsd2.dwSize = sizeof(ddsd2);
        U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);
        hr = IDirectDrawSurface7_GetSurfaceDesc(surface, &ddsd2);
        ok(hr == DD_OK, "GetSurfaceDesc returned %08x\n", hr);

        ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
        "Surface desc flags: %08x\n", ddsd2.dwFlags);
        ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
        ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
        ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY),
        "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
        ok(U1(ddsd2).dwLinearSize == 8192, "Linear size is %d\n", U1(ddsd2).dwLinearSize);
        ok(ddsd2.ddsCaps.dwCaps2 == DDSCAPS2_TEXTUREMANAGE, "Caps2: %08x\n", ddsd2.ddsCaps.dwCaps2);
        IDirectDrawSurface7_Release(surface);

        U4(ddsd).ddpfPixelFormat.dwFourCC = MAKEFOURCC('D','X','T','3');
        hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface, NULL);
        ok(hr == DD_OK, "CreateSurface returned %08x\n", hr);

        memset(&ddsd2, 0, sizeof(ddsd2));
        ddsd2.dwSize = sizeof(ddsd2);
        U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);
        hr = IDirectDrawSurface7_GetSurfaceDesc(surface, &ddsd2);
        ok(hr == DD_OK, "GetSurfaceDesc returned %08x\n", hr);

        ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
        "Surface desc flags: %08x\n", ddsd2.dwFlags);
        ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
        ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
        ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY),
        "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
        ok(U1(ddsd2).dwLinearSize == 16384, "Linear size is %d\n", U1(ddsd2).dwLinearSize);
        IDirectDrawSurface7_Release(surface);

        U4(ddsd).ddpfPixelFormat.dwFourCC = MAKEFOURCC('D','X','T','5');
        hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface, NULL);
        ok(hr == DD_OK, "CreateSurface returned %08x\n", hr);

        memset(&ddsd2, 0, sizeof(ddsd2));
        ddsd2.dwSize = sizeof(ddsd2);
        U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);
        hr = IDirectDrawSurface7_GetSurfaceDesc(surface, &ddsd2);
        ok(hr == DD_OK, "GetSurfaceDesc returned %08x\n", hr);

        ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
        "Surface desc flags: %08x\n", ddsd2.dwFlags);
        ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
        ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
        ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY),
        "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
        ok(U1(ddsd2).dwLinearSize == 16384, "Linear size is %d\n", U1(ddsd2).dwLinearSize);
        ok(ddsd2.lpSurface == 0, "Surface memory is at %p, expected NULL\n", ddsd2.lpSurface);

        memset(&ddsd2, 0, sizeof(ddsd2));
        ddsd2.dwSize = sizeof(ddsd2);
        U4(ddsd2).ddpfPixelFormat.dwSize = sizeof(U4(ddsd2).ddpfPixelFormat);

        /* Show that the description is not changed when locking the surface. What is really interesting
        * about this is that DDSD_LPSURFACE isn't set.
        */
        hr = IDirectDrawSurface7_Lock(surface, NULL, &ddsd2, DDLOCK_READONLY, 0);
        ok(hr == DD_OK, "Lock returned %08x\n", hr);

        ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
        "Surface desc flags: %08x\n", ddsd2.dwFlags);
        ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
        ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
        ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY),
        "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
        ok(U1(ddsd2).dwLinearSize == 16384, "Linear size is %d\n", U1(ddsd2).dwLinearSize);
        ok(ddsd2.lpSurface != 0, "Surface memory is at NULL\n");

        hr = IDirectDrawSurface7_Unlock(surface, NULL);
        ok(hr == DD_OK, "Unlock returned %08x\n", hr);

        /* Now what about a locking rect?  */
        hr = IDirectDrawSurface7_Lock(surface, &r, &ddsd2, DDLOCK_READONLY, 0);
        ok(hr == DD_OK, "Lock returned %08x\n", hr);

        ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
        "Surface desc flags: %08x\n", ddsd2.dwFlags);
        ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
        ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
        ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY),
        "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
        ok(U1(ddsd2).dwLinearSize == 16384, "\"Linear\" size is %d\n", U1(ddsd2).dwLinearSize);
        ok(ddsd2.lpSurface != 0, "Surface memory is at NULL\n");

        hr = IDirectDrawSurface7_Unlock(surface, &r);
        ok(hr == DD_OK, "Unlock returned %08x\n", hr);

        /* Now what about a different locking offset? */
        hr = IDirectDrawSurface7_Lock(surface, &r2, &ddsd2, DDLOCK_READONLY, 0);
        ok(hr == DD_OK, "Lock returned %08x\n", hr);

        ok(ddsd2.dwFlags == (DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_LINEARSIZE),
        "Surface desc flags: %08x\n", ddsd2.dwFlags);
        ok(U4(ddsd2).ddpfPixelFormat.dwFlags == DDPF_FOURCC, "Pixel format flags: %08x\n", U4(ddsd2).ddpfPixelFormat.dwFlags);
        ok(U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount == 0, "RGB bitcount: %08x\n", U1(U4(ddsd2).ddpfPixelFormat).dwRGBBitCount);
        ok(ddsd2.ddsCaps.dwCaps == (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY),
        "Surface caps flags: %08x\n", ddsd2.ddsCaps.dwCaps);
        ok(U1(ddsd2).dwLinearSize == 16384, "\"Linear\" size is %d\n", U1(ddsd2).dwLinearSize);
        ok(ddsd2.lpSurface != 0, "Surface memory is at NULL\n");

        hr = IDirectDrawSurface7_Unlock(surface, &r2);
        ok(hr == DD_OK, "Unlock returned %08x\n", hr);

        IDirectDrawSurface7_Release(surface);
    }
    else
    {
        skip("Hardware DXTN textures not supported\n");
    }

    IDirectDraw7_Release(dd7);
}

static void SizeTest(void)
{
    IDirectDrawSurface *dsurface = NULL;
    DDSURFACEDESC desc;
    HRESULT ret;
    HWND window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            100, 100, 160, 160, NULL, NULL, NULL, NULL);

    /* Create an offscreen surface surface without a size */
    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS;
    desc.ddsCaps.dwCaps |= DDSCAPS_OFFSCREENPLAIN;
    ret = IDirectDraw_CreateSurface(lpDD, &desc, &dsurface, NULL);
    ok(ret == DDERR_INVALIDPARAMS, "Creating an offscreen plain surface without a size info returned %08x (dsurface=%p)\n", ret, dsurface);
    if(dsurface)
    {
        trace("Surface at %p\n", dsurface);
        IDirectDrawSurface_Release(dsurface);
        dsurface = NULL;
    }

    /* Create an offscreen surface surface with only a width parameter */
    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH;
    desc.ddsCaps.dwCaps |= DDSCAPS_OFFSCREENPLAIN;
    desc.dwWidth = 128;
    ret = IDirectDraw_CreateSurface(lpDD, &desc, &dsurface, NULL);
    ok(ret == DDERR_INVALIDPARAMS, "Creating an offscreen plain surface without height info returned %08x\n", ret);
    if(dsurface)
    {
        IDirectDrawSurface_Release(dsurface);
        dsurface = NULL;
    }

    /* Create an offscreen surface surface with only a height parameter */
    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps |= DDSCAPS_OFFSCREENPLAIN;
    desc.dwHeight = 128;
    ret = IDirectDraw_CreateSurface(lpDD, &desc, &dsurface, NULL);
    ok(ret == DDERR_INVALIDPARAMS, "Creating an offscreen plain surface without width info returned %08x\n", ret);
    if(dsurface)
    {
        IDirectDrawSurface_Release(dsurface);
        dsurface = NULL;
    }

    /* Test 0 height. */
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    desc.dwWidth = 1;
    desc.dwHeight = 0;
    ret = IDirectDraw_CreateSurface(lpDD, &desc, &dsurface, NULL);
    ok(ret == DDERR_INVALIDPARAMS, "Creating a 0 height surface returned %#x, expected DDERR_INVALIDPARAMS.\n", ret);
    if (SUCCEEDED(ret)) IDirectDrawSurface_Release(dsurface);
    dsurface = NULL;

    /* Test 0 width. */
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    desc.dwWidth = 0;
    desc.dwHeight = 1;
    ret = IDirectDraw_CreateSurface(lpDD, &desc, &dsurface, NULL);
    ok(ret == DDERR_INVALIDPARAMS, "Creating a 0 width surface returned %#x, expected DDERR_INVALIDPARAMS.\n", ret);
    if (SUCCEEDED(ret)) IDirectDrawSurface_Release(dsurface);
    dsurface = NULL;

    /* Sanity check */
    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    desc.ddsCaps.dwCaps |= DDSCAPS_OFFSCREENPLAIN;
    desc.dwHeight = 128;
    desc.dwWidth = 128;
    ret = IDirectDraw_CreateSurface(lpDD, &desc, &dsurface, NULL);
    ok(ret == DD_OK, "Creating an offscreen plain surface with width and height info returned %08x\n", ret);
    if(dsurface)
    {
        IDirectDrawSurface_Release(dsurface);
        dsurface = NULL;
    }

    /* Test a primary surface size */
    ret = IDirectDraw_SetCooperativeLevel(lpDD, window, DDSCL_NORMAL);
    ok(ret == DD_OK, "SetCooperativeLevel failed with %08x\n", ret);

    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS;
    desc.ddsCaps.dwCaps |= DDSCAPS_PRIMARYSURFACE;
    desc.dwHeight = 128; /* Keep them set to  check what happens */
    desc.dwWidth = 128; /* Keep them set to  check what happens */
    ret = IDirectDraw_CreateSurface(lpDD, &desc, &dsurface, NULL);
    ok(ret == DD_OK, "Creating a primary surface without width and height info returned %08x\n", ret);
    if(dsurface)
    {
        ret = IDirectDrawSurface_GetSurfaceDesc(dsurface, &desc);
        ok(ret == DD_OK, "GetSurfaceDesc returned %x\n", ret);

        IDirectDrawSurface_Release(dsurface);
        dsurface = NULL;

        ok(desc.dwFlags & DDSD_WIDTH, "Primary surface doesn't have width set\n");
        ok(desc.dwFlags & DDSD_HEIGHT, "Primary surface doesn't have height set\n");
        ok(desc.dwWidth == GetSystemMetrics(SM_CXSCREEN), "Surface width differs from screen width\n");
        ok(desc.dwHeight == GetSystemMetrics(SM_CYSCREEN), "Surface height differs from screen height\n");
    }
    ret = IDirectDraw_SetCooperativeLevel(lpDD, NULL, DDSCL_NORMAL);
    ok(ret == DD_OK, "SetCooperativeLevel failed with %08x\n", ret);
}

static void BltParamTest(void)
{
    IDirectDrawSurface *surface1 = NULL, *surface2 = NULL;
    DDSURFACEDESC desc;
    HRESULT hr;
    DDBLTFX BltFx;
    RECT valid = {10, 10, 20, 20};
    RECT invalid1 = {20, 10, 10, 20};
    RECT invalid2 = {20, 20, 20, 20};
    RECT invalid3 = {-1, -1, 20, 20};
    RECT invalid4 = {60, 60, 70, 70};

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    desc.ddsCaps.dwCaps |= DDSCAPS_OFFSCREENPLAIN;
    desc.dwHeight = 128;
    desc.dwWidth = 128;
    hr = IDirectDraw_CreateSurface(lpDD, &desc, &surface1, NULL);
    ok(hr == DD_OK, "Creating an offscreen plain surface failed with %08x\n", hr);

    desc.dwHeight = 64;
    desc.dwWidth = 64;
    hr = IDirectDraw_CreateSurface(lpDD, &desc, &surface2, NULL);
    ok(hr == DD_OK, "Creating an offscreen plain surface failed with %08x\n", hr);

    if(0)
    {
        /* This crashes */
        hr = IDirectDrawSurface_BltFast(surface1, 0, 0, NULL, NULL, 0);
        ok(hr == DD_OK, "BltFast from NULL surface returned %08x\n", hr);
    }
    hr = IDirectDrawSurface_BltFast(surface1, 0, 0, surface2, NULL, 0);
    ok(hr == DD_OK, "BltFast from smaller to bigger surface returned %08x\n", hr);
    hr = IDirectDrawSurface_BltFast(surface2, 0, 0, surface1, NULL, 0);
    ok(hr == DDERR_INVALIDRECT, "BltFast from bigger to smaller surface returned %08x\n", hr);
    hr = IDirectDrawSurface_BltFast(surface2, 0, 0, surface1, &valid, 0);
    ok(hr == DD_OK, "BltFast from bigger to smaller surface using a valid rectangle returned %08x\n", hr);
    hr = IDirectDrawSurface_BltFast(surface2, 60, 60, surface1, &valid, 0);
    ok(hr == DDERR_INVALIDRECT, "BltFast with a rectangle resulting in an off-surface write returned %08x\n", hr);
    hr = IDirectDrawSurface_BltFast(surface1, 90, 90, surface2, NULL, 0);
    ok(hr == DDERR_INVALIDRECT, "BltFast with a rectangle resulting in an off-surface write returned %08x\n", hr);

    hr = IDirectDrawSurface_BltFast(surface1, -10, 0, surface2, NULL, 0);
    ok(hr == DDERR_INVALIDRECT, "BltFast with an offset resulting in an off-surface write returned %08x\n", hr);
    hr = IDirectDrawSurface_BltFast(surface1, 0, -10, surface2, NULL, 0);
    ok(hr == DDERR_INVALIDRECT, "BltFast with an offset resulting in an off-surface write returned %08x\n", hr);
    hr = IDirectDrawSurface_BltFast(surface2, 20, 20, surface1, &valid, 0);
    ok(hr == DD_OK, "BltFast from bigger to smaller surface using a valid rectangle and offset returned %08x\n", hr);

    hr = IDirectDrawSurface_BltFast(surface2, 0, 0, surface1, &invalid1, 0);
    ok(hr == DDERR_INVALIDRECT, "BltFast with invalid rectangle 1 returned %08x\n", hr);
    hr = IDirectDrawSurface_BltFast(surface2, 0, 0, surface1, &invalid2, 0);
    ok(hr == DDERR_INVALIDRECT, "BltFast with invalid rectangle 2 returned %08x\n", hr);
    hr = IDirectDrawSurface_BltFast(surface2, 0, 0, surface1, &invalid3, 0);
    ok(hr == DDERR_INVALIDRECT, "BltFast with invalid rectangle 3 returned %08x\n", hr);
    hr = IDirectDrawSurface_BltFast(surface1, 0, 0, surface2, &invalid4, 0);
    ok(hr == DDERR_INVALIDRECT, "BltFast with invalid rectangle 3 returned %08x\n", hr);
    hr = IDirectDrawSurface_BltFast(surface1, 0, 0, surface1, NULL, 0);
    ok(hr == DD_OK, "BltFast blitting a surface onto itself returned %08x\n", hr);

    /* Blt(non-fast) tests */
    memset(&BltFx, 0, sizeof(BltFx));
    BltFx.dwSize = sizeof(BltFx);
    U5(BltFx).dwFillColor = 0xaabbccdd;

    hr = IDirectDrawSurface_Blt(surface1, &valid, NULL, NULL, DDBLT_COLORFILL, &BltFx);
    ok(hr == DD_OK, "IDirectDrawSurface_Blt with a valid rectangle for color fill returned %08x\n", hr);
    hr = IDirectDrawSurface_Blt(surface1, &valid, NULL, &invalid3, DDBLT_COLORFILL, &BltFx);
    ok(hr == DD_OK, "IDirectDrawSurface_Blt with an invalid, unused rectangle returned %08x\n", hr);
    hr = IDirectDrawSurface_Blt(surface2, &invalid1, NULL, NULL, DDBLT_COLORFILL, &BltFx);
    ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface_Blt with a with invalid rectangle 1 returned %08x\n", hr);
    hr = IDirectDrawSurface_Blt(surface2, &invalid2, NULL, NULL, DDBLT_COLORFILL, &BltFx);
    ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface_Blt with a with invalid rectangle 2 returned %08x\n", hr);
    hr = IDirectDrawSurface_Blt(surface2, &invalid3, NULL, NULL, DDBLT_COLORFILL, &BltFx);
    ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface_Blt with a with invalid rectangle 3 returned %08x\n", hr);
    hr = IDirectDrawSurface_Blt(surface2, &invalid4, NULL, NULL, DDBLT_COLORFILL, &BltFx);
    ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface_Blt with a with invalid rectangle 4 returned %08x\n", hr);

    /* Valid on surface 1 */
    hr = IDirectDrawSurface_Blt(surface1, &invalid4, NULL, NULL, DDBLT_COLORFILL, &BltFx);
    ok(hr == DD_OK, "IDirectDrawSurface_Blt with a subrectangle fill returned %08x\n", hr);

    /* Works - stretched blit */
    hr = IDirectDrawSurface_Blt(surface1, NULL, surface2, NULL, 0, NULL);
    ok(hr == DD_OK, "IDirectDrawSurface_Blt from a smaller to a bigger surface returned %08x\n", hr);
    hr = IDirectDrawSurface_Blt(surface2, NULL, surface1, NULL, 0, NULL);
    ok(hr == DD_OK, "IDirectDrawSurface_Blt from a bigger to a smaller surface %08x\n", hr);

    /* Invalid dest rects in sourced blits */
    hr = IDirectDrawSurface_Blt(surface2, &invalid1, surface1, NULL, 0, NULL);
    ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface_Blt with a with invalid rectangle 1 returned %08x\n", hr);
    hr = IDirectDrawSurface_Blt(surface2, &invalid2, surface1, NULL, 0, NULL);
    ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface_Blt with a with invalid rectangle 2 returned %08x\n", hr);
    hr = IDirectDrawSurface_Blt(surface2, &invalid3, surface1, NULL, 0, NULL);
    ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface_Blt with a with invalid rectangle 3 returned %08x\n", hr);
    hr = IDirectDrawSurface_Blt(surface2, &invalid4, surface1, NULL, 0, NULL);
    ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface_Blt with a with invalid rectangle 4 returned %08x\n", hr);

    /* Invalid src rects */
    hr = IDirectDrawSurface_Blt(surface2, NULL, surface1, &invalid1, 0, NULL);
    ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface_Blt with a with invalid rectangle 1 returned %08x\n", hr);
    hr = IDirectDrawSurface_Blt(surface2, NULL, surface1, &invalid2, 0, NULL);
    ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface_Blt with a with invalid rectangle 2 returned %08x\n", hr);
    hr = IDirectDrawSurface_Blt(surface2, NULL, surface1, &invalid3, 0, NULL);
    ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface_Blt with a with invalid rectangle 3 returned %08x\n", hr);
    hr = IDirectDrawSurface_Blt(surface1, NULL, surface2, &invalid4, 0, NULL);
    ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface_Blt with a with invalid rectangle 4 returned %08x\n", hr);

    IDirectDrawSurface_Release(surface1);
    IDirectDrawSurface_Release(surface2);
}

static void PaletteTest(void)
{
    HRESULT hr;
    IDirectDrawSurface *lpSurf = NULL;
    IDirectDrawSurface *backbuffer = NULL;
    DDSCAPS ddscaps;
    DDSURFACEDESC ddsd;
    IDirectDrawPalette *palette = NULL;
    PALETTEENTRY Table[256];
    PALETTEENTRY palEntries[256];
    int i;

    for(i=0; i<256; i++)
    {
        Table[i].peRed   = 0xff;
        Table[i].peGreen = 0;
        Table[i].peBlue  = 0;
        Table[i].peFlags = 0;
    }

    /* Create a 8bit palette without DDPCAPS_ALLOW256 set */
    hr = IDirectDraw_CreatePalette(lpDD, DDPCAPS_8BIT, Table, &palette, NULL);
    ok(hr == DD_OK, "CreatePalette failed with %08x\n", hr);
    if (FAILED(hr)) goto err;
    /* Read back the palette and verify the entries. Without DDPCAPS_ALLOW256 set
    /  entry 0 and 255 should have been overwritten with black and white */
    hr = IDirectDrawPalette_GetEntries(palette , 0, 0, 256, &palEntries[0]);
    ok(hr == DD_OK, "GetEntries failed with %08x\n", hr);
    if(hr == DD_OK)
    {
        ok((palEntries[0].peRed == 0) && (palEntries[0].peGreen == 0) && (palEntries[0].peBlue == 0),
           "Palette entry 0 of a palette without DDPCAPS_ALLOW256 set should be (0,0,0) but it is (%d,%d,%d)\n",
           palEntries[0].peRed, palEntries[0].peGreen, palEntries[0].peBlue);
        ok((palEntries[255].peRed == 255) && (palEntries[255].peGreen == 255) && (palEntries[255].peBlue == 255),
           "Palette entry 255 of a palette without DDPCAPS_ALLOW256 set should be (255,255,255) but it is (%d,%d,%d)\n",
           palEntries[255].peRed, palEntries[255].peGreen, palEntries[255].peBlue);

        /* Entry 1-254 should contain red */
        for(i=1; i<255; i++)
            ok((palEntries[i].peRed == 255) && (palEntries[i].peGreen == 0) && (palEntries[i].peBlue == 0),
               "Palette entry %d should have contained (255,0,0) but was set to (%d,%d,%d)\n",
               i, palEntries[i].peRed, palEntries[i].peGreen, palEntries[i].peBlue);
    }

    /* CreatePalette without DDPCAPS_ALLOW256 ignores entry 0 and 255,
    /  now check we are able to update the entries afterwards. */
    hr = IDirectDrawPalette_SetEntries(palette , 0, 0, 256, &Table[0]);
    ok(hr == DD_OK, "SetEntries failed with %08x\n", hr);
    hr = IDirectDrawPalette_GetEntries(palette , 0, 0, 256, &palEntries[0]);
    ok(hr == DD_OK, "GetEntries failed with %08x\n", hr);
    if(hr == DD_OK)
    {
        ok((palEntries[0].peRed == 0) && (palEntries[0].peGreen == 0) && (palEntries[0].peBlue == 0),
           "Palette entry 0 should have been set to (0,0,0) but it contains (%d,%d,%d)\n",
           palEntries[0].peRed, palEntries[0].peGreen, palEntries[0].peBlue);
        ok((palEntries[255].peRed == 255) && (palEntries[255].peGreen == 255) && (palEntries[255].peBlue == 255),
           "Palette entry 255 should have been set to (255,255,255) but it contains (%d,%d,%d)\n",
           palEntries[255].peRed, palEntries[255].peGreen, palEntries[255].peBlue);
    }
    IDirectDrawPalette_Release(palette);

    /* Create a 8bit palette with DDPCAPS_ALLOW256 set */
    hr = IDirectDraw_CreatePalette(lpDD, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, Table, &palette, NULL);
    ok(hr == DD_OK, "CreatePalette failed with %08x\n", hr);
    if (FAILED(hr)) goto err;

    hr = IDirectDrawPalette_GetEntries(palette , 0, 0, 256, &palEntries[0]);
    ok(hr == DD_OK, "GetEntries failed with %08x\n", hr);
    if(hr == DD_OK)
    {
        /* All entries should contain red */
        for(i=0; i<256; i++)
            ok((palEntries[i].peRed == 255) && (palEntries[i].peGreen == 0) && (palEntries[i].peBlue == 0),
               "Palette entry %d should have contained (255,0,0) but was set to (%d,%d,%d)\n",
               i, palEntries[i].peRed, palEntries[i].peGreen, palEntries[i].peBlue);
    }

    /* Try to set palette to a non-palettized surface */
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
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpSurf, NULL);
    ok(hr==DD_OK, "CreateSurface returned: %x\n",hr);
    if (FAILED(hr))
    {
        skip("failed to create surface\n");
        goto err;
    }

    hr = IDirectDrawSurface_SetPalette(lpSurf, palette);
    ok(hr == DDERR_INVALIDPIXELFORMAT, "CreateSurface returned: %x\n",hr);

    IDirectDrawPalette_Release(palette);
    palette = NULL;

    hr = IDirectDrawSurface_GetPalette(lpSurf, &palette);
    ok(hr == DDERR_NOPALETTEATTACHED, "CreateSurface returned: %x\n",hr);

    err:

    if (lpSurf) IDirectDrawSurface_Release(lpSurf);
    if (palette) IDirectDrawPalette_Release(palette);

    hr = IDirectDraw_CreatePalette(lpDD, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, Table, &palette, NULL);
    ok(hr == DD_OK, "CreatePalette failed with %08x\n", hr);

    ddsd.dwSize = sizeof(ddsd);
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_BACKBUFFERCOUNT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
    ddsd.dwWidth = 64;
    ddsd.dwHeight = 64;
    ddsd.dwBackBufferCount = 1;
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
    U1(ddsd.ddpfPixelFormat).dwRGBBitCount = 8;

    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpSurf, NULL);
    ok(hr==DD_OK, "CreateSurface returned: %x\n",hr);
    if (FAILED(hr))
    {
        skip("failed to create surface\n");
        return;
    }

    ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
    hr = IDirectDrawSurface_GetAttachedSurface(lpSurf, &ddscaps, &backbuffer);
    ok(hr == DD_OK, "GetAttachedSurface returned: %x\n",hr);

    hr = IDirectDrawSurface_SetPalette(backbuffer, palette);
    ok(hr == DD_OK, "SetPalette returned: %x\n",hr);

    IDirectDrawPalette_Release(palette);
    palette = NULL;

    hr = IDirectDrawSurface_GetPalette(backbuffer, &palette);
    ok(hr == DD_OK, "CreateSurface returned: %x\n",hr);

    IDirectDrawSurface_Release(backbuffer);
    IDirectDrawSurface_Release(lpSurf);
}

static void StructSizeTest(void)
{
    IDirectDrawSurface *surface1;
    IDirectDrawSurface7 *surface7;
    union {
        DDSURFACEDESC desc1;
        DDSURFACEDESC2 desc2;
        char blob[1024]; /* To get a bunch of writable memory */
    } desc;
    DDSURFACEDESC create;
    HRESULT hr;

    memset(&desc, 0, sizeof(desc));
    memset(&create, 0, sizeof(create));

    memset(&create, 0, sizeof(create));
    create.dwSize = sizeof(create);
    create.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    create.ddsCaps.dwCaps |= DDSCAPS_OFFSCREENPLAIN;
    create.dwHeight = 128;
    create.dwWidth = 128;
    hr = IDirectDraw_CreateSurface(lpDD, &create, &surface1, NULL);
    ok(hr == DD_OK, "Creating an offscreen plain surface failed with %08x\n", hr);
    hr = IDirectDrawSurface_QueryInterface(surface1, &IID_IDirectDrawSurface7, (void **) &surface7);
    ok(hr == DD_OK, "IDirectDrawSurface_QueryInterface failed with %08x\n", hr);

    desc.desc1.dwSize = sizeof(DDSURFACEDESC);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface1, &desc.desc1);
    ok(hr == DD_OK, "IDirectDrawSurface_GetSurfaceDesc with desc size sizeof(DDSURFACEDESC) returned %08x\n", hr);
    hr = IDirectDrawSurface7_GetSurfaceDesc(surface7, &desc.desc2);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDrawSurface7_GetSurfaceDesc with desc size sizeof(DDSURFACEDESC) returned %08x\n", hr);

    desc.desc2.dwSize = sizeof(DDSURFACEDESC2);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface1, &desc.desc1);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDrawSurface_GetSurfaceDesc with desc size sizeof(DDSURFACEDESC2) returned %08x\n", hr);
    hr = IDirectDrawSurface7_GetSurfaceDesc(surface7, &desc.desc2);
    ok(hr == DD_OK, "IDirectDrawSurface7_GetSurfaceDesc with desc size sizeof(DDSURFACEDESC2) returned %08x\n", hr);

    desc.desc2.dwSize = 0;
    hr = IDirectDrawSurface_GetSurfaceDesc(surface1, &desc.desc1);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDrawSurface_GetSurfaceDesc with desc size 0 returned %08x\n", hr);
    hr = IDirectDrawSurface7_GetSurfaceDesc(surface7, &desc.desc2);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDrawSurface7_GetSurfaceDesc with desc size 0 returned %08x\n", hr);

    desc.desc1.dwSize = sizeof(DDSURFACEDESC) + 1;
    hr = IDirectDrawSurface_GetSurfaceDesc(surface1, &desc.desc1);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDrawSurface_GetSurfaceDesc with desc size sizeof(DDSURFACEDESC) + 1 returned %08x\n", hr);
    hr = IDirectDrawSurface7_GetSurfaceDesc(surface7, &desc.desc2);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDrawSurface7_GetSurfaceDesc with desc size sizeof(DDSURFACEDESC) + 1 returned %08x\n", hr);

    desc.desc2.dwSize = sizeof(DDSURFACEDESC2) + 1;
    hr = IDirectDrawSurface_GetSurfaceDesc(surface1, &desc.desc1);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDrawSurface_GetSurfaceDesc with desc size sizeof(DDSURFACEDESC2) + 1returned %08x\n", hr);
    hr = IDirectDrawSurface7_GetSurfaceDesc(surface7, &desc.desc2);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDrawSurface7_GetSurfaceDesc with desc size sizeof(DDSURFACEDESC2) + 1returned %08x\n", hr);

    /* Tests for Lock() */

    desc.desc1.dwSize = sizeof(DDSURFACEDESC);
    hr = IDirectDrawSurface_Lock(surface1, NULL, &desc.desc1, 0, 0);
    ok(hr == DD_OK, "IDirectDrawSurface_Lock with desc size sizeof(DDSURFACEDESC) returned %08x\n", hr);
    if(SUCCEEDED(hr)) IDirectDrawSurface_Unlock(surface1, NULL);
    ok(desc.desc1.dwSize == sizeof(DDSURFACEDESC), "Destination size was changed to %d\n", desc.desc1.dwSize);
    hr = IDirectDrawSurface7_Lock(surface7, NULL, &desc.desc2, 0, 0);
    ok(hr == DD_OK, "IDirectDrawSurface7_Lock with desc size sizeof(DDSURFACEDESC) returned %08x\n", hr);
    if(SUCCEEDED(hr)) IDirectDrawSurface7_Unlock(surface7, NULL);
    ok(desc.desc2.dwSize == sizeof(DDSURFACEDESC), "Destination size was changed to %d\n", desc.desc1.dwSize);

    desc.desc2.dwSize = sizeof(DDSURFACEDESC2);
    hr = IDirectDrawSurface_Lock(surface1, NULL, &desc.desc1, 0, 0);
    ok(hr == DD_OK, "IDirectDrawSurface_Lock with desc size sizeof(DDSURFACEDESC2) returned %08x\n", hr);
    ok(desc.desc1.dwSize == sizeof(DDSURFACEDESC2), "Destination size was changed to %d\n", desc.desc1.dwSize);
    if(SUCCEEDED(hr)) IDirectDrawSurface_Unlock(surface1, NULL);
    hr = IDirectDrawSurface7_Lock(surface7, NULL, &desc.desc2, 0, 0);
    ok(hr == DD_OK, "IDirectDrawSurface7_Lock with desc size sizeof(DDSURFACEDESC2) returned %08x\n", hr);
    if(SUCCEEDED(hr)) IDirectDrawSurface7_Unlock(surface7, NULL);
    ok(desc.desc2.dwSize == sizeof(DDSURFACEDESC2), "Destination size was changed to %d\n", desc.desc1.dwSize);

    desc.desc2.dwSize = 0;
    hr = IDirectDrawSurface_Lock(surface1, NULL, &desc.desc1, 0, 0);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDrawSurface_Lock with desc size 0 returned %08x\n", hr);
    if(SUCCEEDED(hr)) IDirectDrawSurface_Unlock(surface1, NULL);
    hr = IDirectDrawSurface7_Lock(surface7, NULL, &desc.desc2, 0, 0);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDrawSurface7_Lock with desc size 0 returned %08x\n", hr);
    if(SUCCEEDED(hr)) IDirectDrawSurface7_Unlock(surface7, NULL);

    desc.desc1.dwSize = sizeof(DDSURFACEDESC) + 1;
    hr = IDirectDrawSurface_Lock(surface1, NULL, &desc.desc1, 0, 0);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDrawSurface_Lock with desc size sizeof(DDSURFACEDESC) + 1 returned %08x\n", hr);
    if(SUCCEEDED(hr)) IDirectDrawSurface_Unlock(surface1, NULL);
    hr = IDirectDrawSurface7_Lock(surface7, NULL, &desc.desc2, 0, 0);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDrawSurface7_Lock with desc size sizeof(DDSURFACEDESC) + 1 returned %08x\n", hr);
    if(SUCCEEDED(hr)) IDirectDrawSurface7_Unlock(surface7, NULL);

    desc.desc2.dwSize = sizeof(DDSURFACEDESC2) + 1;
    hr = IDirectDrawSurface_Lock(surface1, NULL, &desc.desc1, 0, 0);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDrawSurface_Lock with desc size sizeof(DDSURFACEDESC2) + 1returned %08x\n", hr);
    if(SUCCEEDED(hr)) IDirectDrawSurface_Unlock(surface1, NULL);
    hr = IDirectDrawSurface7_Lock(surface7, NULL, &desc.desc2, 0, 0);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDrawSurface7_Lock with desc size sizeof(DDSURFACEDESC2) + 1returned %08x\n", hr);
    if(SUCCEEDED(hr)) IDirectDrawSurface7_Unlock(surface7, NULL);

    IDirectDrawSurface7_Release(surface7);
    IDirectDrawSurface_Release(surface1);
}

static void SurfaceCapsTest(void)
{
    DDSURFACEDESC create;
    DDSURFACEDESC desc;
    HRESULT hr;
    IDirectDrawSurface *surface1 = NULL;
    DDSURFACEDESC2 create2, desc2;
    IDirectDrawSurface7 *surface7 = NULL;
    IDirectDraw7 *dd7 = NULL;
    DWORD create_caps[] = {
        DDSCAPS_OFFSCREENPLAIN,
        DDSCAPS_TEXTURE,
        DDSCAPS_TEXTURE | DDSCAPS_ALLOCONLOAD,
        0,
        DDSCAPS_TEXTURE | DDSCAPS_ALLOCONLOAD | DDSCAPS_SYSTEMMEMORY,
        DDSCAPS_PRIMARYSURFACE,
        DDSCAPS_PRIMARYSURFACE | DDSCAPS_SYSTEMMEMORY,
        DDSCAPS_3DDEVICE,
        DDSCAPS_ZBUFFER,
        DDSCAPS_3DDEVICE | DDSCAPS_OFFSCREENPLAIN
    };
    DWORD expected_caps[] = {
        DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
        DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
        DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM | DDSCAPS_ALLOCONLOAD,
        DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
        DDSCAPS_TEXTURE | DDSCAPS_ALLOCONLOAD | DDSCAPS_SYSTEMMEMORY,
        DDSCAPS_PRIMARYSURFACE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM | DDSCAPS_VISIBLE,
        DDSCAPS_PRIMARYSURFACE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_VISIBLE,
        DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
        DDSCAPS_ZBUFFER | DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY,
        DDSCAPS_3DDEVICE | DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM
    };
    UINT i;

    /* Tests various surface flags, what changes do they undergo during
     * surface creation. Forsaken engine expects texture surfaces without
     * memory flag to get a video memory flag right after creation. */

    if (!(ddcaps.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
    {
        skip("DDraw reported no VIDEOMEMORY cap. Broken video driver? Skipping surface caps tests.\n");
        return ;
    }

    for (i = 0; i < sizeof(create_caps) / sizeof(DWORD); i++)
    {
        memset(&create, 0, sizeof(create));
        create.dwSize = sizeof(create);
        create.ddsCaps.dwCaps = create_caps[i];
        create.dwFlags = DDSD_CAPS;

        if (!(create.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
        {
            create.dwFlags |= DDSD_HEIGHT | DDSD_WIDTH;
            create.dwHeight = 128;
            create.dwWidth = 128;
        }

        if (create.ddsCaps.dwCaps & DDSCAPS_ZBUFFER)
        {
            create.dwFlags |= DDSD_PIXELFORMAT;
            create.ddpfPixelFormat.dwSize = sizeof(create.ddpfPixelFormat);
            create.ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
            U1(create.ddpfPixelFormat).dwZBufferBitDepth = 16;
            U3(create.ddpfPixelFormat).dwZBitMask = 0x0000FFFF;
        }

        hr = IDirectDraw_CreateSurface(lpDD, &create, &surface1, NULL);
        ok(hr == DD_OK, "IDirectDraw_CreateSurface failed with %08x\n", hr);

        if (SUCCEEDED(hr))
        {
            memset(&desc, 0, sizeof(desc));
            desc.dwSize = sizeof(DDSURFACEDESC);
            hr = IDirectDrawSurface_GetSurfaceDesc(surface1, &desc);
            ok(hr == DD_OK, "IDirectDrawSurface_GetSurfaceDesc failed with %08x\n", hr);

            ok(desc.ddsCaps.dwCaps == expected_caps[i],
                    "GetSurfaceDesc test %d returned caps %x, expected %x\n",
                    i, desc.ddsCaps.dwCaps, expected_caps[i]);

            IDirectDrawSurface_Release(surface1);
        }
    }

    /* Test for differences in ddraw 7 */
    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    ok(hr == DD_OK, "IDirectDraw_QueryInterface returned %08x\n", hr);
    if (FAILED(hr))
    {
        skip("Failed to get IDirectDraw7 interface, skipping tests\n");
    }
    else
    {
        for (i = 0; i < sizeof(create_caps) / sizeof(DWORD); i++)
        {
            memset(&create2, 0, sizeof(create2));
            create2.dwSize = sizeof(create2);
            create2.ddsCaps.dwCaps = create_caps[i];
            create2.dwFlags = DDSD_CAPS;

            if (!(create2.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
            {
                create2.dwFlags |= DDSD_HEIGHT | DDSD_WIDTH;
                create2.dwHeight = 128;
                create2.dwWidth = 128;
            }

            if (create2.ddsCaps.dwCaps & DDSCAPS_ZBUFFER)
            {
                create2.dwFlags |= DDSD_PIXELFORMAT;
                U4(create2).ddpfPixelFormat.dwSize = sizeof(U4(create2).ddpfPixelFormat);
                U4(create2).ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
                U1(U4(create2).ddpfPixelFormat).dwZBufferBitDepth = 16;
                U3(U4(create2).ddpfPixelFormat).dwZBitMask = 0x0000FFFF;
            }

            hr = IDirectDraw7_CreateSurface(dd7, &create2, &surface7, NULL);
            ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

            if (SUCCEEDED(hr))
            {
                memset(&desc2, 0, sizeof(desc2));
                desc2.dwSize = sizeof(DDSURFACEDESC2);
                hr = IDirectDrawSurface7_GetSurfaceDesc(surface7, &desc2);
                ok(hr == DD_OK, "IDirectDrawSurface_GetSurfaceDesc failed with %08x\n", hr);

                ok(desc2.ddsCaps.dwCaps == expected_caps[i],
                        "GetSurfaceDesc test %d returned caps %x, expected %x\n",
                        i, desc2.ddsCaps.dwCaps, expected_caps[i]);

                IDirectDrawSurface7_Release(surface7);
            }
        }

        IDirectDraw7_Release(dd7);
    }

    memset(&create, 0, sizeof(create));
    create.dwSize = sizeof(create);
    create.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    create.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_VIDEOMEMORY;
    create.dwWidth = 64;
    create.dwHeight = 64;
    hr = IDirectDraw_CreateSurface(lpDD, &create, &surface1, NULL);
    ok(hr == DDERR_INVALIDCAPS, "Creating a SYSMEM | VIDMEM surface returned 0x%08x, expected DDERR_INVALIDCAPS\n", hr);
    if(surface1) IDirectDrawSurface_Release(surface1);
}

static BOOL can_create_primary_surface(void)
{
    DDSURFACEDESC ddsd;
    IDirectDrawSurface *surface;
    HRESULT hr;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface, NULL);
    if(FAILED(hr)) return FALSE;
    IDirectDrawSurface_Release(surface);
    return TRUE;
}

static void dctest_surf(IDirectDrawSurface *surf, int ddsdver) {
    HRESULT hr;
    HDC dc, dc2 = (HDC) 0x1234;
    DDSURFACEDESC ddsd;
    DDSURFACEDESC2 ddsd2;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);

    hr = IDirectDrawSurface_GetDC(surf, &dc);
    ok(hr == DD_OK, "IDirectDrawSurface_GetDC failed: 0x%08x\n", hr);

    hr = IDirectDrawSurface_GetDC(surf, &dc2);
    ok(hr == DDERR_DCALREADYCREATED, "IDirectDrawSurface_GetDC failed: 0x%08x\n", hr);
    ok(dc2 == (HDC) 0x1234, "The failed GetDC call changed the dc: %p\n", dc2);

    hr = IDirectDrawSurface_Lock(surf, NULL, ddsdver == 1 ? &ddsd : ((DDSURFACEDESC *) &ddsd2), 0, NULL);
    ok(hr == DDERR_SURFACEBUSY, "IDirectDrawSurface_Lock returned 0x%08x, expected DDERR_ALREADYLOCKED\n", hr);

    hr = IDirectDrawSurface_ReleaseDC(surf, dc);
    ok(hr == DD_OK, "IDirectDrawSurface_ReleaseDC failed: 0x%08x\n", hr);
    hr = IDirectDrawSurface_ReleaseDC(surf, dc);
    ok(hr == DDERR_NODC, "IDirectDrawSurface_ReleaseDC returned 0x%08x, expected DDERR_NODC\n", hr);
}

static void GetDCTest(void)
{
    DDSURFACEDESC ddsd;
    DDSURFACEDESC2 ddsd2;
    IDirectDrawSurface *surf;
    IDirectDrawSurface2 *surf2;
    IDirectDrawSurface4 *surf4;
    IDirectDrawSurface7 *surf7;
    IDirectDrawSurface *tmp;
    IDirectDrawSurface7 *tmp7;
    HRESULT hr;
    IDirectDraw2 *dd2;
    IDirectDraw4 *dd4;
    IDirectDraw7 *dd7;
    HDC dc;
    ULONG ref;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = 64;
    ddsd.dwHeight = 64;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    ddsd2.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd2.dwWidth = 64;
    ddsd2.dwHeight = 64;
    ddsd2.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surf, NULL);
    ok(hr == DD_OK, "IDirectDraw_CreateSurface failed: 0x%08x\n", hr);
    dctest_surf(surf, 1);
    IDirectDrawSurface_Release(surf);

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw2, (void **) &dd2);
    ok(hr == DD_OK, "IDirectDraw_QueryInterface failed: 0x%08x\n", hr);

    hr = IDirectDraw2_CreateSurface(dd2, &ddsd, &surf, NULL);
    ok(hr == DD_OK, "IDirectDraw2_CreateSurface failed: 0x%08x\n", hr);
    dctest_surf(surf, 1);

    hr = IDirectDrawSurface_QueryInterface(surf, &IID_IDirectDrawSurface2, (void **) &surf2);
    ok(hr == DD_OK, "IDirectDrawSurface_QueryInterface failed: 0x%08x\n", hr);
    dctest_surf((IDirectDrawSurface *) surf2, 1);

    IDirectDrawSurface2_Release(surf2);
    IDirectDrawSurface_Release(surf);
    IDirectDraw2_Release(dd2);

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw4, (void **) &dd4);
    ok(hr == DD_OK, "IDirectDraw_QueryInterface failed: 0x%08x\n", hr);

    surf = NULL;
    hr = IDirectDraw4_CreateSurface(dd4, &ddsd2, &surf4, NULL);
    ok(hr == DD_OK, "IDirectDraw4_CreateSurface failed: 0x%08x\n", hr);
    dctest_surf((IDirectDrawSurface *) surf4, 2);

    hr = IDirectDrawSurface4_QueryInterface(surf4, &IID_IDirectDrawSurface, (void **)&surf);
    ok(SUCCEEDED(hr), "QueryInterface failed, hr %#x.\n", hr);

    ref = getRefcount((IUnknown *) surf);
    ok(ref == 1, "Refcount is %u, expected 1\n", ref);
    ref = getRefcount((IUnknown *) surf4);
    ok(ref == 1, "Refcount is %u, expected 1\n", ref);

    hr = IDirectDrawSurface4_GetDC(surf4, &dc);
    ok(SUCCEEDED(hr), "GetDC failed, hr %#x.\n", hr);

    hr = IDirectDraw4_GetSurfaceFromDC(dd4, dc, NULL);
    ok(hr == E_INVALIDARG, "Expected hr E_INVALIDARG, got %#x.\n", hr);

    hr = IDirectDraw4_GetSurfaceFromDC(dd4, dc, (IDirectDrawSurface4 **)&tmp);
    ok(SUCCEEDED(hr), "GetSurfaceFromDC failed, hr %#x.\n", hr);
    ok(tmp == surf, "Expected surface %p, got %p.\n\n", surf, tmp);

    ref = getRefcount((IUnknown *) surf);
    ok(ref == 2, "Refcount is %u, expected 2\n", ref);
    ref = getRefcount((IUnknown *) tmp);
    ok(ref == 2, "Refcount is %u, expected 2\n", ref);
    ref = getRefcount((IUnknown *) surf4);
    ok(ref == 1, "Refcount is %u, expected 1\n", ref);

    hr = IDirectDrawSurface4_ReleaseDC(surf4, dc);
    ok(SUCCEEDED(hr), "ReleaseDC failed, hr %#x.\n", hr);

    IDirectDrawSurface_Release(tmp);

    dc = CreateCompatibleDC(NULL);
    ok(!!dc, "CreateCompatibleDC failed.\n");

    tmp = (IDirectDrawSurface *)0xdeadbeef;
    hr = IDirectDraw4_GetSurfaceFromDC(dd4, dc, (IDirectDrawSurface4 **)&tmp);
    ok(hr == DDERR_NOTFOUND, "GetSurfaceFromDC failed, hr %#x.\n", hr);
    ok(!tmp, "Expected surface NULL, got %p.\n", tmp);

    ok(DeleteDC(dc), "DeleteDC failed.\n");

    tmp = (IDirectDrawSurface *)0xdeadbeef;
    hr = IDirectDraw4_GetSurfaceFromDC(dd4, NULL, (IDirectDrawSurface4 **)&tmp);
    ok(hr == DDERR_NOTFOUND, "GetSurfaceFromDC failed, hr %#x.\n", hr);
    ok(!tmp, "Expected surface NULL, got %p.\n", tmp);

    IDirectDrawSurface4_Release(surf4);
    IDirectDrawSurface_Release(surf);
    IDirectDraw4_Release(dd4);

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    ok(hr == DD_OK, "IDirectDraw_QueryInterface failed: 0x%08x\n", hr);

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd2, &surf7, NULL);
    ok(hr == DD_OK, "IDirectDraw7_CreateSurface failed: 0x%08x\n", hr);
    dctest_surf((IDirectDrawSurface *) surf7, 2);

    hr = IDirectDrawSurface7_GetDC(surf7, &dc);
    ok(SUCCEEDED(hr), "GetDC failed, hr %#x.\n", hr);

    hr = IDirectDraw7_GetSurfaceFromDC(dd7, dc, NULL);
    ok(hr == E_INVALIDARG, "Expected hr E_INVALIDARG, got %#x.\n", hr);

    hr = IDirectDraw7_GetSurfaceFromDC(dd7, dc, &tmp7);
    ok(SUCCEEDED(hr), "GetSurfaceFromDC failed, hr %#x.\n", hr);
    ok(tmp7 == surf7, "Expected surface %p, got %p.\n\n", surf7, tmp7);
    IDirectDrawSurface7_Release(tmp7);

    hr = IDirectDrawSurface7_ReleaseDC(surf7, dc);
    ok(SUCCEEDED(hr), "ReleaseDC failed, hr %#x.\n", hr);

    dc = CreateCompatibleDC(NULL);
    ok(!!dc, "CreateCompatibleDC failed.\n");

    tmp7 = (IDirectDrawSurface7 *)0xdeadbeef;
    hr = IDirectDraw7_GetSurfaceFromDC(dd7, dc, &tmp7);
    ok(hr == DDERR_NOTFOUND, "GetSurfaceFromDC failed, hr %#x.\n", hr);
    ok(!tmp7, "Expected surface NULL, got %p.\n", tmp7);

    ok(DeleteDC(dc), "DeleteDC failed.\n");

    tmp7 = (IDirectDrawSurface7 *)0xdeadbeef;
    hr = IDirectDraw7_GetSurfaceFromDC(dd7, NULL, (IDirectDrawSurface7 **)&tmp7);
    ok(hr == DDERR_NOTFOUND, "GetSurfaceFromDC failed, hr %#x.\n", hr);
    ok(!tmp7, "Expected surface NULL, got %p.\n", tmp7);

    IDirectDrawSurface7_Release(surf7);
    IDirectDraw7_Release(dd7);
}

static void GetDCFormatTest(void)
{
    DDSURFACEDESC2 ddsd;
    unsigned int i;
    IDirectDrawSurface7 *surface;
    IDirectDraw7 *dd7;
    HRESULT hr;
    HDC dc;

    struct
    {
        const char *name;
        DDPIXELFORMAT fmt;
        BOOL getdc_capable;
        HRESULT alt_result;
    } testdata[] = {
        {
            "D3DFMT_A8R8G8B8",
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS, 0,
                {32}, {0x00ff0000}, {0x0000ff00}, {0x000000ff}, {0xff000000}
            },
            TRUE
        },
        {
            "D3DFMT_X8R8G8B8",
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB, 0,
                {32}, {0x00ff0000}, {0x0000ff00}, {0x000000ff}, {0x00000000}
            },
            TRUE
        },
        {
            "D3DFMT_X8B8G8R8",
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB, 0,
                {32}, {0x000000ff}, {0x0000ff00}, {0x00ff0000}, {0x00000000}
            },
            TRUE,
            DDERR_CANTCREATEDC /* Vista+ */
        },
        {
            "D3DFMT_X8B8G8R8",
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS, 0,
                       {32}, {0x000000ff}, {0x0000ff00}, {0x00ff0000}, {0xff000000}
            },
            TRUE,
            DDERR_CANTCREATEDC /* Vista+ */
        },
        {
            "D3DFMT_A4R4G4B4",
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS, 0,
                       {16}, {0x00000f00}, {0x000000f0}, {0x0000000f}, {0x0000f000}
            },
            TRUE,
            DDERR_CANTCREATEDC /* Vista+ */
        },
        {
            "D3DFMT_X4R4G4B4",
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB, 0,
                       {16}, {0x00000f00}, {0x000000f0}, {0x0000000f}, {0x00000000}
            },
            TRUE,
            DDERR_CANTCREATEDC /* Vista+ */
        },
        {
            "D3DFMT_R5G6B5",
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB, 0,
                       {16}, {0x0000F800}, {0x000007E0}, {0x0000001F}, {0x00000000}
            },
            TRUE
        },
        {
            "D3DFMT_A1R5G5B5",
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS, 0,
                       {16}, {0x00007C00}, {0x000003E0}, {0x0000001F}, {0x00008000}
            },
            TRUE
        },
        {
            "D3DFMT_X1R5G5B5",
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB, 0,
                       {16}, {0x00007C00}, {0x000003E0}, {0x0000001F}, {0x00000000}
            },
            TRUE
        },
        {
            "D3DFMT_R3G3B2",
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB, 0,
                       { 8}, {0x000000E0}, {0x0000001C}, {0x00000003}, {0x00000000}
            },
            FALSE
        },
        {
            /* Untested, windows test machine didn't support this format */
            "D3DFMT_A2R10G10B10",
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS, 0,
                       {32}, {0xC0000000}, {0x3FF00000}, {0x000FFC00}, {0x000003FF}
            },
            TRUE
        },
        /*
         * GetDC on a P8 surface fails unless the display mode is 8 bpp. This is not
         * implemented in wine yet, so disable the test for now. Succeeding P8 getDC
         * calls are tested in the ddraw.visual test.
         *
        {
            "D3DFMT_P8",
            {
                sizeof(DDPIXELFORMAT), DDPF_PALETTEINDEXED8 | DDPF_RGB, 0,
                {8 }, {0x00000000}, {0x00000000}, {0x00000000}, {0x00000000}
            },
            FALSE
        },
         */
        {
            "D3DFMT_L8",
            {
                sizeof(DDPIXELFORMAT), DDPF_LUMINANCE, 0,
                {8 }, {0x000000ff}, {0x00000000}, {0x00000000}, {0x00000000}
            },
            FALSE
        },
        {
            "D3DFMT_A8L8",
            {
                sizeof(DDPIXELFORMAT), DDPF_ALPHAPIXELS | DDPF_LUMINANCE, 0,
                {16}, {0x000000ff}, {0x00000000}, {0x00000000}, {0x0000ff00}
            },
            FALSE
        },
        {
            "D3DFMT_DXT1",
            {
                sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('D','X','T','1'),
                {0 }, {0x00000000}, {0x00000000}, {0x00000000}, {0x00000000}
            },
            FALSE
        },
        {
            "D3DFMT_DXT2",
            {
                sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('D','X','T','2'),
                {0 }, {0x00000000}, {0x00000000}, {0x00000000}, {0x00000000}
            },
            FALSE
        },
        {
            "D3DFMT_DXT3",
            {
                sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('D','X','T','3'),
                {0 }, {0x00000000}, {0x00000000}, {0x00000000}, {0x00000000}
            },
            FALSE
        },
        {
            "D3DFMT_DXT4",
            {
                sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('D','X','T','4'),
                {0 }, {0x00000000}, {0x00000000}, {0x00000000}, {0x00000000}
            },
            FALSE
        },
        {
            "D3DFMT_DXT5",
            {
                sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('D','X','T','5'),
                {0 }, {0x00000000}, {0x00000000}, {0x00000000}, {0x00000000}
            },
            FALSE
        },
    };

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    ok(hr == DD_OK, "IDirectDraw_QueryInterface failed, hr = 0x%08x\n", hr);

    for(i = 0; i < (sizeof(testdata) / sizeof(testdata[0])); i++)
    {
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
        ddsd.dwWidth = 64;
        ddsd.dwHeight = 64;
        U4(ddsd).ddpfPixelFormat = testdata[i].fmt;
        ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

        hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface, NULL);
        if(FAILED(hr))
        {
            ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
            ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
            hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface, NULL);
            if(FAILED(hr))
            {
                skip("IDirectDraw7_CreateSurface failed, hr = 0x%08x, format %s\n", hr, testdata[i].name);
                continue;
            }
        }

        dc = (void *) 0x1234;
        hr = IDirectDrawSurface7_GetDC(surface, &dc);
        if(testdata[i].getdc_capable)
        {
            ok(SUCCEEDED(hr) ||
               (testdata[i].alt_result && hr == testdata[i].alt_result),
               "GetDC on a %s surface failed(0x%08x), expected it to work\n",
               testdata[i].name, hr);
        }
        else
        {
            ok(FAILED(hr), "GetDC on a %s surface succeeded(0x%08x), expected it to fail\n",
               testdata[i].name, hr);
        }

        if(SUCCEEDED(hr))
        {
            hr = IDirectDrawSurface7_ReleaseDC(surface, dc);
            ok(hr == DD_OK, "IDirectDrawSurface7_ReleaseDC failed, hr = 0x%08x\n", hr);
            dc = 0;
        }
        else
        {
            ok(dc == NULL, "After failed GetDC dc is %p\n", dc);
        }

        IDirectDrawSurface7_Release(surface);
    }

    IDirectDraw7_Release(dd7);
}

static void BackBufferCreateSurfaceTest(void)
{
    DDSURFACEDESC ddsd;
    DDSURFACEDESC created_ddsd;
    DDSURFACEDESC2 ddsd2;
    IDirectDrawSurface *surf;
    IDirectDrawSurface4 *surf4;
    IDirectDrawSurface7 *surf7;
    HRESULT hr;
    IDirectDraw2 *dd2;
    IDirectDraw4 *dd4;
    IDirectDraw7 *dd7;

    const DWORD caps = DDSCAPS_BACKBUFFER;
    const DWORD expected_caps = DDSCAPS_BACKBUFFER | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;

    if (!(ddcaps.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
    {
        skip("DDraw reported no VIDEOMEMORY cap. Broken video driver? Skipping surface caps tests.\n");
        return ;
    }

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = 64;
    ddsd.dwHeight = 64;
    ddsd.ddsCaps.dwCaps = caps;
    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    ddsd2.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd2.dwWidth = 64;
    ddsd2.dwHeight = 64;
    ddsd2.ddsCaps.dwCaps = caps;
    memset(&created_ddsd, 0, sizeof(created_ddsd));
    created_ddsd.dwSize = sizeof(DDSURFACEDESC);

    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surf, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw_CreateSurface failed: 0x%08x\n", hr);
    if (surf != NULL)
    {
        hr = IDirectDrawSurface_GetSurfaceDesc(surf, &created_ddsd);
        ok(SUCCEEDED(hr), "IDirectDraw_GetSurfaceDesc failed: 0x%08x\n", hr);
        ok(created_ddsd.ddsCaps.dwCaps == expected_caps,
           "GetSurfaceDesc returned caps %x, expected %x\n", created_ddsd.ddsCaps.dwCaps,
           expected_caps);
        IDirectDrawSurface_Release(surf);
    }

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw2, (void **) &dd2);
    ok(SUCCEEDED(hr), "IDirectDraw_QueryInterface failed: 0x%08x\n", hr);

    hr = IDirectDraw2_CreateSurface(dd2, &ddsd, &surf, NULL);
    ok(hr == DDERR_INVALIDCAPS, "IDirectDraw2_CreateSurface didn't return %x08x, but %x08x\n",
       DDERR_INVALIDCAPS, hr);

    IDirectDraw2_Release(dd2);

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw4, (void **) &dd4);
    ok(SUCCEEDED(hr), "IDirectDraw_QueryInterface failed: 0x%08x\n", hr);

    hr = IDirectDraw4_CreateSurface(dd4, &ddsd2, &surf4, NULL);
    ok(hr == DDERR_INVALIDCAPS, "IDirectDraw4_CreateSurface didn't return %x08x, but %x08x\n",
       DDERR_INVALIDCAPS, hr);

    IDirectDraw4_Release(dd4);

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    ok(SUCCEEDED(hr), "IDirectDraw_QueryInterface failed: 0x%08x\n", hr);

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd2, &surf7, NULL);
    ok(hr == DDERR_INVALIDCAPS, "IDirectDraw7_CreateSurface didn't return %x08x, but %x08x\n",
       DDERR_INVALIDCAPS, hr);

    IDirectDraw7_Release(dd7);
}

static void BackBufferAttachmentFlipTest(void)
{
    HRESULT hr;
    IDirectDrawSurface *surface1, *surface2, *surface3, *surface4;
    DDSURFACEDESC ddsd;
    HWND window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            100, 100, 160, 160, NULL, NULL, NULL, NULL);

    hr = IDirectDraw_SetCooperativeLevel(lpDD, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "SetCooperativeLevel returned %08x\n", hr);

    /* Perform attachment tests on a back-buffer */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
    ddsd.dwWidth = GetSystemMetrics(SM_CXSCREEN);
    ddsd.dwHeight = GetSystemMetrics(SM_CYSCREEN);
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface2, NULL);
    ok(SUCCEEDED(hr), "CreateSurface returned: %x\n",hr);

    if (surface2 != NULL)
    {
        /* Try a single primary and a two back buffers */
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface1, NULL);
        ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
        ddsd.dwWidth = GetSystemMetrics(SM_CXSCREEN);
        ddsd.dwHeight = GetSystemMetrics(SM_CYSCREEN);
        hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface3, NULL);
        ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

        /* This one has a different size */
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
        ddsd.dwWidth = 128;
        ddsd.dwHeight = 128;
        hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface4, NULL);
        ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

        hr = IDirectDrawSurface_AddAttachedSurface(surface1, surface2);
        todo_wine ok(hr == DD_OK || broken(hr == DDERR_CANNOTATTACHSURFACE),
           "Attaching a back buffer to a front buffer returned %08x\n", hr);
        if(SUCCEEDED(hr))
        {
            /* Try flipping the surfaces */
            hr = IDirectDrawSurface_Flip(surface1, NULL, DDFLIP_WAIT);
            todo_wine ok(hr == DD_OK, "IDirectDrawSurface_Flip returned 0x%08x\n", hr);
            hr = IDirectDrawSurface_Flip(surface2, NULL, DDFLIP_WAIT);
            todo_wine ok(hr == DDERR_NOTFLIPPABLE, "IDirectDrawSurface_Flip returned 0x%08x\n", hr);

            /* Try the reverse without detaching first */
            hr = IDirectDrawSurface_AddAttachedSurface(surface2, surface1);
            ok(hr == DDERR_SURFACEALREADYATTACHED, "Attaching an attached surface to its attachee returned %08x\n", hr);
            hr = IDirectDrawSurface_DeleteAttachedSurface(surface1, 0, surface2);
            ok(hr == DD_OK, "DeleteAttachedSurface failed with %08x\n", hr);
        }
        hr = IDirectDrawSurface_AddAttachedSurface(surface2, surface1);
        todo_wine ok(hr == DD_OK || broken(hr == DDERR_CANNOTATTACHSURFACE),
           "Attaching a front buffer to a back buffer returned %08x\n", hr);
        if(SUCCEEDED(hr))
        {
            /* Try flipping the surfaces */
            hr = IDirectDrawSurface_Flip(surface1, NULL, DDFLIP_WAIT);
            todo_wine ok(hr == DD_OK, "IDirectDrawSurface_Flip returned 0x%08x\n", hr);
            hr = IDirectDrawSurface_Flip(surface2, NULL, DDFLIP_WAIT);
            todo_wine ok(hr == DDERR_NOTFLIPPABLE, "IDirectDrawSurface_Flip returned 0x%08x\n", hr);

            /* Try to detach reversed */
            hr = IDirectDrawSurface_DeleteAttachedSurface(surface1, 0, surface2);
            ok(hr == DDERR_CANNOTDETACHSURFACE, "DeleteAttachedSurface returned %08x\n", hr);
            /* Now the proper detach */
            hr = IDirectDrawSurface_DeleteAttachedSurface(surface2, 0, surface1);
            ok(hr == DD_OK, "DeleteAttachedSurface failed with %08x\n", hr);
        }
        hr = IDirectDrawSurface_AddAttachedSurface(surface2, surface3);
        todo_wine ok(hr == DD_OK || broken(hr == DDERR_CANNOTATTACHSURFACE),
           "Attaching a back buffer to another back buffer returned %08x\n", hr);
        if(SUCCEEDED(hr))
        {
            /* Try flipping the surfaces */
            hr = IDirectDrawSurface_Flip(surface3, NULL, DDFLIP_WAIT);
            todo_wine ok(hr == DD_OK, "IDirectDrawSurface_Flip returned 0x%08x\n", hr);
            hr = IDirectDrawSurface_Flip(surface2, NULL, DDFLIP_WAIT);
            todo_wine ok(hr == DDERR_NOTFLIPPABLE, "IDirectDrawSurface_Flip returned 0x%08x\n", hr);
            hr = IDirectDrawSurface_Flip(surface1, NULL, DDFLIP_WAIT);
            ok(hr == DDERR_NOTFLIPPABLE, "IDirectDrawSurface_Flip returned 0x%08x\n", hr);

            hr = IDirectDrawSurface_DeleteAttachedSurface(surface2, 0, surface3);
            ok(hr == DD_OK, "DeleteAttachedSurface failed with %08x\n", hr);
        }
        hr = IDirectDrawSurface_AddAttachedSurface(surface1, surface4);
        ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a back buffer to a front buffer of different size returned %08x\n", hr);
        hr = IDirectDrawSurface_AddAttachedSurface(surface4, surface1);
        ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a front buffer to a back buffer of different size returned %08x\n", hr);

        IDirectDrawSurface_Release(surface4);
        IDirectDrawSurface_Release(surface3);
        IDirectDrawSurface_Release(surface2);
        IDirectDrawSurface_Release(surface1);
    }

    hr =IDirectDraw_SetCooperativeLevel(lpDD, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "SetCooperativeLevel returned %08x\n", hr);

    DestroyWindow(window);
}

static void CreateSurfaceBadCapsSizeTest(void)
{
    DDSURFACEDESC ddsd_ok;
    DDSURFACEDESC ddsd_bad1;
    DDSURFACEDESC ddsd_bad2;
    DDSURFACEDESC ddsd_bad3;
    DDSURFACEDESC ddsd_bad4;
    DDSURFACEDESC2 ddsd2_ok;
    DDSURFACEDESC2 ddsd2_bad1;
    DDSURFACEDESC2 ddsd2_bad2;
    DDSURFACEDESC2 ddsd2_bad3;
    DDSURFACEDESC2 ddsd2_bad4;
    IDirectDrawSurface *surf;
    IDirectDrawSurface4 *surf4;
    IDirectDrawSurface7 *surf7;
    HRESULT hr;
    IDirectDraw2 *dd2;
    IDirectDraw4 *dd4;
    IDirectDraw7 *dd7;

    const DWORD caps = DDSCAPS_OFFSCREENPLAIN;

    memset(&ddsd_ok, 0, sizeof(ddsd_ok));
    ddsd_ok.dwSize = sizeof(ddsd_ok);
    ddsd_ok.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd_ok.dwWidth = 64;
    ddsd_ok.dwHeight = 64;
    ddsd_ok.ddsCaps.dwCaps = caps;
    ddsd_bad1 = ddsd_ok;
    ddsd_bad1.dwSize--;
    ddsd_bad2 = ddsd_ok;
    ddsd_bad2.dwSize++;
    ddsd_bad3 = ddsd_ok;
    ddsd_bad3.dwSize = 0;
    ddsd_bad4 = ddsd_ok;
    ddsd_bad4.dwSize = sizeof(DDSURFACEDESC2);

    memset(&ddsd2_ok, 0, sizeof(ddsd2_ok));
    ddsd2_ok.dwSize = sizeof(ddsd2_ok);
    ddsd2_ok.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd2_ok.dwWidth = 64;
    ddsd2_ok.dwHeight = 64;
    ddsd2_ok.ddsCaps.dwCaps = caps;
    ddsd2_bad1 = ddsd2_ok;
    ddsd2_bad1.dwSize--;
    ddsd2_bad2 = ddsd2_ok;
    ddsd2_bad2.dwSize++;
    ddsd2_bad3 = ddsd2_ok;
    ddsd2_bad3.dwSize = 0;
    ddsd2_bad4 = ddsd2_ok;
    ddsd2_bad4.dwSize = sizeof(DDSURFACEDESC);

    hr = IDirectDraw_CreateSurface(lpDD, &ddsd_ok, &surf, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw_CreateSurface failed: 0x%08x\n", hr);
    IDirectDrawSurface_Release(surf);

    hr = IDirectDraw_CreateSurface(lpDD, &ddsd_bad1, &surf, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd_bad2, &surf, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd_bad3, &surf, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd_bad4, &surf, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw_CreateSurface(lpDD, NULL, &surf, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw2, (void **) &dd2);
    ok(SUCCEEDED(hr), "IDirectDraw_QueryInterface failed: 0x%08x\n", hr);

    hr = IDirectDraw2_CreateSurface(dd2, &ddsd_ok, &surf, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw2_CreateSurface failed: 0x%08x\n", hr);
    IDirectDrawSurface_Release(surf);

    hr = IDirectDraw2_CreateSurface(dd2, &ddsd_bad1, &surf, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw2_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw2_CreateSurface(dd2, &ddsd_bad2, &surf, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw2_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw2_CreateSurface(dd2, &ddsd_bad3, &surf, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw2_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw2_CreateSurface(dd2, &ddsd_bad4, &surf, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw2_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw2_CreateSurface(dd2, NULL, &surf, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw2_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);

    IDirectDraw2_Release(dd2);

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw4, (void **) &dd4);
    ok(SUCCEEDED(hr), "IDirectDraw_QueryInterface failed: 0x%08x\n", hr);

    hr = IDirectDraw4_CreateSurface(dd4, &ddsd2_ok, &surf4, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw4_CreateSurface failed: 0x%08x\n", hr);
    IDirectDrawSurface4_Release(surf4);

    hr = IDirectDraw4_CreateSurface(dd4, &ddsd2_bad1, &surf4, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw4_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw4_CreateSurface(dd4, &ddsd2_bad2, &surf4, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw4_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw4_CreateSurface(dd4, &ddsd2_bad3, &surf4, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw4_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw4_CreateSurface(dd4, &ddsd2_bad4, &surf4, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw4_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw4_CreateSurface(dd4, NULL, &surf4, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw4_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);

    IDirectDraw4_Release(dd4);

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    ok(SUCCEEDED(hr), "IDirectDraw_QueryInterface failed: 0x%08x\n", hr);

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd2_ok, &surf7, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw7_CreateSurface failed: 0x%08x\n", hr);
    IDirectDrawSurface7_Release(surf7);

    hr = IDirectDraw7_CreateSurface(dd7, &ddsd2_bad1, &surf7, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw7_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw7_CreateSurface(dd7, &ddsd2_bad2, &surf7, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw7_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw7_CreateSurface(dd7, &ddsd2_bad3, &surf7, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw7_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw7_CreateSurface(dd7, &ddsd2_bad4, &surf7, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw7_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);
    hr = IDirectDraw7_CreateSurface(dd7, NULL, &surf7, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDraw7_CreateSurface didn't return 0x%08x, but 0x%08x\n",
       DDERR_INVALIDPARAMS, hr);

    IDirectDraw7_Release(dd7);
}

static void reset_ddsd(DDSURFACEDESC *ddsd)
{
    memset(ddsd, 0, sizeof(*ddsd));
    ddsd->dwSize = sizeof(*ddsd);
}

static void no_ddsd_caps_test(void)
{
    DDSURFACEDESC ddsd;
    HRESULT hr;
    IDirectDrawSurface *surface;

    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw_CreateSurface failed, hr %#x.\n", hr);
    reset_ddsd(&ddsd);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &ddsd);
    IDirectDrawSurface_Release(surface);
    ok(ddsd.dwFlags & DDSD_CAPS, "DDSD_CAPS is not set\n");
    ok(ddsd.ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN, "DDSCAPS_OFFSCREENPLAIN is not set\n");

    reset_ddsd(&ddsd);
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw_CreateSurface failed, hr %#x.\n", hr);
    reset_ddsd(&ddsd);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &ddsd);
    IDirectDrawSurface_Release(surface);
    ok(ddsd.dwFlags & DDSD_CAPS, "DDSD_CAPS is not set\n");
    ok(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE, "DDSCAPS_OFFSCREENPLAIN is not set\n");

    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_SYSTEMMEMORY;
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface, NULL);
    ok(hr == DDERR_INVALIDCAPS, "IDirectDraw_CreateSurface returned %#x, expected DDERR_INVALIDCAPS.\n", hr);
}

static void dump_format(const DDPIXELFORMAT *fmt)
{
    trace("dwFlags %08x, FourCC %08x, dwZBufferBitDepth %u, stencil %u\n", fmt->dwFlags, fmt->dwFourCC,
          U1(*fmt).dwZBufferBitDepth, U2(*fmt).dwStencilBitDepth);
    trace("dwZBitMask %08x, dwStencilBitMask %08x, dwRGBZBitMask %08x\n", U3(*fmt).dwZBitMask,
          U4(*fmt).dwStencilBitMask, U5(*fmt).dwRGBZBitMask);
}

static void zbufferbitdepth_test(void)
{
    enum zfmt_succeed
    {
        ZFMT_SUPPORTED_ALWAYS,
        ZFMT_SUPPORTED_NEVER,
        ZFMT_SUPPORTED_HWDEPENDENT
    };
    struct
    {
        DWORD depth;
        enum zfmt_succeed supported;
        DDPIXELFORMAT pf;
    }
    test_data[] =
    {
        {
            16, ZFMT_SUPPORTED_ALWAYS,
            {
                sizeof(DDPIXELFORMAT), DDPF_ZBUFFER, 0,
                {16}, {0}, {0x0000ffff}, {0x00000000}, {0x00000000}
            }
        },
        {
            24, ZFMT_SUPPORTED_HWDEPENDENT,
            {
                sizeof(DDPIXELFORMAT), DDPF_ZBUFFER, 0,
                {24}, {0}, {0x00ffffff}, {0x00000000}, {0x00000000}
            }
        },
        {
            32, ZFMT_SUPPORTED_HWDEPENDENT,
            {
                sizeof(DDPIXELFORMAT), DDPF_ZBUFFER, 0,
                {32}, {0}, {0xffffffff}, {0x00000000}, {0x00000000}
            }
        },
        /* Returns DDERR_INVALIDPARAMS instead of DDERR_INVALIDPIXELFORMAT.
         * Disabled for now
        {
            0, ZFMT_SUPPORTED_NEVER
        },
        */
        {
            15, ZFMT_SUPPORTED_NEVER
        },
        {
            28, ZFMT_SUPPORTED_NEVER
        },
        {
            40, ZFMT_SUPPORTED_NEVER
        },
    };

    DDSURFACEDESC ddsd;
    IDirectDrawSurface *surface;
    HRESULT hr;
    unsigned int i;
    DDCAPS caps;

    memset(&caps, 0, sizeof(caps));
    caps.dwSize = sizeof(caps);
    hr = IDirectDraw_GetCaps(lpDD, &caps, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw_GetCaps failed, hr %#x.\n", hr);
    if (!(caps.ddsCaps.dwCaps & DDSCAPS_ZBUFFER))
    {
        skip("Z buffers not supported, skipping DDSD_ZBUFFERBITDEPTH test\n");
        return;
    }

    for (i = 0; i < (sizeof(test_data) / sizeof(*test_data)); i++)
    {
        reset_ddsd(&ddsd);
        ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_ZBUFFERBITDEPTH;
        ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
        ddsd.dwWidth = 256;
        ddsd.dwHeight = 256;
        U2(ddsd).dwZBufferBitDepth = test_data[i].depth;

        hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface, NULL);
        if (test_data[i].supported == ZFMT_SUPPORTED_ALWAYS)
        {
            ok(SUCCEEDED(hr), "IDirectDraw_CreateSurface failed, hr %#x.\n", hr);
        }
        else if (test_data[i].supported == ZFMT_SUPPORTED_NEVER)
        {
            ok(hr == DDERR_INVALIDPIXELFORMAT, "IDirectDraw_CreateSurface returned %#x, expected %x.\n",
               hr, DDERR_INVALIDPIXELFORMAT);
        }
        if (!surface) continue;

        reset_ddsd(&ddsd);
        hr = IDirectDrawSurface_GetSurfaceDesc(surface, &ddsd);
        ok(SUCCEEDED(hr), "IDirectDrawSurface_GetSurfaceDesc failed, hr %#x.\n", hr);
        IDirectDrawSurface_Release(surface);

        ok(ddsd.dwFlags & DDSD_ZBUFFERBITDEPTH, "DDSD_ZBUFFERBITDEPTH is not set\n");
        ok(!(ddsd.dwFlags & DDSD_PIXELFORMAT), "DDSD_PIXELFORMAT is set\n");
        /* Yet the ddpfPixelFormat member contains valid data */
        if (memcmp(&ddsd.ddpfPixelFormat, &test_data[i].pf, ddsd.ddpfPixelFormat.dwSize))
        {
            ok(0, "Unexpected format for depth %u\n", test_data[i].depth);
            dump_format(&ddsd.ddpfPixelFormat);
        }
    }

    /* DDSD_ZBUFFERBITDEPTH vs DDSD_PIXELFORMAT? */
    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_ZBUFFERBITDEPTH;
    ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    ddsd.dwWidth = 256;
    ddsd.dwHeight = 256;
    U2(ddsd).dwZBufferBitDepth = 24;
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
    U1(ddsd.ddpfPixelFormat).dwZBufferBitDepth = 16;
    U3(ddsd.ddpfPixelFormat).dwZBitMask = 0x0000ffff;

    surface = NULL;
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw_CreateSurface failed, hr %#x.\n", hr);
    if (!surface) return;
    reset_ddsd(&ddsd);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &ddsd);
    ok(SUCCEEDED(hr), "IDirectDrawSurface_GetSurfaceDesc failed, hr %#x.\n", hr);
    IDirectDrawSurface_Release(surface);
    ok(U1(ddsd.ddpfPixelFormat).dwZBufferBitDepth == 16, "Expected a 16bpp depth buffer, got %ubpp\n",
       U1(ddsd.ddpfPixelFormat).dwZBufferBitDepth);
    ok(ddsd.dwFlags & DDSD_ZBUFFERBITDEPTH, "DDSD_ZBUFFERBITDEPTH is not set\n");
    ok(!(ddsd.dwFlags & DDSD_PIXELFORMAT), "DDSD_PIXELFORMAT is set\n");
    ok(U2(ddsd).dwZBufferBitDepth == 16, "Expected dwZBufferBitDepth=16, got %u\n",
       U2(ddsd).dwZBufferBitDepth);

    /* DDSD_PIXELFORMAT vs invalid ZBUFFERBITDEPTH */
    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_ZBUFFERBITDEPTH | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    ddsd.dwWidth = 256;
    ddsd.dwHeight = 256;
    U2(ddsd).dwZBufferBitDepth = 40;
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
    U1(ddsd.ddpfPixelFormat).dwZBufferBitDepth = 16;
    U3(ddsd.ddpfPixelFormat).dwZBitMask = 0x0000ffff;
    surface = NULL;
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw_CreateSurface failed, hr %#x.\n", hr);
    if (surface) IDirectDrawSurface_Release(surface);

    /* Create a PIXELFORMAT-only surface, see if ZBUFFERBITDEPTH is set */
    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    ddsd.dwWidth = 256;
    ddsd.dwHeight = 256;
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
    U1(ddsd.ddpfPixelFormat).dwZBufferBitDepth = 16;
    U3(ddsd.ddpfPixelFormat).dwZBitMask = 0x0000ffff;
    surface = NULL;
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "IDirectDrawSurface_GetSurfaceDesc failed, hr %#x.\n", hr);
    reset_ddsd(&ddsd);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &ddsd);
    ok(SUCCEEDED(hr), "IDirectDrawSurface_GetSurfaceDesc failed, hr %#x.\n", hr);
    IDirectDrawSurface_Release(surface);
    ok(U1(ddsd.ddpfPixelFormat).dwZBufferBitDepth == 16, "Expected a 16bpp depth buffer, got %ubpp\n",
       U1(ddsd.ddpfPixelFormat).dwZBufferBitDepth);
    ok(ddsd.dwFlags & DDSD_ZBUFFERBITDEPTH, "DDSD_ZBUFFERBITDEPTH is not set\n");
    ok(!(ddsd.dwFlags & DDSD_PIXELFORMAT), "DDSD_PIXELFORMAT is set\n");
    ok(U2(ddsd).dwZBufferBitDepth == 16, "Expected dwZBufferBitDepth=16, got %u\n",
       U2(ddsd).dwZBufferBitDepth);
}

static void test_ddsd(DDSURFACEDESC *ddsd, BOOL expect_pf, BOOL expect_zd, const char *name, DWORD z_bit_depth)
{
    IDirectDrawSurface *surface;
    IDirectDrawSurface7 *surface7;
    HRESULT hr;
    DDSURFACEDESC out;
    DDSURFACEDESC2 out2;

    hr = IDirectDraw_CreateSurface(lpDD, ddsd, &surface, NULL);
    if (hr == DDERR_NOZBUFFERHW)
    {
        skip("Z buffers not supported, skipping Z flag test\n");
        return;
    }
    ok(SUCCEEDED(hr), "IDirectDraw_CreateSurface failed, hr %#x.\n", hr);
    hr = IDirectDrawSurface_QueryInterface(surface, &IID_IDirectDrawSurface7, (void **) &surface7);
    ok(SUCCEEDED(hr), "IDirectDrawSurface_QueryInterface failed, hr %#x.\n", hr);

    reset_ddsd(&out);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &out);
    ok(SUCCEEDED(hr), "IDirectDrawSurface_GetSurfaceDesc failed, hr %#x.\n", hr);
    memset(&out2, 0, sizeof(out2));
    out2.dwSize = sizeof(out2);
    hr = IDirectDrawSurface7_GetSurfaceDesc(surface7, &out2);
    ok(SUCCEEDED(hr), "IDirectDrawSurface_GetSurfaceDesc failed, hr %#x.\n", hr);

    if (expect_pf)
    {
        ok(out.dwFlags & DDSD_PIXELFORMAT, "%s surface: Expected DDSD_PIXELFORMAT to be set\n", name);
        ok(out2.dwFlags & DDSD_PIXELFORMAT,
                "%s surface: Expected DDSD_PIXELFORMAT to be set in DDSURFACEDESC2\n", name);
    }
    else
    {
        ok(!(out.dwFlags & DDSD_PIXELFORMAT), "%s surface: Expected DDSD_PIXELFORMAT not to be set\n", name);
        ok(out2.dwFlags & DDSD_PIXELFORMAT,
                "%s surface: Expected DDSD_PIXELFORMAT to be set in DDSURFACEDESC2\n", name);
    }
    if (expect_zd)
    {
        ok(out.dwFlags & DDSD_ZBUFFERBITDEPTH, "%s surface: Expected DDSD_ZBUFFERBITDEPTH to be set\n", name);
        ok(U2(out).dwZBufferBitDepth == z_bit_depth, "ZBufferBitDepth is %u, expected %u\n",
                U2(out).dwZBufferBitDepth, z_bit_depth);
        ok(!(out2.dwFlags & DDSD_ZBUFFERBITDEPTH),
                "%s surface: Did not expect DDSD_ZBUFFERBITDEPTH to be set in DDSURFACEDESC2\n", name);
        /* dwMipMapCount and dwZBufferBitDepth share the same union */
        ok(U2(out2).dwMipMapCount == 0, "dwMipMapCount is %u, expected 0\n", U2(out2).dwMipMapCount);
    }
    else
    {
        ok(!(out.dwFlags & DDSD_ZBUFFERBITDEPTH), "%s surface: Expected DDSD_ZBUFFERBITDEPTH not to be set\n", name);
        ok(U2(out).dwZBufferBitDepth == 0, "ZBufferBitDepth is %u, expected 0\n", U2(out).dwZBufferBitDepth);
        ok(!(out2.dwFlags & DDSD_ZBUFFERBITDEPTH),
                "%s surface: Did not expect DDSD_ZBUFFERBITDEPTH to be set in DDSURFACEDESC2\n", name);
        ok(U2(out2).dwMipMapCount == 0, "dwMipMapCount is %u, expected 0\n", U2(out2).dwMipMapCount);
    }

    reset_ddsd(&out);
    hr = IDirectDrawSurface_Lock(surface, NULL, &out, 0, NULL);
    if (SUCCEEDED(hr))
    {
        hr = IDirectDrawSurface_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "IDirectDrawSurface_GetSurfaceDesc failed, hr %#x.\n", hr);

        /* DDSD_ZBUFFERBITDEPTH is never set on Nvidia, but follows GetSurfaceDesc rules on AMD */
        if (!expect_zd)
        {
            ok(!(out.dwFlags & DDSD_ZBUFFERBITDEPTH),
                "Lock %s surface: Expected DDSD_ZBUFFERBITDEPTH not to be set\n", name);
        }

        /* DDSD_PIXELFORMAT follows GetSurfaceDesc rules */
        if (expect_pf)
        {
            ok(out.dwFlags & DDSD_PIXELFORMAT, "%s surface: Expected DDSD_PIXELFORMAT to be set\n", name);
        }
        else
        {
            ok(!(out.dwFlags & DDSD_PIXELFORMAT),
                "Lock %s surface: Expected DDSD_PIXELFORMAT not to be set\n", name);
        }
        if (out.dwFlags & DDSD_ZBUFFERBITDEPTH)
            ok(U2(out).dwZBufferBitDepth == z_bit_depth, "ZBufferBitDepth is %u, expected %u\n",
                    U2(out).dwZBufferBitDepth, z_bit_depth);
        else
            ok(U2(out).dwZBufferBitDepth == 0, "ZBufferBitDepth is %u, expected 0\n", U2(out).dwZBufferBitDepth);
    }

    hr = IDirectDrawSurface7_Lock(surface7, NULL, &out2, 0, NULL);
    ok(SUCCEEDED(hr), "IDirectDrawSurface7_Lock failed, hr %#x.\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDirectDrawSurface7_Unlock(surface7, NULL);
        ok(SUCCEEDED(hr), "IDirectDrawSurface7_Unlock failed, hr %#x.\n", hr);
        /* DDSD_PIXELFORMAT is always set, DDSD_ZBUFFERBITDEPTH never */
        ok(out2.dwFlags & DDSD_PIXELFORMAT,
                "Lock %s surface: Expected DDSD_PIXELFORMAT to be set in DDSURFACEDESC2\n", name);
        ok(!(out2.dwFlags & DDSD_ZBUFFERBITDEPTH),
                "Lock %s surface: Did not expect DDSD_ZBUFFERBITDEPTH to be set in DDSURFACEDESC2\n", name);
        ok(U2(out2).dwMipMapCount == 0, "dwMipMapCount is %u, expected 0\n", U2(out2).dwMipMapCount);
    }

    IDirectDrawSurface7_Release(surface7);
    IDirectDrawSurface_Release(surface);
}

static void pixelformat_flag_test(void)
{
    DDSURFACEDESC ddsd;
    DDCAPS caps;
    HRESULT hr;

    memset(&caps, 0, sizeof(caps));
    caps.dwSize = sizeof(caps);
    hr = IDirectDraw_GetCaps(lpDD, &caps, NULL);
    ok(SUCCEEDED(hr), "IDirectDraw_GetCaps failed, hr %#x.\n", hr);
    if (!(caps.ddsCaps.dwCaps & DDSCAPS_ZBUFFER))
    {
        skip("Z buffers not supported, skipping DDSD_PIXELFORMAT test\n");
        return;
    }

    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = 64;
    ddsd.dwHeight = 64;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    test_ddsd(&ddsd, TRUE, FALSE, "offscreen plain", ~0U);

    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    test_ddsd(&ddsd, TRUE, FALSE, "primary", ~0U);

    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_ZBUFFERBITDEPTH;
    ddsd.dwWidth = 64;
    ddsd.dwHeight = 64;
    U2(ddsd).dwZBufferBitDepth = 16;
    ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    test_ddsd(&ddsd, FALSE, TRUE, "Z buffer", 16);
}

static BOOL fourcc_supported(DWORD fourcc, DWORD caps)
{
    DDSURFACEDESC ddsd;
    HRESULT hr;
    IDirectDrawSurface *surface;

    reset_ddsd(&ddsd);
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd.dwWidth = 4;
    ddsd.dwHeight = 4;
    ddsd.ddsCaps.dwCaps = caps;
    ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
    ddsd.ddpfPixelFormat.dwFourCC = fourcc;
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface, NULL);
    if (FAILED(hr))
    {
        return FALSE;
    }
    IDirectDrawSurface_Release(surface);
    return TRUE;
}

static void partial_block_lock_test(void)
{
    IDirectDrawSurface7 *surface;
    HRESULT hr;
    DDSURFACEDESC2 ddsd;
    IDirectDraw7 *dd7;
    const struct
    {
        DWORD caps, caps2;
        const char *name;
        BOOL success;
    }
    pools[] =
    {
        {
            DDSCAPS_VIDEOMEMORY, 0,
            "D3DPOOL_DEFAULT", FALSE
        },
        {
            DDSCAPS_SYSTEMMEMORY, 0,
            "D3DPOOL_SYSTEMMEM", TRUE
        },
        {
            0, DDSCAPS2_TEXTUREMANAGE,
            "D3DPOOL_MANAGED", TRUE
        }
    };
    const struct
    {
        DWORD fourcc;
        DWORD caps;
        const char *name;
        unsigned int block_width;
        unsigned int block_height;
    }
    formats[] =
    {
        {MAKEFOURCC('D','X','T','1'), DDSCAPS_TEXTURE, "D3DFMT_DXT1", 4, 4},
        {MAKEFOURCC('D','X','T','2'), DDSCAPS_TEXTURE, "D3DFMT_DXT2", 4, 4},
        {MAKEFOURCC('D','X','T','3'), DDSCAPS_TEXTURE, "D3DFMT_DXT3", 4, 4},
        {MAKEFOURCC('D','X','T','4'), DDSCAPS_TEXTURE, "D3DFMT_DXT4", 4, 4},
        {MAKEFOURCC('D','X','T','5'), DDSCAPS_TEXTURE, "D3DFMT_DXT5", 4, 4},
        /* ATI2N surfaces aren't available in ddraw */
        {MAKEFOURCC('U','Y','V','Y'), DDSCAPS_OVERLAY, "D3DFMT_UYVY", 2, 1},
        {MAKEFOURCC('Y','U','Y','2'), DDSCAPS_OVERLAY, "D3DFMT_YUY2", 2, 1},
    };
    unsigned int i, j;
    RECT rect;

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    ok(SUCCEEDED(hr), "QueryInterface failed, hr %#x.\n", hr);

    for (i = 0; i < sizeof(formats) / sizeof(formats[0]); i++)
    {
        if (!fourcc_supported(formats[i].fourcc, formats[i].caps | DDSCAPS_VIDEOMEMORY))
        {
            skip("%s surfaces not supported, skipping partial block lock test\n", formats[i].name);
            continue;
        }

        for (j = 0; j < (sizeof(pools) / sizeof(*pools)); j++)
        {
            if (formats[i].caps & DDSCAPS_OVERLAY && !(pools[j].caps & DDSCAPS_VIDEOMEMORY))
                continue;

            memset(&ddsd, 0, sizeof(ddsd));
            ddsd.dwSize = sizeof(ddsd);
            ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
            ddsd.dwWidth = 128;
            ddsd.dwHeight = 128;
            ddsd.ddsCaps.dwCaps = pools[j].caps | formats[i].caps;
            ddsd.ddsCaps.dwCaps2 = pools[j].caps2;
            U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_FOURCC;
            U4(ddsd).ddpfPixelFormat.dwFourCC = formats[i].fourcc;
            hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface, NULL);
            ok(SUCCEEDED(hr), "CreateSurface failed, hr %#x, format %s, pool %s\n",
                hr, formats[i].name, pools[j].name);

            /* All Windows versions allow partial block locks with DDSCAPS_SYSTEMMEMORY and
             * DDSCAPS2_TEXTUREMANAGE, just like in d3d8 and d3d9. Windows XP also allows those locks
             * with DDSCAPS_VIDEOMEMORY. Windows Vista and Windows 7 disallow partial locks of vidmem
             * surfaces, making the ddraw behavior consistent with d3d8 and 9.
             *
             * Mark the Windows XP behavior as broken until we find an application that needs it */
            if (formats[i].block_width > 1)
            {
                SetRect(&rect, formats[i].block_width >> 1, 0, formats[i].block_width, formats[i].block_height);
                hr = IDirectDrawSurface7_Lock(surface, &rect, &ddsd, 0, NULL);
                ok(SUCCEEDED(hr) == pools[j].success || broken(SUCCEEDED(hr)),
                        "Partial block lock %s, expected %s, format %s, pool %s\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed", pools[j].success ? "success" : "failure",
                        formats[i].name, pools[j].name);
                if (SUCCEEDED(hr))
                {
                    hr = IDirectDrawSurface7_Unlock(surface, NULL);
                    ok(SUCCEEDED(hr), "Unlock failed, hr %#x.\n", hr);
                }

                SetRect(&rect, 0, 0, formats[i].block_width >> 1, formats[i].block_height);
                hr = IDirectDrawSurface7_Lock(surface, &rect, &ddsd, 0, NULL);
                ok(SUCCEEDED(hr) == pools[j].success || broken(SUCCEEDED(hr)),
                        "Partial block lock %s, expected %s, format %s, pool %s\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed", pools[j].success ? "success" : "failure",
                        formats[i].name, pools[j].name);
                if (SUCCEEDED(hr))
                {
                    hr = IDirectDrawSurface7_Unlock(surface, NULL);
                    ok(SUCCEEDED(hr), "Unlock failed, hr %#x.\n", hr);
                }
            }

            if (formats[i].block_height > 1)
            {
                SetRect(&rect, 0, formats[i].block_height >> 1, formats[i].block_width, formats[i].block_height);
                hr = IDirectDrawSurface7_Lock(surface, &rect, &ddsd, 0, NULL);
                ok(SUCCEEDED(hr) == pools[j].success || broken(SUCCEEDED(hr)),
                        "Partial block lock %s, expected %s, format %s, pool %s\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed", pools[j].success ? "success" : "failure",
                        formats[i].name, pools[j].name);
                if (SUCCEEDED(hr))
                {
                    hr = IDirectDrawSurface7_Unlock(surface, NULL);
                    ok(SUCCEEDED(hr), "Unlock failed, hr %#x.\n", hr);
                }

                SetRect(&rect, 0, 0, formats[i].block_width, formats[i].block_height >> 1);
                hr = IDirectDrawSurface7_Lock(surface, &rect, &ddsd, 0, NULL);
                ok(SUCCEEDED(hr) == pools[j].success || broken(SUCCEEDED(hr)),
                        "Partial block lock %s, expected %s, format %s, pool %s\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed", pools[j].success ? "success" : "failure",
                        formats[i].name, pools[j].name);
                if (SUCCEEDED(hr))
                {
                    hr = IDirectDrawSurface7_Unlock(surface, NULL);
                    ok(SUCCEEDED(hr), "Unlock failed, hr %#x.\n", hr);
                }
            }

            SetRect(&rect, 0, 0, formats[i].block_width, formats[i].block_height);
            hr = IDirectDrawSurface7_Lock(surface, &rect, &ddsd, 0, NULL);
            ok(SUCCEEDED(hr), "Full block lock returned %08x, expected %08x, format %s, pool %s\n",
                    hr, DD_OK, formats[i].name, pools[j].name);
            if (SUCCEEDED(hr))
            {
                hr = IDirectDrawSurface7_Unlock(surface, NULL);
                ok(SUCCEEDED(hr), "Unlock failed, hr %#x.\n", hr);
            }

            IDirectDrawSurface7_Release(surface);
        }
    }

    IDirectDraw7_Release(dd7);
}

static void create_surface_test(void)
{
    HRESULT hr;
    IDirectDraw2 *ddraw2;
    IDirectDraw4 *ddraw4;
    IDirectDraw7 *ddraw7;
    IDirectDrawSurface *surface;
    IDirectDrawSurface4 *surface4;
    IDirectDrawSurface7 *surface7;

    hr = IDirectDraw_CreateSurface(lpDD, NULL, &surface, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "CreateSurface(ddsd=NULL) returned %#x,"
            " expected %#x.\n", hr, DDERR_INVALIDPARAMS);

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw2, (void **) &ddraw2);
    ok(SUCCEEDED(hr), "QueryInterface failed, hr %#x.\n", hr);

    hr = IDirectDraw2_CreateSurface(ddraw2, NULL, &surface, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "CreateSurface(ddsd=NULL) returned %#x,"
            " expected %#x.\n", hr, DDERR_INVALIDPARAMS);

    IDirectDraw2_Release(ddraw2);

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw4, (void **) &ddraw4);
    ok(SUCCEEDED(hr), "QueryInterface failed, hr %#x.\n", hr);

    hr = IDirectDraw4_CreateSurface(ddraw4, NULL, &surface4, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "CreateSurface(ddsd=NULL) returned %#x,"
            " expected %#x.\n", hr, DDERR_INVALIDPARAMS);

    IDirectDraw4_Release(ddraw4);

    if (!pDirectDrawCreateEx)
    {
        skip("DirectDrawCreateEx not available, skipping IDirectDraw7 tests.\n");
        return;
    }
    hr = pDirectDrawCreateEx(NULL, (void **) &ddraw7, &IID_IDirectDraw7, NULL);
    ok(SUCCEEDED(hr), "DirectDrawCreateEx failed, hr %#x.\n", hr);

    hr = IDirectDraw7_CreateSurface(ddraw7, NULL, &surface7, NULL);
    ok(hr == DDERR_NOCOOPERATIVELEVELSET, "CreateSurface(ddsd=NULL, pre-SCL) returned %#x,"
            " expected %#x.\n", hr, DDERR_NOCOOPERATIVELEVELSET);

    hr = IDirectDraw7_SetCooperativeLevel(ddraw7, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "SetCooperativeLevel failed, hr %#x.\n", hr);

    hr = IDirectDraw7_CreateSurface(ddraw7, NULL, &surface7, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "CreateSurface(ddsd=NULL) returned %#x,"
            " expected %#x.\n", hr, DDERR_INVALIDPARAMS);

    IDirectDraw7_Release(ddraw7);
}

START_TEST(dsurface)
{
    HRESULT ret;
    IDirectDraw4 *dd4;

    HMODULE ddraw_mod = GetModuleHandleA("ddraw.dll");
    pDirectDrawCreateEx = (void *) GetProcAddress(ddraw_mod, "DirectDrawCreateEx");

    if (!CreateDirectDraw())
        return;

    ret = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw4, (void **) &dd4);
    if (ret == E_NOINTERFACE)
    {
        win_skip("DirectDraw4 and higher are not supported\n");
        ReleaseDirectDraw();
        return;
    }
    IDirectDraw_Release(dd4);

    if(!can_create_primary_surface())
    {
        skip("Unable to create primary surface\n");
        return;
    }

    ddcaps.dwSize = sizeof(DDCAPS);
    ret = IDirectDraw_GetCaps(lpDD, &ddcaps, NULL);
    if (ret != DD_OK)
    {
        skip("IDirectDraw_GetCaps failed with %08x\n", ret);
        return;
    }

    MipMapCreationTest();
    SrcColorKey32BlitTest();
    QueryInterface();
    GetDDInterface_1();
    GetDDInterface_2();
    GetDDInterface_4();
    GetDDInterface_7();
    EnumTest();
    CubeMapTest();
    test_lockrect_invalid();
    CompressedTest();
    SizeTest();
    BltParamTest();
    StructSizeTest();
    PaletteTest();
    SurfaceCapsTest();
    GetDCTest();
    GetDCFormatTest();
    BackBufferCreateSurfaceTest();
    BackBufferAttachmentFlipTest();
    CreateSurfaceBadCapsSizeTest();
    no_ddsd_caps_test();
    zbufferbitdepth_test();
    pixelformat_flag_test();
    partial_block_lock_test();
    create_surface_test();
    ReleaseDirectDraw();
}
