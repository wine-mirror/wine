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


extern BOOL16 REGION_DeleteObject( HRGN32 hrgn, RGNOBJ * obj );
extern BOOL16 REGION_FrameRgn( HRGN32 dest, HRGN32 src, INT32 x, INT32 y );

#endif  /* __WINE_REGION_H */
