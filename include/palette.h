/*
 * GDI palette definitions
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

#ifndef __WINE_PALETTE_H
#define __WINE_PALETTE_H

#include <gdi.h>

#define NB_RESERVED_COLORS     20   /* number of fixed colors in system palette */

#define PC_SYS_USED            0x80 /* palentry is used (both system and logical) */
#define PC_SYS_RESERVED        0x40 /* system palentry is not to be mapped to */
#define PC_SYS_MAPPED          0x10 /* logical palentry is a direct alias for system palentry */

  /* GDI logical palette object */
typedef struct tagPALETTEOBJ
{
    GDIOBJHDR                    header;
    int                          *mapping;
    LOGPALETTE                   logpalette; /* _MUST_ be the last field */
} PALETTEOBJ;

extern HPALETTE PALETTE_Init(void);

#endif /* __WINE_PALETTE_H */
