/* Copyright 2000 TransGaming Technologies Inc. */

#ifndef DDRAW_DSURFACE_DIBTEXTURE_H_INCLUDED
#define DDRAW_DSURFACE_DIBTEXTURE_H_INCLUDED

#define DIBTEXTURE_PRIV(surf) \
	((DIBTexture_DirectDrawSurfaceImpl*)(surf->private))

#define DIBTEXTURE_PRIV_VAR(name,surf) \
	DIBTexture_DirectDrawSurfaceImpl* name = DIBTEXTURE_PRIV(surf)

/* We add a spot for 3D drivers to store some private data. A cleaner
 * solution would be to use SetPrivateData, but it's much too slow. */
union DIBTexture_data
{
    int i;
    void* p;
};

struct DIBTexture_DirectDrawSurfaceImpl_Part
{
    union DIBTexture_data data;
};

typedef struct
{
    struct DIB_DirectDrawSurfaceImpl_Part dib;
    struct DIBTexture_DirectDrawSurfaceImpl_Part dibtexture;
} DIBTexture_DirectDrawSurfaceImpl;

HRESULT
DIBTexture_DirectDrawSurface_Construct(IDirectDrawSurfaceImpl* This,
				       IDirectDrawImpl* pDD,
				       const DDSURFACEDESC2* pDDSD);

HRESULT
DIBTexture_DirectDrawSurface_Create(IDirectDrawImpl *pDD,
				    const DDSURFACEDESC2 *pDDSD,
				    LPDIRECTDRAWSURFACE7 *ppSurf,
				    IUnknown *pUnkOuter);

void DIBTexture_DirectDrawSurface_final_release(IDirectDrawSurfaceImpl* This);

HRESULT
DIBTexture_DirectDrawSurface_duplicate_surface(IDirectDrawSurfaceImpl* This,
					       LPDIRECTDRAWSURFACE7* ppDup);

#endif
