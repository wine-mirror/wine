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
    LOGPEN32    logpen;
} PENOBJ;

extern INT16 PEN_GetObject16( PENOBJ * pen, INT16 count, LPSTR buffer );
extern INT32 PEN_GetObject32( PENOBJ * pen, INT32 count, LPSTR buffer );

#endif  /* __WINE_PEN_H */
