/*
 * GDI bit-blit operations
 *
 * Copyright 1993, 1994  Alexandre Julliard
 */

#include "gdi.h"
#include "metafile.h"
#include "metafiledrv.h"

/***********************************************************************
 *           MFDRV_PatBlt
 */
BOOL32 MFDRV_PatBlt( DC *dc, INT32 left, INT32 top,
                     INT32 width, INT32 height, DWORD rop )
{
    MF_MetaParam6( dc, META_PATBLT, left, top, width, height,
                   HIWORD(rop), LOWORD(rop) );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_BitBlt
 */
BOOL32 MFDRV_BitBlt( DC *dcDst, INT32 xDst, INT32 yDst,
                      INT32 width, INT32 height, DC *dcSrc,
                      INT32 xSrc, INT32 ySrc, DWORD rop )
{
    return MF_BitBlt( dcDst, xDst, yDst, width, height,
                      dcSrc, xSrc, ySrc, rop );
}


/***********************************************************************
 *           MFDRV_StretchBlt
 */
BOOL32 MFDRV_StretchBlt( DC *dcDst, INT32 xDst, INT32 yDst,
                          INT32 widthDst, INT32 heightDst,
                          DC *dcSrc, INT32 xSrc, INT32 ySrc,
                          INT32 widthSrc, INT32 heightSrc, DWORD rop )
{
    return MF_StretchBlt( dcDst, xDst, yDst, widthDst, heightDst,
                          dcSrc, xSrc, ySrc, widthSrc, heightSrc, rop );
}
