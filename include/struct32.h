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

void PARAM32_POINT32to16(const POINT32*,POINT*);
void PARAM32_POINT16to32(const POINT*,POINT32*);

#endif
