/* Structure definitions for Win32 -- used only internally */
#ifndef _STRUCT32_H
#define _STRUCT32_H
#include "handle32.h"

#ifndef WINELIB
#pragma pack(1)
#endif

typedef struct tagRECT32
{
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
} RECT32;

void STRUCT32_RECT32to16(const RECT32*,RECT*);
void STRUCT32_RECT16to32(const RECT*,RECT32*);

typedef struct tagPOINT32
{
	LONG x;
	LONG y;
} POINT32;

typedef struct tagSIZE32
{
        LONG cx;
        LONG cy;
} SIZE32;
  
void STRUCT32_POINT32to16(const POINT32*,POINT*);
void STRUCT32_POINT16to32(const POINT*,POINT32*);
void STRUCT32_SIZE16to32(const SIZE* p16, SIZE32* p32);

typedef struct tagMINMAXINFO32
{
	POINT32 ptReserved;
	POINT32 ptMaxSize;
	POINT32 ptMaxPosition;
	POINT32 ptMinTrackSize;
	POINT32 ptMaxTrackSize;
} MINMAXINFO32;

void STRUCT32_MINMAXINFO32to16(const MINMAXINFO32*,MINMAXINFO*);
void STRUCT32_MINMAXINFO16to32(const MINMAXINFO*,MINMAXINFO32*);

typedef struct {
	DWORD style;
	DWORD dwExtendedStyle;
	WORD noOfItems WINE_PACKED;
	short x WINE_PACKED;
	short y WINE_PACKED;
	WORD cx WINE_PACKED;
	WORD cy WINE_PACKED;
} DLGTEMPLATE32;

typedef struct {
	DWORD style;
	DWORD dwExtendedStyle;
	short x WINE_PACKED;
	short y WINE_PACKED;
	short cx WINE_PACKED;
	short cy WINE_PACKED;
	WORD id WINE_PACKED;
} DLGITEMTEMPLATE32;

#define CW_USEDEFAULT32	0x80000000

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

typedef struct tagPAINTSTRUCT32
{
	DWORD hdc;
	DWORD fErase;
	RECT32 rcPaint;
	DWORD fRestore;
	DWORD fIncUpdate;
	BYTE rgbReserved[32];
} PAINTSTRUCT32;

typedef struct tagWINDOWPOS32
{
	DWORD	hwnd;
	DWORD	hwndInsertAfter;
	LONG	x;
	LONG	y;
	LONG	cx;
	LONG	cy;
	DWORD	flags;
} WINDOWPOS32;

void STRUCT32_WINDOWPOS32to16(const WINDOWPOS32*,WINDOWPOS*);
void STRUCT32_WINDOWPOS16to32(const WINDOWPOS*,WINDOWPOS32*);

typedef struct tagNCCALCSIZE_PARAMS32
{
	RECT32	rgrc[3];
	WINDOWPOS32	*lppos;
} NCCALCSIZE_PARAMS32;

void STRUCT32_NCCALCSIZE32to16Flat(const NCCALCSIZE_PARAMS32*,
	NCCALCSIZE_PARAMS*);
void STRUCT32_NCCALCSIZE16to32Flat(const NCCALCSIZE_PARAMS* from,
	NCCALCSIZE_PARAMS32* to);

typedef struct tagCREATESTRUCT32
{
	DWORD	lpCreateParams;
	DWORD	hInstance;
	DWORD	hMenu;
	DWORD	hwndParent;
	LONG	cy;
	LONG	cx;
	LONG	y;
	LONG	x;
	LONG	style;
	LPSTR	lpszName;
	LPSTR	lpszClass;
	DWORD	dwExStyle;
} CREATESTRUCT32;
typedef CREATESTRUCT32	CREATESTRUCTA;

void STRUCT32_CREATESTRUCT32to16(const CREATESTRUCT32*,CREATESTRUCT*);
void STRUCT32_CREATESTRUCT16to32(const CREATESTRUCT*,CREATESTRUCT32*);

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
