/*
 * Desktop window definitions.
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef DESKTOP_H
#define DESKTOP_H

#include "windows.h"

typedef struct
{
    HBRUSH   hbrushPattern;
    HBITMAP  hbitmapWallPaper;
    SIZE     bitmapSize;
    BOOL     fTileWallPaper;
} DESKTOPINFO;

extern BOOL DESKTOP_SetPattern(char *pattern );

#endif  /* DESKTOP_H */
