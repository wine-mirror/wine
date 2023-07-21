/*
 * Copyright 2023 Dmitry Timoshkov
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
#include "winbase.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(runas);

int __cdecl wmain(int argc, WCHAR *argv[])
{
    WCHAR *user, *domain;
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    BOOL ret;

    if (argc < 3 || wcsncmp(argv[1], L"/user:", 5) != 0)
    {
        printf("Syntax: runas.exe /user:user@domain.com program.exe\n");
        return -1;
    }

    user = argv[1] + 6;
    domain = wcschr(user, '@');
    if (!*user || !domain || !*domain)
    {
        ERR("Invalid user/domain specified\n");
        return -1;
    }

    *domain++ = 0;
    TRACE("Running as %s@%s\n", debugstr_w(user), debugstr_w(domain));

    ret = CreateProcessWithLogonW(user, domain, NULL, 0, NULL, argv[2], 0, NULL, NULL, &si, &pi);
    if (!ret) ERR("CreateProcessWithLogon error %lu\n", GetLastError());

    return 0;
}
