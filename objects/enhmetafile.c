/*
 * Enhanced metafile functions
 * Copyright 1998 Douglas Ridgway
 *           1999 Huw D M Davies 
 *
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

#include <string.h>
#include <assert.h>
#include "winbase.h"
#include "wingdi.h"
#include "wine/winestring.h"
#include "winerror.h"
#include "enhmetafile.h"
#include "debugtools.h"
#include "heap.h"
#include "metafile.h"

DEFAULT_DEBUG_CHANNEL(enhmetafile)

/****************************************************************************
 *          EMF_Create_HENHMETAFILE
 */
HENHMETAFILE EMF_Create_HENHMETAFILE(ENHMETAHEADER *emh, HFILE hFile, HANDLE
				     hMapping )
{
    HENHMETAFILE hmf = GDI_AllocObject( sizeof(ENHMETAFILEOBJ),
					ENHMETAFILE_MAGIC );
    ENHMETAFILEOBJ *metaObj = (ENHMETAFILEOBJ *)GDI_HEAP_LOCK( hmf );
    metaObj->emh = emh;
    metaObj->hFile = hFile;
    metaObj->hMapping = hMapping;
    GDI_HEAP_UNLOCK( hmf );
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
    if(metaObj->hMapping) {
        UnmapViewOfFile( metaObj->emh );
	CloseHandle( metaObj->hMapping );
	CloseHandle( metaObj->hFile );
    } else
        HeapFree( GetProcessHeap(), 0, metaObj->emh );
    return GDI_FreeObject( hmf );
}

/******************************************************************
 *         EMF_GetEnhMetaHeader
 *
 * Returns ptr to ENHMETAHEADER associated with HENHMETAFILE
 * Should be followed by call to EMF_ReleaseEnhMetaHeader
 */
static ENHMETAHEADER *EMF_GetEnhMetaHeader( HENHMETAFILE hmf )
{
    ENHMETAFILEOBJ *metaObj = (ENHMETAFILEOBJ *)GDI_GetObjPtr( hmf,
							   ENHMETAFILE_MAGIC );
    TRACE("hmf %04x -> enhmetaObj %p\n", hmf, metaObj);
    return metaObj ? metaObj->emh : NULL;
}

/******************************************************************
 *         EMF_ReleaseEnhMetaHeader
 *
 * Releases ENHMETAHEADER associated with HENHMETAFILE
 */
static BOOL EMF_ReleaseEnhMetaHeader( HENHMETAFILE hmf )
{
    return GDI_HEAP_UNLOCK( hmf );
}

/*****************************************************************************
 *         EMF_GetEnhMetaFile
 *
 */
static HENHMETAFILE EMF_GetEnhMetaFile( HFILE hFile )
{
    ENHMETAHEADER *emh;
    HANDLE hMapping;
    
    hMapping = CreateFileMappingA( hFile, NULL, PAGE_READONLY, 0, 0, NULL );
    emh = MapViewOfFile( hMapping, FILE_MAP_READ, 0, 0, 0 );

    if (emh->iType != EMR_HEADER || emh->dSignature != ENHMETA_SIGNATURE) {
        WARN("Invalid emf header type 0x%08lx sig 0x%08lx.\n",
	     emh->iType, emh->dSignature);
	UnmapViewOfFile( emh );
	CloseHandle( hMapping );
	return 0;
    }
    return EMF_Create_HENHMETAFILE( emh, hFile, hMapping );
}


/*****************************************************************************
 *          GetEnhMetaFileA (GDI32.174)
 *
 *
 */
HENHMETAFILE WINAPI GetEnhMetaFileA( 
	     LPCSTR lpszMetaFile  /* filename of enhanced metafile */
    )
{
    HENHMETAFILE hmf;
    HFILE hFile;

    hFile = CreateFileA(lpszMetaFile, GENERIC_READ, FILE_SHARE_READ, 0,
			OPEN_EXISTING, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE) {
        WARN("could not open %s\n", lpszMetaFile);
	return 0;
    }
    hmf = EMF_GetEnhMetaFile( hFile );
    if(!hmf)
        CloseHandle( hFile );
    return hmf;
}

/*****************************************************************************
 *          GetEnhMetaFileW  (GDI32.180)
 */
HENHMETAFILE WINAPI GetEnhMetaFileW(
             LPCWSTR lpszMetaFile)  /* filename of enhanced metafile */ 
{
    HENHMETAFILE hmf;
    HFILE hFile;

    hFile = CreateFileW(lpszMetaFile, GENERIC_READ, FILE_SHARE_READ, 0,
			OPEN_EXISTING, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE) {
        WARN("could not open %s\n", debugstr_w(lpszMetaFile));
	return 0;
    }
    hmf = EMF_GetEnhMetaFile( hFile );
    if(!hmf)
        CloseHandle( hFile );
    return hmf;
}

/*****************************************************************************
 *        GetEnhMetaFileHeader  (GDI32.178)
 *
 *  If buf is NULL, returns the size of buffer required.
 *  Otherwise, copy up to bufsize bytes of enhanced metafile header into 
 *  buf.
 */
UINT WINAPI GetEnhMetaFileHeader( 
       HENHMETAFILE hmf, /* enhanced metafile */
       UINT bufsize,     /* size of buffer */
       LPENHMETAHEADER buf /* buffer */ 
    )
{
    LPENHMETAHEADER emh;
    UINT size;

    emh = EMF_GetEnhMetaHeader(hmf);
    if(!emh) return FALSE;
    size = emh->nSize;
    if (!buf) {
        EMF_ReleaseEnhMetaHeader(hmf);
        return size;
    }
    size = min(size, bufsize);
    memmove(buf, emh, size);
    EMF_ReleaseEnhMetaHeader(hmf);
    return size;
}


/*****************************************************************************
 *          GetEnhMetaFileDescriptionA  (GDI32.176)
 */
UINT WINAPI GetEnhMetaFileDescriptionA( 
       HENHMETAFILE hmf, /* enhanced metafile */
       UINT size, /* size of buf */ 
       LPSTR buf /* buffer to receive description */
    )
{
     LPENHMETAHEADER emh = EMF_GetEnhMetaHeader(hmf);
     INT first, first_A;
 
     if(!emh) return FALSE;
     if(emh->nDescription == 0 || emh->offDescription == 0) {
         EMF_ReleaseEnhMetaHeader(hmf);
 	return 0;
     }
     if (!buf || !size ) {
         EMF_ReleaseEnhMetaHeader(hmf);
 	return emh->nDescription;
     }
 
     first = lstrlenW( (WCHAR *) ((char *) emh + emh->offDescription));
 
     lstrcpynWtoA(buf, (WCHAR *) ((char *) emh + emh->offDescription), size);
     first_A = lstrlenA( buf );
     buf += first_A + 1;
     lstrcpynWtoA(buf, (WCHAR *) ((char *) emh + emh->offDescription+2*(first+1)),
 		 size - first_A - 1); /* i18n ready */
     first_A += lstrlenA(buf) + 1;
 
     EMF_ReleaseEnhMetaHeader(hmf);
     return min(size, first_A);
}

/*****************************************************************************
 *          GetEnhMetaFileDescriptionW  (GDI32.177)
 *
 *  Copies the description string of an enhanced metafile into a buffer 
 *  _buf_.
 *
 *  If _buf_ is NULL, returns size of _buf_ required. Otherwise, returns
 *  number of characters copied.
 */
UINT WINAPI GetEnhMetaFileDescriptionW( 
       HENHMETAFILE hmf, /* enhanced metafile */
       UINT size, /* size of buf */ 
       LPWSTR buf /* buffer to receive description */
    )
{
     LPENHMETAHEADER emh = EMF_GetEnhMetaHeader(hmf);

     if(!emh) return FALSE;
     if(emh->nDescription == 0 || emh->offDescription == 0) {
         EMF_ReleaseEnhMetaHeader(hmf);
 	return 0;
     }
     if (!buf || !size ) {
         EMF_ReleaseEnhMetaHeader(hmf);
 	return emh->nDescription;
     }
 
     memmove(buf, (char *) emh + emh->offDescription, 
 	    min(size,emh->nDescription));
     EMF_ReleaseEnhMetaHeader(hmf);
     return min(size, emh->nDescription);
}

/****************************************************************************
 *    SetEnhMetaFileBits (GDI32.315)
 *
 *  Creates an enhanced metafile by copying _bufsize_ bytes from _buf_.
 */
HENHMETAFILE WINAPI SetEnhMetaFileBits(UINT bufsize, const BYTE *buf)
{
    ENHMETAHEADER *emh = HeapAlloc( GetProcessHeap(), 0, bufsize );
    memmove(emh, buf, bufsize);
    return EMF_Create_HENHMETAFILE( emh, 0, 0 );
}

/*****************************************************************************
 *  GetEnhMetaFileBits (GDI32.175)
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
    if( buf == NULL ) {
        EMF_ReleaseEnhMetaHeader( hmf ); 
	return size;
    }

    size = min( size, bufsize );
    memmove(buf, emh, size);

    EMF_ReleaseEnhMetaHeader( hmf ); 
    return size;
}

/*****************************************************************************
 *           PlayEnhMetaFileRecord  (GDI32.264)
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
     HDC hdc, /* device context in which to render EMF record */
     LPHANDLETABLE handletable, /* array of handles to be used in rendering record */
     const ENHMETARECORD *mr, /* EMF record to render */
     UINT handles  /* size of handle array */
     ) 
{
  int type;
  TRACE(
	"hdc = %08x, handletable = %p, record = %p, numHandles = %d\n", 
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
	PEMRSETWINDOWORGEX pSetWindowOrgEx = (PEMRSETWINDOWORGEX) mr;
	SetWindowOrgEx(hdc, pSetWindowOrgEx->ptlOrigin.x,
		       pSetWindowOrgEx->ptlOrigin.y, NULL);
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

    case EMR_POLYPOLYGON16:
      {
        PEMRPOLYPOLYGON16 pPolyPoly = (PEMRPOLYPOLYGON16) mr;
	/* NB POINTS array doesn't start at pPolyPoly->apts it's actually
	   pPolyPoly->aPolyCounts + pPolyPoly->nPolys */

	POINT *pts = HeapAlloc( GetProcessHeap(), 0,
				pPolyPoly->cpts * sizeof(POINT) );
	DWORD i;
	for(i = 0; i < pPolyPoly->cpts; i++)
	  CONV_POINT16TO32((POINTS*) (pPolyPoly->aPolyCounts +
				      pPolyPoly->nPolys) + i, pts + i);

	PolyPolygon(hdc, pts, (INT*)pPolyPoly->aPolyCounts, pPolyPoly->nPolys);
	HeapFree( GetProcessHeap(), 0, pts );
	break;
      }

    case EMR_STRETCHDIBITS:
      {
	LONG xDest = mr->dParm[4];
	LONG yDest = mr->dParm[5];
	LONG xSrc = mr->dParm[6];
	LONG ySrc = mr->dParm[7];
	LONG cxSrc = mr->dParm[8];
	LONG cySrc = mr->dParm[9];
	DWORD offBmiSrc = mr->dParm[10];
	DWORD offBitsSrc = mr->dParm[12];
	DWORD iUsageSrc = mr->dParm[14];
	DWORD dwRop = mr->dParm[15];
	LONG cxDest = mr->dParm[16];
	LONG cyDest = mr->dParm[17];

	StretchDIBits(hdc,xDest,yDest,cxDest,cyDest,
			    xSrc,ySrc,cxSrc,cySrc,
			    ((char *)mr)+offBitsSrc,
			    (const BITMAPINFO *)(((char *)mr)+offBmiSrc),
			    iUsageSrc,dwRop);
	break;
      }
    case EMR_EXTTEXTOUTW:
      {
	/* 0-3: ??? */
	DWORD flags = mr->dParm[4];
	/* 5, 6: ??? */
	DWORD x = mr->dParm[7], y = mr->dParm[8];
	DWORD count = mr->dParm[9];
	/* 10-16: ??? */
	LPWSTR str = (LPWSTR)& mr->dParm[17];
	/* trailing info: dx array? */
	FIXME("Many ExtTextOut args not handled\n");
	ExtTextOutW(hdc, x, y, flags, /* lpRect */ NULL, 
		      str, count, /* lpDx */ NULL); 
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

	/* FIXME: Should this be forcing background mode? */
	(handletable->objectHandle)[ lpSelectPal->ihPal ] = 
		SelectPalette( hdc, lpSelectPal->ihPal, FALSE );
	break;
      }

    case EMR_REALIZEPALETTE:
      {
	RealizePalette( hdc );
	break;
      }

#if 0
    case EMR_EXTSELECTCLIPRGN:
      {
	PEMREXTSELECTCLIPRGN lpRgn = (PEMREXTSELECTCLIPRGN)mr;

	/* Need to make a region out of the RGNDATA we have */
	ExtSelectClipRgn( hdc, ..., (INT)(lpRgn->iMode) );

      }
#endif

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
        PERMSETICMMODE lpSetICMMode = (PERMSETICMMODE)mr;
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

        /* This is a BITMAPINFO struct followed directly by bitmap bits */
        LPVOID lpPackedStruct = HeapAlloc( GetProcessHeap(), 
                                           0, 
                                           lpCreate->cbBmi + lpCreate->cbBits );
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

        break; 
      }

    case EMR_BITBLT:
    case EMR_STRETCHBLT:
    case EMR_MASKBLT:
    case EMR_PLGBLT:
    case EMR_SETDIBITSTODEVICE:
    case EMR_EXTTEXTOUTA:
    case EMR_POLYBEZIER16:
    case EMR_POLYBEZIERTO16:
    case EMR_POLYLINETO16:
    case EMR_POLYPOLYLINE16:
    case EMR_POLYDRAW16:
    case EMR_CREATEMONOBRUSH:
    case EMR_POLYTEXTOUTA:
    case EMR_POLYTEXTOUTW:
    case EMR_FILLRGN:
    case EMR_FRAMERGN:
    case EMR_INVERTRGN:
    case EMR_PAINTRGN:
    case EMR_GLSRECORD:
    case EMR_GLSBOUNDEDRECORD:
    default:
      /* From docs: If PlayEnhMetaFileRecord doesn't recognize a 
                    record then ignore and return TRUE. */
      FIXME("type %d is unimplemented\n", type);
      break;
    }
  return TRUE;
}


/*****************************************************************************
 *
 *        EnumEnhMetaFile  (GDI32.79)
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
     HDC hdc, /* device context to pass to _EnhMetaFunc_ */
     HENHMETAFILE hmf, /* EMF to walk */
     ENHMFENUMPROC callback, /* callback function */ 
     LPVOID data, /* optional data for callback function */
     const RECT *lpRect  /* bounding rectangle for rendered metafile */
    )
{
    BOOL ret = TRUE;
    LPENHMETAHEADER emh = EMF_GetEnhMetaHeader(hmf);
    INT count, i;
    HANDLETABLE *ht;
    INT savedMode = 0;
    FLOAT xSrcPixSize, ySrcPixSize, xscale, yscale;
    XFORM savedXform, xform;

    if(!emh) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if(!lpRect) {
        SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
    count = emh->nHandles;
    ht = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
		    sizeof(HANDLETABLE) * count );
    ht->objectHandle[0] = hmf;

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
    if(emh->rclFrame.left || emh->rclFrame.top)
      FIXME("Can't cope with nonzero rclFrame origin yet\n");
 /* eDx = lpRect->left - (lpRect width) / (rclFrame width) * rclFrame.left ? */
    xform.eDx = lpRect->left;
    xform.eDy = lpRect->top;
    savedMode = SetGraphicsMode(hdc, GM_ADVANCED);
    GetWorldTransform(hdc, &savedXform);
    if (!ModifyWorldTransform(hdc, &xform, MWT_LEFTMULTIPLY)) {
        ERR("World transform failed!\n");
    }

    while (ret) {
        ret = (*callback)(hdc, ht, (LPENHMETARECORD) emh, count, data); 
	if (emh->iType == EMR_EOF) break;
	emh = (LPENHMETAHEADER) ((char *) emh + emh->nSize);
    }
    for(i = 1; i < count; i++) /* Don't delete element 0 (hmf) */
        if( (ht->objectHandle)[i] )
	    DeleteObject( (ht->objectHandle)[i] );
    HeapFree( GetProcessHeap(), 0, ht );
    EMF_ReleaseEnhMetaHeader(hmf);
    SetWorldTransform(hdc, &savedXform);
    if (savedMode) SetGraphicsMode(hdc, savedMode);
    return ret;
}

static INT CALLBACK EMF_PlayEnhMetaFileCallback(HDC hdc, HANDLETABLE *ht,
						ENHMETARECORD *emr,
						INT handles, LPVOID data)
{
    return PlayEnhMetaFileRecord(hdc, ht, emr, handles);
}
						
/**************************************************************************
 *    PlayEnhMetaFile  (GDI32.263)
 *
 *    Renders an enhanced metafile into a specified rectangle *lpRect
 *    in device context hdc.
 *
 */
BOOL WINAPI PlayEnhMetaFile( 
       HDC hdc, /* DC to render into */
       HENHMETAFILE hmf, /* metafile to render */
       const RECT *lpRect  /* rectangle to place metafile inside */
      )
{
    return EnumEnhMetaFile(hdc, hmf, EMF_PlayEnhMetaFileCallback, NULL,
			   lpRect);
}

/*****************************************************************************
 *  DeleteEnhMetaFile (GDI32.68)
 *
 *  Deletes an enhanced metafile and frees the associated storage.
 */
BOOL WINAPI DeleteEnhMetaFile(HENHMETAFILE hmf)
{
    return EMF_Delete_HENHMETAFILE( hmf );
}

/*****************************************************************************
 *  CopyEnhMetaFileA (GDI32.21)  Duplicate an enhanced metafile
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
	hmfDst = EMF_Create_HENHMETAFILE( emrDst, 0, 0 );
    } else {
        HFILE hFile;
        hFile = CreateFileA( file, GENERIC_WRITE | GENERIC_READ, 0, NULL,
			     CREATE_ALWAYS, 0, -1);
	WriteFile( hFile, emrSrc, emrSrc->nBytes, 0, 0);
	hmfDst = EMF_GetEnhMetaFile( hFile );
    }
    EMF_ReleaseEnhMetaHeader( hmfSrc );
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
 *  GetEnhMetaFilePaletteEntries (GDI32.179)  
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
  UINT uReturnValue = GDI_ERROR;
  EMF_PaletteCopy infoForCallBack; 

  TRACE( "(%04x,%d,%p)\n", hEmf, cEntries, lpPe ); 

  /* First check if there are any palettes associated with
     this metafile. */
  if ( enhHeader->nPalEntries == 0 )
  {
    /* No palette associated with this enhanced metafile */
    uReturnValue = 0;
    goto done; 
  }

  /* Is the user requesting the number of palettes? */
  if ( lpPe == NULL )
  {
     uReturnValue = (UINT)enhHeader->nPalEntries;
     goto done;
  } 

  /* Copy cEntries worth of PALETTEENTRY structs into the buffer */
  infoForCallBack.cEntries = cEntries;
  infoForCallBack.lpPe     = lpPe; 

  if ( !EnumEnhMetaFile( 0, hEmf, cbEnhPaletteCopy, 
                         &infoForCallBack, NULL ) )
  {
     goto done; 
  }

  /* Verify that the callback executed correctly */
  if ( infoForCallBack.lpPe != NULL )
  {
     /* Callback proc had error! */
     ERR( "cbEnhPaletteCopy didn't execute correctly\n" );
     goto done;
  }

  uReturnValue = infoForCallBack.cEntries;

done:
  
  EMF_ReleaseEnhMetaHeader( hEmf );

  return uReturnValue;
}

/******************************************************************
 *         SetWinMetaFileBits   (GDI32.343)
 *      
 *         Translate from old style to new style.
 *
 * BUGS: - This doesn't take the DC and scaling into account
 *       - Most record conversions aren't implemented
 *       - Handle slot assignement is primative and most likely doesn't work
 */
HENHMETAFILE WINAPI SetWinMetaFileBits(UINT cbBuffer,
					   CONST BYTE *lpbBuffer,
					   HDC hdcRef,
					   CONST METAFILEPICT *lpmfp
					   ) 
{
     HENHMETAFILE    hMf;
     LPVOID          lpNewEnhMetaFileBuffer = NULL;
     UINT            uNewEnhMetaFileBufferSize = 0;
     BOOL            bFoundEOF = FALSE;

     FIXME( "(%d,%p,%04x,%p):stub\n", cbBuffer, lpbBuffer, hdcRef, lpmfp );

     /* 1. Get the header - skip over this and get straight to the records  */

     uNewEnhMetaFileBufferSize = sizeof( ENHMETAHEADER );
     lpNewEnhMetaFileBuffer = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 
                                         uNewEnhMetaFileBufferSize );

     if( lpNewEnhMetaFileBuffer == NULL )
     {
       goto error; 
     }

     /* Fill in the header record */ 
     {
       LPENHMETAHEADER lpNewEnhMetaFileHeader = (LPENHMETAHEADER)lpNewEnhMetaFileBuffer;

       lpNewEnhMetaFileHeader->iType = EMR_HEADER;
       lpNewEnhMetaFileHeader->nSize = sizeof( ENHMETAHEADER );

       /* FIXME: Not right. Must be able to get this from the DC */
       lpNewEnhMetaFileHeader->rclBounds.left   = 0;
       lpNewEnhMetaFileHeader->rclBounds.right  = 0;
       lpNewEnhMetaFileHeader->rclBounds.top    = 0;
       lpNewEnhMetaFileHeader->rclBounds.bottom = 0;

       /* FIXME: Not right. Must be able to get this from the DC */
       lpNewEnhMetaFileHeader->rclFrame.left   = 0;
       lpNewEnhMetaFileHeader->rclFrame.right  = 0;
       lpNewEnhMetaFileHeader->rclFrame.top    = 0;
       lpNewEnhMetaFileHeader->rclFrame.bottom = 0;

       lpNewEnhMetaFileHeader->nHandles = 0; /* No handles yet */

       /* FIXME: Add in the rest of the fields to the header */
       /* dSignature
          nVersion
          nRecords
          sReserved
          nDescription
          offDescription
          nPalEntries
          szlDevice
          szlMillimeters
          cbPixelFormat
          offPixelFormat,
          bOpenGL */
     } 

     (char*)lpbBuffer += ((METAHEADER*)lpbBuffer)->mtHeaderSize * 2; /* Point past the header - FIXME: metafile quirk? */ 

     /* 2. Enum over individual records and convert them to the new type of records */
     while( !bFoundEOF )
     { 

        LPMETARECORD lpMetaRecord = (LPMETARECORD)lpbBuffer;

#define EMF_ReAllocAndAdjustPointers( a , b ) \
                                        { \
                                          LPVOID lpTmp; \
                                          lpTmp = HeapReAlloc( GetProcessHeap(), 0, \
                                                               lpNewEnhMetaFileBuffer, \
                                                               uNewEnhMetaFileBufferSize + (b) ); \
                                          if( lpTmp == NULL ) { ERR( "No memory!\n" ); goto error; } \
                                          lpNewEnhMetaFileBuffer = lpTmp; \
                                          lpRecord = (a)( (char*)lpNewEnhMetaFileBuffer + uNewEnhMetaFileBufferSize ); \
                                          uNewEnhMetaFileBufferSize += (b); \
                                        }

        switch( lpMetaRecord->rdFunction )
        {
          case META_EOF:  
          { 
             PEMREOF lpRecord;
             size_t uRecord = sizeof(*lpRecord);

             EMF_ReAllocAndAdjustPointers(PEMREOF,uRecord);
              
             /* Fill the new record - FIXME: This is not right */
             lpRecord->emr.iType = EMR_EOF; 
             lpRecord->emr.nSize = sizeof( *lpRecord );
             lpRecord->nPalEntries = 0;     /* FIXME */
             lpRecord->offPalEntries = 0;   /* FIXME */ 
             lpRecord->nSizeLast = 0;       /* FIXME */

             /* No more records after this one */
             bFoundEOF = TRUE;

             FIXME( "META_EOF conversion not correct\n" );
             break;
          }

          case META_SETMAPMODE:
          {  
             PEMRSETMAPMODE lpRecord;
             size_t uRecord = sizeof(*lpRecord);

             EMF_ReAllocAndAdjustPointers(PEMRSETMAPMODE,uRecord);

             lpRecord->emr.iType = EMR_SETMAPMODE;
             lpRecord->emr.nSize = sizeof( *lpRecord );

             lpRecord->iMode = lpMetaRecord->rdParm[0];

             break;
          }

          case META_DELETEOBJECT: /* Select and Delete structures are the same */
          case META_SELECTOBJECT: 
          {
            PEMRDELETEOBJECT lpRecord;
            size_t uRecord = sizeof(*lpRecord);

            EMF_ReAllocAndAdjustPointers(PEMRDELETEOBJECT,uRecord);

            if( lpMetaRecord->rdFunction == META_DELETEOBJECT )
            {
              lpRecord->emr.iType = EMR_DELETEOBJECT;
            }
            else
            {
              lpRecord->emr.iType = EMR_SELECTOBJECT;
            }
            lpRecord->emr.nSize = sizeof( *lpRecord );

            lpRecord->ihObject = lpMetaRecord->rdParm[0]; /* FIXME: Handle */

            break;
          }

          case META_POLYGON: /* This is just plain busted. I don't know what I'm doing */
          {
             PEMRPOLYGON16 lpRecord; /* FIXME: Should it be a poly or poly16? */  
             size_t uRecord = sizeof(*lpRecord);

             EMF_ReAllocAndAdjustPointers(PEMRPOLYGON16,uRecord);

             /* FIXME: This is mostly all wrong */
             lpRecord->emr.iType = EMR_POLYGON16;
             lpRecord->emr.nSize = sizeof( *lpRecord );

             lpRecord->rclBounds.left   = 0;
             lpRecord->rclBounds.right  = 0;
             lpRecord->rclBounds.top    = 0;
             lpRecord->rclBounds.bottom = 0;

             lpRecord->cpts = 0;
             lpRecord->apts[0].x = 0;
             lpRecord->apts[0].y = 0;

             FIXME( "META_POLYGON conversion not correct\n" );

             break;
          }

          case META_SETPOLYFILLMODE:
          {
             PEMRSETPOLYFILLMODE lpRecord;
             size_t uRecord = sizeof(*lpRecord);

             EMF_ReAllocAndAdjustPointers(PEMRSETPOLYFILLMODE,uRecord);

             lpRecord->emr.iType = EMR_SETPOLYFILLMODE;
             lpRecord->emr.nSize = sizeof( *lpRecord );

             lpRecord->iMode = lpMetaRecord->rdParm[0];
             
             break;
          }

          case META_SETWINDOWORG:
          {
             PEMRSETWINDOWORGEX lpRecord; /* Seems to be the closest thing */
             size_t uRecord = sizeof(*lpRecord);

             EMF_ReAllocAndAdjustPointers(PEMRSETWINDOWORGEX,uRecord);

             lpRecord->emr.iType = EMR_SETWINDOWORGEX;
             lpRecord->emr.nSize = sizeof( *lpRecord );

             lpRecord->ptlOrigin.x = lpMetaRecord->rdParm[1];
             lpRecord->ptlOrigin.y = lpMetaRecord->rdParm[0];

             break;
          }

          case META_SETWINDOWEXT:  /* Structure is the same for SETWINDOWEXT & SETVIEWPORTEXT */
          case META_SETVIEWPORTEXT:
          {
             PEMRSETWINDOWEXTEX lpRecord;
             size_t uRecord = sizeof(*lpRecord);

             EMF_ReAllocAndAdjustPointers(PEMRSETWINDOWEXTEX,uRecord);

             if ( lpMetaRecord->rdFunction == META_SETWINDOWEXT )
             {
               lpRecord->emr.iType = EMR_SETWINDOWORGEX;
             }
             else
             {
               lpRecord->emr.iType = EMR_SETVIEWPORTEXTEX;
             }
             lpRecord->emr.nSize = sizeof( *lpRecord );

             lpRecord->szlExtent.cx = lpMetaRecord->rdParm[1];
             lpRecord->szlExtent.cy = lpMetaRecord->rdParm[0];

             break;
          }

          case META_CREATEBRUSHINDIRECT:
          {
             PEMRCREATEBRUSHINDIRECT lpRecord;
             size_t uRecord = sizeof(*lpRecord);

             EMF_ReAllocAndAdjustPointers(PEMRCREATEBRUSHINDIRECT,uRecord);

             lpRecord->emr.iType = EMR_CREATEBRUSHINDIRECT;
             lpRecord->emr.nSize = sizeof( *lpRecord );

             lpRecord->ihBrush    = ((LPENHMETAHEADER)lpNewEnhMetaFileBuffer)->nHandles;
             lpRecord->lb.lbStyle = ((LPLOGBRUSH16)lpMetaRecord->rdParm)->lbStyle; 
             lpRecord->lb.lbColor = ((LPLOGBRUSH16)lpMetaRecord->rdParm)->lbColor; 
             lpRecord->lb.lbHatch = ((LPLOGBRUSH16)lpMetaRecord->rdParm)->lbHatch; 
             
             ((LPENHMETAHEADER)lpNewEnhMetaFileBuffer)->nHandles += 1; /* New handle */

             break;
          }


          /* These are all unimplemented and as such are intended to fall through to the default case */
          case META_SETBKCOLOR:
          case META_SETBKMODE:
          case META_SETROP2:
          case META_SETRELABS:
          case META_SETSTRETCHBLTMODE:
          case META_SETTEXTCOLOR:
          case META_SETVIEWPORTORG:
          case META_OFFSETWINDOWORG:
          case META_SCALEWINDOWEXT:
          case META_OFFSETVIEWPORTORG:
          case META_SCALEVIEWPORTEXT:
          case META_LINETO:
          case META_MOVETO:
          case META_EXCLUDECLIPRECT:
          case META_INTERSECTCLIPRECT:
          case META_ARC:
          case META_ELLIPSE:
          case META_FLOODFILL:
          case META_PIE:
          case META_RECTANGLE:
          case META_ROUNDRECT:
          case META_PATBLT:
          case META_SAVEDC:
          case META_SETPIXEL:
          case META_OFFSETCLIPRGN:
          case META_TEXTOUT:
          case META_POLYPOLYGON:
          case META_POLYLINE:
          case META_RESTOREDC:  
          case META_CHORD:
          case META_CREATEPATTERNBRUSH:
          case META_CREATEPENINDIRECT:
          case META_CREATEFONTINDIRECT:
          case META_CREATEPALETTE:
          case META_SETTEXTALIGN:
          case META_SELECTPALETTE:
          case META_SETMAPPERFLAGS:
          case META_REALIZEPALETTE:
          case META_ESCAPE:
          case META_EXTTEXTOUT:
          case META_STRETCHDIB:
          case META_DIBSTRETCHBLT:
          case META_STRETCHBLT:
          case META_BITBLT:
          case META_CREATEREGION:
          case META_FILLREGION:
          case META_FRAMEREGION:
          case META_INVERTREGION:
          case META_PAINTREGION:
          case META_SELECTCLIPREGION:
          case META_DIBCREATEPATTERNBRUSH:
          case META_DIBBITBLT:
          case META_SETTEXTCHAREXTRA:
          case META_SETTEXTJUSTIFICATION:
          case META_EXTFLOODFILL:
          case META_SETDIBTODEV:
          case META_DRAWTEXT:
          case META_ANIMATEPALETTE:
          case META_SETPALENTRIES:
          case META_RESIZEPALETTE:
          case META_RESETDC:
          case META_STARTDOC:
          case META_STARTPAGE:
          case META_ENDPAGE:
          case META_ABORTDOC:
          case META_ENDDOC:
          case META_CREATEBRUSH:
          case META_CREATEBITMAPINDIRECT:
          case META_CREATEBITMAP:    
          /* Fall through to unimplemented */
          default:
          {
            /* Not implemented yet */
            FIXME( "Conversion of record type 0x%x not implemented.\n", lpMetaRecord->rdFunction );
            break;
          }
       }

       /* Move to the next record */
       (char*)lpbBuffer += ((LPMETARECORD)lpbBuffer)->rdSize * 2; /* FIXME: Seem to be doing this in metafile.c */ 

#undef ReAllocAndAdjustPointers
     } 

     /* We know the last of the header information now */
     ((LPENHMETAHEADER)lpNewEnhMetaFileBuffer)->nBytes = uNewEnhMetaFileBufferSize;

     /* Create the enhanced metafile */
     hMf = SetEnhMetaFileBits( uNewEnhMetaFileBufferSize, (const BYTE*)lpNewEnhMetaFileBuffer ); 

     if( !hMf )
       ERR( "Problem creating metafile. Did the conversion fail somewhere?\n" );

     return hMf;

error:
     /* Free the data associated with our copy since it's been copied */
     HeapFree( GetProcessHeap(), 0, lpNewEnhMetaFileBuffer ); 

     return 0;  
}



