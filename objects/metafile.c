/*
 * Metafile functions
 *
 * Copyright  David W. Metcalfe, 1994
 */

static char Copyright[] = "Copyright  David W. Metcalfe, 1994";

#include "windows.h"
#include "gdi.h"
#include "metafile.h"
#include "prototypes.h"

#define DEBUG_METAFILE

/******************************************************************
 *         CreateMetafile         GDI.125
 */
HANDLE CreateMetaFile(LPSTR lpFilename)
{
    DC *dc;
    HANDLE handle;
    METAFILE *mf;
    METAHEADER *mh;

#ifdef DEBUG_METAFILE
    printf("CreateMetaFile: %s\n", lpFilename);
#endif

    handle = GDI_AllocObject(sizeof(DC), METAFILE_DC_MAGIC);
    if (!handle) return 0;
    dc = (DC *)GDI_HEAP_ADDR(handle);

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
    char buffer[15];
    METARECORD *mr = (METARECORD *)&buffer;

#ifdef DEBUG_METAFILE
    printf("CloseMetaFile\n");
#endif

    dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
    if (!dc) return 0;
    mf = (METAFILE *)GlobalLock(dc->w.hMetaFile);
    mh = (METAHEADER *)GlobalLock(mf->hMetaHdr);

    /* Construct the end of metafile record - this is undocumented
     * but is created by MS Windows 3.1.
     */
    mr->rdSize = 3;
    mr->rdFunction = META_EOF;
    MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);

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

    GlobalUnlock(mf->hMetaHdr);
    hmf = dc->w.hMetaFile;
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
BOOL MF_CreateBrushIndirect(DC *dc, LOGBRUSH *logbrush)
{
    char buffer[sizeof(METARECORD) - 2 + sizeof(LOGBRUSH)];
    METARECORD *mr = (METARECORD *)&buffer;
    METAFILE *mf = (METAFILE *)GlobalLock(dc->w.hMetaFile);
    METAHEADER *mh = (METAHEADER *)GlobalLock(mf->hMetaHdr);

#ifdef DEBUG_METAFILE
    printf("MF_CreateBrushIndirect\n");
#endif
    mr->rdSize = (sizeof(METARECORD) + sizeof(LOGBRUSH) - 2) / 2;
    mr->rdFunction = META_CREATEBRUSHINDIRECT;
    memcpy(&(mr->rdParam), logbrush, sizeof(LOGBRUSH));
    if (!MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2))
	return FALSE;

    mr->rdSize = sizeof(METARECORD) / 2;
    mr->rdFunction = META_SELECTOBJECT;
    *(mr->rdParam) = mh->mtNoObjects++;
    return MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MF_CreatePatternBrush
 */
BOOL MF_CreatePatternBrush(DC *dc, LOGBRUSH *logbrush)
{
    DWORD len, bmSize, biSize;
    HANDLE hmr;
    METARECORD *mr;
    BITMAPOBJ *bmp;
    BITMAPINFO *info;
    char buffer[sizeof(METARECORD)];
    METAFILE *mf = (METAFILE *)GlobalLock(dc->w.hMetaFile);
    METAHEADER *mh = (METAHEADER *)GlobalLock(mf->hMetaHdr);

    switch (logbrush->lbStyle)
    {
    case BS_PATTERN:
	bmp = (BITMAPOBJ *)GDI_GetObjPtr(logbrush->lbHatch, BITMAP_MAGIC);
	if (!bmp) return FALSE;
	len = sizeof(METARECORD) + sizeof(BITMAPINFOHEADER) + 
	      (bmp->bitmap.bmHeight * bmp->bitmap.bmWidthBytes) + 2;
	if (!(hmr = GlobalAlloc(GMEM_MOVEABLE, len)))
	    return FALSE;
	mr = (METARECORD *)GlobalLock(hmr);
	memset(mr, 0, len);
	mr->rdFunction = META_DIBCREATEPATTERNBRUSH;
	mr->rdSize = len / 2;
	*(mr->rdParam) = logbrush->lbStyle;
	memcpy(mr->rdParam + (sizeof(BITMAPINFOHEADER) / 2) + 2, 
	       bmp->bitmap.bmBits, 
	       bmp->bitmap.bmHeight * bmp->bitmap.bmWidthBytes);
	break;

    case BS_DIBPATTERN:
	info = (BITMAPINFO *)GlobalLock(logbrush->lbHatch);
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
    (WORD)(*(mr->rdParam)) = mh->mtNoObjects++;
    return MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
}


/******************************************************************
 *         MF_CreatePenIndirect
 */
BOOL MF_CreatePenIndirect(DC *dc, LOGPEN *logpen)
{
    char buffer[sizeof(METARECORD) - 2 + sizeof(LOGPEN)];
    METARECORD *mr = (METARECORD *)&buffer;
    METAFILE *mf = (METAFILE *)GlobalLock(dc->w.hMetaFile);
    METAHEADER *mh = (METAHEADER *)GlobalLock(mf->hMetaHdr);

#ifdef DEBUG_METAFILE
    printf("MF_CreatePenIndirect\n");
#endif
    mr->rdSize = (sizeof(METARECORD) + sizeof(LOGPEN) - 2) / 2;
    mr->rdFunction = META_CREATEPENINDIRECT;
    memcpy(&(mr->rdParam), logpen, sizeof(LOGPEN));
    if (!MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2))
	return FALSE;

    mr->rdSize = sizeof(METARECORD) / 2;
    mr->rdFunction = META_SELECTOBJECT;
    (WORD)(*(mr->rdParam)) = mh->mtNoObjects++;
    return MF_WriteRecord(dc->w.hMetaFile, mr, mr->rdSize * 2);
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

    len = sizeof(METARECORD) + count + 4;
    if (!(hmr = GlobalAlloc(GMEM_MOVEABLE, len)))
	return FALSE;
    mr = (METARECORD *)GlobalLock(hmr);
    memset(mr, 0, len);

    mr->rdSize = len / 2;
    mr->rdFunction = META_TEXTOUT;
    *(mr->rdParam) = count;
    memcpy(mr->rdParam + 1, str, count);
    *(mr->rdParam + (count / 2) + 1) = y;
    *(mr->rdParam + (count / 2) + 2) = x;
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
