/*
 * GDI pen definitions
 *
 * Copyright 1994 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_PEN_H
#define __WINE_PEN_H

#include "gdi.h"

  /* GDI logical pen object */
typedef struct
{
    GDIOBJHDR   header;
    LOGPEN    logpen;
} PENOBJ;

extern INT16 PEN_GetObject16( PENOBJ * pen, INT16 count, LPSTR buffer );
extern INT PEN_GetObject( PENOBJ * pen, INT count, LPSTR buffer );

#endif  /* __WINE_PEN_H */
