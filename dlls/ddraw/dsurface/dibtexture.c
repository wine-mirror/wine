/*		DIB Section Texture DirectDrawSurface Driver
 *
 * Copyright 2000 TransGaming Technologies Inc.
 */

#include "config.h"
#include "winerror.h"

#include <assert.h>
#include <stdlib.h>

#include "debugtools.h"
#include "ddraw_private.h"
#include "dsurface/main.h"
#include "dsurface/dib.h"
#include "dsurface/dibtexture.h"

DEFAULT_DEBUG_CHANNEL(ddraw);

static ICOM_VTABLE(IDirectDrawSurface7) DIBTexture_IDirectDrawSurface7_VTable;

HRESULT
DIBTexture_DirectDrawSurface_Construct(IDirectDrawSurfaceImpl* This,
				       IDirectDrawImpl* pDD,
				       const DDSURFACEDESC2* pDDSD)
{
    HRESULT hr;

    hr = DIB_DirectDrawSurface_Construct(This, pDD, pDDSD);
    if (FAILED(hr)) return hr;

    ICOM_INIT_INTERFACE(This, IDirectDrawSurface7,
			DIBTexture_IDirectDrawSurface7_VTable);

    This->final_release = DIBTexture_DirectDrawSurface_final_release;
    This->duplicate_surface = DIBTexture_DirectDrawSurface_duplicate_surface;

    return S_OK;
}

HRESULT
DIBTexture_DirectDrawSurface_Create(IDirectDrawImpl *pDD,
				    const DDSURFACEDESC2 *pDDSD,
				    LPDIRECTDRAWSURFACE7 *ppSurf,
				    IUnknown *pUnkOuter)
{
    IDirectDrawSurfaceImpl* This;
    HRESULT hr;

    assert(pUnkOuter == NULL);
    assert(pDDSD->ddsCaps.dwCaps & DDSCAPS_TEXTURE);

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		     sizeof(*This) + sizeof(DIBTexture_DirectDrawSurfaceImpl));
    if (This == NULL) return E_OUTOFMEMORY;

    This->private = (DIBTexture_DirectDrawSurfaceImpl*)(This+1);

    hr = DIBTexture_DirectDrawSurface_Construct(This, pDD, pDDSD);
    if (FAILED(hr))
	HeapFree(GetProcessHeap(), 0, This);
    else
	*ppSurf = ICOM_INTERFACE(This, IDirectDrawSurface7);

    return hr;
}

void DIBTexture_DirectDrawSurface_final_release(IDirectDrawSurfaceImpl* This)
{
    DIB_DirectDrawSurface_final_release(This);
}

HRESULT
DIBTexture_DirectDrawSurface_duplicate_surface(IDirectDrawSurfaceImpl* This,
					       LPDIRECTDRAWSURFACE7* ppDup)
{
    return DIBTexture_DirectDrawSurface_Create(This->ddraw_owner,
					       &This->surface_desc, ppDup,
					       NULL);
}

static ICOM_VTABLE(IDirectDrawSurface7) DIBTexture_IDirectDrawSurface7_VTable =
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
