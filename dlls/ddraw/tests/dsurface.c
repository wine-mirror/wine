/*
 * Unit tests for (a few) ddraw surface functions
 *
 * Copyright (C) 2005 Antoine Chavasse (a.chavasse@gmail.com)
 * Copyright (C) 2005 Christian Costa
 * Copyright 2005 Ivan Leo Puoti
 * Copyright (C) 2007 Stefan Dösinger
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
#include "unknwn.h"

static LPDIRECTDRAW lpDD = NULL;

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
    ok(rc==DD_OK,"CreateSurface returned: %x\n",rc);
    if (FAILED(rc)) {
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
    if (FAILED(rc)) {
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
    if (FAILED(rc)) {
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
    if (FAILED(rc)) {
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
}

static void SrcColorKey32BlitTest(void)
{
    LPDIRECTDRAWSURFACE lpSrc;
    LPDIRECTDRAWSURFACE lpDst;
    DDSURFACEDESC ddsd, ddsd2, ddsd3;
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
    if (FAILED(rc)) {
	skip("failed to create surface\n");
	return;
    }

    ddsd.dwFlags |= DDSD_CKSRCBLT;
    ddsd.ddckCKSrcBlt.dwColorSpaceLowValue = 0xFF00FF;
    ddsd.ddckCKSrcBlt.dwColorSpaceHighValue = 0xFF00FF;
    rc = IDirectDraw_CreateSurface(lpDD, &ddsd, &lpSrc, NULL);
    ok(rc==DD_OK,"CreateSurface returned: %x\n",rc);
    if (FAILED(rc)) {
	skip("failed to create surface\n");
	return;
    }

    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    lpData = (LPDWORD)ddsd2.lpSurface;
    lpData[0] = 0xCCCCCCCC;
    lpData[1] = 0xCCCCCCCC;
    lpData[2] = 0xCCCCCCCC;
    lpData[3] = 0xCCCCCCCC;

    memset(&ddsd3, 0, sizeof(ddsd3));
    ddsd3.dwSize = sizeof(ddsd3);
    ddsd3.ddpfPixelFormat.dwSize = sizeof(ddsd3.ddpfPixelFormat);
    rc = IDirectDrawSurface_GetSurfaceDesc(lpDst, &ddsd3);
    ok(rc == DD_OK, "IDirectDrawSurface_GetSurfaceDesc between a lock/unlock pair returned %08x\n", rc);
    ok(ddsd3.lpSurface == ddsd3.lpSurface, "lpSurface from GetSurfaceDesc(%p) differs from the one returned by Lock(%p)\n", ddsd3.lpSurface, ddsd2.lpSurface);

    rc = IDirectDrawSurface_Unlock(lpDst, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    memset(&ddsd3, 0, sizeof(ddsd3));
    ddsd3.dwSize = sizeof(ddsd3);
    ddsd3.ddpfPixelFormat.dwSize = sizeof(ddsd3.ddpfPixelFormat);
    rc = IDirectDrawSurface_GetSurfaceDesc(lpDst, &ddsd3);
    ok(rc == DD_OK, "IDirectDrawSurface_GetSurfaceDesc between a lock/unlock pair returned %08x\n", rc);
    ok(ddsd3.lpSurface == NULL, "lpSurface from GetSurfaceDesc(%p) is not NULL after unlock\n", ddsd3.lpSurface);

    rc = IDirectDrawSurface_Lock(lpSrc, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    ok((ddsd2.dwFlags & DDSD_LPSURFACE) == 0, "Surface desc has LPSURFACE Flags set\n");
    lpData = (LPDWORD)ddsd2.lpSurface;
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
    lpData = (LPDWORD)ddsd2.lpSurface;
    ok((lpData[0]==0x77010203)&&(lpData[1]==0x00010203)&&(lpData[2]==0xCCCCCCCC)&&(lpData[3]==0xCCCCCCCC),
       "Destination data after blitting is not correct\n");
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
    lpData = (LPDWORD)ddsd2.lpSurface;
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
    lpData = (LPDWORD)ddsd2.lpSurface;
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
    lpData = (LPDWORD)ddsd2.lpSurface;
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
    lpData = (LPDWORD)ddsd2.lpSurface;

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
    lpData = (LPDWORD)ddsd2.lpSurface;

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
    lpData = (LPDWORD)ddsd2.lpSurface;

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
    lpData = (LPDWORD)ddsd2.lpSurface;

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
    lpData = (LPDWORD)ddsd2.lpSurface;

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
    lpData = (LPDWORD)ddsd2.lpSurface;

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
    lpData = (LPDWORD)ddsd2.lpSurface;
    lpData[5] = 0x000000FF; /* Applies to src blt key in src surface */
    rc = IDirectDrawSurface_Unlock(lpSrc, NULL);
    ok(rc==DD_OK,"Unlock returned: %x\n",rc);

    /* Source and destination key */
    rc = IDirectDrawSurface_Blt(lpDst, NULL, lpSrc, NULL, DDBLT_KEYDEST | DDBLT_KEYSRC, &fx);
    ok(rc == DD_OK, "IDirectDrawSurface_Blt returned %08x\n", rc);

    rc = IDirectDrawSurface_Lock(lpDst, NULL, &ddsd2, DDLOCK_WAIT, NULL);
    ok(rc==DD_OK,"Lock returned: %x\n",rc);
    ok((ddsd2.dwFlags & DDSD_LPSURFACE) == 0, "Surface desc has LPSURFACE Flags set\n");
    lpData = (LPDWORD)ddsd2.lpSurface;

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

    /* With korrectly passed override keys no key in the surface is needed.
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

static unsigned long getref(IUnknown *iface)
{
    IUnknown_AddRef(iface);
    return IUnknown_Release(iface);
}

static void GetDDInterface_1(void)
{
    LPDIRECTDRAWSURFACE dsurface;
    LPDIRECTDRAWSURFACE2 dsurface2;
    DDSURFACEDESC surface;
    HRESULT ret;
    IDirectDraw2 *dd2;
    IDirectDraw4 *dd4;
    IDirectDraw7 *dd7;
    unsigned long ref1, ref2, ref4, ref7;
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
    ok(ref1 == 1, "IDirectDraw refcount is %ld\n", ref1);
    ref2 = getref((IUnknown *) dd2);
    ok(ref2 == 1, "IDirectDraw2 refcount is %ld\n", ref2);
    ref4 = getref((IUnknown *) dd4);
    ok(ref4 == 1, "IDirectDraw4 refcount is %ld\n", ref4);
    ref7 = getref((IUnknown *) dd7);
    ok(ref7 == 1, "IDirectDraw7 refcount is %ld\n", ref7);


    ret = IDirectDrawSurface2_GetDDInterface(dsurface2, &dd);
    ok(ret == DD_OK, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);
    ok(getref((IUnknown *) lpDD) == ref1 + 1, "IDirectDraw refcount was increased by %ld\n", getref((IUnknown *) lpDD) - ref1);
    ok(getref((IUnknown *) dd2) == ref2 + 0, "IDirectDraw2 refcount was increased by %ld\n", getref((IUnknown *) dd2) - ref2);
    ok(getref((IUnknown *) dd4) == ref4 + 0, "IDirectDraw4 refcount was increased by %ld\n", getref((IUnknown *) dd4) - ref4);
    ok(getref((IUnknown *) dd7) == ref7 + 0, "IDirectDraw7 refcount was increased by %ld\n", getref((IUnknown *) dd7) - ref7);

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
    LPDIRECTDRAWSURFACE dsurface;
    LPDIRECTDRAWSURFACE2 dsurface2;
    DDSURFACEDESC surface;
    HRESULT ret;
    IDirectDraw2 *dd2;
    IDirectDraw4 *dd4;
    IDirectDraw7 *dd7;
    unsigned long ref1, ref2, ref4, ref7;
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
    ok(ref1 == 1, "IDirectDraw refcount is %ld\n", ref1);
    ref2 = getref((IUnknown *) dd2);
    ok(ref2 == 1, "IDirectDraw2 refcount is %ld\n", ref2);
    ref4 = getref((IUnknown *) dd4);
    ok(ref4 == 1, "IDirectDraw4 refcount is %ld\n", ref4);
    ref7 = getref((IUnknown *) dd7);
    ok(ref7 == 1, "IDirectDraw7 refcount is %ld\n", ref7);


    ret = IDirectDrawSurface2_GetDDInterface(dsurface2, &dd);
    ok(ret == DD_OK, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);
    ok(getref((IUnknown *) lpDD) == ref1 + 0, "IDirectDraw refcount was increased by %ld\n", getref((IUnknown *) lpDD) - ref1);
    ok(getref((IUnknown *) dd2) == ref2 + 1, "IDirectDraw2 refcount was increased by %ld\n", getref((IUnknown *) dd2) - ref2);
    ok(getref((IUnknown *) dd4) == ref4 + 0, "IDirectDraw4 refcount was increased by %ld\n", getref((IUnknown *) dd4) - ref4);
    ok(getref((IUnknown *) dd7) == ref7 + 0, "IDirectDraw7 refcount was increased by %ld\n", getref((IUnknown *) dd7) - ref7);

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
    LPDIRECTDRAWSURFACE2 dsurface2;
    LPDIRECTDRAWSURFACE4 dsurface4;
    DDSURFACEDESC2 surface;
    HRESULT ret;
    IDirectDraw2 *dd2;
    IDirectDraw4 *dd4;
    IDirectDraw7 *dd7;
    unsigned long ref1, ref2, ref4, ref7;
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
    ok(ref1 == 1, "IDirectDraw refcount is %ld\n", ref1);
    ref2 = getref((IUnknown *) dd2);
    ok(ref2 == 1, "IDirectDraw2 refcount is %ld\n", ref2);
    ref4 = getref((IUnknown *) dd4);
    ok(ref4 == 2, "IDirectDraw4 refcount is %ld\n", ref4);
    ref7 = getref((IUnknown *) dd7);
    ok(ref7 == 1, "IDirectDraw7 refcount is %ld\n", ref7);

    ret = IDirectDrawSurface4_GetDDInterface(dsurface4, &dd);
    ok(ret == DD_OK, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);
    ok(getref((IUnknown *) lpDD) == ref1 + 0, "IDirectDraw refcount was increased by %ld\n", getref((IUnknown *) lpDD) - ref1);
    ok(getref((IUnknown *) dd2) == ref2 + 0, "IDirectDraw2 refcount was increased by %ld\n", getref((IUnknown *) dd2) - ref2);
    ok(getref((IUnknown *) dd4) == ref4 + 1, "IDirectDraw4 refcount was increased by %ld\n", getref((IUnknown *) dd4) - ref4);
    ok(getref((IUnknown *) dd7) == ref7 + 0, "IDirectDraw7 refcount was increased by %ld\n", getref((IUnknown *) dd7) - ref7);

    ok(dd == dd4, "Returned interface pointer is not equal to the creation interface\n");
    IUnknown_Release((IUnknown *) dd);

    /* Now test what happens if we QI the surface for some other version - It should still return the creation interface */
    ret = IDirectDrawSurface2_GetDDInterface(dsurface2, &dd);
    ok(ret == DD_OK, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);
    ok(getref((IUnknown *) lpDD) == ref1 + 0, "IDirectDraw refcount was increased by %ld\n", getref((IUnknown *) lpDD) - ref1);
    ok(getref((IUnknown *) dd2) == ref2 + 0, "IDirectDraw2 refcount was increased by %ld\n", getref((IUnknown *) dd2) - ref2);
    ok(getref((IUnknown *) dd4) == ref4 + 1, "IDirectDraw4 refcount was increased by %ld\n", getref((IUnknown *) dd4) - ref4);
    ok(getref((IUnknown *) dd7) == ref7 + 0, "IDirectDraw7 refcount was increased by %ld\n", getref((IUnknown *) dd7) - ref7);

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
    LPDIRECTDRAWSURFACE4 dsurface4;
    LPDIRECTDRAWSURFACE7 dsurface7;
    DDSURFACEDESC2 surface;
    HRESULT ret;
    IDirectDraw2 *dd2;
    IDirectDraw4 *dd4;
    IDirectDraw7 *dd7;
    unsigned long ref1, ref2, ref4, ref7;
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
    ok(ref1 == 1, "IDirectDraw refcount is %ld\n", ref1);
    ref2 = getref((IUnknown *) dd2);
    ok(ref2 == 1, "IDirectDraw2 refcount is %ld\n", ref2);
    ref4 = getref((IUnknown *) dd4);
    ok(ref4 == 1, "IDirectDraw4 refcount is %ld\n", ref4);
    ref7 = getref((IUnknown *) dd7);
    ok(ref7 == 2, "IDirectDraw7 refcount is %ld\n", ref7);

    ret = IDirectDrawSurface7_GetDDInterface(dsurface7, &dd);
    ok(ret == DD_OK, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);
    ok(getref((IUnknown *) lpDD) == ref1 + 0, "IDirectDraw refcount was increased by %ld\n", getref((IUnknown *) lpDD) - ref1);
    ok(getref((IUnknown *) dd2) == ref2 + 0, "IDirectDraw2 refcount was increased by %ld\n", getref((IUnknown *) dd2) - ref2);
    ok(getref((IUnknown *) dd4) == ref4 + 0, "IDirectDraw4 refcount was increased by %ld\n", getref((IUnknown *) dd4) - ref4);
    ok(getref((IUnknown *) dd7) == ref7 + 1, "IDirectDraw7 refcount was increased by %ld\n", getref((IUnknown *) dd7) - ref7);

    ok(dd == dd7, "Returned interface pointer is not equal to the creation interface\n");
    IUnknown_Release((IUnknown *) dd);

    /* Now test what happens if we QI the surface for some other version - It should still return the creation interface */
    ret = IDirectDrawSurface4_GetDDInterface(dsurface4, &dd);
    ok(ret == DD_OK, "IDirectDrawSurface7_GetDDInterface returned %08x\n", ret);
    ok(getref((IUnknown *) lpDD) == ref1 + 0, "IDirectDraw refcount was increased by %ld\n", getref((IUnknown *) lpDD) - ref1);
    ok(getref((IUnknown *) dd2) == ref2 + 0, "IDirectDraw2 refcount was increased by %ld\n", getref((IUnknown *) dd2) - ref2);
    ok(getref((IUnknown *) dd4) == ref4 + 0, "IDirectDraw4 refcount was increased by %ld\n", getref((IUnknown *) dd4) - ref4);
    ok(getref((IUnknown *) dd7) == ref7 + 1, "IDirectDraw7 refcount was increased by %ld\n", getref((IUnknown *) dd7) - ref7);

    ok(dd == dd7, "Returned interface pointer is not equal to the creation interface\n");
    IUnknown_Release((IUnknown *) dd);

    IDirectDraw_Release(dd2);
    IDirectDraw_Release(dd4);
    IDirectDraw_Release(dd7);
    IDirectDrawSurface4_Release(dsurface4);
    IDirectDrawSurface7_Release(dsurface7);
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
    ctx.count = 0;

    rc = IDirectDraw_EnumSurfaces(lpDD, DDENUMSURFACES_DOESEXIST | DDENUMSURFACES_ALL, &ddsd, (void *) &ctx, enumCB);
    ok(rc == DD_OK, "IDirectDraw_EnumSurfaces returned %08x\n", rc);
    ok(ctx.count == 3, "%d surfaces enumerated, expected 3\n", ctx.count);

    IDirectDrawSurface_Release(ctx.expected[2]);
    IDirectDrawSurface_Release(ctx.expected[1]);
    IDirectDrawSurface_Release(surface);
}

HRESULT WINAPI SurfaceCounter(IDirectDrawSurface7 *surface, DDSURFACEDESC2 *desc, void *context)
{
    UINT *num = context;
    (*num)++;
    IDirectDrawSurface_Release(surface);
    return DDENUMRET_OK;
}

static void AttachmentTest7(void)
{
    HRESULT hr;
    IDirectDraw7 *dd7;
    IDirectDrawSurface7 *surface1, *surface2, *surface3, *surface4;
    DDSURFACEDESC2 ddsd;
    UINT num;
    DDSCAPS2 caps = {DDSCAPS_TEXTURE, 0, 0, 0};
    HWND window = CreateWindow( "static", "ddraw_test", WS_OVERLAPPEDWINDOW, 100, 100, 160, 160, NULL, NULL, NULL, NULL );

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    ok(hr == DD_OK, "IDirectDraw_QueryInterface returned %08x\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_MIPMAPCOUNT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    U2(ddsd).dwMipMapCount = 3; /* Will create 128x128, 64x64, 32x32 */
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface1, NULL);
    ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

    /* ROOT */
    num = 0;
    IDirectDrawSurface7_EnumAttachedSurfaces(surface1, &num, SurfaceCounter);
    ok(num == 1, "Mipmap root has %d surfaces attached, expected 1\n", num);
    /* DONE ROOT */

    /* LEVEL 1 */
    hr = IDirectDrawSurface7_GetAttachedSurface(surface1, &caps, &surface2);
    ok(hr == DD_OK, "GetAttachedSurface returned %08x\n", hr);
    num = 0;
    IDirectDrawSurface7_EnumAttachedSurfaces(surface2, &num, SurfaceCounter);
    ok(num == 1, "First mip level has %d surfaces attached, expected 1\n", num);
    /* DONE LEVEL 1 */

    /* LEVEL 2 */
    hr = IDirectDrawSurface7_GetAttachedSurface(surface2, &caps, &surface3);
    ok(hr == DD_OK, "GetAttachedSurface returned %08x\n", hr);
    IDirectDrawSurface7_Release(surface2);
    num = 0;
    IDirectDrawSurface7_EnumAttachedSurfaces(surface3, &num, SurfaceCounter);
    ok(num == 0, "Second mip level has %d surfaces attached, expected 1\n", num);
    /* Done level 2 */
    /* Mip level 3 is still needed */

    /* Try to attach a 16x16 miplevel - Should not work as far I can see */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    ddsd.dwWidth = 16;
    ddsd.dwHeight = 16;
    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface2, NULL);
    ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

    hr = IDirectDrawSurface7_AddAttachedSurface(surface1, surface2);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a 16x16 surface to a 128x128 texture root returned %08x\n", hr);
    hr = IDirectDrawSurface7_AddAttachedSurface(surface2, surface1);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a 128x128 texture root to a 16x16 texture returned %08x\n", hr);
    hr = IDirectDrawSurface7_AddAttachedSurface(surface3, surface2);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a 16x16 surface to a 32x32 texture mip level returned %08x\n", hr);
    hr = IDirectDrawSurface7_AddAttachedSurface(surface2, surface3);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a 32x32 texture mip level to a 16x16 surface returned %08x\n", hr);

    IDirectDrawSurface7_Release(surface2);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = 16;
    ddsd.dwHeight = 16;
    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface2, NULL);
    ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

    hr = IDirectDrawSurface7_AddAttachedSurface(surface1, surface2);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a 16x16 offscreen plain surface to a 128x128 texture root returned %08x\n", hr);
    hr = IDirectDrawSurface7_AddAttachedSurface(surface2, surface1);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a 128x128 texture root to a 16x16 offscreen plain surface returned %08x\n", hr);
    hr = IDirectDrawSurface7_AddAttachedSurface(surface3, surface2);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a 16x16 offscreen plain surface to a 32x32 texture mip level returned %08x\n", hr);
    hr = IDirectDrawSurface7_AddAttachedSurface(surface2, surface3);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a 32x32 texture mip level to a 16x16 offscreen plain surface returned %08x\n", hr);

    IDirectDrawSurface7_Release(surface3);
    IDirectDrawSurface7_Release(surface2);
    IDirectDrawSurface7_Release(surface1);

    hr = IDirectDraw7_SetCooperativeLevel(dd7, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "SetCooperativeLevel returned %08x\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_BACKBUFFERCOUNT | DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
    ddsd.dwBackBufferCount = 2;
    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface1, NULL);
    ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

    num = 0;
    IDirectDrawSurface7_EnumAttachedSurfaces(surface1, &num, SurfaceCounter);
    ok(num == 1, "Primary surface has %d surfaces attached, expected 1\n", num);
    IDirectDrawSurface7_Release(surface1);

    /* Those are some invalid descriptions, no need to test attachments with them */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FRONTBUFFER;
    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface1, NULL);
    ok(hr==DDERR_INVALIDCAPS,"CreateSurface returned: %x\n",hr);
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_BACKBUFFER;
    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface2, NULL);
    ok(hr==DDERR_INVALIDCAPS,"CreateSurface returned: %x\n",hr);

    /* Try a single primary and two offscreen plain surfaces */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface1, NULL);
    ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = GetSystemMetrics(SM_CXSCREEN);
    ddsd.dwHeight = GetSystemMetrics(SM_CYSCREEN);
    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface2, NULL);
    ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = GetSystemMetrics(SM_CXSCREEN);
    ddsd.dwHeight = GetSystemMetrics(SM_CYSCREEN);
    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface3, NULL);
    ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

    /* This one has a different size */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    hr = IDirectDraw7_CreateSurface(dd7, &ddsd, &surface4, NULL);
    ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

    hr = IDirectDrawSurface7_AddAttachedSurface(surface1, surface2);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching an offscreen plain surface to a front buffer returned %08x\n", hr);
    hr = IDirectDrawSurface7_AddAttachedSurface(surface2, surface1);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a front buffer to an offscreen plain surface returned %08x\n", hr);
    hr = IDirectDrawSurface7_AddAttachedSurface(surface2, surface3);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching an offscreen plain surface to another offscreen plain surface returned %08x\n", hr);
    hr = IDirectDrawSurface7_AddAttachedSurface(surface1, surface4);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching an offscreen plain surface to a front buffer of different size returned %08x\n", hr);
    hr = IDirectDrawSurface7_AddAttachedSurface(surface4, surface1);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a front buffer to an offscreen plain surface of different size returned %08x\n", hr);

    IDirectDrawSurface7_Release(surface4);
    IDirectDrawSurface7_Release(surface3);
    IDirectDrawSurface7_Release(surface2);
    IDirectDrawSurface7_Release(surface1);

    hr =IDirectDraw7_SetCooperativeLevel(dd7, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "SetCooperativeLevel returned %08x\n", hr);
    IDirectDraw7_Release(dd7);
}

static void AttachmentTest(void)
{
    HRESULT hr;
    IDirectDrawSurface *surface1, *surface2, *surface3, *surface4;
    DDSURFACEDESC ddsd;
    DDSCAPS caps = {DDSCAPS_TEXTURE};
    HWND window = CreateWindow( "static", "ddraw_test", WS_OVERLAPPEDWINDOW, 100, 100, 160, 160, NULL, NULL, NULL, NULL );

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_MIPMAPCOUNT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    U2(ddsd).dwMipMapCount = 3; /* Will create 128x128, 64x64, 32x32 */
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    hr = IDirectDraw7_CreateSurface(lpDD, &ddsd, &surface1, NULL);
    ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

    hr = IDirectDrawSurface7_GetAttachedSurface(surface1, &caps, &surface2);
    ok(hr == DD_OK, "GetAttachedSurface returned %08x\n", hr);
    hr = IDirectDrawSurface7_GetAttachedSurface(surface2, &caps, &surface3);
    ok(hr == DD_OK, "GetAttachedSurface returned %08x\n", hr);

    /* Try to attach a 16x16 miplevel - Should not work as far I can see */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    ddsd.dwWidth = 16;
    ddsd.dwHeight = 16;
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface4, NULL);
    ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

    hr = IDirectDrawSurface7_AddAttachedSurface(surface1, surface4);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a 16x16 surface to a 128x128 texture root returned %08x\n", hr);
    hr = IDirectDrawSurface7_AddAttachedSurface(surface4, surface1);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a 128x128 texture root to a 16x16 texture returned %08x\n", hr);
    hr = IDirectDrawSurface7_AddAttachedSurface(surface3, surface4);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a 16x16 surface to a 32x32 texture mip level returned %08x\n", hr);
    hr = IDirectDrawSurface7_AddAttachedSurface(surface4, surface3);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a 32x32 texture mip level to a 16x16 surface returned %08x\n", hr);
    hr = IDirectDrawSurface7_AddAttachedSurface(surface2, surface4);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a 16x16 surface to a 64x64 texture sublevel returned %08x\n", hr);
    hr = IDirectDrawSurface7_AddAttachedSurface(surface4, surface2);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a 64x64 texture sublevel to a 16x16 texture returned %08x\n", hr);

    IDirectDrawSurface7_Release(surface4);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = 16;
    ddsd.dwHeight = 16;
    hr = IDirectDraw7_CreateSurface(lpDD, &ddsd, &surface4, NULL);
    ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

    hr = IDirectDrawSurface7_AddAttachedSurface(surface1, surface4); /* Succeeds on refrast */
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a 16x16 offscreen plain surface to a 128x128 texture root returned %08x\n", hr);
    if(SUCCEEDED(hr)) IDirectDrawSurface7_DeleteAttachedSurface(surface1, 0, surface4);
    hr = IDirectDrawSurface7_AddAttachedSurface(surface4, surface1);  /* Succeeds on refrast */
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a 128x128 texture root to a 16x16 offscreen plain surface returned %08x\n", hr);
    if(SUCCEEDED(hr)) IDirectDrawSurface7_DeleteAttachedSurface(surface1, 0, surface1);
    hr = IDirectDrawSurface7_AddAttachedSurface(surface3, surface4);  /* Succeeds on refrast */
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a 16x16 offscreen plain surface to a 32x32 texture mip level returned %08x\n", hr);
    if(SUCCEEDED(hr)) IDirectDrawSurface7_DeleteAttachedSurface(surface3, 0, surface4);
    hr = IDirectDrawSurface7_AddAttachedSurface(surface4, surface3);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a 32x32 texture mip level to a 16x16 offscreen plain surface returned %08x\n", hr);
    if(SUCCEEDED(hr)) IDirectDrawSurface7_DeleteAttachedSurface(surface4, 0, surface3);
    hr = IDirectDrawSurface7_AddAttachedSurface(surface2, surface4);  /* Succeeds on refrast */
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a 16x16 offscreen plain surface to a 64x64 texture sublevel returned %08x\n", hr);
    if(SUCCEEDED(hr)) IDirectDrawSurface7_DeleteAttachedSurface(surface2, 0, surface4);
    hr = IDirectDrawSurface7_AddAttachedSurface(surface4, surface2);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a 64x64 texture sublevel to a 16x16 offscreen plain surface returned %08x\n", hr);
    if(SUCCEEDED(hr)) IDirectDrawSurface7_DeleteAttachedSurface(surface4, 0, surface2);

    IDirectDrawSurface7_Release(surface4);
    IDirectDrawSurface7_Release(surface3);
    IDirectDrawSurface7_Release(surface2);
    IDirectDrawSurface7_Release(surface1);

    hr = IDirectDraw_SetCooperativeLevel(lpDD, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "SetCooperativeLevel returned %08x\n", hr);

    /* Creating a back buffer as-is, is not allowed. No need to perform attachment tests */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_BACKBUFFER;
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface2, NULL);
    ok(hr==DDERR_INVALIDCAPS,"CreateSurface returned: %x\n",hr);
    /* This old ddraw version happily creates explicit front buffers */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FRONTBUFFER;
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface1, NULL);
    ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);
    IDirectDrawSurface_Release(surface1);

    /* Try a single primary and two offscreen plain surfaces */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface1, NULL);
    ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = GetSystemMetrics(SM_CXSCREEN);
    ddsd.dwHeight = GetSystemMetrics(SM_CYSCREEN);
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface2, NULL);
    ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = GetSystemMetrics(SM_CXSCREEN);
    ddsd.dwHeight = GetSystemMetrics(SM_CYSCREEN);
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface3, NULL);
    ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

    /* This one has a different size */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    hr = IDirectDraw_CreateSurface(lpDD, &ddsd, &surface4, NULL);
    ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);

    hr = IDirectDrawSurface_AddAttachedSurface(surface1, surface2);
    ok(hr == DD_OK, "Attaching an offscreen plain surface to a front buffer returned %08x\n", hr);
    /* Try the reverse without detaching first */
    hr = IDirectDrawSurface_AddAttachedSurface(surface2, surface1);
    ok(hr == DDERR_SURFACEALREADYATTACHED, "Attaching an attached surface to its attachee returned %08x\n", hr);
    hr = IDirectDrawSurface_DeleteAttachedSurface(surface1, 0, surface2);
    ok(hr == DD_OK, "DeleteAttachedSurface failed with %08x\n", hr);
    hr = IDirectDrawSurface_AddAttachedSurface(surface2, surface1);
    ok(hr == DD_OK, "Attaching a front buffer to an offscreen plain surface returned %08x\n", hr);
    /* Try to detach reversed */
    hr = IDirectDrawSurface_DeleteAttachedSurface(surface1, 0, surface2);
    ok(hr == DDERR_CANNOTDETACHSURFACE, "DeleteAttachedSurface returned %08x\n", hr);
    /* Now the proper detach */
    hr = IDirectDrawSurface_DeleteAttachedSurface(surface2, 0, surface1);
    ok(hr == DD_OK, "DeleteAttachedSurface failed with %08x\n", hr);
    hr = IDirectDrawSurface_AddAttachedSurface(surface2, surface3); /* Fails on refrast */
    ok(hr == DD_OK, "Attaching an offscreen plain surface to another offscreen plain surface returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirectDrawSurface_DeleteAttachedSurface(surface2, 0, surface3);
        ok(hr == DD_OK, "DeleteAttachedSurface failed with %08x\n", hr);
    }
    hr = IDirectDrawSurface_AddAttachedSurface(surface1, surface4);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching an offscreen plain surface to a front buffer of different size returned %08x\n", hr);
    hr = IDirectDrawSurface_AddAttachedSurface(surface4, surface1);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Attaching a front buffer to an offscreen plain surface of different size returned %08x\n", hr);

    IDirectDrawSurface_Release(surface4);
    IDirectDrawSurface_Release(surface3);
    IDirectDrawSurface_Release(surface2);
    IDirectDrawSurface_Release(surface1);

    hr =IDirectDraw_SetCooperativeLevel(lpDD, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "SetCooperativeLevel returned %08x\n", hr);

    DestroyWindow(window);
}

struct compare
{
    DWORD width, height;
    DWORD caps, caps2;
    UINT mips;
};

HRESULT WINAPI CubeTestLvl2Enum(IDirectDrawSurface7 *surface, DDSURFACEDESC2 *desc, void *context)
{
    UINT *mips = context;

    (*mips)++;
    IDirectDrawSurface7_EnumAttachedSurfaces(surface,
                                             context,
                                             CubeTestLvl2Enum);

    return DDENUMRET_OK;
}

HRESULT WINAPI CubeTestLvl1Enum(IDirectDrawSurface7 *surface, DDSURFACEDESC2 *desc, void *context)
{
    UINT mips = 0;
    UINT *num = (UINT *) context;
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
    IDirectDrawSurface7 *cubemap;
    DDSURFACEDESC2 ddsd;
    HRESULT hr;
    UINT num = 0;
    struct enumstruct ctx;

    hr = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw7, (void **) &dd7);
    ok(hr == DD_OK, "IDirectDraw::QueryInterface returned %08x\n", hr);

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
    ok(hr == DD_OK, "IDirectDraw7::CreateSurface returned %08x\n", hr);

    hr = IDirectDrawSurface7_GetSurfaceDesc(cubemap, &ddsd);
    ok(hr == DD_OK, "IDirectDrawSurface7_GetSurfaceDesc returned %08x\n", hr);
    ok(ddsd.ddsCaps.dwCaps == (DDSCAPS_MIPMAP | DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_COMPLEX),
       "Root Caps are %08x\n", ddsd.ddsCaps.dwCaps);
    ok(ddsd.ddsCaps.dwCaps2 == (DDSCAPS2_CUBEMAP_POSITIVEX | DDSCAPS2_CUBEMAP),
       "Root Caps2 are %08x\n", ddsd.ddsCaps.dwCaps2);

    IDirectDrawSurface7_EnumAttachedSurfaces(cubemap,
                                             &num,
                                             CubeTestLvl1Enum);
    trace("Enumerated %d surfaces in total\n", num);
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

    /* Make sure everything is cleaned up properly. Use the enumSurfaces test infrastructure */
    memset(&ctx, 0, sizeof(ctx));
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    hr = IDirectDraw_EnumSurfaces(lpDD, DDENUMSURFACES_DOESEXIST | DDENUMSURFACES_ALL, (DDSURFACEDESC *) &ddsd, (void *) &ctx, enumCB);
    ok(hr == DD_OK, "IDirectDraw_EnumSurfaces returned %08x\n", hr);
    ok(ctx.count == 0, "%d surfaces enumerated, expected 0\n", ctx.count);

    IDirectDraw7_Release(dd7);
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
        DDSCAPS_OFFSCREENPLAIN,
        DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE,
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
        if (FAILED(hr)) {
	    skip("failed to create surface\n");
	    continue;
	}

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

            hr = IDirectDrawSurface_Lock(surface, rect, &locked_desc, DDLOCK_WAIT, NULL);
            ok(hr == DDERR_INVALIDPARAMS, "Lock returned 0x%08x for rect [%d, %d]->[%d, %d]"
                    ", expected DDERR_INVALIDPARAMS (0x%08x)\n", hr, rect->left, rect->top,
                    rect->right, rect->bottom, DDERR_INVALIDPARAMS);
        }

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
    if (FAILED(hr)) {
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
    if (FAILED(hr)) {
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
    if (FAILED(hr)) {
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
    LPDIRECTDRAWSURFACE dsurface = NULL;
    DDSURFACEDESC desc;
    HRESULT ret;
    HWND window = CreateWindow( "static", "ddraw_test", WS_OVERLAPPEDWINDOW, 100, 100, 160, 160, NULL, NULL, NULL, NULL );

    /* Create an offscreen surface surface without a size */
    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS;
    desc.ddsCaps.dwCaps |= DDSCAPS_OFFSCREENPLAIN;
    trace("before offscreenplain create dsurface = %p\n", dsurface);
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
    ok(ret == DDERR_INVALIDPARAMS, "Creating an offscreen plain surface without hight info returned %08x\n", ret);
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

static void PrivateDataTest(void)
{
    HRESULT hr;
    IDirectDrawSurface7 *surface7 = NULL;
    IDirectDrawSurface *surface = NULL;
    DDSURFACEDESC desc;
    ULONG ref, ref2;
    IUnknown *ptr;
    DWORD size = sizeof(IUnknown *);

    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    desc.ddsCaps.dwCaps |= DDSCAPS_OFFSCREENPLAIN;
    desc.dwHeight = 128;
    desc.dwWidth = 128;
    hr = IDirectDraw_CreateSurface(lpDD, &desc, &surface, NULL);
    ok(hr == DD_OK, "Creating an offscreen plain surface failed with %08x\n", hr);
    if(!surface)
    {
        return;
    }
    hr = IDirectDrawSurface_QueryInterface(surface, &IID_IDirectDrawSurface7, (void **) &surface7);
    ok(hr == DD_OK, "IDirectDrawSurface_QueryInterface failed with %08x\n", hr);
    if(!surface)
    {
        IDirectDrawSurface_Release(surface);
        return;
    }

    /* This fails */
    hr = IDirectDrawSurface7_SetPrivateData(surface7, &IID_IDirectDrawSurface7 /* Abuse this tag */, lpDD, 0, DDSPD_IUNKNOWNPOINTER);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDrawSurface7_SetPrivateData failed with %08x\n", hr);
    hr = IDirectDrawSurface7_SetPrivateData(surface7, &IID_IDirectDrawSurface7 /* Abuse this tag */, lpDD, 5, DDSPD_IUNKNOWNPOINTER);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDrawSurface7_SetPrivateData failed with %08x\n", hr);
    hr = IDirectDrawSurface7_SetPrivateData(surface7, &IID_IDirectDrawSurface7 /* Abuse this tag */, lpDD, sizeof(IUnknown *) * 2, DDSPD_IUNKNOWNPOINTER);
    ok(hr == DDERR_INVALIDPARAMS, "IDirectDrawSurface7_SetPrivateData failed with %08x\n", hr);

    ref = getref((IUnknown *) lpDD);
    hr = IDirectDrawSurface7_SetPrivateData(surface7, &IID_IDirectDrawSurface7 /* Abuse this tag */, lpDD, sizeof(IUnknown *), DDSPD_IUNKNOWNPOINTER);
    ok(hr == DD_OK, "IDirectDrawSurface7_SetPrivateData failed with %08x\n", hr);
    ref2 = getref((IUnknown *) lpDD);
    ok(ref2 == ref + 1, "Object reference is %d, expected %d\n", ref2, ref + 1);
    hr = IDirectDrawSurface7_FreePrivateData(surface7, &IID_IDirectDrawSurface7);
    ref2 = getref((IUnknown *) lpDD);
    ok(ref2 == ref, "Object reference is %d, expected %d\n", ref2, ref);

    hr = IDirectDrawSurface7_SetPrivateData(surface7, &IID_IDirectDrawSurface7, lpDD, sizeof(IUnknown *), DDSPD_IUNKNOWNPOINTER);
    ok(hr == DD_OK, "IDirectDrawSurface7_SetPrivateData failed with %08x\n", hr);
    hr = IDirectDrawSurface7_SetPrivateData(surface7, &IID_IDirectDrawSurface7, surface7, sizeof(IUnknown *), DDSPD_IUNKNOWNPOINTER);
    ok(hr == DD_OK, "IDirectDrawSurface7_SetPrivateData failed with %08x\n", hr);
    ref2 = getref((IUnknown *) lpDD);
    ok(ref2 == ref, "Object reference is %d, expected %d\n", ref2, ref);

    hr = IDirectDrawSurface7_SetPrivateData(surface7, &IID_IDirectDrawSurface7, lpDD, sizeof(IUnknown *), DDSPD_IUNKNOWNPOINTER);
    ok(hr == DD_OK, "IDirectDrawSurface7_SetPrivateData failed with %08x\n", hr);
    hr = IDirectDrawSurface7_GetPrivateData(surface7, &IID_IDirectDrawSurface7, &ptr, &size);
    ok(hr == DD_OK, "IDirectDrawSurface7_GetPrivateData failed with %08x\n", hr);
    ref2 = getref((IUnknown *) lpDD);
    /* Object is NOT beein addrefed */
    ok(ptr == (IUnknown *) lpDD, "Returned interface pointer is %p, expected %p\n", ptr, lpDD);
    ok(ref2 == ref + 1, "Object reference is %d, expected %d. ptr at %p, orig at %p\n", ref2, ref + 1, ptr, lpDD);

    IDirectDrawSurface_Release(surface);
    IDirectDrawSurface7_Release(surface7);

    /* Destroying the surface frees the held reference */
    ref2 = getref((IUnknown *) lpDD);
    ok(ref2 == ref, "Object reference is %d, expected %d\n", ref2, ref);
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
    ok(hr == DD_OK, "IDirectDrawSurface_Blt with a invalid, unused rectangle returned %08x\n", hr);
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

static void StructSizeTest(void)
{
    IDirectDrawSurface *surface1;
    IDirectDrawSurface7 *surface7;
    union {
        DDSURFACEDESC desc1;
        DDSURFACEDESC2 desc2;
        char blob[1024]; /* To get a buch of writeable memory */
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

START_TEST(dsurface)
{
    if (!CreateDirectDraw())
        return;
    MipMapCreationTest();
    SrcColorKey32BlitTest();
    QueryInterface();
    GetDDInterface_1();
    GetDDInterface_2();
    GetDDInterface_4();
    GetDDInterface_7();
    EnumTest();
    AttachmentTest();
    AttachmentTest7();
    CubeMapTest();
    test_lockrect_invalid();
    CompressedTest();
    SizeTest();
    PrivateDataTest();
    BltParamTest();
    StructSizeTest();
    ReleaseDirectDraw();
}
