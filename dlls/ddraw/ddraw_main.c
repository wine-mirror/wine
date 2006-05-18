/*		DirectDraw IDirectDraw interface (generic)
 *
 * Copyright 1997-2000 Marcus Meissner
 * Copyright 1998-2000 Lionel Ulmer (most of Direct3D stuff)
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
 *
 * NOTES
 *
 * WINE currently implements a very basic set of the DirectDraw functionality
 * in graphics/ddraw.c. This implementation uses either the XFree86-DGA extension 
 * to get very fast access to the graphics card framebuffer and doublebuffering
 * features or Xlib, which is slower.
 * The implementation using XFree86-DGA is as fast as the MS equivalent for the
 * stuff that is implemented.
 *
 * Several applications already work, see below.
 * Problems of the implementation using XFree86-DGA:
 *
 *	- XFree86 cannot switch depth on the fly.
 *	  This is a problem with X and unavoidable.
 *	  Current solution is to pop up a MessageBox with an error for 
 *	  mismatched parameters and advice the user to restart the X server
 *	  with the specified depth.
 *	- The rest of the functionality that has to be implemented will have
 *	  to be done in software and will be very slow.
 *	- This requires WINE to be run as root user so XF86DGA can mmap the
 *	  framebuffer into the addressspace of the process.
 *	- Blocks all other X windowed applications.
 *
 * This file contains all the interface functions that are shared between
 * all interfaces. Or better, it is a "common stub" library for the
 * IDirectDraw* objects
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ddraw.h"
#include "d3d.h"
#include "wine/debug.h"

#include "ddraw_private.h"
#include "opengl_private.h" /* To have the D3D creation function */

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

extern const IDirectDrawVtbl DDRAW_IDirectDraw_VTable;
extern const IDirectDraw2Vtbl DDRAW_IDirectDraw2_VTable;
extern const IDirectDraw4Vtbl DDRAW_IDirectDraw4_VTable;

static void DDRAW_UnsubclassWindow(IDirectDrawImpl* This);

static void Main_DirectDraw_DeleteSurfaces(IDirectDrawImpl* This);
static void Main_DirectDraw_DeleteClippers(IDirectDrawImpl* This);
static void Main_DirectDraw_DeletePalettes(IDirectDrawImpl* This);
static void LosePrimarySurface(IDirectDrawImpl* This);

static INT32 allocate_memory(IDirectDrawImpl *This, DWORD mem) ;
static void free_memory(IDirectDrawImpl *This, DWORD mem) ;


static const char ddProp[] = "WINE_DDRAW_Property";

/* Not called from the vtable. */
HRESULT Main_DirectDraw_Construct(IDirectDrawImpl *This, BOOL ex)
{
    /* NOTE: The creator must use HEAP_ZERO_MEMORY or equivalent. */
    This->ref = 1;
    This->ex = ex;

    if (ex) This->local.dwLocalFlags |= DDRAWILCL_DIRECTDRAW7;
    This->local.dwProcessId = GetCurrentProcessId();

    This->final_release = Main_DirectDraw_final_release;

    This->create_palette = Main_DirectDrawPalette_Create;

    This->create_offscreen = Main_create_offscreen;
    This->create_texture   = Main_create_texture;
    This->create_zbuffer   = Main_create_zbuffer;
    /* There are no generic versions of create_{primary,backbuffer}. */

    ICOM_INIT_INTERFACE(This, IDirectDraw,  DDRAW_IDirectDraw_VTable);
    ICOM_INIT_INTERFACE(This, IDirectDraw2, DDRAW_IDirectDraw2_VTable);
    ICOM_INIT_INTERFACE(This, IDirectDraw4, DDRAW_IDirectDraw4_VTable);
    /* There is no generic implementation of IDD7 */

    /* This is for the moment here... */
    This->free_memory = free_memory;
    This->allocate_memory = allocate_memory;
    This->total_vidmem = 64 * 1024 * 1024;
    This->available_vidmem = This->total_vidmem;
      
    return DD_OK;
}

void Main_DirectDraw_final_release(IDirectDrawImpl* This)
{
    if (IsWindow(This->window))
    {
	if (GetPropA(This->window, ddProp))
	    DDRAW_UnsubclassWindow(This);
	else
	    FIXME("this shouldn't happen, right?\n");
    }

    Main_DirectDraw_DeleteSurfaces(This);
    Main_DirectDraw_DeleteClippers(This);
    Main_DirectDraw_DeletePalettes(This);
    if (This->local.lpGbl && This->local.lpGbl->lpExclusiveOwner == &This->local)
    {
	This->local.lpGbl->lpExclusiveOwner = NULL;
	if (This->set_exclusive_mode)
	    This->set_exclusive_mode(This, FALSE);
    }
}

/* There is no Main_DirectDraw_Create. */

ULONG WINAPI Main_DirectDraw_AddRef(LPDIRECTDRAW7 iface) {
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->() incrementing from %lu.\n", This, ref -1);

    return ref;
}

ULONG WINAPI Main_DirectDraw_Release(LPDIRECTDRAW7 iface) {
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->() decrementing from %lu.\n", This, ref +1);

    if (ref == 0)
    {
	if (This->final_release != NULL)
	    This->final_release(This);

	/* We free the private. This is an artifact of the fact that I don't
	 * have the destructors set up correctly. */
	if (This->private != (This+1))
	    HeapFree(GetProcessHeap(), 0, This->private);

	HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

HRESULT WINAPI Main_DirectDraw_QueryInterface(
    LPDIRECTDRAW7 iface,REFIID refiid,LPVOID *obj
) {
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(refiid), obj);

    /* According to COM docs, if the QueryInterface fails, obj should be set to NULL */
    *obj = NULL;
    
    if ( IsEqualGUID( &IID_IUnknown, refiid )
	 || IsEqualGUID( &IID_IDirectDraw7, refiid ) )
    {
	*obj = ICOM_INTERFACE(This, IDirectDraw7);
    }
    else if ( IsEqualGUID( &IID_IDirectDraw, refiid ) )
    {
	*obj = ICOM_INTERFACE(This, IDirectDraw);
    }
    else if ( IsEqualGUID( &IID_IDirectDraw2, refiid ) )
    {
	*obj = ICOM_INTERFACE(This, IDirectDraw2);
    }
    else if ( IsEqualGUID( &IID_IDirectDraw4, refiid ) )
    {
	*obj = ICOM_INTERFACE(This, IDirectDraw4);
    }
#ifdef HAVE_OPENGL
    else if ( IsEqualGUID( &IID_IDirect3D  , refiid ) ||
	      IsEqualGUID( &IID_IDirect3D2 , refiid ) ||
	      IsEqualGUID( &IID_IDirect3D3 , refiid ) ||
	      IsEqualGUID( &IID_IDirect3D7 , refiid ) )
    {
        if (opengl_initialized) {
	    HRESULT ret_value;

	    ret_value = direct3d_create(This);
	    if (FAILED(ret_value)) return ret_value;
	    
	    if ( IsEqualGUID( &IID_IDirect3D  , refiid ) ) {
	        *obj = ICOM_INTERFACE(This, IDirect3D);
		TRACE(" returning Direct3D interface at %p.\n", *obj);	    
	    } else if ( IsEqualGUID( &IID_IDirect3D2  , refiid ) ) {
	        *obj = ICOM_INTERFACE(This, IDirect3D2);
		TRACE(" returning Direct3D2 interface at %p.\n", *obj);	    
	    } else if ( IsEqualGUID( &IID_IDirect3D3  , refiid ) ) {
	        *obj = ICOM_INTERFACE(This, IDirect3D3);
		TRACE(" returning Direct3D3 interface at %p.\n", *obj);	    
	    } else {
	        *obj = ICOM_INTERFACE(This, IDirect3D7);
		TRACE(" returning Direct3D7 interface at %p.\n", *obj);	    
	    }
	} else {
	    ERR("Application requests a Direct3D interface but dynamic OpenGL support loading failed !\n");
	    ERR("(%p)->(%s,%p): no interface\n",This,debugstr_guid(refiid),obj);
	    return E_NOINTERFACE;
	}
    }
#else
    else if ( IsEqualGUID( &IID_IDirect3D  , refiid ) ||
	      IsEqualGUID( &IID_IDirect3D2 , refiid ) ||
	      IsEqualGUID( &IID_IDirect3D3 , refiid ) ||
	      IsEqualGUID( &IID_IDirect3D7 , refiid ) )
    {
        ERR("Application requests a Direct3D interface but OpenGL support not built-in !\n");
	ERR("(%p)->(%s,%p): no interface\n",This,debugstr_guid(refiid),obj);
	return E_NOINTERFACE;
    }
#endif
    else
    {
	FIXME("(%p)->(%s,%p): no interface\n",This,debugstr_guid(refiid),obj);
	return E_NOINTERFACE;
    }

    IDirectDraw7_AddRef(iface);
    return S_OK;
}

/* MSDN: "not currently implemented". */
HRESULT WINAPI Main_DirectDraw_Compact(LPDIRECTDRAW7 iface)
{
    TRACE("(%p)\n", iface);

    return DD_OK;
}

HRESULT WINAPI Main_DirectDraw_CreateClipper(LPDIRECTDRAW7 iface,
					     DWORD dwFlags,
					     LPDIRECTDRAWCLIPPER *ppClipper,
					     IUnknown *pUnkOuter)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    HRESULT hr;

    TRACE("(%p)->(0x%lx, %p, %p)\n", iface, dwFlags, ppClipper, pUnkOuter);

    hr = DirectDrawCreateClipper(dwFlags, ppClipper, pUnkOuter);
    if (FAILED(hr)) return hr;

    /* dwFlags is passed twice, apparently an API wart. */
    hr = IDirectDrawClipper_Initialize(*ppClipper,
				       ICOM_INTERFACE(This, IDirectDraw),
				       dwFlags);
    if (FAILED(hr))
    {
	IDirectDrawClipper_Release(*ppClipper);
	return hr;
    }

    return DD_OK;
}

HRESULT WINAPI
Main_DirectDraw_CreatePalette(LPDIRECTDRAW7 iface, DWORD dwFlags,
			      LPPALETTEENTRY palent,
			      LPDIRECTDRAWPALETTE* ppPalette,
			      LPUNKNOWN pUnknown)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    LPDIRECTDRAWPALETTE pPalette;
    HRESULT hr;

    TRACE("(%p)->(%08lx,%p,%p,%p)\n",This,dwFlags,palent,ppPalette,pUnknown);

    if (ppPalette == NULL) return E_POINTER; /* unchecked */
    if (pUnknown != NULL) return CLASS_E_NOAGGREGATION; /* unchecked */

    hr = This->create_palette(This, dwFlags, &pPalette, pUnknown);
    if (FAILED(hr)) return hr;

    hr = IDirectDrawPalette_SetEntries(pPalette, 0, 0,
				       Main_DirectDrawPalette_Size(dwFlags),
				       palent);
    if (FAILED(hr))
    {
	IDirectDrawPalette_Release(pPalette);
	return hr;
    }
    else
    {
	*ppPalette = pPalette;
	return DD_OK;
    }
}

HRESULT
Main_create_offscreen(IDirectDrawImpl* This, const DDSURFACEDESC2* pDDSD,
		      LPDIRECTDRAWSURFACE7* ppSurf, LPUNKNOWN pOuter)
{
    assert(pOuter == NULL);

    return DIB_DirectDrawSurface_Create(This, pDDSD, ppSurf, pOuter);
}

HRESULT
Main_create_texture(IDirectDrawImpl* This, const DDSURFACEDESC2* pDDSD,
		    LPDIRECTDRAWSURFACE7* ppSurf, LPUNKNOWN pOuter,
		    DWORD dwMipMapLevel)
{
    assert(pOuter == NULL);

    return DIB_DirectDrawSurface_Create(This, pDDSD, ppSurf, pOuter);
}

HRESULT
Main_create_zbuffer(IDirectDrawImpl* This, const DDSURFACEDESC2* pDDSD,
		    LPDIRECTDRAWSURFACE7* ppSurf, LPUNKNOWN pOuter)
{
    assert(pOuter == NULL);

    return FakeZBuffer_DirectDrawSurface_Create(This, pDDSD, ppSurf, pOuter);
}

/* Does the texture surface described in pDDSD have any smaller mipmaps? */
static BOOL more_mipmaps(const DDSURFACEDESC2 *pDDSD)
{
    return ((pDDSD->dwFlags & DDSD_MIPMAPCOUNT) && pDDSD->u2.dwMipMapCount > 1
	    && (pDDSD->dwWidth > 1 || pDDSD->dwHeight > 1));
}

/* Create a texture surface along with any of its mipmaps. */
static HRESULT
create_texture(IDirectDrawImpl* This, const DDSURFACEDESC2 *pDDSD,
	       LPDIRECTDRAWSURFACE7* ppSurf, LPUNKNOWN pUnkOuter)
{
    DDSURFACEDESC2 ddsd;
    DWORD mipmap_level = 0;
    HRESULT hr;

    assert(pUnkOuter == NULL);

    /* is this check right? (pixelformat can be copied from primary) */
    if ((pDDSD->dwFlags&(DDSD_HEIGHT|DDSD_WIDTH)) != (DDSD_HEIGHT|DDSD_WIDTH))
	return DDERR_INVALIDPARAMS;

    ddsd.dwSize = sizeof(ddsd);
    DD_STRUCT_COPY_BYSIZE((&ddsd),pDDSD);

    if (!(ddsd.dwFlags & DDSD_PIXELFORMAT))
    {
	ddsd.u4.ddpfPixelFormat = This->pixelformat;
    }

#ifdef HAVE_OPENGL
    /* We support for now only DXT1, DXT3 & DXT5 compressed texture formats... */
    if ((ddsd.u4.ddpfPixelFormat.dwFlags & DDPF_FOURCC) &&
        (ddsd.u4.ddpfPixelFormat.dwFourCC != MAKE_FOURCC('D','X','T','1')) &&
        (ddsd.u4.ddpfPixelFormat.dwFourCC != MAKE_FOURCC('D','X','T','3')) &&
        (ddsd.u4.ddpfPixelFormat.dwFourCC != MAKE_FOURCC('D','X','T','5')) )
    {
        return DDERR_INVALIDPIXELFORMAT;
    }

    /* Check if we can really support DXT1, DXT3 & DXT5 */
    if ((ddsd.u4.ddpfPixelFormat.dwFlags & DDPF_FOURCC) &&
	!GL_extensions.s3tc_compressed_texture && !s3tc_initialized) {
	static BOOLEAN user_warned = 0;
	if (user_warned == 0) {
	    ERR("Trying to create DXT1, DXT3 or DXT5 texture which is not supported by the video card!!!\n");
	    ERR("However there is a library libtxc_dxtn.so that can be used to do the software decompression...\n");
	    user_warned = 1;
	}
        return DDERR_INVALIDPIXELFORMAT;
    }
#else
    if (ddsd.u4.ddpfPixelFormat.dwFlags & DDPF_FOURCC)
    {
	return DDERR_INVALIDPIXELFORMAT;
    }
#endif

    if ((ddsd.u4.ddpfPixelFormat.dwFlags & DDPF_FOURCC) && !(ddsd.dwFlags & DDSD_LINEARSIZE))
    {
	int size = 0;
	int width = ddsd.dwWidth;
	int height = ddsd.dwHeight;
	switch(ddsd.u4.ddpfPixelFormat.dwFourCC) {
	    case MAKE_FOURCC('D','X','T','1'): size = ((width+3)&~3) * ((height+3)&~3) / 16 * 8; break;
	    case MAKE_FOURCC('D','X','T','3'): size = ((width+3)&~3) * ((height+3)&~3) / 16 * 16; break;
	    case MAKE_FOURCC('D','X','T','5'): size = ((width+3)&~3) * ((height+3)&~3) / 16 * 16; break;
	    default: FIXME("FOURCC not supported\n"); break;
	}
	ddsd.u1.dwLinearSize = size;
	ddsd.dwFlags |= DDSD_LINEARSIZE;
    } else if (!(ddsd.u4.ddpfPixelFormat.dwFlags & DDPF_FOURCC) && !(ddsd.dwFlags & DDSD_PITCH)) {
	ddsd.u1.lPitch = DDRAW_width_bpp_to_pitch(ddsd.dwWidth, GET_BPP(ddsd)*8);
	ddsd.dwFlags |= DDSD_PITCH;
    }

    if((ddsd.ddsCaps.dwCaps & DDSCAPS_MIPMAP) &&
        !(ddsd.dwFlags & DDSD_MIPMAPCOUNT))
    {
        if(ddsd.ddsCaps.dwCaps & DDSCAPS_COMPLEX)
        {
            /* Undocumented feature: if DDSCAPS_MIPMAP and DDSCAPS_COMPLEX are
             * both set, but mipmap count isn't given, as many mipmap levels
             * as necessary are created to get down to a size where either
             * the width or the height of the texture is 1.
             *
             * This is needed by Anarchy Online. */
            DWORD min = ddsd.dwWidth < ddsd.dwHeight ?
                        ddsd.dwWidth : ddsd.dwHeight;
            ddsd.u2.dwMipMapCount = 0;
            while( min )
            {
                ddsd.u2.dwMipMapCount++;
                min >>= 1;
            }
        }
        else
            /* Create a single mipmap. */
            ddsd.u2.dwMipMapCount = 1;
 
        ddsd.dwFlags |= DDSD_MIPMAPCOUNT;
    }
    
    ddsd.dwFlags |= DDSD_PIXELFORMAT;

    hr = This->create_texture(This, &ddsd, ppSurf, pUnkOuter, mipmap_level);
    if (FAILED(hr)) return hr;

    if (This->d3d_private) This->d3d_create_texture(This, ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, *ppSurf), TRUE, 
						    ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, *ppSurf));

    /* Create attached mipmaps if required. */
    if (more_mipmaps(&ddsd))
    {
	LPDIRECTDRAWSURFACE7 mipmap;
	LPDIRECTDRAWSURFACE7 prev_mipmap;
	DDSURFACEDESC2 mipmap_surface_desc;

	prev_mipmap = *ppSurf;
	IDirectDrawSurface7_AddRef(prev_mipmap);
	mipmap_surface_desc = ddsd;
	mipmap_surface_desc.ddsCaps.dwCaps2 |= DDSCAPS2_MIPMAPSUBLEVEL;

	while (more_mipmaps(&mipmap_surface_desc))
	{
	    IDirectDrawSurfaceImpl *mipmap_impl;
	    
	    mipmap_level++;
	    mipmap_surface_desc.u2.dwMipMapCount--;

	    if (mipmap_surface_desc.dwWidth > 1)
		mipmap_surface_desc.dwWidth /= 2;

	    if (mipmap_surface_desc.dwHeight > 1)
		mipmap_surface_desc.dwHeight /= 2;

	    if (mipmap_surface_desc.u4.ddpfPixelFormat.dwFlags & DDPF_FOURCC) {
		int size = 0;
		int width = mipmap_surface_desc.dwWidth;
		int height = mipmap_surface_desc.dwHeight;
		switch(mipmap_surface_desc.u4.ddpfPixelFormat.dwFourCC) {
		    case MAKE_FOURCC('D','X','T','1'): size = ((width+3)&~3) * ((height+3)&~3) / 16 * 8; break;
		    case MAKE_FOURCC('D','X','T','3'): size = ((width+3)&~3) * ((height+3)&~3) / 16 * 16; break;
		    case MAKE_FOURCC('D','X','T','5'): size = ((width+3)&~3) * ((height+3)&~3) / 16 * 16; break;
		    default: FIXME("FOURCC not supported\n"); break;
		}
		mipmap_surface_desc.u1.dwLinearSize = size;
	    } else {
		ddsd.u1.lPitch = DDRAW_width_bpp_to_pitch(ddsd.dwWidth, GET_BPP(ddsd)*8);
		mipmap_surface_desc.u1.lPitch
		    = DDRAW_width_bpp_to_pitch(mipmap_surface_desc.dwWidth,
					       GET_BPP(ddsd)*8);
	    }

	    hr = This->create_texture(This, &mipmap_surface_desc, &mipmap,
				      pUnkOuter, mipmap_level);
	    if (FAILED(hr))
	    {
		IDirectDrawSurface7_Release(prev_mipmap);
		IDirectDrawSurface7_Release(*ppSurf);
		return hr;
	    }
	    
	    /* This is needed for delayed mipmap creation */
	    mipmap_impl = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, mipmap);
	    mipmap_impl->mip_main = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, *ppSurf);
	    mipmap_impl->mipmap_level = mipmap_level;

	    if (This->d3d_private) This->d3d_create_texture(This, ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, mipmap), TRUE,
							    ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, *ppSurf));

	    IDirectDrawSurface7_AddAttachedSurface(prev_mipmap, mipmap);
	    IDirectDrawSurface7_Release(prev_mipmap);
	    prev_mipmap = mipmap;
	}

	IDirectDrawSurface7_Release(prev_mipmap);
    }

    return DD_OK;
}

/* Creates a primary surface and any indicated backbuffers. */
static HRESULT
create_primary(IDirectDrawImpl* This, LPDDSURFACEDESC2 pDDSD,
	       LPDIRECTDRAWSURFACE7* ppSurf, LPUNKNOWN pUnkOuter)
{
    DDSURFACEDESC2 ddsd;
    HRESULT hr;

    assert(pUnkOuter == NULL);

    if (This->primary_surface != NULL)
	return DDERR_PRIMARYSURFACEALREADYEXISTS;

    /* as documented (what about pitch?) */
    if (pDDSD->dwFlags & (DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT))
	return DDERR_INVALIDPARAMS;

    ddsd.dwSize = sizeof(ddsd);
    DD_STRUCT_COPY_BYSIZE((&ddsd),pDDSD);
    ddsd.dwFlags |= DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT;
    ddsd.dwHeight = This->height;
    ddsd.dwWidth = This->width;
    ddsd.u1.lPitch = This->pitch;
    ddsd.u4.ddpfPixelFormat = This->pixelformat;
    ddsd.ddsCaps.dwCaps |= DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY
	| DDSCAPS_VISIBLE | DDSCAPS_FRONTBUFFER;

    if ((ddsd.dwFlags & DDSD_BACKBUFFERCOUNT) && ddsd.dwBackBufferCount > 0)
	ddsd.ddsCaps.dwCaps |= DDSCAPS_FLIP;

    hr = This->create_primary(This, &ddsd, ppSurf, pUnkOuter);
    if (FAILED(hr)) return hr;

    if (ddsd.dwFlags & DDSD_BACKBUFFERCOUNT)
    {
	IDirectDrawSurfaceImpl* primary;
	LPDIRECTDRAWSURFACE7 pPrev;
	DWORD i;

	ddsd.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
	ddsd.ddsCaps.dwCaps &= ~(DDSCAPS_VISIBLE | DDSCAPS_PRIMARYSURFACE
				 | DDSCAPS_BACKBUFFER | DDSCAPS_FRONTBUFFER);

	primary = ICOM_OBJECT(IDirectDrawSurfaceImpl,IDirectDrawSurface7,
			      *ppSurf);
	pPrev = *ppSurf;
	IDirectDrawSurface7_AddRef(pPrev);

	for (i=0; i < ddsd.dwBackBufferCount; i++)
	{
	    LPDIRECTDRAWSURFACE7 pBack;

	    if (i == 0)
		ddsd.ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER;
	    else
		ddsd.ddsCaps.dwCaps &= ~DDSCAPS_BACKBUFFER;

	    hr = This->create_backbuffer(This, &ddsd, &pBack, pUnkOuter,
					 primary);

	    if (FAILED(hr))
	    {
		IDirectDraw7_Release(pPrev);
		IDirectDraw7_Release(*ppSurf);
		return hr;
	    }

	    IDirectDrawSurface7_AddAttachedSurface(pPrev, pBack);
	    IDirectDrawSurface7_Release(pPrev);
	    pPrev = pBack;
	}

	IDirectDrawSurface7_Release(pPrev);
    }

    This->primary_surface = (IDirectDrawSurfaceImpl *)*ppSurf;

    return DD_OK;
}

static HRESULT
create_offscreen(IDirectDrawImpl* This, LPDDSURFACEDESC2 pDDSD,
		 LPDIRECTDRAWSURFACE7* ppSurf, LPUNKNOWN pUnkOuter)
{
    DDSURFACEDESC2 ddsd;
    HRESULT hr;

    /* is this check right? (pixelformat can be copied from primary) */
    if ((pDDSD->dwFlags&(DDSD_HEIGHT|DDSD_WIDTH)) != (DDSD_HEIGHT|DDSD_WIDTH))
	return DDERR_INVALIDPARAMS;

    ddsd.dwSize = sizeof(ddsd);
    DD_STRUCT_COPY_BYSIZE((&ddsd),pDDSD);

    if (!(ddsd.dwFlags & DDSD_PIXELFORMAT))
    {
	ddsd.u4.ddpfPixelFormat = This->pixelformat;
    }

    if (!(ddsd.dwFlags & DDSD_PITCH))
    {
	ddsd.u1.lPitch = DDRAW_width_bpp_to_pitch(ddsd.dwWidth,
						  GET_BPP(ddsd)*8);
    }

    ddsd.dwFlags |= DDSD_PITCH | DDSD_PIXELFORMAT;

    hr = This->create_offscreen(This, &ddsd, ppSurf, pUnkOuter);
    if (FAILED(hr)) return hr;

    return hr;
}

HRESULT WINAPI
Main_DirectDraw_CreateSurface(LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD,
			      LPDIRECTDRAWSURFACE7 *ppSurf,
			      IUnknown *pUnkOuter)
{
    HRESULT hr;
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;

    TRACE("(%p)->(%p,%p,%p)\n",This,pDDSD,ppSurf,pUnkOuter);
    if (TRACE_ON(ddraw)) {
        TRACE("Requesting surface desc :\n");
        DDRAW_dump_surface_desc(pDDSD);
    }

    if (pUnkOuter != NULL) {
	FIXME("outer != NULL?\n");
	return CLASS_E_NOAGGREGATION; /* unchecked */
    }

    if (!(pDDSD->dwFlags & DDSD_CAPS)) {
	/* DVIDEO.DLL does forget the DDSD_CAPS flag ... *sigh* */
    	pDDSD->dwFlags |= DDSD_CAPS;
    }
    if (pDDSD->ddsCaps.dwCaps == 0) {
	/* This has been checked on real Windows */
	pDDSD->ddsCaps.dwCaps = DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY;
    }

    if (pDDSD->ddsCaps.dwCaps & DDSCAPS_ALLOCONLOAD) {
        /* If the surface is of the 'alloconload' type, ignore the LPSURFACE field */
        pDDSD->dwFlags &= ~DDSD_LPSURFACE;
    }

    if ((pDDSD->dwFlags & DDSD_LPSURFACE) && (pDDSD->lpSurface == NULL)) {
        /* Frank Herbert's Dune specifies a null pointer for the surface, ignore the LPSURFACE field */
        WARN("Null surface pointer specified, ignore it!\n");
        pDDSD->dwFlags &= ~DDSD_LPSURFACE;
    }

    if (ppSurf == NULL) {
	FIXME("You want to get back a surface? Don't give NULL ptrs!\n");
	return E_POINTER; /* unchecked */
    }

    if (pDDSD->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
	/* create primary surface & backbuffers */
	hr = create_primary(This, pDDSD, ppSurf, pUnkOuter);
    }
    else if (pDDSD->ddsCaps.dwCaps & DDSCAPS_BACKBUFFER)
    {
       /* create backbuffer surface */
       hr = This->create_backbuffer(This, pDDSD, ppSurf, pUnkOuter, NULL);
    }
    else if (pDDSD->ddsCaps.dwCaps & DDSCAPS_TEXTURE)
    {
	/* create texture */
	hr = create_texture(This, pDDSD, ppSurf, pUnkOuter);
    }
    else if ( (pDDSD->ddsCaps.dwCaps & DDSCAPS_ZBUFFER) &&
	     !(pDDSD->ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN)) /* Support DDSCAPS_SYSTEMMEMORY */
    {
	/* create z-buffer */
	hr = This->create_zbuffer(This, pDDSD, ppSurf, pUnkOuter);
    }
    else if ((pDDSD->ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN) ||
	     (pDDSD->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)) /* No difference in Wine right now */
    {
	/* create offscreenplain surface */
	hr = create_offscreen(This, pDDSD, ppSurf, pUnkOuter);
    }
    else
    {
	/* Otherwise, assume offscreenplain surface */
	TRACE("App didn't request a valid surface type - assuming offscreenplain\n");
	hr = create_offscreen(This, pDDSD, ppSurf, pUnkOuter);
    }

    if (FAILED(hr)) {
	FIXME("failed surface creation with code 0x%08lx\n",hr);
	return hr;
    }

    return DD_OK;
}

HRESULT WINAPI
Main_DirectDraw_DuplicateSurface(LPDIRECTDRAW7 iface, LPDIRECTDRAWSURFACE7 src,
				 LPDIRECTDRAWSURFACE7* dst)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;

    IDirectDrawSurfaceImpl *pSrc = ICOM_OBJECT(IDirectDrawSurfaceImpl,
					       IDirectDrawSurface7, src);

    TRACE("(%p)->(%p,%p)\n",This,src,dst);

    return pSrc->duplicate_surface(pSrc, dst);
}

/* EnumDisplayModes */

BOOL Main_DirectDraw_DDPIXELFORMAT_Match(const DDPIXELFORMAT *requested,
					 const DDPIXELFORMAT *provided)
{
    /* Some flags must be present in both or neither for a match. */
    static const DWORD must_match = DDPF_PALETTEINDEXED1 | DDPF_PALETTEINDEXED2
	| DDPF_PALETTEINDEXED4 | DDPF_PALETTEINDEXED8 | DDPF_FOURCC
	| DDPF_ZBUFFER | DDPF_STENCILBUFFER;

    if ((requested->dwFlags & provided->dwFlags) != requested->dwFlags)
	return FALSE;

    if ((requested->dwFlags & must_match) != (provided->dwFlags & must_match))
	return FALSE;

    if (requested->dwFlags & DDPF_FOURCC)
	if (requested->dwFourCC != provided->dwFourCC)
	    return FALSE;

    if (requested->dwFlags & (DDPF_RGB|DDPF_YUV|DDPF_ZBUFFER|DDPF_ALPHA
			      |DDPF_LUMINANCE|DDPF_BUMPDUDV))
	if (requested->u1.dwRGBBitCount != provided->u1.dwRGBBitCount)
	    return FALSE;

    if (requested->dwFlags & (DDPF_RGB|DDPF_YUV|DDPF_STENCILBUFFER
			      |DDPF_LUMINANCE|DDPF_BUMPDUDV))
	if (requested->u2.dwRBitMask != provided->u2.dwRBitMask)
	    return FALSE;

    if (requested->dwFlags & (DDPF_RGB|DDPF_YUV|DDPF_ZBUFFER|DDPF_BUMPDUDV))
	if (requested->u3.dwGBitMask != provided->u3.dwGBitMask)
	    return FALSE;

    /* I could be wrong about the bumpmapping. MSDN docs are vague. */
    if (requested->dwFlags & (DDPF_RGB|DDPF_YUV|DDPF_STENCILBUFFER
			      |DDPF_BUMPDUDV))
	if (requested->u4.dwBBitMask != provided->u4.dwBBitMask)
	    return FALSE;

    if (requested->dwFlags & (DDPF_ALPHAPIXELS|DDPF_ZPIXELS))
	if (requested->u5.dwRGBAlphaBitMask != provided->u5.dwRGBAlphaBitMask)
	    return FALSE;

    return TRUE;
}

BOOL Main_DirectDraw_DDSD_Match(const DDSURFACEDESC2* requested,
				const DDSURFACEDESC2* provided)
{
    struct compare_info
    {
	DWORD flag;
	ptrdiff_t offset;
	size_t size;
    };

#define CMP(FLAG, FIELD)				\
	{ DDSD_##FLAG, offsetof(DDSURFACEDESC2, FIELD),	\
	  sizeof(((DDSURFACEDESC2 *)(NULL))->FIELD) }

    static const struct compare_info compare[] = {
	CMP(ALPHABITDEPTH, dwAlphaBitDepth),
	CMP(BACKBUFFERCOUNT, dwBackBufferCount),
	CMP(CAPS, ddsCaps),
	CMP(CKDESTBLT, ddckCKDestBlt),
	CMP(CKDESTOVERLAY, u3.ddckCKDestOverlay),
	CMP(CKSRCBLT, ddckCKSrcBlt),
	CMP(CKSRCOVERLAY, ddckCKSrcOverlay),
	CMP(HEIGHT, dwHeight),
	CMP(LINEARSIZE, u1.dwLinearSize),
	CMP(LPSURFACE, lpSurface),
	CMP(MIPMAPCOUNT, u2.dwMipMapCount),
	CMP(PITCH, u1.lPitch),
	/* PIXELFORMAT: manual */
	CMP(REFRESHRATE, u2.dwRefreshRate),
	CMP(TEXTURESTAGE, dwTextureStage),
	CMP(WIDTH, dwWidth),
	/* ZBUFFERBITDEPTH: "obsolete" */
    };

#undef CMP

    unsigned int i;

    if ((requested->dwFlags & provided->dwFlags) != requested->dwFlags)
	return FALSE;

    for (i=0; i < sizeof(compare)/sizeof(compare[0]); i++)
    {
	if (requested->dwFlags & compare[i].flag
	    && memcmp((const char *)provided + compare[i].offset,
		      (const char *)requested + compare[i].offset,
		      compare[i].size) != 0)
	    return FALSE;
    }

    if (requested->dwFlags & DDSD_PIXELFORMAT)
    {
	if (!Main_DirectDraw_DDPIXELFORMAT_Match(&requested->u4.ddpfPixelFormat,
						 &provided->u4.ddpfPixelFormat))
	    return FALSE;
    }

    return TRUE;
}

#define DDENUMSURFACES_SEARCHTYPE (DDENUMSURFACES_CANBECREATED|DDENUMSURFACES_DOESEXIST)
#define DDENUMSURFACES_MATCHTYPE (DDENUMSURFACES_ALL|DDENUMSURFACES_MATCH|DDENUMSURFACES_NOMATCH)

/* This should be extended so that it can be used by
 * IDirectDrawSurface7::EnumAttachedSurfaces. */
HRESULT
Main_DirectDraw_EnumExistingSurfaces(IDirectDrawImpl *This, DWORD dwFlags,
				     LPDDSURFACEDESC2 lpDDSD2, LPVOID context,
				     LPDDENUMSURFACESCALLBACK7 callback)
{
    IDirectDrawSurfaceImpl *surf;
    BOOL all, nomatch;

    /* A NULL lpDDSD2 is permitted if we are enumerating all surfaces anyway */
    if (lpDDSD2 == NULL && !(dwFlags & DDENUMSURFACES_ALL))
	return DDERR_INVALIDPARAMS;

    all = dwFlags & DDENUMSURFACES_ALL;
    nomatch = dwFlags & DDENUMSURFACES_NOMATCH;

    for (surf = This->surfaces; surf != NULL; surf = surf->next_ddraw)
    {
	if (all
	    || (nomatch != Main_DirectDraw_DDSD_Match(lpDDSD2,
						      &surf->surface_desc)))
	{
	    LPDIRECTDRAWSURFACE7 isurf = ICOM_INTERFACE(surf, IDirectDrawSurface7);
	    DDSURFACEDESC2 desc;

	    if (TRACE_ON(ddraw)) {
		TRACE("  => enumerating surface %p (priv. %p) with description:\n", isurf, surf);
		DDRAW_dump_surface_desc(&surf->surface_desc);
	    }

	    IDirectDrawSurface7_AddRef(isurf);

	    desc = surf->surface_desc;
	    if (callback(isurf, &desc, context)	== DDENUMRET_CANCEL)
		break;
	}
    }
    TRACE(" end of enumeration.\n");
    
    return DD_OK;
}

/* I really don't understand how this is supposed to work.
 * We only consider dwHeight, dwWidth and ddpfPixelFormat.dwFlags. */
HRESULT
Main_DirectDraw_EnumCreateableSurfaces(IDirectDrawImpl *This, DWORD dwFlags,
				       LPDDSURFACEDESC2 lpDDSD2,
				       LPVOID context,
				       LPDDENUMSURFACESCALLBACK7 callback)
{
    FIXME("This isn't going to work.\n");

    if ((dwFlags & DDENUMSURFACES_MATCHTYPE) != DDENUMSURFACES_MATCH)
	return DDERR_INVALIDPARAMS;

    /* TODO: implement this.
     * Does this work before SCL is called?
     * Does it only consider off-screen surfaces?
     */

    return E_FAIL;
}

/* For unsigned x. 0 is not a power of 2. */
#define IS_POW_2(x) (((x) & ((x) - 1)) == 0)

HRESULT WINAPI
Main_DirectDraw_EnumSurfaces(LPDIRECTDRAW7 iface, DWORD dwFlags,
			     LPDDSURFACEDESC2 lpDDSD2, LPVOID context,
			     LPDDENUMSURFACESCALLBACK7 callback)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    TRACE("(%p)->(0x%lx, %p, %p, %p)\n", iface, dwFlags, lpDDSD2, context,
	  callback);
    if (TRACE_ON(ddraw)) {
	TRACE(" flags: "); DDRAW_dump_DDENUMSURFACES(dwFlags);
    }
    
    if (callback == NULL)
	return DDERR_INVALIDPARAMS;

    if (dwFlags & ~(DDENUMSURFACES_SEARCHTYPE|DDENUMSURFACES_MATCHTYPE))
	return DDERR_INVALIDPARAMS;

    if (!IS_POW_2(dwFlags & DDENUMSURFACES_SEARCHTYPE)
	|| !IS_POW_2(dwFlags & DDENUMSURFACES_MATCHTYPE))
	return DDERR_INVALIDPARAMS;

    if (dwFlags & DDENUMSURFACES_DOESEXIST)
    {
	return Main_DirectDraw_EnumExistingSurfaces(This, dwFlags, lpDDSD2,
						    context, callback);
    }
    else
    {
	return Main_DirectDraw_EnumCreateableSurfaces(This, dwFlags, lpDDSD2,
						      context, callback);
    }
}

HRESULT WINAPI
Main_DirectDraw_EvaluateMode(LPDIRECTDRAW7 iface,DWORD a,DWORD* b)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    FIXME("(%p)->() stub\n", This);

    return DD_OK;
}

HRESULT WINAPI
Main_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7 iface)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    TRACE("(%p)->()\n",This);
    return DD_OK;
}

HRESULT WINAPI
Main_DirectDraw_GetCaps(LPDIRECTDRAW7 iface, LPDDCAPS pDriverCaps,
			LPDDCAPS pHELCaps)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    TRACE("(%p,%p,%p)\n",This,pDriverCaps,pHELCaps);
    if (pDriverCaps != NULL) {
	DD_STRUCT_COPY_BYSIZE(pDriverCaps,&This->caps);
	if (TRACE_ON(ddraw)) {
	  TRACE("Driver Caps :\n");
	  DDRAW_dump_DDCAPS(pDriverCaps);
	}
    }
    if (pHELCaps != NULL) {
	DD_STRUCT_COPY_BYSIZE(pHELCaps,&This->caps);
	if (TRACE_ON(ddraw)) {
	  TRACE("HEL Caps :\n");
	  DDRAW_dump_DDCAPS(pHELCaps);
	}
    }
    return DD_OK;
}

/* GetCaps */
/* GetDeviceIdentifier */
/* GetDIsplayMode */

HRESULT WINAPI
Main_DirectDraw_GetFourCCCodes(LPDIRECTDRAW7 iface, LPDWORD pNumCodes,
			       LPDWORD pCodes)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    if (*pNumCodes) {
	    *pNumCodes=0;
    }
    FIXME("(%p,%p,%p), stub\n",This,pNumCodes,pCodes);
    return DD_OK;
}

HRESULT WINAPI
Main_DirectDraw_GetGDISurface(LPDIRECTDRAW7 iface,
			      LPDIRECTDRAWSURFACE7 *lplpGDIDDSSurface)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    TRACE("(%p)->(%p)\n", This, lplpGDIDDSSurface);
    TRACE("returning primary (%p)\n", This->primary_surface);
    *lplpGDIDDSSurface = ICOM_INTERFACE(This->primary_surface, IDirectDrawSurface7);
    if (*lplpGDIDDSSurface)
	IDirectDrawSurface7_AddRef(*lplpGDIDDSSurface);
    return DD_OK;
}

HRESULT WINAPI
Main_DirectDraw_GetMonitorFrequency(LPDIRECTDRAW7 iface,LPDWORD freq)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    FIXME("(%p)->(%p) returns 60 Hz always\n",This,freq);
    *freq = 60*100; /* 60 Hz */
    return DD_OK;
}

HRESULT WINAPI
Main_DirectDraw_GetScanLine(LPDIRECTDRAW7 iface, LPDWORD lpdwScanLine)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    static BOOL hide;

    /* Since this method is called often, show the fixme only once */
    if (!hide) {
	FIXME("(%p)->(%p) semi-stub\n", This, lpdwScanLine);
	hide = TRUE;
    }

    /* Fake the line sweeping of the monitor */
    /* FIXME: We should synchronize with a source to keep the refresh rate */ 
    *lpdwScanLine = This->cur_scanline++;
    /* Assume 20 scan lines in the vertical blank */
    if (This->cur_scanline >= This->height + 20)
	This->cur_scanline = 0;

    return DD_OK;
}

HRESULT WINAPI
Main_DirectDraw_GetSurfaceFromDC(LPDIRECTDRAW7 iface, HDC hdc,
				 LPDIRECTDRAWSURFACE7 *lpDDS)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    FIXME("(%p)->(%p,%p)\n", This, hdc, lpDDS);

    return DD_OK;
}

HRESULT WINAPI
Main_DirectDraw_GetVerticalBlankStatus(LPDIRECTDRAW7 iface, LPBOOL status)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    TRACE("(%p)->(%p)\n",This,status);
    *status = This->fake_vblank;
    This->fake_vblank = !This->fake_vblank;
    return DD_OK;
}

/* If we were not initialised then Uninit_Main_IDirectDraw7_Initialize would
 * have been called instead. */
HRESULT WINAPI
Main_DirectDraw_Initialize(LPDIRECTDRAW7 iface, LPGUID lpGuid)
{
    TRACE("(%p)->(%s)\n", iface, debugstr_guid(lpGuid));

    return DDERR_ALREADYINITIALIZED;
}

HRESULT WINAPI
Main_DirectDraw_RestoreAllSurfaces(LPDIRECTDRAW7 iface)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    IDirectDrawSurfaceImpl* surf;

    TRACE("(%p)->()\n", This);

    for (surf = This->surfaces; surf != NULL; surf = surf->next_ddraw)
	IDirectDrawSurface7_Restore(ICOM_INTERFACE(surf, IDirectDrawSurface7));

    return DD_OK;
}

static void DDRAW_SubclassWindow(IDirectDrawImpl* This)
{
    /* Well we don't actually subclass the window yet. */
    SetPropA(This->window, ddProp, This);
}

static void DDRAW_UnsubclassWindow(IDirectDrawImpl* This)
{
    RemovePropA(This->window, ddProp);
}

HRESULT WINAPI
Main_DirectDraw_SetCooperativeLevel(LPDIRECTDRAW7 iface, HWND hwnd,
				    DWORD cooplevel)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;

    FIXME("(%p)->(%p,%08lx)\n",This,hwnd,cooplevel);
    DDRAW_dump_cooperativelevel(cooplevel);

    /* Makes realMYST test happy. */
    if (This->cooperative_level == cooplevel
	&& This->window == hwnd)
	return DD_OK;

    /* XXX "It cannot be reset while the process has surfaces or palettes
     * created." Otherwise the window can be changed???
     *
     * This appears to be wrong - comment it out for now.
     * This seems to be true at least for DDSCL_SETFOCUSWINDOW
     * It looks like Windows doesn't store the HWND in all cases,
     * probably if DDSCL_NORMAL is specified, but that's not sure
    if (This->window)
	return DDERR_HWNDALREADYSET;
    */

    /* DDSCL_EXCLUSIVE or DDSCL_NORMAL or DDSCL_SETFOCUSWINDOW must be given */
    if (!(cooplevel & (DDSCL_EXCLUSIVE|DDSCL_NORMAL|DDSCL_SETFOCUSWINDOW)))
    {
        ERR("(%p) : Call to SetCooperativeLevel failed: cooplevel  != DDSCL_EXCLUSIVE|DDSCL_NORMAL|DDSCL_SETFOCUSWINDOW, returning DDERR_INVALIDPARAMS\n", This);
        return DDERR_INVALIDPARAMS;
    }
    /* Device window and focus Window. They only really matter in a
     * Multi-Monitor application, but some games specify them and we
     * have to react correctly. */
    if(cooplevel & DDSCL_SETFOCUSWINDOW)
    {
        /* This flag is a biest: It is only valid when DDSCL_NORMAL has been set
         * or no hwnd is set and no other flags are allowed, except DDSCL_NOWINDOWCHANGES
         */
        if(This->window)
            if(!(This->cooperative_level & DDSCL_NORMAL))
            {
                ERR("(%p) : Call to SetCooperativeLevel failed: DDSCL_SETFOCUSWINDOW may not be used in Cooplevel %08lx, returning DDERR_HWNDALREADYSET\n",
                            This, This->cooperative_level);
                return DDERR_HWNDALREADYSET;
            }
        if((cooplevel != DDSCL_SETFOCUSWINDOW))
            if(cooplevel != (DDSCL_SETFOCUSWINDOW | DDSCL_NOWINDOWCHANGES) )
            {
                ERR("(%p) : Call to SetCooperativeLevel failed: Invalid use of DDSCL_SETFOCUSWINDOW, returning DDERR_INVALIDPARAMS\n", This);
                return DDERR_INVALIDPARAMS;
            }

        /* Don't know what exactly to do, but it's perfectly valid
         * to pass DDSCL_SETFOCUSWINDOW only */
        FIXME("(%p) : Poorly handled flag DDSCL_SETFOCUSWINDOW\n", This);

        /* Store the flag in the cooperative level. I don't think that all other
         * flags should be overwritten, so just add it 
         * (In the most cases this will be DDSCL_SETFOCUSWINDOW | DDSCL_NORMAL) */
        cooplevel |= DDSCL_SETFOCUSWINDOW;

        return DD_OK;
    }

    /* DDSCL_EXCLUSE mode requires  DDSCL_FULLSCREEN and vice versa */
    if((cooplevel & DDSCL_EXCLUSIVE) && !(cooplevel & DDSCL_FULLSCREEN))
        return DDERR_INVALIDPARAMS;
    /* The other case is checked above */

    /* Unhandled flags. Give a warning */
    if(cooplevel & DDSCL_SETDEVICEWINDOW)
        FIXME("(%p) : Unhandled flag DDSCL_SETDEVICEWINDOW.\n", This);
    if(cooplevel & DDSCL_CREATEDEVICEWINDOW)
        FIXME("(%p) : Unhandled flag DDSCL_CREATEDEVICEWINDOW.\n", This);

    /* Perhaps the hwnd is only set in DDSCL_EXLUSIVE and DDSCL_FULLSCREEN mode. Not sure */
    This->window = hwnd;
    This->cooperative_level = cooplevel;

    This->local.hWnd = (ULONG_PTR)hwnd;
    This->local.dwLocalFlags |= DDRAWILCL_SETCOOPCALLED;
    /* not entirely sure about these */
    if (cooplevel & DDSCL_EXCLUSIVE)     This->local.dwLocalFlags |= DDRAWILCL_HASEXCLUSIVEMODE;
    if (cooplevel & DDSCL_FULLSCREEN)    This->local.dwLocalFlags |= DDRAWILCL_ISFULLSCREEN;
    if (cooplevel & DDSCL_ALLOWMODEX)    This->local.dwLocalFlags |= DDRAWILCL_ALLOWMODEX;
    if (cooplevel & DDSCL_MULTITHREADED) This->local.dwLocalFlags |= DDRAWILCL_MULTITHREADED;
    if (cooplevel & DDSCL_FPUSETUP)      This->local.dwLocalFlags |= DDRAWILCL_FPUSETUP;
    if (cooplevel & DDSCL_FPUPRESERVE)   This->local.dwLocalFlags |= DDRAWILCL_FPUPRESERVE;

    if (This->local.lpGbl) {
	/* assume that this app is the active app (in wine, there's
	 * probably only one app per global ddraw object anyway) */
	if (cooplevel & DDSCL_EXCLUSIVE) This->local.lpGbl->lpExclusiveOwner = &This->local;
	else if (This->local.lpGbl->lpExclusiveOwner == &This->local)
	    This->local.lpGbl->lpExclusiveOwner = NULL;
	if (This->set_exclusive_mode)
	    This->set_exclusive_mode(This, (cooplevel & DDSCL_EXCLUSIVE) != 0);
    }

    ShowWindow(hwnd, SW_SHOW);

    DDRAW_SubclassWindow(This);

    /* TODO Does it also get resized to the current screen size? */

    return DD_OK;
}

HRESULT WINAPI
Main_DirectDraw_SetDisplayMode(LPDIRECTDRAW7 iface, DWORD dwWidth,
			       DWORD dwHeight, LONG lPitch,
			       DWORD dwRefreshRate, DWORD dwFlags,
			       const DDPIXELFORMAT* pixelformat)
{
    short screenX;
    short screenY;

    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;

    TRACE("(%p)->SetDisplayMode(%ld,%ld)\n",This,dwWidth,dwHeight);

    if (!(This->cooperative_level & DDSCL_EXCLUSIVE))
	return DDERR_NOEXCLUSIVEMODE;

    if (!IsWindow(This->window))
	return DDERR_GENERIC; /* unchecked */

    LosePrimarySurface(This);

    screenX = GetSystemMetrics(SM_CXSCREEN);
    screenY = GetSystemMetrics(SM_CYSCREEN);

    This->width = dwWidth;
    This->height = dwHeight;
    This->pitch = lPitch;
    This->pixelformat = *pixelformat;

    /* Position the window in the center of the screen - don't center for now */
    /* MoveWindow(This->window, (screenX-dwWidth)/2, (screenY-dwHeight)/2,
                  dwWidth, dwHeight, TRUE);*/
    MoveWindow(This->window, 0, 0, dwWidth, dwHeight, TRUE);

    SetFocus(This->window);

    return DD_OK;
}

HRESULT WINAPI
Main_DirectDraw_RestoreDisplayMode(LPDIRECTDRAW7 iface)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;

    TRACE("(%p)\n",This);
    if (!(This->cooperative_level & DDSCL_EXCLUSIVE))
	return DDERR_NOEXCLUSIVEMODE;

    /* Lose the primary surface if the resolution changes. */
    if (This->orig_width != This->width || This->orig_height != This->height
	|| This->orig_pitch != This->pitch
	|| This->orig_pixelformat.dwFlags != This->pixelformat.dwFlags
	|| !Main_DirectDraw_DDPIXELFORMAT_Match(&This->pixelformat,
						&This->orig_pixelformat))
    {
	LosePrimarySurface(This);
    }

    /* TODO Move the window back where it belongs. */

    return DD_OK;
}

HRESULT WINAPI
Main_DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW7 iface, DWORD dwFlags,
				     HANDLE h)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    FIXME("(%p)->(flags=0x%08lx,handle=%p)\n",This,dwFlags,h);
    return DD_OK;
}

HRESULT WINAPI
Main_DirectDraw_GetDisplayMode(LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    TRACE("(%p)->GetDisplayMode(%p)\n",This,pDDSD);

    pDDSD->dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PITCH|DDSD_PIXELFORMAT|DDSD_REFRESHRATE;
    pDDSD->dwHeight = This->height;
    pDDSD->dwWidth = This->width;
    pDDSD->u1.lPitch = This->pitch;
    pDDSD->u2.dwRefreshRate = 60;
    pDDSD->u4.ddpfPixelFormat = This->pixelformat;
    pDDSD->ddsCaps.dwCaps = 0;

    return DD_OK;
}

static INT32 allocate_memory(IDirectDrawImpl *This, DWORD mem)
{
    if (mem > This->available_vidmem) return -1;
    This->available_vidmem -= mem;
    return This->available_vidmem;
}

static void free_memory(IDirectDrawImpl *This, DWORD mem)
{
    This->available_vidmem += mem;
}

HRESULT WINAPI
Main_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 iface, LPDDSCAPS2 ddscaps,
				   LPDWORD total, LPDWORD free)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    TRACE("(%p)->(%p,%p,%p)\n", This,ddscaps,total,free);

    if (TRACE_ON(ddraw)) {
        TRACE(" Asking for memory of type : ");
        DDRAW_dump_DDSCAPS2(ddscaps); TRACE("\n");
    }

    /* We have 16 MB videomemory */
    if (total)	*total= This->total_vidmem;
    if (free)	*free = This->available_vidmem;

    TRACE(" returning (total) %ld / (free) %ld\n", 
	  total != NULL ? *total : 0, 
	  free  != NULL ? *free  : 0);
    
    return DD_OK;
}

HRESULT WINAPI Main_DirectDraw_TestCooperativeLevel(LPDIRECTDRAW7 iface) {
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    TRACE("(%p)->(): stub\n", This);

    return DD_OK;
}

HRESULT WINAPI
Main_DirectDraw_StartModeTest(LPDIRECTDRAW7 iface, LPSIZE pModes,
			      DWORD dwNumModes, DWORD dwFlags)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    FIXME("(%p)->() stub\n", This);

    return DD_OK;
}

/*** Owned object management. */

void Main_DirectDraw_AddSurface(IDirectDrawImpl* This,
				IDirectDrawSurfaceImpl* surface)
{
    assert(surface->ddraw_owner == NULL || surface->ddraw_owner == This);

    surface->ddraw_owner = This;

    /* where should it go? */
    surface->next_ddraw = This->surfaces;
    surface->prev_ddraw = NULL;
    if (This->surfaces)
	This->surfaces->prev_ddraw = surface;
    This->surfaces = surface;
}

void Main_DirectDraw_RemoveSurface(IDirectDrawImpl* This,
				   IDirectDrawSurfaceImpl* surface)
{
    assert(surface->ddraw_owner == This);

    if (This->surfaces == surface)
	This->surfaces = surface->next_ddraw;

    if (This->primary_surface == surface)
	This->primary_surface = NULL;

    if (surface->next_ddraw)
	surface->next_ddraw->prev_ddraw = surface->prev_ddraw;
    if (surface->prev_ddraw)
	surface->prev_ddraw->next_ddraw = surface->next_ddraw;
}

static void Main_DirectDraw_DeleteSurfaces(IDirectDrawImpl* This)
{
    while (This->surfaces != NULL)
	Main_DirectDrawSurface_ForceDestroy(This->surfaces);
}

void Main_DirectDraw_AddClipper(IDirectDrawImpl* This,
				IDirectDrawClipperImpl* clipper)
{
    assert(clipper->ddraw_owner == NULL || clipper->ddraw_owner == This);

    clipper->ddraw_owner = This;

    clipper->next_ddraw = This->clippers;
    clipper->prev_ddraw = NULL;
    if (This->clippers)
	This->clippers->prev_ddraw = clipper;
    This->clippers = clipper;
}

void Main_DirectDraw_RemoveClipper(IDirectDrawImpl* This,
				   IDirectDrawClipperImpl* clipper)
{
    assert(clipper->ddraw_owner == This);

    if (This->clippers == clipper)
	This->clippers = clipper->next_ddraw;

    if (clipper->next_ddraw)
	clipper->next_ddraw->prev_ddraw = clipper->prev_ddraw;
    if (clipper->prev_ddraw)
	clipper->prev_ddraw->next_ddraw = clipper->next_ddraw;
}

static void Main_DirectDraw_DeleteClippers(IDirectDrawImpl* This)
{
    while (This->clippers != NULL)
	Main_DirectDrawClipper_ForceDestroy(This->clippers);
}

void Main_DirectDraw_AddPalette(IDirectDrawImpl* This,
				IDirectDrawPaletteImpl* palette)
{
    assert(palette->ddraw_owner == NULL || palette->ddraw_owner == This);

    palette->ddraw_owner = This;

    /* where should it go? */
    palette->next_ddraw = This->palettes;
    palette->prev_ddraw = NULL;
    if (This->palettes)
	This->palettes->prev_ddraw = palette;
    This->palettes = palette;
}

void Main_DirectDraw_RemovePalette(IDirectDrawImpl* This,
				   IDirectDrawPaletteImpl* palette)
{
    IDirectDrawSurfaceImpl *surf;

    assert(palette->ddraw_owner == This);

    if (This->palettes == palette)
	This->palettes = palette->next_ddraw;

    if (palette->next_ddraw)
	palette->next_ddraw->prev_ddraw = palette->prev_ddraw;
    if (palette->prev_ddraw)
	palette->prev_ddraw->next_ddraw = palette->next_ddraw;

    /* Here we need also to remove tha palette from any surface which has it as the
     * current palette (checked on Windows)
     */
    for (surf = This->surfaces; surf != NULL; surf = surf->next_ddraw) {
	if (surf->palette == palette) {
	    TRACE("Palette %p attached to surface %p.\n", palette, surf);
	    surf->palette = NULL;
	    surf->set_palette(surf, NULL);
	}
    }
}

static void Main_DirectDraw_DeletePalettes(IDirectDrawImpl* This)
{
    while (This->palettes != NULL)
	Main_DirectDrawPalette_ForceDestroy(This->palettes);
}

/*** ??? */

static void
LoseSurface(IDirectDrawSurfaceImpl *surface)
{
    if (surface != NULL) surface->lose_surface(surface);
}

static void
LosePrimarySurface(IDirectDrawImpl *This)
{
    /* MSDN: "If another application changes the display mode, the primary
     * surface is lost, and the method returns DDERR_SURFACELOST until the
     * primary surface is recreated to match the new display mode."
     *
     * We mark all the primary surfaces as lost as soon as the display
     * mode is changed (by any application). */

    LoseSurface(This->primary_surface);
}

/******************************************************************************
 * Uninitialised DirectDraw functions
 *
 * This vtable is used when a DirectDraw object is created with
 * CoCreateInstance. The only usable method is Initialize.
 */

void Uninit_DirectDraw_final_release(IDirectDrawImpl *This)
{
    Main_DirectDraw_final_release(This);
}

static const IDirectDraw7Vtbl Uninit_DirectDraw_VTable;

/* Not called from the vtable. */
HRESULT Uninit_DirectDraw_Construct(IDirectDrawImpl *This, BOOL ex)
{
    HRESULT hr;

    hr = Main_DirectDraw_Construct(This, ex);
    if (FAILED(hr)) return hr;

    This->final_release = Uninit_DirectDraw_final_release;
    ICOM_INIT_INTERFACE(This, IDirectDraw7, Uninit_DirectDraw_VTable);

    return S_OK;
}

HRESULT Uninit_DirectDraw_Create(const GUID* pGUID,
				       LPDIRECTDRAW7* pIface,
				       IUnknown* pUnkOuter, BOOL ex)
{
    HRESULT hr;
    IDirectDrawImpl* This;

    assert(pUnkOuter == NULL); /* XXX no: we must check this */

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		     sizeof(IDirectDrawImpl));
    if (This == NULL) return E_OUTOFMEMORY;

    hr = Uninit_DirectDraw_Construct(This, ex);
    if (FAILED(hr))
	HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, This);
    else
	*pIface = ICOM_INTERFACE(This, IDirectDraw7);

    return hr;
}

static HRESULT WINAPI
Uninit_DirectDraw_Initialize(LPDIRECTDRAW7 iface, LPGUID pDeviceGuid)
{
    const ddraw_driver* driver;
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;

    TRACE("(%p)->(%p)\n", iface, pDeviceGuid);

    driver = DDRAW_FindDriver(pDeviceGuid);
    /* XXX This return value is not documented. (Not checked.) */
    if (driver == NULL) return DDERR_INVALIDDIRECTDRAWGUID;

    return driver->init(This, pDeviceGuid);
}

static HRESULT WINAPI
Uninit_DirectDraw_Compact(LPDIRECTDRAW7 iface)
{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_CreateClipper(LPDIRECTDRAW7 iface, DWORD dwFlags,
				LPDIRECTDRAWCLIPPER *lplpDDClipper,
				IUnknown *pUnkOuter)

{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_CreatePalette(LPDIRECTDRAW7 iface, DWORD dwFlags,
				LPPALETTEENTRY lpColorTable,
				LPDIRECTDRAWPALETTE *lplpDDPalette,
				IUnknown *pUnkOuter)
{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_CreateSurface(LPDIRECTDRAW7 iface,
				LPDDSURFACEDESC2 lpDDSurfaceDesc,
				LPDIRECTDRAWSURFACE7 *lplpDDSurface,
				IUnknown *pUnkOuter)
{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_DuplicateSurface(LPDIRECTDRAW7 iface,
				   LPDIRECTDRAWSURFACE7 pSurf,
				   LPDIRECTDRAWSURFACE7 *pDupSurf)

{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_EnumDisplayModes(LPDIRECTDRAW7 iface, DWORD dwFlags,
				   LPDDSURFACEDESC2 lpDDSD,
				   LPVOID context,
				   LPDDENUMMODESCALLBACK2 cb)
{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_EnumSurfaces(LPDIRECTDRAW7 iface, DWORD dwFlags,
			       LPDDSURFACEDESC2 pDDSD, LPVOID context,
			       LPDDENUMSURFACESCALLBACK7 cb)
{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7 iface)
{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_GetCaps(LPDIRECTDRAW7 iface, LPDDCAPS pDriverCaps,
			  LPDDCAPS pHELCaps)
{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_GetDisplayMode(LPDIRECTDRAW7 iface,
				 LPDDSURFACEDESC2 pDDSD)
{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_GetFourCCCodes(LPDIRECTDRAW7 iface, LPDWORD pNumCodes,
				 LPDWORD pCodes)
{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_GetGDISurface(LPDIRECTDRAW7 iface,
				LPDIRECTDRAWSURFACE7 *pGDISurf)
{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_GetMonitorFrequency(LPDIRECTDRAW7 iface, LPDWORD pdwFreq)
{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_GetScanLine(LPDIRECTDRAW7 iface, LPDWORD pdwScanLine)
{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_GetVerticalBlankStatus(LPDIRECTDRAW7 iface, PBOOL pbIsInVB)
{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_RestoreDisplayMode(LPDIRECTDRAW7 iface)
{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_SetCooperativeLevel(LPDIRECTDRAW7 iface, HWND hWnd,
				      DWORD dwFlags)
{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_SetDisplayMode(LPDIRECTDRAW7 iface, DWORD dwWidth,
				 DWORD dwHeight, DWORD dwBPP,
				 DWORD dwRefreshRate, DWORD dwFlags)
{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW7 iface, DWORD dwFlags,
				       HANDLE hEvent)
{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 iface, LPDDSCAPS2 pDDCaps,
				     LPDWORD pdwTotal, LPDWORD pdwFree)
{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_GetSurfaceFromDC(LPDIRECTDRAW7 iface, HDC hDC,
				   LPDIRECTDRAWSURFACE7 *pSurf)
{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_RestoreAllSurfaces(LPDIRECTDRAW7 iface)
{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_TestCooperativeLevel(LPDIRECTDRAW7 iface)
{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_GetDeviceIdentifier(LPDIRECTDRAW7 iface,
				      LPDDDEVICEIDENTIFIER2 pDDDI,
				      DWORD dwFlags)
{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_StartModeTest(LPDIRECTDRAW7 iface, LPSIZE pszModes,
				DWORD cModes, DWORD dwFlags)
{
    return DDERR_NOTINITIALIZED;
}

static HRESULT WINAPI
Uninit_DirectDraw_EvaluateMode(LPDIRECTDRAW7 iface, DWORD dwFlags,
			       LPDWORD pTimeout)
{
    return DDERR_NOTINITIALIZED;
}

static const IDirectDraw7Vtbl Uninit_DirectDraw_VTable =
{
    Main_DirectDraw_QueryInterface,
    Main_DirectDraw_AddRef,
    Main_DirectDraw_Release,
    Uninit_DirectDraw_Compact,
    Uninit_DirectDraw_CreateClipper,
    Uninit_DirectDraw_CreatePalette,
    Uninit_DirectDraw_CreateSurface,
    Uninit_DirectDraw_DuplicateSurface,
    Uninit_DirectDraw_EnumDisplayModes,
    Uninit_DirectDraw_EnumSurfaces,
    Uninit_DirectDraw_FlipToGDISurface,
    Uninit_DirectDraw_GetCaps,
    Uninit_DirectDraw_GetDisplayMode,
    Uninit_DirectDraw_GetFourCCCodes,
    Uninit_DirectDraw_GetGDISurface,
    Uninit_DirectDraw_GetMonitorFrequency,
    Uninit_DirectDraw_GetScanLine,
    Uninit_DirectDraw_GetVerticalBlankStatus,
    Uninit_DirectDraw_Initialize,
    Uninit_DirectDraw_RestoreDisplayMode,
    Uninit_DirectDraw_SetCooperativeLevel,
    Uninit_DirectDraw_SetDisplayMode,
    Uninit_DirectDraw_WaitForVerticalBlank,
    Uninit_DirectDraw_GetAvailableVidMem,
    Uninit_DirectDraw_GetSurfaceFromDC,
    Uninit_DirectDraw_RestoreAllSurfaces,
    Uninit_DirectDraw_TestCooperativeLevel,
    Uninit_DirectDraw_GetDeviceIdentifier,
    Uninit_DirectDraw_StartModeTest,
    Uninit_DirectDraw_EvaluateMode
};
