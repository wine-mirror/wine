/*
 * GDI bit-blit operations
 *
 * Copyright 1993, 1994  Alexandre Julliard
 */

#include "gdi.h"
#include "metafiledrv.h"
#include "heap.h"
#include "debug.h"
#include "bitmap.h"

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


/***********************************************************************
 *           MFDRV_StretchDIBits
 */
INT MFDRV_StretchDIBits( DC *dc, INT xDst, INT yDst, INT widthDst,
			 INT heightDst, INT xSrc, INT ySrc, INT widthSrc,
			 INT heightSrc, const void *bits,
			 const BITMAPINFO *info, UINT wUsage, DWORD dwRop )
{
    DWORD len, infosize, imagesize;
    METARECORD *mr;

    infosize = DIB_BitmapInfoSize(info, wUsage);
    imagesize = DIB_GetDIBImageBytes( info->bmiHeader.biWidth,
				      info->bmiHeader.biHeight,
				      info->bmiHeader.biBitCount );

    len = sizeof(METARECORD) + 10 * sizeof(WORD) + infosize + imagesize;
    mr = (METARECORD *)HeapAlloc( SystemHeap, 0, len );
    if(!mr) return 0;

    mr->rdSize = len / 2;
    mr->rdFunction = META_STRETCHDIB;
    mr->rdParm[0] = LOWORD(dwRop);
    mr->rdParm[1] = HIWORD(dwRop);
    mr->rdParm[2] = wUsage;
    mr->rdParm[3] = (INT16)heightSrc;
    mr->rdParm[4] = (INT16)widthSrc;
    mr->rdParm[5] = (INT16)ySrc;
    mr->rdParm[6] = (INT16)xSrc;
    mr->rdParm[7] = (INT16)heightDst;
    mr->rdParm[8] = (INT16)widthDst;
    mr->rdParm[9] = (INT16)yDst;
    mr->rdParm[10] = (INT16)xDst;
    memcpy(mr->rdParm + 11, info, infosize);
    memcpy(mr->rdParm + 11 + infosize / 2, bits, imagesize);
    MFDRV_WriteRecord( dc, mr, mr->rdSize * 2 );
    HeapFree( SystemHeap, 0, mr );
    return heightSrc;
}

	
/***********************************************************************
 *           MFDRV_SetDIBitsToDeivce
 */
INT MFDRV_SetDIBitsToDevice( DC *dc, INT xDst, INT yDst, DWORD cx,
			     DWORD cy, INT xSrc, INT ySrc, UINT startscan,
			     UINT lines, LPCVOID bits, const BITMAPINFO *info,
			     UINT coloruse )

{
    DWORD len, infosize, imagesize;
    METARECORD *mr;

    infosize = DIB_BitmapInfoSize(info, coloruse);
    imagesize = DIB_GetDIBImageBytes( info->bmiHeader.biWidth,
				      info->bmiHeader.biHeight,
				      info->bmiHeader.biBitCount );

    len = sizeof(METARECORD) + 8 * sizeof(WORD) + infosize + imagesize;
    mr = (METARECORD *)HeapAlloc( SystemHeap, 0, len );
    if(!mr) return 0;

    mr->rdSize = len / 2;
    mr->rdFunction = META_SETDIBTODEV;
    mr->rdParm[0] = coloruse;
    mr->rdParm[1] = lines;
    mr->rdParm[2] = startscan;
    mr->rdParm[3] = (INT16)ySrc;
    mr->rdParm[4] = (INT16)xSrc;
    mr->rdParm[5] = (INT16)cy;
    mr->rdParm[6] = (INT16)cx;
    mr->rdParm[7] = (INT16)yDst;
    mr->rdParm[8] = (INT16)xDst;
    memcpy(mr->rdParm + 9, info, infosize);
    memcpy(mr->rdParm + 9 + infosize / 2, bits, imagesize);
    MFDRV_WriteRecord( dc, mr, mr->rdSize * 2 );
    HeapFree( SystemHeap, 0, mr );
    return lines;
}

	
