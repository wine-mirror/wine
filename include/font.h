/*
 * GDI font definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_FONT_H
#define __WINE_FONT_H

#include "gdi.h"

#pragma pack(1)

  /* GDI logical font object */
typedef struct
{
    GDIOBJHDR   header;
    LOGFONT16   logfont WINE_PACKED;
} FONTOBJ;

typedef struct {
    WORD	dfVersion;
    DWORD	dfSize;
    CHAR	dfCopyright[60];
    WORD	dfType;
    WORD	dfPoints;
    WORD	dfVertRes;
    WORD	dfHorizRes;
    WORD	dfAscent;
    WORD	dfInternalLeading;
    WORD	dfExternalLeading;
    BYTE	dfItalic;
    BYTE	dfUnderline;
    BYTE	dfStrikeOut;
    WORD	dfWeight;
    BYTE	dfCharSet;
    WORD	dfPixWidth;
    WORD	dfPixHeight;
    BYTE	dfPitchAndFamily;
    WORD	dfAvgWidth;
    WORD	dfMaxWidth;
    BYTE	dfFirstChar;
    BYTE	dfLastChar;
    BYTE	dfDefaultChar;
    BYTE	dfBreakChar;
    WORD	dfWidthBytes;
    DWORD	dfDevice;
    DWORD	dfFace;
    DWORD	dfReserved;
    CHAR	szDeviceName[60]; /* FIXME: length unknown */
    CHAR	szFaceName[60];   /* dito */
} FONTDIR16, *LPFONTDIR16;

#pragma pack(4)

#define FONTCACHE 	32	/* dynamic font cache size */

extern BOOL32 FONT_Init( UINT16* pTextCaps );
extern INT16  FONT_GetObject16( FONTOBJ * font, INT16 count, LPSTR buffer );
extern INT32  FONT_GetObject32A( FONTOBJ * font, INT32 count, LPSTR buffer );

extern void FONT_LogFont32ATo16( const LOGFONT32A* font32, LPLOGFONT16 font16 );
extern void FONT_LogFont32WTo16( const LOGFONT32W* font32, LPLOGFONT16 font16 );
extern void FONT_LogFont16To32A( const LPLOGFONT16 font16, LPLOGFONT32A font32 );
extern void FONT_LogFont16To32W( const LPLOGFONT16 font16, LPLOGFONT32W font32 );
extern void FONT_TextMetric32Ato16(const LPTEXTMETRIC32A ptm32, LPTEXTMETRIC16 ptm16 );
extern void FONT_TextMetric32Wto16(const LPTEXTMETRIC32W ptm32, LPTEXTMETRIC16 ptm16 );
extern void FONT_TextMetric16to32A(const LPTEXTMETRIC16 ptm16, LPTEXTMETRIC32A ptm32 );
extern void FONT_TextMetric16to32W(const LPTEXTMETRIC16 ptm16, LPTEXTMETRIC32W ptm32 );
extern void FONT_TextMetric32Ato32W(const LPTEXTMETRIC32A ptm32A, LPTEXTMETRIC32W ptm32W );



#endif /* __WINE_FONT_H */
