/* Structure definitions for Win32 -- used only internally */
#ifndef __WINE__STRUCT32_H
#define __WINE__STRUCT32_H
#include "handle32.h"

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

void STRUCT32_MSG16to32(const MSG16 *msg16,MSG32 *msg32);
void STRUCT32_MSG32to16(const MSG32 *msg32,MSG16 *msg16);

void STRUCT32_CREATESTRUCT32Ato16(const CREATESTRUCT32A*,CREATESTRUCT16*);
void STRUCT32_CREATESTRUCT16to32A(const CREATESTRUCT16*,CREATESTRUCT32A*);
void STRUCT32_MDICREATESTRUCT32Ato16( const MDICREATESTRUCT32A*,
                                      MDICREATESTRUCT16*);
void STRUCT32_MDICREATESTRUCT16to32A( const MDICREATESTRUCT16*,
                                      MDICREATESTRUCT32A*);

#pragma pack(1)

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


#pragma pack(4)

#endif  /* __WINE_STRUCT32_H */
