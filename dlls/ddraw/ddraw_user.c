/*	DirectDraw driver for User-based primary surfaces
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
#include <stdarg.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "ddraw.h"
#include "ddraw_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

static const IDirectDraw7Vtbl User_DirectDraw_VTable;

static const DDDEVICEIDENTIFIER2 user_device =
{
    "display",
    "User (and GDI)",
    { { 0x00010001, 0x00010001 } },
    0, 0, 0, 0,
    /* fe38440c-8969-4283-bc73-749e7bc3c2eb */
    {0xfe38440c,0x8969,0x428e, {0x73,0xbc,0x74,0x9e,0x7b,0xc3,0xc2,0xeb}},
    0
};

static const DDPIXELFORMAT pixelformats[] =
{
    /* 8bpp paletted */
    { sizeof(DDPIXELFORMAT), DDPF_RGB|DDPF_PALETTEINDEXED8, 0, { 8 } },
    /* 15bpp 5/5/5 */
    { sizeof(DDPIXELFORMAT), DDPF_RGB, 0, { 16 }, { 0x7C00 }, { 0x3E0 },
      { 0x1F } },
    /* 16bpp 5/6/5 */
    { sizeof(DDPIXELFORMAT), DDPF_RGB, 0, { 16 }, { 0xF800 }, { 0x7E0 },
      { 0x1F } },
    /* 24bpp 8/8/8 */
    { sizeof(DDPIXELFORMAT), DDPF_RGB, 0, { 24 }, { 0xFF0000 },
      { 0x00FF00 }, { 0x0000FF } },
    /* 32bpp 8/8/8 */
    { sizeof(DDPIXELFORMAT), DDPF_RGB, 0, { 32 }, { 0xFF0000 },
      { 0x00FF00 }, { 0x0000FF } }
};

HRESULT User_DirectDraw_Create(const GUID* pGUID, LPDIRECTDRAW7* pIface,
				     IUnknown* pUnkOuter, BOOL ex);
HRESULT User_DirectDraw_Initialize(IDirectDrawImpl*, const GUID*);

static const ddraw_driver user_driver =
{
    &user_device,
    10,
    User_DirectDraw_Create,
    User_DirectDraw_Initialize
};

BOOL DDRAW_User_Init(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
	DDRAW_register_driver(&user_driver);

    return TRUE;
}

static const DDPIXELFORMAT* pixelformat_for_depth(DWORD depth)
{
    switch (depth)
    {
    case  8: return pixelformats + 0;
    case 15: return pixelformats + 1;
    case 16: return pixelformats + 2;
    case 24: return pixelformats + 3;
    case 32: return pixelformats + 4;
    default: return NULL;
    }
}

/* Not called from the vtable. */
HRESULT User_DirectDraw_Construct(IDirectDrawImpl *This, BOOL ex)
{
    HRESULT hr;
    DWORD depth;
    HDC hDC;

    TRACE("(%p,%d)\n",This,ex);

    hr = Main_DirectDraw_Construct(This, ex);
    if (FAILED(hr)) return hr;

    This->final_release = User_DirectDraw_final_release;

    This->create_primary    = User_DirectDraw_create_primary;
    This->create_backbuffer = User_DirectDraw_create_backbuffer;

    hDC = CreateDCA("DISPLAY", NULL, NULL, NULL);
    depth = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES);
    DeleteDC(hDC);

    This->width       = GetSystemMetrics(SM_CXSCREEN);
    This->height      = GetSystemMetrics(SM_CYSCREEN);
    This->pitch       = DDRAW_width_bpp_to_pitch(This->width, depth);
    This->pixelformat = *pixelformat_for_depth(depth);

    This->orig_width       = This->width;
    This->orig_height      = This->height;
    This->orig_pitch       = This->pitch;
    This->orig_pixelformat = This->pixelformat;

    ICOM_INIT_INTERFACE(This, IDirectDraw7, User_DirectDraw_VTable);

    /* capabilities */
#define BLIT_CAPS (DDCAPS_BLT | DDCAPS_BLTCOLORFILL | DDCAPS_BLTDEPTHFILL \
	  | DDCAPS_BLTSTRETCH | DDCAPS_CANBLTSYSMEM | DDCAPS_CANCLIP	  \
	  | DDCAPS_CANCLIPSTRETCHED | DDCAPS_COLORKEY			  \
	  | DDCAPS_COLORKEYHWASSIST | DDCAPS_OVERLAY | DDCAPS_OVERLAYSTRETCH)
#define CKEY_CAPS (DDCKEYCAPS_DESTBLT | DDCKEYCAPS_SRCBLT)
#define FX_CAPS (DDFXCAPS_BLTALPHA | DDFXCAPS_BLTMIRRORLEFTRIGHT	\
		| DDFXCAPS_BLTMIRRORUPDOWN | DDFXCAPS_BLTROTATION90	\
		| DDFXCAPS_BLTSHRINKX | DDFXCAPS_BLTSHRINKXN		\
		| DDFXCAPS_BLTSHRINKY | DDFXCAPS_BLTSHRINKXN		\
		| DDFXCAPS_BLTSTRETCHX | DDFXCAPS_BLTSTRETCHXN		\
		| DDFXCAPS_BLTSTRETCHY | DDFXCAPS_BLTSTRETCHYN)
    This->caps.dwCaps |= DDCAPS_GDI | DDCAPS_PALETTE | BLIT_CAPS;
    if( opengl_initialized )
    {
        /* Hack for D3D code */
        This->caps.dwCaps |= DDCAPS_3D;
    }
    This->caps.dwCaps2 |= DDCAPS2_CERTIFIED | DDCAPS2_NOPAGELOCKREQUIRED |
			  DDCAPS2_PRIMARYGAMMA | DDCAPS2_WIDESURFACES;
    This->caps.dwCKeyCaps |= CKEY_CAPS;
    This->caps.dwFXCaps |= FX_CAPS;
    This->caps.dwPalCaps |= DDPCAPS_8BIT | DDPCAPS_PRIMARYSURFACE;
    This->caps.dwVidMemTotal = 16*1024*1024;
    This->caps.dwVidMemFree = 16*1024*1024;
    This->caps.dwSVBCaps |= BLIT_CAPS;
    This->caps.dwSVBCKeyCaps |= CKEY_CAPS;
    This->caps.dwSVBFXCaps |= FX_CAPS;
    This->caps.dwVSBCaps |= BLIT_CAPS;
    This->caps.dwVSBCKeyCaps |= CKEY_CAPS;
    This->caps.dwVSBFXCaps |= FX_CAPS;
    This->caps.dwSSBCaps |= BLIT_CAPS;
    This->caps.dwSSBCKeyCaps |= CKEY_CAPS;
    This->caps.dwSSBFXCaps |= FX_CAPS;
    This->caps.ddsCaps.dwCaps |= DDSCAPS_ALPHA | DDSCAPS_BACKBUFFER |
				 DDSCAPS_FLIP | DDSCAPS_FRONTBUFFER |
				 DDSCAPS_OFFSCREENPLAIN | DDSCAPS_PALETTE |
				 DDSCAPS_PRIMARYSURFACE | DDSCAPS_SYSTEMMEMORY |
				 DDSCAPS_VIDEOMEMORY | DDSCAPS_VISIBLE;
    if( opengl_initialized )
    {
        /* Hacks for D3D code */
        This->caps.ddsCaps.dwCaps |= DDSCAPS_3DDEVICE | DDSCAPS_MIPMAP | DDSCAPS_TEXTURE | DDSCAPS_ZBUFFER;
    }
    
    This->caps.ddsOldCaps.dwCaps = This->caps.ddsCaps.dwCaps;
#undef BLIT_CAPS
#undef CKEY_CAPS
#undef FX_CAPS

    return S_OK;
}

/* This function is called from DirectDrawCreate(Ex) on the most-derived
 * class to start construction.
 * Not called from the vtable. */
HRESULT User_DirectDraw_Create(const GUID* pGUID, LPDIRECTDRAW7* pIface,
			       IUnknown* pUnkOuter, BOOL ex)
{
    HRESULT hr;
    IDirectDrawImpl* This;

    assert(pUnkOuter == NULL);

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		     sizeof(IDirectDrawImpl) + sizeof(User_DirectDrawImpl));
    if (This == NULL) return E_OUTOFMEMORY;

    /* Note that this relation does *not* hold true if the DD object was
     * CoCreateInstanced then Initialized. */
    This->private = (User_DirectDrawImpl *)(This+1);

    /* Initialize the DDCAPS structure */
    This->caps.dwSize = sizeof(This->caps);

    hr = User_DirectDraw_Construct(This, ex);
    if (FAILED(hr))
	HeapFree(GetProcessHeap(), 0, This);
    else
	*pIface = ICOM_INTERFACE(This, IDirectDraw7);

    return hr;
}

/* This function is called from Uninit_DirectDraw_Initialize on the
 * most-derived-class to start initialization.
 * Not called from the vtable. */
HRESULT User_DirectDraw_Initialize(IDirectDrawImpl *This, const GUID* guid)
{
    HRESULT hr;
    This->private = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			      sizeof(User_DirectDrawImpl));
    if (This->private == NULL) return E_OUTOFMEMORY;

    /* Initialize the DDCAPS structure */
    This->caps.dwSize = sizeof(This->caps);

    hr = User_DirectDraw_Construct(This, TRUE); /* XXX ex? */
    if (FAILED(hr))
    {
	HeapFree(GetProcessHeap(), 0, This->private);
	return hr;
    }

    return DD_OK;
}

/* Called from an internal function pointer. */
void User_DirectDraw_final_release(IDirectDrawImpl *This)
{
    Main_DirectDraw_final_release(This);
}

/* Compact: generic */
/* CreateClipper: generic */
/* CreatePalette: generic (with callback) */
/* CreateSurface: generic (with callbacks) */

HRESULT
User_DirectDraw_create_primary(IDirectDrawImpl* This,
			       const DDSURFACEDESC2* pDDSD,
			       LPDIRECTDRAWSURFACE7* ppSurf,
			       IUnknown* pUnkOuter)
{
    return User_DirectDrawSurface_Create(This, pDDSD, ppSurf, pUnkOuter);
}

HRESULT
User_DirectDraw_create_backbuffer(IDirectDrawImpl* This,
				  const DDSURFACEDESC2* pDDSD,
				  LPDIRECTDRAWSURFACE7* ppSurf,
				  IUnknown* pUnkOuter,
				  IDirectDrawSurfaceImpl* primary)
{
    return User_DirectDrawSurface_Create(This, pDDSD, ppSurf, pUnkOuter);
}

/* DuplicateSurface: generic */

/* Originally derived from Xlib_IDirectDraw2Impl_EnumDisplayModes.
 *
 * The depths are whatever DIBsections support on the client side.
 * Should they be limited by screen depth?
 */
HRESULT WINAPI
User_DirectDraw_EnumDisplayModes(LPDIRECTDRAW7 iface, DWORD dwFlags,
				 LPDDSURFACEDESC2 pDDSD, LPVOID context,
				 LPDDENUMMODESCALLBACK2 callback)
{
    DDSURFACEDESC2 callback_sd;
    DEVMODEW DevModeW;
    const DDPIXELFORMAT* pixelformat;

    int i;

    TRACE("(%p)->(0x%08lx,%p,%p,%p)\n",iface,dwFlags,pDDSD,context,callback);

    if (pDDSD && TRACE_ON(ddraw))
    {
	TRACE("Enumerate modes matching:\n");
	DDRAW_dump_surface_desc(pDDSD);
    }

    ZeroMemory(&callback_sd, sizeof(callback_sd));
    callback_sd.dwSize = sizeof(callback_sd);

    callback_sd.dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT|DDSD_CAPS
	| DDSD_PITCH;

    if (dwFlags & DDEDM_REFRESHRATES)
	callback_sd.dwFlags |= DDSD_REFRESHRATE;

    callback_sd.u2.dwRefreshRate = 60.0;

    for (i = 0; EnumDisplaySettingsExW(NULL, i, &DevModeW, 0); i++)
    {
	if (pDDSD)
	{
	    if ((pDDSD->dwFlags & DDSD_WIDTH) && (pDDSD->dwWidth != DevModeW.dmPelsWidth))
		continue; 
	    if ((pDDSD->dwFlags & DDSD_HEIGHT) && (pDDSD->dwHeight != DevModeW.dmPelsHeight))
		continue; 
	    if ((pDDSD->dwFlags & DDSD_PIXELFORMAT) && (pDDSD->u4.ddpfPixelFormat.dwFlags & DDPF_RGB) &&
		(pDDSD->u4.ddpfPixelFormat.u1.dwRGBBitCount != DevModeW.dmBitsPerPel))
		    continue; 
	}

	callback_sd.dwHeight = DevModeW.dmPelsHeight;
	callback_sd.dwWidth = DevModeW.dmPelsWidth;
        if (DevModeW.dmFields & DM_DISPLAYFREQUENCY)
        {
            callback_sd.u2.dwRefreshRate = DevModeW.dmDisplayFrequency;
        }

	TRACE("- mode: %ldx%ld\n", callback_sd.dwWidth, callback_sd.dwHeight);
        
        pixelformat = pixelformat_for_depth(DevModeW.dmBitsPerPel);
        callback_sd.u1.lPitch
            = DDRAW_width_bpp_to_pitch(DevModeW.dmPelsWidth,
                                       pixelformat->u1.dwRGBBitCount);

        callback_sd.u4.ddpfPixelFormat = *pixelformat;

        callback_sd.ddsCaps.dwCaps = 0;
        if (pixelformat->dwFlags & DDPF_PALETTEINDEXED8) /* ick */
            callback_sd.ddsCaps.dwCaps |= DDSCAPS_PALETTE;

        TRACE(" - %2ld bpp, R=%08lx G=%08lx B=%08lx\n",
            callback_sd.u4.ddpfPixelFormat.u1.dwRGBBitCount,
            callback_sd.u4.ddpfPixelFormat.u2.dwRBitMask,
            callback_sd.u4.ddpfPixelFormat.u3.dwGBitMask,
            callback_sd.u4.ddpfPixelFormat.u4.dwBBitMask);
        if (callback(&callback_sd, context) == DDENUMRET_CANCEL)
            return DD_OK;
    }

    return DD_OK;
}

/* EnumSurfaces: generic */
/* FlipToGDISurface: ??? */

#if 0
HRESULT WINAPI
User_DirectDraw_GetCaps(LPDIRECTDRAW7 iface, LPDDCAPS pDriverCaps,
			LPDDCAPS pHELCaps)
{
/* Based on my guesses for what is appropriate with some clues from the
 * NVidia driver. Not everything is actually implemented yet.
 * NV has but we don't: Overlays, Video Ports, DDCAPS_READSCANLINE,
 * DDCAPS2_CERTIFIED (heh), DDSCAPS2_NONLOCALVIDMEM, DDSCAPS2_COPYFOURCC.
 * It actually has no FX alpha caps.
 * Oddly, it doesn't list DDPCAPS_PRIMARYSURFACE.
 * And the HEL caps make little sense.
 */
#define BLIT_CAPS (DDCAPS_BLT | DDCAPS_BLTCOLORFILL | DDCAPS_BLTDEPTHFILL \
	  | DDCAPS_BLTSTRETCH | DDCAPS_CANBLTSYSMEM | DDCAPS_CANCLIP	  \
	  | DDCAPS_CANCLIPSTRETCHED | DDCAPS_COLORKEY			  \
	  | DDCAPS_COLORKEYHWASSIST | DDCAPS_OVERLAY | DDCAPS_OVERLAYSTRETCH)

#define CKEY_CAPS (DDCKEYCAPS_DESTBLT | DDCKEYCAPS_SRCBLT)

#define FX_CAPS (DDFXCAPS_BLTALPHA | DDFXCAPS_BLTMIRRORLEFTRIGHT	\
		| DDFXCAPS_BLTMIRRORUPDOWN | DDFXCAPS_BLTROTATION90	\
		| DDFXCAPS_BLTSHRINKX | DDFXCAPS_BLTSHRINKXN		\
		| DDFXCAPS_BLTSHRINKY | DDFXCAPS_BLTSHRINKXN		\
		| DDFXCAPS_BLTSTRETCHX | DDFXCAPS_BLTSTRETCHXN		\
		| DDFXCAPS_BLTSTRETCHY | DDFXCAPS_BLTSTRETCHYN)

#if 0
#define ROPS { SRCCOPY, SRCPAINT, SRCAND, SRCINVERT, SRCERASE, NOTSRCCOPY, \
	NOTSRCERASE, MERGEPAINT, BLACKNESS, WHITENESS, }
#else
#define ROPS { 0, }
#endif

    static const DDCAPS caps =
    { sizeof(DDCAPS),
      DDCAPS_3D | DDCAPS_GDI | DDCAPS_PALETTE | BLIT_CAPS,
      DDCAPS2_CANMANAGETEXTURE | DDCAPS2_CANRENDERWINDOWED | DDCAPS2_CERTIFIED
      | DDCAPS2_NOPAGELOCKREQUIRED | DDCAPS2_PRIMARYGAMMA
      | DDCAPS2_WIDESURFACES,
      CKEY_CAPS,
      FX_CAPS,
      0, /* dwFXAlphaCaps */
      DDPCAPS_8BIT | DDPCAPS_PRIMARYSURFACE,
      0, /* dwSVCaps */
      0, /* ? dwAlphaBitConstBitDepths */
      0, /* ? dwAlphaBitPixelPitDepths */
      0, /* ? dwAlphaBltSurfaceBitDepths */
      0, /* ? dwAlphaOverlayConstBitDepths */
      0, /* ? dwAlphaOverlayPixelBitDepths */
      0, /* ? dwAlphaOverlaySurfaceBitDepths */
      DDBD_16, /* ? dwZBufferBitDepths */
      16*1024*1024, /* dwVidMemTotal */
      16*1024*1024, /* dwVidMemFree */
      0, /* dwMaxVisibleOverlays */
      0, /* dwCurrVisibleOverlays */
      0, /* dwNumFourCCCodes */
      0, /* dwAlignBoundarySrc */
      0, /* dwAlignSizeSrc */
      0, /* dwAlignBoundaryDest */
      0, /* dwAlignSizeDest */
      0, /* dwAlignStrideAlign */
      ROPS, /* XXX dwRops[DD_ROP_SPACE] */
      { 0, }, /* XXX ddsOldCaps */
      1000, /* dwMinOverlayStretch */
      1000, /* dwMaxOverlayStretch */
      1000, /* dwMinLiveVideoStretch */
      1000, /* dwMaxLiveVideoStretch */
      1000, /* dwMinHwCodecStretch */
      1000, /* dwMaxHwCodecStretch */
      0, 0, 0, /* dwReserved1, 2, 3 */
      BLIT_CAPS, /* dwSVBCaps */
      CKEY_CAPS, /* dwSVBCKeyCaps */
      FX_CAPS, /* dwSVBFXCaps */
      ROPS, /* dwSVBRops */
      BLIT_CAPS, /* dwVSBCaps */
      CKEY_CAPS, /* dwVSBCKeyCaps */
      FX_CAPS, /* dwVSBFXCaps */
      ROPS, /* dwVSBRops */
      BLIT_CAPS, /* dwSSBCaps */
      CKEY_CAPS, /* dwSSBCKeyCaps */
      FX_CAPS, /* dwSSBFXCaps */
      ROPS, /* dwSSBRops */
      0, /* dwMaxVideoPorts */
      0, /* dwCurrVideoPorts */
      0, /* ? dwSVBCaps2 */
      BLIT_CAPS, /* ? dwNLVBCaps */
      0, /* ? dwNLVBCaps2 */
      CKEY_CAPS, /* dwNLVBCKeyCaps */
      FX_CAPS, /* dwNLVBFXCaps */
      ROPS, /* dwNLVBRops */
      { /* ddsCaps */
	  DDSCAPS_3DDEVICE | DDSCAPS_ALPHA | DDSCAPS_BACKBUFFER | DDSCAPS_FLIP
	  | DDSCAPS_FRONTBUFFER | DDSCAPS_MIPMAP | DDSCAPS_OFFSCREENPLAIN
	  | DDSCAPS_PALETTE | DDSCAPS_PRIMARYSURFACE | DDSCAPS_SYSTEMMEMORY
	  | DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_VISIBLE
	  | DDSCAPS_ZBUFFER,
	  DDSCAPS2_CUBEMAP,
	  0,
	  0
      }
    };

#undef BLIT_CAPS
#undef CKEY_CAPS
#undef FX_CAPS
#undef ROPS

    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;

    TRACE("(%p)->(%p,%p)\n",This,pDriverCaps,pHELCaps);

    if (pDriverCaps != NULL)
	DD_STRUCT_COPY_BYSIZE(pDriverCaps,&caps);

    if (pHELCaps != NULL)
	DD_STRUCT_COPY_BYSIZE(pHELCaps,&caps);

    return DD_OK;
}
#endif

HRESULT WINAPI
User_DirectDraw_GetDeviceIdentifier(LPDIRECTDRAW7 iface,
				    LPDDDEVICEIDENTIFIER2 pDDDI,
				    DWORD dwFlags)
{
    TRACE("(%p)->(%p,%08lx)\n",iface,pDDDI,dwFlags);
    *pDDDI = user_device;
    return DD_OK;
}

/* GetDisplayMode: generic */
/* GetFourCCCodes: generic */
/* GetGDISurface: ??? */
/* GetMonitorFrequency: generic */
/* GetScanLine: generic */
/* GetSurfaceFromDC: generic */
/* GetVerticalBlankStatus: generic */
/* Initialize: generic */
/* RestoreAllSurfaces: generic */
/* RestoreDisplayMode: generic */
/* SetCooperativeLevel: ??? */

HRESULT WINAPI
User_DirectDraw_SetDisplayMode(LPDIRECTDRAW7 iface, DWORD dwWidth,
			       DWORD dwHeight, DWORD dwBPP,
			       DWORD dwRefreshRate, DWORD dwFlags)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;

    const DDPIXELFORMAT* pixelformat;
    DEVMODEW devmode;
    LONG pitch;

    TRACE("(%p)->(%ldx%ldx%ld,%ld Hz,%08lx)\n",This,dwWidth,dwHeight,dwBPP,dwRefreshRate,dwFlags);
    devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
    devmode.dmBitsPerPel = dwBPP;
    devmode.dmPelsWidth  = dwWidth;
    devmode.dmPelsHeight = dwHeight;
    /* '0' means default frequency */
    if (dwRefreshRate != 0) 
    {
	devmode.dmFields |= DM_DISPLAYFREQUENCY;
	devmode.dmDisplayFrequency = dwRefreshRate;
    }
    if (ChangeDisplaySettingsExW(NULL, &devmode, NULL, CDS_FULLSCREEN, NULL) != DISP_CHANGE_SUCCESSFUL)
	return DDERR_INVALIDMODE;

    pixelformat = pixelformat_for_depth(dwBPP);
    if (pixelformat == NULL)
    {
	assert(0);
	return DDERR_GENERIC;
    }

    pitch = DDRAW_width_bpp_to_pitch(dwWidth, dwBPP);
    return Main_DirectDraw_SetDisplayMode(iface, dwWidth, dwHeight, pitch,
					  dwRefreshRate, dwFlags, pixelformat);
}

/* StartModeTest: ??? */
/* TestCooperativeLevel: generic? */
/* WaitForVerticalBlank: ??? */

static const IDirectDraw7Vtbl User_DirectDraw_VTable =
{
    Main_DirectDraw_QueryInterface,
    Main_DirectDraw_AddRef,
    Main_DirectDraw_Release,
    Main_DirectDraw_Compact,
    Main_DirectDraw_CreateClipper,
    Main_DirectDraw_CreatePalette,
    Main_DirectDraw_CreateSurface,
    Main_DirectDraw_DuplicateSurface,
    User_DirectDraw_EnumDisplayModes,
    Main_DirectDraw_EnumSurfaces,
    Main_DirectDraw_FlipToGDISurface,
    Main_DirectDraw_GetCaps,
    Main_DirectDraw_GetDisplayMode,
    Main_DirectDraw_GetFourCCCodes,
    Main_DirectDraw_GetGDISurface,
    Main_DirectDraw_GetMonitorFrequency,
    Main_DirectDraw_GetScanLine,
    Main_DirectDraw_GetVerticalBlankStatus,
    Main_DirectDraw_Initialize,
    Main_DirectDraw_RestoreDisplayMode,
    Main_DirectDraw_SetCooperativeLevel,
    User_DirectDraw_SetDisplayMode,
    Main_DirectDraw_WaitForVerticalBlank,
    Main_DirectDraw_GetAvailableVidMem,
    Main_DirectDraw_GetSurfaceFromDC,
    Main_DirectDraw_RestoreAllSurfaces,
    Main_DirectDraw_TestCooperativeLevel,
    User_DirectDraw_GetDeviceIdentifier,
    Main_DirectDraw_StartModeTest,
    Main_DirectDraw_EvaluateMode
};
