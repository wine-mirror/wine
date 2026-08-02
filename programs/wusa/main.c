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

#include "shlobj.h"
#include "wine/debug.h"
#include "wine/list.h"
#include "wusa.h"

WINE_DEFAULT_DEBUG_CHANNEL(wusa);

struct strbuf
{
    WCHAR *buf;
    DWORD pos;
    DWORD len;
};

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
    struct list updates;
};

static void * CDECL cabinet_alloc(ULONG cb)
{
    return malloc(cb);
}

static void CDECL cabinet_free(void *pv)
{
    free(pv);
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
    if (!(result = malloc(length * sizeof(WCHAR)))) return NULL;

    lstrcpyW(result, path);
    if (result[0] && result[lstrlenW(result) - 1] != '\\') lstrcatW(result, L"\\");
    lstrcatW(result, filename);
    return result;
}

static WCHAR *get_uncompressed_path(PFDINOTIFICATION pfdin)
{
    WCHAR *file = strdupAtoW(pfdin->psz1);
    WCHAR *path = path_combine(pfdin->pv, file);
    free(file);
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
    WCHAR *p, *path = wcsdup(filename);
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
    free(path);
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

    free(file);
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
        free(filenameA);
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
    if (!(entry = malloc(sizeof(*entry)))) return NULL;
    if (!(entry->path = malloc((MAX_PATH + 20) * sizeof(WCHAR))))
    {
        free(entry);
        return NULL;
    }
    for (;;)
    {
        if (!GetTempFileNameW(tmp, L"msu", ++id, entry->path))
        {
            free(entry->path);
            free(entry);
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
    free(full_path);

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
            free(full_path);
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
    struct dependency_entry *dependency, *dependency2;

    LIST_FOR_EACH_ENTRY_SAFE(tempdir, tempdir2, &state->tempdirs, struct installer_tempdir, entry)
    {
        list_remove(&tempdir->entry);
        delete_directory(tempdir->path);
        free(tempdir->path);
        free(tempdir);
    }
    LIST_FOR_EACH_ENTRY_SAFE(assembly, assembly2, &state->assemblies, struct assembly_entry, entry)
    {
        list_remove(&assembly->entry);
        free_assembly(assembly);
    }
    LIST_FOR_EACH_ENTRY_SAFE(dependency, dependency2, &state->updates, struct dependency_entry, entry)
    {
        list_remove(&dependency->entry);
        free_dependency(dependency);
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
    free(path);

    if (!(path = path_combine(temp_path, L"*"))) return FALSE;
    search = FindFirstFileW(path, &data);
    free(path);

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
            free(path);
        }
        while (FindNextFileW(search, &data));
        FindClose(search);
    }

    return TRUE;
}

static BOOL compare_assembly_string(const WCHAR *str1, const WCHAR *str2)
{
    return !wcscmp(str1, str2) || !wcscmp(str1, L"*") || !wcscmp(str2, L"*");
}

static struct assembly_entry *lookup_assembly(struct list *manifest_list, struct assembly_identity *identity)
{
    struct assembly_entry *assembly;

    LIST_FOR_EACH_ENTRY(assembly, manifest_list, struct assembly_entry, entry)
    {
        if (wcsicmp(assembly->identity.name, identity->name)) continue;
        if (!compare_assembly_string(assembly->identity.architecture, identity->architecture)) continue;
        if (!compare_assembly_string(assembly->identity.language, identity->language)) continue;
        if (!compare_assembly_string(assembly->identity.pubkey_token, identity->pubkey_token)) continue;
        if (!compare_assembly_string(assembly->identity.version, identity->version))
        {
            WARN("Ignoring version difference for %s (expected %s, found %s)\n",
                 debugstr_w(identity->name), debugstr_w(identity->version), debugstr_w(assembly->identity.version));
        }
        return assembly;
    }

    return NULL;
}

static WCHAR *get_assembly_source(struct assembly_entry *assembly)
{
    WCHAR *p, *path = wcsdup(assembly->filename);
    if (path && (p = wcsrchr(path, '.'))) *p = 0;
    return path;
}

static BOOL strbuf_init(struct strbuf *buf)
{
    buf->pos = 0;
    buf->len = 64;
    buf->buf = malloc(buf->len * sizeof(WCHAR));
    return buf->buf != NULL;
}

static void strbuf_free(struct strbuf *buf)
{
    free(buf->buf);
    buf->buf = NULL;
}

static BOOL strbuf_append(struct strbuf *buf, const WCHAR *str, DWORD len)
{
    DWORD new_len;
    WCHAR *new_buf;

    if (!buf->buf) return FALSE;
    if (!str) return TRUE;

    if (len == ~0U) len = lstrlenW(str);
    if (buf->pos + len + 1 > buf->len)
    {
        new_len = max(buf->pos + len + 1, buf->len * 2);
        new_buf = realloc(buf->buf, new_len * sizeof(WCHAR));
        if (!new_buf)
        {
            strbuf_free(buf);
            return FALSE;
        }
        buf->buf = new_buf;
        buf->len = new_len;
    }

    memcpy(&buf->buf[buf->pos], str, len * sizeof(WCHAR));
    buf->buf[buf->pos + len] = 0;
    buf->pos += len;
    return TRUE;
}

static BOOL assembly_is_wow64(const struct assembly_entry *assembly)
{
#ifdef __x86_64__
    return !wcsicmp(assembly->identity.architecture, L"x86") || !wcsicmp(assembly->identity.architecture, L"wow64");
#endif
    return FALSE;
}

static WCHAR *lookup_expression(struct assembly_entry *assembly, const WCHAR *key)
{
    WCHAR path[MAX_PATH];
    int csidl = 0;

    if (!wcsicmp(key, L"runtime.system32") || !wcsicmp(key, L"runtime.drivers") || !wcsicmp(key, L"runtime.wbem"))
    {
        if (assembly_is_wow64(assembly)) csidl = CSIDL_SYSTEMX86;
        else csidl = CSIDL_SYSTEM;
    }
    else if (!wcsicmp(key, L"runtime.windows") || !wcsicmp(key, L"runtime.inf")) csidl = CSIDL_WINDOWS;
    else if (!wcsicmp(key, L"runtime.programfiles"))
    {
        if (assembly_is_wow64(assembly)) csidl = CSIDL_PROGRAM_FILESX86;
        else csidl = CSIDL_PROGRAM_FILES;
    }
    else if (!wcsicmp(key, L"runtime.commonfiles"))
    {
        if (assembly_is_wow64(assembly)) csidl = CSIDL_PROGRAM_FILES_COMMONX86;
        else csidl = CSIDL_PROGRAM_FILES_COMMON;
    }
#ifdef __x86_64__
    else if (!wcsicmp(key, L"runtime.programfilesx86")) csidl = CSIDL_PROGRAM_FILESX86;
    else if (!wcsicmp(key, L"runtime.commonfilesx86")) csidl = CSIDL_PROGRAM_FILES_COMMONX86;
#endif
    else if (!wcsicmp(key, L"runtime.programdata")) csidl = CSIDL_COMMON_APPDATA;
    else if (!wcsicmp(key, L"runtime.fonts")) csidl = CSIDL_FONTS;

    if (!csidl)
    {
        FIXME("Unknown expression %s\n", debugstr_w(key));
        return NULL;
    }
    if (!SHGetSpecialFolderPathW(NULL, path, csidl, TRUE))
    {
        ERR("Failed to get folder path for %s\n", debugstr_w(key));
        return NULL;
    }

    if (!wcsicmp(key, L"runtime.inf")) wcscat(path, L"\\inf");
    else if (!wcsicmp(key, L"runtime.drivers")) wcscat(path, L"\\drivers");
    else if (!wcsicmp(key, L"runtime.wbem")) wcscat(path, L"\\wbem");
    return wcsdup(path);
}

static WCHAR *expand_expression(struct assembly_entry *assembly, const WCHAR *expression)
{
    const WCHAR *pos, *next;
    WCHAR *key, *value;
    struct strbuf buf;

    if (!expression || !strbuf_init(&buf)) return NULL;

    for (pos = expression; (next = wcsstr(pos, L"$(")); pos = next + 1)
    {
        strbuf_append(&buf, pos, next - pos);
        pos = next + 2;
        if (!(next = wcsstr(pos, L")")))
        {
            strbuf_append(&buf, L"$(", 2);
            break;
        }

        if (!(key = strdupWn(pos, next - pos))) goto error;
        value = lookup_expression(assembly, key);
        free(key);
        if (!value) goto error;
        strbuf_append(&buf, value, ~0U);
        free(value);
    }

    strbuf_append(&buf, pos, ~0U);
    return buf.buf;

error:
    FIXME("Couldn't resolve expression %s\n", debugstr_w(expression));
    strbuf_free(&buf);
    return NULL;
}

static BOOL install_files_copy(struct assembly_entry *assembly, const WCHAR *source_path, struct fileop_entry *fileop, BOOL dryrun)
{
    WCHAR *target_path, *target, *source = NULL;
    BOOL ret = FALSE;

    if (!(target_path = expand_expression(assembly, fileop->target))) return FALSE;
    if (!(target = path_combine(target_path, fileop->source))) goto error;
    if (!(source = path_combine(source_path, fileop->source))) goto error;

    if (dryrun)
    {
        if (!(ret = PathFileExistsW(source)))
        {
            ERR("Required file %s not found\n", debugstr_w(source));
            goto error;
        }
    }
    else
    {
        TRACE("Copying %s -> %s\n", debugstr_w(source), debugstr_w(target));

        if (!create_parent_directory(target))
        {
            ERR("Failed to create parent directory for %s\n", debugstr_w(target));
            goto error;
        }
        if (!(ret = CopyFileExW(source, target, NULL, NULL, NULL, 0)))
        {
            ERR("Failed to copy %s to %s\n", debugstr_w(source), debugstr_w(target));
            goto error;
        }
    }

error:
    free(target_path);
    free(target);
    free(source);
    return ret;
}

static BOOL install_files(struct assembly_entry *assembly, BOOL dryrun)
{
    struct fileop_entry *fileop;
    WCHAR *source_path;
    BOOL ret = TRUE;

    if (!(source_path = get_assembly_source(assembly)))
    {
        ERR("Failed to get assembly source directory\n");
        return FALSE;
    }

    LIST_FOR_EACH_ENTRY(fileop, &assembly->fileops, struct fileop_entry, entry)
    {
        if (!(ret = install_files_copy(assembly, source_path, fileop, dryrun))) break;
    }

    free(source_path);
    return ret;
}

static WCHAR *split_registry_key(WCHAR *key, HKEY *root)
{
    DWORD size;
    WCHAR *p;

    if (!(p = wcschr(key, '\\'))) return NULL;
    size = p - key;

    if (lstrlenW(L"HKEY_CLASSES_ROOT") == size && !wcsncmp(key, L"HKEY_CLASSES_ROOT", size))
        *root = HKEY_CLASSES_ROOT;
    else if (lstrlenW(L"HKEY_CURRENT_CONFIG") == size && !wcsncmp(key, L"HKEY_CURRENT_CONFIG", size))
        *root = HKEY_CURRENT_CONFIG;
    else if (lstrlenW(L"HKEY_CURRENT_USER") == size && !wcsncmp(key, L"HKEY_CURRENT_USER", size))
        *root = HKEY_CURRENT_USER;
    else if (lstrlenW(L"HKEY_LOCAL_MACHINE") == size && !wcsncmp(key, L"HKEY_LOCAL_MACHINE", size))
        *root = HKEY_LOCAL_MACHINE;
    else if (lstrlenW(L"HKEY_USERS") == size && !wcsncmp(key, L"HKEY_USERS", size))
        *root = HKEY_USERS;
    else
    {
        FIXME("Unknown root key %s\n", debugstr_wn(key, size));
        return NULL;
    }

    return p + 1;
}

static BOOL install_registry_string(struct assembly_entry *assembly, HKEY key, struct registrykv_entry *registrykv, DWORD type, BOOL dryrun)
{
    DWORD value_size;
    WCHAR *value = expand_expression(assembly, registrykv->value);
    BOOL ret = TRUE;

    if (registrykv->value && !value)
        return FALSE;

    value_size = value ? (lstrlenW(value) + 1) * sizeof(WCHAR) : 0;
    if (!dryrun && RegSetValueExW(key, registrykv->name, 0, type, (void *)value, value_size))
    {
        ERR("Failed to set registry key %s\n", debugstr_w(registrykv->name));
        ret = FALSE;
    }

    free(value);
    return ret;
}

static WCHAR *parse_multisz(const WCHAR *input, DWORD *size)
{
    const WCHAR *pos, *next;
    struct strbuf buf;

    *size = 0;
    if (!input || !input[0] || !strbuf_init(&buf)) return NULL;

    for (pos = input; pos[0] == '"'; pos++)
    {
        pos++;
        if (!(next = wcsstr(pos, L"\""))) goto error;
        strbuf_append(&buf, pos, next - pos);
        strbuf_append(&buf, L"", ARRAY_SIZE(L""));

        pos = next + 1;
        if (!pos[0]) break;
        if (pos[0] != ',')
        {
            FIXME("Error while parsing REG_MULTI_SZ string: Expected comma but got '%c'\n", pos[0]);
            goto error;
        }
    }

    if (pos[0])
    {
        FIXME("Error while parsing REG_MULTI_SZ string: Garbage at end of string\n");
        goto error;
    }

    strbuf_append(&buf, L"", ARRAY_SIZE(L""));
    *size = buf.pos * sizeof(WCHAR);
    return buf.buf;

error:
    strbuf_free(&buf);
    return NULL;
}

static BOOL install_registry_multisz(struct assembly_entry *assembly, HKEY key, struct registrykv_entry *registrykv, BOOL dryrun)
{
    DWORD value_size;
    WCHAR *value = parse_multisz(registrykv->value, &value_size);
    BOOL ret = TRUE;

    if (registrykv->value && registrykv->value[0] && !value)
        return FALSE;

    if (!dryrun && RegSetValueExW(key, registrykv->name, 0, REG_MULTI_SZ, (void *)value, value_size))
    {
        ERR("Failed to set registry key %s\n", debugstr_w(registrykv->name));
        ret = FALSE;
    }

    free(value);
    return ret;
}

static BOOL install_registry_dword(struct assembly_entry *assembly, HKEY key, struct registrykv_entry *registrykv, BOOL dryrun)
{
    DWORD value = registrykv->value_type ? wcstoul(registrykv->value_type, NULL, 16) : 0;
    BOOL ret = TRUE;

    if (!dryrun && RegSetValueExW(key, registrykv->name, 0, REG_DWORD, (void *)&value, sizeof(value)))
    {
        ERR("Failed to set registry key %s\n", debugstr_w(registrykv->name));
        ret = FALSE;
    }

    return ret;
}

static BYTE *parse_hex(const WCHAR *input, DWORD *size)
{
    WCHAR number[3] = {0, 0, 0};
    BYTE *output, *p;
    int length;

    *size = 0;
    if (!input) return NULL;
    length = lstrlenW(input);
    if (length & 1) return NULL;
    length >>= 1;

    if (!(output = malloc(length))) return NULL;
    for (p = output; *input; input += 2)
    {
        number[0] = input[0];
        number[1] = input[1];
        *p++ = wcstoul(number, 0, 16);
    }
    *size = length;
    return output;
}

static BOOL install_registry_binary(struct assembly_entry *assembly, HKEY key, struct registrykv_entry *registrykv, BOOL dryrun)
{
    DWORD value_size;
    BYTE *value = parse_hex(registrykv->value, &value_size);
    BOOL ret = TRUE;

    if (registrykv->value && !value)
        return FALSE;

    if (!dryrun && RegSetValueExW(key, registrykv->name, 0, REG_BINARY, value, value_size))
    {
        ERR("Failed to set registry key %s\n", debugstr_w(registrykv->name));
        ret = FALSE;
    }

    free(value);
    return ret;
}

static BOOL install_registry_value(struct assembly_entry *assembly, HKEY key, struct registrykv_entry *registrykv, BOOL dryrun)
{
    TRACE("Setting registry key %s = %s\n", debugstr_w(registrykv->name), debugstr_w(registrykv->value));

    if (!wcscmp(registrykv->value_type, L"REG_SZ"))
        return install_registry_string(assembly, key, registrykv, REG_SZ, dryrun);
    if (!wcscmp(registrykv->value_type, L"REG_EXPAND_SZ"))
        return install_registry_string(assembly, key, registrykv, REG_EXPAND_SZ, dryrun);
    if (!wcscmp(registrykv->value_type, L"REG_MULTI_SZ"))
        return install_registry_multisz(assembly, key, registrykv, dryrun);
    if (!wcscmp(registrykv->value_type, L"REG_DWORD"))
        return install_registry_dword(assembly, key, registrykv, dryrun);
    if (!wcscmp(registrykv->value_type, L"REG_BINARY"))
        return install_registry_binary(assembly, key, registrykv, dryrun);

    FIXME("Unsupported registry value type %s\n", debugstr_w(registrykv->value_type));
    return FALSE;
}

static BOOL install_registry(struct assembly_entry *assembly, BOOL dryrun)
{
    struct registryop_entry *registryop;
    struct registrykv_entry *registrykv;
    HKEY root, subkey;
    WCHAR *path;
    REGSAM sam = KEY_ALL_ACCESS;
    BOOL ret = TRUE;

#ifdef __x86_64__
    if (!wcscmp(assembly->identity.architecture, L"x86")) sam |= KEY_WOW64_32KEY;
#endif

    LIST_FOR_EACH_ENTRY(registryop, &assembly->registryops, struct registryop_entry, entry)
    {
        if (!(path = split_registry_key(registryop->key, &root)))
        {
            ret = FALSE;
            break;
        }

        TRACE("Processing registry key %s\n", debugstr_w(registryop->key));

        if (!dryrun && RegCreateKeyExW(root, path, 0, NULL, 0, sam, NULL, &subkey, NULL))
        {
            ERR("Failed to open registry key %s\n", debugstr_w(registryop->key));
            ret = FALSE;
            break;
        }

        LIST_FOR_EACH_ENTRY(registrykv, &registryop->keyvalues, struct registrykv_entry, entry)
        {
            if (!(ret = install_registry_value(assembly, subkey, registrykv, dryrun))) break;
        }

        if (!dryrun) RegCloseKey(subkey);
        if (!ret) break;
    }

    return ret;
}

static BOOL install_assembly(struct list *manifest_list, struct assembly_identity *identity, BOOL dryrun)
{
    struct dependency_entry *dependency;
    struct assembly_entry *assembly;
    const WCHAR *name;

    if (!(assembly = lookup_assembly(manifest_list, identity)))
    {
        FIXME("Assembly %s not found\n", debugstr_w(identity->name));
        return FALSE;
    }

    name = assembly->identity.name;

    if (assembly->status == ASSEMBLY_STATUS_INSTALLED)
    {
        TRACE("Assembly %s already installed\n", debugstr_w(name));
        return TRUE;
    }
    if (assembly->status == ASSEMBLY_STATUS_IN_PROGRESS)
    {
        ERR("Assembly %s caused circular dependency\n", debugstr_w(name));
        return FALSE;
    }

#ifdef __i386__
    if (!wcscmp(assembly->identity.architecture, L"amd64"))
    {
        ERR("Cannot install amd64 assembly in 32-bit prefix\n");
        return FALSE;
    }
#endif

    assembly->status = ASSEMBLY_STATUS_IN_PROGRESS;

    LIST_FOR_EACH_ENTRY(dependency, &assembly->dependencies, struct dependency_entry, entry)
    {
        if (!install_assembly(manifest_list, &dependency->identity, dryrun)) return FALSE;
    }

    TRACE("Installing assembly %s%s\n", debugstr_w(name), dryrun ? " (dryrun)" : "");

    if (!install_files(assembly, dryrun))
    {
        ERR("Failed to install all files for %s\n", debugstr_w(name));
        return FALSE;
    }

    if (!install_registry(assembly, dryrun))
    {
        ERR("Failed to install registry keys for %s\n", debugstr_w(name));
        return FALSE;
    }

    TRACE("Installation of %s finished\n", debugstr_w(name));

    assembly->status = ASSEMBLY_STATUS_INSTALLED;
    return TRUE;
}

static BOOL install_updates(struct installer_state *state, BOOL dryrun)
{
    struct dependency_entry *dependency;
    LIST_FOR_EACH_ENTRY(dependency, &state->updates, struct dependency_entry, entry)
    {
        if (!install_assembly(&state->assemblies, &dependency->identity, dryrun))
        {
            ERR("Failed to install update %s\n", debugstr_w(dependency->identity.name));
            return FALSE;
        }
    }
    return TRUE;
}

static void set_assembly_status(struct list *manifest_list, DWORD status)
{
    struct assembly_entry *assembly;
    LIST_FOR_EACH_ENTRY(assembly, manifest_list, struct assembly_entry, entry)
    {
        assembly->status = status;
    }
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
    list_init(&state->updates);
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
    free(path);

    if (search != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            if (!wcsicmp(data.cFileName, L"WSUSSCAN.cab")) continue;
            if (!(path = path_combine(temp_path, data.cFileName))) continue;
            if (!load_assemblies_from_cab(path, state))
                ERR("Failed to load all manifests from %s, ignoring\n", debugstr_w(path));
            free(path);
        }
        while (FindNextFileW(search, &data));
        FindClose(search);
    }

    /* load all update descriptions */
    if (!(path = path_combine(temp_path, L"*.xml"))) goto done;
    search = FindFirstFileW(path, &data);
    free(path);

    if (search != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            if (!(path = path_combine(temp_path, data.cFileName))) continue;
            if (!load_update(path, &state->updates))
                ERR("Failed to load all updates from %s, ignoring\n", debugstr_w(path));
            free(path);
        }
        while (FindNextFileW(search, &data));
        FindClose(search);
    }

    /* dump package information (for debugging) */
    if (TRACE_ON(wusa))
    {
        struct dependency_entry *dependency;
        struct assembly_entry *assembly;

        TRACE("List of updates:\n");
        LIST_FOR_EACH_ENTRY(dependency, &state->updates, struct dependency_entry, entry)
            TRACE(" * %s\n", debugstr_w(dependency->identity.name));

        TRACE("List of manifests (with dependencies):\n");
        LIST_FOR_EACH_ENTRY(assembly, &state->assemblies, struct assembly_entry, entry)
        {
            TRACE(" * %s\n", debugstr_w(assembly->identity.name));
            LIST_FOR_EACH_ENTRY(dependency, &assembly->dependencies, struct dependency_entry, entry)
                TRACE("   -> %s\n", debugstr_w(dependency->identity.name));
        }
    }

    if (list_empty(&state->updates))
    {
        ERR("No updates found, probably incompatible MSU file format?\n");
        goto done;
    }

    /* perform dry run */
    set_assembly_status(&state->assemblies, ASSEMBLY_STATUS_NONE);
    if (!install_updates(state, TRUE))
    {
        ERR("Dry run failed, aborting installation\n");
        goto done;
    }

    /* installation */
    set_assembly_status(&state->assemblies, ASSEMBLY_STATUS_NONE);
    if (!install_updates(state, FALSE))
    {
        ERR("Installation failed\n");
        goto done;
    }

    TRACE("Installation finished\n");
    ret = TRUE;

done:
    installer_cleanup(state);
    return ret;
}

static void restart_as_x86_64(void)
{
    WCHAR filename[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    DWORD exit_code = 1;
    void *redir;

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    GetSystemDirectoryW( filename, MAX_PATH );
    wcscat( filename, L"\\wusa.exe" );

    Wow64DisableWow64FsRedirection(&redir);
    if (CreateProcessW(filename, GetCommandLineW(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        TRACE("Restarting %s\n", wine_dbgstr_w(filename));
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &exit_code);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else ERR("Failed to restart 64-bit %s, err %lu\n", wine_dbgstr_w(filename), GetLastError());
    Wow64RevertWow64FsRedirection(redir);

    ExitProcess(exit_code);
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    struct installer_state state;
    const WCHAR *filename = NULL;
    BOOL is_wow64;
    int i;

    if (IsWow64Process( GetCurrentProcess(), &is_wow64 ) && is_wow64) restart_as_x86_64();

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
