/*
 * GDI bit-blit operations
 *
 * Copyright 1993, 1994  Alexandre Julliard
 */

#include "gdi.h"
#include "metafiledrv.h"
#include "heap.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(metafile)

/***********************************************************************
 *           MFDRV_PatBlt
 */
BOOL MFDRV_PatBlt( DC *dc, INT left, INT top,
                     INT width, INT height, DWORD rop )
{
    MFDRV_MetaParam6( dc, META_PATBLT, left, top, width, height,
		      HIWORD(rop), LOWORD(rop) );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_BitBlt
 */
BOOL MFDRV_BitBlt( DC *dcDst, INT xDst, INT yDst, INT width, INT height,
		   DC *dcSrc, INT xSrc, INT ySrc, DWORD rop )
{
    BOOL ret;
    DWORD len;
    METARECORD *mr;
    BITMAP16  BM;

    GetObject16(dcSrc->w.hBitmap, sizeof(BITMAP16), &BM);
    len = sizeof(METARECORD) + 12 * sizeof(INT16) + BM.bmWidthBytes * BM.bmHeight;
    if (!(mr = HeapAlloc(SystemHeap, 0, len)))
	return FALSE;
    mr->rdFunction = META_BITBLT;
    *(mr->rdParm + 7) = BM.bmWidth;
    *(mr->rdParm + 8) = BM.bmHeight;
    *(mr->rdParm + 9) = BM.bmWidthBytes;
    *(mr->rdParm +10) = BM.bmPlanes;
    *(mr->rdParm +11) = BM.bmBitsPixel;
    TRACE(metafile,"len = %ld  rop=%lx  \n",len,rop);
    if (GetBitmapBits(dcSrc->w.hBitmap,BM.bmWidthBytes * BM.bmHeight,
                        mr->rdParm +12))
    {
      mr->rdSize = len / sizeof(INT16);
      *(mr->rdParm) = HIWORD(rop);
      *(mr->rdParm + 1) = ySrc;
      *(mr->rdParm + 2) = xSrc;
      *(mr->rdParm + 3) = height;
      *(mr->rdParm + 4) = width;
      *(mr->rdParm + 5) = yDst;
      *(mr->rdParm + 6) = xDst;
      ret = MFDRV_WriteRecord( dcDst, mr, mr->rdSize * 2);
    } 
    else
        ret = FALSE;
    HeapFree( SystemHeap, 0, mr);
    return ret;
}



/***********************************************************************
 *           MFDRV_StretchBlt
 * this function contains TWO ways for procesing StretchBlt in metafiles,
 * decide between rdFunction values  META_STRETCHBLT or META_DIBSTRETCHBLT
 * via #define STRETCH_VIA_DIB
 */
#define STRETCH_VIA_DIB
#undef  STRETCH_VIA_DIB

BOOL MFDRV_StretchBlt( DC *dcDst, INT xDst, INT yDst, INT widthDst,
		       INT heightDst, DC *dcSrc, INT xSrc, INT ySrc,
		       INT widthSrc, INT heightSrc, DWORD rop )
{
    BOOL ret;
    DWORD len;
    METARECORD *mr;
    BITMAP16  BM;
#ifdef STRETCH_VIA_DIB    
    LPBITMAPINFOHEADER lpBMI;
    WORD nBPP;
#endif  
    GetObject16(dcSrc->w.hBitmap, sizeof(BITMAP16), &BM);
#ifdef STRETCH_VIA_DIB
    nBPP = BM.bmPlanes * BM.bmBitsPixel;
    len = sizeof(METARECORD) + 10 * sizeof(INT16) 
            + sizeof(BITMAPINFOHEADER) + (nBPP != 24 ? 1 << nBPP: 0) * sizeof(RGBQUAD) 
              + ((BM.bmWidth * nBPP + 31) / 32) * 4 * BM.bmHeight;
    if (!(mr = HeapAlloc( SystemHeap, 0, len)))
	return FALSE;
    mr->rdFunction = META_DIBSTRETCHBLT;
    lpBMI=(LPBITMAPINFOHEADER)(mr->rdParm+10);
    lpBMI->biSize      = sizeof(BITMAPINFOHEADER);
    lpBMI->biWidth     = BM.bmWidth;
    lpBMI->biHeight    = BM.bmHeight;
    lpBMI->biPlanes    = 1;
    lpBMI->biBitCount  = nBPP;                              /* 1,4,8 or 24 */
    lpBMI->biClrUsed   = nBPP != 24 ? 1 << nBPP : 0;
    lpBMI->biSizeImage = ((lpBMI->biWidth * nBPP + 31) / 32) * 4 * lpBMI->biHeight;
    lpBMI->biCompression = BI_RGB;
    lpBMI->biXPelsPerMeter = MulDiv(GetDeviceCaps(dcSrc->hSelf,LOGPIXELSX),3937,100);
    lpBMI->biYPelsPerMeter = MulDiv(GetDeviceCaps(dcSrc->hSelf,LOGPIXELSY),3937,100);
    lpBMI->biClrImportant  = 0;                          /* 1 meter  = 39.37 inch */

    TRACE(metafile,"MF_StretchBltViaDIB->len = %ld  rop=%lx  PixYPM=%ld Caps=%d\n",
               len,rop,lpBMI->biYPelsPerMeter,GetDeviceCaps(hdcSrc,LOGPIXELSY));
    if (GetDIBits(hdcSrc,dcSrc->w.hBitmap,0,(UINT)lpBMI->biHeight,
                  (LPSTR)lpBMI + DIB_BitmapInfoSize( (BITMAPINFO *)lpBMI,
                                                     DIB_RGB_COLORS ),
                  (LPBITMAPINFO)lpBMI, DIB_RGB_COLORS))
#else
    len = sizeof(METARECORD) + 15 * sizeof(INT16) + BM.bmWidthBytes * BM.bmHeight;
    if (!(mr = HeapAlloc( SystemHeap, 0, len )))
	return FALSE;
    mr->rdFunction = META_STRETCHBLT;
    *(mr->rdParm +10) = BM.bmWidth;
    *(mr->rdParm +11) = BM.bmHeight;
    *(mr->rdParm +12) = BM.bmWidthBytes;
    *(mr->rdParm +13) = BM.bmPlanes;
    *(mr->rdParm +14) = BM.bmBitsPixel;
    TRACE(metafile,"len = %ld  rop=%lx  \n",len,rop);
    if (GetBitmapBits( dcSrc->w.hBitmap, BM.bmWidthBytes * BM.bmHeight,
                         mr->rdParm +15))
#endif    
    {
      mr->rdSize = len / sizeof(INT16);
      *(mr->rdParm) = LOWORD(rop);
      *(mr->rdParm + 1) = HIWORD(rop);
      *(mr->rdParm + 2) = heightSrc;
      *(mr->rdParm + 3) = widthSrc;
      *(mr->rdParm + 4) = ySrc;
      *(mr->rdParm + 5) = xSrc;
      *(mr->rdParm + 6) = heightDst;
      *(mr->rdParm + 7) = widthDst;
      *(mr->rdParm + 8) = yDst;
      *(mr->rdParm + 9) = xDst;
      ret = MFDRV_WriteRecord( dcDst, mr, mr->rdSize * 2);
    }  
    else
        ret = FALSE;
    HeapFree( SystemHeap, 0, mr);
    return ret;
}


