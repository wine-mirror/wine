/*
 * Metafile functions
 *
 * Copyright  David W. Metcalfe, 1994
 *            Niels de Carpentier, Albrecht Kleine, Huw Davies 1996
 *
 */

#include <string.h>
#include <fcntl.h>
#include "gdi.h"
#include "bitmap.h"
#include "file.h"
#include "heap.h"
#include "metafile.h"
#include "metafiledrv.h"
#include "stddebug.h"
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
 *         GetMetafile16   (GDI.124)
 */
HMETAFILE16 GetMetaFile16( LPCSTR lpFilename )
{
    return GetMetaFile32A( lpFilename );
}


/******************************************************************
 *         GetMetafile32A   (GDI32.197)
 */
HMETAFILE32 GetMetaFile32A( LPCSTR lpFilename )
{
  HMETAFILE16 hmf;
  METAHEADER *mh;
  HFILE32 hFile;
  DWORD size;
  
  dprintf_metafile(stddeb,"GetMetaFile: %s\n", lpFilename);

  if (!lpFilename)
    return 0;

  hmf = GlobalAlloc16(GMEM_MOVEABLE, MFHEADERSIZE);
  mh = (METAHEADER *)GlobalLock16(hmf);
  
  if (!mh)
  {
    GlobalFree16(hmf);
    return 0;
  }
  
  if ((hFile = _lopen32(lpFilename, OF_READ)) == HFILE_ERROR32)
  {
    GlobalFree16(hmf);
    return 0;
  }
  
  if (_lread32(hFile, (char *)mh, MFHEADERSIZE) == HFILE_ERROR32)
  {
    _lclose32( hFile );
    GlobalFree16(hmf);
    return 0;
  }
  
  size = mh->mtSize * 2;         /* alloc memory for whole metafile */
  GlobalUnlock16(hmf);
  hmf = GlobalReAlloc16(hmf,size,GMEM_MOVEABLE);
  mh = (METAHEADER *)GlobalLock16(hmf);
  
  if (!mh)
  {
    _lclose32( hFile );
    GlobalFree16(hmf);
    return 0;
  }
  
  if (_lread32(hFile, (char*)mh + mh->mtHeaderSize * 2, 
	        size - mh->mtHeaderSize * 2) == HFILE_ERROR32)
  {
    _lclose32( hFile );
    GlobalFree16(hmf);
    return 0;
  }
  
  _lclose32(hFile);

  if (mh->mtType != 1)
  {
    GlobalFree16(hmf);
    return 0;
  }
  
  GlobalUnlock16(hmf);
  return hmf;

}


/******************************************************************
 *         GetMetafile32W   (GDI32.199)
 */
HMETAFILE32 GetMetaFile32W( LPCWSTR lpFilename )
{
    LPSTR p = HEAP_strdupWtoA( GetProcessHeap(), 0, lpFilename );
    HMETAFILE32 ret = GetMetaFile32A( p );
    HeapFree( GetProcessHeap(), 0, p );
    return ret;
}


/******************************************************************
 *         CopyMetaFile16   (GDI.151)
 */

HMETAFILE16 CopyMetaFile16( HMETAFILE16 hSrcMetaFile, LPCSTR lpFilename )
{
    return CopyMetaFile32A( hSrcMetaFile, lpFilename );
}


/******************************************************************
 *         CopyMetaFile32A   (GDI32.23)
 */
HMETAFILE32 CopyMetaFile32A( HMETAFILE32 hSrcMetaFile, LPCSTR lpFilename )
{
    HMETAFILE16 handle = 0;
    METAHEADER *mh;
    METAHEADER *mh2;
    HFILE32 hFile;
    
    dprintf_metafile(stddeb,"CopyMetaFile: %s\n", lpFilename);
    
    mh = (METAHEADER *)GlobalLock16(hSrcMetaFile);
    
    if (!mh)
      return 0;
    
    if (lpFilename)          /* disk based metafile */
        {
        int i,j;
	hFile = _lcreat32(lpFilename, 0);
	j=mh->mtType;
	mh->mtType=1;        /* disk file version stores 1 here */
	i=_lwrite32(hFile, (char *)mh, mh->mtSize * 2) ;
	mh->mtType=j;        /* restore old value  [0 or 1] */	
        _lclose32(hFile);
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
    
    return handle;
}


/******************************************************************
 *         CopyMetaFile32W   (GDI32.24)
 */
HMETAFILE32 CopyMetaFile32W( HMETAFILE32 hSrcMetaFile, LPCWSTR lpFilename )
{
    LPSTR p = HEAP_strdupWtoA( GetProcessHeap(), 0, lpFilename );
    HMETAFILE32 ret = CopyMetaFile32A( hSrcMetaFile, p );
    HeapFree( GetProcessHeap(), 0, p );
    return ret;
}


/******************************************************************
 *         IsValidMetaFile   (GDI.410)
 *         (This is not exactly what windows does, see "Undoc Win")
 */

BOOL16 IsValidMetaFile(HMETAFILE16 hmf)
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
    dprintf_metafile(stddeb,"IsValidMetaFile %x => %d\n",hmf,resu);
    return resu;         
}


/******************************************************************
 *         PlayMetafile16   (GDI.123)
 */
BOOL16 PlayMetaFile16( HDC16 hdc, HMETAFILE16 hmf )
{
    return PlayMetaFile32( hdc, hmf );
}


/******************************************************************
 *         PlayMetafile32   (GDI32.265)
 */
BOOL32 PlayMetaFile32( HDC32 hdc, HMETAFILE32 hmf )
{
    METAHEADER *mh = (METAHEADER *)GlobalLock16(hmf);
    METARECORD *mr;
    HANDLETABLE16 *ht;
    HGLOBAL16 hHT;
    int offset = 0;
    WORD i;
    HPEN32 hPen;
    HBRUSH32 hBrush;
    HFONT32 hFont;
    DC *dc;

    dprintf_metafile(stddeb,"PlayMetaFile(%04x %04x)\n",hdc,hmf);

    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    hPen = dc->w.hPen;
    hBrush = dc->w.hBrush;
    hFont = dc->w.hFont;

    /* create the handle table */
    hHT = GlobalAlloc16(GMEM_MOVEABLE|GMEM_ZEROINIT,
		      sizeof(HANDLETABLE16) * mh->mtNoObjects);
    ht = (HANDLETABLE16 *)GlobalLock16(hHT);

    /* loop through metafile playing records */
    offset = mh->mtHeaderSize * 2;
    while (offset < mh->mtSize * 2)
    {
	mr = (METARECORD *)((char *)mh + offset);
	dprintf_metafile(stddeb,"offset = %04x size = %08lx function = %04x\n",
			 offset,mr->rdSize,mr->rdFunction);
	offset += mr->rdSize * 2;
	PlayMetaFileRecord16( hdc, ht, mr, mh->mtNoObjects );
    }

    SelectObject32(hdc, hBrush);
    SelectObject32(hdc, hPen);
    SelectObject32(hdc, hFont);

    /* free objects in handle table */
    for(i = 0; i < mh->mtNoObjects; i++)
      if(*(ht->objectHandle + i) != 0)
        DeleteObject32(*(ht->objectHandle + i));
    
    /* free handle table */
    GlobalFree16(hHT);

    return TRUE;
}


/******************************************************************
 *            EnumMetaFile16   (GDI.175)
 *                                    Niels de carpentier, april 1996
 */
BOOL16 EnumMetaFile16( HDC16 hdc, HMETAFILE16 hmf, MFENUMPROC16 lpEnumFunc,
                       LPARAM lpData )
{
    METAHEADER *mh = (METAHEADER *)GlobalLock16(hmf);
    METARECORD *mr;
    HGLOBAL16 hHT;
    SEGPTR ht, spRecord;
    int offset = 0;
  
    dprintf_metafile(stddeb,"EnumMetaFile(%04x, %04x, %08lx, %08lx)\n",
		     hdc, hmf, (DWORD)lpEnumFunc, lpData);
   
    /* create the handle table */
    
    hHT = GlobalAlloc16(GMEM_MOVEABLE | GMEM_ZEROINIT,
		     sizeof(HANDLETABLE16) * mh->mtNoObjects);
    ht = WIN16_GlobalLock16(hHT);
   
    offset = mh->mtHeaderSize * 2;
    
    /* loop through metafile records */
    
    spRecord = WIN16_GlobalLock16(hmf);
    while (offset < (mh->mtSize * 2))
    {
	mr = (METARECORD *)((char *)mh + offset);
        if (!lpEnumFunc( hdc, (HANDLETABLE16 *)ht,
                         (METARECORD *)((UINT32)spRecord + offset),
                         mh->mtNoObjects, (LONG)lpData))
	    break;

	offset += (mr->rdSize * 2);
    }

    /* free handle table */
    GlobalFree16(hHT);

    return TRUE;
}


/******************************************************************
 *             PlayMetaFileRecord16   (GDI.176)
 */
void PlayMetaFileRecord16( HDC16 hdc, HANDLETABLE16 *ht, METARECORD *mr,
                           UINT16 nHandles )
{
    short s1;
    HANDLE16 hndl;
    char *ptr;
    BITMAPINFOHEADER *infohdr;

    dprintf_metafile(stddeb,"PlayMetaFileRecord(%04x %08lx %08lx %04x)\n",
		     hdc,(LONG)ht, (LONG)mr, nHandles);
    
    switch (mr->rdFunction)
    {
    case META_EOF:
      break;

    case META_DELETEOBJECT:
      DeleteObject32(*(ht->objectHandle + *(mr->rdParam)));
      *(ht->objectHandle + *(mr->rdParam)) = 0;
      break;

    case META_SETBKCOLOR:
	SetBkColor16(hdc, MAKELONG(*(mr->rdParam), *(mr->rdParam + 1)));
	break;

    case META_SETBKMODE:
	SetBkMode16(hdc, *(mr->rdParam));
	break;

    case META_SETMAPMODE:
	SetMapMode16(hdc, *(mr->rdParam));
	break;

    case META_SETROP2:
	SetROP216(hdc, *(mr->rdParam));
	break;

    case META_SETRELABS:
	SetRelAbs16(hdc, *(mr->rdParam));
	break;

    case META_SETPOLYFILLMODE:
	SetPolyFillMode16(hdc, *(mr->rdParam));
	break;

    case META_SETSTRETCHBLTMODE:
	SetStretchBltMode16(hdc, *(mr->rdParam));
	break;
    case META_SETTEXTCOLOR:
	SetTextColor16(hdc, MAKELONG(*(mr->rdParam), *(mr->rdParam + 1)));
	break;

    case META_SETWINDOWORG:
	SetWindowOrg(hdc, *(mr->rdParam + 1), *(mr->rdParam));
	break;

    case META_SETWINDOWEXT:
	SetWindowExt(hdc, *(mr->rdParam + 1), *(mr->rdParam));
	break;

    case META_SETVIEWPORTORG:
	SetViewportOrg(hdc, *(mr->rdParam + 1), *(mr->rdParam));
	break;

    case META_SETVIEWPORTEXT:
	SetViewportExt(hdc, *(mr->rdParam + 1), *(mr->rdParam));
	break;

    case META_OFFSETWINDOWORG:
	OffsetWindowOrg(hdc, *(mr->rdParam + 1), *(mr->rdParam));
	break;

    case META_SCALEWINDOWEXT:
	ScaleWindowExt(hdc, *(mr->rdParam + 3), *(mr->rdParam + 2),
		       *(mr->rdParam + 1), *(mr->rdParam));
	break;

    case META_OFFSETVIEWPORTORG:
	OffsetViewportOrg(hdc, *(mr->rdParam + 1), *(mr->rdParam));
	break;

    case META_SCALEVIEWPORTEXT:
	ScaleViewportExt(hdc, *(mr->rdParam + 3), *(mr->rdParam + 2),
			 *(mr->rdParam + 1), *(mr->rdParam));
	break;

    case META_LINETO:
	LineTo32(hdc, (INT16)*(mr->rdParam + 1), (INT16)*(mr->rdParam));
	break;

    case META_MOVETO:
	MoveTo(hdc, *(mr->rdParam + 1), *(mr->rdParam));
	break;

    case META_EXCLUDECLIPRECT:
	ExcludeClipRect16( hdc, *(mr->rdParam + 3), *(mr->rdParam + 2),
                           *(mr->rdParam + 1), *(mr->rdParam) );
	break;

    case META_INTERSECTCLIPRECT:
	IntersectClipRect16( hdc, *(mr->rdParam + 3), *(mr->rdParam + 2),
                             *(mr->rdParam + 1), *(mr->rdParam) );
	break;

    case META_ARC:
	Arc32(hdc, (INT16)*(mr->rdParam + 7), (INT16)*(mr->rdParam + 6),
	      (INT16)*(mr->rdParam + 5), (INT16)*(mr->rdParam + 4),
	      (INT16)*(mr->rdParam + 3), (INT16)*(mr->rdParam + 2),
             (INT16)*(mr->rdParam + 1), (INT16)*(mr->rdParam));
	break;

    case META_ELLIPSE:
	Ellipse32(hdc, (INT16)*(mr->rdParam + 3), (INT16)*(mr->rdParam + 2),
                  (INT16)*(mr->rdParam + 1), (INT16)*(mr->rdParam));
	break;

    case META_FLOODFILL:
	FloodFill32(hdc, (INT16)*(mr->rdParam + 3), (INT16)*(mr->rdParam + 2),
                    MAKELONG(*(mr->rdParam), *(mr->rdParam + 1)));
	break;

    case META_PIE:
	Pie32(hdc, (INT16)*(mr->rdParam + 7), (INT16)*(mr->rdParam + 6),
	      (INT16)*(mr->rdParam + 5), (INT16)*(mr->rdParam + 4),
	      (INT16)*(mr->rdParam + 3), (INT16)*(mr->rdParam + 2),
             (INT16)*(mr->rdParam + 1), (INT16)*(mr->rdParam));
	break;

    case META_RECTANGLE:
	Rectangle32(hdc, (INT16)*(mr->rdParam + 3), (INT16)*(mr->rdParam + 2),
                    (INT16)*(mr->rdParam + 1), (INT16)*(mr->rdParam));
	break;

    case META_ROUNDRECT:
	RoundRect32(hdc, (INT16)*(mr->rdParam + 5), (INT16)*(mr->rdParam + 4),
                    (INT16)*(mr->rdParam + 3), (INT16)*(mr->rdParam + 2),
                    (INT16)*(mr->rdParam + 1), (INT16)*(mr->rdParam));
	break;

    case META_PATBLT:
	PatBlt16(hdc, *(mr->rdParam + 5), *(mr->rdParam + 4),
                 *(mr->rdParam + 3), *(mr->rdParam + 2),
                 MAKELONG(*(mr->rdParam), *(mr->rdParam + 1)));
	break;

    case META_SAVEDC:
	SaveDC32(hdc);
	break;

    case META_SETPIXEL:
	SetPixel32(hdc, (INT16)*(mr->rdParam + 3), (INT16)*(mr->rdParam + 2),
                   MAKELONG(*(mr->rdParam), *(mr->rdParam + 1)));
	break;

    case META_OFFSETCLIPRGN:
	OffsetClipRgn16( hdc, *(mr->rdParam + 1), *(mr->rdParam) );
	break;

    case META_TEXTOUT:
	s1 = *(mr->rdParam);
	TextOut16(hdc, *(mr->rdParam + ((s1 + 1) >> 1) + 2),
                  *(mr->rdParam + ((s1 + 1) >> 1) + 1), 
                  (char *)(mr->rdParam + 1), s1);
	break;

    case META_POLYGON:
	Polygon16(hdc, (LPPOINT16)(mr->rdParam + 1), *(mr->rdParam));
	break;

    case META_POLYPOLYGON:
      PolyPolygon16(hdc, (LPPOINT16)(mr->rdParam + *(mr->rdParam) + 1),
                    (LPINT16)(mr->rdParam + 1), *(mr->rdParam)); 
      break;

    case META_POLYLINE:
	Polyline16(hdc, (LPPOINT16)(mr->rdParam + 1), *(mr->rdParam));
	break;

    case META_RESTOREDC:
	RestoreDC32(hdc, (INT16)*(mr->rdParam));
	break;

    case META_SELECTOBJECT:
	SelectObject32(hdc, *(ht->objectHandle + *(mr->rdParam)));
	break;

    case META_CHORD:
	Chord32(hdc, (INT16)*(mr->rdParam + 7), (INT16)*(mr->rdParam + 6),
		(INT16)*(mr->rdParam+5), (INT16)*(mr->rdParam + 4),
		(INT16)*(mr->rdParam + 3), (INT16)*(mr->rdParam + 2),
               (INT16)*(mr->rdParam + 1), (INT16)*(mr->rdParam));
	break;

    case META_CREATEPATTERNBRUSH:
	switch (*(mr->rdParam))
	{
	case BS_PATTERN:
	    infohdr = (BITMAPINFOHEADER *)(mr->rdParam + 2);
	    MF_AddHandle(ht, nHandles,
			 CreatePatternBrush32(CreateBitmap32(infohdr->biWidth, 
				      infohdr->biHeight, 
				      infohdr->biPlanes, 
				      infohdr->biBitCount,
				      (LPSTR)(mr->rdParam + 
				      (sizeof(BITMAPINFOHEADER) / 2) + 4))));
	    break;

	case BS_DIBPATTERN:
	    s1 = mr->rdSize * 2 - sizeof(METARECORD) - 2;
	    hndl = GlobalAlloc16(GMEM_MOVEABLE, s1);
	    ptr = GlobalLock16(hndl);
	    memcpy(ptr, mr->rdParam + 2, s1);
	    GlobalUnlock16(hndl);
	    MF_AddHandle(ht, nHandles,
			 CreateDIBPatternBrush32(hndl, *(mr->rdParam + 1)));
	    GlobalFree16(hndl);
	}
	break;
	
    case META_CREATEPENINDIRECT:
	MF_AddHandle(ht, nHandles, 
		     CreatePenIndirect16((LOGPEN16 *)(&(mr->rdParam))));
	break;

    case META_CREATEFONTINDIRECT:
	MF_AddHandle(ht, nHandles, 
		     CreateFontIndirect16((LOGFONT16 *)(&(mr->rdParam))));
	break;

    case META_CREATEBRUSHINDIRECT:
	MF_AddHandle(ht, nHandles, 
		     CreateBrushIndirect16((LOGBRUSH16 *)(&(mr->rdParam))));
	break;

    /* W. Magro: Some new metafile operations.  Not all debugged. */
    case META_CREATEPALETTE:
	MF_AddHandle(ht, nHandles, 
		     CreatePalette16((LPLOGPALETTE)mr->rdParam));
	break;

    case META_SETTEXTALIGN:
       	SetTextAlign16(hdc, *(mr->rdParam));
	break;

    case META_SELECTPALETTE:
	SelectPalette16(hdc, *(ht->objectHandle + *(mr->rdParam+1)),*(mr->rdParam));
	break;

    case META_SETMAPPERFLAGS:
	SetMapperFlags16(hdc, *(mr->rdParam));
	break;

    case META_REALIZEPALETTE:
	RealizePalette16(hdc);
	break;

    case META_ESCAPE:
	dprintf_metafile(stddeb,"PlayMetaFileRecord: META_ESCAPE unimplemented.\n");
        break;

        /* --- Begin of fixed or new metafile operations. July 1996 ----*/
    case META_EXTTEXTOUT:
      {
        LPINT16 dxx;
        DWORD len;

        s1 = mr->rdParam[2];                              /* String length */
        len = sizeof(METARECORD) + (((s1 + 1) >> 1) * 2) + 2 * sizeof(short)
         + sizeof(UINT16) + sizeof(RECT16);
        if (mr->rdSize == len / 2)
          dxx = NULL;                                    /* No array present */
        else if (mr->rdSize == (len + s1 * sizeof(INT16)) / 2)
          dxx = &mr->rdParam[8+(s1+1)/2];                /* start of array */
        else {
          fprintf(stderr,
           "PlayMetaFileRecord ExtTextOut mr->rdSize = %08lx, count = %x\n",
           mr->rdSize, s1);
          dxx = NULL;
        }
        ExtTextOut16( hdc, mr->rdParam[1],              /* X position */
	                   mr->rdParam[0],              /* Y position */
	                   mr->rdParam[3],              /* options */
	                   (LPRECT16) &mr->rdParam[4],  /* rectangle */
                           (char *)(mr->rdParam + 8),   /* string */
                           s1, dxx);                    /* length, dx array */
        if (dxx)                      
          dprintf_metafile(stddeb,"EXTTEXTOUT len: %ld  (%hd %hd)  [%s].\n",
            mr->rdSize,dxx[0],dxx[1],(char*) &(mr->rdParam[8]) );
       }
       break;
    
    case META_STRETCHDIB:
      {
       LPBITMAPINFO info = (LPBITMAPINFO) &(mr->rdParam[11]);
       LPSTR bits = (LPSTR)info + DIB_BitmapInfoSize( info, mr->rdParam[2] );
       StretchDIBits16(hdc,mr->rdParam[10],mr->rdParam[9],mr->rdParam[8],
                       mr->rdParam[7],mr->rdParam[6],mr->rdParam[5],
                       mr->rdParam[4],mr->rdParam[3],bits,info,
                       mr->rdParam[2],MAKELONG(mr->rdParam[0],mr->rdParam[1]));
      }
      break;

    case META_DIBSTRETCHBLT:
      {
       LPBITMAPINFO info = (LPBITMAPINFO) &(mr->rdParam[10]); 
       LPSTR bits = (LPSTR)info + DIB_BitmapInfoSize( info, mr->rdParam[2] );
       StretchDIBits16(hdc,mr->rdParam[9],mr->rdParam[8],mr->rdParam[7],
                       mr->rdParam[6],mr->rdParam[5],mr->rdParam[4],
                       mr->rdParam[3],mr->rdParam[2],bits,info,
                       DIB_RGB_COLORS,MAKELONG(mr->rdParam[0],mr->rdParam[1]));
      }
      break;		  

    case META_STRETCHBLT:
      {
       HDC16 hdcSrc=CreateCompatibleDC16(hdc);
       HBITMAP32 hbitmap=CreateBitmap32(mr->rdParam[10], /*Width */
                                        mr->rdParam[11], /*Height*/
                                        mr->rdParam[13], /*Planes*/
                                        mr->rdParam[14], /*BitsPixel*/
                                        (LPSTR)&mr->rdParam[15]);  /*bits*/
       SelectObject32(hdcSrc,hbitmap);
       StretchBlt16(hdc,mr->rdParam[9],mr->rdParam[8],
                    mr->rdParam[7],mr->rdParam[6],
		    hdcSrc,mr->rdParam[5],mr->rdParam[4],
		    mr->rdParam[3],mr->rdParam[2],
		    MAKELONG(mr->rdParam[0],mr->rdParam[1]));
       DeleteDC32(hdcSrc);		    
      }
      break;

    case META_BITBLT:            /* <-- not yet debugged */
      {
       HDC16 hdcSrc=CreateCompatibleDC16(hdc);
       HBITMAP32 hbitmap=CreateBitmap32(mr->rdParam[7]/*Width */,
                                        mr->rdParam[8]/*Height*/,
                                        mr->rdParam[10]/*Planes*/,
                                        mr->rdParam[11]/*BitsPixel*/,
                                        (LPSTR)&mr->rdParam[12]/*bits*/);
       SelectObject32(hdcSrc,hbitmap);
       BitBlt32(hdc,(INT16)mr->rdParam[6],(INT16)mr->rdParam[5],
                (INT16)mr->rdParam[4],(INT16)mr->rdParam[3],
                hdcSrc, (INT16)mr->rdParam[2],(INT16)mr->rdParam[1],
                MAKELONG(0,mr->rdParam[0]));
       DeleteDC32(hdcSrc);		    
      }
      break;

       /* --- Begin of new metafile operations. April, 1997 (ak) ----*/
    case META_CREATEREGION:
	 {
	    int i;
	    HRGN32 h1,h2,hrgn=CreateRectRgn32(mr->rdParam[7],mr->rdParam[8],
					      mr->rdParam[9],mr->rdParam[10]);
	    for (i=0,h1=CreateRectRgn32(0,0,0,0);i<mr->rdParam[5];i++)
	    {
	     if (mr->rdParam[11+i*6]==2)
	     { 
	       h2=CreateRectRgn32(mr->rdParam[14+i*6],mr->rdParam[12+i*6],
				  mr->rdParam[15+i*6],mr->rdParam[13+i*6]);
	       CombineRgn32(hrgn,h1,h2,mr->rdParam[16+i*6]);	/* e.g. RGN_OR */
	       h1=hrgn;
	     }
	    }
	    MF_AddHandle(ht, nHandles,hrgn);
         }
       break;

     case META_FILLREGION:
        FillRgn16(hdc, *(ht->objectHandle + *(mr->rdParam)),
		       *(ht->objectHandle + *(mr->rdParam+1)));
        break;

     case META_INVERTREGION:
        InvertRgn16(hdc, *(ht->objectHandle + *(mr->rdParam)));
        break; 

     case META_PAINTREGION:
        PaintRgn16(hdc, *(ht->objectHandle + *(mr->rdParam)));
        break;

     case META_SELECTCLIPREGION:
       	SelectClipRgn32(hdc, *(ht->objectHandle + *(mr->rdParam)));
	break;

#define META_UNIMP(x) case x: fprintf(stderr,"PlayMetaFileRecord:record type "#x" not implemented.\n");break;
    META_UNIMP(META_SETTEXTCHAREXTRA)
    META_UNIMP(META_SETTEXTJUSTIFICATION)
    META_UNIMP(META_FRAMEREGION)
    META_UNIMP(META_DRAWTEXT)
    META_UNIMP(META_SETDIBTODEV)
    META_UNIMP(META_ANIMATEPALETTE)
    META_UNIMP(META_SETPALENTRIES)
    META_UNIMP(META_RESIZEPALETTE)
    META_UNIMP(META_DIBBITBLT)
    META_UNIMP(META_DIBCREATEPATTERNBRUSH)
    META_UNIMP(META_EXTFLOODFILL)
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
	fprintf(stddeb,"PlayMetaFileRecord: Unknown record type %x\n",
	                                      mr->rdFunction);
    }
}


/******************************************************************
 *         GetMetaFileBits   (GDI.159)
 *
 * Trade in a meta file object handle for a handle to the meta file memory
 */

HGLOBAL16 GetMetaFileBits(HMETAFILE16 hmf)
{
    dprintf_metafile(stddeb,"GetMetaFileBits: hMem out: %04x\n", hmf);

    return hmf;
}

/******************************************************************
 *         SetMetaFileBits   (GDI.160)
 *
 * Trade in a meta file memory handle for a handle to a meta file object
 */
HMETAFILE16 SetMetaFileBits( HGLOBAL16 hMem )
{
    dprintf_metafile(stddeb,"SetMetaFileBits: hmf out: %04x\n", hMem);

    return hMem;
}

/******************************************************************
 *         SetMetaFileBitsBetter   (GDI.196)
 */
HMETAFILE16 SetMetaFileBitsBetter( HMETAFILE16 hMeta )
{
   if( IsValidMetaFile( hMeta ) )
       return (HMETAFILE16)GlobalReAlloc16( hMeta, 0, 
			   GMEM_SHARE | GMEM_NODISCARD | GMEM_MODIFY);
   return (HMETAFILE16)0;
}

/******************************************************************
 *         MF_WriteRecord
 *
 * Warning: this function can change the metafile handle.
 */

static BOOL32 MF_WriteRecord( DC *dc, METARECORD *mr, WORD rlen)
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
        dprintf_metafile(stddeb,"Writing record to disk\n");
	if (_lwrite32(physDev->mh->mtNoParameters, (char *)mr, rlen) == -1)
	    return FALSE;
        break;
    default:
        fprintf( stderr, "Unknown metafile type %d\n", physDev->mh->mtType );
        return FALSE;
    }

    physDev->mh->mtSize += rlen / 2;
    physDev->mh->mtMaxRecord = MAX(physDev->mh->mtMaxRecord, rlen / 2);
    return TRUE;
}


/******************************************************************
 *         MF_MetaParam0
 */

BOOL32 MF_MetaParam0(DC *dc, short func)
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
BOOL32 MF_MetaParam1(DC *dc, short func, short param1)
{
    char buffer[8];
    METARECORD *mr = (METARECORD *)&buffer;
    
    mr->rdSize = 4;
    mr->rdFunction = func;
    *(mr->rdParam) = param1;
    return MF_WriteRecord( dc, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MF_MetaParam2
 */
BOOL32 MF_MetaParam2(DC *dc, short func, short param1, short param2)
{
    char buffer[10];
    METARECORD *mr = (METARECORD *)&buffer;
    
    mr->rdSize = 5;
    mr->rdFunction = func;
    *(mr->rdParam) = param2;
    *(mr->rdParam + 1) = param1;
    return MF_WriteRecord( dc, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MF_MetaParam4
 */

BOOL32 MF_MetaParam4(DC *dc, short func, short param1, short param2, 
		   short param3, short param4)
{
    char buffer[14];
    METARECORD *mr = (METARECORD *)&buffer;
    
    mr->rdSize = 7;
    mr->rdFunction = func;
    *(mr->rdParam) = param4;
    *(mr->rdParam + 1) = param3;
    *(mr->rdParam + 2) = param2;
    *(mr->rdParam + 3) = param1;
    return MF_WriteRecord( dc, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MF_MetaParam6
 */

BOOL32 MF_MetaParam6(DC *dc, short func, short param1, short param2, 
		   short param3, short param4, short param5, short param6)
{
    char buffer[18];
    METARECORD *mr = (METARECORD *)&buffer;
    
    mr->rdSize = 9;
    mr->rdFunction = func;
    *(mr->rdParam) = param6;
    *(mr->rdParam + 1) = param5;
    *(mr->rdParam + 2) = param4;
    *(mr->rdParam + 3) = param3;
    *(mr->rdParam + 4) = param2;
    *(mr->rdParam + 5) = param1;
    return MF_WriteRecord( dc, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MF_MetaParam8
 */
BOOL32 MF_MetaParam8(DC *dc, short func, short param1, short param2, 
		   short param3, short param4, short param5,
		   short param6, short param7, short param8)
{
    char buffer[22];
    METARECORD *mr = (METARECORD *)&buffer;
    
    mr->rdSize = 11;
    mr->rdFunction = func;
    *(mr->rdParam) = param8;
    *(mr->rdParam + 1) = param7;
    *(mr->rdParam + 2) = param6;
    *(mr->rdParam + 3) = param5;
    *(mr->rdParam + 4) = param4;
    *(mr->rdParam + 5) = param3;
    *(mr->rdParam + 6) = param2;
    *(mr->rdParam + 7) = param1;
    return MF_WriteRecord( dc, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MF_CreateBrushIndirect
 */

BOOL32 MF_CreateBrushIndirect(DC *dc, HBRUSH16 hBrush, LOGBRUSH16 *logbrush)
{
    int index;
    char buffer[sizeof(METARECORD) - 2 + sizeof(*logbrush)];
    METARECORD *mr = (METARECORD *)&buffer;

    mr->rdSize = (sizeof(METARECORD) + sizeof(*logbrush) - 2) / 2;
    mr->rdFunction = META_CREATEBRUSHINDIRECT;
    memcpy(&(mr->rdParam), logbrush, sizeof(*logbrush));
    if (!(MF_WriteRecord( dc, mr, mr->rdSize * 2))) return FALSE;

    mr->rdSize = sizeof(METARECORD) / 2;
    mr->rdFunction = META_SELECTOBJECT;

    if ((index = MF_AddHandleDC( dc )) == -1) return FALSE;
    *(mr->rdParam) = index;
    return MF_WriteRecord( dc, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MF_CreatePatternBrush
 */

BOOL32 MF_CreatePatternBrush(DC *dc, HBRUSH16 hBrush, LOGBRUSH16 *logbrush)
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
	    return FALSE;
	mr = (METARECORD *)GlobalLock16(hmr);
	memset(mr, 0, len);
	mr->rdFunction = META_DIBCREATEPATTERNBRUSH;
	mr->rdSize = len / 2;
	*(mr->rdParam) = logbrush->lbStyle;
	*(mr->rdParam + 1) = DIB_RGB_COLORS;
	infohdr = (BITMAPINFOHEADER *)(mr->rdParam + 2);
	infohdr->biSize = sizeof(BITMAPINFOHEADER);
	infohdr->biWidth = bmp->bitmap.bmWidth;
	infohdr->biHeight = bmp->bitmap.bmHeight;
	infohdr->biPlanes = bmp->bitmap.bmPlanes;
	infohdr->biBitCount = bmp->bitmap.bmBitsPixel;
	memcpy(mr->rdParam + (sizeof(BITMAPINFOHEADER) / 2) + 4,
	       PTR_SEG_TO_LIN(bmp->bitmap.bmBits),
	       bmp->bitmap.bmHeight * bmp->bitmap.bmWidthBytes);
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
	*(mr->rdParam) = logbrush->lbStyle;
	*(mr->rdParam + 1) = LOWORD(logbrush->lbColor);
	memcpy(mr->rdParam + 2, info, biSize + bmSize);
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
    *(mr->rdParam) = index;
    return MF_WriteRecord( dc, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MF_CreatePenIndirect
 */

BOOL32 MF_CreatePenIndirect(DC *dc, HPEN16 hPen, LOGPEN16 *logpen)
{
    int index;
    char buffer[sizeof(METARECORD) - 2 + sizeof(*logpen)];
    METARECORD *mr = (METARECORD *)&buffer;

    mr->rdSize = (sizeof(METARECORD) + sizeof(*logpen) - 2) / 2;
    mr->rdFunction = META_CREATEPENINDIRECT;
    memcpy(&(mr->rdParam), logpen, sizeof(*logpen));
    if (!(MF_WriteRecord( dc, mr, mr->rdSize * 2))) return FALSE;

    mr->rdSize = sizeof(METARECORD) / 2;
    mr->rdFunction = META_SELECTOBJECT;

    if ((index = MF_AddHandleDC( dc )) == -1) return FALSE;
    *(mr->rdParam) = index;
    return MF_WriteRecord( dc, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MF_CreateFontIndirect
 */

BOOL32 MF_CreateFontIndirect(DC *dc, HFONT16 hFont, LOGFONT16 *logfont)
{
    int index;
    char buffer[sizeof(METARECORD) - 2 + sizeof(LOGFONT16)];
    METARECORD *mr = (METARECORD *)&buffer;

    mr->rdSize = (sizeof(METARECORD) + sizeof(LOGFONT16) - 2) / 2;
    mr->rdFunction = META_CREATEFONTINDIRECT;
    memcpy(&(mr->rdParam), logfont, sizeof(LOGFONT16));
    if (!(MF_WriteRecord( dc, mr, mr->rdSize * 2))) return FALSE;

    mr->rdSize = sizeof(METARECORD) / 2;
    mr->rdFunction = META_SELECTOBJECT;

    if ((index = MF_AddHandleDC( dc )) == -1) return FALSE;
    *(mr->rdParam) = index;
    return MF_WriteRecord( dc, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MF_TextOut
 */
BOOL32 MF_TextOut(DC *dc, short x, short y, LPCSTR str, short count)
{
    BOOL32 ret;
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
    *(mr->rdParam) = count;
    memcpy(mr->rdParam + 1, str, count);
    *(mr->rdParam + ((count + 1) >> 1) + 1) = y;
    *(mr->rdParam + ((count + 1) >> 1) + 2) = x;
    ret = MF_WriteRecord( dc, mr, mr->rdSize * 2);
    GlobalFree16(hmr);
    return ret;
}

/******************************************************************
 *         MF_ExtTextOut
 */
BOOL32 MF_ExtTextOut(DC*dc, short x, short y, UINT16 flags, const RECT16 *rect,
                     LPCSTR str, short count, const INT16 *lpDx)
{
    BOOL32 ret;
    DWORD len;
    HGLOBAL16 hmr;
    METARECORD *mr;

    len = sizeof(METARECORD) + (((count + 1) >> 1) * 2) + 2 * sizeof(short)
	    + sizeof(UINT16) + sizeof(RECT16);
    if (lpDx)
     len+=count*sizeof(INT16);
    if (!(hmr = GlobalAlloc16(GMEM_MOVEABLE, len)))
	return FALSE;
    mr = (METARECORD *)GlobalLock16(hmr);
    memset(mr, 0, len);

    mr->rdSize = len / 2;
    mr->rdFunction = META_EXTTEXTOUT;
    *(mr->rdParam) = y;
    *(mr->rdParam + 1) = x;
    *(mr->rdParam + 2) = count;
    *(mr->rdParam + 3) = flags;
    if (rect) memcpy(mr->rdParam + 4, rect, sizeof(RECT16));
    memcpy(mr->rdParam + 8, str, count);
    if (lpDx)
     memcpy(mr->rdParam + 8+ ((count + 1) >> 1),lpDx,count*sizeof(INT16));
    ret = MF_WriteRecord( dc, mr, mr->rdSize * 2);
    GlobalFree16(hmr);
    return ret;
}

/******************************************************************
 *         MF_MetaPoly - implements Polygon and Polyline
 */
BOOL32 MF_MetaPoly(DC *dc, short func, LPPOINT16 pt, short count)
{
    BOOL32 ret;
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
    *(mr->rdParam) = count;
    memcpy(mr->rdParam + 1, pt, count * 4);
    ret = MF_WriteRecord( dc, mr, mr->rdSize * 2);
    GlobalFree16(hmr);
    return ret;
}


/******************************************************************
 *         MF_BitBlt
 */
BOOL32 MF_BitBlt(DC *dcDest, short xDest, short yDest, short width,
                 short height, DC *dcSrc, short xSrc, short ySrc, DWORD rop)
{
    BOOL32 ret;
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
    *(mr->rdParam + 7) = BM.bmWidth;
    *(mr->rdParam + 8) = BM.bmHeight;
    *(mr->rdParam + 9) = BM.bmWidthBytes;
    *(mr->rdParam +10) = BM.bmPlanes;
    *(mr->rdParam +11) = BM.bmBitsPixel;
    dprintf_metafile(stddeb,"MF_StretchBlt->len = %ld  rop=%lx  \n",len,rop);
    if (GetBitmapBits32(dcSrc->w.hBitmap,BM.bmWidthBytes * BM.bmHeight,
                        mr->rdParam +12))
    {
      mr->rdSize = len / sizeof(INT16);
      *(mr->rdParam) = HIWORD(rop);
      *(mr->rdParam + 1) = ySrc;
      *(mr->rdParam + 2) = xSrc;
      *(mr->rdParam + 3) = height;
      *(mr->rdParam + 4) = width;
      *(mr->rdParam + 5) = yDest;
      *(mr->rdParam + 6) = xDest;
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
BOOL32 MF_StretchBlt(DC *dcDest, short xDest, short yDest, short widthDest,
                     short heightDest, DC *dcSrc, short xSrc, short ySrc, 
                     short widthSrc, short heightSrc, DWORD rop)
{
    BOOL32 ret;
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
    lpBMI=(LPBITMAPINFOHEADER)(mr->rdParam+10);
    lpBMI->biSize      = sizeof(BITMAPINFOHEADER);
    lpBMI->biWidth     = BM.bmWidth;
    lpBMI->biHeight    = BM.bmHeight;
    lpBMI->biPlanes    = 1;
    lpBMI->biBitCount  = nBPP;                              /* 1,4,8 or 24 */
    lpBMI->biClrUsed   = nBPP != 24 ? 1 << nBPP : 0;
    lpBMI->biSizeImage = ((lpBMI->biWidth * nBPP + 31) / 32) * 4 * lpBMI->biHeight;
    lpBMI->biCompression = BI_RGB;
    lpBMI->biXPelsPerMeter = MulDiv32(GetDeviceCaps(dcSrc->hSelf,LOGPIXELSX),3937,100);
    lpBMI->biYPelsPerMeter = MulDiv32(GetDeviceCaps(dcSrc->hSelf,LOGPIXELSY),3937,100);
    lpBMI->biClrImportant  = 0;                          /* 1 meter  = 39.37 inch */

    dprintf_metafile(stddeb,"MF_StretchBltViaDIB->len = %ld  rop=%lx  PixYPM=%ld Caps=%d\n",
               len,rop,lpBMI->biYPelsPerMeter,GetDeviceCaps(hdcSrc,LOGPIXELSY));
    if (GetDIBits(hdcSrc,dcSrc->w.hBitmap,0,(UINT32)lpBMI->biHeight,
                  (LPSTR)lpBMI + DIB_BitmapInfoSize( (BITMAPINFO *)lpBMI,
                                                     DIB_RGB_COLORS ),
                  (LPBITMAPINFO)lpBMI, DIB_RGB_COLORS))
#else
    len = sizeof(METARECORD) + 15 * sizeof(INT16) + BM.bmWidthBytes * BM.bmHeight;
    if (!(hmr = GlobalAlloc16(GMEM_MOVEABLE, len)))
	return FALSE;
    mr = (METARECORD *)GlobalLock16(hmr);
    mr->rdFunction = META_STRETCHBLT;
    *(mr->rdParam +10) = BM.bmWidth;
    *(mr->rdParam +11) = BM.bmHeight;
    *(mr->rdParam +12) = BM.bmWidthBytes;
    *(mr->rdParam +13) = BM.bmPlanes;
    *(mr->rdParam +14) = BM.bmBitsPixel;
    dprintf_metafile(stddeb,"MF_StretchBlt->len = %ld  rop=%lx  \n",len,rop);
    if (GetBitmapBits32( dcSrc->w.hBitmap, BM.bmWidthBytes * BM.bmHeight,
                         mr->rdParam +15))
#endif    
    {
      mr->rdSize = len / sizeof(INT16);
      *(mr->rdParam) = LOWORD(rop);
      *(mr->rdParam + 1) = HIWORD(rop);
      *(mr->rdParam + 2) = heightSrc;
      *(mr->rdParam + 3) = widthSrc;
      *(mr->rdParam + 4) = ySrc;
      *(mr->rdParam + 5) = xSrc;
      *(mr->rdParam + 6) = heightDest;
      *(mr->rdParam + 7) = widthDest;
      *(mr->rdParam + 8) = yDest;
      *(mr->rdParam + 9) = xDest;
      ret = MF_WriteRecord( dcDest, mr, mr->rdSize * 2);
    }  
    else
        ret = FALSE;
    GlobalFree16(hmr);
    return ret;
}


