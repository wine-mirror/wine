/*
 * USER DCE definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef DCE_H
#define DCE_H

#include "windows.h"

typedef enum
{
    DCE_CACHE_DC,   /* This is a cached DC (allocated by USER) */
    DCE_CLASS_DC,   /* This is a class DC (style CS_CLASSDC) */
    DCE_WINDOW_DC   /* This is a window DC (style CS_OWNDC) */
} DCE_TYPE;


typedef struct tagDCE
{
    HANDLE     hNext;
    HWND       hwndCurrent;
    HDC        hdc;
    DCE_TYPE   type;
    BOOL       inUse;
    WORD       xOrigin;
    WORD       yOrigin;
} DCE;


extern HANDLE DCE_AllocDCE( DCE_TYPE type );
extern void DCE_FreeDCE( HANDLE hdce );

#endif  /* DCE_H */
