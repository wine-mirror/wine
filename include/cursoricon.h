/*
 * Cursor and icon definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_CURSORICON_H
#define __WINE_CURSORICON_H

#include "wingdi.h"

#pragma pack(1)

typedef struct
{
    BYTE   bWidth;
    BYTE   bHeight;
    BYTE   bColorCount;
    BYTE   bReserved;
    WORD   wPlanes;
    WORD   wBitCount;
    DWORD  dwBytesInRes;
    WORD   wResId;
} ICONDIRENTRY;

typedef struct
{
    WORD   wWidth;
    WORD   wHeight;
    WORD   wPlanes;
    WORD   wBitCount;
    DWORD  dwBytesInRes;
    WORD   wResId;
} CURSORDIRENTRY;

typedef union
{
    ICONDIRENTRY    icon;
    CURSORDIRENTRY  cursor;
} CURSORICONDIRENTRY;

typedef struct
{
    WORD                idReserved;
    WORD                idType;
    WORD                idCount;
    CURSORICONDIRENTRY  idEntries[1] WINE_PACKED;
} CURSORICONDIR;

typedef struct {
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD xHotspot;
    WORD yHotspot;
    DWORD dwDIBSize;
    DWORD dwDIBOffset;
} CURSORICONFILEDIRENTRY;

typedef struct
{
    WORD                idReserved;
    WORD                idType;
    WORD                idCount;
    CURSORICONFILEDIRENTRY  idEntries[1];
} CURSORICONFILEDIR;


#pragma pack(4)

extern HCURSOR16 CURSORICON_IconToCursor( HICON16 hIcon,
                                          BOOL32 bSemiTransparent );

extern HGLOBAL32 CURSORICON_Load32( HINSTANCE32 hInstance, LPCWSTR name,
                                    int width, int height, int colors,
                                    BOOL32 fCursor, UINT32 loadflags);
				    
#endif /* __WINE_CURSORICON_H */
