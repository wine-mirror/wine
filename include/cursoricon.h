/*
 * Cursor and icon definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_CURSORICON_H
#define __WINE_CURSORICON_H

#include <X11/Xlib.h>
#include "windows.h"

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

#pragma pack(1)

typedef struct
{
    WORD                idReserved;
    WORD                idType;
    WORD                idCount;
    CURSORICONDIRENTRY  idEntries[1] WINE_PACKED;
} CURSORICONDIR;

#pragma pack(4)

extern Cursor CURSORICON_XCursor;  /* Current X cursor */

#endif /* __WINE_CURSORICON_H */
