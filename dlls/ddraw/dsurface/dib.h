/* Copyright 2000 TransGaming Technologies Inc. */

#ifndef DDRAW_DSURFACE_DIB_H_INCLUDED
#define DDRAW_DSURFACE_DIB_H_INCLUDED

#define DIB_PRIV(surf) ((DIB_DirectDrawSurfaceImpl*)((surf)->private))

#define DIB_PRIV_VAR(name, surf) \
	DIB_DirectDrawSurfaceImpl* name = DIB_PRIV(surf)


struct DIB_DirectDrawSurfaceImpl_Part
{
    HBITMAP DIBsection;
    void* bitmap_data;
    HGDIOBJ holdbitmap;
    BOOL client_memory;
    DWORD d3d_data[4]; /* room for Direct3D driver data */
};

typedef struct
{
    struct DIB_DirectDrawSurfaceImpl_Part dib;
} DIB_DirectDrawSurfaceImpl;

HRESULT
DIB_DirectDrawSurface_Construct(IDirectDrawSurfaceImpl *This,
				IDirectDrawImpl *pDD,
				const DDSURFACEDESC2 *pDDSD);
HRESULT
DIB_DirectDrawSurface_Create(IDirectDrawImpl *pDD,
			     const DDSURFACEDESC2 *pDDSD,
			     LPDIRECTDRAWSURFACE7 *ppSurf,
			     IUnknown *pUnkOuter);

void DIB_DirectDrawSurface_final_release(IDirectDrawSurfaceImpl* This);
BOOL DIB_DirectDrawSurface_flip_data(IDirectDrawSurfaceImpl* front,
				     IDirectDrawSurfaceImpl* back,
				     DWORD dwFlags);

void DIB_DirectDrawSurface_set_palette(IDirectDrawSurfaceImpl* This,
				       IDirectDrawPaletteImpl* pal);
void DIB_DirectDrawSurface_update_palette(IDirectDrawSurfaceImpl* This,
					  IDirectDrawPaletteImpl* pal,
					  DWORD dwStart, DWORD dwCount,
					  LPPALETTEENTRY palent);

HRESULT DIB_DirectDrawSurface_get_dc(IDirectDrawSurfaceImpl* This, HDC* phDC);
HRESULT DIB_DirectDrawSurface_release_dc(IDirectDrawSurfaceImpl* This,HDC hDC);

HRESULT DIB_DirectDrawSurface_alloc_dc(IDirectDrawSurfaceImpl* This,HDC* phDC);
HRESULT DIB_DirectDrawSurface_free_dc(IDirectDrawSurfaceImpl* This, HDC hDC);


HRESULT WINAPI
DIB_DirectDrawSurface_Blt(LPDIRECTDRAWSURFACE7 iface, LPRECT prcDest,
			  LPDIRECTDRAWSURFACE7 pSrcSurf, LPRECT prcSrc,
			  DWORD dwFlags, LPDDBLTFX pBltFx);
HRESULT WINAPI
DIB_DirectDrawSurface_BltFast(LPDIRECTDRAWSURFACE7 iface, DWORD dwX,
			      DWORD dwY, LPDIRECTDRAWSURFACE7 pSrcSurf,
			      LPRECT prcSrc, DWORD dwTrans);
HRESULT WINAPI
DIB_DirectDrawSurface_Restore(LPDIRECTDRAWSURFACE7 iface);
HRESULT WINAPI
DIB_DirectDrawSurface_SetSurfaceDesc(LPDIRECTDRAWSURFACE7 iface,
				     LPDDSURFACEDESC2 pDDSD, DWORD dwFlags);
#endif
