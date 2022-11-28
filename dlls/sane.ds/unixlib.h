/*
 * Unix library interface for SANE
 *
 * Copyright 2000 Corel Corporation
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

#include <stdarg.h>
#include "windef.h"
#include "winternl.h"
#include "wine/unixlib.h"
#include "twain.h"

struct frame_parameters
{
    enum { FMT_GRAY, FMT_RGB, FMT_OTHER } format;
    BOOL last_frame;
    int  bytes_per_line;
    int  pixels_per_line;
    int  lines;
    int  depth;
};

struct option_descriptor
{
    int optno;
    int size;
    int is_active;
    enum { TYPE_BOOL, TYPE_INT, TYPE_FIXED, TYPE_STRING, TYPE_BUTTON, TYPE_GROUP } type;
    enum { UNIT_NONE, UNIT_PIXEL, UNIT_BIT, UNIT_MM, UNIT_DPI, UNIT_PERCENT, UNIT_MICROSECOND } unit;
    enum { CONSTRAINT_NONE, CONSTRAINT_RANGE, CONSTRAINT_WORD_LIST, CONSTRAINT_STRING_LIST } constraint_type;

    WCHAR title[256];

    union
    {
        WCHAR strings[256];
        int word_list[256];	/* first element is list-length */
        struct { int min, max, quant; } range;
    } constraint;
};

struct read_data_params
{
    void        *buffer;
    int          len;
    int         *retlen;
};

struct option_get_value_params
{
    int          optno;
    void        *val;
};

struct option_set_value_params
{
    int          optno;
    void        *val;
    BOOL        *reload;
};

struct option_find_descriptor_params
{
    const char *name;
    int         type;
    struct option_descriptor *descr;
};

enum sane_funcs
{
    unix_process_attach,
    unix_process_detach,
    unix_get_identity,
    unix_open_ds,
    unix_close_ds,
    unix_start_device,
    unix_cancel_device,
    unix_read_data,
    unix_get_params,
    unix_option_get_value,
    unix_option_set_value,
    unix_option_get_descriptor,
    unix_option_find_descriptor,
};

#define SANE_CALL( func, params ) WINE_UNIX_CALL( unix_ ## func, params )
