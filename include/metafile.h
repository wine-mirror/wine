/*
 * Metafile definitions
 *
 * Copyright  David W. Metcalfe, 1994
 */

#ifndef __WINE_METAFILE_H
#define __WINE_METAFILE_H

#include "windows.h"

#define MFHEADERSIZE (sizeof(METAHEADER))
#define MFVERSION 0x300
#define META_EOF 0x0000

BOOL32 MF_MetaParam0(DC *dc, short func);
BOOL32 MF_MetaParam1(DC *dc, short func, short param1);
BOOL32 MF_MetaParam2(DC *dc, short func, short param1, short param2);
BOOL32 MF_MetaParam4(DC *dc, short func, short param1, short param2, 
		   short param3, short param4);
BOOL32 MF_MetaParam6(DC *dc, short func, short param1, short param2, 
		   short param3, short param4, short param5, short param6);
BOOL32 MF_MetaParam8(DC *dc, short func, short param1, short param2, 
		   short param3, short param4, short param5,
		   short param6, short param7, short param8);
BOOL32 MF_CreateBrushIndirect(DC *dc, HBRUSH16 hBrush, LOGBRUSH16 *logbrush);
BOOL32 MF_CreatePatternBrush(DC *dc, HBRUSH16 hBrush, LOGBRUSH16 *logbrush);
BOOL32 MF_CreatePenIndirect(DC *dc, HPEN16 hPen, LOGPEN16 *logpen);
BOOL32 MF_CreateFontIndirect(DC *dc, HFONT16 hFont, LOGFONT16 *logfont);
BOOL32 MF_TextOut(DC *dc, short x, short y, LPCSTR str, short count);
BOOL32 MF_ExtTextOut(DC *dc, short x, short y, UINT16 flags, const RECT16 *rect,
                   LPCSTR str, short count, const INT16 *lpDx);
BOOL32 MF_MetaPoly(DC *dc, short func, LPPOINT16 pt, short count);
BOOL32 MF_BitBlt(DC *dcDest, short xDest, short yDest, short width,
	       short height, DC *dcSrc, short xSrc, short ySrc, DWORD rop);
BOOL32 MF_StretchBlt(DC *dcDest, short xDest, short yDest, short widthDest,
		   short heightDest, DC *dcSrc, short xSrc, short ySrc, 
		   short widthSrc, short heightSrc, DWORD rop);

#endif   /* __WINE_METAFILE_H */

