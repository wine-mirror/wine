/*
 * GDI pen definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_PEN_H
#define __WINE_PEN_H

#include "gdi.h"

  /* GDI logical pen object */
typedef struct
{
    GDIOBJHDR   header;
    LOGPEN16    logpen;
} PENOBJ;

extern int PEN_GetObject( PENOBJ * pen, int count, LPSTR buffer );
extern HPEN16 PEN_SelectObject( DC * dc, HPEN16 hpen, PENOBJ * pen );

#endif  /* __WINE_PEN_H */
