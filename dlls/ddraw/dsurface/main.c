/*		DirectDrawSurface base implementation
 *
 * Copyright 1997-2000 Marcus Meissner
 * Copyright 1998-2000 Lionel Ulmer (most of Direct3D stuff)
 */
#include "config.h"
#include "winerror.h"


#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "debugtools.h"
#include "bitmap.h"
#include "ddraw_private.h"

DEFAULT_DEBUG_CHANNEL(ddraw);

/******************************************************************************
 *		IDirectDrawSurface methods
 *
 * Since DDS3 and DDS2 are supersets of DDS, we implement DDS3 and let
 * DDS and DDS2 use those functions. (Function calls did not change (except
 * using different DirectDrawSurfaceX version), just added flags and functions)
 */

HRESULT WINAPI IDirectDrawSurface4Impl_Lock(
    LPDIRECTDRAWSURFACE4 iface,LPRECT lprect,LPDDSURFACEDESC lpddsd,DWORD flags, HANDLE hnd
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);

    TRACE("(%p)->Lock(%p,%p,%08lx,%08lx)\n",
	This,lprect,lpddsd,flags,(DWORD)hnd);

    /* DO NOT AddRef the surface! Lock/Unlock must not come in matched pairs
     * -Marcus Meissner 20000509
     */
    if (flags & ~(DDLOCK_WAIT|DDLOCK_READONLY|DDLOCK_WRITEONLY))
	WARN("(%p)->Lock(%p,%p,%08lx,%08lx)\n",
		 This,lprect,lpddsd,flags,(DWORD)hnd);

    /* First, copy the Surface description */
    *lpddsd = This->s.surface_desc;
    TRACE("locked surface: height=%ld, width=%ld, pitch=%ld\n",
	  lpddsd->dwHeight,lpddsd->dwWidth,lpddsd->lPitch);

    /* If asked only for a part, change the surface pointer */
    if (lprect) {
	TRACE("	lprect: %dx%d-%dx%d\n",
		lprect->top,lprect->left,lprect->bottom,lprect->right
	);
	if ((lprect->top < 0) ||
	    (lprect->left < 0) ||
	    (lprect->bottom < 0) ||
	    (lprect->right < 0)) {
	  ERR(" Negative values in LPRECT !!!\n");
	  return DDERR_INVALIDPARAMS;
       }
       
	lpddsd->u1.lpSurface = (LPVOID) ((char *) This->s.surface_desc.u1.lpSurface +
		(lprect->top*This->s.surface_desc.lPitch) +
		lprect->left*GET_BPP(This->s.surface_desc));
    } else {
	assert(This->s.surface_desc.u1.lpSurface);
    }
    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_Unlock(
    LPDIRECTDRAWSURFACE4 iface,LPVOID surface
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);

    /* DO NOT Release the surface! Lock/Unlock MUST NOT come in matched pairs
     * Marcus Meissner 20000509
     */
    TRACE("(%p)->Unlock(%p)\n",This,surface);
    return DD_OK;
}

IDirectDrawSurface4Impl* _common_find_flipto(
    IDirectDrawSurface4Impl* This,IDirectDrawSurface4Impl* flipto
) {
    int	i,j,flipable=0;
    struct _surface_chain	*chain = This->s.chain;

    /* if there was no override flipto, look for current backbuffer */
    if (!flipto) {
	/* walk the flip chain looking for backbuffer */
	for (i=0;i<chain->nrofsurfaces;i++) {
	    if (SDDSCAPS(chain->surfaces[i]) & DDSCAPS_FLIP)
	    	flipable++;
	    if (SDDSCAPS(chain->surfaces[i]) & DDSCAPS_BACKBUFFER)
		flipto = chain->surfaces[i];
	}
	/* sanity checks ... */
	if (!flipto) {
	    if (flipable>1) {
		for (i=0;i<chain->nrofsurfaces;i++)
		    if (SDDSCAPS(chain->surfaces[i]) & DDSCAPS_FRONTBUFFER)
		    	break;
		if (i==chain->nrofsurfaces) {
		    /* we do not have a frontbuffer either */
		    for (i=0;i<chain->nrofsurfaces;i++)
			if (SDDSCAPS(chain->surfaces[i]) & DDSCAPS_FLIP) {
			    SDDSCAPS(chain->surfaces[i])|=DDSCAPS_FRONTBUFFER;
			    break;
			}
		    for (j=i+1;j<i+chain->nrofsurfaces+1;j++) {
		    	int k = j % chain->nrofsurfaces;
			if (SDDSCAPS(chain->surfaces[k]) & DDSCAPS_FLIP) {
			    SDDSCAPS(chain->surfaces[k])|=DDSCAPS_BACKBUFFER;
			    flipto = chain->surfaces[k];
			    break;
			}
		    }
		}
	    }
	    if (!flipto)
		flipto = This;
	}
	TRACE("flipping to %p\n",flipto);
    }
    return flipto;
}

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

HRESULT WINAPI IDirectDrawSurface4Impl_Blt(
    LPDIRECTDRAWSURFACE4 iface,LPRECT rdst,LPDIRECTDRAWSURFACE4 src,LPRECT rsrc,
    DWORD dwFlags,LPDDBLTFX lpbltfx
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    RECT		xdst,xsrc;
    DDSURFACEDESC	ddesc,sdesc;
    HRESULT		ret = DD_OK;
    int bpp, srcheight, srcwidth, dstheight, dstwidth, width;
    int x, y;
    LPBYTE dbuf, sbuf;

    TRACE("(%p)->(%p,%p,%p,%08lx,%p)\n", This,rdst,src,rsrc,dwFlags,lpbltfx);

    if (src) IDirectDrawSurface4_Lock(src, NULL, &sdesc, 0, 0);
    IDirectDrawSurface4_Lock(iface,NULL,&ddesc,0,0);

    if (TRACE_ON(ddraw)) {
	if (rdst) TRACE("\tdestrect :%dx%d-%dx%d\n",rdst->left,rdst->top,rdst->right,rdst->bottom);
	if (rsrc) TRACE("\tsrcrect  :%dx%d-%dx%d\n",rsrc->left,rsrc->top,rsrc->right,rsrc->bottom);
	TRACE("\tflags: ");
	_dump_DDBLT(dwFlags);
	if (dwFlags & DDBLT_DDFX) {
	    TRACE("\tblitfx: ");
	    _dump_DDBLTFX(lpbltfx->dwDDFX);
	}
    }

    if (rdst) {
	if ((rdst->top < 0) ||
	    (rdst->left < 0) ||
	    (rdst->bottom < 0) ||
	    (rdst->right < 0)) {
	  ERR(" Negative values in LPRECT !!!\n");
	  goto release;
	}
	memcpy(&xdst,rdst,sizeof(xdst));
    } else {
	xdst.top	= 0;
	xdst.bottom	= ddesc.dwHeight;
	xdst.left	= 0;
	xdst.right	= ddesc.dwWidth;
    }

    if (rsrc) {
	if ((rsrc->top < 0) ||
	    (rsrc->left < 0) ||
	    (rsrc->bottom < 0) ||
	    (rsrc->right < 0)) {
	  ERR(" Negative values in LPRECT !!!\n");
	  goto release;
	}
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
    if (src) assert(xsrc.bottom <= sdesc.dwHeight);
    assert(xdst.bottom <= ddesc.dwHeight);

    bpp = GET_BPP(ddesc);
    srcheight = xsrc.bottom - xsrc.top;
    srcwidth = xsrc.right - xsrc.left;
    dstheight = xdst.bottom - xdst.top;
    dstwidth = xdst.right - xdst.left;
    width = (xdst.right - xdst.left) * bpp;

    assert(width <= ddesc.lPitch);

    dbuf = (BYTE*)ddesc.u1.lpSurface+(xdst.top*ddesc.lPitch)+(xdst.left*bpp);

    dwFlags &= ~(DDBLT_WAIT|DDBLT_ASYNC);/* FIXME: can't handle right now */

    /* First, all the 'source-less' blits */
    if (dwFlags & DDBLT_COLORFILL) {
	ret = _Blt_ColorFill(dbuf, dstwidth, dstheight, bpp,
			     ddesc.lPitch, lpbltfx->u4.dwFillColor);
	dwFlags &= ~DDBLT_COLORFILL;
    }

    if (dwFlags & DDBLT_DEPTHFILL)
	FIXME("DDBLT_DEPTHFILL needs to be implemented!\n");
    if (dwFlags & DDBLT_ROP) {
	/* Catch some degenerate cases here */
	switch(lpbltfx->dwROP) {
	case BLACKNESS:
	    ret = _Blt_ColorFill(dbuf,dstwidth,dstheight,bpp,ddesc.lPitch,0);
	    break;
	case 0xAA0029: /* No-op */
	    break;
	case WHITENESS:
	    ret = _Blt_ColorFill(dbuf,dstwidth,dstheight,bpp,ddesc.lPitch,~0);
	    break;
	default: 
	    FIXME("Unsupported raster op: %08lx  Pattern: %p\n", lpbltfx->dwROP, lpbltfx->u4.lpDDSPattern);
	    goto error;
	}
	dwFlags &= ~DDBLT_ROP;
    }
    if (dwFlags & DDBLT_DDROPS) {
	FIXME("\tDdraw Raster Ops: %08lx  Pattern: %p\n", lpbltfx->dwDDROP, lpbltfx->u4.lpDDSPattern);
    }
    /* Now the 'with source' blits */
    if (src) {
	LPBYTE sbase;
	int sx, xinc, sy, yinc;

	sbase = (BYTE*)sdesc.u1.lpSurface+(xsrc.top*sdesc.lPitch)+xsrc.left*bpp;
	xinc = (srcwidth << 16) / dstwidth;
	yinc = (srcheight << 16) / dstheight;

	if (!dwFlags) {
	    assert(ddesc.lPitch >= width);
	    assert(sdesc.lPitch >= width);

	    /* No effects, we can cheat here */
	    if (dstwidth == srcwidth) {
		if (dstheight == srcheight) {
		    /* No stretching in either direction. This needs to be as
		     * fast as possible */
		    sbuf = sbase;
		    for (y = 0; y < dstheight; y++) {
			memcpy(dbuf, sbuf, width);
			sbuf += sdesc.lPitch;
			dbuf += ddesc.lPitch;
		    }
		} else {
		    /* Stretching in Y direction only */
		    for (y = sy = 0; y < dstheight; y++, sy += yinc) {
			sbuf = sbase + (sy >> 16) * sdesc.lPitch;
			memcpy(dbuf, sbuf, width);
			dbuf += ddesc.lPitch;
		    }
		}
	    } else {
		/* Stretching in X direction */
		int last_sy = -1;
		for (y = sy = 0; y < dstheight; y++, sy += yinc) {
		    sbuf = sbase + (sy >> 16) * sdesc.lPitch;

		    assert((sy>>16) < srcheight);

		    if ((sy >> 16) == (last_sy >> 16)) {
			/* this sourcerow is the same as last sourcerow -
			 * copy already stretched row
			 */
			memcpy(dbuf, dbuf - ddesc.lPitch, width);
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
			    pixel = (s[0]<<16)|(s[1]<<8)|s[2];
			    d[0] = (pixel>>16)&0xff;
			    d[1] = (pixel>> 8)&0xff;
			    d[2] = (pixel    )&0xff;
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
		    dbuf += ddesc.lPitch;
		    last_sy = sy;
		}
	    }
	} else if (dwFlags & (DDBLT_KEYSRC | DDBLT_KEYDEST)) {
	    DWORD keylow, keyhigh;

	    if (dwFlags & DDBLT_KEYSRC) {
		keylow  = sdesc.ddckCKSrcBlt.dwColorSpaceLowValue;
		keyhigh = sdesc.ddckCKSrcBlt.dwColorSpaceHighValue;
	    } else {
		/* I'm not sure if this is correct */
		FIXME("DDBLT_KEYDEST not fully supported yet.\n");
		keylow  = ddesc.ddckCKDestBlt.dwColorSpaceLowValue;
		keyhigh = ddesc.ddckCKDestBlt.dwColorSpaceHighValue;
	    }


	    for (y = sy = 0; y < dstheight; y++, sy += yinc) {
		sbuf = sbase + (sy >> 16) * sdesc.lPitch;

#define COPYROW_COLORKEY(type) { \
		type *s = (type *) sbuf, *d = (type *) dbuf, tmp; \
		for (x = sx = 0; x < dstwidth; x++, sx += xinc) { \
		    tmp = s[sx >> 16]; \
		    if (tmp < keylow || tmp > keyhigh) d[x] = tmp; \
		} \
		break; }

		switch (bpp) {
		case 1: COPYROW_COLORKEY(BYTE)
		case 2: COPYROW_COLORKEY(WORD)
		case 4: COPYROW_COLORKEY(DWORD)
		default:
		    FIXME("%s color-keyed blit not implemented for bpp %d!\n",
		    (dwFlags & DDBLT_KEYSRC) ? "Source" : "Destination", bpp*8);
		    ret = DDERR_UNSUPPORTED;
		    goto error;
		}
		dbuf += ddesc.lPitch;
	    }
#undef COPYROW_COLORKEY
	    dwFlags &= ~(DDBLT_KEYSRC | DDBLT_KEYDEST);
	}
    }

error:
    if (dwFlags && FIXME_ON(ddraw)) {
	FIXME("\tUnsupported flags: ");
	_dump_DDBLT(dwFlags);
    }

release:
    IDirectDrawSurface4_Unlock(iface,ddesc.u1.lpSurface);
    if (src) IDirectDrawSurface4_Unlock(src,sdesc.u1.lpSurface);
    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_BltFast(
    LPDIRECTDRAWSURFACE4 iface,DWORD dstx,DWORD dsty,LPDIRECTDRAWSURFACE4 src,
    LPRECT rsrc,DWORD trans
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    int			bpp, w, h, x, y;
    DDSURFACEDESC	ddesc,sdesc;
    HRESULT		ret = DD_OK;
    LPBYTE		sbuf, dbuf;
    RECT		rsrc2;


    if (TRACE_ON(ddraw)) {
	FIXME("(%p)->(%ld,%ld,%p,%p,%08lx)\n",
		This,dstx,dsty,src,rsrc,trans
	);
	FIXME("	trans:");
	if (FIXME_ON(ddraw))
	  _dump_DDBLTFAST(trans);
	if (rsrc)
	  FIXME("\tsrcrect: %dx%d-%dx%d\n",rsrc->left,rsrc->top,rsrc->right,rsrc->bottom);
	else
	  FIXME(" srcrect: NULL\n");
    }
    
    /* We need to lock the surfaces, or we won't get refreshes when done. */
    IDirectDrawSurface4_Lock(src, NULL,&sdesc,DDLOCK_READONLY, 0);
    IDirectDrawSurface4_Lock(iface,NULL,&ddesc,DDLOCK_WRITEONLY,0);

   if (!rsrc) {
	   WARN("rsrc is NULL!\n");
	   rsrc = &rsrc2;
	   rsrc->left = rsrc->top = 0;
	   rsrc->right = sdesc.dwWidth;
	   rsrc->bottom = sdesc.dwHeight;
   }

    bpp = GET_BPP(This->s.surface_desc);
    sbuf = (BYTE *)sdesc.u1.lpSurface+(rsrc->top*sdesc.lPitch)+rsrc->left*bpp;
    dbuf = (BYTE *)ddesc.u1.lpSurface+(dsty*ddesc.lPitch)+dstx* bpp;


    h=rsrc->bottom-rsrc->top;
    if (h>ddesc.dwHeight-dsty) h=ddesc.dwHeight-dsty;
    if (h>sdesc.dwHeight-rsrc->top) h=sdesc.dwHeight-rsrc->top;
    if (h<0) h=0;

    w=rsrc->right-rsrc->left;
    if (w>ddesc.dwWidth-dstx) w=ddesc.dwWidth-dstx;
    if (w>sdesc.dwWidth-rsrc->left) w=sdesc.dwWidth-rsrc->left;
    if (w<0) w=0;

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
    type *d = (type *)dbuf, *s = (type *)sbuf, tmp; \
    s = (type *) ((BYTE *) sdesc.u1.lpSurface + (rsrc->top * sdesc.lPitch) + rsrc->left * bpp); \
    d = (type *) ((BYTE *) ddesc.u1.lpSurface + (dsty * ddesc.lPitch) + dstx * bpp); \
    for (y = 0; y < h; y++) { \
	for (x = 0; x < w; x++) { \
	    tmp = s[x]; \
	    if (tmp < keylow || tmp > keyhigh) d[x] = tmp; \
	} \
	(LPBYTE)s += sdesc.lPitch; \
	(LPBYTE)d += ddesc.lPitch; \
    } \
    break; \
}

	switch (bpp) {
	case 1: COPYBOX_COLORKEY(BYTE)
	case 2: COPYBOX_COLORKEY(WORD)
	case 4: COPYBOX_COLORKEY(DWORD)
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
	    sbuf += sdesc.lPitch;
	    dbuf += ddesc.lPitch;
	}
    }
error:
    IDirectDrawSurface4_Unlock(iface,ddesc.u1.lpSurface);
    IDirectDrawSurface4_Unlock(src,sdesc.u1.lpSurface);
    return ret;
}

HRESULT WINAPI IDirectDrawSurface4Impl_BltBatch(
    LPDIRECTDRAWSURFACE4 iface,LPDDBLTBATCH ddbltbatch,DWORD x,DWORD y
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    FIXME("(%p)->BltBatch(%p,%08lx,%08lx),stub!\n",This,ddbltbatch,x,y);
    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_GetCaps(
    LPDIRECTDRAWSURFACE4 iface,LPDDSCAPS caps
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    TRACE("(%p)->GetCaps(%p)\n",This,caps);
    caps->dwCaps = DDSCAPS_PALETTE; /* probably more */
    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_GetSurfaceDesc(
    LPDIRECTDRAWSURFACE4 iface,LPDDSURFACEDESC ddsd
) { 
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    TRACE("(%p)->GetSurfaceDesc(%p)\n", This,ddsd);
  
    /* Simply copy the surface description stored in the object */
    *ddsd = This->s.surface_desc;
  
    if (TRACE_ON(ddraw)) { _dump_surface_desc(ddsd); }

    return DD_OK;
}

ULONG WINAPI IDirectDrawSurface4Impl_AddRef(LPDIRECTDRAWSURFACE4 iface) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    TRACE("(%p)->() incrementing from %lu.\n", This, This->ref );
    return ++(This->ref);
}


HRESULT WINAPI IDirectDrawSurface4Impl_GetAttachedSurface(
    LPDIRECTDRAWSURFACE4 iface,LPDDSCAPS lpddsd,LPDIRECTDRAWSURFACE4 *lpdsf
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    int	i,found = 0,xstart;
    struct _surface_chain	*chain;

    TRACE("(%p)->GetAttachedSurface(%p,%p)\n", This, lpddsd, lpdsf);
    if (TRACE_ON(ddraw)) {
	TRACE("	caps ");_dump_DDSCAPS((void *) &(lpddsd->dwCaps));
	DPRINTF("\n");
    }
    chain = This->s.chain;
    if (!chain)
    	return DDERR_NOTFOUND;

    for (i=0;i<chain->nrofsurfaces;i++)
    	if (chain->surfaces[i] == This)
	    break;

    xstart = i;
    for (i=0;i<chain->nrofsurfaces;i++) {
	if ((SDDSCAPS(chain->surfaces[(xstart+i)%chain->nrofsurfaces])&lpddsd->dwCaps) == lpddsd->dwCaps) {
#if 0
	    if (found) /* may not find the same caps twice, (doc) */
		return DDERR_INVALIDPARAMS;/*FIXME: correct? */
#endif
	    found = (i+1)+xstart;
	}
    }
    if (!found)
	return DDERR_NOTFOUND;
    *lpdsf = (LPDIRECTDRAWSURFACE4)chain->surfaces[found-1-xstart];
    /* FIXME: AddRef? */
    TRACE("found %p\n",*lpdsf);
    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_Initialize(
    LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAW ddraw,LPDDSURFACEDESC lpdsfd
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    TRACE("(%p)->(%p, %p)\n",This,ddraw,lpdsfd);

    return DDERR_ALREADYINITIALIZED;
}

HRESULT WINAPI IDirectDrawSurface4Impl_GetPixelFormat(
    LPDIRECTDRAWSURFACE4 iface,LPDDPIXELFORMAT pf
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    TRACE("(%p)->(%p)\n",This,pf);

    *pf = This->s.surface_desc.ddpfPixelFormat;
    if (TRACE_ON(ddraw)) { _dump_pixelformat(pf); DPRINTF("\n"); }
    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_GetBltStatus(LPDIRECTDRAWSURFACE4 iface,DWORD dwFlags) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    FIXME("(%p)->(0x%08lx),stub!\n",This,dwFlags);
    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_GetOverlayPosition(
    LPDIRECTDRAWSURFACE4 iface,LPLONG x1,LPLONG x2
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    FIXME("(%p)->(%p,%p),stub!\n",This,x1,x2);
    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_SetClipper(
    LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAWCLIPPER lpClipper
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    TRACE("(%p)->(%p)!\n",This,lpClipper);

    if (This->s.lpClipper) IDirectDrawClipper_Release( This->s.lpClipper );
    This->s.lpClipper = lpClipper;
    if (lpClipper) IDirectDrawClipper_AddRef( lpClipper );
    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_AddAttachedSurface(
    LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAWSURFACE4 surf
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    IDirectDrawSurface4Impl*isurf = (IDirectDrawSurface4Impl*)surf;
    int i;
    struct _surface_chain *chain;

    FIXME("(%p)->(%p)\n",This,surf);
    chain = This->s.chain;

    /* IDirectDrawSurface4_AddRef(surf); */
    
    if (chain) {
	for (i=0;i<chain->nrofsurfaces;i++)
	    if (chain->surfaces[i] == isurf)
		FIXME("attaching already attached surface %p to %p!\n",iface,isurf);
    } else {
    	chain = HeapAlloc(GetProcessHeap(),0,sizeof(*chain));
	chain->nrofsurfaces = 1;
	chain->surfaces = HeapAlloc(GetProcessHeap(),0,sizeof(chain->surfaces[0]));
	chain->surfaces[0] = This;
	This->s.chain = chain;
    }

    if (chain->surfaces)
	chain->surfaces = HeapReAlloc(
	    GetProcessHeap(),
	    0,
	    chain->surfaces,
	    sizeof(chain->surfaces[0])*(chain->nrofsurfaces+1)
	);
    else
	chain->surfaces = HeapAlloc(GetProcessHeap(),0,sizeof(chain->surfaces[0]));
    isurf->s.chain = chain;
    chain->surfaces[chain->nrofsurfaces++] = isurf;
    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_GetDC(LPDIRECTDRAWSURFACE4 iface,HDC* lphdc) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    DDSURFACEDESC desc;
    BITMAPINFO *b_info;
    UINT usage;
    HDC ddc;

    TRACE("(%p)->GetDC(%p)\n",This,lphdc);

    /* Creates a DIB Section of the same size / format as the surface */
    IDirectDrawSurface4_Lock(iface,NULL,&desc,0,0);

    if (This->s.hdc == 0) {
	switch (desc.ddpfPixelFormat.u.dwRGBBitCount) {
	case 16:
	case 32:
#if 0 /* This should be filled if Wine's DIBSection did understand BI_BITFIELDS */
	    b_info = (BITMAPINFO *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER) + 3 * sizeof(DWORD));
	    break;
#endif

	case 24:
	    b_info = (BITMAPINFO *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER));
	    break;

	default:
	    b_info = (BITMAPINFO *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		    sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (2 << desc.ddpfPixelFormat.u.dwRGBBitCount));
	    break;
	}

	b_info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	b_info->bmiHeader.biWidth = desc.dwWidth;
	b_info->bmiHeader.biHeight = -desc.dwHeight;
	b_info->bmiHeader.biPlanes = 1;
	b_info->bmiHeader.biBitCount = desc.ddpfPixelFormat.u.dwRGBBitCount;
#if 0
	if ((desc.ddpfPixelFormat.u.dwRGBBitCount != 16) &&
	    (desc.ddpfPixelFormat.u.dwRGBBitCount != 32))
#endif
	b_info->bmiHeader.biCompression = BI_RGB;
#if 0
	else
	b_info->bmiHeader.biCompression = BI_BITFIELDS;
#endif
	b_info->bmiHeader.biSizeImage = (desc.ddpfPixelFormat.u.dwRGBBitCount / 8) * desc.dwWidth * desc.dwHeight;
	b_info->bmiHeader.biXPelsPerMeter = 0;
	b_info->bmiHeader.biYPelsPerMeter = 0;
	b_info->bmiHeader.biClrUsed = 0;
	b_info->bmiHeader.biClrImportant = 0;

	switch (desc.ddpfPixelFormat.u.dwRGBBitCount) {
	case 16:
	case 32:
#if 0
	    {
		DWORD *masks = (DWORD *) &(b_info->bmiColors);

		usage = 0;
		masks[0] = desc.ddpfPixelFormat.u1.dwRBitMask;
		masks[1] = desc.ddpfPixelFormat.u2.dwGBitMask;
		masks[2] = desc.ddpfPixelFormat.u3.dwBBitMask;
	    }
	    break;
#endif
	case 24:
	    /* Nothing to do */
	    usage = DIB_RGB_COLORS;
	    break;

	default: {
		int i;

		/* Fill the palette */
		usage = DIB_RGB_COLORS;

		if (This->s.palette == NULL) {
		    ERR("Bad palette !!!\n");
		} else {
		    RGBQUAD *rgb = (RGBQUAD *) &(b_info->bmiColors);
		    PALETTEENTRY *pent = (PALETTEENTRY *)&(This->s.palette->palents);

		    for (i=0;i<(1<<desc.ddpfPixelFormat.u.dwRGBBitCount);i++) {
			rgb[i].rgbBlue = pent[i].peBlue;
			rgb[i].rgbRed = pent[i].peRed;
			rgb[i].rgbGreen = pent[i].peGreen; 
		    }
		}
	    }
	    break;
	}
	ddc = CreateDCA("DISPLAY",NULL,NULL,NULL);
	This->s.DIBsection = ddc ? DIB_CreateDIBSection(ddc,
	    b_info,
	    usage,
	    &(This->s.bitmap_data),
	    0,
	    (DWORD)desc.u1.lpSurface,
	    desc.lPitch
	) : 0;
	if (!This->s.DIBsection) {
		ERR("CreateDIBSection failed!\n");
		if (ddc) DeleteDC(ddc);
		HeapFree(GetProcessHeap(), 0, b_info);
		return E_FAIL;
	}
	TRACE("DIBSection at : %p\n", This->s.bitmap_data);

	/* b_info is not useful anymore */
	HeapFree(GetProcessHeap(), 0, b_info);

	/* Create the DC */
	This->s.hdc = CreateCompatibleDC(ddc);
	This->s.holdbitmap = SelectObject(This->s.hdc, This->s.DIBsection);

	if (ddc) DeleteDC(ddc);
    }

    if (This->s.bitmap_data != desc.u1.lpSurface) {
        FIXME("DIBSection not created for frame buffer, reverting to old code\n");
	/* Copy our surface in the DIB section */
	if ((GET_BPP(desc) * desc.dwWidth) == desc.lPitch)
	    memcpy(This->s.bitmap_data,desc.u1.lpSurface,desc.lPitch*desc.dwHeight);
	else
	    /* TODO */
	    FIXME("This case has to be done :/\n");
    }

    if (lphdc) {
	TRACE("HDC : %08lx\n", (DWORD) This->s.hdc);
	*lphdc = This->s.hdc;
    }

    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_ReleaseDC(LPDIRECTDRAWSURFACE4 iface,HDC hdc) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);

    TRACE("(%p)->(0x%08lx)\n",This,(long)hdc);

    if (This->s.bitmap_data != This->s.surface_desc.u1.lpSurface) {
	TRACE( "Copying DIBSection at : %p\n", This->s.bitmap_data);
	/* Copy the DIB section to our surface */
	if ((GET_BPP(This->s.surface_desc) * This->s.surface_desc.dwWidth) == This->s.surface_desc.lPitch) {
	    memcpy(This->s.surface_desc.u1.lpSurface, This->s.bitmap_data, This->s.surface_desc.lPitch * This->s.surface_desc.dwHeight);
	} else {
	    /* TODO */
	    FIXME("This case has to be done :/\n");
	}
    }
    /* Unlock the surface */
    IDirectDrawSurface4_Unlock(iface,This->s.surface_desc.u1.lpSurface);
    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_QueryInterface(
    LPDIRECTDRAWSURFACE4 iface,REFIID refiid,LPVOID *obj
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);

    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(refiid),obj);
    
    /* All DirectDrawSurface versions (1, 2, 3 and 4) use
     * the same interface. And IUnknown does that too of course.
     */
    if ( IsEqualGUID( &IID_IDirectDrawSurface4, refiid )	||
	 IsEqualGUID( &IID_IDirectDrawSurface3, refiid )	||
	 IsEqualGUID( &IID_IDirectDrawSurface2, refiid )	||
	 IsEqualGUID( &IID_IDirectDrawSurface,  refiid )	||
	 IsEqualGUID( &IID_IUnknown,            refiid )
    ) {
	    *obj = This;
	    IDirectDrawSurface4_AddRef(iface);

	    TRACE("  Creating IDirectDrawSurface interface (%p)\n", *obj);
	    return S_OK;
    }
    FIXME("(%p):interface for IID %s NOT found!\n",This,debugstr_guid(refiid));
    return OLE_E_ENUM_NOMORE;
}

HRESULT WINAPI IDirectDrawSurface4Impl_IsLost(LPDIRECTDRAWSURFACE4 iface) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    TRACE("(%p)->(), stub!\n",This);
    return DD_OK; /* hmm */
}

HRESULT WINAPI IDirectDrawSurface4Impl_EnumAttachedSurfaces(
    LPDIRECTDRAWSURFACE4 iface,LPVOID context,LPDDENUMSURFACESCALLBACK esfcb
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    int i;
    struct _surface_chain *chain = This->s.chain;

    TRACE("(%p)->(%p,%p)\n",This,context,esfcb);
    for (i=0;i<chain->nrofsurfaces;i++) {
      TRACE( "Enumerating attached surface (%p)\n", chain->surfaces[i]);
      if (esfcb((LPDIRECTDRAWSURFACE) chain->surfaces[i], &(chain->surfaces[i]->s.surface_desc), context) == DDENUMRET_CANCEL)
	return DD_OK; /* FIXME: return value correct? */
    }
    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_Restore(LPDIRECTDRAWSURFACE4 iface) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    FIXME("(%p)->(),stub!\n",This);
    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_SetColorKey(
    LPDIRECTDRAWSURFACE4 iface, DWORD dwFlags, LPDDCOLORKEY ckey ) 
{
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    TRACE("(%p)->(0x%08lx,%p)\n",This,dwFlags,ckey);
    if (TRACE_ON(ddraw)) {
	_dump_colorkeyflag(dwFlags);
	DPRINTF(" : ");
	_dump_DDCOLORKEY((void *) ckey);
	DPRINTF("\n");
    }

    /* If this surface was loaded as a texture, call also the texture
     * SetColorKey callback. FIXME: hack approach :(
     */
    if (This->s.texture)
	This->s.SetColorKey_cb(This->s.texture, dwFlags, ckey);

    if( dwFlags & DDCKEY_SRCBLT ) {
	dwFlags &= ~DDCKEY_SRCBLT;
	This->s.surface_desc.dwFlags |= DDSD_CKSRCBLT;
	memcpy( &(This->s.surface_desc.ddckCKSrcBlt), ckey, sizeof( *ckey ) );
    }

    if( dwFlags & DDCKEY_DESTBLT ) {
	dwFlags &= ~DDCKEY_DESTBLT;
	This->s.surface_desc.dwFlags |= DDSD_CKDESTBLT;
	memcpy( &(This->s.surface_desc.ddckCKDestBlt), ckey, sizeof( *ckey ) );
    }

    if( dwFlags & DDCKEY_SRCOVERLAY ) {
	dwFlags &= ~DDCKEY_SRCOVERLAY;
	This->s.surface_desc.dwFlags |= DDSD_CKSRCOVERLAY;
	memcpy( &(This->s.surface_desc.ddckCKSrcOverlay), ckey, sizeof( *ckey ) );	   
    }

    if( dwFlags & DDCKEY_DESTOVERLAY ) {
	dwFlags &= ~DDCKEY_DESTOVERLAY;
	This->s.surface_desc.dwFlags |= DDSD_CKDESTOVERLAY;
	memcpy( &(This->s.surface_desc.ddckCKDestOverlay), ckey, sizeof( *ckey ) );	   
    }
    if( dwFlags )
	FIXME("unhandled dwFlags: 0x%08lx\n", dwFlags );
    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_AddOverlayDirtyRect(
    LPDIRECTDRAWSURFACE4 iface, LPRECT lpRect
) {
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME("(%p)->(%p),stub!\n",This,lpRect); 

  return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_DeleteAttachedSurface(
    LPDIRECTDRAWSURFACE4 iface, DWORD dwFlags,
    LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    int i;
    struct _surface_chain *chain;

    TRACE("(%p)->(0x%08lx,%p)\n",This,dwFlags,lpDDSAttachedSurface);
    chain = This->s.chain;
    for (i=0;i<chain->nrofsurfaces;i++) {
	if ((IDirectDrawSurface4Impl*)lpDDSAttachedSurface==chain->surfaces[i]){
	    IDirectDrawSurface4_Release(lpDDSAttachedSurface);

	    chain->surfaces[i]->s.chain = NULL;
	    memcpy( chain->surfaces+i,
		    chain->surfaces+(i+1),
		    (chain->nrofsurfaces-i-1)*sizeof(chain->surfaces[i])
	    );
	    chain->surfaces = HeapReAlloc(
		GetProcessHeap(),
		0,
		chain->surfaces,
		sizeof(chain->surfaces[i])*(chain->nrofsurfaces-1)
	    );
	    chain->nrofsurfaces--;
	    return DD_OK;
	}
    }
    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_EnumOverlayZOrders(
    LPDIRECTDRAWSURFACE4 iface, DWORD dwFlags, LPVOID lpContext,
    LPDDENUMSURFACESCALLBACK lpfnCallback
) {
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,%p),stub!\n", This,dwFlags,
          lpContext, lpfnCallback );

  return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_GetClipper(
    LPDIRECTDRAWSURFACE4 iface, LPDIRECTDRAWCLIPPER* lplpDDClipper
) {
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME("(%p)->(%p),stub!\n", This, lplpDDClipper);

  return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_GetColorKey(
    LPDIRECTDRAWSURFACE4 iface, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    TRACE("(%p)->(0x%08lx,%p)\n", This, dwFlags, lpDDColorKey);

    if( dwFlags & DDCKEY_SRCBLT )  {
	dwFlags &= ~DDCKEY_SRCBLT;
	memcpy( lpDDColorKey, &(This->s.surface_desc.ddckCKSrcBlt), sizeof( *lpDDColorKey ) );
    }
    if( dwFlags & DDCKEY_DESTBLT ) {
	dwFlags &= ~DDCKEY_DESTBLT;
	memcpy( lpDDColorKey, &(This->s.surface_desc.ddckCKDestBlt), sizeof( *lpDDColorKey ) );
    }
    if( dwFlags & DDCKEY_SRCOVERLAY ) {
	dwFlags &= ~DDCKEY_SRCOVERLAY;
	memcpy( lpDDColorKey, &(This->s.surface_desc.ddckCKSrcOverlay), sizeof( *lpDDColorKey ) );
    }
    if( dwFlags & DDCKEY_DESTOVERLAY ) {
	dwFlags &= ~DDCKEY_DESTOVERLAY;
	memcpy( lpDDColorKey, &(This->s.surface_desc.ddckCKDestOverlay), sizeof( *lpDDColorKey ) );
    }
    if( dwFlags )
	FIXME("unhandled dwFlags: 0x%08lx\n", dwFlags );
    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_GetFlipStatus(
    LPDIRECTDRAWSURFACE4 iface, DWORD dwFlags
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    FIXME("(%p)->(0x%08lx),stub!\n", This, dwFlags);

    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_GetPalette(
    LPDIRECTDRAWSURFACE4 iface, LPDIRECTDRAWPALETTE* lplpDDPalette
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    TRACE("(%p)->(%p),stub!\n", This, lplpDDPalette);

    if (!This->s.palette)
	return DDERR_NOPALETTEATTACHED;

    IDirectDrawPalette_AddRef( (IDirectDrawPalette*) This->s.palette );
    *lplpDDPalette = (IDirectDrawPalette*) This->s.palette;
    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_SetOverlayPosition(
    LPDIRECTDRAWSURFACE4 iface, LONG lX, LONG lY
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    FIXME("(%p)->(%ld,%ld),stub!\n", This, lX, lY);

    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_UpdateOverlay(
    LPDIRECTDRAWSURFACE4 iface, LPRECT lpSrcRect,
    LPDIRECTDRAWSURFACE4 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags,
    LPDDOVERLAYFX lpDDOverlayFx
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    FIXME("(%p)->(%p,%p,%p,0x%08lx,%p),stub!\n", This,
	 lpSrcRect, lpDDDestSurface, lpDestRect, dwFlags, lpDDOverlayFx );  

    return DD_OK;
}
 
HRESULT WINAPI IDirectDrawSurface4Impl_UpdateOverlayDisplay(
    LPDIRECTDRAWSURFACE4 iface, DWORD dwFlags
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    FIXME("(%p)->(0x%08lx),stub!\n", This, dwFlags); 

    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_UpdateOverlayZOrder(
    LPDIRECTDRAWSURFACE4 iface,DWORD dwFlags,LPDIRECTDRAWSURFACE4 lpDDSReference
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    FIXME("(%p)->(0x%08lx,%p),stub!\n", This, dwFlags, lpDDSReference);

    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_GetDDInterface(
    LPDIRECTDRAWSURFACE4 iface, LPVOID* lplpDD
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    FIXME("(%p)->(%p),stub!\n", This, lplpDD);

    /* Not sure about that... */

    IDirectDraw_AddRef((LPDIRECTDRAW)This->s.ddraw),
    *lplpDD = (void *) This->s.ddraw;

    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_PageLock(
    LPDIRECTDRAWSURFACE4 iface, DWORD dwFlags
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    FIXME("(%p)->(0x%08lx),stub!\n", This, dwFlags);

    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_PageUnlock(
    LPDIRECTDRAWSURFACE4 iface, DWORD dwFlags
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    FIXME("(%p)->(0x%08lx),stub!\n", This, dwFlags);

    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_SetSurfaceDesc(
    LPDIRECTDRAWSURFACE4 iface, LPDDSURFACEDESC lpDDSD, DWORD dwFlags
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    FIXME("(%p)->(%p,0x%08lx),stub!\n", This, lpDDSD, dwFlags);

    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_SetPrivateData(
    LPDIRECTDRAWSURFACE4 iface, REFGUID guidTag, LPVOID lpData, DWORD cbSize,
    DWORD dwFlags
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    FIXME("(%p)->(%p,%p,%ld,%08lx\n", This, guidTag, lpData, cbSize, dwFlags);

    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_GetPrivateData(
    LPDIRECTDRAWSURFACE4 iface, REFGUID guidTag, LPVOID lpBuffer,
    LPDWORD lpcbBufferSize
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    FIXME("(%p)->(%p,%p,%p)\n", This, guidTag, lpBuffer, lpcbBufferSize);

    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_FreePrivateData(
    LPDIRECTDRAWSURFACE4 iface, REFGUID guidTag
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    FIXME("(%p)->(%p)\n", This, guidTag);

    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_GetUniquenessValue(
    LPDIRECTDRAWSURFACE4 iface, LPDWORD lpValue
)  {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    FIXME("(%p)->(%p)\n", This, lpValue);

    return DD_OK;
}

HRESULT WINAPI IDirectDrawSurface4Impl_ChangeUniquenessValue(
    LPDIRECTDRAWSURFACE4 iface
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    FIXME("(%p)\n", This);

    return DD_OK;
}
