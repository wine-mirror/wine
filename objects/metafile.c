/*
 * Metafile functions
 *
 * Copyright  David W. Metcalfe, 1994
 *
static char Copyright[] = "Copyright  David W. Metcalfe, 1994";
*/

#include <string.h>
#include "gdi.h"
#include "bitmap.h"
#include "metafile.h"
#include "stddebug.h"
/* #define DEBUG_METAFILE */
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
  METAFILE *mf;
  METAHEADER *mh;

  dprintf_metafile(stddeb,"GetMetaFile: %s\n", lpFilename);

  if (!lpFilename)
    return 0;

  hmf = GlobalAlloc(GMEM_MOVEABLE, sizeof(METAFILE));
  mf = (METAFILE *)GlobalLock(hmf);
  if (!mf) {
    GlobalFree(hmf);
    return 0;
  }

  mf->hMetaHdr = GlobalAlloc(GMEM_MOVEABLE, MFHEADERSIZE);
  mh = (METAHEADER *)GlobalLock(mf->hMetaHdr);
  if (!mh) {
    GlobalFree(mf->hMetaHdr);
    GlobalFree(hmf);
    return 0;
  }
  strcpy(mf->Filename, lpFilename);
  mf->wMagic = METAFILE_MAGIC;
  if ((mf->hFile = _lopen(lpFilename, OF_READ)) == HFILE_ERROR) {
    GlobalFree(hmf);
    return 0;
  }
  if (_lread(mf->hFile, (char *)mh, MFHEADERSIZE) == HFILE_ERROR) {
    GlobalFree(mf->hMetaHdr);
    GlobalFree(hmf);
    return 0;
  }
  _lclose(mf->hFile);


  GlobalUnlock(mf->hMetaHdr);
  GlobalUnlock(hmf);
  if (mh->mtType != 1)
    return 0;
  else
    return hmf;
}

/******************************************************************
 *         CreateMetafile         GDI.125
 */
HANDLE CreateMetaFile(LPSTR lpFilename)
{
    DC *dc;
    HANDLE handle;
    METAFILE *mf;
    METAHEADER *mh;
    HANDLETABLE *ht;

    dprintf_metafile(stddeb,"CreateMetaFile: %s\n", lpFilename);

    handle = GDI_AllocObject(sizeof(DC), METAFILE_DC_MAGIC);
    if (!handle) return 0;
    dc = (DC *)GDI_HEAP_LIN_ADDR(handle);

    if (!(dc->w.hMetaFile = GlobalAlloc(GMEM_MOVEABLE, sizeof(METAFILE))))
	return 0;
    mf = (METAFILE *)GlobalLock(dc->w.hMetaFile);
    if (!(mf->hMetaHdr = GlobalAlloc(GMEM_MOVEABLE, sizeof(METAHEADER))))
    {
	GlobalFree(dc->w.hMetaFile);
	return 0;
    }
    mh = (METAHEADER *)GlobalLock(mf->hMetaHdr);

    mf->wMagic = METAFILE_MAGIC;
    mh->mtHeaderSize = MFHEADERSIZE / 2;
    mh->mtVersion = MFVERSION;
    mh->mtSize = MFHEADERSIZE / 2;
    mh->mtNoObjects = 0;
    mh->mtMaxRecord = 0;
    mh->mtNoParameters = 0;

    if (lpFilename)          /* disk based metafile */
    {
	mh->mtType = 1;
	strcpy(mf->Filename, lpFilename);
	mf->hFile = _lcreat(lpFilename, 0);
	if (_lwrite(mf->hFile, (char *)mh, MFHEADERSIZE) == -1)
	{
	    GlobalFree(mf->hMetaHdr);
	    GlobalFree(dc->w.hMetaFile);
	    return 0;
	}
    }
    else                     /* memory based metafile */
	mh->mtType = 0;

    /* create the handle table */
    HTLen = HTINCR;
    hHT = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 
		      sizeof(HANDLETABLE) * HTLen);
    ht = (HANDLETABLE *)GlobalLock(hHT);

    GlobalUnlock(mf->hMetaHdr);
    GlobalUnlock(dc->w.hMetaFile);
    return handle;
}


/******************************************************************
 *         CloseMetafile         GDI.126
 */
HMETAFILE CloseMetaFile(HDC hdc)
{
    DC *dc;
    METAFILE *mf;
    METAHEADER *mh;
    HMETAFILE hmf;
/*    METARECORD *mr = (METARECORD *)&buffer;*/

    dprintf_metafile(stddeb,"CloseMetaFile\n");

    dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
    if (!dc) return 0;
    mf = (METAFILE *)GlobalLock(dc->w.hMetaFile);
    mh = (METAHEADER *)GlobalLock(mf->hMetaHdr);

    /* Construct the end of metafile record - this is documented
     * in SDK Knowledgebase Q99334.
     */
    if (!MF_MetaParam0(dc, META_EOF))
    {
	GlobalFree(mf->hMetaHdr);
	GlobalFree(dc->w.hMetaFile);
	return 0;
    }	

    if (mh->mtType == 1)        /* disk based metafile */
    {
	if (_llseek(mf->hFile, 0L, 0) == -1)
	{
	    GlobalFree(mf->hMetaHdr);
	    GlobalFree(dc->w.hMetaFile);
	    return 0;
	}
	if (_lwrite(mf->hFile, (char *)mh, MFHEADERSIZE) == -1)
	{
	    GlobalFree(mf->hMetaHdr);
	    GlobalFree(dc->w.hMetaFile);
	    return 0;
	}
	_lclose(mf->hFile);
    }

    /* delete the handle table */
    GlobalFree(hHT);

    GlobalUnlock(mf->hMetaHdr);
    hmf = dc->w.hMetaFile;
    GlobalUnlock(hmf);
    GDI_FreeObject(hdc);
    return hmf;
}


/******************************************************************
 *         DeleteMetafile         GDI.127
 */
BOOL DeleteMetaFile(HMETAFILE hmf)
{
    METAFILE *mf = (METAFILE *)GlobalLock(hmf);

    if (mf->wMagic != METAFILE_MAGIC)
	return FALSE;

    GlobalFree(mf->hMetaHdr);
    GlobalFree(hmf);
    return TRUE;
}


/******************************************************************
 *         PlayMetafile         GDI.123
 */
BOOL PlayMetaFile(HDC hdc, HMETAFILE hmf)
{
    METAFILE *mf = (METAFILE *)GlobalLock(hmf);
    METAHEADER *mh;
    METARECORD *mr;
    HANDLETABLE *ht;
    char *buffer = (char *)NULL;

    if (mf->wMagic != METAFILE_MAGIC)
	return FALSE;

    mh = (METAHEADER *)GlobalLock(mf->hMetaHdr);
    if (mh->mtType == 1)       /* disk based metafile */
    {
	mf->hFile = _lopen(mf->Filename, OF_READ);
	mf->hBuffer = GlobalAlloc(GMEM_MOVEABLE, mh->mtMaxRecord * 2);
	buffer = (char *)GlobalLock(mf->hBuffer);
	_llseek(mf->hFile, mh->mtHeaderSize * 2, 0);
	mf->MetaOffset = mh->mtHeaderSize * 2;
    }
    else if (mh->mtType == 0)   /* memory based metafile */
	mf->MetaOffset = mh->mtHeaderSize * 2;
    else                       /* not a valid metafile type */
	return FALSE;

    /* create the handle table */
    hHT = GlobalAlloc(GMEM_MOVEABLE, sizeof(HANDLETABLE) * mh->mtNoObjects);
    ht = (HANDLETABLE *)GlobalLock(hHT);

    /* loop through metafile playing records */
    while (mf->MetaOffset < mh->mtSize * 2)
    {
	if (mh->mtType == 1)   /* disk based metafile */
	{
	    _lread(mf->hFile, buffer, sizeof(METARECORD));
	    mr = (METARECORD *)buffer;
	    _lread(mf->hFile, (char *)(mr->rdParam + 1), (mr->rdSize * 2) -
		                                       sizeof(METARECORD));
	    mf->MetaOffset += mr->rdSize * 2;
	}
	else                   /* memory based metafile */
	{
	    mr = (METARECORD *)((char *)mh + mf->MetaOffset);
	    mf->MetaOffset += mr->rdSize * 2;
	}
	PlayMetaFileRecord(hdc, ht, mr, mh->mtNoObjects);
    }

    /* close disk based metafile and free buffer */
    if (mh->mtType == 1)
    {
	GlobalFree(mf->hBuffer);
	_lclose(mf->hFile);
    }

    /* free handle table */
    GlobalFree(hHT);

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

    switch (mr->rdFunction)
    {
    case META_EOF:
      break;

    case META_DELETEOBJECT:
      DeleteObject(*(ht->objectHandle + *(mr->rdParam)));
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
	Ellipse(hdc, *(mr->rdParam + 3), *(mr->rdParam + 2),
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
		  (mr->rdParam + 1), *(mr->rdParam)); 
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
	    hndl = GlobalAlloc(GMEM_MOVEABLE, s1);
	    ptr = GlobalLock(hndl);
	    memcpy(ptr, mr->rdParam + 2, s1);
	    GlobalUnlock(hndl);
	    MF_AddHandle(ht, nHandles,
			 CreateDIBPatternBrush(hndl, *(mr->rdParam + 1)));
	    GlobalFree(hndl);
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

    default:
	fprintf(stderr,"PlayMetaFileRecord: Unknown record type %x\n",
	                                      mr->rdFunction);
    }
}


/******************************************************************
 *         MF_WriteRecord
 */
BOOL MF_WriteRecord(HMETAFILE hmf, METARECORD *mr, WORD rlen)
{
    DWORD len;
    METAFILE *mf = (METAFILE *)GlobalLock(hmf);
    METAHEADER *mh = (METAHEADER *)GlobalLock(mf->hMetaHdr);

    if (mh->mtType == 0)          /* memory based metafile */
    {
	len = mh->mtSize * 2 + rlen;
	GlobalUnlock(mf->hMetaHdr);
	mf->hMetaHdr = GlobalReAlloc(mf->hMetaHdr, len, GMEM_MOVEABLE);
	mh = (METAHEADER *)GlobalLock(mf->hMetaHdr);
	memcpy(mh + mh->mtSize * 2, mr, rlen);
    }
    else if (mh->mtType == 1)     /* disk based metafile */
    {
	if (_lwrite(mf->hFile, (char *)mr, rlen) == -1)
	{
	    GlobalUnlock(mf->hMetaHdr);
	    return FALSE;
	}
    }
    else
    {
	GlobalUnlock(mf->hMetaHdr);
	return FALSE;
    }

    mh->mtSize += rlen / 2;
    mh->mtMaxRecord = max(mh->mtMaxRecord, rlen / 2);
    GlobalUnlock(mf->hMetaHdr);
    return TRUE;
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
    HANDLETABLE *ht = (HANDLETABLE *)GlobalLock(hHT);

    for (i = 0; i < HTLen; i++)
    {
	if (*(ht->objectHandle + i) == 0)
	{
	    *(ht->objectHandle + i) = hobj;
	    GlobalUnlock(hHT);
	    return i;
	}
    }
    GlobalUnlock(hHT);
    if (!(hHT = GlobalReAlloc(hHT, HTINCR, GMEM_MOVEABLE | GMEM_ZEROINIT)))
	return -1;
    HTLen += HTINCR;
    ht = (HANDLETABLE *)GlobalLock(hHT);
    *(ht->objectHandle + i) = hobj;
    GlobalUnlock(hHT);
    return i;
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
    return MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
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
    *(mr->rdParam) = param1;
    return MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
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
    *(mr->rdParam) = param2;
    *(mr->rdParam + 1) = param1;
    return MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
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
    *(mr->rdParam) = param4;
    *(mr->rdParam + 1) = param3;
    *(mr->rdParam + 2) = param2;
    *(mr->rdParam + 3) = param1;
    return MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
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
    *(mr->rdParam) = param6;
    *(mr->rdParam + 1) = param5;
    *(mr->rdParam + 2) = param4;
    *(mr->rdParam + 3) = param3;
    *(mr->rdParam + 4) = param2;
    *(mr->rdParam + 5) = param1;
    return MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
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
    *(mr->rdParam) = param8;
    *(mr->rdParam + 1) = param7;
    *(mr->rdParam + 2) = param6;
    *(mr->rdParam + 3) = param5;
    *(mr->rdParam + 4) = param4;
    *(mr->rdParam + 5) = param3;
    *(mr->rdParam + 6) = param2;
    *(mr->rdParam + 7) = param1;
    return MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MF_CreateBrushIndirect
 */
BOOL MF_CreateBrushIndirect(DC *dc, HBRUSH hBrush, LOGBRUSH *logbrush)
{
    int index;
    BOOL rc;
    char buffer[sizeof(METARECORD) - 2 + sizeof(LOGBRUSH)];
    METARECORD *mr = (METARECORD *)&buffer;
    METAFILE *mf;
    METAHEADER *mh;

    mr->rdSize = (sizeof(METARECORD) + sizeof(LOGBRUSH) - 2) / 2;
    mr->rdFunction = META_CREATEBRUSHINDIRECT;
    memcpy(&(mr->rdParam), logbrush, sizeof(LOGBRUSH));
    if (!MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2))
	return FALSE;

    mr->rdSize = sizeof(METARECORD) / 2;
    mr->rdFunction = META_SELECTOBJECT;
    if ((index = MF_AddHandleInternal(hBrush)) == -1)
	return FALSE;

    mf = (METAFILE *)GlobalLock(dc->w.hMetaFile);
    mh = (METAHEADER *)GlobalLock(mf->hMetaHdr);
    *(mr->rdParam) = index;
    if (index >= mh->mtNoObjects)
	mh->mtNoObjects++;
    rc = MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
    GlobalUnlock(mf->hMetaHdr);
    GlobalUnlock(dc->w.hMetaFile);
    return rc;
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
    BOOL rc;
    char buffer[sizeof(METARECORD)];
    METAFILE *mf;
    METAHEADER *mh;

    switch (logbrush->lbStyle)
    {
    case BS_PATTERN:
	bmp = (BITMAPOBJ *)GDI_GetObjPtr((HANDLE)logbrush->lbHatch, BITMAP_MAGIC);
	if (!bmp) return FALSE;
	len = sizeof(METARECORD) + sizeof(BITMAPINFOHEADER) + 
	      (bmp->bitmap.bmHeight * bmp->bitmap.bmWidthBytes) + 6;
	if (!(hmr = GlobalAlloc(GMEM_MOVEABLE, len)))
	    return FALSE;
	mr = (METARECORD *)GlobalLock(hmr);
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
	       bmp->bitmap.bmBits, 
	       bmp->bitmap.bmHeight * bmp->bitmap.bmWidthBytes);
	break;

    case BS_DIBPATTERN:
	info = (BITMAPINFO *)GlobalLock((HANDLE)logbrush->lbHatch);
	bmSize = info->bmiHeader.biSizeImage;
	if (!bmSize)
	    bmSize = (info->bmiHeader.biWidth * info->bmiHeader.biBitCount 
		    + 31) / 32 * 8 * info->bmiHeader.biHeight;
	biSize = DIB_BitmapInfoSize(info, LOWORD(logbrush->lbColor)); 
	len = sizeof(METARECORD) + biSize + bmSize + 2;
	if (!(hmr = GlobalAlloc(GMEM_MOVEABLE, len)))
	    return FALSE;
	mr = (METARECORD *)GlobalLock(hmr);
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
    if (!MF_WriteRecord(dc->w.hMetaFile, mr, len))
    {
	GlobalFree(hmr);
	return FALSE;
    }

    GlobalFree(hmr);
    mr = (METARECORD *)&buffer;
    mr->rdSize = sizeof(METARECORD) / 2;
    mr->rdFunction = META_SELECTOBJECT;
    if ((index = MF_AddHandleInternal(hBrush)) == -1)
	return FALSE;

    mf = (METAFILE *)GlobalLock(dc->w.hMetaFile);
    mh = (METAHEADER *)GlobalLock(mf->hMetaHdr);
    *(mr->rdParam) = index;
    if (index >= mh->mtNoObjects)
	mh->mtNoObjects++;
    rc = MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
    GlobalUnlock(mf->hMetaHdr);
    GlobalUnlock(dc->w.hMetaFile);
    return rc;
}


/******************************************************************
 *         MF_CreatePenIndirect
 */
BOOL MF_CreatePenIndirect(DC *dc, HPEN hPen, LOGPEN *logpen)
{
    int index;
    BOOL rc;
    char buffer[sizeof(METARECORD) - 2 + sizeof(LOGPEN)];
    METARECORD *mr = (METARECORD *)&buffer;
    METAFILE *mf;
    METAHEADER *mh;

    mr->rdSize = (sizeof(METARECORD) + sizeof(LOGPEN) - 2) / 2;
    mr->rdFunction = META_CREATEPENINDIRECT;
    memcpy(&(mr->rdParam), logpen, sizeof(LOGPEN));
    if (!MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2))
	return FALSE;

    mr->rdSize = sizeof(METARECORD) / 2;
    mr->rdFunction = META_SELECTOBJECT;
    if ((index = MF_AddHandleInternal(hPen)) == -1)
	return FALSE;

    mf = (METAFILE *)GlobalLock(dc->w.hMetaFile);
    mh = (METAHEADER *)GlobalLock(mf->hMetaHdr);
    *(mr->rdParam) = index;
    if (index >= mh->mtNoObjects)
	mh->mtNoObjects++;
    rc = MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
    GlobalUnlock(mf->hMetaHdr);
    GlobalUnlock(dc->w.hMetaFile);
    return rc;
}


/******************************************************************
 *         MF_CreateFontIndirect
 */
BOOL MF_CreateFontIndirect(DC *dc, HFONT hFont, LOGFONT *logfont)
{
    int index;
    BOOL rc;
    char buffer[sizeof(METARECORD) - 2 + sizeof(LOGFONT)];
    METARECORD *mr = (METARECORD *)&buffer;
    METAFILE *mf;
    METAHEADER *mh;

    mr->rdSize = (sizeof(METARECORD) + sizeof(LOGFONT) - 2) / 2;
    mr->rdFunction = META_CREATEFONTINDIRECT;
    memcpy(&(mr->rdParam), logfont, sizeof(LOGFONT));
    if (!MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2))
	return FALSE;

    mr->rdSize = sizeof(METARECORD) / 2;
    mr->rdFunction = META_SELECTOBJECT;
    if ((index = MF_AddHandleInternal(hFont)) == -1)
	return FALSE;

    mf = (METAFILE *)GlobalLock(dc->w.hMetaFile);
    mh = (METAHEADER *)GlobalLock(mf->hMetaHdr);
    *(mr->rdParam) = index;
    if (index >= mh->mtNoObjects)
	mh->mtNoObjects++;
    rc = MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
    GlobalUnlock(mf->hMetaHdr);
    GlobalUnlock(dc->w.hMetaFile);
    return rc;
}


/******************************************************************
 *         MF_TextOut
 */
BOOL MF_TextOut(DC *dc, short x, short y, LPSTR str, short count)
{
    BOOL rc;
    DWORD len;
    HANDLE hmr;
    METARECORD *mr;

    len = sizeof(METARECORD) + (((count + 1) >> 1) * 2) + 4;
    if (!(hmr = GlobalAlloc(GMEM_MOVEABLE, len)))
	return FALSE;
    mr = (METARECORD *)GlobalLock(hmr);
    memset(mr, 0, len);

    mr->rdSize = len / 2;
    mr->rdFunction = META_TEXTOUT;
    *(mr->rdParam) = count;
    memcpy(mr->rdParam + 1, str, count);
    *(mr->rdParam + ((count + 1) >> 1) + 1) = y;
    *(mr->rdParam + ((count + 1) >> 1) + 2) = x;
    rc = MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
    GlobalFree(hmr);
    return rc;
}


/******************************************************************
 *         MF_MetaPoly - implements Polygon and Polyline
 */
BOOL MF_MetaPoly(DC *dc, short func, LPPOINT pt, short count)
{
    BOOL rc;
    DWORD len;
    HANDLE hmr;
    METARECORD *mr;

    len = sizeof(METARECORD) + (count * 4); 
    if (!(hmr = GlobalAlloc(GMEM_MOVEABLE, len)))
	return FALSE;
    mr = (METARECORD *)GlobalLock(hmr);
    memset(mr, 0, len);

    mr->rdSize = len / 2;
    mr->rdFunction = func;
    *(mr->rdParam) = count;
    memcpy(mr->rdParam + 1, pt, count * 4);
    rc = MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
    GlobalFree(hmr);
    return rc;
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
