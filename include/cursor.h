/*
 * structure definitions for CURSOR
 *
 * Copyright  Martin Ayotte, 1993
 *
 */
#ifndef __WINE_CURSOR_H
#define __WINE_CURSOR_H


typedef struct {
        POINT           pntHotSpot;     /* cursor hot spot */
        WORD            nWidth;         /* width of bitmap in pixels */
        WORD            nHeight;
        WORD            nWidthBytes;
        BYTE            byPlanes;       /* number of bit planes */
        BYTE            byBitsPix;      /* bits per pixel */
        } CURSORICONINFO, FAR *LPCURSORICONINFO;

typedef struct  {
	WORD	cdReserved;
	WORD	cdType;
	WORD	cdCount;
        } CURSORDIR;
 
typedef struct {
	WORD	Width;
	WORD	Height;
	WORD	Planes;
	WORD	BitCount;
	DWORD	curDIBSize;
	WORD	curDIBOffset;
	} CURSORDESCRIP;

typedef struct  {
	Cursor	xcursor;
	} CURSORALLOC;

extern void CURSOR_SetWinCursor( HWND hwnd, HCURSOR hcursor );   /* cursor.c */

#endif /* __WINE_CURSOR_H */
