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
    LOGBRUSH  logbrush;
} BRUSHOBJ;

#define NB_HATCH_STYLES  6

extern INT16 BRUSH_GetObject16( BRUSHOBJ * brush, INT16 count, LPSTR buffer );
extern INT BRUSH_GetObject( BRUSHOBJ * brush, INT count, LPSTR buffer );
extern BOOL BRUSH_DeleteObject( HBRUSH16 hbrush, BRUSHOBJ * brush );

#endif  /* __WINE_BRUSH_H */
