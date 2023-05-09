/*
 * Copyright 2022 Piotr Caban for CodeWeavers
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

#include "ntuser.h"
#include "wine/unixlib.h"

/* escapes */
#define PSDRV_GET_GLYPH_NAME 0x10000
#define PSDRV_GET_BUILTIN_FONT_INFO 0x10001

struct font_info
{
    char font_name[LF_FACESIZE];
    SIZE size;
    int escapement;
};

/* devmode */
struct input_slot
{
    int win_bin;
};

struct resolution
{
    int x;
    int y;
};

struct page_size
{
    WCHAR name[CCHFORMNAME];
    struct
    {
        float left;
        float bottom;
        float right;
        float top;
    } imageable_area;
    struct
    {
        float x;
        float y;
    } paper_dimension;
    short win_page;
};

struct font_sub
{
    WCHAR name[LF_FACESIZE];
    WCHAR sub[LF_FACESIZE];
};

/* Unix calls */
enum wineps_funcs
{
    unix_init_dc,
    unix_free_printer_info,
    unix_funcs_count,
};

struct init_dc_params
{
    const struct gdi_dc_funcs *funcs;
    PRINTERINFO *pi;

    const WCHAR *name;
};
