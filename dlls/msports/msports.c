/*
 * Microsoft Serial Controller Driver API implementation
 *
 * Copyright 2025 Stefan Brüns
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

#include "windef.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msports);

DECLARE_HANDLE(HCOMDB);

LONG WINAPI ComDBClose(HCOMDB hComDB)
{
    FIXME("stub\n");
    return ERROR_INVALID_PARAMETER;
}

LONG WINAPI ComDBOpen(HCOMDB *phComDB)
{
    FIXME("stub\n");
    *phComDB = INVALID_HANDLE_VALUE;
    return ERROR_ACCESS_DENIED;
}
