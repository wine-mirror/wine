/*
 * Metafile definitions
 *
 * Copyright  David W. Metcalfe, 1994
 */

#ifndef METAFILE_H
#define METAFILE_H

#include "windows.h"

#define MFHEADERSIZE (sizeof(METAHEADER))
#define MFVERSION 0x300
#define META_EOF 0x0000

typedef struct tagMETAFILE
{
    WORD   wMagic;	    /* `PO' */
    char   Filename[80];    /* metafile name, if disk based */
    int    hFile;           /* MSDOS file handle for metafile */
    HANDLE hMetaHdr;	    /* handle of metafile header */
    int    MetaOffset;      /* offset of current record in metafile */
    HANDLE hBuffer;	    /* handle of buffer for disk based metafiles */
} METAFILE;
typedef METAFILE *LPMETAFILE;


BOOL MF_WriteRecord(HMETAFILE hmf, METARECORD *mr, WORD rlen);
int MF_AddHandle(HANDLETABLE *ht, WORD htlen, HANDLE hobj);
int MF_AddHandleInternal(HANDLE hobj);
BOOL MF_MetaParam0(DC *dc, short func);
BOOL MF_MetaParam1(DC *dc, short func, short param1);
BOOL MF_MetaParam2(DC *dc, short func, short param1, short param2);
BOOL MF_MetaParam4(DC *dc, short func, short param1, short param2, 
		   short param3, short param4);
BOOL MF_MetaParam6(DC *dc, short func, short param1, short param2, 
		   short param3, short param4, short param5, short param6);
BOOL MF_MetaParam8(DC *dc, short func, short param1, short param2, 
		   short param3, short param4, short param5,
		   short param6, short param7, short param8);
BOOL MF_CreateBrushIndirect(DC *dc, HBRUSH hBrush, LOGBRUSH *logbrush);
BOOL MF_CreatePatternBrush(DC *dc, HBRUSH hBrush, LOGBRUSH *logbrush);
BOOL MF_CreatePenIndirect(DC *dc, HPEN hPen, LOGPEN *logpen);
BOOL MF_CreateFontIndirect(DC *dc, HFONT hFont, LOGFONT *logfont);
BOOL MF_TextOut(DC *dc, short x, short y, LPSTR str, short count);
BOOL MF_MetaPoly(DC *dc, short func, LPPOINT pt, short count);
BOOL MF_BitBlt(DC *dcDest, short xDest, short yDest, short width,
	       short height, HDC hdcSrc, short xSrc, short ySrc, DWORD rop);
BOOL MF_StretchBlt(DC *dcDest, short xDest, short yDest, short widthDest,
		   short heightDest, HDC hdcSrc, short xSrc, short ySrc, 
		   short widthSrc, short heightSrc, DWORD rop);

#endif   /* METAFILE_H */

