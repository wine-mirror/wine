/*
 * File path.c - managing path in debugging environments
 *
 * Copyright (C) 2004,2008, Eric Pouech
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dbghelp_private.h"
#include "image_private.h"
#include "winnls.h"
#include "winternl.h"
#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

static const struct machine_dir
{
    DWORD machine;
    const WCHAR *pe_dir;
    const WCHAR *so_dir;
}
    all_machine_dir[] =
{
    {IMAGE_FILE_MACHINE_I386,  L"\\i386-windows\\",    L"\\i386-unix\\"},
    {IMAGE_FILE_MACHINE_AMD64, L"\\x86_64-windows\\",  L"\\x86_64-unix\\"},
    {IMAGE_FILE_MACHINE_ARMNT, L"\\arm-windows\\",     L"\\arm-unix\\"},
    {IMAGE_FILE_MACHINE_ARM64, L"\\aarch64-windows\\", L"\\aarch64-unix\\"},
};

static inline BOOL is_sepA(char ch) {return ch == '/' || ch == '\\';}
static inline BOOL is_sep(WCHAR ch) {return ch == '/' || ch == '\\';}

const char* file_nameA(const char* str)
{
    const char*       p;

    for (p = str + strlen(str) - 1; p >= str && !is_sepA(*p); p--);
    return p + 1;
}

const WCHAR* file_name(const WCHAR* str)
{
    const WCHAR*      p;

    for (p = str + lstrlenW(str) - 1; p >= str && !is_sep(*p); p--);
    return p + 1;
}

static inline void file_pathW(const WCHAR *src, WCHAR *dst)
{
    int len;

    for (len = lstrlenW(src) - 1; (len > 0) && (!is_sep(src[len])); len--);
    memcpy( dst, src, len * sizeof(WCHAR) );
    dst[len] = 0;
}

/******************************************************************
 *		FindDebugInfoFile (DBGHELP.@)
 *
 */
HANDLE WINAPI FindDebugInfoFile(PCSTR FileName, PCSTR SymbolPath, PSTR DebugFilePath)
{
    HANDLE      h;

    h = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
    {
        if (!SearchPathA(SymbolPath, file_nameA(FileName), NULL, MAX_PATH, DebugFilePath, NULL))
            return NULL;
        h = CreateFileA(DebugFilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    return (h == INVALID_HANDLE_VALUE) ? NULL : h;
}
 
/******************************************************************
 *		FindDebugInfoFileEx (DBGHELP.@)
 *
 */
HANDLE WINAPI FindDebugInfoFileEx(PCSTR FileName, PCSTR SymbolPath,
                                  PSTR DebugFilePath, 
                                  PFIND_DEBUG_FILE_CALLBACK Callback,
                                  PVOID CallerData)
{
    FIXME("(%s %s %s %p %p): stub\n", debugstr_a(FileName), debugstr_a(SymbolPath),
            debugstr_a(DebugFilePath), Callback, CallerData);
    return NULL;
}

/******************************************************************
 *		FindExecutableImageExW (DBGHELP.@)
 *
 */
HANDLE WINAPI FindExecutableImageExW(PCWSTR FileName, PCWSTR SymbolPath, PWSTR ImageFilePath,
                                     PFIND_EXE_FILE_CALLBACKW Callback, PVOID user)
{
    HANDLE h;

    if (Callback) FIXME("Unsupported callback yet\n");
    if (!SearchPathW(SymbolPath, FileName, NULL, MAX_PATH, ImageFilePath, NULL))
        return NULL;
    h = CreateFileW(ImageFilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    return (h == INVALID_HANDLE_VALUE) ? NULL : h;
}

/******************************************************************
 *		FindExecutableImageEx (DBGHELP.@)
 *
 */
HANDLE WINAPI FindExecutableImageEx(PCSTR FileName, PCSTR SymbolPath, PSTR ImageFilePath,
                                    PFIND_EXE_FILE_CALLBACK Callback, PVOID user)
{
    HANDLE h;

    if (Callback) FIXME("Unsupported callback yet\n");
    if (!SearchPathA(SymbolPath, FileName, NULL, MAX_PATH, ImageFilePath, NULL))
        return NULL;
    h = CreateFileA(ImageFilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    return (h == INVALID_HANDLE_VALUE) ? NULL : h;
}

/******************************************************************
 *		FindExecutableImage (DBGHELP.@)
 *
 */
HANDLE WINAPI FindExecutableImage(PCSTR FileName, PCSTR SymbolPath, PSTR ImageFilePath)
{
    return FindExecutableImageEx(FileName, SymbolPath, ImageFilePath, NULL, NULL);
}

/***********************************************************************
 *           MakeSureDirectoryPathExists (DBGHELP.@)
 */
BOOL WINAPI MakeSureDirectoryPathExists(PCSTR DirPath)
{
    char path[MAX_PATH];
    const char *p = DirPath;
    int  n;

    if (p[0] && p[1] == ':') p += 2;
    while (*p == '\\') p++; /* skip drive root */
    while ((p = strchr(p, '\\')) != NULL)
    {
       n = p - DirPath + 1;
       memcpy(path, DirPath, n);
       path[n] = '\0';
       if( !CreateDirectoryA(path, NULL)            &&
           (GetLastError() != ERROR_ALREADY_EXISTS))
           return FALSE;
       p++;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS)
       SetLastError(ERROR_SUCCESS);

    return TRUE;
}

/******************************************************************
 *		SymMatchFileNameW (DBGHELP.@)
 *
 */
BOOL WINAPI SymMatchFileNameW(PCWSTR file, PCWSTR match,
                              PWSTR* filestop, PWSTR* matchstop)
{
    PCWSTR fptr;
    PCWSTR mptr;

    TRACE("(%s %s %p %p)\n",
          debugstr_w(file), debugstr_w(match), filestop, matchstop);

    fptr = file + lstrlenW(file) - 1;
    mptr = match + lstrlenW(match) - 1;

    while (fptr >= file && mptr >= match)
    {
        if (towupper(*fptr) != towupper(*mptr) && !(is_sep(*fptr) && is_sep(*mptr)))
            break;
        fptr--; mptr--;
    }
    if (filestop) *filestop = (PWSTR)fptr;
    if (matchstop) *matchstop = (PWSTR)mptr;

    return mptr == match - 1;
}

/******************************************************************
 *		SymMatchFileName (DBGHELP.@)
 *
 */
BOOL WINAPI SymMatchFileName(PCSTR file, PCSTR match,
                             PSTR* filestop, PSTR* matchstop)
{
    PCSTR fptr;
    PCSTR mptr;

    TRACE("(%s %s %p %p)\n", debugstr_a(file), debugstr_a(match), filestop, matchstop);

    fptr = file + strlen(file) - 1;
    mptr = match + strlen(match) - 1;

    while (fptr >= file && mptr >= match)
    {
        if (toupper(*fptr) != toupper(*mptr) && !(is_sepA(*fptr) && is_sepA(*mptr)))
            break;
        fptr--; mptr--;
    }
    if (filestop) *filestop = (PSTR)fptr;
    if (matchstop) *matchstop = (PSTR)mptr;

    return mptr == match - 1;
}

static BOOL do_searchW(PCWSTR file, PWSTR buffer, BOOL recurse,
                       PENUMDIRTREE_CALLBACKW cb, PVOID user)
{
    HANDLE              h;
    WIN32_FIND_DATAW    fd;
    unsigned            pos;
    BOOL                found = FALSE;

    pos = lstrlenW(buffer);
    if (pos == 0) return FALSE;
    if (buffer[pos - 1] != '\\') buffer[pos++] = '\\';
    lstrcpyW(buffer + pos, L"*.*");
    if ((h = FindFirstFileW(buffer, &fd)) == INVALID_HANDLE_VALUE)
        return FALSE;
    /* doc doesn't specify how the tree is enumerated...
     * doing a depth first based on, but may be wrong
     */
    do
    {
        if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L"..")) continue;

        lstrcpyW(buffer + pos, fd.cFileName);
        if (recurse && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            found = do_searchW(file, buffer, TRUE, cb, user);
        else if (SymMatchFileNameW(buffer, file, NULL, NULL))
        {
            if (!cb || cb(buffer, user)) found = TRUE;
        }
    } while (!found && FindNextFileW(h, &fd));
    if (!found) buffer[--pos] = '\0';
    FindClose(h);

    return found;
}

/***********************************************************************
 *           SearchTreeForFileW (DBGHELP.@)
 */
BOOL WINAPI SearchTreeForFileW(PCWSTR root, PCWSTR file, PWSTR buffer)
{
    TRACE("(%s, %s, %p)\n",
          debugstr_w(root), debugstr_w(file), buffer);
    lstrcpyW(buffer, root);
    return do_searchW(file, buffer, TRUE, NULL, NULL);
}

/***********************************************************************
 *           SearchTreeForFile (DBGHELP.@)
 */
BOOL WINAPI SearchTreeForFile(PCSTR root, PCSTR file, PSTR buffer)
{
    WCHAR       rootW[MAX_PATH];
    WCHAR       fileW[MAX_PATH];
    WCHAR       bufferW[MAX_PATH];
    BOOL        ret;

    MultiByteToWideChar(CP_ACP, 0, root, -1, rootW, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, file, -1, fileW, MAX_PATH);
    ret = SearchTreeForFileW(rootW, fileW, bufferW);
    if (ret)
        WideCharToMultiByte(CP_ACP, 0, bufferW, -1, buffer, MAX_PATH, NULL, NULL);
    return ret;
}

/******************************************************************
 *		EnumDirTreeW (DBGHELP.@)
 *
 *
 */
BOOL WINAPI EnumDirTreeW(HANDLE hProcess, PCWSTR root, PCWSTR file,
                        PWSTR buffer, PENUMDIRTREE_CALLBACKW cb, PVOID user)
{
    TRACE("(%p %s %s %p %p %p)\n",
          hProcess, debugstr_w(root), debugstr_w(file), buffer, cb, user);

    lstrcpyW(buffer, root);
    return do_searchW(file, buffer, TRUE, cb, user);
}

/******************************************************************
 *		EnumDirTree (DBGHELP.@)
 *
 *
 */
struct enum_dir_treeWA
{
    PENUMDIRTREE_CALLBACK       cb;
    void*                       user;
    char                        name[MAX_PATH];
};

static BOOL CALLBACK enum_dir_treeWA(PCWSTR name, PVOID user)
{
    struct enum_dir_treeWA*     edt = user;

    WideCharToMultiByte(CP_ACP, 0, name, -1, edt->name, MAX_PATH, NULL, NULL);
    return edt->cb(edt->name, edt->user);
}

BOOL WINAPI EnumDirTree(HANDLE hProcess, PCSTR root, PCSTR file,
                        PSTR buffer, PENUMDIRTREE_CALLBACK cb, PVOID user)
{
    WCHAR                       rootW[MAX_PATH];
    WCHAR                       fileW[MAX_PATH];
    WCHAR                       bufferW[MAX_PATH];
    struct enum_dir_treeWA      edt;
    BOOL                        ret;

    edt.cb = cb;
    edt.user = user;
    MultiByteToWideChar(CP_ACP, 0, root, -1, rootW, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, file, -1, fileW, MAX_PATH);
    if ((ret = EnumDirTreeW(hProcess, rootW, fileW, bufferW, enum_dir_treeWA, &edt)))
        WideCharToMultiByte(CP_ACP, 0, bufferW, -1, buffer, MAX_PATH, NULL, NULL);
    return ret;
}

struct sffip
{
    PFINDFILEINPATHCALLBACKW    cb;
    void*                       user;
};

/* checks that buffer (as found by matching the name) matches the info
 * (information is based on file type)
 * returns TRUE when file is found, FALSE to continue searching
 * (NB this is the opposite convention of SymFindFileInPathProc)
 */
static BOOL CALLBACK sffip_cb(PCWSTR buffer, PVOID user)
{
    struct sffip*       s = user;

    if (!s->cb) return TRUE;
    /* yes, EnumDirTree/do_search and SymFindFileInPath callbacks use the opposite
     * convention to stop/continue enumeration. sigh.
     */
    return !(s->cb)(buffer, s->user);
}

/******************************************************************
 *		SymFindFileInPathW (DBGHELP.@)
 *
 */
BOOL WINAPI SymFindFileInPathW(HANDLE hProcess, PCWSTR searchPath, PCWSTR full_path,
                               PVOID id, DWORD two, DWORD three, DWORD flags,
                               PWSTR buffer, PFINDFILEINPATHCALLBACKW cb,
                               PVOID user)
{
    struct sffip        s;
    struct process*     pcs = process_find_by_handle(hProcess);
    WCHAR               tmp[MAX_PATH];
    WCHAR*              ptr;
    const WCHAR*        filename;

    TRACE("(hProcess = %p, searchPath = %s, full_path = %s, id = %p, two = 0x%08lx, three = 0x%08lx, flags = 0x%08lx, buffer = %p, cb = %p, user = %p)\n",
          hProcess, debugstr_w(searchPath), debugstr_w(full_path),
          id, two, three, flags, buffer, cb, user);

    if (!pcs) return FALSE;
    if (!searchPath) searchPath = pcs->search_path;

    s.cb = cb;
    s.user = user;

    filename = file_name(full_path);

    while (searchPath)
    {
        ptr = wcschr(searchPath, ';');
        if (ptr)
        {
            memcpy(tmp, searchPath, (ptr - searchPath) * sizeof(WCHAR));
            tmp[ptr - searchPath] = 0;
            searchPath = ptr + 1;
        }
        else
        {
            lstrcpyW(tmp, searchPath);
            searchPath = NULL;
        }
        if (do_searchW(filename, tmp, FALSE, sffip_cb, &s))
        {
            lstrcpyW(buffer, tmp);
            return TRUE;
        }
    }
    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}

/******************************************************************
 *		SymFindFileInPath (DBGHELP.@)
 *
 */
BOOL WINAPI SymFindFileInPath(HANDLE hProcess, PCSTR searchPath, PCSTR full_path,
                              PVOID id, DWORD two, DWORD three, DWORD flags,
                              PSTR buffer, PFINDFILEINPATHCALLBACK cb,
                              PVOID user)
{
    WCHAR                       searchPathW[MAX_PATH];
    WCHAR                       full_pathW[MAX_PATH];
    WCHAR                       bufferW[MAX_PATH];
    struct enum_dir_treeWA      edt;
    BOOL                        ret;

    /* a PFINDFILEINPATHCALLBACK and a PENUMDIRTREE_CALLBACK have actually the
     * same signature & semantics, hence we can reuse the EnumDirTree W->A
     * conversion helper
     */
    edt.cb = cb;
    edt.user = user;
    if (searchPath)
        MultiByteToWideChar(CP_ACP, 0, searchPath, -1, searchPathW, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, full_path, -1, full_pathW, MAX_PATH);
    if ((ret =  SymFindFileInPathW(hProcess, searchPath ? searchPathW : NULL, full_pathW,
                                   id, two, three, flags,
                                   bufferW, enum_dir_treeWA, &edt)))
        WideCharToMultiByte(CP_ACP, 0, bufferW, -1, buffer, MAX_PATH, NULL, NULL);
    return ret;
}

struct module_find
{
    BOOL                        is_pdb;
    /* pdb: guid        PDB guid (if DS PDB file)
     *      or dw1      PDB timestamp (if JG PDB file)
     *      dw2         PDB age
     * dbg: dw1         DWORD:timestamp
     *      dw2         size of image (from PE header)
     */
    const GUID*                 guid;
    DWORD                       dw1;
    DWORD                       dw2;
    SYMSRV_INDEX_INFOW         *info;
    WCHAR                      *buffer; /* MAX_PATH + 1 */
    unsigned                    matched;
};

/* checks that buffer (as found by matching the name) matches the info
 * (information is based on file type)
 * returns TRUE when file is found, FALSE to continue searching
 * (NB this is the opposite convention of SymFindFileInPathProc)
 */
static BOOL CALLBACK module_find_cb(PCWSTR buffer, PVOID user)
{
    struct module_find* mf = user;
    unsigned            matched = 0;
    SYMSRV_INDEX_INFOW  info;

    info.sizeofstruct = sizeof(info);
    if (!SymSrvGetFileIndexInfoW(buffer, &info, 0))
        return FALSE;
    matched++;
    if (!memcmp(&info.guid, mf->guid, sizeof(GUID))) matched++;
    if (info.timestamp == mf->dw1) matched++;
    if (info.age == mf->dw2) matched++;

    if (matched > mf->matched)
    {
        size_t len = min(wcslen(buffer), MAX_PATH);
        memcpy(mf->buffer, buffer, len * sizeof(WCHAR));
        mf->buffer[len] = L'\0';
        mf->matched = matched;
        mf->info->guid = info.guid;
        mf->info->timestamp = info.timestamp;
        mf->info->age = info.age;
        mf->info->sig = info.sig;
    }
    /* yes, EnumDirTree/do_search and SymFindFileInPath callbacks use the opposite
     * convention to stop/continue enumeration. sigh.
     */
    return mf->matched == 4;
}

BOOL path_find_symbol_file(const struct process* pcs, const struct module* module,
                           PCSTR full_path, BOOL is_pdb, const GUID* guid, DWORD dw1, DWORD dw2,
                           SYMSRV_INDEX_INFOW *info, BOOL* is_unmatched)
{
    struct module_find  mf;
    WCHAR              *ptr, *ext;
    const WCHAR*        filename;
    WCHAR              *searchPath = pcs->search_path;
    WCHAR               buffer[MAX_PATH];

    TRACE("(pcs = %p, full_path = %s, guid = %s, dw1 = 0x%08lx, dw2 = 0x%08lx)\n",
          pcs, debugstr_a(full_path), debugstr_guid(guid), dw1, dw2);

    mf.info = info;
    mf.guid = guid;
    mf.dw1 = dw1;
    mf.dw2 = dw2;
    mf.matched = 0;
    mf.buffer = is_pdb ? info->pdbfile : info->dbgfile;

    MultiByteToWideChar(CP_ACP, 0, full_path, -1, info->file, MAX_PATH);
    filename = file_name(info->file);
    mf.is_pdb = is_pdb;
    *is_unmatched = FALSE;

    /* first check full path to file */
    if (is_pdb && module_find_cb(info->file, &mf))
    {
        wcscpy( info->pdbfile, info->file );
        return TRUE;
    }

    /* FIXME: Use Environment-Variables (see MS docs)
                 _NT_SYMBOL_PATH and _NT_ALT_SYMBOL_PATH
    */

    ext = wcsrchr(module->module.LoadedImageName, L'.');
    while (searchPath)
    {
        size_t len;

        ptr = wcschr(searchPath, ';');
        len = (ptr) ? ptr - searchPath : wcslen(searchPath);

        if (len + 1 < ARRAY_SIZE(buffer))
        {
            memcpy(buffer, searchPath, len * sizeof(WCHAR));
            buffer[len] = L'\0';
            /* return first fully matched file */
            if (do_searchW(filename, buffer, FALSE, module_find_cb, &mf)) return TRUE;
            len = wcslen(buffer); /* do_searchW removes the trailing \ in buffer when present */
            /* check once max size for \symbols\<ext>\ */
            if (ext && len + 9 /* \symbols\ */ + wcslen(ext + 1) + 1 + 1 <= ARRAY_SIZE(buffer))
            {
                buffer[len++] = L'\\';
                wcscpy(buffer + len, ext + 1);
                wcscat(buffer + len, L"\\");
                if (do_searchW(filename, buffer, FALSE, module_find_cb, &mf)) return TRUE;
                wcscpy(buffer + len, L"symbols\\");
                wcscat(buffer + len, ext + 1);
                wcscat(buffer + len, L"\\");
                if (do_searchW(filename, buffer, FALSE, module_find_cb, &mf)) return TRUE;
            }
        }
        else
            ERR("Too long search element %ls\n", searchPath);
        searchPath = ptr ? ptr + 1 : NULL;
    }

    /* check module-path */
    if (module->real_path)
    {
        file_pathW(module->real_path, buffer);
        if (do_searchW(filename, buffer, FALSE, module_find_cb, &mf)) return TRUE;
    }
    file_pathW(module->module.LoadedImageName, buffer);
    if (do_searchW(filename, buffer, FALSE, module_find_cb, &mf)) return TRUE;

    /* if no fully matching file is found, return the best matching file if any */
    if ((dbghelp_options & SYMOPT_LOAD_ANYTHING) && mf.matched)
    {
        *is_unmatched = TRUE;
        return TRUE;
    }
    mf.buffer[0] = L'\0';
    return FALSE;
}

WCHAR *get_dos_file_name(const WCHAR *filename)
{
    WCHAR *dos_path;
    size_t len;

    if (*filename == '/')
    {
        char *unix_path;
        len = WideCharToMultiByte(CP_UNIXCP, 0, filename, -1, NULL, 0, NULL, NULL);
        unix_path = heap_alloc(len * sizeof(WCHAR));
        WideCharToMultiByte(CP_UNIXCP, 0, filename, -1, unix_path, len, NULL, NULL);
        dos_path = wine_get_dos_file_name(unix_path);
        heap_free(unix_path);
    }
    else
    {
        len = lstrlenW(filename);
        dos_path = heap_alloc((len + 1) * sizeof(WCHAR));
        memcpy(dos_path, filename, (len + 1) * sizeof(WCHAR));
    }
    return dos_path;
}

static inline const WCHAR* get_machine_dir(const struct machine_dir *machine_dir, const WCHAR *name)
{
    WCHAR *ptr;
    if ((ptr = wcsrchr(name, L'.')) && !lstrcmpW(ptr, L".so"))
        return machine_dir->so_dir;
    return machine_dir->pe_dir;
}

static BOOL try_match_file(const WCHAR *name, BOOL (*match)(void*, HANDLE, const WCHAR*), void *param)
{
    HANDLE file = CreateFileW(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file != INVALID_HANDLE_VALUE)
    {
        BOOL ret = match(param, file, name);
        CloseHandle(file);
        return ret;
    }
    return FALSE;
}

BOOL search_dll_path(const struct process *process, const WCHAR *name, WORD machine, BOOL (*match)(void*, HANDLE, const WCHAR*), void *param)
{
    const WCHAR *env;
    WCHAR *p, *end;
    size_t len, i, machine_dir_len;
    WCHAR *buf;
    const struct cpu* cpu;
    const struct machine_dir* machine_dir;

    name = file_name(name);

    cpu = machine == IMAGE_FILE_MACHINE_UNKNOWN ? process_get_cpu(process) : cpu_find(machine);

    for (machine_dir = all_machine_dir; machine_dir < all_machine_dir + ARRAY_SIZE(all_machine_dir); machine_dir++)
        if (machine_dir->machine == cpu->machine) break;
    if (machine_dir >= all_machine_dir + ARRAY_SIZE(all_machine_dir)) return FALSE;
    machine_dir_len = max(wcslen(machine_dir->pe_dir), wcslen(machine_dir->so_dir));

    if ((env = process_getenv(process, L"WINEBUILDDIR")))
    {
        len = lstrlenW(env);
        if (!(buf = heap_alloc((len + wcslen(L"\\programs\\") + machine_dir_len +
                                2 * lstrlenW(name) + 1) * sizeof(WCHAR)))) return FALSE;
        wcscpy(buf, env);
        end = buf + len;

        wcscpy(end, L"\\dlls\\");
        wcscat(end, name);
        if ((p = wcsrchr(end, '.')) && !lstrcmpW(p, L".so")) *p = 0;
        if ((p = wcsrchr(end, '.')) && !lstrcmpW(p, L".dll")) *p = 0;
        p = end + lstrlenW(end);
        /* try multi-arch first */
        wcscpy(p, get_machine_dir(machine_dir, name));
        wcscpy(p + wcslen(p), name);
        if (try_match_file(buf, match, param)) goto found;
        /* then old mono-arch */
        *p++ = '\\';
        lstrcpyW(p, name);
        if (try_match_file(buf, match, param)) goto found;

        wcscpy(end, L"\\programs\\");
        end += wcslen(end);
        wcscpy(end, name);
        if ((p = wcsrchr(end, '.')) && !lstrcmpW(p, L".so")) *p = 0;
        if ((p = wcsrchr(end, '.')) && !lstrcmpW(p, L".exe")) *p = 0;
        p = end + lstrlenW(end);
        /* try multi-arch first */
        wcscpy(p, get_machine_dir(machine_dir, name));
        wcscpy(p + wcslen(p), name);
        if (try_match_file(buf, match, param)) goto found;
        /* then old mono-arch */
        *p++ = '\\';
        lstrcpyW(p, name);
        if (try_match_file(buf, match, param)) goto found;

        heap_free(buf);
    }

    for (i = 0;; i++)
    {
        WCHAR env_name[64];
        swprintf(env_name, ARRAY_SIZE(env_name), L"WINEDLLDIR%u", i);
        if (!(env = process_getenv(process, env_name))) return FALSE;
        len = wcslen(env) + machine_dir_len + wcslen(name) + 1;
        if (!(buf = heap_alloc(len * sizeof(WCHAR)))) return FALSE;
        swprintf(buf, len, L"%s%s%s", env, get_machine_dir(machine_dir, name), name);
        if (try_match_file(buf, match, param)) goto found;
        swprintf(buf, len, L"%s\\%s", env, name);
        if (try_match_file(buf, match, param)) goto found;
        heap_free(buf);
    }

    return FALSE;

found:
    TRACE("found %s\n", debugstr_w(buf));
    heap_free(buf);
    return TRUE;
}

BOOL search_unix_path(const WCHAR *name, const WCHAR *path, BOOL (*match)(void*, HANDLE, const WCHAR*), void *param)
{
    const WCHAR *iter, *next;
    size_t size, len;
    WCHAR *dos_path;
    char *buf;
    BOOL ret = FALSE;

    if (!path) return FALSE;
    name = file_name(name);

    size = WideCharToMultiByte(CP_UNIXCP, 0, name, -1, NULL, 0, NULL, NULL)
        + WideCharToMultiByte(CP_UNIXCP, 0, path, -1, NULL, 0, NULL, NULL);
    if (!(buf = heap_alloc(size))) return FALSE;

    for (iter = path;; iter = next + 1)
    {
        if (!(next = wcschr(iter, ':'))) next = iter + lstrlenW(iter);
        if (*iter == '/')
        {
            len = WideCharToMultiByte(CP_UNIXCP, 0, iter, next - iter, buf, size, NULL, NULL);
            if (buf[len - 1] != '/') buf[len++] = '/';
            WideCharToMultiByte(CP_UNIXCP, 0, name, -1, buf + len, size - len, NULL, NULL);
            if ((dos_path = wine_get_dos_file_name(buf)))
            {
                ret = try_match_file(dos_path, match, param);
                if (ret) TRACE("found %s\n", debugstr_w(dos_path));
                heap_free(dos_path);
                if (ret) break;
            }
        }
        if (*next != ':') break;
    }

    heap_free(buf);
    return ret;
}

/******************************************************************
 *      SymSrvGetFileIndexInfo (DBGHELP.@)
 *
 */
BOOL WINAPI SymSrvGetFileIndexInfo(const char *file, SYMSRV_INDEX_INFO* info, DWORD flags)
{
    SYMSRV_INDEX_INFOW infoW;
    WCHAR fileW[MAX_PATH];
    BOOL ret;

    TRACE("(%s, %p, 0x%08lx)\n", debugstr_a(file), info, flags);

    if (info->sizeofstruct < sizeof(*info))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    MultiByteToWideChar(CP_ACP, 0, file, -1, fileW, ARRAY_SIZE(fileW));
    infoW.sizeofstruct = sizeof(infoW);
    ret = SymSrvGetFileIndexInfoW(fileW, &infoW, flags);
    if (ret)
    {
        WideCharToMultiByte(CP_ACP, 0, infoW.file, -1, info->file, ARRAY_SIZE(info->file), NULL, NULL);
        info->stripped = infoW.stripped;
        info->timestamp = infoW.timestamp;
        info->size = infoW.size;
        WideCharToMultiByte(CP_ACP, 0, infoW.dbgfile, -1, info->dbgfile, ARRAY_SIZE(info->dbgfile), NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, infoW.pdbfile, -1, info->pdbfile, ARRAY_SIZE(info->pdbfile), NULL, NULL);
        info->guid = infoW.guid;
        info->sig = infoW.sig;
        info->age = infoW.age;
    }
    return ret;
}

/******************************************************************
 *      SymSrvGetFileIndexInfoW (DBGHELP.@)
 *
 */
BOOL WINAPI SymSrvGetFileIndexInfoW(const WCHAR *file, SYMSRV_INDEX_INFOW* info, DWORD flags)
{
    HANDLE      hFile, hMap = NULL;
    void*       image = NULL;
    DWORD       fsize, ret;

    TRACE("(%s, %p, 0x%08lx)\n", debugstr_w(file), info, flags);

    if (info->sizeofstruct < sizeof(*info))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ((hFile = CreateFileW(file, GENERIC_READ, FILE_SHARE_READ, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE &&
        ((hMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) != NULL) &&
        ((image = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0)) != NULL))
    {
        /* must handle PE, or .dbg or .pdb files. So each helper will return:
         * - ERROR_SUCCESS: if the file format is recognized and index info filled,
         * - ERROR_BAD_FORMAT: if the file doesn't match the expected format,
         * - any other error: if the file has expected format, but internal errors
         */
        fsize = GetFileSize(hFile, NULL);
        /* try PE module first */
        ret = pe_get_file_indexinfo(image, fsize, info);
        if (ret == ERROR_BAD_FORMAT)
            ret = pdb_get_file_indexinfo(image, fsize, info);
        if (ret == ERROR_BAD_FORMAT)
            ret = dbg_get_file_indexinfo(image, fsize, info);
    }
    else ret = ERROR_FILE_NOT_FOUND;

    if (image) UnmapViewOfFile(image);
    if (hMap) CloseHandle(hMap);
    if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);

    if (ret == ERROR_SUCCESS || ret == ERROR_BAD_EXE_FORMAT) wcscpy(info->file, file_name(file)); /* overflow? */
    SetLastError(ret);
    return ret == ERROR_SUCCESS;
}

/******************************************************************
 *      SymSrvGetFileIndexes (DBGHELP.@)
 *
 */
BOOL WINAPI SymSrvGetFileIndexes(PCSTR file, GUID* guid, PDWORD pdw1, PDWORD pdw2, DWORD flags)
{
    WCHAR fileW[MAX_PATH];

    TRACE("(%s, %p, %p, %p, 0x%08lx)\n", debugstr_a(file), guid, pdw1, pdw2, flags);

    MultiByteToWideChar(CP_ACP, 0, file, -1, fileW, ARRAY_SIZE(fileW));
    return SymSrvGetFileIndexesW(fileW, guid, pdw1, pdw2, flags);
}

/******************************************************************
 *      SymSrvGetFileIndexesW (DBGHELP.@)
 *
 */
BOOL WINAPI SymSrvGetFileIndexesW(PCWSTR file, GUID* guid, PDWORD pdw1, PDWORD pdw2, DWORD flags)
{
    FIXME("(%s, %p, %p, %p, 0x%08lx): stub!\n", debugstr_w(file), guid, pdw1, pdw2, flags);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
