/*
 * structure definitions for CURSOR
 *
 * Copyright  Martin Ayotte, 1993
 *
 */

typedef struct {
	BYTE	Width;
	BYTE	Reserved1;
	BYTE	Height;
	BYTE	Reserved2;
	WORD	curXHotspot;
	WORD	curYHotspot;
	DWORD	curDIBSize;
	DWORD	curDIBOffset;
	} CURSORDESCRIP;

typedef struct {
	CURSORDESCRIP	descriptor;
	HBITMAP		hBitmap;
	Display		*display;
	Pixmap		pixshape;
	Pixmap		pixmask;
	Cursor		xcursor;
	} CURSORALLOC;

