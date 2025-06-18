/*
 * SetupAPI DiskSpace functions
 *
 * Copyright 2004 CodeWeavers (Aric Stewart)
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
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "setupapi.h"
#include "wine/debug.h"

#include "setupapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

struct file
{
    WCHAR *path;
    LONGLONG size;
    UINT op;
};

struct disk_space_list
{
    unsigned int flags;
    struct file *files;
    size_t count, capacity;
};

static bool ascii_isalpha(WCHAR c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

/***********************************************************************
 *		SetupCreateDiskSpaceListW  (SETUPAPI.@)
 */
HDSKSPC WINAPI SetupCreateDiskSpaceListW(PVOID Reserved1, DWORD Reserved2, UINT Flags)
{
    struct disk_space_list *list;

    TRACE("(%p, %lu, 0x%08x)\n", Reserved1, Reserved2, Flags);

    if (Reserved1 || Reserved2 || Flags & ~SPDSL_IGNORE_DISK)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    list = calloc(1, sizeof(*list));
    list->flags = Flags;
    return list;
}


/***********************************************************************
 *		SetupCreateDiskSpaceListA  (SETUPAPI.@)
 */
HDSKSPC WINAPI SetupCreateDiskSpaceListA(PVOID Reserved1, DWORD Reserved2, UINT Flags)
{
    return SetupCreateDiskSpaceListW( Reserved1, Reserved2, Flags );
}

/***********************************************************************
 *		SetupDuplicateDiskSpaceListW  (SETUPAPI.@)
 */
HDSKSPC WINAPI SetupDuplicateDiskSpaceListW(HDSKSPC handle, PVOID Reserved1, DWORD Reserved2, UINT Flags)
{
    struct disk_space_list *copy, *list = handle;

    if (Reserved1 || Reserved2 || Flags)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (!handle)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }

    if (!(copy = malloc(sizeof(*copy))))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    copy->flags = list->flags;
    copy->count = list->count;
    copy->capacity = 0;
    copy->files = NULL;
    array_reserve((void **)&copy->files, &copy->capacity, copy->count, sizeof(*copy->files));
    for (size_t i = 0; i < list->count; ++i)
    {
        copy->files[i].path = wcsdup(list->files[i].path);
        copy->files[i].op = list->files[i].op;
        copy->files[i].size = list->files[i].size;
    }

    return copy;
}

/***********************************************************************
 *		SetupDuplicateDiskSpaceListA  (SETUPAPI.@)
 */
HDSKSPC WINAPI SetupDuplicateDiskSpaceListA(HDSKSPC DiskSpace, PVOID Reserved1, DWORD Reserved2, UINT Flags)
{
    return SetupDuplicateDiskSpaceListW(DiskSpace, Reserved1, Reserved2, Flags);
}

/***********************************************************************
 *		SetupAddInstallSectionToDiskSpaceListA  (SETUPAPI.@)
 */
BOOL WINAPI SetupAddInstallSectionToDiskSpaceListA(HDSKSPC DiskSpace, 
                        HINF InfHandle, HINF LayoutInfHandle, 
                        LPCSTR SectionName, PVOID Reserved1, UINT Reserved2)
{
    FIXME ("Stub\n");
    return TRUE;
}

/***********************************************************************
*		SetupQuerySpaceRequiredOnDriveW  (SETUPAPI.@)
*/
BOOL WINAPI SetupQuerySpaceRequiredOnDriveW(HDSKSPC handle,
        const WCHAR *drive, LONGLONG *ret_size, void *reserved1, UINT reserved2)
{
    struct disk_space_list *list = handle;
    bool has_files = false;
    LONGLONG size = 0;

    TRACE("handle %p, drive %s, ret_size %p, reserved1 %p, reserved2 %#x.\n",
            handle, debugstr_w(drive), ret_size, reserved1, reserved2);

    if (!handle)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!drive)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!ascii_isalpha(drive[0]) || drive[1] != ':' || drive[2])
    {
        SetLastError(ERROR_INVALID_DRIVE);
        return FALSE;
    }

    for (size_t i = 0; i < list->count; ++i)
    {
        if (towlower(drive[0]) == towlower(list->files[i].path[0]))
        {
            has_files = true;
            size += list->files[i].size;
        }
    }

    if (!has_files)
    {
        SetLastError(ERROR_INVALID_DRIVE);
        return FALSE;
    }

    *ret_size = size;
    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

/***********************************************************************
*		SetupQuerySpaceRequiredOnDriveA  (SETUPAPI.@)
*/
BOOL WINAPI SetupQuerySpaceRequiredOnDriveA(HDSKSPC DiskSpace,
                        LPCSTR DriveSpec, LONGLONG *SpaceRequired,
                        PVOID Reserved1, UINT Reserved2)
{
    DWORD len;
    LPWSTR DriveSpecW;
    BOOL ret;

    /* The parameter validation checks are in a different order from the
     * Unicode variant of SetupQuerySpaceRequiredOnDrive. */
    if (!DriveSpec)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!DiskSpace)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    len = MultiByteToWideChar(CP_ACP, 0, DriveSpec, -1, NULL, 0);

    DriveSpecW = malloc(len * sizeof(WCHAR));
    if (!DriveSpecW)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    MultiByteToWideChar(CP_ACP, 0, DriveSpec, -1, DriveSpecW, len);

    ret = SetupQuerySpaceRequiredOnDriveW(DiskSpace, DriveSpecW, SpaceRequired,
                                          Reserved1, Reserved2);

    free(DriveSpecW);

    return ret;
}

/***********************************************************************
*		SetupDestroyDiskSpaceList  (SETUPAPI.@)
*/
BOOL WINAPI SetupDestroyDiskSpaceList(HDSKSPC handle)
{
    struct disk_space_list *list = handle;

    for (size_t i = 0; i < list->count; ++i)
        free(list->files[i].path);
    free(list->files);
    free(list);
    return TRUE;
}

static LONGLONG get_aligned_size(LONGLONG size)
{
    return (size + 4095) & ~4095;
}

/***********************************************************************
*		SetupAddToDiskSpaceListA  (SETUPAPI.@)
*/
BOOL WINAPI SetupAddToDiskSpaceListA(HDSKSPC handle, const char *file,
        LONGLONG size, UINT op, void *reserved1, UINT reserved2)
{
    WCHAR *fileW = strdupAtoW(file);
    BOOL ret = SetupAddToDiskSpaceListW(handle, fileW, size, op, reserved1, reserved2);
    free(fileW);
    return ret;
}

/***********************************************************************
*		SetupAddToDiskSpaceListW  (SETUPAPI.@)
*/
BOOL WINAPI SetupAddToDiskSpaceListW(HDSKSPC handle, const WCHAR *file,
        LONGLONG size, UINT op, void *reserved1, UINT reserved2)
{
    struct disk_space_list *list = handle;
    WIN32_FILE_ATTRIBUTE_DATA attr;
    WCHAR *full_path;
    DWORD len;

    TRACE("handle %p, file %s, size %I64d, op %#x, reserved1 %p, reserved2 %#x.\n",
            handle, debugstr_w(file), size, op, reserved1, reserved2);

    size = get_aligned_size(size);

    if (op != FILEOP_COPY && op != FILEOP_DELETE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (op == FILEOP_DELETE)
        size = 0;

    if (!(len = GetFullPathNameW(file, 0, NULL, NULL)))
    {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }
    full_path = malloc(len * sizeof(WCHAR));
    GetFullPathNameW(file, len, full_path, NULL);

    if (!ascii_isalpha(full_path[0]) || full_path[1] != ':' || full_path[len - 2] == '\\')
    {
        free(full_path);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!(list->flags & SPDSL_IGNORE_DISK)
            && GetFileAttributesExW(full_path, GetFileExInfoStandard, &attr)
            && !(attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        size -= get_aligned_size(((LONGLONG)attr.nFileSizeHigh << 32) | attr.nFileSizeLow);

    for (size_t i = 0; i < list->count; ++i)
    {
        if (!wcscmp(full_path, list->files[i].path))
        {
            if (!(op == FILEOP_DELETE && list->files[i].op == FILEOP_COPY))
            {
                list->files[i].op = op;
                list->files[i].size = size;
            }
            free(full_path);
            SetLastError(ERROR_SUCCESS);
            return TRUE;
        }
    }

    array_reserve((void **)&list->files, &list->capacity, list->count + 1, sizeof(*list->files));
    list->files[list->count].path = full_path;
    list->files[list->count].op = op;
    list->files[list->count].size = size;
    ++list->count;
    SetLastError(ERROR_SUCCESS);
    return TRUE;
}
