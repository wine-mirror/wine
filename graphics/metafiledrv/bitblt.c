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
BOOL MFDRV_PatBlt( DC *dc, INT left, INT top,
                     INT width, INT height, DWORD rop )
{
    MF_MetaParam6( dc, META_PATBLT, left, top, width, height,
                   HIWORD(rop), LOWORD(rop) );
    return TRUE;
}


/***********************************************************************
 *           MFDRV_BitBlt
 */
BOOL MFDRV_BitBlt( DC *dcDst, INT xDst, INT yDst,
                      INT width, INT height, DC *dcSrc,
                      INT xSrc, INT ySrc, DWORD rop )
{
    return MF_BitBlt( dcDst, xDst, yDst, width, height,
                      dcSrc, xSrc, ySrc, rop );
}


/***********************************************************************
 *           MFDRV_StretchBlt
 */
BOOL MFDRV_StretchBlt( DC *dcDst, INT xDst, INT yDst,
                          INT widthDst, INT heightDst,
                          DC *dcSrc, INT xSrc, INT ySrc,
                          INT widthSrc, INT heightSrc, DWORD rop )
{
    return MF_StretchBlt( dcDst, xDst, yDst, widthDst, heightDst,
                          dcSrc, xSrc, ySrc, widthSrc, heightSrc, rop );
}
