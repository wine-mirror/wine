/*
 * GDI region definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_REGION_H
#define __WINE_REGION_H

#include "gdi.h"

typedef struct
{
    WORD        type;
    RECT        box;
    Pixmap      pixmap;
    Region      xrgn;
} REGION;

  /* GDI logical region object */
typedef struct
{
    GDIOBJHDR   header;
    REGION      region;
} RGNOBJ;


extern BOOL REGION_Init(void);
extern BOOL REGION_DeleteObject( HRGN hrgn, RGNOBJ * obj );

#endif  /* __WINE_REGION_H */
