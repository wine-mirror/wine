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

HMETAFILE16 MF_WriteRecord(HMETAFILE16 hmf, METARECORD *mr, WORD rlen);
int MF_AddHandle(HANDLETABLE16 *ht, WORD htlen, HANDLE hobj);
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
BOOL MF_CreateBrushIndirect(DC *dc, HBRUSH hBrush, LOGBRUSH16 *logbrush);
BOOL MF_CreatePatternBrush(DC *dc, HBRUSH hBrush, LOGBRUSH16 *logbrush);
BOOL MF_CreatePenIndirect(DC *dc, HPEN16 hPen, LOGPEN16 *logpen);
BOOL MF_CreateFontIndirect(DC *dc, HFONT hFont, LOGFONT16 *logfont);
BOOL MF_TextOut(DC *dc, short x, short y, LPCSTR str, short count);
BOOL MF_ExtTextOut(DC *dc, short x, short y, UINT16 flags, const RECT16 *rect,
                   LPCSTR str, short count, const INT16 *lpDx);
BOOL MF_MetaPoly(DC *dc, short func, LPPOINT16 pt, short count);
BOOL MF_BitBlt(DC *dcDest, short xDest, short yDest, short width,
	       short height, HDC hdcSrc, short xSrc, short ySrc, DWORD rop);
BOOL MF_StretchBlt(DC *dcDest, short xDest, short yDest, short widthDest,
		   short heightDest, HDC hdcSrc, short xSrc, short ySrc, 
		   short widthSrc, short heightSrc, DWORD rop);

#endif   /* METAFILE_H */

