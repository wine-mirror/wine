/*
 * Metafile functions
 *
 * Copyright  David W. Metcalfe, 1994
 *            Niels de Carpentier, Albrecht Kleine, Huw Davies 1996
 *
 */

#include <string.h>
#include <fcntl.h>
#include "wine/winbase16.h"
#include "metafiledrv.h"
#include "metafile.h"
#include "bitmap.h"
#include "heap.h"
#include "toolhelp.h"
#include "debug.h"

/******************************************************************
 *         MF_AddHandle
 *
 *    Add a handle to an external handle table and return the index
 */

static int MF_AddHandle(HANDLETABLE16 *ht, WORD htlen, HGDIOBJ16 hobj)
{
    int i;

    for (i = 0; i < htlen; i++)
    {
	if (*(ht->objectHandle + i) == 0)
	{
	    *(ht->objectHandle + i) = hobj;
	    return i;
	}
    }
    return -1;
}


/******************************************************************
 *         MF_AddHandleDC
 *
 * Note: this function assumes that we never delete objects.
 * If we do someday, we'll need to maintain a table to re-use deleted
 * handles.
 */
static int MF_AddHandleDC( DC *dc )
{
    METAFILEDRV_PDEVICE *physDev = (METAFILEDRV_PDEVICE *)dc->physDev;
    physDev->mh->mtNoObjects++;
    return physDev->nextHandle++;
}


/******************************************************************
 *         GetMetaFile16   (GDI.124)
 */
HMETAFILE16 WINAPI GetMetaFile16( LPCSTR lpFilename )
{
    return GetMetaFileA( lpFilename );
}


/******************************************************************
 *         GetMetaFile32A   (GDI32.197)
 *
 *  Read a metafile from a file. Returns handle to a disk-based metafile.
 */
HMETAFILE WINAPI GetMetaFileA( 
				  LPCSTR lpFilename 
		      /* pointer to string containing filename to read */
)
{
  HMETAFILE16 hmf;
  METAHEADER *mh;
  HFILE hFile;
  DWORD size;
  
  TRACE(metafile,"%s\n", lpFilename);

  if (!lpFilename)
    return 0;

  hmf = GlobalAlloc16(GMEM_MOVEABLE, MFHEADERSIZE);
  mh = (METAHEADER *)GlobalLock16(hmf);
  
  if (!mh)
  {
    GlobalFree16(hmf);
    return 0;
  }
  
  if ((hFile = _lopen(lpFilename, OF_READ)) == HFILE_ERROR)
  {
    GlobalFree16(hmf);
    return 0;
  }
  
  if (_lread(hFile, (char *)mh, MFHEADERSIZE) == HFILE_ERROR)
  {
    _lclose( hFile );
    GlobalFree16(hmf);
    return 0;
  }
  
  size = mh->mtSize * 2;         /* alloc memory for whole metafile */
  GlobalUnlock16(hmf);
  hmf = GlobalReAlloc16(hmf,size,GMEM_MOVEABLE);
  mh = (METAHEADER *)GlobalLock16(hmf);
  
  if (!mh)
  {
    _lclose( hFile );
    GlobalFree16(hmf);
    return 0;
  }
  
  if (_lread(hFile, (char*)mh + mh->mtHeaderSize * 2, 
	        size - mh->mtHeaderSize * 2) == HFILE_ERROR)
  {
    _lclose( hFile );
    GlobalFree16(hmf);
    return 0;
  }
  
  _lclose(hFile);

  if (mh->mtType != 1)
  {
    GlobalFree16(hmf);
    return 0;
  }
  
  GlobalUnlock16(hmf);
  return hmf;

}


/******************************************************************
 *         GetMetaFile32W   (GDI32.199)
 */
HMETAFILE WINAPI GetMetaFileW( LPCWSTR lpFilename )
{
    LPSTR p = HEAP_strdupWtoA( GetProcessHeap(), 0, lpFilename );
    HMETAFILE ret = GetMetaFileA( p );
    HeapFree( GetProcessHeap(), 0, p );
    return ret;
}


/******************************************************************
 *         CopyMetaFile16   (GDI.151)
 */

HMETAFILE16 WINAPI CopyMetaFile16( HMETAFILE16 hSrcMetaFile, LPCSTR lpFilename )
{
    return CopyMetaFileA( hSrcMetaFile, lpFilename );
}


/******************************************************************
 *         CopyMetaFile32A   (GDI32.23)
 *
 *  Copies the metafile corresponding to hSrcMetaFile to either
 *  a disk file, if a filename is given, or to a new memory based
 *  metafile, if lpFileName is NULL.
 *
 * RETURNS
 *
 *  Handle to metafile copy on success, NULL on failure.
 *
 * BUGS
 *
 *  Copying to disk returns NULL even if successful.
 */
HMETAFILE WINAPI CopyMetaFileA(
		   HMETAFILE hSrcMetaFile, /* handle of metafile to copy */
		   LPCSTR lpFilename /* filename if copying to a file */
) {
    HMETAFILE16 handle = 0;
    METAHEADER *mh;
    METAHEADER *mh2;
    HFILE hFile;
    
    TRACE(metafile,"(%08x,%s)\n", hSrcMetaFile, lpFilename);
    
    mh = (METAHEADER *)GlobalLock16(hSrcMetaFile);
    
    if (!mh)
      return 0;
    
    if (lpFilename)          /* disk based metafile */
        {
        int i,j;
	hFile = _lcreat(lpFilename, 0);
	j=mh->mtType;
	mh->mtType=1;        /* disk file version stores 1 here */
	i=_lwrite(hFile, (char *)mh, mh->mtSize * 2) ;
	mh->mtType=j;        /* restore old value  [0 or 1] */	
        _lclose(hFile);
	if (i == -1)
	    return 0;
        /* FIXME: return value */
        }
    else                     /* memory based metafile */
        {
	handle = GlobalAlloc16(GMEM_MOVEABLE,mh->mtSize * 2);
	mh2 = (METAHEADER *)GlobalLock16(handle);
	memcpy(mh2,mh, mh->mtSize * 2);
	GlobalUnlock16(handle);
        }

    GlobalUnlock16(hSrcMetaFile);
    return handle;
}


/******************************************************************
 *         CopyMetaFile32W   (GDI32.24)
 */
HMETAFILE WINAPI CopyMetaFileW( HMETAFILE hSrcMetaFile,
                                    LPCWSTR lpFilename )
{
    LPSTR p = HEAP_strdupWtoA( GetProcessHeap(), 0, lpFilename );
    HMETAFILE ret = CopyMetaFileA( hSrcMetaFile, p );
    HeapFree( GetProcessHeap(), 0, p );
    return ret;
}


/******************************************************************
 *         IsValidMetaFile   (GDI.410)
 *
 *  Attempts to check if a given metafile is correctly formatted.
 *  Currently, the only things verified are several properties of the
 *  header.
 *
 * RETURNS
 *  TRUE if hmf passes some tests for being a valid metafile, FALSE otherwise.
 *
 * BUGS
 *  This is not exactly what windows does, see _Undocumented_Windows_
 *  for details.
 */

BOOL16 WINAPI IsValidMetaFile16(HMETAFILE16 hmf)
{
    BOOL16 resu=FALSE;
    METAHEADER *mh = (METAHEADER *)GlobalLock16(hmf);
    if (mh) {
      if (mh->mtType == 1 || mh->mtType == 0) 
        if (mh->mtHeaderSize == MFHEADERSIZE/sizeof(INT16))
          if (mh->mtVersion == MFVERSION)
            resu=TRUE;
      GlobalUnlock16(hmf);
    }
    TRACE(metafile,"IsValidMetaFile %x => %d\n",hmf,resu);
    return resu;         
}


/******************************************************************
 *         PlayMetaFile16   (GDI.123)
 *
 */
BOOL16 WINAPI PlayMetaFile16( HDC16 hdc, HMETAFILE16 hmf )
{
    return PlayMetaFile( hdc, hmf );
}

/******************************************************************
 *         PlayMetaFile32   (GDI32.265)
 *
 *  Renders the metafile specified by hmf in the DC specified by
 *  hdc. Returns FALSE on failure, TRUE on success.
 */
BOOL WINAPI PlayMetaFile( 
			     HDC hdc, /* handle of DC to render in */
			     HMETAFILE hmf /* handle of metafile to render */
)
{
    METAHEADER *mh = (METAHEADER *)GlobalLock16(hmf);
    METARECORD *mr;
    HANDLETABLE16 *ht;
    HGLOBAL16 hHT;
    int offset = 0;
    WORD i;
    HPEN hPen;
    HBRUSH hBrush;
    HFONT hFont;
    DC *dc;
    
    TRACE(metafile,"(%04x %04x)\n",hdc,hmf);
    if (!mh) return FALSE;

    /* save the current pen, brush and font */
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    hPen = dc->w.hPen;
    hBrush = dc->w.hBrush;
    hFont = dc->w.hFont;
    GDI_HEAP_UNLOCK(hdc);
    /* create the handle table */
    hHT = GlobalAlloc16(GMEM_MOVEABLE|GMEM_ZEROINIT,
		      sizeof(HANDLETABLE16) * mh->mtNoObjects);
    ht = (HANDLETABLE16 *)GlobalLock16(hHT);

    
    /* loop through metafile playing records */
    offset = mh->mtHeaderSize * 2;
    while (offset < mh->mtSize * 2)
    {
        mr = (METARECORD *)((char *)mh + offset);
	TRACE(metafile,"offset=%04x,size=%08lx\n",
            offset, mr->rdSize);
	if (!mr->rdSize) {
            TRACE(metafile,"Entry got size 0 at offset %d, total mf length is %ld\n",
                offset,mh->mtSize*2);
		break; /* would loop endlessly otherwise */
	}
	offset += mr->rdSize * 2;
	PlayMetaFileRecord16( hdc, ht, mr, mh->mtNoObjects );
    }

    SelectObject(hdc, hBrush);
    SelectObject(hdc, hPen);
    SelectObject(hdc, hFont);

    /* free objects in handle table */
    for(i = 0; i < mh->mtNoObjects; i++)
      if(*(ht->objectHandle + i) != 0)
        DeleteObject(*(ht->objectHandle + i));
    
    /* free handle table */
    GlobalFree16(hHT);

    return TRUE;
}


/******************************************************************
 *            EnumMetaFile16   (GDI.175)
 *
 *  Loop through the metafile records in hmf, calling the user-specified
 *  function for each one, stopping when the user's function returns FALSE
 *  (which is considered to be failure)
 *  or when no records are left (which is considered to be success). 
 *
 * RETURNS
 *  TRUE on success, FALSE on failure.
 * 
 * HISTORY
 *   Niels de carpentier, april 1996
 */
BOOL16 WINAPI EnumMetaFile16( 
			     HDC16 hdc, 
			     HMETAFILE16 hmf,
			     MFENUMPROC16 lpEnumFunc, 
			     LPARAM lpData 
)
{
    METAHEADER *mh = (METAHEADER *)GlobalLock16(hmf);
    METARECORD *mr;
    HANDLETABLE16 *ht;
    HGLOBAL16 hHT;
    SEGPTR spht;
    int offset = 0;
    WORD i, seg;
    HPEN hPen;
    HBRUSH hBrush;
    HFONT hFont;
    DC *dc;
    BOOL16 result = TRUE;
    
    TRACE(metafile,"(%04x, %04x, %08lx, %08lx)\n",
		     hdc, hmf, (DWORD)lpEnumFunc, lpData);

    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    hPen = dc->w.hPen;
    hBrush = dc->w.hBrush;
    hFont = dc->w.hFont;
    GDI_HEAP_UNLOCK(hdc);

    /* create the handle table */
    
    hHT = GlobalAlloc16(GMEM_MOVEABLE | GMEM_ZEROINIT,
		     sizeof(HANDLETABLE16) * mh->mtNoObjects);
    spht = WIN16_GlobalLock16(hHT);
   
    seg = GlobalHandleToSel16(hmf);
    offset = mh->mtHeaderSize * 2;
    
    /* loop through metafile records */
    
    while (offset < (mh->mtSize * 2))
    {
	mr = (METARECORD *)((char *)mh + offset);
        if (!lpEnumFunc( hdc, (HANDLETABLE16 *)spht,
			 (METARECORD *) PTR_SEG_OFF_TO_HUGEPTR(seg, offset),
                         mh->mtNoObjects, (LONG)lpData ))
	{
	    result = FALSE;
	    break;
	}
	

	offset += (mr->rdSize * 2);
    }

    SelectObject(hdc, hBrush);
    SelectObject(hdc, hPen);
    SelectObject(hdc, hFont);

    ht = (HANDLETABLE16 *)GlobalLock16(hHT);

    /* free objects in handle table */
    for(i = 0; i < mh->mtNoObjects; i++)
      if(*(ht->objectHandle + i) != 0)
        DeleteObject(*(ht->objectHandle + i));

    /* free handle table */
    GlobalFree16(hHT);
    GlobalUnlock16(hmf);
    return result;
}

BOOL WINAPI EnumMetaFile( 
			     HDC hdc, 
			     HMETAFILE hmf,
			     MFENUMPROC lpEnumFunc, 
			     LPARAM lpData 
) {
    METAHEADER *mh = (METAHEADER *)GlobalLock16(hmf);
    METARECORD *mr;
    HANDLETABLE *ht;
    BOOL result = TRUE;
    int i, offset = 0;
    DC *dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    HPEN hPen;
    HBRUSH hBrush;
    HFONT hFont;

    TRACE(metafile,"(%08x,%08x,%p,%p)\n",
		     hdc, hmf, lpEnumFunc, (void*)lpData);
    if (!mh) return 0;

    /* save the current pen, brush and font */
    if (!dc) return 0;
    hPen = dc->w.hPen;
    hBrush = dc->w.hBrush;
    hFont = dc->w.hFont;
    GDI_HEAP_UNLOCK(hdc);


    ht = (HANDLETABLE *) GlobalAlloc(GPTR, 
			    sizeof(HANDLETABLE) * mh->mtNoObjects);
    
    /* loop through metafile records */
    offset = mh->mtHeaderSize * 2;
    
    while (offset < (mh->mtSize * 2))
    {
	mr = (METARECORD *)((char *)mh + offset);
        if (!lpEnumFunc( hdc, ht, mr, mh->mtNoObjects, (LONG)lpData ))
	{
	    result = FALSE;
	    break;
	}
	
	offset += (mr->rdSize * 2);
    }

    /* restore pen, brush and font */
    SelectObject(hdc, hBrush);
    SelectObject(hdc, hPen);
    SelectObject(hdc, hFont);

    /* free objects in handle table */
    for(i = 0; i < mh->mtNoObjects; i++)
      if(*(ht->objectHandle + i) != 0)
        DeleteObject(*(ht->objectHandle + i));

    /* free handle table */
    GlobalFree((HGLOBAL)ht);
    GlobalUnlock16(hmf);
    return result;
}

static BOOL MF_Meta_CreateRegion( METARECORD *mr, HRGN hrgn );

/******************************************************************
 *             PlayMetaFileRecord16   (GDI.176)
 *
 *   Render a single metafile record specified by *mr in the DC hdc, while
 *   using the handle table *ht, of length nHandles, 
 *   to store metafile objects.
 *
 * BUGS
 *  The following metafile records are unimplemented:
 *
 *  FRAMEREGION, DRAWTEXT, SETDIBTODEV, ANIMATEPALETTE, SETPALENTRIES,
 *  RESIZEPALETTE, EXTFLOODFILL, RESETDC, STARTDOC, STARTPAGE, ENDPAGE,
 *  ABORTDOC, ENDDOC, CREATEBRUSH, CREATEBITMAPINDIRECT, and CREATEBITMAP.
 *
 */
void WINAPI PlayMetaFileRecord16( 
	    HDC16 hdc, /* DC to render metafile into */
	    HANDLETABLE16 *ht, /* pointer to handle table for metafile objects */
	    METARECORD *mr, /* pointer to metafile record to render */
	    UINT16 nHandles /* size of handle table */
) {
    short s1;
    HANDLE16 hndl;
    char *ptr;
    BITMAPINFOHEADER *infohdr;

    TRACE(metafile,"(%04x %08lx %08lx %04x) function %04x\n",
		 hdc,(LONG)ht, (LONG)mr, nHandles, mr->rdFunction);
    
    switch (mr->rdFunction)
    {
    case META_EOF:
      break;

    case META_DELETEOBJECT:
      DeleteObject(*(ht->objectHandle + *(mr->rdParm)));
      *(ht->objectHandle + *(mr->rdParm)) = 0;
      break;

    case META_SETBKCOLOR:
	SetBkColor16(hdc, MAKELONG(*(mr->rdParm), *(mr->rdParm + 1)));
	break;

    case META_SETBKMODE:
	SetBkMode16(hdc, *(mr->rdParm));
	break;

    case META_SETMAPMODE:
	SetMapMode16(hdc, *(mr->rdParm));
	break;

    case META_SETROP2:
	SetROP216(hdc, *(mr->rdParm));
	break;

    case META_SETRELABS:
	SetRelAbs16(hdc, *(mr->rdParm));
	break;

    case META_SETPOLYFILLMODE:
	SetPolyFillMode16(hdc, *(mr->rdParm));
	break;

    case META_SETSTRETCHBLTMODE:
	SetStretchBltMode16(hdc, *(mr->rdParm));
	break;

    case META_SETTEXTCOLOR:
	SetTextColor16(hdc, MAKELONG(*(mr->rdParm), *(mr->rdParm + 1)));
	break;

    case META_SETWINDOWORG:
	SetWindowOrg16(hdc, *(mr->rdParm + 1), *(mr->rdParm));
	break;

    case META_SETWINDOWEXT:
	SetWindowExt16(hdc, *(mr->rdParm + 1), *(mr->rdParm));
	break;

    case META_SETVIEWPORTORG:
	SetViewportOrg16(hdc, *(mr->rdParm + 1), *(mr->rdParm));
	break;

    case META_SETVIEWPORTEXT:
	SetViewportExt16(hdc, *(mr->rdParm + 1), *(mr->rdParm));
	break;

    case META_OFFSETWINDOWORG:
	OffsetWindowOrg16(hdc, *(mr->rdParm + 1), *(mr->rdParm));
	break;

    case META_SCALEWINDOWEXT:
	ScaleWindowExt16(hdc, *(mr->rdParm + 3), *(mr->rdParm + 2),
		       *(mr->rdParm + 1), *(mr->rdParm));
	break;

    case META_OFFSETVIEWPORTORG:
	OffsetViewportOrg16(hdc, *(mr->rdParm + 1), *(mr->rdParm));
	break;

    case META_SCALEVIEWPORTEXT:
	ScaleViewportExt16(hdc, *(mr->rdParm + 3), *(mr->rdParm + 2),
			 *(mr->rdParm + 1), *(mr->rdParm));
	break;

    case META_LINETO:
	LineTo(hdc, (INT16)*(mr->rdParm + 1), (INT16)*(mr->rdParm));
	break;

    case META_MOVETO:
	MoveTo16(hdc, *(mr->rdParm + 1), *(mr->rdParm));
	break;

    case META_EXCLUDECLIPRECT:
	ExcludeClipRect16( hdc, *(mr->rdParm + 3), *(mr->rdParm + 2),
                           *(mr->rdParm + 1), *(mr->rdParm) );
	break;

    case META_INTERSECTCLIPRECT:
	IntersectClipRect16( hdc, *(mr->rdParm + 3), *(mr->rdParm + 2),
                             *(mr->rdParm + 1), *(mr->rdParm) );
	break;

    case META_ARC:
	Arc(hdc, (INT16)*(mr->rdParm + 7), (INT16)*(mr->rdParm + 6),
	      (INT16)*(mr->rdParm + 5), (INT16)*(mr->rdParm + 4),
	      (INT16)*(mr->rdParm + 3), (INT16)*(mr->rdParm + 2),
             (INT16)*(mr->rdParm + 1), (INT16)*(mr->rdParm));
	break;

    case META_ELLIPSE:
	Ellipse(hdc, (INT16)*(mr->rdParm + 3), (INT16)*(mr->rdParm + 2),
                  (INT16)*(mr->rdParm + 1), (INT16)*(mr->rdParm));
	break;

    case META_FLOODFILL:
	FloodFill(hdc, (INT16)*(mr->rdParm + 3), (INT16)*(mr->rdParm + 2),
                    MAKELONG(*(mr->rdParm), *(mr->rdParm + 1)));
	break;

    case META_PIE:
	Pie(hdc, (INT16)*(mr->rdParm + 7), (INT16)*(mr->rdParm + 6),
	      (INT16)*(mr->rdParm + 5), (INT16)*(mr->rdParm + 4),
	      (INT16)*(mr->rdParm + 3), (INT16)*(mr->rdParm + 2),
             (INT16)*(mr->rdParm + 1), (INT16)*(mr->rdParm));
	break;

    case META_RECTANGLE:
	Rectangle(hdc, (INT16)*(mr->rdParm + 3), (INT16)*(mr->rdParm + 2),
                    (INT16)*(mr->rdParm + 1), (INT16)*(mr->rdParm));
	break;

    case META_ROUNDRECT:
	RoundRect(hdc, (INT16)*(mr->rdParm + 5), (INT16)*(mr->rdParm + 4),
                    (INT16)*(mr->rdParm + 3), (INT16)*(mr->rdParm + 2),
                    (INT16)*(mr->rdParm + 1), (INT16)*(mr->rdParm));
	break;

    case META_PATBLT:
	PatBlt16(hdc, *(mr->rdParm + 5), *(mr->rdParm + 4),
                 *(mr->rdParm + 3), *(mr->rdParm + 2),
                 MAKELONG(*(mr->rdParm), *(mr->rdParm + 1)));
	break;

    case META_SAVEDC:
	SaveDC(hdc);
	break;

    case META_SETPIXEL:
	SetPixel(hdc, (INT16)*(mr->rdParm + 3), (INT16)*(mr->rdParm + 2),
                   MAKELONG(*(mr->rdParm), *(mr->rdParm + 1)));
	break;

    case META_OFFSETCLIPRGN:
	OffsetClipRgn16( hdc, *(mr->rdParm + 1), *(mr->rdParm) );
	break;

    case META_TEXTOUT:
	s1 = *(mr->rdParm);
	TextOut16(hdc, *(mr->rdParm + ((s1 + 1) >> 1) + 2),
                  *(mr->rdParm + ((s1 + 1) >> 1) + 1), 
                  (char *)(mr->rdParm + 1), s1);
	break;

    case META_POLYGON:
	Polygon16(hdc, (LPPOINT16)(mr->rdParm + 1), *(mr->rdParm));
	break;

    case META_POLYPOLYGON:
      PolyPolygon16(hdc, (LPPOINT16)(mr->rdParm + *(mr->rdParm) + 1),
                    (LPINT16)(mr->rdParm + 1), *(mr->rdParm)); 
      break;

    case META_POLYLINE:
	Polyline16(hdc, (LPPOINT16)(mr->rdParm + 1), *(mr->rdParm));
	break;

    case META_RESTOREDC:
	RestoreDC(hdc, (INT16)*(mr->rdParm));
	break;

    case META_SELECTOBJECT:
	SelectObject(hdc, *(ht->objectHandle + *(mr->rdParm)));
	break;

    case META_CHORD:
	Chord(hdc, (INT16)*(mr->rdParm + 7), (INT16)*(mr->rdParm + 6),
		(INT16)*(mr->rdParm+5), (INT16)*(mr->rdParm + 4),
		(INT16)*(mr->rdParm + 3), (INT16)*(mr->rdParm + 2),
               (INT16)*(mr->rdParm + 1), (INT16)*(mr->rdParm));
	break;

    case META_CREATEPATTERNBRUSH:
	switch (*(mr->rdParm))
	{
	case BS_PATTERN:
	    infohdr = (BITMAPINFOHEADER *)(mr->rdParm + 2);
	    MF_AddHandle(ht, nHandles,
			 CreatePatternBrush(CreateBitmap(infohdr->biWidth, 
				      infohdr->biHeight, 
				      infohdr->biPlanes, 
				      infohdr->biBitCount,
				      (LPSTR)(mr->rdParm + 
				      (sizeof(BITMAPINFOHEADER) / 2) + 4))));
	    break;

	case BS_DIBPATTERN:
	    s1 = mr->rdSize * 2 - sizeof(METARECORD) - 2;
	    hndl = GlobalAlloc16(GMEM_MOVEABLE, s1);
	    ptr = GlobalLock16(hndl);
	    memcpy(ptr, mr->rdParm + 2, s1);
	    GlobalUnlock16(hndl);
	    MF_AddHandle(ht, nHandles,
			 CreateDIBPatternBrush(hndl, *(mr->rdParm + 1)));
	    GlobalFree16(hndl);
	}
	break;
	
    case META_CREATEPENINDIRECT:
	MF_AddHandle(ht, nHandles, 
		     CreatePenIndirect16((LOGPEN16 *)(&(mr->rdParm))));
	break;

    case META_CREATEFONTINDIRECT:
	MF_AddHandle(ht, nHandles, 
		     CreateFontIndirect16((LOGFONT16 *)(&(mr->rdParm))));
	break;

    case META_CREATEBRUSHINDIRECT:
	MF_AddHandle(ht, nHandles, 
		     CreateBrushIndirect16((LOGBRUSH16 *)(&(mr->rdParm))));
	break;

    /* W. Magro: Some new metafile operations.  Not all debugged. */
    case META_CREATEPALETTE:
	MF_AddHandle(ht, nHandles, 
		     CreatePalette16((LPLOGPALETTE)mr->rdParm));
	break;

    case META_SETTEXTALIGN:
       	SetTextAlign16(hdc, *(mr->rdParm));
	break;

    case META_SELECTPALETTE:
	SelectPalette16(hdc, *(ht->objectHandle + *(mr->rdParm+1)),*(mr->rdParm));
	break;

    case META_SETMAPPERFLAGS:
	SetMapperFlags16(hdc, *(mr->rdParm));
	break;

    case META_REALIZEPALETTE:
	RealizePalette16(hdc);
	break;

    case META_ESCAPE:
	FIXME(metafile, "META_ESCAPE unimplemented.\n");
        break;

        /* --- Begin of fixed or new metafile operations. July 1996 ----*/
    case META_EXTTEXTOUT:
      {
        LPINT16 dxx;
	LPSTR sot; 
        DWORD len;

        s1 = mr->rdParm[2];                              /* String length */
        len = sizeof(METARECORD) + (((s1 + 1) >> 1) * 2) + 2 * sizeof(short)
	 + sizeof(UINT16) +  (mr->rdParm[3] ? sizeof(RECT16) : 0);  /* rec len without dx array */

	sot= (LPSTR)&mr->rdParm[4];			/* start_of_text */
	if (mr->rdParm[3])
	   sot+=sizeof(RECT16);				/* there is a rectangle, so add offset */
	 
	if (mr->rdSize == len / 2)
          dxx = NULL;                                   /* determine if array present */
        else 
	  if (mr->rdSize == (len + s1 * sizeof(INT16)) / 2)
            dxx = (LPINT16)(sot+(((s1+1)>>1)*2));	   
          else 
	  {
	   TRACE(metafile,"%s  len: %ld\n",
             sot,mr->rdSize);
           WARN(metafile,
	     "Please report: PlayMetaFile/ExtTextOut len=%ld slen=%d rdSize=%ld opt=%04x\n",
		   len,s1,mr->rdSize,mr->rdParm[3]);
           dxx = NULL; /* should't happen -- but if, we continue with NULL [for workaround] */
          }
        ExtTextOut16( hdc, mr->rdParm[1],              /* X position */
	                   mr->rdParm[0],              /* Y position */
	                   mr->rdParm[3],              /* options */
		     	   mr->rdParm[3] ? (LPRECT16) &mr->rdParm[4]:NULL,  /* rectangle */
		           sot,				/* string */
                           s1, dxx);                    /* length, dx array */
        if (dxx)                      
          TRACE(metafile,"%s  len: %ld  dx0: %d\n",
            sot,mr->rdSize,dxx[0]);
       }
       break;
    
    case META_STRETCHDIB:
      {
       LPBITMAPINFO info = (LPBITMAPINFO) &(mr->rdParm[11]);
       LPSTR bits = (LPSTR)info + DIB_BitmapInfoSize( info, mr->rdParm[2] );
       StretchDIBits16(hdc,mr->rdParm[10],mr->rdParm[9],mr->rdParm[8],
                       mr->rdParm[7],mr->rdParm[6],mr->rdParm[5],
                       mr->rdParm[4],mr->rdParm[3],bits,info,
                       mr->rdParm[2],MAKELONG(mr->rdParm[0],mr->rdParm[1]));
      }
      break;

    case META_DIBSTRETCHBLT:
      {
       LPBITMAPINFO info = (LPBITMAPINFO) &(mr->rdParm[10]); 
       LPSTR bits = (LPSTR)info + DIB_BitmapInfoSize( info, mr->rdParm[2] );
       StretchDIBits16(hdc,mr->rdParm[9],mr->rdParm[8],mr->rdParm[7],
                       mr->rdParm[6],mr->rdParm[5],mr->rdParm[4],
                       mr->rdParm[3],mr->rdParm[2],bits,info,
                       DIB_RGB_COLORS,MAKELONG(mr->rdParm[0],mr->rdParm[1]));
      }
      break;		  

    case META_STRETCHBLT:
      {
       HDC16 hdcSrc=CreateCompatibleDC16(hdc);
       HBITMAP hbitmap=CreateBitmap(mr->rdParm[10], /*Width */
                                        mr->rdParm[11], /*Height*/
                                        mr->rdParm[13], /*Planes*/
                                        mr->rdParm[14], /*BitsPixel*/
                                        (LPSTR)&mr->rdParm[15]);  /*bits*/
       SelectObject(hdcSrc,hbitmap);
       StretchBlt16(hdc,mr->rdParm[9],mr->rdParm[8],
                    mr->rdParm[7],mr->rdParm[6],
		    hdcSrc,mr->rdParm[5],mr->rdParm[4],
		    mr->rdParm[3],mr->rdParm[2],
		    MAKELONG(mr->rdParm[0],mr->rdParm[1]));
       DeleteDC(hdcSrc);		    
      }
      break;

    case META_BITBLT:            /* <-- not yet debugged */
      {
       HDC16 hdcSrc=CreateCompatibleDC16(hdc);
       HBITMAP hbitmap=CreateBitmap(mr->rdParm[7]/*Width */,
                                        mr->rdParm[8]/*Height*/,
                                        mr->rdParm[10]/*Planes*/,
                                        mr->rdParm[11]/*BitsPixel*/,
                                        (LPSTR)&mr->rdParm[12]/*bits*/);
       SelectObject(hdcSrc,hbitmap);
       BitBlt(hdc,(INT16)mr->rdParm[6],(INT16)mr->rdParm[5],
                (INT16)mr->rdParm[4],(INT16)mr->rdParm[3],
                hdcSrc, (INT16)mr->rdParm[2],(INT16)mr->rdParm[1],
                MAKELONG(0,mr->rdParm[0]));
       DeleteDC(hdcSrc);		    
      }
      break;

       /* --- Begin of new metafile operations. April, 1997 (ak) ----*/
    case META_CREATEREGION:
      {
	HRGN hrgn = CreateRectRgn(0,0,0,0);
 
	MF_Meta_CreateRegion(mr, hrgn);
	MF_AddHandle(ht, nHandles, hrgn);
      }
      break;

     case META_FILLREGION:
        FillRgn16(hdc, *(ht->objectHandle + *(mr->rdParm)),
		       *(ht->objectHandle + *(mr->rdParm+1)));
        break;

     case META_INVERTREGION:
        InvertRgn16(hdc, *(ht->objectHandle + *(mr->rdParm)));
        break; 

     case META_PAINTREGION:
        PaintRgn16(hdc, *(ht->objectHandle + *(mr->rdParm)));
        break;

     case META_SELECTCLIPREGION:
       	SelectClipRgn(hdc, *(ht->objectHandle + *(mr->rdParm)));
	break;

     case META_DIBCREATEPATTERNBRUSH:
	/*  *(mr->rdParm) may be BS_PATTERN or BS_DIBPATTERN: but there's no difference */
        TRACE(metafile,"%d\n",*(mr->rdParm));
	s1 = mr->rdSize * 2 - sizeof(METARECORD) - 2;
	hndl = GlobalAlloc16(GMEM_MOVEABLE, s1);
	ptr = GlobalLock16(hndl);
	memcpy(ptr, mr->rdParm + 2, s1);
	GlobalUnlock16(hndl);
	MF_AddHandle(ht, nHandles,CreateDIBPatternBrush16(hndl, *(mr->rdParm + 1)));
	GlobalFree16(hndl);
	break;

     case META_DIBBITBLT:
        {
		/*In practice ive found that theres two layout for META_DIBBITBLT,
		one (the first here) is the usual one when a src dc is actually passed
		int, the second occurs when the src dc is passed in as NULL to
		the creating BitBlt.
		as the second case has no dib, a size check will suffice to distinguish.


		Caolan.McNamara@ul.ie
		*/
		if (mr->rdSize > 12)
                {
                    LPBITMAPINFO info = (LPBITMAPINFO) &(mr->rdParm[8]);
                    LPSTR bits = (LPSTR)info + DIB_BitmapInfoSize( info, mr->rdParm[0] );
                    StretchDIBits16(hdc,mr->rdParm[7],mr->rdParm[6],mr->rdParm[5],
                                    mr->rdParm[4],mr->rdParm[3],mr->rdParm[2],
                                    mr->rdParm[5],mr->rdParm[4],bits,info,
                                    DIB_RGB_COLORS,MAKELONG(mr->rdParm[0],mr->rdParm[1]));
                }
		else	/*equivalent to a PatBlt*/
                {
                    PatBlt16(hdc, mr->rdParm[8], mr->rdParm[7],
                             mr->rdParm[6], mr->rdParm[5],
                             MAKELONG(*(mr->rdParm), *(mr->rdParm + 1)));
                }
        }
        break;	
       
     case META_SETTEXTCHAREXTRA:
	    SetTextCharacterExtra16(hdc, (INT16)*(mr->rdParm));
	    break;

     case META_SETTEXTJUSTIFICATION:
       	SetTextJustification(hdc, *(mr->rdParm + 1), *(mr->rdParm));
	break;

    case META_EXTFLOODFILL:
        ExtFloodFill(hdc, (INT16)*(mr->rdParm + 4), (INT16)*(mr->rdParm + 3),
                     MAKELONG(*(mr->rdParm+1), *(mr->rdParm + 2)),*(mr->rdParm));
        break;

#define META_UNIMP(x) case x: FIXME(metafile, "PlayMetaFileRecord:record type "#x" not implemented.\n");break;
    META_UNIMP(META_FRAMEREGION)
    META_UNIMP(META_DRAWTEXT)
    META_UNIMP(META_SETDIBTODEV)
    META_UNIMP(META_ANIMATEPALETTE)
    META_UNIMP(META_SETPALENTRIES)
    META_UNIMP(META_RESIZEPALETTE)
    META_UNIMP(META_RESETDC)
    META_UNIMP(META_STARTDOC)
    META_UNIMP(META_STARTPAGE)
    META_UNIMP(META_ENDPAGE)
    META_UNIMP(META_ABORTDOC)
    META_UNIMP(META_ENDDOC)
    META_UNIMP(META_CREATEBRUSH)
    META_UNIMP(META_CREATEBITMAPINDIRECT)
    META_UNIMP(META_CREATEBITMAP)
#undef META_UNIMP

    default:
	WARN(metafile, "PlayMetaFileRecord: Unknown record type %x\n",
	                                      mr->rdFunction);
    }
}


BOOL WINAPI PlayMetaFileRecord( 
     HDC hdc, 
     HANDLETABLE *handletable, 
     METARECORD *metarecord, 
     UINT handles  
    )
{
  HANDLETABLE16 * ht = (void *)GlobalAlloc(GPTR, 
                               handles*sizeof(HANDLETABLE16));
  int i = 0;
  TRACE(metafile, "(%08x,%p,%p,%d)\n", hdc, handletable, metarecord, handles); 
  for (i=0; i<handles; i++)  
    ht->objectHandle[i] =  handletable->objectHandle[i];
  PlayMetaFileRecord16(hdc, ht, metarecord, handles);
  for (i=0; i<handles; i++) 
    handletable->objectHandle[i] = ht->objectHandle[i];
  GlobalFree((HGLOBAL)ht);
  return TRUE;
}

/******************************************************************
 *         GetMetaFileBits   (GDI.159)
 *
 * Trade in a metafile object handle for a handle to the metafile memory.
 *
 */

HGLOBAL16 WINAPI GetMetaFileBits16(
				 HMETAFILE16 hmf /* metafile handle */
				 )
{
    TRACE(metafile,"hMem out: %04x\n", hmf);
    return hmf;
}

/******************************************************************
 *         SetMetaFileBits   (GDI.160)
 *
 * Trade in a metafile memory handle for a handle to a metafile object.
 * The memory region should hold a proper metafile, otherwise
 * problems will occur when it is used. Validity of the memory is not
 * checked. The function is essentially just the identity function.
 */
HMETAFILE16 WINAPI SetMetaFileBits16( 
				   HGLOBAL16 hMem 
			/* handle to a memory region holding a metafile */
)
{
    TRACE(metafile,"hmf out: %04x\n", hMem);

    return hMem;
}

/******************************************************************
 *         SetMetaFileBitsBetter   (GDI.196)
 *
 * Trade in a metafile memory handle for a handle to a metafile object,
 * making a cursory check (using IsValidMetaFile()) that the memory
 * handle points to a valid metafile.
 *
 * RETURNS
 *  Handle to a metafile on success, NULL on failure..
 */
HMETAFILE16 WINAPI SetMetaFileBitsBetter16( HMETAFILE16 hMeta )
{
   if( IsValidMetaFile16( hMeta ) )
       return (HMETAFILE16)GlobalReAlloc16( hMeta, 0, 
			   GMEM_SHARE | GMEM_NODISCARD | GMEM_MODIFY);
   return (HMETAFILE16)0;
}

/******************************************************************
 *         SetMetaFileBitsEx    (GDI32.323)
 * 
 *  Create a metafile from raw data. No checking of the data is performed.
 *  Use _GetMetaFileBitsEx_ to get raw data from a metafile.
 */
HMETAFILE WINAPI SetMetaFileBitsEx( 
     UINT size, /* size of metafile, in bytes */
     const BYTE *lpData /* pointer to metafile data */  
    )
{
  HMETAFILE hmf = GlobalAlloc16(GHND, size);
  BYTE *p = GlobalLock16(hmf) ;
  TRACE(metafile, "(%d,%p) returning %08x\n", size, lpData, hmf);
  if (!hmf || !p) return 0;
  memcpy(p, lpData, size);
  GlobalUnlock16(hmf);
  return hmf;
}

/*****************************************************************
 *  GetMetaFileBitsEx     (GDI32.198)  Get raw metafile data
 * 
 *  Copies the data from metafile _hmf_ into the buffer _buf_.
 *  If _buf_ is zero, returns size of buffer required. Otherwise,
 *  returns number of bytes copied.
 */
UINT WINAPI GetMetaFileBitsEx( 
     HMETAFILE hmf, /* metafile */
     UINT nSize, /* size of buf */ 
     LPVOID buf   /* buffer to receive raw metafile data */  
) {
  METAHEADER *h = GlobalLock16(hmf);
  UINT mfSize;

  TRACE(metafile, "(%08x,%d,%p)\n", hmf, nSize, buf);
  if (!h) return 0;  /* FIXME: error code */
  mfSize = h->mtSize * 2;
  if (!buf) {
    GlobalUnlock16(hmf);
    TRACE(metafile,"returning size %d\n", mfSize);
    return mfSize;
  }
  if(mfSize > nSize) mfSize = nSize;
  memmove(buf, h, mfSize);
  GlobalUnlock16(hmf);
  return mfSize;
}

/******************************************************************
 *         GetWinMetaFileBits [GDI32.241]
 */
UINT WINAPI GetWinMetaFileBits(HENHMETAFILE hemf,
                                UINT cbBuffer, LPBYTE lpbBuffer,
                                INT fnMapMode, HDC hdcRef)
{
  FIXME(metafile, "(%d,%d,%p,%d,%d): stub\n",
        hemf, cbBuffer, lpbBuffer, fnMapMode, hdcRef);
  return 0;
}

/******************************************************************
 *         MF_Meta_CreateRegion
 *
 *  Handles META_CREATEREGION for PlayMetaFileRecord().
 */

/*
 *	The layout of the record looks something like this:
 *	 
 *	 rdParm	meaning
 *	 0		Always 0?
 *	 1		Always 6?
 *	 2		Looks like a handle? - not constant
 *	 3		0 or 1 ??
 *	 4		Total number of bytes
 *	 5		No. of seperate bands = n [see below]
 *	 6		Largest number of x co-ords in a band
 *	 7-10		Bounding box x1 y1 x2 y2
 *	 11-...		n bands
 *
 *	 Regions are divided into bands that are uniform in the
 *	 y-direction. Each band consists of pairs of on/off x-coords and is
 *	 written as
 *		m y0 y1 x1 x2 x3 ... xm m
 *	 into successive rdParm[]s.
 *
 *	 This is probably just a dump of the internal RGNOBJ?
 *
 *	 HDMD - 18/12/97
 *
 */

static BOOL MF_Meta_CreateRegion( METARECORD *mr, HRGN hrgn )
{
    WORD band, pair;
    WORD *start, *end;
    INT16 y0, y1;
    HRGN hrgn2 = CreateRectRgn( 0, 0, 0, 0 );

    for(band  = 0, start = &(mr->rdParm[11]); band < mr->rdParm[5];
 					        band++, start = end + 1) {
        if(*start / 2 != (*start + 1) / 2) {
 	    WARN(metafile, "Delimiter not even.\n");
	    DeleteObject( hrgn2 );
 	    return FALSE;
        }

	end = start + *start + 3;
	if(end > (WORD *)mr + mr->rdSize) {
	    WARN(metafile, "End points outside record.\n");
	    DeleteObject( hrgn2 );
	    return FALSE;
        }

	if(*start != *end) {
	    WARN(metafile, "Mismatched delimiters.\n");
	    DeleteObject( hrgn2 );
	    return FALSE;
	}

	y0 = *(INT16 *)(start + 1);
	y1 = *(INT16 *)(start + 2);
	for(pair = 0; pair < *start / 2; pair++) {
	    SetRectRgn( hrgn2, *(INT16 *)(start + 3 + 2*pair), y0,
				 *(INT16 *)(start + 4 + 2*pair), y1 );
	    CombineRgn(hrgn, hrgn, hrgn2, RGN_OR);
        }
    }
    DeleteObject( hrgn2 );
    return TRUE;
 }
 

/******************************************************************
 *         MF_WriteRecord
 *
 * Warning: this function can change the metafile handle.
 */

static BOOL MF_WriteRecord( DC *dc, METARECORD *mr, DWORD rlen)
{
    DWORD len;
    METAHEADER *mh;
    METAFILEDRV_PDEVICE *physDev = (METAFILEDRV_PDEVICE *)dc->physDev;

    switch(physDev->mh->mtType)
    {
    case METAFILE_MEMORY:
	len = physDev->mh->mtSize * 2 + rlen;
	mh = HeapReAlloc( SystemHeap, 0, physDev->mh, len );
        if (!mh) return FALSE;
        physDev->mh = mh;
	memcpy((WORD *)physDev->mh + physDev->mh->mtSize, mr, rlen);
        break;
    case METAFILE_DISK:
        TRACE(metafile,"Writing record to disk\n");
	if (_lwrite(physDev->mh->mtNoParameters, (char *)mr, rlen) == -1)
	    return FALSE;
        break;
    default:
        ERR(metafile, "Unknown metafile type %d\n", physDev->mh->mtType );
        return FALSE;
    }

    physDev->mh->mtSize += rlen / 2;
    physDev->mh->mtMaxRecord = MAX(physDev->mh->mtMaxRecord, rlen / 2);
    return TRUE;
}


/******************************************************************
 *         MF_MetaParam0
 */

BOOL MF_MetaParam0(DC *dc, short func)
{
    char buffer[8];
    METARECORD *mr = (METARECORD *)&buffer;
    
    mr->rdSize = 3;
    mr->rdFunction = func;
    return MF_WriteRecord( dc, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MF_MetaParam1
 */
BOOL MF_MetaParam1(DC *dc, short func, short param1)
{
    char buffer[8];
    METARECORD *mr = (METARECORD *)&buffer;
    
    mr->rdSize = 4;
    mr->rdFunction = func;
    *(mr->rdParm) = param1;
    return MF_WriteRecord( dc, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MF_MetaParam2
 */
BOOL MF_MetaParam2(DC *dc, short func, short param1, short param2)
{
    char buffer[10];
    METARECORD *mr = (METARECORD *)&buffer;
    
    mr->rdSize = 5;
    mr->rdFunction = func;
    *(mr->rdParm) = param2;
    *(mr->rdParm + 1) = param1;
    return MF_WriteRecord( dc, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MF_MetaParam4
 */

BOOL MF_MetaParam4(DC *dc, short func, short param1, short param2, 
		   short param3, short param4)
{
    char buffer[14];
    METARECORD *mr = (METARECORD *)&buffer;
    
    mr->rdSize = 7;
    mr->rdFunction = func;
    *(mr->rdParm) = param4;
    *(mr->rdParm + 1) = param3;
    *(mr->rdParm + 2) = param2;
    *(mr->rdParm + 3) = param1;
    return MF_WriteRecord( dc, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MF_MetaParam6
 */

BOOL MF_MetaParam6(DC *dc, short func, short param1, short param2, 
		   short param3, short param4, short param5, short param6)
{
    char buffer[18];
    METARECORD *mr = (METARECORD *)&buffer;
    
    mr->rdSize = 9;
    mr->rdFunction = func;
    *(mr->rdParm) = param6;
    *(mr->rdParm + 1) = param5;
    *(mr->rdParm + 2) = param4;
    *(mr->rdParm + 3) = param3;
    *(mr->rdParm + 4) = param2;
    *(mr->rdParm + 5) = param1;
    return MF_WriteRecord( dc, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MF_MetaParam8
 */
BOOL MF_MetaParam8(DC *dc, short func, short param1, short param2, 
		   short param3, short param4, short param5,
		   short param6, short param7, short param8)
{
    char buffer[22];
    METARECORD *mr = (METARECORD *)&buffer;
    
    mr->rdSize = 11;
    mr->rdFunction = func;
    *(mr->rdParm) = param8;
    *(mr->rdParm + 1) = param7;
    *(mr->rdParm + 2) = param6;
    *(mr->rdParm + 3) = param5;
    *(mr->rdParm + 4) = param4;
    *(mr->rdParm + 5) = param3;
    *(mr->rdParm + 6) = param2;
    *(mr->rdParm + 7) = param1;
    return MF_WriteRecord( dc, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MF_CreateBrushIndirect
 */

BOOL MF_CreateBrushIndirect(DC *dc, HBRUSH16 hBrush, LOGBRUSH16 *logbrush)
{
    int index;
    char buffer[sizeof(METARECORD) - 2 + sizeof(*logbrush)];
    METARECORD *mr = (METARECORD *)&buffer;

    mr->rdSize = (sizeof(METARECORD) + sizeof(*logbrush) - 2) / 2;
    mr->rdFunction = META_CREATEBRUSHINDIRECT;
    memcpy(&(mr->rdParm), logbrush, sizeof(*logbrush));
    if (!(MF_WriteRecord( dc, mr, mr->rdSize * 2))) return FALSE;

    mr->rdSize = sizeof(METARECORD) / 2;
    mr->rdFunction = META_SELECTOBJECT;

    if ((index = MF_AddHandleDC( dc )) == -1) return FALSE;
    *(mr->rdParm) = index;
    return MF_WriteRecord( dc, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MF_CreatePatternBrush
 */

BOOL MF_CreatePatternBrush(DC *dc, HBRUSH16 hBrush, LOGBRUSH16 *logbrush)
{
    DWORD len, bmSize, biSize;
    HGLOBAL16 hmr;
    METARECORD *mr;
    BITMAPOBJ *bmp;
    BITMAPINFO *info;
    BITMAPINFOHEADER *infohdr;
    int index;
    char buffer[sizeof(METARECORD)];

    switch (logbrush->lbStyle)
    {
    case BS_PATTERN:
	bmp = (BITMAPOBJ *)GDI_GetObjPtr((HGDIOBJ16)logbrush->lbHatch, BITMAP_MAGIC);
	if (!bmp) return FALSE;
	len = sizeof(METARECORD) + sizeof(BITMAPINFOHEADER) + 
	      (bmp->bitmap.bmHeight * bmp->bitmap.bmWidthBytes) + 6;
	if (!(hmr = GlobalAlloc16(GMEM_MOVEABLE, len)))
	  {
	    GDI_HEAP_UNLOCK((HGDIOBJ16)logbrush->lbHatch);
	    return FALSE;
	  }
	mr = (METARECORD *)GlobalLock16(hmr);
	memset(mr, 0, len);
	mr->rdFunction = META_DIBCREATEPATTERNBRUSH;
	mr->rdSize = len / 2;
	*(mr->rdParm) = logbrush->lbStyle;
	*(mr->rdParm + 1) = DIB_RGB_COLORS;
	infohdr = (BITMAPINFOHEADER *)(mr->rdParm + 2);
	infohdr->biSize = sizeof(BITMAPINFOHEADER);
	infohdr->biWidth = bmp->bitmap.bmWidth;
	infohdr->biHeight = bmp->bitmap.bmHeight;
	infohdr->biPlanes = bmp->bitmap.bmPlanes;
	infohdr->biBitCount = bmp->bitmap.bmBitsPixel;
	memcpy(mr->rdParm + (sizeof(BITMAPINFOHEADER) / 2) + 4,
	       PTR_SEG_TO_LIN(bmp->bitmap.bmBits),
	       bmp->bitmap.bmHeight * bmp->bitmap.bmWidthBytes);
	GDI_HEAP_UNLOCK(logbrush->lbHatch);
	break;

    case BS_DIBPATTERN:
	info = (BITMAPINFO *)GlobalLock16((HGLOBAL16)logbrush->lbHatch);
	if (info->bmiHeader.biCompression)
            bmSize = info->bmiHeader.biSizeImage;
        else
	    bmSize = (info->bmiHeader.biWidth * info->bmiHeader.biBitCount 
		    + 31) / 32 * 8 * info->bmiHeader.biHeight;
	biSize = DIB_BitmapInfoSize(info, LOWORD(logbrush->lbColor)); 
	len = sizeof(METARECORD) + biSize + bmSize + 2;
	if (!(hmr = GlobalAlloc16(GMEM_MOVEABLE, len)))
	    return FALSE;
	mr = (METARECORD *)GlobalLock16(hmr);
	memset(mr, 0, len);
	mr->rdFunction = META_DIBCREATEPATTERNBRUSH;
	mr->rdSize = len / 2;
	*(mr->rdParm) = logbrush->lbStyle;
	*(mr->rdParm + 1) = LOWORD(logbrush->lbColor);
	memcpy(mr->rdParm + 2, info, biSize + bmSize);
	break;
    default:
        return FALSE;
    }
    if (!(MF_WriteRecord(dc, mr, len)))
    {
	GlobalFree16(hmr);
	return FALSE;
    }

    GlobalFree16(hmr);
    
    mr = (METARECORD *)&buffer;
    mr->rdSize = sizeof(METARECORD) / 2;
    mr->rdFunction = META_SELECTOBJECT;

    if ((index = MF_AddHandleDC( dc )) == -1) return FALSE;
    *(mr->rdParm) = index;
    return MF_WriteRecord( dc, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MF_CreatePenIndirect
 */

BOOL MF_CreatePenIndirect(DC *dc, HPEN16 hPen, LOGPEN16 *logpen)
{
    int index;
    char buffer[sizeof(METARECORD) - 2 + sizeof(*logpen)];
    METARECORD *mr = (METARECORD *)&buffer;

    mr->rdSize = (sizeof(METARECORD) + sizeof(*logpen) - 2) / 2;
    mr->rdFunction = META_CREATEPENINDIRECT;
    memcpy(&(mr->rdParm), logpen, sizeof(*logpen));
    if (!(MF_WriteRecord( dc, mr, mr->rdSize * 2))) return FALSE;

    mr->rdSize = sizeof(METARECORD) / 2;
    mr->rdFunction = META_SELECTOBJECT;

    if ((index = MF_AddHandleDC( dc )) == -1) return FALSE;
    *(mr->rdParm) = index;
    return MF_WriteRecord( dc, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MF_CreateFontIndirect
 */

BOOL MF_CreateFontIndirect(DC *dc, HFONT16 hFont, LOGFONT16 *logfont)
{
    int index;
    char buffer[sizeof(METARECORD) - 2 + sizeof(LOGFONT16)];
    METARECORD *mr = (METARECORD *)&buffer;

    mr->rdSize = (sizeof(METARECORD) + sizeof(LOGFONT16) - 2) / 2;
    mr->rdFunction = META_CREATEFONTINDIRECT;
    memcpy(&(mr->rdParm), logfont, sizeof(LOGFONT16));
    if (!(MF_WriteRecord( dc, mr, mr->rdSize * 2))) return FALSE;

    mr->rdSize = sizeof(METARECORD) / 2;
    mr->rdFunction = META_SELECTOBJECT;

    if ((index = MF_AddHandleDC( dc )) == -1) return FALSE;
    *(mr->rdParm) = index;
    return MF_WriteRecord( dc, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MF_TextOut
 */
BOOL MF_TextOut(DC *dc, short x, short y, LPCSTR str, short count)
{
    BOOL ret;
    DWORD len;
    HGLOBAL16 hmr;
    METARECORD *mr;

    len = sizeof(METARECORD) + (((count + 1) >> 1) * 2) + 4;
    if (!(hmr = GlobalAlloc16(GMEM_MOVEABLE, len)))
	return FALSE;
    mr = (METARECORD *)GlobalLock16(hmr);
    memset(mr, 0, len);

    mr->rdSize = len / 2;
    mr->rdFunction = META_TEXTOUT;
    *(mr->rdParm) = count;
    memcpy(mr->rdParm + 1, str, count);
    *(mr->rdParm + ((count + 1) >> 1) + 1) = y;
    *(mr->rdParm + ((count + 1) >> 1) + 2) = x;
    ret = MF_WriteRecord( dc, mr, mr->rdSize * 2);
    GlobalFree16(hmr);
    return ret;
}

/******************************************************************
 *         MF_ExtTextOut
 */
BOOL MF_ExtTextOut(DC*dc, short x, short y, UINT16 flags, const RECT16 *rect,
                     LPCSTR str, short count, const INT16 *lpDx)
{
    BOOL ret;
    DWORD len;
    HGLOBAL16 hmr;
    METARECORD *mr;
    
    if((!flags && rect) || (flags && !rect))
	WARN(metafile, "Inconsistent flags and rect\n");
    len = sizeof(METARECORD) + (((count + 1) >> 1) * 2) + 2 * sizeof(short)
	    + sizeof(UINT16);
    if(rect)
        len += sizeof(RECT16);
    if (lpDx)
     len+=count*sizeof(INT16);
    if (!(hmr = GlobalAlloc16(GMEM_MOVEABLE, len)))
	return FALSE;
    mr = (METARECORD *)GlobalLock16(hmr);
    memset(mr, 0, len);

    mr->rdSize = len / 2;
    mr->rdFunction = META_EXTTEXTOUT;
    *(mr->rdParm) = y;
    *(mr->rdParm + 1) = x;
    *(mr->rdParm + 2) = count;
    *(mr->rdParm + 3) = flags;
    if (rect) memcpy(mr->rdParm + 4, rect, sizeof(RECT16));
    memcpy(mr->rdParm + (rect ? 8 : 4), str, count);
    if (lpDx)
     memcpy(mr->rdParm + (rect ? 8 : 4) + ((count + 1) >> 1),lpDx,
      count*sizeof(INT16));
    ret = MF_WriteRecord( dc, mr, mr->rdSize * 2);
    GlobalFree16(hmr);
    return ret;
}

/******************************************************************
 *         MF_MetaPoly - implements Polygon and Polyline
 */
BOOL MF_MetaPoly(DC *dc, short func, LPPOINT16 pt, short count)
{
    BOOL ret;
    DWORD len;
    HGLOBAL16 hmr;
    METARECORD *mr;

    len = sizeof(METARECORD) + (count * 4); 
    if (!(hmr = GlobalAlloc16(GMEM_MOVEABLE, len)))
	return FALSE;
    mr = (METARECORD *)GlobalLock16(hmr);
    memset(mr, 0, len);

    mr->rdSize = len / 2;
    mr->rdFunction = func;
    *(mr->rdParm) = count;
    memcpy(mr->rdParm + 1, pt, count * 4);
    ret = MF_WriteRecord( dc, mr, mr->rdSize * 2);
    GlobalFree16(hmr);
    return ret;
}


/******************************************************************
 *         MF_BitBlt
 */
BOOL MF_BitBlt(DC *dcDest, short xDest, short yDest, short width,
                 short height, DC *dcSrc, short xSrc, short ySrc, DWORD rop)
{
    BOOL ret;
    DWORD len;
    HGLOBAL16 hmr;
    METARECORD *mr;
    BITMAP16  BM;

    GetObject16(dcSrc->w.hBitmap, sizeof(BITMAP16), &BM);
    len = sizeof(METARECORD) + 12 * sizeof(INT16) + BM.bmWidthBytes * BM.bmHeight;
    if (!(hmr = GlobalAlloc16(GMEM_MOVEABLE, len)))
	return FALSE;
    mr = (METARECORD *)GlobalLock16(hmr);
    mr->rdFunction = META_BITBLT;
    *(mr->rdParm + 7) = BM.bmWidth;
    *(mr->rdParm + 8) = BM.bmHeight;
    *(mr->rdParm + 9) = BM.bmWidthBytes;
    *(mr->rdParm +10) = BM.bmPlanes;
    *(mr->rdParm +11) = BM.bmBitsPixel;
    TRACE(metafile,"MF_StretchBlt->len = %ld  rop=%lx  \n",len,rop);
    if (GetBitmapBits(dcSrc->w.hBitmap,BM.bmWidthBytes * BM.bmHeight,
                        mr->rdParm +12))
    {
      mr->rdSize = len / sizeof(INT16);
      *(mr->rdParm) = HIWORD(rop);
      *(mr->rdParm + 1) = ySrc;
      *(mr->rdParm + 2) = xSrc;
      *(mr->rdParm + 3) = height;
      *(mr->rdParm + 4) = width;
      *(mr->rdParm + 5) = yDest;
      *(mr->rdParm + 6) = xDest;
      ret = MF_WriteRecord( dcDest, mr, mr->rdSize * 2);
    }  
    else
        ret = FALSE;
    GlobalFree16(hmr);
    return ret;
}


/**********************************************************************
 *         MF_StretchBlt         
 * this function contains TWO ways for procesing StretchBlt in metafiles,
 * decide between rdFunction values  META_STRETCHBLT or META_DIBSTRETCHBLT
 * via #define STRETCH_VIA_DIB
 */
#define STRETCH_VIA_DIB
#undef  STRETCH_VIA_DIB
BOOL MF_StretchBlt(DC *dcDest, short xDest, short yDest, short widthDest,
                     short heightDest, DC *dcSrc, short xSrc, short ySrc, 
                     short widthSrc, short heightSrc, DWORD rop)
{
    BOOL ret;
    DWORD len;
    HGLOBAL16 hmr;
    METARECORD *mr;
    BITMAP16  BM;
#ifdef STRETCH_VIA_DIB    
    LPBITMAPINFOHEADER lpBMI;
    WORD nBPP;
#endif  
    GetObject16(dcSrc->w.hBitmap, sizeof(BITMAP16), &BM);
#ifdef STRETCH_VIA_DIB
    nBPP = BM.bmPlanes * BM.bmBitsPixel;
    len = sizeof(METARECORD) + 10 * sizeof(INT16) 
            + sizeof(BITMAPINFOHEADER) + (nBPP != 24 ? 1 << nBPP: 0) * sizeof(RGBQUAD) 
              + ((BM.bmWidth * nBPP + 31) / 32) * 4 * BM.bmHeight;
    if (!(hmr = GlobalAlloc16(GMEM_MOVEABLE, len)))
	return FALSE;
    mr = (METARECORD *)GlobalLock16(hmr);
    mr->rdFunction = META_DIBSTRETCHBLT;
    lpBMI=(LPBITMAPINFOHEADER)(mr->rdParm+10);
    lpBMI->biSize      = sizeof(BITMAPINFOHEADER);
    lpBMI->biWidth     = BM.bmWidth;
    lpBMI->biHeight    = BM.bmHeight;
    lpBMI->biPlanes    = 1;
    lpBMI->biBitCount  = nBPP;                              /* 1,4,8 or 24 */
    lpBMI->biClrUsed   = nBPP != 24 ? 1 << nBPP : 0;
    lpBMI->biSizeImage = ((lpBMI->biWidth * nBPP + 31) / 32) * 4 * lpBMI->biHeight;
    lpBMI->biCompression = BI_RGB;
    lpBMI->biXPelsPerMeter = MulDiv(GetDeviceCaps(dcSrc->hSelf,LOGPIXELSX),3937,100);
    lpBMI->biYPelsPerMeter = MulDiv(GetDeviceCaps(dcSrc->hSelf,LOGPIXELSY),3937,100);
    lpBMI->biClrImportant  = 0;                          /* 1 meter  = 39.37 inch */

    TRACE(metafile,"MF_StretchBltViaDIB->len = %ld  rop=%lx  PixYPM=%ld Caps=%d\n",
               len,rop,lpBMI->biYPelsPerMeter,GetDeviceCaps(hdcSrc,LOGPIXELSY));
    if (GetDIBits(hdcSrc,dcSrc->w.hBitmap,0,(UINT)lpBMI->biHeight,
                  (LPSTR)lpBMI + DIB_BitmapInfoSize( (BITMAPINFO *)lpBMI,
                                                     DIB_RGB_COLORS ),
                  (LPBITMAPINFO)lpBMI, DIB_RGB_COLORS))
#else
    len = sizeof(METARECORD) + 15 * sizeof(INT16) + BM.bmWidthBytes * BM.bmHeight;
    if (!(hmr = GlobalAlloc16(GMEM_MOVEABLE, len)))
	return FALSE;
    mr = (METARECORD *)GlobalLock16(hmr);
    mr->rdFunction = META_STRETCHBLT;
    *(mr->rdParm +10) = BM.bmWidth;
    *(mr->rdParm +11) = BM.bmHeight;
    *(mr->rdParm +12) = BM.bmWidthBytes;
    *(mr->rdParm +13) = BM.bmPlanes;
    *(mr->rdParm +14) = BM.bmBitsPixel;
    TRACE(metafile,"MF_StretchBlt->len = %ld  rop=%lx  \n",len,rop);
    if (GetBitmapBits( dcSrc->w.hBitmap, BM.bmWidthBytes * BM.bmHeight,
                         mr->rdParm +15))
#endif    
    {
      mr->rdSize = len / sizeof(INT16);
      *(mr->rdParm) = LOWORD(rop);
      *(mr->rdParm + 1) = HIWORD(rop);
      *(mr->rdParm + 2) = heightSrc;
      *(mr->rdParm + 3) = widthSrc;
      *(mr->rdParm + 4) = ySrc;
      *(mr->rdParm + 5) = xSrc;
      *(mr->rdParm + 6) = heightDest;
      *(mr->rdParm + 7) = widthDest;
      *(mr->rdParm + 8) = yDest;
      *(mr->rdParm + 9) = xDest;
      ret = MF_WriteRecord( dcDest, mr, mr->rdSize * 2);
    }  
    else
        ret = FALSE;
    GlobalFree16(hmr);
    return ret;
}


/******************************************************************
 *         MF_CreateRegion
 */
INT16 MF_CreateRegion(DC *dc, HRGN hrgn)
{
    DWORD len;
    METARECORD *mr;
    RGNDATA *rgndata;
    RECT *pCurRect, *pEndRect;
    WORD Bands = 0, MaxBands = 0;
    WORD *Param, *StartBand;
    BOOL ret;

    len = GetRegionData( hrgn, 0, NULL );
    if( !(rgndata = HeapAlloc( SystemHeap, 0, len )) ) {
        WARN(metafile, "MF_CreateRegion: can't alloc rgndata buffer\n");
	return -1;
    }
    GetRegionData( hrgn, len, rgndata );

    /* Overestimate of length:
     * Assume every rect is a separate band -> 6 WORDs per rect
     */
    len = sizeof(METARECORD) + 20 + (rgndata->rdh.nCount * 12);
    if( !(mr = HeapAlloc( SystemHeap, 0, len )) ) {
        WARN(metafile, "MF_CreateRegion: can't alloc METARECORD buffer\n");
	HeapFree( SystemHeap, 0, rgndata );
	return -1;
    }

    memset(mr, 0, len);
        
    Param = mr->rdParm + 11;
    StartBand = NULL;

    pEndRect = (RECT *)rgndata->Buffer + rgndata->rdh.nCount;
    for(pCurRect = (RECT *)rgndata->Buffer; pCurRect < pEndRect; pCurRect++)
    {
        if( StartBand && pCurRect->top == *(StartBand + 1) )
        {
	    *Param++ = pCurRect->left;
	    *Param++ = pCurRect->right;
	}
	else
	{
	    if(StartBand)
	    {
	        *StartBand = Param - StartBand - 3;
		*Param++ = *StartBand;
		if(*StartBand > MaxBands)
		    MaxBands = *StartBand;
		Bands++;
	    }
	    StartBand = Param++;
	    *Param++ = pCurRect->top;
	    *Param++ = pCurRect->bottom;
	    *Param++ = pCurRect->left;
	    *Param++ = pCurRect->right;
	}
    }
    len = Param - (WORD *)mr;
    
    mr->rdParm[0] = 0;
    mr->rdParm[1] = 6;
    mr->rdParm[2] = 0x1234;
    mr->rdParm[3] = 0;
    mr->rdParm[4] = len * 2;
    mr->rdParm[5] = Bands;
    mr->rdParm[6] = MaxBands;
    mr->rdParm[7] = rgndata->rdh.rcBound.left;
    mr->rdParm[8] = rgndata->rdh.rcBound.top;
    mr->rdParm[9] = rgndata->rdh.rcBound.right;
    mr->rdParm[10] = rgndata->rdh.rcBound.bottom;
    mr->rdFunction = META_CREATEREGION;
    mr->rdSize = len / 2;
    ret = MF_WriteRecord( dc, mr, mr->rdSize * 2 );	
    HeapFree( SystemHeap, 0, mr );
    HeapFree( SystemHeap, 0, rgndata );
    if(!ret) 
    {
        WARN(metafile, "MF_CreateRegion: MF_WriteRecord failed\n");
	return -1;
    }
    return MF_AddHandleDC( dc );
}
