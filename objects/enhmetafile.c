/*
 * Enhanced metafile functions
 * Copyright 1998 Douglas Ridgway
 *           1999 Huw D M Davies
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * NOTES:
 *
 * The enhanced format consists of the following elements:
 *
 *    A header
 *    A table of handles to GDI objects
 *    An array of metafile records
 *    A private palette
 *
 *
 *  The standard format consists of a header and an array of metafile records.
 *
 */

#include "config.h"
#include "wine/port.h"

#include <string.h>
#include <assert.h>
#include "winnls.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "gdi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(enhmetafile);

typedef struct
{
    GDIOBJHDR      header;
    ENHMETAHEADER  *emh;
    BOOL           on_disk;   /* true if metafile is on disk */
} ENHMETAFILEOBJ;


/****************************************************************************
 *          EMF_Create_HENHMETAFILE
 */
HENHMETAFILE EMF_Create_HENHMETAFILE(ENHMETAHEADER *emh, BOOL on_disk )
{
    HENHMETAFILE hmf = 0;
    ENHMETAFILEOBJ *metaObj = GDI_AllocObject( sizeof(ENHMETAFILEOBJ),
                                               ENHMETAFILE_MAGIC,
					       (HGDIOBJ *)&hmf, NULL );
    if (metaObj)
    {
        metaObj->emh = emh;
        metaObj->on_disk = on_disk;
        GDI_ReleaseObj( hmf );
    }
    return hmf;
}

/****************************************************************************
 *          EMF_Delete_HENHMETAFILE
 */
static BOOL EMF_Delete_HENHMETAFILE( HENHMETAFILE hmf )
{
    ENHMETAFILEOBJ *metaObj = (ENHMETAFILEOBJ *)GDI_GetObjPtr( hmf,
							   ENHMETAFILE_MAGIC );
    if(!metaObj) return FALSE;

    if(metaObj->on_disk)
        UnmapViewOfFile( metaObj->emh );
    else
        HeapFree( GetProcessHeap(), 0, metaObj->emh );
    return GDI_FreeObject( hmf, metaObj );
}

/******************************************************************
 *         EMF_GetEnhMetaHeader
 *
 * Returns ptr to ENHMETAHEADER associated with HENHMETAFILE
 */
static ENHMETAHEADER *EMF_GetEnhMetaHeader( HENHMETAFILE hmf )
{
    ENHMETAHEADER *ret = NULL;
    ENHMETAFILEOBJ *metaObj = (ENHMETAFILEOBJ *)GDI_GetObjPtr( hmf, ENHMETAFILE_MAGIC );
    TRACE("hmf %p -> enhmetaObj %p\n", hmf, metaObj);
    if (metaObj)
    {
        ret = metaObj->emh;
        GDI_ReleaseObj( hmf );
    }
    return ret;
}

/*****************************************************************************
 *         EMF_GetEnhMetaFile
 *
 */
static HENHMETAFILE EMF_GetEnhMetaFile( HANDLE hFile )
{
    ENHMETAHEADER *emh;
    HANDLE hMapping;

    hMapping = CreateFileMappingA( hFile, NULL, PAGE_READONLY, 0, 0, NULL );
    emh = MapViewOfFile( hMapping, FILE_MAP_READ, 0, 0, 0 );
    CloseHandle( hMapping );

    if (!emh) return 0;

    if (emh->iType != EMR_HEADER || emh->dSignature != ENHMETA_SIGNATURE) {
        WARN("Invalid emf header type 0x%08lx sig 0x%08lx.\n",
	     emh->iType, emh->dSignature);
	UnmapViewOfFile( emh );
	return 0;
    }
    return EMF_Create_HENHMETAFILE( emh, TRUE );
}


/*****************************************************************************
 *          GetEnhMetaFileA (GDI32.@)
 *
 *
 */
HENHMETAFILE WINAPI GetEnhMetaFileA(
	     LPCSTR lpszMetaFile  /* [in] filename of enhanced metafile */
    )
{
    HENHMETAFILE hmf;
    HANDLE hFile;

    hFile = CreateFileA(lpszMetaFile, GENERIC_READ, FILE_SHARE_READ, 0,
			OPEN_EXISTING, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE) {
        WARN("could not open %s\n", lpszMetaFile);
	return 0;
    }
    hmf = EMF_GetEnhMetaFile( hFile );
    CloseHandle( hFile );
    return hmf;
}

/*****************************************************************************
 *          GetEnhMetaFileW  (GDI32.@)
 */
HENHMETAFILE WINAPI GetEnhMetaFileW(
             LPCWSTR lpszMetaFile)  /* [in] filename of enhanced metafile */
{
    HENHMETAFILE hmf;
    HANDLE hFile;

    hFile = CreateFileW(lpszMetaFile, GENERIC_READ, FILE_SHARE_READ, 0,
			OPEN_EXISTING, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE) {
        WARN("could not open %s\n", debugstr_w(lpszMetaFile));
	return 0;
    }
    hmf = EMF_GetEnhMetaFile( hFile );
    CloseHandle( hFile );
    return hmf;
}

/*****************************************************************************
 *        GetEnhMetaFileHeader  (GDI32.@)
 *
 *  If buf is NULL, returns the size of buffer required.
 *  Otherwise, copy up to bufsize bytes of enhanced metafile header into
 *  buf.
 */
UINT WINAPI GetEnhMetaFileHeader(
       HENHMETAFILE hmf,   /* [in] enhanced metafile */
       UINT bufsize,       /* [in] size of buffer */
       LPENHMETAHEADER buf /* [out] buffer */
    )
{
    LPENHMETAHEADER emh;
    UINT size;

    emh = EMF_GetEnhMetaHeader(hmf);
    if(!emh) return FALSE;
    size = emh->nSize;
    if (!buf) return size;
    size = min(size, bufsize);
    memmove(buf, emh, size);
    return size;
}


/*****************************************************************************
 *          GetEnhMetaFileDescriptionA  (GDI32.@)
 */
UINT WINAPI GetEnhMetaFileDescriptionA(
       HENHMETAFILE hmf, /* [in] enhanced metafile */
       UINT size,        /* [in] size of buf */
       LPSTR buf         /* [out] buffer to receive description */
    )
{
     LPENHMETAHEADER emh = EMF_GetEnhMetaHeader(hmf);
     DWORD len;
     WCHAR *descrW;

     if(!emh) return FALSE;
     if(emh->nDescription == 0 || emh->offDescription == 0) return 0;
     descrW = (WCHAR *) ((char *) emh + emh->offDescription);
     len = WideCharToMultiByte( CP_ACP, 0, descrW, emh->nDescription, NULL, 0, NULL, NULL );

     if (!buf || !size ) return len;

     len = min( size, len );
     WideCharToMultiByte( CP_ACP, 0, descrW, emh->nDescription, buf, len, NULL, NULL );
     return len;
}

/*****************************************************************************
 *          GetEnhMetaFileDescriptionW  (GDI32.@)
 *
 *  Copies the description string of an enhanced metafile into a buffer
 *  _buf_.
 *
 *  If _buf_ is NULL, returns size of _buf_ required. Otherwise, returns
 *  number of characters copied.
 */
UINT WINAPI GetEnhMetaFileDescriptionW(
       HENHMETAFILE hmf, /* [in] enhanced metafile */
       UINT size,        /* [in] size of buf */
       LPWSTR buf        /* [out] buffer to receive description */
    )
{
     LPENHMETAHEADER emh = EMF_GetEnhMetaHeader(hmf);

     if(!emh) return FALSE;
     if(emh->nDescription == 0 || emh->offDescription == 0) return 0;
     if (!buf || !size ) return emh->nDescription;

     memmove(buf, (char *) emh + emh->offDescription, min(size,emh->nDescription)*sizeof(WCHAR));
     return min(size, emh->nDescription);
}

/****************************************************************************
 *    SetEnhMetaFileBits (GDI32.@)
 *
 *  Creates an enhanced metafile by copying _bufsize_ bytes from _buf_.
 */
HENHMETAFILE WINAPI SetEnhMetaFileBits(UINT bufsize, const BYTE *buf)
{
    ENHMETAHEADER *emh = HeapAlloc( GetProcessHeap(), 0, bufsize );
    memmove(emh, buf, bufsize);
    return EMF_Create_HENHMETAFILE( emh, FALSE );
}

/*****************************************************************************
 *  GetEnhMetaFileBits (GDI32.@)
 *
 */
UINT WINAPI GetEnhMetaFileBits(
    HENHMETAFILE hmf,
    UINT bufsize,
    LPBYTE buf
)
{
    LPENHMETAHEADER emh = EMF_GetEnhMetaHeader( hmf );
    UINT size;

    if(!emh) return 0;

    size = emh->nBytes;
    if( buf == NULL ) return size;

    size = min( size, bufsize );
    memmove(buf, emh, size);
    return size;
}

/*****************************************************************************
 *           PlayEnhMetaFileRecord  (GDI32.@)
 *
 *  Render a single enhanced metafile record in the device context hdc.
 *
 *  RETURNS
 *    TRUE (non zero) on success, FALSE on error.
 *  BUGS
 *    Many unimplemented records.
 *    No error handling on record play failures (ie checking return codes)
 */
BOOL WINAPI PlayEnhMetaFileRecord(
     HDC hdc,                   /* [in] device context in which to render EMF record */
     LPHANDLETABLE handletable, /* [in] array of handles to be used in rendering record */
     const ENHMETARECORD *mr,   /* [in] EMF record to render */
     UINT handles               /* [in] size of handle array */
     )
{
  int type;
  POINT pt[2];

  TRACE("hdc = %p, handletable = %p, record = %p, numHandles = %d\n",
        hdc, handletable, mr, handles);
  if (!mr) return FALSE;

  type = mr->iType;

  TRACE(" type=%d\n", type);
  switch(type)
    {
    case EMR_HEADER:
      break;
    case EMR_EOF:
      break;
    case EMR_GDICOMMENT:
      {
        PEMRGDICOMMENT lpGdiComment = (PEMRGDICOMMENT)mr;
        /* In an enhanced metafile, there can be both public and private GDI comments */
        GdiComment( hdc, lpGdiComment->cbData, lpGdiComment->Data );
        break;
      }
    case EMR_SETMAPMODE:
      {
	PEMRSETMAPMODE pSetMapMode = (PEMRSETMAPMODE) mr;
	SetMapMode(hdc, pSetMapMode->iMode);
	break;
      }
    case EMR_SETBKMODE:
      {
	PEMRSETBKMODE pSetBkMode = (PEMRSETBKMODE) mr;
	SetBkMode(hdc, pSetBkMode->iMode);
	break;
      }
    case EMR_SETBKCOLOR:
      {
       	PEMRSETBKCOLOR pSetBkColor = (PEMRSETBKCOLOR) mr;
	SetBkColor(hdc, pSetBkColor->crColor);
	break;
      }
    case EMR_SETPOLYFILLMODE:
      {
	PEMRSETPOLYFILLMODE pSetPolyFillMode = (PEMRSETPOLYFILLMODE) mr;
	SetPolyFillMode(hdc, pSetPolyFillMode->iMode);
	break;
      }
    case EMR_SETROP2:
      {
	PEMRSETROP2 pSetROP2 = (PEMRSETROP2) mr;
	SetROP2(hdc, pSetROP2->iMode);
	break;
      }
    case EMR_SETSTRETCHBLTMODE:
      {
	PEMRSETSTRETCHBLTMODE pSetStretchBltMode = (PEMRSETSTRETCHBLTMODE) mr;
	SetStretchBltMode(hdc, pSetStretchBltMode->iMode);
	break;
      }
    case EMR_SETTEXTALIGN:
      {
	PEMRSETTEXTALIGN pSetTextAlign = (PEMRSETTEXTALIGN) mr;
	SetTextAlign(hdc, pSetTextAlign->iMode);
	break;
      }
    case EMR_SETTEXTCOLOR:
      {
	PEMRSETTEXTCOLOR pSetTextColor = (PEMRSETTEXTCOLOR) mr;
	SetTextColor(hdc, pSetTextColor->crColor);
	break;
      }
    case EMR_SAVEDC:
      {
	SaveDC(hdc);
	break;
      }
    case EMR_RESTOREDC:
      {
	PEMRRESTOREDC pRestoreDC = (PEMRRESTOREDC) mr;
	RestoreDC(hdc, pRestoreDC->iRelative);
	break;
      }
    case EMR_INTERSECTCLIPRECT:
      {
	PEMRINTERSECTCLIPRECT pClipRect = (PEMRINTERSECTCLIPRECT) mr;
	IntersectClipRect(hdc, pClipRect->rclClip.left, pClipRect->rclClip.top,
			  pClipRect->rclClip.right, pClipRect->rclClip.bottom);
	break;
      }
    case EMR_SELECTOBJECT:
      {
	PEMRSELECTOBJECT pSelectObject = (PEMRSELECTOBJECT) mr;
	if( pSelectObject->ihObject & 0x80000000 ) {
	  /* High order bit is set - it's a stock object
	   * Strip the high bit to get the index.
	   * See MSDN article Q142319
	   */
	  SelectObject( hdc, GetStockObject( pSelectObject->ihObject &
					     0x7fffffff ) );
	} else {
	  /* High order bit wasn't set - not a stock object
	   */
	      SelectObject( hdc,
			(handletable->objectHandle)[pSelectObject->ihObject] );
	}
	break;
      }
    case EMR_DELETEOBJECT:
      {
	PEMRDELETEOBJECT pDeleteObject = (PEMRDELETEOBJECT) mr;
	DeleteObject( (handletable->objectHandle)[pDeleteObject->ihObject]);
	(handletable->objectHandle)[pDeleteObject->ihObject] = 0;
	break;
      }
    case EMR_SETWINDOWORGEX:
      {
		XFORM xform;
		PEMRSETWINDOWORGEX pSetWindowOrgEx = (PEMRSETWINDOWORGEX) mr;

		xform.eM11 = 1;
		xform.eM12 = 0;
		xform.eM21 = 0;
		xform.eM22 = 1;
		xform.eDx = -pSetWindowOrgEx->ptlOrigin.x;
		xform.eDy = -pSetWindowOrgEx->ptlOrigin.y;

		ModifyWorldTransform(hdc, &xform, MWT_LEFTMULTIPLY);
		break;
      }
    case EMR_SETWINDOWEXTEX:
      {
	PEMRSETWINDOWEXTEX pSetWindowExtEx = (PEMRSETWINDOWEXTEX) mr;
	SetWindowExtEx(hdc, pSetWindowExtEx->szlExtent.cx,
		       pSetWindowExtEx->szlExtent.cy, NULL);
	break;
      }
    case EMR_SETVIEWPORTORGEX:
      {
	PEMRSETVIEWPORTORGEX pSetViewportOrgEx = (PEMRSETVIEWPORTORGEX) mr;
	SetViewportOrgEx(hdc, pSetViewportOrgEx->ptlOrigin.x,
			 pSetViewportOrgEx->ptlOrigin.y, NULL);
	break;
      }
    case EMR_SETVIEWPORTEXTEX:
      {
	PEMRSETVIEWPORTEXTEX pSetViewportExtEx = (PEMRSETVIEWPORTEXTEX) mr;
	SetViewportExtEx(hdc, pSetViewportExtEx->szlExtent.cx,
			 pSetViewportExtEx->szlExtent.cy, NULL);
	break;
      }
    case EMR_CREATEPEN:
      {
	PEMRCREATEPEN pCreatePen = (PEMRCREATEPEN) mr;
	(handletable->objectHandle)[pCreatePen->ihPen] =
	  CreatePenIndirect(&pCreatePen->lopn);
	break;
      }
    case EMR_EXTCREATEPEN:
      {
	PEMREXTCREATEPEN pPen = (PEMREXTCREATEPEN) mr;
	LOGBRUSH lb;
	lb.lbStyle = pPen->elp.elpBrushStyle;
	lb.lbColor = pPen->elp.elpColor;
	lb.lbHatch = pPen->elp.elpHatch;

	if(pPen->offBmi || pPen->offBits)
	  FIXME("EMR_EXTCREATEPEN: Need to copy brush bitmap\n");

	(handletable->objectHandle)[pPen->ihPen] =
	  ExtCreatePen(pPen->elp.elpPenStyle, pPen->elp.elpWidth, &lb,
		       pPen->elp.elpNumEntries, pPen->elp.elpStyleEntry);
	break;
      }
    case EMR_CREATEBRUSHINDIRECT:
      {
	PEMRCREATEBRUSHINDIRECT pBrush = (PEMRCREATEBRUSHINDIRECT) mr;
	(handletable->objectHandle)[pBrush->ihBrush] =
	  CreateBrushIndirect(&pBrush->lb);
	break;
      }
    case EMR_EXTCREATEFONTINDIRECTW:
      {
	PEMREXTCREATEFONTINDIRECTW pFont = (PEMREXTCREATEFONTINDIRECTW) mr;
	(handletable->objectHandle)[pFont->ihFont] =
	  CreateFontIndirectW(&pFont->elfw.elfLogFont);
	break;
      }
    case EMR_MOVETOEX:
      {
	PEMRMOVETOEX pMoveToEx = (PEMRMOVETOEX) mr;
	MoveToEx(hdc, pMoveToEx->ptl.x, pMoveToEx->ptl.y, NULL);
	break;
      }
    case EMR_LINETO:
      {
	PEMRLINETO pLineTo = (PEMRLINETO) mr;
        LineTo(hdc, pLineTo->ptl.x, pLineTo->ptl.y);
	break;
      }
    case EMR_RECTANGLE:
      {
	PEMRRECTANGLE pRect = (PEMRRECTANGLE) mr;
	Rectangle(hdc, pRect->rclBox.left, pRect->rclBox.top,
		  pRect->rclBox.right, pRect->rclBox.bottom);
	break;
      }
    case EMR_ELLIPSE:
      {
	PEMRELLIPSE pEllipse = (PEMRELLIPSE) mr;
	Ellipse(hdc, pEllipse->rclBox.left, pEllipse->rclBox.top,
		pEllipse->rclBox.right, pEllipse->rclBox.bottom);
	break;
      }
    case EMR_POLYGON16:
      {
	PEMRPOLYGON16 pPoly = (PEMRPOLYGON16) mr;
	/* Shouldn't use Polygon16 since pPoly->cpts is DWORD */
	POINT *pts = HeapAlloc( GetProcessHeap(), 0,
				pPoly->cpts * sizeof(POINT) );
	DWORD i;
	for(i = 0; i < pPoly->cpts; i++)
	  CONV_POINT16TO32(pPoly->apts + i, pts + i);
	Polygon(hdc, pts, pPoly->cpts);
	HeapFree( GetProcessHeap(), 0, pts );
	break;
      }
    case EMR_POLYLINE16:
      {
	PEMRPOLYLINE16 pPoly = (PEMRPOLYLINE16) mr;
	/* Shouldn't use Polyline16 since pPoly->cpts is DWORD */
	POINT *pts = HeapAlloc( GetProcessHeap(), 0,
				pPoly->cpts * sizeof(POINT) );
	DWORD i;
	for(i = 0; i < pPoly->cpts; i++)
	  CONV_POINT16TO32(pPoly->apts + i, pts + i);
	Polyline(hdc, pts, pPoly->cpts);
	HeapFree( GetProcessHeap(), 0, pts );
	break;
      }
    case EMR_POLYLINETO16:
      {
	PEMRPOLYLINETO16 pPoly = (PEMRPOLYLINETO16) mr;
	/* Shouldn't use PolylineTo16 since pPoly->cpts is DWORD */
	POINT *pts = HeapAlloc( GetProcessHeap(), 0,
				pPoly->cpts * sizeof(POINT) );
	DWORD i;
	for(i = 0; i < pPoly->cpts; i++)
	  CONV_POINT16TO32(pPoly->apts + i, pts + i);
	PolylineTo(hdc, pts, pPoly->cpts);
	HeapFree( GetProcessHeap(), 0, pts );
	break;
      }
    case EMR_POLYBEZIER16:
      {
	PEMRPOLYBEZIER16 pPoly = (PEMRPOLYBEZIER16) mr;
	/* Shouldn't use PolyBezier16 since pPoly->cpts is DWORD */
	POINT *pts = HeapAlloc( GetProcessHeap(), 0,
				pPoly->cpts * sizeof(POINT) );
	DWORD i;
	for(i = 0; i < pPoly->cpts; i++)
	  CONV_POINT16TO32(pPoly->apts + i, pts + i);
	PolyBezier(hdc, pts, pPoly->cpts);
	HeapFree( GetProcessHeap(), 0, pts );
	break;
      }
    case EMR_POLYBEZIERTO16:
      {
	PEMRPOLYBEZIERTO16 pPoly = (PEMRPOLYBEZIERTO16) mr;
	/* Shouldn't use PolyBezierTo16 since pPoly->cpts is DWORD */
	POINT *pts = HeapAlloc( GetProcessHeap(), 0,
				pPoly->cpts * sizeof(POINT) );
	DWORD i;
	for(i = 0; i < pPoly->cpts; i++)
	  CONV_POINT16TO32(pPoly->apts + i, pts + i);
	PolyBezierTo(hdc, pts, pPoly->cpts);
	HeapFree( GetProcessHeap(), 0, pts );
	break;
      }
    case EMR_POLYPOLYGON16:
      {
        PEMRPOLYPOLYGON16 pPolyPoly = (PEMRPOLYPOLYGON16) mr;
	/* NB POINTS array doesn't start at pPolyPoly->apts it's actually
	   pPolyPoly->aPolyCounts + pPolyPoly->nPolys */

	POINT *pts = HeapAlloc( GetProcessHeap(), 0,
				pPolyPoly->cpts * sizeof(POINT) );
	DWORD i;
	for(i = 0; i < pPolyPoly->cpts; i++)
	  CONV_POINT16TO32((POINT16*) (pPolyPoly->aPolyCounts +
				      pPolyPoly->nPolys) + i, pts + i);

	PolyPolygon(hdc, pts, (INT*)pPolyPoly->aPolyCounts, pPolyPoly->nPolys);
	HeapFree( GetProcessHeap(), 0, pts );
	break;
      }
    case EMR_POLYPOLYLINE16:
      {
        PEMRPOLYPOLYLINE16 pPolyPoly = (PEMRPOLYPOLYLINE16) mr;
	/* NB POINTS array doesn't start at pPolyPoly->apts it's actually
	   pPolyPoly->aPolyCounts + pPolyPoly->nPolys */

	POINT *pts = HeapAlloc( GetProcessHeap(), 0,
				pPolyPoly->cpts * sizeof(POINT) );
	DWORD i;
	for(i = 0; i < pPolyPoly->cpts; i++)
	  CONV_POINT16TO32((POINT16*) (pPolyPoly->aPolyCounts +
				      pPolyPoly->nPolys) + i, pts + i);

	PolyPolyline(hdc, pts, pPolyPoly->aPolyCounts, pPolyPoly->nPolys);
	HeapFree( GetProcessHeap(), 0, pts );
	break;
      }

    case EMR_STRETCHDIBITS:
      {
	EMRSTRETCHDIBITS *pStretchDIBits = (EMRSTRETCHDIBITS *)mr;

	StretchDIBits(hdc,
		      pStretchDIBits->xDest,
		      pStretchDIBits->yDest,
		      pStretchDIBits->cxDest,
		      pStretchDIBits->cyDest,
		      pStretchDIBits->xSrc,
		      pStretchDIBits->ySrc,
		      pStretchDIBits->cxSrc,
		      pStretchDIBits->cySrc,
		      (BYTE *)mr + pStretchDIBits->offBitsSrc,
		      (const BITMAPINFO *)((BYTE *)mr + pStretchDIBits->offBmiSrc),
		      pStretchDIBits->iUsageSrc,
		      pStretchDIBits->dwRop);
	break;
      }

    case EMR_EXTTEXTOUTA:
    {
	PEMREXTTEXTOUTA pExtTextOutA = (PEMREXTTEXTOUTA)mr;
	RECT rc;

	rc.left = pExtTextOutA->emrtext.rcl.left;
	rc.top = pExtTextOutA->emrtext.rcl.top;
	rc.right = pExtTextOutA->emrtext.rcl.right;
	rc.bottom = pExtTextOutA->emrtext.rcl.bottom;
	ExtTextOutA(hdc, pExtTextOutA->emrtext.ptlReference.x, pExtTextOutA->emrtext.ptlReference.y,
	    pExtTextOutA->emrtext.fOptions, &rc,
	    (LPSTR)((BYTE *)mr + pExtTextOutA->emrtext.offString), pExtTextOutA->emrtext.nChars,
	    (INT *)((BYTE *)mr + pExtTextOutA->emrtext.offDx));
	break;
    }

    case EMR_EXTTEXTOUTW:
    {
	PEMREXTTEXTOUTW pExtTextOutW = (PEMREXTTEXTOUTW)mr;
	RECT rc;

	rc.left = pExtTextOutW->emrtext.rcl.left;
	rc.top = pExtTextOutW->emrtext.rcl.top;
	rc.right = pExtTextOutW->emrtext.rcl.right;
	rc.bottom = pExtTextOutW->emrtext.rcl.bottom;
	ExtTextOutW(hdc, pExtTextOutW->emrtext.ptlReference.x, pExtTextOutW->emrtext.ptlReference.y,
	    pExtTextOutW->emrtext.fOptions, &rc,
	    (LPWSTR)((BYTE *)mr + pExtTextOutW->emrtext.offString), pExtTextOutW->emrtext.nChars,
	    (INT *)((BYTE *)mr + pExtTextOutW->emrtext.offDx));
	break;
    }

    case EMR_CREATEPALETTE:
      {
	PEMRCREATEPALETTE lpCreatePal = (PEMRCREATEPALETTE)mr;

	(handletable->objectHandle)[ lpCreatePal->ihPal ] =
		CreatePalette( &lpCreatePal->lgpl );

	break;
      }

    case EMR_SELECTPALETTE:
      {
	PEMRSELECTPALETTE lpSelectPal = (PEMRSELECTPALETTE)mr;

	if( lpSelectPal->ihPal & 0x80000000 ) {
		SelectPalette( hdc, GetStockObject(lpSelectPal->ihPal & 0x7fffffff), TRUE);
	} else {
	(handletable->objectHandle)[ lpSelectPal->ihPal ] =
		SelectPalette( hdc, (handletable->objectHandle)[lpSelectPal->ihPal], TRUE);
	}
	break;
      }

    case EMR_REALIZEPALETTE:
      {
	RealizePalette( hdc );
	break;
      }

    case EMR_EXTSELECTCLIPRGN:
      {
	PEMREXTSELECTCLIPRGN lpRgn = (PEMREXTSELECTCLIPRGN)mr;
	HRGN hRgn = ExtCreateRegion(NULL, lpRgn->cbRgnData, (RGNDATA *)lpRgn->RgnData);
	ExtSelectClipRgn(hdc, hRgn, (INT)(lpRgn->iMode));
	/* ExtSelectClipRgn created a copy of the region */
	DeleteObject(hRgn);
        break;
      }

    case EMR_SETMETARGN:
      {
        SetMetaRgn( hdc );
        break;
      }

    case EMR_SETWORLDTRANSFORM:
      {
        PEMRSETWORLDTRANSFORM lpXfrm = (PEMRSETWORLDTRANSFORM)mr;
        SetWorldTransform( hdc, &lpXfrm->xform );
        break;
      }

    case EMR_POLYBEZIER:
      {
        PEMRPOLYBEZIER lpPolyBez = (PEMRPOLYBEZIER)mr;
        PolyBezier(hdc, (const LPPOINT)lpPolyBez->aptl, (UINT)lpPolyBez->cptl);
        break;
      }

    case EMR_POLYGON:
      {
        PEMRPOLYGON lpPoly = (PEMRPOLYGON)mr;
        Polygon( hdc, (const LPPOINT)lpPoly->aptl, (UINT)lpPoly->cptl );
        break;
      }

    case EMR_POLYLINE:
      {
        PEMRPOLYLINE lpPolyLine = (PEMRPOLYLINE)mr;
        Polyline(hdc, (const LPPOINT)lpPolyLine->aptl, (UINT)lpPolyLine->cptl);
        break;
      }

    case EMR_POLYBEZIERTO:
      {
        PEMRPOLYBEZIERTO lpPolyBezierTo = (PEMRPOLYBEZIERTO)mr;
        PolyBezierTo( hdc, (const LPPOINT)lpPolyBezierTo->aptl,
		      (UINT)lpPolyBezierTo->cptl );
        break;
      }

    case EMR_POLYLINETO:
      {
        PEMRPOLYLINETO lpPolyLineTo = (PEMRPOLYLINETO)mr;
        PolylineTo( hdc, (const LPPOINT)lpPolyLineTo->aptl,
		    (UINT)lpPolyLineTo->cptl );
        break;
      }

    case EMR_POLYPOLYLINE:
      {
        PEMRPOLYPOLYLINE pPolyPolyline = (PEMRPOLYPOLYLINE) mr;
	/* NB Points at pPolyPolyline->aPolyCounts + pPolyPolyline->nPolys */

        PolyPolyline(hdc, (LPPOINT)(pPolyPolyline->aPolyCounts +
				    pPolyPolyline->nPolys),
		     pPolyPolyline->aPolyCounts,
		     pPolyPolyline->nPolys );

        break;
      }

    case EMR_POLYPOLYGON:
      {
        PEMRPOLYPOLYGON pPolyPolygon = (PEMRPOLYPOLYGON) mr;

	/* NB Points at pPolyPolygon->aPolyCounts + pPolyPolygon->nPolys */

        PolyPolygon(hdc, (LPPOINT)(pPolyPolygon->aPolyCounts +
				   pPolyPolygon->nPolys),
		    (INT*)pPolyPolygon->aPolyCounts, pPolyPolygon->nPolys );
        break;
      }

    case EMR_SETBRUSHORGEX:
      {
        PEMRSETBRUSHORGEX lpSetBrushOrgEx = (PEMRSETBRUSHORGEX)mr;

        SetBrushOrgEx( hdc,
                       (INT)lpSetBrushOrgEx->ptlOrigin.x,
                       (INT)lpSetBrushOrgEx->ptlOrigin.y,
                       NULL );

        break;
      }

    case EMR_SETPIXELV:
      {
        PEMRSETPIXELV lpSetPixelV = (PEMRSETPIXELV)mr;

        SetPixelV( hdc,
                   (INT)lpSetPixelV->ptlPixel.x,
                   (INT)lpSetPixelV->ptlPixel.y,
                   lpSetPixelV->crColor );

        break;
      }

    case EMR_SETMAPPERFLAGS:
      {
        PEMRSETMAPPERFLAGS lpSetMapperFlags = (PEMRSETMAPPERFLAGS)mr;

        SetMapperFlags( hdc, lpSetMapperFlags->dwFlags );

        break;
      }

    case EMR_SETCOLORADJUSTMENT:
      {
        PEMRSETCOLORADJUSTMENT lpSetColorAdjust = (PEMRSETCOLORADJUSTMENT)mr;

        SetColorAdjustment( hdc, &lpSetColorAdjust->ColorAdjustment );

        break;
      }

    case EMR_OFFSETCLIPRGN:
      {
        PEMROFFSETCLIPRGN lpOffsetClipRgn = (PEMROFFSETCLIPRGN)mr;

        OffsetClipRgn( hdc,
                       (INT)lpOffsetClipRgn->ptlOffset.x,
                       (INT)lpOffsetClipRgn->ptlOffset.y );

        break;
      }

    case EMR_EXCLUDECLIPRECT:
      {
        PEMREXCLUDECLIPRECT lpExcludeClipRect = (PEMREXCLUDECLIPRECT)mr;

        ExcludeClipRect( hdc,
                         lpExcludeClipRect->rclClip.left,
                         lpExcludeClipRect->rclClip.top,
                         lpExcludeClipRect->rclClip.right,
                         lpExcludeClipRect->rclClip.bottom  );

         break;
      }

    case EMR_SCALEVIEWPORTEXTEX:
      {
        PEMRSCALEVIEWPORTEXTEX lpScaleViewportExtEx = (PEMRSCALEVIEWPORTEXTEX)mr;

        ScaleViewportExtEx( hdc,
                            lpScaleViewportExtEx->xNum,
                            lpScaleViewportExtEx->xDenom,
                            lpScaleViewportExtEx->yNum,
                            lpScaleViewportExtEx->yDenom,
                            NULL );

        break;
      }

    case EMR_SCALEWINDOWEXTEX:
      {
        PEMRSCALEWINDOWEXTEX lpScaleWindowExtEx = (PEMRSCALEWINDOWEXTEX)mr;

        ScaleWindowExtEx( hdc,
                          lpScaleWindowExtEx->xNum,
                          lpScaleWindowExtEx->xDenom,
                          lpScaleWindowExtEx->yNum,
                          lpScaleWindowExtEx->yDenom,
                          NULL );

        break;
      }

    case EMR_MODIFYWORLDTRANSFORM:
      {
        PEMRMODIFYWORLDTRANSFORM lpModifyWorldTrans = (PEMRMODIFYWORLDTRANSFORM)mr;

        ModifyWorldTransform( hdc, &lpModifyWorldTrans->xform,
                              lpModifyWorldTrans->iMode );

        break;
      }

    case EMR_ANGLEARC:
      {
        PEMRANGLEARC lpAngleArc = (PEMRANGLEARC)mr;

        AngleArc( hdc,
                 (INT)lpAngleArc->ptlCenter.x, (INT)lpAngleArc->ptlCenter.y,
                 lpAngleArc->nRadius, lpAngleArc->eStartAngle,
                 lpAngleArc->eSweepAngle );

        break;
      }

    case EMR_ROUNDRECT:
      {
        PEMRROUNDRECT lpRoundRect = (PEMRROUNDRECT)mr;

        RoundRect( hdc,
                   lpRoundRect->rclBox.left,
                   lpRoundRect->rclBox.top,
                   lpRoundRect->rclBox.right,
                   lpRoundRect->rclBox.bottom,
                   lpRoundRect->szlCorner.cx,
                   lpRoundRect->szlCorner.cy );

        break;
      }

    case EMR_ARC:
      {
        PEMRARC lpArc = (PEMRARC)mr;

        Arc( hdc,
             (INT)lpArc->rclBox.left,
             (INT)lpArc->rclBox.top,
             (INT)lpArc->rclBox.right,
             (INT)lpArc->rclBox.bottom,
             (INT)lpArc->ptlStart.x,
             (INT)lpArc->ptlStart.y,
             (INT)lpArc->ptlEnd.x,
             (INT)lpArc->ptlEnd.y );

        break;
      }

    case EMR_CHORD:
      {
        PEMRCHORD lpChord = (PEMRCHORD)mr;

        Chord( hdc,
             (INT)lpChord->rclBox.left,
             (INT)lpChord->rclBox.top,
             (INT)lpChord->rclBox.right,
             (INT)lpChord->rclBox.bottom,
             (INT)lpChord->ptlStart.x,
             (INT)lpChord->ptlStart.y,
             (INT)lpChord->ptlEnd.x,
             (INT)lpChord->ptlEnd.y );

        break;
      }

    case EMR_PIE:
      {
        PEMRPIE lpPie = (PEMRPIE)mr;

        Pie( hdc,
             (INT)lpPie->rclBox.left,
             (INT)lpPie->rclBox.top,
             (INT)lpPie->rclBox.right,
             (INT)lpPie->rclBox.bottom,
             (INT)lpPie->ptlStart.x,
             (INT)lpPie->ptlStart.y,
             (INT)lpPie->ptlEnd.x,
             (INT)lpPie->ptlEnd.y );

       break;
      }

    case EMR_ARCTO:
      {
        PEMRARC lpArcTo = (PEMRARC)mr;

        ArcTo( hdc,
               (INT)lpArcTo->rclBox.left,
               (INT)lpArcTo->rclBox.top,
               (INT)lpArcTo->rclBox.right,
               (INT)lpArcTo->rclBox.bottom,
               (INT)lpArcTo->ptlStart.x,
               (INT)lpArcTo->ptlStart.y,
               (INT)lpArcTo->ptlEnd.x,
               (INT)lpArcTo->ptlEnd.y );

        break;
      }

    case EMR_EXTFLOODFILL:
      {
        PEMREXTFLOODFILL lpExtFloodFill = (PEMREXTFLOODFILL)mr;

        ExtFloodFill( hdc,
                      (INT)lpExtFloodFill->ptlStart.x,
                      (INT)lpExtFloodFill->ptlStart.y,
                      lpExtFloodFill->crColor,
                      (UINT)lpExtFloodFill->iMode );

        break;
      }

    case EMR_POLYDRAW:
      {
        PEMRPOLYDRAW lpPolyDraw = (PEMRPOLYDRAW)mr;
        PolyDraw( hdc,
                  (const LPPOINT)lpPolyDraw->aptl,
                  lpPolyDraw->abTypes,
                  (INT)lpPolyDraw->cptl );

        break;
      }

    case EMR_SETARCDIRECTION:
      {
        PEMRSETARCDIRECTION lpSetArcDirection = (PEMRSETARCDIRECTION)mr;
        SetArcDirection( hdc, (INT)lpSetArcDirection->iArcDirection );
        break;
      }

    case EMR_SETMITERLIMIT:
      {
        PEMRSETMITERLIMIT lpSetMiterLimit = (PEMRSETMITERLIMIT)mr;
        SetMiterLimit( hdc, lpSetMiterLimit->eMiterLimit, NULL );
        break;
      }

    case EMR_BEGINPATH:
      {
        BeginPath( hdc );
        break;
      }

    case EMR_ENDPATH:
      {
        EndPath( hdc );
        break;
      }

    case EMR_CLOSEFIGURE:
      {
        CloseFigure( hdc );
        break;
      }

    case EMR_FILLPATH:
      {
        /*PEMRFILLPATH lpFillPath = (PEMRFILLPATH)mr;*/
        FillPath( hdc );
        break;
      }

    case EMR_STROKEANDFILLPATH:
      {
        /*PEMRSTROKEANDFILLPATH lpStrokeAndFillPath = (PEMRSTROKEANDFILLPATH)mr;*/
        StrokeAndFillPath( hdc );
        break;
      }

    case EMR_STROKEPATH:
      {
        /*PEMRSTROKEPATH lpStrokePath = (PEMRSTROKEPATH)mr;*/
        StrokePath( hdc );
        break;
      }

    case EMR_FLATTENPATH:
      {
        FlattenPath( hdc );
        break;
      }

    case EMR_WIDENPATH:
      {
        WidenPath( hdc );
        break;
      }

    case EMR_SELECTCLIPPATH:
      {
        PEMRSELECTCLIPPATH lpSelectClipPath = (PEMRSELECTCLIPPATH)mr;
        SelectClipPath( hdc, (INT)lpSelectClipPath->iMode );
        break;
      }

    case EMR_ABORTPATH:
      {
        AbortPath( hdc );
        break;
      }

    case EMR_CREATECOLORSPACE:
      {
        PEMRCREATECOLORSPACE lpCreateColorSpace = (PEMRCREATECOLORSPACE)mr;
        (handletable->objectHandle)[lpCreateColorSpace->ihCS] =
           CreateColorSpaceA( &lpCreateColorSpace->lcs );
        break;
      }

    case EMR_SETCOLORSPACE:
      {
        PEMRSETCOLORSPACE lpSetColorSpace = (PEMRSETCOLORSPACE)mr;
        SetColorSpace( hdc,
                       (handletable->objectHandle)[lpSetColorSpace->ihCS] );
        break;
      }

    case EMR_DELETECOLORSPACE:
      {
        PEMRDELETECOLORSPACE lpDeleteColorSpace = (PEMRDELETECOLORSPACE)mr;
        DeleteColorSpace( (handletable->objectHandle)[lpDeleteColorSpace->ihCS] );
        break;
      }

    case EMR_SETICMMODE:
      {
        PEMRSETICMMODE lpSetICMMode = (PEMRSETICMMODE)mr;
        SetICMMode( hdc, (INT)lpSetICMMode->iMode );
        break;
      }

    case EMR_PIXELFORMAT:
      {
        INT iPixelFormat;
        PEMRPIXELFORMAT lpPixelFormat = (PEMRPIXELFORMAT)mr;

        iPixelFormat = ChoosePixelFormat( hdc, &lpPixelFormat->pfd );
        SetPixelFormat( hdc, iPixelFormat, &lpPixelFormat->pfd );

        break;
      }

    case EMR_SETPALETTEENTRIES:
      {
        PEMRSETPALETTEENTRIES lpSetPaletteEntries = (PEMRSETPALETTEENTRIES)mr;

        SetPaletteEntries( (handletable->objectHandle)[lpSetPaletteEntries->ihPal],
                           (UINT)lpSetPaletteEntries->iStart,
                           (UINT)lpSetPaletteEntries->cEntries,
                           lpSetPaletteEntries->aPalEntries );

        break;
      }

    case EMR_RESIZEPALETTE:
      {
        PEMRRESIZEPALETTE lpResizePalette = (PEMRRESIZEPALETTE)mr;

        ResizePalette( (handletable->objectHandle)[lpResizePalette->ihPal],
                       (UINT)lpResizePalette->cEntries );

        break;
      }

    case EMR_CREATEDIBPATTERNBRUSHPT:
      {
        PEMRCREATEDIBPATTERNBRUSHPT lpCreate = (PEMRCREATEDIBPATTERNBRUSHPT)mr;
        LPVOID lpPackedStruct;

        /* check that offsets and data are contained within the record */
        if ( !( (lpCreate->cbBmi>=0) && (lpCreate->cbBits>=0) &&
                (lpCreate->offBmi>=0) && (lpCreate->offBits>=0) &&
                ((lpCreate->offBmi +lpCreate->cbBmi ) <= mr->nSize) &&
                ((lpCreate->offBits+lpCreate->cbBits) <= mr->nSize) ) )
        {
            ERR("Invalid EMR_CREATEDIBPATTERNBRUSHPT record\n");
            break;
        }

        /* This is a BITMAPINFO struct followed directly by bitmap bits */
        lpPackedStruct = HeapAlloc( GetProcessHeap(), 0,
                                    lpCreate->cbBmi + lpCreate->cbBits );
        if(!lpPackedStruct)
        {
	    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            break;
        }

        /* Now pack this structure */
        memcpy( lpPackedStruct,
                ((BYTE*)lpCreate) + lpCreate->offBmi,
                lpCreate->cbBmi );
        memcpy( ((BYTE*)lpPackedStruct) + lpCreate->cbBmi,
                ((BYTE*)lpCreate) + lpCreate->offBits,
                lpCreate->cbBits );

        (handletable->objectHandle)[lpCreate->ihBrush] =
           CreateDIBPatternBrushPt( lpPackedStruct,
                                    (UINT)lpCreate->iUsage );

        HeapFree(GetProcessHeap(), 0, lpPackedStruct);

        break;
      }

    case EMR_CREATEMONOBRUSH:
    {
	PEMRCREATEMONOBRUSH pCreateMonoBrush = (PEMRCREATEMONOBRUSH)mr;
	BITMAPINFO *pbi = (BITMAPINFO *)((BYTE *)mr + pCreateMonoBrush->offBmi);
	HBITMAP hBmp = CreateDIBitmap(0, (BITMAPINFOHEADER *)pbi, CBM_INIT,
		(BYTE *)mr + pCreateMonoBrush->offBits, pbi, pCreateMonoBrush->iUsage);
	(handletable->objectHandle)[pCreateMonoBrush->ihBrush] = CreatePatternBrush(hBmp);
	/* CreatePatternBrush created a copy of the bitmap */
	DeleteObject(hBmp);
	break;
    }

    case EMR_BITBLT:
    {
	PEMRBITBLT pBitBlt = (PEMRBITBLT)mr;
	HDC hdcSrc = CreateCompatibleDC(hdc);
	HBRUSH hBrush, hBrushOld;
	HBITMAP hBmp = 0, hBmpOld = 0;
	BITMAPINFO *pbi = (BITMAPINFO *)((BYTE *)mr + pBitBlt->offBmiSrc);

	SetWorldTransform(hdcSrc, &pBitBlt->xformSrc);

	hBrush = CreateSolidBrush(pBitBlt->crBkColorSrc);
	hBrushOld = SelectObject(hdcSrc, hBrush);
	PatBlt(hdcSrc, pBitBlt->rclBounds.left, pBitBlt->rclBounds.top,
	       pBitBlt->rclBounds.right - pBitBlt->rclBounds.left,
	       pBitBlt->rclBounds.bottom - pBitBlt->rclBounds.top, PATCOPY);
	SelectObject(hdcSrc, hBrushOld);
	DeleteObject(hBrush);

	if (pBitBlt->offBmiSrc > 0)
	{
		hBmp = CreateDIBitmap(0, (BITMAPINFOHEADER *)pbi, CBM_INIT,
					  (BYTE *)mr + pBitBlt->offBitsSrc, pbi, pBitBlt->iUsageSrc);
		hBmpOld = SelectObject(hdcSrc, hBmp);
	}

	BitBlt(hdc,
	       pBitBlt->xDest,
	       pBitBlt->yDest,
	       pBitBlt->cxDest,
	       pBitBlt->cyDest,
	       hdcSrc,
	       pBitBlt->xSrc,
	       pBitBlt->ySrc,
	       pBitBlt->dwRop);

	if (pBitBlt->offBmiSrc > 0)
	{
		SelectObject(hdcSrc, hBmpOld);
		DeleteObject(hBmp);
	}
	DeleteDC(hdcSrc);
	break;
    }

    case EMR_STRETCHBLT:
    {
	PEMRSTRETCHBLT pStretchBlt= (PEMRSTRETCHBLT)mr;
	HDC hdcSrc = CreateCompatibleDC(hdc);
	HBRUSH hBrush, hBrushOld;
	HBITMAP hBmp = 0, hBmpOld = 0;
	BITMAPINFO *pbi = (BITMAPINFO *)((BYTE *)mr + pStretchBlt->offBmiSrc);

	SetWorldTransform(hdcSrc, &pStretchBlt->xformSrc);

	hBrush = CreateSolidBrush(pStretchBlt->crBkColorSrc);
	hBrushOld = SelectObject(hdcSrc, hBrush);
	PatBlt(hdcSrc, pStretchBlt->rclBounds.left, pStretchBlt->rclBounds.top,
	       pStretchBlt->rclBounds.right - pStretchBlt->rclBounds.left,
	       pStretchBlt->rclBounds.bottom - pStretchBlt->rclBounds.top, PATCOPY);
	SelectObject(hdcSrc, hBrushOld);
	DeleteObject(hBrush);

	if (pStretchBlt->offBmiSrc)
	{
		hBmp = CreateDIBitmap(0, (BITMAPINFOHEADER *)pbi, CBM_INIT,
					  (BYTE *)mr + pStretchBlt->offBitsSrc, pbi, pStretchBlt->iUsageSrc);
		hBmpOld = SelectObject(hdcSrc, hBmp);
	}

	StretchBlt(hdc,
	       pStretchBlt->xDest,
	       pStretchBlt->yDest,
	       pStretchBlt->cxDest,
	       pStretchBlt->cyDest,
	       hdcSrc,
	       pStretchBlt->xSrc,
	       pStretchBlt->ySrc,
	       pStretchBlt->cxSrc,
	       pStretchBlt->cySrc,
	       pStretchBlt->dwRop);


	if (pStretchBlt->offBmiSrc)
	{
		SelectObject(hdcSrc, hBmpOld);
		DeleteObject(hBmp);
	}

	DeleteDC(hdcSrc);
	break;
    }

    case EMR_MASKBLT:
    {
	PEMRMASKBLT pMaskBlt= (PEMRMASKBLT)mr;
	HDC hdcSrc = CreateCompatibleDC(hdc);
	HBRUSH hBrush, hBrushOld;
	HBITMAP hBmp, hBmpOld, hBmpMask;
	BITMAPINFO *pbi;

	SetWorldTransform(hdcSrc, &pMaskBlt->xformSrc);

	hBrush = CreateSolidBrush(pMaskBlt->crBkColorSrc);
	hBrushOld = SelectObject(hdcSrc, hBrush);
	PatBlt(hdcSrc, pMaskBlt->rclBounds.left, pMaskBlt->rclBounds.top,
	       pMaskBlt->rclBounds.right - pMaskBlt->rclBounds.left,
	       pMaskBlt->rclBounds.bottom - pMaskBlt->rclBounds.top, PATCOPY);
	SelectObject(hdcSrc, hBrushOld);
	DeleteObject(hBrush);

	pbi = (BITMAPINFO *)((BYTE *)mr + pMaskBlt->offBmiMask);
	hBmpMask = CreateDIBitmap(0, (BITMAPINFOHEADER *)pbi, CBM_INIT,
			      (BYTE *)mr + pMaskBlt->offBitsMask, pbi, pMaskBlt->iUsageMask);

	pbi = (BITMAPINFO *)((BYTE *)mr + pMaskBlt->offBmiSrc);
	hBmp = CreateDIBitmap(0, (BITMAPINFOHEADER *)pbi, CBM_INIT,
			      (BYTE *)mr + pMaskBlt->offBitsSrc, pbi, pMaskBlt->iUsageSrc);
	hBmpOld = SelectObject(hdcSrc, hBmp);
	MaskBlt(hdc,
		pMaskBlt->xDest,
	        pMaskBlt->yDest,
	        pMaskBlt->cxDest,
	        pMaskBlt->cyDest,
	        hdcSrc,
	        pMaskBlt->xSrc,
	        pMaskBlt->ySrc,
	        hBmpMask,
		pMaskBlt->xMask,
		pMaskBlt->yMask,
	        pMaskBlt->dwRop);
	SelectObject(hdcSrc, hBmpOld);
	DeleteObject(hBmp);
	DeleteObject(hBmpMask);
	DeleteDC(hdcSrc);
	break;
    }

    case EMR_PLGBLT:
    {
	PEMRPLGBLT pPlgBlt= (PEMRPLGBLT)mr;
	HDC hdcSrc = CreateCompatibleDC(hdc);
	HBRUSH hBrush, hBrushOld;
	HBITMAP hBmp, hBmpOld, hBmpMask;
	BITMAPINFO *pbi;
	POINT pts[3];

	SetWorldTransform(hdcSrc, &pPlgBlt->xformSrc);

	pts[0].x = pPlgBlt->aptlDest[0].x; pts[0].y = pPlgBlt->aptlDest[0].y;
	pts[1].x = pPlgBlt->aptlDest[1].x; pts[1].y = pPlgBlt->aptlDest[1].y;
	pts[2].x = pPlgBlt->aptlDest[2].x; pts[2].y = pPlgBlt->aptlDest[2].y;

	hBrush = CreateSolidBrush(pPlgBlt->crBkColorSrc);
	hBrushOld = SelectObject(hdcSrc, hBrush);
	PatBlt(hdcSrc, pPlgBlt->rclBounds.left, pPlgBlt->rclBounds.top,
	       pPlgBlt->rclBounds.right - pPlgBlt->rclBounds.left,
	       pPlgBlt->rclBounds.bottom - pPlgBlt->rclBounds.top, PATCOPY);
	SelectObject(hdcSrc, hBrushOld);
	DeleteObject(hBrush);

	pbi = (BITMAPINFO *)((BYTE *)mr + pPlgBlt->offBmiMask);
	hBmpMask = CreateDIBitmap(0, (BITMAPINFOHEADER *)pbi, CBM_INIT,
			      (BYTE *)mr + pPlgBlt->offBitsMask, pbi, pPlgBlt->iUsageMask);

	pbi = (BITMAPINFO *)((BYTE *)mr + pPlgBlt->offBmiSrc);
	hBmp = CreateDIBitmap(0, (BITMAPINFOHEADER *)pbi, CBM_INIT,
			      (BYTE *)mr + pPlgBlt->offBitsSrc, pbi, pPlgBlt->iUsageSrc);
	hBmpOld = SelectObject(hdcSrc, hBmp);
	PlgBlt(hdc,
	       pts,
	       hdcSrc,
	       pPlgBlt->xSrc,
	       pPlgBlt->ySrc,
	       pPlgBlt->cxSrc,
	       pPlgBlt->cySrc,
	       hBmpMask,
	       pPlgBlt->xMask,
	       pPlgBlt->yMask);
	SelectObject(hdcSrc, hBmpOld);
	DeleteObject(hBmp);
	DeleteObject(hBmpMask);
	DeleteDC(hdcSrc);
	break;
    }

    case EMR_SETDIBITSTODEVICE:
    {
	PEMRSETDIBITSTODEVICE pSetDIBitsToDevice = (PEMRSETDIBITSTODEVICE)mr;

	SetDIBitsToDevice(hdc,
			  pSetDIBitsToDevice->xDest,
			  pSetDIBitsToDevice->yDest,
			  pSetDIBitsToDevice->cxSrc,
			  pSetDIBitsToDevice->cySrc,
			  pSetDIBitsToDevice->xSrc,
			  pSetDIBitsToDevice->ySrc,
			  pSetDIBitsToDevice->iStartScan,
			  pSetDIBitsToDevice->cScans,
			  (BYTE *)mr + pSetDIBitsToDevice->offBitsSrc,
			  (BITMAPINFO *)((BYTE *)mr + pSetDIBitsToDevice->offBmiSrc),
			  pSetDIBitsToDevice->iUsageSrc);
	break;
    }

    case EMR_POLYTEXTOUTA:
    {
	PEMRPOLYTEXTOUTA pPolyTextOutA = (PEMRPOLYTEXTOUTA)mr;
	POLYTEXTA *polytextA = HeapAlloc(GetProcessHeap(), 0, pPolyTextOutA->cStrings * sizeof(POLYTEXTA));
	LONG i;
	XFORM xform, xformOld;
	int gModeOld;

	gModeOld = SetGraphicsMode(hdc, pPolyTextOutA->iGraphicsMode);
	GetWorldTransform(hdc, &xformOld);

	xform.eM11 = pPolyTextOutA->exScale;
	xform.eM12 = 0.0;
	xform.eM21 = 0.0;
	xform.eM22 = pPolyTextOutA->eyScale;
	xform.eDx = 0.0;
	xform.eDy = 0.0;
	SetWorldTransform(hdc, &xform);

	/* Set up POLYTEXTA structures */
	for(i = 0; i < pPolyTextOutA->cStrings; i++)
	{
	    polytextA[i].x = pPolyTextOutA->aemrtext[i].ptlReference.x;
	    polytextA[i].y = pPolyTextOutA->aemrtext[i].ptlReference.y;
	    polytextA[i].n = pPolyTextOutA->aemrtext[i].nChars;
	    polytextA[i].lpstr = (LPSTR)((BYTE *)mr + pPolyTextOutA->aemrtext[i].offString);
	    polytextA[i].uiFlags = pPolyTextOutA->aemrtext[i].fOptions;
	    polytextA[i].rcl.left = pPolyTextOutA->aemrtext[i].rcl.left;
	    polytextA[i].rcl.right = pPolyTextOutA->aemrtext[i].rcl.right;
	    polytextA[i].rcl.top = pPolyTextOutA->aemrtext[i].rcl.top;
	    polytextA[i].rcl.bottom = pPolyTextOutA->aemrtext[i].rcl.bottom;
	    polytextA[i].pdx = (int *)((BYTE *)mr + pPolyTextOutA->aemrtext[i].offDx);
	}
	PolyTextOutA(hdc, polytextA, pPolyTextOutA->cStrings);
	HeapFree(GetProcessHeap(), 0, polytextA);

	SetWorldTransform(hdc, &xformOld);
	SetGraphicsMode(hdc, gModeOld);
	break;
    }

    case EMR_POLYTEXTOUTW:
    {
	PEMRPOLYTEXTOUTW pPolyTextOutW = (PEMRPOLYTEXTOUTW)mr;
	POLYTEXTW *polytextW = HeapAlloc(GetProcessHeap(), 0, pPolyTextOutW->cStrings * sizeof(POLYTEXTW));
	LONG i;
	XFORM xform, xformOld;
	int gModeOld;

	gModeOld = SetGraphicsMode(hdc, pPolyTextOutW->iGraphicsMode);
	GetWorldTransform(hdc, &xformOld);

	xform.eM11 = pPolyTextOutW->exScale;
	xform.eM12 = 0.0;
	xform.eM21 = 0.0;
	xform.eM22 = pPolyTextOutW->eyScale;
	xform.eDx = 0.0;
	xform.eDy = 0.0;
	SetWorldTransform(hdc, &xform);

	/* Set up POLYTEXTW structures */
	for(i = 0; i < pPolyTextOutW->cStrings; i++)
	{
	    polytextW[i].x = pPolyTextOutW->aemrtext[i].ptlReference.x;
	    polytextW[i].y = pPolyTextOutW->aemrtext[i].ptlReference.y;
	    polytextW[i].n = pPolyTextOutW->aemrtext[i].nChars;
	    polytextW[i].lpstr = (LPWSTR)((BYTE *)mr + pPolyTextOutW->aemrtext[i].offString);
	    polytextW[i].uiFlags = pPolyTextOutW->aemrtext[i].fOptions;
	    polytextW[i].rcl.left = pPolyTextOutW->aemrtext[i].rcl.left;
	    polytextW[i].rcl.right = pPolyTextOutW->aemrtext[i].rcl.right;
	    polytextW[i].rcl.top = pPolyTextOutW->aemrtext[i].rcl.top;
	    polytextW[i].rcl.bottom = pPolyTextOutW->aemrtext[i].rcl.bottom;
	    polytextW[i].pdx = (int *)((BYTE *)mr + pPolyTextOutW->aemrtext[i].offDx);
	}
	PolyTextOutW(hdc, polytextW, pPolyTextOutW->cStrings);
	HeapFree(GetProcessHeap(), 0, polytextW);

	SetWorldTransform(hdc, &xformOld);
	SetGraphicsMode(hdc, gModeOld);
	break;
    }

    case EMR_FILLRGN:
    {
	PEMRFILLRGN pFillRgn = (PEMRFILLRGN)mr;
	HRGN hRgn = ExtCreateRegion(NULL, pFillRgn->cbRgnData, (RGNDATA *)pFillRgn->RgnData);
	FillRgn(hdc,
		hRgn,
		(handletable->objectHandle)[pFillRgn->ihBrush]);
	DeleteObject(hRgn);
	break;
    }

    case EMR_FRAMERGN:
    {
	PEMRFRAMERGN pFrameRgn = (PEMRFRAMERGN)mr;
	HRGN hRgn = ExtCreateRegion(NULL, pFrameRgn->cbRgnData, (RGNDATA *)pFrameRgn->RgnData);
	FrameRgn(hdc,
		 hRgn,
		 (handletable->objectHandle)[pFrameRgn->ihBrush],
		 pFrameRgn->szlStroke.cx,
		 pFrameRgn->szlStroke.cy);
	DeleteObject(hRgn);
	break;
    }

    case EMR_INVERTRGN:
    {
	PEMRINVERTRGN pInvertRgn = (PEMRINVERTRGN)mr;
	HRGN hRgn = ExtCreateRegion(NULL, pInvertRgn->cbRgnData, (RGNDATA *)pInvertRgn->RgnData);
	InvertRgn(hdc, hRgn);
	DeleteObject(hRgn);
	break;
    }

    case EMR_PAINTRGN:
    {
	PEMRPAINTRGN pPaintRgn = (PEMRPAINTRGN)mr;
	HRGN hRgn = ExtCreateRegion(NULL, pPaintRgn->cbRgnData, (RGNDATA *)pPaintRgn->RgnData);
	PaintRgn(hdc, hRgn);
	DeleteObject(hRgn);
	break;
    }

    case EMR_SETTEXTJUSTIFICATION:
    {
	PEMRSETTEXTJUSTIFICATION pSetTextJust = (PEMRSETTEXTJUSTIFICATION)mr;
	SetTextJustification(hdc, pSetTextJust->nBreakExtra, pSetTextJust->nBreakCount);
	break;
    }

    case EMR_SETLAYOUT:
    {
	PEMRSETLAYOUT pSetLayout = (PEMRSETLAYOUT)mr;
	SetLayout(hdc, pSetLayout->iMode);
	break;
    }

    case EMR_POLYDRAW16:
    case EMR_GLSRECORD:
    case EMR_GLSBOUNDEDRECORD:
	case EMR_DRAWESCAPE :
	case EMR_EXTESCAPE:
	case EMR_STARTDOC:
	case EMR_SMALLTEXTOUT:
	case EMR_FORCEUFIMAPPING:
	case EMR_NAMEDESCAPE:
	case EMR_COLORCORRECTPALETTE:
	case EMR_SETICMPROFILEA:
	case EMR_SETICMPROFILEW:
	case EMR_ALPHABLEND:
	case EMR_TRANSPARENTBLT:
	case EMR_GRADIENTFILL:
	case EMR_SETLINKEDUFI:
	case EMR_COLORMATCHTOTARGETW:
	case EMR_CREATECOLORSPACEW:

    default:
      /* From docs: If PlayEnhMetaFileRecord doesn't recognize a
                    record then ignore and return TRUE. */
      FIXME("type %d is unimplemented\n", type);
      break;
    }

  pt[0].x = pt[0].y = 0;
  pt[1].x = pt[1].y = 1000;
  LPtoDP(hdc, pt, 2);
  TRACE("L:0,0 - 1000,1000 -> D:%ld,%ld - %ld,%ld\n", pt[0].x, pt[0].y,
	pt[1].x, pt[1].y);
  return TRUE;
}


/*****************************************************************************
 *
 *        EnumEnhMetaFile  (GDI32.@)
 *
 *  Walk an enhanced metafile, calling a user-specified function _EnhMetaFunc_
 *  for each
 *  record. Returns when either every record has been used or
 *  when _EnhMetaFunc_ returns FALSE.
 *
 *
 * RETURNS
 *  TRUE if every record is used, FALSE if any invocation of _EnhMetaFunc_
 *  returns FALSE.
 *
 * BUGS
 *   Ignores rect.
 */
BOOL WINAPI EnumEnhMetaFile(
     HDC hdc,                /* [in] device context to pass to _EnhMetaFunc_ */
     HENHMETAFILE hmf,       /* [in] EMF to walk */
     ENHMFENUMPROC callback, /* [in] callback function */
     LPVOID data,            /* [in] optional data for callback function */
     const RECT *lpRect      /* [in] bounding rectangle for rendered metafile */
    )
{
    BOOL ret;
    ENHMETAHEADER *emh;
    ENHMETARECORD *emr;
    DWORD offset;
    UINT i;
    HANDLETABLE *ht;
    INT savedMode = 0;
    FLOAT xSrcPixSize, ySrcPixSize, xscale, yscale;
    XFORM savedXform, xform;
    HPEN hPen = (HPEN)NULL;
    HBRUSH hBrush = (HBRUSH)NULL;
    HFONT hFont = (HFONT)NULL;
    POINT pt[2];

    if(!lpRect)
    {
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }

    emh = EMF_GetEnhMetaHeader(hmf);
    if(!emh) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    ht = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
		    sizeof(HANDLETABLE) * emh->nHandles );
    if(!ht)
    {
	SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	return FALSE;
    }
    ht->objectHandle[0] = hmf;

    if (hdc && (emh->rclFrame.right - emh->rclFrame.left) && (emh->rclFrame.bottom - emh->rclFrame.top))
    {
        TRACE("rect: %d,%d - %d,%d. rclFrame: %ld,%ld - %ld,%ld\n",
	      lpRect->left, lpRect->top, lpRect->right, lpRect->bottom,
	      emh->rclFrame.left, emh->rclFrame.top, emh->rclFrame.right,
	      emh->rclFrame.bottom);

	xSrcPixSize = (FLOAT) emh->szlMillimeters.cx / emh->szlDevice.cx;
	ySrcPixSize = (FLOAT) emh->szlMillimeters.cy / emh->szlDevice.cy;
	xscale = (FLOAT)(lpRect->right - lpRect->left) * 100.0 /
	  (emh->rclFrame.right - emh->rclFrame.left) * xSrcPixSize;
	yscale = (FLOAT)(lpRect->bottom - lpRect->top) * 100.0 /
	  (emh->rclFrame.bottom - emh->rclFrame.top) * ySrcPixSize;

	xform.eM11 = xscale;
	xform.eM12 = 0;
	xform.eM21 = 0;
	xform.eM22 = yscale;
	xform.eDx = (FLOAT) lpRect->left - (lpRect->right - lpRect->left) *
	  emh->rclFrame.left / (emh->rclFrame.right - emh->rclFrame.left);
	xform.eDy = (FLOAT) lpRect->top - (lpRect->bottom - lpRect->top) *
	  emh->rclFrame.top / (emh->rclFrame.bottom - emh->rclFrame.top);

	savedMode = SetGraphicsMode(hdc, GM_ADVANCED);
	GetWorldTransform(hdc, &savedXform);

	if (!ModifyWorldTransform(hdc, &xform, MWT_RIGHTMULTIPLY)) {
	    ERR("World transform failed!\n");
	}

	/* save the current pen, brush and font */
	hPen = GetCurrentObject(hdc, OBJ_PEN);
	hBrush = GetCurrentObject(hdc, OBJ_BRUSH);
	hFont = GetCurrentObject(hdc, OBJ_FONT);
    }

    pt[0].x = pt[0].y = 0;
    pt[1].x = pt[1].y = 1000;
    LPtoDP(hdc, pt, 2);
    TRACE("L:0,0-1000,1000 maps to D:%ld,%ld - %ld,%ld\n", pt[0].x, pt[0].y,
	  pt[1].x, pt[1].y);
    TRACE("nSize = %ld, nBytes = %ld, nHandles = %d, nRecords = %ld, nPalEntries = %ld\n",
	emh->nSize, emh->nBytes, emh->nHandles, emh->nRecords, emh->nPalEntries);

    ret = TRUE;
    offset = 0;
    while(ret && offset < emh->nBytes)
    {
	emr = (ENHMETARECORD *)((char *)emh + offset);
	TRACE("Calling EnumFunc with record type %ld, size %ld\n", emr->iType, emr->nSize);
	ret = (*callback)(hdc, ht, emr, emh->nHandles, data);
	offset += emr->nSize;
    }

    if (hdc)
    {
	/* restore pen, brush and font */
	SelectObject(hdc, hBrush);
	SelectObject(hdc, hPen);
	SelectObject(hdc, hFont);

	SetWorldTransform(hdc, &savedXform);
	if (savedMode)
	    SetGraphicsMode(hdc, savedMode);
    }

    for(i = 1; i < emh->nHandles; i++) /* Don't delete element 0 (hmf) */
        if( (ht->objectHandle)[i] )
	    DeleteObject( (ht->objectHandle)[i] );

    HeapFree( GetProcessHeap(), 0, ht );
    return ret;
}

static INT CALLBACK EMF_PlayEnhMetaFileCallback(HDC hdc, HANDLETABLE *ht,
						ENHMETARECORD *emr,
						INT handles, LPVOID data)
{
    return PlayEnhMetaFileRecord(hdc, ht, emr, handles);
}

/**************************************************************************
 *    PlayEnhMetaFile  (GDI32.@)
 *
 *    Renders an enhanced metafile into a specified rectangle *lpRect
 *    in device context hdc.
 *
 */
BOOL WINAPI PlayEnhMetaFile(
       HDC hdc,           /* [in] DC to render into */
       HENHMETAFILE hmf,  /* [in] metafile to render */
       const RECT *lpRect /* [in] rectangle to place metafile inside */
      )
{
    return EnumEnhMetaFile(hdc, hmf, EMF_PlayEnhMetaFileCallback, NULL,
			   lpRect);
}

/*****************************************************************************
 *  DeleteEnhMetaFile (GDI32.@)
 *
 *  Deletes an enhanced metafile and frees the associated storage.
 */
BOOL WINAPI DeleteEnhMetaFile(HENHMETAFILE hmf)
{
    return EMF_Delete_HENHMETAFILE( hmf );
}

/*****************************************************************************
 *  CopyEnhMetaFileA (GDI32.@)  Duplicate an enhanced metafile
 *
 *
 */
HENHMETAFILE WINAPI CopyEnhMetaFileA(
    HENHMETAFILE hmfSrc,
    LPCSTR file)
{
    ENHMETAHEADER *emrSrc = EMF_GetEnhMetaHeader( hmfSrc ), *emrDst;
    HENHMETAFILE hmfDst;

    if(!emrSrc) return FALSE;
    if (!file) {
        emrDst = HeapAlloc( GetProcessHeap(), 0, emrSrc->nBytes );
	memcpy( emrDst, emrSrc, emrSrc->nBytes );
	hmfDst = EMF_Create_HENHMETAFILE( emrDst, FALSE );
    } else {
        HANDLE hFile;
        hFile = CreateFileA( file, GENERIC_WRITE | GENERIC_READ, 0,
			     NULL, CREATE_ALWAYS, 0, 0);
	WriteFile( hFile, emrSrc, emrSrc->nBytes, 0, 0);
	CloseHandle( hFile );
	/* Reopen file for reading only, so that apps can share
	   read access to the file while hmf is still valid */
        hFile = CreateFileA( file, GENERIC_READ, FILE_SHARE_READ,
			     NULL, OPEN_EXISTING, 0, 0);
	if(hFile == INVALID_HANDLE_VALUE) {
	    ERR("Can't reopen emf for reading\n");
	    return 0;
	}
	hmfDst = EMF_GetEnhMetaFile( hFile );
        CloseHandle( hFile );
    }
    return hmfDst;
}


/* Struct to be used to be passed in the LPVOID parameter for cbEnhPaletteCopy */
typedef struct tagEMF_PaletteCopy
{
   UINT cEntries;
   LPPALETTEENTRY lpPe;
} EMF_PaletteCopy;

/***************************************************************
 * Find the EMR_EOF record and then use it to find the
 * palette entries for this enhanced metafile.
 * The lpData is actually a pointer to a EMF_PaletteCopy struct
 * which contains the max number of elements to copy and where
 * to copy them to.
 *
 * NOTE: To be used by GetEnhMetaFilePaletteEntries only!
 */
INT CALLBACK cbEnhPaletteCopy( HDC a,
                               LPHANDLETABLE b,
                               LPENHMETARECORD lpEMR,
                               INT c,
                               LPVOID lpData )
{

  if ( lpEMR->iType == EMR_EOF )
  {
    PEMREOF lpEof = (PEMREOF)lpEMR;
    EMF_PaletteCopy* info = (EMF_PaletteCopy*)lpData;
    DWORD dwNumPalToCopy = min( lpEof->nPalEntries, info->cEntries );

    TRACE( "copying 0x%08lx palettes\n", dwNumPalToCopy );

    memcpy( (LPVOID)info->lpPe,
            (LPVOID)(((LPSTR)lpEof) + lpEof->offPalEntries),
            sizeof( *(info->lpPe) ) * dwNumPalToCopy );

    /* Update the passed data as a return code */
    info->lpPe     = NULL; /* Palettes were copied! */
    info->cEntries = (UINT)dwNumPalToCopy;

    return FALSE; /* That's all we need */
  }

  return TRUE;
}

/*****************************************************************************
 *  GetEnhMetaFilePaletteEntries (GDI32.@)
 *
 *  Copy the palette and report size
 *
 *  BUGS: Error codes (SetLastError) are not set on failures
 */
UINT WINAPI GetEnhMetaFilePaletteEntries( HENHMETAFILE hEmf,
					  UINT cEntries,
					  LPPALETTEENTRY lpPe )
{
  ENHMETAHEADER* enhHeader = EMF_GetEnhMetaHeader( hEmf );
  EMF_PaletteCopy infoForCallBack;

  TRACE( "(%p,%d,%p)\n", hEmf, cEntries, lpPe );

  /* First check if there are any palettes associated with
     this metafile. */
  if ( enhHeader->nPalEntries == 0 ) return 0;

  /* Is the user requesting the number of palettes? */
  if ( lpPe == NULL ) return (UINT)enhHeader->nPalEntries;

  /* Copy cEntries worth of PALETTEENTRY structs into the buffer */
  infoForCallBack.cEntries = cEntries;
  infoForCallBack.lpPe     = lpPe;

  if ( !EnumEnhMetaFile( 0, hEmf, cbEnhPaletteCopy,
                         &infoForCallBack, NULL ) )
      return GDI_ERROR;

  /* Verify that the callback executed correctly */
  if ( infoForCallBack.lpPe != NULL )
  {
     /* Callback proc had error! */
     ERR( "cbEnhPaletteCopy didn't execute correctly\n" );
     return GDI_ERROR;
  }

  return infoForCallBack.cEntries;
}

/******************************************************************
 *         SetWinMetaFileBits   (GDI32.@)
 *
 *         Translate from old style to new style.
 *
 */
HENHMETAFILE WINAPI SetWinMetaFileBits(UINT cbBuffer,
					   CONST BYTE *lpbBuffer,
					   HDC hdcRef,
					   CONST METAFILEPICT *lpmfp
					   )
{
    HMETAFILE hmf = 0;
    HENHMETAFILE ret = 0;
    HDC hdc = 0, hdcdisp = 0;
    INT horzres, vertres;
    METAFILEPICT mfp;
    RECT rc, *prcFrame = NULL;

    TRACE("(%d, %p, %p, %p)\n", cbBuffer, lpbBuffer, hdcRef, lpmfp);

    if(!(hmf = SetMetaFileBitsEx(cbBuffer, lpbBuffer))) {
        WARN("SetMetaFileBitsEx fails\n");
	return 0;
    }

    if(!hdcRef)
        hdcRef = hdcdisp = CreateDCA("DISPLAY", NULL, NULL, NULL);

    if(!lpmfp) {
        lpmfp = &mfp;
	mfp.mm = MM_ANISOTROPIC;
	mfp.xExt = 100;
	mfp.yExt = 100;
	FIXME("Correct Exts from dc\n");
    } else
        TRACE("mm = %ld %ldx%ld\n", lpmfp->mm, lpmfp->xExt, lpmfp->yExt);

    horzres = GetDeviceCaps(hdcRef, HORZRES);
    vertres = GetDeviceCaps(hdcRef, VERTRES);

    if(lpmfp->mm == MM_ISOTROPIC || lpmfp->mm == MM_ANISOTROPIC) {
        rc.left = rc.top = 0;
	rc.right = lpmfp->xExt;
	rc.bottom = lpmfp->yExt;
	prcFrame = &rc;
    }

    if(!(hdc = CreateEnhMetaFileW(hdcRef, NULL, prcFrame, NULL))) {
        ERR("CreateEnhMetaFile fails?\n");
	goto end;
    }

    if(hdcdisp) {
        DeleteDC(hdcdisp);
	hdcRef = 0;
    }

    if(lpmfp->mm != MM_TEXT)
        SetMapMode(hdc, lpmfp->mm);

    SetViewportExtEx(hdc, horzres, vertres, NULL);
    SetWindowExtEx(hdc, horzres, vertres, NULL);

    PlayMetaFile(hdc, hmf); /* It's got to be harder than this... */

    ret = CloseEnhMetaFile(hdc);
 end:
    DeleteMetaFile(hmf);
    return ret;
}
