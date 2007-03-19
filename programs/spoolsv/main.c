/*
 * Copyright 2007 Jacek Caban for CodeWeavers
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

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(spoolsv);

static void WINAPI serv_main(DWORD argc, LPWSTR *argv)
{
    WINE_FIXME("(%d %p)\n", argc, argv);
}

int main(int argc, char **argv)
{
    static WCHAR wszSPOOLER[] = {'S','P','O','O','L','E','R',0};
    static const SERVICE_TABLE_ENTRYW servtbl[] = {
        {wszSPOOLER, serv_main},
        {NULL, NULL}
    };

    WINE_TRACE("(%d %p)\n", argc, argv);

    StartServiceCtrlDispatcherW(servtbl);
    return 0;
}
