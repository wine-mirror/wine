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
#include "debug.h"

#include <string.h>

static const DC_FUNCTIONS MFDRV_Funcs =
{
    MFDRV_Arc,                       /* pArc */
    MFDRV_BitBlt,                    /* pBitBlt */
    NULL,                            /* pBitmapBits */	
    MFDRV_Chord,                     /* pChord */
    NULL,                            /* pCreateBitmap */
    NULL, /* no implementation */    /* pCreateDC */
    NULL, /* no implementation */    /* pDeleteDC */
    NULL,                            /* pDeleteObject */
    MFDRV_Ellipse,                   /* pEllipse */
    NULL,                            /* pEnumDeviceFonts */
    NULL,                            /* pEscape */
    NULL,                            /* pExcludeClipRect */
    NULL,                            /* pExcludeVisRect */
    MFDRV_ExtFloodFill,              /* pExtFloodFill */
    MFDRV_ExtTextOut,                /* pExtTextOut */
    NULL,                            /* pGetCharWidth */
    NULL, /* no implementation */    /* pGetPixel */
    NULL,                            /* pGetTextExtentPoint */
    NULL,                            /* pGetTextMetrics */
    NULL,                            /* pIntersectClipRect */
    NULL,                            /* pIntersectVisRect */
    MFDRV_LineTo,                    /* pLineTo */
    NULL,                            /* pLoadOEMResource */
    MFDRV_MoveToEx,                  /* pMoveToEx */
    NULL,                            /* pOffsetClipRgn */
    MFDRV_OffsetViewportOrg,         /* pOffsetViewportOrg */
    MFDRV_OffsetWindowOrg,           /* pOffsetWindowOrg */
    MFDRV_PaintRgn,                  /* pPaintRgn */
    MFDRV_PatBlt,                    /* pPatBlt */
    MFDRV_Pie,                       /* pPie */
    MFDRV_PolyPolygon,               /* pPolyPolygon */
    NULL,                            /* pPolyPolyline */
    MFDRV_Polygon,                   /* pPolygon */
    MFDRV_Polyline,                  /* pPolyline */
    NULL,                            /* pPolyBezier */
    NULL,                            /* pRealizePalette */
    MFDRV_Rectangle,                 /* pRectangle */
    NULL,                            /* pRestoreDC */
    MFDRV_RoundRect,                 /* pRoundRect */
    NULL,                            /* pSaveDC */
    MFDRV_ScaleViewportExt,          /* pScaleViewportExt */
    MFDRV_ScaleWindowExt,            /* pScaleWindowExt */
    NULL,                            /* pSelectClipRgn */
    MFDRV_SelectObject,              /* pSelectObject */
    NULL,                            /* pSelectPalette */
    MFDRV_SetBkColor,                /* pSetBkColor */
    NULL,                            /* pSetBkMode */
    NULL,                            /* pSetDeviceClipping */
    NULL,                            /* pSetDIBitsToDevice */
    MFDRV_SetMapMode,                /* pSetMapMode */
    NULL,                            /* pSetMapperFlags */
    MFDRV_SetPixel,                  /* pSetPixel */
    NULL,                            /* pSetPolyFillMode */
    NULL,                            /* pSetROP2 */
    NULL,                            /* pSetRelAbs */
    NULL,                            /* pSetStretchBltMode */
    NULL,                            /* pSetTextAlign */
    NULL,                            /* pSetTextCharacterExtra */
    MFDRV_SetTextColor,              /* pSetTextColor */
    NULL,                            /* pSetTextJustification */
    MFDRV_SetViewportExt,            /* pSetViewportExt */
    MFDRV_SetViewportOrg,            /* pSetViewportOrg */
    MFDRV_SetWindowExt,              /* pSetWindowExt */
    MFDRV_SetWindowOrg,              /* pSetWindowOrg */
    MFDRV_StretchBlt,                /* pStretchBlt */
    NULL                             /* pStretchDIBits */
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
    GDI_FreeObject(dc->hSelf);
    return TRUE;
}


/**********************************************************************
 *	     CreateMetaFile16   (GDI.125)
 *
 *  Create a new DC and associate it with a metafile. Pass a filename
 *  to create a disk-based metafile, NULL to create a memory metafile.
 *
 * RETURNS
 *  A handle to the metafile DC if successful, NULL on failure.
 */
HDC16 WINAPI CreateMetaFile16( 
			      LPCSTR filename /* Filename of disk metafile */
)
{
    DC *dc;
    METAFILEDRV_PDEVICE *physDev;
    HFILE32 hFile;

    TRACE(metafile, "'%s'\n", filename );

    if (!(dc = MFDRV_AllocMetaFile())) return 0;
    physDev = (METAFILEDRV_PDEVICE *)dc->physDev;
    
    if (filename)  /* disk based metafile */
    {
        physDev->mh->mtType = METAFILE_DISK;
        if ((hFile = _lcreat32( filename, 0 )) == HFILE_ERROR32)
        {
            MFDRV_DeleteDC( dc );
            return 0;
        }
        if (_lwrite32( hFile, (LPSTR)physDev->mh,
                       sizeof(*physDev->mh)) == HFILE_ERROR32)
	{
            MFDRV_DeleteDC( dc );
            return 0;
	}
	physDev->mh->mtNoParameters = hFile; /* store file descriptor here */
	                            /* windows probably uses this too*/
    }
    else  /* memory based metafile */
	physDev->mh->mtType = METAFILE_MEMORY;

    TRACE(metafile, "returning %04x\n", dc->hSelf);
    return dc->hSelf;
}

/**********************************************************************
 *	     CreateMetaFile32A   (GDI32.51)
 */
HDC32 WINAPI CreateMetaFile32A( 
			      LPCSTR filename /* Filename of disk metafile */
)
{
  return CreateMetaFile16( filename );
}

/**********************************************************************
 *          CreateMetaFile32W   (GDI32.52)
 */
HDC32 WINAPI CreateMetaFile32W(LPCWSTR filename)
{
    LPSTR filenameA;
    HDC32 hReturnDC;

    filenameA = HEAP_strdupWtoA( GetProcessHeap(), 0, filename );

    hReturnDC = CreateMetaFile32A(filenameA);

    HeapFree( GetProcessHeap(), 0, filenameA );

    return hReturnDC;
}

static DC *METAFILE_CloseMetaFile( HDC32 hdc ) 
{
    DC *dc;
    HFILE32 hFile;
    METAFILEDRV_PDEVICE *physDev;
    
    TRACE(metafile, "(%04x)\n", hdc );

    if (!(dc = (DC *) GDI_GetObjPtr( hdc, METAFILE_DC_MAGIC ))) return 0;
    physDev = (METAFILEDRV_PDEVICE *)dc->physDev;

    /* Construct the end of metafile record - this is documented
     * in SDK Knowledgebase Q99334.
     */

    if (!MF_MetaParam0(dc, META_EOF))
    {
        MFDRV_DeleteDC( dc );
	return 0;
    }	

    if (physDev->mh->mtType == METAFILE_DISK)  /* disk based metafile */
    {
        hFile = physDev->mh->mtNoParameters;
	physDev->mh->mtNoParameters = 0;
        if (_llseek32(hFile, 0L, 0) == HFILE_ERROR32)
        {
            MFDRV_DeleteDC( dc );
            return 0;
        }
        if (_lwrite32( hFile, (LPSTR)physDev->mh,
                       sizeof(*physDev->mh)) == HFILE_ERROR32)
        {
            MFDRV_DeleteDC( dc );
            return 0;
        }
        _lclose32(hFile);
    }

    return dc;
}

/******************************************************************
 *	     CloseMetaFile16   (GDI.126)
 */
HMETAFILE16 WINAPI CloseMetaFile16( 
				   HDC16 hdc /* Metafile DC to close */
)
{
    HMETAFILE16 hmf;
    METAFILEDRV_PDEVICE *physDev;
    DC *dc = METAFILE_CloseMetaFile(hdc);
    if (!dc) return 0;
    physDev = (METAFILEDRV_PDEVICE *)dc->physDev;

    /* Now allocate a global handle for the metafile */

    hmf = GLOBAL_CreateBlock( GMEM_MOVEABLE, physDev->mh,
                              physDev->mh->mtSize * sizeof(WORD),
                              GetCurrentPDB(), FALSE, FALSE, FALSE, NULL );
    physDev->mh = NULL;  /* So it won't be deleted */
    MFDRV_DeleteDC( dc );
    return hmf;
}

/******************************************************************
 *	     CloseMetaFile32   (GDI32.17)
 *
 *  Stop recording graphics operations in metafile associated with
 *  hdc and retrieve metafile.
 *
 * RETURNS
 *  Handle of newly created metafile on success, NULL on failure.
 */
HMETAFILE32 WINAPI CloseMetaFile32( 
				   HDC32 hdc /* Metafile DC to close */
)
{
  return CloseMetaFile16(hdc);
}


/******************************************************************
 *	     DeleteMetaFile16   (GDI.127)
 */
BOOL16 WINAPI DeleteMetaFile16( 
			       HMETAFILE16 hmf 
			       /* Handle of memory metafile to delete */
)
{
    return !GlobalFree16( hmf );
}

/******************************************************************
 *          DeleteMetaFile32  (GDI32.69)
 *
 *  Delete a memory-based metafile.
 */

BOOL32 WINAPI DeleteMetaFile32(
	      HMETAFILE32 hmf
) {
  return !GlobalFree16( hmf );
}

/********************************************************************

       Enhanced Metafile driver initializations

       This possibly  should be moved to their own file/directory
       at some point.

**********************************************************************/

/**********************************************************************
 *          CreateEnhMetaFile32A   (GDI32.41)
*/
HDC32 WINAPI CreateEnhMetaFile32A( 
    HDC32 hdc, /* optional reference DC */
    LPCSTR filename, /* optional filename for disk metafiles */
    const RECT32 *rect, /* optional bounding rectangle */
    LPCSTR description /* optional description */ 
    )
{
#if 0
    DC *dc;
    METAFILEDRV_PDEVICE *physDev;
    HFILE32 hFile;

    if (!(dc = MFDRV_AllocMetaFile())) return 0;
    physDev = (METAFILEDRV_PDEVICE *)dc->physDev;
    
    if (filename)  /* disk based metafile */
    {
        physDev->mh->mtType = METAFILE_DISK;
        if ((hFile = _lcreat32( filename, 0 )) == HFILE_ERROR32)
        {
            MFDRV_DeleteDC( dc );
            return 0;
        }
        if (_lwrite32( hFile, (LPSTR)physDev->mh,
                       sizeof(*physDev->mh)) == HFILE_ERROR32)
	{
            MFDRV_DeleteDC( dc );
            return 0;
	}
	physDev->mh->mtNoParameters = hFile; /* store file descriptor here */
	                            /* windows probably uses this too*/
    }
    else  /* memory based metafile */
	physDev->mh->mtType = METAFILE_MEMORY;

    TRACE(metafile, "returning %04x\n", dc->hSelf);
    return dc->hSelf;
#endif

    FIXME(metafile,
         "(0x%lx,%s,%p,%s): stub\n",
         hdc,
         filename,
         rect,
         description);

    return 0;
}

/**********************************************************************
 *          CreateEnhMetaFile32W   (GDI32.42)
 */
HDC32 WINAPI CreateEnhMetaFile32W(
    HDC32         hdc,        /* optional reference DC */
    LPCWSTR       filename,   /* optional filename for disk metafiles */
    const RECT32* rect,       /* optional bounding rectangle */
    LPCWSTR       description /* optional description */ 
    )
{
    LPSTR filenameA;
    LPSTR descriptionA;
    HDC32 hReturnDC;

    filenameA    = HEAP_strdupWtoA( GetProcessHeap(), 0, filename );
    descriptionA = HEAP_strdupWtoA( GetProcessHeap(), 0, description );

    hReturnDC = CreateEnhMetaFile32A(hdc,
                                    filenameA,
                                    rect,
                                    descriptionA);

    HeapFree( GetProcessHeap(), 0, filenameA );
    HeapFree( GetProcessHeap(), 0, descriptionA );

    return hReturnDC;
}

HENHMETAFILE32 WINAPI CloseEnhMetaFile( HDC32 hdc /* metafile DC */ )
{
  /* write EMR_EOF(0x0, 0x10, 0x14) */
  return 0;
}


