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
    LOGBRUSH16  logbrush;
} BRUSHOBJ;

extern BOOL BRUSH_Init(void);
extern int BRUSH_GetObject( BRUSHOBJ * brush, int count, LPSTR buffer );
extern BOOL32 BRUSH_DeleteObject( HBRUSH16 hbrush, BRUSHOBJ * brush );
extern HBRUSH16 BRUSH_SelectObject(DC * dc, HBRUSH16 hbrush, BRUSHOBJ * brush);

#endif  /* __WINE_BRUSH_H */
