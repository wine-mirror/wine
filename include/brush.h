/*
 * GDI brush definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_BRUSH_H
#define __WINE_BRUSH_H

#include "gdi.h"

#ifndef WINELIB
#pragma pack(1)
#endif

  /* GDI logical brush object */
typedef struct
{
    GDIOBJHDR   header;
    LOGBRUSH16  logbrush WINE_PACKED;
} BRUSHOBJ;

#ifndef WINELIB
#pragma pack(4)
#endif

extern BOOL BRUSH_Init(void);
extern int BRUSH_GetObject( BRUSHOBJ * brush, int count, LPSTR buffer );
extern BOOL BRUSH_DeleteObject( HBRUSH hbrush, BRUSHOBJ * brush );
extern HBRUSH BRUSH_SelectObject( DC * dc, HBRUSH hbrush, BRUSHOBJ * brush );

#endif  /* __WINE_BRUSH_H */
