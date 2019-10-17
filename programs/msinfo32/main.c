/*
 * Copyright 2016 Austin English
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

#include <shlwapi.h>
#include <shellapi.h>

#include "wine/debug.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(msinfo);

int __cdecl wmain(int argc, WCHAR *argv[])
{
    int i;
    WCHAR system_info[64];

    WINE_FIXME("stub:");
    for (i = 0; i < argc; i++)
        WINE_FIXME(" %s", wine_dbgstr_w(argv[i]));
    WINE_FIXME("\n");

    LoadStringW(GetModuleHandleW(NULL), STRING_SYSTEM_INFO, system_info, ARRAY_SIZE(system_info));
    ShellAboutW(NULL, system_info, NULL, NULL);

    return 0;
}
