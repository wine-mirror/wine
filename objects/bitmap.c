/*
 * GDI bitmap objects
 *
 * Copyright 1993 Alexandre Julliard
 *           1998 Huw D M Davies
 */

#include <stdlib.h>
#include <string.h>
#include "gdi.h"
#include "dc.h"
#include "bitmap.h"
#include "heap.h"
#include "global.h"
#include "sysmetrics.h"
#include "cursoricon.h"
#include "color.h"
#include "debug.h"


/***********************************************************************
 *           BITMAP_GetPadding
 *
 * Return number of bytes to pad a scanline of 16-bit aligned Windows DDB data.
 */
INT32 BITMAP_GetPadding( int bmWidth, int bpp )
{
    INT32 pad;

    switch (bpp) 
    {
    case 1:
        pad = ((bmWidth-1) & 8) ? 0 : 1;
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
 *           BITMAP_GetWidthBytes
 *
 * Return number of bytes taken by a scanline of 16-bit aligned Windows DDB
 * data.
 */
INT32 BITMAP_GetWidthBytes( INT32 bmWidth, INT32 bpp )
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
    return CreateUserBitmap16( width, height, 1, screenDepth, NULL );
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
 *    Failure: 0
 */
HBITMAP32 WINAPI CreateBitmap32( INT32 width, INT32 height, UINT32 planes,
                                 UINT32 bpp, LPCVOID bits )
{
    BITMAPOBJ *bmp;
    HBITMAP32 hbitmap;

    planes = (BYTE)planes;
    bpp    = (BYTE)bpp;


      /* Check parameters */
    if (!height || !width) return 0;
    if (planes != 1) {
        FIXME(bitmap, "planes = %d\n", planes);
	return 0;
    }
    if (height < 0) height = -height;
    if (width < 0) width = -width;

      /* Create the BITMAPOBJ */
    hbitmap = GDI_AllocObject( sizeof(BITMAPOBJ), BITMAP_MAGIC );
    if (!hbitmap) return 0;

    TRACE(bitmap, "%dx%d, %d colors returning %08x\n", width, height,
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
	SetBitmapBits32( hbitmap, height * bmp->bitmap.bmWidthBytes,
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
 *    Failure: 0
 */
HBITMAP32 WINAPI CreateCompatibleBitmap32( HDC32 hdc, INT32 width, INT32 height)
{
    HBITMAP32 hbmpRet = 0;
    DC *dc;

    TRACE(bitmap, "(%04x,%d,%d) = \n", hdc, width, height );
    if (!(dc = DC_GetDCPtr( hdc ))) return 0;
    if ((width >0x1000) || (height > 0x1000)) {
	FIXME(bitmap,"got bad width %d or height %d, please look for reason\n",
	      width, height );
	return 0;
    }
    hbmpRet = CreateBitmap32( width, height, 1, dc->w.bitsPerPixel, NULL );

    if(dc->funcs->pCreateBitmap)
        dc->funcs->pCreateBitmap( hbmpRet );

    TRACE(bitmap,"\t\t%04x\n", hbmpRet);
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
    LPVOID bits)       /* [out] Pointer to buffer to receive bits */
{
    BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    LONG height, ret;
    
    if (!bmp) return 0;

    if (count < 0) {
	WARN(bitmap, "(%ld): Negative number of bytes passed???\n", count );
	count = -count;
    }

    /* Only get entire lines */
    height = count / bmp->bitmap.bmWidthBytes;
    if (height > bmp->bitmap.bmHeight) height = bmp->bitmap.bmHeight;
    count = height * bmp->bitmap.bmWidthBytes;
    if (count == 0)
      {
	WARN(bitmap, "Less then one entire line requested\n");
	GDI_HEAP_UNLOCK( hbitmap );
	return 0;
      }


    TRACE(bitmap, "(%08x, %ld, %p) %dx%d %d colors fetched height: %ld\n",
	    hbitmap, count, bits, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
	    1 << bmp->bitmap.bmBitsPixel, height );

    if(bmp->DDBitmap) { 

        TRACE(bitmap, "Calling device specific BitmapBits\n");
	if(bmp->DDBitmap->funcs->pBitmapBits)
	    ret = bmp->DDBitmap->funcs->pBitmapBits(hbitmap, bits, count,
						    DDB_GET);
	else {
	    ERR(bitmap, "BitmapBits == NULL??\n");
	    ret = 0;
	}	

    } else {

        if(!bmp->bitmap.bmBits) {
	    WARN(bitmap, "Bitmap is empty\n");
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
    LPCVOID bits)      /* [in] Address of array with bitmap bits */
{
    BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    LONG height, ret;
    
    if (!bmp) return 0;

    if (count < 0) {
	WARN(bitmap, "(%ld): Negative number of bytes passed???\n", count );
	count = -count;
    }

    /* Only get entire lines */
    height = count / bmp->bitmap.bmWidthBytes;
    if (height > bmp->bitmap.bmHeight) height = bmp->bitmap.bmHeight;
    count = height * bmp->bitmap.bmWidthBytes;

    TRACE(bitmap, "(%08x, %ld, %p) %dx%d %d colors fetched height: %ld\n",
	    hbitmap, count, bits, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
	    1 << bmp->bitmap.bmBitsPixel, height );

    if(bmp->DDBitmap) {

        TRACE(bitmap, "Calling device specific BitmapBits\n");
	if(bmp->DDBitmap->funcs->pBitmapBits)
	    ret = bmp->DDBitmap->funcs->pBitmapBits(hbitmap, (void *) bits,
						    count, DDB_SET);
	else {
	    ERR(bitmap, "BitmapBits == NULL??\n");
	    ret = 0;
	}
	
    } else {

        if(!bmp->bitmap.bmBits) /* Alloc enough for entire bitmap */
	    bmp->bitmap.bmBits = HeapAlloc( GetProcessHeap(), 0, count );
	if(!bmp->bitmap.bmBits) {
	    WARN(bitmap, "Unable to allocate bit buffer\n");
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
 * FIXME: implementation lacks some features, see LR_ defines in windows.h
 */

HANDLE32 WINAPI LoadImage32A( HINSTANCE32 hinst, LPCSTR name, UINT32 type,
                              INT32 desiredx, INT32 desiredy, UINT32 loadflags)
{
    HANDLE32 res;
    LPWSTR u_name;

    if (HIWORD(name)) u_name = HEAP_strdupAtoW(GetProcessHeap(), 0, name);
    else u_name=(LPWSTR)name;
    res = LoadImage32W(hinst, u_name, type, desiredx, desiredy, loadflags);
    if (HIWORD(name)) HeapFree(GetProcessHeap(), 0, u_name);
    return res;
}


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
 * FIXME: Implementation lacks some features, see LR_ defines in windows.h
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
    if (loadflags & LR_DEFAULTSIZE)
    {
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
            return BITMAP_LoadBitmap32W(hinst, name, loadflags);
        case IMAGE_ICON:
	    return CURSORICON_Load32(hinst, name, desiredx, desiredy,
	      MIN(16, COLOR_GetSystemPaletteSize()), FALSE, loadflags);
        case IMAGE_CURSOR:
	    return CURSORICON_Load32(hinst, name, desiredx, desiredy, 1, TRUE,
	      loadflags);
    }
    return 0;
}


/**********************************************************************
 *		BITMAP_CopyBitmap
 *
 */
HBITMAP32 BITMAP_CopyBitmap(HBITMAP32 hbitmap)
{
    BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr( hbitmap, BITMAP_MAGIC );
    HBITMAP32 res = 0;
    BITMAP32 bm;

    if(!bmp) return 0;

    bm = bmp->bitmap;
    bm.bmBits = NULL;
    res = CreateBitmapIndirect32(&bm);

    if(res) {
        char *buf = HeapAlloc( GetProcessHeap(), 0, bm.bmWidthBytes *
			       bm.bmHeight );
        GetBitmapBits32 (hbitmap, bm.bmWidthBytes * bm.bmHeight, buf);
	SetBitmapBits32 (res, bm.bmWidthBytes * bm.bmHeight, buf);
	HeapFree( GetProcessHeap(), 0, buf );
    }

    GDI_HEAP_UNLOCK( hbitmap );
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
HICON32 WINAPI CopyImage32( HANDLE32 hnd, UINT32 type, INT32 desiredx,
                             INT32 desiredy, UINT32 flags )
{
    switch (type)
    {
	case IMAGE_BITMAP:
		return BITMAP_CopyBitmap(hnd);
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


/**********************************************************************
 *       BITMAP_LoadBitmap32W
 */
HBITMAP32 BITMAP_LoadBitmap32W(HINSTANCE32 instance,LPCWSTR name, 
			       UINT32 loadflags)
{
    HBITMAP32 hbitmap = 0;
    HDC32 hdc;
    HRSRC32 hRsrc;
    HGLOBAL32 handle;
    char *ptr = NULL;
    BITMAPINFO *info, *fix_info=NULL;
    HGLOBAL32 hFix;
    int size;

    if (!(loadflags & LR_LOADFROMFILE)) {
      if (!instance)  /* OEM bitmap */
      {
          if (HIWORD((int)name)) return 0;
          return OBM_LoadBitmap( LOWORD((int)name) );
      }

      if (!(hRsrc = FindResource32W( instance, name, RT_BITMAP32W ))) return 0;
      if (!(handle = LoadResource32( instance, hRsrc ))) return 0;

      if ((info = (BITMAPINFO *)LockResource32( handle )) == NULL) return 0;
    }
    else
    {
        if (!(ptr = (char *)VIRTUAL_MapFileW( name ))) return 0;
        info = (BITMAPINFO *)(ptr + sizeof(BITMAPFILEHEADER));
    }
    size = DIB_BitmapInfoSize(info, DIB_RGB_COLORS);
    if ((hFix = GlobalAlloc32(0, size))) fix_info=GlobalLock32(hFix);
    if (fix_info) {
      BYTE pix;

      memcpy(fix_info, info, size);
      pix = *((LPBYTE)info+DIB_BitmapInfoSize(info, DIB_RGB_COLORS));
      DIB_FixColorsToLoadflags(fix_info, loadflags, pix);
      if ((hdc = GetDC32(0)) != 0) {
	if (loadflags & LR_CREATEDIBSECTION)
	  hbitmap = CreateDIBSection32(hdc, fix_info, DIB_RGB_COLORS, NULL, 0, 0);
        else {
          char *bits = (char *)info + size;;
          hbitmap = CreateDIBitmap32( hdc, &fix_info->bmiHeader, CBM_INIT,
                                      bits, fix_info, DIB_RGB_COLORS );
	}
        ReleaseDC32( 0, hdc );
      }
      GlobalUnlock32(hFix);
      GlobalFree32(hFix);
    }
    if (loadflags & LR_LOADFROMFILE) UnmapViewOfFile( ptr );
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
    return BITMAP_LoadBitmap32W(instance, name, 0);
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
            BITMAP32 *bmp32 = &bmp->dib->dibSection.dsBm;
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
	    FIXME(bitmap, "not implemented for DIBs: count %d\n", count);
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
INT32 BITMAP_GetObject32( BITMAPOBJ * bmp, INT32 count, LPVOID buffer )
{
    if (bmp->dib)
    {
	if (count < sizeof(DIBSECTION))
	{
	    if (count > sizeof(BITMAP32)) count = sizeof(BITMAP32);
	}
	else
	{
	    if (count > sizeof(DIBSECTION)) count = sizeof(DIBSECTION);
	}

	memcpy( buffer, &bmp->dib->dibSection, count );
	return count;
    }
    else
    {
	if (count > sizeof(BITMAP32)) count = sizeof(BITMAP32);
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
BOOL32 WINAPI GetBitmapDimensionEx32(
    HBITMAP32 hbitmap, /* [in]  Handle to bitmap */
    LPSIZE32 size)     /* [out] Address of struct receiving dimensions */
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
BOOL32 WINAPI SetBitmapDimensionEx32(
    HBITMAP32 hbitmap, /* [in]  Handle to bitmap */
    INT32 x,           /* [in]  Bitmap width */
    INT32 y,           /* [in]  Bitmap height */
    LPSIZE32 prevSize) /* [out] Address of structure for orig dims */
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
DWORD WINAPI SetBitmapDimension( HBITMAP16 hbitmap, INT16 x, INT16 y )
{
    SIZE16 size;
    if (!SetBitmapDimensionEx16( hbitmap, x, y, &size )) return 0;
    return MAKELONG( size.cx, size.cy );
}

