/*
 * GDI pen definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_PEN_H
#define __WINE_PEN_H

#include "gdi.h"

#pragma pack(1)

  /* GDI logical pen object */
typedef struct
{
    GDIOBJHDR   header;
    LOGPEN      logpen WINE_PACKED;
} PENOBJ;

#pragma pack(4)

extern int PEN_GetObject( PENOBJ * pen, int count, LPSTR buffer );
extern HPEN PEN_SelectObject( DC * dc, HPEN hpen, PENOBJ * pen );

#endif  /* __WINE_PEN_H */
