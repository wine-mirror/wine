/* Copyright 200 TransGaming Technologies Inc. */

#ifndef DDRAW_DSURFACE_FAKEZBUFFER_H_INCLUDED
#define DDRAW_DSURFACE_FAKEZBUFFER_H_INCLUDED

struct FakeZBuffer_DirectDrawSurfaceImpl_Part
{
};

typedef struct
{
    struct FakeZBuffer_DirectDrawSurfaceImpl_Part fakezbuffer;
} FakeZBuffer_DirectDrawSurfaceImpl;

HRESULT
FakeZBuffer_DirectDrawSurface_Construct(IDirectDrawSurfaceImpl* This,
					IDirectDrawImpl* pDD,
					const DDSURFACEDESC2* pDDSD);

HRESULT FakeZBuffer_DirectDrawSurface_Create(IDirectDrawImpl* pDD,
					     const DDSURFACEDESC2* pDDSD,
					     LPDIRECTDRAWSURFACE7* ppSurf,
					     IUnknown* pUnkOuter);

void
FakeZBuffer_DirectDrawSurface_final_release(IDirectDrawSurfaceImpl* This);

HRESULT
FakeZBuffer_DirectDrawSurface_duplicate_surface(IDirectDrawSurfaceImpl* This,
						LPDIRECTDRAWSURFACE7* ppDup);


#endif
