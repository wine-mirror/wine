/*
 * GDI bitmap objects
 *
 * Copyright 1993 Alexandre Julliard
 *           1998 Huw D M Davies
 */

#include <stdlib.h>
#include <string.h>

#include "wine/winbase16.h"
#include "gdi.h"
#include "dc.h"
#include "bitmap.h"
#include "heap.h"
#include "global.h"
#include "sysmetrics.h"
#include "cursoricon.h"
#include "debugtools.h"
#include "monitor.h"
#include "wine/winuser16.h"

DECLARE_DEBUG_CHANNEL(bitmap)
DECLARE_DEBUG_CHANNEL(resource)

BITMAP_DRIVER *BITMAP_Driver = NULL;


/***********************************************************************
 *           BITMAP_GetWidthBytes
 *
 * Return number of bytes taken by a scanline of 16-bit aligned Windows DDB
 * data.
 */
INT BITMAP_GetWidthBytes( INT bmWidth, INT bpp )
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
	WARN_(bitmap)("Unknown depth %d, please report.\n", bpp );
    }
    return -1;
}

/***********************************************************************
 *           CreateUserBitmap16    (GDI.407)
 */
HBITMAP16 WINAPI CreateUserBitmap16( INT16 width, INT16 height, UINT16 planes,
                                     UINT16 bpp, LPCVOID bits )
{
    return CreateBitmap16( width, height, planes, bpp, bits );
}

/***********************************************************************
 *           CreateUserDiscardableBitmap16    (GDI.409)
 */
HBITMAP16 WINAPI CreateUserDiscardableBitmap16( WORD dummy, 
                                                INT16 width, INT16 height )
{
    return CreateUserBitmap16( width, height, 1, MONITOR_GetDepth(&MONITOR_PrimaryMonitor), NULL );
}


/***********************************************************************
 *           CreateBitmap16    (GDI.48)
 */
HBITMAP16 WINAPI CreateBitmap16( INT16 width, INT16 height, UINT16 planes,
                                 UINT16 bpp, LPCVOID bits )
{
    return CreateBitmap( width, height, planes, bpp, bits );
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
    if (!height || !width) return 0;
    if (planes != 1) {
        FIXME_(bitmap)("planes = %d\n", planes);
	return 0;
    }
    if (height < 0) height = -height;
    if (width < 0) width = -width;

      /* Create the BITMAPOBJ */
    hbitmap = GDI_AllocObject( sizeof(BITMAPOBJ), BITMAP_MAGIC );
    if (!hbitmap) return 0;

    TRACE_(bitmap)("%dx%d, %d colors returning %08x\n", width, height,
	  1 << (planes*bpp), hbitmap);

    bmp = (BITMAPOBJ *) GDI_HEAP_LOCK( hbitmap );

    bmp->size.cx = 0;
    bmp->size.cy = 0;
    bmp->bitmap.bmType = 0;
    bmp->bitmap.bmWidth = width;
    bmp->bitmap.bmHeight = height;
    bmp->bitmap.bmPlanes = planes;
    bmp->bitmap.bmBitsPixel = bpp;
    bmp->bitmap.bmWidthBytes = BITMAP_GetWidthBytes( width, bpp );
    bmp->bitmap.bmBits = NULL;

    bmp->DDBitmap = NULL;
    bmp->dib = NULL;

    if (bits) /* Set bitmap bits */
	SetBitmapBits( hbitmap, height * bmp->bitmap.bmWidthBytes,
                         bits );
    GDI_HEAP_UNLOCK( hbitmap );
    return hbitmap;
}


/***********************************************************************
 *           CreateCompatibleBitmap16    (GDI.51)
 */
HBITMAP16 WINAPI CreateCompatibleBitmap16(HDC16 hdc, INT16 width, INT16 height)
{
    return CreateCompatibleBitmap( hdc, width, height );
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
 *    Failure: 0
 */
HBITMAP WINAPI CreateCompatibleBitmap( HDC hdc, INT width, INT height)
{
    HBITMAP hbmpRet = 0;
    DC *dc;

    TRACE_(bitmap)("(%04x,%d,%d) = \n", hdc, width, height );
    if (!(dc = DC_GetDCPtr( hdc ))) return 0;
    if ((width >= 0x10000) || (height >= 0x10000)) {
	FIXME_(bitmap)("got bad width %d or height %d, please look for reason\n",
	      width, height );
    } else {
        hbmpRet = CreateBitmap( width, height, 1, dc->w.bitsPerPixel, NULL );
	if(dc->funcs->pCreateBitmap)
	    dc->funcs->pCreateBitmap( hbmpRet );
    }
    TRACE_(bitmap)("\t\t%04x\n", hbmpRet);
    GDI_HEAP_UNLOCK(hdc);
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
HBITMAP WINAPI CreateBitmapIndirect(
    const BITMAP * bmp) /* [in] Pointer to the bitmap data */
{
    return CreateBitmap( bmp->bmWidth, bmp->bmHeight, bmp->bmPlanes,
                           bmp->bmBitsPixel, bmp->bmBits );
}


/***********************************************************************
 *           GetBitmapBits16    (GDI.74)
 */
LONG WINAPI GetBitmapBits16( HBITMAP16 hbitmap, LONG count, LPVOID buffer )
{
    return GetBitmapBits( hbitmap, count, buffer );
}


/***********************************************************************
 * GetBitmapBits32 [GDI32.143]  Copies bitmap bits of bitmap to buffer
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
	return bmp->bitmap.bmWidthBytes * bmp->bitmap.bmHeight;

    if (count < 0) {
	WARN_(bitmap)("(%ld): Negative number of bytes passed???\n", count );
	count = -count;
    }

    /* Only get entire lines */
    height = count / bmp->bitmap.bmWidthBytes;
    if (height > bmp->bitmap.bmHeight) height = bmp->bitmap.bmHeight;
    count = height * bmp->bitmap.bmWidthBytes;
    if (count == 0)
      {
	WARN_(bitmap)("Less then one entire line requested\n");
	GDI_HEAP_UNLOCK( hbitmap );
	return 0;
      }


    TRACE_(bitmap)("(%08x, %ld, %p) %dx%d %d colors fetched height: %ld\n",
	    hbitmap, count, bits, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
	    1 << bmp->bitmap.bmBitsPixel, height );

    if(bmp->DDBitmap) { 

        TRACE_(bitmap)("Calling device specific BitmapBits\n");
	if(bmp->DDBitmap->funcs->pBitmapBits)
	    ret = bmp->DDBitmap->funcs->pBitmapBits(hbitmap, bits, count,
						    DDB_GET);
	else {
	    ERR_(bitmap)("BitmapBits == NULL??\n");
	    ret = 0;
	}	

    } else {

        if(!bmp->bitmap.bmBits) {
	    WARN_(bitmap)("Bitmap is empty\n");
	    ret = 0;
	} else {
	    memcpy(bits, bmp->bitmap.bmBits, count);
	    ret = count;
	}

    }

    GDI_HEAP_UNLOCK( hbitmap );
    return ret;
}


/***********************************************************************
 *           SetBitmapBits16    (GDI.106)
 */
LONG WINAPI SetBitmapBits16( HBITMAP16 hbitmap, LONG count, LPCVOID buffer )
{
    return SetBitmapBits( hbitmap, count, buffer );
}


/******************************************************************************
 * SetBitmapBits32 [GDI32.303]  Sets bits of color data for a bitmap
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
	WARN_(bitmap)("(%ld): Negative number of bytes passed???\n", count );
	count = -count;
    }

    /* Only get entire lines */
    height = count / bmp->bitmap.bmWidthBytes;
    if (height > bmp->bitmap.bmHeight) height = bmp->bitmap.bmHeight;
    count = height * bmp->bitmap.bmWidthBytes;

    TRACE_(bitmap)("(%08x, %ld, %p) %dx%d %d colors fetched height: %ld\n",
	    hbitmap, count, bits, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
	    1 << bmp->bitmap.bmBitsPixel, height );

    if(bmp->DDBitmap) {

        TRACE_(bitmap)("Calling device specific BitmapBits\n");
	if(bmp->DDBitmap->funcs->pBitmapBits)
	    ret = bmp->DDBitmap->funcs->pBitmapBits(hbitmap, (void *) bits,
						    count, DDB_SET);
	else {
	    ERR_(bitmap)("BitmapBits == NULL??\n");
	    ret = 0;
	}
	
    } else {

        if(!bmp->bitmap.bmBits) /* Alloc enough for entire bitmap */
	    bmp->bitmap.bmBits = HeapAlloc( GetProcessHeap(), 0, count );
	if(!bmp->bitmap.bmBits) {
	    WARN_(bitmap)("Unable to allocate bit buffer\n");
	    ret = 0;
	} else {
	    memcpy(bmp->bitmap.bmBits, bits, count);
	    ret = count;
	}
    }

    GDI_HEAP_UNLOCK( hbitmap );
    return ret;
}

/***********************************************************************
 * LoadImage16 [USER.389]
 *
 */
HANDLE16 WINAPI LoadImage16( HINSTANCE16 hinst, LPCSTR name, UINT16 type,
                             INT16 desiredx, INT16 desiredy, UINT16 loadflags)
{
    LPCSTR nameStr = HIWORD(name)? PTR_SEG_TO_LIN(name) : (LPCSTR)name;
    return LoadImageA( hinst, nameStr, type, 
                       desiredx, desiredy, loadflags );
}

/**********************************************************************
 *	    LoadImageA    (USER32.365)
 * 
 * FIXME: implementation lacks some features, see LR_ defines in windows.h
 */

HANDLE WINAPI LoadImageA( HINSTANCE hinst, LPCSTR name, UINT type,
                              INT desiredx, INT desiredy, UINT loadflags)
{
    HANDLE res;
    LPWSTR u_name;

    if (HIWORD(name)) u_name = HEAP_strdupAtoW(GetProcessHeap(), 0, name);
    else u_name=(LPWSTR)name;
    res = LoadImageW(hinst, u_name, type, desiredx, desiredy, loadflags);
    if (HIWORD(name)) HeapFree(GetProcessHeap(), 0, u_name);
    return res;
}


/******************************************************************************
 * LoadImageW [USER32.366]  Loads an icon, cursor, or bitmap
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
 * FIXME: Implementation lacks some features, see LR_ defines in windows.h
 */
HANDLE WINAPI LoadImageW( HINSTANCE hinst, LPCWSTR name, UINT type,
                INT desiredx, INT desiredy, UINT loadflags )
{
    if (HIWORD(name)) {
        TRACE_(resource)("(0x%04x,%p,%d,%d,%d,0x%08x)\n",
	      hinst,name,type,desiredx,desiredy,loadflags);
    } else {
        TRACE_(resource)("(0x%04x,%p,%d,%d,%d,0x%08x)\n",
	      hinst,name,type,desiredx,desiredy,loadflags);
    }
    if (loadflags & LR_DEFAULTSIZE) {
        if (type == IMAGE_ICON) {
	    if (!desiredx) desiredx = SYSMETRICS_CXICON;
	    if (!desiredy) desiredy = SYSMETRICS_CYICON;
	} else if (type == IMAGE_CURSOR) {
            if (!desiredx) desiredx = SYSMETRICS_CXCURSOR;
	    if (!desiredy) desiredy = SYSMETRICS_CYCURSOR;
	}
    }
    if (loadflags & LR_LOADFROMFILE) loadflags &= ~LR_SHARED;
    switch (type) {
    case IMAGE_BITMAP:
        return BITMAP_Load( hinst, name, loadflags );

    case IMAGE_ICON:
        {
	HDC hdc = GetDC(0);
	UINT palEnts = GetSystemPaletteEntries(hdc, 0, 0, NULL);
	if (palEnts == 0)
	    palEnts = 256;
	ReleaseDC(0, hdc);

	return CURSORICON_Load(hinst, name, desiredx, desiredy,
				 palEnts, FALSE, loadflags);
	}

    case IMAGE_CURSOR:
        return CURSORICON_Load(hinst, name, desiredx, desiredy,
				 1, TRUE, loadflags);
    }
    return 0;
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

    GDI_HEAP_UNLOCK( hbitmap );
    return res;
}

/******************************************************************************
 * CopyImage16 [USER.390]  Creates new image and copies attributes to it
 *
 */
HICON16 WINAPI CopyImage16( HANDLE16 hnd, UINT16 type, INT16 desiredx,
                             INT16 desiredy, UINT16 flags )
{
    return (HICON16)CopyImage((HANDLE)hnd, (UINT)type, (INT)desiredx,
                                (INT)desiredy, (UINT)flags);
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
HICON WINAPI CopyImage( HANDLE hnd, UINT type, INT desiredx,
                             INT desiredy, UINT flags )
{
    switch (type)
    {
	case IMAGE_BITMAP:
		return BITMAP_CopyBitmap(hnd);
	case IMAGE_ICON:
		return CopyIcon(hnd);
	case IMAGE_CURSOR:
		return CopyCursor(hnd);
    }
    return 0;
}

/**********************************************************************
 *       BITMAP_Load
 */
HBITMAP BITMAP_Load( HINSTANCE instance,LPCWSTR name, UINT loadflags )
{
    HBITMAP hbitmap = 0;
    HDC hdc;
    HRSRC hRsrc;
    HGLOBAL handle;
    char *ptr = NULL;
    BITMAPINFO *info, *fix_info=NULL;
    HGLOBAL hFix;
    int size;

    if (!(loadflags & LR_LOADFROMFILE)) {
      if (!instance)  /* OEM bitmap */
      {
          HDC hdc;
	  DC *dc;

	  if (HIWORD((int)name)) return 0;
	  hdc = CreateDCA( "DISPLAY", NULL, NULL, NULL );
	  dc = DC_GetDCPtr( hdc );
	  if(dc->funcs->pLoadOEMResource)
	      hbitmap = dc->funcs->pLoadOEMResource( LOWORD((int)name), 
						     OEM_BITMAP );
	  GDI_HEAP_UNLOCK( hdc );
	  DeleteDC( hdc );
	  return hbitmap;
      }

      if (!(hRsrc = FindResourceW( instance, name, RT_BITMAPW ))) return 0;
      if (!(handle = LoadResource( instance, hRsrc ))) return 0;

      if ((info = (BITMAPINFO *)LockResource( handle )) == NULL) return 0;
    }
    else
    {
        if (!(ptr = (char *)VIRTUAL_MapFileW( name ))) return 0;
        info = (BITMAPINFO *)(ptr + sizeof(BITMAPFILEHEADER));
    }
    size = DIB_BitmapInfoSize(info, DIB_RGB_COLORS);
    if ((hFix = GlobalAlloc(0, size))) fix_info=GlobalLock(hFix);
    if (fix_info) {
      BYTE pix;

      memcpy(fix_info, info, size);
      pix = *((LPBYTE)info+DIB_BitmapInfoSize(info, DIB_RGB_COLORS));
      DIB_FixColorsToLoadflags(fix_info, loadflags, pix);
      if ((hdc = GetDC(0)) != 0) {
        char *bits = (char *)info + size;
	if (loadflags & LR_CREATEDIBSECTION) {
          DIBSECTION dib;
	  hbitmap = CreateDIBSection(hdc, fix_info, DIB_RGB_COLORS, NULL, 0, 0);
          GetObjectA(hbitmap, sizeof(DIBSECTION), &dib);
          SetDIBits(hdc, hbitmap, 0, dib.dsBm.bmHeight, bits, info, 
                    DIB_RGB_COLORS);
        }
        else {
          hbitmap = CreateDIBitmap( hdc, &fix_info->bmiHeader, CBM_INIT,
                                      bits, fix_info, DIB_RGB_COLORS );
	}
        ReleaseDC( 0, hdc );
      }
      GlobalUnlock(hFix);
      GlobalFree(hFix);
    }
    if (loadflags & LR_LOADFROMFILE) UnmapViewOfFile( ptr );
    return hbitmap;
}


/******************************************************************************
 * LoadBitmapW [USER32.358]  Loads bitmap from the executable file
 *
 * RETURNS
 *    Success: Handle to specified bitmap
 *    Failure: NULL
 */
HBITMAP WINAPI LoadBitmapW(
    HINSTANCE instance, /* [in] Handle to application instance */
    LPCWSTR name)         /* [in] Address of bitmap resource name */
{
    return LoadImageW( instance, name, IMAGE_BITMAP, 0, 0, 0 );
}

/**********************************************************************
 *	    LoadBitmapA   (USER32.357)
 */
HBITMAP WINAPI LoadBitmapA( HINSTANCE instance, LPCSTR name )
{
    return LoadImageA( instance, name, IMAGE_BITMAP, 0, 0, 0 );
}

/**********************************************************************
 *	    LoadBitmap16    (USER.175)
 */
HBITMAP16 WINAPI LoadBitmap16( HINSTANCE16 instance, SEGPTR name )
{
    LPCSTR nameStr = HIWORD(name)? PTR_SEG_TO_LIN(name) : (LPCSTR)name;
    return LoadBitmapA( instance, nameStr );
}



/***********************************************************************
 *           BITMAP_DeleteObject
 */
BOOL BITMAP_DeleteObject( HBITMAP16 hbitmap, BITMAPOBJ * bmp )
{
    if( bmp->DDBitmap ) {
        if( bmp->DDBitmap->funcs->pDeleteObject )
	    bmp->DDBitmap->funcs->pDeleteObject( hbitmap );
    }

    if( bmp->bitmap.bmBits )
        HeapFree( GetProcessHeap(), 0, bmp->bitmap.bmBits );

    DIB_DeleteDIBSection( bmp );

    return GDI_FreeObject( hbitmap );
}

	
/***********************************************************************
 *           BITMAP_GetObject16
 */
INT16 BITMAP_GetObject16( BITMAPOBJ * bmp, INT16 count, LPVOID buffer )
{
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
	    FIXME_(bitmap)("not implemented for DIBs: count %d\n", count);
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
 *           BITMAP_GetObject32
 */
INT BITMAP_GetObject( BITMAPOBJ * bmp, INT count, LPVOID buffer )
{
    if (bmp->dib)
    {
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
	if (count > sizeof(BITMAP)) count = sizeof(BITMAP);
	memcpy( buffer, &bmp->bitmap, count );
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
HBITMAP WINAPI CreateDiscardableBitmap(
    HDC hdc,    /* [in] Handle to device context */
    INT width,  /* [in] Bitmap width */
    INT height) /* [in] Bitmap height */
{
    return CreateCompatibleBitmap( hdc, width, height );
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
    CONV_SIZE32TO16( &bmp->size, size );
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
BOOL WINAPI GetBitmapDimensionEx(
    HBITMAP hbitmap, /* [in]  Handle to bitmap */
    LPSIZE size)     /* [out] Address of struct receiving dimensions */
{
    BITMAPOBJ * bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    if (!bmp) return FALSE;
    *size = bmp->size;
    GDI_HEAP_UNLOCK( hbitmap );
    return TRUE;
}


/***********************************************************************
 *           GetBitmapDimension    (GDI.162)
 */
DWORD WINAPI GetBitmapDimension16( HBITMAP16 hbitmap )
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
    if (prevSize) CONV_SIZE32TO16( &bmp->size, prevSize );
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
    GDI_HEAP_UNLOCK( hbitmap );
    return TRUE;
}


/***********************************************************************
 *           SetBitmapDimension    (GDI.163)
 */
DWORD WINAPI SetBitmapDimension16( HBITMAP16 hbitmap, INT16 x, INT16 y )
{
    SIZE16 size;
    if (!SetBitmapDimensionEx16( hbitmap, x, y, &size )) return 0;
    return MAKELONG( size.cx, size.cy );
}

