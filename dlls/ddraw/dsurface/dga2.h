/* Copyright 2000 TransGaming Technologies Inc. */

#ifndef DDRAW_DSURFACE_DGA2_H_INCLUDED
#define DDRAW_DSURFACE_DGA2_H_INCLUDED

#define XF86DGA2_PRIV(surf) ((XF86DGA2_DirectDrawSurfaceImpl*)((surf)->private))

#define XF86DGA2_PRIV_VAR(name,surf) \
	XF86DGA2_DirectDrawSurfaceImpl* name = XF86DGA2_PRIV(surf)

struct XF86DGA2_DirectDrawSurfaceImpl_Part
{
    LPVOID fb_addr;
    DWORD fb_pitch, fb_vofs;
    Colormap pal;
};

typedef struct
{
    struct DIB_DirectDrawSurfaceImpl_Part dib;
    struct XF86DGA2_DirectDrawSurfaceImpl_Part xf86dga2;
} XF86DGA2_DirectDrawSurfaceImpl;

HRESULT
XF86DGA2_DirectDrawSurface_Construct(IDirectDrawSurfaceImpl* This,
				     IDirectDrawImpl* pDD,
				     const DDSURFACEDESC2* pDDSD);

HRESULT
XF86DGA2_DirectDrawSurface_Create(IDirectDrawImpl *pDD,
				  const DDSURFACEDESC2 *pDDSD,
				  LPDIRECTDRAWSURFACE7 *ppSurf,
				  IUnknown *pUnkOuter);

void XF86DGA2_DirectDrawSurface_final_release(IDirectDrawSurfaceImpl* This);

void XF86DGA2_DirectDrawSurface_set_palette(IDirectDrawSurfaceImpl* This,
					    IDirectDrawPaletteImpl* pal);
void XF86DGA2_DirectDrawSurface_update_palette(IDirectDrawSurfaceImpl* This,
					       IDirectDrawPaletteImpl* pal,
					       DWORD dwStart, DWORD dwCount,
					       LPPALETTEENTRY palent);
HRESULT XF86DGA2_DirectDrawSurface_duplicate_surface(IDirectDrawSurfaceImpl* This,
						 LPDIRECTDRAWSURFACE7* ppDup);
void XF86DGA2_DirectDrawSurface_flip_data(IDirectDrawSurfaceImpl* front,
					  IDirectDrawSurfaceImpl* back);
void XF86DGA2_DirectDrawSurface_flip_update(IDirectDrawSurfaceImpl* This);
HWND XF86DGA2_DirectDrawSurface_get_display_window(IDirectDrawSurfaceImpl* This);

#endif
