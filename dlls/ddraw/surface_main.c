/*		DirectDrawSurface base implementation
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
 */
#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "winerror.h"
#include "wine/debug.h"
#include "ddraw_private.h"
#include "opengl_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);
WINE_DECLARE_DEBUG_CHANNEL(ddraw_flip);
WINE_DECLARE_DEBUG_CHANNEL(ddraw_fps);

/** Creation/Destruction functions */

HRESULT
Main_DirectDrawSurface_Construct(IDirectDrawSurfaceImpl *This,
				 IDirectDrawImpl *pDD,
				 const DDSURFACEDESC2 *pDDSD)
{
    TRACE("(%p)->(%p,%p)\n", This, pDD, pDDSD);

    if (pDDSD != &This->surface_desc) {
	This->surface_desc.dwSize = sizeof(This->surface_desc);
	DD_STRUCT_COPY_BYSIZE(&(This->surface_desc),pDDSD);
    }
    This->uniqueness_value = 1; /* unchecked */
    This->ref = 1;

    This->local.lpSurfMore = &This->more;
    This->local.lpGbl = &This->global;
    This->local.dwProcessId = GetCurrentProcessId();
    This->local.dwFlags = 0; /* FIXME */
    This->local.ddsCaps.dwCaps = This->surface_desc.ddsCaps.dwCaps;
    /* FIXME: more local stuff */
    This->more.lpDD_lcl = &pDD->local;
    This->more.ddsCapsEx.dwCaps2 = This->surface_desc.ddsCaps.dwCaps2;
    This->more.ddsCapsEx.dwCaps3 = This->surface_desc.ddsCaps.dwCaps3;
    This->more.ddsCapsEx.dwCaps4 = This->surface_desc.ddsCaps.dwCaps4;
    /* FIXME: more more stuff */
    This->gmore = &This->global_more;
    This->global.u3.lpDD = pDD->local.lpGbl;
    /* FIXME: more global stuff */

    This->final_release = Main_DirectDrawSurface_final_release;
    This->late_allocate = Main_DirectDrawSurface_late_allocate;
    This->attach = Main_DirectDrawSurface_attach;
    This->detach = Main_DirectDrawSurface_detach;
    This->lock_update = Main_DirectDrawSurface_lock_update;
    This->unlock_update = Main_DirectDrawSurface_unlock_update;
    This->lose_surface = Main_DirectDrawSurface_lose_surface;
    This->set_palette    = Main_DirectDrawSurface_set_palette;
    This->update_palette = Main_DirectDrawSurface_update_palette;
    This->get_display_window = Main_DirectDrawSurface_get_display_window;
    This->get_gamma_ramp = Main_DirectDrawSurface_get_gamma_ramp;
    This->set_gamma_ramp = Main_DirectDrawSurface_set_gamma_ramp;

    ICOM_INIT_INTERFACE(This, IDirectDrawSurface3,
			DDRAW_IDDS3_Thunk_VTable);
    ICOM_INIT_INTERFACE(This, IDirectDrawGammaControl,
			DDRAW_IDDGC_VTable);

    /* There is no generic implementation of IDDS7 or texture */

    Main_DirectDraw_AddSurface(pDD, This);
    return DD_OK;
}

void Main_DirectDrawSurface_final_release(IDirectDrawSurfaceImpl* This)
{
    Main_DirectDraw_RemoveSurface(This->ddraw_owner, This);
}

HRESULT Main_DirectDrawSurface_late_allocate(IDirectDrawSurfaceImpl* This)
{
    return DD_OK;
}

static void Main_DirectDrawSurface_Destroy(IDirectDrawSurfaceImpl* This)
{
    if (This->palette) {
        IDirectDrawPalette_Release(ICOM_INTERFACE(This->palette, IDirectDrawPalette));
	This->palette = NULL;
    }
    This->final_release(This);
    if (This->private != This+1) HeapFree(GetProcessHeap(), 0, This->private);
    HeapFree(GetProcessHeap(), 0, This->tex_private);
    HeapFree(GetProcessHeap(), 0, This);
}

void Main_DirectDrawSurface_ForceDestroy(IDirectDrawSurfaceImpl* This)
{
    WARN("destroying surface %p with refcnt %lu\n", This, This->ref);
    Main_DirectDrawSurface_Destroy(This);
}

ULONG WINAPI Main_DirectDrawSurface_Release(LPDIRECTDRAWSURFACE7 iface)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): decreasing from %ld\n", This, ref + 1);
    
    if (ref == 0)
    {
	if (This->aux_release)
	    This->aux_release(This->aux_ctx, This->aux_data);
	Main_DirectDrawSurface_Destroy(This);

	TRACE("released surface %p\n", This);
	
	return 0;
    }

    return ref;
}

ULONG WINAPI Main_DirectDrawSurface_AddRef(LPDIRECTDRAWSURFACE7 iface)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): increasing from %ld\n", This, ref - 1);
    
    return ref;
}

HRESULT WINAPI
Main_DirectDrawSurface_QueryInterface(LPDIRECTDRAWSURFACE7 iface, REFIID riid,
				      LPVOID* ppObj)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), ppObj);

    *ppObj = NULL;

    if(!riid)
        return DDERR_INVALIDPARAMS;

    if (IsEqualGUID(&IID_IUnknown, riid)
	|| IsEqualGUID(&IID_IDirectDrawSurface7, riid)
	|| IsEqualGUID(&IID_IDirectDrawSurface4, riid))
    {
        InterlockedIncrement(&This->ref);
	*ppObj = ICOM_INTERFACE(This, IDirectDrawSurface7);
	return S_OK;
    }
    else if (IsEqualGUID(&IID_IDirectDrawSurface, riid)
	     || IsEqualGUID(&IID_IDirectDrawSurface2, riid)
	     || IsEqualGUID(&IID_IDirectDrawSurface3, riid))
    {
        InterlockedIncrement(&This->ref);
	*ppObj = ICOM_INTERFACE(This, IDirectDrawSurface3);
	return S_OK;
    }
    else if (IsEqualGUID(&IID_IDirectDrawGammaControl, riid))
    {
        InterlockedIncrement(&This->ref);
	*ppObj = ICOM_INTERFACE(This, IDirectDrawGammaControl);
	return S_OK;
    }
#ifdef HAVE_OPENGL
    /* interfaces following here require OpenGL */
    if( !opengl_initialized )
        return E_NOINTERFACE;

    if ( IsEqualGUID( &IID_D3DDEVICE_OpenGL, riid ) ||
	  IsEqualGUID( &IID_IDirect3DHALDevice, riid) )
    {
        IDirect3DDeviceImpl *d3ddevimpl;
	HRESULT ret_value;

	ret_value = d3ddevice_create(&d3ddevimpl, This->ddraw_owner, This, 1);
	if (FAILED(ret_value)) return ret_value;

	*ppObj = ICOM_INTERFACE(d3ddevimpl, IDirect3DDevice);
	TRACE(" returning Direct3DDevice interface at %p.\n", *ppObj);
	
	InterlockedIncrement(&This->ref); /* No idea if this is correct.. Need to check using real Windows */
	return ret_value;
    }
    else if (IsEqualGUID( &IID_IDirect3DTexture, riid ) ||
	     IsEqualGUID( &IID_IDirect3DTexture2, riid ))
    {
	HRESULT ret_value = S_OK;

	/* Note: this is not exactly how Windows does it... But this seems not to hurt the only
	         application I know creating a texture without this flag set and it will prevent
		 bugs in other parts of Wine.
	*/
	This->surface_desc.ddsCaps.dwCaps |= DDSCAPS_TEXTURE;

	/* In case the texture surface was created before the D3D creation */
	if (This->tex_private == NULL) {
   	    if (This->ddraw_owner->d3d_private == NULL) {
	        ERR("Texture created with no D3D object yet.. Not supported !\n");
		return E_NOINTERFACE;
	    }

	    ret_value = This->ddraw_owner->d3d_create_texture(This->ddraw_owner, This, FALSE, This->mip_main);
	    if (FAILED(ret_value)) return ret_value;
	}
	if (IsEqualGUID( &IID_IDirect3DTexture, riid )) {
	    *ppObj = ICOM_INTERFACE(This, IDirect3DTexture);
	    TRACE(" returning Direct3DTexture interface at %p.\n", *ppObj);
	} else {
	    *ppObj = ICOM_INTERFACE(This, IDirect3DTexture2);
	    TRACE(" returning Direct3DTexture2 interface at %p.\n", *ppObj);
	}
	InterlockedIncrement(&This->ref);
	return ret_value;
    }
#endif

    return E_NOINTERFACE;
}

/*** Callbacks */

BOOL
Main_DirectDrawSurface_attach(IDirectDrawSurfaceImpl *This,
			      IDirectDrawSurfaceImpl *to)
{
    return TRUE;
}

BOOL Main_DirectDrawSurface_detach(IDirectDrawSurfaceImpl *This)
{
    return TRUE;
}

void
Main_DirectDrawSurface_lock_update(IDirectDrawSurfaceImpl* This, LPCRECT pRect,
	DWORD dwFlags)
{
}

void
Main_DirectDrawSurface_unlock_update(IDirectDrawSurfaceImpl* This,
				     LPCRECT pRect)
{
}

void
Main_DirectDrawSurface_lose_surface(IDirectDrawSurfaceImpl* This)
{
}

void
Main_DirectDrawSurface_set_palette(IDirectDrawSurfaceImpl* This,
				   IDirectDrawPaletteImpl* pal)
{
}

void
Main_DirectDrawSurface_update_palette(IDirectDrawSurfaceImpl* This,
				      IDirectDrawPaletteImpl* pal,
				      DWORD dwStart, DWORD dwCount,
				      LPPALETTEENTRY palent)
{
}

HWND
Main_DirectDrawSurface_get_display_window(IDirectDrawSurfaceImpl* This)
{
    return 0;
}

HRESULT
Main_DirectDrawSurface_get_gamma_ramp(IDirectDrawSurfaceImpl* This,
				      DWORD dwFlags,
				      LPDDGAMMARAMP lpGammaRamp)
{
    HDC hDC;
    HRESULT hr;
    hr = This->get_dc(This, &hDC);
    if (FAILED(hr)) return hr;
    hr = GetDeviceGammaRamp(hDC, lpGammaRamp) ? DD_OK : DDERR_UNSUPPORTED;
    This->release_dc(This, hDC);
    return hr;
}

HRESULT
Main_DirectDrawSurface_set_gamma_ramp(IDirectDrawSurfaceImpl* This,
				      DWORD dwFlags,
				      LPDDGAMMARAMP lpGammaRamp)
{
    HDC hDC;
    HRESULT hr;
    hr = This->get_dc(This, &hDC);
    if (FAILED(hr)) return hr;
    hr = SetDeviceGammaRamp(hDC, lpGammaRamp) ? DD_OK : DDERR_UNSUPPORTED;
    This->release_dc(This, hDC);
    return hr;
}


/*** Interface functions */

HRESULT WINAPI
Main_DirectDrawSurface_AddAttachedSurface(LPDIRECTDRAWSURFACE7 iface,
					  LPDIRECTDRAWSURFACE7 pAttach)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    IDirectDrawSurfaceImpl* surf = ICOM_OBJECT(IDirectDrawSurfaceImpl,
					       IDirectDrawSurface7, pAttach);

    TRACE("(%p)->(%p)\n",This,pAttach);

    /* Does windows check this? */
    if (surf == This)
	return DDERR_CANNOTATTACHSURFACE; /* unchecked */

    /* Does windows check this? */
    if (surf->ddraw_owner != This->ddraw_owner)
	return DDERR_CANNOTATTACHSURFACE; /* unchecked */

    if (surf->surface_owner != NULL)
	return DDERR_SURFACEALREADYATTACHED; /* unchecked */

    /* TODO MSDN: "You can attach only z-buffer surfaces with this method."
     * But apparently backbuffers and mipmaps can be attached too. */

    /* Set MIPMAPSUBLEVEL if this seems to be one */
    if (This->surface_desc.ddsCaps.dwCaps &
	surf->surface_desc.ddsCaps.dwCaps & DDSCAPS_MIPMAP) {
	surf->surface_desc.ddsCaps.dwCaps2 |= DDSCAPS2_MIPMAPSUBLEVEL;
	/* FIXME: we should probably also add to dwMipMapCount of this
	 * and all parent surfaces (update create_texture if you do) */
    }

    /* Callback to allow the surface to do something special now that it is
     * attached. (e.g. maybe the Z-buffer tells the renderer to use it.) */
    if (!surf->attach(surf, This))
	return DDERR_CANNOTATTACHSURFACE;

    /* check: Where should it go in the chain? This puts it on the head. */
    if (This->attached)
	This->attached->prev_attached = surf;
    surf->next_attached = This->attached;
    surf->prev_attached = NULL;
    This->attached = surf;
    surf->surface_owner = This;

    IDirectDrawSurface7_AddRef(pAttach);

    return DD_OK;
}

/* MSDN: "not currently implemented." */
HRESULT WINAPI
Main_DirectDrawSurface_AddOverlayDirtyRect(LPDIRECTDRAWSURFACE7 iface,
					   LPRECT pRect)
{
    TRACE("(%p)->(%p)\n",iface,pRect);
    return DDERR_UNSUPPORTED; /* unchecked */
}

/* MSDN: "not currently implemented." */
HRESULT WINAPI
Main_DirectDrawSurface_BltBatch(LPDIRECTDRAWSURFACE7 iface,
				LPDDBLTBATCH pBatch, DWORD dwCount,
				DWORD dwFlags)
{
    TRACE("(%p)->(%p,%ld,%08lx)\n",iface,pBatch,dwCount,dwFlags);
    return DDERR_UNSUPPORTED; /* unchecked */
}

HRESULT WINAPI
Main_DirectDrawSurface_ChangeUniquenessValue(LPDIRECTDRAWSURFACE7 iface)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    volatile IDirectDrawSurfaceImpl* vThis = This;

    TRACE("(%p)\n",This);
    /* A uniquness value of 0 is apparently special.
     * This needs to be checked. */
    while (1)
    {
	DWORD old_uniqueness_value = vThis->uniqueness_value;
	DWORD new_uniqueness_value = old_uniqueness_value+1;

	if (old_uniqueness_value == 0) break;
	if (new_uniqueness_value == 0) new_uniqueness_value = 1;

	if (InterlockedCompareExchange((LONG*)&vThis->uniqueness_value,
                                       old_uniqueness_value,
                                       new_uniqueness_value)
	    == old_uniqueness_value)
	    break;
    }

    return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawSurface_DeleteAttachedSurface(LPDIRECTDRAWSURFACE7 iface,
					     DWORD dwFlags,
					     LPDIRECTDRAWSURFACE7 pAttach)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    IDirectDrawSurfaceImpl* surf = ICOM_OBJECT(IDirectDrawSurfaceImpl,
					       IDirectDrawSurface7, pAttach);

    TRACE("(%p)->(%08lx,%p)\n",This,dwFlags,pAttach);

    if (!surf || (surf->surface_owner != This))
	return DDERR_SURFACENOTATTACHED; /* unchecked */

    surf->detach(surf);

    /* Remove MIPMAPSUBLEVEL if this seemed to be one */
    if (This->surface_desc.ddsCaps.dwCaps &
	surf->surface_desc.ddsCaps.dwCaps & DDSCAPS_MIPMAP) {
	surf->surface_desc.ddsCaps.dwCaps2 &= ~DDSCAPS2_MIPMAPSUBLEVEL;
	/* FIXME: we should probably also subtract from dwMipMapCount of this
	 * and all parent surfaces */
    }

    if (surf->next_attached)
	surf->next_attached->prev_attached = surf->prev_attached;
    if (surf->prev_attached)
	surf->prev_attached->next_attached = surf->next_attached;
    if (This->attached == surf)
	This->attached = surf->next_attached;

    IDirectDrawSurface7_Release(pAttach);

    return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawSurface_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE7 iface,
					    LPVOID context,
					    LPDDENUMSURFACESCALLBACK7 cb)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    IDirectDrawSurfaceImpl* surf;
    DDSURFACEDESC2 desc;
    
    TRACE("(%p)->(%p,%p)\n",This,context,cb);

    for (surf = This->attached; surf != NULL; surf = surf->next_attached)
    {
	LPDIRECTDRAWSURFACE7 isurf = ICOM_INTERFACE(surf, IDirectDrawSurface7);

	if (TRACE_ON(ddraw)) {
	    TRACE("  => enumerating surface %p (priv. %p) with description:\n", isurf, surf);
	    DDRAW_dump_surface_desc(&surf->surface_desc);
	}

	IDirectDrawSurface7_AddRef(isurf);
	desc = surf->surface_desc;
	/* check: != DDENUMRET_OK or == DDENUMRET_CANCEL? */
	if (cb(isurf, &desc, context) == DDENUMRET_CANCEL)
	    break;
    }

    TRACE(" end of enumeration.\n");
    
    return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawSurface_EnumOverlayZOrders(LPDIRECTDRAWSURFACE7 iface,
					  DWORD dwFlags, LPVOID context,
					  LPDDENUMSURFACESCALLBACK7 cb)
{
    TRACE("(%p)->(%08lx,%p,%p)\n",iface,dwFlags,context,cb);
    return DD_OK;
}

BOOL Main_DirectDrawSurface_flip_data(IDirectDrawSurfaceImpl* front,
				      IDirectDrawSurfaceImpl* back,
				      DWORD dwFlags)
{
    /* uniqueness_value? */
    /* This is necessary. But is it safe? */
    {
	HDC tmp = front->hDC;
	front->hDC = back->hDC;
	back->hDC = tmp;
    }

    {
	BOOL tmp = front->dc_in_use;
	front->dc_in_use = back->dc_in_use;
	back->dc_in_use = tmp;
    }

    {
	FLATPTR tmp = front->global.fpVidMem;
	front->global.fpVidMem = back->global.fpVidMem;
	back->global.fpVidMem = tmp;
    }

    {
	ULONG_PTR tmp = front->global_more.hKernelSurface;
	front->global_more.hKernelSurface = back->global_more.hKernelSurface;
	back->global_more.hKernelSurface = tmp;
    }

    return TRUE;
}

/* This is unnecessarely complicated :-) */
#define MEASUREMENT_WINDOW 5
#define NUMBER_OF_WINDOWS 10

static LONGLONG perf_freq;
static LONGLONG perf_storage[NUMBER_OF_WINDOWS];
static LONGLONG prev_time = 0;
static unsigned int current_window;
static unsigned int measurements_in_window;
static unsigned int valid_windows;

HRESULT WINAPI
Main_DirectDrawSurface_Flip(LPDIRECTDRAWSURFACE7 iface,
			    LPDIRECTDRAWSURFACE7 override, DWORD dwFlags)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    IDirectDrawSurfaceImpl* target;
    HRESULT hr;

    TRACE("(%p)->(%p,%08lx)\n",This,override,dwFlags);

    if (TRACE_ON(ddraw_fps)) {
	LONGLONG current_time;
	LONGLONG frame_duration;
	QueryPerformanceCounter((LARGE_INTEGER *) &current_time);

	if (prev_time != 0) {
	    LONGLONG total_time = 0;
	    int tot_meas;
	    
	    frame_duration = current_time - prev_time;
	    prev_time = current_time;
	    
	    perf_storage[current_window] += frame_duration;
	    measurements_in_window++;
	    
	    if (measurements_in_window >= MEASUREMENT_WINDOW) {
		current_window++;
		valid_windows++;

		if (valid_windows < NUMBER_OF_WINDOWS) {
		    unsigned int i;
		    tot_meas = valid_windows * MEASUREMENT_WINDOW;
		    for (i = 0; i < valid_windows; i++) {
			total_time += perf_storage[i];
		    }
		} else {
		    int i;
		    tot_meas = NUMBER_OF_WINDOWS * MEASUREMENT_WINDOW;
		    for (i = 0; i < NUMBER_OF_WINDOWS; i++) {
			total_time += perf_storage[i];
		    }
		}

		TRACE_(ddraw_fps)(" %9.5f\n", (double) (perf_freq * tot_meas) / (double) total_time);
		
		if (current_window >= NUMBER_OF_WINDOWS) {
		    current_window = 0;
		}
		perf_storage[current_window] = 0;
		measurements_in_window = 0;
	    }
	} else {
	    prev_time = current_time;
	    memset(perf_storage, 0, sizeof(perf_storage));
	    current_window = 0;
	    valid_windows = 0;
	    measurements_in_window = 0;
	    QueryPerformanceFrequency((LARGE_INTEGER *) &perf_freq);
	}
    }
    
    /* MSDN: "This method can be called only for a surface that has the
     * DDSCAPS_FLIP and DDSCAPS_FRONTBUFFER capabilities." */
    if ((This->surface_desc.ddsCaps.dwCaps&(DDSCAPS_FLIP|DDSCAPS_FRONTBUFFER))
	!= (DDSCAPS_FLIP|DDSCAPS_FRONTBUFFER))
	return DDERR_NOTFLIPPABLE;

    if (This->aux_flip)
	if (This->aux_flip(This->aux_ctx, This->aux_data))
	    return DD_OK;

    /* 1. find the flip target */
    /* XXX I don't think this algorithm works for more than 1 backbuffer. */
    if (override == NULL)
    {
	static DDSCAPS2 back_caps = { DDSCAPS_BACKBUFFER };
	LPDIRECTDRAWSURFACE7 tgt;

	hr = IDirectDrawSurface7_GetAttachedSurface(iface, &back_caps, &tgt);
	if (FAILED(hr)) return DDERR_NOTFLIPPABLE; /* unchecked */

	target = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7,
			     tgt);
	IDirectDrawSurface7_Release(tgt);
    }
    else
    {
	BOOL on_chain = FALSE;
	IDirectDrawSurfaceImpl* surf;

	/* MSDN: "The method fails if the specified [override] surface is not
	 * a member of the flipping chain." */

	/* Verify that override is on this flip chain. We assume that
	 * surf is the head of the flipping chain, because it's the front
	 * buffer. */
	target = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7,
			     override);

	/* Either target is (indirectly) attached to This or This is
	 * (indirectly) attached to target. */
	for (surf = target; surf != NULL; surf = surf->surface_owner)
	{
	    if (surf == This)
	    {
		on_chain = TRUE;
		break;
	    }
	}

	if (!on_chain)
	    return DDERR_INVALIDPARAMS; /* unchecked */
    }

    TRACE("flip to backbuffer: %p\n",target);
    if (TRACE_ON(ddraw_flip)) {
	static unsigned int flip_count = 0;
	IDirectDrawPaletteImpl *palette;
	char buf[32];
	FILE *f;

	/* Hack for paletted games... */
	palette = target->palette;
	target->palette = This->palette;
	
	sprintf(buf, "flip_%08d.ppm", flip_count++);
	TRACE_(ddraw_flip)("Dumping file %s to disk.\n", buf);
	f = fopen(buf, "wb");
	DDRAW_dump_surface_to_disk(target, f, 8);
	target->palette = palette;
    }

    if (This->flip_data(This, target, dwFlags))
	This->flip_update(This, dwFlags);
    
    return DD_OK;
}

static PrivateData* find_private_data(IDirectDrawSurfaceImpl *This,
				      REFGUID tag)
{
    PrivateData* data;
    for (data = This->private_data; data != NULL; data = data->next)
    {
	if (IsEqualGUID(&data->tag, tag)) break;
    }

    return data;
}

HRESULT WINAPI
Main_DirectDrawSurface_FreePrivateData(LPDIRECTDRAWSURFACE7 iface, REFGUID tag)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    PrivateData *data;

    data = find_private_data(This, tag);
    if (data == NULL) return DDERR_NOTFOUND;

    if (data->prev)
	data->prev->next = data->next;
    if (data->next)
	data->next->prev = data->prev;

    if (data->flags & DDSPD_IUNKNOWNPTR)
    {
	if (data->ptr.object != NULL)
	    IUnknown_Release(data->ptr.object);
    }
    else
	HeapFree(GetProcessHeap(), 0, data->ptr.data);

    HeapFree(GetProcessHeap(), 0, data);

    return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawSurface_GetAttachedSurface(LPDIRECTDRAWSURFACE7 iface,
					  LPDDSCAPS2 pCaps,
					  LPDIRECTDRAWSURFACE7* ppSurface)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    IDirectDrawSurfaceImpl* surf;
    IDirectDrawSurfaceImpl* found = NULL;
    DDSCAPS2 our_caps;
    
    if (TRACE_ON(ddraw)) {
        TRACE("(%p)->Looking for caps: %lx,%lx,%lx,%lx output: %p\n",This,pCaps->dwCaps, pCaps->dwCaps2,
	      pCaps->dwCaps3, pCaps->dwCaps4, ppSurface);
	TRACE("   Caps are : "); DDRAW_dump_DDSCAPS2(pCaps); TRACE("\n");
    }

    our_caps = *pCaps;
    if ((This->ddraw_owner->local.dwLocalFlags & DDRAWILCL_DIRECTDRAW7) == 0) {
        /* As this is not a DirectDraw7 application, remove the garbage that some games
	   put in the new fields of the DDSCAPS2 structure. */
        our_caps.dwCaps2 = 0;
	our_caps.dwCaps3 = 0;
	our_caps.dwCaps4 = 0;
	if (TRACE_ON(ddraw)) {
	    TRACE("   Real caps are : "); DDRAW_dump_DDSCAPS2(&our_caps); TRACE("\n");
	}
    }
    
    for (surf = This->attached; surf != NULL; surf = surf->next_attached)
    {
        if (TRACE_ON(ddraw)) {
	    TRACE("Surface: (%p) caps: %lx,%lx,%lx,%lx\n", surf,
		  surf->surface_desc.ddsCaps.dwCaps,
		  surf->surface_desc.ddsCaps.dwCaps2,
		  surf->surface_desc.ddsCaps.dwCaps3,
		  surf->surface_desc.ddsCaps.dwCaps4);
	    TRACE("   Surface caps are : "); DDRAW_dump_DDSCAPS2(&(surf->surface_desc.ddsCaps)); TRACE("\n");
	}
	if (((surf->surface_desc.ddsCaps.dwCaps & our_caps.dwCaps) == our_caps.dwCaps) &&
	    ((surf->surface_desc.ddsCaps.dwCaps2 & our_caps.dwCaps2) == our_caps.dwCaps2))
	{
	    /* MSDN: "This method fails if more than one surface is attached
	     * that matches the capabilities requested." */
	    if (found != NULL)
            {
                FIXME("More than one attached surface matches requested caps.  What should we do here?\n");
                /* Previous code returned 'DDERR_NOTFOUND'.  That appears not
                   to be correct, given what 3DMark expects from MipMapped surfaces.
                   We shall just continue instead. */
            }

	    found = surf;
	}
    }

    if (found == NULL) {
        TRACE("Did not find any valid surface\n");
	return DDERR_NOTFOUND;
    }

    *ppSurface = ICOM_INTERFACE(found, IDirectDrawSurface7);

    if (TRACE_ON(ddraw)) {
        TRACE("Returning surface %p with description :\n", *ppSurface);
	DDRAW_dump_surface_desc(&(found->surface_desc));
    }
    
    /* XXX d3dframe.cpp sometimes AddRefs things that it gets from us. */
    IDirectDrawSurface7_AddRef(ICOM_INTERFACE(found, IDirectDrawSurface7));
    return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawSurface_GetBltStatus(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags)
{
    TRACE("(%p)->(%08lx)\n",iface,dwFlags);

    switch (dwFlags)
    {
    case DDGBS_CANBLT:
    case DDGBS_ISBLTDONE:
	return DD_OK;

    default:
	return DDERR_INVALIDPARAMS;
    }
}

HRESULT WINAPI
Main_DirectDrawSurface_GetCaps(LPDIRECTDRAWSURFACE7 iface, LPDDSCAPS2 pCaps)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    TRACE("(%p)->(%p)\n",This,pCaps);
    *pCaps = This->surface_desc.ddsCaps;
    return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawSurface_GetClipper(LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWCLIPPER* ppClipper)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    TRACE("(%p)->(%p)\n",This,ppClipper);
    if (This->clipper == NULL)
	return DDERR_NOCLIPPERATTACHED;

    *ppClipper = ICOM_INTERFACE(This->clipper, IDirectDrawClipper);
    IDirectDrawClipper_AddRef(ICOM_INTERFACE(This->clipper,
					     IDirectDrawClipper));
    return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawSurface_GetColorKey(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags,
				   LPDDCOLORKEY pCKey)
{
    /* There is a DDERR_NOCOLORKEY error, but how do we know if a color key
     * isn't there? That's like saying that an int isn't there. (Which MS
     * has done in other docs.) */

    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    TRACE("(%p)->(%08lx,%p)\n",This,dwFlags,pCKey);
    if (TRACE_ON(ddraw)) {
        TRACE(" - colorkey flags : ");
	DDRAW_dump_colorkeyflag(dwFlags);
    }

    switch (dwFlags)
    {
    case DDCKEY_DESTBLT:
	*pCKey = This->surface_desc.ddckCKDestBlt;
	break;

    case DDCKEY_DESTOVERLAY:
	*pCKey = This->surface_desc.u3.ddckCKDestOverlay;
	break;

    case DDCKEY_SRCBLT:
	*pCKey = This->surface_desc.ddckCKSrcBlt;
	break;

    case DDCKEY_SRCOVERLAY:
	*pCKey = This->surface_desc.ddckCKSrcOverlay;
	break;

    default:
	return DDERR_INVALIDPARAMS;
    }

    return DD_OK;
}

/* XXX We need to do something with the DC if the surface gets lost. */
HRESULT WINAPI
Main_DirectDrawSurface_GetDC(LPDIRECTDRAWSURFACE7 iface, HDC *phDC)
{
    DDSURFACEDESC2 ddsd;
    HRESULT hr;
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    TRACE("(%p)->(%p)\n",This,phDC);
    CHECK_LOST(This);

    LOCK_OBJECT(This);

    if (This->dc_in_use)
    {
	UNLOCK_OBJECT(This);
	return DDERR_DCALREADYCREATED;
    }

    /* Lock as per MSDN.
     * Strange: Lock lists DDERR_SURFACEBUSY as an error, meaning that another
     * thread has it locked, but GetDC does not. */
    ddsd.dwSize = sizeof(ddsd);
    hr = IDirectDrawSurface7_Lock(iface, NULL, &ddsd, 0, 0);
    if (FAILED(hr))
    {
	UNLOCK_OBJECT(This);
	return hr;
    }

    hr = This->get_dc(This, &This->hDC);

    if ((This->surface_desc.u4.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) &&
	(This->palette == NULL)) {
	IDirectDrawImpl *ddraw = This->ddraw_owner;
	IDirectDrawSurfaceImpl *surf;
	
	for (surf = ddraw->surfaces; surf != NULL; surf = surf->next_ddraw) {
	    if (((surf->surface_desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_FRONTBUFFER)) == (DDSCAPS_PRIMARYSURFACE | DDSCAPS_FRONTBUFFER)) &&
		(surf->palette != NULL)) {
		RGBQUAD col[256];
		IDirectDrawPaletteImpl *pal = surf->palette;
		unsigned int n;
		for (n=0; n<256; n++) {
		    col[n].rgbRed   = pal->palents[n].peRed;
		    col[n].rgbGreen = pal->palents[n].peGreen;
		    col[n].rgbBlue  = pal->palents[n].peBlue;
		    col[n].rgbReserved = 0;
		}
		SetDIBColorTable(This->hDC, 0, 256, col);
		break;
	    }
	}

    }
    
    if (SUCCEEDED(hr))
    {
	TRACE("returning %p\n",This->hDC);

	*phDC = This->hDC;
	This->dc_in_use = TRUE;
    }
    else WARN("No DC! Prepare for trouble\n");

    UNLOCK_OBJECT(This);
    return hr;
}

HRESULT WINAPI
Main_DirectDrawSurface_GetDDInterface(LPDIRECTDRAWSURFACE7 iface, LPVOID* pDD)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    TRACE("(%p)->(%p)\n",This,pDD);
    *pDD = ICOM_INTERFACE(This->ddraw_owner, IDirectDraw7);
    IDirectDraw7_AddRef(ICOM_INTERFACE(This->ddraw_owner, IDirectDraw7));
    return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawSurface_GetFlipStatus(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags)
{
    /* XXX: DDERR_INVALIDSURFACETYPE */

    TRACE("(%p)->(%08lx)\n",iface,dwFlags);
    switch (dwFlags)
    {
    case DDGFS_CANFLIP:
    case DDGFS_ISFLIPDONE:
	return DD_OK;

    default:
	return DDERR_INVALIDPARAMS;
    }
}

HRESULT WINAPI
Main_DirectDrawSurface_GetLOD(LPDIRECTDRAWSURFACE7 iface, LPDWORD pdwMaxLOD)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    TRACE("(%p)->(%p)\n",This,pdwMaxLOD);
    CHECK_TEXTURE(This);

    *pdwMaxLOD = This->max_lod;
    return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawSurface_GetOverlayPosition(LPDIRECTDRAWSURFACE7 iface,
					  LPLONG pX, LPLONG pY)
{
    return DDERR_NOTAOVERLAYSURFACE;
}

HRESULT WINAPI
Main_DirectDrawSurface_GetPalette(LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWPALETTE* ppPalette)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    TRACE("(%p)->(%p)\n",This,ppPalette);
    if (This->palette == NULL)
	return DDERR_NOPALETTEATTACHED;

    *ppPalette = ICOM_INTERFACE(This->palette, IDirectDrawPalette);
    IDirectDrawPalette_AddRef(ICOM_INTERFACE(This->palette,
					     IDirectDrawPalette));
    return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawSurface_GetPixelFormat(LPDIRECTDRAWSURFACE7 iface,
				      LPDDPIXELFORMAT pDDPixelFormat)
{
    /* What is DDERR_INVALIDSURFACETYPE for here? */
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    TRACE("(%p)->(%p)\n",This,pDDPixelFormat);
    DD_STRUCT_COPY_BYSIZE(pDDPixelFormat,&This->surface_desc.u4.ddpfPixelFormat);
    return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawSurface_GetPriority(LPDIRECTDRAWSURFACE7 iface,
				   LPDWORD pdwPriority)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    TRACE("(%p)->(%p)\n",This,pdwPriority);
    CHECK_TEXTURE(This);

    *pdwPriority = This->priority;
    return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawSurface_GetPrivateData(LPDIRECTDRAWSURFACE7 iface,
				      REFGUID tag, LPVOID pBuffer,
				      LPDWORD pcbBufferSize)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    PrivateData* data;

    TRACE("(%p)->(%p), size = %ld\n", This, pBuffer, *pcbBufferSize);

    data = find_private_data(This, tag);
    if (data == NULL) return DDERR_NOTFOUND;

    /* This may not be right. */
    if ((data->flags & DDSPD_VOLATILE)
	&& data->uniqueness_value != This->uniqueness_value)
	return DDERR_EXPIRED;

    if (*pcbBufferSize < data->size)
    {
	*pcbBufferSize = data->size;
	return DDERR_MOREDATA;
    }

    if (data->flags & DDSPD_IUNKNOWNPTR)
    {
	*(LPUNKNOWN *)pBuffer = data->ptr.object;
	IUnknown_AddRef(data->ptr.object);
    }
    else
    {
	memcpy(pBuffer, data->ptr.data, data->size);
    }

    return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawSurface_GetSurfaceDesc(LPDIRECTDRAWSURFACE7 iface,
				      LPDDSURFACEDESC2 pDDSD)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    TRACE("(%p)->(%p)\n",This,pDDSD);
    if ((pDDSD->dwSize < sizeof(DDSURFACEDESC)) ||
    	(pDDSD->dwSize > sizeof(DDSURFACEDESC2))) {
	ERR("Impossible/Strange struct size %ld.\n",pDDSD->dwSize);
	return DDERR_GENERIC;
    }

    DD_STRUCT_COPY_BYSIZE(pDDSD,&This->surface_desc);
    if (TRACE_ON(ddraw)) {
      DDRAW_dump_surface_desc(pDDSD);
    }
    return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawSurface_GetUniquenessValue(LPDIRECTDRAWSURFACE7 iface,
					  LPDWORD pValue)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    TRACE("(%p)->(%p)\n",This,pValue);
    *pValue = This->uniqueness_value;
    return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawSurface_Initialize(LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAW pDD, LPDDSURFACEDESC2 pDDSD)
{
    TRACE("(%p)->(%p,%p)\n",iface,pDD,pDDSD);
    return DDERR_ALREADYINITIALIZED;
}

HRESULT WINAPI
Main_DirectDrawSurface_IsLost(LPDIRECTDRAWSURFACE7 iface)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    TRACE("(%p) is%s lost\n",This, (This->lost ? "" : " not"));
    return This->lost ? DDERR_SURFACELOST : DD_OK;
}


/* XXX This doesn't actually do any locking or keep track of the locked
 * rectangles. The behaviour is poorly documented. */
HRESULT WINAPI
Main_DirectDrawSurface_Lock(LPDIRECTDRAWSURFACE7 iface, LPRECT prect,
			    LPDDSURFACEDESC2 pDDSD, DWORD flags, HANDLE h)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    if (TRACE_ON(ddraw)) {
        TRACE("(%p)->Lock(%p,%p,%08lx,%p)\n",This,prect,pDDSD,flags,h);
	TRACE(" - locking flags : "); DDRAW_dump_lockflag(flags);
    }
    if (WARN_ON(ddraw)) {
	if (flags & ~(DDLOCK_WAIT|DDLOCK_READONLY|DDLOCK_WRITEONLY)) {
	    WARN(" - unsupported locking flag : "); DDRAW_dump_lockflag(flags & ~(DDLOCK_WAIT|DDLOCK_READONLY|DDLOCK_WRITEONLY));
	}
    }
    if (NULL != h) {
        return DDERR_INVALIDPARAMS;
    }
    if (NULL == pDDSD) {
        return DDERR_INVALIDPARAMS;
    }

    /* If the surface is already locked, return busy */
    if (This->locked) {
        WARN(" Surface is busy, returning DDERR_SURFACEBUSY\n");
        return DDERR_SURFACEBUSY;
    }

    /* First, copy the Surface description */
    DD_STRUCT_COPY_BYSIZE(pDDSD,&(This->surface_desc));

    /* Used to optimize the D3D Device locking */
    This->lastlocktype = flags & (DDLOCK_READONLY|DDLOCK_WRITEONLY);
    
    /* If asked only for a part, change the surface pointer.
     * (Not documented.) */
    if (prect != NULL) {
	TRACE("	lprect: %ldx%ld-%ldx%ld\n",
		prect->left,prect->top,prect->right,prect->bottom);
	/* First do some sanity checkings on the rectangle we receive.
	   DungeonSiege seems to gives us once a very bad rectangle for example */
	if ((prect->top < 0) ||
	    (prect->left < 0) ||
	    (prect->bottom < 0) ||
	    (prect->right < 0) ||
	    (prect->left >= prect->right) ||
	    (prect->top >= prect->bottom) ||
	    (prect->left >= This->surface_desc.dwWidth) ||
	    (prect->right > This->surface_desc.dwWidth) ||
	    (prect->top >= This->surface_desc.dwHeight) ||
	    (prect->bottom > This->surface_desc.dwHeight)) {
	    ERR(" Invalid values in LPRECT !!!\n");
	    return DDERR_INVALIDPARAMS;
	}

	This->lock_update(This, prect, flags);

	if (pDDSD->u4.ddpfPixelFormat.dwFlags & DDPF_FOURCC) {
	    int blksize;
	    switch(pDDSD->u4.ddpfPixelFormat.dwFourCC) {
		case MAKE_FOURCC('D','X','T','1') : blksize = 8; break;
		case MAKE_FOURCC('D','X','T','3') : blksize = 16; break;
		case MAKE_FOURCC('D','X','T','5') : blksize = 16; break;
		default: return DDERR_INVALIDPIXELFORMAT;
	    }
	    pDDSD->lpSurface = (char *)This->surface_desc.lpSurface
		+ prect->top/4 * (pDDSD->dwWidth+3)/4 * blksize
	    	+ prect->left/4 * blksize;
	} else
	    pDDSD->lpSurface = (char *)This->surface_desc.lpSurface
		+ prect->top * This->surface_desc.u1.lPitch
		+ prect->left * GET_BPP(This->surface_desc);
    } else {
	This->lock_update(This, NULL, flags);
    }

    This->locked = TRUE;

    TRACE("locked surface returning description :\n");
    if (TRACE_ON(ddraw)) DDRAW_dump_surface_desc(pDDSD);
    
    return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawSurface_PageLock(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags)
{
    /* Some surface types should return DDERR_CANTPAGELOCK. */
    return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawSurface_PageUnlock(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags)
{
    /* Some surface types should return DDERR_CANTPAGEUNLOCK, and we should
     * keep track so we can return DDERR_NOTPAGELOCKED as appropriate. */
    return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawSurface_ReleaseDC(LPDIRECTDRAWSURFACE7 iface, HDC hDC)
{
    HRESULT hr;
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    TRACE("(%p)->(%p)\n",This,hDC);

    if (!This->dc_in_use || This->hDC != hDC)
	return DDERR_INVALIDPARAMS;

    This->release_dc(This, hDC);

    hr = IDirectDrawSurface7_Unlock(iface, NULL);
    if (FAILED(hr)) return hr;

    This->dc_in_use = FALSE;
    This->hDC = 0;

    return DD_OK;
}

/* Restore */

HRESULT WINAPI
Main_DirectDrawSurface_SetClipper(LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWCLIPPER pDDClipper)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    TRACE("(%p)->(%p)\n",This,pDDClipper);
    if (pDDClipper == ICOM_INTERFACE(This->clipper, IDirectDrawClipper))
	return DD_OK;

    if (This->clipper != NULL)
	IDirectDrawClipper_Release(ICOM_INTERFACE(This->clipper,
						  IDirectDrawClipper));

    This->clipper = ICOM_OBJECT(IDirectDrawClipperImpl, IDirectDrawClipper,
				pDDClipper);
    if (pDDClipper != NULL)
	IDirectDrawClipper_AddRef(pDDClipper);

    return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawSurface_SetColorKey(LPDIRECTDRAWSURFACE7 iface,
				   DWORD dwFlags, LPDDCOLORKEY pCKey)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    
    TRACE("(%p)->(%08lx,%p)\n",This,dwFlags,pCKey);

    if (TRACE_ON(ddraw)) {
        TRACE(" - colorkey flags : ");
	DDRAW_dump_colorkeyflag(dwFlags);
    }

    if ((dwFlags & DDCKEY_COLORSPACE) != 0) {
        FIXME(" colorkey value not supported (%08lx) !\n", dwFlags);
	return DDERR_INVALIDPARAMS;
    }
    
    /* TODO: investigate if this function can take multiple bits set at the same
             time (ie setting multiple colorkey values at the same time with only
	     one API call).
    */
    if (pCKey) {
        switch (dwFlags & ~DDCKEY_COLORSPACE) {
	    case DDCKEY_DESTBLT:
	        This->surface_desc.ddckCKDestBlt = *pCKey;
		This->surface_desc.dwFlags |= DDSD_CKDESTBLT;
		break;

	    case DDCKEY_DESTOVERLAY:
	        This->surface_desc.u3.ddckCKDestOverlay = *pCKey;
		This->surface_desc.dwFlags |= DDSD_CKDESTOVERLAY;
		break;

	    case DDCKEY_SRCOVERLAY:
	        This->surface_desc.ddckCKSrcOverlay = *pCKey;
		This->surface_desc.dwFlags |= DDSD_CKSRCOVERLAY;
		break;

	    case DDCKEY_SRCBLT:
	        This->surface_desc.ddckCKSrcBlt = *pCKey;
		This->surface_desc.dwFlags |= DDSD_CKSRCBLT;
		break;

	    default:
	        return DDERR_INVALIDPARAMS;
	}
    } else {
        switch (dwFlags & ~DDCKEY_COLORSPACE) {
	    case DDCKEY_DESTBLT:
		This->surface_desc.dwFlags &= ~DDSD_CKDESTBLT;
		break;

	    case DDCKEY_DESTOVERLAY:
		This->surface_desc.dwFlags &= ~DDSD_CKDESTOVERLAY;
		break;

	    case DDCKEY_SRCOVERLAY:
		This->surface_desc.dwFlags &= ~DDSD_CKSRCOVERLAY;
		break;

	    case DDCKEY_SRCBLT:
		This->surface_desc.dwFlags &= ~DDSD_CKSRCBLT;
		break;

	    default:
	        return DDERR_INVALIDPARAMS;
	}
    }

    if (This->aux_setcolorkey_cb) This->aux_setcolorkey_cb(This, dwFlags, pCKey);

    return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawSurface_SetLOD(LPDIRECTDRAWSURFACE7 iface, DWORD dwMaxLOD)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    TRACE("(%p)->(%08lx)\n",This,dwMaxLOD);
    CHECK_TEXTURE(This);

    This->max_lod = dwMaxLOD;
    return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawSurface_SetOverlayPosition(LPDIRECTDRAWSURFACE7 iface,
					  LONG X, LONG Y)
{
    return DDERR_NOTAOVERLAYSURFACE;
}

HRESULT WINAPI
Main_DirectDrawSurface_SetPalette(LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWPALETTE pPalette)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    IDirectDrawPalette *pal_to_rel = NULL;

    TRACE("(%p)->(%p)\n",This,pPalette);
    if (pPalette == ICOM_INTERFACE(This->palette, IDirectDrawPalette))
	return DD_OK;

    if (This->palette != NULL) {
	if (This->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	    This->palette->global.dwFlags &= ~DDPCAPS_PRIMARYSURFACE;
	pal_to_rel = ICOM_INTERFACE(This->palette, IDirectDrawPalette);
    }

    This->palette = ICOM_OBJECT(IDirectDrawPaletteImpl, IDirectDrawPalette,
				pPalette);
    if (pPalette != NULL) {
	IDirectDrawPalette_AddRef(pPalette);
	if (This->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	    This->palette->global.dwFlags |= DDPCAPS_PRIMARYSURFACE;
    }

    This->set_palette(This, This->palette);

    /* Do the palette release at the end to prevent doing some 'loop' when removing
     * the surface maintaining the last reference on a palette.
     */
    if (pal_to_rel != NULL)
	IDirectDrawPalette_Release(pal_to_rel);

    return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawSurface_SetPriority(LPDIRECTDRAWSURFACE7 iface,
				   DWORD dwPriority)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    TRACE("(%p)->(%08lx)\n",This,dwPriority);
    CHECK_TEXTURE(This);

    This->priority = dwPriority;
    return DD_OK;
}

/* Be careful when locking this: it is risky to call the object's AddRef
 * or Release holding a lock. */
HRESULT WINAPI
Main_DirectDrawSurface_SetPrivateData(LPDIRECTDRAWSURFACE7 iface,
				      REFGUID tag, LPVOID pData,
				      DWORD cbSize, DWORD dwFlags)
{
    PrivateData* data;
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    TRACE("(%p)->(%p), size=%ld\n", This, pData, cbSize);

    data = find_private_data(This, tag);
    if (data == NULL)
    {
	data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*data));
	if (data == NULL) return DDERR_OUTOFMEMORY;

	data->tag = *tag;
	data->flags = dwFlags;
	data->uniqueness_value = This->uniqueness_value;

	if (dwFlags & DDSPD_IUNKNOWNPTR)
	{
	    data->ptr.object = (LPUNKNOWN)pData;
	    data->size = sizeof(LPUNKNOWN);
	    IUnknown_AddRef(data->ptr.object);
	}
	else
	{
	    data->ptr.data = HeapAlloc(GetProcessHeap(), 0, cbSize);
	    if (data->ptr.data == NULL)
	    {
		HeapFree(GetProcessHeap(), 0, data);
		return DDERR_OUTOFMEMORY;
	    }

            data->size = cbSize;
            memcpy(data->ptr.data, pData, data->size);
	}

	/* link it in */
	data->next = This->private_data;
	data->prev = NULL;
	if (This->private_data)
	    This->private_data->prev = data;
	This->private_data = data;

	return DD_OK;
    }
    else
    {
	/* I don't actually know how windows handles this case. The only
	 * reason I don't just call FreePrivateData is because I want to
	 * guarantee SetPrivateData working when using LPUNKNOWN or data
	 * that is no larger than the old data. */

        FIXME("Replacing existing private data not implemented yet.\n");
	return E_FAIL;
    }
}

/* SetSurfaceDesc */

HRESULT WINAPI
Main_DirectDrawSurface_Unlock(LPDIRECTDRAWSURFACE7 iface, LPRECT pRect)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    TRACE("(%p)->Unlock(%p)\n",This,pRect);

    if (!This->locked) {
        WARN("Surface not locked - returing DDERR_NOTLOCKED\n");
        return DDERR_NOTLOCKED;
    }

    This->locked = FALSE;
    This->unlock_update(This, pRect);
    if (This->aux_unlock)
	This->aux_unlock(This->aux_ctx, This->aux_data, pRect);

    return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawSurface_UpdateOverlay(LPDIRECTDRAWSURFACE7 iface,
				     LPRECT pSrcRect,
				     LPDIRECTDRAWSURFACE7 pDstSurface,
				     LPRECT pDstRect, DWORD dwFlags,
				     LPDDOVERLAYFX pFX)
{
    return DDERR_UNSUPPORTED;
}

/* MSDN: "not currently implemented." */
HRESULT WINAPI
Main_DirectDrawSurface_UpdateOverlayDisplay(LPDIRECTDRAWSURFACE7 iface,
					    DWORD dwFlags)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DirectDrawSurface_UpdateOverlayZOrder(LPDIRECTDRAWSURFACE7 iface,
					   DWORD dwFlags,
					   LPDIRECTDRAWSURFACE7 pDDSRef)
{
    return DDERR_NOTAOVERLAYSURFACE;
}
