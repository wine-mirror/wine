/*
 * Metafile functions
 *
 * Copyright  David W. Metcalfe, 1994
 *            Niels de Carpentier, 1996
 *
 */

#include <string.h>
#include <fcntl.h>
#include "gdi.h"
#include "bitmap.h"
#include "file.h"
#include "metafile.h"
#include "stddebug.h"
#include "callback.h"
#include "debug.h"

#define HTINCR  10      /* handle table allocation size increment */

static HANDLE hHT;      /* handle of the handle table */
static int HTLen;       /* allocated length of handle table */

/******************************************************************
 *         GetMetafile         GDI.124 By Kenny MacDonald 30 Nov 94
 */

HMETAFILE GetMetaFile(LPSTR lpFilename)
{
  HMETAFILE hmf;
  METAHEADER *mh;
  HFILE hFile;
  DWORD size;
  
  dprintf_metafile(stddeb,"GetMetaFile: %s\n", lpFilename);

  if (!lpFilename)
    return 0;

  hmf = GlobalAlloc16(GMEM_MOVEABLE, MFHEADERSIZE);
  mh = (METAHEADER *)GlobalLock16(hmf);
  
  if (!mh) {
    GlobalFree16(hmf);
    return 0;
  }
  
  if ((hFile = _lopen(lpFilename, OF_READ)) == HFILE_ERROR) {
    GlobalFree16(hmf);
    return 0;
  }
  
  if (FILE_Read(hFile, (char *)mh, MFHEADERSIZE) == HFILE_ERROR) {
    GlobalFree16(hmf);
    return 0;
  }
  
  size = mh->mtSize * 2;         /* alloc memory for whole metafile */
  GlobalUnlock16(hmf);
  hmf = GlobalReAlloc16(hmf,size,GMEM_MOVEABLE);
  mh = (METAHEADER *)GlobalLock16(hmf);
  
  if (!mh) {
    GlobalFree16(hmf);
    return 0;
  }
  
  if (FILE_Read(hFile, (char*)mh + mh->mtHeaderSize * 2, 
	        size - mh->mtHeaderSize * 2) == HFILE_ERROR) {
    GlobalFree16(hmf);
    return 0;
  }
  
  _lclose(hFile);

  if (mh->mtType != 1) {
    GlobalFree16(hmf);
    return 0;
  }
  
  GlobalUnlock16(hmf);
  return hmf;

}

/******************************************************************
 *         CreateMetafile         GDI.125
 */

HANDLE CreateMetaFile(LPCSTR lpFilename)
{
    DC *dc;
    HANDLE handle;
    METAHEADER *mh;
    int hFile;
    
    dprintf_metafile(stddeb,"CreateMetaFile: %s\n", lpFilename);

    handle = GDI_AllocObject(sizeof(DC), METAFILE_DC_MAGIC);
    
    if (!handle) 
      return 0;
    
    dc = (DC *)GDI_HEAP_LIN_ADDR(handle);

    if (!(dc->w.hMetaFile = GlobalAlloc16(GMEM_MOVEABLE, sizeof(METAHEADER)))) {
        GDI_FreeObject(handle);
	return 0;
    }
    
    mh = (METAHEADER *)GlobalLock16(dc->w.hMetaFile);

    mh->mtHeaderSize = MFHEADERSIZE / 2;
    mh->mtVersion = MFVERSION;
    mh->mtSize = MFHEADERSIZE / 2;
    mh->mtNoObjects = 0;
    mh->mtMaxRecord = 0;
    mh->mtNoParameters = 0;

    if (lpFilename)          /* disk based metafile */
    {
	mh->mtType = 1;     /* disk */
	hFile = _lcreat(lpFilename, 0);
	if (_lwrite(hFile, (char *)mh, MFHEADERSIZE) == -1)
	{
	    GlobalFree16(dc->w.hMetaFile);
	    return 0;
	}
	mh->mtNoParameters = hFile; /* store file descriptor here */
	                            /* windows probably uses this too*/
    }
    else                     /* memory based metafile */
	mh->mtType = 0;

    /* create the handle table */
    HTLen = HTINCR;
    hHT = GlobalAlloc16(GMEM_MOVEABLE | GMEM_ZEROINIT, 
		      sizeof(HANDLETABLE) * HTLen);
    
    GlobalUnlock16(dc->w.hMetaFile);
    dprintf_metafile(stddeb,"CreateMetaFile: returning %04x\n", handle);
    return handle;
}


/******************************************************************
 *         CopyMetafile         GDI.151 Niels de Carpentier, April 1996
 */

HMETAFILE CopyMetaFile(HMETAFILE hSrcMetaFile, LPCSTR lpFilename)
{
    HMETAFILE handle = 0;
    METAHEADER *mh;
    METAHEADER *mh2;
    int hFile;
    
    dprintf_metafile(stddeb,"CopyMetaFile: %s\n", lpFilename);
    
    mh = (METAHEADER *)GlobalLock16(hSrcMetaFile);
    
    if (!mh)
      return 0;
    
    if (lpFilename)          /* disk based metafile */
        {
	hFile = _lcreat(lpFilename, 0);
	if (_lwrite(hFile, (char *)mh, mh->mtSize * 2) == -1)
	    {
	    _lclose(hFile);
	    return 0;
	    }
	_lclose(hFile);
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
 *         CloseMetafile         GDI.126
 */

HMETAFILE CloseMetaFile(HDC hdc)
{
    DC *dc;
    METAHEADER *mh;
    HMETAFILE hmf;
    HFILE hFile;
    
    dprintf_metafile(stddeb,"CloseMetaFile\n");

    dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
    
    if (!dc) 
      return 0;
    
    mh = (METAHEADER *)GlobalLock16(dc->w.hMetaFile);

    /* Construct the end of metafile record - this is documented
     * in SDK Knowledgebase Q99334.
     */

    if (!MF_MetaParam0(dc, META_EOF))
    {
	GlobalFree16(dc->w.hMetaFile);
	return 0;
    }	

    if (mh->mtType == 1)        /* disk based metafile */
    {
        hFile = mh->mtNoParameters;
	mh->mtNoParameters = 0;
        if (_llseek(hFile, 0L, 0) == -1)
        {
            GlobalFree16(dc->w.hMetaFile);
            return 0;
        }
        if (_lwrite(hFile, (char *)mh, MFHEADERSIZE) == -1)
        {
            GlobalFree16(dc->w.hMetaFile);
            return 0;
        }
        _lclose(hFile);
    }

    /* delete the handle table */
    GlobalFree16(hHT);

    hmf = dc->w.hMetaFile;
    GlobalUnlock16(hmf);
    GDI_FreeObject(hdc);
    return hmf;
}


/******************************************************************
 *         DeleteMetafile         GDI.127
 */

BOOL DeleteMetaFile(HMETAFILE hmf)
{
    METAHEADER *mh = (METAHEADER *)GlobalLock16(hmf);

    if (!mh)
	return FALSE;

    GlobalFree16(hmf);
    return TRUE;
}


/******************************************************************
 *         PlayMetafile         GDI.123
 */

BOOL PlayMetaFile(HDC hdc, HMETAFILE hmf)
{
    METAHEADER *mh = (METAHEADER *)GlobalLock16(hmf);
    METARECORD *mr;
    HANDLETABLE *ht;
    int offset = 0;
    WORD i;

    dprintf_metafile(stddeb,"PlayMetaFile(%04x %04x)\n",hdc,hmf);
    
    /* create the handle table */
    hHT = GlobalAlloc16(GMEM_MOVEABLE|GMEM_ZEROINIT,
		      sizeof(HANDLETABLE) * mh->mtNoObjects);
    ht = (HANDLETABLE *)GlobalLock16(hHT);

    /* loop through metafile playing records */
    offset = mh->mtHeaderSize * 2;
    while (offset < mh->mtSize * 2)
    {
	mr = (METARECORD *)((char *)mh + offset);
	dprintf_metafile(stddeb,"offset = %04x size = %08lx function = %04x\n",
			 offset,mr->rdSize,mr->rdFunction);
	offset += mr->rdSize * 2;
	PlayMetaFileRecord(hdc, ht, mr, mh->mtNoObjects);
    }

    /* free objects in handle table */
    for(i = 0; i < mh->mtNoObjects; i++)
      if(*(ht->objectHandle + i) != 0)
        DeleteObject(*(ht->objectHandle + i));
    
    /* free handle table */
    GlobalFree16(hHT);

    return TRUE;
}


/******************************************************************
 *         EnumMetafile         GDI.175    
 *                                    Niels de carpentier, april 1996
 */

BOOL EnumMetaFile(HDC hdc, HMETAFILE hmf, MFENUMPROC lpEnumFunc,LPARAM lpData)
{
    METAHEADER *mh = (METAHEADER *)GlobalLock16(hmf);
    METARECORD *mr;
    HANDLETABLE *ht;
    int offset = 0;
  
    dprintf_metafile(stddeb,"EnumMetaFile(%04x, %04x, %08lx, %08lx)\n",
		     hdc, hmf, (DWORD)lpEnumFunc, lpData);
   
    /* create the handle table */
    
    hHT = GlobalAlloc16(GMEM_MOVEABLE | GMEM_ZEROINIT,
		     sizeof(HANDLETABLE) * mh->mtNoObjects);
    ht = (HANDLETABLE *)GlobalLock16(hHT);
   
    offset = mh->mtHeaderSize * 2;
    
    /* loop through metafile records */
    
    while (offset < (mh->mtSize * 2))
    {
	mr = (METARECORD *)((char *)mh + offset);
        if (!CallEnumMetafileProc(lpEnumFunc, hdc, MAKE_SEGPTR(ht),
	                          MAKE_SEGPTR(mr), mh->mtNoObjects,
				  (LONG)lpData))
	    break;

	offset += (mr->rdSize * 2);
    }

    /* free handle table */
    GlobalFree16(hHT);

    return TRUE;
}


/******************************************************************
 *         PlayMetaFileRecord      GDI.176
 */

void PlayMetaFileRecord(HDC hdc, HANDLETABLE *ht, METARECORD *mr,
			                           WORD nHandles)
{
    short s1;
    HANDLE hndl;
    char *ptr;
    BITMAPINFOHEADER *infohdr;

    dprintf_metafile(stddeb,"PlayMetaFileRecord(%04x %08lx %08lx %04x)\n",
		     hdc,(LONG)ht, (LONG)mr, nHandles);
    
    switch (mr->rdFunction)
    {
    case META_EOF:
      break;

    case META_DELETEOBJECT:
      DeleteObject(*(ht->objectHandle + *(mr->rdParam)));
      *(ht->objectHandle + *(mr->rdParam)) = 0;
      break;

    case META_SETBKCOLOR:
	SetBkColor(hdc, *(mr->rdParam));
	break;

    case META_SETBKMODE:
	SetBkMode(hdc, *(mr->rdParam));
	break;

    case META_SETMAPMODE:
	SetMapMode(hdc, *(mr->rdParam));
	break;

    case META_SETROP2:
	SetROP2(hdc, *(mr->rdParam));
	break;

    case META_SETRELABS:
	SetRelAbs(hdc, *(mr->rdParam));
	break;

    case META_SETPOLYFILLMODE:
	SetPolyFillMode(hdc, *(mr->rdParam));
	break;

    case META_SETSTRETCHBLTMODE:
	SetStretchBltMode(hdc, *(mr->rdParam));
	break;

    case META_SETTEXTCOLOR:
	SetTextColor(hdc, MAKELONG(*(mr->rdParam), *(mr->rdParam + 1)));
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
	LineTo(hdc, *(mr->rdParam + 1), *(mr->rdParam));
	break;

    case META_MOVETO:
	MoveTo(hdc, *(mr->rdParam + 1), *(mr->rdParam));
	break;

    case META_EXCLUDECLIPRECT:
	ExcludeClipRect(hdc, *(mr->rdParam + 3), *(mr->rdParam + 2),
			*(mr->rdParam + 1), *(mr->rdParam));
	break;

    case META_INTERSECTCLIPRECT:
	IntersectClipRect(hdc, *(mr->rdParam + 3), *(mr->rdParam + 2),
			*(mr->rdParam + 1), *(mr->rdParam));
	break;

    case META_ARC:
	Arc(hdc, *(mr->rdParam + 7), *(mr->rdParam + 6), *(mr->rdParam + 5),
	    *(mr->rdParam + 4), *(mr->rdParam + 3), *(mr->rdParam + 2),
	    *(mr->rdParam + 1), *(mr->rdParam));
	break;

    case META_ELLIPSE:
	Ellipse(hdc, *(mr->rdParam + 3), *(mr->rdParam + 2),
		*(mr->rdParam + 1), *(mr->rdParam));
	break;

    case META_FLOODFILL:
	FloodFill(hdc, *(mr->rdParam + 3), *(mr->rdParam + 2),
		  MAKELONG(*(mr->rdParam + 1), *(mr->rdParam)));
	break;

    case META_PIE:
	Pie(hdc, *(mr->rdParam + 7), *(mr->rdParam + 6), *(mr->rdParam + 5),
	    *(mr->rdParam + 4), *(mr->rdParam + 3), *(mr->rdParam + 2),
	    *(mr->rdParam + 1), *(mr->rdParam));
	break;

    case META_RECTANGLE:
	Rectangle(hdc, *(mr->rdParam + 3), *(mr->rdParam + 2),
		*(mr->rdParam + 1), *(mr->rdParam));
	break;

    case META_ROUNDRECT:
	RoundRect(hdc, *(mr->rdParam + 5), *(mr->rdParam + 4),
		  *(mr->rdParam + 3), *(mr->rdParam + 2),
		  *(mr->rdParam + 1), *(mr->rdParam));
	break;

    case META_PATBLT:
	PatBlt(hdc, *(mr->rdParam + 5), *(mr->rdParam + 4),
	       *(mr->rdParam + 3), *(mr->rdParam + 2),
	       MAKELONG(*(mr->rdParam), *(mr->rdParam + 1)));
	break;

    case META_SAVEDC:
	SaveDC(hdc);
	break;

    case META_SETPIXEL:
	SetPixel(hdc, *(mr->rdParam + 3), *(mr->rdParam + 2),
		 MAKELONG(*(mr->rdParam + 1), *(mr->rdParam)));
	break;

    case META_OFFSETCLIPRGN:
	OffsetClipRgn(hdc, *(mr->rdParam + 1), *(mr->rdParam));
	break;

    case META_TEXTOUT:
	s1 = *(mr->rdParam);
	TextOut(hdc, *(mr->rdParam + ((s1 + 1) >> 1) + 2),
		*(mr->rdParam + ((s1 + 1) >> 1) + 1), 
		(char *)(mr->rdParam + 1), s1);
	break;

    case META_POLYGON:
	Polygon(hdc, (LPPOINT)(mr->rdParam + 1), *(mr->rdParam));
	break;

    case META_POLYPOLYGON:
      PolyPolygon(hdc, (LPPOINT)(mr->rdParam + *(mr->rdParam) + 1),
		  (LPINT16)(mr->rdParam + 1), *(mr->rdParam)); 
      break;

    case META_POLYLINE:
	Polyline(hdc, (LPPOINT)(mr->rdParam + 1), *(mr->rdParam));
	break;

    case META_RESTOREDC:
	RestoreDC(hdc, *(mr->rdParam));
	break;

    case META_SELECTOBJECT:
	SelectObject(hdc, *(ht->objectHandle + *(mr->rdParam)));
	break;

    case META_CHORD:
	Chord(hdc, *(mr->rdParam + 7), *(mr->rdParam + 6), *(mr->rdParam + 5),
	      *(mr->rdParam + 4), *(mr->rdParam + 3), *(mr->rdParam + 2),
	      *(mr->rdParam + 1), *(mr->rdParam));
	break;

    case META_CREATEPATTERNBRUSH:
	switch (*(mr->rdParam))
	{
	case BS_PATTERN:
	    infohdr = (BITMAPINFOHEADER *)(mr->rdParam + 2);
	    MF_AddHandle(ht, nHandles,
			 CreatePatternBrush(CreateBitmap(infohdr->biWidth, 
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
			 CreateDIBPatternBrush(hndl, *(mr->rdParam + 1)));
	    GlobalFree16(hndl);
	}
	break;
	
    case META_CREATEPENINDIRECT:
	MF_AddHandle(ht, nHandles, 
		     CreatePenIndirect((LOGPEN *)(&(mr->rdParam))));
	break;

    case META_CREATEFONTINDIRECT:
	MF_AddHandle(ht, nHandles, 
		     CreateFontIndirect((LOGFONT *)(&(mr->rdParam))));
	break;

    case META_CREATEBRUSHINDIRECT:
	MF_AddHandle(ht, nHandles, 
		     CreateBrushIndirect((LOGBRUSH *)(&(mr->rdParam))));
	break;

    /* W. Magro: Some new metafile operations.  Not all debugged. */
    case META_CREATEPALETTE:
	MF_AddHandle(ht, nHandles, 
		     CreatePalette((LPLOGPALETTE)mr->rdParam));
	break;

    case META_SETTEXTALIGN:
       	SetTextAlign(hdc, *(mr->rdParam));
	break;

    case META_SELECTPALETTE:
	SelectPalette(hdc, *(ht->objectHandle + *(mr->rdParam+1)),*(mr->rdParam));
	break;

    case META_SETMAPPERFLAGS:
	SetMapperFlags(hdc, *(mr->rdParam));
	break;

    case META_REALIZEPALETTE:
	RealizePalette(hdc);
	break;

    case META_ESCAPE:
	dprintf_metafile(stddeb,"PlayMetaFileRecord: META_ESCAPE unimplemented.\n");
        break;

    case META_EXTTEXTOUT: /* FIXME: don't know the exact parameters here */
        {
        short x,y,options,x5,x6,x7,x8;
        y=mr->rdParam[0];  /* X position */
        x=mr->rdParam[1];  /* Y position */
        s1=mr->rdParam[2]; /* String length */
        options=mr->rdParam[3];
        x5=mr->rdParam[(s1+1)/2+4]; /* unknown meaning */
        x6=mr->rdParam[(s1+1)/2+5]; /* unknown meaning */
        x7=mr->rdParam[(s1+1)/2+6]; /* unknown meaning */
        x8=mr->rdParam[(s1+1)/2+7]; /* unknown meaning */
	ExtTextOut(hdc, x, y, options, (LPRECT) &mr->rdParam[(s1+1)/2+4], (char *)(mr->rdParam + 4), s1, NULL);
	/* fprintf(stderr,"EXTTEXTOUT (len: %d) %hd : %hd %hd %hd %hd [%s].\n",
            (mr->rdSize-s1),options,x5,x6,x7,x8,(char*) &(mr->rdParam[4]) );*/
        }
        break;
        /* End new metafile operations. */
    
    case META_STRETCHDIB:
      {
      LPSTR bits;
      LPBITMAPINFO info;
      int offset;
      info = (LPBITMAPINFO) &(mr->rdParam[11]);
      if (info->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
	{
        if (info->bmiHeader.biClrUsed)
	  {
	  if (info->bmiHeader.biClrUsed < (1 << info->bmiHeader.biBitCount))
	    offset = info->bmiHeader.biClrUsed * 4;
          else
	    offset = (1 << info->bmiHeader.biBitCount) * 4;
          }
        else
	  offset = (1 << info->bmiHeader.biBitCount) * 4;
	}
      else if (info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
	{
	if (info->bmiHeader.biClrUsed)
	  {
	  if (info->bmiHeader.biClrUsed < (1 << info->bmiHeader.biBitCount))
	    offset = info->bmiHeader.biClrUsed * 3;
          else
	    offset = (1 << info->bmiHeader.biBitCount) * 3;
          }
        else
	  offset = (1 << info->bmiHeader.biBitCount) * 3;
	}
      else
	{
	fprintf(stderr,"Unknown size for BITMAPHEADER in PlayMetaRecord!\n");
	break;
	}
	
      offset += info->bmiHeader.biSize; 
      bits = (LPSTR) info + offset;
      StretchDIBits(hdc,mr->rdParam[10],mr->rdParam[9],mr->rdParam[8],
		    mr->rdParam[7],mr->rdParam[6],mr->rdParam[5],
		    mr->rdParam[4],mr->rdParam[3],bits,info,
		    mr->rdParam[2],(DWORD)mr->rdParam[0]);
      }
      break;
		    
    default:
	fprintf(stddeb,"PlayMetaFileRecord: Unknown record type %x\n",
	                                      mr->rdFunction);
    }
}

/******************************************************************
 *         GetMetaFileBits		by William Magro, 19 Sep 1995
 *
 * Trade in a meta file object handle for a handle to the meta file memory
 */

HANDLE GetMetaFileBits(HMETAFILE hmf)
{
    dprintf_metafile(stddeb,"GetMetaFileBits: hMem out: %04x\n", hmf);

    return hmf;
}

/******************************************************************
 *         SetMetaFileBits		by William Magro, 19 Sep 1995
 *
 * Trade in a meta file memory handle for a handle to a meta file object
 */

HMETAFILE SetMetaFileBits(HANDLE hMem)
{
    dprintf_metafile(stddeb,"SetMetaFileBits: hmf out: %04x\n", hMem);

    return hMem;
}

/******************************************************************
 *         MF_WriteRecord
 */

HMETAFILE MF_WriteRecord(HMETAFILE hmf, METARECORD *mr, WORD rlen)
{
    DWORD len;
    METAHEADER *mh = (METAHEADER *)GlobalLock16(hmf);
    
    if (mh->mtType == 0)          /* memory based metafile */
    {
	len = mh->mtSize * 2 + rlen;
	GlobalUnlock16(hmf);
	hmf = GlobalReAlloc16(hmf, len, GMEM_MOVEABLE); /* hmf can change */
	mh = (METAHEADER *)GlobalLock16(hmf);
	memcpy((WORD *)mh + mh->mtSize, mr, rlen);
    }
    else if (mh->mtType == 1)     /* disk based metafile */
    {
      dprintf_metafile(stddeb,"Writing record to disk\n");
	if (_lwrite(mh->mtNoParameters, (char *)mr, rlen) == -1)
	{
	    GlobalUnlock16(hmf);
	    return 0;
	}
    }
    else
    {
	GlobalUnlock16(hmf);
	return 0;
    }

    mh->mtSize += rlen / 2;
    mh->mtMaxRecord = MAX(mh->mtMaxRecord, rlen / 2);
    GlobalUnlock16(hmf);
    return hmf;
}


/******************************************************************
 *         MF_AddHandle
 *
 *    Add a handle to an external handle table and return the index
 */

int MF_AddHandle(HANDLETABLE *ht, WORD htlen, HANDLE hobj)
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
 *         MF_AddHandleInternal
 *
 *    Add a handle to the internal handle table and return the index
 */

int MF_AddHandleInternal(HANDLE hobj)
{
    int i;
    HANDLETABLE *ht = (HANDLETABLE *)GlobalLock16(hHT);

    for (i = 0; i < HTLen; i++)
    {
	if (*(ht->objectHandle + i) == 0)
	{
	    *(ht->objectHandle + i) = hobj;
	    GlobalUnlock16(hHT);
	    return i;
	}
    }
    GlobalUnlock16(hHT);
    if (!(hHT = GlobalReAlloc16(hHT, HTINCR, GMEM_MOVEABLE | GMEM_ZEROINIT)))
	return -1;
    HTLen += HTINCR;
    ht = (HANDLETABLE *)GlobalLock16(hHT);
    *(ht->objectHandle + i) = hobj;
    GlobalUnlock16(hHT);
    return i;
}


/******************************************************************
 *         MF_MetaParam0
 */

BOOL MF_MetaParam0(DC *dc, short func)
{
    char buffer[8];
    METARECORD *mr = (METARECORD *)&buffer;
    HMETAFILE handle;
    
    mr->rdSize = 3;
    mr->rdFunction = func;
    handle = MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
    dc->w.hMetaFile = handle;
    
    return handle; 
}


/******************************************************************
 *         MF_MetaParam1
 */
BOOL MF_MetaParam1(DC *dc, short func, short param1)
{
    char buffer[8];
    METARECORD *mr = (METARECORD *)&buffer;
    HMETAFILE handle;
    
    mr->rdSize = 4;
    mr->rdFunction = func;
    *(mr->rdParam) = param1;
    handle = MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
    dc->w.hMetaFile = handle;

    return handle;
}


/******************************************************************
 *         MF_MetaParam2
 */
BOOL MF_MetaParam2(DC *dc, short func, short param1, short param2)
{
    char buffer[10];
    METARECORD *mr = (METARECORD *)&buffer;
    HMETAFILE handle;
    
    mr->rdSize = 5;
    mr->rdFunction = func;
    *(mr->rdParam) = param2;
    *(mr->rdParam + 1) = param1;
    handle = MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
    dc->w.hMetaFile = handle;
    
    return handle;
}


/******************************************************************
 *         MF_MetaParam4
 */

BOOL MF_MetaParam4(DC *dc, short func, short param1, short param2, 
		   short param3, short param4)
{
    char buffer[14];
    METARECORD *mr = (METARECORD *)&buffer;
    HMETAFILE handle;
    
    mr->rdSize = 7;
    mr->rdFunction = func;
    *(mr->rdParam) = param4;
    *(mr->rdParam + 1) = param3;
    *(mr->rdParam + 2) = param2;
    *(mr->rdParam + 3) = param1;
    handle = MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
    dc->w.hMetaFile = handle;
    
    return handle;
}


/******************************************************************
 *         MF_MetaParam6
 */

BOOL MF_MetaParam6(DC *dc, short func, short param1, short param2, 
		   short param3, short param4, short param5, short param6)
{
    char buffer[18];
    METARECORD *mr = (METARECORD *)&buffer;
    HMETAFILE handle;
    
    mr->rdSize = 9;
    mr->rdFunction = func;
    *(mr->rdParam) = param6;
    *(mr->rdParam + 1) = param5;
    *(mr->rdParam + 2) = param4;
    *(mr->rdParam + 3) = param3;
    *(mr->rdParam + 4) = param2;
    *(mr->rdParam + 5) = param1;
    handle = MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
    dc->w.hMetaFile = handle;
    
    return handle; 
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
    HMETAFILE handle;
    
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
    handle = MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
    dc->w.hMetaFile = handle;
    
    return handle;
}


/******************************************************************
 *         MF_CreateBrushIndirect
 */

BOOL MF_CreateBrushIndirect(DC *dc, HBRUSH hBrush, LOGBRUSH *logbrush)
{
    int index;
    HMETAFILE handle;
    char buffer[sizeof(METARECORD) - 2 + sizeof(LOGBRUSH)];
    METARECORD *mr = (METARECORD *)&buffer;
    METAHEADER *mh;

    mr->rdSize = (sizeof(METARECORD) + sizeof(LOGBRUSH) - 2) / 2;
    mr->rdFunction = META_CREATEBRUSHINDIRECT;
    memcpy(&(mr->rdParam), logbrush, sizeof(LOGBRUSH));
    if (!(dc->w.hMetaFile = MF_WriteRecord(dc->w.hMetaFile, 
					  mr, mr->rdSize * 2)))
	return FALSE;

    mr->rdSize = sizeof(METARECORD) / 2;
    mr->rdFunction = META_SELECTOBJECT;

    if ((index = MF_AddHandleInternal(hBrush)) == -1)
	return FALSE;

    mh = (METAHEADER *)GlobalLock16(dc->w.hMetaFile);
    *(mr->rdParam) = index;
    if (index >= mh->mtNoObjects)
	mh->mtNoObjects++;
    
    GlobalUnlock16(dc->w.hMetaFile);
    handle = MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
    dc->w.hMetaFile = handle;

    return handle;
}


/******************************************************************
 *         MF_CreatePatternBrush
 */

BOOL MF_CreatePatternBrush(DC *dc, HBRUSH hBrush, LOGBRUSH *logbrush)
{
    DWORD len, bmSize, biSize;
    HANDLE hmr;
    METARECORD *mr;
    BITMAPOBJ *bmp;
    BITMAPINFO *info;
    BITMAPINFOHEADER *infohdr;
    int index;
    HMETAFILE handle;
    char buffer[sizeof(METARECORD)];
    METAHEADER *mh;

    switch (logbrush->lbStyle)
    {
    case BS_PATTERN:
	bmp = (BITMAPOBJ *)GDI_GetObjPtr((HANDLE)logbrush->lbHatch, BITMAP_MAGIC);
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
	info = (BITMAPINFO *)GlobalLock16((HANDLE)logbrush->lbHatch);
	bmSize = info->bmiHeader.biSizeImage;
	if (!bmSize)
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
    if (!(dc->w.hMetaFile = MF_WriteRecord(dc->w.hMetaFile, mr, len)))
    {
	GlobalFree16(hmr);
	return FALSE;
    }

    GlobalFree16(hmr);
    
    mr = (METARECORD *)&buffer;
    mr->rdSize = sizeof(METARECORD) / 2;
    mr->rdFunction = META_SELECTOBJECT;
    if ((index = MF_AddHandleInternal(hBrush)) == -1)
	return FALSE;

    mh = (METAHEADER *)GlobalLock16(dc->w.hMetaFile);
    *(mr->rdParam) = index;
    if (index >= mh->mtNoObjects)
	mh->mtNoObjects++;
    GlobalUnlock16(dc->w.hMetaFile);
    handle = MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
    dc->w.hMetaFile = handle;

    return handle;
}


/******************************************************************
 *         MF_CreatePenIndirect
 */

BOOL MF_CreatePenIndirect(DC *dc, HPEN hPen, LOGPEN *logpen)
{
    int index;
    HMETAFILE handle;
    char buffer[sizeof(METARECORD) - 2 + sizeof(LOGPEN)];
    METARECORD *mr = (METARECORD *)&buffer;
    METAHEADER *mh;

    mr->rdSize = (sizeof(METARECORD) + sizeof(LOGPEN) - 2) / 2;
    mr->rdFunction = META_CREATEPENINDIRECT;
    memcpy(&(mr->rdParam), logpen, sizeof(LOGPEN));
    if (!(dc->w.hMetaFile = MF_WriteRecord(dc->w.hMetaFile, mr, 
					   mr->rdSize * 2)))
	return FALSE;

    mr->rdSize = sizeof(METARECORD) / 2;
    mr->rdFunction = META_SELECTOBJECT;

    if ((index = MF_AddHandleInternal(hPen)) == -1)
	return FALSE;

    mh = (METAHEADER *)GlobalLock16(dc->w.hMetaFile);
    *(mr->rdParam) = index;
    if (index >= mh->mtNoObjects)
	mh->mtNoObjects++;
    GlobalUnlock16(dc->w.hMetaFile);
    handle = MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
    dc->w.hMetaFile = handle;
    
    return handle;
}


/******************************************************************
 *         MF_CreateFontIndirect
 */

BOOL MF_CreateFontIndirect(DC *dc, HFONT hFont, LOGFONT *logfont)
{
    int index;
    HMETAFILE handle;
    char buffer[sizeof(METARECORD) - 2 + sizeof(LOGFONT)];
    METARECORD *mr = (METARECORD *)&buffer;
    METAHEADER *mh;

    mr->rdSize = (sizeof(METARECORD) + sizeof(LOGFONT) - 2) / 2;
    mr->rdFunction = META_CREATEFONTINDIRECT;
    memcpy(&(mr->rdParam), logfont, sizeof(LOGFONT));
    if (!(dc->w.hMetaFile = MF_WriteRecord(dc->w.hMetaFile, mr, 
					  mr->rdSize * 2)))
	return FALSE;

    mr->rdSize = sizeof(METARECORD) / 2;
    mr->rdFunction = META_SELECTOBJECT;

    if ((index = MF_AddHandleInternal(hFont)) == -1)
	return FALSE;

    mh = (METAHEADER *)GlobalLock16(dc->w.hMetaFile);
    *(mr->rdParam) = index;
    if (index >= mh->mtNoObjects)
	mh->mtNoObjects++;
    GlobalUnlock16(dc->w.hMetaFile);
    handle  = MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
    dc->w.hMetaFile = handle;
    
    return handle;
}


/******************************************************************
 *         MF_TextOut
 */
BOOL MF_TextOut(DC *dc, short x, short y, LPSTR str, short count)
{
    HMETAFILE handle;
    DWORD len;
    HANDLE hmr;
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
    handle = MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
    dc->w.hMetaFile = handle;
    GlobalFree16(hmr);
    return handle;
}


/******************************************************************
 *         MF_MetaPoly - implements Polygon and Polyline
 */
BOOL MF_MetaPoly(DC *dc, short func, LPPOINT pt, short count)
{
    HMETAFILE handle;
    DWORD len;
    HANDLE hmr;
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
    handle  = MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
    dc->w.hMetaFile = handle;
    GlobalFree16(hmr);
    return handle;
}


/******************************************************************
 *         MF_BitBlt
 */
BOOL MF_BitBlt(DC *dcDest, short xDest, short yDest, short width,
	       short height, HDC hdcSrc, short xSrc, short ySrc, DWORD rop)
{
    fprintf(stdnimp,"MF_BitBlt: not implemented yet\n");
    return FALSE;
}


/******************************************************************
 *         MF_StretchBlt
 */
BOOL MF_StretchBlt(DC *dcDest, short xDest, short yDest, short widthDest,
		   short heightDest, HDC hdcSrc, short xSrc, short ySrc, 
		   short widthSrc, short heightSrc, DWORD rop)
{
    fprintf(stdnimp,"MF_StretchBlt: not implemented yet\n");
    return FALSE;
}
