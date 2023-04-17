/*
 * Copyright 2023 Zhiyi Zhang for CodeWeavers
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

#include <windef.h>

#define MAXSTRING 8192

#define STRING_USAGE            101
#define STRING_BAD_COMMAND_LINE 102
#define STRING_CANNOT_OPEN      103
#define STRING_IGNORED          104

struct findstr_string
{
    const WCHAR *string;
    struct findstr_string *next;
};

struct findstr_file
{
    FILE *file;
    struct findstr_file *next;
};
