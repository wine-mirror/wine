/*
 * Enhanced MetaFile driver initialisation functions
 *
 * Copyright 1999 Huw D M Davies
 */

#include "windef.h"
#include "wingdi.h"
#include "dc.h"
#include "heap.h"
#include "global.h"
#include "enhmetafile.h"
#include "enhmetafiledrv.h"
#include "wine/winestring.h"
#include "debugtools.h"

#include <string.h>

DEFAULT_DEBUG_CHANNEL(enhmetafile)

static const DC_FUNCTIONS EMFDRV_Funcs =
{
    NULL,                            /* pAbortDoc */
    EMFDRV_AbortPath,                /* pAbortPath */
    NULL,                            /* pAngleArc */
    EMFDRV_Arc,                      /* pArc */
    NULL,                            /* pArcTo */
    EMFDRV_BeginPath,                /* pBeginPath */
    NULL,                            /* pBitBlt */
    NULL,                            /* pBitmapBits */	
    NULL,                            /* pChoosePixelFormat */
    EMFDRV_Chord,                    /* pChord */
    EMFDRV_CloseFigure,              /* pCloseFigure */
    NULL,                            /* pCreateBitmap */
    NULL, /* no implementation */    /* pCreateDC */
    NULL,                            /* pCreateDIBSection */
    NULL,                            /* pCreateDIBSection16 */
    NULL, /* no implementation */    /* pDeleteDC */
    NULL,                            /* pDeleteObject */
    NULL,                            /* pDescribePixelFormat */
    NULL,                            /* pDeviceCapabilities */
    EMFDRV_Ellipse,                  /* pEllipse */
    NULL,                            /* pEndDoc */
    NULL,                            /* pEndPage */
    EMFDRV_EndPath,                  /* pEndPath */
    NULL,                            /* pEnumDeviceFonts */
    NULL,                            /* pEscape */
    EMFDRV_ExcludeClipRect,          /* pExcludeClipRect */
    NULL,                            /* pExtDeviceMode */
    EMFDRV_ExtFloodFill,             /* pExtFloodFill */
    NULL,                            /* pExtTextOut */
    EMFDRV_FillPath,                 /* pFillPath */
    EMFDRV_FillRgn,                  /* pFillRgn */
    EMFDRV_FlattenPath,              /* pFlattenPath */
    EMFDRV_FrameRgn,                 /* pFrameRgn */
    NULL,                            /* pGetCharWidth */
    NULL,                            /* pGetDCOrgEx */
    NULL, /* no implementation */    /* pGetPixel */
    NULL,                            /* pGetPixelFormat */
    NULL,                            /* pGetTextExtentPoint */
    NULL,                            /* pGetTextMetrics */
    EMFDRV_IntersectClipRect,        /* pIntersectClipRect */
    EMFDRV_InvertRgn,                /* pInvertRgn */
    EMFDRV_LineTo,                   /* pLineTo */
    NULL,                            /* pLoadOEMResource */
    EMFDRV_MoveToEx,                 /* pMoveToEx */
    EMFDRV_OffsetClipRgn,            /* pOffsetClipRgn */
    NULL,                            /* pOffsetViewportOrg */
    NULL,                            /* pOffsetWindowOrg */
    EMFDRV_PaintRgn,                 /* pPaintRgn */
    NULL,                            /* pPatBlt */
    EMFDRV_Pie,                      /* pPie */
    NULL,                            /* pPolyBezier */
    NULL,                            /* pPolyBezierTo */
    NULL,                            /* pPolyDraw */
    EMFDRV_PolyPolygon,              /* pPolyPolygon */
    EMFDRV_PolyPolyline,             /* pPolyPolyline */
    EMFDRV_Polygon,                  /* pPolygon */
    EMFDRV_Polyline,                 /* pPolyline */
    NULL,                            /* pPolylineTo */
    NULL,                            /* pRealizePalette */
    EMFDRV_Rectangle,                /* pRectangle */
    EMFDRV_RestoreDC,                /* pRestoreDC */
    EMFDRV_RoundRect,                /* pRoundRect */
    EMFDRV_SaveDC,                   /* pSaveDC */
    EMFDRV_ScaleViewportExt,         /* pScaleViewportExt */
    EMFDRV_ScaleWindowExt,           /* pScaleWindowExt */
    EMFDRV_SelectClipPath,           /* pSelectClipPath */
    NULL,                            /* pSelectClipRgn */
    EMFDRV_SelectObject,             /* pSelectObject */
    NULL,                            /* pSelectPalette */
    EMFDRV_SetBkColor,               /* pSetBkColor */
    EMFDRV_SetBkMode,                /* pSetBkMode */
    NULL,                            /* pSetDeviceClipping */
    NULL,                            /* pSetDIBitsToDevice */
    EMFDRV_SetMapMode,               /* pSetMapMode */
    EMFDRV_SetMapperFlags,           /* pSetMapperFlags */
    NULL,                            /* pSetPixel */
    NULL,                            /* pSetPixelFormat */
    EMFDRV_SetPolyFillMode,          /* pSetPolyFillMode */
    EMFDRV_SetROP2,                  /* pSetROP2 */
    NULL,                            /* pSetRelAbs */
    EMFDRV_SetStretchBltMode,        /* pSetStretchBltMode */
    EMFDRV_SetTextAlign,             /* pSetTextAlign */
    NULL,                            /* pSetTextCharacterExtra */
    EMFDRV_SetTextColor,             /* pSetTextColor */
    NULL,                            /* pSetTextJustification */
    EMFDRV_SetViewportExt,           /* pSetViewportExt */
    EMFDRV_SetViewportOrg,           /* pSetViewportOrg */
    EMFDRV_SetWindowExt,             /* pSetWindowExt */
    EMFDRV_SetWindowOrg,             /* pSetWindowOrg */
    NULL,                            /* pStartDoc */
    NULL,                            /* pStartPage */
    NULL,                            /* pStretchBlt */
    NULL,                            /* pStretchDIBits */
    EMFDRV_StrokeAndFillPath,        /* pStrokeAndFillPath */
    EMFDRV_StrokePath,               /* pStrokePath */
    NULL,                            /* pSwapBuffers */
    EMFDRV_WidenPath                 /* pWiddenPath */
};


/**********************************************************************
 *	     EMFDRV_DeleteDC
 */
static BOOL EMFDRV_DeleteDC( DC *dc )
{
    EMFDRV_PDEVICE *physDev = (EMFDRV_PDEVICE *)dc->physDev;
    
    if (physDev->emh) HeapFree( GetProcessHeap(), 0, physDev->emh );
    HeapFree( GetProcessHeap(), 0, physDev );
    dc->physDev = NULL;
    GDI_FreeObject(dc->hSelf);
    return TRUE;
}


/******************************************************************
 *         EMFDRV_WriteRecord
 *
 * Warning: this function can change the pointer to the metafile header.
 */
BOOL EMFDRV_WriteRecord( DC *dc, EMR *emr )
{
    DWORD len;
    ENHMETAHEADER *emh;
    EMFDRV_PDEVICE *physDev = (EMFDRV_PDEVICE *)dc->physDev;

    physDev->emh->nBytes += emr->nSize;
    physDev->emh->nRecords++;

    if(physDev->hFile) {
        TRACE("Writing record to disk\n");
	if (!WriteFile(physDev->hFile, (char *)emr, emr->nSize, NULL, NULL))
	    return FALSE;
    } else {
	len = physDev->emh->nBytes;
	emh = HeapReAlloc( GetProcessHeap(), 0, physDev->emh, len );
        if (!emh) return FALSE;
        physDev->emh = emh;
	memcpy((CHAR *)physDev->emh + physDev->emh->nBytes - emr->nSize, emr,
	       emr->nSize);
    }
    return TRUE;
}


/******************************************************************
 *         EMFDRV_UpdateBBox
 */
void EMFDRV_UpdateBBox( DC *dc, RECTL *rect )
{
    EMFDRV_PDEVICE *physDev = (EMFDRV_PDEVICE *)dc->physDev;
    RECTL *bounds = &physDev->emh->rclBounds;

    if(bounds->left > bounds->right) {/* first rect */
        *bounds = *rect;
	return;
    }
    bounds->left   = min(bounds->left,   rect->left);
    bounds->top    = min(bounds->top,    rect->top);
    bounds->right  = max(bounds->right,  rect->right);
    bounds->bottom = max(bounds->bottom, rect->bottom);
    return;
}

/******************************************************************
 *         EMFDRV_AddHandleDC
 *
 * Note: this function assumes that we never delete objects.
 * If we do someday, we'll need to maintain a table to re-use deleted
 * handles.
 */
int EMFDRV_AddHandleDC( DC *dc )
{
    EMFDRV_PDEVICE *physDev = (EMFDRV_PDEVICE *)dc->physDev;
    physDev->emh->nHandles++;
    return physDev->nextHandle++;
}


/**********************************************************************
 *          CreateEnhMetaFileA   (GDI32.41)
*/
HDC WINAPI CreateEnhMetaFileA( 
    HDC hdc, /* optional reference DC */
    LPCSTR filename, /* optional filename for disk metafiles */
    const RECT *rect, /* optional bounding rectangle */
    LPCSTR description /* optional description */ 
    )
{
    LPWSTR filenameW = NULL;
    LPWSTR descriptionW = NULL;
    HDC hReturnDC;
    DWORD len1, len2;

    if(filename) 
        filenameW = HEAP_strdupAtoW( GetProcessHeap(), 0, filename );

    if(description) {
        len1 = strlen(description);
	len2 = strlen(description + len1 + 1);
	descriptionW = HeapAlloc( GetProcessHeap(), 0, (len1 + len2 + 3) * 2);
	lstrcpyAtoW(descriptionW, description );
	lstrcpyAtoW(descriptionW + len1 + 1 , description + len1 + 1);
	*(descriptionW + len1 + len2 + 2) = 0;
    }

    hReturnDC = CreateEnhMetaFileW(hdc, filenameW, rect, descriptionW);

    if(filenameW)
        HeapFree( GetProcessHeap(), 0, filenameW );
    if(descriptionW)
        HeapFree( GetProcessHeap(), 0, descriptionW );

    return hReturnDC;
}

/**********************************************************************
 *          CreateEnhMetaFileW   (GDI32.42)
 */
HDC WINAPI CreateEnhMetaFileW(
    HDC           hdc,        /* optional reference DC */
    LPCWSTR       filename,   /* optional filename for disk metafiles */
    const RECT*   rect,       /* optional bounding rectangle */
    LPCWSTR       description /* optional description */ 
    )
{
    DC *dc;
    HDC hRefDC = hdc ? hdc : CreateDCA("DISPLAY",NULL,NULL,NULL); /* If no ref, use current display */
    EMFDRV_PDEVICE *physDev;
    HFILE hFile;
    DWORD size = 0, length = 0;

    TRACE("'%s'\n", debugstr_w(filename) );

    if (!(dc = DC_AllocDC( &EMFDRV_Funcs ))) return 0;
    dc->header.wMagic = ENHMETAFILE_DC_MAGIC;

    physDev = (EMFDRV_PDEVICE *)HeapAlloc(GetProcessHeap(),0,sizeof(*physDev));
    if (!physDev) {
        GDI_HEAP_FREE( dc->hSelf );
        return 0;
    }
    dc->physDev = physDev;

    if(description) { /* App name\0Title\0\0 */
        length = lstrlenW(description);
	length += lstrlenW(description + length + 1);
	length += 3;
	length *= 2;
    }
    size = sizeof(ENHMETAHEADER) + (length + 3) / 4 * 4;

    if (!(physDev->emh = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size))) {
        HeapFree( GetProcessHeap(), 0, physDev );
        GDI_HEAP_FREE( dc->hSelf );
        return 0;
    }

    physDev->nextHandle = 1;
    physDev->hFile = 0;

    physDev->emh->iType = EMR_HEADER;
    physDev->emh->nSize = size;

    physDev->emh->rclBounds.left = physDev->emh->rclBounds.top = 0;
    physDev->emh->rclBounds.right = physDev->emh->rclBounds.bottom = -1;

    if(rect) {
        physDev->emh->rclFrame.left   = rect->left;
	physDev->emh->rclFrame.top    = rect->top;
	physDev->emh->rclFrame.right  = rect->right;
	physDev->emh->rclFrame.bottom = rect->bottom;
    } else {  /* Set this to {0,0 - -1,-1} and update it at the end */
        physDev->emh->rclFrame.left = physDev->emh->rclFrame.top = 0;
	physDev->emh->rclFrame.right = physDev->emh->rclFrame.bottom = -1;
    }

    physDev->emh->dSignature = ENHMETA_SIGNATURE;
    physDev->emh->nVersion = 0x10000;
    physDev->emh->nBytes = physDev->emh->nSize;
    physDev->emh->nRecords = 1;
    physDev->emh->nHandles = 1;

    physDev->emh->sReserved = 0; /* According to docs, this is reserved and must be 0 */
    physDev->emh->nDescription = length / 2;

    physDev->emh->offDescription = length ? sizeof(ENHMETAHEADER) : 0;

    physDev->emh->nPalEntries = 0; /* I guess this should start at 0 */
  
    /* Size in pixels */
    physDev->emh->szlDevice.cx = GetDeviceCaps( hRefDC, HORZRES );
    physDev->emh->szlDevice.cy = GetDeviceCaps( hRefDC, VERTRES );

    /* Size in millimeters */
    physDev->emh->szlMillimeters.cx = GetDeviceCaps( hRefDC, HORZSIZE );
    physDev->emh->szlMillimeters.cy = GetDeviceCaps( hRefDC, VERTSIZE );

    memcpy((char *)physDev->emh + sizeof(ENHMETAHEADER), description, length);

    if (filename)  /* disk based metafile */
    {
        if ((hFile = CreateFileW(filename, GENERIC_WRITE | GENERIC_READ, 0,
				 NULL, CREATE_ALWAYS, 0, -1)) == HFILE_ERROR) {
            EMFDRV_DeleteDC( dc );
            return 0;
        }
        if (!WriteFile( hFile, (LPSTR)physDev->emh, size, NULL, NULL )) {
            EMFDRV_DeleteDC( dc );
            return 0;
	}
	physDev->hFile = hFile;
    }

    if( !hdc )
      DeleteDC( hRefDC );
	
    TRACE("returning %04x\n", dc->hSelf);
    return dc->hSelf;

}

/******************************************************************
 *             CloseEnhMetaFile
 */
HENHMETAFILE WINAPI CloseEnhMetaFile( HDC hdc /* metafile DC */ )
{
    HENHMETAFILE hmf;
    EMFDRV_PDEVICE *physDev;
    DC *dc;
    EMREOF emr;
    HANDLE hMapping = 0;

    TRACE("(%04x)\n", hdc );

    if (!(dc = (DC *) GDI_GetObjPtr( hdc, ENHMETAFILE_DC_MAGIC ))) return 0;
    physDev = (EMFDRV_PDEVICE *)dc->physDev;

    emr.emr.iType = EMR_EOF;
    emr.emr.nSize = sizeof(emr);
    emr.nPalEntries = 0;
    emr.offPalEntries = 0;
    emr.nSizeLast = emr.emr.nSize;
    EMFDRV_WriteRecord( dc, &emr.emr );

    /* Update rclFrame if not initialized in CreateEnhMetaFile */
    if(physDev->emh->rclFrame.left > physDev->emh->rclFrame.right) {
        physDev->emh->rclFrame.left = physDev->emh->rclBounds.left *
	  physDev->emh->szlMillimeters.cx * 100 / physDev->emh->szlDevice.cx;
        physDev->emh->rclFrame.top = physDev->emh->rclBounds.top *
	  physDev->emh->szlMillimeters.cy * 100 / physDev->emh->szlDevice.cy;
        physDev->emh->rclFrame.right = physDev->emh->rclBounds.right *
	  physDev->emh->szlMillimeters.cx * 100 / physDev->emh->szlDevice.cx;
        physDev->emh->rclFrame.bottom = physDev->emh->rclBounds.bottom *
	  physDev->emh->szlMillimeters.cy * 100 / physDev->emh->szlDevice.cy;
    }

    if (physDev->hFile)  /* disk based metafile */
    {
        if (SetFilePointer(physDev->hFile, 0, NULL, FILE_BEGIN) != 0) {
            EMFDRV_DeleteDC( dc );
            return 0;
        }

        if (!WriteFile(physDev->hFile, (LPSTR)physDev->emh,
                       sizeof(*physDev->emh), NULL, NULL)) {
            EMFDRV_DeleteDC( dc );
            return 0;
        }
	HeapFree( GetProcessHeap(), 0, physDev->emh );
        hMapping = CreateFileMappingA(physDev->hFile, NULL, PAGE_READONLY, 0,
				      0, NULL);
	TRACE("hMapping = %08x\n", hMapping );
	physDev->emh = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
	TRACE("view = %p\n", physDev->emh );
    }


    hmf = EMF_Create_HENHMETAFILE( physDev->emh, physDev->hFile, hMapping );
    physDev->emh = NULL;  /* So it won't be deleted */
    EMFDRV_DeleteDC( dc );
    return hmf;
}


