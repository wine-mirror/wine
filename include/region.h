/*
 * GDI region definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_REGION_H
#define __WINE_REGION_H

#include "gdi.h"

  /* GDI logical region object */
typedef struct
{
    GDIOBJHDR   header;
    Region      xrgn;
} RGNOBJ;


extern BOOL REGION_DeleteObject( HRGN hrgn, RGNOBJ * obj );
extern BOOL REGION_FrameRgn( HRGN dest, HRGN src, INT32 x, INT32 y );

#endif  /* __WINE_REGION_H */
