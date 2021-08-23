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
    NULL,                            /* pDeleteObject */
    NULL,                            /* pDeviceCapabilities */
    EMFDRV_Ellipse,                  /* pEllipse */
    NULL,                            /* pEndDoc */
    NULL,                            /* pEndPage */
    NULL,                            /* pEndPath */
    NULL,                            /* pEnumFonts */
    NULL,                            /* pEnumICMProfiles */
    NULL,                            /* pExtDeviceMode */
    NULL,                            /* pExtEscape */
    NULL,                            /* pExtFloodFill */
    EMFDRV_ExtTextOut,               /* pExtTextOut */
    EMFDRV_FillPath,                 /* pFillPath */
    EMFDRV_FillRgn,                  /* pFillRgn */
    NULL,                            /* pFontIsLinked */
    EMFDRV_FrameRgn,                 /* pFrameRgn */
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
    EMFDRV_InvertRgn,                /* pInvertRgn */
    EMFDRV_LineTo,                   /* pLineTo */
    NULL,                            /* pMoveTo */
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
    EMFDRV_RoundRect,                /* pRoundRect */
    EMFDRV_SelectBitmap,             /* pSelectBitmap */
    NULL,                            /* pSelectBrush */
    EMFDRV_SelectFont,               /* pSelectFont */
    NULL,                            /* pSelectPen */
    NULL,                            /* pSetBkColor */
    NULL,                            /* pSetBoundsRect */
    NULL,                            /* pSetDCBrushColor*/
    NULL,                            /* pSetDCPenColor*/
    EMFDRV_SetDIBitsToDevice,        /* pSetDIBitsToDevice */
    NULL,                            /* pSetDeviceClipping */
    NULL,                            /* pSetDeviceGammaRamp */
    EMFDRV_SetPixel,                 /* pSetPixel */
    NULL,                            /* pSetTextColor */
    NULL,                            /* pStartDoc */
    NULL,                            /* pStartPage */
    NULL,                            /* pStretchBlt */
    EMFDRV_StretchDIBits,            /* pStretchDIBits */
    EMFDRV_StrokeAndFillPath,        /* pStrokeAndFillPath */
    EMFDRV_StrokePath,               /* pStrokePath */
    NULL,                            /* pUnrealizePalette */
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
 *           NtGdiCreateMetafileDC   (win32u.@)
 */
HDC WINAPI NtGdiCreateMetafileDC( HDC hdc )
{
    EMFDRV_PDEVICE *physDev;
    HDC ref_dc, ret;
    int cap;
    DC *dc;

    if (!(dc = alloc_dc_ptr( NTGDI_OBJ_ENHMETADC ))) return 0;

    physDev = HeapAlloc( GetProcessHeap(), 0, sizeof(*physDev) );
    if (!physDev)
    {
        free_dc_ptr( dc );
        return 0;
    }
    dc->attr->emf = physDev;

    push_dc_driver( &dc->physDev, &physDev->dev, &emfdrv_driver );

    if (hdc)  /* if no ref, use current display */
        ref_dc = hdc;
    else
        ref_dc = CreateDCW( L"DISPLAY", NULL, NULL, NULL );

    memset( physDev->dev_caps, 0, sizeof(physDev->dev_caps) );
    for (cap = 0; cap < ARRAY_SIZE( physDev->dev_caps ); cap++)
        if (devcap_is_valid( cap ))
            physDev->dev_caps[cap] = NtGdiGetDeviceCaps( ref_dc, cap );

    if (!hdc) NtGdiDeleteObjectApp( ref_dc );

    NtGdiSetVirtualResolution( dc->hSelf, 0, 0, 0, 0 );

    ret = dc->hSelf;
    release_dc_ptr( dc );
    return ret;
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
    HDC ret;
    EMFDRV_PDEVICE *emf;
    DC_ATTR *dc_attr;
    HANDLE hFile;
    DWORD size = 0, length = 0;

    TRACE("(%p %s %s %s)\n", hdc, debugstr_w(filename), wine_dbgstr_rect(rect), debugstr_w(description) );

    if (!(ret = NtGdiCreateMetafileDC( hdc ))) return 0;

    if (!(dc_attr = get_dc_attr( ret )))
    {
        DeleteDC( ret );
        return 0;
    }
    emf = dc_attr->emf;

    if(description) { /* App name\0Title\0\0 */
        length = lstrlenW(description);
	length += lstrlenW(description + length + 1);
	length += 3;
	length *= 2;
    }
    size = sizeof(ENHMETAHEADER) + (length + 3) / 4 * 4;

    if (!(emf->emh = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size)))
    {
        DeleteDC( ret );
        return 0;
    }

    emf->handles = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                              HANDLE_LIST_INC * sizeof(emf->handles[0]) );
    emf->handles_size = HANDLE_LIST_INC;
    emf->cur_handles = 1;
    emf->hFile = 0;
    emf->dc_brush = 0;
    emf->dc_pen = 0;
    emf->path = FALSE;

    emf->emh->iType = EMR_HEADER;
    emf->emh->nSize = size;

    emf->emh->rclBounds.left = emf->emh->rclBounds.top = 0;
    emf->emh->rclBounds.right = emf->emh->rclBounds.bottom = -1;

    if (rect)
    {
        emf->emh->rclFrame.left   = rect->left;
        emf->emh->rclFrame.top    = rect->top;
        emf->emh->rclFrame.right  = rect->right;
        emf->emh->rclFrame.bottom = rect->bottom;
    }
    else
    {
        /* Set this to {0,0 - -1,-1} and update it at the end */
        emf->emh->rclFrame.left = emf->emh->rclFrame.top = 0;
        emf->emh->rclFrame.right = emf->emh->rclFrame.bottom = -1;
    }

    emf->emh->dSignature = ENHMETA_SIGNATURE;
    emf->emh->nVersion = 0x10000;
    emf->emh->nBytes = emf->emh->nSize;
    emf->emh->nRecords = 1;
    emf->emh->nHandles = 1;

    emf->emh->sReserved = 0; /* According to docs, this is reserved and must be 0 */
    emf->emh->nDescription = length / 2;

    emf->emh->offDescription = length ? sizeof(ENHMETAHEADER) : 0;

    emf->emh->nPalEntries = 0; /* I guess this should start at 0 */

    /* Size in pixels */
    emf->emh->szlDevice.cx = GetDeviceCaps( ret, HORZRES );
    emf->emh->szlDevice.cy = GetDeviceCaps( ret, VERTRES );

    /* Size in millimeters */
    emf->emh->szlMillimeters.cx = GetDeviceCaps( ret, HORZSIZE );
    emf->emh->szlMillimeters.cy = GetDeviceCaps( ret, VERTSIZE );

    /* Size in micrometers */
    emf->emh->szlMicrometers.cx = emf->emh->szlMillimeters.cx * 1000;
    emf->emh->szlMicrometers.cy = emf->emh->szlMillimeters.cy * 1000;

    memcpy( (char *)emf->emh + sizeof(ENHMETAHEADER), description, length );

    if (filename)  /* disk based metafile */
    {
        if ((hFile = CreateFileW(filename, GENERIC_WRITE | GENERIC_READ, 0,
				 NULL, CREATE_ALWAYS, 0, 0)) == INVALID_HANDLE_VALUE) {
            DeleteDC( ret );
            return 0;
        }
        emf->hFile = hFile;
    }

    TRACE( "returning %p\n", ret );
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

    if (dc->attr->save_level)
        RestoreDC( hdc, 1 );

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
