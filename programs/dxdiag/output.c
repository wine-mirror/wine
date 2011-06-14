/*
 * DxDiag file information output
 *
 * Copyright 2011 Andrew Nguyen
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

#include <assert.h>

#include "wine/debug.h"

#include "dxdiag_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxdiag);

static struct output_backend
{
    const WCHAR filename_ext[5];
} output_backends[] =
{
    /* OUTPUT_TEXT */
    {
        {'.','t','x','t',0},
    },
    /* OUTPUT_XML */
    {
        {'.','x','m','l',0},
    },
};

const WCHAR *get_output_extension(enum output_type type)
{
    assert(type > OUTPUT_NONE && type <= sizeof(output_backends)/sizeof(output_backends[0]));

    return output_backends[type - 1].filename_ext;
}

BOOL output_dxdiag_information(const WCHAR *filename, enum output_type type)
{
    WINE_FIXME("File information output is not implemented\n");
    return FALSE;
}
