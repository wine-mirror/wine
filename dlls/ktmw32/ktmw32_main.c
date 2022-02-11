/*
 * Copyright 2010 Vincent Povirk for CodeWeavers
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

WINE_DEFAULT_DEBUG_CHANNEL(ktmw32);


/***********************************************************************
 * CommitTransaction (ktmw32.@)
 */
BOOL WINAPI CommitTransaction(HANDLE transaction)
{
    FIXME("(%p): stub\n", transaction);
    return TRUE;
}

/***********************************************************************
 * CreateTransaction (ktmw32.@)
 */
HANDLE WINAPI CreateTransaction(LPSECURITY_ATTRIBUTES pattr, LPGUID pguid, DWORD options,
                                DWORD level, DWORD flags, DWORD timeout, LPWSTR description)
{

    FIXME("(%p %p 0x%lx 0x%lx 0x%lx, %lu, %s): stub\n",
            pattr, pguid, options, level, flags, timeout, debugstr_w(description));

    return (HANDLE) 1;
}

/***********************************************************************
 * Rollback Transaction (ktmw32.@)
 */
BOOL WINAPI RollbackTransaction(HANDLE transaction)
{
    FIXME("stub: %p\n", transaction);
    SetLastError(ERROR_ACCESS_DENIED);
    return FALSE;
}
