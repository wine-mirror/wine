/*
 * Enhanced MetaFile driver initialisation functions
 *
 * Copyright 1999 Huw D M Davies
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "ntgdi_private.h"
#include "enhmfdrv/enhmetafiledrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(enhmetafile);

static BOOL CDECL EMFDRV_DeleteDC( PHYSDEV dev );

static const struct gdi_dc_funcs emfdrv_driver =
{
    NULL,                            /* pAbortDoc */
    NULL,                            /* pAbortPath */
    EMFDRV_AlphaBlend,               /* pAlphaBlend */
    NULL,                            /* pAngleArc */
    EMFDRV_Arc,                      /* pArc */
    EMFDRV_ArcTo,                    /* pArcTo */
    NULL,                            /* pBeginPath */
    NULL,                            /* pBlendImage */
    NULL,                            /* pChord */
    NULL,                            /* pCloseFigure */
    NULL,                            /* pCreateCompatibleDC */
    NULL,                            /* pCreateDC */
    EMFDRV_DeleteDC,                 /* pDeleteDC */
    EMFDRV_DeleteObject,             /* pDeleteObject */
    NULL,                            /* pDeviceCapabilities */
    EMFDRV_Ellipse,                  /* pEllipse */
    NULL,                            /* pEndDoc */
    NULL,                            /* pEndPage */
    NULL,                            /* pEndPath */
    NULL,                            /* pEnumFonts */
    NULL,                            /* pEnumICMProfiles */
    EMFDRV_ExcludeClipRect,          /* pExcludeClipRect */
    NULL,                            /* pExtDeviceMode */
    NULL,                            /* pExtEscape */
    NULL,                            /* pExtFloodFill */
    EMFDRV_ExtSelectClipRgn,         /* pExtSelectClipRgn */
    EMFDRV_ExtTextOut,               /* pExtTextOut */
    EMFDRV_FillPath,                 /* pFillPath */
    EMFDRV_FillRgn,                  /* pFillRgn */
    EMFDRV_FlattenPath,              /* pFlattenPath */
    NULL,                            /* pFontIsLinked */
    EMFDRV_FrameRgn,                 /* pFrameRgn */
    EMFDRV_GdiComment,               /* pGdiComment */
    NULL,                            /* pGetBoundsRect */
    NULL,                            /* pGetCharABCWidths */
    NULL,                            /* pGetCharABCWidthsI */
    NULL,                            /* pGetCharWidth */
    NULL,                            /* pGetCharWidthInfo */
    EMFDRV_GetDeviceCaps,            /* pGetDeviceCaps */
    NULL,                            /* pGetDeviceGammaRamp */
    NULL,                            /* pGetFontData */
    NULL,                            /* pGetFontRealizationInfo */
    NULL,                            /* pGetFontUnicodeRanges */
    NULL,                            /* pGetGlyphIndices */
    NULL,                            /* pGetGlyphOutline */
    NULL,                            /* pGetICMProfile */
    NULL,                            /* pGetImage */
    NULL,                            /* pGetKerningPairs */
    NULL,                            /* pGetNearestColor */
    NULL,                            /* pGetOutlineTextMetrics */
    NULL,                            /* pGetPixel */
    NULL,                            /* pGetSystemPaletteEntries */
    NULL,                            /* pGetTextCharsetInfo */
    NULL,                            /* pGetTextExtentExPoint */
    NULL,                            /* pGetTextExtentExPointI */
    NULL,                            /* pGetTextFace */
    NULL,                            /* pGetTextMetrics */
    EMFDRV_GradientFill,             /* pGradientFill */
    EMFDRV_IntersectClipRect,        /* pIntersectClipRect */
    EMFDRV_InvertRgn,                /* pInvertRgn */
    EMFDRV_LineTo,                   /* pLineTo */
    EMFDRV_ModifyWorldTransform,     /* pModifyWorldTransform */
    NULL,                            /* pMoveTo */
    EMFDRV_OffsetClipRgn,            /* pOffsetClipRgn */
    EMFDRV_OffsetViewportOrgEx,      /* pOffsetViewportOrgEx */
    EMFDRV_OffsetWindowOrgEx,        /* pOffsetWindowOrgEx */
    NULL,                            /* pPaintRgn */
    EMFDRV_PatBlt,                   /* pPatBlt */
    EMFDRV_Pie,                      /* pPie */
    EMFDRV_PolyBezier,               /* pPolyBezier */
    EMFDRV_PolyBezierTo,             /* pPolyBezierTo */
    EMFDRV_PolyDraw,                 /* pPolyDraw */
    EMFDRV_PolyPolygon,              /* pPolyPolygon */
    EMFDRV_PolyPolyline,             /* pPolyPolyline */
    EMFDRV_PolylineTo,               /* pPolylineTo */
    NULL,                            /* pPutImage */
    NULL,                            /* pRealizeDefaultPalette */
    NULL,                            /* pRealizePalette */
    EMFDRV_Rectangle,                /* pRectangle */
    NULL,                            /* pResetDC */
    EMFDRV_RestoreDC,                /* pRestoreDC */
    EMFDRV_RoundRect,                /* pRoundRect */
    EMFDRV_ScaleViewportExtEx,       /* pScaleViewportExtEx */
    EMFDRV_ScaleWindowExtEx,         /* pScaleWindowExtEx */
    EMFDRV_SelectBitmap,             /* pSelectBitmap */
    EMFDRV_SelectBrush,              /* pSelectBrush */
    EMFDRV_SelectClipPath,           /* pSelectClipPath */
    EMFDRV_SelectFont,               /* pSelectFont */
    EMFDRV_SelectPalette,            /* pSelectPalette */
    EMFDRV_SelectPen,                /* pSelectPen */
    EMFDRV_SetBkColor,               /* pSetBkColor */
    NULL,                            /* pSetBoundsRect */
    EMFDRV_SetDCBrushColor,          /* pSetDCBrushColor*/
    EMFDRV_SetDCPenColor,            /* pSetDCPenColor*/
    EMFDRV_SetDIBitsToDevice,        /* pSetDIBitsToDevice */
    NULL,                            /* pSetDeviceClipping */
    NULL,                            /* pSetDeviceGammaRamp */
    EMFDRV_SetLayout,                /* pSetLayout */
    EMFDRV_SetMapMode,               /* pSetMapMode */
    EMFDRV_SetMapperFlags,           /* pSetMapperFlags */
    EMFDRV_SetPixel,                 /* pSetPixel */
    NULL,                            /* pSetTextCharacterExtra */
    EMFDRV_SetTextColor,             /* pSetTextColor */
    EMFDRV_SetTextJustification,     /* pSetTextJustification */ 
    EMFDRV_SetViewportExtEx,         /* pSetViewportExtEx */
    EMFDRV_SetViewportOrgEx,         /* pSetViewportOrgEx */
    EMFDRV_SetWindowExtEx,           /* pSetWindowExtEx */
    EMFDRV_SetWindowOrgEx,           /* pSetWindowOrgEx */
    EMFDRV_SetWorldTransform,        /* pSetWorldTransform */
    NULL,                            /* pStartDoc */
    NULL,                            /* pStartPage */
    EMFDRV_StretchBlt,               /* pStretchBlt */
    EMFDRV_StretchDIBits,            /* pStretchDIBits */
    EMFDRV_StrokeAndFillPath,        /* pStrokeAndFillPath */
    EMFDRV_StrokePath,               /* pStrokePath */
    NULL,                            /* pUnrealizePalette */
    EMFDRV_WidenPath,                /* pWidenPath */
    NULL,                            /* pD3DKMTCheckVidPnExclusiveOwnership */
    NULL,                            /* pD3DKMTSetVidPnSourceOwner */
    NULL,                            /* wine_get_wgl_driver */
    NULL,                            /* wine_get_vulkan_driver */
    GDI_PRIORITY_GRAPHICS_DRV        /* priority */
};


/**********************************************************************
 *	     EMFDRV_DeleteDC
 */
static BOOL CDECL EMFDRV_DeleteDC( PHYSDEV dev )
{
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );
    UINT index;

    HeapFree( GetProcessHeap(), 0, physDev->emh );
    for(index = 0; index < physDev->handles_size; index++)
        if(physDev->handles[index])
	    GDI_hdc_not_using_object(physDev->handles[index], dev->hdc);
    HeapFree( GetProcessHeap(), 0, physDev->handles );
    HeapFree( GetProcessHeap(), 0, physDev );
    return TRUE;
}


/******************************************************************
 *         EMFDRV_WriteRecord
 *
 * Warning: this function can change the pointer to the metafile header.
 */
BOOL EMFDRV_WriteRecord( PHYSDEV dev, EMR *emr )
{
    DWORD len, size;
    ENHMETAHEADER *emh;
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );

    TRACE("record %d, size %d %s\n",
          emr->iType, emr->nSize, physDev->hFile ? "(to disk)" : "");

    assert( !(emr->nSize & 3) );

    physDev->emh->nBytes += emr->nSize;
    physDev->emh->nRecords++;

    size = HeapSize(GetProcessHeap(), 0, physDev->emh);
    len = physDev->emh->nBytes;
    if (len > size) {
        size += (size / 2) + emr->nSize;
        emh = HeapReAlloc(GetProcessHeap(), 0, physDev->emh, size);
        if (!emh) return FALSE;
        physDev->emh = emh;
    }
    memcpy((CHAR *)physDev->emh + physDev->emh->nBytes - emr->nSize, emr,
           emr->nSize);
    return TRUE;
}


/******************************************************************
 *         EMFDRV_UpdateBBox
 */
void EMFDRV_UpdateBBox( PHYSDEV dev, RECTL *rect )
{
    EMFDRV_PDEVICE *physDev = get_emf_physdev( dev );
    RECTL *bounds = &physDev->emh->rclBounds;
    RECTL vportRect = *rect;

    LPtoDP( dev->hdc, (LPPOINT)&vportRect, 2 );

    /* The coordinate systems may be mirrored
       (LPtoDP handles points, not rectangles) */
    if (vportRect.left > vportRect.right)
    {
        LONG temp = vportRect.right;
        vportRect.right = vportRect.left;
        vportRect.left = temp;
    }
    if (vportRect.top > vportRect.bottom)
    {
        LONG temp = vportRect.bottom;
        vportRect.bottom = vportRect.top;
        vportRect.top = temp;
    }

    if (bounds->left > bounds->right)
    {
        /* first bounding rectangle */
        *bounds = vportRect;
    }
    else
    {
        bounds->left   = min(bounds->left,   vportRect.left);
        bounds->top    = min(bounds->top,    vportRect.top);
        bounds->right  = max(bounds->right,  vportRect.right);
        bounds->bottom = max(bounds->bottom, vportRect.bottom);
    }
}

/**********************************************************************
 *          CreateEnhMetaFileA   (GDI32.@)
 */
HDC WINAPI CreateEnhMetaFileA(
    HDC hdc,           /* [in] optional reference DC */
    LPCSTR filename,   /* [in] optional filename for disk metafiles */
    const RECT *rect,  /* [in] optional bounding rectangle */
    LPCSTR description /* [in] optional description */
    )
{
    LPWSTR filenameW = NULL;
    LPWSTR descriptionW = NULL;
    HDC hReturnDC;
    DWORD len1, len2, total;

    if(filename)
    {
        total = MultiByteToWideChar( CP_ACP, 0, filename, -1, NULL, 0 );
        filenameW = HeapAlloc( GetProcessHeap(), 0, total * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, filename, -1, filenameW, total );
    }
    if(description) {
        len1 = strlen(description);
	len2 = strlen(description + len1 + 1);
        total = MultiByteToWideChar( CP_ACP, 0, description, len1 + len2 + 3, NULL, 0 );
	descriptionW = HeapAlloc( GetProcessHeap(), 0, total * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, description, len1 + len2 + 3, descriptionW, total );
    }

    hReturnDC = CreateEnhMetaFileW(hdc, filenameW, rect, descriptionW);

    HeapFree( GetProcessHeap(), 0, filenameW );
    HeapFree( GetProcessHeap(), 0, descriptionW );

    return hReturnDC;
}

static inline BOOL devcap_is_valid( int cap )
{
    if (cap >= 0 && cap <= ASPECTXY) return !(cap & 1);
    if (cap >= PHYSICALWIDTH && cap <= COLORMGMTCAPS) return TRUE;
    switch (cap)
    {
    case LOGPIXELSX:
    case LOGPIXELSY:
    case CAPS1:
    case SIZEPALETTE:
    case NUMRESERVED:
    case COLORRES:
        return TRUE;
    }
    return FALSE;
}

/**********************************************************************
 *          CreateEnhMetaFileW   (GDI32.@)
 */
HDC WINAPI CreateEnhMetaFileW(
    HDC           hdc,        /* [in] optional reference DC */
    LPCWSTR       filename,   /* [in] optional filename for disk metafiles */
    const RECT*   rect,       /* [in] optional bounding rectangle */
    LPCWSTR       description /* [in] optional description */
    )
{
    HDC ret, ref_dc;
    DC *dc;
    EMFDRV_PDEVICE *physDev;
    HANDLE hFile;
    DWORD size = 0, length = 0;
    int cap;

    TRACE("(%p %s %s %s)\n", hdc, debugstr_w(filename), wine_dbgstr_rect(rect), debugstr_w(description) );

    if (!(dc = alloc_dc_ptr( NTGDI_OBJ_ENHMETADC ))) return 0;

    physDev = HeapAlloc(GetProcessHeap(),0,sizeof(*physDev));
    if (!physDev) {
        free_dc_ptr( dc );
        return 0;
    }
    dc->attr->emf = physDev;
    if(description) { /* App name\0Title\0\0 */
        length = lstrlenW(description);
	length += lstrlenW(description + length + 1);
	length += 3;
	length *= 2;
    }
    size = sizeof(ENHMETAHEADER) + (length + 3) / 4 * 4;

    if (!(physDev->emh = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size))) {
        HeapFree( GetProcessHeap(), 0, physDev );
        free_dc_ptr( dc );
        return 0;
    }

    push_dc_driver( &dc->physDev, &physDev->dev, &emfdrv_driver );

    physDev->handles = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, HANDLE_LIST_INC * sizeof(physDev->handles[0]));
    physDev->handles_size = HANDLE_LIST_INC;
    physDev->cur_handles = 1;
    physDev->hFile = 0;
    physDev->dc_brush = 0;
    physDev->dc_pen = 0;
    physDev->restoring = 0;
    physDev->modifying_transform = 0;
    physDev->path = FALSE;

    if (hdc)  /* if no ref, use current display */
        ref_dc = hdc;
    else
        ref_dc = CreateDCW( L"DISPLAY", NULL, NULL, NULL );

    memset( physDev->dev_caps, 0, sizeof(physDev->dev_caps) );
    for (cap = 0; cap < ARRAY_SIZE( physDev->dev_caps ); cap++)
        if (devcap_is_valid( cap ))
            physDev->dev_caps[cap] = GetDeviceCaps( ref_dc, cap );

    if (!hdc) DeleteDC( ref_dc );

    SetVirtualResolution(physDev->dev.hdc, 0, 0, 0, 0);

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
    physDev->emh->szlDevice.cx = physDev->dev_caps[HORZRES];
    physDev->emh->szlDevice.cy = physDev->dev_caps[VERTRES];

    /* Size in millimeters */
    physDev->emh->szlMillimeters.cx = physDev->dev_caps[HORZSIZE];
    physDev->emh->szlMillimeters.cy = physDev->dev_caps[VERTSIZE];

    /* Size in micrometers */
    physDev->emh->szlMicrometers.cx = physDev->emh->szlMillimeters.cx * 1000;
    physDev->emh->szlMicrometers.cy = physDev->emh->szlMillimeters.cy * 1000;

    memcpy((char *)physDev->emh + sizeof(ENHMETAHEADER), description, length);

    if (filename)  /* disk based metafile */
    {
        if ((hFile = CreateFileW(filename, GENERIC_WRITE | GENERIC_READ, 0,
				 NULL, CREATE_ALWAYS, 0, 0)) == INVALID_HANDLE_VALUE) {
            free_dc_ptr( dc );
            return 0;
        }
	physDev->hFile = hFile;
    }

    TRACE("returning %p\n", physDev->dev.hdc);
    ret = physDev->dev.hdc;
    release_dc_ptr( dc );

    return ret;
}

/******************************************************************
 *             CloseEnhMetaFile (GDI32.@)
 */
HENHMETAFILE WINAPI CloseEnhMetaFile(HDC hdc) /* [in] metafile DC */
{
    HENHMETAFILE hmf;
    EMFDRV_PDEVICE *physDev;
    DC *dc;
    EMREOF emr;
    HANDLE hMapping = 0;

    TRACE("(%p)\n", hdc );

    if (!(dc = get_dc_ptr( hdc ))) return NULL;
    if (GetObjectType( hdc ) != OBJ_ENHMETADC)
    {
        release_dc_ptr( dc );
        return NULL;
    }
    if (dc->refcount != 1)
    {
        FIXME( "not deleting busy DC %p refcount %u\n", hdc, dc->refcount );
        release_dc_ptr( dc );
        return NULL;
    }
    physDev = get_emf_physdev( find_dc_driver( dc, &emfdrv_driver ));

    if(dc->saveLevel)
        RestoreDC(hdc, 1);

    if (physDev->dc_brush) DeleteObject( physDev->dc_brush );
    if (physDev->dc_pen) DeleteObject( physDev->dc_pen );

    emr.emr.iType = EMR_EOF;
    emr.emr.nSize = sizeof(emr);
    emr.nPalEntries = 0;
    emr.offPalEntries = FIELD_OFFSET(EMREOF, nSizeLast);
    emr.nSizeLast = emr.emr.nSize;
    EMFDRV_WriteRecord( &physDev->dev, &emr.emr );

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
        if (!WriteFile(physDev->hFile, physDev->emh, physDev->emh->nBytes,
                       NULL, NULL))
        {
            CloseHandle( physDev->hFile );
            free_dc_ptr( dc );
            return 0;
        }
	HeapFree( GetProcessHeap(), 0, physDev->emh );
        hMapping = CreateFileMappingA(physDev->hFile, NULL, PAGE_READONLY, 0,
				      0, NULL);
	TRACE("hMapping = %p\n", hMapping );
	physDev->emh = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
	TRACE("view = %p\n", physDev->emh );
        CloseHandle( hMapping );
        CloseHandle( physDev->hFile );
    }

    hmf = EMF_Create_HENHMETAFILE( physDev->emh, physDev->emh->nBytes, (physDev->hFile != 0) );
    physDev->emh = NULL;  /* So it won't be deleted */
    free_dc_ptr( dc );
    return hmf;
}
