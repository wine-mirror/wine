/*
 * USER DCE definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef DCE_H
#define DCE_H

#include "windows.h"

typedef struct tagDCE
{
    HANDLE     hNext;
    HWND       hwndCurrent;
    HDC        hdc;
    BYTE       flags;
    BOOL       inUse;
    WORD       xOrigin;
    WORD       yOrigin;
} DCE;


#endif  /* DCE_H */
