/* Structure definitions for Win32 -- used only internally */
#ifndef _STRUCT32_H
#define _STRUCT32_H

typedef struct tagRECT32
{
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
} RECT32;

void USER32_RECT32to16(const RECT32*,RECT*);
void USER32_RECT16to32(const RECT*,RECT32*);

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
  
void PARAM32_POINT32to16(const POINT32*,POINT*);
void PARAM32_POINT16to32(const POINT*,POINT32*);
void PARAM32_SIZE16to32(const SIZE* p16, SIZE32* p32);

typedef struct {
	DWORD style;
	DWORD dwExtendedStyle;
	WORD noOfItems;
	short x;
	short y;
	WORD cx;
	WORD cy;
} DLGTEMPLATE32;

typedef struct {
	DWORD style;
	DWORD dwExtendedStyle;
	short x;
	short y;
	short cx;
	short cy;
	WORD id;
} DLGITEMTEMPLATE32;

#define CW_USEDEFAULT32	0x80000000

#endif
