/*
 * WingG support
 *
 * Started by Robert Pouliot <krynos@clic.net>
 */

#include "ts_xlib.h"
#include "ts_xshm.h"
#include <sys/types.h>
#include <sys/ipc.h>
#ifndef __EMX__
#include <sys/shm.h>
#endif

#include "windows.h"
#include "bitmap.h"
#include "dc.h"
#include "gdi.h"
#include "xmalloc.h"
#include "debug.h"

extern void CLIPPING_UpdateGCRegion(DC* );

typedef enum WING_DITHER_TYPE
{
  WING_DISPERSED_4x4, WING_DISPERSED_8x8, WING_CLUSTERED_4x4
} WING_DITHER_TYPE;

static int	__WinGOK = -1;

/* 
 * WinG DIB bitmaps can be selected into DC and then scribbled upon
 * by GDI functions. They can also be changed directly. This gives us 
 * three choices 
 *	- use original WinG 16-bit DLL
 *		requires working 16-bit driver interface
 * 	- implement DIB graphics driver from scratch
 *		see wing.zip size
 *	- use shared pixmaps
 *		won't work with some videocards and/or videomodes
 * 961208 - AK
 */

static BITMAPINFOHEADER __bmpiWinG = { 0, 1, -1, 1, 8, BI_RGB, 1, 0, 0, 0, 0 };

static void __initWinG(void)
{
  if( __WinGOK < 0 )
  {
    Status s = TSXShmQueryExtension(display);
    if( s )
    {
      int i = TSXShmPixmapFormat(display);
      if( i == ZPixmap && screenDepth == 8 ) 
      {
        __WinGOK = True;
	return;
      }
    } 
    FIXME(wing,"WinG: no joy.\n");
    __WinGOK = False;
  }
}

/***********************************************************************
 *          WinGCreateDC16	(WING.1001)
 */
HDC16 WINAPI WinGCreateDC16(void)
{
  __initWinG();

  if( __WinGOK > 0 )
	return CreateCompatibleDC16(NULL);
  return (HDC16)NULL;
}

/***********************************************************************
 *  WinGRecommendDIBFormat16    (WING.1002)
 */
BOOL16 WINAPI WinGRecommendDIBFormat16(BITMAPINFO *fmt)
{
  FIXME(wing,"(%p): stub\n", fmt);

  if( __WinGOK > 0 && fmt )
  {
    memcpy(&fmt->bmiHeader, &__bmpiWinG, sizeof(BITMAPINFOHEADER));
    return TRUE;
  }
  return FALSE;
}

/***********************************************************************
 *        WinGCreateBitmap16    (WING.1003)
 */
HBITMAP16 WINAPI WinGCreateBitmap16(HDC16 winDC, BITMAPINFO *header,
                                    void **bits)
{
  FIXME(wing,"(%x,%p,%p): empty stub! (expect failure)\n", 
	winDC, header, bits);
  if( __WinGOK > 0 && header )
  {
    BITMAPINFOHEADER* bmpi = &header->bmiHeader;

     FIXME(wing,"bytes=%i,planes=%i,bpp=%i,x=%i,y=%i,rle=0x%08x,size=%i\n",
	   (int)bmpi->biSize, bmpi->biPlanes, bmpi->biBitCount,
	   (int)bmpi->biWidth, (int)bmpi->biHeight, 
	   (unsigned)bmpi->biCompression, (int)bmpi->biSizeImage);

#ifdef PRELIMINARY_WING16_SUPPORT
    if( bmpi->biPlanes == __bmpiWinG.biPlanes && bmpi->biBitCount == __bmpiWinG.biBitCount &&
	bmpi->biCompression == __bmpiWinG.biCompression && (int)bmpi->biHeight < 0 &&
	bmpi->biWidth )
    {
	unsigned bytes = (bmpi->biWidth + bmpi->biWidth % 2)*(-bmpi->biHeight) * bmpi->biBitCount/8;
	int	 key = shmget(IPC_PRIVATE, bytes, IPC_CREAT | 0x01FF);

	if( key )
	{
	    /* Create the BITMAPOBJ 
	     *
	     * FIXME: A facility to manage shared memory structures
	     * which would clean up when Wine crashes. Perhaps a part of 
	     * IPC code can be adapted. Otherwise this code leaves a lot
	     * of junk in shared memory. 
	     */

	    HBITMAP16 hbitmap = GDI_AllocObject( sizeof(BITMAPOBJ), BITMAP_MAGIC );
	    if (hbitmap)
	    {
	      __ShmBitmapCtl* p = (__ShmBitmapCtl*)xmalloc(sizeof(__ShmBitmapCtl));
		BITMAPOBJ* 	 bmpObjPtr = (BITMAPOBJ *) GDI_HEAP_LOCK( hbitmap );

		bmpObjPtr->size.cx = 0;
		bmpObjPtr->size.cy = 0;
		bmpObjPtr->bitmap.bmType = 0;
		bmpObjPtr->bitmap.bmWidth = (INT16)abs(bmpi->biWidth);
		bmpObjPtr->bitmap.bmHeight = -(INT16)bmpi->biHeight;
		bmpObjPtr->bitmap.bmPlanes = (BYTE)bmpi->biPlanes;
		bmpObjPtr->bitmap.bmBitsPixel = (BYTE)bmpi->biBitCount;
		bmpObjPtr->bitmap.bmWidthBytes = 
		  (INT16)BITMAP_WIDTH_BYTES( bmpObjPtr->bitmap.bmWidth, bmpi->biBitCount );
		bmpObjPtr->bitmap.bmBits = (SEGPTR)p;

		p->si.shmid = key;
		p->si.shmaddr = shmat(key, NULL, 0);
		p->si.readOnly = False;

		if( p->si.shmaddr )
		{
		    WORD	sel = 0;

		    XShmAttach(display, &p->si);
		    bmpObjPtr->pixmap = XShmCreatePixmap(display, rootWindow, 
				  p->si.shmaddr, &p->si, bmpObjPtr->bitmap.bmWidth, 
				  bmpObjPtr->bitmap.bmHeight, bmpi->biBitCount );
		    if( bmpObjPtr->pixmap )
		    {
                        sel = SELECTOR_AllocBlock( p->si.shmaddr, bytes,
                                                   SEGMENT_DATA, FALSE, FALSE);
                        if (sel) p->bits = PTR_SEG_OFF_TO_SEGPTR(sel,0);
			else TSXFreePixmap( display, bmpObjPtr->pixmap );
		    }
		    if( !sel )
		    {
		      shmdt( p->si.shmaddr );
		      p->si.shmaddr = NULL;
		    }
		} 
		if( !p->si.shmaddr )
		{
		    GDI_FreeObject( hbitmap );
		    hbitmap = 0;
		}
	    }
	    GDI_HEAP_UNLOCK( hbitmap );
	    return hbitmap;
	}
    }
#endif
  }
  return 0;
}

/***********************************************************************
 *  WinGGetDIBPointer   (WING.1004)
 */
SEGPTR WINAPI WinGGetDIBPointer16(HBITMAP16 hWinGBitmap, BITMAPINFO* bmpi)
{
#ifdef PRELIMINARY_WING16_SUPPORT
  BITMAPOBJ*	bmp = (BITMAPOBJ *) GDI_GetObjPtr( hWinGBitmap, BITMAP_MAGIC );

  if( bmp )
  {
    __ShmBitmapCtl* p = (__ShmBitmapCtl*)bmp->bitmap.bmBits;
    if( p )
    {
      if( bmpi ) memcpy( bmpi, &__bmpiWinG, sizeof(BITMAPINFOHEADER));
      GDI_HEAP_UNLOCK( hWinGBitmap );
      return p->bits;
    }
  }
#endif
  return (SEGPTR)NULL;
}

/***********************************************************************
 *  WinGSetDIBColorTable   (WING.1004)
 */
UINT16 WINAPI WinGSetDIBColorTable16(HDC16 hWinGDC, UINT16 start, UINT16 num,
                                     RGBQUAD* pColor)
{
        FIXME(wing,"(%x,%d,%d,%p): empty stub!\n",hWinGDC,start,num,pColor);
        return num;
}

/***********************************************************************
 *  WinGGetDIBColorTable16   (WING.1005)
 */
UINT16 WINAPI WinGGetDIBColorTable16(HDC16 winDC, UINT16 start,
                                     UINT16 num, RGBQUAD* colors)
{
        FIXME(wing,"(%x,%d,%d,%p): empty stub!\n",winDC,start,num,colors);
	return 0;
}

/***********************************************************************
 *  WinGCreateHalfTonePalette16   (WING.1007)
 */
HPALETTE16 WINAPI WinGCreateHalfTonePalette16(void)
{
        FIXME(wing,"(void): empty stub!\n");
	return 0;
}

/***********************************************************************
 *  WinGCreateHalfToneBrush16   (WING.1008)
 */
HPALETTE16 WINAPI WinGCreateHalfToneBrush16(HDC16 winDC, COLORREF col,
                                            WING_DITHER_TYPE type)
{
        FIXME(wing,"(...): empty stub!\n");
	return 0;
}

/***********************************************************************
 *  WinGStretchBlt16   (WING.1009)
 */
BOOL16 WINAPI WinGStretchBlt16(HDC16 destDC, INT16 xDest, INT16 yDest,
                               INT16 widDest, INT16 heiDest,
                               HDC16 srcDC, INT16 xSrc, INT16 ySrc,
                               INT16 widSrc, INT16 heiSrc)
{

        return StretchBlt16(destDC, xDest, yDest, widDest, heiDest, srcDC, xSrc, ySrc, widSrc, heiSrc, SRCCOPY);
}

/***********************************************************************
 *  WinGBitBlt16   (WING.1010)
 */
BOOL16 WINAPI WinGBitBlt16(HDC16 destDC, INT16 xDest, INT16 yDest,
                           INT16 widDest, INT16 heiDest, HDC16 srcDC,
                           INT16 xSrc, INT16 ySrc)
{
    /* destDC is a display DC, srcDC is a memory DC */

    DC *dcDst, *dcSrc;

    if (!(dcDst = (DC *)GDI_GetObjPtr( destDC, DC_MAGIC ))) return FALSE;
    if (!(dcSrc = (DC *) GDI_GetObjPtr( srcDC, DC_MAGIC ))) return FALSE;

    if (dcDst->w.flags & DC_DIRTY) CLIPPING_UpdateGCRegion( dcDst );

    xSrc    = dcSrc->w.DCOrgX + XLPTODP( dcSrc, xSrc );
    ySrc    = dcSrc->w.DCOrgY + YLPTODP( dcSrc, ySrc );
    xDest   = dcDst->w.DCOrgX + XLPTODP( dcDst, xDest );
    yDest   = dcDst->w.DCOrgY + YLPTODP( dcDst, yDest );
    widDest = widDest * dcDst->vportExtX / dcDst->wndExtX;
    heiDest = heiDest * dcDst->vportExtY / dcDst->wndExtY;

    TSXSetFunction( display, dcDst->u.x.gc, GXcopy );
    TSXCopyArea( display, dcSrc->u.x.drawable,
               dcDst->u.x.drawable, dcDst->u.x.gc,
               xSrc, ySrc, widDest, heiDest, xDest, yDest );
    return TRUE;
}

