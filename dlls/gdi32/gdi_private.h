/*
 * GDI definitions
 *
 * Copyright 1993 Alexandre Julliard
 * Copyright 2021 Jacek Caban for CodeWeavers
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_GDI_PRIVATE_H
#define __WINE_GDI_PRIVATE_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ntgdi.h"

void set_gdi_client_ptr( HGDIOBJ handle, void *ptr ) DECLSPEC_HIDDEN;
void *get_gdi_client_ptr( HGDIOBJ handle, WORD type ) DECLSPEC_HIDDEN;

static inline BOOL is_meta_dc( HDC hdc )
{
    unsigned int handle = HandleToULong( hdc );
    return (handle & NTGDI_HANDLE_TYPE_MASK) >> NTGDI_HANDLE_TYPE_SHIFT == NTGDI_OBJ_METADC;
}

extern BOOL METADC_Arc( HDC hdc, INT left, INT top, INT right, INT bottom,
                        INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL METADC_Chord( HDC hdc, INT left, INT top, INT right, INT bottom, INT xstart,
                          INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL METADC_LineTo( HDC hdc, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL METADC_MoveTo( HDC hdc, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL METADC_Pie( HDC hdc, INT left, INT top, INT right, INT bottom,
                        INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;

#endif /* __WINE_GDI_PRIVATE_H */
