/*
 * GDI bitmap objects
 *
 * Copyright 1993 Alexandre Julliard
 *           1998 Huw D M Davies
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

#include <stdlib.h>
#include <string.h>

#include "wine/winbase16.h"
#include "gdi.h"
#include "bitmap.h"
#include "wine/debug.h"
#include "wine/winuser16.h"

WINE_DEFAULT_DEBUG_CHANNEL(bitmap);


static HGDIOBJ BITMAP_SelectObject( HGDIOBJ handle, void *obj, HDC hdc );
static INT BITMAP_GetObject16( HGDIOBJ handle, void *obj, INT count, LPVOID buffer );
static INT BITMAP_GetObject( HGDIOBJ handle, void *obj, INT count, LPVOID buffer );
static BOOL BITMAP_DeleteObject( HGDIOBJ handle, void *obj );

static const struct gdi_obj_funcs bitmap_funcs =
{
    BITMAP_SelectObject,  /* pSelectObject */
    BITMAP_GetObject16,   /* pGetObject16 */
    BITMAP_GetObject,     /* pGetObjectA */
    BITMAP_GetObject,     /* pGetObjectW */
    NULL,                 /* pUnrealizeObject */
    BITMAP_DeleteObject   /* pDeleteObject */
};

/***********************************************************************
 *           BITMAP_GetWidthBytes
 *
 * Return number of bytes taken by a scanline of 16-bit aligned Windows DDB
 * data.
 */
static INT BITMAP_GetWidthBytes( INT bmWidth, INT bpp )
{
    switch(bpp)
    {
    case 1:
	return 2 * ((bmWidth+15) >> 4);

    case 24:
	bmWidth *= 3; /* fall through */
    case 8:
	return bmWidth + (bmWidth & 1);

    case 32:
	return bmWidth * 4;

    case 16:
    case 15:
	return bmWidth * 2;

    case 4:
	return 2 * ((bmWidth+3) >> 2);

    default:
	WARN("Unknown depth %d, please report.\n", bpp );
    }
    return -1;
}


/******************************************************************************
 * CreateBitmap [GDI32.@]  Creates a bitmap with the specified info
 *
 * PARAMS
 *    width  [I] bitmap width
 *    height [I] bitmap height
 *    planes [I] Number of color planes
 *    bpp    [I] Number of bits to identify a color
 *    bits   [I] Pointer to array containing color data
 *
 * RETURNS
 *    Success: Handle to bitmap
 *    Failure: 0
 */
HBITMAP WINAPI CreateBitmap( INT width, INT height, UINT planes,
                                 UINT bpp, LPCVOID bits )
{
    BITMAPOBJ *bmp;
    HBITMAP hbitmap;

    planes = (BYTE)planes;
    bpp    = (BYTE)bpp;


      /* Check parameters */
    if (!height || !width)
    {
        height = 1;
        width  = 1;
        planes = 1;
        bpp    = 1;
    }
    if (planes != 1) {
        FIXME("planes = %d\n", planes);
	return 0;
    }
    if (height < 0) height = -height;
    if (width < 0) width = -width;

      /* Create the BITMAPOBJ */
    if (!(bmp = GDI_AllocObject( sizeof(BITMAPOBJ), BITMAP_MAGIC,
				 (HGDIOBJ *)&hbitmap, &bitmap_funcs )))
        return 0;

    TRACE("%dx%d, %d colors returning %p\n", width, height, 1 << (planes*bpp), hbitmap);

    bmp->size.cx = 0;
    bmp->size.cy = 0;
    bmp->bitmap.bmType = 0;
    bmp->bitmap.bmWidth = width;
    bmp->bitmap.bmHeight = height;
    bmp->bitmap.bmPlanes = planes;
    bmp->bitmap.bmBitsPixel = bpp;
    bmp->bitmap.bmWidthBytes = BITMAP_GetWidthBytes( width, bpp );
    bmp->bitmap.bmBits = NULL;

    bmp->funcs = NULL;
    bmp->physBitmap = NULL;
    bmp->dib = NULL;
    bmp->segptr_bits = 0;

    if (bits) /* Set bitmap bits */
	SetBitmapBits( hbitmap, height * bmp->bitmap.bmWidthBytes,
                         bits );
    GDI_ReleaseObj( hbitmap );
    return hbitmap;
}


/******************************************************************************
 * CreateCompatibleBitmap [GDI32.@]  Creates a bitmap compatible with the DC
 *
 * PARAMS
 *    hdc    [I] Handle to device context
 *    width  [I] Width of bitmap
 *    height [I] Height of bitmap
 *
 * RETURNS
 *    Success: Handle to bitmap
 *    Failure: 0
 */
HBITMAP WINAPI CreateCompatibleBitmap( HDC hdc, INT width, INT height)
{
    HBITMAP hbmpRet = 0;
    DC *dc;

    TRACE("(%p,%d,%d) = \n", hdc, width, height );
    if (!(dc = DC_GetDCPtr( hdc ))) return 0;
    if ((width >= 0x10000) || (height >= 0x10000)) {
	FIXME("got bad width %d or height %d, please look for reason\n",
	      width, height );
    } else {
        /* MS doc says if width or height is 0, return 1-by-1 pixel, monochrome bitmap */
        if (!width || !height)
	   hbmpRet = CreateBitmap( 1, 1, 1, 1, NULL );
	else
	   hbmpRet = CreateBitmap( width, height, 1, dc->bitsPerPixel, NULL );

        if (!BITMAP_SetOwnerDC( hbmpRet, dc ))
        {
            DeleteObject( hbmpRet );
            hbmpRet = 0;
        }
    }
    TRACE("\t\t%p\n", hbmpRet);
    GDI_ReleaseObj(hdc);
    return hbmpRet;
}


/******************************************************************************
 * CreateBitmapIndirect [GDI32.@]  Creates a bitmap with the specifies info
 *
 * RETURNS
 *    Success: Handle to bitmap
 *    Failure: NULL
 */
HBITMAP WINAPI CreateBitmapIndirect(
    const BITMAP * bmp) /* [in] Pointer to the bitmap data */
{
    return CreateBitmap( bmp->bmWidth, bmp->bmHeight, bmp->bmPlanes,
                           bmp->bmBitsPixel, bmp->bmBits );
}


/***********************************************************************
 * GetBitmapBits [GDI32.@]  Copies bitmap bits of bitmap to buffer
 *
 * RETURNS
 *    Success: Number of bytes copied
 *    Failure: 0
 */
LONG WINAPI GetBitmapBits(
    HBITMAP hbitmap, /* [in]  Handle to bitmap */
    LONG count,        /* [in]  Number of bytes to copy */
    LPVOID bits)       /* [out] Pointer to buffer to receive bits */
{
    BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    LONG height, ret;

    if (!bmp) return 0;

    /* If the bits vector is null, the function should return the read size */
    if(bits == NULL)
    {
        ret = bmp->bitmap.bmWidthBytes * bmp->bitmap.bmHeight;
        goto done;
    }

    if (count < 0) {
	WARN("(%ld): Negative number of bytes passed???\n", count );
	count = -count;
    }

    /* Only get entire lines */
    height = count / bmp->bitmap.bmWidthBytes;
    if (height > bmp->bitmap.bmHeight) height = bmp->bitmap.bmHeight;
    count = height * bmp->bitmap.bmWidthBytes;
    if (count == 0)
      {
	WARN("Less than one entire line requested\n");
        ret = 0;
        goto done;
      }


    TRACE("(%p, %ld, %p) %dx%d %d colors fetched height: %ld\n",
          hbitmap, count, bits, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
          1 << bmp->bitmap.bmBitsPixel, height );

    if(bmp->funcs)
    {
        TRACE("Calling device specific BitmapBits\n");
        if(bmp->funcs->pGetBitmapBits)
            ret = bmp->funcs->pGetBitmapBits(hbitmap, bits, count);
        else
        {
            memset( bits, 0, count );
            ret = count;
        }
    } else {

        if(!bmp->bitmap.bmBits) {
	    WARN("Bitmap is empty\n");
	    ret = 0;
	} else {
	    memcpy(bits, bmp->bitmap.bmBits, count);
	    ret = count;
	}

    }
 done:
    GDI_ReleaseObj( hbitmap );
    return ret;
}


/******************************************************************************
 * SetBitmapBits [GDI32.@]  Sets bits of color data for a bitmap
 *
 * RETURNS
 *    Success: Number of bytes used in setting the bitmap bits
 *    Failure: 0
 */
LONG WINAPI SetBitmapBits(
    HBITMAP hbitmap, /* [in] Handle to bitmap */
    LONG count,        /* [in] Number of bytes in bitmap array */
    LPCVOID bits)      /* [in] Address of array with bitmap bits */
{
    BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    LONG height, ret;

    if ((!bmp) || (!bits))
	return 0;

    if (count < 0) {
	WARN("(%ld): Negative number of bytes passed???\n", count );
	count = -count;
    }

    /* Only get entire lines */
    height = count / bmp->bitmap.bmWidthBytes;
    if (height > bmp->bitmap.bmHeight) height = bmp->bitmap.bmHeight;
    count = height * bmp->bitmap.bmWidthBytes;

    TRACE("(%p, %ld, %p) %dx%d %d colors fetched height: %ld\n",
          hbitmap, count, bits, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
          1 << bmp->bitmap.bmBitsPixel, height );

    if(bmp->funcs) {

        TRACE("Calling device specific BitmapBits\n");
        if(bmp->funcs->pSetBitmapBits)
            ret = bmp->funcs->pSetBitmapBits(hbitmap, bits, count);
        else
            ret = 0;
    } else {

        if(!bmp->bitmap.bmBits) /* Alloc enough for entire bitmap */
	    bmp->bitmap.bmBits = HeapAlloc( GetProcessHeap(), 0, count );
	if(!bmp->bitmap.bmBits) {
	    WARN("Unable to allocate bit buffer\n");
	    ret = 0;
	} else {
	    memcpy(bmp->bitmap.bmBits, bits, count);
	    ret = count;
	}
    }

    GDI_ReleaseObj( hbitmap );
    return ret;
}

/**********************************************************************
 *		BITMAP_CopyBitmap
 *
 */
HBITMAP BITMAP_CopyBitmap(HBITMAP hbitmap)
{
    BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    HBITMAP res = 0;
    BITMAP bm;

    if(!bmp) return 0;

    bm = bmp->bitmap;
    bm.bmBits = NULL;
    res = CreateBitmapIndirect(&bm);

    if(res) {
        char *buf = HeapAlloc( GetProcessHeap(), 0, bm.bmWidthBytes *
			       bm.bmHeight );
        GetBitmapBits (hbitmap, bm.bmWidthBytes * bm.bmHeight, buf);
	SetBitmapBits (res, bm.bmWidthBytes * bm.bmHeight, buf);
	HeapFree( GetProcessHeap(), 0, buf );
    }

    GDI_ReleaseObj( hbitmap );
    return res;
}


/***********************************************************************
 *           BITMAP_SetOwnerDC
 *
 * Set the type of DC that owns the bitmap. This is used when the
 * bitmap is selected into a device to initialize the bitmap function
 * table.
 */
BOOL BITMAP_SetOwnerDC( HBITMAP hbitmap, DC *dc )
{
    BITMAPOBJ *bitmap;
    BOOL ret;

    /* never set the owner of the stock bitmap since it can be selected in multiple DCs */
    if (hbitmap == GetStockObject(DEFAULT_BITMAP)) return TRUE;

    if (!(bitmap = GDI_GetObjPtr( hbitmap, BITMAP_MAGIC ))) return FALSE;

    ret = TRUE;
    if (!bitmap->funcs)  /* not owned by a DC yet */
    {
        if (dc->funcs->pCreateBitmap) ret = dc->funcs->pCreateBitmap( dc->physDev, hbitmap );
        if (ret) bitmap->funcs = dc->funcs;
    }
    else if (bitmap->funcs != dc->funcs)
    {
        FIXME( "Trying to select bitmap %p in different DC type\n", hbitmap );
        ret = FALSE;
    }
    GDI_ReleaseObj( hbitmap );
    return ret;
}


/***********************************************************************
 *           BITMAP_SelectObject
 */
static HGDIOBJ BITMAP_SelectObject( HGDIOBJ handle, void *obj, HDC hdc )
{
    HGDIOBJ ret;
    BITMAPOBJ *bitmap = obj;
    DC *dc = DC_GetDCPtr( hdc );

    if (!dc) return 0;
    if (GetObjectType( hdc ) != OBJ_MEMDC)
    {
        GDI_ReleaseObj( hdc );
        return 0;
    }
    ret = dc->hBitmap;
    if (handle == dc->hBitmap) goto done;  /* nothing to do */

    if (bitmap->header.dwCount && (handle != GetStockObject(DEFAULT_BITMAP)))
    {
        WARN( "Bitmap already selected in another DC\n" );
        GDI_ReleaseObj( hdc );
        return 0;
    }

    if (!bitmap->funcs && !BITMAP_SetOwnerDC( handle, dc ))
    {
        GDI_ReleaseObj( hdc );
        return 0;
    }

    if (dc->funcs->pSelectBitmap) handle = dc->funcs->pSelectBitmap( dc->physDev, handle );

    if (handle)
    {
        dc->hBitmap            = handle;
        dc->totalExtent.left   = 0;
        dc->totalExtent.top    = 0;
        dc->totalExtent.right  = bitmap->bitmap.bmWidth;
        dc->totalExtent.bottom = bitmap->bitmap.bmHeight;
        dc->flags &= ~DC_DIRTY;
        SetRectRgn( dc->hVisRgn, 0, 0, bitmap->bitmap.bmWidth, bitmap->bitmap.bmHeight);
        CLIPPING_UpdateGCRegion( dc );

        if (dc->bitsPerPixel != bitmap->bitmap.bmBitsPixel)
        {
            /* depth changed, reinitialize the DC */
            dc->bitsPerPixel = bitmap->bitmap.bmBitsPixel;
            DC_InitDC( dc );
        }
    }
    else ret = 0;

 done:
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           BITMAP_DeleteObject
 */
static BOOL BITMAP_DeleteObject( HGDIOBJ handle, void *obj )
{
    BITMAPOBJ * bmp = obj;

    if (bmp->funcs && bmp->funcs->pDeleteBitmap)
        bmp->funcs->pDeleteBitmap( handle );

    if( bmp->bitmap.bmBits )
        HeapFree( GetProcessHeap(), 0, bmp->bitmap.bmBits );

    if (bmp->dib)
    {
        DIBSECTION *dib = bmp->dib;

        if (dib->dsBm.bmBits)
        {
            if (dib->dshSection)
            {
                SYSTEM_INFO SystemInfo;
                GetSystemInfo( &SystemInfo );
                UnmapViewOfFile( (char *)dib->dsBm.bmBits -
                                 (dib->dsOffset % SystemInfo.dwAllocationGranularity) );
            }
            else if (!dib->dsOffset)
                VirtualFree(dib->dsBm.bmBits, 0L, MEM_RELEASE );
        }
        HeapFree(GetProcessHeap(), 0, dib);
        bmp->dib = NULL;
        if (bmp->segptr_bits)
        { /* free its selector array */
            WORD sel = SELECTOROF(bmp->segptr_bits);
            WORD count = (GetSelectorLimit16(sel) / 0x10000) + 1;
            int i;

            for (i = 0; i < count; i++) FreeSelector16(sel + (i << __AHSHIFT));
        }
    }
    return GDI_FreeObject( handle, obj );
}


/***********************************************************************
 *           BITMAP_GetObject16
 */
static INT BITMAP_GetObject16( HGDIOBJ handle, void *obj, INT count, LPVOID buffer )
{
    BITMAPOBJ *bmp = obj;

    if (bmp->dib)
    {
        if ( count <= sizeof(BITMAP16) )
        {
            BITMAP *bmp32 = &bmp->dib->dsBm;
	    BITMAP16 bmp16;
	    bmp16.bmType       = bmp32->bmType;
	    bmp16.bmWidth      = bmp32->bmWidth;
	    bmp16.bmHeight     = bmp32->bmHeight;
	    bmp16.bmWidthBytes = bmp32->bmWidthBytes;
	    bmp16.bmPlanes     = bmp32->bmPlanes;
	    bmp16.bmBitsPixel  = bmp32->bmBitsPixel;
	    bmp16.bmBits       = (SEGPTR)0;
	    memcpy( buffer, &bmp16, count );
	    return count;
        }
        else
        {
	    FIXME("not implemented for DIBs: count %d\n", count);
	    return 0;
        }
    }
    else
    {
	BITMAP16 bmp16;
	bmp16.bmType       = bmp->bitmap.bmType;
	bmp16.bmWidth      = bmp->bitmap.bmWidth;
	bmp16.bmHeight     = bmp->bitmap.bmHeight;
	bmp16.bmWidthBytes = bmp->bitmap.bmWidthBytes;
	bmp16.bmPlanes     = bmp->bitmap.bmPlanes;
	bmp16.bmBitsPixel  = bmp->bitmap.bmBitsPixel;
	bmp16.bmBits       = (SEGPTR)0;
	if (count > sizeof(bmp16)) count = sizeof(bmp16);
	memcpy( buffer, &bmp16, count );
	return count;
    }
}


/***********************************************************************
 *           BITMAP_GetObject
 */
static INT BITMAP_GetObject( HGDIOBJ handle, void *obj, INT count, LPVOID buffer )
{
    BITMAPOBJ *bmp = obj;

    if (bmp->dib)
    {
        if( !buffer )
            return sizeof(DIBSECTION);
	if (count < sizeof(DIBSECTION))
	{
	    if (count > sizeof(BITMAP)) count = sizeof(BITMAP);
	}
	else
	{
	    if (count > sizeof(DIBSECTION)) count = sizeof(DIBSECTION);
	}

	memcpy( buffer, bmp->dib, count );
	return count;
    }
    else
    {
        if( !buffer )
            return sizeof(BITMAP);
	if (count > sizeof(BITMAP)) count = sizeof(BITMAP);
	memcpy( buffer, &bmp->bitmap, count );
	return count;
    }
}


/******************************************************************************
 * CreateDiscardableBitmap [GDI32.@]  Creates a discardable bitmap
 *
 * RETURNS
 *    Success: Handle to bitmap
 *    Failure: NULL
 */
HBITMAP WINAPI CreateDiscardableBitmap(
    HDC hdc,    /* [in] Handle to device context */
    INT width,  /* [in] Bitmap width */
    INT height) /* [in] Bitmap height */
{
    return CreateCompatibleBitmap( hdc, width, height );
}


/******************************************************************************
 * GetBitmapDimensionEx [GDI32.@]  Retrieves dimensions of a bitmap
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetBitmapDimensionEx(
    HBITMAP hbitmap, /* [in]  Handle to bitmap */
    LPSIZE size)     /* [out] Address of struct receiving dimensions */
{
    BITMAPOBJ * bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    if (!bmp) return FALSE;
    *size = bmp->size;
    GDI_ReleaseObj( hbitmap );
    return TRUE;
}


/******************************************************************************
 * SetBitmapDimensionEx [GDI32.@]  Assignes dimensions to a bitmap
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetBitmapDimensionEx(
    HBITMAP hbitmap, /* [in]  Handle to bitmap */
    INT x,           /* [in]  Bitmap width */
    INT y,           /* [in]  Bitmap height */
    LPSIZE prevSize) /* [out] Address of structure for orig dims */
{
    BITMAPOBJ * bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    if (!bmp) return FALSE;
    if (prevSize) *prevSize = bmp->size;
    bmp->size.cx = x;
    bmp->size.cy = y;
    GDI_ReleaseObj( hbitmap );
    return TRUE;
}
