/*
 * GDI brush definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_BRUSH_H
#define __WINE_BRUSH_H

#include "gdi.h"

  /* GDI logical brush object */
typedef struct
{
    GDIOBJHDR   header;
    LOGBRUSH32  logbrush;
} BRUSHOBJ;

extern BOOL32 BRUSH_Init(void);
extern INT16 BRUSH_GetObject16( BRUSHOBJ * brush, INT16 count, LPSTR buffer );
extern INT32 BRUSH_GetObject32( BRUSHOBJ * brush, INT32 count, LPSTR buffer );
extern BOOL32 BRUSH_DeleteObject( HBRUSH16 hbrush, BRUSHOBJ * brush );
extern HBRUSH32 BRUSH_SelectObject(DC * dc, HBRUSH32 hbrush, BRUSHOBJ * brush);

#endif  /* __WINE_BRUSH_H */
