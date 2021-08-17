/*
 * Metafile driver definitions
 *
 * Copyright 1996 Alexandre Julliard
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

#ifndef __WINE_METAFILEDRV_H
#define __WINE_METAFILEDRV_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ntgdi_private.h"
#include "gdi_private.h"

/* Metafile driver physical DC */

struct metadc
{
    HDC        hdc;
    METAHEADER  *mh;           /* Pointer to metafile header */
    UINT       handles_size, cur_handles;
    HGDIOBJ   *handles;
    HANDLE     hFile;          /* Handle for disk based MetaFile */
    HPEN       pen;
    HBRUSH     brush;
    HFONT      font;
};

#define HANDLE_LIST_INC 20


extern UINT metadc_add_handle( struct metadc *metadc, HGDIOBJ obj ) DECLSPEC_HIDDEN;
extern BOOL metadc_remove_handle( struct metadc *metadc, UINT index ) DECLSPEC_HIDDEN;
extern INT16 metadc_create_brush( struct metadc *metadc, HBRUSH brush ) DECLSPEC_HIDDEN;

extern struct metadc *get_metadc_ptr( HDC hdc ) DECLSPEC_HIDDEN;
extern BOOL metadc_param0( HDC hdc, short func ) DECLSPEC_HIDDEN;
extern BOOL metadc_param1( HDC hdc, short func, short param ) DECLSPEC_HIDDEN;
extern BOOL metadc_param2( HDC hdc, short func, short param1, short param2 ) DECLSPEC_HIDDEN;
extern BOOL metadc_param4( HDC hdc, short func, short param1, short param2,
                           short param3, short param4 ) DECLSPEC_HIDDEN;
extern BOOL metadc_param5( HDC hdc, short func, short param1, short param2, short param3,
                           short param4, short param5 ) DECLSPEC_HIDDEN;
extern BOOL metadc_param6( HDC hdc, short func, short param1, short param2, short param3,
                           short param4, short param5, short param6 ) DECLSPEC_HIDDEN;
extern BOOL metadc_param8( HDC hdc, short func, short param1, short param2,
                           short param3, short param4, short param5, short param6,
                           short param7, short param8 ) DECLSPEC_HIDDEN;
extern BOOL metadc_record( HDC hdc, METARECORD *mr, DWORD rlen ) DECLSPEC_HIDDEN;
extern BOOL metadc_write_record( struct metadc *metadc, METARECORD *mr, DWORD rlen ) DECLSPEC_HIDDEN;

#endif  /* __WINE_METAFILEDRV_H */
