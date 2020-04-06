/*
 * Win32 libwine stubs
 *
 * Copyright 2019 Alexandre Julliard
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

#ifdef _WIN32

#include "config.h"

#include <stdlib.h>
#include "windef.h"

int __wine_main_argc = 0;
char **__wine_main_argv = NULL;
WCHAR **__wine_main_wargv = NULL;

const char *wine_get_config_dir(void)
{
    const char *ret = getenv( "WINEPREFIX" );
    if (!ret) ret = getenv( "USERPROFILE" );
    return ret;
}

const char *wine_get_data_dir(void)
{
    return NULL;
}

const char *wine_get_build_dir(void)
{
    return NULL;
}

const char *wine_get_user_name(void)
{
    return getenv( "USERNAME" );
}

const char *wine_dll_enum_load_path( unsigned int index )
{
    return NULL;
}

#endif  /* _WIN32 */
