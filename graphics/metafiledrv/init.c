/*
 * Metafile driver initialisation functions
 *
 * Copyright 1996 Alexandre Julliard
 */

#include "metafiledrv.h"
#include "dc.h"
#include "heap.h"
#include "global.h"
#include "metafile.h"
#include "stddebug.h"
#include "debug.h"

static BOOL32 MFDRV_DeleteDC( DC *dc );

static const DC_FUNCTIONS MFDRV_Funcs =
{
    NULL,                            /* pArc */
    MFDRV_BitBlt,                    /* pBitBlt */
    NULL,                            /* pChord */
    NULL,                            /* pCreateDC */
    MFDRV_DeleteDC,                  /* pDeleteDC */
    NULL,                            /* pDeleteObject */
    NULL,                            /* pEllipse */
    NULL,                            /* pEscape */
    NULL,                            /* pExcludeClipRect */
    NULL,                            /* pExcludeVisRect */
    NULL,                            /* pExtFloodFill */
    NULL,                            /* pExtTextOut */
    NULL,                            /* pFillRgn */
    NULL,                            /* pFloodFill */
    NULL,                            /* pFrameRgn */
    NULL,                            /* pGetTextExtentPoint */
    NULL,                            /* pGetTextMetrics */
    NULL,                            /* pIntersectClipRect */
    NULL,                            /* pIntersectVisRect */
    NULL,                            /* pInvertRgn */
    NULL,                            /* pLineTo */
    NULL,                            /* pMoveToEx */
    NULL,                            /* pOffsetClipRgn */
    MFDRV_OffsetViewportOrg,         /* pOffsetViewportOrg */
    MFDRV_OffsetWindowOrg,           /* pOffsetWindowOrg */
    NULL,                            /* pPaintRgn */
    MFDRV_PatBlt,                    /* pPatBlt */
    NULL,                            /* pPie */
    NULL,                            /* pPolyPolygon */
    NULL,                            /* pPolygon */
    NULL,                            /* pPolyline */
    NULL,                            /* pRealizePalette */
    NULL,                            /* pRectangle */
    NULL,                            /* pRestoreDC */
    NULL,                            /* pRoundRect */
    NULL,                            /* pSaveDC */
    MFDRV_ScaleViewportExt,          /* pScaleViewportExt */
    MFDRV_ScaleWindowExt,            /* pScaleWindowExt */
    NULL,                            /* pSelectClipRgn */
    NULL,                            /* pSelectObject */
    NULL,                            /* pSelectPalette */
    NULL,                            /* pSetBkColor */
    NULL,                            /* pSetBkMode */
    NULL,                            /* pSetDeviceClipping */
    NULL,                            /* pSetDIBitsToDevice */
    MFDRV_SetMapMode,                /* pSetMapMode */
    NULL,                            /* pSetMapperFlags */
    NULL,                            /* pSetPixel */
    NULL,                            /* pSetPolyFillMode */
    NULL,                            /* pSetROP2 */
    NULL,                            /* pSetRelAbs */
    NULL,                            /* pSetStretchBltMode */
    NULL,                            /* pSetTextAlign */
    NULL,                            /* pSetTextCharacterExtra */
    NULL,                            /* pSetTextColor */
    NULL,                            /* pSetTextJustification */
    MFDRV_SetViewportExt,            /* pSetViewportExt */
    MFDRV_SetViewportOrg,            /* pSetViewportOrg */
    MFDRV_SetWindowExt,              /* pSetWindowExt */
    MFDRV_SetWindowOrg,              /* pSetWindowOrg */
    MFDRV_StretchBlt,                /* pStretchBlt */
    NULL,                            /* pStretchDIBits */
    NULL                             /* pTextOut */
};



/**********************************************************************
 *	     MFDRV_AllocMetaFile
 */
static DC *MFDRV_AllocMetaFile(void)
{
    DC *dc;
    METAFILEDRV_PDEVICE *physDev;
    
    if (!(dc = DC_AllocDC( &MFDRV_Funcs ))) return NULL;
    dc->header.wMagic = METAFILE_DC_MAGIC;

    physDev = (METAFILEDRV_PDEVICE *)HeapAlloc(SystemHeap,0,sizeof(*physDev));
    if (!physDev)
    {
        GDI_HEAP_FREE( dc->hSelf );
        return NULL;
    }
    dc->physDev = physDev;

    if (!(physDev->mh = HeapAlloc( SystemHeap, 0, sizeof(*physDev->mh) )))
    {
        HeapFree( SystemHeap, 0, physDev );
        GDI_HEAP_FREE( dc->hSelf );
        return NULL;
    }

    physDev->nextHandle = 0;

    physDev->mh->mtHeaderSize   = sizeof(METAHEADER) / sizeof(WORD);
    physDev->mh->mtVersion      = 0x0300;
    physDev->mh->mtSize         = physDev->mh->mtHeaderSize;
    physDev->mh->mtNoObjects    = 0;
    physDev->mh->mtMaxRecord    = 0;
    physDev->mh->mtNoParameters = 0;

/*    DC_InitDC( dc ); */
    return dc;
}


/**********************************************************************
*	     MFDRV_DeleteDC
 */
static BOOL32 MFDRV_DeleteDC( DC *dc )
{
    METAFILEDRV_PDEVICE *physDev = (METAFILEDRV_PDEVICE *)dc->physDev;
    
    if (physDev->mh) HeapFree( SystemHeap, 0, physDev->mh );
    HeapFree( SystemHeap, 0, physDev );
    dc->physDev = NULL;
    return TRUE;
}


/**********************************************************************
 *	     CreateMetafile16   (GDI.125)
 */
HDC16 CreateMetaFile16( LPCSTR filename )
{
    DC *dc;
    METAFILEDRV_PDEVICE *physDev;
    HFILE32 hFile;

    dprintf_metafile( stddeb, "CreateMetaFile16: '%s'\n", filename );

    if (!(dc = MFDRV_AllocMetaFile())) return 0;
    physDev = (METAFILEDRV_PDEVICE *)dc->physDev;
    
    if (filename)  /* disk based metafile */
    {
        physDev->mh->mtType = METAFILE_DISK;
        if ((hFile = _lcreat32( filename, 0 )) == HFILE_ERROR32)
        {
            DeleteDC32( dc->hSelf );
            return 0;
        }
        if (_lwrite32( hFile, (LPSTR)physDev->mh,
                       sizeof(*physDev->mh)) == HFILE_ERROR32)
	{
            DeleteDC32( dc->hSelf );
            return 0;
	}
	physDev->mh->mtNoParameters = hFile; /* store file descriptor here */
	                            /* windows probably uses this too*/
    }
    else  /* memory based metafile */
	physDev->mh->mtType = METAFILE_MEMORY;

    dprintf_metafile( stddeb, "CreateMetaFile16: returning %04x\n", dc->hSelf);
    return dc->hSelf;
}


/******************************************************************
 *	     CloseMetafile16   (GDI.126)
 */
HMETAFILE16 CloseMetaFile16( HDC16 hdc )
{
    DC *dc;
    HMETAFILE16 hmf;
    HFILE32 hFile;
    METAFILEDRV_PDEVICE *physDev;
    
    dprintf_metafile( stddeb, "CloseMetaFile(%04x)\n", hdc );

    if (!(dc = DC_GetDCPtr( hdc ))) return 0;
    physDev = (METAFILEDRV_PDEVICE *)dc->physDev;

    /* Construct the end of metafile record - this is documented
     * in SDK Knowledgebase Q99334.
     */

    if (!MF_MetaParam0(dc, META_EOF))
    {
        DeleteDC32( hdc );
	return 0;
    }	

    if (physDev->mh->mtType == METAFILE_DISK)  /* disk based metafile */
    {
        hFile = physDev->mh->mtNoParameters;
	physDev->mh->mtNoParameters = 0;
        if (_llseek32(hFile, 0L, 0) == HFILE_ERROR32)
        {
            DeleteDC32( hdc );
            return 0;
        }
        if (_lwrite32( hFile, (LPSTR)physDev->mh,
                       sizeof(*physDev->mh)) == HFILE_ERROR32)
        {
            DeleteDC32( hdc );
            return 0;
        }
        _lclose32(hFile);
    }

    /* Now allocate a global handle for the metafile */

    hmf = GLOBAL_CreateBlock( GMEM_MOVEABLE, physDev->mh,
                              physDev->mh->mtSize * sizeof(WORD),
                              GetCurrentPDB(), FALSE, FALSE, FALSE, NULL );
    physDev->mh = NULL;  /* So it won't be deleted */
    DeleteDC32( hdc );
    return hmf;
}


/******************************************************************
 *	     DeleteMetafile16   (GDI.127)
 */
BOOL16 DeleteMetaFile16( HMETAFILE16 hmf )
{
    return !GlobalFree16( hmf );
}

