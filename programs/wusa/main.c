/*
 * Copyright 2012 Austin English
 * Copyright 2015 Michael Müller
 * Copyright 2015 Sebastian Lackner
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

#include <windows.h>
#include <fcntl.h>
#include <fdi.h>
#include <shlwapi.h>

#include "wine/debug.h"
#include "wine/list.h"
#include "wusa.h"

WINE_DEFAULT_DEBUG_CHANNEL(wusa);

struct installer_tempdir
{
    struct list entry;
    WCHAR *path;
};

struct installer_state
{
    BOOL norestart;
    BOOL quiet;
    struct list tempdirs;
    struct list assemblies;
};

static void * CDECL cabinet_alloc(ULONG cb)
{
    return heap_alloc(cb);
}

static void CDECL cabinet_free(void *pv)
{
    heap_free(pv);
}

static INT_PTR CDECL cabinet_open(char *pszFile, int oflag, int pmode)
{
    DWORD dwAccess = 0;
    DWORD dwShareMode = 0;
    DWORD dwCreateDisposition = OPEN_EXISTING;

    switch (oflag & _O_ACCMODE)
    {
    case _O_RDONLY:
        dwAccess = GENERIC_READ;
        dwShareMode = FILE_SHARE_READ | FILE_SHARE_DELETE;
        break;
    case _O_WRONLY:
        dwAccess = GENERIC_WRITE;
        dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        break;
    case _O_RDWR:
        dwAccess = GENERIC_READ | GENERIC_WRITE;
        dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        break;
    }

    if ((oflag & (_O_CREAT | _O_EXCL)) == (_O_CREAT | _O_EXCL))
        dwCreateDisposition = CREATE_NEW;
    else if (oflag & _O_CREAT)
        dwCreateDisposition = CREATE_ALWAYS;

    return (INT_PTR)CreateFileA(pszFile, dwAccess, dwShareMode, NULL, dwCreateDisposition, 0, NULL);
}

static UINT CDECL cabinet_read(INT_PTR hf, void *pv, UINT cb)
{
    HANDLE handle = (HANDLE)hf;
    DWORD read;

    if (ReadFile(handle, pv, cb, &read, NULL))
        return read;

    return 0;
}

static UINT CDECL cabinet_write(INT_PTR hf, void *pv, UINT cb)
{
    HANDLE handle = (HANDLE)hf;
    DWORD written;

    if (WriteFile(handle, pv, cb, &written, NULL))
        return written;

    return 0;
}

static int CDECL cabinet_close(INT_PTR hf)
{
    HANDLE handle = (HANDLE)hf;
    return CloseHandle(handle) ? 0 : -1;
}

static LONG CDECL cabinet_seek(INT_PTR hf, LONG dist, int seektype)
{
    HANDLE handle = (HANDLE)hf;
    /* flags are compatible and so are passed straight through */
    return SetFilePointer(handle, dist, NULL, seektype);
}

static WCHAR *path_combine(const WCHAR *path, const WCHAR *filename)
{
    WCHAR *result;
    DWORD length;

    if (!path || !filename) return NULL;
    length = lstrlenW(path) + lstrlenW(filename) + 2;
    if (!(result = heap_alloc(length * sizeof(WCHAR)))) return NULL;

    lstrcpyW(result, path);
    if (result[0] && result[lstrlenW(result) - 1] != '\\') lstrcatW(result, L"\\");
    lstrcatW(result, filename);
    return result;
}

static WCHAR *get_uncompressed_path(PFDINOTIFICATION pfdin)
{
    WCHAR *file = strdupAtoW(pfdin->psz1);
    WCHAR *path = path_combine(pfdin->pv, file);
    heap_free(file);
    return path;
}

static BOOL is_directory(const WCHAR *path)
{
    DWORD attrs = GetFileAttributesW(path);
    if (attrs == INVALID_FILE_ATTRIBUTES) return FALSE;
    return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

static BOOL create_directory(const WCHAR *path)
{
    if (is_directory(path)) return TRUE;
    if (CreateDirectoryW(path, NULL)) return TRUE;
    return (GetLastError() == ERROR_ALREADY_EXISTS);
}

static BOOL create_parent_directory(const WCHAR *filename)
{
    WCHAR *p, *path = strdupW(filename);
    BOOL ret = FALSE;

    if (!path) return FALSE;
    if (!PathRemoveFileSpecW(path)) goto done;
    if (is_directory(path))
    {
        ret = TRUE;
        goto done;
    }

    for (p = path; *p; p++)
    {
        if (*p != '\\') continue;
        *p = 0;
        if (!create_directory(path)) goto done;
        *p = '\\';
    }
    ret = create_directory(path);

done:
    heap_free(path);
    return ret;
}

static INT_PTR cabinet_copy_file(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin)
{
    HANDLE handle = INVALID_HANDLE_VALUE;
    WCHAR *file;
    DWORD attrs;

    if (!(file = get_uncompressed_path(pfdin)))
        return -1;

    TRACE("Extracting %s -> %s\n", debugstr_a(pfdin->psz1), debugstr_w(file));

    if (create_parent_directory(file))
    {
        attrs = pfdin->attribs;
        if (!attrs) attrs = FILE_ATTRIBUTE_NORMAL;
        handle = CreateFileW(file, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, attrs, NULL);
    }

    heap_free(file);
    return (handle != INVALID_HANDLE_VALUE) ? (INT_PTR)handle : -1;
}

static INT_PTR cabinet_close_file_info(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin)
{
    HANDLE handle = (HANDLE)pfdin->hf;
    CloseHandle(handle);
    return 1;
}

static INT_PTR CDECL cabinet_notify(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin)
{
    switch (fdint)
    {
    case fdintPARTIAL_FILE:
        FIXME("fdintPARTIAL_FILE not implemented\n");
        return 0;

    case fdintNEXT_CABINET:
        FIXME("fdintNEXT_CABINET not implemented\n");
        return 0;

    case fdintCOPY_FILE:
        return cabinet_copy_file(fdint, pfdin);

    case fdintCLOSE_FILE_INFO:
        return cabinet_close_file_info(fdint, pfdin);

    default:
        return 0;
    }
}

static BOOL extract_cabinet(const WCHAR *filename, const WCHAR *destination)
{
    char *filenameA = NULL;
    BOOL ret = FALSE;
    HFDI hfdi;
    ERF erf;

    hfdi = FDICreate(cabinet_alloc, cabinet_free, cabinet_open, cabinet_read,
                     cabinet_write, cabinet_close, cabinet_seek, 0, &erf);
    if (!hfdi) return FALSE;

    if ((filenameA = strdupWtoA(filename)))
    {
        ret = FDICopy(hfdi, filenameA, NULL, 0, cabinet_notify, NULL, (void *)destination);
        heap_free(filenameA);
    }

    FDIDestroy(hfdi);
    return ret;
}

static const WCHAR *create_temp_directory(struct installer_state *state)
{
    static UINT id;
    struct installer_tempdir *entry;
    WCHAR tmp[MAX_PATH];

    if (!GetTempPathW(ARRAY_SIZE(tmp), tmp)) return NULL;
    if (!(entry = heap_alloc(sizeof(*entry)))) return NULL;
    if (!(entry->path = heap_alloc((MAX_PATH + 20) * sizeof(WCHAR))))
    {
        heap_free(entry);
        return NULL;
    }
    for (;;)
    {
        if (!GetTempFileNameW(tmp, L"msu", ++id, entry->path))
        {
            heap_free(entry->path);
            heap_free(entry);
            return NULL;
        }
        if (CreateDirectoryW(entry->path, NULL)) break;
    }

    list_add_tail(&state->tempdirs, &entry->entry);
    return entry->path;
}

static BOOL delete_directory(const WCHAR *path)
{
    WIN32_FIND_DATAW data;
    WCHAR *full_path;
    HANDLE search;

    if (!(full_path = path_combine(path, L"*"))) return FALSE;
    search = FindFirstFileW(full_path, &data);
    heap_free(full_path);

    if (search != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!wcscmp(data.cFileName, L".")) continue;
            if (!wcscmp(data.cFileName, L"..")) continue;
            if (!(full_path = path_combine(path, data.cFileName))) continue;
            if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                delete_directory(full_path);
            else
                DeleteFileW(full_path);
            heap_free(full_path);
        }
        while (FindNextFileW(search, &data));
        FindClose(search);
    }

    return RemoveDirectoryW(path);
}

static void installer_cleanup(struct installer_state *state)
{
    struct installer_tempdir *tempdir, *tempdir2;
    struct assembly_entry *assembly, *assembly2;

    LIST_FOR_EACH_ENTRY_SAFE(tempdir, tempdir2, &state->tempdirs, struct installer_tempdir, entry)
    {
        list_remove(&tempdir->entry);
        delete_directory(tempdir->path);
        heap_free(tempdir->path);
        heap_free(tempdir);
    }
    LIST_FOR_EACH_ENTRY_SAFE(assembly, assembly2, &state->assemblies, struct assembly_entry, entry)
    {
        list_remove(&assembly->entry);
        free_assembly(assembly);
    }
}

static BOOL str_ends_with(const WCHAR *str, const WCHAR *suffix)
{
    DWORD str_len = lstrlenW(str), suffix_len = lstrlenW(suffix);
    if (suffix_len > str_len) return FALSE;
    return !wcsicmp(str + str_len - suffix_len, suffix);
}

static BOOL load_assemblies_from_cab(const WCHAR *filename, struct installer_state *state)
{
    struct assembly_entry *assembly;
    const WCHAR *temp_path;
    WIN32_FIND_DATAW data;
    HANDLE search;
    WCHAR *path;

    TRACE("Processing cab file %s\n", debugstr_w(filename));

    if (!(temp_path = create_temp_directory(state))) return FALSE;
    if (!extract_cabinet(filename, temp_path))
    {
        ERR("Failed to extract %s\n", debugstr_w(filename));
        return FALSE;
    }

    if (!(path = path_combine(temp_path, L"_manifest_.cix.xml"))) return FALSE;
    if (GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES)
    {
        FIXME("Cabinet uses proprietary msdelta file compression which is not (yet) supported\n");
        FIXME("Installation of msu file will most likely fail\n");
    }
    heap_free(path);

    if (!(path = path_combine(temp_path, L"*"))) return FALSE;
    search = FindFirstFileW(path, &data);
    heap_free(path);

    if (search != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            if (!str_ends_with(data.cFileName, L".manifest") &&
                !str_ends_with(data.cFileName, L".mum")) continue;
            if (!(path = path_combine(temp_path, data.cFileName))) continue;
            if ((assembly = load_manifest(path)))
                list_add_tail(&state->assemblies, &assembly->entry);
            heap_free(path);
        }
        while (FindNextFileW(search, &data));
        FindClose(search);
    }

    return TRUE;
}

static BOOL install_msu(const WCHAR *filename, struct installer_state *state)
{
    const WCHAR *temp_path;
    WIN32_FIND_DATAW data;
    HANDLE search;
    WCHAR *path;
    BOOL ret = FALSE;

    list_init(&state->tempdirs);
    list_init(&state->assemblies);
    CoInitialize(NULL);

    TRACE("Processing msu file %s\n", debugstr_w(filename));

    if (!(temp_path = create_temp_directory(state))) return FALSE;
    if (!extract_cabinet(filename, temp_path))
    {
        ERR("Failed to extract %s\n", debugstr_w(filename));
        goto done;
    }

    /* load all manifests from contained cabinet archives */
    if (!(path = path_combine(temp_path, L"*.cab"))) goto done;
    search = FindFirstFileW(path, &data);
    heap_free(path);

    if (search != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            if (!wcsicmp(data.cFileName, L"WSUSSCAN.cab")) continue;
            if (!(path = path_combine(temp_path, data.cFileName))) continue;
            if (!load_assemblies_from_cab(path, state))
                ERR("Failed to load all manifests from %s, ignoring\n", debugstr_w(path));
            heap_free(path);
        }
        while (FindNextFileW(search, &data));
        FindClose(search);
    }

    ret = TRUE;

done:
    installer_cleanup(state);
    return ret;
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    struct installer_state state;
    const WCHAR *filename = NULL;
    int i;

    state.norestart = FALSE;
    state.quiet = FALSE;

    if (TRACE_ON(wusa))
    {
        TRACE("Command line:");
        for (i = 0; i < argc; i++)
            TRACE(" %s", wine_dbgstr_w(argv[i]));
        TRACE("\n");
    }

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '/')
        {
            if (!wcscmp(argv[i], L"/norestart"))
                state.norestart = TRUE;
            else if (!wcscmp(argv[i], L"/quiet"))
                state.quiet = TRUE;
            else
                FIXME("Unknown option: %s\n", wine_dbgstr_w(argv[i]));
        }
        else if (!filename)
            filename = argv[i];
        else
            FIXME("Unknown option: %s\n", wine_dbgstr_w(argv[i]));
    }

    if (!filename)
    {
        FIXME("Missing filename argument\n");
        return 1;
    }

    return !install_msu(filename, &state);
}
