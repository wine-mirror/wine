/*
 * Cursor and icon definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_CURSORICON_H
#define __WINE_CURSORICON_H

#include <X11/Xlib.h>
#include "windows.h"

#ifndef WINELIB
#pragma pack(1)
#endif

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

#ifndef WINELIB
#pragma pack(4)
#endif

extern Cursor CURSORICON_XCursor;  /* Current X cursor */

#endif /* __WINE_CURSORICON_H */
