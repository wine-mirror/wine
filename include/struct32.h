/* Structure definitions for Win32 -- used only internally */
#ifndef _STRUCT32_H
#define _STRUCT32_H
#include "handle32.h"

#ifndef WINELIB
#pragma pack(1)
#endif

void STRUCT32_RECT32to16(const RECT32*,RECT16*);
void STRUCT32_RECT16to32(const RECT16*,RECT32*);
void STRUCT32_POINT32to16(const POINT32*,POINT16*);
void STRUCT32_POINT16to32(const POINT16*,POINT32*);
void STRUCT32_SIZE16to32(const SIZE16*, SIZE32*);

extern void STRUCT32_MINMAXINFO32to16( const MINMAXINFO32*, MINMAXINFO16* );
extern void STRUCT32_MINMAXINFO16to32( const MINMAXINFO16*, MINMAXINFO32* );
extern void STRUCT32_WINDOWPOS32to16( const WINDOWPOS32*, WINDOWPOS16* );
extern void STRUCT32_WINDOWPOS16to32( const WINDOWPOS16*, WINDOWPOS32* );
extern void STRUCT32_NCCALCSIZE32to16Flat( const NCCALCSIZE_PARAMS32 *from,
                                           NCCALCSIZE_PARAMS16 *to,
                                           int validRects );
extern void STRUCT32_NCCALCSIZE16to32Flat( const NCCALCSIZE_PARAMS16* from,
                                           NCCALCSIZE_PARAMS32* to,
                                           int validRects );


typedef struct {
	DWORD style;
	DWORD dwExtendedStyle;
	WORD noOfItems WINE_PACKED;
	short x WINE_PACKED;
	short y WINE_PACKED;
	WORD cx WINE_PACKED;
	WORD cy WINE_PACKED;
} DLGTEMPLATE32;

typedef struct tagMSG32
{
	DWORD hwnd;
	DWORD message;
	DWORD wParam;
	DWORD lParam;
	DWORD time;
	POINT32 pt;
} MSG32;

void STRUCT32_MSG16to32(MSG *msg16,MSG32 *msg32);
void STRUCT32_MSG32to16(MSG32 *msg32,MSG *msg16);

void STRUCT32_CREATESTRUCT32Ato16(const CREATESTRUCT32A*,CREATESTRUCT16*);
void STRUCT32_CREATESTRUCT16to32A(const CREATESTRUCT16*,CREATESTRUCT32A*);

typedef struct {
	BYTE   bWidth;
	BYTE   bHeight;
	BYTE   bColorCount;
	BYTE   bReserved;
	WORD   wPlanes;
	WORD   wBitCount;
	DWORD  dwBytesInRes;
	WORD   wResId WINE_PACKED;
	/*WORD   padding;	Spec is wrong, no padding here*/
} ICONDIRENTRY32;

typedef struct {
	WORD   wWidth;
	WORD   wHeight;
	WORD   wPlanes;
	WORD   wBitCount;
	DWORD  dwBytesInRes;
	WORD   wResId WINE_PACKED;
	/*WORD   padding;*/
} CURSORDIRENTRY32;

typedef union{
	ICONDIRENTRY32	icon;
	CURSORDIRENTRY32	cursor;
} CURSORICONDIRENTRY32;

typedef struct {
	WORD    idReserved;
	WORD    idType;
	WORD    idCount;
	/*WORD	padding;*/
	CURSORICONDIRENTRY32	idEntries[1];
} CURSORICONDIR32;



#ifndef WINELIB
#pragma pack(4)
#endif

#endif
