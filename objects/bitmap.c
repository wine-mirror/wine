/*
 * GDI bitmap objects
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <stdlib.h>
#include <string.h>
#include "ts_xlib.h"
#include "ts_xutil.h"
#include "gdi.h"
#include "callback.h"
#include "dc.h"
#include "bitmap.h"
#include "heap.h"
#include "debug.h"

#ifdef PRELIMINARY_WING16_SUPPORT
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

  /* GCs used for B&W and color bitmap operations */
GC BITMAP_monoGC = 0, BITMAP_colorGC = 0;

/***********************************************************************
 *           XPutImage_wrapper
 *
 * Wrapper to call XPutImage with CALL_LARGE_STACK.
 */

struct XPutImage_descr
{
    BITMAPOBJ *bmp;
    XImage    *image;
    INT32      width;
    INT32      height;
};

static int XPutImage_wrapper( const struct XPutImage_descr *descr )
{
    return XPutImage( display, descr->bmp->pixmap, BITMAP_GC(descr->bmp),
                      descr->image, 0, 0, 0, 0, descr->width, descr->height );
}

/***********************************************************************
 *           BITMAP_GetBitsPadding
 *
 * Return number of bytes to pad a scanline of 16-bit aligned Windows DDB data.
 */
INT32 BITMAP_GetBitsPadding( int bmWidth, int bpp )
{
    INT32 pad;

    switch (bpp) 
    {
    case 1:
	if (!(bmWidth & 15)) pad = 0;
	else pad = ((16 - (bmWidth & 15)) + 7) / 8;
	break;

    case 8:
	pad = (2 - (bmWidth & 1)) & 1;
	break;

    case 24:
	pad = (bmWidth*3) & 1;
	break;

    case 32:
    case 16:
    case 15:
	pad = 0; /* we have 16bit alignment already */
	break;

    case 4:
	if (!(bmWidth & 3)) pad = 0;
	else pad = ((4 - (bmWidth & 3)) + 1) / 2;
	break;

    default:
	WARN(bitmap,"Unknown depth %d, please report.\n", bpp );
        return -1;
    }
    return pad;
}

/***********************************************************************
 *           BITMAP_GetBitsWidth
 *
 * Return number of bytes taken by a scanline of 16-bit aligned Windows DDB data.
 */
INT32 BITMAP_GetBitsWidth( int bmWidth, int bpp )
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
	WARN(bitmap,"Unknown depth %d, please report.\n", bpp );
    }
    return -1;
}

/***********************************************************************
 *           CreateBitmap16    (GDI.48)
 */
HBITMAP16 WINAPI CreateBitmap16( INT16 width, INT16 height, UINT16 planes,
                                 UINT16 bpp, LPCVOID bits )
{
    return CreateBitmap32( width, height, planes, bpp, bits );
}


/******************************************************************************
 * CreateBitmap32 [GDI32.25]  Creates a bitmap with the specified info
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
 *    Failure: NULL
 */
HBITMAP32 WINAPI CreateBitmap32( INT32 width, INT32 height, UINT32 planes,
                                 UINT32 bpp, LPCVOID bits )
{
    BITMAPOBJ * bmpObjPtr;
    HBITMAP32 hbitmap;

    planes = (BYTE)planes;
    bpp    = (BYTE)bpp;

    TRACE(gdi, "%dx%d, %d colors\n", width, height, 1 << (planes*bpp) );

      /* Check parameters */
    if (!height || !width || planes != 1) return 0;
    if ((bpp != 1) && (bpp != screenDepth)) return 0;
    if (height < 0) height = -height;
    if (width < 0) width = -width;

      /* Create the BITMAPOBJ */
    hbitmap = GDI_AllocObject( sizeof(BITMAPOBJ), BITMAP_MAGIC );
    if (!hbitmap) return 0;
    bmpObjPtr = (BITMAPOBJ *) GDI_HEAP_LOCK( hbitmap );

    bmpObjPtr->size.cx = 0;
    bmpObjPtr->size.cy = 0;
    bmpObjPtr->bitmap.bmType = 0;
    bmpObjPtr->bitmap.bmWidth = (INT16)width;
    bmpObjPtr->bitmap.bmHeight = (INT16)height;
    bmpObjPtr->bitmap.bmPlanes = (BYTE)planes;
    bmpObjPtr->bitmap.bmBitsPixel = (BYTE)bpp;
    bmpObjPtr->bitmap.bmWidthBytes = (INT16)BITMAP_WIDTH_BYTES( width, bpp );
    bmpObjPtr->bitmap.bmBits = NULL;

    bmpObjPtr->dibSection = NULL;
    bmpObjPtr->colorMap = NULL;
    bmpObjPtr->nColorMap = 0;
    bmpObjPtr->status = DIB_NoHandler;

      /* Create the pixmap */
    bmpObjPtr->pixmap = TSXCreatePixmap(display, rootWindow, width, height, bpp);
    if (!bmpObjPtr->pixmap)
    {
	GDI_HEAP_FREE( hbitmap );
	hbitmap = 0;
    }
    else if (bits) /* Set bitmap bits */
	SetBitmapBits32( hbitmap, height * bmpObjPtr->bitmap.bmWidthBytes,
                         bits );
    GDI_HEAP_UNLOCK( hbitmap );
    return hbitmap;
}


/***********************************************************************
 *           CreateCompatibleBitmap16    (GDI.51)
 */
HBITMAP16 WINAPI CreateCompatibleBitmap16(HDC16 hdc, INT16 width, INT16 height)
{
    return CreateCompatibleBitmap32( hdc, width, height );
}


/******************************************************************************
 * CreateCompatibleBitmap32 [GDI32.30]  Creates a bitmap compatible with the DC
 *
 * PARAMS
 *    hdc    [I] Handle to device context
 *    width  [I] Width of bitmap
 *    height [I] Height of bitmap
 *
 * RETURNS
 *    Success: Handle to bitmap
 *    Failure: NULL
 */
HBITMAP32 WINAPI CreateCompatibleBitmap32( HDC32 hdc, INT32 width, INT32 height)
{
    HBITMAP32 hbmpRet = 0;
    DC *dc;

    TRACE(gdi, "(%04x,%d,%d) = \n", hdc, width, height );
    if (!(dc = DC_GetDCPtr( hdc ))) return 0;
    if ((width >0x1000) || (height > 0x1000))
      {
	FIXME(gdi,"got bad width %d or height %d, please look for reason\n",
	       width, height );
	return 0;
      }
    hbmpRet = CreateBitmap32( width, height, 1, dc->w.bitsPerPixel, NULL );
    TRACE(gdi,"\t\t%04x\n", hbmpRet);
    return hbmpRet;
}


/***********************************************************************
 *           CreateBitmapIndirect16    (GDI.49)
 */
HBITMAP16 WINAPI CreateBitmapIndirect16( const BITMAP16 * bmp )
{
    return CreateBitmap16( bmp->bmWidth, bmp->bmHeight, bmp->bmPlanes,
                           bmp->bmBitsPixel, PTR_SEG_TO_LIN( bmp->bmBits ) );
}


/******************************************************************************
 * CreateBitmapIndirect32 [GDI32.26]  Creates a bitmap with the specifies info
 *
 * RETURNS
 *    Success: Handle to bitmap
 *    Failure: NULL
 */
HBITMAP32 WINAPI CreateBitmapIndirect32(
    const BITMAP32 * bmp) /* [in] Pointer to the bitmap data */
{
    return CreateBitmap32( bmp->bmWidth, bmp->bmHeight, bmp->bmPlanes,
                           bmp->bmBitsPixel, bmp->bmBits );
}


/***********************************************************************
 *           BITMAP_GetXImage
 *
 * Get an X image for a bitmap. For use with CALL_LARGE_STACK.
 */
XImage *BITMAP_GetXImage( const BITMAPOBJ *bmp )
{
    return XGetImage( display, bmp->pixmap, 0, 0, bmp->bitmap.bmWidth,
                      bmp->bitmap.bmHeight, AllPlanes, ZPixmap );
}


/***********************************************************************
 *           GetBitmapBits16    (GDI.74)
 */
LONG WINAPI GetBitmapBits16( HBITMAP16 hbitmap, LONG count, LPVOID buffer )
{
    return GetBitmapBits32( hbitmap, count, buffer );
}


/***********************************************************************
 * GetBitmapBits32 [GDI32.143]  Copies bitmap bits of bitmap to buffer
 * 
 * RETURNS
 *    Success: Number of bytes copied
 *    Failure: 0
 */
LONG WINAPI GetBitmapBits32(
    HBITMAP32 hbitmap, /* [in]  Handle to bitmap */
    LONG count,        /* [in]  Number of bytes to copy */
    LPVOID buffer)     /* [out] Pointer to buffer to receive bits */
{
    BITMAPOBJ * bmp;
    LONG height, old_height;
    XImage * image;
    LPBYTE tbuf;
    int	h,w,pad;
    
    /* KLUDGE! */
    if (count < 0) {
	WARN(bitmap, "(%ld): Negative number of bytes passed???\n", count );
	count = -count;
    }
    bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    if (!bmp) return 0;

      /* Only get entire lines */
    height = count / bmp->bitmap.bmWidthBytes;
    if (height > bmp->bitmap.bmHeight) height = bmp->bitmap.bmHeight;

    TRACE(bitmap, "%dx%d %d colors %p fetched height: %ld\n",
	    bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
	    1 << bmp->bitmap.bmBitsPixel, buffer, height );

    pad = BITMAP_GetBitsPadding( bmp->bitmap.bmWidth, bmp->bitmap.bmBitsPixel );

    if (!height || (pad == -1))
    {
      GDI_HEAP_UNLOCK( hbitmap );
      return 0;
    }

    EnterCriticalSection( &X11DRV_CritSection );

    /* Hack: change the bitmap height temporarily to avoid */
    /*       getting unnecessary bitmap rows. */
    old_height = bmp->bitmap.bmHeight;
    bmp->bitmap.bmHeight = height;
    image = (XImage *)CALL_LARGE_STACK( BITMAP_GetXImage, bmp );
    bmp->bitmap.bmHeight = old_height;

    /* copy XImage to 16 bit padded image buffer with real bitsperpixel */

    tbuf = buffer;
    switch (bmp->bitmap.bmBitsPixel)
    {
    case 1:
        for (h=0;h<height;h++)
        {
            *tbuf = 0;
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
                if ((w%8) == 0)
                    *tbuf = 0;
                *tbuf |= XGetPixel(image,w,h)<<(7-(w&7));
                if ((w&7) == 7) ++tbuf;
            }
            tbuf += pad;
        }
        break;
    case 4:
        for (h=0;h<height;h++)
        {
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
                if (!(w & 1)) *tbuf = XGetPixel( image, w, h) << 4;
	    	else *tbuf++ |= XGetPixel( image, w, h) & 0x0f;
            }
            tbuf += pad;
        }
        break;
    case 8:
        for (h=0;h<height;h++)
        {
            for (w=0;w<bmp->bitmap.bmWidth;w++)
                *tbuf++ = XGetPixel(image,w,h);
            tbuf += pad;
        }
        break;
    case 15:
    case 16:
        for (h=0;h<height;h++)
        {
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
	    	long pixel = XGetPixel(image,w,h);

		*tbuf++ = pixel & 0xff;
		*tbuf++ = (pixel>>8) & 0xff;
            }
        }
        break;
    case 24:
        for (h=0;h<height;h++)
        {
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
	    	long pixel = XGetPixel(image,w,h);

		*tbuf++ = pixel & 0xff;
		*tbuf++ = (pixel>> 8) & 0xff;
		*tbuf++ = (pixel>>16) & 0xff;
	    }
            tbuf += pad;
	}
        break;
    default:
        FIXME(bitmap, "Unhandled bits:%d\n", bmp->bitmap.bmBitsPixel);
    }
    XDestroyImage( image );
    LeaveCriticalSection( &X11DRV_CritSection );

    GDI_HEAP_UNLOCK( hbitmap );
    return height * bmp->bitmap.bmWidthBytes;
}


/***********************************************************************
 *           SetBitmapBits16    (GDI.106)
 */
LONG WINAPI SetBitmapBits16( HBITMAP16 hbitmap, LONG count, LPCVOID buffer )
{
    return SetBitmapBits32( hbitmap, count, buffer );
}


/******************************************************************************
 * SetBitmapBits32 [GDI32.303]  Sets bits of color data for a bitmap
 *
 * RETURNS
 *    Success: Number of bytes used in setting the bitmap bits
 *    Failure: 0
 */
LONG WINAPI SetBitmapBits32(
    HBITMAP32 hbitmap, /* [in] Handle to bitmap */
    LONG count,        /* [in] Number of bytes in bitmap array */
    LPCVOID buffer)    /* [in] Address of array with bitmap bits */
{
    struct XPutImage_descr descr;
    BITMAPOBJ * bmp;
    LONG height;
    XImage * image;
    LPBYTE sbuf,tmpbuffer;
    int	w,h,pad,widthbytes;
    
    /* KLUDGE! */
    if (count < 0) {
	WARN(bitmap, "(%ld): Negative number of bytes passed???\n", count );
	count = -count;
    }
    bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    if (!bmp) return 0;

    TRACE(bitmap, "%dx%d %d colors %p\n",
	    bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
	    1 << bmp->bitmap.bmBitsPixel, buffer );

      /* Only set entire lines */
    height = count / bmp->bitmap.bmWidthBytes;
    if (height > bmp->bitmap.bmHeight) height = bmp->bitmap.bmHeight;

    pad = BITMAP_GetBitsPadding( bmp->bitmap.bmWidth, bmp->bitmap.bmBitsPixel );

    if (!height || (pad == -1)) 
    {
      GDI_HEAP_UNLOCK( hbitmap );
      return 0;
    }
	
    sbuf = (LPBYTE)buffer;

    widthbytes	= DIB_GetXImageWidthBytes(bmp->bitmap.bmWidth,bmp->bitmap.bmBitsPixel);
    tmpbuffer	= (LPBYTE)xmalloc(widthbytes*height);

    EnterCriticalSection( &X11DRV_CritSection );
    image = XCreateImage( display, DefaultVisualOfScreen(screen),
                          bmp->bitmap.bmBitsPixel, ZPixmap, 0, tmpbuffer,
                          bmp->bitmap.bmWidth,height,32,widthbytes );
    
    /* copy 16 bit padded image buffer with real bitsperpixel to XImage */
    sbuf = (LPBYTE)buffer;
    switch (bmp->bitmap.bmBitsPixel)
    {
    case 1:
        for (h=0;h<height;h++)
        {
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
                XPutPixel(image,w,h,(sbuf[0]>>(7-(w&7))) & 1);
                if ((w&7) == 7)
                    sbuf++;
            }
            sbuf += pad;
        }
        break;
    case 4:
        for (h=0;h<height;h++)
        {
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
                if (!(w & 1)) XPutPixel( image, w, h, *sbuf >> 4 );
                else XPutPixel( image, w, h, *sbuf++ & 0xf );
            }
            sbuf += pad;
        }
        break;
    case 8:
        for (h=0;h<height;h++)
        {
            for (w=0;w<bmp->bitmap.bmWidth;w++)
                XPutPixel(image,w,h,*sbuf++);
            sbuf += pad;
        }
        break;
    case 15:
    case 16:
        for (h=0;h<height;h++)
        {
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
                XPutPixel(image,w,h,sbuf[1]*256+sbuf[0]);
                sbuf+=2;
            }
        }
        break;
    case 24: 
        for (h=0;h<height;h++)
        {
            for (w=0;w<bmp->bitmap.bmWidth;w++)
            {
                XPutPixel(image,w,h,(sbuf[2]<<16)+(sbuf[1]<<8)+sbuf[0]);
                sbuf += 3;
            }
            sbuf += pad;
        }
        break;
    }

    descr.bmp    = bmp;
    descr.image  = image;
    descr.width  = bmp->bitmap.bmWidth;
    descr.height = height;
    CALL_LARGE_STACK( XPutImage_wrapper, &descr );
    XDestroyImage( image ); /* frees tmpbuffer too */
    LeaveCriticalSection( &X11DRV_CritSection );
    
    GDI_HEAP_UNLOCK( hbitmap );
    return height * bmp->bitmap.bmWidthBytes;
}

/***********************************************************************
 * LoadImage16 [USER.389]
 *
 */
HANDLE16 WINAPI LoadImage16( HINSTANCE16 hinst, LPCSTR name, UINT16 type,
                             INT16 desiredx, INT16 desiredy, UINT16 loadflags)
{
	if (HIWORD(name)) {
	    TRACE(resource,"(0x%04x,%s,%d,%d,%d,0x%08x)\n",
                hinst,(char *)PTR_SEG_TO_LIN(name),type,desiredx,desiredy,loadflags);
	} else {
	    TRACE(resource,"LoadImage16(0x%04x,%p,%d,%d,%d,0x%08x)\n",
                hinst,name,type,desiredx,desiredy,loadflags);
	}
	switch (type) {
	case IMAGE_BITMAP:
		return LoadBitmap16(hinst,(SEGPTR)name);
	case IMAGE_ICON:
		return LoadIcon16(hinst,(SEGPTR)name);
	case IMAGE_CURSOR:
		return LoadCursor16(hinst,(SEGPTR)name);
	}
	return 0;
	
}

/**********************************************************************
 *	    LoadImage32A    (USER32.365)
 * 
 * FIXME: implementation still lacks nearly all features, see LR_*
 * defines in windows.h
 */

HANDLE32 WINAPI LoadImage32A( HINSTANCE32 hinst, LPCSTR name, UINT32 type,
                              INT32 desiredx, INT32 desiredy, UINT32 loadflags)
{
	if (HIWORD(name)) {
		TRACE(resource,"(0x%04x,%s,%d,%d,%d,0x%08x)\n",
			hinst,name,type,desiredx,desiredy,loadflags
		);
	} else {
		TRACE(resource,"(0x%04x,%p,%d,%d,%d,0x%08x)\n",
			hinst,name,type,desiredx,desiredy,loadflags
		);
	}
	switch (type) {
	case IMAGE_BITMAP:
		return LoadBitmap32A(hinst,name);
	case IMAGE_ICON:
		return LoadIcon32A(hinst,name);
	case IMAGE_CURSOR:
		return LoadCursor32A(hinst,name);
	}
	return 0;
}

/**********************************************************************
 *	    LoadImage32W    (USER32.366)
 * 
 * FIXME: implementation still lacks nearly all features, see LR_*
 * defines in windows.h
 */


/******************************************************************************
 * LoadImage32W [USER32.366]  Loads an icon, cursor, or bitmap
 *
 * PARAMS
 *    hinst     [I] Handle of instance that contains image
 *    name      [I] Name of image
 *    type      [I] Type of image
 *    desiredx  [I] Desired width
 *    desiredy  [I] Desired height
 *    loadflags [I] Load flags
 *
 * RETURNS
 *    Success: Handle to newly loaded image
 *    Failure: NULL
 *
 * BUGS
 *    Implementation still lacks nearly all features, see LR_*
 *    defines in windows.h
 */
HANDLE32 WINAPI LoadImage32W( HINSTANCE32 hinst, LPCWSTR name, UINT32 type,
                INT32 desiredx, INT32 desiredy, UINT32 loadflags )
{
	if (HIWORD(name)) {
		TRACE(resource,"(0x%04x,%p,%d,%d,%d,0x%08x)\n",
			hinst,name,type,desiredx,desiredy,loadflags
		);
	} else {
		TRACE(resource,"(0x%04x,%p,%d,%d,%d,0x%08x)\n",
			hinst,name,type,desiredx,desiredy,loadflags
		);
	}

    switch (type) {
        case IMAGE_BITMAP:
            return LoadBitmap32W(hinst,name);
        case IMAGE_ICON:
            return LoadIcon32W(hinst,name);
        case IMAGE_CURSOR:
            return LoadCursor32W(hinst,name);
    }
    return NULL;
}


/**********************************************************************
 *          CopyBitmap32 (not an API)
 *
 * NOTES
 *    If it is not an API, why is it declared with WINAPI?
 *
 */
HBITMAP32 WINAPI CopyBitmap32 (HBITMAP32 hnd)
{
    HBITMAP32 res = 0;
    BITMAP32 bmp;

    if (GetObject32A (hnd, sizeof (bmp), &bmp))
    {
	res = CreateBitmapIndirect32 (&bmp);
	SetBitmapBits32 (res, bmp.bmWidthBytes * bmp.bmHeight, bmp.bmBits);
    }
    return res;
}


/******************************************************************************
 * CopyImage32 [USER32.61]  Creates new image and copies attributes to it
 *
 * PARAMS
 *    hnd      [I] Handle to image to copy
 *    type     [I] Type of image to copy
 *    desiredx [I] Desired width of new image
 *    desiredy [I] Desired height of new image
 *    flags    [I] Copy flags
 *
 * RETURNS
 *    Success: Handle to newly created image
 *    Failure: NULL
 *
 * FIXME: implementation still lacks nearly all features, see LR_*
 * defines in windows.h
 */
HANDLE32 WINAPI CopyImage32( HANDLE32 hnd, UINT32 type, INT32 desiredx,
                             INT32 desiredy, UINT32 flags )
{
    switch (type)
    {
	case IMAGE_BITMAP:
		return CopyBitmap32(hnd);
	case IMAGE_ICON:
		return CopyIcon32(hnd);
	case IMAGE_CURSOR:
		return CopyCursor32(hnd);
    }
    return 0;
}


/**********************************************************************
 *	    LoadBitmap16    (USER.175)
 *
 * NOTES
 *    Can this call LoadBitmap32?
 */
HBITMAP16 WINAPI LoadBitmap16( HINSTANCE16 instance, SEGPTR name )
{
    HBITMAP32 hbitmap = 0;
    HDC32 hdc;
    HRSRC16 hRsrc;
    HGLOBAL16 handle;
    BITMAPINFO *info;

    if (HIWORD(name))
    {
        char *str = (char *)PTR_SEG_TO_LIN( name );
        TRACE(bitmap, "(%04x,'%s')\n", instance, str );
        if (str[0] == '#') name = (SEGPTR)(DWORD)(WORD)atoi( str + 1 );
    }
    else
        TRACE(bitmap, "(%04x,%04x)\n",
                        instance, LOWORD(name) );

    if (!instance)  /* OEM bitmap */
    {
        if (HIWORD((int)name)) return 0;
        return OBM_LoadBitmap( LOWORD((int)name) );
    }

    if (!(hRsrc = FindResource16( instance, name, RT_BITMAP16 ))) return 0;
    if (!(handle = LoadResource16( instance, hRsrc ))) return 0;

    info = (BITMAPINFO *)LockResource16( handle );
    if ((hdc = GetDC32(0)) != 0)
    {
        char *bits = (char *)info + DIB_BitmapInfoSize( info, DIB_RGB_COLORS );
        hbitmap = CreateDIBitmap32( hdc, &info->bmiHeader, CBM_INIT,
                                    bits, info, DIB_RGB_COLORS );
        ReleaseDC32( 0, hdc );
    }
    FreeResource16( handle );
    return hbitmap;
}


/******************************************************************************
 * LoadBitmap32W [USER32.358]  Loads bitmap from the executable file
 *
 * RETURNS
 *    Success: Handle to specified bitmap
 *    Failure: NULL
 */
HBITMAP32 WINAPI LoadBitmap32W(
    HINSTANCE32 instance, /* [in] Handle to application instance */
    LPCWSTR name)         /* [in] Address of bitmap resource name */
{
    HBITMAP32 hbitmap = 0;
    HDC32 hdc;
    HRSRC32 hRsrc;
    HGLOBAL32 handle;
    BITMAPINFO *info;

    if (!instance)  /* OEM bitmap */
    {
        if (HIWORD((int)name)) return 0;
        return OBM_LoadBitmap( LOWORD((int)name) );
    }

    if (!(hRsrc = FindResource32W( instance, name, RT_BITMAP32W ))) return 0;
    if (!(handle = LoadResource32( instance, hRsrc ))) return 0;

    info = (BITMAPINFO *)LockResource32( handle );
    if ((hdc = GetDC32(0)) != 0)
    {
        char *bits = (char *)info + DIB_BitmapInfoSize( info, DIB_RGB_COLORS );
        hbitmap = CreateDIBitmap32( hdc, &info->bmiHeader, CBM_INIT,
                                    bits, info, DIB_RGB_COLORS );
        ReleaseDC32( 0, hdc );
    }
    return hbitmap;
}


/**********************************************************************
 *	    LoadBitmap32A   (USER32.357)
 */
HBITMAP32 WINAPI LoadBitmap32A( HINSTANCE32 instance, LPCSTR name )
{
    HBITMAP32 res;
    if (!HIWORD(name)) res = LoadBitmap32W( instance, (LPWSTR)name );
    else
    {
        LPWSTR uni = HEAP_strdupAtoW( GetProcessHeap(), 0, name );
        res = LoadBitmap32W( instance, uni );
        HeapFree( GetProcessHeap(), 0, uni );
    }
    return res;
}


/***********************************************************************
 *           BITMAP_DeleteObject
 */
BOOL32 BITMAP_DeleteObject( HBITMAP16 hbitmap, BITMAPOBJ * bmp )
{
#ifdef PRELIMINARY_WING16_SUPPORT
    if( bmp->bitmap.bmBits )
 	TSXShmDetach( display, (XShmSegmentInfo*)bmp->bitmap.bmBits );
#endif

    TSXFreePixmap( display, bmp->pixmap );
#ifdef PRELIMINARY_WING16_SUPPORT
    if( bmp->bitmap.bmBits )
    {
    __ShmBitmapCtl* p = (__ShmBitmapCtl*)bmp->bitmap.bmBits;
      WORD          sel = HIWORD(p->bits);
      unsigned long l, limit = GetSelectorLimit(sel);

      for( l = 0; l < limit; l += 0x10000, sel += __AHINCR )
	   FreeSelector(sel);
      shmctl(p->si.shmid, IPC_RMID, NULL); 
      shmdt(p->si.shmaddr);  /* already marked for destruction */
    }
#endif

    if (bmp->dibSection)
    {
	DIBSECTION *dib = bmp->dibSection;

	if (dib->dsBm.bmBits)
            if (dib->dshSection)
                UnmapViewOfFile(dib->dsBm.bmBits);
            else
                VirtualFree(dib->dsBm.bmBits, MEM_RELEASE, 0L);

	HeapFree(GetProcessHeap(), 0, dib);
    }
    if (bmp->colorMap)
	HeapFree(GetProcessHeap(), 0, bmp->colorMap);

    return GDI_FreeObject( hbitmap );
}

	
/***********************************************************************
 *           BITMAP_GetObject16
 */
INT16 BITMAP_GetObject16( BITMAPOBJ * bmp, INT16 count, LPVOID buffer )
{
    if (bmp->dibSection)
    {
	FIXME(bitmap, "not implemented for DIBs\n");
	return 0;
    }
    else
    {
	if (count > sizeof(bmp->bitmap)) count = sizeof(bmp->bitmap);
	memcpy( buffer, &bmp->bitmap, count );
	return count;
    }
}
    

/***********************************************************************
 *           BITMAP_GetObject32
 */
INT32 BITMAP_GetObject32( BITMAPOBJ * bmp, INT32 count, LPVOID buffer )
{
    if (bmp->dibSection)
    {
	if (count < sizeof(DIBSECTION))
	{
	    if (count > sizeof(BITMAP32)) count = sizeof(BITMAP32);
	}
	else
	{
	    if (count > sizeof(DIBSECTION)) count = sizeof(DIBSECTION);
	}

	memcpy( buffer, bmp->dibSection, count );
	return count;
    }
    else
    {
	BITMAP32 bmp32;
	bmp32.bmType       = bmp->bitmap.bmType;
	bmp32.bmWidth      = bmp->bitmap.bmWidth;
	bmp32.bmHeight     = bmp->bitmap.bmHeight;
	bmp32.bmWidthBytes = bmp->bitmap.bmWidthBytes;
	bmp32.bmPlanes     = bmp->bitmap.bmPlanes;
	bmp32.bmBitsPixel  = bmp->bitmap.bmBitsPixel;
	bmp32.bmBits       = NULL;
	if (count > sizeof(bmp32)) count = sizeof(bmp32);
	memcpy( buffer, &bmp32, count );
	return count;
    }
}
    

/***********************************************************************
 *           CreateDiscardableBitmap16    (GDI.156)
 */
HBITMAP16 WINAPI CreateDiscardableBitmap16( HDC16 hdc, INT16 width,
                                            INT16 height )
{
    return CreateCompatibleBitmap16( hdc, width, height );
}


/******************************************************************************
 * CreateDiscardableBitmap32 [GDI32.38]  Creates a discardable bitmap
 *
 * RETURNS
 *    Success: Handle to bitmap
 *    Failure: NULL
 */
HBITMAP32 WINAPI CreateDiscardableBitmap32(
    HDC32 hdc,    /* [in] Handle to device context */
    INT32 width,  /* [in] Bitmap width */
    INT32 height) /* [in] Bitmap height */
{
    return CreateCompatibleBitmap32( hdc, width, height );
}


/***********************************************************************
 *           GetBitmapDimensionEx16    (GDI.468)
 *
 * NOTES
 *    Can this call GetBitmapDimensionEx32?
 */
BOOL16 WINAPI GetBitmapDimensionEx16( HBITMAP16 hbitmap, LPSIZE16 size )
{
    BITMAPOBJ * bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    if (!bmp) return FALSE;
    *size = bmp->size;
    GDI_HEAP_UNLOCK( hbitmap );
    return TRUE;
}


/******************************************************************************
 * GetBitmapDimensionEx32 [GDI32.144]  Retrieves dimensions of a bitmap
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL32 WINAPI GetBitmapDimensionEx32(
    HBITMAP32 hbitmap, /* [in]  Handle to bitmap */
    LPSIZE32 size)     /* [out] Address of struct receiving dimensions */
{
    BITMAPOBJ * bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    if (!bmp) return FALSE;
    size->cx = (INT32)bmp->size.cx;
    size->cy = (INT32)bmp->size.cy;
    GDI_HEAP_UNLOCK( hbitmap );
    return TRUE;
}


/***********************************************************************
 *           GetBitmapDimension    (GDI.162)
 */
DWORD WINAPI GetBitmapDimension( HBITMAP16 hbitmap )
{
    SIZE16 size;
    if (!GetBitmapDimensionEx16( hbitmap, &size )) return 0;
    return MAKELONG( size.cx, size.cy );
}


/***********************************************************************
 *           SetBitmapDimensionEx16    (GDI.478)
 */
BOOL16 WINAPI SetBitmapDimensionEx16( HBITMAP16 hbitmap, INT16 x, INT16 y,
                                      LPSIZE16 prevSize )
{
    BITMAPOBJ * bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    if (!bmp) return FALSE;
    if (prevSize) *prevSize = bmp->size;
    bmp->size.cx = x;
    bmp->size.cy = y;
    GDI_HEAP_UNLOCK( hbitmap );
    return TRUE;
}


/******************************************************************************
 * SetBitmapDimensionEx32 [GDI32.304]  Assignes dimensions to a bitmap
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL32 WINAPI SetBitmapDimensionEx32(
    HBITMAP32 hbitmap, /* [in]  Handle to bitmap */
    INT32 x,           /* [in]  Bitmap width */
    INT32 y,           /* [in]  Bitmap height */
    LPSIZE32 prevSize) /* [out] Address of structure for orig dims */
{
    BITMAPOBJ * bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    if (!bmp) return FALSE;
    if (prevSize) CONV_SIZE16TO32( &bmp->size, prevSize );
    bmp->size.cx = (INT16)x;
    bmp->size.cy = (INT16)y;
    GDI_HEAP_UNLOCK( hbitmap );
    return TRUE;
}


/***********************************************************************
 *           SetBitmapDimension    (GDI.163)
 */
DWORD WINAPI SetBitmapDimension( HBITMAP16 hbitmap, INT16 x, INT16 y )
{
    SIZE16 size;
    if (!SetBitmapDimensionEx16( hbitmap, x, y, &size )) return 0;
    return MAKELONG( size.cx, size.cy );
}

