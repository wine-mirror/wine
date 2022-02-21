/*
 * Copyright 2017 Louis Lenders
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
#include "virtdisk.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(virtdisk);

DWORD WINAPI GetStorageDependencyInformation(HANDLE obj, GET_STORAGE_DEPENDENCY_FLAG flags, ULONG size, STORAGE_DEPENDENCY_INFO *info, ULONG *used)
{
    ULONG temp_size = sizeof(STORAGE_DEPENDENCY_INFO);

    FIXME("(%p, 0x%x, %lu, %p, %p): stub\n", obj, flags, size, info, used);

    if (used) *used = temp_size;

    if (!info || !size)
        return ERROR_INVALID_PARAMETER;

    if (size < temp_size)
        return ERROR_INSUFFICIENT_BUFFER;

    info->NumberEntries = 0;

    return ERROR_SUCCESS;
}

DWORD WINAPI OpenVirtualDisk(VIRTUAL_STORAGE_TYPE *type, const WCHAR *path, VIRTUAL_DISK_ACCESS_MASK mask, OPEN_VIRTUAL_DISK_FLAG flags,
                             OPEN_VIRTUAL_DISK_PARAMETERS *param, HANDLE *handle)
{
    FIXME("(%p, %s, %d, 0x%x, %p, %p): stub\n", type, wine_dbgstr_w(path), mask, flags, param, handle);

    if (!type || !path || (mask & ~VIRTUAL_DISK_ACCESS_ALL) || (flags & ~(OPEN_VIRTUAL_DISK_FLAG_NO_PARENTS | OPEN_VIRTUAL_DISK_FLAG_BLANK_FILE)) || !param)
        return ERROR_INVALID_PARAMETER;

    if (param->Version != OPEN_VIRTUAL_DISK_VERSION_1)
        return ERROR_INVALID_PARAMETER;

    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI DetachVirtualDisk(HANDLE handle, DETACH_VIRTUAL_DISK_FLAG flags, ULONG specific_flags)
{
    FIXME("(%p, 0x%x, %ld): stub\n", handle, flags, specific_flags);
    return ERROR_INVALID_PARAMETER;
}
