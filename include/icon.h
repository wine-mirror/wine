/*
 * structure definitions for ICON
 *
 * Copyright  Martin Ayotte, 1993
 *
 */


typedef struct {
	BYTE	Width;
	BYTE	Height;
	BYTE	ColorCount;
	BYTE	Reserved1;
	WORD	Reserved2;
	WORD	Reserved3;
	DWORD	icoDIBSize;
	DWORD	icoDIBOffset;
	} ICONDESCRIP;

typedef struct {
	ICONDESCRIP	descriptor;
	HBITMAP		hBitmap;
	HBITMAP		hBitMask;
	} ICONALLOC;


