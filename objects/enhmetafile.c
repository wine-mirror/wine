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

/* Prototypes */
BOOL WINAPI EnumEnhMetaFile( HDC hdc, HENHMETAFILE hmf, ENHMFENUMPROC callback,
                             LPVOID data, const RECT *rect );


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
 *          GetEnhMetaFile32W  (GDI32.180)
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
 *  If _buf_ is NULL, returns the size of buffer required.
 *  Otherwise, copy up to _bufsize_ bytes of enhanced metafile header into 
 *  _buf.
 */
UINT WINAPI GetEnhMetaFileHeader( 
       HENHMETAFILE hmf, /* enhanced metafile */
       UINT bufsize,     /* size of buffer */
       LPENHMETAHEADER buf /* buffer */ 
    )
{
    LPENHMETAHEADER emh;

    if (!buf) return sizeof(ENHMETAHEADER);
    emh = EMF_GetEnhMetaHeader(hmf);
    if(!emh) return FALSE;
    memmove(buf, emh, min(sizeof(ENHMETAHEADER), bufsize));
    EMF_ReleaseEnhMetaHeader(hmf);
    return min(sizeof(ENHMETAHEADER), bufsize);
}


/*****************************************************************************
 *          GetEnhMetaFileDescription32A  (GDI32.176)
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
 *          GetEnhMetaFileDescription32W  (GDI32.177)
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

INT CALLBACK cbCountSizeOfEnhMetaFile( HDC a, 
                                       LPHANDLETABLE b,
                                       LPENHMETARECORD lpEMR, 
                                       INT c, 
                                       LPVOID lpData )
{
  LPUINT uSizeOfRecordData = (LPUINT)lpData;

  *uSizeOfRecordData += lpEMR->nSize;

  return TRUE;
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
  LPENHMETAHEADER lpEnhMetaFile;
  UINT uEnhMetaFileSize = 0;

  FIXME( "(%04x,%u,%p): untested\n", hmf, bufsize, buf );

  /* Determine the required buffer size */
  /* Enumerate all records and count their size */ 
  if( !EnumEnhMetaFile( 0, hmf, cbCountSizeOfEnhMetaFile, &uEnhMetaFileSize, NULL ) )
  {
    ERR( "Unable to enumerate enhanced metafile!\n" );
    return 0;
  }

  if( buf == NULL )
  {
    return uEnhMetaFileSize;
  }

  /* Copy the lesser of the two byte counts */
  uEnhMetaFileSize = min( uEnhMetaFileSize, bufsize );

  /* Copy everything */
  lpEnhMetaFile = EMF_GetEnhMetaHeader( hmf );

  if( lpEnhMetaFile == NULL )
  {
    return 0;
  }

  /* Use memmove just in case they overlap */
  memmove(buf, lpEnhMetaFile, bufsize);

  EMF_ReleaseEnhMetaHeader( hmf ); 

  return bufsize;
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
      {
#if 0
	ENHMETAHEADER* h = (LPENHMETAHEADER)mr;
        XFORM dcTransform;

        /* Scale the enhanced metafile according to the reference hdc
           it was created with */
        if( h->szlDevice.cx )
        {
          dcTransform.eM11 = (FLOAT)( (DOUBLE)GetDeviceCaps( hdc, HORZRES ) /  
                                      (DOUBLE)h->szlDevice.cx );
        }
        else 
        {
          ERR( "Invalid szlDevice.cx in header\n" ); 
          dcTransform.eM11 = (FLOAT)1.0;
        }

        if( h->szlDevice.cy )
        { 
          dcTransform.eM22 = (FLOAT)( (DOUBLE)GetDeviceCaps( hdc, VERTRES ) /
                                      (DOUBLE)h->szlDevice.cy );
        }
        else 
        {
          ERR( "Invalid szlDevice.cy in header\n" ); 
          dcTransform.eM22 = (FLOAT)1.0;
        }

        dcTransform.eM12 = dcTransform.eM21 = (FLOAT)0;
        dcTransform.eDx = dcTransform.eDy = (FLOAT)0;

        ModifyWorldTransform( hdc, &dcTransform, MWT_RIGHTMULTIPLY ); 
#endif
	break;
      }
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
	DWORD mode = mr->dParm[0];
	SetMapMode(hdc, mode);
	break;
      }
    case EMR_SETBKMODE:
      {
	DWORD mode = mr->dParm[0];
	SetBkMode(hdc, mode);
	break;
      }
    case EMR_SETBKCOLOR:
      {
	DWORD mode = mr->dParm[0];
	SetBkColor(hdc, mode);
	break;
      }
    case EMR_SETPOLYFILLMODE:
      {
	DWORD mode = mr->dParm[0];
	SetPolyFillMode(hdc, mode);
	break;
      }
    case EMR_SETROP2:
      {
	DWORD mode = mr->dParm[0];
	SetROP2(hdc, mode);
	break;
      }
    case EMR_SETSTRETCHBLTMODE:
      {
	DWORD mode = mr->dParm[0];
	SetStretchBltMode(hdc, mode);
	break;
      }
    case EMR_SETTEXTALIGN:
      {
	DWORD align = mr->dParm[0];
	SetTextAlign(hdc, align);
	break;
      }
    case EMR_SETTEXTCOLOR:
      {
	DWORD color = mr->dParm[0];
	SetTextColor(hdc, color);
	break;
      }
    case EMR_SAVEDC:
      {
	SaveDC(hdc);
	break;
      }
    case EMR_RESTOREDC:
      {
	RestoreDC(hdc, mr->dParm[0]);
	break;
      }
    case EMR_INTERSECTCLIPRECT:
      {
	INT left = mr->dParm[0], top = mr->dParm[1], right = mr->dParm[2],
	      bottom = mr->dParm[3];
	IntersectClipRect(hdc, left, top, right, bottom);
	break;
      }
    case EMR_SELECTOBJECT:
      {
	DWORD obj = mr->dParm[0];
	SelectObject(hdc, (handletable->objectHandle)[obj]);
	break;
      }
    case EMR_DELETEOBJECT:
      {
	DWORD obj = mr->dParm[0];
	DeleteObject( (handletable->objectHandle)[obj]);
	(handletable->objectHandle)[obj] = 0;
	break;
      }
    case EMR_SETWINDOWORGEX:
      {
	DWORD x = mr->dParm[0], y = mr->dParm[1];
	SetWindowOrgEx(hdc, x, y, NULL);
	break;
      }
    case EMR_SETWINDOWEXTEX:
      {
	DWORD x = mr->dParm[0], y = mr->dParm[1];
	SetWindowExtEx(hdc, x, y, NULL);
	break;
      }
    case EMR_SETVIEWPORTORGEX:
      {
	DWORD x = mr->dParm[0], y = mr->dParm[1];
	SetViewportOrgEx(hdc, x, y, NULL);
	break;
      }
    case EMR_SETVIEWPORTEXTEX:
      {
	DWORD x = mr->dParm[0], y = mr->dParm[1];
	SetViewportExtEx(hdc, x, y, NULL);
	break;
      }
    case EMR_CREATEPEN:
      {
	DWORD obj = mr->dParm[0];
	(handletable->objectHandle)[obj] = 
	  CreatePenIndirect((LOGPEN *) &(mr->dParm[1]));
	break;
      }
    case EMR_EXTCREATEPEN:
      {
	DWORD obj = mr->dParm[0];
	DWORD style = mr->dParm[1], brush = mr->dParm[2];
	LOGBRUSH *b = (LOGBRUSH *) &mr->dParm[3];
	FIXME("Some ExtCreatePen args not handled\n");
	(handletable->objectHandle)[obj] = 
	  ExtCreatePen(style, brush, b, 0, NULL);
	break;
      }
    case EMR_CREATEBRUSHINDIRECT:
      {
	DWORD obj = mr->dParm[0];
	(handletable->objectHandle)[obj] = 
	  CreateBrushIndirect((LOGBRUSH *) &(mr->dParm[1]));
	break;
      }
    case EMR_EXTCREATEFONTINDIRECTW:
	{
	DWORD obj = mr->dParm[0];
	(handletable->objectHandle)[obj] = 
	  CreateFontIndirectW((LOGFONTW *) &(mr->dParm[1]));
	break;
	}
    case EMR_MOVETOEX:
      {
	DWORD x = mr->dParm[0], y = mr->dParm[1];
	MoveToEx(hdc, x, y, NULL);
	break;
      }
    case EMR_LINETO:
      {
	DWORD x = mr->dParm[0], y = mr->dParm[1];
        LineTo(hdc, x, y);
	break;
      }
    case EMR_RECTANGLE:
      {
	INT left = mr->dParm[0], top = mr->dParm[1], right = mr->dParm[2],
	      bottom = mr->dParm[3];
	Rectangle(hdc, left, top, right, bottom);
	break;
      }
    case EMR_ELLIPSE:
      {
	INT left = mr->dParm[0], top = mr->dParm[1], right = mr->dParm[2],
	      bottom = mr->dParm[3];
	Ellipse(hdc, left, top, right, bottom);
	break;
      }
    case EMR_POLYGON16:
      {
	/* 0-3 : a bounding rectangle? */
	INT count = mr->dParm[4];
	FIXME("Some Polygon16 args not handled\n");
	Polygon16(hdc, (POINT16 *)&mr->dParm[5], count);
	break;
      }
    case EMR_POLYLINE16:
      {
	/* 0-3 : a bounding rectangle? */
	INT count = mr->dParm[4];
	FIXME("Some Polyline16 args not handled\n");
	Polyline16(hdc, (POINT16 *)&mr->dParm[5], count);
	break;
      }
#if 0
    case EMR_POLYPOLYGON16:
      {
        PEMRPOLYPOLYGON16 lpPolyPoly16 = (PEMRPOLYPOLYGON16)mr;

	PolyPolygon16( hdc, 
                       lpPolyPoly16->apts, 
                       lpPolyPoly16->aPolyCounts,
                       lpPolyPoly16->nPolys );

	break;
      }
#endif
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

        FIXME( "Bounding rectangle ignored for EMR_POLYBEZIER\n" );
        PolyBezier( hdc, (const LPPOINT)lpPolyBez->aptl, (UINT)lpPolyBez->cptl ); 

        break;
      }

    case EMR_POLYGON:
      {
        PEMRPOLYGON lpPoly = (PEMRPOLYGON)mr;
     
        FIXME( "Bounding rectangle ignored for EMR_POLYGON\n" );
        Polygon( hdc, (const LPPOINT)lpPoly->aptl, (UINT)lpPoly->cptl );

        break;
      }

    case EMR_POLYLINE:
      {
        PEMRPOLYLINE lpPolyLine = (PEMRPOLYLINE)mr;
 
        FIXME( "Bounding rectangle ignored for EMR_POLYLINE\n" ); 
        Polyline( hdc, (const LPPOINT)lpPolyLine->aptl, (UINT)lpPolyLine->cptl );

        break; 
      }

    case EMR_POLYBEZIERTO:
      {
        PEMRPOLYBEZIERTO lpPolyBezierTo = (PEMRPOLYBEZIERTO)mr;

        FIXME( "Bounding rectangle ignored for EMR_POLYBEZIERTO\n" );
        PolyBezierTo( hdc, (const LPPOINT)lpPolyBezierTo->aptl, (UINT)lpPolyBezierTo->cptl );

        break; 
      }

    case EMR_POLYLINETO:
      {
        PEMRPOLYLINETO lpPolyLineTo = (PEMRPOLYLINETO)mr;

        FIXME( "Bounding rectangle ignored for EMR_POLYLINETO\n" );
        PolylineTo( hdc, (const LPPOINT)lpPolyLineTo->aptl, (UINT)lpPolyLineTo->cptl );

        break;
      }

    case EMR_POLYPOLYLINE:
      {
        PEMRPOLYPOLYLINE lpPolyPolyLine = (PEMRPOLYPOLYLINE)mr;

        FIXME( "Bounding rectangle ignored for EMR_POLYPOLYLINE\n" );
        PolyPolyline( hdc, 
                      (const LPPOINT)lpPolyPolyLine->aptl, 
                      lpPolyPolyLine->aPolyCounts, 
                      lpPolyPolyLine->nPolys ); 

        break;
      }

    case EMR_POLYPOLYGON:
      {
        PEMRPOLYPOLYGON lpPolyPolygon = (PEMRPOLYPOLYGON)mr;
        INT* lpPolyCounts;
        UINT i;

        /* We get the polygon point counts in a dword struct. Transform
           this into an INT struct */ 
        lpPolyCounts = (INT*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                        sizeof(INT)*lpPolyPolygon->cptl ); 

        for( i=0; i<lpPolyPolygon->cptl; i++ )
        {
          lpPolyCounts[i] = (INT)lpPolyPolygon->aPolyCounts[i];
        }

        FIXME( "Bounding rectangle ignored for EMR_POLYPOLYGON\n" );
        PolyPolygon( hdc, (const LPPOINT)lpPolyPolygon->aptl,
                     lpPolyCounts, lpPolyPolygon->nPolys );

        HeapFree( GetProcessHeap(), 0, lpPolyCounts ); 

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

        FIXME( "Bounding rectangle ignored for EMR_POLYDRAW\n" );
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

        FIXME( "Bounding rectangle ignored for EMR_FILLPATH\n" );
        FillPath( hdc );

        break;
      }

    case EMR_STROKEANDFILLPATH:
      {
        /*PEMRSTROKEANDFILLPATH lpStrokeAndFillPath = (PEMRSTROKEANDFILLPATH)mr;*/

        FIXME( "Bounding rectangle ignored for EMR_STROKEANDFILLPATH\n" );
        StrokeAndFillPath( hdc );

        break;
      }

    case EMR_STROKEPATH:
      {
        /*PEMRSTROKEPATH lpStrokePath = (PEMRSTROKEPATH)mr;*/

        FIXME( "Bounding rectangle ignored for EMR_STROKEPATH\n" );  
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

        SetICMMode( hdc,
                    (INT)lpSetICMMode->iMode );

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
    case EMR_POLYPOLYGON16:
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
 *        EnumEnhMetaFile32  (GDI32.79)
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
     const RECT *rect  /* bounding rectangle for rendered metafile */
    )
{
    BOOL ret = TRUE;
    LPENHMETARECORD p = (LPENHMETARECORD) EMF_GetEnhMetaHeader(hmf);
    INT count;
    HANDLETABLE *ht;

    if(!p) return FALSE;
    count = ((LPENHMETAHEADER) p)->nHandles;
    ht = HeapAlloc( GetProcessHeap(), 0, sizeof(HANDLETABLE)*count);
    ht->objectHandle[0] = hmf;
    while (ret) {
        ret = (*callback)(hdc, ht, p, count, data); 
	if (p->iType == EMR_EOF) break;
	p = (LPENHMETARECORD) ((char *) p + p->nSize);
    }
    HeapFree( GetProcessHeap(), 0, ht);
    EMF_ReleaseEnhMetaHeader(hmf);
    return ret;
}


/**************************************************************************
 *    PlayEnhMetaFile  (GDI32.263)
 *
 *    Renders an enhanced metafile into a specified rectangle *lpRect
 *    in device context hdc.
 *
 * BUGS
 *    Almost entirely unimplemented
 *
 */
BOOL WINAPI PlayEnhMetaFile( 
       HDC hdc, /* DC to render into */
       HENHMETAFILE hmf, /* metafile to render */
       const RECT *lpRect  /* rectangle to place metafile inside */
      )
{
    LPENHMETARECORD p = (LPENHMETARECORD) EMF_GetEnhMetaHeader(hmf);
    INT count;
    HANDLETABLE *ht;
    BOOL ret = FALSE;
    INT savedMode = 0;

    if(!p) return FALSE;
    count = ((LPENHMETAHEADER) p)->nHandles;
    ht = HeapAlloc( GetProcessHeap(), 0, sizeof(HANDLETABLE) * count);
    if (lpRect) {
        LPENHMETAHEADER h = (LPENHMETAHEADER) p;
	FLOAT xscale = (h->rclBounds.right - h->rclBounds.left) /
	  (lpRect->right - lpRect->left);
	FLOAT yscale = (h->rclBounds.bottom - h->rclBounds.top) /
	  (lpRect->bottom - lpRect->top);
	XFORM xform;
	xform.eM11 = xscale;
	xform.eM12 = 0;
	xform.eM21 = 0;
	xform.eM22 = yscale;
        xform.eDx = lpRect->left;
	xform.eDy = lpRect->top; 
	FIXME("play into rect doesn't work\n");
	savedMode = SetGraphicsMode(hdc, GM_ADVANCED);
	if (!SetWorldTransform(hdc, &xform)) {
	    WARN("World transform failed!\n");
	}
    }

    ht->objectHandle[0] = hmf;
    while (1) {
        PlayEnhMetaFileRecord(hdc, ht, p, count);
	if (p->iType == EMR_EOF) break;
	p = (LPENHMETARECORD) ((char *) p + p->nSize); /* casted so that arithmetic is in bytes */
    }
    HeapFree( GetProcessHeap(), 0, ht );
    EMF_ReleaseEnhMetaHeader(hmf);
    if (savedMode) SetGraphicsMode(hdc, savedMode);
    ret = TRUE; /* FIXME: calculate a more accurate return value */
    return ret;
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



