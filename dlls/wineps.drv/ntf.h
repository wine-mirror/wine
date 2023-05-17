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

struct ntf_header
{
    int signature;
    int driver_type;
    int version;
    int unk[5];
    int glyph_set_count;
    int glyph_set_off;
    int font_mtx_count;
    int font_mtx_off;
};

struct list_entry
{
    int name_off;
    int hash;
    int size;
    int off;
    int unk[4];
};

#define GLYPH_SET_OMIT_CP 1
struct glyph_set
{
    int size;
    int version;
    int flags;
    int name_off;
    int glyph_count;
    int run_count;
    int run_off;
    int cp_count;
    int cp_off;
    int glyph_set_off;
    int unk[2];
};

struct code_page
{
    int cp;
    int win_char_set;
    int name_off;
    int unk[2];
};

struct font_mtx
{
    int size;
    int version;
    int flags;
    int name_off;
    int display_off;
    int font_version;
    int glyph_set_name_off;
    int glyph_count;
    int metrics_off;
    int unk;
    int width_count;
    int width_off;
    int def_width;
    /* TODO: there are more fields */
};

struct width_range
{
    short first;
    short count;
    int width;
};
