/*
 * Copyright 200 TransGaming Technologies Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef DDRAW_DSURFACE_FAKEZBUFFER_H_INCLUDED
#define DDRAW_DSURFACE_FAKEZBUFFER_H_INCLUDED

typedef struct
{
    BOOLEAN in_memory;
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
