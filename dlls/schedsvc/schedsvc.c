/*
 * Task Scheduler Service
 *
 * Copyright 2014 Dmitry Timoshkov
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
#include "schrpc.h"
#include "taskschd.h"
#include "wine/debug.h"

#include "schedsvc_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(schedsvc);

static const char bom_utf8[] = { 0xef,0xbb,0xbf };

HRESULT __cdecl SchRpcHighestVersion(DWORD *version)
{
    TRACE("%p\n", version);

    *version = MAKELONG(3, 1);
    return S_OK;
}

static WCHAR *get_full_name(const WCHAR *path, WCHAR **relative_path)
{
    static const WCHAR tasksW[] = { '\\','t','a','s','k','s','\\',0 };
    WCHAR *target;
    int len;

    len = GetSystemDirectoryW(NULL, 0);
    len += lstrlenW(tasksW) + lstrlenW(path);

    target = heap_alloc(len * sizeof(WCHAR));
    if (target)
    {
        GetSystemDirectoryW(target, len);
        lstrcatW(target, tasksW);
        if (relative_path)
            *relative_path = target + lstrlenW(target) - 1;
        while (*path == '\\') path++;
        lstrcatW(target, path);
    }
    return target;
}

/*
 * Recursively create all directories in the path.
 */
static HRESULT create_directory(const WCHAR *path)
{
    HRESULT hr = S_OK;
    WCHAR *new_path;
    int len;

    new_path = heap_alloc((lstrlenW(path) + 1) * sizeof(WCHAR));
    if (!new_path) return E_OUTOFMEMORY;

    lstrcpyW(new_path, path);

    len = lstrlenW(new_path);
    while (len && new_path[len - 1] == '\\')
    {
        new_path[len - 1] = 0;
        len--;
    }

    while (!CreateDirectoryW(new_path, NULL))
    {
        WCHAR *slash;
        DWORD last_error = GetLastError();

        if (last_error != ERROR_PATH_NOT_FOUND || !(slash = wcsrchr(new_path, '\\')))
        {
            hr = HRESULT_FROM_WIN32(last_error);
            break;
        }

        len = slash - new_path;
        new_path[len] = 0;
        hr = create_directory(new_path);
        if (hr != S_OK) break;
        new_path[len] = '\\';
    }

    heap_free(new_path);
    return hr;
}

static HRESULT write_xml_utf8(const WCHAR *name, DWORD disposition, const WCHAR *xmlW)
{
    static const char comment[] = "<!-- Task definition created by Wine -->\n";
    HANDLE hfile;
    DWORD size;
    char *xml;
    HRESULT hr = S_OK;

    hfile = CreateFileW(name, GENERIC_WRITE, 0, NULL, disposition, 0, 0);
    if (hfile == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() == ERROR_FILE_EXISTS)
            return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);

        return HRESULT_FROM_WIN32(GetLastError());
    }

    size = WideCharToMultiByte(CP_UTF8, 0, xmlW, -1, NULL, 0, NULL, NULL);
    xml = heap_alloc(size);
    if (!xml)
    {
        CloseHandle(hfile);
        return E_OUTOFMEMORY;
    }
    WideCharToMultiByte(CP_UTF8, 0, xmlW, -1, xml, size, NULL, NULL);

    if (!WriteFile(hfile, bom_utf8, sizeof(bom_utf8), &size, NULL))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto failed;
    }
    if (!WriteFile(hfile, comment, strlen(comment), &size, NULL))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto failed;
    }

    /* skip XML declaration with UTF-16 specifier */
    if (!memcmp(xml, "<?xml", 5))
    {
        const char *p = strchr(xml, '>');
        if (p++) while (isspace(*p)) p++;
        else p = xml;
        if (!WriteFile(hfile, p, strlen(p), &size, NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        if (!WriteFile(hfile, xml, strlen(xml), &size, NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());
    }

failed:
    heap_free(xml);
    CloseHandle(hfile);
    return hr;
}

HRESULT __cdecl SchRpcRegisterTask(const WCHAR *path, const WCHAR *xml, DWORD flags, const WCHAR *sddl,
                                   DWORD task_logon_type, DWORD n_creds, const TASK_USER_CRED *creds,
                                   WCHAR **actual_path, TASK_XML_ERROR_INFO **xml_error_info)
{
    WCHAR *full_name, *relative_path;
    DWORD disposition;
    HRESULT hr;

    TRACE("%s,%s,%#lx,%s,%lu,%lu,%p,%p,%p\n", debugstr_w(path), debugstr_w(xml), flags,
          debugstr_w(sddl), task_logon_type, n_creds, creds, actual_path, xml_error_info);

    *actual_path = NULL;
    *xml_error_info = NULL;

    /* FIXME: assume that validation is performed on the client side */
    if (flags & TASK_VALIDATE_ONLY) return S_OK;

    if (path)
    {
        full_name = get_full_name(path, &relative_path);
        if (!full_name) return E_OUTOFMEMORY;

        if (wcschr(path, '\\') || wcschr(path, '/'))
        {
            WCHAR *p = wcsrchr(full_name, '/');
            if (!p) p = wcsrchr(full_name, '\\');
            *p = 0;
            hr = create_directory(full_name);
            if (hr != S_OK && hr != HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
            {
                heap_free(full_name);
                return hr;
            }
            *p = '\\';
        }
    }
    else
    {
        IID iid;
        WCHAR uuid_str[39];

        UuidCreate(&iid);
        StringFromGUID2(&iid, uuid_str, 39);

        full_name = get_full_name(uuid_str, &relative_path);
        if (!full_name) return E_OUTOFMEMORY;
        /* skip leading '\' */
        relative_path++;
    }

    switch (flags & (TASK_CREATE | TASK_UPDATE))
    {
    default:
    case TASK_CREATE:
        disposition = CREATE_NEW;
        break;

    case TASK_UPDATE:
        disposition = OPEN_EXISTING;
        break;

    case (TASK_CREATE | TASK_UPDATE):
        disposition = OPEN_ALWAYS;
        break;
    }

    hr = write_xml_utf8(full_name, disposition, xml);
    if (hr == S_OK)
    {
        *actual_path = heap_strdupW(relative_path);
        schedsvc_auto_start();
    }

    heap_free(full_name);
    return hr;
}

static int detect_encoding(const void *buffer, DWORD size)
{
    if (size >= sizeof(bom_utf8) && !memcmp(buffer, bom_utf8, sizeof(bom_utf8)))
        return CP_UTF8;
    else
    {
        int flags = IS_TEXT_UNICODE_SIGNATURE |
                    IS_TEXT_UNICODE_REVERSE_SIGNATURE |
                    IS_TEXT_UNICODE_ODD_LENGTH;
        IsTextUnicode(buffer, size, &flags);
        if (flags & IS_TEXT_UNICODE_SIGNATURE)
            return -1;
        if (flags & IS_TEXT_UNICODE_REVERSE_SIGNATURE)
            return -2;
        return CP_ACP;
    }
}

static HRESULT read_xml(const WCHAR *name, WCHAR **xml)
{
    char *src, *buff;
    HANDLE hfile;
    DWORD size, attrs;
    HRESULT hr = S_OK;
    int cp;

    attrs = GetFileAttributesW(name);
    if (attrs == INVALID_FILE_ATTRIBUTES)
        return HRESULT_FROM_WIN32(GetLastError());
    if (attrs & FILE_ATTRIBUTE_DIRECTORY)
        return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);

    hfile = CreateFileW(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    if (hfile == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(GetLastError());

    size = GetFileSize(hfile, NULL);
    buff = src = heap_alloc(size + 2);
    if (!src)
    {
        CloseHandle(hfile);
        return E_OUTOFMEMORY;
    }

    src[size] = 0;
    src[size + 1] = 0;

    ReadFile(hfile, src, size, &size, NULL);
    CloseHandle(hfile);

    cp = detect_encoding(src, size);
    if (cp < 0)
    {
        *xml = (WCHAR *)src;
        return S_OK;
    }

    if (cp == CP_UTF8 && size >= sizeof(bom_utf8) && !memcmp(src, bom_utf8, sizeof(bom_utf8)))
        src += sizeof(bom_utf8);

    size = MultiByteToWideChar(cp, 0, src, -1, NULL, 0);
    *xml = heap_alloc(size * sizeof(WCHAR));
    if (*xml)
        MultiByteToWideChar(cp, 0, src, -1, *xml, size);
    else
        hr = E_OUTOFMEMORY;
    heap_free(buff);

    return hr;
}

HRESULT __cdecl SchRpcRetrieveTask(const WCHAR *path, const WCHAR *languages, ULONG *n_languages, WCHAR **xml)
{
    WCHAR *full_name;
    HRESULT hr;

    TRACE("%s,%s,%p,%p\n", debugstr_w(path), debugstr_w(languages), n_languages, xml);

    full_name = get_full_name(path, NULL);
    if (!full_name) return E_OUTOFMEMORY;

    hr = read_xml(full_name, xml);
    if (hr != S_OK) *xml = NULL;

    heap_free(full_name);
    return hr;
}

HRESULT __cdecl SchRpcCreateFolder(const WCHAR *path, const WCHAR *sddl, DWORD flags)
{
    WCHAR *full_name;
    HRESULT hr;

    TRACE("%s,%s,%#lx\n", debugstr_w(path), debugstr_w(sddl), flags);

    if (flags) return E_INVALIDARG;

    full_name = get_full_name(path, NULL);
    if (!full_name) return E_OUTOFMEMORY;

    hr = create_directory(full_name);

    heap_free(full_name);
    return hr;
}

HRESULT __cdecl SchRpcSetSecurity(const WCHAR *path, const WCHAR *sddl, DWORD flags)
{
    FIXME("%s,%s,%#lx: stub\n", debugstr_w(path), debugstr_w(sddl), flags);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcGetSecurity(const WCHAR *path, DWORD flags, WCHAR **sddl)
{
    FIXME("%s,%#lx,%p: stub\n", debugstr_w(path), flags, sddl);
    return E_NOTIMPL;
}

static void free_list(TASK_NAMES list, LONG count)
{
    LONG i;

    for (i = 0; i < count; i++)
        heap_free(list[i]);

    heap_free(list);
}

static inline BOOL is_directory(const WIN32_FIND_DATAW *data)
{
    if (data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        if (data->cFileName[0] == '.')
        {
            if (!data->cFileName[1] || (data->cFileName[1] == '.' && !data->cFileName[2]))
                return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

static inline BOOL is_file(const WIN32_FIND_DATAW *data)
{
    return !(data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

HRESULT __cdecl SchRpcEnumFolders(const WCHAR *path, DWORD flags, DWORD *start_index, DWORD n_requested,
                                  DWORD *n_names, TASK_NAMES *names)
{
    static const WCHAR allW[] = {'\\','*',0};
    HRESULT hr = S_OK;
    WCHAR *full_name;
    WCHAR pathW[MAX_PATH];
    WIN32_FIND_DATAW data;
    HANDLE handle;
    DWORD allocated, count, index;
    TASK_NAMES list;

    TRACE("%s,%#lx,%lu,%lu,%p,%p\n", debugstr_w(path), flags, *start_index, n_requested, n_names, names);

    *n_names = 0;
    *names = NULL;

    if (flags & ~TASK_ENUM_HIDDEN) return E_INVALIDARG;

    if (!n_requested) n_requested = ~0u;

    full_name = get_full_name(path, NULL);
    if (!full_name) return E_OUTOFMEMORY;

    if (lstrlenW(full_name) + 2 > MAX_PATH)
    {
        heap_free(full_name);
        return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
    }

    lstrcpyW(pathW, full_name);
    lstrcatW(pathW, allW);

    heap_free(full_name);

    allocated = 64;
    list = heap_alloc(allocated * sizeof(list[0]));
    if (!list) return E_OUTOFMEMORY;

    index = count = 0;

    handle = FindFirstFileW(pathW, &data);
    if (handle == INVALID_HANDLE_VALUE)
    {
        heap_free(list);
        if (GetLastError() == ERROR_PATH_NOT_FOUND)
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    do
    {
        if (is_directory(&data) && index++ >= *start_index)
        {
            if (count >= allocated)
            {
                TASK_NAMES new_list;
                allocated *= 2;
                new_list = heap_realloc(list, allocated * sizeof(list[0]));
                if (!new_list)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }
                list = new_list;
            }

            TRACE("adding %s\n", debugstr_w(data.cFileName));

            list[count] = heap_strdupW(data.cFileName);
            if (!list[count])
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            count++;

            if (count >= n_requested)
            {
                hr = S_FALSE;
                break;
            }
        }
    } while (FindNextFileW(handle, &data));

    FindClose(handle);

    if (FAILED(hr))
    {
        free_list(list, count);
        return hr;
    }

    *n_names = count;

    if (count)
    {
        *names = list;
        *start_index = index;
        return hr;
    }

    heap_free(list);
    *names = NULL;
    return *start_index ? S_FALSE : S_OK;
}

HRESULT __cdecl SchRpcEnumTasks(const WCHAR *path, DWORD flags, DWORD *start_index, DWORD n_requested,
                                DWORD *n_names, TASK_NAMES *names)
{
    static const WCHAR allW[] = {'\\','*',0};
    HRESULT hr = S_OK;
    WCHAR *full_name;
    WCHAR pathW[MAX_PATH];
    WIN32_FIND_DATAW data;
    HANDLE handle;
    DWORD allocated, count, index;
    TASK_NAMES list;

    TRACE("%s,%#lx,%lu,%lu,%p,%p\n", debugstr_w(path), flags, *start_index, n_requested, n_names, names);

    *n_names = 0;
    *names = NULL;

    if (flags & ~TASK_ENUM_HIDDEN) return E_INVALIDARG;

    if (!n_requested) n_requested = ~0u;

    full_name = get_full_name(path, NULL);
    if (!full_name) return E_OUTOFMEMORY;

    if (lstrlenW(full_name) + 2 > MAX_PATH)
    {
        heap_free(full_name);
        return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
    }

    lstrcpyW(pathW, full_name);
    lstrcatW(pathW, allW);

    heap_free(full_name);

    allocated = 64;
    list = heap_alloc(allocated * sizeof(list[0]));
    if (!list) return E_OUTOFMEMORY;

    index = count = 0;

    handle = FindFirstFileW(pathW, &data);
    if (handle == INVALID_HANDLE_VALUE)
    {
        heap_free(list);
        if (GetLastError() == ERROR_PATH_NOT_FOUND)
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    do
    {
        if (is_file(&data) && index++ >= *start_index)
        {
            if (count >= allocated)
            {
                TASK_NAMES new_list;
                allocated *= 2;
                new_list = heap_realloc(list, allocated * sizeof(list[0]));
                if (!new_list)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }
                list = new_list;
            }

            TRACE("adding %s\n", debugstr_w(data.cFileName));

            list[count] = heap_strdupW(data.cFileName);
            if (!list[count])
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            count++;

            if (count >= n_requested)
            {
                hr = S_FALSE;
                break;
            }
        }
    } while (FindNextFileW(handle, &data));

    FindClose(handle);

    if (FAILED(hr))
    {
        free_list(list, count);
        return hr;
    }

    *n_names = count;

    if (count)
    {
        *names = list;
        *start_index = index;
        return hr;
    }

    heap_free(list);
    *names = NULL;
    return *start_index ? S_FALSE : S_OK;
}

HRESULT __cdecl SchRpcEnumInstances(const WCHAR *path, DWORD flags, DWORD *n_guids, GUID **guids)
{
    FIXME("%s,%#lx,%p,%p: stub\n", debugstr_w(path), flags, n_guids, guids);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcGetInstanceInfo(GUID guid, WCHAR **path, DWORD *task_state, WCHAR **action,
                                      WCHAR **info, DWORD *n_instances, GUID **instances, DWORD *pid)
{
    FIXME("%s,%p,%p,%p,%p,%p,%p,%p: stub\n", wine_dbgstr_guid(&guid), path, task_state, action,
          info, n_instances, instances, pid);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcStopInstance(GUID guid, DWORD flags)
{
    FIXME("%s,%#lx: stub\n", wine_dbgstr_guid(&guid), flags);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcStop(const WCHAR *path, DWORD flags)
{
    FIXME("%s,%#lx: stub\n", debugstr_w(path), flags);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcRun(const WCHAR *path, DWORD n_args, const WCHAR **args, DWORD flags,
                          DWORD session_id, const WCHAR *user, GUID *guid)
{
    FIXME("%s,%lu,%p,%#lx,%#lx,%s,%p: stub\n", debugstr_w(path), n_args, args, flags,
          session_id, debugstr_w(user), guid);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcDelete(const WCHAR *path, DWORD flags)
{
    WCHAR *full_name;
    HRESULT hr = S_OK;

    TRACE("%s,%#lx\n", debugstr_w(path), flags);

    if (flags) return E_INVALIDARG;

    while (*path == '\\' || *path == '/') path++;
    if (!*path) return E_ACCESSDENIED;

    full_name = get_full_name(path, NULL);
    if (!full_name) return E_OUTOFMEMORY;

    if (!RemoveDirectoryW(full_name))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        if (hr == HRESULT_FROM_WIN32(ERROR_DIRECTORY))
            hr = DeleteFileW(full_name) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
    }

    heap_free(full_name);
    return hr;
}

HRESULT __cdecl SchRpcRename(const WCHAR *path, const WCHAR *name, DWORD flags)
{
    FIXME("%s,%s,%#lx: stub\n", debugstr_w(path), debugstr_w(name), flags);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcScheduledRuntimes(const WCHAR *path, SYSTEMTIME *start, SYSTEMTIME *end, DWORD flags,
                                        DWORD n_requested, DWORD *n_runtimes, SYSTEMTIME **runtimes)
{
    FIXME("%s,%p,%p,%#lx,%lu,%p,%p: stub\n", debugstr_w(path), start, end, flags,
          n_requested, n_runtimes, runtimes);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcGetLastRunInfo(const WCHAR *path, SYSTEMTIME *last_runtime, DWORD *last_return_code)
{
    FIXME("%s,%p,%p: stub\n", debugstr_w(path), last_runtime, last_return_code);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcGetTaskInfo(const WCHAR *path, DWORD flags, DWORD *enabled, DWORD *task_state)
{
    WCHAR *full_name, *xml;
    HRESULT hr;

    FIXME("%s,%#lx,%p,%p: stub\n", debugstr_w(path), flags, enabled, task_state);

    full_name = get_full_name(path, NULL);
    if (!full_name) return E_OUTOFMEMORY;

    hr = read_xml(full_name, &xml);
    heap_free(full_name);
    if (hr != S_OK) return hr;
    heap_free(xml);

    *enabled = 0;
    *task_state = (flags & SCH_FLAG_STATE) ? TASK_STATE_DISABLED : TASK_STATE_UNKNOWN;
    return S_OK;
}

HRESULT __cdecl SchRpcGetNumberOfMissedRuns(const WCHAR *path, DWORD *runs)
{
    FIXME("%s,%p: stub\n", debugstr_w(path), runs);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcEnableTask(const WCHAR *path, DWORD enabled)
{
    FIXME("%s,%lu: stub\n", debugstr_w(path), enabled);
    return E_NOTIMPL;
}
