/*		DIBSection DirectDrawSurface driver
 *
 * Copyright 1997-2000 Marcus Meissner
 * Copyright 1998-2000 Lionel Ulmer
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
#include <stdlib.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "winerror.h"
#include "wine/debug.h"
#include "ddraw_private.h"
#include "d3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

/* FIXME */
extern HBITMAP DIB_CreateDIBSection( HDC hdc, const BITMAPINFO *bmi, UINT usage, VOID **bits,
                                     HANDLE section, DWORD offset, DWORD ovr_pitch );

static const IDirectDrawSurface7Vtbl DIB_IDirectDrawSurface7_VTable;

/* Return the width of a DIB bitmap in bytes. DIB bitmap data is 32-bit aligned. */
inline static int get_dib_width_bytes( int width, int depth )
{
    int words;

    switch(depth)
    {
    case 1:  words = (width + 31) / 32; break;
    case 4:  words = (width + 7) / 8; break;
    case 8:  words = (width + 3) / 4; break;
    case 15:
    case 16: words = (width + 1) / 2; break;
    case 24: words = (width * 3 + 3)/4; break;
    default:
        WARN("(%d): Unsupported depth\n", depth );
        /* fall through */
    case 32: words = width; break;
    }
    return 4 * words;
}


static HRESULT create_dib(IDirectDrawSurfaceImpl* This)
{
    BITMAPINFO* b_info;
    UINT usage;
    HDC ddc;
    DIB_DirectDrawSurfaceImpl* priv = This->private;

    assert(This->surface_desc.lpSurface != NULL);

    switch (This->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount)
    {
    case 16:
    case 32:
	/* Allocate extra space to store the RGB bit masks. */
	b_info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                           sizeof(BITMAPINFOHEADER) + 3 * sizeof(DWORD));
	break;

    case 24:
	b_info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                           sizeof(BITMAPINFOHEADER));
	break;

    default:
	/* Allocate extra space for a palette. */
	b_info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                           sizeof(BITMAPINFOHEADER)
                           + sizeof(RGBQUAD)
                           * (1 << This->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount));
	break;
    }

    b_info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    b_info->bmiHeader.biWidth = This->surface_desc.dwWidth;
    b_info->bmiHeader.biHeight = -This->surface_desc.dwHeight;
    b_info->bmiHeader.biPlanes = 1;
    b_info->bmiHeader.biBitCount = This->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount;

    if ((This->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount != 16)
	&& (This->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount != 32))
        b_info->bmiHeader.biCompression = BI_RGB;
    else
        b_info->bmiHeader.biCompression = BI_BITFIELDS;

    b_info->bmiHeader.biSizeImage
	= (This->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount / 8)
	* This->surface_desc.dwWidth * This->surface_desc.dwHeight;

    b_info->bmiHeader.biXPelsPerMeter = 0;
    b_info->bmiHeader.biYPelsPerMeter = 0;
    b_info->bmiHeader.biClrUsed = 0;
    b_info->bmiHeader.biClrImportant = 0;

    if (!This->surface_desc.u1.lPitch) {
	/* This can't happen, right? */
	/* or use GDI_GetObj to get it from the created DIB? */
	This->surface_desc.u1.lPitch = get_dib_width_bytes(b_info->bmiHeader.biWidth, b_info->bmiHeader.biBitCount);
	This->surface_desc.dwFlags |= DDSD_PITCH;
    }
    
    switch (This->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount)
    {
    case 16:
    case 32:
    {
	DWORD *masks = (DWORD *) &(b_info->bmiColors);

	usage = 0;
	masks[0] = This->surface_desc.u4.ddpfPixelFormat.u2.dwRBitMask;
	masks[1] = This->surface_desc.u4.ddpfPixelFormat.u3.dwGBitMask;
	masks[2] = This->surface_desc.u4.ddpfPixelFormat.u4.dwBBitMask;
    }
    break;

    case 24:
	/* Nothing to do */
	usage = DIB_RGB_COLORS;
	break;

    default:
	/* Don't know palette */
	usage = 0;
	break;
    }

    ddc = CreateDCA("DISPLAY", NULL, NULL, NULL);
    if (ddc == 0)
    {
	HeapFree(GetProcessHeap(), 0, b_info);
	return HRESULT_FROM_WIN32(GetLastError());
    }

    priv->dib.DIBsection
	= DIB_CreateDIBSection(ddc, b_info, usage, &(priv->dib.bitmap_data), 0,
			       (DWORD)This->surface_desc.lpSurface,
			       This->surface_desc.u1.lPitch);
    DeleteDC(ddc);
    if (!priv->dib.DIBsection) {
	ERR("CreateDIBSection failed!\n");
	HeapFree(GetProcessHeap(), 0, b_info);
	return HRESULT_FROM_WIN32(GetLastError());
    }

    TRACE("DIBSection at : %p\n", priv->dib.bitmap_data);

    if (!This->surface_desc.lpSurface) {
	This->surface_desc.lpSurface = priv->dib.bitmap_data;
	This->surface_desc.dwFlags |= DDSD_LPSURFACE;
    }

    HeapFree(GetProcessHeap(), 0, b_info);

    /* I don't think it's worth checking for this. */
    if (priv->dib.bitmap_data != This->surface_desc.lpSurface)
	ERR("unexpected error creating DirectDrawSurface DIB section\n");

    /* this seems like a good place to put the handle for HAL driver use */
    This->global_more.hKernelSurface = (ULONG_PTR)priv->dib.DIBsection;

    return S_OK;
}

void DIB_DirectDrawSurface_final_release(IDirectDrawSurfaceImpl* This)
{
    DIB_DirectDrawSurfaceImpl* priv = This->private;

    DeleteObject(priv->dib.DIBsection);

    if (!priv->dib.client_memory)
	VirtualFree(This->surface_desc.lpSurface, 0, MEM_RELEASE);

    Main_DirectDrawSurface_final_release(This);
}

HRESULT DIB_DirectDrawSurface_duplicate_surface(IDirectDrawSurfaceImpl* This,
						LPDIRECTDRAWSURFACE7* ppDup)
{
    return DIB_DirectDrawSurface_Create(This->ddraw_owner,
					&This->surface_desc, ppDup, NULL);
}

HRESULT DIB_DirectDrawSurface_Construct(IDirectDrawSurfaceImpl *This,
					IDirectDrawImpl *pDD,
					const DDSURFACEDESC2 *pDDSD)
{
    HRESULT hr;
    DIB_DirectDrawSurfaceImpl* priv = This->private;

    TRACE("(%p)->(%p,%p)\n",This,pDD,pDDSD);
    hr = Main_DirectDrawSurface_Construct(This, pDD, pDDSD);
    if (FAILED(hr)) return hr;

    ICOM_INIT_INTERFACE(This, IDirectDrawSurface7,
			DIB_IDirectDrawSurface7_VTable);

    This->final_release = DIB_DirectDrawSurface_final_release;
    This->duplicate_surface = DIB_DirectDrawSurface_duplicate_surface;
    This->flip_data = DIB_DirectDrawSurface_flip_data;

    This->get_dc     = DIB_DirectDrawSurface_get_dc;
    This->release_dc = DIB_DirectDrawSurface_release_dc;
    This->hDC = NULL;

    This->set_palette    = DIB_DirectDrawSurface_set_palette;
    This->update_palette = DIB_DirectDrawSurface_update_palette;

    TRACE("(%ldx%ld, pitch=%ld)\n",
	  This->surface_desc.dwWidth, This->surface_desc.dwHeight,
	  This->surface_desc.u1.lPitch);
    /* XXX load dwWidth and dwHeight from pDD if they are not specified? */

    if (This->surface_desc.dwFlags & DDSD_LPSURFACE)
    {
	/* "Client memory": it is managed by the application. */
	/* XXX What if lPitch is not set? Use dwWidth or fail? */

	priv->dib.client_memory = TRUE;
    }
    else
    {
	if (!(This->surface_desc.dwFlags & DDSD_PITCH))
	{
	    int pitch = This->surface_desc.u1.lPitch;
	    if (pitch % 8 != 0)
		pitch += 8 - (pitch % 8);
	}
	/* XXX else: how should lPitch be verified? */

	This->surface_desc.dwFlags |= DDSD_LPSURFACE;

	/* Ensure that DDSD_PITCH is respected for DDPF_FOURCC surfaces too */
 	if (This->surface_desc.u4.ddpfPixelFormat.dwFlags & DDPF_FOURCC && !(This->surface_desc.dwFlags & DDSD_PITCH)) {
	    This->surface_desc.lpSurface
		= VirtualAlloc(NULL, This->surface_desc.u1.dwLinearSize, MEM_COMMIT, PAGE_READWRITE);
	    This->surface_desc.dwFlags |= DDSD_LINEARSIZE;
	} else {
	    This->surface_desc.lpSurface
		= VirtualAlloc(NULL, This->surface_desc.u1.lPitch
			   * This->surface_desc.dwHeight + 4, /* The + 4 here is for dumb games reading after the end of the surface
								 when reading the last byte / half using word access */
			   MEM_COMMIT, PAGE_READWRITE);
	    This->surface_desc.dwFlags |= DDSD_PITCH;
	}

	if (This->surface_desc.lpSurface == NULL)
	{
	    Main_DirectDrawSurface_final_release(This);
	    return HRESULT_FROM_WIN32(GetLastError());
	}

	priv->dib.client_memory = FALSE;
    }

    hr = create_dib(This);
    if (FAILED(hr))
    {
	if (!priv->dib.client_memory)
	    VirtualFree(This->surface_desc.lpSurface, 0, MEM_RELEASE);

	Main_DirectDrawSurface_final_release(This);

	return hr;
    }

    return DD_OK;
}

/* Not an API */
HRESULT DIB_DirectDrawSurface_Create(IDirectDrawImpl *pDD,
				     const DDSURFACEDESC2 *pDDSD,
				     LPDIRECTDRAWSURFACE7 *ppSurf,
				     IUnknown *pUnkOuter)
{
    IDirectDrawSurfaceImpl* This;
    HRESULT hr;
    assert(pUnkOuter == NULL);

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		     sizeof(*This) + sizeof(DIB_DirectDrawSurfaceImpl));
    if (This == NULL) return E_OUTOFMEMORY;

    This->private = (DIB_DirectDrawSurfaceImpl*)(This+1);

    hr = DIB_DirectDrawSurface_Construct(This, pDD, pDDSD);
    if (FAILED(hr))
	HeapFree(GetProcessHeap(), 0, This);
    else
	*ppSurf = ICOM_INTERFACE(This, IDirectDrawSurface7);

    return hr;

}

/* AddAttachedSurface: generic */
/* AddOverlayDirtyRect: generic, unimplemented */

static HRESULT _Blt_ColorFill(
    LPBYTE buf, int width, int height, int bpp, LONG lPitch, DWORD color
) {
    int x, y;
    LPBYTE first;

    /* Do first row */

#define COLORFILL_ROW(type) { \
    type *d = (type *) buf; \
    for (x = 0; x < width; x++) \
	d[x] = (type) color; \
    break; \
}

    switch(bpp) {
    case 1: COLORFILL_ROW(BYTE)
    case 2: COLORFILL_ROW(WORD)
    case 3: { BYTE *d = (BYTE *) buf;
              for (x = 0; x < width; x++,d+=3) {
                d[0] = (color    ) & 0xFF;
                d[1] = (color>> 8) & 0xFF;
                d[2] = (color>>16) & 0xFF;
              }
              break;}
    case 4: COLORFILL_ROW(DWORD)
    default:
	FIXME("Color fill not implemented for bpp %d!\n", bpp*8);
	return DDERR_UNSUPPORTED;
    }

#undef COLORFILL_ROW

    /* Now copy first row */
    first = buf;
    for (y = 1; y < height; y++) {
	buf += lPitch;
	memcpy(buf, first, width * bpp);
    }
    return DD_OK;
}

static void ComputeShifts(DWORD mask, DWORD* lshift, DWORD* rshift)
{
    int pos = 0;
    int bits = 0;
    *lshift = 0;
    *rshift = 0;
    
    if (!mask)
	return;
    
    while(!(mask & (1 << pos)))
	pos++; 
    
    while(mask & (1 << (pos+bits)))
	bits++;
    
    *lshift = pos;
    *rshift = 8 - bits;
}

/* This is used to factorize the decompression between the Blt and BltFast code */
static void DoDXTCDecompression(const DDSURFACEDESC2 *sdesc, const DDSURFACEDESC2 *ddesc)
{
    DWORD rs,rb,rm;
    DWORD gs,gb,gm;
    DWORD bs,bb,bm;
    DWORD as,ab,am;

    if (!s3tc_initialized) {
	/* FIXME: We may fake this by rendering the texture into the framebuffer using OpenGL functions and reading back
	 *        the framebuffer. This will be slow and somewhat ugly. */ 
	FIXME("Manual S3TC decompression is not supported in native mode\n");
	return;
    }
    
    rm = ddesc->u4.ddpfPixelFormat.u2.dwRBitMask;
    ComputeShifts(rm, &rs, &rb);
    gm = ddesc->u4.ddpfPixelFormat.u3.dwGBitMask;
    ComputeShifts(gm, &gs, &gb);
    bm = ddesc->u4.ddpfPixelFormat.u4.dwBBitMask;
    ComputeShifts(bm, &bs, &bb);
    am = ddesc->u4.ddpfPixelFormat.u5.dwRGBAlphaBitMask;
    ComputeShifts(am, &as, &ab);
    if (sdesc->u4.ddpfPixelFormat.dwFourCC == MAKE_FOURCC('D','X','T','1')) {
	int is16 = ddesc->u4.ddpfPixelFormat.u1.dwRGBBitCount == 16;
	int pitch = ddesc->u1.lPitch;
	int width = ddesc->dwWidth;
	int height = ddesc->dwHeight;
	int x,y;
	unsigned char* dst = (unsigned char*) ddesc->lpSurface;
	unsigned char* src = (unsigned char*) sdesc->lpSurface;
	for (x = 0; x < width; x++)
	    for (y =0; y < height; y++) {
		DWORD pixel = 0;
		BYTE data[4];
		(*fetch_2d_texel_rgba_dxt1)(width, src, x, y, data);
		pixel = 0;
		pixel |= ((data[0] >> rb) << rs) & rm;
		pixel |= ((data[1] >> gb) << gs) & gm;
		pixel |= ((data[2] >> bb) << bs) & bm;
		pixel |= ((data[3] >> ab) << as) & am;
		if (is16)
		    *((WORD*)(dst+y*pitch+x*(is16?2:4))) = pixel;
		else
		    *((DWORD*)(dst+y*pitch+x*(is16?2:4))) = pixel;
	    }
    } else if (sdesc->u4.ddpfPixelFormat.dwFourCC == MAKE_FOURCC('D','X','T','3')) {
	int is16 = ddesc->u4.ddpfPixelFormat.u1.dwRGBBitCount == 16;
	int pitch = ddesc->u1.lPitch;
	int width = ddesc->dwWidth;
	int height = ddesc->dwHeight;
	int x,y;
	unsigned char* dst = (unsigned char*) ddesc->lpSurface;
	unsigned char* src = (unsigned char*) sdesc->lpSurface;
	for (x = 0; x < width; x++)
	    for (y =0; y < height; y++) {
		DWORD pixel = 0;
		BYTE data[4];
		(*fetch_2d_texel_rgba_dxt3)(width, src, x, y, data);
		pixel = 0;
		pixel |= ((data[0] >> rb) << rs) & rm;
		pixel |= ((data[1] >> gb) << gs) & gm;
		pixel |= ((data[2] >> bb) << bs) & bm;
		pixel |= ((data[3] >> ab) << as) & am;
		if (is16)
		    *((WORD*)(dst+y*pitch+x*(is16?2:4))) = pixel;
		else
		    *((DWORD*)(dst+y*pitch+x*(is16?2:4))) = pixel;
	    }
    } else if (sdesc->u4.ddpfPixelFormat.dwFourCC == MAKE_FOURCC('D','X','T','5')) {
	int is16 = ddesc->u4.ddpfPixelFormat.u1.dwRGBBitCount == 16;
	int pitch = ddesc->u1.lPitch;
	int width = ddesc->dwWidth;
	int height = ddesc->dwHeight;
	int x,y;
	unsigned char* dst = (unsigned char*) ddesc->lpSurface;
	unsigned char* src = (unsigned char*) sdesc->lpSurface;
	for (x = 0; x < width; x++)
	    for (y =0; y < height; y++) {
		DWORD pixel = 0;
		BYTE data[4];
		(*fetch_2d_texel_rgba_dxt5)(width, src, x, y, data);
		pixel = 0;
		pixel |= ((data[0] >> rb) << rs) & rm;
		pixel |= ((data[1] >> gb) << gs) & gm;
		pixel |= ((data[2] >> bb) << bs) & bm;
		pixel |= ((data[3] >> ab) << as) & am;
		if (is16)
		    *((WORD*)(dst+y*pitch+x*(is16?2:4))) = pixel;
		else
		    *((DWORD*)(dst+y*pitch+x*(is16?2:4))) = pixel;
	    }
    }
#if 0 /* Useful for debugging */
    {
	static int idx;
	char texname[255];
	FILE* f;
	sprintf(texname, "dxt_%d.pnm", idx++);
	f = fopen(texname,"w");
	DDRAW_dump_surface_to_disk(This, f, 1);
	fclose(f);
    }
#endif
}

HRESULT WINAPI
DIB_DirectDrawSurface_Blt(LPDIRECTDRAWSURFACE7 iface, LPRECT rdst,
			  LPDIRECTDRAWSURFACE7 src, LPRECT rsrc,
			  DWORD dwFlags, LPDDBLTFX lpbltfx)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    RECT		xdst,xsrc;
    DDSURFACEDESC2	ddesc,sdesc;
    HRESULT		ret = DD_OK;
    int bpp, srcheight, srcwidth, dstheight, dstwidth, width;
    int x, y;
    LPBYTE dbuf, sbuf;
    
    TRACE("(%p)->(%p,%p,%p,%08lx,%p)\n", This,rdst,src,rsrc,dwFlags,lpbltfx);

    if (TRACE_ON(ddraw)) {
	if (rdst) TRACE("\tdestrect :%ldx%ld-%ldx%ld\n",rdst->left,rdst->top,rdst->right,rdst->bottom);
	if (rsrc) TRACE("\tsrcrect  :%ldx%ld-%ldx%ld\n",rsrc->left,rsrc->top,rsrc->right,rsrc->bottom);
	TRACE("\tflags: ");
	DDRAW_dump_DDBLT(dwFlags);
	if (dwFlags & DDBLT_DDFX) {
	    TRACE("\tblitfx: ");
	    DDRAW_dump_DDBLTFX(lpbltfx->dwDDFX);
	}
    }

    if ((This->locked) || ((src != NULL) && (((IDirectDrawSurfaceImpl *)src)->locked))) {
        WARN(" Surface is busy, returning DDERR_SURFACEBUSY\n");
        return DDERR_SURFACEBUSY;
    }

    /* First, check if the possible override function handles this case */
    if (This->aux_blt != NULL) {
        if (This->aux_blt(This, rdst, src, rsrc, dwFlags, lpbltfx) == DD_OK) return DD_OK;
    }

    DD_STRUCT_INIT(&ddesc);
    DD_STRUCT_INIT(&sdesc);

    sdesc.dwSize = sizeof(sdesc);
    ddesc.dwSize = sizeof(ddesc);

    if (src == iface) {
        IDirectDrawSurface7_Lock(iface, NULL, &ddesc, 0, 0);
        DD_STRUCT_COPY_BYSIZE(&sdesc, &ddesc);
    } else {
        if (src) IDirectDrawSurface7_Lock(src, NULL, &sdesc, DDLOCK_READONLY, 0);
        IDirectDrawSurface7_Lock(iface,NULL,&ddesc,DDLOCK_WRITEONLY,0);
    }

    if (!lpbltfx || !(lpbltfx->dwDDFX)) dwFlags &= ~DDBLT_DDFX;

    if ((sdesc.u4.ddpfPixelFormat.dwFlags & DDPF_FOURCC) &&
	(ddesc.u4.ddpfPixelFormat.dwFlags & DDPF_FOURCC)) {
	if (sdesc.u4.ddpfPixelFormat.dwFourCC != sdesc.u4.ddpfPixelFormat.dwFourCC) {
	    FIXME("FOURCC->FOURCC copy only supported for the same type of surface\n");
	    ret = DDERR_INVALIDPIXELFORMAT;
	    goto release;
	}
	memcpy(ddesc.lpSurface, sdesc.lpSurface, ddesc.u1.dwLinearSize);
	goto release;
    }

    if ((sdesc.u4.ddpfPixelFormat.dwFlags & DDPF_FOURCC) &&
	(!(ddesc.u4.ddpfPixelFormat.dwFlags & DDPF_FOURCC))) {
	DoDXTCDecompression(&sdesc, &ddesc);
	goto release;
    }
    
    if (rdst) {
	memcpy(&xdst,rdst,sizeof(xdst));
    } else {
	xdst.top	= 0;
	xdst.bottom	= ddesc.dwHeight;
	xdst.left	= 0;
	xdst.right	= ddesc.dwWidth;
    }

    if (rsrc) {
	memcpy(&xsrc,rsrc,sizeof(xsrc));
    } else {
	if (src) {
	    xsrc.top	= 0;
	    xsrc.bottom	= sdesc.dwHeight;
	    xsrc.left	= 0;
	    xsrc.right	= sdesc.dwWidth;
	} else {
	    memset(&xsrc,0,sizeof(xsrc));
	}
    }

    /* First check for the validity of source / destination rectangles. This was
       verified using a test application + by MSDN.
    */
    if ((src != NULL) &&
	((xsrc.bottom > sdesc.dwHeight) || (xsrc.bottom < 0) ||
	 (xsrc.top > sdesc.dwHeight) || (xsrc.top < 0) ||
	 (xsrc.left > sdesc.dwWidth) || (xsrc.left < 0) ||
	 (xsrc.right > sdesc.dwWidth) || (xsrc.right < 0) ||
	 (xsrc.right < xsrc.left) || (xsrc.bottom < xsrc.top))) {
        WARN("Application gave us bad source rectangle for Blt.\n");
	ret = DDERR_INVALIDRECT;
	goto release;
    }
    /* For the Destination rect, it can be out of bounds on the condition that a clipper
       is set for the given surface.
    */
    if ((This->clipper == NULL) &&
	((xdst.bottom > ddesc.dwHeight) || (xdst.bottom < 0) ||
	 (xdst.top > ddesc.dwHeight) || (xdst.top < 0) ||
	 (xdst.left > ddesc.dwWidth) || (xdst.left < 0) ||
	 (xdst.right > ddesc.dwWidth) || (xdst.right < 0) ||
	 (xdst.right < xdst.left) || (xdst.bottom < xdst.top))) {
        WARN("Application gave us bad destination rectangle for Blt without a clipper set.\n");
	ret = DDERR_INVALIDRECT;
	goto release;
    }
    
    /* Now handle negative values in the rectangles. Warning: only supported for now
       in the 'simple' cases (ie not in any stretching / rotation cases).

       First, the case where nothing is to be done.
    */
    if (((xdst.bottom <= 0) || (xdst.right <= 0) || (xdst.top >= (int) ddesc.dwHeight) || (xdst.left >= (int) ddesc.dwWidth)) ||
        ((src != NULL) &&
         ((xsrc.bottom <= 0) || (xsrc.right <= 0) || (xsrc.top >= (int) sdesc.dwHeight) || (xsrc.left >= (int) sdesc.dwWidth))))
    {
        TRACE("Nothing to be done !\n");
        goto release;
    }

    /* The easy case : the source-less blits.... */
    if (src == NULL) {
        RECT full_rect;
        RECT temp_rect; /* No idea if intersect rect can be the same as one of the source rect */

	full_rect.left   = 0;
	full_rect.top    = 0;
	full_rect.right  = ddesc.dwWidth;
	full_rect.bottom = ddesc.dwHeight;
        IntersectRect(&temp_rect, &full_rect, &xdst);
        xdst = temp_rect;
    } else {
        /* Only handle clipping on the destination rectangle */
        int clip_horiz = (xdst.left < 0) || (xdst.right  > (int) ddesc.dwWidth );
        int clip_vert  = (xdst.top  < 0) || (xdst.bottom > (int) ddesc.dwHeight);
        if (clip_vert || clip_horiz) {
            /* Now check if this is a special case or not... */
            if ((((xdst.bottom - xdst.top ) != (xsrc.bottom - xsrc.top )) && clip_vert ) ||
                (((xdst.right  - xdst.left) != (xsrc.right  - xsrc.left)) && clip_horiz) ||
                (dwFlags & DDBLT_DDFX)) {
                WARN("Out of screen rectangle in special case. Not handled right now.\n");
                goto release;
            }

            if (clip_horiz) {
              if (xdst.left < 0) { xsrc.left -= xdst.left; xdst.left = 0; }
              if (xdst.right > ddesc.dwWidth) { xsrc.right -= (xdst.right - (int) ddesc.dwWidth); xdst.right = (int) ddesc.dwWidth; }
            }
            if (clip_vert) {
                if (xdst.top < 0) { xsrc.top -= xdst.top; xdst.top = 0; }
                if (xdst.bottom > ddesc.dwHeight) { xsrc.bottom -= (xdst.bottom - (int) ddesc.dwHeight); xdst.bottom = (int) ddesc.dwHeight; }
            }
            /* And check if after clipping something is still to be done... */
            if ((xdst.bottom <= 0) || (xdst.right <= 0) || (xdst.top >= (int) ddesc.dwHeight) || (xdst.left >= (int) ddesc.dwWidth) ||
                (xsrc.bottom <= 0) || (xsrc.right <= 0) || (xsrc.top >= (int) sdesc.dwHeight) || (xsrc.left >= (int) sdesc.dwWidth)) {
                TRACE("Nothing to be done after clipping !\n");
                goto release;
            }
        }
    }

    bpp = GET_BPP(ddesc);
    srcheight = xsrc.bottom - xsrc.top;
    srcwidth = xsrc.right - xsrc.left;
    dstheight = xdst.bottom - xdst.top;
    dstwidth = xdst.right - xdst.left;
    width = (xdst.right - xdst.left) * bpp;

    assert(width <= ddesc.u1.lPitch);

    dbuf = (BYTE*)ddesc.lpSurface+(xdst.top*ddesc.u1.lPitch)+(xdst.left*bpp);

    if (dwFlags & DDBLT_WAIT) {
	static BOOL displayed = FALSE;
	if (!displayed)
	    FIXME("Can't handle DDBLT_WAIT flag right now.\n");
	displayed = TRUE;
	dwFlags &= ~DDBLT_WAIT;
    }
    if (dwFlags & DDBLT_ASYNC) {
	static BOOL displayed = FALSE;
	if (!displayed)
	    FIXME("Can't handle DDBLT_ASYNC flag right now.\n");
	displayed = TRUE;
	dwFlags &= ~DDBLT_ASYNC;
    }
    if (dwFlags & DDBLT_DONOTWAIT) {
	/* DDBLT_DONOTWAIT appeared in DX7 */
	static BOOL displayed = FALSE;
	if (!displayed)
	    FIXME("Can't handle DDBLT_DONOTWAIT flag right now.\n");
	displayed = TRUE;
	dwFlags &= ~DDBLT_DONOTWAIT;
    }

    /* First, all the 'source-less' blits */
    if (dwFlags & DDBLT_COLORFILL) {
	ret = _Blt_ColorFill(dbuf, dstwidth, dstheight, bpp,
			     ddesc.u1.lPitch, lpbltfx->u5.dwFillColor);
	dwFlags &= ~DDBLT_COLORFILL;
    }

    if (dwFlags & DDBLT_DEPTHFILL)
	FIXME("DDBLT_DEPTHFILL needs to be implemented!\n");
    if (dwFlags & DDBLT_ROP) {
	/* Catch some degenerate cases here */
	switch(lpbltfx->dwROP) {
	case BLACKNESS:
	    ret = _Blt_ColorFill(dbuf,dstwidth,dstheight,bpp,ddesc.u1.lPitch,0);
	    break;
	case 0xAA0029: /* No-op */
	    break;
	case WHITENESS:
	    ret = _Blt_ColorFill(dbuf,dstwidth,dstheight,bpp,ddesc.u1.lPitch,~0);
	    break;
	case SRCCOPY: /* well, we do that below ? */
	    break;
	default:
	    FIXME("Unsupported raster op: %08lx  Pattern: %p\n", lpbltfx->dwROP, lpbltfx->u5.lpDDSPattern);
	    goto error;
	}
	dwFlags &= ~DDBLT_ROP;
    }
    if (dwFlags & DDBLT_DDROPS) {
	FIXME("\tDdraw Raster Ops: %08lx  Pattern: %p\n", lpbltfx->dwDDROP, lpbltfx->u5.lpDDSPattern);
    }
    /* Now the 'with source' blits */
    if (src) {
	LPBYTE sbase;
	int sx, xinc, sy, yinc;

	if (!dstwidth || !dstheight) /* hmm... stupid program ? */
	    goto release;
	sbase = (BYTE*)sdesc.lpSurface+(xsrc.top*sdesc.u1.lPitch)+xsrc.left*bpp;
	xinc = (srcwidth << 16) / dstwidth;
	yinc = (srcheight << 16) / dstheight;

	if (!dwFlags) {
	    /* No effects, we can cheat here */
	    if (dstwidth == srcwidth) {
		if (dstheight == srcheight) {
		    /* No stretching in either direction. This needs to be as
		     * fast as possible */
		    sbuf = sbase;

                    /* check for overlapping surfaces */
                    if (src != iface || xdst.top < xsrc.top ||
                        xdst.right <= xsrc.left || xsrc.right <= xdst.left)
                    {
                        /* no overlap, or dst above src, so copy from top downwards */
                        for (y = 0; y < dstheight; y++)
                        {
                            memcpy(dbuf, sbuf, width);
                            sbuf += sdesc.u1.lPitch;
                            dbuf += ddesc.u1.lPitch;
                        }
                    }
                    else if (xdst.top > xsrc.top)  /* copy from bottom upwards */
                    {
                        sbuf += (sdesc.u1.lPitch*dstheight);
                        dbuf += (ddesc.u1.lPitch*dstheight);
                        for (y = 0; y < dstheight; y++)
                        {
                            sbuf -= sdesc.u1.lPitch;
                            dbuf -= ddesc.u1.lPitch;
                            memcpy(dbuf, sbuf, width);
                        }
                    }
                    else /* src and dst overlapping on the same line, use memmove */
                    {
                        for (y = 0; y < dstheight; y++)
                        {
                            memmove(dbuf, sbuf, width);
                            sbuf += sdesc.u1.lPitch;
                            dbuf += ddesc.u1.lPitch;
                        }
                    }
		} else {
		    /* Stretching in Y direction only */
		    for (y = sy = 0; y < dstheight; y++, sy += yinc) {
			sbuf = sbase + (sy >> 16) * sdesc.u1.lPitch;
			memcpy(dbuf, sbuf, width);
			dbuf += ddesc.u1.lPitch;
		    }
		}
	    } else {
		/* Stretching in X direction */
		int last_sy = -1;
		for (y = sy = 0; y < dstheight; y++, sy += yinc) {
		    sbuf = sbase + (sy >> 16) * sdesc.u1.lPitch;

		    if ((sy >> 16) == (last_sy >> 16)) {
			/* this sourcerow is the same as last sourcerow -
			 * copy already stretched row
			 */
			memcpy(dbuf, dbuf - ddesc.u1.lPitch, width);
		    } else {
#define STRETCH_ROW(type) { \
		    type *s = (type *) sbuf, *d = (type *) dbuf; \
		    for (x = sx = 0; x < dstwidth; x++, sx += xinc) \
		    d[x] = s[sx >> 16]; \
		    break; }

		    switch(bpp) {
		    case 1: STRETCH_ROW(BYTE)
		    case 2: STRETCH_ROW(WORD)
		    case 4: STRETCH_ROW(DWORD)
		    case 3: {
			LPBYTE s,d = dbuf;
			for (x = sx = 0; x < dstwidth; x++, sx+= xinc) {
			    DWORD pixel;

			    s = sbuf+3*(sx>>16);
			    pixel = s[0]|(s[1]<<8)|(s[2]<<16);
			    d[0] = (pixel    )&0xff;
			    d[1] = (pixel>> 8)&0xff;
			    d[2] = (pixel>>16)&0xff;
			    d+=3;
			}
			break;
		    }
		    default:
			FIXME("Stretched blit not implemented for bpp %d!\n", bpp*8);
			ret = DDERR_UNSUPPORTED;
			goto error;
		    }
#undef STRETCH_ROW
		    }
		    dbuf += ddesc.u1.lPitch;
		    last_sy = sy;
		}
	    }
	} else {
           LONG dstyinc = ddesc.u1.lPitch, dstxinc = bpp;
           DWORD keylow = 0xFFFFFFFF, keyhigh = 0, keymask = 0xFFFFFFFF;
           if (dwFlags & (DDBLT_KEYSRC | DDBLT_KEYDEST | DDBLT_KEYSRCOVERRIDE | DDBLT_KEYDESTOVERRIDE)) {

	      if (dwFlags & DDBLT_KEYSRC) {
		 keylow  = sdesc.ddckCKSrcBlt.dwColorSpaceLowValue;
		 keyhigh = sdesc.ddckCKSrcBlt.dwColorSpaceHighValue;
	      } else if (dwFlags & DDBLT_KEYDEST){
		 keylow  = ddesc.ddckCKDestBlt.dwColorSpaceLowValue;
		 keyhigh = ddesc.ddckCKDestBlt.dwColorSpaceHighValue;
	      } else if (dwFlags & DDBLT_KEYSRCOVERRIDE) {
		 keylow  = lpbltfx->ddckSrcColorkey.dwColorSpaceLowValue;
		 keyhigh = lpbltfx->ddckSrcColorkey.dwColorSpaceHighValue;
	      } else {
		 keylow  = lpbltfx->ddckDestColorkey.dwColorSpaceLowValue;
		 keyhigh = lpbltfx->ddckDestColorkey.dwColorSpaceHighValue;
	      }

		if(bpp == 1)
			keymask = 0xff;
		else
			keymask = sdesc.u4.ddpfPixelFormat.u2.dwRBitMask | sdesc.u4.ddpfPixelFormat.u3.dwGBitMask |
			          sdesc.u4.ddpfPixelFormat.u4.dwBBitMask;

	      dwFlags &= ~(DDBLT_KEYSRC | DDBLT_KEYDEST | DDBLT_KEYSRCOVERRIDE | DDBLT_KEYDESTOVERRIDE);
           }

           if (dwFlags & DDBLT_DDFX)  {
              LPBYTE dTopLeft, dTopRight, dBottomLeft, dBottomRight, tmp;
              LONG tmpxy;
              dTopLeft     = dbuf;
              dTopRight    = dbuf+((dstwidth-1)*bpp);
              dBottomLeft  = dTopLeft+((dstheight-1)*ddesc.u1.lPitch);
              dBottomRight = dBottomLeft+((dstwidth-1)*bpp);

              if (lpbltfx->dwDDFX & DDBLTFX_ARITHSTRETCHY){
                 /* I don't think we need to do anything about this flag */
                 WARN("dwflags=DDBLT_DDFX nothing done for DDBLTFX_ARITHSTRETCHY\n");
              }
              if (lpbltfx->dwDDFX & DDBLTFX_MIRRORLEFTRIGHT) {
                 tmp          = dTopRight;
                 dTopRight    = dTopLeft;
                 dTopLeft     = tmp;
                 tmp          = dBottomRight;
                 dBottomRight = dBottomLeft;
                 dBottomLeft  = tmp;
                 dstxinc = dstxinc *-1;
              }
              if (lpbltfx->dwDDFX & DDBLTFX_MIRRORUPDOWN) {
                 tmp          = dTopLeft;
                 dTopLeft     = dBottomLeft;
                 dBottomLeft  = tmp;
                 tmp          = dTopRight;
                 dTopRight    = dBottomRight;
                 dBottomRight = tmp;
                 dstyinc = dstyinc *-1;
              }
              if (lpbltfx->dwDDFX & DDBLTFX_NOTEARING) {
                 /* I don't think we need to do anything about this flag */
                 WARN("dwflags=DDBLT_DDFX nothing done for DDBLTFX_NOTEARING\n");
              }
              if (lpbltfx->dwDDFX & DDBLTFX_ROTATE180) {
                 tmp          = dBottomRight;
                 dBottomRight = dTopLeft;
                 dTopLeft     = tmp;
                 tmp          = dBottomLeft;
                 dBottomLeft  = dTopRight;
                 dTopRight    = tmp;
                 dstxinc = dstxinc * -1;
                 dstyinc = dstyinc * -1;
              }
              if (lpbltfx->dwDDFX & DDBLTFX_ROTATE270) {
                 tmp          = dTopLeft;
                 dTopLeft     = dBottomLeft;
                 dBottomLeft  = dBottomRight;
                 dBottomRight = dTopRight;
                 dTopRight    = tmp;
                 tmpxy   = dstxinc;
                 dstxinc = dstyinc;
                 dstyinc = tmpxy;
                 dstxinc = dstxinc * -1;
              }
              if (lpbltfx->dwDDFX & DDBLTFX_ROTATE90) {
                 tmp          = dTopLeft;
                 dTopLeft     = dTopRight;
                 dTopRight    = dBottomRight;
                 dBottomRight = dBottomLeft;
                 dBottomLeft  = tmp;
                 tmpxy   = dstxinc;
                 dstxinc = dstyinc;
                 dstyinc = tmpxy;
                 dstyinc = dstyinc * -1;
              }
              if (lpbltfx->dwDDFX & DDBLTFX_ZBUFFERBASEDEST) {
                 /* I don't think we need to do anything about this flag */
                 WARN("dwflags=DDBLT_DDFX nothing done for DDBLTFX_ZBUFFERBASEDEST\n");
              }
              dbuf = dTopLeft;
              dwFlags &= ~(DDBLT_DDFX);
           }

#define COPY_COLORKEY_FX(type) { \
	    type *s, *d = (type *) dbuf, *dx, tmp; \
            for (y = sy = 0; y < dstheight; y++, sy += yinc) { \
               s = (type*)(sbase + (sy >> 16) * sdesc.u1.lPitch); \
               dx = d; \
	       for (x = sx = 0; x < dstwidth; x++, sx += xinc) { \
		  tmp = s[sx >> 16]; \
		  if ((tmp & keymask) < keylow || (tmp & keymask) > keyhigh) dx[0] = tmp; \
                  dx = (type*)(((LPBYTE)dx)+dstxinc); \
	       } \
               d = (type*)(((LPBYTE)d)+dstyinc); \
	    } \
            break; }

	    switch (bpp) {
	    case 1: COPY_COLORKEY_FX(BYTE)
	    case 2: COPY_COLORKEY_FX(WORD)
	    case 4: COPY_COLORKEY_FX(DWORD)
 	    case 3: {LPBYTE s,d = dbuf, dx;
		for (y = sy = 0; y < dstheight; y++, sy += yinc) {
		    sbuf = sbase + (sy >> 16) * sdesc.u1.lPitch;
		    dx = d;
		    for (x = sx = 0; x < dstwidth; x++, sx+= xinc) {
			DWORD pixel;
			s = sbuf+3*(sx>>16);
			pixel = s[0]|(s[1]<<8)|(s[2]<<16);
                        if ((pixel & keymask) < keylow || (pixel & keymask) > keyhigh) {
		            dx[0] = (pixel    )&0xff;
			    dx[1] = (pixel>> 8)&0xff;
			    dx[2] = (pixel>>16)&0xff;
                        }
		        dx+= dstxinc;
		    }
		    d += dstyinc;
                }
                break;}
	    default:
	       FIXME("%s color-keyed blit not implemented for bpp %d!\n",
	          (dwFlags & DDBLT_KEYSRC) ? "Source" : "Destination", bpp*8);
		  ret = DDERR_UNSUPPORTED;
		  goto error;
#undef COPY_COLORKEY_FX
            }
	}
    }

error:
    if (dwFlags && FIXME_ON(ddraw)) {
	FIXME("\tUnsupported flags: ");
	DDRAW_dump_DDBLT(dwFlags);
    }

release:
    IDirectDrawSurface7_Unlock(iface,NULL);
    if (src && src != iface) IDirectDrawSurface7_Unlock(src,NULL);
    return ret;
}

/* BltBatch: generic, unimplemented */

HRESULT WINAPI
DIB_DirectDrawSurface_BltFast(LPDIRECTDRAWSURFACE7 iface, DWORD dstx,
			      DWORD dsty, LPDIRECTDRAWSURFACE7 src,
			      LPRECT rsrc, DWORD trans)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    int			bpp, w, h, x, y;
    DDSURFACEDESC2	ddesc,sdesc;
    HRESULT		ret = DD_OK;
    LPBYTE		sbuf, dbuf;
    RECT		rsrc2;
    RECT                lock_src, lock_dst, lock_union;

    if (TRACE_ON(ddraw)) {
	TRACE("(%p)->(%ld,%ld,%p,%p,%08lx)\n",
		This,dstx,dsty,src,rsrc,trans
	);
	TRACE("\ttrans:");
	if (FIXME_ON(ddraw))
	  DDRAW_dump_DDBLTFAST(trans);
	if (rsrc)
	  TRACE("\tsrcrect: %ldx%ld-%ldx%ld\n",rsrc->left,rsrc->top,rsrc->right,rsrc->bottom);
	else
	  TRACE(" srcrect: NULL\n");
    }

    if ((This->locked) || ((src != NULL) && (((IDirectDrawSurfaceImpl *)src)->locked))) {
        WARN(" Surface is busy, returning DDERR_SURFACEBUSY\n");
        return DDERR_SURFACEBUSY;
    }

    /* First, check if the possible override function handles this case */
    if (This->aux_bltfast != NULL) {
        if (This->aux_bltfast(This, dstx, dsty, src, rsrc, trans) == DD_OK) return DD_OK;
    }

    /* Get the surface description without locking to first compute the width / height */
    ddesc = This->surface_desc;
    sdesc = (ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, src))->surface_desc;

    if (!rsrc) {
	WARN("rsrc is NULL!\n");
	rsrc = &rsrc2;
	rsrc->left = rsrc->top = 0;
	rsrc->right = sdesc.dwWidth;
	rsrc->bottom = sdesc.dwHeight;
    }

    /* Check source rect for validity. Copied from normal Blt. Fixes Baldur's Gate.*/
    if ((rsrc->bottom > sdesc.dwHeight) || (rsrc->bottom < 0) ||
	(rsrc->top > sdesc.dwHeight) || (rsrc->top < 0) ||
	(rsrc->left > sdesc.dwWidth) || (rsrc->left < 0) ||
	(rsrc->right > sdesc.dwWidth) || (rsrc->right < 0) ||
	(rsrc->right < rsrc->left) || (rsrc->bottom < rsrc->top)) {
        WARN("Application gave us bad source rectangle for BltFast.\n");
	return DDERR_INVALIDRECT;
    }
 
    h=rsrc->bottom-rsrc->top;
    if (h>ddesc.dwHeight-dsty) h=ddesc.dwHeight-dsty;
    if (h>sdesc.dwHeight-rsrc->top) h=sdesc.dwHeight-rsrc->top;
    if (h<=0) return DDERR_INVALIDRECT;

    w=rsrc->right-rsrc->left;
    if (w>ddesc.dwWidth-dstx) w=ddesc.dwWidth-dstx;
    if (w>sdesc.dwWidth-rsrc->left) w=sdesc.dwWidth-rsrc->left;
    if (w<=0) return DDERR_INVALIDRECT;

    /* Now compute the locking rectangle... */
    lock_src.left = rsrc->left;
    lock_src.top = rsrc->top;
    lock_src.right = lock_src.left + w;
    lock_src.bottom = lock_src.top + h;

    lock_dst.left = dstx;
    lock_dst.top = dsty;
    lock_dst.right = dstx + w;
    lock_dst.bottom = dsty + h;
    
    bpp = GET_BPP(This->surface_desc);

    /* We need to lock the surfaces, or we won't get refreshes when done. */
    if (src == iface) {
        int pitch;

        UnionRect(&lock_union, &lock_src, &lock_dst);

        /* Lock the union of the two rectangles */
        IDirectDrawSurface7_Lock(iface, &lock_union, &ddesc, 0, 0);

        pitch = This->surface_desc.u1.lPitch;

        /* Since sdesc was originally copied from this surface's description, we can just reuse it */
        sdesc.lpSurface = (BYTE *)This->surface_desc.lpSurface + lock_src.top * pitch + lock_src.left * bpp; 
        ddesc.lpSurface = (BYTE *)This->surface_desc.lpSurface + lock_dst.top * pitch + lock_dst.left * bpp; 
    } else {
        sdesc.dwSize = sizeof(sdesc);
        IDirectDrawSurface7_Lock(src, &lock_src, &sdesc, DDLOCK_READONLY, 0);
        ddesc.dwSize = sizeof(ddesc);
        IDirectDrawSurface7_Lock(iface, &lock_dst, &ddesc, DDLOCK_WRITEONLY, 0);
    }

    /* Handle first the FOURCC surfaces... */
    if ((sdesc.u4.ddpfPixelFormat.dwFlags & DDPF_FOURCC) && (ddesc.u4.ddpfPixelFormat.dwFlags & DDPF_FOURCC)) {
	if (trans)
	    FIXME("trans arg not supported when a FOURCC surface is involved\n");
	if (dstx || dsty)
	    FIXME("offset for destination surface is not supported\n");
	if (sdesc.u4.ddpfPixelFormat.dwFourCC != sdesc.u4.ddpfPixelFormat.dwFourCC) {
	    FIXME("FOURCC->FOURCC copy only supported for the same type of surface\n");
	    ret = DDERR_INVALIDPIXELFORMAT;
	    goto error;
	}
	memcpy(ddesc.lpSurface, sdesc.lpSurface, ddesc.u1.dwLinearSize);
	goto error;
    }
    if ((sdesc.u4.ddpfPixelFormat.dwFlags & DDPF_FOURCC) &&
	(!(ddesc.u4.ddpfPixelFormat.dwFlags & DDPF_FOURCC))) {
	DoDXTCDecompression(&sdesc, &ddesc);
	goto error;
    }
    
    sbuf = (BYTE *) sdesc.lpSurface;
    dbuf = (BYTE *) ddesc.lpSurface;
    
    if (trans & (DDBLTFAST_SRCCOLORKEY | DDBLTFAST_DESTCOLORKEY)) {
	DWORD keylow, keyhigh;
	if (trans & DDBLTFAST_SRCCOLORKEY) {
	    keylow  = sdesc.ddckCKSrcBlt.dwColorSpaceLowValue;
	    keyhigh = sdesc.ddckCKSrcBlt.dwColorSpaceHighValue;
	} else {
	    /* I'm not sure if this is correct */
	    FIXME("DDBLTFAST_DESTCOLORKEY not fully supported yet.\n");
	    keylow  = ddesc.ddckCKDestBlt.dwColorSpaceLowValue;
	    keyhigh = ddesc.ddckCKDestBlt.dwColorSpaceHighValue;
	}

#define COPYBOX_COLORKEY(type) { \
            type *d, *s, tmp; \
            s = (type *) sdesc.lpSurface; \
            d = (type *) ddesc.lpSurface; \
            for (y = 0; y < h; y++) { \
	        for (x = 0; x < w; x++) { \
	            tmp = s[x]; \
	            if (tmp < keylow || tmp > keyhigh) d[x] = tmp; \
	        } \
	        s = (type *)((BYTE *)s + sdesc.u1.lPitch); \
	        d = (type *)((BYTE *)d + ddesc.u1.lPitch); \
            } \
            break; \
        }

        switch (bpp) {
	    case 1: COPYBOX_COLORKEY(BYTE)
	    case 2: COPYBOX_COLORKEY(WORD)
	    case 4: COPYBOX_COLORKEY(DWORD)
	    case 3:
            {
                BYTE *d, *s;
                DWORD tmp;
                s = (BYTE *) sdesc.lpSurface;
                d = (BYTE *) ddesc.lpSurface;
                for (y = 0; y < h; y++) {
                    for (x = 0; x < w * 3; x += 3) {
                        tmp = (DWORD)s[x] + ((DWORD)s[x + 1] << 8) + ((DWORD)s[x + 2] << 16);
                        if (tmp < keylow || tmp > keyhigh) {
                            d[x + 0] = s[x + 0];
                            d[x + 1] = s[x + 1];
                            d[x + 2] = s[x + 2];
                        }
                    }
                    s += sdesc.u1.lPitch;
                    d += ddesc.u1.lPitch;
                }
                break;
            }
	    default:
		FIXME("Source color key blitting not supported for bpp %d\n",bpp*8);
	        ret = DDERR_UNSUPPORTED;
	        goto error;
	}
#undef COPYBOX_COLORKEY
    } else {
	int width = w * bpp;

	for (y = 0; y < h; y++) {
	    memcpy(dbuf, sbuf, width);
	    sbuf += sdesc.u1.lPitch;
	    dbuf += ddesc.u1.lPitch;
	}
    }
    
error:
    if (src == iface) {
        IDirectDrawSurface7_Unlock(iface, &lock_union);
    } else {
        IDirectDrawSurface7_Unlock(iface, &lock_dst);
        IDirectDrawSurface7_Unlock(src, &lock_src);
    }

    return ret;
}

/* ChangeUniquenessValue: generic */
/* DeleteAttachedSurface: generic */
/* EnumAttachedSurfaces: generic */
/* EnumOverlayZOrders: generic, unimplemented */

BOOL DIB_DirectDrawSurface_flip_data(IDirectDrawSurfaceImpl* front,
				     IDirectDrawSurfaceImpl* back,
				     DWORD dwFlags)
{
    DIB_DirectDrawSurfaceImpl* front_priv = front->private;
    DIB_DirectDrawSurfaceImpl* back_priv = back->private;

    TRACE("(%p,%p)\n",front,back);

    {
	HBITMAP tmp;
	tmp = front_priv->dib.DIBsection;
	front_priv->dib.DIBsection = back_priv->dib.DIBsection;
	back_priv->dib.DIBsection = tmp;
    }

    {
	void* tmp;
	tmp = front_priv->dib.bitmap_data;
	front_priv->dib.bitmap_data = back_priv->dib.bitmap_data;
	back_priv->dib.bitmap_data = tmp;

	tmp = front->surface_desc.lpSurface;
	front->surface_desc.lpSurface = back->surface_desc.lpSurface;
	back->surface_desc.lpSurface = tmp;
    }

    /* client_memory should not be different, but just in case */
    {
	BOOL tmp;
	tmp = front_priv->dib.client_memory;
	front_priv->dib.client_memory = back_priv->dib.client_memory;
	back_priv->dib.client_memory = tmp;
    }

    return Main_DirectDrawSurface_flip_data(front, back, dwFlags);
}

/* Flip: generic */
/* FreePrivateData: generic */
/* GetAttachedSurface: generic */
/* GetBltStatus: generic */
/* GetCaps: generic (Returns the caps from This->surface_desc.) */
/* GetClipper: generic */
/* GetColorKey: generic */

HRESULT DIB_DirectDrawSurface_alloc_dc(IDirectDrawSurfaceImpl* This, HDC* phDC)
{
    DIB_PRIV_VAR(priv, This);
    HDC hDC;

    TRACE("Grabbing a DC for surface: %p\n", This);

    hDC = CreateCompatibleDC(0);
    priv->dib.holdbitmap = SelectObject(hDC, priv->dib.DIBsection);
    if (This->palette)
	SelectPalette(hDC, This->palette->hpal, FALSE);

    *phDC = hDC;

    return S_OK;
}

HRESULT DIB_DirectDrawSurface_free_dc(IDirectDrawSurfaceImpl* This, HDC hDC)
{
    DIB_PRIV_VAR(priv, This);

    TRACE("Releasing DC for surface: %p\n", This);

    SelectObject(hDC, priv->dib.holdbitmap);
    DeleteDC(hDC);

    return S_OK;
}

HRESULT DIB_DirectDrawSurface_get_dc(IDirectDrawSurfaceImpl* This, HDC* phDC)
{
    return DIB_DirectDrawSurface_alloc_dc(This, phDC);
}

HRESULT DIB_DirectDrawSurface_release_dc(IDirectDrawSurfaceImpl* This, HDC hDC)
{
    return DIB_DirectDrawSurface_free_dc(This, hDC);
}

/* GetDDInterface: generic */
/* GetFlipStatus: generic */
/* GetLOD: generic */
/* GetOverlayPosition: generic */
/* GetPalette: generic */
/* GetPixelFormat: generic */
/* GetPriority: generic */
/* GetPrivateData: generic */
/* GetSurfaceDesc: generic */
/* GetUniquenessValue: generic */
/* Initialize: generic */
/* IsLost: generic */
/* Lock: generic with callback? */
/* PageLock: generic */
/* PageUnlock: generic */

HRESULT WINAPI
DIB_DirectDrawSurface_Restore(LPDIRECTDRAWSURFACE7 iface)
{
    TRACE("(%p)\n",iface);
    return DD_OK;	/* ??? */
}

/* SetClipper: generic */
/* SetColorKey: generic */
/* SetLOD: generic */
/* SetOverlayPosition: generic */

void DIB_DirectDrawSurface_set_palette(IDirectDrawSurfaceImpl* This,
				       IDirectDrawPaletteImpl* pal)
{
    if (!pal) return;
    if (This->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	This->update_palette(This, pal,
			     0, pal->palNumEntries,
			     pal->palents);
}

void DIB_DirectDrawSurface_update_palette(IDirectDrawSurfaceImpl* This,
					  IDirectDrawPaletteImpl* pal,
					  DWORD dwStart, DWORD dwCount,
					  LPPALETTEENTRY palent)
{
    RGBQUAD col[256];
    unsigned int n;
    HDC dc;

    TRACE("updating primary palette\n");
    for (n=0; n<dwCount; n++) {
      col[n].rgbRed   = palent[n].peRed;
      col[n].rgbGreen = palent[n].peGreen;
      col[n].rgbBlue  = palent[n].peBlue;
      col[n].rgbReserved = 0;
    }
    This->get_dc(This, &dc);
    SetDIBColorTable(dc, dwStart, dwCount, col);
    This->release_dc(This, dc);

    /* Propagate change to backbuffers if there are any */
    /* Basically this is a modification of the Flip code to find the backbuffer */
    /* and duplicate the palette update there as well */
    if ((This->surface_desc.ddsCaps.dwCaps&(DDSCAPS_FLIP|DDSCAPS_FRONTBUFFER))
	== (DDSCAPS_FLIP|DDSCAPS_FRONTBUFFER))
    {
	static DDSCAPS2 back_caps = { DDSCAPS_BACKBUFFER };
	LPDIRECTDRAWSURFACE7 tgt;

	HRESULT hr = IDirectDrawSurface7_GetAttachedSurface(ICOM_INTERFACE(This,IDirectDrawSurface7),
							    &back_caps, &tgt);
	if (!FAILED(hr))
	{
	    IDirectDrawSurfaceImpl* target = ICOM_OBJECT(IDirectDrawSurfaceImpl,
							 IDirectDrawSurface7,tgt);
	    IDirectDrawSurface7_Release(tgt);
	    target->get_dc(target, &dc);
	    SetDIBColorTable(dc, dwStart, dwCount, col);
	    target->release_dc(target, dc);
	}
    }
}

/* SetPalette: generic */
/* SetPriority: generic */
/* SetPrivateData: generic */

HRESULT WINAPI
DIB_DirectDrawSurface_SetSurfaceDesc(LPDIRECTDRAWSURFACE7 iface,
				     LPDDSURFACEDESC2 pDDSD, DWORD dwFlags)
{
    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;
    DIB_PRIV_VAR(priv, This);
    HRESULT hr = DD_OK;
    DWORD flags = pDDSD->dwFlags;

    if (TRACE_ON(ddraw)) {
        TRACE("(%p)->(%p,%08lx)\n",iface,pDDSD,dwFlags);
        DDRAW_dump_surface_desc(pDDSD);
    }

    if (pDDSD->dwFlags & DDSD_PIXELFORMAT) {
        flags &= ~DDSD_PIXELFORMAT;
	if (flags & DDSD_LPSURFACE) {
	    This->surface_desc.u4.ddpfPixelFormat = pDDSD->u4.ddpfPixelFormat;
	} else {
	    FIXME("Change of pixel format without surface re-allocation is not supported !\n");
	}
    }
    if (pDDSD->dwFlags & DDSD_LPSURFACE) {
	HBITMAP oldbmp = priv->dib.DIBsection;
	LPVOID oldsurf = This->surface_desc.lpSurface;
	BOOL oldc = priv->dib.client_memory;

	flags &= ~DDSD_LPSURFACE;

	TRACE("new lpSurface=%p\n",pDDSD->lpSurface);
	This->surface_desc.lpSurface = pDDSD->lpSurface;
	priv->dib.client_memory = TRUE;

	hr = create_dib(This);
	if (FAILED(hr))
	{
	    priv->dib.DIBsection = oldbmp;
	    This->surface_desc.lpSurface = oldsurf;
	    priv->dib.client_memory = oldc;
	    return hr;
	}

	DeleteObject(oldbmp);

	if (!oldc)
	    VirtualFree(oldsurf, 0, MEM_RELEASE);
    }
    if (flags) {
        WARN("Unhandled flags : %08lx\n", flags);
    }
    return hr;
}

/* Unlock: ???, need callback */
/* UpdateOverlay: generic */
/* UpdateOverlayDisplay: generic */
/* UpdateOverlayZOrder: generic */

static const IDirectDrawSurface7Vtbl DIB_IDirectDrawSurface7_VTable =
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
